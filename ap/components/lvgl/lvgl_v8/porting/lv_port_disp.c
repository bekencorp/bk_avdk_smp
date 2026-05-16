/**
 * @file lv_port_disp.c
 *
 */

#if 1

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_disp.h"
#include "lv_vendor.h"
#include "lv_hpdma.h"

#define TAG "LVGL_DISP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/*********************
 *      DEFINES
 *********************/
#if (LV_COLOR_DEPTH == 32)
#define LV_FRAME_COLOR_SIZE 3
#else
#define LV_FRAME_COLOR_SIZE sizeof(lv_color_t)
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void disp_init(lv_vnd_data_t *vnd_data);

static void disp_deinit(lv_vnd_data_t *vnd_data);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
volatile bool disp_flush_enabled = true;

void bk_lv_port_disp_init(lv_vnd_data_t *vnd_data)
{
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    disp_init(vnd_data);

    static lv_disp_draw_buf_t draw_buf_dsc_2;
    lv_disp_draw_buf_init(&draw_buf_dsc_2, vnd_data->config.draw_buf_2_1,
                          vnd_data->config.draw_buf_2_2, vnd_data->config.draw_pixel_size);

    LOGI("LVGL addr1:%x, addr2:%x, pixel size:%d, fb1:%x, fb2:%x\r\n",
         vnd_data->config.draw_buf_2_1, vnd_data->config.draw_buf_2_2,
         vnd_data->config.draw_pixel_size, vnd_data->config.frame_buffer[0],
         vnd_data->config.frame_buffer[1]);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res = vnd_data->config.width;
    disp_drv.ver_res = vnd_data->config.height;

    if (vnd_data->config.render_mode == RENDER_DIRECT_MODE) {
        disp_drv.full_refresh = 0;
        disp_drv.direct_mode = 1;
    } else if (vnd_data->config.render_mode == RENDER_FULL_MODE) {
        disp_drv.full_refresh = 1;
        disp_drv.direct_mode = 0;
    }

    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf_dsc_2;

#if LV_USE_USER_DATA
    disp_drv.user_data = vnd_data;
#endif

    if (vnd_data->config.rotation != ROTATE_NONE) {
        disp_drv.sw_rotate = 1;
        if (vnd_data->config.rotation == ROTATE_90) {
            disp_drv.rotated = LV_DISP_ROT_90;
        } else if (vnd_data->config.rotation == ROTATE_180) {
            disp_drv.rotated = LV_DISP_ROT_180;
        } else if (vnd_data->config.rotation == ROTATE_270) {
            disp_drv.rotated = LV_DISP_ROT_270;
        }
    }

    lv_disp_drv_register(&disp_drv);
}

void lv_port_disp_deinit(lv_vnd_data_t *vnd_data)
{
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    disp_deinit(vnd_data);
    lv_disp_remove(lv_disp_get_default());
}

