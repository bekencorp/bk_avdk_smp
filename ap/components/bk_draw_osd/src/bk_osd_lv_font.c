/*
 * Minimal LVGL fmt_txt font decoder + rasterizer (no CONFIG_LVGL).
 * Reads glyphs from lv_font_t->dsc (lv_font_fmt_txt_dsc_t); supports uncompressed bpp 1/2/4/8.
 * text_extent/blit mirror bk_osd_emwin_font so the LCD engine and the H264 overlay path share
 * the same shape for both font formats.
 */
#include "bk_osd_lv_font.h"
#include "bk_osd_emwin_font.h"   /* osd_utf8_next() — shared UTF-8 iterator (format-agnostic) */

/* Resolve codepoint to glyph_id in one cmap; returns 0 if not found */
static uint32_t osd_lv_cmap_lookup(const lv_font_fmt_txt_cmap_t *cmap, uint32_t cp)
{
    if (cp < cmap->range_start || cp >= cmap->range_start + cmap->range_length) {
        return 0;
    }
    uint32_t rcp = cp - cmap->range_start;

    switch (cmap->type) {
        case LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY:
            return cmap->glyph_id_start + rcp;

        case LV_FONT_FMT_TXT_CMAP_FORMAT0_FULL: {
            const uint8_t *ofs = (const uint8_t *)cmap->glyph_id_ofs_list;
            return cmap->glyph_id_start + (ofs ? ofs[rcp] : rcp);
        }

        case LV_FONT_FMT_TXT_CMAP_SPARSE_TINY: {
            const uint16_t *ul = cmap->unicode_list;
            for (uint16_t i = 0; i < cmap->list_length; i++) {
                if (ul[i] == (uint16_t)rcp) {
                    return cmap->glyph_id_start + i;
                }
            }
            return 0;
        }

        case LV_FONT_FMT_TXT_CMAP_SPARSE_FULL: {
            const uint16_t *ul = cmap->unicode_list;
            const uint16_t *ofs = (const uint16_t *)cmap->glyph_id_ofs_list;
            for (uint16_t i = 0; i < cmap->list_length; i++) {
                if (ul[i] == (uint16_t)rcp) {
                    return cmap->glyph_id_start + (ofs ? ofs[i] : i);
                }
            }
            return 0;
        }
        default:
            return 0;
    }
}

bool osd_lv_font_get_glyph(const lv_font_t *font, uint32_t cp, osd_glyph_t *out)
{
    if (font == NULL || out == NULL || font->dsc == NULL) {
        return false;
    }
    const lv_font_fmt_txt_dsc_t *dsc = (const lv_font_fmt_txt_dsc_t *)font->dsc;

    uint32_t gid = 0;
    for (uint16_t i = 0; i < dsc->cmap_num; i++) {
        gid = osd_lv_cmap_lookup(&dsc->cmaps[i], cp);
        if (gid != 0) {
            break;
        }
    }
    if (gid == 0) {
        return false;
    }

    const lv_font_fmt_txt_glyph_dsc_t *g = &dsc->glyph_dsc[gid];

    out->box_w  = g->box_w;
    out->box_h  = g->box_h;
    out->ofs_x  = g->ofs_x;
    out->ofs_y  = g->ofs_y;
    out->adv_w  = (uint16_t)(g->adv_w >> 4);   /* 8.4 fixed-point -> pixels */
    out->bitmap = dsc->glyph_bitmap + g->bitmap_index;
    out->bpp    = (uint8_t)dsc->bpp;
    out->stride = dsc->stride;
    return true;
}

uint8_t osd_lv_font_glyph_a8(const osd_glyph_t *g, uint16_t x, uint16_t y)
{
    if (x >= g->box_w || y >= g->box_h) {
        return 0;
    }

    uint32_t bit_pos;
    if (g->stride == 0) {
        /* Contiguous bitstream: whole glyph packed row-major, MSB first */
        bit_pos = ((uint32_t)y * g->box_w + x) * g->bpp;
    } else {
        /* Each row padded to stride bytes */
        bit_pos = (uint32_t)y * g->stride * 8u + (uint32_t)x * g->bpp;
    }

    uint32_t byte_idx = bit_pos >> 3;
    uint32_t bit_in_byte = bit_pos & 7u;

    /* Fast path for common bpp when pixel fits in one byte/nibble */
    switch (g->bpp) {
        case 8:
            return g->bitmap[byte_idx];
        case 4: {
            uint8_t byte = g->bitmap[byte_idx];
            uint8_t nib = (bit_in_byte == 0u) ? (uint8_t)(byte >> 4) : (uint8_t)(byte & 0x0Fu);
            return (uint8_t)(nib * 17u);            /* 0..255 */
        }
        case 2: {
            uint8_t v = (uint8_t)((g->bitmap[byte_idx] >> (6u - bit_in_byte)) & 0x03u);
            return (uint8_t)(v * 85u);              /* 0,85,170,255 */
        }
        case 1:
            return (g->bitmap[byte_idx] >> (7u - bit_in_byte)) & 1u ? 255u : 0u;
        default:
            break;
    }

    /* Fallback: bit-by-bit read for other bpp (e.g. 3), MSB first */
    uint32_t val = 0;
    for (uint8_t b = 0; b < g->bpp; b++) {
        uint32_t cur_byte = g->bitmap[byte_idx + ((bit_in_byte + b) >> 3)];
        uint8_t shift = 7u - ((bit_in_byte + b) & 7u);
        val = (val << 1) | ((cur_byte >> shift) & 1u);
    }
    if (g->bpp == 3) return (uint8_t)(val * 36u > 255 ? 255 : val * 36u);
    return val ? 255u : 0u;
}

