/*
 * SPDX-FileCopyrightText: 2024 Beken Corp.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal bus controller layout. Consumed by the bus backends
 * (DSI / SPI), panel-common, and the public bus dispatchers.
 * Not part of the public ABI.
 */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <common/bk_err.h>
#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel.h>
#include <driver/dpu_types.h>
#include "bk_lcd_panel_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bus controller ops table.
 *
 * Backends embed this struct as @c ops; the public
 * ::bk_display_bus_handle_t is a pointer to the embedded base. Optional
 * ops MUST be left NULL when the backend cannot service them - the
 * dispatchers in bk_display_bus.c return ::AVDK_ERR_UNSUPPORTED /
 * ::BK_ERR_NOT_SUPPORT in that case.
 *
 * The command channel (tx_param / rx_param) lives directly on the bus,
 * with no separate panel-IO object. SW SPI uses the cmd_width field of
 * ::bk_display_spi_bus_config_t to pick the wire format; the DSI host
 * has a single hardware command channel and ignores it.
 */
typedef struct bk_display_bus_ctlr_t bk_display_bus_ctlr_t;
struct bk_display_bus_ctlr_t
{
    avdk_err_t (*delete)(bk_display_bus_ctlr_t *controller);
    avdk_err_t (*set_clock)(bk_display_bus_ctlr_t *controller, bk_panel_clock_config_t *clock);
    /**
     * Optional. Push the DPU's ::dpu_clk_src_t into the bus so the
     * next ::set_clock() call routes through the right PHY path.
     * Buses without a clock-source notion leave this NULL.
     */
    avdk_err_t (*set_clock_src)(bk_display_bus_ctlr_t *controller, dpu_clk_src_t clk_src);
    avdk_err_t (*flush)(bk_display_bus_ctlr_t *controller, uint8_t *frame, flush_free_cb_t cb);
    /**
     * Optional. Send LCD command + parameters down the bus' command
     * channel (DSI generic write, SW SPI bit-bang, ...). Buses without
     * a command channel (e.g. HW SPI which only carries pixel DMA)
     * leave this NULL.
     */
    bk_err_t (*tx_param)(bk_display_bus_ctlr_t *controller,
                         int lcd_cmd, const void *param, uint16_t param_size);
    /**
     * Optional. Read parameter bytes back after sending a command.
     * Required for ::bk_lcd_panel_read_id() to work.
     */
    bk_err_t (*rx_param)(bk_display_bus_ctlr_t *controller,
                         int lcd_cmd, void *param, uint16_t param_size);
};

/**
 * @brief Drive the bus pixel/byte clock from a panel-common timing snapshot.
 *
 * @param[in] handle Bus handle.
 * @param[in] clock  Clock parameters.
 *
 * @return AVDK_ERR_OK on success.
 * @return AVDK_ERR_INVAL on NULL @p handle.
 * @return AVDK_ERR_UNSUPPORTED if the backend has no set_clock op.
 */
avdk_err_t bk_display_bus_set_clock(bk_display_bus_handle_t handle, bk_panel_clock_config_t *clock);

/* bk_display_bus_set_clock_src() is the public counterpart and is
 * declared in <components/bk_display_bus.h>. */

#ifdef __cplusplus
}
#endif
