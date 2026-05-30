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
 * @file lcd_rgb_h050iwv_800x480.c
 * @brief H050IWV RGB Panel Driver (800x480)
 * 
 * This file uses the common RGB panel driver to simplify code.
 * Panel configuration is defined in this file.
 */



#include <components/bk_lcd_panel.h>
#include <common/avdk_pixel_types.h>


#if CONFIG_LCD_H050IWV


// Panel descriptor - single source of truth for H050IWV RGB panel
const bk_display_rgb_panel_t h050iwv_rgb_panel = {
    .id = 0x0500,
    .name = "h050iwv_rgb_800x480",
    .pixel_clock_hz = BK_RGB_PIXEL_CLK_HZ(30),
    .timing = {
        .h_size = 800,
        .v_size = 480,
        .hsync_pulse_width = 2,
        .vsync_pulse_width = 2,
        .hsync_back_porch = 46,
        .hsync_front_porch = 48,
        .vsync_back_porch = 24,
        .vsync_front_porch = 24,
    },
    .init_cmds = NULL,
    .spi_cmd_16bit = 0,
    .read_id_regs = NULL,
    .read_id_bytes = 3,
    .reset_active_level = false,
    .reset = bk_lcd_rgb_default_reset,
    .init  = bk_lcd_rgb_default_init,
};

BK_LCD_PANEL_DEVICE_SECTION(h050iwv_rgb_panel, "h050iwv_rgb_800x480", BK_LCD_PANEL_BUS_RGB);
#endif
