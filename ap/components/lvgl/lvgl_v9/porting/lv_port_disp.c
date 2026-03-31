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
#include <driver/lcd_types.h>
#include "../src/misc/lv_area_private.h"
#include <driver/hpdma.h>
#include <driver/hal/hal_hpdma_types.h>

#define TAG "LVGL_DISP"

#if defined(CONFIG_VG_LITE_GPU) && defined(CONFIG_LV_USE_DRAW_VG_LITE)
#error "CONFIG_VG_LITE_GPU and CONFIG_LV_USE_DRAW_VG_LITE cannot be enabled at the same time"
#endif


#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#if CONFIG_LVGL_GPU_ROTATE_ENABLE
#define LV_USE_GPU_ROTATE    1
#endif

#if LV_USE_GPU_ROTATE
#include <modules/vg_lite_gpu/vg_lite.h>

static vg_lite_buffer_t lv_rotate_buf;
static vg_lite_buffer_t lv_draw_buf;
static vg_lite_matrix_t lv_matrix;
#endif
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(lv_vnd_data_t *vnd_data);

static void disp_deinit(void);

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
static void *rotate_buffer = NULL;
static void *disp_buf = NULL;

#if CONFIG_LVGL_FRAME_BUFFER_NUM > 1
static void *copy_buf = NULL;
#endif

static bool lv_new_frame_flag = true;
static beken_semaphore_t lv_disp_sem = NULL;
static void *link_dma_list_table = NULL;
static hpdma_id_t lv_hpdma_id = HPDMA_ID_MAX;
static beken_semaphore_t lv_hpdma_sem = NULL;
static bool lv_hpdma_in_use = false;

void bk_lv_port_disp_init(lv_vnd_data_t *vnd_data)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init(vnd_data);

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = NULL;

    disp = lv_display_create(vnd_data->config.width, vnd_data->config.height);
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_user_data(disp, vnd_data);
    lv_display_set_rotation(disp, vnd_data->config.rotation);

    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE) {
        lv_display_set_buffers(disp,
            vnd_data->config.draw_buf_2_1,
            vnd_data->config.draw_buf_2_2,
            vnd_data->config.draw_pixel_size,
            LV_DISPLAY_RENDER_MODE_PARTIAL);
    } else if (vnd_data->config.render_mode == RENDER_DIRECT_MODE) {
        lv_display_set_buffers(disp,
            vnd_data->config.draw_buf_2_1,
            vnd_data->config.draw_buf_2_2,
            vnd_data->config.draw_pixel_size,
            LV_DISPLAY_RENDER_MODE_DIRECT);
    } else {
        lv_display_set_buffers(disp,
            vnd_data->config.draw_buf_2_1,
            vnd_data->config.draw_buf_2_2,
            vnd_data->config.draw_pixel_size,
            LV_DISPLAY_RENDER_MODE_FULL);
    }

    LOGI("LVGL addr1:%x, addr2:%x, pixel size:%d, fb1:%x, fb2:%x\r\n", vnd_data->config.draw_buf_2_1, vnd_data->config.draw_buf_2_2,
                                        vnd_data->config.draw_pixel_size, vnd_data->config.frame_buffer[0], vnd_data->config.frame_buffer[1]);

    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE && (vnd_data->config.rotation != ROTATE_NONE || vnd_data->config.compress_enable)) {

    #if CONFIG_LV_USE_DRAW_VG_LITE
        rotate_buffer = lv_vendor_malloc(vnd_data->config.draw_pixel_size + 64);
        if (rotate_buffer == NULL) {
            LOGE("%s lvgl rotate buffer malloc fail!\n", __func__);
            return;
        }
        rotate_buffer = (void *)(((uint32_t)rotate_buffer + 63) & ~63);
    #else
        #if LV_USE_GPU_ROTATE
            rotate_buffer = lv_vendor_malloc(vnd_data->config.draw_pixel_size + 64);
        #else
            rotate_buffer = lv_vendor_malloc(vnd_data->config.draw_pixel_size);
        #endif
        if (rotate_buffer == NULL) {
            LOGE("%s lvgl rotate buffer malloc fail!\n", __func__);
            return;
        }
        #if LV_USE_GPU_ROTATE
            rotate_buffer = (void *)(((uint32_t)rotate_buffer + 63) & ~63);
        #endif
    #endif

    }
}

void lv_port_disp_deinit(void)
{
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(lv_disp_get_default());
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE && (vnd_data->config.rotation != ROTATE_NONE || vnd_data->config.compress_enable)) {
        if (rotate_buffer) {
            os_free(rotate_buffer);
            rotate_buffer = NULL;
        }
    }

    lv_display_delete(lv_disp_get_default());

    disp_deinit();
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
    if (lv_hpdma_sem != NULL) {
        rtos_set_semaphore(&lv_hpdma_sem);
    }
}

