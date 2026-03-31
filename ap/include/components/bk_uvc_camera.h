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

#include <components/bk_uvc_camera_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Create a new UVC controller instance
 *
 * Allocates and initialises an abstract controller object. Callbacks are stored
 * internally and invoked when frames are allocated, released, or when the UVC
 * device changes state.
 *
 * @param[out] handle    Pointer that receives the newly created controller handle
 * @param[in]  callbacks Application provided callback hooks (frame allocation, completion, state changes)
 *
 * @return
 *    - AVDK_ERR_OK: controller created successfully
 *    - others: other errors.
 */
avdk_err_t bk_uvc_ctrl_new(bk_uvc_ctlr_handle_t *handle, const bk_uvc_callback_t *callbacks);

/**
 * @brief     Initialize the UVC controller hardware and resources
 *
 * Must be called before opening a device. The controller configures clocks,
 * endpoints and any other hardware level state needed to communicate with a
 * UVC camera.
 *
 * @param handle Controller handle obtained from bk_uvc_ctrl_new
 *
 * @return
 *    - AVDK_ERR_OK: initialization success
 *    - others: other errors.
 */
avdk_err_t bk_uvc_init(bk_uvc_ctlr_handle_t handle);

/**
 * @brief     Deinitialize the UVC controller and release resources
 *
 * Reverses bk_uvc_init by powering down hardware resources and freeing
 * controller level allocations. The controller can be initialised again after
 * this call.
 *
 * @param handle Controller handle
 *
 * @return
 *    - AVDK_ERR_OK: deinitialisation success
 *    - others: other errors.
 */
avdk_err_t bk_uvc_deinit(bk_uvc_ctlr_handle_t handle);

/**
 * @brief     Open a UVC device using the provided configuration
 *
 * Applies stream parameters such as image format, resolution and frame rate,
 * then arms the controller to start receiving data from the device.
 *
 * @param handle Controller handle
 * @param config Runtime configuration describing format, resolution, FPS, etc.
 *
 * @return
 *    - AVDK_ERR_OK: device opened successfully
 *    - others: other errors.
 */
avdk_err_t bk_uvc_open(bk_uvc_ctlr_handle_t handle, bk_cam_uvc_config_t *config);

/**
 * @brief     Close the currently opened UVC device
 *
 * Stops streaming, flushes pending transfers and releases device specific
 * resources. After calling close, the controller can be opened again with a new
 * configuration.
 *
 * @param handle Controller handle
 *
 * @return
 *    - AVDK_ERR_OK: device closed successfully
 *    - others: other errors.
 */
avdk_err_t bk_uvc_close(bk_uvc_ctlr_handle_t handle);

/**
 * @brief     Suspend the UVC controller (used for low-power management)
 *
 * Places the controller in a low-power state. Streaming must be stopped prior
 * to suspension.
 *
 * @param handle Controller handle
 *
 * @return
 *    - AVDK_ERR_OK: controller suspended successfully
 *    - others: other errors.
 */
avdk_err_t bk_uvc_suspend(bk_uvc_ctlr_handle_t handle);

/**
 * @brief     Resume the UVC controller after a suspend operation
 *
 * Restores controller state saved during suspend so that streaming can be
 * restarted.
 *
 * @param handle Controller handle
 *
 * @return
 *    - AVDK_ERR_OK: controller resumed successfully
 *    - others: other errors.
 */
avdk_err_t bk_uvc_resume(bk_uvc_ctlr_handle_t handle);

/**
 * @brief     Issue a controller-specific IOCTL command
 *
 * Provides access to extended control paths that are not covered by the
 * standard open/close/suspend/resume APIs.
 *
 * @param handle Controller handle
 * @param event IOCTL command identifier
 * @param arg   Optional argument pointer associated with the command
 *
 * @return
 *    - AVDK_ERR_OK: IOCTL executed successfully
 *    - others: other errors.
 */
avdk_err_t bk_uvc_ioctl(bk_uvc_ctlr_handle_t handle, uint32_t event, void *arg);

/**
 * @brief     Destroy a controller instance created by bk_uvc_ctrl_new
 *
 * This API releases the handle and any residual resources created by the
 * controller constructor.
 *
 * @param handle Controller handle
 *
 * @return
 *    - AVDK_ERR_OK: deletion success
 *    - others: other errors.
 */
avdk_err_t bk_uvc_delete(bk_uvc_ctlr_handle_t handle);

/**
 * @brief     register separate packet cb
 *
 * This API will register separate packet data, some uvc device a endpoint will output mjpeg and h264 data meantime, need register this callback
 *
 * @param cb
 *
 * @attation 1. this api called before bk_uvc_init, not all uvc need register
 *
 * @return
 *    - AVDK_ERR_OK: set success
 *    - others: other errors.
 */
avdk_err_t bk_uvc_register_separate_packet_callback(uvc_separate_config_t *cb);


#ifdef __cplusplus
}
#endif
