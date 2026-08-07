/*
 * Shared emWin (gui_font_digit_struct) font rasterization.
 *
 * The LCD OSD engine (bk_osd_engine.c) and the H264 overlay path (video_osd_h264.c)
 * both turn a UTF-8 string into an ARGB8888 sprite using the same 1/2/4-bpp glyph
 * format. This util is the single source of truth for that decode so the two paths
 * cannot drift apart. Counterpart of bk_osd_lv_font.* for LVGL-format fonts.
 */
#ifndef __BK_OSD_EMWIN_FONT_H__
#define __BK_OSD_EMWIN_FONT_H__

#include <stdint.h>
#include <stddef.h>             /* NULL */
#include "modules/lcd_font.h"   /* gui_font_digit_struct */

#ifdef __cplusplus
extern "C" {
#endif

/** Drawn bounding box in target-buffer coords, half-open range [x0,x1) x [y0,y1). */
typedef struct {
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
} osd_emwin_font_rect_t;

/** Read next UTF-8 codepoint into @cp; returns bytes consumed (>=1). Generic helper shared by
 *  the emWin and (optional) LVGL text paths, kept here so emWin has no LVGL dependency. */
uint32_t osd_utf8_next(const char *s, uint32_t *cp);

/**
 * Pixel extent of @utf8 rendered with the emWin glyph table @tbl, plus a 2px safety
 * margin (matches the sprite sizing both OSD paths expect). Outputs 0 on NULL/empty input.
 */
void osd_emwin_font_text_extent(const gui_font_digit_struct *tbl, const char *utf8,
                                uint16_t *out_w, uint16_t *out_h);

/**
 * Rasterize @utf8 into ARGB8888 target @dst (dst_w x dst_h, row stride = dst_w) at pen
 * origin (x,y). @argb supplies RGB; per-pixel alpha comes from glyph coverage. Writes are
 * clipped to the target. When @out_bbox != NULL it receives the union of drawn glyph rects
 * (clipped to the target; zeroed if nothing was drawn). Returns the final pen x (end advance).
 */
uint16_t osd_emwin_font_blit(uint32_t *dst, uint16_t dst_w, uint16_t dst_h,
                             const gui_font_digit_struct *tbl, const char *utf8,
                             uint16_t x, uint16_t y, uint32_t argb,
                             osd_emwin_font_rect_t *out_bbox);

#ifdef __cplusplus
}
#endif

#endif /* __BK_OSD_EMWIN_FONT_H__ */
