/*
 * osd_engine — reusable OSD compositor (per-instance, no module statics).
 *
 * Composites icons (ARGB8888) and text (LVGL or bk_font) into a transparent ARGB8888
 * sprite, then registers it with an external pipeline GPU (bk_gpu_blit_set, SRC_OVER).
 * Each instance owns its sprite and free-callback closure; MIPI/UVC can run concurrently.
 *
 * Usage: osd_engine_begin() -> osd_engine_put_icon()/put_text() -> osd_engine_commit().
 */
#ifndef __BK_OSD_ENGINE_H__
#define __BK_OSD_ENGINE_H__

#include <stdint.h>
#include <stdbool.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_gpu.h>
#include <common/avdk_pixel_types.h>
#include "components/bk_draw_osd_types.h"   /* bk_blend_t, osd_font_kind_t */
#include "bk_osd_lv_font.h"                    /* lv_font_t; gui_font_digit_struct via modules/lcd_font.h */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct osd_engine *osd_engine_handle_t;

typedef struct {
    bk_gpu_ctlr_handle_t gpu;        /**< Bound external pipeline GPU (required) */
    uint16_t panel_w;                /**< Target display (panel buffer) width */
    uint16_t panel_h;                /**< Target display (panel buffer) height */
    /* OSD content rotation (0/90/270). 0: dst_x/dst_y passed to begin() are panel-buffer coords
     * (legacy). 90/270: they are pre-rotation viewer coords; commit rotates the sprite and maps
     * it into the panel buffer. Must match the video display rotation. */
    uint16_t rotate_degree;
    bk_pixel_format_t src_format;    /**< Sprite source format (MIPI=ABGR8888 / UVC=ARGB8888) */
} osd_engine_config_t;

avdk_err_t osd_engine_new(osd_engine_handle_t *out, const osd_engine_config_t *cfg);
avdk_err_t osd_engine_delete(osd_engine_handle_t eng);

/**
 * @brief Start a frame: allocate a transparent w*h sprite at panel (dst_x, dst_y).
 *        On OOM, shrinks height in steps (alloc-fit); use osd_engine_sprite_h() for actual height.
 */
avdk_err_t osd_engine_begin(osd_engine_handle_t eng, uint16_t w, uint16_t h,
                            uint16_t dst_x, uint16_t dst_y);
uint16_t osd_engine_sprite_w(osd_engine_handle_t eng);
uint16_t osd_engine_sprite_h(osd_engine_handle_t eng);

/**
 * @brief Set GPU blit slot for the next commit (default 0).
 *        Repeated begin/commit to different slots enables multi-corner OSD on one screen.
 */
void osd_engine_set_slot(osd_engine_handle_t eng, uint8_t slot);

/** @brief Copy an ARGB8888 icon into the sprite at (x,y); clips at bounds */
avdk_err_t osd_engine_put_icon(osd_engine_handle_t eng, const bk_blend_t *icon,
                               uint16_t x, uint16_t y);

/**
 * @brief Rasterize UTF-8 text into the current sprite.
 * @param kind  OSD_FONT_LVGL: font is const lv_font_t* (integer scale-up)
 *              OSD_FONT_BKFONT: font is const gui_font_digit_struct* (scale ignored)
 * @param argb  0x00RRGGBB; alpha comes from the glyph
 * @return Next pen_x (absolute); returns x on error
 */
uint16_t osd_engine_put_text(osd_engine_handle_t eng, osd_font_kind_t kind, const void *font,
                             const char *utf8, uint16_t x, uint16_t y, uint32_t argb, uint8_t scale);

/**
 * @brief Measure text bounds without drawing (for one-shot draw_text sprite sizing).
 *        LVGL: w=total advance*scale, h=line_height*scale; bkfont: w=advance, h=max(y_pos+y_size).
 *        Output includes small margin; commit tightens the actual blit rect.
 */
void osd_engine_text_extent(osd_font_kind_t kind, const void *font, const char *utf8,
                            uint8_t scale, uint16_t *out_w, uint16_t *out_h);

/** @brief Submit composed sprite to bound GPU (SRC_OVER); ownership transfers to GPU on success */
avdk_err_t osd_engine_commit(osd_engine_handle_t eng);

/** @brief Clear registered OSD blits on bound GPU (GPU frees old sprites via free callback) */
avdk_err_t osd_engine_clear(osd_engine_handle_t eng);

#ifdef __cplusplus
}
#endif

#endif /* __BK_OSD_ENGINE_H__ */
