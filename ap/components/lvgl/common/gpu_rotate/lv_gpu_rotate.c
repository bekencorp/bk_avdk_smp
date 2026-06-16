/**
 * @file lv_gpu_rotate.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_gpu_rotate.h"
#include <modules/vg_lite_gpu/vg_lite.h>

#define TAG "LVGL_GPU_ROTATE"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static vg_lite_buffer_t s_lv_gpu_rotate_dst_buf;
static vg_lite_buffer_t s_lv_gpu_rotate_src_buf;
static vg_lite_matrix_t s_lv_gpu_rotate_matrix;

static bool lv_gpu_rotate_is_enabled(const lv_vnd_data_t *vnd_data)
{
    return (vnd_data != NULL) &&
           (vnd_data->config.render_mode == RENDER_PARTIAL_MODE) &&
           (vnd_data->config.rotation != ROTATE_NONE);
}

static void lv_gpu_rotate_set_buffer_format(vg_lite_buffer_t *buf)
{
    os_memset(buf, 0, sizeof(vg_lite_buffer_t));

#if (LV_COLOR_DEPTH == 16)
    buf->format = VG_LITE_BGR565;
#elif (LV_COLOR_DEPTH == 24)
    buf->format = VG_LITE_BGR888;
#elif (LV_COLOR_DEPTH == 32)
    buf->format = VG_LITE_BGRA8888;
#endif
    buf->compress_mode = VG_LITE_DEC_DISABLE;
}

static void lv_gpu_rotate_set_matrix(rott_angle_t rotation, lv_coord_t src_width, lv_coord_t src_height)
{
    vg_lite_identity(&s_lv_gpu_rotate_matrix);

    switch (rotation) {
        case ROTATE_90:
            vg_lite_rotate(270.0f, &s_lv_gpu_rotate_matrix);
            s_lv_gpu_rotate_matrix.m[0][2] = 0.0f;
            s_lv_gpu_rotate_matrix.m[1][2] = src_width;
            break;
        case ROTATE_270:
            vg_lite_rotate(90.0f, &s_lv_gpu_rotate_matrix);
            s_lv_gpu_rotate_matrix.m[0][2] = src_height;
            s_lv_gpu_rotate_matrix.m[1][2] = 0.0f;
            break;
        case ROTATE_180:
            vg_lite_rotate(180.0f, &s_lv_gpu_rotate_matrix);
            s_lv_gpu_rotate_matrix.m[0][2] = src_width;
            s_lv_gpu_rotate_matrix.m[1][2] = src_height;
            break;
        default:
            break;
    }
}

void lv_gpu_rotate_init(lv_vnd_data_t *vnd_data)
{
    if (!lv_gpu_rotate_is_enabled(vnd_data)) {
        LOGE("%s lv_gpu_rotate is not enabled\n", __func__);
        return;
    }

    lv_gpu_rotate_set_buffer_format(&s_lv_gpu_rotate_dst_buf);
    lv_gpu_rotate_set_buffer_format(&s_lv_gpu_rotate_src_buf);
    vg_lite_identity(&s_lv_gpu_rotate_matrix);
}

void lv_gpu_rotate_deinit(lv_vnd_data_t *vnd_data)
{
    if (!lv_gpu_rotate_is_enabled(vnd_data)) {
        LOGE("%s lv_gpu_rotate is not enabled\n", __func__);
        return;
    }

    vg_lite_free_without_free_data(&s_lv_gpu_rotate_src_buf);
    vg_lite_free_without_free_data(&s_lv_gpu_rotate_dst_buf);
}

void lv_gpu_rotate_process(lv_vnd_data_t *vnd_data, uint8_t *src_buf, lv_coord_t src_width, lv_coord_t src_height)
{
    if ((vnd_data == NULL) || (src_buf == NULL) || (vnd_data->rotate_buffer == NULL)) {
        LOGE("%s invalid param: vnd=%p src=%p rotate=%p\n", __func__, vnd_data, src_buf, vnd_data ? vnd_data->rotate_buffer : NULL);
        return;
    }

    bool gpu_locked = lv_vendor_gpu_lock();

    s_lv_gpu_rotate_src_buf.width = src_width;
    s_lv_gpu_rotate_src_buf.height = src_height;
    vg_lite_allocate_with_data(&s_lv_gpu_rotate_src_buf, src_buf, NULL, NULL, NULL);

    if (vnd_data->config.rotation == ROTATE_90 || vnd_data->config.rotation == ROTATE_270) {
        s_lv_gpu_rotate_dst_buf.width = src_height;
        s_lv_gpu_rotate_dst_buf.height = src_width;
    } else {
        s_lv_gpu_rotate_dst_buf.width = src_width;
        s_lv_gpu_rotate_dst_buf.height = src_height;
    }
    vg_lite_allocate_with_data(&s_lv_gpu_rotate_dst_buf, vnd_data->rotate_buffer, NULL, NULL, NULL);

    lv_gpu_rotate_set_matrix(vnd_data->config.rotation, src_width, src_height);
    vg_lite_blit(&s_lv_gpu_rotate_dst_buf, &s_lv_gpu_rotate_src_buf, &s_lv_gpu_rotate_matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
    vg_lite_finish();

    lv_vendor_gpu_unlock(gpu_locked);
}
