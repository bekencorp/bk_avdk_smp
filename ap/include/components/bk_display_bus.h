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
 * @file bk_display_bus.h
 * @brief LCD display bus domain (DSI / SPI). Pulled in by the umbrella
 *        ::components/bk_display.h.
 */

#include <stdint.h>
#include <stddef.h>
#include <avdk_error.h>
#include <driver/dpu_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque display bus handle. Body lives in private_include/. */
typedef struct bk_display_bus_ctlr_t *bk_display_bus_handle_t;

/**
 * @brief MIPI DSI bus creation parameters.
 *
 * ::bk_display_dsi_bus_new() accepts NULL today; this struct is reserved
 * for future per-board options.
 */
typedef struct
{
    uint8_t reserved;
} bk_display_dsi_bus_config_t;

/**
 * @brief SPI bus configuration for GPIO bit-bang command channel.
 *
 * Used by RGB panels for register initialization. SPI LCD display controllers
 * use ::bk_display_spi_ctlr_config_t instead and do not depend on this bus.
 */
typedef struct
{
    uint8_t clk_pin;                      /**< SCK */
    uint8_t csx_pin;                      /**< CS  */
    uint8_t sda_pin;                      /**< MOSI */
    /**
     * Wire format selector. Copy from the chosen panel descriptor as
     * ``cmd_width = panel->spi_cmd_16bit ? 16 : 8``.
     *  - 8  = 9-bit SPI (D/C bit prepended) - ST7701S / GC9503 / AML01
     *  - 16 = 4-byte packed SPI            - NT35512 / NT35510
     */
    uint8_t cmd_width;
} bk_display_spi_bus_config_t;

/* ::bk_panel_clock_config_t is defined in <driver/dpu_types.h> (see the
 * include of dpu_types via the bk_lcd_panel_types.h chain), so that the
 * driver-layer mipi_dsi_clock_set() can take it without pulling in any
 * component header. */

/**
 * @brief Create a MIPI DSI bus controller.
 *
 * Performs MIPI sub-domain vote-on, the SoC-internal D-PHY analog
 * rail vote (PMU AUXLDO 2.8V + 3V, file-static inside
 * bk_display_dsi_bus.c) and mipi_dsi_init() in one shot. The DCS
 * generic-write / read primitives are wired directly onto the bus
 * controller's tx_param / rx_param ops, callable through
 * ::bk_display_bus_tx_param() / ::bk_display_bus_rx_param() (raw mode)
 * or ::bk_lcd_panel_tx_param() / ::bk_lcd_panel_rx_param() (panel mode).
 *
 * @param[out] handle Bus handle.
 * @param[in]  config Optional, may be NULL.
 *
 * @return AVDK_ERR_OK on success.
 * @return AVDK_ERR_INVAL if @p handle is NULL.
 * @return AVDK_ERR_NO_MEM if allocation fails.
 */
avdk_err_t bk_display_dsi_bus_new(bk_display_bus_handle_t *handle, bk_display_dsi_bus_config_t *config);

/**
 * @brief Create a SPI bus controller.
 *
 * Muxes clk/csx/sda for bit-bang panel-init writes. The parallel 24-bit RGB
 * pixel lanes are driven directly by the DPU and are not mediated by this bus.
 * Hardware SPI LCD frame output is owned by ::bk_display_spi_ctlr_new(), not
 * by this bus API.
 *
 * @param[out] handle Bus handle.
 * @param[in]  config Bus configuration; must not be NULL.
 *
 * @return AVDK_ERR_OK on success.
 * @return AVDK_ERR_INVAL on bad arguments.
 */
avdk_err_t bk_display_spi_bus_new(bk_display_bus_handle_t *handle, bk_display_spi_bus_config_t *config);

