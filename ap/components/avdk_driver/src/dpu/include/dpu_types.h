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

#include "components/bk_lcd_types.h"

typedef enum {
    DPU_LAYER_VIDEO,
    DPU_LAYER_GRAPHIC,
    DPU_LAYER_MAX,
} dpu_layer_t;

typedef enum {
    DPU_BLEND_MODE_CLEAR,
    DPU_BLEND_MODE_SRC,
    DPU_BLEND_MODE_DST,
    DPU_BLEND_MODE_SRC_OVER = 12,   // dst alpha value must be 1
    DPU_BLEND_MODE_DST_OVER = 13,   // dst alpha value must be 1
} dup_blend_mode_t;

typedef struct {
    bool enable;
    bool decompress;
    bk_pixel_format_t format;
    uint32_t   disp_x;         /* Rectangle start point X coordinate */
    uint32_t   disp_y;         /* Rectangle start point Y coordinate */
    uint32_t   disp_w;         /* Rectangle width*/
    uint32_t   disp_h;         /* Rectangle height */
} dpu_video_layer_config_t;

typedef struct {
    bool enable;
    dup_blend_mode_t blend_mode;
    bk_pixel_format_t format;
    uint32_t   disp_x;   
    uint32_t   disp_y;       
    uint32_t   disp_w;       
    uint32_t   disp_h;       
} dpu_graphic_layer_config_t;


typedef avdk_err_t (*flush_free_cb_t)(void *args);
