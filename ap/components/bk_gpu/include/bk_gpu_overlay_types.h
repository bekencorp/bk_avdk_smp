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
 * @file bk_gpu_overlay_types.h
 * @brief Internal types for bk_gpu_overlay (layer lease / blit compositor).
 *
 * Not part of the application API. bk_gpu_overlay sits on top of one bk_gpu
 * controller and manages up to BK_GPU_OVERLAY_LAYER_MAX independent layers
 * (OSD sprites, PIP windows, …). SDK components (bk_draw_osd, the display
 * compositor, PIP, AOV) use these; product code drives OSD/PIP through the
 * higher-level bk_draw_osd / bk_display_overlay APIs, which take a GPU
 * controller handle and fetch the shared overlay via bk_gpu_get_overlay().
 *
 * Composition timing (frame-done vs per-FLEXA-block) is NOT configured here.
 * It lives on the GPU controller and is switched at runtime via
 * bk_gpu_ioctl(..., BK_GPU_IOCTL_SET_OSD_BY_FLEXA, &per_flexa).
 * The enum bk_gpu_overlay_render_mode_t documents the two modes for clients
 * that keep a local copy (e.g. a display compositor).
 */

#include <stdint.h>
#include <common/avdk_pixel_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum concurrently leased layers on one overlay instance (hardware blit slots). */
#define BK_GPU_OVERLAY_LAYER_MAX 4U

/**
 * Invalid / unowned layer handle.
 * Valid handles are non-zero opaque tokens (generation + slot encoding);
 * never treat the numeric value as a slot index.
 */
#define BK_GPU_OVERLAY_LAYER_INVALID 0U

/** Bitmask covering every layer slot (e.g. for internal “all layers” ops). */
#define BK_GPU_OVERLAY_ALL_LAYERS ((1UL << BK_GPU_OVERLAY_LAYER_MAX) - 1UL)

/** Opaque overlay compositor instance (one per bound GPU controller). */
typedef struct bk_gpu_overlay *bk_gpu_overlay_handle_t;

/**
 * Opaque leased-layer token from layer_acquire().
 * Pass the same value to submit/clear/release; do not invent or reuse values.
 */
typedef uint32_t bk_gpu_overlay_layer_handle_t;

/**
 * When the GPU pipeline applies overlay layers onto the main picture.
 *
 * Not a field of bk_gpu_ctlr_config_t. After bk_gpu_ctlr_new the GPU defaults
 * to AT_FRAME_DONE; change with:
 *   bool per_flexa = (mode == BK_GPU_OVERLAY_RENDER_PER_FLEXA_BLOCK);
 *   bk_gpu_ioctl(gpu, BK_GPU_IOCTL_SET_OSD_BY_FLEXA, &per_flexa);
 */
typedef enum
{
    /**
     * Compose once after the full GPU output frame is ready (frame_done).
     * Client (or compositor) typically calls the internal compose_frame() on
     * the finished buffer, then pushes it to DPU / display.
     * Default after GPU controller creation.
     */
    BK_GPU_OVERLAY_RENDER_AT_FRAME_DONE = 0,

    /**
     * Compose per FLEXA block while the main picture is still being produced
     * (flexa_line_done). Lower latency for early OSD/PIP on streaming paths;
     * requires a valid flexa_line_done callback and matching compositor logic
     * (compose_flexa_block + commit_flexa_frame on success).
     */
    BK_GPU_OVERLAY_RENDER_PER_FLEXA_BLOCK,
} bk_gpu_overlay_render_mode_t;

/**
 * Synchronous view of the FLEXA block currently owned by the GPU worker.
 *
 * Internal (not part of the application API): produced by the GPU controller
 * and handed to the overlay's per-block compose hook while a FLEXA frame
 * streams out. The view and native_target are valid only for the duration of
 * that hook call; the controller can also expose it via the internal
 * BK_GPU_IOCTL_GET_FLEXA_BLOCK while its flexa callback runs.
 */
