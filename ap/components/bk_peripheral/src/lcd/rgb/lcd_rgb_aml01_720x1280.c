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
 * @file lcd_rgb_aml01_720x1280.c
 * @brief AML01 RGB Panel Driver (720x1280)
 * 
 * This file uses the common RGB panel driver to simplify code.
 * Panel configuration is defined in this file.
 */



#include <components/bk_lcd_panel.h>
#include <common/avdk_pixel_types.h>


#if CONFIG_LCD_AML01

// AML01 720x1280 RGB Panel Configuration
static const lcd_rgb_spi_init_cmd_t aml01_rgb_720x1280_init_cmds[] = {
    {0xFF, (const uint8_t []){0x30}, 1},
    {0xFF, (const uint8_t []){0x52}, 1},
    {0xFF, (const uint8_t []){0x01}, 1},
    {0xE3, (const uint8_t []){0x00}, 1},
    {0x0A, (const uint8_t []){0x01}, 1},
    {0x23, (const uint8_t []){0xA2}, 1},
    {0x24, (const uint8_t []){0x10}, 1},
    {0x25, (const uint8_t []){0x0A}, 1},
    {0x26, (const uint8_t []){0x3C}, 1},
    {0x27, (const uint8_t []){0x46}, 1},
    {0x38, (const uint8_t []){0x9C}, 1},
    {0x39, (const uint8_t []){0xA7}, 1},
    {0x3A, (const uint8_t []){0x47}, 1},
    {0x91, (const uint8_t []){0x77}, 1},
    {0x92, (const uint8_t []){0x77}, 1},
    {0x99, (const uint8_t []){0x51}, 1},
    {0x9B, (const uint8_t []){0x59}, 1},
    {0xA0, (const uint8_t []){0x55}, 1},
    {0xA1, (const uint8_t []){0x50}, 1},
    {0xA4, (const uint8_t []){0x9C}, 1},
    {0xA7, (const uint8_t []){0x02}, 1},
    {0xA8, (const uint8_t []){0x01}, 1},
    {0xA9, (const uint8_t []){0x01}, 1},
    {0xAA, (const uint8_t []){0xFC}, 1},
    {0xAB, (const uint8_t []){0x28}, 1},
    {0xAC, (const uint8_t []){0x06}, 1},
    {0xAD, (const uint8_t []){0x06}, 1},
    {0xAE, (const uint8_t []){0x06}, 1},
    {0xAF, (const uint8_t []){0x03}, 1},
    {0xB0, (const uint8_t []){0x08}, 1},
    {0xB1, (const uint8_t []){0x26}, 1},
    {0xB2, (const uint8_t []){0x28}, 1},
    {0xB3, (const uint8_t []){0x28}, 1},
    {0xB4, (const uint8_t []){0x03}, 1},
    {0xB5, (const uint8_t []){0x08}, 1},
    {0xB6, (const uint8_t []){0x26}, 1},
    {0xB7, (const uint8_t []){0x08}, 1},
    {0xB8, (const uint8_t []){0x26}, 1},
    {0xFF, (const uint8_t []){0x30}, 1},
    {0xFF, (const uint8_t []){0x52}, 1},
    {0xFF, (const uint8_t []){0x02}, 1},
    {0xB0, (const uint8_t []){0x01}, 1},
    {0xB1, (const uint8_t []){0x12}, 1},
    {0xB2, (const uint8_t []){0x09}, 1},
    {0xB3, (const uint8_t []){0x2B}, 1},
    {0xB4, (const uint8_t []){0x2F}, 1},
    {0xB5, (const uint8_t []){0x30}, 1},
    {0xB6, (const uint8_t []){0x19}, 1},
    {0xB7, (const uint8_t []){0x35}, 1},
    {0xB8, (const uint8_t []){0x0D}, 1},
    {0xB9, (const uint8_t []){0x03}, 1},
    {0xBA, (const uint8_t []){0x12}, 1},
    {0xBB, (const uint8_t []){0x12}, 1},
    {0xBC, (const uint8_t []){0x14}, 1},
    {0xBD, (const uint8_t []){0x15}, 1},
    {0xBE, (const uint8_t []){0x18}, 1},
    {0xBF, (const uint8_t []){0x0F}, 1},
    {0xC0, (const uint8_t []){0x17}, 1},
    {0xC1, (const uint8_t []){0x08}, 1},
    {0xD0, (const uint8_t []){0x0F}, 1},
    {0xD1, (const uint8_t []){0x12}, 1},
    {0xD2, (const uint8_t []){0x1A}, 1},
    {0xD3, (const uint8_t []){0x38}, 1},
    {0xD4, (const uint8_t []){0x36}, 1},
    {0xD5, (const uint8_t []){0x3a}, 1},
    {0xD6, (const uint8_t []){0x22}, 1},
    {0xD7, (const uint8_t []){0x40}, 1},
    {0xD8, (const uint8_t []){0x0D}, 1},
    {0xD9, (const uint8_t []){0x03}, 1},
    {0xDA, (const uint8_t []){0x11}, 1},
    {0xDB, (const uint8_t []){0x10}, 1},
    {0xDC, (const uint8_t []){0x12}, 1},
    {0xDD, (const uint8_t []){0x13}, 1},
    {0xDE, (const uint8_t []){0x18}, 1},
    {0xDF, (const uint8_t []){0x10}, 1},
    {0xE0, (const uint8_t []){0x17}, 1},
    {0xE1, (const uint8_t []){0x08}, 1},
    {0xFF, (const uint8_t []){0x30}, 1},
    {0xFF, (const uint8_t []){0x52}, 1},
    {0xFF, (const uint8_t []){0x03}, 1},
    {0x00, (const uint8_t []){0x2A}, 1},
    {0x01, (const uint8_t []){0x2A}, 1},
    {0x02, (const uint8_t []){0x2A}, 1},
    {0x03, (const uint8_t []){0x2A}, 1},
    {0x08, (const uint8_t []){0x02}, 1},
    {0x09, (const uint8_t []){0x03}, 1},
    {0x0A, (const uint8_t []){0x04}, 1},
    {0x0B, (const uint8_t []){0x05}, 1},
    {0x30, (const uint8_t []){0x2A}, 1},
    {0x31, (const uint8_t []){0x2A}, 1},
    {0x32, (const uint8_t []){0x2A}, 1},
    {0x33, (const uint8_t []){0x2A}, 1},
    {0x34, (const uint8_t []){0x81}, 1},
    {0x35, (const uint8_t []){0x26}, 1},
    {0x37, (const uint8_t []){0x13}, 1},
    {0x40, (const uint8_t []){0x03}, 1},
    {0x41, (const uint8_t []){0x04}, 1},
    {0x42, (const uint8_t []){0x05}, 1},
    {0x43, (const uint8_t []){0x06}, 1},
    {0x45, (const uint8_t []){0x08}, 1},
    {0x46, (const uint8_t []){0x09}, 1},
    {0x48, (const uint8_t []){0x0a}, 1},
    {0x49, (const uint8_t []){0x0b}, 1},
    {0x50, (const uint8_t []){0x07}, 1},
    {0x51, (const uint8_t []){0x08}, 1},
    {0x52, (const uint8_t []){0x09}, 1},
    {0x53, (const uint8_t []){0x0a}, 1},
    {0x55, (const uint8_t []){0x0c}, 1},
    {0x56, (const uint8_t []){0x0d}, 1},
    {0x58, (const uint8_t []){0x0e}, 1},
    {0x59, (const uint8_t []){0x0f}, 1},
    {0x80, (const uint8_t []){0x00}, 1},
    {0x81, (const uint8_t []){0x00}, 1},
    {0x82, (const uint8_t []){0x04}, 1},
    {0x83, (const uint8_t []){0x02}, 1},
    {0x84, (const uint8_t []){0x0E}, 1},
    {0x85, (const uint8_t []){0x10}, 1},
    {0x86, (const uint8_t []){0x0A}, 1},
    {0x87, (const uint8_t []){0x0C}, 1},
    {0x91, (const uint8_t []){0x00}, 1},
    {0x92, (const uint8_t []){0x00}, 1},
    {0x93, (const uint8_t []){0x00}, 1},
    {0x94, (const uint8_t []){0x1f}, 1},
    {0x95, (const uint8_t []){0x1F}, 1},
    {0x96, (const uint8_t []){0x00}, 1},
    {0x97, (const uint8_t []){0x00}, 1},
    {0x98, (const uint8_t []){0x03}, 1},
    {0x99, (const uint8_t []){0x01}, 1},
    {0x9A, (const uint8_t []){0x0D}, 1},
    {0x9B, (const uint8_t []){0x0F}, 1},
    {0x9C, (const uint8_t []){0x09}, 1},
    {0x9D, (const uint8_t []){0x0B}, 1},
    {0xA7, (const uint8_t []){0x00}, 1},
    {0xA8, (const uint8_t []){0x00}, 1},
    {0xA9, (const uint8_t []){0x00}, 1},
    {0xAA, (const uint8_t []){0x1F}, 1},
    {0xAB, (const uint8_t []){0x1F}, 1},
    {0xB0, (const uint8_t []){0x00}, 1},
    {0xB1, (const uint8_t []){0x1F}, 1},
    {0xB2, (const uint8_t []){0x01}, 1},
    {0xB3, (const uint8_t []){0x03}, 1},
    {0xB4, (const uint8_t []){0x0B}, 1},
    {0xB5, (const uint8_t []){0x09}, 1},
    {0xB6, (const uint8_t []){0x0F}, 1},
    {0xB7, (const uint8_t []){0x0D}, 1},
    {0xC1, (const uint8_t []){0x00}, 1},
    {0xC2, (const uint8_t []){0x00}, 1},
    {0xC3, (const uint8_t []){0x00}, 1},
    {0xC4, (const uint8_t []){0x1F}, 1},
    {0xC5, (const uint8_t []){0x00}, 1},
    {0xC6, (const uint8_t []){0x00}, 1},
    {0xC7, (const uint8_t []){0x1F}, 1},
    {0xC8, (const uint8_t []){0x02}, 1},
    {0xC9, (const uint8_t []){0x04}, 1},
    {0xCA, (const uint8_t []){0x0C}, 1},
    {0xCB, (const uint8_t []){0x0A}, 1},
    {0xCC, (const uint8_t []){0x10}, 1},
    {0xCD, (const uint8_t []){0x0E}, 1},
    {0xD7, (const uint8_t []){0x00}, 1},
    {0xD8, (const uint8_t []){0x00}, 1},
    {0xD9, (const uint8_t []){0x00}, 1},
    {0xDA, (const uint8_t []){0x1F}, 1},
    {0xDB, (const uint8_t []){0x00}, 1},
    {0xFF, (const uint8_t []){0x30}, 1},
    {0xFF, (const uint8_t []){0x52}, 1},
    {0xFF, (const uint8_t []){0x00}, 1},
    {0x36, (const uint8_t []){0x0A}, 1},
    {0x11, NULL, 0},  // sleep out
    {0xFF, (const uint8_t []){200}, 0xFF},  // delay 200ms
    {0x29, NULL, 0},  // disp on
    {0xFF, (const uint8_t []){100}, 0xFF},  // delay 100ms
    {0x00, NULL, 0}  // End marker
};

static const uint8_t aml01_rgb_720x1280_read_id_regs[] = {0x04, 0};  // RDDID command

// Panel descriptor - single source of truth for AML01 RGB panel
const bk_display_rgb_panel_t aml01_rgb_panel = {
    .id = 0x0001,
    .name = "aml01_rgb_720x1280",
    .timing = {
        .clk = LCD_60M,
        .h_size = 720,
        .v_size = 1280,
        .hsync_pulse_width = 2,
        .vsync_pulse_width = 2,
        .hsync_back_porch = 42,
        .hsync_front_porch = 44,
        .vsync_back_porch = 14,
        .vsync_front_porch = 16,
    },
    .init_cmds = aml01_rgb_720x1280_init_cmds,
    .spi_cmd_16bit = 0,
    .read_id_regs = aml01_rgb_720x1280_read_id_regs,
    .read_id_bytes = 3,
    .custom_reset = NULL,
};

BK_LCD_PANEL_DEVICE_SECTION(aml01_rgb_panel, "aml01_rgb_720x1280", BK_LCD_PANEL_BUS_RGB);
#endif
