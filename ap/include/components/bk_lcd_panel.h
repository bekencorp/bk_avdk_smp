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
 * @file bk_lcd_panel.h
 * @brief LCD panel public API (factories + control + registry helpers).
 *        Type definitions live in <components/bk_lcd_panel_types.h>.
 */

#include <components/bk_lcd_panel_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef likely
#define likely(x)      (x)
#endif
#ifndef unlikely
#define unlikely(x)    (x)
#endif

/**
 * @brief Create a MIPI-DSI panel handle.
 *
 * Reset polarity / timing live on the descriptor; @p panel_config only
 * carries the RESETn GPIO.
 *
 * @param[in]  bus_handle    DSI bus the panel is attached to.
 * @param[in]  panel_config  Reset pin wiring.
 * @param[in]  panel_desc    Panel descriptor (timing / init_cmds / read_id_regs ...).
 * @param[out] ret_panel     Receives the new panel handle.
 * @return BK_OK; BK_ERR_NULL_PARAM / BK_ERR_NO_MEM on failure.
 */
bk_err_t bk_lcd_mipi_panel_new(bk_display_bus_handle_t bus_handle,
                               const bk_lcd_panel_config_t *panel_config,
                               const bk_display_dsi_panel_t *panel_desc,
                               bk_avdk_lcd_panel_handle_t *ret_panel);

/**
 * @brief Create an RGB panel handle.
 *
 * The SW SPI bus carries the register-init channel; reset polarity /
 * timing live on the descriptor.
 *
 * @param[in]  bus_handle    SW SPI bus.
 * @param[in]  panel_config  Reset pin wiring.
 * @param[in]  panel_desc    RGB panel descriptor.
 * @param[out] ret_panel     Receives the new panel handle.
 * @return BK_OK; BK_ERR_NULL_PARAM / BK_ERR_NO_MEM on failure.
 */
bk_err_t bk_lcd_rgb_panel_new(bk_display_bus_handle_t bus_handle,
                              const bk_lcd_panel_config_t *panel_config,
                              const bk_display_rgb_panel_t *panel_desc,
                              bk_avdk_lcd_panel_handle_t *ret_panel);

/**
 * @brief Free the panel handle. Does not release the owning bus.
 *
 * @param[in] panel Panel handle.
 * @return BK_OK; BK_ERR_NULL_PARAM if @p panel is NULL.
 */
bk_err_t bk_lcd_panel_delete(bk_avdk_lcd_panel_handle_t panel);

/**
 * @brief Read the panel chip ID via the descriptor's @c read_id_regs.
 *
 * Only valid after ::bk_display_init() (which drives reset + runs
 * @c init_cmds, leaving the panel in a state where DCS reads work).
 *
 * @param[in]  panel Panel handle.
 * @param[out] id    Concatenated ID bytes.
 * @return BK_OK; BK_ERR_NOT_SUPPORT when @c read_id_regs is NULL.
 */
bk_err_t bk_lcd_panel_read_id(bk_avdk_lcd_panel_handle_t panel, uint32_t *id);

/**
 * @brief Send a single command (with optional payload) to the panel.
 *
 * Runtime equivalent of one @c init_cmds entry. Use for brightness
 * control (0x51), runtime MADCTL, hot-recover SLPOUT/DISPON, ...
 *
 * @param[in] panel       Panel handle.
 * @param[in] lcd_cmd     Command byte; pass -1 to send raw data only.
 * @param[in] param       Optional parameter buffer.
 * @param[in] param_size  Payload size in bytes (0 = no payload).
 * @return BK_OK; BK_ERR_NULL_PARAM / BK_ERR_NOT_SUPPORT.
 */
bk_err_t bk_lcd_panel_tx_param(bk_avdk_lcd_panel_handle_t panel,
                               int lcd_cmd,
                               const void *param,
                               size_t param_size);

/**
 * @brief Send a command and read parameter bytes back from the panel.
 *
 * Use for status readback (0x09), power mode (0x0A), MADCTL (0x0B),
 * NVM / OTP, ...
 *
 * @param[in]  panel       Panel handle.
 * @param[in]  lcd_cmd     Command byte to send.
 * @param[out] param       Buffer that receives the readback bytes.
 * @param[in]  param_size  Number of bytes to read into @p param.
 * @return BK_OK; BK_ERR_NULL_PARAM / BK_ERR_NOT_SUPPORT.
 */
bk_err_t bk_lcd_panel_rx_param(bk_avdk_lcd_panel_handle_t panel,
                               int lcd_cmd,
                               void *param,
                               size_t param_size);

/**
 * @name Default panel @c init / @c reset implementations
 *
 * Plug into ::bk_display_dsi_panel_t / ::bk_display_rgb_panel_t. A custom
 * impl may compose by calling the default first.
 * @{
 */

