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
 * One instance binds one external pipeline GPU; the component registers
 * composited sprites that the GPU SRC_OVER blends onto video each frame.
 *
 * Vocabulary: the client works with elements that auto-cluster into logical
 * "overlay layers" (leased from bk_gpu_overlay). "slot" is only the engine's
 * internal array ordinal for a layer and is not a client-facing concept.
 * begin/commit are also internal. Three render entries:
 *   - bk_draw_osd_array(h, list)  — batch blend_info[] (NULL = default list), auto cluster into layers
 *   - bk_draw_osd_element(h, &info) — one-shot single element (image or text)
 *   - bk_draw_osd_text(h, ...)     — one-shot raw font text (no blend_info)
 * Runtime refresh: add_or_update()/remove() then array(NULL).
 * MIPI and UVC each use a separate instance; safe to run concurrently.
 *******************************************************************/

/**
 * @brief Create an OSD controller instance (binds config->gpu; OSD shares the
 *        controller's overlay with PIP and other layers).
 */
avdk_err_t bk_draw_osd_new(bk_draw_osd_ctlr_handle_t *handle, osd_ctlr_config_t *config);

/**
 * @brief Delete an OSD controller instance (clears the overlay layers it owns and frees sprites).
 */
avdk_err_t bk_draw_osd_delete(bk_draw_osd_ctlr_handle_t handle);

/**
 * @brief One-shot render of a single element (image or text) into the next free overlay layer.
 *        - Image: copies info->addr->image.data;
 *        - Font: uses info->addr->font + .color; text from info->content (or name if empty).
 *        Assets may be passed inline, e.g. &(blend_info_t){.addr=&font_text1, .content="12:35"}.
 */
avdk_err_t bk_draw_osd_element(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *info);

/**
 * @brief One-shot render of raw UTF-8 font text (no blend_info, e.g. LVGL fonts) into the next free overlay layer.
 * @param kind  OSD_FONT_LVGL (font = const lv_font_t*, integer scale)
 *              OSD_FONT_BKFONT (font = const gui_font_digit_struct*, scale ignored)
 * @param x,y   Position in panel coordinates
 * @param argb  0x00RRGGBB; alpha comes from the glyph bitmap
 */
avdk_err_t bk_draw_osd_text(bk_draw_osd_ctlr_handle_t handle, osd_font_kind_t kind,
                            const void *font, const char *utf8,
                            uint16_t x, uint16_t y, uint32_t argb, uint8_t scale);

/**
 * @brief Array render entry (recommended): render blend_info[] (NULL = default dynamic list).
 *        Elements auto-cluster by spatial proximity into leased GPU layers (one tight sprite per
 *        cluster); when count exceeds current hardware capacity, merge by least wasted area
 *        (no elements dropped). The layer cursor stops after used clusters; clear() resets it.
 *
 *        Incremental by default on the dynamic list (list == NULL): only elements changed via
 *        add_or_update() are re-composited; unchanged layers keep being blitted. An explicit list,
 *        an add/remove/clear, or a merged layout triggers a full repaint.
 * @param list  Display list (NULL = default dynamic list); terminated by {.addr=NULL}
 */
avdk_err_t bk_draw_osd_array(bk_draw_osd_ctlr_handle_t handle, const blend_info_t *list);

/**
 * @brief Clear layers owned by this OSD instance without disturbing other users.
 */
avdk_err_t bk_draw_osd_clear(bk_draw_osd_ctlr_handle_t handle);

/**
 * @brief Render the dynamic-list cluster containing one named element.
 */
avdk_err_t bk_draw_osd_render_element(bk_draw_osd_ctlr_handle_t handle,
                                      const char *name);

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
