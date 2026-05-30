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
 * @brief DPU register clock mux selector.
 *
 * Carried on each panel via ::bk_lcd_panel_config_t::clk_src; the
 * panel-common factory pushes it to the bus at creation time and the
 * DPU controller reads it back when programming the DPU clock mux. The
 * DSI D-PHY itself is always brought up by the unified
 * mipi_dsi_clock_set(); the PHY register layout is the same regardless
 * of this enum value, only the fallback strategy when the PHY's
 * internal PLL cannot satisfy the panel's lane:pclk ratio differs:
 *
 *   - ::DPU_CLK_SRC_SYSCLK    : DPU sources DPI from the SYSCLK ladder;
 *                               the PHY's unused dpi_clk lets us fall
 *                               back to a fixed-rate lane lookup (and
 *                               default 800 Mbps as last resort).
 *   - ::DPU_CLK_SRC_DPHY_DPLL : DPU consumes the PHY's dpi_clk directly,
 *                               so a PLL miss is a hard error - retry
 *                               with ::DPU_CLK_SRC_SYSCLK.
 */
typedef enum {
    DPU_CLK_SRC_UNKNOWN   = 0,                /**< treated as DPU_CLK_SRC_DPHY_DPLL */
    DPU_CLK_SRC_SYSCLK    = 1,                /**< DPU clock from SYSCLK ladder via dpu_clk_sel_div() */
    DPU_CLK_SRC_DPHY_DPLL = 2,                /**< DPU clock from Naneng D-PHY internal dpi_clk output */
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
 * The actual DPI pixel clock is derived from @c fps * (h_total *
 * v_total) inside the driver; the panel descriptor no longer carries a
 * pre-computed @c clk value.
 */
typedef struct bk_panel_clock_config_t
{
    uint8_t  n_lanes;            /**< MIPI active data lanes (1~4) */
    uint8_t  fps;                /**< frame rate */
    dpu_clk_src_t clk_src;       /**< DPU register clock mux selector */
    bk_display_timing_t timing;  /**< DPU video timing */
} bk_panel_clock_config_t;
