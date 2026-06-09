#pragma once

/**
 * @file vcenc_jpeg_types.h
 * @brief JPEG-encoder-specific public types.
 *
 * Holds types that only apply to the JPEG encoder front-end (per-frame
 * encode parameter). Common encoder types (return codes, mode/input enums,
 * callbacks) live in vcenc_types.h.
 */

#include <stdint.h>
#include "vcenc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-frame JPEG encode parameter.
 *
 * Filled by the caller before invoking vcenc_jpeg_encode_frame() and reused
 * across calls within an open/close session. The opaque @ref instance handle
 * is filled in by vcenc_jpeg_init() and released by vcenc_jpeg_deinit().
 */
typedef struct jpeg_enc_param_t {
	vcenc_handle instance;
	uint16_t width;
	uint16_t height;
	vcenc_mode_e enc_mode;
	vcenc_input_e in_type;
	uint32_t in_buffer;
	uint32_t in_lines;
	uint32_t out_buffer;
	uint32_t out_len;
	uint8_t  quality; /* 0..10 -> EncJpeg QuantLuminance index */
	uint32_t input_linebuf_depth;
	uint32_t input_linebuf_loopback_en;
	uint32_t input_linebuf_hw_mode_en;
	uint32_t amount_per_loopback;
	uint32_t linebuf_wr_cnt;
	vcenc_frame_done_cb frame_done_cb;
	vcenc_slice_done_cb slice_done_cb;
	uint32_t args;
} jpeg_enc_param_t;

#ifdef __cplusplus
}
#endif
