// Copyright 2020-2026 Beken
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
 * @file lcd_mipi_co5300_320x360.c
 * @brief CO5300 1.6" AMOLED MIPI DSI panel (320x360, 1-lane).
 *
 * Init sequence and timing from 1.6-OLED 初始化.txt:
 *   1 lane, DPI 10 MHz, CASET 0..319, RASET 0..359, COLMOD 0x77 (RGB888).
 */

#include <components/bk_lcd_panel.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>

#if CONFIG_LCD_CO5300_MIPI_320x360

static const lcd_mipi_init_cmd_t co5300_mipi_320x360_init_cmds[] = {
    {0xFE, (const uint8_t []){0x00}, 1},
    {0x3A, (const uint8_t []){0x77}, 1},  /* 0x55=RGB565, 0x77=RGB888 */
    {0x35, (const uint8_t []){0x00}, 1},
    {0x53, (const uint8_t []){0x20}, 1},
    {0x51, (const uint8_t []){0xFF}, 1},
    {0x63, (const uint8_t []){0xFF}, 1},
    /* CASET: 0..319 (0x013F), RASET: 0..359 (0x0167) */
    {0x2A, (const uint8_t []){0x00, 0x00, 0x01, 0x3F}, 4},
    {0x2B, (const uint8_t []){0x00, 0x00, 0x01, 0x67}, 4},
    {0x11, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){60}, 0xFF},
    {0x29, (const uint8_t []){0x00}, 0},
    {0x00, NULL, 0},
};

static const lcd_mipi_init_cmd_t co5300_mipi_320x360_off_cmds[] = {
    {0x28, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){20},   0xFF},
    {0x10, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){120},  0xFF},
    {0x00, NULL, 0},
};

static const uint8_t co5300_mipi_320x360_read_id_regs[] = {0x04, 0};

const bk_display_dsi_panel_t lcd_device_co5300_mipi_320x360 = {
    .id = 0x530000,
    .name = "co5300_mipi_320x360",
    .n_lanes = DSI_ACTIVE_LANES_1, /* 1-lane Maximum total bit rate is 500Mbps with 24-bit data format */
    .fps = 58, 
    .timing = {
        .h_size = PIXEL_320,
        .v_size = 360,
        .hsync_pulse_width = 4,
        .vsync_pulse_width = 4,
        .hsync_back_porch = 40,  
        .hsync_front_porch = 40,
        .vsync_back_porch = 12,
        .vsync_front_porch = 18,
    },
    .init_cmds = co5300_mipi_320x360_init_cmds,
    .off_cmds = co5300_mipi_320x360_off_cmds,
    .read_id_regs = co5300_mipi_320x360_read_id_regs,
    .read_id_bytes = 3,
    .reset_active_level = false,
    .reset = bk_lcd_mipi_default_reset,
    .init  = bk_lcd_mipi_default_init,
    .off   = bk_lcd_mipi_default_off,
};

BK_LCD_PANEL_DEVICE_SECTION(lcd_device_co5300_mipi_320x360, "co5300_mipi_320x360", BK_LCD_PANEL_BUS_DSI);
#endif
