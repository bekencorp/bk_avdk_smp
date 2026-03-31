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
    AVDK_RETURN_ON_FALSE(panel->del, BK_ERR_NOT_SUPPORT, TAG, "reset is not supported by this panel");
    return panel->del(panel);
}


bk_err_t bk_lcd_panel_mirror(bk_avdk_lcd_panel_handle_t panel, bool mirror_x, bool mirror_y)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->mirror, BK_ERR_NOT_SUPPORT, TAG, "mirror is not supported by this panel");
    return panel->mirror(panel, mirror_x, mirror_y);
}

bk_err_t bk_lcd_panel_swap_xy(bk_avdk_lcd_panel_handle_t panel, bool swap_axes)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->swap_xy, BK_ERR_NOT_SUPPORT, TAG, "swap_xy is not supported by this panel");
    return panel->swap_xy(panel, swap_axes);
}

bk_err_t bk_lcd_panel_set_gap(bk_avdk_lcd_panel_handle_t panel, int x_gap, int y_gap)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->set_gap, BK_ERR_NOT_SUPPORT, TAG, "set_gap is not supported by this panel");
    return panel->set_gap(panel, x_gap, y_gap);
}

bk_err_t bk_lcd_panel_invert_color(bk_avdk_lcd_panel_handle_t panel, bool invert_color_data)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->invert_color, BK_ERR_NOT_SUPPORT, TAG, "invert_color is not supported by this panel");
    return panel->invert_color(panel, invert_color_data);
}

bk_err_t bk_lcd_panel_disp_on_off(bk_avdk_lcd_panel_handle_t panel, bool on_off)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->disp_on_off, BK_ERR_NOT_SUPPORT, TAG, "disp_on_off is not supported by this panel");
    return panel->disp_on_off(panel, on_off);
}

bk_err_t bk_lcd_panel_disp_off(bk_avdk_lcd_panel_handle_t panel, bool off)
{
    return bk_lcd_panel_disp_on_off(panel, !off);
}

bk_err_t bk_lcd_panel_disp_sleep(bk_avdk_lcd_panel_handle_t panel, bool sleep)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->disp_sleep, BK_ERR_NOT_SUPPORT, TAG, "sleep is not supported by this panel");
    return panel->disp_sleep(panel, sleep);
}

bk_err_t bk_lcd_panel_read_id(bk_avdk_lcd_panel_handle_t panel, uint32_t* id)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid panel handle");
    AVDK_RETURN_ON_FALSE(panel->read_id, BK_ERR_NOT_SUPPORT, TAG, "sleep is not supported by this panel");
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
