/*
 * Shared emWin (gui_font_digit_struct) font rasterization.
 * See bk_osd_emwin_font.h. Dependency-free (no GPU / frame buffer) so both the LCD engine and the
 * H264 overlay path can call it.
 */
#include "bk_osd_emwin_font.h"

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

/* Locate the glyph entry for codepoint @cp (table terminated by value==0). */
static const gui_font_digit_struct *emwin_font_glyph(const gui_font_digit_struct *tbl, uint32_t cp)
{
    if (tbl == NULL) {
        return NULL;
    }
    for (uint32_t i = 0; tbl[i].value != 0; i++) {
        if (tbl[i].value == cp && tbl[i].data != NULL) {
            return &tbl[i];
        }
    }
    return NULL;
}

void osd_emwin_font_text_extent(const gui_font_digit_struct *tbl, const char *utf8,
                                uint16_t *out_w, uint16_t *out_h)
{
    uint32_t w = 0;
    uint32_t h = 0;

    if (tbl != NULL && utf8 != NULL) {
        uint16_t def_adv = tbl[0].width ? tbl[0].width : 12;
        const char *p = utf8;
        while (*p) {
            uint32_t cp = 0;
            p += osd_utf8_next(p, &cp);
            const gui_font_digit_struct *g = emwin_font_glyph(tbl, cp);
            if (g == NULL) {
                w += def_adv;
                continue;
            }
            uint32_t bottom = (uint32_t)g->y_pos + g->y_size;
            if (bottom > h) {
                h = bottom;
            }
            w += g->width;
        }
    }

    /* Small margin to avoid edge clipping; the LCD commit tightens the actual blit rect. */
    if (out_w) *out_w = (uint16_t)(w ? w + 2U : 0U);
    if (out_h) *out_h = (uint16_t)(h ? h + 2U : 0U);
}

uint16_t osd_emwin_font_blit(uint32_t *dst, uint16_t dst_w, uint16_t dst_h,
                             const gui_font_digit_struct *tbl, const char *utf8,
                             uint16_t x, uint16_t y, uint32_t argb,
                             osd_emwin_font_rect_t *out_bbox)
{
    uint16_t bx0 = dst_w, by0 = dst_h, bx1 = 0, by1 = 0;   /* accumulate drawn rect */

    if (dst == NULL || tbl == NULL || utf8 == NULL || dst_w == 0U || dst_h == 0U) {
        if (out_bbox) {
            out_bbox->x0 = out_bbox->y0 = out_bbox->x1 = out_bbox->y1 = 0;
        }
        return x;
    }

    uint32_t rgb = argb & 0x00FFFFFFU;
    uint16_t def_adv = tbl[0].width ? tbl[0].width : 12;
    uint16_t pen_x = x;
    const char *p = utf8;
    while (*p) {
        uint32_t cp = 0;
        p += osd_utf8_next(p, &cp);
        const gui_font_digit_struct *g = emwin_font_glyph(tbl, cp);
        if (g == NULL) {
            pen_x += def_adv;
            continue;
        }

        uint8_t bp = g->bit_point ? g->bit_point : 4;
        uint32_t stride = ((uint32_t)g->x_size * bp + (8U - bp)) / 8U;
        int gx0 = (int)pen_x + g->x_pos;
        int gy0 = (int)y + g->y_pos;

        /* Union the clipped glyph box into the drawn bbox. */
        {
            int cx0 = gx0 < 0 ? 0 : gx0;
            int cy0 = gy0 < 0 ? 0 : gy0;
            int cx1 = gx0 + g->x_size; if (cx1 > (int)dst_w) cx1 = dst_w;
            int cy1 = gy0 + g->y_size; if (cy1 > (int)dst_h) cy1 = dst_h;
            if (cx1 > cx0 && cy1 > cy0) {
                if (cx0 < (int)bx0) bx0 = (uint16_t)cx0;
                if (cy0 < (int)by0) by0 = (uint16_t)cy0;
                if (cx1 > (int)bx1) bx1 = (uint16_t)cx1;
                if (cy1 > (int)by1) by1 = (uint16_t)cy1;
            }
        }

        /* Clip the glyph's x range once (instead of per-pixel), so the inner loop is branch-light. */
        int cgx0 = gx0 < 0 ? -gx0 : 0;                 /* first glyph column that lands on-screen */
        int cgx1 = (int)g->x_size;
        if (gx0 + cgx1 > (int)dst_w) cgx1 = (int)dst_w - gx0;   /* one past last on-screen column */

        /* Hoist the bit-depth decision out of the pixel loop: one specialized row loop per bpp
         * removes the per-pixel function call and per-pixel bp switch that dominated the raster. */
        for (uint16_t gy = 0; gy < g->y_size; gy++) {
            int dy = gy0 + (int)gy;
            if (dy < 0) continue;
            if (dy >= (int)dst_h) break;
            uint32_t *drow = dst + (uint32_t)dy * dst_w + gx0;   /* drow[gx] == dst pixel for glyph col gx */
            const uint8_t *grow = &g->data[(uint32_t)gy * stride];

            if (bp == 4) {
                for (int gx = cgx0; gx < cgx1; gx++) {
                    uint8_t byte = grow[(uint32_t)gx >> 1];
                    uint8_t nib  = (gx & 1) ? (uint8_t)(byte & 0x0FU) : (uint8_t)((byte >> 4) & 0x0FU);
                    if (nib) drow[gx] = ((uint32_t)(nib * 17U) << 24) | rgb;
                }
            } else if (bp == 1) {
                for (int gx = cgx0; gx < cgx1; gx++) {
                    if (grow[(uint32_t)gx >> 3] & (uint8_t)(0x80U >> (gx & 7)))
                        drow[gx] = 0xFF000000U | rgb;
                }
            } else if (bp == 2) {
                for (int gx = cgx0; gx < cgx1; gx++) {
                    uint8_t q = (uint8_t)((grow[(uint32_t)gx >> 2] >> (6U - 2U * (gx & 3))) & 0x03U);
                    if (q) drow[gx] = ((uint32_t)(q * 85U) << 24) | rgb;
                }
            }
        }

        pen_x += g->width;
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
