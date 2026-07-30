/*
 * UVC full pipeline (merged from uvc/src encode_frame_que.c + decode_test.c + uvc_test.c + pipeline_test.c).
 *
 *   UVC host camera (MJPEG) -> encode frame queue -> MJPEG hw decode (flexa NV12) -> display GPU (rotate90+compress) -> LCD
 *
 * Except uvc_pipeline_open/close/is_open, queue / decode / camera are all static in this file.
 * Display and GPU use shared display module (display.h).
 */
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <avdk_error.h>
#include <components/log.h>

#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_decode/bk_jpeg_decode_ctlr.h>
#include <components/bk_decode/bk_jpeg_decode_types.h>

#include <components/usb_types.h>
#include <components/bk_uvc_camera.h>
#include <components/usbh_hub_multiple_classes_api.h>
#include <components/bk_flexa_bond.h>

#include "display.h"
#include "uvc_pipeline.h"

#define TAG "uvc_pipe"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/* ==================== Encode frame queue (MJPEG frames from UVC queued for decode) ==================== */

#ifdef CONFIG_UVC_FRAME_SIZE
#define ENCODE_FRAME_SIZE (CONFIG_UVC_FRAME_SIZE)
#else
#define ENCODE_FRAME_SIZE (1024 * 200)
#endif
#define ENCODE_FRAME_COUNT 5

typedef struct {
    beken_queue_t ready_queue;
    beken_queue_t free_queue;
    beken_mutex_t mutex;
    uint8_t queue_count;
    uint8_t enable;
} encode_frame_queue_t;

static encode_frame_queue_t *s_encode_frame_queue = NULL;

static avdk_err_t encode_frame_que_init(void)
{
    avdk_err_t ret = AVDK_ERR_OK;
    if (s_encode_frame_queue != NULL) {
        LOGD("%s, %d, encode frame queue already initialized\n", __func__, __LINE__);
        return AVDK_ERR_OK;
    }
    s_encode_frame_queue = (encode_frame_queue_t *)os_malloc(sizeof(encode_frame_queue_t));
    if (s_encode_frame_queue == NULL) {
        LOGE("%s, %d, malloc encode frame queue failed\n", __func__, __LINE__);
        return AVDK_ERR_NOMEM;
    }
    os_memset(s_encode_frame_queue, 0, sizeof(encode_frame_queue_t));
    s_encode_frame_queue->queue_count = ENCODE_FRAME_COUNT;
    ret = rtos_init_queue(&s_encode_frame_queue->ready_queue, "encode_frame_ready_queue", sizeof(void *), ENCODE_FRAME_COUNT);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init encode frame ready queue failed\n", __func__, __LINE__);
        return ret;
    }
    ret = rtos_init_queue(&s_encode_frame_queue->free_queue, "encode_frame_free_queue", sizeof(void *), ENCODE_FRAME_COUNT);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init encode frame free queue failed\n", __func__, __LINE__);
        return ret;
    }

    ret = rtos_init_mutex(&s_encode_frame_queue->mutex);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init encode frame mutex failed\n", __func__, __LINE__);
        return ret;
    }

    for (uint8_t i = 0; i < ENCODE_FRAME_COUNT; i++) {
        frame_buffer_t *frame = (frame_buffer_t *)os_malloc(sizeof(frame_buffer_t));
        if (frame == NULL) {
            LOGE("%s, %d, malloc encode frame failed\n", __func__, __LINE__);
            return AVDK_ERR_NOMEM;
        }
        os_memset(frame, 0, sizeof(frame_buffer_t));
        uint8_t *frame_data = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, ENCODE_FRAME_SIZE);
        if (frame_data == NULL) {
            LOGE("%s, %d, malloc encode frame data failed\n", __func__, __LINE__);
            os_free(frame);
            return AVDK_ERR_NOMEM;
        }

        frame->frame = frame_data;
        frame->size = ENCODE_FRAME_SIZE;

        /*
         * Queue item size is sizeof(void *). rtos_push_to_queue() copies message_size bytes
         * from the address "message" points to, so we must pass &frame to push the pointer value.
         */
        ret = rtos_push_to_queue(&s_encode_frame_queue->free_queue, &frame, BEKEN_NO_WAIT);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d, push encode frame to free queue failed\n", __func__, __LINE__);
            return ret;
        }
    }
    s_encode_frame_queue->enable = 1;
    return ret;
}

