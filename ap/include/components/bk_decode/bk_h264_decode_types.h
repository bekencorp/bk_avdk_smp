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
	BK_H264_DECODE_IOCTL_SET_OSD,                /* arg: bk_h264_decode_osd_t* */
	/* Zero-copy frame controller only (bk_h264_decode_frame_zerocopy_ctlr_new): */
	BK_H264_DECODE_IOCTL_DEQUEUE,  /* arg: bk_h264_decode_dequeue_t*  - pull next display-order frame */
	BK_H264_DECODE_IOCTL_RELEASE,  /* arg: bk_h264_decode_out_frame_t* - return a dequeued frame */
	BK_H264_DECODE_IOCTL_FLUSH,    /* arg: NULL - emit trailing reordered pictures at EOS */
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
	uint32_t enable;
	int32_t originX;
	int32_t originY;
	uint32_t height;
	uint32_t width;
	uint32_t alphaBlendEna;
	uint8_t *blendComponentBase;
	int32_t blendOriginX;
	int32_t blendOriginY;
	uint32_t blendWidth;
	uint32_t blendHeight;
} bk_h264_decode_osd_config_t;

typedef void (*bk_h264_decode_osd_update_cb)(bk_h264_decode_osd_config_t *osd);

typedef struct {
	bk_h264_decode_osd_config_t osd[2];
	bk_h264_decode_osd_update_cb osd_update_cb;
} bk_h264_decode_osd_t;

typedef struct {
	uint32_t timeout_ms;
	uint16_t out_width;
	uint16_t out_height;
	uint32_t out_format;
	bk_h264_decode_osd_config_t osd[2];
	bk_h264_decode_osd_update_cb osd_update_cb;
	bk_h264_decode_frame_done_cb frame_done_cb;
	void *frame_done_args;
} bk_h264_decode_frame_config_t;

/*
 * Configuration for the zero-copy / B-frame whole-frame controller
 * (bk_h264_decode_frame_zerocopy_ctlr_new). Kept as a dedicated type so the
 * zero-copy controller stays decoupled from the legacy frame controller
 * (bk_h264_decode_frame_ctlr_new / bk_h264_decode_frame_config_t).
 */
typedef struct {
	uint32_t timeout_ms;
	uint16_t out_width;
	uint16_t out_height;
	uint32_t out_format;
	bk_h264_decode_frame_done_cb frame_done_cb;
	void *frame_done_args;
	/*
	 * Number of extra display slots the application may hold concurrently on top
	 * of the codec DPB. Range 1..4; 0 selects the controller default (1).
	 */
	uint16_t disp_depth;
} bk_h264_decode_frame_zerocopy_config_t;

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
	uint8_t *out_buffer;      /* ignored by the zero-copy frame-pool controller */
	uint32_t out_buffer_size; /* ignored by the zero-copy frame-pool controller */
} bk_h264_decode_input_t;

/*
 * Application-facing display view of one decoded frame, returned by the
 * zero-copy frame-pool controller through BK_H264_DECODE_IOCTL_DEQUEUE. Frames
 * are delivered already in display (POC) order, so sequential dequeue yields the
 * correct playback order (B-frame reordering handled internally). Each dequeued
 * frame must be returned exactly once via BK_H264_DECODE_IOCTL_RELEASE.
 */
typedef struct {
	uint8_t *data;        /* pixel data first address (render/copy) */
	uint32_t data_len;    /* valid pixel bytes (NV12=w*h*3/2, GRAY8=w*h) */
	uint32_t capacity;    /* physical slot capacity (>= data_len) */
	uint16_t width;       /* 16-aligned coded width (== row pitch) */
	uint16_t height;      /* 16-aligned coded height */
	uint8_t  format;      /* vcdec_pix_fmt_e */
	uint8_t  frame_type;  /* bk_h264_decode_frame_type_t */
	int32_t  poc;         /* picture order count (display-order key) */
	void    *token;       /* opaque return token, do not dereference */
} bk_h264_decode_out_frame_t;

typedef struct {
	bk_h264_decode_out_frame_t frame; /* out: filled on success */
	uint32_t timeout_ms;              /* in: max wait; 0 = non-blocking poll */
} bk_h264_decode_dequeue_t;

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
	.osd = {{0}}, \
	.osd_update_cb = NULL, \
	.frame_done_cb = NULL, \
	.frame_done_args = NULL, \
}

#define DEFAULT_H264_DECODE_FRAME_ZEROCOPY_CONFIG { \
	.timeout_ms = 1000U, \
	.out_width = 1280U, \
	.out_height = 720U, \
	.out_format = BK_PIXEL_FORMAT_NV12, \
	.frame_done_cb = NULL, \
	.frame_done_args = NULL, \
	.disp_depth = 0U, \
}

#ifdef __cplusplus
}
#endif
