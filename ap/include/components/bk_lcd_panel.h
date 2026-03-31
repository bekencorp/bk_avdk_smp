/*
 * SPDX-FileCopyrightText: 2021-2023 bkressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "components/bk_lcd_types.h"
#include "components/bk_display_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reset LCD panel
 *
 * @note Panel reset must be called before attempting to initialize the panel using `bk_lcd_panel_init()`.
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_panel_reset(bk_avdk_lcd_panel_handle_t panel);

/**
 * @brief Initialize LCD panel
 *
 * @note Before calling this function, make sure the LCD panel has finished the `reset` stage by `bk_lcd_panel_reset()`.
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_panel_init(bk_avdk_lcd_panel_handle_t panel);

/**
 * @brief Deinitialize the LCD panel
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_panel_del(bk_avdk_lcd_panel_handle_t panel);

/**
 * @brief Deinitialize the LCD panel
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_panel_deinit(bk_avdk_lcd_panel_handle_t panel);

/**
 * @brief Draw bitmap on LCD panel
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @param[in] x_start Start pixel index in the target frame buffer, on x-axis (x_start is included)
 * @param[in] y_start Start pixel index in the target frame buffer, on y-axis (y_start is included)
 * @param[in] x_end End pixel index in the target frame buffer, on x-axis (x_end is not included)
 * @param[in] y_end End pixel index in the target frame buffer, on y-axis (y_end is not included)
 * @param[in] color_data RGB color data that will be dumped to the specific window range
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_panel_draw_bitmap(bk_avdk_lcd_panel_handle_t panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);

/**
 * @brief Mirror the LCD panel on specific axis
 *
 * @note Combined with `bk_lcd_panel_swap_xy()`, one can realize screen rotation
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @param[in] mirror_x Whether the panel will be mirrored about the x axis
 * @param[in] mirror_y Whether the panel will be mirrored about the y axis
 * @return
 *          - BK_OK on success
 *          - BK_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
bk_err_t bk_lcd_panel_mirror(bk_avdk_lcd_panel_handle_t panel, bool mirror_x, bool mirror_y);

/**
 * @brief Swap/Exchange x and y axis
 *
 * @note Combined with `bk_lcd_panel_mirror()`, one can realize screen rotation
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `esp_lcd_new_panel_st7789()`
 * @param[in] swap_axes Whether to swap the x and y axis
 * @return
 *          - BK_OK on success
 *          - BK_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
bk_err_t bk_lcd_panel_swap_xy(bk_avdk_lcd_panel_handle_t panel, bool swap_axes);

/**
 * @brief Set extra gap in x and y axis
 *
 * The gap is the space (in pixels) between the left/top sides of the LCD panel and the first row/column respectively of the actual contents displayed.
 *
 * @note Setting a gap is useful when positioning or centering a frame that is smaller than the LCD.
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @param[in] x_gap Extra gap on x axis, in pixels
 * @param[in] y_gap Extra gap on y axis, in pixels
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_panel_set_gap(bk_avdk_lcd_panel_handle_t panel, int x_gap, int y_gap);

/**
 * @brief Invert the color (bit-wise invert the color data line)
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @param[in] invert_color_data Whether to invert the color data
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_panel_invert_color(bk_avdk_lcd_panel_handle_t panel, bool invert_color_data);

/**
 * @brief Turn on or off the display
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @param[in] on_off True to turns on display, False to turns off display
 * @return
 *          - BK_OK on success
 *          - BK_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
bk_err_t bk_lcd_panel_disp_on_off(bk_avdk_lcd_panel_handle_t panel, bool on_off);

/**
 * @brief Enter or exit sleep mode
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @param[in] sleep True to enter sleep mode, False to wake up
 * @return
 *          - BK_OK on success
 *          - BK_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
bk_err_t bk_lcd_panel_disp_sleep(bk_avdk_lcd_panel_handle_t panel, bool sleep);

/**
 * @brief Read LCD panel IC id
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @param[in] id LCD panel IC id
 * @return
 *          - BK_OK on success
 *          - BK_ERR_NOT_SUPPORTED if this function is not supported by the panel
 */
bk_err_t bk_lcd_panel_read_id(bk_avdk_lcd_panel_handle_t panel, uint32_t* id);

/**
 * @brief Get LCD panel clocks
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @param[out] timing LCD panel timing
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_panel_get_disp_timing(bk_avdk_lcd_panel_handle_t panel, bk_display_timing_t *timing);
/**
 * @brief Create a new MIPI panel
 *
 * @param[in] bus_handle Display bus handle
 * @param[in] panel_dev_config Panel device configuration
 * @param[in] panel_desc Panel descriptor
 * @param[out] ret_panel Returned panel handle
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_mipi_panel_new(bk_display_bus_handle_t bus_handle,
    const bk_lcd_panel_dev_config_t *panel_dev_config,
    const bk_display_dsi_panel_t *panel_desc,
    bk_avdk_lcd_panel_handle_t *ret_panel);


/**
 * @brief Create a new RGB panel
 *
 * @param[in] bus_handle Display bus handle
 * @param[in] panel_dev_config Panel device configuration
 * @param[in] panel_desc Panel descriptor
 * @param[out] ret_panel Returned panel handle
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_rgb_panel_new(bk_display_bus_handle_t bus_handle,
        const bk_lcd_panel_dev_config_t *panel_dev_config,
        const bk_display_rgb_panel_t *panel_desc,
        bk_avdk_lcd_panel_handle_t *ret_panel);

/**
 * @brief Get list of MIPI panels
 *
 * @param[out] panels Array of panel pointers
 * @param[in] max_count Maximum number of panels to return
 * @return Number of panels found
 */
uint32_t bk_lcd_get_mipi_panel_list(const bk_display_dsi_panel_t **panels, uint32_t max_count);

/**
 * @brief Get list of RGB panels
 *
 * @param[out] panels Array of panel pointers
 * @param[in] max_count Maximum number of panels to return
 * @return Number of panels found
 */
uint32_t bk_lcd_get_rgb_panel_list(const bk_display_rgb_panel_t **panels, uint32_t max_count);

/**
 * @brief Find MIPI panel by name
 *
 * @param[in] name Panel name
 * @return Panel pointer or NULL if not found
 */
const bk_display_dsi_panel_t *bk_lcd_find_mipi_panel_by_name(const char *name);

/**
 * @brief Find RGB panel by name
 *
 * @param[in] name Panel name
 * @return Panel pointer or NULL if not found
 */
const bk_display_rgb_panel_t *bk_lcd_find_rgb_panel_by_name(const char *name);

#ifdef __cplusplus
}
#endif
