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