static avdk_err_t encode_ready_frame_que_push(frame_buffer_t *frame)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    encode_frame_queue_t *q = s_encode_frame_queue;
    if (q == NULL) {
        return ret;
    }
    rtos_lock_mutex(&q->mutex);
    if (q->enable == 0) {
        rtos_unlock_mutex(&q->mutex);
        return ret;
    }
    /* Queue item size is sizeof(void *), push pointer value via &frame. */
    ret = rtos_push_to_queue(&q->ready_queue, &frame, BEKEN_NO_WAIT);
    rtos_unlock_mutex(&q->mutex);
    return ret;
}

static avdk_err_t encode_free_frame_que_push(frame_buffer_t *frame)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    encode_frame_queue_t *q = s_encode_frame_queue;
    if (q == NULL) {
        return ret;
    }
    rtos_lock_mutex(&q->mutex);
    if (q->enable == 0) {
        rtos_unlock_mutex(&q->mutex);
        return ret;
    }
    ret = rtos_push_to_queue(&q->free_queue, &frame, BEKEN_NO_WAIT);
    rtos_unlock_mutex(&q->mutex);
    return ret;
}

static frame_buffer_t *encode_ready_frame_que_pop(uint32_t timeout)
{
    frame_buffer_t *frame = NULL;
    encode_frame_queue_t *q = s_encode_frame_queue;
    if (q == NULL || q->enable == 0) {
        return NULL;
    }
    avdk_err_t ret = rtos_pop_from_queue(&q->ready_queue, &frame, timeout);
    if (ret != AVDK_ERR_OK) {
        LOGV("%s, %d, pop ready queue failed\n", __func__, __LINE__);
    }
    return frame;
}

static frame_buffer_t *encode_free_frame_que_pop(void)
{
    frame_buffer_t *frame = NULL;
    encode_frame_queue_t *q = s_encode_frame_queue;
    if (q == NULL || q->enable == 0) {
        return NULL;
    }
    rtos_lock_mutex(&q->mutex);
    avdk_err_t ret = rtos_pop_from_queue(&q->free_queue, &frame, BEKEN_NO_WAIT);
    if (ret != AVDK_ERR_OK) {
        rtos_pop_from_queue(&q->ready_queue, &frame, BEKEN_NO_WAIT);
    }
    rtos_unlock_mutex(&q->mutex);
    return frame;
}

/* ==================== MJPEG hw decode (flexa segments to NV12 ring buffer) ==================== */

#define DECODE_BUFFER_CNT       (3)
#define DECODE_FLEXA_LINES      (16)
#define DECODE_FLEXA_ALIGN_SIZE (16)

typedef struct {
    uint8_t task_running;
    uint8_t flexa_mode;
    uint8_t ring_buffer_cnt;
    bk_image_format_t input_format;
    bk_pixel_format_t output_format;
    uint16_t width;
    uint16_t height;
    uint16_t aligned_height;
    bk_jpeg_decode_ctlr_handle_t decode_handle;
    beken_semaphore_t decode_sem;
    beken_thread_t decode_thread;
    uint8_t *decode_buffer;
} decode_ctx_t;

static decode_ctx_t *s_decode_ctx = NULL;

static void *hsram_aligned_malloc(uint32_t alignment, uint32_t size)
{
    if (alignment < (uint32_t)sizeof(void *)) {
        alignment = (uint32_t)sizeof(void *);
    }
    if ((alignment & (alignment - 1U)) != 0U) {
        return NULL;
    }

    uint32_t total = size + alignment - 1U + (uint32_t)sizeof(void *);
    void *raw = hsram_malloc(total);
    if (raw == NULL) {
        return NULL;
    }

    uintptr_t start = (uintptr_t)raw + sizeof(void *);
    uintptr_t aligned = (start + (alignment - 1U)) & ~((uintptr_t)alignment - 1U);
    ((void **)aligned)[-1] = raw;
    return (void *)aligned;
}

static void hsram_aligned_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    os_free(((void **)ptr)[-1]);
}

