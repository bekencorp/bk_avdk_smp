#include "lv_camera_blend.h"

#include <os/mem.h>
#include <os/os.h>
#include <components/bk_frame_buffer.h>
#include <modules/vg_lite_gpu/vg_lite.h>

#define TAG "lv_cam_blend"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define LV_CAMERA_BLEND_INVALID_FORMAT ((vg_lite_buffer_format_t)-1)
#define LV_CAMERA_BLEND_ASYNC_QUEUE_DEPTH_DEFAULT       4
#define LV_CAMERA_BLEND_ASYNC_THREAD_STACK_DEFAULT      (1024 * 6)
#define LV_CAMERA_BLEND_ASYNC_THREAD_PRIORITY_DEFAULT   BEKEN_DEFAULT_WORKER_PRIORITY

typedef struct {
    void *buffer;
    uint32_t size;
    bool ready;
} lv_camera_blend_bg_frame_t;

struct lv_camera_blend_ctx {
    lv_camera_blend_config_t config;
    beken_mutex_t lock;
    lv_camera_blend_bg_frame_t bg;
    uint32_t bg_sequence;
};

typedef enum {
    LV_CAMERA_BLEND_ASYNC_MSG_STOP = 0,
    LV_CAMERA_BLEND_ASYNC_MSG_CAMERA_FRAME,
    LV_CAMERA_BLEND_ASYNC_MSG_REFRESH,
} lv_camera_blend_async_msg_type_t;

typedef struct {
    lv_camera_blend_async_msg_type_t type;
    lv_camera_blend_camera_frame_t camera;
    uint32_t frame_size;
    lv_camera_blend_free_cb_t free_cb;
    bool refresh;
} lv_camera_blend_async_msg_t;

struct lv_camera_blend_async_ctx {
    bool active;
    bool stopping;
    lv_camera_blend_async_config_t config;
    lv_camera_blend_handle_t blend;
    beken_queue_t queue;
    beken_thread_t thread;
    beken_semaphore_t thread_exit_sem;
    beken_mutex_t refresh_mutex;
    void *cached_camera_frame;
    uint32_t cached_camera_frame_size;
    uint32_t cached_camera_frame_capacity;
    lv_camera_blend_camera_frame_t cached_camera;
    bool cached_camera_ready;
    bool refresh_pending;
};

static vg_lite_buffer_format_t lv_camera_blend_format_convert(bk_pixel_format_t format, bool bgra_as_bgrx)
{
    switch (format) {
        case BK_PIXEL_FORMAT_RGB565:
            return VG_LITE_BGR565;
        case BK_PIXEL_FORMAT_BGR565:
            return VG_LITE_RGB565;
        case BK_PIXEL_FORMAT_RGB888:
            return VG_LITE_RGB888;
        case BK_PIXEL_FORMAT_BGR888:
            return VG_LITE_BGR888;
        case BK_PIXEL_FORMAT_BGRA8888:
            return bgra_as_bgrx ? VG_LITE_BGRX8888 : VG_LITE_BGRA8888;
        case BK_PIXEL_FORMAT_ARGB8888:
        case BK_PIXEL_FORMAT_ABGR8888:
        case BK_PIXEL_FORMAT_RGBA8888:
            return VG_LITE_BGRA8888;
        case BK_PIXEL_FORMAT_NV12:
            return VG_LITE_NV12;
        default:
            return LV_CAMERA_BLEND_INVALID_FORMAT;
    }
}

static bool lv_camera_blend_format_is_valid(bk_pixel_format_t format, bool bgra_as_bgrx)
{
    return lv_camera_blend_format_convert(format, bgra_as_bgrx) != LV_CAMERA_BLEND_INVALID_FORMAT;
}

static uint32_t lv_camera_blend_frame_size_get(uint16_t width,
                                               uint16_t height,
                                               bk_pixel_format_t format,
                                               bool compress)
{
    uint32_t pixel_size = bk_pixel_size_get(format);

    if (width == 0 || height == 0 || pixel_size == 0) {
        return 0;
    }

    if (compress) {
        if ((width % 4) != 0) {
            return 0;
        }

        return pixel_size * ((uint32_t)width / 4) * (uint32_t)height;
    }

    return bk_image_size_get(width, height, format);
}