/**
 * @brief Send the descriptor's @c init_cmds DCS sequence (DSI).
 * @param[in] panel Panel handle.
 * @return BK_OK; channel error code on bus failure.
 */
bk_err_t bk_lcd_mipi_default_init(bk_avdk_lcd_panel_t *panel);

/**
 * @brief Drive the GPIO reset waveform per @c reset_active_level + @c reset_timing (DSI).
 * @param[in] panel Panel handle.
 * @return BK_OK; no-op when @c reset_pin < 0.
 */
bk_err_t bk_lcd_mipi_default_reset(bk_avdk_lcd_panel_t *panel);

/**
 * @brief Send the descriptor's @c off_cmds DCS power-down sequence (DSI).
 *
 * Plug into ::bk_display_dsi_panel_t::off. Runs during
 * ::bk_display_deinit() while the DSI command channel is still up, so the
 * panel can be walked to DISPOFF / SLPIN before power is removed.
 *
 * @param[in] panel Panel handle.
 * @return BK_OK (also when @c off_cmds is NULL); channel error on bus failure.
 */
bk_err_t bk_lcd_mipi_default_off(bk_avdk_lcd_panel_t *panel);

/**
 * @brief Send the descriptor's @c init_cmds SPI sequence (RGB).
 * @param[in] panel Panel handle.
 * @return BK_OK; channel error code on bus failure.
 */
bk_err_t bk_lcd_rgb_default_init(bk_avdk_lcd_panel_t *panel);

/**
 * @brief Drive the GPIO reset waveform per @c reset_active_level + @c reset_timing (RGB).
 * @param[in] panel Panel handle.
 * @return BK_OK; no-op when @c reset_pin < 0.
 */
bk_err_t bk_lcd_rgb_default_reset(bk_avdk_lcd_panel_t *panel);

/**
 * @brief Send the descriptor's @c off_cmds SPI sequence (RGB).
 * @param[in] panel Panel handle.
 * @return BK_OK (also when @c off_cmds is NULL); channel error on bus failure.
 */
bk_err_t bk_lcd_rgb_default_off(bk_avdk_lcd_panel_t *panel);
/** @} */

/**
 * @name Section-based panel registry helpers
 *
 * Enumerate / look up panels registered through ::BK_LCD_PANEL_DEVICE_SECTION.
 * The list APIs return the number of entries written into @p panels (capped
 * at @p max_count); the find APIs return NULL when no name matches.
 *
 * @note BK7259 has no MCU/8080 LCD bus - no MCU helpers exist.
 * @{
 */

/**
 * @brief Enumerate all registered MIPI-DSI panels.
 * @param[out] panels    Output array.
 * @param[in]  max_count Capacity of @p panels.
 * @return Number of entries written.
 */
uint32_t bk_lcd_get_mipi_panel_list(const bk_display_dsi_panel_t **panels, uint32_t max_count);

/**
 * @brief Enumerate all registered parallel RGB panels.
 * @param[out] panels    Output array.
 * @param[in]  max_count Capacity of @p panels.
 * @return Number of entries written.
 */
uint32_t bk_lcd_get_rgb_panel_list(const bk_display_rgb_panel_t **panels, uint32_t max_count);

/**
 * @brief Enumerate all registered SPI panels.
 * @param[out] panels    Output array.
 * @param[in]  max_count Capacity of @p panels.
 * @return Number of entries written.
 */
uint32_t bk_lcd_get_spi_panel_list(const bk_display_spi_panel_t **panels, uint32_t max_count);

/**
 * @brief Enumerate all registered QSPI panels.
 * @param[out] panels    Output array.
 * @param[in]  max_count Capacity of @p panels.
 * @return Number of entries written.
 */
uint32_t bk_lcd_get_qspi_panel_list(const bk_display_qspi_panel_t **panels, uint32_t max_count);

/**
 * @brief Find a registered MIPI-DSI panel by name (matches descriptor's @c .name).
 * @param[in] name Panel name.
 * @return Panel descriptor, or NULL when no match.
 */
const bk_display_dsi_panel_t *bk_lcd_find_mipi_panel_by_name(const char *name);

/**
 * @brief Find a registered RGB panel by name.
 * @param[in] name Panel name.
 * @return Panel descriptor, or NULL when no match.
 */
const bk_display_rgb_panel_t *bk_lcd_find_rgb_panel_by_name(const char *name);

/**
 * @brief Find a registered SPI panel by name.
 * @param[in] name Panel name.
 * @return Panel descriptor, or NULL when no match.
 */
const bk_display_spi_panel_t *bk_lcd_find_spi_panel_by_name(const char *name);

/**
 * @brief Find a registered QSPI panel by name.
 * @param[in] name Panel name.
 * @return Panel descriptor, or NULL when no match.
 */
const bk_display_qspi_panel_t *bk_lcd_find_qspi_panel_by_name(const char *name);
/** @} */

#ifdef __cplusplus
}
#endif
