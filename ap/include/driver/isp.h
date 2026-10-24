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

#include <driver/isp_types.h>
#include "isp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_isp_dev_init(isp_handle_t *handle);

bk_err_t bk_isp_port_init(isp_handle_t *handle, void *sensor_attr);

/**
 * @brief Select an initialized ISP port.
 *
 * Logical MIPI switching only publishes the software port identity. It does
 * not restart sensor streaming, reset AE to auto, or synchronously restore
 * all hardware registers. Applications manage streaming and explicitly set
 * the exposure mode when auto exposure is required.
 */
bk_err_t bk_isp_port_select(isp_handle_t *handle, uint8_t port_id);

bk_err_t bk_isp_port_change(isp_handle_t *handle);

bk_err_t bk_isp_deinit(isp_handle_t *handle);

bk_err_t bk_isp_open(isp_handle_t *handle, isp_config_ext_t *config);

bk_err_t bk_isp_close(isp_handle_t *handle, uint8_t chnl_id);

/**
 * @brief Abort a pending DQBUF on a channel so a blocked reader wakes immediately.
 *
 * Performs a buffer-queue StreamOff only: a reader blocked in pop_buf returns at
 * once (BK_FAIL / not-ready) so the caller can join its reader thread promptly.
 * Does NOT stop the MI DMA or free buffers; call bk_isp_close afterwards to do the
 * full channel teardown. Safe no-op on an already-closed channel.
 */
bk_err_t bk_isp_dqbuf_abort(isp_handle_t *handle, uint8_t chnl_id);

bk_err_t bk_isp_register_isr_callback(isp_handle_t *handle, isp_isr_type_t type, isp_isr_t cb, void *arg);

bk_err_t bk_isp_deregister_isr_callback(isp_handle_t *handle, isp_isr_type_t type, void *arg);

bk_err_t bk_isp_frame_port_get(isp_handle_t *handle,
                               uint8_t chnl_id,
                               uint32_t sequence,
                               uint8_t *port_id);

/**
 * @brief Register a software pipeline-start callback in isp_core after frame-end.
 *
 * 3A start/done registration and unregistration require task context;
 * ISR callers receive BK_ERR_STATE. Registering the same cb/arg is idempotent.
 * A conflicting registration or an unregister in progress returns BK_ERR_BUSY.
 * Callers must serialize device DeInit with registration/unregistration and
 * keep the handle valid.
 */
bk_err_t bk_isp_register_3a_start_callback(isp_handle_t *handle,
                                         isp_3a_start_cb_t cb,
                                         void *arg);

/**
 * @brief Unregister a start callback and wait for its in-flight calls to finish.
 *
 * Do not hold locks needed by the callback while waiting. Self-unregistration
 * and concurrent unregistration return BK_ERR_BUSY; a cb/arg mismatch returns
 * BK_ERR_STATE. Only BK_OK permits releasing arg. On failure, retain arg and
 * retry from an external task.
 */
bk_err_t bk_isp_deregister_3a_start_callback(isp_handle_t *handle,
                                           isp_3a_start_cb_t cb,
                                           void *arg);

/** @brief Register a pipeline-done callback; same constraints as start registration. */
bk_err_t bk_isp_register_3a_done_callback(isp_handle_t *handle,
                                         isp_3a_done_cb_t cb,
                                         void *arg);

/** @brief Unregister and drain a done callback; same contract as start unregistration. */
bk_err_t bk_isp_deregister_3a_done_callback(isp_handle_t *handle,
                                           isp_3a_done_cb_t cb,
                                           void *arg);

bool bk_isp_is_in_vblank(isp_handle_t *handle);

bk_err_t bk_isp_sensor_update_mode_set(
    isp_handle_t *handle, bk_isp_sensor_update_mode_t mode);

bk_err_t bk_isp_pipeline_switch_set(
    isp_handle_t *handle, uint8_t enable);

/**
 * @brief Vote to enable/disable ISP (CISP) clock.
 *
 * Shared by ISP driver and camera bus. Clock is powered up on the first
 * enable vote and powered down only when all enable votes are released.
 *
 * @param enable 1 to take a vote, 0 to release a vote
 */
