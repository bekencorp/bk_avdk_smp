/*
 * Minimal LVGL fmt_txt font decoder (no CONFIG_LVGL).
 * Reads glyphs from lv_font_t->dsc (lv_font_fmt_txt_dsc_t); supports uncompressed bpp 1/2/4/8.
 */
#include "bk_osd_lv_font.h"

/* Resolve codepoint to glyph_id in one cmap; returns 0 if not found */
static uint32_t osd_cmap_lookup(const lv_font_fmt_txt_cmap_t *cmap, uint32_t cp)
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

bool osd_font_get_glyph(const lv_font_t *font, uint32_t cp, osd_glyph_t *out)
{
    if (font == NULL || out == NULL || font->dsc == NULL) {
        return false;
    }
    const lv_font_fmt_txt_dsc_t *dsc = (const lv_font_fmt_txt_dsc_t *)font->dsc;

    uint32_t gid = 0;
    for (uint16_t i = 0; i < dsc->cmap_num; i++) {
        gid = osd_cmap_lookup(&dsc->cmaps[i], cp);
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

uint8_t osd_font_glyph_a8(const osd_glyph_t *g, uint16_t x, uint16_t y)
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

uint32_t osd_utf8_next(const char *s, uint32_t *cp)
{
    const uint8_t *p = (const uint8_t *)s;
    uint8_t c = p[0];

    if (c < 0x80) {
        *cp = c;
        return 1;
    } else if ((c & 0xE0) == 0xC0) {
        *cp = ((uint32_t)(c & 0x1F) << 6) | (p[1] & 0x3F);
        return 2;
    } else if ((c & 0xF0) == 0xE0) {
        *cp = ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        return 3;
    } else if ((c & 0xF8) == 0xF0) {
        *cp = ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(p[1] & 0x3F) << 12) |
              ((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        return 4;
    }
    *cp = c;
    return 1;
}
