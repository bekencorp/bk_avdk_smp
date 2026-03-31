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

avdk_err_t bk_h264_encode_hw_flexa_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_hw_flexa_config_t *config);
avdk_err_t bk_h264_encode_sw_flexa_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_sw_flexa_config_t *config);
avdk_err_t bk_h264_encode_frame_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_frame_config_t *config);

// Initialize encoder
avdk_err_t bk_h264_encode_init(bk_h264_encode_ctlr_handle_t handle);

// Deinitialize encoder
avdk_err_t bk_h264_encode_deinit(bk_h264_encode_ctlr_handle_t handle);

// Open encoder
avdk_err_t bk_h264_encode_open(bk_h264_encode_ctlr_handle_t handle);

// Close encoder
avdk_err_t bk_h264_encode_close(bk_h264_encode_ctlr_handle_t handle);

// Execute encoding
avdk_err_t bk_h264_encode_start(bk_h264_encode_ctlr_handle_t handle);

// Control interface
avdk_err_t bk_h264_encode_ioctl(bk_h264_encode_ctlr_handle_t handle, bk_h264_encode_ioctl_cmd_t cmd, void *arg);

// Force IDR frame generation
avdk_err_t bk_h264_encode_force_idr(bk_h264_encode_ctlr_handle_t handle);

// Delete encoder
avdk_err_t bk_h264_encode_delete(bk_h264_encode_ctlr_handle_t handle);

#ifdef __cplusplus
}
#endif
