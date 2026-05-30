/*
 * SPDX-FileCopyrightText: 2024 Beken Corp.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal panel ops table consumed only by panel-common
 * (lcd_mipi/rgb_panel_common.c). Not part of the public ABI.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <common/bk_err.h>
#include <components/bk_lcd_panel.h>
#include <driver/dpu_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Panel base type.
 *
 * panel-common casts a private struct (whose first member is a
 * ::bk_avdk_lcd_panel_t) to / from the public
 * ::bk_avdk_lcd_panel_handle_t cookie.
 *
 * @c bus is the bus handle the panel was created on. The DPU
 * controller reads @c clk_src back during dpu_ctlr_build_core_config()
 * to program the DPU register clock mux; the bus side @c dsi_clk_src
 * was already pushed by the panel-common factory so the unified
 * mipi_dsi_clock_set() inside ::bk_lcd_panel_init() picks the right
 * fallback strategy.
 *
 * @c timing and @c pixel_clock_hz are cached from the panel descriptor
 * at creation time; DSI derives @c pixel_clock_hz from @c fps *
 * h_total * v_total.
 */
struct bk_avdk_lcd_panel_t {
    bk_display_bus_handle_t bus;
    bk_display_timing_t timing;
    uint32_t pixel_clock_hz;
    dpu_clk_src_t clk_src;

    bk_err_t (*reset)(bk_avdk_lcd_panel_t *panel);
    bk_err_t (*init)(bk_avdk_lcd_panel_t *panel);
    bk_err_t (*del)(bk_avdk_lcd_panel_t *panel);
    bk_err_t (*disp_on_off)(bk_avdk_lcd_panel_t *panel, bool on_off);
    bk_err_t (*read_id)(bk_avdk_lcd_panel_t *panel, uint32_t *id);
    bk_err_t (*tx_param)(bk_avdk_lcd_panel_t *panel,
                         int lcd_cmd,
                         const void *param,
                         size_t param_size);
    bk_err_t (*rx_param)(bk_avdk_lcd_panel_t *panel,
                         int lcd_cmd,
                         void *param,
                         size_t param_size);
};

#ifdef __cplusplus
}
#endif
