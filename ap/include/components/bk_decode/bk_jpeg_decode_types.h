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
#ifdef __cplusplus
extern "C" {
#endif

/** JPEG decode result callback: (status, args). */
typedef void (*bk_jpeg_decode_frame_done_cb)(int status, void *args);

/** Flexa done callback: (wr_ptr, args). */
typedef void (*bk_jpeg_decode_flexa_done_cb)(uint32_t wr_ptr, void *args);

/** Request output buffer for decoded frame. Returns pointer to buffer that can hold at least size bytes; or NULL. */
typedef void *(*bk_jpeg_decode_buffer_request_cb)(uint32_t width, uint32_t height, uint32_t *out_y_size, uint32_t *out_c_size);

/** Optional: notify that output buffer is no longer used. */
typedef void (*bk_jpeg_decode_buffer_release_cb)(void *buf);

typedef enum {
	BK_JPEG_DECODE_IOCTL_GET_INFO,
	BK_JPEG_DECODE_IOCTL_ABORT,
	BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR,
	BK_JPEG_DECODE_IOCTL_REGISTER_BOND,
	BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND,
	BK_JPEG_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE,
} bk_jpeg_decode_ioctl_cmd_t;

#define BK_JPEG_DECODE_RD_PORT_MAX (2U)

typedef enum {
	BK_JPEG_DECODE_RD_PORT_GPU = 0,
	BK_JPEG_DECODE_RD_PORT_H264E = 1,
} bk_jpeg_decode_rd_port_id_t;

typedef struct {
	void *port_ptr;
	uint32_t rd_blocks;
} bk_jpeg_decode_port_rd_t;


typedef enum {
	BK_JPEG_DECODE_FLEXA_MODE_NONE = 0,
	BK_JPEG_DECODE_FLEXA_MODE_FLEXA,
	BK_JPEG_DECODE_FLEXA_MODE_SLICE,
} bk_jpeg_decode_flexa_mode_t;

/**
 * Parameters for hardware JPEG decode via vcdec.
 * Pass this struct as arg for BK_JPEG_DECODE_IOCTL_SET_PARAM before bk_jpeg_decode_open().
 */
typedef struct {
	/** vcdec mode: NONE for full-frame; FLEXA enables PP ring-buffer output. */
	bk_jpeg_decode_flexa_mode_t flexa_mode;
	/**
	 * Flexa done callback (Flexa mode only). Note: underlying ISR passes ring-buffer write pointer
	 * as the first argument.
	 */
	bk_jpeg_decode_flexa_done_cb flexa_done_cb;
	/** User argument passed to flexa_done_cb. */
	void *flexa_args;
	/** Decode timeout in seconds. 0 means use default (10). */
	uint32_t timeout_sec;
	/** Output width/height for buffer layout. For Flexa, out_width must be non-zero. */
	uint16_t out_width;
	uint16_t out_height;
	/** Flexa ring-buffer segment height in macroblocks (16 lines per MB). 0 means use default (1). */
	uint16_t segment_height_mb;
	/** Flexa ring-buffer segment number. 0 means use default (2). */
	uint8_t segment_number;
} bk_jpeg_decode_param_t;

typedef struct {
	bk_jpeg_decode_flexa_mode_t decode_mode;
	uint32_t timeout_ms;
	uint16_t width;
	uint16_t height;
	uint16_t segment_height;
	uint8_t segment_number;
	bk_jpeg_decode_frame_done_cb frame_done_cb;
	bk_jpeg_decode_flexa_done_cb flexa_done_cb;
	void *args;
	bk_jpeg_decode_buffer_request_cb buffer_request_cb;
	bk_jpeg_decode_buffer_release_cb buffer_release_cb;
	void *user_data;
} bk_jpeg_decode_config_t;

/** Input for one decode: JPEG stream and optional pre-allocated output buffers. */
typedef struct {
	uint8_t *stream;
	uint32_t stream_len;
	/** If non-NULL, use these buffers (Y and C/UV); otherwise use buffer_request_cb. */
	uint8_t *out_buffer;
	uint32_t out_buffer_size;
} bk_jpeg_decode_input_t;

/** Decoded frame info (e.g. from header or after decode). */
typedef struct {
	uint32_t width;
	uint32_t height;
} bk_jpeg_decode_info_t;

typedef struct bk_jpeg_decode_ctlr_t *bk_jpeg_decode_ctlr_handle_t;
typedef struct bk_jpeg_decode_ctlr_t bk_jpeg_decode_ctlr_t;

struct bk_jpeg_decode_ctlr_t {
	avdk_err_t (*init)(bk_jpeg_decode_ctlr_t *controller);
	avdk_err_t (*open)(bk_jpeg_decode_ctlr_t *controller);
	avdk_err_t (*decode_frame)(bk_jpeg_decode_ctlr_t *controller, bk_jpeg_decode_input_t *input);
	avdk_err_t (*close)(bk_jpeg_decode_ctlr_t *controller);
	avdk_err_t (*deinit)(bk_jpeg_decode_ctlr_t *controller);
	avdk_err_t (*ioctl)(bk_jpeg_decode_ctlr_t *controller, uint32_t cmd, void *arg);
	avdk_err_t (*delete)(bk_jpeg_decode_ctlr_t *controller);
};

#ifdef __cplusplus
}
#endif