bk_err_t bk_isp_clock_enable(uint8_t enable);

bk_err_t bk_cis_auxs_clock_enable(uint32_t clk, uint32_t gpio, uint8_t enable);

bk_err_t bk_cis_mclk_clock_enable(uint32_t clk, uint8_t gpio, uint8_t enable);

bk_err_t bk_isp_flexa_sbi_config(isp_handle_t *handle, uint8_t chnl, uint8_t enable);

/**
 * @brief Compute the flexa ring buffer size for a channel geometry.
 *
 * Single source of the size formula used by the internal allocation, so an
 * externally allocated buffer (bk_isp_flexa_buffer_set) is always sized to
 * match what bk_isp_open would have requested.
 *
 * @param buf_cnt flexa ring slice count planned for channel open
 * @param width channel width in pixels planned for channel open
 * @param size output: required buffer size in bytes
 */
bk_err_t bk_isp_flexa_buffer_size_get(uint8_t buf_cnt, uint16_t width, uint32_t *size);

/**
 * @brief Provide an externally allocated flexa ring buffer for a channel.
 *
 * Must be called before the channel is opened. Once set, bk_isp_open uses this
 * buffer instead of allocating internally and never frees it; the caller keeps
 * ownership and is responsible for freeing it after the channel is closed.
 */
bk_err_t bk_isp_flexa_buffer_set(isp_handle_t *handle, uint8_t chnl, void *buf, uint32_t size);

bk_err_t bk_isp_soft_reset(isp_handle_t *handle);

bk_err_t bk_isp_get_exposure_luminance(isp_handle_t *handle, uint32_t *luminance);

bk_err_t bk_isp_get_cproc_attr(isp_handle_t *handle, void *cproc_attr);

bk_err_t bk_isp_set_cproc_attr(isp_handle_t *handle, void *cproc_attr);

/**
 * @brief Read the gamma-out attributes of the active ISP port.
 *
 * @param attr 962 bytes, must not live on a task stack.
 */
bk_err_t bk_isp_get_gamma_out_attr(isp_handle_t *handle, bk_isp_gamma_out_attr_t *attr);

/**
 * @brief Write the gamma-out attributes of the active ISP port.
 *
 * Must be called after bk_isp_port_init(), which loads the sensor tuning data
 * and overwrites the whole gamma-out curve. Works while the sensor is
 * streaming; the new curve is latched on the next frame.
 *
 * @param attr Not const on purpose: out-of-range nodes are clamped in place, so
 *             on return attr holds exactly what the ISP programmed. 962 bytes,
 *             must not live on a task stack.
 */
bk_err_t bk_isp_set_gamma_out_attr(isp_handle_t *handle, bk_isp_gamma_out_attr_t *attr);

bk_err_t bk_isp_query_exposure_info(isp_handle_t *handle,
                                    bk_isp_exposure_info_t *info);

bk_err_t bk_isp_set_initial_exposure(
    isp_handle_t *handle, const bk_isp_exposure_info_t *info);

bk_err_t bk_isp_resume_auto_exposure(isp_handle_t *handle);

bk_err_t bk_isp_get_wb_attr(isp_handle_t *handle, bk_isp_wb_attr_t *attr);

/**
 * @brief Set the white balance mode and manual gains on the active ISP port.
 *
 * Must be called after bk_isp_port_init(), which loads the sensor tuning data
 * and overwrites the whole WB attribute set.
 */
bk_err_t bk_isp_set_wb_attr(isp_handle_t *handle, const bk_isp_wb_attr_t *attr);

bk_err_t bk_isp_get_exposure_attr(isp_handle_t *handle, bk_isp_exposure_attr_t *attr);

/**
 * @brief Set the exposure mode and manual exposure on the active ISP port.
 *
 * Unlike bk_isp_set_initial_exposure(), this works while the sensor is
 * streaming; the new values reach the sensor within a few frames. Must be
 * called after bk_isp_port_init().
 */
bk_err_t bk_isp_set_exposure_attr(isp_handle_t *handle, const bk_isp_exposure_attr_t *attr);

#ifdef __cplusplus
}
#endif
