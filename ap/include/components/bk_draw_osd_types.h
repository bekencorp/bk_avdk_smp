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
#include <components/bk_gpu_types.h>
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
    const gui_font_digit_struct * font_digit_type;   /**< bkfont/emWin glyph table; set for bkfont */
    uint32_t color;            /**< font color 0x00RRGGBB (alpha comes from the glyph coverage) */
    const void *lv_font;       /**< LVGL font (const lv_font_t*); set INSTEAD of font_digit_type.
                                *   Dispatch is by whichever pointer is non-NULL (bkfont takes precedence). */
    uint8_t scale;             /**< LVGL integer up-scale (0/1 = 1x); ignored for bkfont */
}blend_font_t;

typedef struct 
{
    uint8_t version;               /**< version */
    blend_type_t blend_type;       /**< 0: image, 1:font */
    const char name[MAX_BLEND_NAME_LEN];        /**< image name like "wifi","clock", "weather" */
    uint32_t width;                 /**< IMAGE: bitmap width (required). FONT: sprite box width
                                     *   (0 = auto-size to text extent, recommended).          */
    uint32_t height;                /**< IMAGE: bitmap height (required). FONT: sprite box height
                                     *   (0 = auto-size to text extent).                        */
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
    /* OSD binds this GPU controller and submits composited sprites through the
     * controller's shared overlay (fetched internally). OSD and PIP on the same
     * controller therefore land on the same output frame. */
    bk_gpu_ctlr_handle_t gpu;            /**< bound GPU controller (required) */
    uint16_t panel_w;                    /**< target display width (rotated buffer width) */
    uint16_t panel_h;                    /**< target display height */
    /* OSD content rotation at composite/blit, matching the video display rotation; must equal
     * the pipeline's display rotate_degree.
     * 0 (default): element xpos/ypos are in the final display-buffer space.
     * 90/270: element xpos/ypos are in the pre-rotation viewer space (same frame as the
     *   un-rotated image); each sprite is rotated by this angle into the panel buffer. */
    uint16_t osd_rotate_degree;
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
    /* All render entry points are self-contained sprite/overlay submissions. */
    /* One-shot single element (image or font); takes the next free layer */
    avdk_err_t (*draw_element)(bk_draw_osd_ctlr_handle_t controller, const blend_info_t *info);
    /* One-shot raw font text (LVGL/bkfont); takes the next free layer */
    avdk_err_t (*draw_text)(bk_draw_osd_ctlr_handle_t controller, osd_font_kind_t kind, const void *font,
                            const char *utf8, uint16_t x, uint16_t y, uint32_t argb, uint8_t scale);
    /* Array render: auto-cluster into overlay layers, one tight sprite per cluster.
     * Incremental by default on the dynamic list (list == NULL): only changed clusters re-composited;
     * an explicit list, an add/remove/clear, or a merged layout forces a full repaint. */
    avdk_err_t (*draw_osd_array)(bk_draw_osd_ctlr_handle_t controller, const blend_info_t *list);
    /* Clear this controller's leased layers and reset its local cursor. */
    avdk_err_t (*clear)(bk_draw_osd_ctlr_handle_t controller);
    avdk_err_t (*add_or_update)(bk_draw_osd_ctlr_handle_t controller, const char *name, const char* content);
    avdk_err_t (*remove)(bk_draw_osd_ctlr_handle_t controller, const char *name);
    avdk_err_t (*ioctl)(bk_draw_osd_ctlr_handle_t controller, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3);
    avdk_err_t (*delete)(bk_draw_osd_ctlr_handle_t controller);
}bk_draw_osd_ctlr_t;




#ifdef __cplusplus
}
#endif
