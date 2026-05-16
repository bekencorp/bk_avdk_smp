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
#include "hpdma/lv_hpdma.h"
#include <modules/vg_lite_gpu/vg_lite.h>

#define TAG "LVGL_PARTIAL"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

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

static void lv_memcpy_one_line(void *dest_buf, const void *src_buf, uint32_t point_num)
{
    os_memcpy(dest_buf, src_buf, point_num * sizeof(bk_color_t));
}

void lv_port_disp_partial_init(lv_vnd_data_t *vnd_data)
{
    lv_hpdma_memcpy_init(vnd_data);

    if (vnd_data->config.output_compress || (vnd_data->config.rotation != ROTATE_NONE && LV_USE_GPU_ROTATE)) {
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
        if (vnd_data->config.rotation == ROTATE_90) {
            vg_lite_rotate(270.0f, &lv_matrix);
        } else if (vnd_data->config.rotation == ROTATE_270) {
            vg_lite_rotate(90.0f, &lv_matrix);
        } else if (vnd_data->config.rotation == ROTATE_180) {
            vg_lite_rotate(180.0f, &lv_matrix);
        } else {
            vg_lite_rotate(0.0f, &lv_matrix);
        }
    }
}

void lv_port_disp_partial_deinit(lv_vnd_data_t *vnd_data)
{
    lv_hpdma_memcpy_deinit(vnd_data);

    if (vnd_data->config.output_compress || (vnd_data->config.rotation != ROTATE_NONE && LV_USE_GPU_ROTATE)) {
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

static void lv_partial_flush_compress(lv_vnd_data_t *vnd_data, lv_partial_flush_ctx_t *ctx, uint8_t *px_map)
{
    vg_lite_rectangle_t rect = {
        .x = 0,
        .y = 0,
        .width = ctx->width,
        .height = ctx->height,
    };

    lv_src_buf.width = ctx->width;
    lv_src_buf.height = ctx->height;
    vg_lite_allocate_with_data(&lv_src_buf, px_map, NULL, NULL, NULL);

    lv_dst_buf.width = vnd_data->config.disp_width;
    lv_dst_buf.height = vnd_data->config.disp_height;
    vg_lite_allocate_with_data(&lv_dst_buf, vnd_data->disp_buf, NULL, NULL, NULL);

    vg_lite_identity(&lv_matrix);
    vg_lite_translate(ctx->area->x1, ctx->area->y1, &lv_matrix);
    vg_lite_blit_rect(&lv_dst_buf, &lv_src_buf, &rect, &lv_matrix, VG_LITE_BLEND_SRC_OVER, 0, VG_LITE_FILTER_LINEAR);
    vg_lite_finish();
}

static void lv_partial_flush_rotate(lv_display_t *disp_drv, lv_vnd_data_t *vnd_data, const lv_area_t *area,
                                    uint8_t *px_map, lv_partial_flush_ctx_t *ctx)
{
    lv_color_format_t cf = lv_display_get_color_format(disp_drv);

    if (vnd_data->config.rotation != ROTATE_NONE) {
        #if LV_USE_GPU_ROTATE
            lv_src_buf.width = ctx->width;
            lv_src_buf.height = ctx->height;
            vg_lite_allocate_with_data(&lv_src_buf, px_map, NULL, NULL, NULL);

            if (vnd_data->config.rotation == ROTATE_90 || vnd_data->config.rotation == ROTATE_270) {
                lv_dst_buf.width = ctx->height;
                lv_dst_buf.height = ctx->width;
            } else {
                lv_dst_buf.width = ctx->width;
                lv_dst_buf.height = ctx->height;
            }
            vg_lite_allocate_with_data(&lv_dst_buf, vnd_data->rotate_buffer, NULL, NULL, NULL);

            switch (vnd_data->config.rotation) {
                case ROTATE_90:
                    lv_matrix.m[0][2] = 0.0f;
                    lv_matrix.m[1][2] = ctx->width;
                    break;
                case ROTATE_270:
                    lv_matrix.m[0][2] = ctx->height;
                    lv_matrix.m[1][2] = 0.0f;
                    break;
                case ROTATE_180:
                    lv_matrix.m[0][2] = ctx->width;
                    lv_matrix.m[1][2] = ctx->height;
                    break;
                default:
                    break;
            }

            vg_lite_blit(&lv_dst_buf, &lv_src_buf, &lv_matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
            vg_lite_finish();
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
        offset = ctx->area->y1 * ctx->lv_hor + ctx->area->x1;
        for (y = ctx->area->y1; y <= ctx->area->y2; y++) {
            lv_memcpy_one_line((uint8_t *)vnd_data->disp_buf + offset * ctx->color_size, color_ptr, ctx->width);
            offset += ctx->lv_hor;
            color_ptr += ctx->lv_stride;
        }
    }
}

static void lv_partial_flush_frame_buffer_copy(lv_display_t *disp_drv, lv_vnd_data_t *vnd_data, lv_coord_t lv_hor)
{
    if (vnd_data->config.output_compress) {
        lv_hpdma_memcpy_start(vnd_data->disp_buf, vnd_data->copy_buf,
                              vnd_data->config.disp_width, vnd_data->config.disp_height,
                              vnd_data->config.disp_width, vnd_data->config.disp_height,
                              0, 0);
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

    if (vnd_data->config.output_compress) {
        lv_get_display_buffer(vnd_data, ctx.area);
        lv_partial_flush_compress(vnd_data, &ctx, px_map);
    } else {
        lv_partial_flush_rotate(disp_drv, vnd_data, area, px_map, &ctx);
        lv_get_display_buffer(vnd_data, ctx.area);
        lv_area_join(&vnd_data->d_area, &vnd_data->d_area, ctx.area);
        lv_partial_flush_copy_to_disp_buf(vnd_data, &ctx);
    }

    lv_partial_flush_finish(disp_drv, vnd_data, ctx.lv_hor);
}