// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <components/avdk_utils/avdk_types.h>
#include <components/avdk_utils/avdk_check.h>
#include <components/avdk_utils/avdk_error.h>
#include <common/avdk_pixel_types.h>   /* bk7259: frame_buffer_t + pixel_format_t (no multimedia/frame_buffer.h) */
#include <components/bk_gpu_types.h>   /* bk_gpu_ctlr_handle_t + bk_pixel_format_t (external GPU for pipeline submit model) */
#include "modules/lcd_font.h"

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************
 * component name: draw_osd
 * description: Public API (open interface)
 *******************************************************************/

#define MAX_BLEND_NAME_LEN    20
#define MAX_BLEND_CONTENT_LEN 31

#ifndef ARGB8888
enum { ARGB8888 = 0 };
#endif

typedef enum {
    OSD_CTLR_CMD_SET_PSRAM_USAGE,        /**< set use psram */
    OSD_CTLR_CMD_GET_PSRAM_USAGE,        /**< get psram usage status */
    OSD_CTLR_CMD_GET_ALL_ASSETS,         /**< get all blend_info */
    OSD_CTLR_CMD_GET_DRAW_INFO,          /**< get current blend osd info */
    OSD_CTLR_CMD_SHRINK,                 /**< release internal icon buffers; next draw will reallocate */
}osd_ioctl_cmd_t;

typedef enum
{
     BLEND_TYPE_IMAGE = 0,               /**< image type */
     BLEND_TYPE_FONT,                    /**< font type */
}blend_type_t;

/**< Font backend: LVGL runtime decode / bk_font (emWin) pre-rendered glyphs */
typedef enum
{
    OSD_FONT_LVGL = 0,                   /**< const lv_font_t * (decoded in bk_osd_lv_font.c; scale supported) */
    OSD_FONT_BKFONT,                     /**< const gui_font_digit_struct * (fixed-size prerendered glyphs) */
}osd_font_kind_t;

typedef struct {
    uint8_t format;             /**< data_format_t, should be ARGB8888                    */
    uint32_t data_len;          /**< ARGB8888 image size, should be: (xsize * ysize * 4) (no used)  */
    const uint8_t *data;        /**< ARGB8888 image data                                  */
}blend_image_t;

typedef struct {
    const gui_font_digit_struct * font_digit_type;   /**< character database (bkfont/emWin) */
    uint32_t color;            /**< font color value used by RGB565 date*/
    /* Current: font assets in blend_info[] support bkfont only. To let LVGL text participate in
     * auto-clustering, extend with { osd_font_kind_t kind; const void *lv_font; uint8_t scale; }
     * and dispatch by kind in the controller (see bk_osd_lv_font.h). */
}blend_font_t;

typedef struct 
{
    uint8_t version;               /**< version */
    blend_type_t blend_type;       /**< 0: image, 1:font */
    const char name[MAX_BLEND_NAME_LEN];        /**< image name like "wifi","clock", "weather" */
    uint32_t width;                 /**< icon width   */
    uint32_t height;                /**< icon height  */
    uint32_t icon_width;                 /**< icon width   */
    uint32_t icon_height;                /**< icon height  */
    uint32_t bg_width;                 /**< background window width   */
    uint32_t bg_height;                /**< background window height  */
    uint16_t xpos;                  /**< icon x pos based on background window */
    uint16_t ypos;                  /**< icon y pos based on background window */
    union
    {
        blend_image_t image;
        blend_font_t font;
    };
}bk_blend_t;


typedef struct 
{
    char name[MAX_BLEND_NAME_LEN];        /**<  name like "wifi","clock", "weather" */
    const bk_blend_t *addr;               /**< the pointer, pointer to the struct */
    char content[MAX_BLEND_CONTENT_LEN];  /**< content like "wifi0", "12:00","v 1.0.0" */
}blend_info_t;

typedef struct{
    blend_info_t *entry;                /**< the pointer, pointer to the struct array */
    size_t size;                        /**< the size of the struct array */
    size_t capacity;                    /**< the capacity of the struct array */
}dynamic_array_t;


typedef struct {
    /* Pipeline submit model: OSD registers composited sprites with this external GPU (SRC_OVER each frame).
     * MIPI and UVC each hold a separate instance bound to their own pipeline GPU handle. */
    bk_gpu_ctlr_handle_t gpu;            /**< bound external pipeline GPU handle (required) */
    uint16_t panel_w;                    /**< target display width (rotated buffer width) */
    uint16_t panel_h;                    /**< target display height */
    bk_pixel_format_t src_format;        /**< sprite format: MIPI=ABGR8888 / UVC=ARGB8888 (upstream channel order) */
    const blend_info_t *blend_assets;    /**<  the pointer, pointer to current blend info, lifetime >= handle */
    const blend_info_t *blend_info;      /**<  initial default display items array, only read at new time */
    bool draw_in_psram;                  /**< legacy field; unused in pipeline model */
} osd_ctlr_config_t;

typedef struct{
    frame_buffer_t *frame;      /**< legacy frame buffer pointer (unused in pipeline model) */
    uint16_t width;             /**< osd draw visible width */
    uint16_t height;            /**< osd draw visible height */
}osd_bg_info_t;

/**< osd controller handle */
typedef struct bk_draw_osd_ctlr *bk_draw_osd_ctlr_handle_t;


typedef struct bk_draw_osd_ctlr
{
    /* Pipeline compositing: all render entry points are one-shot/self-contained (internal sprite/slot/GPU submit). */
    /* One-shot single element (image or font); uses element xpos/ypos/color/content; takes next free slot */
    avdk_err_t (*draw_element)(bk_draw_osd_ctlr_handle_t controller, const blend_info_t *info);
    /* One-shot raw font text (LVGL/bkfont); takes next free slot */
    avdk_err_t (*draw_text)(bk_draw_osd_ctlr_handle_t controller, osd_font_kind_t kind, const void *font,
                            const char *utf8, uint16_t x, uint16_t y, uint32_t argb, uint8_t scale);
    /* Array render: auto-cluster by spatial proximity into GPU slots (<= BK_GPU_BLIT_SLOT_MAX), one tight
     * bounding-box sprite per cluster; slot cursor stops after used clusters. No manual slot/begin/commit. */
    avdk_err_t (*draw_osd_array)(bk_draw_osd_ctlr_handle_t controller, const blend_info_t *list);
    /* Clear registered blits and reset slot cursor */
    avdk_err_t (*clear)(bk_draw_osd_ctlr_handle_t controller);
    avdk_err_t (*add_or_update)(bk_draw_osd_ctlr_handle_t controller, const char *name, const char* content);
    avdk_err_t (*remove)(bk_draw_osd_ctlr_handle_t controller, const char *name);
    avdk_err_t (*ioctl)(bk_draw_osd_ctlr_handle_t controller, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3);
    avdk_err_t (*delete)(bk_draw_osd_ctlr_handle_t controller);
}bk_draw_osd_ctlr_t;




#ifdef __cplusplus
}
#endif
