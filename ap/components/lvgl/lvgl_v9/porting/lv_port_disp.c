/**
 * @file lv_port_disp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_disp.h"
#include <modules/image_scale.h>
#include "lv_vendor.h"
#include "../src/misc/lv_area_private.h"
#include <driver/hpdma.h>
#include <driver/hal/hal_hpdma_types.h>
#include <modules/vg_lite_gpu/vg_lite.h>

#define TAG "LVGL_DISP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

static vg_lite_buffer_t lv_dst_buf;
static vg_lite_buffer_t lv_src_buf;
static vg_lite_matrix_t lv_matrix;
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
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

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(lv_vnd_data_t *vnd_data);

static void disp_deinit(lv_vnd_data_t *vnd_data);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void bk_lv_port_disp_init(lv_vnd_data_t *vnd_data)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    if (vnd_data == NULL) {
        LOGE("%s %d vnd_data is NULL\n", __func__, __LINE__);
        return;
    }

    disp_init(vnd_data);

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = NULL;

    disp = lv_display_create(vnd_data->config.width, vnd_data->config.height);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_user_data(disp, vnd_data);
    lv_display_set_rotation(disp, vnd_data->config.rotation);

    lv_display_render_mode_t render_mode = LV_DISPLAY_RENDER_MODE_PARTIAL;
    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE) {
        render_mode = LV_DISPLAY_RENDER_MODE_PARTIAL;
    } else if (vnd_data->config.render_mode == RENDER_DIRECT_MODE) {
        render_mode = LV_DISPLAY_RENDER_MODE_DIRECT;
    } else {
        render_mode = LV_DISPLAY_RENDER_MODE_FULL;
    }
    lv_display_set_buffers(disp, vnd_data->config.draw_buf_2_1, vnd_data->config.draw_buf_2_2, vnd_data->config.draw_pixel_size, render_mode);

    LOGI("LVGL addr1:%x, addr2:%x, pixel size:%d, fb1:%x, fb2:%x\r\n", vnd_data->config.draw_buf_2_1, vnd_data->config.draw_buf_2_2,
                                        vnd_data->config.draw_pixel_size, vnd_data->config.frame_buffer[0], vnd_data->config.frame_buffer[1]);

    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE && vnd_data->config.rotation != ROTATE_NONE) {
        vnd_data->rotate_buffer = lv_vendor_malloc(vnd_data->config.draw_pixel_size);
        if (vnd_data->rotate_buffer == NULL) {
            LOGE("%s lvgl rotate buffer malloc fail!\n", __func__);
            return;
        }
    }
}

void lv_port_disp_deinit(lv_vnd_data_t *vnd_data)
{
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE && vnd_data->config.rotation != ROTATE_NONE) {
        if (vnd_data->rotate_buffer) {
            lv_vendor_free(vnd_data->rotate_buffer);
            vnd_data->rotate_buffer = NULL;
        }
    }

    disp_deinit(vnd_data);

    lv_display_delete(lv_disp_get_default());
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void lv_memcpy_one_line(void *dest_buf, const void *src_buf, uint32_t point_num)
{
    os_memcpy(dest_buf, src_buf, point_num * sizeof(bk_color_t));
}

static void lv_hpdma_transfer_complete_callback(hpdma_id_t hpdma_id, void *user_data)
{
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(lv_disp_get_default());
    if (vnd_data == NULL) {
        LOGE("%s %d vnd_data is NULL\n", __func__, __LINE__);
        return;
    }

    if (vnd_data->lv_hpdma_sem != NULL) {
        rtos_set_semaphore(&vnd_data->lv_hpdma_sem);
    }
}

static void lv_hpdma_memcpy_init(lv_vnd_data_t *vnd_data)
{
    if (vnd_data == NULL) {
        LOGE("%s %d vnd_data is NULL\n", __func__, __LINE__);
        return;
    }

    vnd_data->link_dma_list_table = bk_hpdma_link_init(1);
    if (vnd_data->link_dma_list_table == NULL)
    {
        LOGE("%s, %d bk_hpdma_link_init failed\n", __func__, __LINE__);
        return;
    }

    vnd_data->lv_hpdma_id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (vnd_data->lv_hpdma_id >= HPDMA_ID_MAX)
    {
        LOGE("%s, %d bk_hpdma_alloc failed\n", __func__, __LINE__);
        bk_hpdma_link_deinit(vnd_data->link_dma_list_table);
        vnd_data->link_dma_list_table = NULL;
        return;
    }

    bk_err_t ret = rtos_init_semaphore_ex(&vnd_data->lv_hpdma_sem, 1, 0);
    if (BK_OK != ret) {
        LOGE("%s vnd_data->lv_hpdma_sem init failed\n", __func__);
        bk_hpdma_free(HPDMA_DEV_DTCM, vnd_data->lv_hpdma_id);
        vnd_data->lv_hpdma_id = HPDMA_ID_MAX;
        bk_hpdma_link_deinit(vnd_data->link_dma_list_table);
        vnd_data->link_dma_list_table = NULL;
        return;
    }
}

static void lv_hpdma_memcpy_deinit(lv_vnd_data_t *vnd_data)
{
    if (vnd_data == NULL) {
        LOGE("%s %d vnd_data is NULL\n", __func__, __LINE__);
        return;
    }

    bk_hpdma_stop(vnd_data->lv_hpdma_id);
    bk_hpdma_disable_finish_interrupt(vnd_data->lv_hpdma_id);

    bk_hpdma_link_deinit(vnd_data->link_dma_list_table);
    vnd_data->link_dma_list_table = NULL;

    bk_hpdma_free(HPDMA_DEV_DTCM, vnd_data->lv_hpdma_id);
    vnd_data->lv_hpdma_id = HPDMA_ID_MAX;

    if (vnd_data->lv_hpdma_sem != NULL) {
        rtos_deinit_semaphore(&vnd_data->lv_hpdma_sem);
        vnd_data->lv_hpdma_sem = NULL;
    }

    vnd_data->lv_hpdma_in_use = false;
}

static bk_err_t lv_hpdma_memcpy_start(void *src_buf, void *dst_buf, uint16_t src_xsize, uint16_t src_ysize, uint16_t dst_xsize, uint16_t dst_ysize, uint16_t src_step, uint16_t dst_step)
{
    hpdma_link_config_t config = {0};

    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(lv_disp_get_default());
    if (vnd_data == NULL) {
        LOGE("%s %d vnd_data is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    if ((vnd_data->link_dma_list_table == NULL) || (vnd_data->lv_hpdma_id >= HPDMA_ID_MAX) || (vnd_data->lv_hpdma_sem == NULL)) {
        LOGE("%s hpdma not ready: table=%p id=%d sem=%p\n", __func__, vnd_data->link_dma_list_table, vnd_data->lv_hpdma_id, vnd_data->lv_hpdma_sem);
        return BK_ERR_STATE;
    }

    if ((src_buf == NULL) || (dst_buf == NULL) || (src_xsize == 0) || (src_ysize == 0) || (dst_xsize == 0) || (dst_ysize == 0)) {
        LOGE("%s invalid param: src=%p dst=%p sx=%u sy=%u dx=%u dy=%u\n", __func__, src_buf, dst_buf, src_xsize, src_ysize, dst_xsize, dst_ysize);
        return BK_ERR_PARAM;
    }

    config.src_addr = (uint32_t)src_buf;
    config.dst_addr = (uint32_t)dst_buf;
    config.src_xsize = src_xsize;
    config.src_ysize = src_ysize;
    config.dst_xsize = dst_xsize;
    config.dst_ysize = dst_ysize;
    config.src_step = src_step;
    config.dst_step = dst_step;
    config.finish_int_en = 1;
    config.half_finish_int_en = 0;

    bk_err_t ret = bk_hpdma_link_set_desc(vnd_data->link_dma_list_table, 0, &config);
    if (ret != BK_OK) {
        LOGE("%s bk_hpdma_link_set_desc failed, ret=%d\n", __func__, ret);
        return ret;
    }

    bk_hpdma_set_dest_burst_len(vnd_data->lv_hpdma_id, 0x03);
    bk_hpdma_set_src_burst_len(vnd_data->lv_hpdma_id, 0x03);

    bk_hpdma_register_isr(vnd_data->lv_hpdma_id, NULL, NULL, lv_hpdma_transfer_complete_callback, NULL);
    bk_hpdma_enable_finish_interrupt(vnd_data->lv_hpdma_id);

    ret = bk_hpdma_link_transfer(vnd_data->lv_hpdma_id, vnd_data->link_dma_list_table);
    if (ret != BK_OK) {
        LOGE("%s bk_hpdma_link_transfer failed, ret=%d\n", __func__, ret);
        return ret;
    }

    vnd_data->lv_hpdma_in_use = true;

    return BK_OK;
}

static bk_err_t lv_hpdma_memcpy_wait_finish(uint32_t timeout_ms)
{
    bk_err_t ret = BK_OK;

    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(lv_disp_get_default());
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return BK_FAIL;
    }

    if (vnd_data->lv_hpdma_in_use && (vnd_data->lv_hpdma_sem != NULL)) {
        ret = rtos_get_semaphore(&vnd_data->lv_hpdma_sem, timeout_ms);
        if (ret != BK_OK) {
            LOGE("%s rtos_get_semaphore failed\n", __func__);
            return ret;
        }
        vnd_data->lv_hpdma_in_use = false;
    }

    return ret;
}

/*Initialize your display and the required peripherals.*/
static void disp_init(lv_vnd_data_t *vnd_data)
{
    /*You code here*/
    if (vnd_data->config.render_mode != RENDER_PARTIAL_MODE) {
        bk_err_t ret = rtos_init_semaphore_ex(&vnd_data->lv_disp_sem, 1, 0);
        if (BK_OK != ret) {
            LOGE("%s vnd_data->lv_disp_sem init failed\n", __func__);
            return;
        }
    } else {
        lv_hpdma_memcpy_init(vnd_data);

        if (vnd_data->config.output_compress || (vnd_data->config.rotation != ROTATE_NONE && LV_USE_GPU_ROTATE)) {
            os_memset(&lv_dst_buf, 0, sizeof(vg_lite_buffer_t));
            #if (CONFIG_LV_COLOR_DEPTH == 16)
                lv_dst_buf.format = VG_LITE_BGR565;
            #elif (CONFIG_LV_COLOR_DEPTH == 24)
                lv_dst_buf.format = VG_LITE_BGR888;
            #elif (CONFIG_LV_COLOR_DEPTH == 32)
                lv_dst_buf.format = VG_LITE_BGRA8888;
            #endif
            lv_dst_buf.compress_mode = VG_LITE_DEC_DISABLE;

            if (vnd_data->config.output_compress) {
                lv_dst_buf.format = VG_LITE_BGRA8888;
                lv_dst_buf.tiled = 1;
                lv_dst_buf.compress_mode = VG_LITE_DEC_HV_SAMPLE;
            }

            os_memset(&lv_src_buf, 0, sizeof(vg_lite_buffer_t));
            #if (CONFIG_LV_COLOR_DEPTH == 16)
                lv_src_buf.format = VG_LITE_BGR565;
            #elif (CONFIG_LV_COLOR_DEPTH == 24)
                lv_src_buf.format = VG_LITE_BGR888;
            #elif (CONFIG_LV_COLOR_DEPTH == 32)
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
}

static void disp_deinit(lv_vnd_data_t *vnd_data)
{
    if (vnd_data->config.render_mode != RENDER_PARTIAL_MODE) {
        bk_err_t ret = rtos_deinit_semaphore(&vnd_data->lv_disp_sem);
        if (BK_OK != ret) {
            LOGE("%s vnd_data->lv_disp_sem deinit failed\n", __func__);
            return;
        }
    } else {
        lv_hpdma_memcpy_deinit(vnd_data);

        if (vnd_data->config.output_compress || (vnd_data->config.rotation != ROTATE_NONE && LV_USE_GPU_ROTATE)) {
            vg_lite_free_without_free_data(&lv_src_buf);
            vg_lite_free_without_free_data(&lv_dst_buf);
        }
    }
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

static bk_err_t lvgl_frame_buffer_free_cb(void *frame)
{
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(lv_disp_get_default());

    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return BK_ERR_PARAM;
    }

    if (vnd_data->config.render_mode != RENDER_PARTIAL_MODE) {
        rtos_set_semaphore(&vnd_data->lv_disp_sem);
    } else {
#if CONFIG_LVGL_FRAME_BUFFER_NUM > 1
        lv_vendor_set_ready_frame_buffer(frame);
#endif
    }

    return BK_OK;
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

static void lv_partial_flush_prepare_base(lv_display_t *disp_drv, lv_vnd_data_t *vnd_data, const lv_area_t *area,
                                          uint8_t *px_map, lv_partial_flush_ctx_t *ctx)
{

}

static void lv_partial_flush_get_display_buffer(lv_vnd_data_t *vnd_data, const lv_area_t *area)
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
    uint32_t color_size = lv_color_format_get_size(lv_display_get_color_format(disp_drv));

    if (vnd_data->config.output_compress) {
        uint16_t copy_width = vnd_data->config.disp_width ? vnd_data->config.disp_width : vnd_data->config.width;
        uint16_t copy_height = vnd_data->config.disp_height ? vnd_data->config.disp_height : vnd_data->config.height;
        uint16_t line_bytes = copy_width * color_size;

        lv_hpdma_memcpy_start(vnd_data->disp_buf, vnd_data->copy_buf,
                              line_bytes, copy_height,
                              line_bytes, copy_height,
                              0, 0);
    } else {
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

static void lv_partial_flush_finish(lv_display_t *disp_drv, lv_vnd_data_t *vnd_data, lv_coord_t lv_hor)
{
    if (lv_disp_flush_is_last(disp_drv)) {
        vnd_data->config.flush_cb(vnd_data->config.args, vnd_data->disp_buf, lvgl_frame_buffer_free_cb);
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

static void lv_disp_flush_for_partial_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
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

#if (CONFIG_LV_COLOR_DEPTH == 16 && CONFIG_LV_COLOR_16_SWAP)
    lv_draw_sw_rgb565_swap(px_map, lv_area_get_size(area));
#endif

    if (vnd_data->config.output_compress) {
        lv_partial_flush_get_display_buffer(vnd_data, ctx.area);
        lv_partial_flush_compress(vnd_data, &ctx, px_map);
    } else {
        lv_partial_flush_rotate(disp_drv, vnd_data, area, px_map, &ctx);
        lv_partial_flush_get_display_buffer(vnd_data, ctx.area);
        lv_area_join(&vnd_data->d_area, &vnd_data->d_area, ctx.area);
        lv_partial_flush_copy_to_disp_buf(vnd_data, &ctx);
    }

    lv_partial_flush_finish(disp_drv, vnd_data, ctx.lv_hor);
}

static void lv_disp_flush_for_direct_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    static bool first_flush = true;
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(disp_drv);

    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    #if (CONFIG_LV_COLOR_DEPTH == 16 && CONFIG_LV_COLOR_16_SWAP)
        if (first_flush) {
            lv_draw_sw_rgb565_swap(px_map, lv_area_get_size(area));
        } else {
            uint16_t *color_ptr = (uint16_t *)px_map;
            uint16_t width = lv_area_get_width(area);
            int offset = 0, y = 0;

            offset = area->y1 * LV_HOR_RES + area->x1;
            for (y = area->y1; y <= area->y2; y++) {
                uint16_t *buf16 = color_ptr + offset;
                if ((int)buf16 % 4) {
                    buf16[0] = ((buf16[0] & 0xff00) >> 8) | ((buf16[0] & 0x00ff) << 8);
                    lv_draw_sw_rgb565_swap(buf16 + 1, width - 1);
                } else {
                    lv_draw_sw_rgb565_swap(buf16, width);
                }
                offset += LV_HOR_RES;
            }
        }
    #endif

    if (lv_disp_flush_is_last(disp_drv)) {
        vnd_data->config.flush_cb(vnd_data->config.args, px_map, lvgl_frame_buffer_free_cb);

        if (first_flush) {
            first_flush = false;
        } else {
            bk_err_t ret = rtos_get_semaphore(&vnd_data->lv_disp_sem, 1000);
            if (ret != BK_OK) {
                LOGE("%s rtos_get_semaphore failed\n", __func__);
            }
        }
    }
}

static void lv_disp_flush_for_full_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
#if (CONFIG_LV_COLOR_DEPTH == 16 && CONFIG_LV_COLOR_16_SWAP)
    lv_draw_sw_rgb565_swap(px_map, lv_area_get_size(area));
#endif

    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(disp_drv);

    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    vnd_data->config.flush_cb(vnd_data->config.args, px_map, lvgl_frame_buffer_free_cb);
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(disp_drv);

    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    if (disp_flush_enabled) {
        if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE) {
            lv_disp_flush_for_partial_mode(disp_drv, area, px_map);
        } else if (vnd_data->config.render_mode == RENDER_DIRECT_MODE) {
            lv_disp_flush_for_direct_mode(disp_drv, area, px_map);
        } else {
            lv_disp_flush_for_full_mode(disp_drv, area, px_map);
        }
    }

    lv_disp_flush_ready(disp_drv);
}