static void decode_jpeg_frame_done_cb(int status, void *args)
{
    (void)args;
    if (status != BK_OK) {
        LOGE("%s decode failed, status=%d\n", __func__, status);
    }
}

static void decode_flexa_done_cb(uint32_t wr_cnt, void *args)
{
    (void)wr_cnt;
    (void)args;
}

static void decode_thread_entry(void *arg)
{
    decode_ctx_t *ctx = (decode_ctx_t *)arg;
    frame_buffer_t *encode_buffer = NULL;

    ctx->task_running = 1;
    rtos_set_semaphore(&ctx->decode_sem);

    while (ctx->task_running) {
        if (encode_buffer == NULL) {
            encode_buffer = encode_ready_frame_que_pop(2000);
            if (encode_buffer == NULL) {
                continue;
            }
        }

        uint32_t decode_buffer_size;
        uint8_t *decode_buffer;

        if (ctx->flexa_mode) {
            decode_buffer_size = bk_image_size_get(ctx->width,
                                                   DECODE_FLEXA_LINES * ctx->ring_buffer_cnt,
                                                   ctx->output_format);
            decode_buffer = ctx->decode_buffer;
        } else {
            decode_buffer_size = bk_image_size_get(ctx->width, ctx->aligned_height, ctx->output_format);
            decode_buffer = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, decode_buffer_size);
            if (decode_buffer == NULL) {
                LOGE("%s malloc decode buffer failed\n", __func__);
                encode_free_frame_que_push(encode_buffer);
                encode_buffer = NULL;
                continue;
            }
        }

        avdk_err_t ret = AVDK_ERR_INVAL;
        if (ctx->input_format == BK_IMAGE_FORMAT_MJPEG) {
            bk_jpeg_decode_input_t in = {0};
            in.stream = encode_buffer->frame;
            in.stream_len = encode_buffer->length;
            in.out_buffer = decode_buffer;
            in.out_buffer_size = decode_buffer_size;
            ret = bk_jpeg_decode_frame(ctx->decode_handle, &in);
        }

        if (!ctx->flexa_mode && decode_buffer != NULL) {
            bk_frame_buffer_free(decode_buffer);
        }
        if (ret != AVDK_ERR_OK) {
            LOGE("%s decode failed, ret=%d\n", __func__, ret);
        }

        encode_free_frame_que_push(encode_buffer);
        encode_buffer = NULL;
    }

    if (encode_buffer != NULL) {
        encode_free_frame_que_push(encode_buffer);
    }

    ctx->decode_thread = NULL;
    rtos_set_semaphore(&ctx->decode_sem);
    rtos_delete_thread(NULL);
}

static void decode_destroy_ctx(decode_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->decode_handle != NULL) {
        (void)bk_jpeg_decode_close(ctx->decode_handle);
        (void)bk_jpeg_decode_deinit(ctx->decode_handle);
        (void)bk_jpeg_decode_delete(ctx->decode_handle);
        ctx->decode_handle = NULL;
    }

    if (ctx->decode_sem != NULL) {
        rtos_deinit_semaphore(&ctx->decode_sem);
        ctx->decode_sem = NULL;
    }

    if (ctx->flexa_mode && ctx->decode_buffer != NULL) {
        hsram_aligned_free(ctx->decode_buffer);
        ctx->decode_buffer = NULL;
    }

    os_free(ctx);
}

