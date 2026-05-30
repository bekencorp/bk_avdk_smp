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
#include "components/avdk_utils/avdk_error.h"
#include "common/avdk_pixel_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *outbuf;
	uint32_t length;
	uint32_t type;
	uint32_t status;
	uint32_t sequence;
	void *args;
} bk_jpeg_encode_outbuf_info_t;

typedef void (*bk_jpeg_encode_flexa_done_cb)(uint32_t rd_blocks, void *args);

typedef enum {
	BK_JPEG_ENCODE_IOCTL_SET_QUALITY, /*!< arg: uint8_t * quality 0..10 */
	BK_JPEG_ENCODE_IOCTL_SET_FLEXA_LINES_READY, /*!< Software Flexa: arg is block count cast to void * */
	BK_JPEG_ENCODE_IOCTL_SET_FRAME_READY, /*!< Software Flexa: notify one frame input is ready */
	BK_JPEG_ENCODE_IOCTL_REGISTER_BOND, /*!< arg: bk_flexa_bond_t * */
	BK_JPEG_ENCODE_IOCTL_UNREGISTER_BOND, /*!< arg: bk_flexa_bond_t * */
	BK_JPEG_ENCODE_IOCTL_STOP_ENCODE, /*!< stop current Flexa encode */
} bk_jpeg_encode_ioctl_cmd_t;

typedef enum {
	BK_JPEG_ENCODE_FLEXA_MODE_NONE = 0,
	BK_JPEG_ENCODE_FLEXA_MODE_SOFTWARE,
	BK_JPEG_ENCODE_FLEXA_MODE_HARDWARE,
} bk_jpeg_encode_flexa_mode_t;

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t input_format; /*!< e.g. BK_PIXEL_FORMAT_NV12 */
	uint32_t input_buf;    /*!< NV12 luma base (physical/bus address as used by VCENC) */
	uint32_t input_size;   /*!< reserved; line count for flexa-like paths (unused for JPEG frame mode) */
	uint8_t quality;       /*!< 0..10 */
	void *(*outbuf_malloc)(uint32_t outbuf_size, void *args);
	void *outbuf_malloc_args;
	uint32_t (*outbuf_complete)(bk_jpeg_encode_outbuf_info_t *info);
	void *outbuf_complete_args;
} bk_jpeg_encode_frame_config_t;

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t input_format;
	uint32_t input_flexa_cnt;
	uint32_t input_buf;
	uint32_t input_size;
	uint8_t quality;
	void *(*outbuf_malloc)(uint32_t outbuf_size, void *args);
	void *outbuf_malloc_args;
	uint32_t (*outbuf_complete)(bk_jpeg_encode_outbuf_info_t *info);
	void *outbuf_complete_args;
} bk_jpeg_encode_hw_flexa_config_t;

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t input_format;
	uint32_t input_flexa_cnt;
	uint32_t input_buf;
	uint32_t input_size;
	uint8_t quality;
	void *(*outbuf_malloc)(uint32_t outbuf_size, void *args);
	void *outbuf_malloc_args;
	uint32_t (*outbuf_complete)(bk_jpeg_encode_outbuf_info_t *info);
	void *outbuf_complete_args;
	bk_jpeg_encode_flexa_done_cb flexa_done;
	void *flexa_done_arg;
} bk_jpeg_encode_sw_flexa_config_t;

typedef struct {
	uint32_t pic_buf;   /*!< override input NV12 base for this frame */
	uint32_t pic_lines; /*!< unused */
	uint32_t out_buf;   /*!< optional; 0 = allocate via outbuf_malloc */
	uint32_t out_size;  /*!< optional output capacity when out_buf non-zero */
} bk_jpeg_encode_input_t;

typedef struct bk_jpeg_encode_ctlr_t *bk_jpeg_encode_ctlr_handle_t;
typedef struct bk_jpeg_encode_ctlr_t bk_jpeg_encode_ctlr_t;

struct bk_jpeg_encode_ctlr_t {
	avdk_err_t (*init)(bk_jpeg_encode_ctlr_t *controller);
	avdk_err_t (*open)(bk_jpeg_encode_ctlr_t *controller);
	avdk_err_t (*encode_frame)(bk_jpeg_encode_ctlr_t *controller, bk_jpeg_encode_input_t *input);
	avdk_err_t (*close)(bk_jpeg_encode_ctlr_t *controller);
	avdk_err_t (*deinit)(bk_jpeg_encode_ctlr_t *controller);
	avdk_err_t (*ioctl)(bk_jpeg_encode_ctlr_t *controller, uint32_t cmd, void *arg);
	avdk_err_t (*del)(bk_jpeg_encode_ctlr_t *controller);
};

#define DEFAULT_JPEG_ENCODE_FRAME_CONFIG { \
	.width = 1280, \
	.height = 720, \
	.input_format = BK_PIXEL_FORMAT_NV12, \
	.input_buf = 0, \
	.input_size = 0, \
	.quality = 5, \
	.outbuf_malloc = NULL, \
	.outbuf_malloc_args = NULL, \
	.outbuf_complete = NULL, \
	.outbuf_complete_args = NULL, \
}

#define DEFAULT_JPEG_ENCODE_HW_FLEXA_CONFIG { \
	.width = 1280, \
	.height = 720, \
	.input_format = BK_PIXEL_FORMAT_NV12, \
	.input_flexa_cnt = 1, \
	.input_buf = 0, \
	.input_size = 0, \
	.quality = 5, \
	.outbuf_malloc = NULL, \
	.outbuf_malloc_args = NULL, \
	.outbuf_complete = NULL, \
	.outbuf_complete_args = NULL, \
}

#define DEFAULT_JPEG_ENCODE_SW_FLEXA_CONFIG { \
	.width = 1280, \
	.height = 720, \
	.input_format = BK_PIXEL_FORMAT_NV12, \
	.input_flexa_cnt = 1, \
	.input_buf = 0, \
	.input_size = 0, \
	.quality = 5, \
	.outbuf_malloc = NULL, \
	.outbuf_malloc_args = NULL, \
	.outbuf_complete = NULL, \
	.outbuf_complete_args = NULL, \
	.flexa_done = NULL, \
	.flexa_done_arg = NULL, \
}

#ifdef __cplusplus
}
#endif
