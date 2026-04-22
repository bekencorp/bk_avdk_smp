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

/*
 * Create a new flexa decoder
 * config: decoder configuration
 * return: AVDK_ERR_OK if successful, otherwise an error code
 */
avdk_err_t bk_jpeg_decode_flexa_ctlr_new(bk_jpeg_decode_ctlr_handle_t *handle, bk_jpeg_decode_flexa_config_t *config);

/*
 * Create a new frame decoder
 * config: decoder configuration
 * return: AVDK_ERR_OK if successful, otherwise an error code
 */
avdk_err_t bk_jpeg_decode_frame_ctlr_new(bk_jpeg_decode_ctlr_handle_t *handle, bk_jpeg_decode_frame_config_t *config);

/*
 * Initialize the decoder
 * handle: decoder handle
 * return: AVDK_ERR_OK if successful, otherwise an error code
 */
avdk_err_t bk_jpeg_decode_init(bk_jpeg_decode_ctlr_handle_t handle);

/* 
 * Deinitialize the decoder
 * handle: decoder handle
 * return: AVDK_ERR_OK if successful, otherwise an error code
 */
avdk_err_t bk_jpeg_decode_deinit(bk_jpeg_decode_ctlr_handle_t handle);

/*
 * Open the decoder
 * handle: decoder handle
 * return: AVDK_ERR_OK if successful, otherwise an error code
 */
avdk_err_t bk_jpeg_decode_open(bk_jpeg_decode_ctlr_handle_t handle);

/*
 * Close the decoder
 * handle: decoder handle
 * return: AVDK_ERR_OK if successful, otherwise an error code
 */
avdk_err_t bk_jpeg_decode_close(bk_jpeg_decode_ctlr_handle_t handle);

/*
 * Decode a frame
 * handle: decoder handle
 * input: input data
 * return: AVDK_ERR_OK if successful, otherwise an error code
 */
avdk_err_t bk_jpeg_decode_frame(bk_jpeg_decode_ctlr_handle_t handle, bk_jpeg_decode_input_t *input);

/*
 * Control the decoder
 * handle: decoder handle
 * cmd: command
 * arg: argument
 * return: AVDK_ERR_OK if successful, otherwise an error code
 */
avdk_err_t bk_jpeg_decode_ioctl(bk_jpeg_decode_ctlr_handle_t handle, bk_jpeg_decode_ioctl_cmd_t cmd, void *arg);

/*
 * Delete the decoder
 * handle: decoder handle
 * return: AVDK_ERR_OK if successful, otherwise an error code
 */
avdk_err_t bk_jpeg_decode_delete(bk_jpeg_decode_ctlr_handle_t handle);

#ifdef __cplusplus
}
#endif
