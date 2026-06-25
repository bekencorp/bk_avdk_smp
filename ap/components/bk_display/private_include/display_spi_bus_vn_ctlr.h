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
 *        One controller object backs both ::BK_DISPLAY_SPI_BUS_MODE_HW
 *        (avdk lcd_spi driver ownership) and ::BK_DISPLAY_SPI_BUS_MODE_SW
 *        (GPIO bit-bang command channel for RGB panel SPI register init).
 */

#include <os/os.h>
#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel.h>
#include "bk_display_bus_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/** HW-mode private context (lcd_spi device ownership). */
typedef struct
{
    uint8_t spi_id;
    uint8_t reset_pin;
    uint8_t dc_pin;
    const bk_lcd_panel_t *device;
} private_display_spi_context_t;

/**
 * SPI bus controller body.
 *
 * SW mode reads the @c cmd_width / @c csx_pin / @c sda_pin / @c clk_pin
 * fields straight out of @c config, so no separate per-instance IO
 * struct is needed. HW mode owns @c spi_context (lcd_spi init/deinit
 * state); frame scheduling lives in the SPI display controller.
 */
typedef struct spi_bus_vn_ctlr_t
{
    bk_display_spi_bus_config_t config;
    bk_display_bus_ctlr_t ops;             /**< must stay first member of the public view */
    private_display_spi_context_t spi_context;
} spi_bus_vn_ctlr_t;

#ifdef __cplusplus
}
#endif
