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

#ifdef __cplusplus
extern "C" {
#endif

#include "bk_jpeg_decode_types.h"

/**
 * @brief Create a Flexa-mode JPEG decoder controller
 * @param handle Output pointer that receives the decoder handle
 * @param config Flexa decoder configuration
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_flexa_ctlr_new(bk_jpeg_decode_ctlr_handle_t *handle, bk_jpeg_decode_flexa_config_t *config);

/**
 * @brief Create a frame-mode JPEG decoder controller
 * @param handle Output pointer that receives the decoder handle
 * @param config Frame decoder configuration
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_frame_ctlr_new(bk_jpeg_decode_ctlr_handle_t *handle, bk_jpeg_decode_frame_config_t *config);

/**
 * @brief Initialize the JPEG decoder
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_init(bk_jpeg_decode_ctlr_handle_t handle);

/**
 * @brief Deinitialize the JPEG decoder
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_deinit(bk_jpeg_decode_ctlr_handle_t handle);

/**
 * @brief Open the JPEG decoder for decoding
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_open(bk_jpeg_decode_ctlr_handle_t handle);

/**
 * @brief Close the JPEG decoder
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_close(bk_jpeg_decode_ctlr_handle_t handle);

/**
 * @brief Decode one JPEG image
 * @param handle Decoder handle
 * @param input Input bitstream and output buffer descriptors
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_frame(bk_jpeg_decode_ctlr_handle_t handle, bk_jpeg_decode_input_t *input);

/**
 * @brief Issue an IOCTL command on the JPEG decoder
 * @param handle Decoder handle
 * @param cmd IOCTL command
 * @param arg Command-specific argument
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_ioctl(bk_jpeg_decode_ctlr_handle_t handle, bk_jpeg_decode_ioctl_cmd_t cmd, void *arg);

/**
 * @brief Delete the JPEG decoder controller
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_delete(bk_jpeg_decode_ctlr_handle_t handle);

/**
 * @brief Parse JPEG header and get image information without full decode
 * @param img_info Input/output image info structure (contains JPEG bitstream pointer)
 * @return AVDK error code
 */
avdk_err_t bk_jpeg_decode_get_img_info(bk_jpeg_decode_img_info_t *img_info);

#ifdef __cplusplus
}
#endif
