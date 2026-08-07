/**
 * @file lv_port_disp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_disp_private.h"
#include "../src/misc/lv_area_private.h"
#include "lv_hpdma.h"
#include "lv_gpu_rotate.h"
#include <modules/vg_lite_gpu/vg_lite.h>

#define TAG "LVGL_PARTIAL"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#define LV_COMPRESSED_TILE_WIDTH 16
#define LV_COMPRESSED_TILE_HEIGHT 4
#define LV_PARTIAL_HPDMA_WAIT_MS 5000

typedef struct {
    const lv_area_t *area;
    lv_area_t rotated_area;
    uint8_t *color_ptr;
    uint8_t color_size;
    lv_coord_t width;
    lv_coord_t height;
    lv_coord_t lv_hor;
    lv_coord_t lv_stride;
} lv_partial_flush_ctx_t;

static vg_lite_buffer_t lv_dst_buf;
static vg_lite_buffer_t lv_src_buf;
static vg_lite_matrix_t lv_matrix;

static vg_lite_color_t lv_partial_color_to_vg(lv_color_t color)
{
    lv_color32_t color32 = lv_color_to_32(color, LV_OPA_COVER);

    return ((vg_lite_color_t)color32.alpha << 24) | ((vg_lite_color_t)color32.blue << 16) |
           ((vg_lite_color_t)color32.green << 8) | (vg_lite_color_t)color32.red;
}

static void lv_memcpy_one_line(void *dest_buf, const void *src_buf, uint32_t point_num)
{
    os_memcpy(dest_buf, src_buf, point_num * sizeof(bk_color_t));
}

void lv_port_disp_partial_init(lv_vnd_data_t *vnd_data)
{
    lv_hpdma_memcpy_init(vnd_data);

#if LV_USE_GPU_ROTATE
    if (vnd_data->config.rotation != ROTATE_NONE &&
        !vnd_data->config.output_compress) {
        lv_gpu_rotate_init(vnd_data);
    }
#endif

    if (vnd_data->config.output_compress) {
        os_memset(&lv_dst_buf, 0, sizeof(vg_lite_buffer_t));
        #if (LV_COLOR_DEPTH == 16)
            lv_dst_buf.format = VG_LITE_BGR565;
        #elif (LV_COLOR_DEPTH == 24)
            lv_dst_buf.format = VG_LITE_BGR888;
        #elif (LV_COLOR_DEPTH == 32)
            lv_dst_buf.format = VG_LITE_BGRA8888;
        #endif
        lv_dst_buf.compress_mode = VG_LITE_DEC_DISABLE;

        if (vnd_data->config.output_compress) {
            lv_dst_buf.format = VG_LITE_BGRA8888;
            lv_dst_buf.tiled = 1;
            lv_dst_buf.compress_mode = VG_LITE_DEC_HV_SAMPLE;
        }

        os_memset(&lv_src_buf, 0, sizeof(vg_lite_buffer_t));
        #if (LV_COLOR_DEPTH == 16)
            lv_src_buf.format = VG_LITE_BGR565;
        #elif (LV_COLOR_DEPTH == 24)
            lv_src_buf.format = VG_LITE_BGR888;
        #elif (LV_COLOR_DEPTH == 32)
            lv_src_buf.format = VG_LITE_BGRA8888;
        #endif
        lv_src_buf.compress_mode = VG_LITE_DEC_DISABLE;

        vg_lite_identity(&lv_matrix);
    }
}

void lv_port_disp_partial_deinit(lv_vnd_data_t *vnd_data)
{
    lv_hpdma_memcpy_deinit(vnd_data);

#if LV_USE_GPU_ROTATE
    if (!vnd_data->config.output_compress) {
        lv_gpu_rotate_deinit(vnd_data);
    }
#endif

    if (vnd_data->config.output_compress) {
        vg_lite_free_without_free_data(&lv_src_buf);
        vg_lite_free_without_free_data(&lv_dst_buf);
    }
}

static void *lv_wait_ready_frame_buffer(void)
{
    void *frame_buffer = NULL;

    do {
        frame_buffer = lv_vendor_get_ready_frame_buffer();
        if (frame_buffer != NULL) {
            break;
        }
    } while (frame_buffer == NULL);

    return frame_buffer;
}

static void lv_get_display_buffer(lv_vnd_data_t *vnd_data, const lv_area_t *area)
{
#if (CONFIG_LVGL_FRAME_BUFFER_NUM > 1)
    lv_hpdma_memcpy_wait_finish(BEKEN_WAIT_FOREVER);

    if (vnd_data->lv_new_frame_flag) {
        if (vnd_data->disp_buf == NULL) {
            vnd_data->disp_buf = lv_wait_ready_frame_buffer();
        } else {
            vnd_data->disp_buf = vnd_data->copy_buf;
            vnd_data->copy_buf = NULL;
        }
        vnd_data->lv_new_frame_flag = false;
        vnd_data->d_area = *area;
    }
#else
    if (vnd_data->lv_new_frame_flag) {
        vnd_data->disp_buf = vnd_data->config.frame_buffer[0];
        vnd_data->lv_new_frame_flag = false;
        vnd_data->d_area = *area;
    }
#endif
}

static void lv_partial_set_compress_matrix(const lv_vnd_data_t *vnd_data, const lv_partial_flush_ctx_t *ctx)
{
    vg_lite_identity(&lv_matrix);

    switch (vnd_data->config.rotation) {
        case ROTATE_90:
            vg_lite_rotate(270.0f, &lv_matrix);
            lv_matrix.m[0][2] = ctx->area->x1;
            lv_matrix.m[1][2] = ctx->area->y1 + ctx->width;
            break;
        case ROTATE_270:
            vg_lite_rotate(90.0f, &lv_matrix);
            lv_matrix.m[0][2] = ctx->area->x1 + ctx->height;
            lv_matrix.m[1][2] = ctx->area->y1;
            break;
        case ROTATE_180:
            vg_lite_rotate(180.0f, &lv_matrix);
            lv_matrix.m[0][2] = ctx->area->x1 + ctx->width;
            lv_matrix.m[1][2] = ctx->area->y1 + ctx->height;
            break;
        default:
            vg_lite_translate(ctx->area->x1, ctx->area->y1, &lv_matrix);
            break;
    }
}

static void lv_partial_flush_compress(lv_vnd_data_t *vnd_data, lv_partial_flush_ctx_t *ctx)
{
    vg_lite_rectangle_t rect = {
        .x = 0,
        .y = 0,
        .width = ctx->width,
        .height = ctx->height,
    };

    bool gpu_locked = lv_vendor_gpu_lock();

    lv_src_buf.width = ctx->width;
    lv_src_buf.height = ctx->height;
    vg_lite_allocate_with_data(&lv_src_buf, ctx->color_ptr, NULL, NULL, NULL);

    lv_dst_buf.width = vnd_data->config.disp_width;
    lv_dst_buf.height = vnd_data->config.disp_height;
    vg_lite_allocate_with_data(&lv_dst_buf, vnd_data->disp_buf, NULL, NULL, NULL);

    lv_partial_set_compress_matrix(vnd_data, ctx);
    vg_lite_error_t ret = vg_lite_blit_rect(&lv_dst_buf, &lv_src_buf, &rect, &lv_matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
    if (ret != VG_LITE_SUCCESS) {
        LOGE("%s blit compressed frame buffer failed, ret=%d, area=(%d,%d)-(%d,%d)\n",
             __func__, ret, ctx->area->x1, ctx->area->y1, ctx->area->x2, ctx->area->y2);
    }
    vg_lite_finish();

    lv_vendor_gpu_unlock(gpu_locked);
}

static void lv_partial_prepare_compress(lv_display_t *disp_drv, uint8_t *px_map, lv_partial_flush_ctx_t *ctx)
{
    ctx->color_ptr = px_map;

    if (lv_display_get_rotation(disp_drv) != LV_DISPLAY_ROTATION_0) {
        ctx->rotated_area = *ctx->area;
        lv_display_rotate_area(disp_drv, &ctx->rotated_area);
        ctx->area = &ctx->rotated_area;
    }
}

static void lv_partial_flush_rotate(lv_display_t *disp_drv, lv_vnd_data_t *vnd_data, const lv_area_t *area,
                                    uint8_t *px_map, lv_partial_flush_ctx_t *ctx)
{
    lv_color_format_t cf = lv_display_get_color_format(disp_drv);

    if (vnd_data->config.rotation != ROTATE_NONE) {
        #if LV_USE_GPU_ROTATE
            lv_gpu_rotate_process(vnd_data, px_map, ctx->width, ctx->height);
        #else
            uint32_t w_stride = lv_draw_buf_width_to_stride(ctx->width, cf);
            uint32_t h_stride = lv_draw_buf_width_to_stride(ctx->height, cf);

            if (vnd_data->config.rotation == ROTATE_180) {
                lv_draw_sw_rotate(px_map, vnd_data->rotate_buffer, ctx->width, ctx->height, w_stride, w_stride, vnd_data->config.rotation, cf);
            } else {
                lv_draw_sw_rotate(px_map, vnd_data->rotate_buffer, ctx->width, ctx->height, w_stride, h_stride, vnd_data->config.rotation, cf);
            }
        #endif
        ctx->color_ptr = (uint8_t *)vnd_data->rotate_buffer;
        ctx->rotated_area = *area;
        lv_display_rotate_area(disp_drv, &ctx->rotated_area);
        ctx->area = &ctx->rotated_area;

        if (vnd_data->config.rotation != ROTATE_180) {
            ctx->width = lv_area_get_width(ctx->area);
            ctx->height = lv_area_get_height(ctx->area);
        }

        ctx->lv_stride = lv_draw_buf_width_to_stride(ctx->width, cf);
    } else {
        ctx->color_ptr = px_map;
        ctx->lv_stride = lv_display_get_buf_active(disp_drv)->header.stride;
    }
}

static void lv_partial_flush_copy_to_disp_buf(lv_vnd_data_t *vnd_data, const lv_partial_flush_ctx_t *ctx)
{
    int y = 0;
    int offset = 0;
    uint8_t *color_ptr = ctx->color_ptr;

    if (vnd_data->config.draw_buf_2_2) {
        // to do
    } else {
        uint32_t line_bytes = ctx->width * ctx->color_size;
        uint32_t area_height = lv_area_get_height(ctx->area);
        uint32_t dst_step = (ctx->lv_hor - ctx->width) * ctx->color_size;
        uint32_t src_step = ctx->lv_stride - line_bytes;
        void *dst_start = (uint8_t *)vnd_data->disp_buf +
                          (ctx->area->y1 * ctx->lv_hor + ctx->area->x1) * ctx->color_size;

        bk_err_t ret = lv_hpdma_memcpy_start(color_ptr, dst_start, line_bytes, area_height,
                                             line_bytes, area_height, src_step, dst_step);

        if (ret == BK_OK) {
            ret = lv_hpdma_memcpy_wait_finish(LV_PARTIAL_HPDMA_WAIT_MS);
            if (ret != BK_OK) {
                LOGE("%s %d hpdma wait timeout\n", __func__, __LINE__);
                lv_hpdma_memcpy_stop();
            }
        }

        if (ret != BK_OK) {
            offset = ctx->area->y1 * ctx->lv_hor + ctx->area->x1;
            for (y = ctx->area->y1; y <= ctx->area->y2; y++) {
                lv_memcpy_one_line((uint8_t *)vnd_data->disp_buf + offset * ctx->color_size, color_ptr, ctx->width);
                offset += ctx->lv_hor;
                color_ptr += ctx->lv_stride;
            }
        }
    }
}

static uint32_t lv_partial_align_down(uint32_t value, uint32_t align)
{
    return value & ~(align - 1);
}

static uint32_t lv_partial_align_up(uint32_t value, uint32_t align)
{
    return (value + align - 1) & ~(align - 1);
}

static void lv_partial_flush_compressed_frame_buffer_copy(lv_vnd_data_t *vnd_data)
{
    uint32_t x1 = lv_partial_align_down((uint32_t)vnd_data->d_area.x1, LV_COMPRESSED_TILE_WIDTH);
    uint32_t y1 = lv_partial_align_down((uint32_t)vnd_data->d_area.y1, LV_COMPRESSED_TILE_HEIGHT);
    uint32_t x2 = lv_partial_align_up((uint32_t)vnd_data->d_area.x2 + 1, LV_COMPRESSED_TILE_WIDTH);
    uint32_t y2 = lv_partial_align_up((uint32_t)vnd_data->d_area.y2 + 1, LV_COMPRESSED_TILE_HEIGHT);

    if (x2 > vnd_data->config.disp_width) {
        x2 = vnd_data->config.disp_width;
    }

    if (y2 > vnd_data->config.disp_height) {
        y2 = vnd_data->config.disp_height;
    }

    if (x1 >= x2 || y1 >= y2) {
        LOGE("%s invalid compressed area: (%d,%d)-(%d,%d)\n",
             __func__, vnd_data->d_area.x1, vnd_data->d_area.y1,
             vnd_data->d_area.x2, vnd_data->d_area.y2);
        return;
    }

    /*
     * VG_LITE_DEC_HV_SAMPLE stores BGRA8888 as compressed 16x4 tiles.
     * One tile band contains 4 compressed bytes per horizontal pixel.
     */
    uint32_t line_bytes = (x2 - x1) * LV_COMPRESSED_TILE_HEIGHT;
    uint32_t band_count = (y2 - y1) / LV_COMPRESSED_TILE_HEIGHT;
    uint32_t frame_stride = vnd_data->config.disp_width * LV_COMPRESSED_TILE_HEIGHT;
    uint32_t step_bytes = frame_stride - line_bytes;
    uint32_t offset = (y1 / LV_COMPRESSED_TILE_HEIGHT) * frame_stride + x1 * LV_COMPRESSED_TILE_HEIGHT;
    void *src_start = (uint8_t *)vnd_data->disp_buf + offset;
    void *dst_start = (uint8_t *)vnd_data->copy_buf + offset;

    lv_hpdma_memcpy_start(src_start, dst_start,
                          line_bytes, band_count,
                          line_bytes, band_count,
                          step_bytes, step_bytes);
}

