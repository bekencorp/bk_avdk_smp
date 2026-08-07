// Copyright 2025-2026 Beken
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
 * @file mipi_dsi.h
 * @brief Driver-level MIPI-DSI primitives.
 *
 * The driver layer only knows about hardware operations: PHY/host
 * bring-up, command channel writes/reads, video timing programming.
 * Higher-level abstractions (bus / panel-IO ops tables) live in
 * components/bk_display.
 */

#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <driver/dpu_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Built-in DSI host test pattern.
 */
typedef enum {
    MIPI_DSI_PATTERN_NONE,           /*!< No pattern */
    MIPI_DSI_PATTERN_BAR_VERTICAL,   /*!< Vertical BAR pattern, with different colors */
    MIPI_DSI_PATTERN_BAR_HORIZONTAL, /*!< Horizontal BAR pattern, with different colors */
    MIPI_DSI_PATTERN_BER_VERTICAL,   /*!< Vertical Bit Error Rate(BER) pattern */
} mipi_dsi_pattern_type_t;

/**
 * @brief Bring up the MIPI DSI host (interrupts + sys clock).
 *
 * Must be called after the DPU clock is enabled.
 *
 * @return BK_OK on success.
 */
bk_err_t mipi_dsi_init(void);

/**
 * @brief Tear down the MIPI DSI host (PHY power-down + IRQ unregister).
 *
 * @return BK_OK on success.
 */
bk_err_t mipi_dsi_deinit(void);

/**
 * @brief Program PHY + video timing using the supplied clock config.
 *
 * @param[in] dsi Clock + timing parameters.
 * @return BK_OK on success.
 */
bk_err_t mipi_dsi_clock_set(bk_panel_clock_config_t *dsi);

/**
 * @brief Select the DSI host operating mode (video vs. command).
 *
 * Command mode is required to exchange DCS packets, video mode to stream
 * pixels. Park the host in command mode once the DPU scan is stopped so the
 * video machine does not pull an idle DPI FIFO and storm the FIFO error IRQs.
 *
 * @param[in] video @c true to enter video mode, @c false for command mode.
 */
void mipi_dsi_video_mode_set(bool video);

/**
 * @brief Query whether the DSI host is in video mode.
 *
 * @return @c true if video mode (VIDMODE), @c false if command mode (CMDMODE).
 */
bool mipi_dsi_video_mode_get(void);

/**
 * @brief Send a generic-write packet on the DSI command channel.
 *
 * @param[in] data_len Number of bytes in @p data.
 * @param[in] data     Bytes to send (may be NULL when @p data_len is 0).
 * @return DWC host status word.
 */
uint16_t mipi_dsi_gen_write(uint16_t data_len, const uint8_t *data);

/**
 * @brief Send a DCS-write packet (command + optional payload).
 *
 * @param[in] lcd_cmd     DCS command byte.
 * @param[in] param       Optional payload buffer.
 * @param[in] param_size  Payload length in bytes (0 = command only).
 * @return DWC host status word.
 */
uint16_t mipi_dsi_gen_write_dcs_command(int lcd_cmd, const void *param, uint8_t param_size);

/**
 * @brief Issue a DCS read and wait for the response.
 *
 * @param[in]  cmd            DCS command byte.
 * @param[in]  bytes_to_read  Expected number of response bytes.
 * @param[out] read_buffer    Buffer that receives the response bytes.
 * @return DWC host status word.
 */
uint16_t mipi_dsi_dcs_read(uint8_t cmd, uint8_t bytes_to_read, uint8_t *read_buffer);

/**
 * @brief Set a built-in DSI host test pattern (VID_MODE_CFG vpg_en/mode/orientation).
 *
 * Toggles the internal pattern generator only; does not change command/video mode.
 * Caller must already be in video mode (e.g. after panel init) for patterns to show.
 *
 * @param[in] pattern Pattern selector.
 * @return BK_OK on success.
 */
bk_err_t mipi_dsi_panel_set_pattern(mipi_dsi_pattern_type_t pattern);

#ifdef __cplusplus
}
#endif
