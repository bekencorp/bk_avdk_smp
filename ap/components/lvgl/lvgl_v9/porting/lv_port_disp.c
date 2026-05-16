/**
 * @file lv_port_disp.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <os/os.h>
#include "lv_port_disp_private.h"

#define TAG "LVGL_DISP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

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
static void disp_deinit(lv_vnd_data_t *vnd_data);
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/
volatile bool disp_flush_enabled = true;

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
#if (LV_COLOR_DEPTH == 16 && CONFIG_LV_COLOR_16_SWAP)
    lv_dislay_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
#endif
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

/**********************
 *   STATIC FUNCTIONS
 **********************/
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
        lv_port_disp_partial_init(vnd_data);
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
        lv_port_disp_partial_deinit(vnd_data);
    }
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
