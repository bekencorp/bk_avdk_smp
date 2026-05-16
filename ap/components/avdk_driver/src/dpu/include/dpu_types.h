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
 * @file dpu_types.h
 * @brief Public DPU type definitions consumed by both the bk_display
 *        component and direct avdk_driver users.
 */

#include <stdint.h>
#include <stdbool.h>
#include <avdk_error.h>
#include <common/avdk_pixel_types.h>

/** DPU layer index. */
typedef enum {
    DPU_LAYER_VIDEO,      /**< video / pixel layer (typical YUV/RGB frame) */
    DPU_LAYER_GRAPHIC,    /**< graphics / OSD overlay layer */
    DPU_LAYER_MAX,
} dpu_layer_t;

/** Per-layer blend mode. */
typedef enum {
    DPU_BLEND_MODE_CLEAR,
    DPU_BLEND_MODE_SRC,
    DPU_BLEND_MODE_DST,
    DPU_BLEND_MODE_SRC_OVER = 12,   /**< dst alpha must be 1 */
    DPU_BLEND_MODE_DST_OVER = 13,   /**< dst alpha must be 1 */
} dup_blend_mode_t;

/** Video layer configuration. */
typedef struct {
    bool enable;
    bool decompress;              /**< true = enable on-the-fly decompress */
    bk_pixel_format_t format;
    uint32_t   disp_x;            /**< rectangle start X */
    uint32_t   disp_y;            /**< rectangle start Y */
    uint32_t   disp_w;            /**< rectangle width  */
    uint32_t   disp_h;            /**< rectangle height */
} dpu_video_layer_config_t;

/** Graphics / OSD layer configuration. */
typedef struct {
    bool enable;
    dup_blend_mode_t blend_mode;
    bk_pixel_format_t format;
    uint32_t   disp_x;
    uint32_t   disp_y;
    uint32_t   disp_w;
    uint32_t   disp_h;
} dpu_graphic_layer_config_t;

/** Async free callback for ::bk_display_flush() / dpu_core_flush(). */
typedef avdk_err_t (*flush_free_cb_t)(void *args);
