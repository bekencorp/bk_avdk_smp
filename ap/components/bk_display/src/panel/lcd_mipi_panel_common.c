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

// Common MIPI-DSI panel driver.
//
// Owns the panel-handle ops table. The descriptor's .init / .reset
// function pointers carry the per-panel behaviour:
//   * point them at bk_lcd_mipi_default_init / _reset for stock panels;
//   * point them at a custom function (which may call the defaults) for
//     panels that need extra steps;
//   * leave them NULL to skip the phase entirely (bridge ICs configured
//     out-of-band, already-initialized panels, ...).

#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel.h>
#include <driver/mipi_dsi.h>
#include <avdk_check.h>
#include <components/log.h>

#include "bk_display_bus_priv.h"
#include "bk_lcd_panel_priv.h"

#define TAG "lcd_panel_common"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

typedef struct {
    bk_avdk_lcd_panel_t base;
    bk_display_bus_handle_t bus_handle;
    const bk_display_dsi_panel_t *panel;
    int reset_gpio;
    bool reset_active_level;
    bk_display_reset_timing_t reset_timing;
} lcd_panel_common_t;

static inline uint16_t lcd_panel_common_pick_ms(uint16_t value, uint16_t fallback)
{
    return value != 0u ? value : fallback;
}

bk_err_t bk_lcd_mipi_default_init(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    if (priv->panel->init_cmds == NULL) {
        return BK_OK;
    }

    for (uint32_t i = 0; priv->panel->init_cmds[i].cmd != 0 || priv->panel->init_cmds[i].data != NULL; i++) {
        if (priv->panel->init_cmds[i].cmd == 0 && priv->panel->init_cmds[i].data == NULL) {
            break;
        }
        if (priv->panel->init_cmds[i].cmd == 0 && priv->panel->init_cmds[i].data_len == 0xFF
            && priv->panel->init_cmds[i].data != NULL) {
            rtos_delay_milliseconds(((const uint8_t *)priv->panel->init_cmds[i].data)[0]);
            continue;
        }
        AVDK_RETURN_ON_ERROR(bk_display_bus_tx_param(priv->bus_handle,
                                                    (int)priv->panel->init_cmds[i].cmd,
                                                    priv->panel->init_cmds[i].data,
                                                    priv->panel->init_cmds[i].data_len),
                             TAG, "send init command failed");
    }

    return BK_OK;
}

bk_err_t bk_lcd_mipi_default_off(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    /* DPU has stopped feeding (called after dpu_core_deinit): park the DSI host
     * in command mode so the video machine stops pulling the idle DPI FIFO (else
     * dpi_bpl_udflw storm). Unconditional -- a pipeline concern, not gated on
     * off_cmds. Next bring-up restores video mode in mipi_dsi_clock_set(). */
    mipi_dsi_video_mode_set(false);

    if (priv->panel->off_cmds == NULL) {
        return BK_OK;
    }

    for (uint32_t i = 0; priv->panel->off_cmds[i].cmd != 0 || priv->panel->off_cmds[i].data != NULL; i++) {
        if (priv->panel->off_cmds[i].cmd == 0 && priv->panel->off_cmds[i].data == NULL) {
            break;
        }
        if (priv->panel->off_cmds[i].cmd == 0 && priv->panel->off_cmds[i].data_len == 0xFF
            && priv->panel->off_cmds[i].data != NULL) {
            rtos_delay_milliseconds(((const uint8_t *)priv->panel->off_cmds[i].data)[0]);
            continue;
        }
        AVDK_RETURN_ON_ERROR(bk_display_bus_tx_param(priv->bus_handle,
                                                    (int)priv->panel->off_cmds[i].cmd,
                                                    priv->panel->off_cmds[i].data,
                                                    priv->panel->off_cmds[i].data_len),
                             TAG, "send off command failed");
    }

    return BK_OK;
}

