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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Panel ops table.
 *
 * panel-common casts a private struct (whose first member is a
 * ::bk_avdk_lcd_panel_t) to / from the public
 * ::bk_avdk_lcd_panel_handle_t cookie.
 */
struct bk_avdk_lcd_panel_t {
    bk_err_t (*reset)(bk_avdk_lcd_panel_t *panel);
    bk_err_t (*init)(bk_avdk_lcd_panel_t *panel);
    bk_err_t (*del)(bk_avdk_lcd_panel_t *panel);
    bk_err_t (*disp_on_off)(bk_avdk_lcd_panel_t *panel, bool on_off);
    bk_err_t (*read_id)(bk_avdk_lcd_panel_t *panel, uint32_t *id);
    /**
     * Send a single command (+ optional payload) over the bus' command
     * channel. Backends typically forward to ::bk_display_bus_tx_param().
     */
    bk_err_t (*tx_param)(bk_avdk_lcd_panel_t *panel,
                         int lcd_cmd,
                         const void *param,
                         size_t param_size);
    /**
     * Send a command and read parameter bytes back. Backends typically
     * forward to ::bk_display_bus_rx_param().
     */
    bk_err_t (*rx_param)(bk_avdk_lcd_panel_t *panel,
                         int lcd_cmd,
                         void *param,
                         size_t param_size);
};

#ifdef __cplusplus
}
#endif
