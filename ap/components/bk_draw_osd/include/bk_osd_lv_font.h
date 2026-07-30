/*
 * Minimal LVGL font format header (no CONFIG_LVGL / lv_conf / draw_buf / cache).
 *
 * For lv_font_conv .c assets: replace `#include "lvgl.h"` with this header and set
 * .get_glyph_dsc / .get_glyph_bitmap to NULL. Glyphs are decoded in bk_osd_lv_font.c.
 * Use only when CONFIG_LVGL is off to avoid redefining lv_font_t.
 */
#ifndef __BK_OSD_LV_FONT_H__
#define __BK_OSD_LV_FONT_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Stubs for macros referenced by lv_font_conv .c assets */
#ifndef LVGL_VERSION_MAJOR
#define LVGL_VERSION_MAJOR 8
#endif
#ifndef LV_VERSION_CHECK
#define LV_VERSION_CHECK(a, b, c) 1
#endif
#ifndef LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_CONST
#endif
#ifndef LV_FONT_SUBPX_NONE
#define LV_FONT_SUBPX_NONE 0
#endif

/* Cmap type (matches lv_font_fmt_txt.h) */
typedef enum {
    LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL,
    LV_FONT_FMT_TXT_CMAP_SPARSE_FULL,
    LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY,
    LV_FONT_FMT_TXT_CMAP_SPARSE_TINY,
} lv_font_fmt_txt_cmap_type_t;

/* Glyph descriptor (small variant, LV_FONT_FMT_TXT_LARGE==0) */
typedef struct {
    uint32_t bitmap_index : 20;
    uint32_t adv_w : 12;
    uint8_t box_w;
    uint8_t box_h;
    int8_t ofs_x;
    int8_t ofs_y;
} lv_font_fmt_txt_glyph_dsc_t;

/* Codepoint-to-glyph-id cmap (LVGL-compatible) */
typedef struct {
    uint32_t range_start;
    uint16_t range_length;
    uint16_t glyph_id_start;
    const uint16_t *unicode_list;
    const void *glyph_id_ofs_list;
    uint16_t list_length;
    lv_font_fmt_txt_cmap_type_t type;
} lv_font_fmt_txt_cmap_t;

/* Kerning tables (unused here; present for .c asset compilation) */
typedef struct {
    const void *glyph_ids;
    const int8_t *values;
    uint32_t pair_cnt   : 30;
    uint32_t glyph_ids_size : 2;
} lv_font_fmt_txt_kern_pair_t;

typedef struct {
    const int8_t *class_pair_values;
    const uint8_t *left_class_mapping;
    const uint8_t *right_class_mapping;
    uint8_t left_class_cnt;
    uint8_t right_class_cnt;
} lv_font_fmt_txt_kern_classes_t;

/* Runtime glyph cache (instantiated by v8 .c assets; unused here) */
typedef struct {
    uint32_t last_letter;
    uint32_t last_glyph_id;
} lv_font_fmt_txt_glyph_cache_t;

/* Font private dsc (LVGL-compatible; used for bitmap lookup/decode) */
typedef struct {
    const uint8_t *glyph_bitmap;
    const lv_font_fmt_txt_glyph_dsc_t *glyph_dsc;
    const lv_font_fmt_txt_cmap_t *cmaps;
    const void *kern_dsc;
    uint16_t kern_scale;
    uint16_t cmap_num       : 9;
    uint16_t bpp            : 4;
    uint16_t kern_classes   : 1;
    uint16_t bitmap_format  : 2;
    uint8_t stride;
    lv_font_fmt_txt_glyph_cache_t *cache;   /* .cache field in v8 .c assets; unused here */
} lv_font_fmt_txt_dsc_t;

/* Opaque callback types (not invoked by this component) */
typedef struct _lv_font_glyph_dsc_t lv_font_glyph_dsc_t;
typedef void lv_draw_buf_t;

typedef struct _lv_font_t lv_font_t;

struct _lv_font_t {
    bool (*get_glyph_dsc)(const lv_font_t *, lv_font_glyph_dsc_t *, uint32_t, uint32_t);
    const void *(*get_glyph_bitmap)(lv_font_glyph_dsc_t *, lv_draw_buf_t *);
    void (*release_glyph)(const lv_font_t *, lv_font_glyph_dsc_t *);
    int32_t line_height;
    int32_t base_line;
    uint8_t subpx : 2;
    uint8_t kerning : 1;
    uint8_t static_bitmap : 1;
    int8_t underline_position;
    int8_t underline_thickness;
    const void *dsc;
    const lv_font_t *fallback;
    void *user_data;
};

/* OSD glyph decode API */

typedef struct {
    uint16_t box_w;     /* glyph bitmap width */
    uint16_t box_h;     /* glyph bitmap height */
    int16_t  ofs_x;     /* x offset from pen */
    int16_t  ofs_y;     /* y offset from baseline (box bottom to baseline) */
    uint16_t adv_w;     /* advance width in pixels (fixed-point >> 4) */
    const uint8_t *bitmap; /* packed bitmap data (bpp bits per pixel) */
    uint8_t  bpp;
    uint8_t  stride;
} osd_glyph_t;

/** Look up glyph for a Unicode codepoint. Returns true if found. */
bool osd_font_get_glyph(const lv_font_t *font, uint32_t cp, osd_glyph_t *out);

/** Decode one glyph pixel to A8 (0..255). */
uint8_t osd_font_glyph_a8(const osd_glyph_t *g, uint16_t x, uint16_t y);

/** Read next UTF-8 codepoint; returns bytes consumed (>=1). */
uint32_t osd_utf8_next(const char *s, uint32_t *cp);

#ifdef __cplusplus
}
#endif

#endif /* __BK_OSD_LV_FONT_H__ */
