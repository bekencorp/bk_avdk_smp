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

/**
 * @file lcd_rgb_st7282_480x272.c
 * @brief ST7282 RGB Panel Driver (480x272)
 * 
 * This file uses the common RGB panel driver to simplify code.
 * Panel configuration is defined in this file.
 */

#include <components/bk_display_types.h>

#include <components/bk_lcd_types.h>
#include <common/avdk_pixel_types.h>


#if CONFIG_LCD_ST7282

// Panel descriptor - single source of truth for ST7282 RGB panel
const bk_display_rgb_panel_t st7282_rgb_panel = {
    .id = 0x7282,
    .name = "st7282_rgb_480x272",
    .timing = {
        .clk = LCD_8M,
        .h_size = 480,
        .v_size = 272,
        .hsync_pulse_width = 2,
        .vsync_pulse_width = 2,
        .hsync_back_porch = 40,
        .hsync_front_porch = 5,
        .vsync_back_porch = 8,
        .vsync_front_porch = 8,
    },
    .init_cmds = NULL,
    .spi_cmd_16bit = 0,
    .read_id_regs = NULL,
    .read_id_bytes = 3,
    .custom_reset = NULL,
};

BK_LCD_PANEL_DEVICE_SECTION(st7282_rgb_panel, "st7282_rgb_480x272", 0);
#endif
