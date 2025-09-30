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

#include "components/bk_jpeg_decode/bk_jpeg_decode_utils.h"
#include "components/bk_jpeg_decode/bk_jpeg_decode_types_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Hardware JPEG Decoder API
 * 
 * This file provides the external interface for the hardware-based JPEG decoder module.
 * All functions in this file implement JPEG decoding using dedicated hardware
 * rather than CPU-based software algorithms.
 */

/**
 * @brief Create a new hardware JPEG decoder instance
 *
 * This function initializes and creates a new hardware-based JPEG decoder instance
 * that utilizes dedicated hardware for faster JPEG decoding operations.
 * It allocates memory for the decoder controller structure and initializes it with
 * the provided configuration and hardware-specific operation functions.
 *
 * @param handle [out] Pointer to store the hardware decoder handle, which will be used for subsequent operations
 * @param config [in] Hardware decoder configuration parameters
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_hardware_jpeg_decode_new(bk_jpeg_decode_hw_handle_t *handle, bk_jpeg_decode_hw_config_t *config);

/**
 * @brief Open the hardware JPEG decoder
 *
 * This function prepares the hardware JPEG decoder for operation by
 * initializing hardware registers, setting up the decoding environment,
 * and calling the underlying hardware JPEG decoder's initialization function.
 * The decoder must be opened before starting any decoding operations.
 *
 * @param handle [in] Hardware decoder handle obtained from bk_hardware_jpeg_decode_new
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_hw_open(bk_jpeg_decode_hw_handle_t handle);

/**
 * @brief Close the hardware JPEG decoder
 *
 * This function closes the hardware JPEG decoder, releasing hardware resources
 * that were allocated during the open operation and calling the underlying
 * hardware JPEG decoder's deinitialization function. After closing, the decoder
 * can be reopened or deleted.
 *
 * @param handle [in] Hardware decoder handle
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_hw_close(bk_jpeg_decode_hw_handle_t handle);

/**
 * @brief Start hardware-based JPEG decoding process
 *
 * This function initiates the hardware-based decoding of a JPEG frame.
 * It configures the hardware with the input and output buffers, then triggers
 * the hardware to start the decoding process. The decoder must be opened
 * before calling this function.
 *
 * @param handle [in] Hardware decoder handle
 * @param frame [in] Input JPEG frame buffer containing the encoded JPEG data
 * @param out_frame [out] Output decoded frame buffer where the decoded image will be stored
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_hw_decode(bk_jpeg_decode_hw_handle_t handle, frame_buffer_t *frame, frame_buffer_t *out_frame);

avdk_err_t bk_jpeg_decode_hw_decode_async(bk_jpeg_decode_hw_handle_t handle, frame_buffer_t *frame);

/**
 * @brief Delete the hardware JPEG decoder instance
 *
 * This function deletes the hardware JPEG decoder instance, releasing all
 * associated hardware and software resources including the decoder controller
 * structure memory. After deletion, the handle is no longer valid for any operation.
 *
 * @param handle [in] Hardware decoder handle to be deleted
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_hw_delete(bk_jpeg_decode_hw_handle_t handle);

/**
 * @brief Get image information before decoding
 *
 * This function retrieves image information from the input JPEG frame before actual decoding.
 * It provides details such as image dimensions, color format, and other relevant parameters
 * by calling the underlying hardware JPEG decoder's image information retrieval function.
 *
 * @param handle [in] Hardware decoder handle
 * @param info [out] Pointer to store the retrieved image information
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_hw_get_img_info(bk_jpeg_decode_hw_handle_t handle, bk_jpeg_decode_img_info_t *info);

/**
 * @brief Extended control interface for hardware JPEG decoder
 *
 * This function provides an extended interface for controlling the hardware JPEG decoder
 * and accessing additional functionalities beyond basic decoding operations. It allows
 * sending specific commands to the hardware decoder for advanced configuration and control.
 *
 * @param handle [in] Hardware decoder handle
 * @param cmd [in] Command code specifying the operation to perform (defined in jpeg_decode_hw_ioctl_cmd_t)
 * @param param [in/out] Command parameters specific to the operation being performed
 * @return 
 *  - BK_OK: Success
 *  - Others: Fail (error code indicates specific failure reason)
 */
avdk_err_t bk_jpeg_decode_hw_ioctl(bk_jpeg_decode_hw_handle_t handle, bk_jpeg_decode_hw_ioctl_cmd_t cmd, void *param);

#ifdef __cplusplus
}
#endif
