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

// Common RGB panel driver.
//
// Implements the panel ops table (reset/init/del/disp_on_off) for any
// panel described by ::bk_display_rgb_panel_t. Two GPIO domains are
// managed:
//   * 24-bit parallel RGB pixel lanes muxed through bk_lcd_rgb_pixel_pinmux().
//   * private SW SPI (CLK/CS/SDA) for panel register-init - owned by the
//     SPI bus in ::BK_DISPLAY_SPI_BUS_MODE_SW and accessed through the
//     bus' tx_param op via ::bk_display_bus_tx_param().

#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel.h>
#include <avdk_check.h>

#include "bk_display_bus_priv.h"
#include "bk_lcd_panel_priv.h"

#define TAG "lcd_rgb_panel_common"

#define RGB_PIN_MUX(pin, func)                              \
    do {                                                    \
        gpio_dev_unmap(pin);                                \
        gpio_dev_map(pin, func);                            \
        bk_gpio_set_capacity(pin, GPIO_DRIVER_CAPACITY_3);  \
    } while (0)

static void lcd_rgb_panel_pinmux_init(void)
{
    RGB_PIN_MUX(LCD_RGB_R0_PIN, LCD_RGB_R0_FUNC);
    RGB_PIN_MUX(LCD_RGB_R1_PIN, LCD_RGB_R1_FUNC);
    RGB_PIN_MUX(LCD_RGB_R2_PIN, LCD_RGB_R2_FUNC);
    RGB_PIN_MUX(LCD_RGB_R3_PIN, LCD_RGB_R3_FUNC);
    RGB_PIN_MUX(LCD_RGB_R4_PIN, LCD_RGB_R4_FUNC);
    RGB_PIN_MUX(LCD_RGB_R5_PIN, LCD_RGB_R5_FUNC);
    RGB_PIN_MUX(LCD_RGB_R6_PIN, LCD_RGB_R6_FUNC);
    RGB_PIN_MUX(LCD_RGB_R7_PIN, LCD_RGB_R7_FUNC);

    RGB_PIN_MUX(LCD_RGB_G0_PIN, LCD_RGB_G0_FUNC);
    RGB_PIN_MUX(LCD_RGB_G1_PIN, LCD_RGB_G1_FUNC);
    RGB_PIN_MUX(LCD_RGB_G2_PIN, LCD_RGB_G2_FUNC);
    RGB_PIN_MUX(LCD_RGB_G3_PIN, LCD_RGB_G3_FUNC);
    RGB_PIN_MUX(LCD_RGB_G4_PIN, LCD_RGB_G4_FUNC);
    RGB_PIN_MUX(LCD_RGB_G5_PIN, LCD_RGB_G5_FUNC);
    RGB_PIN_MUX(LCD_RGB_G6_PIN, LCD_RGB_G6_FUNC);
    RGB_PIN_MUX(LCD_RGB_G7_PIN, LCD_RGB_G7_FUNC);

    RGB_PIN_MUX(LCD_RGB_B0_PIN, LCD_RGB_B0_FUNC);
    RGB_PIN_MUX(LCD_RGB_B1_PIN, LCD_RGB_B1_FUNC);
    RGB_PIN_MUX(LCD_RGB_B2_PIN, LCD_RGB_B2_FUNC);
    RGB_PIN_MUX(LCD_RGB_B3_PIN, LCD_RGB_B3_FUNC);
    RGB_PIN_MUX(LCD_RGB_B4_PIN, LCD_RGB_B4_FUNC);
    RGB_PIN_MUX(LCD_RGB_B5_PIN, LCD_RGB_B5_FUNC);
    RGB_PIN_MUX(LCD_RGB_B6_PIN, LCD_RGB_B6_FUNC);
    RGB_PIN_MUX(LCD_RGB_B7_PIN, LCD_RGB_B7_FUNC);

    RGB_PIN_MUX(LCD_RGB_CLK_PIN, LCD_RGB_CLK_FUNC);

    gpio_dev_unmap(LCD_RGB_DISP_PIN);
    BK_LOG_ON_ERR(bk_gpio_enable_output(LCD_RGB_DISP_PIN));
    bk_gpio_set_output_high(LCD_RGB_DISP_PIN);

    RGB_PIN_MUX(LCD_RGB_HSYNC_PIN, LCD_RGB_HSYNC_FUNC);
    RGB_PIN_MUX(LCD_RGB_VSYNC_PIN, LCD_RGB_VSYNC_FUNC);
    RGB_PIN_MUX(LCD_RGB_DE_PIN, LCD_RGB_DE_FUNC);
}

typedef struct {
    bk_avdk_lcd_panel_t base;
    bk_display_bus_handle_t bus_handle;
    const bk_display_rgb_panel_t *panel;
    int reset_gpio;
    uint8_t reset_active_level;
    bk_display_reset_timing_t reset_timing;
    bk_err_t (*custom_reset)(bk_avdk_lcd_panel_t *panel, void *priv);
} lcd_rgb_panel_common_t;

static inline uint16_t lcd_rgb_panel_pick_ms(uint16_t value, uint16_t fallback)
{
    return value != 0u ? value : fallback;
}

