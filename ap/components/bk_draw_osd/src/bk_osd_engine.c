/*
 * osd_engine implementation (see bk_osd_engine.h).
 *
 * Composites into PSRAM (UNCODED) ARGB8888 sprites, then registers via bk_gpu_blit_set
 * for per-frame SRC_OVER. Committed sprites are freed by the GPU free callback.
 */
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>

#include "bk_osd_engine.h"

#define TAG "osd_engine"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Shrink sprite height by this many pixel rows on OOM retry */
#define OSD_ENGINE_SHRINK_UNIT  60u

struct osd_engine {
    bk_gpu_ctlr_handle_t gpu;
    uint16_t panel_w;
    uint16_t panel_h;
    bk_pixel_format_t src_format;

    uint32_t *sprite;      /* in-progress sprite not yet committed (engine-owned) */
    uint16_t  sw;
    uint16_t  sh;
    uint16_t  dst_x;
    uint16_t  dst_y;

    /* Bounding box of all puts since begin (sprite-local; x1/y1 exclusive).
     * commit crops SRC_OVER blit to this rect to avoid full-width flexa resync drops.
     * bb_valid=false means no content yet. */
    bool      bb_valid;
    uint16_t  bb_x0, bb_y0, bb_x1, bb_y1;

    /* GPU blit slot for next commit (default 0). Different slots = multi-corner OSD. */
    uint8_t   slot;
};

/* Merge a drawn rect into the bounding box; coords clamped to sprite bounds. */
static void engine_bbox_add(struct osd_engine *eng, int x0, int y0, int x1, int y1)
{
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int)eng->sw) x1 = eng->sw;
    if (y1 > (int)eng->sh) y1 = eng->sh;
    if (x1 <= x0 || y1 <= y0) {
        return;   /* fully outside sprite or empty rect */
    }
    if (!eng->bb_valid) {
        eng->bb_x0 = (uint16_t)x0; eng->bb_y0 = (uint16_t)y0;
        eng->bb_x1 = (uint16_t)x1; eng->bb_y1 = (uint16_t)y1;
        eng->bb_valid = true;
        return;
    }
    if ((uint16_t)x0 < eng->bb_x0) eng->bb_x0 = (uint16_t)x0;
    if ((uint16_t)y0 < eng->bb_y0) eng->bb_y0 = (uint16_t)y0;
    if ((uint16_t)x1 > eng->bb_x1) eng->bb_x1 = (uint16_t)x1;
    if ((uint16_t)y1 > eng->bb_y1) eng->bb_y1 = (uint16_t)y1;
}

/* ---------------- lifecycle ---------------- */

avdk_err_t osd_engine_new(osd_engine_handle_t *out, const osd_engine_config_t *cfg)
{
    if (out == NULL || cfg == NULL || cfg->gpu == NULL) {
        return AVDK_ERR_INVAL;
    }
    struct osd_engine *eng = (struct osd_engine *)os_malloc(sizeof(*eng));
    if (eng == NULL) {
        return AVDK_ERR_NOMEM;
    }
    os_memset(eng, 0, sizeof(*eng));
    eng->gpu        = cfg->gpu;
    eng->panel_w    = cfg->panel_w;
    eng->panel_h    = cfg->panel_h;
    eng->src_format = cfg->src_format;
    *out = eng;
    return AVDK_ERR_OK;
}

avdk_err_t osd_engine_delete(osd_engine_handle_t eng)
{
    if (eng == NULL) {
        return AVDK_ERR_INVAL;
    }
    /* Clear registered blits (GPU frees committed sprites via free callback) */
    if (eng->gpu) {
        bk_gpu_blit_clear(eng->gpu);
    }
    /* Free in-progress sprite not yet committed */
    if (eng->sprite) {
        bk_frame_buffer_free(eng->sprite);
        eng->sprite = NULL;
    }
    os_free(eng);
    return AVDK_ERR_OK;
}

/* ---------------- compositing ---------------- */

uint16_t osd_engine_sprite_w(osd_engine_handle_t eng) { return eng ? eng->sw : 0; }
uint16_t osd_engine_sprite_h(osd_engine_handle_t eng) { return eng ? eng->sh : 0; }

void osd_engine_set_slot(osd_engine_handle_t eng, uint8_t slot)
{
    if (eng == NULL) return;
    if (slot >= BK_GPU_BLIT_SLOT_MAX) slot = BK_GPU_BLIT_SLOT_MAX - 1;
    eng->slot = slot;
}