static void lv_partial_flush_frame_buffer_copy(lv_display_t *disp_drv, lv_vnd_data_t *vnd_data, lv_coord_t lv_hor)
{
    if (vnd_data->config.output_compress) {
        lv_partial_flush_compressed_frame_buffer_copy(vnd_data);
    } else {
        uint32_t color_size = lv_color_format_get_size(lv_display_get_color_format(disp_drv));
        uint32_t area_width = lv_area_get_width(&vnd_data->d_area);
        uint32_t area_height = lv_area_get_height(&vnd_data->d_area);
        uint32_t line_bytes = area_width * color_size;
        uint32_t step_bytes = (lv_hor - area_width) * color_size;
        void *src_start = (uint8_t *)vnd_data->disp_buf + (vnd_data->d_area.y1 * lv_hor + vnd_data->d_area.x1) * color_size;
        void *dst_start = (uint8_t *)vnd_data->copy_buf + (vnd_data->d_area.y1 * lv_hor + vnd_data->d_area.x1) * color_size;
        lv_hpdma_memcpy_start(src_start, dst_start,
                              line_bytes, area_height,
                              line_bytes, area_height,
                              step_bytes, step_bytes);
    }
}

static bk_err_t lv_partial_frame_buffer_free_cb(void *frame)
{
#if CONFIG_LVGL_FRAME_BUFFER_NUM > 1
    lv_vendor_set_ready_frame_buffer(frame);
#endif

    return BK_OK;
}

