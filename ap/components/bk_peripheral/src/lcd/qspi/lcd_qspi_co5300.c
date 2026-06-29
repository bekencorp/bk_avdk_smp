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

#include <common/bk_include.h>
#include <driver/lcd_types.h>
#include <components/bk_lcd_panel.h>

#define LCD_QSPI_CO5300_REGISTER_WRITE_COMMAND        0x02
#define LCD_QSPI_CO5300_REGISTER_READ_COMMAND         0x03

/* 410x502, 2A col 0x16~0x1AF, 2B row 0x00~0x1F5 */
#define CO5300_PANEL_WIDTH                            410
#define CO5300_PANEL_HEIGHT                           502

static const lcd_qspi_init_cmd_t co5300_init_cmds[] =
{
    {0xFE, {0x00}, 1},
    {0xC4, {0x80}, 1},
    {0x3A, {0x55}, 1},
    {0x35, {0x00}, 1},
    {0x53, {0x20}, 1},
    {0x51, {0xFF}, 1},
    {0x63, {0xFF}, 1},
    {0x2A, {0x00, 0x16, 0x01, 0xAF}, 4},
    {0x2B, {0x00, 0x00, 0x01, 0xF5}, 4},
    {0x11, {0x00}, 0},
    {0x00, {120}, 0xFF},
    {0x29, {0x00}, 0},
};

static uint8_t co5300_cmd[4] = {0x32, 0x00, 0x2c, 0x00};

static const lcd_qspi_t lcd_qspi_co5300_config =
{
    .clk = LCD_QSPI_40M,
    .refresh_method = LCD_QSPI_REFRESH_BY_FRAME,
    .reg_write_cmd = LCD_QSPI_CO5300_REGISTER_WRITE_COMMAND,
    .reg_read_cmd = LCD_QSPI_CO5300_REGISTER_READ_COMMAND,
    .reg_read_config.dummy_clk = 0,
    .reg_read_config.dummy_mode = LCD_QSPI_NO_INSERT_DUMMMY_CLK,
    .pixel_write_config.cmd = co5300_cmd,
    .pixel_write_config.cmd_len = sizeof(co5300_cmd),
    .init_cmd = co5300_init_cmds,
    .device_init_cmd_len = sizeof(co5300_init_cmds) / sizeof(lcd_qspi_init_cmd_t),
    .refresh_config = {0},
    .frame_len = CO5300_PANEL_WIDTH * CO5300_PANEL_HEIGHT * CONFIG_LCD_QSPI_COLOR_DEPTH_BYTE,
};

const bk_display_qspi_panel_t lcd_device_co5300 =
{
    .name = "co5300",
    .width = CO5300_PANEL_WIDTH,
    .height = CO5300_PANEL_HEIGHT,
    .qspi = &lcd_qspi_co5300_config,
};

BK_LCD_PANEL_DEVICE_SECTION(lcd_device_co5300, "co5300", BK_LCD_PANEL_BUS_QSPI);
