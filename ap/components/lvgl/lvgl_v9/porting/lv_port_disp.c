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

/* Compressed output stores the frame as VG_LITE_DEC_HV_SAMPLE 16x4 tiles.
 * A partial flush blits/clears the raw dirty rect into this tiled surface; if
 * the rect edge falls inside a tile, the rest of that tile (static content) is
 * disturbed and shows as stripes -- most visible while scrolling solid cards.
 * Round every invalidated area out to whole tiles so each touched tile is fully
 * re-rendered. The tile grid lives in physical (post-rotation) space, so we
 * rotate the area to physical, align there, then rotate back to logical. */
#define LV_COMPRESS_TILE_W 16
#define LV_COMPRESS_TILE_H 4

static int32_t disp_align_down(int32_t v, int32_t a)
{
    return v & ~(a - 1);
}

static int32_t disp_align_up(int32_t v, int32_t a)
{
    return (v + a - 1) & ~(a - 1);
}

static void disp_invalidate_area_rounder(lv_event_t *e)
{
    lv_vnd_data_t *vnd_data = (lv_vnd_data_t *)lv_event_get_user_data(e);
    lv_area_t *area = (lv_area_t *)lv_event_get_param(e);
    lv_display_t *disp = (lv_display_t *)lv_event_get_target(e);

    if (vnd_data == NULL || area == NULL || disp == NULL ||
        !vnd_data->config.output_compress) {
        return;
    }

    lv_display_rotation_t rot = lv_display_get_rotation(disp);

    if (area->x1 == 0 && area->x2 == 0 && area->y1 == 0) {
        int32_t h = lv_area_get_height(area);
        int32_t row_align = (rot == LV_DISPLAY_ROTATION_90 || rot == LV_DISPLAY_ROTATION_270) ?
                            LV_COMPRESS_TILE_W : LV_COMPRESS_TILE_H;
        if (h > row_align) {
            area->y2 = disp_align_down(h, row_align) - 1;
        }
        return;
    }

    /* lv_display_rotate_area() uses the (un-rotated) disp->hor_res / ver_res,
     * which equal the created resolution == config.width / config.height. */
    int32_t int_hor = vnd_data->config.width;
    int32_t int_ver = vnd_data->config.height;

    /* 1) logical -> physical */
    lv_area_t phys = *area;
    lv_display_rotate_area(disp, &phys);

    /* 2) expand to whole 16x4 tiles in physical space */
    phys.x1 = disp_align_down(phys.x1, LV_COMPRESS_TILE_W);
    phys.y1 = disp_align_down(phys.y1, LV_COMPRESS_TILE_H);
    phys.x2 = disp_align_up(phys.x2 + 1, LV_COMPRESS_TILE_W) - 1;
    phys.y2 = disp_align_up(phys.y2 + 1, LV_COMPRESS_TILE_H) - 1;

    if (phys.x1 < 0) phys.x1 = 0;
    if (phys.y1 < 0) phys.y1 = 0;
    if (phys.x2 > vnd_data->config.disp_width - 1) phys.x2 = vnd_data->config.disp_width - 1;
    if (phys.y2 > vnd_data->config.disp_height - 1) phys.y2 = vnd_data->config.disp_height - 1;

    /* 3) physical -> logical (inverse of lv_display_rotate_area) */
    lv_area_t out = phys;
    switch (rot) {
    case LV_DISPLAY_ROTATION_90:
        out.y1 = phys.x1;
        out.y2 = phys.x2;
        out.x1 = int_ver - phys.y2 - 1;
        out.x2 = int_ver - phys.y1 - 1;
        break;
    case LV_DISPLAY_ROTATION_270:
        out.x1 = phys.y1;
        out.x2 = phys.y2;
        out.y1 = int_hor - phys.x2 - 1;
        out.y2 = int_hor - phys.x1 - 1;
        break;
    case LV_DISPLAY_ROTATION_180:
        out.x1 = int_hor - phys.x2 - 1;
        out.x2 = int_hor - phys.x1 - 1;
        out.y1 = int_ver - phys.y2 - 1;
        out.y2 = int_ver - phys.y1 - 1;
        break;
    default:
        out = phys;
        break;
    }

    /* 4) clamp to the (rotation-aware) logical resolution and write back */
    int32_t app_w = lv_display_get_horizontal_resolution(disp);
    int32_t app_h = lv_display_get_vertical_resolution(disp);
    if (out.x1 < 0) out.x1 = 0;
    if (out.y1 < 0) out.y1 = 0;
    if (out.x1 > app_w - 1) out.x1 = app_w - 1;
    if (out.y1 > app_h - 1) out.y1 = app_h - 1;
    if (out.x2 > app_w - 1) out.x2 = app_w - 1;
    if (out.y2 > app_h - 1) out.y2 = app_h - 1;
    if (out.x2 < out.x1) out.x2 = out.x1;
    if (out.y2 < out.y1) out.y2 = out.y1;

    *area = out;
}

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
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
#endif
    lv_display_set_flush_cb(disp, disp_flush);
    lv_display_set_user_data(disp, vnd_data);
    lv_display_set_rotation(disp, vnd_data->config.rotation);

    /* Compressed output uses a 16x4 tiled frame buffer; round invalidated
     * areas out to whole tiles so partial flushes never disturb the static
     * part of a boundary tile (otherwise visible as stripes on scroll). */
    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE &&
        vnd_data->config.output_compress) {
        lv_display_add_event_cb(disp, disp_invalidate_area_rounder,
                                LV_EVENT_INVALIDATE_AREA, vnd_data);
    }

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

    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE &&
        vnd_data->config.rotation != ROTATE_NONE &&
        !vnd_data->config.output_compress) {
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

    if (vnd_data->config.render_mode == RENDER_PARTIAL_MODE &&
        !vnd_data->config.output_compress) {
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
