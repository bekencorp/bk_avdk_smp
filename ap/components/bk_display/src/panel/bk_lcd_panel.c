/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <avdk_check.h>
#include "components/bk_lcd_types.h"
#include "components/bk_lcd_panel.h"
#include "display_dsi_bus_vn_ctlr.h"

#define TAG  "lcd_panel"

bk_err_t bk_lcd_panel_reset(bk_avdk_lcd_panel_handle_t panel)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->reset, BK_ERR_NOT_SUPPORT, TAG, "reset is not supported by this panel");
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
    AVDK_RETURN_ON_FALSE(panel->del, BK_ERR_NOT_SUPPORT, TAG, "del is not supported by this panel");
    return panel->del(panel);
}

bk_err_t bk_lcd_panel_read_id(bk_avdk_lcd_panel_handle_t panel, uint32_t* id)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->read_id, BK_ERR_NOT_SUPPORT, TAG, "read_id is not supported by this panel");
    return panel->read_id(panel, id);
}

bk_err_t bk_lcd_panel_get_disp_timing(bk_avdk_lcd_panel_handle_t panel, bk_display_timing_t *timing)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->get_disp_timing, BK_ERR_NOT_SUPPORT, TAG, "get_disp_timing is not supported by this panel");
    return panel->get_disp_timing(panel, timing);
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