static void lv_hpdma_memcpy_init(void)
{
    link_dma_list_table = bk_hpdma_link_init(1);
    if (link_dma_list_table == NULL)
    {
        LOGE("%s, %d bk_hpdma_link_init failed\n", __func__, __LINE__);
        return;
    }

    lv_hpdma_id = bk_hpdma_alloc(HPDMA_DEV_DTCM);
    if (lv_hpdma_id >= HPDMA_ID_MAX)
    {
        LOGE("%s, %d bk_hpdma_alloc failed\n", __func__, __LINE__);
        bk_hpdma_link_deinit(link_dma_list_table);
        link_dma_list_table = NULL;
        return;
    }

    bk_err_t ret = rtos_init_semaphore_ex(&lv_hpdma_sem, 1, 0);
    if (BK_OK != ret) {
        LOGE("%s lv_hpdma_sem init failed\n", __func__);
        bk_hpdma_free(HPDMA_DEV_DTCM, lv_hpdma_id);
        lv_hpdma_id = HPDMA_ID_MAX;
        bk_hpdma_link_deinit(link_dma_list_table);
        link_dma_list_table = NULL;
        return;
    }
}

static void lv_hpdma_memcpy_deinit(void)
{
    bk_hpdma_stop(lv_hpdma_id);
    bk_hpdma_disable_finish_interrupt(lv_hpdma_id);

    bk_hpdma_link_deinit(link_dma_list_table);
    link_dma_list_table = NULL;

    bk_hpdma_free(HPDMA_DEV_DTCM, lv_hpdma_id);
    lv_hpdma_id = HPDMA_ID_MAX;

    if (lv_hpdma_sem != NULL) {
        rtos_deinit_semaphore(&lv_hpdma_sem);
        lv_hpdma_sem = NULL;
    }

    lv_hpdma_in_use = false;
}

