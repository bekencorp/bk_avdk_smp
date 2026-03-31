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

#include <stdint.h>
#include <avdk_check.h>

#ifdef __cplusplus
extern "C" {
#endif

// ioctl command definitions
typedef enum {
    BK_H264_DECODE_IOCTL_DEBUG_START,    // Start debug, arg: uint32_t* (time interval ms)
    BK_H264_DECODE_IOCTL_DEBUG_STOP,     // Stop debug, arg: NULL
    BK_H264_DECODE_IOCTL_SET_PARAM,      // Set parameters
    BK_H264_DECODE_IOCTL_GET_INFO,       // Get decoder info
} bk_h264_decode_ioctl_cmd_t;

// Output buffer request callback function prototype
// size: required buffer size for decoded output (Y+UV data)
// Returns pointer to output buffer, or NULL if allocation fails
typedef void* (*bk_h264_decode_output_buffer_request_cb_t)(uint32_t size);

// Decode completion callback function prototype
// buffer: input buffer pointer that was decoded, result: decode result (BK_OK or error code)
typedef uint32_t (*bk_h264_decode_complete_cb_t)(void *buffer, uint32_t result, uint32_t event_type, uint32_t used_size, void *user_data);

typedef struct
{
    void *user_data;                                              // User data pointer passed to callbacks, not modified by decoder
    bk_h264_decode_output_buffer_request_cb_t output_buffer_cb;   // Output buffer request callback
    bk_h264_decode_complete_cb_t decode_complete_cb;              // Decode completion callback
    uint8_t chnl_id;                                              // Channel ID
    uint32_t flexa_mode;                                          // Flexa mode (0: none, 1: flexa, 2+: slice mode)
} bk_h264_decode_config_t;

typedef struct bk_h264_decode_ctlr_t *bk_h264_decode_ctlr_handle_t;
typedef struct bk_h264_decode_ctlr_t bk_h264_decode_ctlr_t;

struct bk_h264_decode_ctlr_t
{
    avdk_err_t (*init)(bk_h264_decode_ctlr_t *controller);
    avdk_err_t (*open)(bk_h264_decode_ctlr_t *controller);
    avdk_err_t (*decode)(bk_h264_decode_ctlr_t *controller, uint8_t *in_buf, uint32_t in_size);
    avdk_err_t (*decode_sync)(bk_h264_decode_ctlr_t *controller, uint8_t *in_buf, uint32_t in_size);   // Synchronous decode, blocks until completion
    avdk_err_t (*decode_async)(bk_h264_decode_ctlr_t *controller, uint8_t *in_buf, uint32_t in_size);  // Asynchronous decode, returns immediately
    avdk_err_t (*pp_config)(bk_h264_decode_ctlr_t *controller, uint32_t out_width, uint32_t out_height, uint32_t rotation);  // Configure PP for scaling/rotation
    avdk_err_t (*close)(bk_h264_decode_ctlr_t *controller);
    avdk_err_t (*deinit)(bk_h264_decode_ctlr_t *controller);
    avdk_err_t (*ioctl)(bk_h264_decode_ctlr_t *controller, uint32_t cmd, void *arg);
    avdk_err_t (*delete)(bk_h264_decode_ctlr_t *controller);
};

#ifdef __cplusplus
}
#endif