static avdk_err_t decode_open(uint16_t width, uint16_t height, bk_image_format_t format, uint8_t flexa_mode)
{
    if (s_decode_ctx != NULL) {
        LOGW("%s already open\n", __func__);
        return AVDK_ERR_OK;
    }

    avdk_err_t ret = encode_frame_que_init();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s encode_frame_que_init failed\n", __func__);
        return ret;
    }

    decode_ctx_t *ctx = (decode_ctx_t *)os_malloc(sizeof(decode_ctx_t));
    if (ctx == NULL) {
        return AVDK_ERR_NOMEM;
    }
    os_memset(ctx, 0, sizeof(decode_ctx_t));

    ctx->flexa_mode = flexa_mode;
    ctx->input_format = format;
    ctx->output_format = BK_PIXEL_FORMAT_NV12;
    ctx->width = width;
    ctx->height = height;
    ctx->aligned_height = (height + DECODE_FLEXA_ALIGN_SIZE - 1) & ~(DECODE_FLEXA_ALIGN_SIZE - 1);
    ctx->ring_buffer_cnt = DECODE_BUFFER_CNT;

    if (flexa_mode) {
        uint32_t flexa_size = bk_image_size_get(width,
                                                DECODE_FLEXA_LINES * ctx->ring_buffer_cnt,
                                                ctx->output_format);
        ctx->decode_buffer = (uint8_t *)hsram_aligned_malloc(64, flexa_size);
        if (ctx->decode_buffer == NULL) {
            LOGE("%s alloc flexa buffer %u failed\n", __func__, flexa_size);
            ret = AVDK_ERR_NOMEM;
            goto out;
        }
        LOGD("%s flexa buffer %p size %u\n", __func__, ctx->decode_buffer, flexa_size);
    }

    if (format == BK_IMAGE_FORMAT_MJPEG) {
        bk_jpeg_decode_flexa_config_t cfg = DEFAULT_JPEG_DECODE_FLEXA_CONFIG;
        cfg.frame_done_cb = decode_jpeg_frame_done_cb;
        cfg.frame_done_args = NULL;
        cfg.flexa_done_cb = decode_flexa_done_cb;
        cfg.flexa_done_args = NULL;
        cfg.out_width = width;
        cfg.out_height = height;
        cfg.segment_height = (uint16_t)(DECODE_FLEXA_LINES / 16);
        cfg.segment_number = DECODE_BUFFER_CNT;

        ret = bk_jpeg_decode_flexa_ctlr_new(&ctx->decode_handle, &cfg);
        if (ret != AVDK_ERR_OK) {
            goto out;
        }
        ret = bk_jpeg_decode_init(ctx->decode_handle);
        if (ret != AVDK_ERR_OK) {
            goto out;
        }
        ret = bk_jpeg_decode_open(ctx->decode_handle);
        if (ret != AVDK_ERR_OK) {
            goto out;
        }
    } else {
        ret = AVDK_ERR_INVAL;
        goto out;
    }

    ret = rtos_init_semaphore(&ctx->decode_sem, 1);
    if (ret != AVDK_ERR_OK) {
        goto out;
    }

    ret = rtos_create_thread(&ctx->decode_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "decode_thread",
                             (beken_thread_function_t)decode_thread_entry,
                             1024 * 4,
                             ctx);
    if (ret != AVDK_ERR_OK) {
        goto out;
    }

    rtos_get_semaphore(&ctx->decode_sem, BEKEN_WAIT_FOREVER);
    s_decode_ctx = ctx;
    LOGI("%s %ux%u flexa=%u ok\n", __func__, width, height, flexa_mode);
    return AVDK_ERR_OK;

out:
    decode_destroy_ctx(ctx);
    return ret;
}