static void *lv_camera_blend_uv_plane_get(const lv_camera_blend_camera_frame_t *camera)
{
    if (camera == NULL || camera->buffer == NULL) {
        return NULL;
    }

    if (camera->src_format == BK_PIXEL_FORMAT_NV12) {
        return (uint8_t *)camera->buffer +
               ((uint32_t)camera->src_width * (uint32_t)camera->src_height);
    }

    return NULL;
}

static bool lv_camera_blend_rotate_is_valid(uint16_t rotate_degree)
{
    return rotate_degree == 0 || rotate_degree == 90 ||
           rotate_degree == 180 || rotate_degree == 270;
}

static void lv_camera_blend_set_buffer(vg_lite_buffer_t *buffer,
                                       uint16_t width,
                                       uint16_t height,
                                       bk_pixel_format_t format,
                                       bool compress,
                                       bool bgra_as_bgrx)
{
    os_memset(buffer, 0, sizeof(vg_lite_buffer_t));
    buffer->width = width;
    buffer->height = height;
    buffer->format = lv_camera_blend_format_convert(format, bgra_as_bgrx);

    if (compress) {
        buffer->tiled = VG_LITE_TILED;
        buffer->compress_mode = VG_LITE_DEC_HV_SAMPLE;
    } else {
        buffer->tiled = VG_LITE_LINEAR;
        buffer->compress_mode = VG_LITE_DEC_DISABLE;
    }
}

static bk_err_t lv_camera_blend_config_validate(const lv_camera_blend_config_t *config)
{
    if (config == NULL || config->width == 0 || config->height == 0) {
        return BK_ERR_PARAM;
    }

    if (!lv_camera_blend_format_is_valid(config->lvgl_format, false) ||
        !lv_camera_blend_format_is_valid(config->output_format, false)) {
        return BK_ERR_PARAM;
    }

    if (lv_camera_blend_frame_size_get(config->width,
                                       config->height,
                                       config->lvgl_format,
                                       config->lvgl_compress) == 0) {
        return BK_ERR_PARAM;
    }

    return BK_OK;
}

static bk_err_t lv_camera_blend_camera_validate(const lv_camera_blend_camera_frame_t *camera)
{
    if (camera == NULL || camera->buffer == NULL ||
        camera->src_width == 0 || camera->src_height == 0) {
        return BK_ERR_PARAM;
    }

    if (!lv_camera_blend_rotate_is_valid(camera->rotate_degree) ||
        !lv_camera_blend_format_is_valid(camera->src_format, true)) {
        return BK_ERR_PARAM;
    }

    return BK_OK;
}

static void lv_camera_blend_set_camera_matrix(const lv_camera_blend_camera_frame_t *camera,
                                              vg_lite_matrix_t *matrix)
{
    vg_lite_identity(matrix);

    switch (camera->rotate_degree) {
        case 90:
            vg_lite_rotate(90.0f, matrix);
            matrix->m[0][2] = (vg_lite_float_t)camera->dst_x +
                              (vg_lite_float_t)camera->src_height;
            matrix->m[1][2] = (vg_lite_float_t)camera->dst_y;
            break;
        case 180:
            vg_lite_rotate(180.0f, matrix);
            matrix->m[0][2] = (vg_lite_float_t)camera->dst_x +
                              (vg_lite_float_t)camera->src_width;
            matrix->m[1][2] = (vg_lite_float_t)camera->dst_y +
                              (vg_lite_float_t)camera->src_height;
            break;
        case 270:
            vg_lite_rotate(270.0f, matrix);
            matrix->m[0][2] = (vg_lite_float_t)camera->dst_x;
            matrix->m[1][2] = (vg_lite_float_t)camera->dst_y +
                              (vg_lite_float_t)camera->src_width;
            break;
        case 0:
        default:
            vg_lite_translate(camera->dst_x, camera->dst_y, matrix);
            break;
    }
}

