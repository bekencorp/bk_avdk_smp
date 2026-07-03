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

#include "components/bk_encode/bk_h264_encode_types.h"
#include "bk_flexa_bond_types.h"
#include "modules/vcenc/vcenc_types.h"
#include "modules/vcenc/vcenc_h264_types.h"
#include "modules/vcenc/vcenc_h264_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	ENCODER_CORE_INITING,
	ENCODER_CORE_INITED,
	ENCODER_CORE_DEINITING,
	ENCODER_CORE_DEINITED,
} encoder_core_status_t;

/**
 * @brief Per-controller H.264 debug stats, accumulated by frame_done_cb.
 *
 * Previously lived inside the h264e_driver wrapper struct; now owned directly
 * by each controller so the three ctlrs are fully self-contained.
 */
typedef struct
{
	uint32_t max_i_frame_size;
	uint32_t max_p_frame_size;
	uint32_t last_i_frame_size;
	uint32_t last_p_frame_size;
	uint32_t all_frame_size;
	uint32_t all_frame_count;
	uint32_t enc_frame_ok_cnt;
	uint32_t enc_frame_err_cnt;
	uint32_t max_i_qp;
	uint32_t min_i_qp;
	uint32_t max_p_qp;
	uint32_t min_p_qp;
	uint32_t last_frame_qp;
} h264_encode_debug_info_t;

/** Sentinel for min_qp before any frame of that type is encoded. */
#define H264_ENCODE_QP_MIN_UNSET  52U

static inline void h264_encode_debug_info_reset_qp(h264_encode_debug_info_t *info)
{
	if (info != NULL) {
		info->max_i_qp = 0;
		info->min_i_qp = H264_ENCODE_QP_MIN_UNSET;
		info->max_p_qp = 0;
		info->min_p_qp = H264_ENCODE_QP_MIN_UNSET;
		info->last_frame_qp = 0;
	}
}

static inline void h264_encode_debug_update_qp(h264_encode_debug_info_t *info, uint32_t frame_type, uint32_t qp)
{
	uint32_t *min_qp;
	uint32_t *max_qp;

	if (info == NULL || qp > 51U) {
		return;
	}

	info->last_frame_qp = qp;

	if (frame_type == VCENC_OUT_IFRAME) {
		min_qp = &info->min_i_qp;
		max_qp = &info->max_i_qp;
	} else if (frame_type == VCENC_OUT_PFRAME) {
		min_qp = &info->min_p_qp;
		max_qp = &info->max_p_qp;
	} else {
		return;
	}

	if (*min_qp > 51U) {
		*min_qp = qp;
		*max_qp = qp;
		return;
	}

	if (qp < *min_qp) {
		*min_qp = qp;
	}
	if (qp > *max_qp) {
		*max_qp = qp;
	}
}

/**
 * @brief Default fixed-QP values applied on controller open, matching what
 * the removed h264e_driver wrapper used to seed via h264e_open(). Keep as
 * named constants so a future tune knob has one obvious place to change.
 */
#define H264_ENCODE_DEFAULT_OPEN_QP_I 23U
#define H264_ENCODE_DEFAULT_OPEN_QP_P 26U

