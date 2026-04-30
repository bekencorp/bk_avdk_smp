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

#include <components/bk_display_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    bk_display_bus_ctlr_t ops;
    bk_lcd_bus_io_t *dsi_handle;
    /** MIPI DSI PHY path; set from bk_display_dsi_bus_new(..., &{ .clk_src = dpu_config.clk_src }). */
    dpu_clk_src_t dsi_clk_src;
} dsi_bus_vn_ctlr_t;

/**
 * @brief Create a new MIPI panel
 *
 * @param[in] bus_handle Display bus handle
 * @param[in] panel_dev_config Panel device configuration
 * @param[in] panel_desc Panel descriptor
 * @param[out] ret_panel Returned panel handle
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_new_mipi_panel_common(bk_display_bus_handle_t bus_handle,
    const bk_lcd_panel_dev_config_t *panel_dev_config,
    const bk_display_dsi_panel_t *panel_desc,
    bk_avdk_lcd_panel_handle_t *ret_panel);

/**
 * @brief Create a new RGB panel
 *
 * @param[in] bus_handle Display bus handle
 * @param[in] panel_dev_config Panel device configuration
 * @param[in] panel_desc Panel descriptor
 * @param[out] ret_panel Returned panel handle
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_new_rgb_panel_common(bk_display_bus_handle_t bus_handle,
        const bk_lcd_panel_dev_config_t *panel_dev_config,
        const bk_display_rgb_panel_t *panel_desc,
        bk_avdk_lcd_panel_handle_t *ret_panel);

#ifdef __cplusplus
}
#endif