static avdk_err_t decode_close(void)
{
    decode_ctx_t *ctx = s_decode_ctx;
    if (ctx == NULL) {
        return AVDK_ERR_OK;
    }

    ctx->task_running = 0;
    rtos_get_semaphore(&ctx->decode_sem, BEKEN_WAIT_FOREVER);

    decode_destroy_ctx(ctx);
    s_decode_ctx = NULL;
    LOGI("%s done\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t decode_get_flexa_context(uint8_t **decode_buffer, uint8_t *ring_buffer_cnt)
{
    if (s_decode_ctx == NULL || decode_buffer == NULL || ring_buffer_cnt == NULL) {
        return AVDK_ERR_INVAL;
    }
    *decode_buffer = s_decode_ctx->decode_buffer;
    *ring_buffer_cnt = s_decode_ctx->ring_buffer_cnt;
    return AVDK_ERR_OK;
}

static avdk_err_t decode_get_handle(bk_jpeg_decode_ctlr_handle_t *handle)
{
    if (s_decode_ctx == NULL || handle == NULL) {
        return AVDK_ERR_INVAL;
    }
    *handle = s_decode_ctx->decode_handle;
    return AVDK_ERR_OK;
}

/* ==================== UVC host camera ==================== */

static beken_semaphore_t s_uvc_connect_sem = NULL;
static bk_uvc_ctlr_handle_t s_uvc_handle[UVC_PORT_MAX] = {NULL};

static frame_buffer_t *uvc_camera_frame_malloc(bk_image_format_t format, uint32_t size)
{
    (void)format;
    (void)size;
    return encode_free_frame_que_pop();
}

static void uvc_camera_frame_complete(uint8_t port, bk_image_format_t format, frame_buffer_t *frame, int result)
{
    (void)port;
    (void)format;
    if (result == AVDK_ERR_OK) {
        encode_ready_frame_que_push(frame);
    } else {
        encode_free_frame_que_push(frame);
    }
}

static void uvc_event_callback(uvc_state_t state, void *user_data)
{
    LOGI("%s, state:%d, user_data:%p\n", __func__, state, *((bk_uvc_ctlr_handle_t *)user_data));
}

static const bk_uvc_callback_t uvc_camera_cbs =
{
    .frame_malloc = uvc_camera_frame_malloc,
    .frame_complete = uvc_camera_frame_complete,
    .state_change_cb = uvc_event_callback,
    .user_data = &s_uvc_handle,
};

static avdk_err_t uvc_check_mjpeg_config(bk_uvc_device_brief_info_t *uvc_device_param, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;

    frame_num = uvc_device_param->all_frame.mjpeg_frame_num;
    for (index = 0; index < frame_num; index++) {
        if (uvc_device_param->all_frame.mjpeg_frame[index].width == user_config->width
            && uvc_device_param->all_frame.mjpeg_frame[index].height == user_config->height) {
            resolution_flag = true;
        }

        for (int i = 0; i < uvc_device_param->all_frame.mjpeg_frame[index].fps_num; i++) {
            if (resolution_flag
                && uvc_device_param->all_frame.mjpeg_frame[index].fps[i] == user_config->fps) {
                fps_flag = true;
            }
        }

        if (resolution_flag) {
            if (fps_flag == false) {
                user_config->fps = uvc_device_param->all_frame.mjpeg_frame[index].fps[0];
                fps_flag = true;
            }
            break;
        }
    }

    if (resolution_flag && fps_flag) {
        ret = AVDK_ERR_OK;
    }

    return ret;
}

static avdk_err_t uvc_check_yuv_config(bk_uvc_device_brief_info_t *uvc_device_param, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;

    frame_num = uvc_device_param->all_frame.yuv_frame_num;
    for (index = 0; index < frame_num; index++) {
        if (uvc_device_param->all_frame.yuv_frame[index].width == user_config->width
            && uvc_device_param->all_frame.yuv_frame[index].height == user_config->height) {
            resolution_flag = true;
        }

        for (int i = 0; i < uvc_device_param->all_frame.yuv_frame[index].fps_num; i++) {
            if (resolution_flag
                && uvc_device_param->all_frame.yuv_frame[index].fps[i] == user_config->fps) {
                fps_flag = true;
            }
        }

        if (resolution_flag) {
            if (fps_flag == false) {
                user_config->fps = uvc_device_param->all_frame.yuv_frame[index].fps[0];
                fps_flag = true;
            }
            break;
        }
    }

    if (resolution_flag && fps_flag) {
        ret = AVDK_ERR_OK;
    }

    return ret;
}

static avdk_err_t uvc_check_h264_config(bk_uvc_device_brief_info_t *uvc_device_param, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;

    frame_num = uvc_device_param->all_frame.h264_frame_num;
    for (index = 0; index < frame_num; index++) {
        if (uvc_device_param->all_frame.h264_frame[index].width == user_config->width
            && uvc_device_param->all_frame.h264_frame[index].height == user_config->height) {
            resolution_flag = true;
        }

        for (int i = 0; i < uvc_device_param->all_frame.h264_frame[index].fps_num; i++) {
            if (resolution_flag
                && uvc_device_param->all_frame.h264_frame[index].fps[i] == user_config->fps) {
                fps_flag = true;
            }
        }

        if (resolution_flag) {
            if (fps_flag == false) {
                user_config->fps = uvc_device_param->all_frame.h264_frame[index].fps[0];
                fps_flag = true;
            }
            break;
        }
    }

    if (resolution_flag && fps_flag) {
        ret = AVDK_ERR_OK;
    }

    return ret;
}

static avdk_err_t uvc_check_h265_config(bk_uvc_device_brief_info_t *uvc_device_param, bk_cam_uvc_config_t *user_config)
{
    uint8_t frame_num = 0;
    uint8_t index = 0;
    uint8_t resolution_flag = false;
    uint8_t fps_flag = false;
    avdk_err_t ret = AVDK_ERR_UNSUPPORTED;

    frame_num = uvc_device_param->all_frame.h265_frame_num;
    for (index = 0; index < frame_num; index++) {
        if (uvc_device_param->all_frame.h265_frame[index].width == user_config->width
            && uvc_device_param->all_frame.h265_frame[index].height == user_config->height) {
            resolution_flag = true;
        }

        for (int i = 0; i < uvc_device_param->all_frame.h265_frame[index].fps_num; i++) {
            if (resolution_flag
                && uvc_device_param->all_frame.h265_frame[index].fps[i] == user_config->fps) {
                fps_flag = true;
            }
        }

        if (resolution_flag) {
            if (fps_flag == false) {
                user_config->fps = uvc_device_param->all_frame.h265_frame[index].fps[0];
                fps_flag = true;
            }
            break;
        }
    }

    if (resolution_flag && fps_flag) {
        ret = AVDK_ERR_OK;
    }

    return ret;
}

static avdk_err_t uvc_checkout_port_info(bk_cam_uvc_config_t *user_config)
{
    bk_usb_hub_port_info *uvc_port_info = NULL;
    avdk_err_t ret = AVDK_ERR_OK;

    if (user_config->format == BK_IMAGE_FORMAT_H264 || user_config->format == BK_IMAGE_FORMAT_H265) {
        ret = bk_usbh_hub_port_check_device(user_config->port, USB_UVC_H26X_DEVICE, &uvc_port_info);
    } else {
        ret = bk_usbh_hub_port_check_device(user_config->port, USB_UVC_DEVICE, &uvc_port_info);
    }

    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, port_check_device failed, retry <USB_UVC_DEVICE>\n", __func__, __LINE__);
        ret = bk_usbh_hub_port_check_device(user_config->port, USB_UVC_DEVICE, &uvc_port_info);
    }

    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, port_check_device failed, format:%d\n", __func__, __LINE__, user_config->format);
        return ret;
    }

    bk_uvc_device_brief_info_t *uvc_device_param = (bk_uvc_device_brief_info_t *)uvc_port_info->usb_device_param;
    if (uvc_device_param == NULL) {
        LOGE("%s, %d, uvc_device_param is NULL\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    switch (user_config->format) {
        case BK_IMAGE_FORMAT_YUV:
            ret = uvc_check_yuv_config(uvc_device_param, user_config);
            break;
        case BK_IMAGE_FORMAT_MJPEG:
            ret = uvc_check_mjpeg_config(uvc_device_param, user_config);
            break;
        case BK_IMAGE_FORMAT_H264:
            ret = uvc_check_h264_config(uvc_device_param, user_config);
            break;
        case BK_IMAGE_FORMAT_H265:
            ret = uvc_check_h265_config(uvc_device_param, user_config);
            break;
        default:
            LOGE("%s, please check usb output format:%d\r\n", __func__, user_config->format);
            break;
    }

    LOGI("%s, %d, ret:%d\r\n", __func__, __LINE__, ret);

    return ret;
}

static void uvc_device_connect_callback(bk_usb_hub_port_info *port_info, void *arg)
{
    LOGI("%s: UVC device connected\n", __func__);

    if (!port_info) {
        LOGE("%s: port_info is NULL\n", __func__);
        return;
    }

    beken_semaphore_t sem = (beken_semaphore_t)arg;
    if (sem) {
        rtos_set_semaphore(&sem);
    }
}

static avdk_err_t uvc_camera_power_on(uint32_t timeout)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_uvc_connect_sem == NULL) {
        ret = rtos_init_semaphore(&s_uvc_connect_sem, 1);
        if (ret != AVDK_ERR_OK) {
            LOGE("%s, %d, rtos_init_semaphore failed, timeout:%d\n", __func__, __LINE__, timeout);
            return ret;
        }
    }

    rtos_get_semaphore(&s_uvc_connect_sem, BEKEN_NO_WAIT);

    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++) {
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_DEVICE, uvc_device_connect_callback, s_uvc_connect_sem);
        bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, port, USB_UVC_DEVICE);
    }

    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++) {
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_H26X_DEVICE, uvc_device_connect_callback, s_uvc_connect_sem);
        bk_usbh_hub_multiple_devices_power_on(USB_HOST_MODE, port, USB_UVC_H26X_DEVICE);
    }

    bk_usb_hub_port_info *port_info = NULL;
    bk_usb_hub_port_info *port_info_h26x = NULL;
    avdk_err_t ret1 = AVDK_ERR_OK;
    avdk_err_t ret2 = AVDK_ERR_OK;
    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++) {
        ret1 = bk_usbh_hub_port_check_device(port, USB_UVC_DEVICE, &port_info);
        ret2 = bk_usbh_hub_port_check_device(port, USB_UVC_H26X_DEVICE, &port_info_h26x);
        if (ret1 == AVDK_ERR_OK || ret2 == AVDK_ERR_OK) {
            break;
        }
    }

    if (ret1 != AVDK_ERR_OK && ret2 != AVDK_ERR_OK) {
        ret = rtos_get_semaphore(&s_uvc_connect_sem, timeout);
    }

    return ret;
}