avdk_err_t osd_engine_begin(osd_engine_handle_t eng, uint16_t w, uint16_t h,
                            uint16_t dst_x, uint16_t dst_y)
{
    if (eng == NULL || w == 0 || h == 0) {
        return AVDK_ERR_INVAL;
    }
    if (w > eng->panel_w) w = eng->panel_w;
    if (h > eng->panel_h) h = eng->panel_h;

    /* Drop previous in-progress sprite (committed ones remain with GPU) */
    if (eng->sprite) {
        bk_frame_buffer_free(eng->sprite);
        eng->sprite = NULL;
    }
    eng->bb_valid = false;   /* new frame: reset bounding box */

    uint16_t cur_h = h;
    while (cur_h >= OSD_ENGINE_SHRINK_UNIT || cur_h == h) {
        uint32_t bytes = (uint32_t)w * cur_h * 4u;
        uint32_t *sp = (uint32_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, bytes);
        if (sp != NULL) {
            os_memset(sp, 0, bytes);
            eng->sprite = sp;
            eng->sw     = w;
            eng->sh     = cur_h;
            eng->dst_x  = dst_x;
            eng->dst_y  = dst_y;
            return AVDK_ERR_OK;
        }
        LOGW("sprite %ux%u (%u KB) not available, shrinking...\n",
             w, cur_h, bytes / 1024u);
        if (cur_h < OSD_ENGINE_SHRINK_UNIT) break;
        cur_h = (uint16_t)(cur_h - OSD_ENGINE_SHRINK_UNIT);
    }
    LOGE("sprite alloc failed (w=%u)\n", w);
    return AVDK_ERR_NOMEM;
}

avdk_err_t osd_engine_put_icon(osd_engine_handle_t eng, const bk_blend_t *icon,
                               uint16_t x, uint16_t y)
{
    if (eng == NULL || eng->sprite == NULL || icon == NULL || icon->image.data == NULL) {
        return AVDK_ERR_INVAL;
    }
    const uint32_t *src = (const uint32_t *)icon->image.data;
    uint16_t iw = (uint16_t)(icon->width  ? icon->width  : icon->icon_width);
    uint16_t ih = (uint16_t)(icon->height ? icon->height : icon->icon_height);
    uint16_t sw = eng->sw, sh = eng->sh;
    for (uint16_t row = 0; row < ih; row++) {
        uint16_t dy = y + row;
        if (dy >= sh) break;
        uint32_t *drow = eng->sprite + (uint32_t)dy * sw;
        const uint32_t *srow = src + (uint32_t)row * iw;
        for (uint16_t col = 0; col < iw; col++) {
            uint16_t dx = x + col;
            if (dx >= sw) break;
            drow[dx] = srow[col];
        }
    }
    engine_bbox_add(eng, x, y, (int)x + iw, (int)y + ih);
    return AVDK_ERR_OK;
}

/* ---- LVGL font rasterization (bk_osd_lv_font.c decode, integer scale, baseline-aligned) ---- */
static uint16_t engine_put_lvgl(struct osd_engine *eng, const lv_font_t *font, const char *utf8,
                                uint16_t x, uint16_t y, uint32_t argb, uint8_t scale)
{
    if (font == NULL || utf8 == NULL) return x;
    if (scale == 0) scale = 1;
    uint32_t rgb = argb & 0x00FFFFFFu;
    uint16_t sw = eng->sw, sh = eng->sh;
    uint16_t pen_x = x;
    int ascent = (int)(font->line_height - font->base_line);
    int baseline = (int)y + ascent * scale;
    const char *p = utf8;
    while (*p) {
        uint32_t cp = 0;
        p += osd_utf8_next(p, &cp);
        osd_glyph_t g;
        if (!osd_font_get_glyph(font, cp, &g)) {
            pen_x += 6 * scale;
            continue;
        }
        int glyph_top = baseline - (g.ofs_y + g.box_h) * scale;
        int gx0 = (int)pen_x + g.ofs_x * scale;
        engine_bbox_add(eng, gx0, glyph_top, gx0 + g.box_w * scale, glyph_top + g.box_h * scale);
        for (uint16_t gy = 0; gy < g.box_h; gy++) {
            for (uint16_t gx = 0; gx < g.box_w; gx++) {
                uint8_t a = osd_font_glyph_a8(&g, gx, gy);
                if (a == 0) continue;
                uint32_t pix = ((uint32_t)a << 24) | rgb;
                for (uint8_t sy = 0; sy < scale; sy++) {
                    int dy = glyph_top + gy * scale + sy;
                    if (dy < 0 || dy >= sh) continue;
                    uint32_t *drow = eng->sprite + (uint32_t)dy * sw;
                    for (uint8_t sx = 0; sx < scale; sx++) {
                        int dx = (int)pen_x + g.ofs_x * scale + gx * scale + sx;
                        if (dx >= 0 && dx < sw) drow[dx] = pix;
                    }
                }
            }
        }
        pen_x += g.adv_w * scale;
        if (pen_x >= sw) break;
    }
    return pen_x;
}