bk_err_t lv_camera_blend_init(lv_camera_blend_handle_t *handle, const lv_camera_blend_config_t *config)
{
    if (handle == NULL) {
        return BK_ERR_PARAM;
    }

    *handle = NULL;

    if (lv_camera_blend_config_validate(config) != BK_OK) {
        return BK_ERR_PARAM;
    }

    lv_camera_blend_handle_t ctx = os_malloc(sizeof(struct lv_camera_blend_ctx));
    if (ctx == NULL) {
        return BK_ERR_NO_MEM;
    }

    os_memset(ctx, 0, sizeof(struct lv_camera_blend_ctx));
    os_memcpy(&ctx->config, config, sizeof(lv_camera_blend_config_t));
    ctx->bg.size = lv_camera_blend_frame_size_get(config->width,
                                                  config->height,
                                                  config->lvgl_format,
                                                  config->lvgl_compress);
    ctx->bg.buffer = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, ctx->bg.size);
    if (ctx->bg.buffer == NULL) {
        os_free(ctx);
        return BK_ERR_NO_MEM;
    }

    bk_err_t ret = rtos_init_mutex(&ctx->lock);
    if (ret != BK_OK) {
        bk_frame_buffer_free(ctx->bg.buffer);
        os_free(ctx);
        return ret;
    }

    *handle = ctx;
    return BK_OK;
}

void lv_camera_blend_deinit(lv_camera_blend_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

    rtos_lock_mutex(&handle->lock);
    if (handle->bg.buffer != NULL) {
        bk_frame_buffer_free(handle->bg.buffer);
        handle->bg.buffer = NULL;
        handle->bg.ready = false;
    }
    rtos_unlock_mutex(&handle->lock);

    rtos_deinit_mutex(&handle->lock);
    os_free(handle);
}