static avdk_err_t uvc_camera_power_off(void)
{
    for (uint8_t index = 0; index < UVC_PORT_MAX; index++) {
        if (s_uvc_handle[index] != NULL) {
            LOGW("%s, %d, uvc port:%d not closed\n", __func__, __LINE__, index + 1);
            return AVDK_ERR_GENERIC;
        }
    }
    for (uint8_t port = 1; port <= UVC_PORT_MAX; port++) {
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_DEVICE, NULL, NULL);
        bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, USB_UVC_DEVICE);
        bk_usbh_hub_port_register_connect_callback(port, USB_UVC_H26X_DEVICE, NULL, NULL);
        bk_usbh_hub_multiple_devices_power_down(USB_HOST_MODE, port, USB_UVC_H26X_DEVICE);
    }
    return AVDK_ERR_OK;
}

static bk_uvc_ctlr_handle_t uvc_camera_turn_on(bk_cam_uvc_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    bk_uvc_ctlr_handle_t handle = NULL;

    if (config == NULL) {
        LOGE("%s: parameters is NULL\n", __func__);
        return NULL;
    }

    ret = encode_frame_que_init();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, encode frame queue init failed\n", __func__, __LINE__);
        return NULL;
    }

    ret = uvc_camera_power_on(4000);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s: uvc_camera_power_on failed\n", __func__);
        goto exit;
    }

    ret = uvc_checkout_port_info(config);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d: uvc_checkout_port_info failed\n", __func__, __LINE__);
        goto exit;
    }

    ret = bk_uvc_ctrl_new(&handle, &uvc_camera_cbs);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d: bk_uvc_ctrl_new failed\n", __func__, __LINE__);
        goto exit;
    }

    ret = bk_uvc_init(handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d: bk_uvc_init failed\n", __func__, __LINE__);
        goto exit;
    }

    ret = bk_uvc_open(handle, config);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d: bk_uvc_open failed\n", __func__, __LINE__);
        goto exit;
    }

    LOGD("%s open successful\n", __func__);

    return handle;

