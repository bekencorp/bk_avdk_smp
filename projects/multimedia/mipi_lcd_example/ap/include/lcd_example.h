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

#pragma once

#include <avdk_check.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_display_types.h>
#include <components/bk_display_bus.h>
#include <components/bk_lcd_types.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif


#define RGB565_RED      0xF800
#define RGB565_GREEN    0x07E0
#define RGB565_BLUE     0x001F
#define RGB565_YELLOW   0xFFE0
#define RGB565_CYAN     0x07FF
#define RGB565_MAGENTA  0xF81F
#define RGB565_WHITE    0xFFFF
#define RGB565_BLACK    0x0000
#define RGB565_GRAY     0x8410
#define RGB565_BROWN    0x0A02
#define RGB565_PINK     0x0F0C
#define RGB565_ORANGE   0x0F05
#define RGB565_PURPLE   0x0800


typedef struct
{
    uint8_t enable;
    uint8_t decompress;
    bk_display_ctlr_handle_t dpu_ctlr_handle;
    bk_display_bus_handle_t dis_bus_handle;
    bk_avdk_lcd_panel_handle_t panel_handle;
    beken_thread_t thread;
    beken_semaphore_t sem;
    uint32_t count;
    uint32_t pixel_index;
    uint16_t width;
    uint16_t height;
    bk_pixel_format_t format;
} display_ctx_t;


avdk_err_t lcd_example_dsi_open(display_ctx_t *context, const char *panel_name, bk_pixel_format_t format);
avdk_err_t lcd_example_dsi_close(display_ctx_t *context);
avdk_err_t lcd_example_flush_thread_start(display_ctx_t *context);
avdk_err_t lcd_example_flush_thread_stop(display_ctx_t *context);
void cli_mipi_lcd_switch_format(const bk_display_pixel_format_config_t *config, const char *name);


#ifdef __cplusplus
}
#endif
