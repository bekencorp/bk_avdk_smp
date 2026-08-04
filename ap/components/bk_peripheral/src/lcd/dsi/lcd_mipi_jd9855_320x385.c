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
 * @file lcd_mipi_jd9855_320x385.c
 * @brief JD9855 MIPI DSI panel (320x385, 1-lane).
 *
 * 1-lane / 24bpp exceeds the PHY pixdiv (lane:pclk ratio > 17), so the
 * default DPHY_DPLL path returns BK_FAIL on bring-up and the driver
 * automatically falls back to DPU_CLK_SRC_SYSCLK; see
 * how_to_add_mipi_panel.md §15.1.
 */

#include <components/bk_lcd_panel.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>

#if CONFIG_LCD_JD9855_MIPI_320x385

static const lcd_mipi_init_cmd_t jd9855_mipi_320x385_init_cmds[] = {
    {0xDE, (const uint8_t []){0x00}, 1},
    {0xDF, (const uint8_t []){0x98, 0x55}, 2},
    {0xB2, (const uint8_t []){0x30}, 1},
    {0xB7, (const uint8_t []){0x01, 0x35, 0x01, 0x5D}, 4},
    {0xBB, (const uint8_t []){0x13, 0x53, 0xD4, 0x25, 0x3E, 0xF3}, 6},
    {0xBC, (const uint8_t []){0x00, 0x18, 0xF3, 0xC0}, 4},
    {0xC0, (const uint8_t []){0x22, 0x22}, 2},
    {0xC3, (const uint8_t []){0x00, 0x02, 0x23, 0x0B, 0x08, 0x47, 0x0C, 0x04, 0x62, 0x30, 0x30}, 11},
    {0xC4, (const uint8_t []){0x40, 0x00, 0xA8, 0x81, 0x3F, 0x06, 0x03, 0x16, 0x3F, 0x07, 0x04}, 11},
    {0xC8, (const uint8_t []){0x3F, 0x36, 0x30, 0x2D, 0x2F, 0x31, 0x2D, 0x2C, 0x2A, 0x26, 0x22, 0x15, 0x10, 0x0A, 0x06, 0x02,
                        0x3F, 0x36, 0x30, 0x2D, 0x2F, 0x31, 0x2D, 0x2C, 0x2A, 0x26, 0x22, 0x15, 0x10, 0x0A, 0x06, 0x02}, 32},
    {0xD3, (const uint8_t []){0x28, 0x13}, 2},
    {0xDE, (const uint8_t []){0x01}, 1},
    {0xB7, (const uint8_t []){0x13, 0xE7, 0x64, 0x39, 0x06, 0x36, 0x18, 0x18}, 8},
    {0xBE, (const uint8_t []){0x00}, 1},
    {0xC1, (const uint8_t []){0x04, 0x4A, 0x90, 0x08}, 4},
    {0xC2, (const uint8_t []){0x00, 0x16, 0xDA, 0xE7}, 4},
    {0xC7, (const uint8_t []){0x00, 0x00, 0x08, 0x31, 0x08, 0x31}, 6},
    {0xC8, (const uint8_t []){0x00, 0x00, 0x0B, 0x31, 0x08, 0x31}, 6},
    {0xC9, (const uint8_t []){0x1E, 0x01, 0x11, 0x0B, 0x09}, 5},
    {0xCA, (const uint8_t []){0x07, 0x05, 0x1F, 0x1F, 0x1F}, 5},
    {0xCB, (const uint8_t []){0x1E, 0x00, 0x10, 0x0A, 0x08}, 5},
    {0xCC, (const uint8_t []){0x06, 0x04, 0x1F, 0x1F, 0x1F}, 5},
    {0xCD, (const uint8_t []){0x3F, 0x30, 0x20, 0x26, 0x28}, 5},
    {0xCE, (const uint8_t []){0x2A, 0x24, 0x3F, 0x3E, 0x3F}, 5},
    {0xCF, (const uint8_t []){0x3F, 0x31, 0x21, 0x27, 0x29}, 5},
    {0xD0, (const uint8_t []){0x2B, 0x25, 0x3F, 0x3E, 0x3F}, 5},
    {0xD1, (const uint8_t []){0x00, 0x10, 0xB5, 0x10, 0x10}, 5},
    {0xD3, (const uint8_t []){0x39, 0x04, 0x0E, 0x0E, 0x00, 0x00}, 6},
    {0xD4, (const uint8_t []){0x62, 0x00, 0x00, 0x00, 0x00, 0x01}, 6},
    {0xD5, (const uint8_t []){0x10, 0x10, 0x07, 0x07, 0x0F, 0x94, 0x26}, 7},
    {0xD6, (const uint8_t []){0x00, 0x00, 0x40}, 3},
    {0xD7, (const uint8_t []){0x03, 0x8F, 0x20}, 3},
    {0xDE, (const uint8_t []){0x02}, 1},
    {0xB6, (const uint8_t []){0x1C}, 1},
    {0xDE, (const uint8_t []){0x00}, 1},
    {0x4D, (const uint8_t []){0x00}, 1},
    {0x4E, (const uint8_t []){0x00}, 1},
    {0x4F, (const uint8_t []){0x00}, 1},
    {0x4C, (const uint8_t []){0x01}, 1},
    {0x00, (const uint8_t []){10}, 0xFF},
    {0x4C, (const uint8_t []){0x00}, 1},
    /* CASET: 0..319 (0x013F), RASET: 0..384 (0x0180) for 320x385 */
    {0x2A, (const uint8_t []){0x00, 0x00, 0x01, 0x3F}, 4},
    {0x2B, (const uint8_t []){0x00, 0x00, 0x01, 0x80}, 4},
    {0x35, NULL, 0},
    {0x36, (const uint8_t []){0x00}, 1},
    {0x3A, (const uint8_t []){0x55}, 1},
    {0xDE, (const uint8_t []){0x00}, 1},
    {0x11, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){120}, 0xFF},
    {0x29, (const uint8_t []){0x00}, 0},
    {0x00, NULL, 0},
};