/**
 * @brief Tear down a bus controller and release every resource it owns.
 *
 * @param[in] handle Bus handle.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t bk_display_bus_delete(bk_display_bus_handle_t handle);



/**
 * @brief Submit a frame through the bus' pixel path.
 *
 * Only meaningful for buses that directly mediate pixel data. The SPI command
 * bus does not carry pixel data, so direct SPI bus flush calls return
 * ::AVDK_ERR_UNSUPPORTED.
 *
 * @param[in] handle Bus handle.
 * @param[in] frame  Frame buffer pointer.
 * @param[in] cb     Optional callback invoked when @p frame is no longer in use.
 *
 * @return AVDK_ERR_OK on success.
 * @return AVDK_ERR_UNSUPPORTED if the bus has no pixel path.
 */
avdk_err_t bk_display_bus_flush(bk_display_bus_handle_t handle, uint8_t *frame, flush_free_cb_t cb);

/**
 * @brief Send LCD command + parameters down the bus' command channel.
 *
 * Direct counterpart of ::bk_lcd_panel_tx_param() for callers that
 * already hold a bus handle (typically raw-API examples that own
 * @c bus_handle / @c panel_handle / @c dpu_ctlr_handle separately).
 * Internally @c bk_lcd_panel_tx_param() forwards into this same op via
 * the panel's bus pointer.
 *
 * Wire format depends on the backend:
 *  - DSI : DCS generic write
 *  - SPI 8-bit (cmd_width = 8)  : 9-bit SPI (D/C bit prepended)
 *  - SPI 16-bit (cmd_width = 16): 4-byte packed protocol
 *
 * @param[in] handle      Bus handle.
 * @param[in] lcd_cmd     Command byte; pass -1 to send raw data only.
 * @param[in] param       Optional parameter buffer.
 * @param[in] param_size  Size of @p param in bytes (0 = no payload).
 *
 * @return BK_OK on success.
 * @return BK_ERR_NULL_PARAM on NULL @p handle.
 * @return BK_ERR_NOT_SUPPORT if the bus does not implement command writes.
 */
bk_err_t bk_display_bus_tx_param(bk_display_bus_handle_t handle,
                                 int lcd_cmd,
                                 const void *param,
                                 size_t param_size);

/**
 * @brief Send a command and read parameter bytes back from the panel.
 *
 * Direct counterpart of ::bk_lcd_panel_rx_param() for raw-API callers.
 * Available on DSI today; SW SPI does not support reads in this
 * implementation (returns ::BK_ERR_NOT_SUPPORT).
 *
 * @param[in]  handle      Bus handle.
 * @param[in]  lcd_cmd     Command byte to send.
 * @param[out] param       Buffer that receives the readback bytes.
 * @param[in]  param_size  Number of bytes to read into @p param.
 *
 * @return BK_OK on success.
 * @return BK_ERR_NULL_PARAM on NULL @p handle.
 * @return BK_ERR_NOT_SUPPORT if the bus does not implement command reads.
 */
bk_err_t bk_display_bus_rx_param(bk_display_bus_handle_t handle,
                                 int lcd_cmd,
                                 void *param,
                                 size_t param_size);

/**
 * @brief Override the DPU register clock source for a MIPI-DSI bus.
 *
 * Call between ::bk_display_dsi_bus_new() and ::bk_lcd_mipi_panel_new()
 * to force a specific clock source. When not called, MIPI-DSI defaults
 * to ::DPU_CLK_SRC_DPHY_DPLL with automatic SYSCLK fallback if the PHY
 * PLL cannot satisfy the panel's lane:pclk ratio.
 *
 * @param[in] handle  Bus handle.
 * @param[in] clk_src DPU register clock mux source.
 *
 * @return AVDK_ERR_OK on success.
 * @return AVDK_ERR_INVAL on NULL @p handle.
 * @return AVDK_ERR_UNSUPPORTED for buses without a clock-source notion (SPI).
 */
avdk_err_t bk_display_bus_set_clock_src(bk_display_bus_handle_t handle, dpu_clk_src_t clk_src);

#ifdef __cplusplus
}
#endif
