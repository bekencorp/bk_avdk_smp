#pragma once

/**
 * @file vcenc_h264_types.h
 * @brief H.264-encoder-specific public types.
 *
 * Holds the per-frame encode parameter that the caller fills in and passes
 * to the H.264 encoder. Profile / level enums and SYNTAX_PROFILE_H264_* are
 * module-internal and live in vcenc_h264_types_private.h; legacy users of
 * those names go through the standalone h264e library's hevcencapi.h which
 * ships its own independent copy of those identifiers. Common encoder types
 * (return codes, mode/input enums, callbacks, RC, NR) live in vcenc_types.h.
 */

#include <stdint.h>
#include "vcenc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-frame H.264 encode parameter, filled by the caller for each
 * vcenc_h264_encode_frame() call. Carries an opaque @ref vcenc_handle that
 * the encoder fills in during vcenc_h264_init().
 */
typedef struct h264_enc_param_t {
	vcenc_handle instance;
	uint16_t width;
	uint16_t height;
	vcenc_mode_e enc_mode; /* 0:frame mode 1:hw slice mode 2:sw slice mode */
	uint32_t slice_count;  /* only valid for hw/sw slice mode */
	uint32_t in_buffer;
	uint32_t in_lines;
	vcenc_input_e in_type;
	uint32_t out_buffer;
	uint32_t out_len;
	uint32_t idr_interval;
	uint32_t update_flag;
	uint32_t force_idr_flag;
	vcenc_frame_done_cb frame_done_cb; /* callback of frame or sps/pps done */
	vcenc_slice_done_cb slice_done_cb; /* callback of 16 lines done */
	uint32_t args;
} h264_enc_param_t;

#ifdef __cplusplus
}
#endif