static bk_err_t lv_camera_blend_copy_lvgl_bg(lv_camera_blend_handle_t handle, void *frame_buffer)
{
    vg_lite_buffer_t src;
    vg_lite_buffer_t dst;
    vg_lite_matrix_t matrix;
    vg_lite_error_t ret = VG_LITE_SUCCESS;

    lv_camera_blend_set_buffer(&src,
                               handle->config.width,
                               handle->config.height,
                               handle->config.lvgl_format,
                               handle->config.lvgl_compress,
                               false);
    lv_camera_blend_set_buffer(&dst,
                               handle->config.width,
                               handle->config.height,
                               handle->config.lvgl_format,
                               handle->config.lvgl_compress,
                               false);
    if (src.format == LV_CAMERA_BLEND_INVALID_FORMAT ||
        dst.format == LV_CAMERA_BLEND_INVALID_FORMAT) {
        return BK_ERR_PARAM;
    }

    rtos_lock_mutex(&handle->lock);

    vg_lite_allocate_with_data(&src, frame_buffer, NULL, NULL, NULL);
    vg_lite_allocate_with_data(&dst, handle->bg.buffer, NULL, NULL, NULL);

    vg_lite_identity(&matrix);
    ret = vg_lite_blit(&dst, &src, &matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
    if (ret == VG_LITE_SUCCESS) {
        ret = vg_lite_finish();
    }

    vg_lite_free_without_free_data(&src);
    vg_lite_free_without_free_data(&dst);

    if (ret == VG_LITE_SUCCESS) {
        handle->bg.ready = true;
        handle->bg_sequence++;
    }

    rtos_unlock_mutex(&handle->lock);

    if (ret != VG_LITE_SUCCESS) {
        LOGE("%s background copy failed, ret=%d\n", __func__, ret);
        return BK_FAIL;
    }

    return BK_OK;
}

void lv_camera_blend_lvgl_flush_cb(void *args, void *frame_buffer, int (*free_cb)(void *args))
{
    lv_camera_blend_handle_t handle = (lv_camera_blend_handle_t)args;

    if (handle == NULL || frame_buffer == NULL) {
        if (free_cb != NULL && frame_buffer != NULL) {
            free_cb(frame_buffer);
        }
        return;
    }

    if (lv_camera_blend_copy_lvgl_bg(handle, frame_buffer) != BK_OK) {
        LOGE("%s copy lvgl background failed\n", __func__);
    }

    if (free_cb != NULL) {
        free_cb(frame_buffer);
    }
}

bk_err_t lv_camera_blend_process(lv_camera_blend_handle_t handle,
                                 void *output_buffer,
                                 const lv_camera_blend_camera_frame_t *camera)
{
    if (handle == NULL || output_buffer == NULL ||
        lv_camera_blend_camera_validate(camera) != BK_OK) {
        return BK_ERR_PARAM;
    }

    vg_lite_buffer_t output;
    vg_lite_buffer_t bg;
    vg_lite_buffer_t camera_src;
    vg_lite_matrix_t matrix;
    vg_lite_rectangle_t camera_rect = {
        .x = camera->src_x,
        .y = camera->src_y,
        .width = camera->src_width,
        .height = camera->src_height,
    };

    rtos_lock_mutex(&handle->lock);

    if (!handle->bg.ready) {
        rtos_unlock_mutex(&handle->lock);
        return BK_ERR_NOT_INIT;
    }

    lv_camera_blend_set_buffer(&output,
                               handle->config.width,
                               handle->config.height,
                               handle->config.output_format,
                               handle->config.output_compress,
                               false);
    lv_camera_blend_set_buffer(&bg,
                               handle->config.width,
                               handle->config.height,
                               handle->config.lvgl_format,
                               handle->config.lvgl_compress,
                               false);
    lv_camera_blend_set_buffer(&camera_src,
                               camera->src_width,
                               camera->src_height,
                               camera->src_format,
                               camera->src_compress,
                               true);

    if (output.format == LV_CAMERA_BLEND_INVALID_FORMAT ||
        bg.format == LV_CAMERA_BLEND_INVALID_FORMAT ||
        camera_src.format == LV_CAMERA_BLEND_INVALID_FORMAT) {
        rtos_unlock_mutex(&handle->lock);
        LOGE("%s unsupported format, out=%u bg=%u cam=%u\n",
             __func__,
             (unsigned)handle->config.output_format,
             (unsigned)handle->config.lvgl_format,
             (unsigned)camera->src_format);
        return BK_ERR_PARAM;
    }

    vg_lite_allocate_with_data(&output, output_buffer, NULL, NULL, NULL);
    vg_lite_allocate_with_data(&bg, handle->bg.buffer, NULL, NULL, NULL);
    vg_lite_allocate_with_data(&camera_src,
                               camera->buffer,
                               lv_camera_blend_uv_plane_get(camera),
                               NULL,
                               NULL);

    vg_lite_identity(&matrix);
    vg_lite_error_t ret = vg_lite_blit(&output, &bg, &matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
    if (ret != VG_LITE_SUCCESS) {
        LOGE("%s background blit failed, ret=%d\n", __func__, ret);
        goto out;
    }

    lv_camera_blend_set_camera_matrix(camera, &matrix);
    ret = vg_lite_blit_rect(&output,
                            &camera_src,
                            &camera_rect,
                            &matrix,
                            camera->alpha_blend ? VG_LITE_BLEND_SRC_OVER : VG_LITE_BLEND_NONE,
                            0,
                            VG_LITE_FILTER_POINT);
    if (ret != VG_LITE_SUCCESS) {
        LOGE("%s camera blit failed, ret=%d\n", __func__, ret);
        goto out;
    }

    ret = vg_lite_finish();

out:
    vg_lite_free_without_free_data(&output);
    vg_lite_free_without_free_data(&bg);
    vg_lite_free_without_free_data(&camera_src);
    rtos_unlock_mutex(&handle->lock);

    return ret == VG_LITE_SUCCESS ? BK_OK : BK_FAIL;
}

uint32_t lv_camera_blend_get_bg_sequence(lv_camera_blend_handle_t handle)
{
    uint32_t sequence = 0;

    if (handle == NULL) {
        return 0;
    }

    rtos_lock_mutex(&handle->lock);
    sequence = handle->bg_sequence;
    rtos_unlock_mutex(&handle->lock);

    return sequence;
}

static int lv_camera_blend_async_output_free_cb(void *args)
{
    if (args != NULL) {
        bk_frame_buffer_free(args);
    }
    return BK_OK;
}

static void lv_camera_blend_async_release_camera(const lv_camera_blend_async_msg_t *msg)
{
    if (msg != NULL && msg->camera.buffer != NULL && msg->free_cb != NULL) {
        msg->free_cb(msg->camera.buffer);
    }
}

static void lv_camera_blend_async_release_cached_camera(lv_camera_blend_async_handle_t handle)
{
    if (handle->cached_camera_frame != NULL) {
        bk_frame_buffer_free(handle->cached_camera_frame);
        handle->cached_camera_frame = NULL;
    }

    handle->cached_camera_frame_size = 0;
    handle->cached_camera_frame_capacity = 0;
    os_memset(&handle->cached_camera, 0, sizeof(handle->cached_camera));
    handle->cached_camera_ready = false;
}

static bk_err_t lv_camera_blend_async_cache_camera(lv_camera_blend_async_handle_t handle,
                                                   const lv_camera_blend_camera_frame_t *camera,
                                                   uint32_t frame_size)
{
    if (handle == NULL || camera == NULL || camera->buffer == NULL || frame_size == 0) {
        return BK_ERR_PARAM;
    }

    if (handle->cached_camera_frame_capacity < frame_size) {
        void *new_frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
        if (new_frame == NULL) {
            LOGE("%s malloc failed, size=%u\n", __func__, (unsigned)frame_size);
            return BK_ERR_NO_MEM;
        }

        lv_camera_blend_async_release_cached_camera(handle);
        handle->cached_camera_frame = new_frame;
        handle->cached_camera_frame_capacity = frame_size;
    }

    os_memcpy(handle->cached_camera_frame, camera->buffer, frame_size);
    os_memcpy(&handle->cached_camera, camera, sizeof(handle->cached_camera));
    handle->cached_camera.buffer = handle->cached_camera_frame;
    handle->cached_camera_frame_size = frame_size;
    handle->cached_camera_ready = true;
    return BK_OK;
}

static void lv_camera_blend_async_clear_refresh_pending(lv_camera_blend_async_handle_t handle)
{
    if (handle == NULL || handle->refresh_mutex == NULL) {
        return;
    }

    rtos_lock_mutex(&handle->refresh_mutex);
    handle->refresh_pending = false;
    rtos_unlock_mutex(&handle->refresh_mutex);
}

static bk_err_t lv_camera_blend_async_lock(lv_camera_blend_async_handle_t handle)
{
    if (handle->config.lock_cb == NULL) {
        return BK_OK;
    }

    return handle->config.lock_cb(handle->config.lock_user_data);
}

static void lv_camera_blend_async_unlock(lv_camera_blend_async_handle_t handle)
{
    if (handle->config.unlock_cb != NULL) {
        (void)handle->config.unlock_cb(handle->config.lock_user_data);
    }
}

static bk_err_t lv_camera_blend_async_draw_overlay(lv_camera_blend_async_handle_t handle,
                                                   void *output_buffer)
{
    if (handle->config.overlay_cb == NULL) {
        return BK_OK;
    }

    vg_lite_buffer_t output;
    lv_camera_blend_set_buffer(&output,
                               handle->config.blend.width,
                               handle->config.blend.height,
                               handle->config.blend.output_format,
                               handle->config.blend.output_compress,
                               false);
    if (output.format == LV_CAMERA_BLEND_INVALID_FORMAT) {
        return BK_ERR_PARAM;
    }

    vg_lite_allocate_with_data(&output, output_buffer, NULL, NULL, NULL);
    handle->config.overlay_cb(handle->config.overlay_user_data, &output);
    vg_lite_error_t ret = vg_lite_finish();
    vg_lite_free_without_free_data(&output);

    return ret == VG_LITE_SUCCESS ? BK_OK : BK_FAIL;
}

static bk_err_t lv_camera_blend_async_process_camera(lv_camera_blend_async_handle_t handle,
                                                     const lv_camera_blend_camera_frame_t *camera)
{
    if (handle == NULL || camera == NULL || camera->buffer == NULL) {
        return BK_ERR_PARAM;
    }

    if (lv_camera_blend_get_bg_sequence(handle->blend) == 0) {
        return BK_ERR_NOT_INIT;
    }

    void *output = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                          handle->config.output_buffer_size);
    if (output == NULL) {
        LOGE("%s output malloc failed, size=%u\n",
             __func__, (unsigned)handle->config.output_buffer_size);
        return BK_ERR_NO_MEM;
    }

    bk_err_t ret = lv_camera_blend_async_lock(handle);
    if (ret != BK_OK) {
        bk_frame_buffer_free(output);
        return ret;
    }

    ret = lv_camera_blend_process(handle->blend, output, camera);
    if (ret == BK_OK) {
        ret = lv_camera_blend_async_draw_overlay(handle, output);
    }

    lv_camera_blend_async_unlock(handle);

    if (ret != BK_OK) {
        bk_frame_buffer_free(output);
        return ret;
    }

    if (handle->config.output_cb == NULL) {
        bk_frame_buffer_free(output);
        return BK_ERR_NOT_INIT;
    }

    ret = handle->config.output_cb(handle->config.output_user_data,
                                   output,
                                   lv_camera_blend_async_output_free_cb);
    if (ret != BK_OK) {
        bk_frame_buffer_free(output);
    }

    return ret;
}

static void lv_camera_blend_async_worker(void *arg)
{
    lv_camera_blend_async_handle_t handle = (lv_camera_blend_async_handle_t)arg;

    while (1) {
        lv_camera_blend_async_msg_t msg;
        os_memset(&msg, 0, sizeof(msg));

        if (rtos_pop_from_queue(&handle->queue, &msg, BEKEN_WAIT_FOREVER) != BK_OK) {
            continue;
        }

        if (msg.type == LV_CAMERA_BLEND_ASYNC_MSG_STOP) {
            break;
        }

        if (handle->active && !handle->stopping) {
            if (msg.type == LV_CAMERA_BLEND_ASYNC_MSG_CAMERA_FRAME) {
                if (handle->cached_camera_frame_capacity != 0) {
                    (void)lv_camera_blend_async_cache_camera(handle, &msg.camera, msg.frame_size);
                }
                (void)lv_camera_blend_async_process_camera(handle, &msg.camera);
            } else if (msg.type == LV_CAMERA_BLEND_ASYNC_MSG_REFRESH) {
                if (msg.refresh) {
                    lv_camera_blend_async_clear_refresh_pending(handle);
                }

                if (msg.refresh && handle->cached_camera_ready) {
                    (void)lv_camera_blend_async_process_camera(handle, &handle->cached_camera);
                }
            }
        }

        if (msg.type == LV_CAMERA_BLEND_ASYNC_MSG_CAMERA_FRAME) {
            lv_camera_blend_async_release_camera(&msg);
        }
    }

    if (handle->thread_exit_sem != NULL) {
        rtos_set_semaphore(&handle->thread_exit_sem);
    }

    rtos_delete_thread(NULL);
}

bk_err_t lv_camera_blend_async_start(lv_camera_blend_async_handle_t *handle,
                                     const lv_camera_blend_async_config_t *config)
{
    if (handle == NULL || config == NULL ||
        config->output_buffer_size == 0 ||
        config->output_cb == NULL) {
        return BK_ERR_PARAM;
    }

    *handle = NULL;

    lv_camera_blend_async_handle_t ctx = os_malloc(sizeof(struct lv_camera_blend_async_ctx));
    if (ctx == NULL) {
        return BK_ERR_NO_MEM;
    }

    os_memset(ctx, 0, sizeof(struct lv_camera_blend_async_ctx));
    os_memcpy(&ctx->config, config, sizeof(ctx->config));

    bk_err_t ret = lv_camera_blend_init(&ctx->blend, &config->blend);
    if (ret != BK_OK) {
        goto fail;
    }

    if (config->camera_frame_cache_size != 0) {
        ctx->cached_camera_frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                                          config->camera_frame_cache_size);
        if (ctx->cached_camera_frame == NULL) {
            ret = BK_ERR_NO_MEM;
            goto fail;
        }
        ctx->cached_camera_frame_capacity = config->camera_frame_cache_size;
    }

    ret = rtos_init_mutex(&ctx->refresh_mutex);
    if (ret != BK_OK) {
        goto fail;
    }

    uint32_t queue_depth = config->queue_depth != 0 ?
                           config->queue_depth :
                           LV_CAMERA_BLEND_ASYNC_QUEUE_DEPTH_DEFAULT;
    ret = rtos_init_queue(&ctx->queue,
                          "lv_cam_blend_q",
                          sizeof(lv_camera_blend_async_msg_t),
                          queue_depth);
    if (ret != BK_OK) {
        goto fail;
    }

    ret = rtos_init_semaphore_ex(&ctx->thread_exit_sem, 1, 0);
    if (ret != BK_OK) {
        goto fail;
    }

    ctx->active = true;
    ctx->stopping = false;

    uint32_t stack_size = config->thread_stack_size != 0 ?
                          config->thread_stack_size :
                          LV_CAMERA_BLEND_ASYNC_THREAD_STACK_DEFAULT;
    uint32_t priority = config->thread_priority != 0 ?
                        config->thread_priority :
                        LV_CAMERA_BLEND_ASYNC_THREAD_PRIORITY_DEFAULT;
    ret = rtos_create_thread(&ctx->thread,
                             priority,
                             "lv_cam_blend",
                             lv_camera_blend_async_worker,
                             stack_size,
                             ctx);
    if (ret != BK_OK) {
        ctx->active = false;
        goto fail;
    }

    *handle = ctx;
    return BK_OK;

fail:
    if (ctx->thread_exit_sem != NULL) {
        rtos_deinit_semaphore(&ctx->thread_exit_sem);
    }
    if (ctx->queue != NULL) {
        rtos_deinit_queue(&ctx->queue);
    }
    if (ctx->refresh_mutex != NULL) {
        rtos_deinit_mutex(&ctx->refresh_mutex);
    }
    if (ctx->blend != NULL) {
        lv_camera_blend_deinit(ctx->blend);
    }
    lv_camera_blend_async_release_cached_camera(ctx);
    os_free(ctx);
    return ret;
}

