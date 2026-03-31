// Common RGB panel driver moved from bk_peripheral/src/lcd/rgb/lcd_rgb_panel_common.c


#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_display_types.h>
#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel_io.h>
#include <components/bk_lcd_types.h>
#include <avdk_check.h>

#define TAG "lcd_rgb_panel_common"

// Common RGB panel private structure
typedef struct {
    bk_avdk_lcd_panel_t base;
    bk_display_bus_handle_t bus_handle;
    const bk_display_rgb_panel_t *panel;
    int reset_gpio;
    uint8_t reset_active_level;
    bk_err_t (*custom_reset)(bk_avdk_lcd_panel_t *panel, void *priv);
} lcd_rgb_panel_common_t;

// Helper macros for SPI write (8-bit format)
#define LCD_SPI_WRITE_CMD(priv, cmd) \
    bk_display_bus_write((priv)->bus_handle, BK_DISPLAY_BUS_RW_SPI_CMD, cmd, NULL, 0)

#define LCD_SPI_WRITE_DATA(priv, data) \
    bk_display_bus_write((priv)->bus_handle, BK_DISPLAY_BUS_RW_SPI_DATA, data, NULL, 0)

// Helper macros for SPI write (16-bit HF format)
#define LCD_SPI_WRITE_HF_CMD(priv, cmd) \
    bk_display_bus_write((priv)->bus_handle, BK_DISPLAY_BUS_RW_SPI_HF_CMD, cmd, NULL, 0)

#define LCD_SPI_WRITE_HF_DATA(priv, data) \
    bk_display_bus_write((priv)->bus_handle, BK_DISPLAY_BUS_RW_SPI_HF_DATA, data, NULL, 0)

// Common init function - all RGB panels reuse this
static bk_err_t lcd_rgb_panel_common_init(bk_avdk_lcd_panel_t *panel)
{
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel, BK_ERR_NULL_PARAM, TAG, "invalid panel");
    // Send initialization command sequence

    // Unified handling for both 8-bit and 16-bit commands based on spi_cmd_16bit flag in panel config
    if (priv->panel->init_cmds != NULL) {
        uint8_t is_16bit = priv->panel->spi_cmd_16bit;
        
        if (is_16bit) {
            // 16-bit command format processing
            for (uint32_t i = 0; ; i++) {
                const lcd_rgb_spi_init_cmd_t *cmd = &priv->panel->init_cmds[i];
                
                // Check for end marker: cmd=0 and data==NULL and data_len=0
                if (cmd->cmd == 0 && cmd->data == NULL && cmd->data_len == 0) {
                    break;  // End marker
                }

                // Check if this is a delay command: {0x0, (const uint8_t []){delay_ms}, 0xFF}
                if (cmd->cmd == 0 && cmd->data_len == 0xFF && cmd->data != NULL) {
                    // Extract delay time from data[0] (always 8-bit format for delay)
                    const uint8_t *delay_data = (const uint8_t *)cmd->data;
                    rtos_delay_milliseconds(delay_data[0]);
                    continue;
                }

                // Send 16-bit HF command
                if (cmd->cmd != 0) {
                    AVDK_RETURN_ON_ERROR(LCD_SPI_WRITE_HF_CMD(priv, cmd->cmd),
                                         TAG, "send init hf command failed");

                    // Send data if present
                    if (cmd->data != NULL && cmd->data_len > 0) {
                        const uint16_t *data = (const uint16_t *)cmd->data;
                        for (uint8_t j = 0; j < cmd->data_len; j++) {
                            AVDK_RETURN_ON_ERROR(LCD_SPI_WRITE_HF_DATA(priv, data[j]),
                                                 TAG, "send init hf data failed");
                        }
                    }
                }
            }
        } else {
            // 8-bit command format processing
            for (uint32_t i = 0; ; i++) {
                const lcd_rgb_spi_init_cmd_t *cmd = &priv->panel->init_cmds[i];
                
                // Check for end marker: cmd=0 and data==NULL and data_len=0
                if (cmd->cmd == 0 && cmd->data == NULL && cmd->data_len == 0) {
                    break;  // End marker
                }

                // Check if this is a delay command: {0x0, (const uint8_t []){delay_ms}, 0xFF}
                if (cmd->cmd == 0 && cmd->data_len == 0xFF && cmd->data != NULL) {
                    // Extract delay time from data[0]
                    const uint8_t *delay_data = (const uint8_t *)cmd->data;
                    rtos_delay_milliseconds(delay_data[0]);
                    continue;
                }

                // Send 8-bit command
                if (cmd->cmd != 0) {
                    AVDK_RETURN_ON_ERROR(LCD_SPI_WRITE_CMD(priv, (uint8_t)cmd->cmd),
                                         TAG, "send init command failed");

                    // Send data if present
                    if (cmd->data != NULL && cmd->data_len > 0) {
                        const uint8_t *data = (const uint8_t *)cmd->data;
                        for (uint8_t j = 0; j < cmd->data_len; j++) {
                            AVDK_RETURN_ON_ERROR(LCD_SPI_WRITE_DATA(priv, data[j]),
                                                 TAG, "send init data failed");
                        }
                    }
                }
            }
        }
    }

    return BK_OK;
}