bk_err_t bk_lcd_mipi_default_reset(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    if (priv->reset_gpio < 0) {
        return BK_OK;
    }

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

static bk_err_t lcd_panel_common_init(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    /* clock_config.clk_src is in/out: bus may downgrade DPHY_DPLL->SYSCLK on
     * PLL miss. Cache the latched choice for the DPU controller. */
    bk_panel_clock_config_t clock_config = {
        .n_lanes = priv->panel->n_lanes,
        .fps     = priv->panel->fps,
        .timing  = priv->panel->timing,
    };
    AVDK_RETURN_ON_ERROR(bk_display_bus_set_clock(priv->bus_handle, &clock_config), TAG, "set clock failed");
    priv->base.clk_src = clock_config.clk_src;

    if (priv->panel->init == NULL) {
        LOGI("%s %s: init is NULL, skip\n", __func__, priv->panel->name);
        return BK_OK;
    }
    return priv->panel->init(panel);
}

static bk_err_t lcd_panel_common_reset(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    if (priv->panel->reset == NULL) {
        LOGI("%s %s: reset is NULL, skip\n", __func__, priv->panel->name);
        return BK_OK;
    }
    return priv->panel->reset(panel);
}

static bk_err_t lcd_panel_common_off(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv && priv->panel, BK_ERR_NULL_PARAM, TAG, "invalid panel");

    bk_err_t ret = BK_OK;
    if (priv->panel->off != NULL) {
        ret = priv->panel->off(panel);
    } else {
        LOGI("%s %s: off is NULL, skip cmds\n", __func__, priv->panel->name);
    }

    /* Assert RESETn to its active level and hold it there (no pulse, no release)
     * so the panel is kept in reset before the caller cuts VDDIO -- avoids driving
     * the reset pin above the removed supply. Polarity follows reset_active_level
     * (active-low panels are held low). The next bring-up re-pulses via
     * bk_lcd_mipi_default_reset(). Done unconditionally: a power-down concern, not
     * gated on off_cmds / .off. */
    if (priv->reset_gpio >= 0) {
        BK_LOG_ON_ERR(bk_gpio_enable_output(priv->reset_gpio));
        if (priv->reset_active_level) {
            bk_gpio_set_output_high(priv->reset_gpio);
        } else {
            bk_gpio_set_output_low(priv->reset_gpio);
        }
    }

    return ret;
}

static bk_err_t lcd_panel_common_read_id(bk_avdk_lcd_panel_t *panel, uint32_t *id)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    uint8_t id_buf[3] = {0};
    uint8_t reg_count = 0;
    uint8_t read_bytes;
    bk_err_t ret;

    AVDK_RETURN_ON_FALSE(priv && priv->panel && id, BK_ERR_NULL_PARAM, TAG, "invalid arguments");

    if (priv->panel->read_id_regs == NULL) {
        return BK_ERR_NOT_SUPPORT;
    }

    while (reg_count < 3 && priv->panel->read_id_regs[reg_count] != 0) {
        reg_count++;
    }
    if (reg_count == 0) {
        return BK_ERR_NOT_SUPPORT;
    }

    read_bytes = priv->panel->read_id_bytes;
    if (read_bytes == 0) {
        read_bytes = reg_count;
    }
    if (read_bytes == 0 || read_bytes > 3) {
        return BK_ERR_NOT_SUPPORT;
    }

    if (reg_count == 1) {
        ret = bk_display_bus_rx_param(priv->bus_handle,
                                      (int)priv->panel->read_id_regs[0],
                                      id_buf, read_bytes);
        if (ret != BK_OK) {
            return ret;
        }
    } else {
        if (reg_count != read_bytes) {
            return BK_ERR_NOT_SUPPORT;
        }
        for (int i = 0; i < reg_count; i++) {
            ret = bk_display_bus_rx_param(priv->bus_handle,
                                          (int)priv->panel->read_id_regs[i],
                                          &id_buf[i], 1);
            if (ret != BK_OK) {
                return ret;
            }
        }
    }

    if (read_bytes == 1) {
        *id = id_buf[0];
    } else if (read_bytes == 2) {
        *id = (id_buf[0] << 8) | id_buf[1];
    } else {
        *id = (id_buf[0] << 16) | (id_buf[1] << 8) | id_buf[2];
    }

    return BK_OK;
}

static bk_err_t lcd_panel_common_del(bk_avdk_lcd_panel_t *panel)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    if (priv) {
        os_free(priv);
    }
    return BK_OK;
}

