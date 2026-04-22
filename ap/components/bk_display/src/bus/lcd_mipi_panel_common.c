// Common MIPI DSI panel driver moved from bk_peripheral/src/lcd/dsi/lcd_mipi_panel_common.c

#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_display_types.h>
#include <components/bk_display_bus.h>

#include <components/bk_lcd_panel_io.h>
#include <components/bk_lcd_types.h>
#include <bk_lcd_panel_commands.h>
#include <avdk_check.h>
#include <components/log.h>

#define TAG "lcd_panel_common"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

// Common panel private structure
typedef struct {
    bk_avdk_lcd_panel_t base;
    bk_display_bus_handle_t bus_handle;
    const bk_display_dsi_panel_t *panel;
    int reset_gpio;
    uint8_t reset_active_level;
    bk_err_t (*custom_reset)(bk_avdk_lcd_panel_t *panel, void *priv);
    bk_err_t (*custom_init)(bk_avdk_lcd_panel_t *panel, void *priv);
} lcd_panel_common_t;

// Common init function - all panels reuse this
static bk_err_t lcd_panel_common_init(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    // Configure clock
    //AVDK_RETURN_ON_FALSE(priv->panel->timing.clk != 0, BK_ERR_NOT_SUPPORT, TAG, "panel timing.clk not set");
    bk_panel_clock_config_t clock_config = {
        .clk = priv->panel->timing.clk,
        .n_lanes = priv->panel->n_lanes,
        .fps = priv->panel->fps,
        .timing = priv->panel->timing,
    };
    AVDK_RETURN_ON_ERROR(bk_display_bus_set_clock(priv->bus_handle, &clock_config), TAG, "set clock failed");
    
    // Optional custom init hook for special panels (e.g. HDMI bridge)
    if (priv->custom_init != NULL) {
        LOGI("%s %s custom init start\n", __func__, priv->panel->name);
        AVDK_RETURN_ON_ERROR(priv->custom_init(panel, priv), TAG, "custom init failed");
        LOGI("%s %s custom init end\n", __func__, priv->panel->name);
        return BK_OK;
    }

    // Send initialization command sequence
    if (priv->panel->init_cmds) {
        for (uint32_t i = 0; priv->panel->init_cmds[i].cmd != 0 || priv->panel->init_cmds[i].data != NULL; i++) {
            if (priv->panel->init_cmds[i].cmd == 0 && priv->panel->init_cmds[i].data == NULL) {
                break;  // End marker
            }
            /* Delay: {0, (const uint8_t []){ms}, 0xFF} — same convention as RGB SPI init */
            if (priv->panel->init_cmds[i].cmd == 0 && priv->panel->init_cmds[i].data_len == 0xFF
                && priv->panel->init_cmds[i].data != NULL) {
                rtos_delay_milliseconds(((const uint8_t *)priv->panel->init_cmds[i].data)[0]);
                continue;
            }
            AVDK_RETURN_ON_ERROR(bk_display_bus_write(priv->bus_handle, BK_DISPLAY_BUS_RW_DSI_CMD,
                                                      priv->panel->init_cmds[i].cmd,
                                                      priv->panel->init_cmds[i].data,
                                                      priv->panel->init_cmds[i].data_len),
                                 TAG, "send init command failed");
        }
    }
    else {
        LOGI("%s %s no init commands, skip init\n", __func__, priv->panel->name);
    }

    return BK_OK;
}

