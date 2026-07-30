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

#ifndef __BK_DRAW_OSD_H__
#define __BK_DRAW_OSD_H__

#include "components/bk_draw_osd_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************
 * component name: draw_osd
 * description: Public API (open interface)
 *
 * Pipeline compositing model: one bk_draw_osd instance binds one external pipeline GPU;
 * the component registers composited transparent sprites with that GPU, which SRC_OVER
 * blends them onto video each frame. Three render entry points only; begin/commit/slot
 * management is internal:
 *   - bk_draw_osd_array(h, list)    — batch render blend_info[] (NULL = default list), auto cluster/slot
 *   - bk_draw_osd_element(h, &info)   — one-shot single element (image or text), uses element coords/color/content
 *   - bk_draw_osd_text(h, kind, font, ...) — one-shot raw font text (when no blend_info)
 * Runtime refresh: bk_draw_osd_add_or_update()/bk_draw_osd_remove() update the dynamic list, then array(NULL).
 * MIPI and UVC each use a separate bk_draw_osd_new instance; no shared state, safe to run concurrently.
 *******************************************************************/

/**
 * @brief Create an OSD controller instance (binds config->gpu).
 */
avdk_err_t bk_draw_osd_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config);

/**
 * @brief Delete an OSD controller instance (clears registered GPU blits and frees sprites).
 */
avdk_err_t bk_draw_osd_delete(bk_draw_osd_ctlr_handle_t handle);

/**
 * @brief One-shot render of a single element (image or text). Self-contained: opens a tight sprite
 *        at info->xpos/ypos, composites, and submits to the next free GPU slot; no begin/commit.
 *        - Image: copies info->addr->image.data;
 *        - Font: uses info->addr->font (bk_font glyph) + .color; text from info->content (or name if empty).
 *        Asset structs may be passed inline, e.g. &(blend_info_t){.addr=&font_text1, .content="12:35"}.
 */
avdk_err_t bk_draw_osd_element(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *info);

/**
 * @brief One-shot render of raw UTF-8 font text (ad-hoc path without blend_info/bk_blend_t, e.g. LVGL fonts).
 *        Self-contained: opens a text-sized sprite and submits to the next free GPU slot.
 * @param kind  OSD_FONT_LVGL (font = const lv_font_t*, integer scale)
 *              OSD_FONT_BKFONT (font = const gui_font_digit_struct*, scale ignored)
 * @param x,y   Position in panel coordinates
 * @param argb  0x00RRGGBB; alpha comes from the glyph bitmap
 */
avdk_err_t bk_draw_osd_text(bk_draw_osd_ctlr_handle_t handle, osd_font_kind_t kind,
                            const void *font, const char *utf8,
                            uint16_t x, uint16_t y, uint32_t argb, uint8_t scale);

/**
 * @brief Array render entry (recommended): render blend_info[] (NULL = default dynamic list from new()).
 *        Elements are auto-clustered by spatial proximity into GPU slots (tight bounding-box sprite per
 *        cluster, total <= BK_GPU_BLIT_SLOT_MAX):
 *        - Distant elements get separate small-region slots (flexa can skip blank blocks);
 *        - When count exceeds slot limit, merge by least wasted area; no elements dropped.
 *        No manual slot/begin/commit; one call suffices. Slot cursor stops after used clusters;
 *        element/text can append to remaining slots; clear() resets the cursor.
 * @param list  Display list (NULL = default dynamic list); terminated by {.addr=NULL}
 */
avdk_err_t bk_draw_osd_array(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *list);

/**
 * @brief Clear registered OSD blits on the bound GPU and reset internal slot cursor (next draw from slot 0).
 */
avdk_err_t bk_draw_osd_clear(bk_draw_osd_ctlr_handle_t handle);

/**
 * @brief Add or update a display-list element at runtime (by asset name; content optional).
 */
avdk_err_t bk_draw_osd_add_or_update(bk_draw_osd_ctlr_handle_t handle, const char *name, const char* content);

/**
 * @brief Remove an element from the display list at runtime.
 */
avdk_err_t bk_draw_osd_remove(bk_draw_osd_ctlr_handle_t handle, const char *name);

/**
 * @brief OSD controller ioctl (OSD_CTLR_CMD_GET_ALL_ASSETS / GET_DRAW_INFO).
 */
avdk_err_t bk_draw_osd_ioctl(bk_draw_osd_ctlr_handle_t handle, uint32_t ioctl_cmd, uint32_t param1, uint32_t param2, uint32_t param3);

#ifdef __cplusplus
}
#endif

#endif /* __BK_DRAW_OSD_H__ */
