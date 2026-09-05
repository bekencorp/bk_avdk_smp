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
 * @file lcd_mipi_gh8555bl_800x1280.c
 * @brief GH8555BL MIPI DSI Panel Driver (800x1280)
 *
 * Init sequence from gh8555bl_800x1280.txt (GH8555BL_F080A11-601).
 */

#include <components/bk_lcd_panel.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>

#if CONFIG_LCD_GH8555BL_MIPI_800x1280

static const lcd_mipi_init_cmd_t gh8555bl_mipi_800x1280_init_cmds[] = {
    {0xEE, (const uint8_t []){0x50}, 1},
    {0xEA, (const uint8_t []){0x85, 0x55}, 2},
    {0x24, (const uint8_t []){0xA0}, 1},
    {0x30, (const uint8_t []){0x00}, 1},
    {0x35, (const uint8_t []){0x00}, 1},
    {0x50, (const uint8_t []){0x00}, 1},
    {0x56, (const uint8_t []){0x83}, 1},
    {0x79, (const uint8_t []){0x00}, 1},
    {0x7A, (const uint8_t []){0x20}, 1},
    {0x90, (const uint8_t []){0x20, 0x50}, 2},
    {0x93, (const uint8_t []){0xF8}, 1},
    {0x95, (const uint8_t []){0x74}, 1},
    {0x97, (const uint8_t []){0x0C}, 1},
    {0x99, (const uint8_t []){0x10}, 1},

    {0xEE, (const uint8_t []){0x60}, 1},
    {0x21, (const uint8_t []){0x01}, 1},
    {0x27, (const uint8_t []){0x62}, 1},
    {0x30, (const uint8_t []){0x01}, 1},
    {0x31, (const uint8_t []){0xAF}, 1},
    {0x32, (const uint8_t []){0xDA}, 1},
    {0x33, (const uint8_t []){0xC3}, 1},
    {0x34, (const uint8_t []){0x3F}, 1},
    {0x3B, (const uint8_t []){0x00}, 1},
    {0x3C, (const uint8_t []){0x23}, 1},
    {0x3D, (const uint8_t []){0x12, 0x93}, 2},
    {0x42, (const uint8_t []){0x55, 0x55}, 2},
    {0x86, (const uint8_t []){0x20}, 1},
    {0x89, (const uint8_t []){0x00}, 1},
    {0x8A, (const uint8_t []){0xAA}, 1},
    {0x91, (const uint8_t []){0x44}, 1},
    {0x92, (const uint8_t []){0x33}, 1},
    {0x93, (const uint8_t []){0x9B}, 1},
    {0x9A, (const uint8_t []){0x00}, 1},
    {0x9B, (const uint8_t []){0x02, 0x80}, 2},

    {0x47, (const uint8_t []){0x15, 0x2D, 0x36, 0x40, 0x46}, 5},
    {0x5A, (const uint8_t []){0x15, 0x2D, 0x36, 0x40, 0x46}, 5},
    {0x4C, (const uint8_t []){0x56, 0x53, 0x6A, 0x55, 0x56}, 5},
    {0x5F, (const uint8_t []){0x56, 0x53, 0x6A, 0x55, 0x56}, 5},
    {0x51, (const uint8_t []){0x5A, 0x3F, 0x53, 0x4C, 0x59}, 5},
    {0x64, (const uint8_t []){0x5A, 0x3F, 0x53, 0x4C, 0x59}, 5},
    {0x56, (const uint8_t []){0x59, 0x63, 0x6E, 0x7F}, 4},
    {0x69, (const uint8_t []){0x59, 0x63, 0x6E, 0x7F}, 4},

    {0xEE, (const uint8_t []){0x70}, 1},
    {0x00, (const uint8_t []){0x08, 0x0B, 0x00, 0x01}, 4},
    {0x04, (const uint8_t []){0x08, 0x0B, 0x55, 0x01}, 4},
    {0x08, (const uint8_t []){0x0B, 0x0E, 0x55, 0x01}, 4},
    {0x0C, (const uint8_t []){0x05, 0x05}, 2},

    {0x10, (const uint8_t []){0x0A, 0x0D, 0x00, 0x00, 0x00}, 5},
    {0x15, (const uint8_t []){0x00, 0x1F, 0x0D, 0x0C}, 4},
    {0x20, (const uint8_t []){0x11, 0x14, 0x00, 0x00, 0x00}, 5},
    {0x25, (const uint8_t []){0x00, 0x1F, 0x0D, 0x0C}, 4},
    {0x29, (const uint8_t []){0x05, 0x05}, 2},

    {0x30, (const uint8_t []){0x12, 0x13, 0x55, 0xDD, 0xDD, 0x3C}, 6},
    {0x36, (const uint8_t []){0x12, 0x13, 0x55, 0xDD, 0xDD, 0x3C}, 6},

    {0x46, (const uint8_t []){0xFF, 0x00, 0x00, 0x00, 0x40}, 5},
    {0x4B, (const uint8_t []){0x88}, 1},

    {0x60, (const uint8_t []){0x08, 0x3C, 0x3C, 0x04, 0x21}, 5},
    {0x65, (const uint8_t []){0x20, 0x3C, 0x1A, 0x18, 0x3C}, 5},
    {0x6A, (const uint8_t []){0x16, 0x14, 0x3C, 0x12, 0x10}, 5},
    {0x6F, (const uint8_t []){0x00, 0x3C, 0x3C, 0x3C, 0x3C}, 5},
    {0x74, (const uint8_t []){0x3C, 0x3C}, 2},

    {0x80, (const uint8_t []){0x09, 0x3C, 0x3C, 0x05, 0x21}, 5},
    {0x85, (const uint8_t []){0x20, 0x3C, 0x1B, 0x19, 0x3C}, 5},
    {0x8A, (const uint8_t []){0x17, 0x15, 0x3C, 0x13, 0x11}, 5},
    {0x8F, (const uint8_t []){0x01, 0x3C, 0x3C, 0x3C, 0x3C}, 5},
    {0x94, (const uint8_t []){0x3C, 0x3C}, 2},

    {0xEA, (const uint8_t []){0x00, 0x00}, 2},
    {0xEE, (const uint8_t []){0x00}, 1},
    {0x11, NULL, 0},
    {0x00, (const uint8_t []){200}, 0xFF},
    {0x29, NULL, 0},
    {0x00, NULL, 0},
};

static const uint8_t gh8555bl_mipi_800x1280_read_id_regs[] = {0x04, 0};

const bk_display_dsi_panel_t lcd_device_gh8555bl_mipi_800x1280 = {
    .id = 0x855501,
    .name = "gh8555bl_mipi_800x1280",
    .n_lanes = DSI_ACTIVE_LANES_4,
    .fps = 36,
    .timing = {
        .h_size = PIXEL_800,
        .v_size = PIXEL_1280,
        .hsync_pulse_width = 20,
        .hsync_back_porch = 20,
        .hsync_front_porch = 80,
        .vsync_pulse_width = 2,
        .vsync_back_porch = 12,
        .vsync_front_porch = 20,
    },
    .init_cmds = gh8555bl_mipi_800x1280_init_cmds,
    .read_id_regs = gh8555bl_mipi_800x1280_read_id_regs,
    .read_id_bytes = 3,
    .reset_active_level = false,
    .reset = bk_lcd_mipi_default_reset,
    .init = bk_lcd_mipi_default_init,
};

BK_LCD_PANEL_DEVICE_SECTION(lcd_device_gh8555bl_mipi_800x1280,
                            "gh8555bl_mipi_800x1280",
                            BK_LCD_PANEL_BUS_DSI);

#endif
