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

#pragma once

/**
 * @file bk_gpu_overlay.h
 * @brief Internal API for the GPU overlay layer compositor.
 *
 * Component: bk_gpu (internal; not exposed to application code).
 *
 * One overlay instance binds one bk_gpu_ctlr and leases up to
 * BK_GPU_OVERLAY_LAYER_MAX layers. Layers are opaque handles — callers must
 * not assume physical blit-slot numbers. Product code does not use these APIs
 * directly: OSD goes through bk_draw_osd and PIP through bk_display_overlay,
 * both of which take a GPU controller handle and fetch the shared per-controller
 * overlay via bk_gpu_get_overlay(). Pipeline/example code that manages its own
 * dedicated controller may still create an overlay explicitly.
 *
 * Lifecycle (explicit, single-owner controller):
 *   1. bk_gpu_ctlr_new / init / open
 *   2. (optional) bk_gpu_ioctl(SET_OSD_BY_FLEXA) for compose timing
 *   3. bk_gpu_overlay_new(gpu)   — or bk_gpu_get_overlay(gpu) for the shared one
 *   4. layer_acquire → layer_submit* → layer_clear / layer_release
 *   5. bk_gpu_overlay_delete before tearing down GPU / frame callbacks
 *
 * Frame composition itself (when layers are blitted onto the live video
 * buffer) is driven by the media pipeline via the "pipeline compose API"
 * section at the bottom of this header.
 *
 * Types: see bk_gpu_overlay_types.h.
 */

#include <stdbool.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_gpu_types.h>
#include "bk_gpu_overlay_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create an overlay compositor bound to @p gpu.
 *
 * @param[out] handle  Receives the new overlay handle on success.
 * @param[in]  gpu     Opened GPU controller used as the blit backend.
 *
 * @return AVDK_ERR_OK on success;
 *         AVDK_ERR_INVAL if handle/gpu is NULL;
 *         AVDK_ERR_NOMEM if allocation fails.
 *
 * @note Prefer bk_gpu_get_overlay() when the controller is shared by OSD/PIP.
 *       Does not change the GPU per-FLEXA-block compose mode
 *       (BK_GPU_IOCTL_SET_OSD_BY_FLEXA). Stop any frame / flexa callbacks that
 *       will touch this overlay before calling delete().
 */
avdk_err_t bk_gpu_overlay_new(bk_gpu_overlay_handle_t *handle, bk_gpu_ctlr_handle_t gpu);

/**
 * @brief Destroy an overlay instance and free all leased layers.
 *
 * Clears every layer (invoking pending buffer_release_cb), frees backing
 * captures, then releases the object. @p handle must not be used afterwards.
 * Do not call on an overlay obtained from bk_gpu_get_overlay(); that one is
 * owned by the controller and torn down on controller delete.
 *
 * @param[in] handle  Overlay from bk_gpu_overlay_new(); NULL → AVDK_ERR_INVAL.
 *
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t bk_gpu_overlay_delete(bk_gpu_overlay_handle_t handle);

/**
 * @brief Get the single overlay bound to @p gpu, creating it on first use.
 *
 * All SDK consumers that share one controller (bk_draw_osd, the display
 * compositor, PIP) call this so their layers land on the same output frame.
 * The overlay is owned by the controller and destroyed with it — callers must
 * NOT bk_gpu_overlay_delete() the returned handle.
 *
 * @param[in]  gpu   Opened GPU controller.
 * @param[out] out   Receives the shared overlay handle on success.
 *
 * @return AVDK_ERR_OK on success;
 *         AVDK_ERR_INVAL on NULL args;
 *         AVDK_ERR_BUSY if the controller is already driven by bk_gpu_blit_set();
 *         AVDK_ERR_NOMEM if allocation fails.
 */
avdk_err_t bk_gpu_get_overlay(bk_gpu_ctlr_handle_t gpu, bk_gpu_overlay_handle_t *out);

/**
 * @brief Lease one free layer slot.
 *
 * @param[in]  handle  Overlay instance.
 * @param[in]  desc    Lifetime policy: z_order and backing_policy
 *                     (must be BACKING_REQUIRED or BACKING_NONE).
 * @param[out] layer   Receives an opaque non-zero layer handle.
 *
 * @return AVDK_ERR_OK on success;
 *         AVDK_ERR_INVAL on bad args / deleting overlay;
 *         AVDK_ERR_NOMEM if all BK_GPU_OVERLAY_LAYER_MAX slots are in use.
 *
 * @note Geometry and pixel buffers are not set here — use layer_submit().
 *       Re-acquire (after release) if backing_policy must change.
 */
avdk_err_t bk_gpu_overlay_layer_acquire( bk_gpu_overlay_handle_t handle,
                      const bk_gpu_overlay_layer_desc_t *desc, bk_gpu_overlay_layer_handle_t *layer);