static bk_err_t lv_hpdma_memcpy_start(void *src_buf, void *dst_buf, uint16_t src_xsize, uint16_t src_ysize, uint16_t dst_xsize, uint16_t dst_ysize, uint16_t src_step, uint16_t dst_step)
{
    hpdma_link_config_t config = {0};

    if ((link_dma_list_table == NULL) || (lv_hpdma_id >= HPDMA_ID_MAX) || (lv_hpdma_sem == NULL)) {
        LOGE("%s hpdma not ready: table=%p id=%d sem=%p\n", __func__, link_dma_list_table, lv_hpdma_id, lv_hpdma_sem);
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

    bk_err_t ret = bk_hpdma_link_set_desc(link_dma_list_table, 0, &config);
    if (ret != BK_OK) {
        LOGE("%s bk_hpdma_link_set_desc failed, ret=%d\n", __func__, ret);
        return ret;
    }

    bk_hpdma_set_dest_burst_len(lv_hpdma_id, 0x03);
    bk_hpdma_set_src_burst_len(lv_hpdma_id, 0x03);

    bk_hpdma_register_isr(lv_hpdma_id, NULL, NULL, lv_hpdma_transfer_complete_callback, NULL);
    bk_hpdma_enable_finish_interrupt(lv_hpdma_id);

    ret = bk_hpdma_link_transfer(lv_hpdma_id, link_dma_list_table);
    if (ret != BK_OK) {
        LOGE("%s bk_hpdma_link_transfer failed, ret=%d\n", __func__, ret);
        return ret;
    }

    lv_hpdma_in_use = true;

    return BK_OK;
}

static bk_err_t lv_hpdma_memcpy_wait_finish(uint32_t timeout_ms)
{
    bk_err_t ret = BK_OK;

    if (lv_hpdma_in_use && (lv_hpdma_sem != NULL)) {
        ret = rtos_get_semaphore(&lv_hpdma_sem, timeout_ms);
        if (ret != BK_OK) {
            LOGE("%s rtos_get_semaphore failed\n", __func__);
            return ret;
        }
        lv_hpdma_in_use = false;
    }

    return ret;
}

/*Initialize your display and the required peripherals.*/
static void disp_init(lv_vnd_data_t *vnd_data)
{
    /*You code here*/
    if (vnd_data->config.render_mode != RENDER_PARTIAL_MODE) {
        bk_err_t ret = rtos_init_semaphore_ex(&lv_disp_sem, 1, 0);
        if (BK_OK != ret) {
            LOGE("%s lv_disp_sem init failed\n", __func__);
            return;
        }
    } else {
        lv_hpdma_memcpy_init();

        #if LV_USE_GPU_ROTATE
            os_memset(&lv_rotate_buf, 0, sizeof(vg_lite_buffer_t));
            lv_rotate_buf.format = VG_LITE_BGR565;
            lv_rotate_buf.compress_mode = VG_LITE_DEC_DISABLE;

            os_memset(&lv_draw_buf, 0, sizeof(vg_lite_buffer_t));
            lv_draw_buf.format = VG_LITE_BGR565;

            lv_draw_buf.compress_mode = VG_LITE_DEC_DISABLE;

            if (vnd_data->config.compress_enable) {
                lv_rotate_buf.format = VG_LITE_BGRA8888;
                lv_rotate_buf.tiled = 1;
                lv_rotate_buf.compress_mode = VG_LITE_DEC_HV_SAMPLE;
            }

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
        #endif
    }
}

static void disp_deinit(void)
{
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(lv_disp_get_default());
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    if (vnd_data->config.render_mode != RENDER_PARTIAL_MODE) {
        bk_err_t ret = rtos_deinit_semaphore(&lv_disp_sem);
        if (BK_OK != ret) {
            LOGE("%s lv_disp_sem deinit failed\n", __func__);
            return;
        }
    } else {
        lv_hpdma_memcpy_deinit();

        #if LV_USE_GPU_ROTATE
            vg_lite_free_without_free_data(&lv_draw_buf);
            vg_lite_free_without_free_data(&lv_rotate_buf);
            vg_lite_close();
        #endif
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
        rtos_set_semaphore(&lv_disp_sem);
    } else {
#if CONFIG_LVGL_FRAME_BUFFER_NUM > 1
        lv_vendor_set_ready_frame_buffer(frame);
#endif
    }

    return BK_OK;
}

static lv_area_t d_area;

static void lv_disp_flush_for_partial_mode(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    lv_coord_t lv_hor = 0;
    lv_coord_t lv_stride = 0;
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_display_get_user_data(disp_drv);

    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    if (vnd_data->config.rotation == ROTATE_NONE || vnd_data->config.rotation == ROTATE_180) {
        lv_hor = LV_HOR_RES;
    } else {
        lv_hor = LV_VER_RES;
    }

#if (CONFIG_LV_COLOR_DEPTH == 16 && CONFIG_LV_COLOR_16_SWAP)
    lv_draw_sw_rgb565_swap(px_map, lv_area_get_size(area));
#endif
    bk_color_t *color_ptr = NULL;
    int y = 0, offset = 0;

    lv_coord_t width = lv_area_get_width(area);
    lv_coord_t height = lv_area_get_height(area);

    if (vnd_data->config.rotation != ROTATE_NONE || vnd_data->config.compress_enable) {
        #if LV_USE_GPU_ROTATE
            lv_draw_buf.width = width;
            lv_draw_buf.height = height;
            vg_lite_allocate_with_data(&lv_draw_buf, px_map, NULL, NULL, NULL);

            if (vnd_data->config.rotation == ROTATE_90 || vnd_data->config.rotation == ROTATE_270) {
                lv_rotate_buf.width = height;
                lv_rotate_buf.height = width;
            } else {
                lv_rotate_buf.width = width;
                lv_rotate_buf.height = height;
            }
            vg_lite_allocate_with_data(&lv_rotate_buf, rotate_buffer, NULL, NULL, NULL);

            switch (vnd_data->config.rotation) {
                case ROTATE_90:
                    lv_matrix.m[0][2] = 0.0f;
                    lv_matrix.m[1][2] = width;
                    break;
                case ROTATE_270:
                    lv_matrix.m[0][2] = height;
                    lv_matrix.m[1][2] = 0.0f;
                    break;
                case ROTATE_180:
                    lv_matrix.m[0][2] = width;
                    lv_matrix.m[1][2] = height;
                    break;
                case ROTATE_NONE:
                    lv_matrix.m[0][2] = 0.0f;
                    lv_matrix.m[1][2] = 0.0f;
                    break;

                    default:
                    break;
            }

            vg_lite_blit(&lv_rotate_buf, &lv_draw_buf, &lv_matrix, VG_LITE_BLEND_NONE, 0, VG_LITE_FILTER_POINT);
            vg_lite_finish();
        #else
            lv_color_format_t cf = lv_display_get_color_format(disp_drv);
            uint32_t w_stride = lv_draw_buf_width_to_stride(width, cf);
            uint32_t h_stride = lv_draw_buf_width_to_stride(height, cf);

            if (vnd_data->config.rotation == ROTATE_180) {
                lv_draw_sw_rotate(px_map, rotate_buffer, width, height, w_stride, w_stride, vnd_data->config.rotation, cf);
            } else {
                lv_draw_sw_rotate(px_map, rotate_buffer, width, height, w_stride, h_stride, vnd_data->config.rotation, cf);
            }
        #endif
        color_ptr = rotate_buffer;

        lv_area_t rotated_area = *area;
        lv_display_rotate_area(disp_drv, &rotated_area);
        area = &rotated_area;

        if (vnd_data->config.rotation != ROTATE_180) {
            width = lv_area_get_width(area);
            height = lv_area_get_height(area);
        }

        uint32_t dst_stride_bytes = lv_draw_buf_width_to_stride(width, cf);
        if (vnd_data->config.compress_enable) {
            lv_stride = dst_stride_bytes;
        } else {
            lv_stride = dst_stride_bytes / sizeof(bk_color_t);
        }
    } else {
        color_ptr = (bk_color_t *)px_map;
        lv_stride = lv_display_get_buf_active(disp_drv)->header.stride / sizeof(bk_color_t);
    }

#if (CONFIG_LVGL_FRAME_BUFFER_NUM > 1)
    lv_hpdma_memcpy_wait_finish(BEKEN_WAIT_FOREVER);

    if (lv_new_frame_flag) {
        if (disp_buf == NULL) {
            do {
                disp_buf = lv_vendor_get_ready_frame_buffer();
                if (disp_buf != NULL) {
                    break;
                }
            } while (disp_buf == NULL);
        } else {
            disp_buf = copy_buf;
            copy_buf = NULL;
        }
        lv_new_frame_flag = false;
        d_area = *area;
    }
#else
    if (lv_new_frame_flag) {
        disp_buf = vnd_data->config.frame_buffer[0];
        lv_new_frame_flag = false;
        d_area = *area;
    }
#endif

    lv_area_join(&d_area, &d_area, area);

    if (vnd_data->config.draw_buf_2_2) {
        // lv_dma2d_memcpy_double_draw_buffer(color_ptr, width, height, disp_buf->frame, area->x1, area->y1);
    } else {
        offset = area->y1 * lv_hor + area->x1;
        for (y = area->y1; y <= area->y2; y++) {
            if (vnd_data->config.compress_enable) {
                os_memcpy((uint8_t *)disp_buf + offset, color_ptr, width);
                offset += lv_hor;
                color_ptr = (bk_color_t*)((uint8_t*)color_ptr + lv_stride);
            } else {
                lv_memcpy_one_line((uint8_t *)disp_buf + offset * sizeof(bk_color_t), color_ptr, width);
                offset += lv_hor;
                color_ptr += lv_stride;
            }
        }
    }

    if (lv_disp_flush_is_last(disp_drv)) {
        #if (!CONFIG_LV_USE_DEMO_BENCHMARK)
            if (vnd_data->config.draw_buf_2_2) {
                // lv_dma2d_memcpy_wait_transfer_finish();
            }
        #endif

        vnd_data->config.flush_cb(vnd_data->config.args, disp_buf, lvgl_frame_buffer_free_cb);
        lv_new_frame_flag = true;

#if (CONFIG_LVGL_FRAME_BUFFER_NUM > 1)
        if (copy_buf == NULL) {
            do {
                copy_buf = lv_vendor_get_ready_frame_buffer();
                if (copy_buf != NULL) {
                    break;
                }
            } while(copy_buf == NULL);
        }

    #if 0
        offset = d_area.y1 * lv_hor + d_area.x1;
        for (y = d_area.y1; y <= d_area.y2; y++) {
            os_memcpy((uint16_t *)copy_buf + offset, (uint16_t *)disp_buf + offset, lv_area_get_width(&d_area) * 2);
            offset += lv_hor;
        }
    #else
        void *src_start = (uint8_t *)disp_buf + (d_area.y1 * lv_hor + d_area.x1) * sizeof(bk_color_t);
        void *dst_start = (uint8_t *)copy_buf + (d_area.y1 * lv_hor + d_area.x1) * sizeof(bk_color_t);
        lv_hpdma_memcpy_start(src_start, dst_start,
                              lv_area_get_width(&d_area) * sizeof(bk_color_t), lv_area_get_height(&d_area),
                              lv_area_get_width(&d_area) * sizeof(bk_color_t), lv_area_get_height(&d_area),
                              (lv_hor - lv_area_get_width(&d_area)) * sizeof(bk_color_t),
                              (lv_hor - lv_area_get_width(&d_area)) * sizeof(bk_color_t));
    #endif
#endif
    }
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
            bk_err_t ret = rtos_get_semaphore(&lv_disp_sem, 1000);
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