static bk_err_t lcd_rgb_panel_common_init(bk_avdk_lcd_panel_t *panel)
{
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    if (priv->panel->init_cmds == NULL) {
        return BK_OK;
    }

    for (uint32_t i = 0; ; i++) {
        const lcd_rgb_spi_init_cmd_t *cmd = &priv->panel->init_cmds[i];

        if (cmd->cmd == 0 && cmd->data == NULL && cmd->data_len == 0) {
            break;
        }

        if (cmd->cmd == 0 && cmd->data_len == 0xFF && cmd->data != NULL) {
            const uint8_t *delay_ms = (const uint8_t *)cmd->data;
            rtos_delay_milliseconds(delay_ms[0]);
            continue;
        }

        if (cmd->cmd != 0) {
            AVDK_RETURN_ON_ERROR(bk_display_bus_tx_param(priv->bus_handle, (int)cmd->cmd,
                                                        cmd->data,
                                                        cmd->data_len),
                                 TAG, "send init cmd 0x%x failed", cmd->cmd);
        }
    }

    return BK_OK;
}

static bk_err_t lcd_rgb_panel_common_reset(bk_avdk_lcd_panel_t *panel)
{
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    if (priv->custom_reset != NULL) {
        return priv->custom_reset(panel, priv);
    }

    if (priv->reset_gpio < 0) {
        return BK_OK;
    }

    gpio_dev_unmap(priv->reset_gpio);
    BK_LOG_ON_ERR(bk_gpio_enable_output(priv->reset_gpio));
    bk_gpio_set_capacity(priv->reset_gpio, GPIO_DRIVER_CAPACITY_3);

    const uint16_t idle_ms    = priv->reset_timing.idle_ms;
    const uint16_t active_ms  = priv->reset_timing.active_ms;
    const uint16_t release_ms = priv->reset_timing.release_ms;

    if (!priv->reset_active_level) {
        bk_gpio_set_output_high(priv->reset_gpio);
        rtos_delay_milliseconds(idle_ms);
        bk_gpio_set_output_low(priv->reset_gpio);
        rtos_delay_milliseconds(active_ms);
        bk_gpio_set_output_high(priv->reset_gpio);
    } else {
        bk_gpio_set_output_low(priv->reset_gpio);
        rtos_delay_milliseconds(idle_ms);
        bk_gpio_set_output_high(priv->reset_gpio);
        rtos_delay_milliseconds(active_ms);
        bk_gpio_set_output_low(priv->reset_gpio);
    }
    rtos_delay_milliseconds(release_ms);
    return BK_OK;
}

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
        bk_err_t ret = bk_display_bus_rx_param(priv->bus_handle, priv->panel->read_id_regs[i],
                                               &id_buf[i], 1);
        if (ret != BK_OK) {
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

static bk_err_t lcd_rgb_panel_common_del(bk_avdk_lcd_panel_t *panel)
{
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    os_free(priv);
    return BK_OK;
}

static bk_err_t lcd_rgb_panel_common_tx_param(bk_avdk_lcd_panel_t *panel,
                                              int lcd_cmd,
                                              const void *param,
                                              size_t param_size)
{
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");
    return bk_display_bus_tx_param(priv->bus_handle, lcd_cmd, param, param_size);
}

static bk_err_t lcd_rgb_panel_common_rx_param(bk_avdk_lcd_panel_t *panel,
                                              int lcd_cmd,
                                              void *param,
                                              size_t param_size)
{
    lcd_rgb_panel_common_t *priv = (lcd_rgb_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");
    return bk_display_bus_rx_param(priv->bus_handle, lcd_cmd, param, param_size);
}

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
    panel->reset_active_level = panel_dev_config->reset_active_level;
    panel->reset_timing.idle_ms    = lcd_rgb_panel_pick_ms(panel_desc->reset_timing.idle_ms,
                                                           BK_DISPLAY_RESET_IDLE_MS_RGB_DEFAULT);
    panel->reset_timing.active_ms  = lcd_rgb_panel_pick_ms(panel_desc->reset_timing.active_ms,
                                                           BK_DISPLAY_RESET_ACTIVE_MS_RGB_DEFAULT);
    panel->reset_timing.release_ms = lcd_rgb_panel_pick_ms(panel_desc->reset_timing.release_ms,
                                                           BK_DISPLAY_RESET_RELEASE_MS_RGB_DEFAULT);
    panel->custom_reset = panel_desc->custom_reset;

    lcd_rgb_panel_pinmux_init();

    panel->base.init = lcd_rgb_panel_common_init;
    panel->base.reset = lcd_rgb_panel_common_reset;
    panel->base.read_id = lcd_rgb_panel_common_read_id;
    panel->base.del = lcd_rgb_panel_common_del;
    panel->base.tx_param = lcd_rgb_panel_common_tx_param;
    panel->base.rx_param = lcd_rgb_panel_common_rx_param;

    *ret_panel = (bk_avdk_lcd_panel_handle_t)&panel->base;
    return BK_OK;
}