/* ---- bk_font/emWin glyph rasterization (gui_font_digit_struct, 4bpp) ---- */
static const gui_font_digit_struct *bkfont_glyph(const gui_font_digit_struct *tbl, uint32_t cp)
{
    if (tbl == NULL) return NULL;
    for (uint32_t i = 0; tbl[i].value != 0; i++) {
        if (tbl[i].value == cp && tbl[i].data != NULL) return &tbl[i];
    }
    return NULL;
}

static uint8_t bkfont_pixel_a8(const gui_font_digit_struct *g, uint32_t stride, uint16_t gx, uint16_t gy)
{
    uint8_t bp = g->bit_point ? g->bit_point : 4;
    const uint8_t *pb = &g->data[(uint32_t)gy * stride + ((uint32_t)gx * bp) / 8u];
    if (bp == 4) {
        uint8_t nib = (gx & 1u) ? (uint8_t)(*pb & 0x0Fu) : (uint8_t)((*pb >> 4) & 0x0Fu);
        return (uint8_t)(nib * 17u);
    }
    if (bp == 1) {
        static const uint8_t bitm[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
        return (*pb & bitm[gx & 7u]) ? 255u : 0u;
    }
    if (bp == 2) {
        return (uint8_t)(((*pb >> (6u - 2u * (gx & 3u))) & 0x03u) * 85u);
    }
    return 0u;
}

static uint16_t engine_put_bkfont(struct osd_engine *eng, const gui_font_digit_struct *tbl,
                                  const char *utf8, uint16_t x, uint16_t y, uint32_t argb)
{
    if (tbl == NULL || utf8 == NULL) return x;
    uint32_t rgb = argb & 0x00FFFFFFu;
    uint16_t sw = eng->sw, sh = eng->sh;
    uint16_t def_adv = tbl[0].width ? tbl[0].width : 12;
    uint16_t pen_x = x;
    const char *p = utf8;
    while (*p) {
        uint32_t cp = 0;
        p += osd_utf8_next(p, &cp);
        const gui_font_digit_struct *g = bkfont_glyph(tbl, cp);
        if (g == NULL) {
            pen_x += def_adv;
            continue;
        }
        uint8_t bp = g->bit_point ? g->bit_point : 4;
        uint32_t stride = ((uint32_t)g->x_size * bp + (8u - bp)) / 8u;
        engine_bbox_add(eng, (int)pen_x + g->x_pos, (int)y + g->y_pos,
                        (int)pen_x + g->x_pos + g->x_size, (int)y + g->y_pos + g->y_size);
        for (uint16_t gy = 0; gy < g->y_size; gy++) {
            uint16_t dy = y + g->y_pos + gy;
            if (dy >= sh) break;
            uint32_t *drow = eng->sprite + (uint32_t)dy * sw;
            for (uint16_t gx = 0; gx < g->x_size; gx++) {
                uint8_t a = bkfont_pixel_a8(g, stride, gx, gy);
                if (a == 0) continue;
                int dx = (int)pen_x + g->x_pos + gx;
                if (dx >= 0 && dx < sw) drow[dx] = ((uint32_t)a << 24) | rgb;
            }
        }
        pen_x += g->width;
        if (pen_x >= sw) break;
    }
    return pen_x;
}

uint16_t osd_engine_put_text(osd_engine_handle_t eng, osd_font_kind_t kind, const void *font,
                             const char *utf8, uint16_t x, uint16_t y, uint32_t argb, uint8_t scale)
{
    if (eng == NULL || eng->sprite == NULL || font == NULL || utf8 == NULL) {
        return x;
    }
    if (kind == OSD_FONT_BKFONT) {
        return engine_put_bkfont(eng, (const gui_font_digit_struct *)font, utf8, x, y, argb);
    }
    return engine_put_lvgl(eng, (const lv_font_t *)font, utf8, x, y, argb, scale);
}

void osd_engine_text_extent(osd_font_kind_t kind, const void *font, const char *utf8,
                            uint8_t scale, uint16_t *out_w, uint16_t *out_h)
{
    uint32_t w = 0, h = 0;
    if (font != NULL && utf8 != NULL) {
        if (scale == 0) scale = 1;
        const char *p = utf8;
        if (kind == OSD_FONT_BKFONT) {
            const gui_font_digit_struct *tbl = (const gui_font_digit_struct *)font;
            uint16_t def_adv = tbl[0].width ? tbl[0].width : 12;
            uint32_t maxb = 0;
            while (*p) {
                uint32_t cp = 0;
                p += osd_utf8_next(p, &cp);
                const gui_font_digit_struct *g = bkfont_glyph(tbl, cp);
                if (g == NULL) { w += def_adv; continue; }
                uint32_t b = (uint32_t)g->y_pos + g->y_size;
                if (b > maxb) maxb = b;
                w += g->width;
            }
            h = maxb;
        } else {
            const lv_font_t *lf = (const lv_font_t *)font;
            while (*p) {
                uint32_t cp = 0;
                p += osd_utf8_next(p, &cp);
                osd_glyph_t g;
                w += (osd_font_get_glyph(lf, cp, &g) ? g.adv_w : 6u) * scale;
            }
            h = (uint32_t)lf->line_height * scale;
        }
    }
    /* Small margin to avoid edge clipping; commit tightens the actual blit rect */
    if (out_w) *out_w = (uint16_t)(w ? w + 2u : 0u);
    if (out_h) *out_h = (uint16_t)(h ? h + 2u : 0u);
}

/* ---------------- commit / clear ---------------- */

static void osd_engine_free_cb(void *frame, void *args)
{
    (void)args;
    if (frame != NULL) {
        bk_frame_buffer_free(frame);
    }
}

avdk_err_t osd_engine_commit(osd_engine_handle_t eng)
{
    if (eng == NULL) {
        return AVDK_ERR_INVAL;
    }
    if (eng->sprite == NULL) {
        return AVDK_ERR_INVAL;
    }
    if (eng->gpu == NULL) {
        LOGE("gpu handle NULL\n");
        bk_frame_buffer_free(eng->sprite);
        eng->sprite = NULL;
        return AVDK_ERR_GENERIC;
    }

    /* Auto bbox crop: blit only the content rect (4px-aligned for tile margin).
     * SRC_OVER cost = cw*ch; smaller area avoids flexa resync window drops.
     * Falls back to full sprite if bb_valid is false. */
    uint16_t cx = 0, cy = 0, cw = eng->sw, ch = eng->sh;
    if (eng->bb_valid) {
        uint16_t x0 = eng->bb_x0 & (uint16_t)~3u;
        uint16_t y0 = eng->bb_y0 & (uint16_t)~3u;
        uint16_t x1 = (uint16_t)((eng->bb_x1 + 3u) & ~3u);
        uint16_t y1 = (uint16_t)((eng->bb_y1 + 3u) & ~3u);
        if (x1 > eng->sw) x1 = eng->sw;
        if (y1 > eng->sh) y1 = eng->sh;
        cx = x0; cy = y0;
        cw = (uint16_t)(x1 - x0);
        ch = (uint16_t)(y1 - y0);
    }

    bk_gpu_blit_config_t blit;
    os_memset(&blit, 0, sizeof(blit));
    blit.src_x        = cx;
    blit.src_y        = cy;
    blit.src_width    = cw;
    blit.src_height   = ch;
    blit.sprite_width  = eng->sw;   /* full sprite stride; src_* is the bbox crop sub-rect */
    blit.sprite_height = eng->sh;
    blit.src_format   = eng->src_format;
    blit.dst_x        = (uint16_t)(eng->dst_x + cx);
    blit.dst_y        = (uint16_t)(eng->dst_y + cy);
    blit.rotate_degree = 0;
    blit.alpha_blend  = 1;               /* transparent OSD -> SRC_OVER */
    blit.osd_slot     = eng->slot;       /* multi-region: submit to this slot */
    blit.args         = eng;
    blit.free         = osd_engine_free_cb;

    /* Per-frame SRC_OVER blit area = cw*ch (tight bbox after auto-crop) */
    LOGI("OSD_COMMIT slot=%u sprite=%ux%u crop=%ux%u area=%u dst=(%u,%u)\n",
         (unsigned)eng->slot, eng->sw, eng->sh, cw, ch, (uint32_t)cw * ch,
         (uint32_t)blit.dst_x, (uint32_t)blit.dst_y);

    uint32_t *sprite = eng->sprite;
    /* Release engine ownership before submit; GPU owns on success, freed below on failure */
    eng->sprite = NULL;
    avdk_err_t ret = bk_gpu_blit_set(eng->gpu, sprite, &blit);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_gpu_blit_set failed %d\n", ret);
        bk_frame_buffer_free(sprite);
    }
    return ret;
}

avdk_err_t osd_engine_clear(osd_engine_handle_t eng)
{
    if (eng == NULL || eng->gpu == NULL) {
        return AVDK_ERR_OK;
    }
    return bk_gpu_blit_clear(eng->gpu);
}