exit:
    if (handle != NULL) {
        bk_uvc_deinit(handle);
        bk_uvc_delete(handle);
    }
    uvc_camera_power_off();
    return NULL;
}

static avdk_err_t uvc_camera_turn_off(bk_uvc_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (handle == NULL) {
        return ret;
    }

    ret = bk_uvc_close(handle);
    if (ret != BK_OK) {
        LOGE("%s: bk_uvc_close failed\n", __func__);
        return ret;
    }

    ret = bk_uvc_deinit(handle);
    if (ret != BK_OK) {
        LOGE("%s: bk_uvc_deinit failed\n", __func__);
        return ret;
    }

    ret = bk_uvc_delete(handle);
    if (ret != BK_OK) {
        LOGE("%s: bk_uvc_delete failed\n", __func__);
        return ret;
    }

    uvc_camera_power_off();

    return ret;
}

/* ==================== Full pipeline orchestration (three public entry points) ==================== */

static void *s_mjpegd_gpu_bond = NULL;
static bk_uvc_ctlr_handle_t s_pipeline_uvc_handle = NULL;

bool uvc_pipeline_is_open(void)
{
    return s_pipeline_uvc_handle != NULL;
}

avdk_err_t uvc_pipeline_open(uint8_t port, uint16_t width, uint16_t height, uint8_t fps)
{
    avdk_err_t ret;
    bk_jpeg_decode_ctlr_handle_t decode_handle = NULL;
    bk_gpu_ctlr_handle_t gpu_handle = NULL;
    uint8_t *flexa_buf = NULL;
    uint8_t flexa_cnt = 0;

    /* Flexa decode needs bond before GPU can consume ring buffer; camera must open last,
     * otherwise decode thread decodes MJPEG before bond/GPU ready -> vcdec timeout.
     * Order: decode (thread idle-waits) -> display+GPU -> bond -> camera (feed MJPEG). */
    ret = decode_open(width, height, BK_IMAGE_FORMAT_MJPEG, 1);
    if (ret != AVDK_ERR_OK) {
        LOGE("decode_open failed, ret=%d\n", ret);
        return ret;
    }

    ret = decode_get_flexa_context(&flexa_buf, &flexa_cnt);
    if (ret != AVDK_ERR_OK || flexa_buf == NULL || flexa_cnt == 0) {
        LOGE("decode_get_flexa_context failed\n");
        goto err_decode;
    }

    ret = display_open_gpu_flexa(width, height, flexa_buf, flexa_cnt);
    if (ret != AVDK_ERR_OK) {
        LOGE("display_open_gpu_flexa failed, ret=%d\n", ret);
        goto err_decode;
    }

    gpu_handle = display_get_gpu_handle();
    if (gpu_handle == NULL) {
        LOGE("display_get_gpu_handle failed\n");
        goto err_display;
    }

    ret = decode_get_handle(&decode_handle);
    if (ret != AVDK_ERR_OK || decode_handle == NULL) {
        LOGE("decode_get_handle failed\n");
        goto err_display;
    }

    ret = bk_flexa_mjpegd_gpu_bond_start(&s_mjpegd_gpu_bond, decode_handle, gpu_handle);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_flexa_mjpegd_gpu_bond_start failed, ret=%d\n", ret);
        goto err_display;
    }

    bk_cam_uvc_config_t uvc_cfg = MEDIA_UVC_MJPEG_864X480_30FPS_CONFIG();
    uvc_cfg.port = port;
    uvc_cfg.width = width;
    uvc_cfg.height = height;
    uvc_cfg.fps = fps;
    uvc_cfg.format = BK_IMAGE_FORMAT_MJPEG;

    s_pipeline_uvc_handle = uvc_camera_turn_on(&uvc_cfg);
    if (s_pipeline_uvc_handle == NULL) {
        LOGE("uvc_camera_turn_on failed\n");
        ret = AVDK_ERR_GENERIC;
        goto err_bond;
    }

    LOGI("uvc pipeline open ok: port=%u %ux%u@%u\n", port, width, height, fps);
    return AVDK_ERR_OK;

err_bond:
    bk_flexa_mjpegd_gpu_bond_stop(s_mjpegd_gpu_bond);
    s_mjpegd_gpu_bond = NULL;
err_display:
    display_close();
err_decode:
    decode_close();
    return ret;
}

avdk_err_t uvc_pipeline_close(void)
{
    if (s_pipeline_uvc_handle != NULL) {
        (void)uvc_camera_turn_off(s_pipeline_uvc_handle);
        s_pipeline_uvc_handle = NULL;
    }

    if (s_mjpegd_gpu_bond != NULL) {
        bk_flexa_mjpegd_gpu_bond_stop(s_mjpegd_gpu_bond);
        s_mjpegd_gpu_bond = NULL;
    }

    (void)display_close();
    (void)decode_close();

    LOGI("uvc pipeline close ok\n");
    return AVDK_ERR_OK;
}
