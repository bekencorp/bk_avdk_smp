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

// Section-based panel registry helpers.
//
// Walks the linker-section array populated by ::BK_LCD_PANEL_DEVICE_SECTION
// and exposes per-bus list/find helpers (DSI, RGB, SPI, QSPI). MCU/8080
// panels are not declared on BK7259 - the collector simply returns 0.

#include <common/bk_include.h>
#include <components/bk_lcd_panel.h>
#include <components/log.h>
#include <os/str.h>

#define TAG "bk_lcd_panel_devices"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static uint32_t bk_lcd_collect_panels(uint8_t want_bus,
                                      const void **panels,
                                      uint32_t max_count)
{
    if (panels == NULL || max_count == 0) {
        return 0;
    }

    uint32_t count = 0;
    for (bk_lcd_panel_device_entry_t *entry = &__lcd_panel_device_array_start;
         entry < &__lcd_panel_device_array_end && count < max_count;
         entry++) {
        if (entry->bus_type == want_bus) {
            panels[count++] = entry->panel;
        }
    }

    return count;
}

static const void *bk_lcd_find_panel_by_name(uint8_t want_bus, const char *name)
{
    if (name == NULL) {
        return NULL;
    }
    for (bk_lcd_panel_device_entry_t *e = &__lcd_panel_device_array_start;
         e < &__lcd_panel_device_array_end; ++e) {
        if (e->bus_type == want_bus && e->name != NULL && os_strcmp(e->name, name) == 0) {
            return e->panel;
        }
    }
    return NULL;
}

uint32_t bk_lcd_get_mipi_panel_list(const bk_display_dsi_panel_t **panels, uint32_t max_count)
{
    uint32_t n = bk_lcd_collect_panels(BK_LCD_PANEL_BUS_DSI, (const void **)panels, max_count);
    LOGI("Total MIPI panels found: %u\n", (unsigned)n);
    return n;
}

uint32_t bk_lcd_get_rgb_panel_list(const bk_display_rgb_panel_t **panels, uint32_t max_count)
{
    uint32_t n = bk_lcd_collect_panels(BK_LCD_PANEL_BUS_RGB, (const void **)panels, max_count);
    LOGI("Total RGB panels found: %u\n", (unsigned)n);
    return n;
}

uint32_t bk_lcd_get_spi_panel_list(const bk_display_spi_panel_t **panels, uint32_t max_count)
{
    uint32_t n = bk_lcd_collect_panels(BK_LCD_PANEL_BUS_SPI, (const void **)panels, max_count);
    LOGI("Total SPI panels found: %u\n", (unsigned)n);
    return n;
}

uint32_t bk_lcd_get_qspi_panel_list(const bk_display_qspi_panel_t **panels, uint32_t max_count)
{
    uint32_t n = bk_lcd_collect_panels(BK_LCD_PANEL_BUS_QSPI, (const void **)panels, max_count);
    LOGI("Total QSPI panels found: %u\n", (unsigned)n);
    return n;
}

const bk_display_dsi_panel_t *bk_lcd_find_mipi_panel_by_name(const char *name)
{
    return (const bk_display_dsi_panel_t *)bk_lcd_find_panel_by_name(BK_LCD_PANEL_BUS_DSI, name);
}

const bk_display_rgb_panel_t *bk_lcd_find_rgb_panel_by_name(const char *name)
{
    return (const bk_display_rgb_panel_t *)bk_lcd_find_panel_by_name(BK_LCD_PANEL_BUS_RGB, name);
}

const bk_display_spi_panel_t *bk_lcd_find_spi_panel_by_name(const char *name)
{
    return (const bk_display_spi_panel_t *)bk_lcd_find_panel_by_name(BK_LCD_PANEL_BUS_SPI, name);
}

const bk_display_qspi_panel_t *bk_lcd_find_qspi_panel_by_name(const char *name)
{
    return (const bk_display_qspi_panel_t *)bk_lcd_find_panel_by_name(BK_LCD_PANEL_BUS_QSPI, name);
}
