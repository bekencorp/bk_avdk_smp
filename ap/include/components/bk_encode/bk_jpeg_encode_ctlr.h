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

#include "bk_jpeg_encode_types.h"

/**
 * @brief Create frame-mode JPEG encoder (VCENC JPEG path).
 */
avdk_err_t bk_jpeg_encode_frame_new(bk_jpeg_encode_ctlr_handle_t *handle, bk_jpeg_encode_frame_config_t *config);

/**
 * @brief Create hardware-Flexa JPEG encoder (VCENC JPEG low-latency HW sync path).
 */
avdk_err_t bk_jpeg_encode_hw_flexa_new(bk_jpeg_encode_ctlr_handle_t *handle,
				       bk_jpeg_encode_hw_flexa_config_t *config);

/**
 * @brief Create software-Flexa JPEG encoder (VCENC JPEG low-latency SW sync path).
 */
avdk_err_t bk_jpeg_encode_sw_flexa_new(bk_jpeg_encode_ctlr_handle_t *handle,
				       bk_jpeg_encode_sw_flexa_config_t *config);

avdk_err_t bk_jpeg_encode_init(bk_jpeg_encode_ctlr_handle_t handle);

avdk_err_t bk_jpeg_encode_deinit(bk_jpeg_encode_ctlr_handle_t handle);

avdk_err_t bk_jpeg_encode_open(bk_jpeg_encode_ctlr_handle_t handle);

avdk_err_t bk_jpeg_encode_close(bk_jpeg_encode_ctlr_handle_t handle);

avdk_err_t bk_jpeg_encode_frame(bk_jpeg_encode_ctlr_handle_t handle, bk_jpeg_encode_input_t *input);

avdk_err_t bk_jpeg_encode_ioctl(bk_jpeg_encode_ctlr_handle_t handle, bk_jpeg_encode_ioctl_cmd_t cmd, void *arg);

avdk_err_t bk_jpeg_encode_delete(bk_jpeg_encode_ctlr_handle_t handle);

#ifdef __cplusplus
}
#endif