/**
 * @brief Queue a full-buffer blit update for a leased layer.
 *
 * Marks the layer dirty so the next pipeline compose (frame or FLEXA block)
 * draws @p src_buffer at config->dst_x/y with the given format/rotation/blend.
 * Overlay takes ownership of @p src_buffer until config->buffer_release_cb runs.
 *
 * @param[in] handle     Overlay instance.
 * @param[in] layer      Handle from layer_acquire().
 * @param[in] src_buffer Pixel buffer; must remain valid until release_cb.
 * @param[in] config     Size, format, destination, rotation, blend, release_cb
 *                       (release_cb must be non-NULL; rotation 0/90/180/270).
 *
 * @return AVDK_ERR_OK on success;
 *         AVDK_ERR_INVAL on bad handle/config/rotation or stale layer handle;
 *         other errors if the GPU output geometry cannot be queried.
 *
 * @note Submitting again replaces the previous buffer (old release_cb fired).
 *       For sub-rectangle / advanced footprint updates use the internal
 *       layer_submit_region() API (not exported here).
 */
avdk_err_t bk_gpu_overlay_layer_submit( bk_gpu_overlay_handle_t handle, bk_gpu_overlay_layer_handle_t layer,
                                     void *src_buffer, const bk_gpu_overlay_layer_submit_config_t *config);

/**
 * @brief Drop the layer’s current content without releasing the lease.
 *
 * Releases any queued/display buffers via their release_cb, clears dirty
 * content, and (with BACKING_REQUIRED) leaves backing available for a clean
 * restore on the next compose/refresh. The layer handle remains valid for
 * another submit.
 *
 * @param[in] handle  Overlay instance.
 * @param[in] layer   Leased layer handle.
 *
 * @return AVDK_ERR_OK on success; AVDK_ERR_INVAL on bad/stale handle.
 */
avdk_err_t bk_gpu_overlay_layer_clear(bk_gpu_overlay_handle_t handle, bk_gpu_overlay_layer_handle_t layer);

/**
 * @brief End a layer lease and return the slot to the free pool.
 *
 * Equivalent to clear plus freeing backing/capture state and invalidating
 * @p layer. Subsequent use of the same handle returns AVDK_ERR_INVAL.
 *
 * @param[in] handle  Overlay instance.
 * @param[in] layer   Leased layer handle.
 *
 * @return AVDK_ERR_OK on success; AVDK_ERR_INVAL on bad/stale handle.
 */
avdk_err_t bk_gpu_overlay_layer_release(bk_gpu_overlay_handle_t handle, bk_gpu_overlay_layer_handle_t layer);

/**
 * @brief How many layer slots are still free for acquire().
 *
 * @param[in] handle  Overlay instance; NULL → returns 0.
 *
 * @return Count in [0, BK_GPU_OVERLAY_LAYER_MAX].
 *
 * @note Concurrent acquire from another context may still fail with NOMEM
 *       even if this returned non-zero — treat as advisory.
 */
uint8_t bk_gpu_overlay_get_available_layer_count(bk_gpu_overlay_handle_t handle);

/* ------------------------------------------------------------------------- */
/* Pipeline compose API (compositor / GPU controller only).                  */
/*                                                                           */
/* Drives when leased layers are blitted onto the live video frame. Only     */
/* frame producers call these: the display compositor and the GPU            */
/* controller's flexa/frame path. Layer consumers (bk_draw_osd, PIP, product */
/* code) use the lease API above and must not call these directly.           */
/* ------------------------------------------------------------------------- */

/* Rectangle in output-frame pixels. */
typedef struct
{
    uint16_t x, y, width, height;
} bk_gpu_overlay_rect_t;

/* One layer's source pixels and placement for a single submit. */
typedef struct
{
    uint16_t buffer_width;                            /* source buffer stride (full canvas) */
    uint16_t buffer_height;
    uint16_t src_x;                                   /* sub-rect taken from the source */
    uint16_t src_y;
    uint16_t src_width;
    uint16_t src_height;
    bk_pixel_format_t src_format;
    uint16_t dst_x;                                   /* placement on the output frame */
    uint16_t dst_y;
    uint16_t rotation_degree;
    uint8_t enable_alpha_blend;
    bk_gpu_overlay_rect_t footprint_rect;             /* area the layer may touch this frame */
    void *release_user_data;
    void (*buffer_release_cb)(void *buffer, void *release_user_data);
} bk_gpu_overlay_layer_update_config_t;

/* Submit one layer; ownership of src_buffer transfers to the overlay. */
avdk_err_t bk_gpu_overlay_layer_submit_region(
    bk_gpu_overlay_handle_t overlay, bk_gpu_overlay_layer_handle_t layer,
    void *src_buffer, const bk_gpu_overlay_layer_update_config_t *config);

/* Compose all layers / only dirty layers onto a complete output frame. */
avdk_err_t bk_gpu_overlay_compose_frame(bk_gpu_overlay_handle_t overlay, void *dst_buffer);
avdk_err_t bk_gpu_overlay_refresh_dirty_layers(bk_gpu_overlay_handle_t overlay, void *dst_buffer);

/* Compose layers onto the current hardware FLEXA block, then commit at frame end. */
avdk_err_t bk_gpu_overlay_compose_flexa_block(bk_gpu_overlay_handle_t overlay, const bk_gpu_flexa_block_t *block);
avdk_err_t bk_gpu_overlay_commit_flexa_frame(bk_gpu_overlay_handle_t overlay);

#ifdef __cplusplus
}
#endif
