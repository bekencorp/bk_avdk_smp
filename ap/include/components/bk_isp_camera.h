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

#include <components/bk_isp_camera_types.h>
#include <components/bk_camera_isp_ctlr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize ISP camera controller device-level resources
 * @param handle ISP camera controller handle from bk_camera_isp_ctlr_new
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_dev_init(bk_isp_camera_ctlr_handle_t handle);

/**
 * @brief Configure the active ISP port (DVP or CSI)
 * @param handle ISP camera controller handle
 * @param config Pointer to bk_isp_camera_ctlr_config_t
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_port_init(bk_isp_camera_ctlr_handle_t handle, void *config);

avdk_err_t bk_isp_camera_port_select(bk_isp_camera_ctlr_handle_t handle, uint8_t port_id);

/**
 * @brief Switch or update the active ISP port configuration
 * @param handle ISP camera controller handle
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_port_change(bk_isp_camera_ctlr_handle_t handle);

/**
 * @brief Deinitialize ISP camera controller and release device-level resources
 * @param handle ISP camera controller handle
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_deinit(bk_isp_camera_ctlr_handle_t handle);

/**
 * @brief Start ISP camera streaming
 * @param handle ISP camera controller handle
 * @param parameter Optional open parameters (implementation-specific, may be NULL)
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_open(bk_isp_camera_ctlr_handle_t handle, void *parameter);

/**
 * @brief Stop ISP camera streaming
 * @param handle ISP camera controller handle
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_close(bk_isp_camera_ctlr_handle_t handle);

/**
 * @brief Read one frame from the specified channel
 * @param handle ISP camera controller handle
 * @param id Channel identifier
 * @param frame Output buffer for frame data
 * @param size Size of the output buffer in bytes
 * @param timeout Read timeout in milliseconds
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_read(bk_isp_camera_ctlr_handle_t handle, uint16_t id, uint8_t *frame, uint32_t size, uint32_t timeout);

avdk_err_t bk_isp_camera_multi_port_read(
    bk_isp_camera_ctlr_handle_t handle,
    const multi_port_read_param_t *param,
    multi_port_read_result_t *result);

avdk_err_t bk_isp_camera_vc_mux_start(bk_isp_camera_vc_mux_handle_t handle, bk_isp_camera_vc_mux_config_t *config);

avdk_err_t bk_isp_camera_vc_mux_stop(bk_isp_camera_vc_mux_handle_t handle);

avdk_err_t bk_isp_camera_vc_mux_vc_enable(bk_isp_camera_vc_mux_handle_t handle, uint8_t vc, uint8_t discard_frames);

avdk_err_t bk_isp_camera_vc_mux_vc_disable(bk_isp_camera_vc_mux_handle_t handle, uint8_t vc);

avdk_err_t bk_isp_camera_vc_mux_peek(bk_isp_camera_vc_mux_handle_t handle, bk_isp_camera_vc_mux_frame_ref_t *frame);

avdk_err_t bk_isp_camera_vc_mux_release(bk_isp_camera_vc_mux_handle_t handle, bk_isp_camera_vc_mux_frame_ref_t *frame);

/**
 * @brief Destroy the ISP camera VC mux controller instance
 * @param handle VC mux handle from bk_camera_isp_vc_mux_new
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_vc_mux_delete(bk_isp_camera_vc_mux_handle_t handle);

/**
 * @brief Register an ISP interrupt callback
 * @param handle ISP camera controller handle
 * @param type ISR event type
 * @param cb Callback function
 * @param arg User argument passed to the callback
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_register_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, bk_camera_isr_t cb, void *arg);

/**
 * @brief Deregister an ISP interrupt callback
 * @param handle ISP camera controller handle
 * @param type ISR event type
 * @param arg User argument that was registered with the callback
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_deregister_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, void *arg);

/**
 * @brief Issue an IOCTL command on the ISP camera controller
 * @param handle ISP camera controller handle
 * @param ioctl IOCTL command identifier
 * @param arg Command-specific argument
 * @return AVDK error code
 *
 * White balance (BK_CAM_IOCTL_GET/SET_WB) and exposure
 * (BK_CAM_IOCTL_GET/SET_EXPOSURE) notes:
 * - Call them only after bk_isp_camera_port_init(). That step loads the sensor
 *   tuning data, which overwrites the whole WB and exposure attribute set, so
 *   anything set earlier is silently lost. Re-apply after a port re-init.
 * - Both act on the currently selected logical ISP port; use
 *   BK_CAM_IOCTL_SELECT_ISP_PORT first on multi-port setups.
 * - Prefer get-modify-set so unrelated fields keep their current value.
 * - Manual values take effect on the next 3A interrupt while streaming, so
 *   expect a delay of one to three frames.
 * - Out-of-range gains and exposure times are clamped by the ISP firmware with
 *   a warning rather than rejected; read back if the exact value matters.
 * - On a dual MIPI logical port setup, BK_CAM_IOCTL_RESTORE_ISP_PORT_CONTEXT
 *   forces AE back to auto, so manual exposure must be re-applied after a port
 *   switch. White balance is unaffected.
 */
avdk_err_t bk_isp_camera_ctlr_ioctl(bk_isp_camera_ctlr_handle_t handle, bk_cam_interface_ioctl_t ioctl, void *arg);

/**
 * @brief Destroy the ISP camera controller instance
 * @param handle ISP camera controller handle
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_delete(bk_isp_camera_ctlr_handle_t handle);

/**
 * @brief Open an ISP output channel
 * @param handle ISP camera controller handle
 * @param channel Channel index
 * @param config Channel configuration
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_channel_open(bk_isp_camera_ctlr_handle_t handle, uint8_t channel, bk_isp_camera_channel_config_t *config);

/**
 * @brief Close an ISP output channel
 * @param handle ISP camera controller handle
 * @param channel Channel index
 * @return AVDK error code
 */
avdk_err_t bk_isp_camera_channel_close(bk_isp_camera_ctlr_handle_t handle, uint8_t channel);

/**
 * @brief Get the state of an ISP output channel
 * @param handle ISP camera controller handle
 * @param channel Channel index
 * @return Channel state
 */
bk_isp_camera_channel_state_t bk_isp_camera_channel_state_get(bk_isp_camera_ctlr_handle_t handle, uint8_t channel);

#ifdef __cplusplus
}
#endif
