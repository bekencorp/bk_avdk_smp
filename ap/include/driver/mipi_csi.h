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

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_err.h>
#include <driver/mipi_csi_types.h>

/**
 * @brief Initialize the MIPI CSI controller.
 *
 * @param width      Frame width in pixels.
 * @param height     Frame height in pixels.
 * @param data_type  MIPI data type, e.g. 0x2b for RAW10.
 * @param lanes      Number of active data lanes (1 or 2). 0 keeps the lane count
 *                   configured by the previous call, or the default 2 lanes if
 *                   never configured. Values greater than 2 fall back to 2 lanes.
 *
 * @note The lane count can only be updated while the PHY lane is in stopstate,
 *       so this must be called during controller init before the PHY is enabled.
 */
bk_err_t bk_mipi_csi_controller_init(uint16_t width, uint16_t height, uint8_t data_type, uint8_t lanes);

bk_err_t bk_mipi_csi_controller_init_vc(uint16_t width, uint16_t height, uint8_t data_type, uint8_t vc, uint8_t lanes);

void bk_mipi_csi_set_default_vc(uint8_t vc);

void bk_mipi_csi_set_runtime_vc(uint8_t vc);

void bk_mipi_csi_controller_deinit(void);

void bk_mipi_csi_controller_reset(void);

void bk_mipi_csi_phy_term_set(uint32_t v1, uint32_t v2);

void bk_mipi_csi_ext_set_enable(uint8_t mode);

void bk_dvp_io_config(void);

void bk_mipi_csi_enable_debug_pin(void);

void bk_mipi_csi_disable_debug_pin(void);

#ifdef __cplusplus
}
#endif
