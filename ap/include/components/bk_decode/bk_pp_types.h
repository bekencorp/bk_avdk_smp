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
#include <components/avdk_utils/avdk_error.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*bk_pp_done_cb)(int status, void *args);

/**
 * @brief PP controller configuration (set once at bk_pp_ctlr_new()).
 */
typedef struct {
	uint32_t      timeout_ms;      /* HW wait timeout in ms (0 -> default) */
	bk_pp_done_cb done_cb;         /* optional, invoked after each operation */
	void         *done_args;       /* user context passed to done_cb */
} bk_pp_config_t;

/**
 * @brief One standalone PP processing request (passed to bk_pp_process()).
 *
 * The PP hardware reads an NV12 (YCbCr 4:2:0 semiplanar) picture from memory
 * and writes the result to out_buffer in the requested pixel format (NV12 /
 * RGB565 / RGB888). out_width / out_height select the output resolution; zero
 * means same as the corresponding input dimension.
 *
 * All buffers must live in PP-bus-accessible memory (e.g. PSRAM via
 * bk_frame_buffer_malloc) and meet the hardware alignment requirement.
 */
typedef struct {
	void    *in_y;         /* NV12 luminance plane */
	void    *in_c;         /* NV12 interleaved CbCr plane */
	uint16_t in_width;     /* input picture width in pixels */
	uint16_t in_height;    /* input picture height in pixels */
	uint32_t in_format;    /* input pixel format; only BK_PIXEL_FORMAT_NV12 is supported */
	uint16_t out_width;    /* output width (0 -> = in_width) */
	uint16_t out_height;   /* output height (0 -> = in_height) */
	void    *out_buffer;   /* output buffer */
	uint32_t out_size;     /* output buffer size in bytes */
	uint32_t out_format;   /* BK_PIXEL_FORMAT_NV12 / RGB565 / RGB888 */
} bk_pp_process_req_t;

typedef struct bk_pp_ctlr_t *bk_pp_ctlr_handle_t;

typedef struct bk_pp_ctlr_t bk_pp_ctlr_t;
struct bk_pp_ctlr_t {
	avdk_err_t (*init)(bk_pp_ctlr_t *controller);
	avdk_err_t (*open)(bk_pp_ctlr_t *controller);
	avdk_err_t (*process)(bk_pp_ctlr_t *controller, const bk_pp_process_req_t *req);
	avdk_err_t (*close)(bk_pp_ctlr_t *controller);
	avdk_err_t (*deinit)(bk_pp_ctlr_t *controller);
	avdk_err_t (*del)(bk_pp_ctlr_t *controller);
};

#define DEFAULT_PP_CONFIG { \
	.timeout_ms = 1000U, \
	.done_cb = NULL, \
	.done_args = NULL, \
}

#ifdef __cplusplus
}
#endif
