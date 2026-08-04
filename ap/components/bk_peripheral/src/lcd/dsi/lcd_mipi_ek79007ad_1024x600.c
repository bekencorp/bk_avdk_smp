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
 * @file lcd_mipi_ek79007ad_1024x600.c
 * @brief EK79007AD MIPI DSI Panel Driver (1024x600)
 *
 * BOE 7.0" (B4) IPS panel with Fitipower EK79007AD source driver.
 * Init sequence from vendor file; gamma registers use MIPI DCS short
 * writes (0x15). Lane count and sleep-out/display-on follow EK79007
 * reference bring-up.
 */

#include <components/bk_lcd_panel.h>
#include <driver/mipi_dsi_types.h>
#include <common/avdk_pixel_types.h>

#if CONFIG_LCD_EK79007AD_MIPI_1024x600

static const lcd_mipi_init_cmd_t ek79007ad_mipi_1024x600_init_cmds[] = {
    /* 4-lane MIPI (0xB2 Pad Control, En_2lane = 0) */
    {0xB2, (const uint8_t []){0x00}, 1},
    /* Gamma settings: BOE7.0 (B4) 1024x600 IPS G2.5 */
    {0x80, (const uint8_t []){0xAC}, 1},
    {0x81, (const uint8_t []){0xB8}, 1},
    {0x82, (const uint8_t []){0x09}, 1},
    {0x83, (const uint8_t []){0x78}, 1},
    {0x84, (const uint8_t []){0x7F}, 1},
    {0x85, (const uint8_t []){0xBB}, 1},
    {0x86, (const uint8_t []){0x70}, 1},
    {0x00, (const uint8_t []){50}, 0xFF},
    {0x11, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){120}, 0xFF},
    {0x29, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){20}, 0xFF},
    {0x00, NULL, 0},
};

// Power-down sequence: DISPOFF -> (>=1 frame) -> SLPIN -> (charge-pump
// discharge). Sent by bk_lcd_mipi_default_off() during bk_display_deinit()
// while the DSI command channel is still up, before RESETn / VDDIO drop.
static const lcd_mipi_init_cmd_t ek79007ad_mipi_1024x600_off_cmds[] = {
    {0x28, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){50},   0xFF},
    {0x10, (const uint8_t []){0x00}, 0},
    {0x00, (const uint8_t []){120},  0xFF},
    {0x00, NULL, 0},
};

static const uint8_t ek79007ad_mipi_1024x600_read_id_regs[] = {0x04, 0};

const bk_display_dsi_panel_t lcd_device_ek79007ad_mipi_1024x600 = {
    .id = 0x7907ad,
    .name = "ek79007ad_mipi_1024x600",
    .n_lanes = DSI_ACTIVE_LANES_4,
    .fps = 60,
    .timing = {
        .h_size = PIXEL_1024,
        .v_size = PIXEL_600,
        .hsync_pulse_width = 10,
        .hsync_back_porch = 160,
        .hsync_front_porch = 160,
        .vsync_pulse_width = 1,
        .vsync_back_porch = 23,
        .vsync_front_porch = 12,
    },
    .init_cmds = ek79007ad_mipi_1024x600_init_cmds,
    .off_cmds = ek79007ad_mipi_1024x600_off_cmds,
    .read_id_regs = ek79007ad_mipi_1024x600_read_id_regs,
    .read_id_bytes = 3,
    .reset_active_level = false,
    .reset = bk_lcd_mipi_default_reset,
    .init  = bk_lcd_mipi_default_init,
    .off   = bk_lcd_mipi_default_off,
};

BK_LCD_PANEL_DEVICE_SECTION(lcd_device_ek79007ad_mipi_1024x600,
                            "ek79007ad_mipi_1024x600",
                            BK_LCD_PANEL_BUS_DSI);
#endif