// Common reset function
static bk_err_t lcd_panel_common_reset(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");
    if (priv->custom_reset != NULL) {
        LOGI("%s %s custom reset start\n", __func__, priv->panel->name);
        return priv->custom_reset(panel, priv);
    }
    // Common reset logic
    if (priv->reset_gpio < 0) {
        LOGI("%s %s no reset pin %d, skip reset\n", __func__, priv->panel->name, priv->reset_gpio);
        return BK_OK;  // No reset pin
    }

    gpio_dev_unmap(priv->reset_gpio);
    BK_LOG_ON_ERR(bk_gpio_enable_output(priv->reset_gpio));
    bk_gpio_set_capacity(priv->reset_gpio, GPIO_DRIVER_CAPACITY_3);

    // Reset sequence: low -> high
    if (!priv->reset_active_level) {
        bk_gpio_set_output_low(priv->reset_gpio);
        rtos_delay_milliseconds(10);
        bk_gpio_set_output_high(priv->reset_gpio);
    } else {
        bk_gpio_set_output_high(priv->reset_gpio);
        rtos_delay_milliseconds(10);
        bk_gpio_set_output_low(priv->reset_gpio);
        rtos_delay_milliseconds(10);
        bk_gpio_set_output_high(priv->reset_gpio);
    }
    rtos_delay_milliseconds(120);  // Wait for panel ready

    return BK_OK;
}

// Common read ID function
static bk_err_t lcd_panel_common_read_id(bk_avdk_lcd_panel_t *panel, uint32_t *id)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel && id, BK_ERR_NULL_PARAM, TAG, "invalid arguments");

    if (priv->panel->read_id_regs == NULL) {
        return BK_ERR_NOT_SUPPORT;
    }

    uint8_t id_buf[3] = {0};
    uint8_t read_bytes = priv->panel->read_id_bytes;
    
    // Determine how many bytes to read
    if (read_bytes == 0) {
        // Auto-detect: count non-zero registers
        for (int i = 0; i < 3 && priv->panel->read_id_regs[i] != 0; i++) {
            read_bytes++;
        }
    }
    
    if (read_bytes == 0 || read_bytes > 3) {
        return BK_ERR_NOT_SUPPORT;
    }

    // Read ID from first register (some panels read multiple bytes from single register)
    uint8_t cmd = priv->panel->read_id_regs[0];
    avdk_err_t ret = bk_display_bus_read(priv->bus_handle, BK_DISPLAY_BUS_RW_DSI_CMD,
                                         cmd, id_buf, read_bytes);
    if (ret != AVDK_ERR_OK) {
        return ret;
    }
    
    rtos_delay_milliseconds(100);

    // Combine ID bytes (handle different byte orders)
    if (read_bytes == 1) {
        *id = id_buf[0];
    } else if (read_bytes == 2) {
        // ST7701S style: ((read_id&0xFF)<<8)|((read_id>>8)&0xFF)
        *id = (id_buf[0] << 8) | id_buf[1];
    } else if (read_bytes == 3) {
        // FL7703NP style: ((read_id&0xFF)<<16)|(read_id&0xFF00)|((read_id>>16)&0xFF)
        *id = (id_buf[0] << 16) | (id_buf[1] << 8) | id_buf[2];
    }

    return BK_OK;
}

// Common get timing function
static bk_err_t lcd_panel_common_get_disp_timing(bk_avdk_lcd_panel_t *panel, bk_display_timing_t *timing)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel && timing, BK_ERR_NULL_PARAM, TAG, "invalid arguments");

    os_memcpy(timing, &priv->panel->timing, sizeof(bk_display_timing_t));
    return BK_OK;
}

// Common delete function
static bk_err_t lcd_panel_common_del(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    if (priv) {
        os_free(priv);
    }
    return BK_OK;
}

// Common display on/off function
static bk_err_t lcd_panel_common_disp_on_off(bk_avdk_lcd_panel_t *panel, bool on_off)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    uint8_t command = on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF;
    AVDK_RETURN_ON_ERROR(bk_display_bus_write(priv->bus_handle, BK_DISPLAY_BUS_RW_DSI_CMD,
                                              command, NULL, 0),
                         TAG, "send display on/off command failed");
    rtos_delay_milliseconds(100);
    return BK_OK;
}

// Common sleep function
static bk_err_t lcd_panel_common_sleep(bk_avdk_lcd_panel_t *panel, bool sleep)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    uint8_t command = sleep ? LCD_CMD_SLPIN : LCD_CMD_SLPOUT;
    AVDK_RETURN_ON_ERROR(bk_display_bus_write(priv->bus_handle, BK_DISPLAY_BUS_RW_DSI_CMD,
                                              command, NULL, 0),
                         TAG, "send sleep command failed");
    rtos_delay_milliseconds(100);
    return BK_OK;
}