// Power-down sequence: DISPOFF -> (>=1 frame) -> SLPIN -> (charge-pump
// discharge). Sent by bk_lcd_mipi_default_off() during bk_display_deinit()
// while the DSI command channel is still up, before RESETn / VDDIO drop.
// Panel is left on bank DE=0x00 by init, so 28h/10h apply directly.
static const lcd_mipi_init_cmd_t jd9855_mipi_320x385_off_cmds[] = {
    {0x28, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){20},   0xFF},
    {0x10, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){120},  0xFF},
    {0x00, NULL, 0},
};

static const uint8_t jd9855_mipi_320x385_read_id_regs[] = {0x04, 0};

const bk_display_dsi_panel_t lcd_device_jd9855_mipi_320x385 = {
    .id = 0x985500,
    .name = "jd9855_mipi_320x385",
    .n_lanes = DSI_ACTIVE_LANES_1,
    .fps = 58,
    .timing = {
        .h_size = PIXEL_320,
        .v_size = 385,
        .hsync_pulse_width = 40,
        .vsync_pulse_width = 8,
        .hsync_back_porch = 40,
        .hsync_front_porch = 40,
        .vsync_back_porch = 40,
        .vsync_front_porch = 40,
    },
    .init_cmds = jd9855_mipi_320x385_init_cmds,
    .off_cmds = jd9855_mipi_320x385_off_cmds,
    .read_id_regs = jd9855_mipi_320x385_read_id_regs,
    .read_id_bytes = 3,
    .reset_active_level = false,
    .reset = bk_lcd_mipi_default_reset,
    .init  = bk_lcd_mipi_default_init,
    .off   = bk_lcd_mipi_default_off,
};

BK_LCD_PANEL_DEVICE_SECTION(lcd_device_jd9855_mipi_320x385, "jd9855_mipi_320x385", BK_LCD_PANEL_BUS_DSI);
#endif