// Common reset function
// Note: Reset is also executed in init function, so this function can be empty for compatibility
// If you need separate reset, uncomment the code below
static bk_err_t lcd_rgb_panel_common_reset(bk_avdk_lcd_panel_t *panel)
{
    
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    // If custom reset function is provided, use it instead of common reset logic
    if (priv->custom_reset != NULL) {
        return priv->custom_reset(panel, priv);
    }

    // Common reset logic
    if (priv->reset_gpio < 0) {
        return BK_OK;  // No reset pin
    }

    // Reset sequence: high -> low -> high
    if (!priv->reset_active_level) { 
        bk_gpio_set_output_high(priv->reset_gpio);
        rtos_delay_milliseconds(10);
        bk_gpio_set_output_low(priv->reset_gpio);
        rtos_delay_milliseconds(120);
        bk_gpio_set_output_high(priv->reset_gpio);
    } else {
        bk_gpio_set_output_low(priv->reset_gpio);
        rtos_delay_milliseconds(10);
        bk_gpio_set_output_high(priv->reset_gpio);
        rtos_delay_milliseconds(120);
        bk_gpio_set_output_low(priv->reset_gpio);
    }
    rtos_delay_milliseconds(60);
    return BK_OK;
}

// Common read ID function
static bk_err_t lcd_rgb_panel_common_read_id(bk_avdk_lcd_panel_t *panel, uint32_t *id)
{
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel && id, BK_ERR_NULL_PARAM, TAG, "invalid arguments");

    if (priv->panel->read_id_regs == NULL) {
        return BK_ERR_NOT_SUPPORT;
    }

    uint8_t id_buf[3] = {0};
    uint8_t reg_count = 0;

    for (int i = 0; i < 3 && priv->panel->read_id_regs[i] != 0; i++) {
        // Send read command
        AVDK_RETURN_ON_ERROR(LCD_SPI_WRITE_CMD(priv, priv->panel->read_id_regs[i]),
                             TAG, "send read id command failed");

        // Read data (for SPI, we may need to read via bus read)
        avdk_err_t ret = bk_display_bus_read(priv->bus_handle, BK_DISPLAY_BUS_RW_SPI_DATA,
                                            priv->panel->read_id_regs[i], &id_buf[i], 1);
        if (ret != AVDK_ERR_OK) {
            return ret;
        }
        reg_count++;
    }

    *id = 0;
    if (priv->panel->read_id_bytes == 1 && reg_count >= 1) {
        *id = id_buf[0];
    } else if (priv->panel->read_id_bytes == 2 && reg_count >= 2) {
        *id = (id_buf[0] << 8) | id_buf[1];
    } else if (priv->panel->read_id_bytes == 3 && reg_count >= 3) {
        *id = (id_buf[0] << 16) | (id_buf[1] << 8) | id_buf[2];
    } else {
        return BK_ERR_NOT_SUPPORT;
    }

    return BK_OK;
}

// Common get display timing function
static bk_err_t lcd_rgb_panel_common_get_disp_timing(bk_avdk_lcd_panel_t *panel, bk_display_timing_t *timing)
{
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel && timing, BK_ERR_NULL_PARAM, TAG, "invalid arguments");

    os_memcpy(timing, &priv->panel->timing, sizeof(bk_display_timing_t));
    return BK_OK;
}

// Common delete function
static bk_err_t lcd_rgb_panel_common_del(bk_avdk_lcd_panel_t *panel)
{
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    os_free(priv);
    return BK_OK;
}

// Common RGB panel creation function - used by all standard RGB panels
bk_err_t bk_lcd_new_rgb_panel_common(bk_display_bus_handle_t bus_handle,
                                     const bk_lcd_panel_dev_config_t *panel_dev_config,
                                     const bk_display_rgb_panel_t *panel_desc,
                                     bk_avdk_lcd_panel_handle_t *ret_panel)
{
    AVDK_RETURN_ON_FALSE(bus_handle && panel_dev_config && panel_desc && ret_panel,
                         BK_ERR_NULL_PARAM, TAG, "invalid arguments");
    AVDK_RETURN_ON_FALSE(panel_desc->name != NULL, BK_ERR_NULL_PARAM, TAG, "panel name is NULL");

    lcd_rgb_panel_common_t *panel = os_malloc(sizeof(lcd_rgb_panel_common_t));
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NO_MEM, TAG, "malloc failed");

    os_memset(panel, 0, sizeof(lcd_rgb_panel_common_t));

    panel->bus_handle = bus_handle;
    panel->panel = panel_desc;
    panel->reset_gpio = panel_dev_config->reset_pin;
    panel->reset_active_level = panel_dev_config->flags.reset_active_level;
    panel->custom_reset = panel_desc->custom_reset;

    // Set common operation functions
    panel->base.init = lcd_rgb_panel_common_init;
    panel->base.reset = lcd_rgb_panel_common_reset;
    panel->base.read_id = lcd_rgb_panel_common_read_id;
    panel->base.get_disp_timing = lcd_rgb_panel_common_get_disp_timing;
    panel->base.del = lcd_rgb_panel_common_del;

    *ret_panel = (bk_avdk_lcd_panel_handle_t)&panel->base;
    return BK_OK;
}

