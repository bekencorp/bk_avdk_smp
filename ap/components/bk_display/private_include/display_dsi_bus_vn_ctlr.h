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
 * @file display_dsi_bus_vn_ctlr.h
 * @brief DSI bus virtual node controller. Internal to bk_display.
 *        Wraps avdk_driver mipi_dsi as a ::bk_display_bus_ctlr_t.
 */

#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel.h>
#include "bk_display_bus_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/** DSI bus controller body. */
typedef struct
{
    bk_display_bus_ctlr_t ops;          /**< embedded base, target of __containerof */
    dpu_clk_src_t dsi_clk_src;          /**< latched by set_clock_src */
} dsi_bus_vn_ctlr_t;

/**
 * @brief Internal MIPI-DSI panel factory.
 *
 * Backs the public ::bk_lcd_mipi_panel_new(); panel-common builds the
 * ops table from the descriptor.
 *
 * @param[in]  bus_handle        DSI bus.
 * @param[in]  panel_dev_config  Reset pin + reset polarity.
 * @param[in]  panel_desc        Panel descriptor.
 * @param[out] ret_panel         Receives the new panel handle.
 *
 * @return BK_OK on success.
 */
bk_err_t bk_lcd_new_mipi_panel_common(bk_display_bus_handle_t bus_handle,
                                      const bk_lcd_panel_dev_config_t *panel_dev_config,
                                      const bk_display_dsi_panel_t *panel_desc,
                                      bk_avdk_lcd_panel_handle_t *ret_panel);

/**
 * @brief Internal RGB panel factory.
 *
 * Backs the public ::bk_lcd_rgb_panel_new().
 *
 * @param[in]  bus_handle        SW SPI bus that carries the register-init channel.
 * @param[in]  panel_dev_config  Reset pin + reset polarity.
 * @param[in]  panel_desc        RGB panel descriptor.
 * @param[out] ret_panel         Receives the new panel handle.
 *
 * @return BK_OK on success.
 */
bk_err_t bk_lcd_new_rgb_panel_common(bk_display_bus_handle_t bus_handle,
                                     const bk_lcd_panel_dev_config_t *panel_dev_config,
                                     const bk_display_rgb_panel_t *panel_desc,
                                     bk_avdk_lcd_panel_handle_t *ret_panel);

#ifdef __cplusplus
}
#endif