static void lv_partial_flush_finish(lv_display_t *disp_drv, lv_vnd_data_t *vnd_data, lv_coord_t lv_hor)
{
    if (lv_disp_flush_is_last(disp_drv)) {
        vnd_data->config.flush_cb(vnd_data->config.args, vnd_data->disp_buf, lv_partial_frame_buffer_free_cb);
        vnd_data->lv_new_frame_flag = true;

        #if (CONFIG_LVGL_FRAME_BUFFER_NUM > 1)
            if (vnd_data->copy_buf == NULL) {
                vnd_data->copy_buf = lv_wait_ready_frame_buffer();
            }
            #if 0
                int y = 0;
                int offset = vnd_data->d_area.y1 * lv_hor + vnd_data->d_area.x1;
                for (y = vnd_data->d_area.y1; y <= vnd_data->d_area.y2; y++) {
                    os_memcpy((uint16_t *)vnd_data->copy_buf + offset, (uint16_t *)vnd_data->disp_buf + offset, lv_area_get_width(&vnd_data->d_area) * 2);
                    offset += lv_hor;
                }
            #else
                lv_partial_flush_frame_buffer_copy(disp_drv, vnd_data, lv_hor);
            #endif
        #endif
    }
}

void lv_disp_flush_for_partial_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(disp_drv);
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    lv_partial_flush_ctx_t ctx = {
        .area = area,
        .width = lv_area_get_width(area),
        .height = lv_area_get_height(area),
        .color_ptr = NULL,
        .color_size = lv_color_format_get_size(lv_display_get_color_format(disp_drv)),
        .lv_stride = 0,
    };

    if (vnd_data->config.rotation == ROTATE_NONE || vnd_data->config.rotation == ROTATE_180) {
        ctx.lv_hor = LV_HOR_RES;
    } else {
        ctx.lv_hor = LV_VER_RES;
    }

    if (ctx.width <= 0 || ctx.height <= 0) {
        LOGW("%s skip empty flush area: (%d,%d)-(%d,%d)\n",
             __func__, area->x1, area->y1, area->x2, area->y2);
        if (lv_disp_flush_is_last(disp_drv) && vnd_data->disp_buf != NULL) {
            lv_partial_flush_finish(disp_drv, vnd_data, ctx.lv_hor);
        }
        return;
    }

    if (vnd_data->config.output_compress) {
        lv_partial_prepare_compress(disp_drv, px_map, &ctx);
    } else {
        lv_partial_flush_rotate(disp_drv, vnd_data, area, px_map, &ctx);
    }

    if (lv_area_get_width(ctx.area) <= 0 || lv_area_get_height(ctx.area) <= 0) {
        LOGW("%s skip empty rotated flush area: (%d,%d)-(%d,%d), original=(%d,%d)-(%d,%d)\n",
             __func__, ctx.area->x1, ctx.area->y1, ctx.area->x2, ctx.area->y2,
             area->x1, area->y1, area->x2, area->y2);
        if (lv_disp_flush_is_last(disp_drv) && vnd_data->disp_buf != NULL) {
            lv_partial_flush_finish(disp_drv, vnd_data, ctx.lv_hor);
        }
        return;
    }

    lv_get_display_buffer(vnd_data, ctx.area);
    lv_area_join(&vnd_data->d_area, &vnd_data->d_area, ctx.area);

    if (vnd_data->config.output_compress) {
        lv_partial_flush_compress(vnd_data, &ctx);
    } else {
        lv_partial_flush_copy_to_disp_buf(vnd_data, &ctx);
    }

    lv_partial_flush_finish(disp_drv, vnd_data, ctx.lv_hor);
}