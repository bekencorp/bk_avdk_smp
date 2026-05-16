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
 * @file lcd_rgb_md0430r_800x480.c
 * @brief MD0430R RGB Panel Driver (800x480)
 * 
 * This file uses the common RGB panel driver to simplify code.
 * Panel configuration is defined in this file.
 */



#include <components/bk_lcd_panel.h>
#include <common/avdk_pixel_types.h>


#if CONFIG_LCD_MD0430R

// MD0430R 800x480 RGB Panel Configuration
// Note: MD0430R may not require initialization commands, using empty sequence
static const lcd_rgb_spi_init_cmd_t md0430r_rgb_800x480_init_cmds[] = {
    {0x00, NULL, 0}  // End marker (no init commands needed)
};

static const uint8_t md0430r_rgb_800x480_read_id_regs[] = {0x04, 0};  // RDDID command

// Panel descriptor - single source of truth for MD0430R RGB panel
const bk_display_rgb_panel_t md0430r_rgb_panel = {
    .id = 0x0430,
    .name = "md0430r_rgb_800x480",
    .timing = {
        .clk = LCD_32M,
        .h_size = 800,
        .v_size = 480,
        .hsync_pulse_width = 2,
        .vsync_pulse_width = 2,
        .hsync_back_porch = 40,
        .hsync_front_porch = 48,
        .vsync_back_porch = 32,
        .vsync_front_porch = 13,
    },
    .init_cmds = md0430r_rgb_800x480_init_cmds,
    .spi_cmd_16bit = 0,
    .read_id_regs = md0430r_rgb_800x480_read_id_regs,
    .read_id_bytes = 3,
    .custom_reset = NULL,
};

BK_LCD_PANEL_DEVICE_SECTION(md0430r_rgb_panel, "md0430r_rgb_800x480", BK_LCD_PANEL_BUS_RGB);
#endif
