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

#pragma once

/**
 * @file bk_display.h
 * @brief LCD display controller domain + umbrella for the bk_display
 *        component. Including this header pulls in the full public
 *        surface (bus + panel + display).
 *
 * Typical bring-up flow:
 *   bk_display_dpu_ctlr_new()  -> handle (DEINIT)
 *   bk_display_init()          -> INITED  (registers programmed)
 *   bk_display_open()          -> ACTIVE  (panel trigger armed)
 *   bk_display_flush()         -> push frames
 *   bk_display_close()         -> CLOSED  (re-openable)
 *   bk_display_deinit()        -> DEINIT
 *   bk_display_delete()        -> free handle
 */

#include <stdint.h>
#include <stdbool.h>
#include <avdk_error.h>
#include <avdk_check.h>
#include <components/bk_lcd_panel.h>
#include <components/bk_display_bus.h>
#include <driver/dpu_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Fixed PSRAM/DPU AXI QoS weight (not user-configurable). */
#define BK_DISPLAY_DPU_QOS_DEFAULT                3


typedef struct
{
    dpu_video_layer_config_t video;   /**< DPU video layer config */
} bk_display_dpu_config_t;

/**
 * @brief Runtime pixel-format reconfiguration payload for ::BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT.
 */
typedef struct
{
    bk_pixel_format_t format;
    bool decompress;
} bk_display_pixel_format_config_t;

/**
 * @brief Display ioctl command codes.
 */
typedef enum {
    BK_DISPLAY_IOCTL_UNKNOWN          = 0,
    BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT = 1,    /**< arg = ::bk_display_pixel_format_config_t* */
} bk_display_ioctl_cmd_t;

/** Opaque display controller handle. Body lives in private_include/. */
typedef struct bk_display_ctlr_t *bk_display_ctlr_handle_t;

/**
 * @brief Create a DPU display controller instance (state = DEINIT).
 *
 * The controller binds to a specific panel and queries the panel's
 * video timing (and, for RGB, the target DPI pixel clock) directly
 * from the panel handle - callers never copy timing or pixel-clock
 * fields into @p config themselves.
 *
 * @param[out] handle  Receives the new controller handle.
 * @param[in]  panel   Panel handle previously created via
 *                     ::bk_lcd_mipi_panel_new() / ::bk_lcd_rgb_panel_new().
 * @param[in]  config  Controller configuration (must remain valid until delete).
 *
 * @return AVDK_ERR_OK on success.
 * @return AVDK_ERR_INVAL if @p handle, @p panel or @p config is NULL.
 * @return AVDK_ERR_NO_MEM if allocation fails.
 */
avdk_err_t bk_display_dpu_ctlr_new(bk_display_ctlr_handle_t *handle,
                                   bk_avdk_lcd_panel_handle_t panel,
                                   const bk_display_dpu_config_t *config);

/**
 * @brief Create a SPI display controller instance (state = DEINIT).
 *
 * This controller owns the HW SPI LCD frame path and exposes it through
 * ::bk_display_flush(). Only ::BK_DISPLAY_SPI_BUS_MODE_HW is supported.
 * Callers must use the normal display lifecycle:
 * ::bk_display_init() -> ::bk_display_open() -> ::bk_display_flush() ->
 * ::bk_display_close() -> ::bk_display_deinit() -> ::bk_display_delete().
 *
 * @param[out] handle  Receives the new controller handle.
 * @param[in]  config  SPI bus configuration copied into the controller.
 *
 * @return AVDK_ERR_OK on success.
 * @return AVDK_ERR_INVAL if @p handle / @p config is NULL or mode is not HW.
 */
avdk_err_t bk_display_spi_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_spi_bus_config_t *config);

/**
 * @brief Bring the controller from DEINIT to INITED.
 *
 * Programs DPU registers and powers the panel; no refresh is started yet.
 *
 * @param[in] handle Display controller.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t bk_display_init(bk_display_ctlr_handle_t handle);

/**
 * @brief Tear DPU + MSP and return the controller to DEINIT.
 * @param[in] handle Display controller.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t bk_display_deinit(bk_display_ctlr_handle_t handle);

/**
 * @brief Move the controller into ACTIVE so ::bk_display_flush() may drive frames.
 * @param[in] handle Display controller.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t bk_display_open(bk_display_ctlr_handle_t handle);

/**
 * @brief Quiesce frame output but keep DPU state for a later ::bk_display_open().
 * @param[in] handle Display controller.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t bk_display_close(bk_display_ctlr_handle_t handle);

/**
 * @brief Release the controller handle.
 *
 * Implicitly calls ::bk_display_deinit() if the controller is still active.
 *
 * @param[in] handle Display controller.
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t bk_display_delete(bk_display_ctlr_handle_t handle);

/**
 * @brief Submit a frame for refresh.
 *
 * @param[in] handle Active display controller.
 * @param[in] frame  Frame buffer pointer.
 * @param[in] cb     Optional callback invoked when @p frame is no longer in use.
 *
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t bk_display_flush(bk_display_ctlr_handle_t handle, void *frame, flush_free_cb_t cb);

/**
 * @brief Issue a runtime control command.
 *
 * @param[in] handle Display controller.
 * @param[in] cmd    Command code, see ::bk_display_ioctl_cmd_t.
 * @param[in] arg    Command-specific argument; refer to each command's doc.
 *
 * @return AVDK_ERR_OK on success.
 * @return AVDK_ERR_UNSUPPORTED if the backend has no ioctl op.
 */
avdk_err_t bk_display_ioctl(bk_display_ctlr_handle_t handle, bk_display_ioctl_cmd_t cmd, void *arg);

/**
 * @brief Convenience wrapper around ::bk_display_ioctl() for pixel format updates.
 *
 * @param[in] handle Display controller.
 * @param[in] config New pixel format / decompress configuration.
 *
 * @return AVDK_ERR_OK on success.
 */
avdk_err_t bk_display_pixel_format_set(bk_display_ctlr_handle_t handle, const bk_display_pixel_format_config_t *config);

#ifdef __cplusplus
}
#endif
