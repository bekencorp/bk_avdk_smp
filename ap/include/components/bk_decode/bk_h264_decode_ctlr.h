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

/**
 * @brief Create a Flexa-mode H.264 decoder controller
 * @param handle Output pointer that receives the decoder handle
 * @param config Flexa decoder configuration
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_flexa_ctlr_new(bk_h264_decode_ctlr_handle_t *handle, bk_h264_decode_flexa_config_t *config);

/**
 * @brief Create a frame-mode H.264 decoder controller
 * @param handle Output pointer that receives the decoder handle
 * @param config Frame decoder configuration
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_frame_ctlr_new(bk_h264_decode_ctlr_handle_t *handle, bk_h264_decode_frame_config_t *config);

/**
 * @brief Create a zero-copy / B-frame capable whole-frame H.264 decoder controller
 *
 * Like bk_h264_decode_frame_ctlr_new() it decodes one access unit per
 * bk_h264_decode_frame() call, but it owns an internal zero-copy frame pool:
 * one physical buffer serves as decode target / DPB reference / display output,
 * eliminating the reference-backup and output copies and enabling B-frame
 * decoding with POC-based display reordering. The per-frame output buffer in
 * bk_h264_decode_input_t is ignored; decoded frames are pulled in display order
 * via BK_H264_DECODE_IOCTL_DEQUEUE and returned via BK_H264_DECODE_IOCTL_RELEASE,
 * with BK_H264_DECODE_IOCTL_FLUSH emitting trailing reordered pictures at EOS.
 *
 * @param handle Output pointer that receives the decoder handle
 * @param config Zero-copy frame decoder configuration (disp_depth applies here)
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_frame_zerocopy_ctlr_new(bk_h264_decode_ctlr_handle_t *handle, bk_h264_decode_frame_zerocopy_config_t *config);

/**
 * @brief Initialize the H.264 decoder
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_init(bk_h264_decode_ctlr_handle_t handle);

/**
 * @brief Deinitialize the H.264 decoder
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_deinit(bk_h264_decode_ctlr_handle_t handle);

/**
 * @brief Open the H.264 decoder for decoding
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_open(bk_h264_decode_ctlr_handle_t handle);

/**
 * @brief Close the H.264 decoder
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_close(bk_h264_decode_ctlr_handle_t handle);

/**
 * @brief Decode one H.264 access unit or frame
 * @param handle Decoder handle
 * @param input Input bitstream and output buffer descriptors
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_frame(bk_h264_decode_ctlr_handle_t handle, bk_h264_decode_input_t *input);

/**
 * @brief Issue an IOCTL command on the H.264 decoder
 * @param handle Decoder handle
 * @param cmd IOCTL command
 * @param arg Command-specific argument
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_ioctl(bk_h264_decode_ctlr_handle_t handle, bk_h264_decode_ioctl_cmd_t cmd, void *arg);

/**
 * @brief Delete the H.264 decoder controller
 * @param handle Decoder handle
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_delete(bk_h264_decode_ctlr_handle_t handle);

/**
 * @brief Get decoded stream or frame metadata
 * @param handle Decoder handle
 * @param info Output structure for stream/frame info
 * @return AVDK error code
 */
avdk_err_t bk_h264_decode_get_info(bk_h264_decode_ctlr_handle_t handle, bk_h264_decode_info_t *info);

#ifdef __cplusplus
}
#endif
