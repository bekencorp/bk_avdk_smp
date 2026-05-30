/*
 * SPDX-FileCopyrightText: 2024 Beken Corp.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal panel ops table + bring-up helpers consumed by the DPU
 * controller and the panel-common backends. Not part of the public ABI.
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
 * ::bk_avdk_lcd_panel_handle_t cookie. @c clk_src is latched by the
 * panel @c init op and consumed by the DPU controller to program the
 * register clock mux. @c pixel_clock_hz is cached at panel creation
 * (DSI: fps * h_total * v_total).
 */
struct bk_avdk_lcd_panel_t {
    bk_display_bus_handle_t bus;
    bk_display_timing_t timing;
    uint32_t pixel_clock_hz;
    dpu_clk_src_t clk_src;

    bk_err_t (*reset)(bk_avdk_lcd_panel_t *panel);
    bk_err_t (*init)(bk_avdk_lcd_panel_t *panel);
    bk_err_t (*del)(bk_avdk_lcd_panel_t *panel);
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

/**
 * @brief Drive the descriptor's @c reset op (called by dpu_ctlr_init / deinit).
 *
 * Apps don't call this directly - bring-up / tear-down is implicit in
 * ::bk_display_init() / ::bk_display_deinit().
 *
 * @param[in] panel Panel handle.
 * @return BK_OK; BK_ERR_NULL_PARAM if @p panel is NULL.
 */
bk_err_t bk_lcd_panel_reset(bk_avdk_lcd_panel_handle_t panel);

/**
 * @brief Set the bus clock and dispatch to the descriptor's @c init op
 *        (called by dpu_ctlr_init).
 *
 * @param[in] panel Panel handle.
 * @return BK_OK; BK_ERR_NULL_PARAM / channel error code on failure.
 */
bk_err_t bk_lcd_panel_init(bk_avdk_lcd_panel_handle_t panel);

#ifdef __cplusplus
}
#endif