static lv_vnd_data_t *lv_get_vnd_data(lv_disp_drv_t *disp_drv)
{
#if LV_USE_USER_DATA
    if (disp_drv != NULL && disp_drv->user_data != NULL) {
        return (lv_vnd_data_t *)disp_drv->user_data;
    }
#endif

    return NULL;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void lv_memcpy_one_line(void *dest_buf, const void *src_buf, uint32_t point_num)
{
    os_memcpy(dest_buf, src_buf, point_num * sizeof(lv_color_t));
}

#if (LV_COLOR_DEPTH == 32)
static void lv_argb8888_to_rgb888_line(uint8_t *dst, const lv_color_t *src, uint32_t point_num)
{
    for (uint32_t i = 0; i < point_num; i++) {
        dst[i * 3 + 0] = src[i].ch.red;
        dst[i * 3 + 1] = src[i].ch.green;
        dst[i * 3 + 2] = src[i].ch.blue;
    }
}
#endif

static void lv_copy_draw_line_to_frame(void *dest_buf, const lv_color_t *src_buf, uint32_t point_num)
{
#if (LV_COLOR_DEPTH == 32)
    lv_argb8888_to_rgb888_line((uint8_t *)dest_buf, src_buf, point_num);
#else
    lv_memcpy_one_line(dest_buf, src_buf, point_num);
#endif
}

static bk_err_t lv_hpdma_copy_area(void *src, void *dst, uint32_t line_bytes, uint32_t height,
                                   uint32_t src_step, uint32_t dst_step, bool wait_finish)
{
    bk_err_t ret = lv_hpdma_memcpy_start(src, dst, line_bytes, height, line_bytes, height,
                                         src_step, dst_step);
    if (ret != BK_OK) {
        LOGE("%s lv_hpdma_memcpy_start failed, ret=%d\n", __func__, ret);
        return ret;
    }

    if (wait_finish) {
        ret = lv_hpdma_memcpy_wait_finish(1000);
        if (ret != BK_OK) {
            LOGE("%s lv_hpdma_memcpy_wait_finish failed, ret=%d\n", __func__, ret);
        }
    }

    return ret;
}

static void lv_copy_draw_buffer_to_frame(void *dst_buf, const lv_color_t *src_buf,
                                         lv_coord_t width, lv_coord_t height)
{
#if (LV_COLOR_DEPTH == 32)
    uint8_t *dst = (uint8_t *)dst_buf;
    const lv_color_t *src = src_buf;

    for (lv_coord_t y = 0; y < height; y++) {
        lv_argb8888_to_rgb888_line(dst, src, width);
        dst += width * LV_FRAME_COLOR_SIZE;
        src += width;
    }
#else
    os_memcpy(dst_buf, src_buf, width * height * sizeof(lv_color_t));
#endif
}

static void disp_init(lv_vnd_data_t *vnd_data)
{
    if (vnd_data->config.render_mode != RENDER_PARTIAL_MODE) {
        bk_err_t ret = rtos_init_semaphore_ex(&vnd_data->lv_disp_sem, 1, 0);
        if (BK_OK != ret) {
            LOGE("%s lv_disp_sem init failed\n", __func__);
            return;
        }
    } else {
        lv_hpdma_memcpy_init(vnd_data);
    }
}

static void disp_deinit(lv_vnd_data_t *vnd_data)
{
    if (vnd_data->config.render_mode != RENDER_PARTIAL_MODE) {
        if (vnd_data->lv_disp_sem != NULL) {
            bk_err_t ret = rtos_deinit_semaphore(&vnd_data->lv_disp_sem);
            if (BK_OK != ret) {
                LOGE("%s lv_disp_sem deinit failed\n", __func__);
                return;
            }
            vnd_data->lv_disp_sem = NULL;
        }
    } else {
        lv_hpdma_memcpy_deinit(vnd_data);
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
    lv_disp_t *disp = lv_disp_get_default();
    if (disp == NULL || disp->driver == NULL) {
        LOGE("%s disp or disp->driver is NULL\n", __func__);
        return BK_FAIL;
    }

    lv_vnd_data_t *vnd_data = lv_get_vnd_data(disp->driver);
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return BK_FAIL;
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

static lv_color_t *lv_update_dual_buffer_with_direct_mode(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *colour_p)
{
    lv_disp_t *disp = _lv_refr_get_disp_refreshing();
    lv_coord_t y, i;
    lv_color_t *buf_cpy;
    lv_coord_t w, hres = lv_disp_get_hor_res(disp);
    lv_area_t *inv_area = NULL;
    int offset = 0;

    if (colour_p == disp_drv->draw_buf->buf1) {
        buf_cpy = disp_drv->draw_buf->buf2;
    } else {
        buf_cpy = disp_drv->draw_buf->buf1;
    }

    for (i = 0; i < disp->inv_p; i++) {
        if (disp->inv_area_joined[i]) {
            continue;
        }

        inv_area = &disp->inv_areas[i];
        w = lv_area_get_width(inv_area);
        offset = inv_area->y1 * hres + inv_area->x1;

        for (y = inv_area->y1; y <= inv_area->y2 && y < disp_drv->ver_res; y++) {
            lv_memcpy_one_line(buf_cpy + offset, colour_p + offset, w);
            offset += hres;
        }
    }

    return buf_cpy;
}

static void lv_partial_copy_to_frame_buffer(lv_vnd_data_t *vnd_data, const lv_area_t *area,
                                            const lv_color_t *color_ptr, lv_coord_t width,
                                            lv_coord_t height, lv_coord_t lv_hor)
{
    uint8_t *dst = (uint8_t *)vnd_data->disp_buf + (area->y1 * lv_hor + area->x1) * LV_FRAME_COLOR_SIZE;

#if (LV_COLOR_DEPTH == 32)
    for (lv_coord_t y = 0; y < height; y++) {
        lv_copy_draw_line_to_frame(dst, color_ptr, width);
        dst += lv_hor * LV_FRAME_COLOR_SIZE;
        color_ptr += width;
    }
#else
    uint32_t line_bytes = width * sizeof(lv_color_t);
    uint32_t dst_step = (lv_hor - width) * sizeof(lv_color_t);
    lv_hpdma_copy_area((void *)color_ptr, dst, line_bytes, height, 0, dst_step, true);
#endif
}

static void lv_partial_copy_last_frame(lv_vnd_data_t *vnd_data, lv_coord_t lv_hor)
{
    uint32_t area_width = lv_area_get_width(&vnd_data->d_area);
    uint32_t area_height = lv_area_get_height(&vnd_data->d_area);
    uint32_t line_bytes = area_width * LV_FRAME_COLOR_SIZE;
    uint32_t step_bytes = (lv_hor - area_width) * LV_FRAME_COLOR_SIZE;
    void *src_start = (uint8_t *)vnd_data->disp_buf + (vnd_data->d_area.y1 * lv_hor + vnd_data->d_area.x1) * LV_FRAME_COLOR_SIZE;
    void *dst_start = (uint8_t *)vnd_data->copy_buf + (vnd_data->d_area.y1 * lv_hor + vnd_data->d_area.x1) * LV_FRAME_COLOR_SIZE;

    lv_hpdma_copy_area(src_start, dst_start, line_bytes, area_height, step_bytes, step_bytes, false);
}

static void lv_disp_flush_for_partial_mode(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    lv_vnd_data_t *vnd_data = lv_get_vnd_data(disp_drv);
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    lv_coord_t lv_hor = disp_drv->hor_res;
    const lv_color_t *color_ptr = color_p;
    lv_coord_t width = lv_area_get_width(area);
    lv_coord_t height = lv_area_get_height(area);
    lv_area_t dst_area = *area;

    lv_get_display_buffer(vnd_data, &dst_area);
    _lv_area_join(&vnd_data->d_area, &vnd_data->d_area, &dst_area);
    lv_partial_copy_to_frame_buffer(vnd_data, &dst_area, color_ptr, width, height, lv_hor);

    if (lv_disp_flush_is_last(disp_drv)) {
        vnd_data->config.flush_cb(vnd_data->config.args, vnd_data->disp_buf, lvgl_frame_buffer_free_cb);
        vnd_data->lv_new_frame_flag = true;

#if CONFIG_LVGL_FRAME_BUFFER_NUM > 1
        if (vnd_data->copy_buf == NULL) {
            vnd_data->copy_buf = lv_wait_ready_frame_buffer();
        }
        lv_partial_copy_last_frame(vnd_data, lv_hor);
#endif
    }
}

static void lv_disp_flush_for_direct_mode(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    static bool first_flush = true;
    void *frame_buffer = NULL;

    lv_vnd_data_t *vnd_data = lv_get_vnd_data(disp_drv);
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    if (lv_disp_flush_is_last(disp_drv)) {
        if (color_p == vnd_data->config.draw_buf_2_1) {
            frame_buffer = vnd_data->config.frame_buffer[0];
        } else {
            frame_buffer = vnd_data->config.frame_buffer[1];
        }

        vnd_data->config.flush_cb(vnd_data->config.args, frame_buffer, lvgl_frame_buffer_free_cb);

        if (first_flush) {
            first_flush = false;
        } else {
            bk_err_t ret = rtos_get_semaphore(&vnd_data->lv_disp_sem, 1000);
            if (ret != BK_OK) {
                LOGE("%s rtos_get_semaphore failed\n", __func__);
            }
        }

        lv_update_dual_buffer_with_direct_mode(disp_drv, area, color_p);
    }
}

static void lv_disp_flush_for_full_mode(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    static bool first_flush = true;
    void *frame_buffer = NULL;

    lv_vnd_data_t *vnd_data = lv_get_vnd_data(disp_drv);
    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        return;
    }

    if (color_p == vnd_data->config.draw_buf_2_1) {
        frame_buffer = vnd_data->config.frame_buffer[0];
    } else {
        frame_buffer = vnd_data->config.frame_buffer[1];
    }

    vnd_data->config.flush_cb(vnd_data->config.args, frame_buffer, lvgl_frame_buffer_free_cb);

    if (first_flush) {
        first_flush = false;
    } else {
        bk_err_t ret = rtos_get_semaphore(&vnd_data->lv_disp_sem, 1000);
        if (ret != BK_OK) {
            LOGE("%s rtos_get_semaphore failed\n", __func__);
        }
    }
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    lv_vnd_data_t *vnd_data = lv_get_vnd_data(disp_drv);

    if (vnd_data == NULL) {
        LOGE("%s vnd_data is NULL\n", __func__);
        lv_disp_flush_ready(disp_drv);
        return;
    }

    if (disp_flush_enabled) {
        if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE) {
            lv_disp_flush_for_partial_mode(disp_drv, area, color_p);
        } else if (vnd_data->config.render_mode == RENDER_DIRECT_MODE) {
            lv_disp_flush_for_direct_mode(disp_drv, area, color_p);
        } else {
            lv_disp_flush_for_full_mode(disp_drv, area, color_p);
        }
    }

    lv_disp_flush_ready(disp_drv);
}

#else
typedef int keep_pedantic_happy;
#endif
