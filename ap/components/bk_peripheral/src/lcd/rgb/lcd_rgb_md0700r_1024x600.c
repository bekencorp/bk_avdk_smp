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
 * @file lcd_rgb_md0700r_1024x600.c
 * @brief MD0700R RGB Panel Driver (1024x600)
 * 
 * This file uses the common RGB panel driver to simplify code.
 * Panel configuration is defined in this file.
 */

#include <components/bk_display_types.h>

#include <components/bk_lcd_types.h>
#include <common/avdk_pixel_types.h>


#if CONFIG_LCD_MD0700R

// MD0700R 1024x600 RGB Panel Configuration
// Note: MD0700R may not require initialization commands, using empty sequence
static const lcd_rgb_spi_init_cmd_t md0700r_rgb_1024x600_init_cmds[] = {
    {0x00, NULL, 0}  // End marker (no init commands needed)
};

static const uint8_t md0700r_rgb_1024x600_read_id_regs[] = {0x04, 0};  // RDDID command

// Panel descriptor - single source of truth for MD0700R RGB panel
const bk_display_rgb_panel_t md0700r_rgb_panel = {
    .id = 0x0700,
    .name = "md0700r_rgb_1024x600",
    .timing = {
        .clk = LCD_26M,
        .h_size = 1024,
        .v_size = 600,
        .hsync_pulse_width = 2,
        .vsync_pulse_width = 2,
        .hsync_back_porch = 140,
        .hsync_front_porch = 127,
        .vsync_back_porch = 20,
        .vsync_front_porch = 12,
    },
    .init_cmds = md0700r_rgb_1024x600_init_cmds,
    .spi_cmd_16bit = 0,
    .read_id_regs = md0700r_rgb_1024x600_read_id_regs,
    .read_id_bytes = 3,
    .custom_reset = NULL,
};

BK_LCD_PANEL_DEVICE_SECTION(md0700r_rgb_panel, "md0700r_rgb_1024x600", 0);
#endif