void lv_camera_blend_async_stop(lv_camera_blend_async_handle_t handle)
{
    if (handle == NULL || !handle->active) {
        return;
    }

    handle->stopping = true;
    handle->active = false;

    if (handle->queue != NULL) {
        lv_camera_blend_async_msg_t msg;
        os_memset(&msg, 0, sizeof(msg));
        msg.type = LV_CAMERA_BLEND_ASYNC_MSG_STOP;
        (void)rtos_push_to_queue(&handle->queue, &msg, BEKEN_WAIT_FOREVER);
    }

    if (handle->thread_exit_sem != NULL) {
        (void)rtos_get_semaphore(&handle->thread_exit_sem, BEKEN_WAIT_FOREVER);
    }

    if (handle->queue != NULL) {
        lv_camera_blend_async_msg_t pending;
        while (rtos_pop_from_queue(&handle->queue, &pending, BEKEN_NO_WAIT) == BK_OK) {
            if (pending.type == LV_CAMERA_BLEND_ASYNC_MSG_CAMERA_FRAME) {
                lv_camera_blend_async_release_camera(&pending);
            }
        }
        rtos_deinit_queue(&handle->queue);
    }

    if (handle->thread_exit_sem != NULL) {
        rtos_deinit_semaphore(&handle->thread_exit_sem);
    }
    if (handle->refresh_mutex != NULL) {
        rtos_deinit_mutex(&handle->refresh_mutex);
    }
    if (handle->blend != NULL) {
        lv_camera_blend_deinit(handle->blend);
    }

    lv_camera_blend_async_release_cached_camera(handle);
    os_free(handle);
}

