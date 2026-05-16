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

#include <stdint.h>
#include <stdbool.h>
#include <avdk_error.h>
#include <common/avdk_pixel_types.h>      /* bk_pixel_format_t */

/**
 * @brief MIPI DPU / DSI clock root.
 *
 * Selects the DPU register clock mux and the DSI PHY init path used by
 * mipi_dsi_clock_set(). Lives in the driver-layer header so that the
 * driver source files can depend on it without pulling in the
 * higher-level component headers (R1.1 strict layering).
 */
typedef enum {
    DPU_CLK_SRC_UNKNOWN = 0,                     /**< default: Naneng DPHY internal PLL + byte-cycle VID timing */
    DPU_CLK_SRC_SYSCLK = 1,                   /**< legacy: fixed DPHY table from dsi_dphy_bitrate_calc + hal_dsi_dphy_init */
    DPU_CLK_SRC_DPHY_DPLL = 2,                /**< DPHY internal PLL + hal_dsi_dphy_init_for_panel */
} dpu_clk_src_t;

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

/** Video timing parameters shared by panel descriptors / bus clock / DPU. */
typedef struct {
    uint32_t clk;
    uint16_t h_size;            /*!< Horizontal resolution (active pixels) */
    uint16_t v_size;            /*!< Vertical resolution (active lines) */
    uint16_t hsync_pulse_width; /*!< HSYNC width, in pixel clocks */
    uint16_t vsync_pulse_width; /*!< VSYNC width, in lines */
    uint16_t hsync_back_porch;  /*!< HBP, pixel clocks between HSYNC and active */
    uint16_t hsync_front_porch; /*!< HFP, pixel clocks between active and next HSYNC */
    uint16_t vsync_back_porch;  /*!< VBP, lines between VSYNC and frame start */
    uint16_t vsync_front_porch; /*!< VFP, lines between frame end and next VSYNC */
} bk_display_timing_t;

/**
 * @brief Runtime clock-update payload shared between bus / driver layers.
 *
 * Lives in the driver-layer header so mipi_dsi_clock_set() can take it
 * without pulling in component-layer headers (R1.1 strict layering).
 */
typedef struct bk_panel_clock_config_t
{
    uint32_t clk;                /**< MIPI lcd clock */
    uint8_t  n_lanes;            /**< MIPI active data lanes (1~4) */
    uint8_t  fps;                /**< frame rate */
    dpu_clk_src_t clk_src;       /**< DSI PHY / VID timing path */
    bk_display_timing_t timing;  /**< DPU video timing */
} bk_panel_clock_config_t;
