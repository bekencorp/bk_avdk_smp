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

#include "os/os.h"
#include "components/avdk_utils/avdk_error.h"
#include "modules/vcdec/vcdec_h264_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bk_h264_decode_frame_done_cb)(int status, void *args);
typedef void (*bk_h264_decode_flexa_done_cb)(uint32_t wr_ptr, void *args);

typedef enum {
	BK_H264_DECODE_IOCTL_GET_INFO,
	BK_H264_DECODE_IOCTL_ABORT,
	BK_H264_DECODE_IOCTL_PORT_SET_RD_PTR,
	BK_H264_DECODE_IOCTL_REGISTER_BOND,
	BK_H264_DECODE_IOCTL_UNREGISTER_BOND,
	BK_H264_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE,
	BK_H264_DECODE_IOCTL_RESET,
} bk_h264_decode_ioctl_cmd_t;

#define BK_H264_DECODE_RD_PORT_MAX (2U)

typedef enum {
	BK_H264_DECODE_RD_PORT_GPU = 0,
	BK_H264_DECODE_RD_PORT_H264E = 1,
} bk_h264_decode_rd_port_id_t;

typedef struct {
	void *port_ptr;
	uint32_t rd_blocks;
} bk_h264_decode_port_rd_t;

typedef struct {
	uint32_t timeout_ms;
	uint16_t out_width;
	uint16_t out_height;
	uint32_t out_format;
	bk_h264_decode_frame_done_cb frame_done_cb;
	void *frame_done_args;
} bk_h264_decode_frame_config_t;

typedef struct {
	uint32_t timeout_ms;
	uint16_t out_width;
	uint16_t out_height;
	uint32_t out_format;
	uint16_t segment_height;
	uint8_t segment_number;
	bk_h264_decode_frame_done_cb frame_done_cb;
	void *frame_done_args;
	bk_h264_decode_flexa_done_cb flexa_done_cb;
	void *flexa_done_args;
} bk_h264_decode_flexa_config_t;

typedef struct {
	uint8_t *stream;
	uint32_t stream_len;
	uint8_t *out_buffer;
	uint32_t out_buffer_size;
} bk_h264_decode_input_t;

typedef vcdec_h264_frame_type_t bk_h264_decode_frame_type_t;
typedef vcdec_h264_info_t bk_h264_decode_info_t;

typedef struct bk_h264_decode_ctlr_t *bk_h264_decode_ctlr_handle_t;

typedef struct bk_h264_decode_ctlr_t bk_h264_decode_ctlr_t;
struct bk_h264_decode_ctlr_t {
	avdk_err_t (*init)(bk_h264_decode_ctlr_t *controller);
	avdk_err_t (*open)(bk_h264_decode_ctlr_t *controller);
	avdk_err_t (*decode_frame)(bk_h264_decode_ctlr_t *controller, bk_h264_decode_input_t *input);
	avdk_err_t (*close)(bk_h264_decode_ctlr_t *controller);
	avdk_err_t (*deinit)(bk_h264_decode_ctlr_t *controller);
	avdk_err_t (*ioctl)(bk_h264_decode_ctlr_t *controller, uint32_t cmd, void *arg);
	avdk_err_t (*del)(bk_h264_decode_ctlr_t *controller);
};

#define DEFAULT_H264_DECODE_FLEXA_CONFIG { \
	.timeout_ms = 1000U, \
	.out_width = 1280U, \
	.out_height = 720U, \
	.out_format = BK_PIXEL_FORMAT_NV12, \
	.segment_height = 1U, \
	.segment_number = 2U, \
	.frame_done_cb = NULL, \
	.frame_done_args = NULL, \
	.flexa_done_cb = NULL, \
	.flexa_done_args = NULL, \
}

#define DEFAULT_H264_DECODE_FRAME_CONFIG { \
	.timeout_ms = 1000U, \
	.out_width = 1280U, \
	.out_height = 720U, \
	.out_format = BK_PIXEL_FORMAT_NV12, \
	.frame_done_cb = NULL, \
	.frame_done_args = NULL, \
}

#ifdef __cplusplus
}
#endif