void osd_lv_font_text_extent(const lv_font_t *font, const char *utf8, uint8_t scale,
                             uint16_t *out_w, uint16_t *out_h)
{
    uint32_t w = 0, h = 0;

    if (font != NULL && utf8 != NULL) {
        if (scale == 0) scale = 1;
        const char *p = utf8;
        while (*p) {
            uint32_t cp = 0;
            p += osd_utf8_next(p, &cp);
            osd_glyph_t g;
            w += (osd_lv_font_get_glyph(font, cp, &g) ? g.adv_w : 6u) * scale;
        }
        h = (uint32_t)font->line_height * scale;
    }

    /* Small margin to avoid edge clipping; the LCD commit tightens the actual blit rect. */
    if (out_w) *out_w = (uint16_t)(w ? w + 2u : 0u);
    if (out_h) *out_h = (uint16_t)(h ? h + 2u : 0u);
}

uint16_t osd_lv_font_blit(uint32_t *dst, uint16_t dst_w, uint16_t dst_h,
                          const lv_font_t *font, const char *utf8,
                          uint16_t x, uint16_t y, uint32_t argb, uint8_t scale,
                          osd_lv_font_rect_t *out_bbox)
{
    uint16_t bx0 = dst_w, by0 = dst_h, bx1 = 0, by1 = 0;   /* accumulate drawn rect */

    if (dst == NULL || font == NULL || utf8 == NULL || dst_w == 0U || dst_h == 0U) {
        if (out_bbox) {
            out_bbox->x0 = out_bbox->y0 = out_bbox->x1 = out_bbox->y1 = 0;
        }
        return x;
    }
    if (scale == 0) scale = 1;

    uint32_t rgb = argb & 0x00FFFFFFU;
    uint16_t pen_x = x;
    int ascent = (int)(font->line_height - font->base_line);
    int baseline = (int)y + ascent * scale;
    const char *p = utf8;
    while (*p) {
        uint32_t cp = 0;
        p += osd_utf8_next(p, &cp);
        osd_glyph_t g;
        if (!osd_lv_font_get_glyph(font, cp, &g)) {
            pen_x += 6 * scale;
            continue;
        }

        int glyph_top = baseline - (g.ofs_y + g.box_h) * scale;
        int gx0 = (int)pen_x + g.ofs_x * scale;

        /* Union the clipped glyph box into the drawn bbox. */
        {
            int cx0 = gx0 < 0 ? 0 : gx0;
            int cy0 = glyph_top < 0 ? 0 : glyph_top;
            int cx1 = gx0 + g.box_w * scale; if (cx1 > (int)dst_w) cx1 = dst_w;
            int cy1 = glyph_top + g.box_h * scale; if (cy1 > (int)dst_h) cy1 = dst_h;
            if (cx1 > cx0 && cy1 > cy0) {
                if (cx0 < (int)bx0) bx0 = (uint16_t)cx0;
                if (cy0 < (int)by0) by0 = (uint16_t)cy0;
                if (cx1 > (int)bx1) bx1 = (uint16_t)cx1;
                if (cy1 > (int)by1) by1 = (uint16_t)cy1;
            }
        }

        for (uint16_t gy = 0; gy < g.box_h; gy++) {
            for (uint16_t gx = 0; gx < g.box_w; gx++) {
                uint8_t a = osd_lv_font_glyph_a8(&g, gx, gy);
                if (a == 0) continue;
                uint32_t pix = ((uint32_t)a << 24) | rgb;
                for (uint8_t sy = 0; sy < scale; sy++) {
                    int dy = glyph_top + gy * scale + sy;
                    if (dy < 0 || dy >= (int)dst_h) continue;
                    uint32_t *drow = dst + (uint32_t)dy * dst_w;
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        int dx = gx0 + gx * scale + sx;
                        if (dx >= 0 && dx < (int)dst_w) drow[dx] = pix;
                    }
                }
            }
        }

        pen_x += g.adv_w * scale;
        if (pen_x >= dst_w) break;
    }

    if (out_bbox) {
        if (bx1 > bx0 && by1 > by0) {
            out_bbox->x0 = bx0; out_bbox->y0 = by0;
            out_bbox->x1 = bx1; out_bbox->y1 = by1;
        } else {
            out_bbox->x0 = out_bbox->y0 = out_bbox->x1 = out_bbox->y1 = 0;
        }
    }
    return pen_x;
}
