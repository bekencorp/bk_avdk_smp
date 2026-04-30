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
 * @brief Destroy/free the LCD panel
 *
 * @param[in] panel LCD panel handle, which is created by other factory API like `bk_lcd_new_panel_st7789()`
 * @return
 *          - BK_OK on success
 */
bk_err_t bk_lcd_panel_del(bk_avdk_lcd_panel_handle_t panel);

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
