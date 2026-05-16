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

#include "bk_h264_encode_types.h"

/**
 * @brief Create a new hardware flexa encoder
 * @param handle Encoder handle
 * @param config Hardware flexa configuration
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_hw_flexa_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_hw_flexa_config_t *config);

/**
 * @brief Create a new software flexa encoder
 * @param handle Encoder handle
 * @param config Software flexa configuration
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_sw_flexa_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_sw_flexa_config_t *config);

/**
 * @brief Create a new frame encoder
 * @param handle Encoder handle
 * @param config Frame configuration
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_frame_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_frame_config_t *config);

/**
 * @brief Initialize encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_init(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Deinitialize encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_deinit(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Open encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_open(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Close encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_close(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Execute encoding
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_start(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Control interface
 * @param handle Encoder handle
 * @param cmd Command
 * @param arg Argument
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_ioctl(bk_h264_encode_ctlr_handle_t handle, bk_h264_encode_ioctl_cmd_t cmd, void *arg);
/**
 * @brief Force IDR frame generation
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_force_idr(bk_h264_encode_ctlr_handle_t handle);

/**
 * @brief Configure GOP frame count
 * @param handle Encoder handle
 * @param gop_frame_count Number of frames in one GOP
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_set_gop_frame_count(bk_h264_encode_ctlr_handle_t handle,
                                              uint32_t gop_frame_count);

/**
 * @brief Get GOP frame count
 * @param handle Encoder handle
 * @param gop_frame_count Output GOP frame count
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_get_gop_frame_count(bk_h264_encode_ctlr_handle_t handle,
                                              uint32_t *gop_frame_count);

/**
 * @brief Configure rate control
 * @param handle Encoder handle
 * @param rate_ctrl Rate control config
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_set_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                         bk_h264_encode_rate_ctrl_t *rate_ctrl);

/**
 * @brief Get rate control config
 * @param handle Encoder handle
 * @param rate_ctrl Output rate control config
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_get_rate_ctrl(bk_h264_encode_ctlr_handle_t handle,
                                         bk_h264_encode_rate_ctrl_t *rate_ctrl);

/**
 * @brief Delete encoder
 * @param handle Encoder handle
 * @return AVSDK error code
 */
avdk_err_t bk_h264_encode_delete(bk_h264_encode_ctlr_handle_t handle);

#ifdef __cplusplus
}
#endif
