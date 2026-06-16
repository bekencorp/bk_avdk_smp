/**
 * @file lvgl_vendor.h
 */

#ifndef LVGL_VENDOR_H
#define LVGL_VENDOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "components/media_types.h"
#include "components/bk_display.h"
#include <driver/hpdma.h>
#include <driver/hal/hal_hpdma_types.h>


#if CONFIG_LVGL_USE_GPU_ROTATE
#define LV_USE_GPU_ROTATE    1
#else
#define LV_USE_GPU_ROTATE    0
#endif

#if CONFIG_LVGL_V8
    #define bk_color_t    lv_color_t
#else
#if (LV_COLOR_DEPTH == 16)
    #define bk_color_t    lv_color16_t
#elif (LV_COLOR_DEPTH == 24)
    #define bk_color_t    lv_color_t
#elif (LV_COLOR_DEPTH == 32)
    #define bk_color_t    lv_color32_t
#endif
#endif

typedef enum {
    STATE_INIT,
    STATE_RUNNING,
    STATE_STOP
} lvgl_task_state_t;

typedef enum {
    RENDER_PARTIAL_MODE,            /**< The buffer is smaller than the screen size and lvgl will render the screen in samller parts. */
    RENDER_DIRECT_MODE,             /**< The buffer has to be screen sized but lvgl will render into the correct location of the buffer. */
    RENDER_FULL_MODE,               /**< The buffer has to be screen sized and lvgl always redraw the whole screen even if only 1 pixel has been changed. Not recommended.*/
} lvgl_render_mode_t;

typedef struct {
    lv_coord_t width;               /**< Horizontal resolution.*/
    lv_coord_t height;              /**< Vertical resolution.*/
    lvgl_render_mode_t render_mode; /**< Partial mode use sram as draw buffer, other mode use psram as draw_buffer. */
    rott_angle_t rotation;        /**< 0: 0 degree, 1: 90 degree, 2: 180 degree, 3: 270 degree. */
    void *args;
    void (*flush_cb)(void *args, void *frame_buffer, int (*cb)(void *args));

    /**< The following parameters do not need to be configured by default. */
    uint32_t draw_pixel_size;       /**< v8 size is in pixel, v9 size is in byte. It's recommended to choose size of 1/10 screen sized*/
    void *draw_buf_2_1;             /**< LVGL draw buffer 1. */
    void *draw_buf_2_2;             /**< LVGL draw buffer 2. Not used by default and only is used when requires high performance. It will cost more memory. */
    void *frame_buffer[CONFIG_LVGL_FRAME_BUFFER_NUM];    /**< LVGL frame buffers */

    bool output_compress;          /**< Just for partial mode and must use the GPU */
    uint16_t disp_width;           /**< No need config, only used for output compress enable */
    uint16_t disp_height;          /**< No need config, only used for output compress enable */
} lv_vnd_config_t;

typedef struct {
    lv_vnd_config_t config;
    void *rotate_buffer;
    void *disp_buf;
    void *copy_buf;
    bool lv_new_frame_flag;
    beken_semaphore_t lv_disp_sem;
    void *link_dma_list_table;
    hpdma_id_t lv_hpdma_id;
    beken_semaphore_t lv_hpdma_sem;
    bool lv_hpdma_in_use;
    lv_area_t d_area;
} lv_vnd_data_t;

typedef struct {
    uint32_t param0;
    uint32_t param1;
} lv_frame_msg_t;


void *lv_vendor_malloc(size_t size);

void *lv_vendor_realloc(void *ptr, size_t size);

void lv_vendor_free(void *ptr);

bk_err_t lv_vendor_init(lv_vnd_config_t *config);

void lv_vendor_deinit(void);

void lv_vendor_start(void);

void lv_vendor_stop(void);

void lv_vendor_disp_lock(void);

void lv_vendor_disp_unlock(void);

/**
 * Register the shared bk_gpu handle before LVGL uses GPU operations.
 * Leave it unset when LVGL does not share bk_gpu with other modules.
 */
void lv_vendor_gpu_handle_set(void *gpu_handle);

bool lv_vendor_gpu_lock(void);

void lv_vendor_gpu_unlock(bool locked);

void *lv_vendor_get_ready_frame_buffer(void);

void lv_vendor_set_ready_frame_buffer(void *frame_buffer);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LVGL_VENDOR_H*/

