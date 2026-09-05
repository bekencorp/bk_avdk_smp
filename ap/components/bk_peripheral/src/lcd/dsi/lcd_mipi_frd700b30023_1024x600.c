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
 * @file lcd_mipi_frd700b30023_1024x600.c
 * @brief FRD700B30023 MIPI DSI Panel Driver (1024x600)
 */

#include <components/bk_lcd_panel.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>

#if CONFIG_LCD_FRD700B30023_MIPI_1024x600

static const lcd_mipi_init_cmd_t frd700b30023_mipi_1024x600_init_cmds[] = {
    {0x80, (const uint8_t []){0xAB}, 1},
    {0x81, (const uint8_t []){0x4B}, 1},
    {0x82, (const uint8_t []){0x84}, 1},
    {0x83, (const uint8_t []){0x88}, 1},
    {0x84, (const uint8_t []){0xA8}, 1},
    {0x85, (const uint8_t []){0xE3}, 1},
    {0x86, (const uint8_t []){0xB8}, 1},
    {0xB2, (const uint8_t []){0x10}, 1},
    {0x00, NULL, 0},
};

static const uint8_t frd700b30023_mipi_1024x600_read_id_regs[] = {
    0xDA, 0xDB, 0xDC, 0
};

const bk_display_dsi_panel_t lcd_device_frd700b30023_mipi_1024x600 = {
    .id = 0x70010F,
    .name = "frd700b30023_mipi_1024x600",
    .n_lanes = DSI_ACTIVE_LANES_4,
    .fps = 36,
    .timing = {
        .h_size = PIXEL_1024,
        .v_size = PIXEL_600,
        .hsync_pulse_width = 10,
        .hsync_back_porch = 160,
        .hsync_front_porch = 160,
        .vsync_pulse_width = 1,
        .vsync_back_porch = 23,
        .vsync_front_porch = 16,
    },
    .init_cmds = frd700b30023_mipi_1024x600_init_cmds,
    .read_id_regs = frd700b30023_mipi_1024x600_read_id_regs,
    .read_id_bytes = 3,
    .reset_active_level = false,
    .reset = bk_lcd_mipi_default_reset,
    .init = bk_lcd_mipi_default_init,
};

BK_LCD_PANEL_DEVICE_SECTION(lcd_device_frd700b30023_mipi_1024x600,
                            "frd700b30023_mipi_1024x600",
                            BK_LCD_PANEL_BUS_DSI);

#endif
