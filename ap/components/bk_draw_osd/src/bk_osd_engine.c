/*
 * osd_engine implementation (see bk_osd_engine.h).
 *
 * Composites into PSRAM sprites, then submits them to bk_gpu_overlay.
 */
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include "cache.h"

#include "bk_osd_engine.h"
#include "bk_osd_emwin_font.h"

#define TAG "osd_engine"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Shrink sprite height by this many pixel rows on OOM retry */
#define OSD_ENGINE_SHRINK_UNIT  60u
#define OSD_ENGINE_TILE_W       16u
#define OSD_ENGINE_TILE_H       4u
#define OSD_ENGINE_Z_ORDER_BASE 100

struct osd_engine {
    bk_gpu_overlay_handle_t overlay;
    uint16_t panel_w;
    uint16_t panel_h;
    uint16_t rotate_degree;   /* OSD content rotation: 0 / 90 / 270 */
    bk_pixel_format_t src_format;
    bk_gpu_overlay_layer_handle_t layer_handles[BK_GPU_OVERLAY_LAYER_MAX];
    uint32_t owned_slot_mask;

    uint32_t *sprite;      /* in-progress sprite not yet committed (engine-owned) */
    uint16_t  sw;          /* allocated storage width / GPU stride */
    uint16_t  sh;          /* allocated storage height */
    uint16_t  content_w;   /* logical source width, excluding alignment padding */
    uint16_t  content_h;   /* logical source height, excluding alignment padding */
    uint16_t  dst_x;
    uint16_t  dst_y;
    uint16_t  content_x;
    uint16_t  content_y;

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
    uint16_t width = eng->content_w != 0U ? eng->content_w : eng->sw;
    uint16_t height = eng->content_h != 0U ? eng->content_h : eng->sh;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int)width) x1 = width;
    if (y1 > (int)height) y1 = height;
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
    if (out == NULL || cfg == NULL || cfg->overlay == NULL) {
        return AVDK_ERR_INVAL;
    }
    struct osd_engine *eng = (struct osd_engine *)os_malloc(sizeof(*eng));
    if (eng == NULL) {
        return AVDK_ERR_NOMEM;
    }
    os_memset(eng, 0, sizeof(*eng));
    eng->overlay    = cfg->overlay;
    eng->panel_w    = cfg->panel_w;
    eng->panel_h    = cfg->panel_h;
    eng->rotate_degree = cfg->rotate_degree;
    eng->src_format = cfg->src_format;
    *out = eng;
    return AVDK_ERR_OK;
}

