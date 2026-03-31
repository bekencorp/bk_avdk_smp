#include <components/bk_lcd_types.h>
#include <components/bk_display_types.h>
#include <components/log.h>
#include <os/str.h>

#define TAG "bk_lcd_panel_devices"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

uint32_t bk_lcd_get_mipi_panel_list(const bk_display_dsi_panel_t **panels, uint32_t max_count)
{
    if (panels == NULL || max_count == 0) {
        return 0;
    }

    uint32_t count = 0;

    // Iterate through the section to find all DSI panels
    for (bk_lcd_panel_device_entry_t *entry = &__lcd_panel_device_array_start;
         entry < &__lcd_panel_device_array_end && count < max_count;
         entry++) {
        if (entry->type == 1) {  // DSI panel
            LOGI("MIPI Panel: name=%s, address=%p\n", entry->name, entry->panel);
            panels[count++] = (const bk_display_dsi_panel_t *)entry->panel;
        }
    }

    LOGI("Total MIPI panels found: %d\n", count);
    return count;
}

uint32_t bk_lcd_get_rgb_panel_list(const bk_display_rgb_panel_t **panels, uint32_t max_count)
{
    if (panels == NULL || max_count == 0) {
        return 0;
    }

    uint32_t count = 0;

    // Iterate through the section to find all RGB panels
    for (bk_lcd_panel_device_entry_t *entry = &__lcd_panel_device_array_start;
         entry < &__lcd_panel_device_array_end && count < max_count;
         entry++) {
        if (entry->type == 0) {  // RGB panel
            LOGI("RGB Panel: name=%s, address=%p\n", entry->name, entry->panel);
            panels[count++] = (const bk_display_rgb_panel_t *)entry->panel;
        }
    }

    LOGI("Total RGB panels found: %d\n", count);
    return count;
}

const bk_display_dsi_panel_t *bk_lcd_find_mipi_panel_by_name(const char *name)
{
    for (bk_lcd_panel_device_entry_t *e = &__lcd_panel_device_array_start;
         e < &__lcd_panel_device_array_end; ++e) {
        if (e->type == 1 && e->name && os_strcmp(e->name, name) == 0) {
            return (const bk_display_dsi_panel_t *)e->panel;
        }
    }
    return NULL;
}

const bk_display_rgb_panel_t *bk_lcd_find_rgb_panel_by_name(const char *name)
{
    for (bk_lcd_panel_device_entry_t *e = &__lcd_panel_device_array_start;
         e < &__lcd_panel_device_array_end; ++e) {
        if (e->type == 0 && e->name && os_strcmp(e->name, name) == 0) {
            return (const bk_display_rgb_panel_t *)e->panel;
        }
    }
    return NULL;
}