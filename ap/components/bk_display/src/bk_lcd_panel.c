/*
 * SPDX-FileCopyrightText: 2021-2024 Beken Corp.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Public panel API dispatcher.
 *
 *  - bk_lcd_panel_*()      :: dispatch into the panel ops table built
 *                             by panel-common (lcd_mipi/rgb_panel_common.c).
 *  - bk_lcd_*_panel_new()  :: thin shims on top of panel-common factories.
 *
 * Bus-side command-channel dispatchers live in bk_display_bus.c.
 */

#include <avdk_check.h>
#include <components/log.h>
#include <components/bk_lcd_panel.h>
#include "bk_lcd_panel_priv.h"
#include "display_dsi_bus_vn_ctlr.h"

#define TAG "lcd_panel"

bk_err_t bk_lcd_panel_reset(bk_avdk_lcd_panel_handle_t panel)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->reset, BK_ERR_NOT_SUPPORT, TAG, "reset not supported");
    return panel->reset(panel);
}

bk_err_t bk_lcd_panel_init(bk_avdk_lcd_panel_handle_t panel)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    return panel->init(panel);
}

bk_err_t bk_lcd_panel_del(bk_avdk_lcd_panel_handle_t panel)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->del, BK_ERR_NOT_SUPPORT, TAG, "del not supported");
    return panel->del(panel);
}

bk_err_t bk_lcd_panel_disp_on_off(bk_avdk_lcd_panel_handle_t panel, bool on_off)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->disp_on_off, BK_ERR_NOT_SUPPORT, TAG, "disp_on_off not supported");
    return panel->disp_on_off(panel, on_off);
}

bk_err_t bk_lcd_panel_read_id(bk_avdk_lcd_panel_handle_t panel, uint32_t *id)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->read_id, BK_ERR_NOT_SUPPORT, TAG, "read_id not supported");
    return panel->read_id(panel, id);
}

bk_err_t bk_lcd_panel_tx_param(bk_avdk_lcd_panel_handle_t panel,
                               int lcd_cmd,
                               const void *param,
                               size_t param_size)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->tx_param, BK_ERR_NOT_SUPPORT, TAG, "tx_param not supported");
    return panel->tx_param(panel, lcd_cmd, param, param_size);
}

bk_err_t bk_lcd_panel_rx_param(bk_avdk_lcd_panel_handle_t panel,
                               int lcd_cmd,
                               void *param,
                               size_t param_size)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(param, BK_ERR_NULL_PARAM, TAG, "invalid param buffer");
    AVDK_RETURN_ON_FALSE(panel->rx_param, BK_ERR_NOT_SUPPORT, TAG, "rx_param not supported");
    return panel->rx_param(panel, lcd_cmd, param, param_size);
}

bk_err_t bk_lcd_mipi_panel_new(bk_display_bus_handle_t bus_handle,
                               const bk_lcd_panel_dev_config_t *panel_dev_config,
                               const bk_display_dsi_panel_t *panel,
                               bk_avdk_lcd_panel_handle_t *ret_panel)
{
    AVDK_RETURN_ON_FALSE(panel != NULL, BK_ERR_NULL_PARAM, TAG, "panel is NULL");
    return bk_lcd_new_mipi_panel_common(bus_handle, panel_dev_config, panel, ret_panel);
}

bk_err_t bk_lcd_rgb_panel_new(bk_display_bus_handle_t bus_handle,
                              const bk_lcd_panel_dev_config_t *panel_dev_config,
                              const bk_display_rgb_panel_t *panel,
                              bk_avdk_lcd_panel_handle_t *ret_panel)
{
    AVDK_RETURN_ON_FALSE(panel != NULL, BK_ERR_NULL_PARAM, TAG, "panel is NULL");
    return bk_lcd_new_rgb_panel_common(bus_handle, panel_dev_config, panel, ret_panel);
}