avdk_err_t osd_engine_delete(osd_engine_handle_t eng)
{
    if (eng == NULL) {
        return AVDK_ERR_INVAL;
    }
    /* Clear only slots owned by this instance. */
    (void)osd_engine_clear(eng);
    for (uint8_t slot = 0; slot < BK_GPU_OVERLAY_LAYER_MAX; slot++) {
        if (eng->layer_handles[slot] != BK_GPU_OVERLAY_LAYER_INVALID) {
            (void)bk_gpu_overlay_layer_release(
                eng->overlay, eng->layer_handles[slot]);
            eng->layer_handles[slot] = BK_GPU_OVERLAY_LAYER_INVALID;
        }
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

uint16_t osd_engine_sprite_w(osd_engine_handle_t eng) { return eng ? eng->content_w : 0; }
uint16_t osd_engine_sprite_h(osd_engine_handle_t eng) { return eng ? eng->content_h : 0; }

void osd_engine_set_slot(osd_engine_handle_t eng, uint8_t slot)
{
    if (eng == NULL) return;
    if (slot >= BK_GPU_OVERLAY_LAYER_MAX) {
        return;
    }
    eng->slot = slot;
}

avdk_err_t osd_engine_begin(osd_engine_handle_t eng, uint16_t w, uint16_t h,
                            uint16_t dst_x, uint16_t dst_y)
{
    if (eng == NULL || w == 0 || h == 0) {
        return AVDK_ERR_INVAL;
    }
    uint16_t content_x = 0U;
    uint16_t content_y = 0U;
    /* For 90/270 the sprite is authored in viewer space (axes swapped vs panel buffer).
     * Unrotated sprites are padded transparently to the physical 16x4 compressed
     * tile grid, so callers continue to provide only logical content geometry. */
    if (eng->rotate_degree == 0U) {
        uint32_t panel_w = ((uint32_t)eng->panel_w + OSD_ENGINE_TILE_W - 1U) &
                           ~(OSD_ENGINE_TILE_W - 1U);
        uint32_t panel_h = ((uint32_t)eng->panel_h + OSD_ENGINE_TILE_H - 1U) &
                           ~(OSD_ENGINE_TILE_H - 1U);
        if (dst_x >= eng->panel_w || dst_y >= eng->panel_h) {
            return AVDK_ERR_INVAL;
        }
        uint32_t x0 = (uint32_t)dst_x & ~(OSD_ENGINE_TILE_W - 1U);
        uint32_t y0 = (uint32_t)dst_y & ~(OSD_ENGINE_TILE_H - 1U);
        uint32_t x1 = ((uint32_t)dst_x + w + OSD_ENGINE_TILE_W - 1U) &
                      ~(OSD_ENGINE_TILE_W - 1U);
        uint32_t y1 = ((uint32_t)dst_y + h + OSD_ENGINE_TILE_H - 1U) &
                      ~(OSD_ENGINE_TILE_H - 1U);
        if (x1 > panel_w) x1 = panel_w;
        if (y1 > panel_h) y1 = panel_h;
        content_x = (uint16_t)((uint32_t)dst_x - x0);
        content_y = (uint16_t)((uint32_t)dst_y - y0);
        dst_x = (uint16_t)x0;
        dst_y = (uint16_t)y0;
        w = (uint16_t)(x1 - x0);
        h = (uint16_t)(y1 - y0);
    } else {
        uint16_t max_w = (eng->rotate_degree == 90U ||
                          eng->rotate_degree == 270U)
                             ? eng->panel_h : eng->panel_w;
        uint16_t max_h = (eng->rotate_degree == 90U ||
                          eng->rotate_degree == 270U)
                             ? eng->panel_w : eng->panel_h;
        if (w > max_w) w = max_w;
        if (h > max_h) h = max_h;
    }

    /* Drop previous in-progress sprite (committed ones remain with GPU) */
    if (eng->sprite) {
        bk_frame_buffer_free(eng->sprite);
        eng->sprite = NULL;
    }
    eng->bb_valid = false;   /* new frame: reset bounding box */

    uint16_t cur_h = h;
    while (cur_h >= OSD_ENGINE_SHRINK_UNIT || cur_h == h) {
        uint16_t storage_w = (uint16_t)(((uint32_t)w + OSD_ENGINE_TILE_W - 1U) &
                                        ~(OSD_ENGINE_TILE_W - 1U));
        uint16_t storage_h = (uint16_t)(((uint32_t)cur_h + OSD_ENGINE_TILE_H - 1U) &
                                        ~(OSD_ENGINE_TILE_H - 1U));
        uint32_t bytes = (uint32_t)storage_w * storage_h * 4u;
        uint32_t *sp = (uint32_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, bytes);
        if (sp != NULL) {
            os_memset(sp, 0, bytes);
            eng->sprite = sp;
            eng->sw     = storage_w;
            eng->sh     = storage_h;
            eng->content_w = w;
            eng->content_h = cur_h;
            eng->dst_x  = dst_x;
            eng->dst_y  = dst_y;
            eng->content_x = content_x;
            eng->content_y = content_y;
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

/*
 * Premultiply a straight-alpha ARGB pixel by its own alpha. The sprite is later
 * composited with a premultiplied SRC_OVER blend, so copying straight-alpha icon
 * pixels verbatim would over-brighten anti-aliased edges into a white fringe.
 * Fully opaque pixels are returned unchanged.
 */
static inline uint32_t osd_engine_premul_pixel(uint32_t px)
{
    uint8_t a = (uint8_t)(px >> 24);
    if (a == 0xFFU) {
        return px;
    }
    if (a == 0U) {
        return 0U;
    }
    uint32_t r = (((px >> 16) & 0xFFU) * a + 127U) / 255U;
    uint32_t g = (((px >> 8) & 0xFFU) * a + 127U) / 255U;
    uint32_t b = ((px & 0xFFU) * a + 127U) / 255U;
    return ((uint32_t)a << 24) | (r << 16) | (g << 8) | b;
}

avdk_err_t osd_engine_put_icon(osd_engine_handle_t eng, const bk_blend_t *icon,
                               uint16_t x, uint16_t y)
{
    if (eng == NULL || eng->sprite == NULL || icon == NULL || icon->image.data == NULL) {
        return AVDK_ERR_INVAL;
    }
    x = (uint16_t)(x + eng->content_x);
    y = (uint16_t)(y + eng->content_y);
    const uint32_t *src = (const uint32_t *)icon->image.data;
    uint16_t iw = (uint16_t)icon->width;
    uint16_t ih = (uint16_t)icon->height;
    uint16_t sw = eng->sw, sh = eng->sh;
    for (uint16_t row = 0; row < ih; row++) {
        uint16_t dy = y + row;
        if (dy >= sh) break;
        uint32_t *drow = eng->sprite + (uint32_t)dy * sw;
        const uint32_t *srow = src + (uint32_t)row * iw;
        for (uint16_t col = 0; col < iw; col++) {
            uint16_t dx = x + col;
            if (dx >= sw) break;
            drow[dx] = osd_engine_premul_pixel(srow[col]);
        }
    }
    engine_bbox_add(eng, x, y, (int)x + iw, (int)y + ih);
    return AVDK_ERR_OK;
}

/* ---- LVGL font rasterization (shared with the H264 path via bk_osd_lv_font) ---- */
static uint16_t engine_put_lvgl(struct osd_engine *eng, const lv_font_t *font, const char *utf8,
                                uint16_t x, uint16_t y, uint32_t argb, uint8_t scale)
{
    osd_lv_font_rect_t bb;
    uint16_t pen_x = osd_lv_font_blit(eng->sprite, eng->sw, eng->sh, font, utf8, x, y, argb, scale, &bb);
    if (bb.x1 > bb.x0 && bb.y1 > bb.y0) {
        engine_bbox_add(eng, (int)bb.x0, (int)bb.y0, (int)bb.x1, (int)bb.y1);
    }
    return pen_x;
}

/* ---- emWin glyph rasterization (shared with the H264 path via bk_osd_emwin_font) ---- */
static uint16_t engine_put_bkfont(struct osd_engine *eng, const gui_font_digit_struct *tbl,
                                  const char *utf8, uint16_t x, uint16_t y, uint32_t argb)
{
    osd_emwin_font_rect_t bb;
    uint16_t pen_x = osd_emwin_font_blit(eng->sprite, eng->sw, eng->sh, tbl, utf8, x, y, argb, &bb);
    if (bb.x1 > bb.x0 && bb.y1 > bb.y0) {
        engine_bbox_add(eng, (int)bb.x0, (int)bb.y0, (int)bb.x1, (int)bb.y1);
    }
    return pen_x;
}

uint16_t osd_engine_put_text(osd_engine_handle_t eng, osd_font_kind_t kind, const void *font,
                             const char *utf8, uint16_t x, uint16_t y, uint32_t argb, uint8_t scale)
{
    if (eng == NULL || eng->sprite == NULL || font == NULL || utf8 == NULL) {
        return x;
    }
    uint16_t local_x = x;
    x = (uint16_t)(x + eng->content_x);
    y = (uint16_t)(y + eng->content_y);
    uint16_t pen_x;
    if (kind == OSD_FONT_BKFONT) {
        pen_x = engine_put_bkfont(
            eng, (const gui_font_digit_struct *)font, utf8, x, y, argb);
    } else {
        pen_x = engine_put_lvgl(
            eng, (const lv_font_t *)font, utf8, x, y, argb, scale);
    }
    return pen_x >= eng->content_x
               ? (uint16_t)(pen_x - eng->content_x)
               : local_x;
}

void osd_engine_text_extent(osd_font_kind_t kind, const void *font, const char *utf8,
                            uint8_t scale, uint16_t *out_w, uint16_t *out_h)
{
    if (kind == OSD_FONT_BKFONT) {
        /* emWin extent (incl. margin) lives in the shared util so both OSD paths agree. */
        osd_emwin_font_text_extent((const gui_font_digit_struct *)font, utf8, out_w, out_h);
        return;
    }

    /* LVGL extent (incl. margin) lives in the shared util so both OSD paths agree. */
    osd_lv_font_text_extent((const lv_font_t *)font, utf8, scale, out_w, out_h);
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
    if (eng->overlay == NULL) {
        LOGE("overlay handle NULL\n");
        bk_frame_buffer_free(eng->sprite);
        eng->sprite = NULL;
        return AVDK_ERR_GENERIC;
    }

    /* Auto bbox crop: blit only the content rect. The compressed BGRA target
     * uses 16x4 tiles, so the physical destination must cover whole tiles.
     * SRC_OVER cost = cw*ch; smaller area avoids flexa resync window drops.
     * Falls back to full sprite if bb_valid is false.
     * Crop is only applied for rotate_degree == 0; rotated blits submit the full sprite so the
     * GPU rotation matrix maps a whole viewer-space sprite (crop offset under rotation would
     * need an axis-transformed dst, avoided here for correctness). */
    uint16_t cx = 0, cy = 0, cw = eng->content_w, ch = eng->content_h;
    if (eng->bb_valid && eng->rotate_degree == 0) {
        uint16_t x0 = eng->bb_x0 & (uint16_t)~15u;
        uint16_t y0 = eng->bb_y0 & (uint16_t)~3u;
        uint16_t x1 = (uint16_t)((eng->bb_x1 + 15u) & ~15u);
        uint16_t y1 = (uint16_t)((eng->bb_y1 + 3u) & ~3u);
        if (x1 > eng->content_w) x1 = eng->content_w;
        if (y1 > eng->content_h) y1 = eng->content_h;
        cx = x0; cy = y0;
        cw = (uint16_t)(x1 - x0);
        ch = (uint16_t)(y1 - y0);
    }

    /* Map composed sprite to panel-buffer placement.
     * rotate 0  : dst = viewer/buffer coords + crop offset (legacy).
     * rotate 90 : viewer (ex,ey) sprite (sw,sh) -> buffer x[panel_w-ey-sh .. panel_w-ey], y[ex .. ex+sw].
     * rotate 270: viewer (ex,ey) sprite (sw,sh) -> buffer x[ey .. ey+sh],           y[panel_h-ex-sw .. panel_h-ex].
     * (see GPU frame blit / bk_gpu_blit_to_flexa_block matrix convention) */
    uint16_t dst_x, dst_y;
    if (eng->rotate_degree == 90) {
        int32_t bx = (int32_t)eng->panel_w - (int32_t)eng->dst_y - (int32_t)eng->content_h;
        dst_x = (uint16_t)(bx < 0 ? 0 : bx);
        dst_y = eng->dst_x;
    } else if (eng->rotate_degree == 270) {
        int32_t by = (int32_t)eng->panel_h - (int32_t)eng->dst_x - (int32_t)eng->content_w;
        dst_x = eng->dst_y;
        dst_y = (uint16_t)(by < 0 ? 0 : by);
    } else {
        dst_x = (uint16_t)(eng->dst_x + cx);
        dst_y = (uint16_t)(eng->dst_y + cy);
    }

    /* Footprint in the final display buffer. Rotated blits submit the whole sprite with axes
     * swapped, so the on-screen size is (sh x sw); the un-rotated path uses the crop rect. */
    uint16_t fw = (eng->rotate_degree == 90 || eng->rotate_degree == 270) ? eng->content_h : cw;
    uint16_t fh = (eng->rotate_degree == 90 || eng->rotate_degree == 270) ? eng->content_w : ch;
    uint32_t target_w = eng->panel_w;
    uint32_t target_h = eng->panel_h;
    if (eng->rotate_degree == 0U) {
        target_w = (target_w + OSD_ENGINE_TILE_W - 1U) &
                   ~(OSD_ENGINE_TILE_W - 1U);
        target_h = (target_h + OSD_ENGINE_TILE_H - 1U) &
                   ~(OSD_ENGINE_TILE_H - 1U);
    }

    /* Bounds guard: an element whose xpos/ypos (in the space implied by rotate_degree) lands off
     * the panel is a coordinate/rotation misconfig (e.g. viewer-space coords rendered at rotate 0).
     * Never hand the GPU an out-of-buffer dst: drop fully off-screen elements, clip the non-rotated
     * partial-overflow case, and warn so the wrong coordinate/rotate pairing is visible in the log. */
    if (dst_x >= target_w || dst_y >= target_h) {
        LOGW("OSD element off-screen (dst=%u,%u panel=%ux%u rot=%u), skipped\n",
             dst_x, dst_y, eng->panel_w, eng->panel_h, eng->rotate_degree);
        bk_frame_buffer_free(eng->sprite);
        eng->sprite = NULL;
        return AVDK_ERR_OK;
    }
    if ((uint32_t)dst_x + fw > target_w ||
        (uint32_t)dst_y + fh > target_h) {
        if (eng->rotate_degree == 0) {
            if ((uint32_t)dst_x + cw > target_w)
                cw = (uint16_t)(target_w - dst_x);
            if ((uint32_t)dst_y + ch > target_h)
                ch = (uint16_t)(target_h - dst_y);
            LOGW("OSD element exceeds panel, clipped to %ux%u at (%u,%u)\n", cw, ch, dst_x, dst_y);
        } else {
            LOGW("OSD element partially off-screen (rot=%u dst=%u,%u fp=%ux%u panel=%ux%u)\n",
                 eng->rotate_degree, dst_x, dst_y, fw, fh, eng->panel_w, eng->panel_h);
        }
    }

    if (eng->layer_handles[eng->slot] ==
        BK_GPU_OVERLAY_LAYER_INVALID) {
        bk_gpu_overlay_layer_desc_t desc = {
            .z_order = (int16_t)(OSD_ENGINE_Z_ORDER_BASE + eng->slot),
            .backing_policy = BK_GPU_OVERLAY_BACKING_REQUIRED,
        };
        avdk_err_t acquire_ret = bk_gpu_overlay_layer_acquire(
            eng->overlay, &desc, &eng->layer_handles[eng->slot]);
        if (acquire_ret != AVDK_ERR_OK) {
            bk_frame_buffer_free(eng->sprite);
            eng->sprite = NULL;
            return acquire_ret;
        }
    }

    bk_gpu_overlay_layer_update_config_t blit;
    os_memset(&blit, 0, sizeof(blit));
    blit.src_x        = cx;
    blit.src_y        = cy;
    blit.src_width    = cw;
    blit.src_height   = ch;
    blit.buffer_width = eng->sw;
    blit.buffer_height = eng->sh;
    blit.src_format   = eng->src_format;
    blit.dst_x        = dst_x;
    blit.dst_y        = dst_y;
    blit.rotation_degree = eng->rotate_degree;
    blit.enable_alpha_blend = 1;         /* transparent OSD -> SRC_OVER */
    blit.footprint_rect.x =
        eng->rotate_degree == 0U ? eng->dst_x : dst_x;
    blit.footprint_rect.y =
        eng->rotate_degree == 0U ? eng->dst_y : dst_y;
    blit.footprint_rect.width =
        (eng->rotate_degree == 90U || eng->rotate_degree == 270U)
            ? eng->content_h
            : eng->content_w;
    blit.footprint_rect.height =
        (eng->rotate_degree == 90U || eng->rotate_degree == 270U)
            ? eng->content_w
            : eng->content_h;
    blit.release_user_data = NULL;
    blit.buffer_release_cb = osd_engine_free_cb;

    uint32_t *sprite = eng->sprite;
    bk_dcache_clean_for_producer(
        sprite, (size_t)((uint32_t)eng->sw * eng->sh * 4U));
    /* Release engine ownership before submit; GPU owns on success, freed below on failure */
    eng->sprite = NULL;
    avdk_err_t ret = bk_gpu_overlay_layer_submit_region(
        eng->overlay, eng->layer_handles[eng->slot], sprite, &blit);
    if (ret != AVDK_ERR_OK) {
        LOGE("bk_gpu_overlay_layer_submit_region failed %d\n", ret);
        bk_frame_buffer_free(sprite);
    } else {
        eng->owned_slot_mask |= 1UL << eng->slot;
    }
    return ret;
}

avdk_err_t osd_engine_clear(osd_engine_handle_t eng)
{
    if (eng == NULL) {
        return AVDK_ERR_INVAL;
    }
    return osd_engine_clear_slots(eng, eng->owned_slot_mask);
}

avdk_err_t osd_engine_clear_slot(osd_engine_handle_t eng, uint8_t slot)
{
    if (eng == NULL || slot >= BK_GPU_OVERLAY_LAYER_MAX) {
        return AVDK_ERR_INVAL;
    }
    if ((eng->owned_slot_mask & (1UL << slot)) == 0U) {
        return AVDK_ERR_OK;
    }
    if (eng->overlay != NULL) {
        avdk_err_t ret = bk_gpu_overlay_layer_clear(
            eng->overlay, eng->layer_handles[slot]);
        if (ret != AVDK_ERR_OK) {
            return ret;
        }
    }
    eng->owned_slot_mask &= ~(1UL << slot);
    return AVDK_ERR_OK;
}

avdk_err_t osd_engine_clear_slots(osd_engine_handle_t eng, uint32_t slot_mask)
{
    avdk_err_t result = AVDK_ERR_OK;

    if (eng == NULL) {
        return AVDK_ERR_INVAL;
    }
    slot_mask &= eng->owned_slot_mask;
    for (uint8_t slot = 0; slot < BK_GPU_OVERLAY_LAYER_MAX; slot++) {
        if ((slot_mask & (1UL << slot)) != 0U) {
            avdk_err_t ret = osd_engine_clear_slot(eng, slot);
            if (ret != AVDK_ERR_OK) {
                result = ret;
            }
        }
    }
    return result;
}

uint8_t osd_engine_layer_capacity(osd_engine_handle_t eng)
{
    uint8_t leased = 0U;
    if (eng == NULL || eng->overlay == NULL) return 0U;
    for (uint8_t slot = 0; slot < BK_GPU_OVERLAY_LAYER_MAX; slot++) {
        if (eng->layer_handles[slot] != BK_GPU_OVERLAY_LAYER_INVALID) leased++;
    }
    uint8_t available =
        bk_gpu_overlay_get_available_layer_count(eng->overlay);
    uint8_t total = (uint8_t)(leased + available);
    return total > BK_GPU_OVERLAY_LAYER_MAX
               ? BK_GPU_OVERLAY_LAYER_MAX : total;
}

