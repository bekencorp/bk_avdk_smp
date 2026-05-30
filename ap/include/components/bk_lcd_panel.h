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
 * @brief Drive the panel reset waveform.
 *
 * Toggles the reset GPIO (taken from ::bk_lcd_panel_config_t and the
 * panel descriptor's reset_timing) per the configured polarity.
 *
 * @param[in] panel Panel handle returned by ::bk_lcd_mipi_panel_new() / ::bk_lcd_rgb_panel_new().
 *
 * @return BK_OK on success.
 * @return BK_ERR_NULL_PARAM if @p panel is NULL.
 * @return BK_ERR_NOT_SUPPORT if the backend has no reset op.
 */
bk_err_t bk_lcd_panel_reset(bk_avdk_lcd_panel_handle_t panel);

/**
 * @brief Run the panel initialisation sequence.
 *
 * Sends the descriptor's @c init_cmds over the bus' panel-IO channel,
 * or invokes the descriptor's @c custom_init() hook (used by HDMI bridges).
 *
 * @param[in] panel Panel handle.
 * @return BK_OK on success, BK_ERR_NULL_PARAM if @p panel is NULL.
 */
bk_err_t bk_lcd_panel_init(bk_avdk_lcd_panel_handle_t panel);

/**
 * @brief Free the panel handle.
 *
 * The owning bus is NOT released - call ::bk_display_bus_delete() when
 * the bus is no longer needed.
 *
 * @param[in] panel Panel handle.
 * @return BK_OK on success.
 */
bk_err_t bk_lcd_panel_del(bk_avdk_lcd_panel_handle_t panel);

/**
 * @brief Send DISPON / DISPOFF to the panel.
 *
 * @param[in] panel   Panel handle.
 * @param[in] on_off  true = display ON, false = display OFF.
 * @return BK_OK on success.
 */
bk_err_t bk_lcd_panel_disp_on_off(bk_avdk_lcd_panel_handle_t panel, bool on_off);

/**
 * @brief Read the panel chip ID via the descriptor's @c read_id_regs.
 *
 * @param[in]  panel Panel handle.
 * @param[out] id    Receives the concatenated ID bytes.
 * @return BK_OK on success.
 */
bk_err_t bk_lcd_panel_read_id(bk_avdk_lcd_panel_handle_t panel, uint32_t *id);

/**
 * @brief Send a single command (with optional payload) to the panel.
 *
 * Runtime equivalent of one entry in the panel descriptor's @c init_cmds
 * array. The command is dispatched through the panel's bus command
 * channel (DSI generic write / SPI 9-bit or 16-bit / ...).
 *
 * Typical use cases: bring-up debugging (re-send DISPON / SLPOUT,
 * tweak MADCTL at runtime), brightness control (0x51), runtime
 * orientation switch, etc.
 *
 * Equivalent to calling ::bk_display_bus_tx_param() with the underlying
 * bus handle - convenient when callers already hold a panel handle and
 * the bus handle is hidden inside a higher-level wrapper (e.g. BSP).
 *
 * Example - re-send display-on after a hot-recover sequence:
 * @code
 * bk_lcd_panel_tx_param(panel, 0x11, NULL, 0);   // SLPOUT
 * rtos_delay_milliseconds(120);
 * bk_lcd_panel_tx_param(panel, 0x29, NULL, 0);   // DISPON
 * @endcode
 *
 * @param[in] panel       Panel handle.
 * @param[in] lcd_cmd     Command byte; pass -1 to send raw data only.
 * @param[in] param       Optional parameter buffer.
 * @param[in] param_size  Size of @p param in bytes (0 = no payload).
 *
 * @return BK_OK on success.
 * @return BK_ERR_NULL_PARAM if @p panel is NULL.
 * @return BK_ERR_NOT_SUPPORT if the bus does not implement command writes.
 */
bk_err_t bk_lcd_panel_tx_param(bk_avdk_lcd_panel_handle_t panel,
                               int lcd_cmd,
                               const void *param,
                               size_t param_size);

/**
 * @brief Send a command and read parameter bytes back from the panel.
 *
 * Runtime read on the same command channel used by ::bk_lcd_panel_read_id().
 * Useful for reading display status (0x09), power mode (0x0A), MADCTL (0x0B),
 * NVM / OTP registers, etc.
 *
 * Equivalent to ::bk_display_bus_rx_param() with the underlying bus handle.
 *
 * Example - read display status:
 * @code
 * uint8_t dstat[4] = {0};
 * bk_lcd_panel_rx_param(panel, 0x09, dstat, sizeof(dstat));
 * @endcode
 *
 * @param[in]  panel       Panel handle.
 * @param[in]  lcd_cmd     Command byte to send.
 * @param[out] param       Buffer that receives the readback bytes.
 * @param[in]  param_size  Number of bytes to read into @p param.
 *
 * @return BK_OK on success.
 * @return BK_ERR_NULL_PARAM if @p panel or @p param is NULL.
 * @return BK_ERR_NOT_SUPPORT if the bus does not implement command reads.
 */
bk_err_t bk_lcd_panel_rx_param(bk_avdk_lcd_panel_handle_t panel,
                               int lcd_cmd,
                               void *param,
                               size_t param_size);

/**
 * @brief Create a MIPI-DSI panel handle.
 *
 * @param[in]  bus_handle        DSI bus the panel is attached to.
 * @param[in]  panel_config  Reset pin + reset polarity.
 * @param[in]  panel_desc        Panel descriptor (timing + init_cmds + read_id_regs ...).
 * @param[out] ret_panel         Receives the new panel handle.
 *
 * @return BK_OK on success, BK_ERR_NULL_PARAM if any required argument is NULL.
 */
bk_err_t bk_lcd_mipi_panel_new(bk_display_bus_handle_t bus_handle,
                               const bk_lcd_panel_config_t *panel_config,
                               const bk_display_dsi_panel_t *panel_desc,
                               bk_avdk_lcd_panel_handle_t *ret_panel);

/**
 * @brief Create an RGB panel handle.
 *
 * @param[in]  bus_handle        SW SPI bus carrying the register-init channel.
 * @param[in]  panel_config  Reset pin + reset polarity.
 * @param[in]  panel_desc        RGB panel descriptor.
 * @param[out] ret_panel         Receives the new panel handle.
 *
 * @return BK_OK on success.
 */
bk_err_t bk_lcd_rgb_panel_new(bk_display_bus_handle_t bus_handle,
                              const bk_lcd_panel_config_t *panel_config,
                              const bk_display_rgb_panel_t *panel_desc,
                              bk_avdk_lcd_panel_handle_t *ret_panel);

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
 *
 * @param[out] panels    Output array.
 * @param[in]  max_count Capacity of @p panels.
 * @return Number of entries written.
 */
uint32_t bk_lcd_get_mipi_panel_list(const bk_display_dsi_panel_t **panels, uint32_t max_count);

/**
 * @brief Enumerate all registered parallel RGB panels.
 *
 * @param[out] panels    Output array.
 * @param[in]  max_count Capacity of @p panels.
 * @return Number of entries written.
 */
uint32_t bk_lcd_get_rgb_panel_list(const bk_display_rgb_panel_t **panels, uint32_t max_count);

/**
 * @brief Enumerate all registered SPI panels.
 *
 * @param[out] panels    Output array.
 * @param[in]  max_count Capacity of @p panels.
 * @return Number of entries written.
 */
uint32_t bk_lcd_get_spi_panel_list(const lcd_device_t **panels, uint32_t max_count);

/**
 * @brief Enumerate all registered QSPI panels.
 *
 * @param[out] panels    Output array.
 * @param[in]  max_count Capacity of @p panels.
 * @return Number of entries written.
 */
uint32_t bk_lcd_get_qspi_panel_list(const lcd_device_t **panels, uint32_t max_count);

/**
 * @brief Find a registered MIPI-DSI panel by name.
 * @param[in] name Panel name (must match the descriptor's @c .name).
 * @return Panel descriptor on success, NULL when no match.
 */
const bk_display_dsi_panel_t *bk_lcd_find_mipi_panel_by_name(const char *name);

/**
 * @brief Find a registered RGB panel by name.
 * @param[in] name Panel name.
 * @return Panel descriptor on success, NULL when no match.
 */
const bk_display_rgb_panel_t *bk_lcd_find_rgb_panel_by_name(const char *name);

/**
 * @brief Find a registered SPI panel by name.
 * @param[in] name Panel name.
 * @return Panel descriptor on success, NULL when no match.
 */
const lcd_device_t *bk_lcd_find_spi_panel_by_name(const char *name);

/**
 * @brief Find a registered QSPI panel by name.
 * @param[in] name Panel name.
 * @return Panel descriptor on success, NULL when no match.
 */
const lcd_device_t *bk_lcd_find_qspi_panel_by_name(const char *name);
/** @} */

#ifdef __cplusplus
}
#endif
