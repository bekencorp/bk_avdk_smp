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

#include "bk_h264_decode_types.h"

// Create decoder
avdk_err_t bk_h264_decode_new(bk_h264_decode_ctlr_handle_t *handle, bk_h264_decode_config_t *config);

//internal function
avdk_err_t bk_h264_decode_ctlr_new(bk_h264_decode_ctlr_handle_t *handle, bk_h264_decode_config_t *config);

// Initialize decoder
avdk_err_t bk_h264_decode_init(bk_h264_decode_ctlr_handle_t handle);

// Deinitialize decoder
avdk_err_t bk_h264_decode_deinit(bk_h264_decode_ctlr_handle_t handle);

// Open decoder
avdk_err_t bk_h264_decode_open(bk_h264_decode_ctlr_handle_t handle);

// Close decoder
avdk_err_t bk_h264_decode_close(bk_h264_decode_ctlr_handle_t handle);

// Execute decoding
avdk_err_t bk_h264_decode_decode(bk_h264_decode_ctlr_handle_t handle, uint8_t *in_buf, uint32_t in_size);

// Configure PP (Post-Processor) for scaling/rotation
avdk_err_t bk_h264_decode_pp_config(bk_h264_decode_ctlr_handle_t handle, uint32_t out_width, uint32_t out_height, uint32_t rotation);

// Control interface
avdk_err_t bk_h264_decode_ioctl(bk_h264_decode_ctlr_handle_t handle, bk_h264_decode_ioctl_cmd_t cmd, void *arg);

// Delete decoder
avdk_err_t bk_h264_decode_delete(bk_h264_decode_ctlr_handle_t handle);

#ifdef __cplusplus
}
#endif

