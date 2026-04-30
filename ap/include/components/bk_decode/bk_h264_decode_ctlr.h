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

avdk_err_t bk_h264_decode_flexa_ctlr_new(bk_h264_decode_ctlr_handle_t *handle, bk_h264_decode_flexa_config_t *config);
avdk_err_t bk_h264_decode_frame_ctlr_new(bk_h264_decode_ctlr_handle_t *handle, bk_h264_decode_frame_config_t *config);

avdk_err_t bk_h264_decode_init(bk_h264_decode_ctlr_handle_t handle);
avdk_err_t bk_h264_decode_deinit(bk_h264_decode_ctlr_handle_t handle);
avdk_err_t bk_h264_decode_open(bk_h264_decode_ctlr_handle_t handle);
avdk_err_t bk_h264_decode_close(bk_h264_decode_ctlr_handle_t handle);
avdk_err_t bk_h264_decode_frame(bk_h264_decode_ctlr_handle_t handle, bk_h264_decode_input_t *input);
avdk_err_t bk_h264_decode_ioctl(bk_h264_decode_ctlr_handle_t handle, bk_h264_decode_ioctl_cmd_t cmd, void *arg);
avdk_err_t bk_h264_decode_delete(bk_h264_decode_ctlr_handle_t handle);
avdk_err_t bk_h264_decode_get_info(bk_h264_decode_ctlr_handle_t handle, bk_h264_decode_info_t *info);

#ifdef __cplusplus
}
#endif