bool lv_camera_blend_async_is_active(lv_camera_blend_async_handle_t handle)
{
    return handle != NULL && handle->active && !handle->stopping;
}

bk_err_t lv_camera_blend_async_push_camera_frame(lv_camera_blend_async_handle_t handle,
                                                 const lv_camera_blend_camera_frame_t *camera,
                                                 uint32_t frame_size,
                                                 lv_camera_blend_free_cb_t free_cb)
{
    if (camera == NULL || camera->buffer == NULL || frame_size == 0) {
        return BK_ERR_PARAM;
    }

    if (!lv_camera_blend_async_is_active(handle) || handle->queue == NULL) {
        if (free_cb != NULL) {
            free_cb(camera->buffer);
        }
        return BK_FAIL;
    }

    lv_camera_blend_async_msg_t msg;
    os_memset(&msg, 0, sizeof(msg));
    msg.type = LV_CAMERA_BLEND_ASYNC_MSG_CAMERA_FRAME;
    os_memcpy(&msg.camera, camera, sizeof(msg.camera));
    msg.frame_size = frame_size;
    msg.free_cb = free_cb;

    if (rtos_push_to_queue(&handle->queue, &msg, BEKEN_NO_WAIT) != BK_OK) {
        LOGW("%s queue full, drop camera frame\n", __func__);
        if (free_cb != NULL) {
            free_cb(camera->buffer);
        }
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t lv_camera_blend_async_update_lvgl_frame(lv_camera_blend_async_handle_t handle,
                                                 void *frame_buffer,
                                                 lv_camera_blend_free_cb_t free_cb)
{
    if (!lv_camera_blend_async_is_active(handle) ||
        handle->blend == NULL ||
        frame_buffer == NULL) {
        return BK_FAIL;
    }

    bk_err_t ret = lv_camera_blend_async_lock(handle);
    if (ret != BK_OK) {
        return ret;
    }

    uint32_t old_sequence = lv_camera_blend_get_bg_sequence(handle->blend);
    lv_camera_blend_lvgl_flush_cb(handle->blend, frame_buffer, free_cb);
    uint32_t new_sequence = lv_camera_blend_get_bg_sequence(handle->blend);
    lv_camera_blend_async_unlock(handle);

    if (new_sequence == old_sequence) {
        return BK_OK;
    }

    if (handle->cached_camera_frame_capacity == 0) {
        return BK_OK;
    }

    bool should_refresh = false;
    rtos_lock_mutex(&handle->refresh_mutex);
    if (!handle->refresh_pending) {
        handle->refresh_pending = true;
        should_refresh = true;
    }
    rtos_unlock_mutex(&handle->refresh_mutex);

    if (!should_refresh) {
        return BK_OK;
    }

    if (handle->queue == NULL || handle->stopping) {
        lv_camera_blend_async_clear_refresh_pending(handle);
        return BK_OK;
    }

    lv_camera_blend_async_msg_t msg;
    os_memset(&msg, 0, sizeof(msg));
    msg.type = LV_CAMERA_BLEND_ASYNC_MSG_REFRESH;
    msg.refresh = true;

    if (rtos_push_to_queue(&handle->queue, &msg, BEKEN_WAIT_FOREVER) != BK_OK) {
        lv_camera_blend_async_clear_refresh_pending(handle);
        return BK_OK;
    }

    return BK_OK;
}
