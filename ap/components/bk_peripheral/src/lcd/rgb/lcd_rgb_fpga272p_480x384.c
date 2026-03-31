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
 * @file lcd_rgb_fpga272p_480x384.c
* @brief FPGA272P RGB Panel Driver (480x384)
 * 
 * This file uses the common RGB panel driver to simplify code.
 * Panel configuration is defined in this file.
 */

#include <components/bk_display_types.h>

#include <components/bk_lcd_types.h>
#include <common/avdk_pixel_types.h>


#if CONFIG_LCD_FPGA272P

// FPGA272P 480x384 RGB Panel Configuration
// Note: FPGA272P may not require initialization commands, using empty sequence
static const lcd_rgb_spi_init_cmd_t fpga272p_rgb_480x384_init_cmds[] = {
    {0x00, NULL, 0}  // End marker (no init commands needed)
};

static const uint8_t fpga272p_rgb_480x384_read_id_regs[] = {0x04, 0};  // RDDID command

// Panel descriptor - single source of truth for FPGA272P RGB panel
const bk_display_rgb_panel_t fpga272p_rgb_panel = {
    .id = 0x2720,
    .name = "fpga272p_rgb_480x384",
    .timing = {
        .clk = LCD_8M,
        .h_size = 480,
        .v_size = 384,
        .hsync_pulse_width = 2,
        .vsync_pulse_width = 2,
        .hsync_back_porch = 40,
        .hsync_front_porch = 5,
        .vsync_back_porch = 8,
        .vsync_front_porch = 8,
    },
    .init_cmds = fpga272p_rgb_480x384_init_cmds,
    .spi_cmd_16bit = 0,
    .read_id_regs = fpga272p_rgb_480x384_read_id_regs,
    .read_id_bytes = 3,
    .custom_reset = NULL,
};

BK_LCD_PANEL_DEVICE_SECTION(fpga272p_rgb_panel, "fpga272p_rgb_480x384", 0);
#endif
