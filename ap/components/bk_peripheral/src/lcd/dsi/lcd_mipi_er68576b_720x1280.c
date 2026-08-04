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
 * @file lcd_mipi_er68576b_720x1280.c
 * @brief ER68576B MIPI DSI Panel Driver (720x1280)
 *
 * This file uses the common panel driver to simplify code.
 * Panel configuration is defined in this file.
 *
 * Panel parameters (from vendor init sequence):
 *   Resolution      : 720x1280
 *   Frame rate      : 60Hz
 *   Power           : IOVCC=1.8V, VCI=3.3V
 *   System porch    : VS=4, VBP=12, VFP=20, HS=20, HBP=80, HFP=80
 *   Mode            : SET_HS_BURST
 */



#include <components/bk_lcd_panel.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>

#if CONFIG_LCD_ER68576B_MIPI_720x1280

// ER68576B 720x1280 Panel Configuration
// GAMMA 2.2 is selected (GAMMA 2.0 / 2.5 variants omitted)
static const lcd_mipi_init_cmd_t er68576b_mipi_720x1280_init_cmds[] = {
    {0xE0, (const uint8_t []){0xAB,0xBA}, 2},
    {0xE1, (const uint8_t []){0xBA,0xAB}, 2},
    {0xB1, (const uint8_t []){0x10,0x01,0x47,0xFF}, 4},
    {0xB2, (const uint8_t []){0x0C,0x14,0x04,0x50,0x50,0x14}, 6},
    {0xB3, (const uint8_t []){0x56,0x52,0x00}, 3},
    {0xB4, (const uint8_t []){0x33,0x30,0x04}, 3},
    {0xB6, (const uint8_t []){0xB0,0x00,0x00,0x10,0x00,0x10,0x00}, 7},
    {0xB8, (const uint8_t []){0x05,0x12,0x29,0x49,0x48}, 5},
    // GAMMA 2.2
    {0xB9, (const uint8_t []){0x7C,0x61,0x50,0x44,0x41,0x32,0x37,0x20,0x38,0x37,0x38,0x57,0x46,0x4F,0x42,0x3D,0x30,0x1F,0x06,
                              0x7C,0x61,0x50,0x44,0x41,0x32,0x37,0x20,0x38,0x37,0x38,0x57,0x46,0x4F,0x42,0x3D,0x30,0x1F,0x06}, 38},
    {0xC0, (const uint8_t []){0xDC,0xBA,0x23,0x45,0x44,0x44,0x44,0x44,0x90,0x04,0x50,0x04,0x0F,0x00,0x00,0xC1}, 16},
    {0xC1, (const uint8_t []){0x94,0x94,0x02,0x89,0x90,0x04,0x50,0x04,0x54,0x00}, 10},
    {0xC2, (const uint8_t []){0x37,0x09,0x08,0x89,0x08,0x11,0x22,0x20,0x44,0xBB,0x18,0x00}, 12},
    {0xC3, (const uint8_t []){0xA4,0x5D,0x0C,0x0E,0x10,0x12,0x24,0x24,0x24,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x25,0x1C,0x04,0x06}, 22},
    {0xC4, (const uint8_t []){0x24,0x1D,0x0D,0x0F,0x11,0x13,0x24,0x24,0x24,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x25,0x1C,0x05,0x07}, 22},
    {0xC6, (const uint8_t []){0x43,0x43}, 2},
    {0xC8, (const uint8_t []){0x21,0x00,0x31,0x42,0x34,0x16}, 6},
    {0xCA, (const uint8_t []){0xCB,0x43}, 2},
    {0xCD, (const uint8_t []){0x0E,0x4B,0x4B,0x11,0x1E,0x6B,0x06,0xB3}, 8},
    {0xD2, (const uint8_t []){0xE3,0x2B,0x38,0x08}, 4},
    {0xD4, (const uint8_t []){0x00,0x01,0x00,0x0E,0x04,0x44,0x08,0x10,0x00,0x00,0x00}, 11},
    {0xE6, (const uint8_t []){0x80,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, 8},
    {0xF0, (const uint8_t []){0x12,0x03,0x20,0x00,0xFF}, 5},
    {0xF3, (const uint8_t []){0x00}, 1},
    {0x11, (const uint8_t []){0x00}, 0},        // sleep out
    {0x00, (const uint8_t []){120}, 0xFF},      // delay 120ms
    {0x29, (const uint8_t []){0x00}, 0},        // disp on
    {0x00, (const uint8_t []){20}, 0xFF},       // delay 20ms
    {0x00, NULL, 0}  // End marker
};

// Power-down sequence: DISPOFF -> (>=1 frame) -> SLPIN -> (charge-pump
// discharge). Sent by bk_lcd_mipi_default_off() during bk_display_deinit()
// while the DSI command channel is still up, before RESETn / VDDIO drop.
static const lcd_mipi_init_cmd_t er68576b_mipi_720x1280_off_cmds[] = {
    {0x28, (const uint8_t []){0x00}, 0},        // disp off
    {0x00, (const uint8_t []){50},   0xFF},     // delay 20ms
    {0x10, (const uint8_t []){0x00}, 0},        // sleep in
    {0x00, (const uint8_t []){120},  0xFF},     // delay 120ms
    {0x00, NULL, 0}  // End marker
};

static const uint8_t er68576b_mipi_720x1280_read_id_regs[] = {0x04, 0};  // RDDID command (0x04)

// Panel descriptor - referenced by board config and CLI
const bk_display_dsi_panel_t lcd_device_er68576b_mipi_720x1280 = {
    .id = 0x685760,
    .name = "er68576b_mipi_720x1280",
    .n_lanes = DSI_ACTIVE_LANES_4,
    .fps = 36,   // (720+20+80+80)*(1280+4+12+20) * 36 * 24 * 1.3 / 4 = 332 Mbps/lane, max DSI CLK rate 550Mbps /4-lane
    .timing = {
        .h_size = PIXEL_720,
        .v_size = PIXEL_1280,
        .hsync_pulse_width = 20,
        .vsync_pulse_width = 4,
        .hsync_back_porch = 80,
        .hsync_front_porch = 80,
        .vsync_back_porch = 12,
        .vsync_front_porch = 20,
    },
    .init_cmds = er68576b_mipi_720x1280_init_cmds,
    .off_cmds = er68576b_mipi_720x1280_off_cmds,
    .read_id_regs = er68576b_mipi_720x1280_read_id_regs,
    .read_id_bytes = 3,
    .reset_active_level = false,
    .reset = bk_lcd_mipi_default_reset,
    .init  = bk_lcd_mipi_default_init,
    .off   = bk_lcd_mipi_default_off,
};

BK_LCD_PANEL_DEVICE_SECTION(lcd_device_er68576b_mipi_720x1280, "er68576b_mipi_720x1280", BK_LCD_PANEL_BUS_DSI);
#endif
