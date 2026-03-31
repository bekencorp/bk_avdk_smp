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

bk_err_t bk_mipi_csi_controller_init(uint16_t width, uint16_t height, uint8_t data_type);

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