typedef struct
{
    uint16_t block_index;
    int32_t frame_x;
    int32_t frame_y;
    uint16_t frame_width;
    uint16_t frame_height;
    uint16_t frame_total_width;
    uint16_t frame_total_height;
    uint16_t local_width;
    uint16_t local_height;
    uint8_t compress_mode;
    void *memory;
    void *native_target;
} bk_gpu_flexa_block_t;

/**
 * Whether a layer keeps a clean-background “backing” under its footprint.
 *
 * Fixed for the layer’s lease lifetime (set at acquire). Prefer matching the
 * content semantics: translucent OSD → REQUIRED; opaque PIP → NONE.
 */
typedef enum
{
    /**
     * Capture/restore the covered main-picture pixels under the layer.
     * Needed for alpha-blended content, partial updates, idle refresh, and
     * clean removal without leaving stale sprites on the frame.
     */
    BK_GPU_OVERLAY_BACKING_REQUIRED = 0,

    /**
     * No backing: the layer fully replaces pixels in its footprint.
     * Use for opaque content (e.g. PIP NV12) that does not need restore on
     * clear/move. Avoid for translucent OSD.
     */
    BK_GPU_OVERLAY_BACKING_NONE,
} bk_gpu_overlay_backing_policy_t;

/**
 * Layer lease descriptor (passed to bk_gpu_overlay_layer_acquire).
 * Describes lifetime policy; geometry and buffers are supplied later via submit.
 */
typedef struct
{
    /**
     * Draw order among leased layers. Higher values are drawn later (on top).
     * Relative ordering only; absolute values need not be contiguous.
     */
    int16_t z_order;

    /**
     * Backing policy for this lease; see bk_gpu_overlay_backing_policy_t.
     * Changing it after acquire is not supported — release and re-acquire.
     */
    bk_gpu_overlay_backing_policy_t backing_policy;
} bk_gpu_overlay_layer_desc_t;

/**
 * Per-submit blit parameters for bk_gpu_overlay_layer_submit().
 *
 * Coordinates are in the GPU output / panel space (same as dst_width/dst_height
 * of the bound GPU). The public submit API treats the whole @p src_buffer as
 * the source rectangle (origin 0,0; size width×height).
 *
 * Ownership: overlay takes ownership of @p src_buffer for the update cycle and
 * invokes buffer_release_cb when the buffer is no longer needed (including on
 * replace/clear/delete). buffer_release_cb must be non-NULL.
 */
typedef struct
{
    /** Source buffer width in pixels (full buffer; also used as blit src size). */
    uint16_t width;
    /** Source buffer height in pixels. */
    uint16_t height;
    /** Pixel format of src_buffer (e.g. ARGB8888 for OSD, NV12 for PIP). */
    bk_pixel_format_t src_format;
    /** Destination top-left X on the composed frame. */
    uint16_t dst_x;
    /** Destination top-left Y on the composed frame. */
    uint16_t dst_y;
    /**
     * Clockwise rotation of the source onto the destination: 0 / 90 / 180 / 270.
     * Other values are rejected. 90/270 swap the on-screen footprint width/height.
     */
    uint16_t rotation_degree;
    /**
     * Non-zero: SRC_OVER (or equivalent) alpha blend onto the target.
     * Zero: replace / opaque blit. Pair with BACKING_REQUIRED when alpha is used.
     */
    uint8_t enable_alpha_blend;
    /** Cookie passed to buffer_release_cb as the second argument. */
    void *release_user_data;
    /**
     * Called when overlay releases @p buffer (may run after a later submit
     * replaces this one, or on clear/delete). Must not be NULL.
     */
    void (*buffer_release_cb)(void *buffer, void *release_user_data);
} bk_gpu_overlay_layer_submit_config_t;

#ifdef __cplusplus
}
#endif
