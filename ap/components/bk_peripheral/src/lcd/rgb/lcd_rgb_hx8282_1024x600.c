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
 * @file lcd_rgb_hx8282_1024x600.c
 * @brief HX8282 RGB Panel Driver (1024x600)
 * 
 * This file uses the common RGB panel driver to simplify code.
 * Panel configuration is defined in this file.
 */



#include <components/bk_lcd_panel.h>
#include <common/avdk_pixel_types.h>


#if CONFIG_LCD_HX8282
// Panel descriptor - single source of truth for HX8282 RGB panel
const bk_display_rgb_panel_t hx8282_rgb_panel = {
    .id = 0x8282,
    .name = "hx8282_rgb_1024x600",
    .pixel_clock_hz = BK_RGB_PIXEL_CLK_HZ(32),
    .timing = {
        .h_size = 1024,
        .v_size = 600,
        .hsync_pulse_width = 2,
        .vsync_pulse_width = 2,
        .hsync_back_porch = 160,
        .hsync_front_porch = 160,
        .vsync_back_porch = 50,
        .vsync_front_porch = 50,
    },
    .init_cmds = NULL,
    .spi_cmd_16bit = 0,
    .read_id_regs = NULL,
    .read_id_bytes = 3,
    .custom_reset = NULL,
};

BK_LCD_PANEL_DEVICE_SECTION(hx8282_rgb_panel, "hx8282_rgb_1024x600", BK_LCD_PANEL_BUS_RGB);
#endif
