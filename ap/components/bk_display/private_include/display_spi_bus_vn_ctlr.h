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
 * @file display_spi_bus_vn_ctlr.h
 * @brief SPI bus virtual node controller. Internal to bk_display.
 *        Backs the GPIO bit-bang command channel for RGB panel SPI register
 *        init. Hardware SPI LCD frame output is owned by the SPI display
 *        controller, not by this bus.
 */

#include <os/os.h>
#include <components/bk_display_bus.h>
#include "bk_display_bus_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * SPI bus controller body.
 *
 * The SPI bus reads the @c cmd_width / @c csx_pin / @c sda_pin / @c clk_pin
 * fields straight out of @c config, so no separate per-instance IO
 * struct is needed.
 */
typedef struct spi_bus_vn_ctlr_t
{
    bk_display_spi_bus_config_t config;
    bk_display_bus_ctlr_t ops;             /**< must stay first member of the public view */
} spi_bus_vn_ctlr_t;

#ifdef __cplusplus
}
#endif