static bk_err_t lcd_panel_common_tx_param(bk_avdk_lcd_panel_t *panel,
                                          int lcd_cmd,
                                          const void *param,
                                          size_t param_size)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");
    return bk_display_bus_tx_param(priv->bus_handle, lcd_cmd, param, param_size);
}

static bk_err_t lcd_panel_common_rx_param(bk_avdk_lcd_panel_t *panel,
                                          int lcd_cmd,
                                          void *param,
                                          size_t param_size)
{
    lcd_panel_common_t *priv = (lcd_panel_common_t *)panel;
    AVDK_RETURN_ON_FALSE(priv, BK_ERR_NULL_PARAM, TAG, "invalid panel");
    return bk_display_bus_rx_param(priv->bus_handle, lcd_cmd, param, param_size);
}

bk_err_t bk_lcd_new_mipi_panel_common(bk_display_bus_handle_t bus_handle,
                                      const bk_lcd_panel_config_t *panel_config,
                                      const bk_display_dsi_panel_t *panel_desc,
                                      bk_avdk_lcd_panel_handle_t *ret_panel)
{
    AVDK_RETURN_ON_FALSE(bus_handle && panel_config && panel_desc && ret_panel,
                         BK_ERR_NULL_PARAM, TAG, "invalid arguments");
    AVDK_RETURN_ON_FALSE(panel_desc->name != NULL, BK_ERR_NULL_PARAM, TAG, "panel name is NULL");
    AVDK_RETURN_ON_FALSE(panel_desc->fps != 0U, BK_ERR_PARAM, TAG,
                         "panel %s descriptor .fps must be non-zero", panel_desc->name);

    lcd_panel_common_t *panel = os_malloc(sizeof(lcd_panel_common_t));
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NO_MEM, TAG, "malloc failed");

    os_memset(panel, 0, sizeof(lcd_panel_common_t));

    panel->bus_handle = bus_handle;
    panel->panel = panel_desc;
    panel->reset_gpio = panel_config->reset_pin;
    panel->reset_active_level = panel_desc->reset_active_level;
    panel->reset_timing.idle_ms    = lcd_panel_common_pick_ms(panel_desc->reset_timing.idle_ms,
                                                              BK_DISPLAY_RESET_IDLE_MS_DEFAULT);
    panel->reset_timing.active_ms  = lcd_panel_common_pick_ms(panel_desc->reset_timing.active_ms,
                                                              BK_DISPLAY_RESET_ACTIVE_MS_DEFAULT);
    panel->reset_timing.release_ms = lcd_panel_common_pick_ms(panel_desc->reset_timing.release_ms,
                                                              BK_DISPLAY_RESET_RELEASE_MS_DEFAULT);

    const uint32_t h_total = (uint32_t)panel_desc->timing.h_size
                           + (uint32_t)panel_desc->timing.hsync_pulse_width
                           + (uint32_t)panel_desc->timing.hsync_back_porch
                           + (uint32_t)panel_desc->timing.hsync_front_porch;
    const uint32_t v_total = (uint32_t)panel_desc->timing.v_size
                           + (uint32_t)panel_desc->timing.vsync_pulse_width
                           + (uint32_t)panel_desc->timing.vsync_back_porch
                           + (uint32_t)panel_desc->timing.vsync_front_porch;
    panel->base.bus                = bus_handle;
    panel->base.timing             = panel_desc->timing;
    panel->base.pixel_clock_hz     = h_total * v_total * (uint32_t)panel_desc->fps;
    /* clk_src is latched by lcd_panel_common_init() (defaults to bus state). */

    panel->base.init               = lcd_panel_common_init;
    panel->base.reset              = lcd_panel_common_reset;
    panel->base.off                = lcd_panel_common_off;
    panel->base.read_id            = lcd_panel_common_read_id;
    panel->base.del                = lcd_panel_common_del;
    panel->base.tx_param           = lcd_panel_common_tx_param;
    panel->base.rx_param           = lcd_panel_common_rx_param;

    *ret_panel = (bk_avdk_lcd_panel_handle_t)&panel->base;
    return BK_OK;
}