// Common invert color function
static bk_err_t lcd_panel_common_invert_color(bk_avdk_lcd_panel_t *panel, bool invert_color_data)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    uint8_t command = invert_color_data ? LCD_CMD_INVON : LCD_CMD_INVOFF;
    AVDK_RETURN_ON_ERROR(bk_display_bus_write(priv->bus_handle, BK_DISPLAY_BUS_RW_DSI_CMD,
                                              command, NULL, 0),
                         TAG, "send invert color command failed");
    return BK_OK;
}

// Common mirror function (basic implementation, may need customization for specific panels)
static bk_err_t lcd_panel_common_mirror(bk_avdk_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    uint8_t madctl_val = 0;
    // Read current MADCTL value
    avdk_err_t ret = bk_display_bus_read(priv->bus_handle, BK_DISPLAY_BUS_RW_DSI_CMD,
                                         LCD_CMD_MADCTL, &madctl_val, 1);
    if (ret != AVDK_ERR_OK) {
        return ret;
    }

    // Control mirror through LCD command bits
    if (mirror_x) {
        madctl_val |= LCD_CMD_MX_BIT;  // Column address order
    } else {
        madctl_val &= ~LCD_CMD_MX_BIT;
    }
    if (mirror_y) {
        madctl_val |= LCD_CMD_MY_BIT;  // Row address order
    } else {
        madctl_val &= ~LCD_CMD_MY_BIT;
    }

    AVDK_RETURN_ON_ERROR(bk_display_bus_write(priv->bus_handle, BK_DISPLAY_BUS_RW_DSI_CMD,
                                              LCD_CMD_MADCTL, (const uint8_t[]){madctl_val}, 1),
                         TAG, "send mirror command failed");
    return BK_OK;
}

// Common panel creation function - used by all standard panels
bk_err_t bk_lcd_new_mipi_panel_common(bk_display_bus_handle_t bus_handle,
                                 const bk_lcd_panel_dev_config_t *panel_dev_config,
                                 const bk_display_dsi_panel_t *panel_desc,
                                 bk_avdk_lcd_panel_handle_t *ret_panel)
{
    AVDK_RETURN_ON_FALSE(bus_handle && panel_dev_config && panel_desc && ret_panel,
                         BK_ERR_NULL_PARAM, TAG, "invalid arguments");
    AVDK_RETURN_ON_FALSE(panel_desc->name != NULL, BK_ERR_NULL_PARAM, TAG, "panel name is NULL");

    lcd_panel_common_t *panel = os_malloc(sizeof(lcd_panel_common_t));
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NO_MEM, TAG, "malloc failed");

    os_memset(panel, 0, sizeof(lcd_panel_common_t));

    panel->bus_handle = bus_handle;
    panel->panel = panel_desc;
    // Store custom callbacks at creation time to avoid repeated dereference
    panel->custom_reset = panel_desc->custom_reset;
    panel->custom_init = panel_desc->custom_init;
    panel->reset_gpio = panel_dev_config->reset_pin;
    panel->reset_active_level = panel_dev_config->flags.reset_active_level;
    panel->base.user_data = panel_dev_config->vendor_config;

    // Set common operation functions
    panel->base.init = lcd_panel_common_init;
    panel->base.reset = lcd_panel_common_reset;
    panel->base.read_id = lcd_panel_common_read_id;
    panel->base.get_disp_timing = lcd_panel_common_get_disp_timing;
    panel->base.del = lcd_panel_common_del;
    panel->base.disp_on_off = lcd_panel_common_disp_on_off;
    panel->base.disp_sleep = lcd_panel_common_sleep;
    panel->base.invert_color = lcd_panel_common_invert_color;
    panel->base.mirror = lcd_panel_common_mirror;

    *ret_panel = (bk_avdk_lcd_panel_handle_t)&panel->base;
    return BK_OK;
}