typedef struct
{
	h264_enc_param_t enc_param;       /* vcenc per-instance params + opaque handle */
	bool encoder_inited;              /* set after vcenc_h264_init + _open succeed */
	bool force_idr;                   /* request next frame as IDR */

	beken_thread_t thread;            /* Encoder worker thread */
	beken_semaphore_t sem;            /* Thread startup/shutdown handshake */
	beken_semaphore_t enc_start_sem;  /* Kick off one encode */
	uint32_t enc_start_flag;          /* Encode start flag */
	uint32_t enc_start_first;         /* First-frame flag */
	uint32_t enc_line_cnt;            /* Encoded line counter */
	uint8_t enc_status;               /* Encode state */

	beken_timer_t debug_timer;        /* Debug timer */
	uint32_t debug_time_ms;           /* Debug interval (ms) */
	h264_encode_debug_info_t debug_info;      /* Accumulated stats from frame_done_cb */
	h264_encode_debug_info_t last_debug_info; /* Snapshot for periodic delta logging */

	/*
	 * Per-frame staged I/O: filled by the encoder thread before kick and
	 * mutated by frame_done_cb to preinstall the next output buffer. Owned
	 * by the controller itself (no separate pointer/struct indirection).
	 */
	uint32_t pending_in_buf;
	uint32_t pending_in_lines;
	uint32_t pending_out_buf;
	uint32_t pending_out_size;
	bool pending_valid;               /* thread loop currently has a frame queued */
	beken_semaphore_t enc_done_sem;   /* Posted when one frame encode completes */

	bk_flexa_bond_t *bond;            /* Bond callbacks (unused in frame mode but kept for symmetry) */

	bk_h264_encode_frame_config_t config;   /* User configuration */
	bk_h264_encode_ctlr_t ops;        /* Control vtable */
} private_h264_encode_frame_ctlr_t;

typedef struct
{
	h264_enc_param_t enc_param;
	bool encoder_inited;
	bool force_idr;

	beken_thread_t thread;
	beken_semaphore_t sem;
	beken_semaphore_t enc_start_sem;
	uint32_t enc_start_flag;
	uint32_t enc_start_first;
	uint32_t enc_line_cnt;
	uint8_t enc_status;

	beken_timer_t debug_timer;
	uint32_t debug_time_ms;
	h264_encode_debug_info_t debug_info;
	h264_encode_debug_info_t last_debug_info;

	uint32_t pending_in_buf;
	uint32_t pending_in_lines;
	uint32_t pending_out_buf;
	uint32_t pending_out_size;
	bool pending_valid;
	/*
	 * Last sync error from vcenc_h264_encode_frame; published in the
	 * worker dispatch callback so the encoder thread can decide whether to
	 * report BK_OK / BK_FAIL to the bond after the per-frame semaphore.
	 */
	uint32_t encode_result;
	beken_semaphore_t enc_done_sem;

	bk_flexa_bond_t *bond;

	bk_h264_encode_hw_flexa_config_t config;
	bk_h264_encode_ctlr_t ops;
} private_h264_encode_hw_flexa_ctlr_t;

typedef struct
{
	h264_enc_param_t enc_param;
	bool encoder_inited;
	bool force_idr;

	beken_thread_t thread;
	beken_semaphore_t sem;
	beken_semaphore_t enc_start_sem;
	uint32_t enc_start_flag;
	uint32_t enc_start_first;
	uint32_t enc_line_cnt;
	uint8_t enc_status;

	beken_timer_t debug_timer;
	uint32_t debug_time_ms;
	h264_encode_debug_info_t debug_info;
	h264_encode_debug_info_t last_debug_info;

	uint32_t pending_in_buf;
	uint32_t pending_in_lines;
	uint32_t pending_out_buf;
	uint32_t pending_out_size;
	bool pending_valid;
	beken_semaphore_t enc_done_sem;

	bk_flexa_bond_t *bond;
	/** Flexa blocks per frame (height / 16); used to clamp rd_blocks. */
	uint32_t flexa_blocks_per_frame;
	/** Last rd_blocks reported to bond/flexa_done; used to detect line counter wrap. */
	uint32_t last_flexa_line;

	bk_h264_encode_sw_flexa_config_t config;
	bk_h264_encode_ctlr_t ops;
} private_h264_encode_sw_flexa_ctlr_t;

avdk_err_t bk_h264_encode_frame_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_frame_config_t *config);
avdk_err_t bk_h264_encode_hw_flexa_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_hw_flexa_config_t *config);
avdk_err_t bk_h264_encode_sw_flexa_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_sw_flexa_config_t *config);

#ifdef __cplusplus
}
#endif
