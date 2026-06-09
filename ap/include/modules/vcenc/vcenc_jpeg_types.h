#pragma once

/**
 * @file vcenc_jpeg_types.h
 * @brief JPEG-encoder-specific public types.
 *
 * Holds the JPEG-only init configuration (geometry + flexa line-buffer setup)
 * and the per-frame encode configuration (input/output buffer pointers,
 * quality). The opaque session handle (@ref vcenc_handle) is produced by
 * vcenc_jpeg_init() and consumed by every other API entry. Common encoder
 * types (return codes, mode/input enums, callbacks, vcenc_config_t) live in
 * vcenc_types.h.
 */

#include <stdint.h>
#include "vcenc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief JPEG init configuration.
 *
 * One-shot configuration handed to vcenc_jpeg_init() together with the common
 * @ref vcenc_config_t. The geometry given here is the picture size; the
 * flexa fields below are only meaningful when the @ref vcenc_config_t mode is
 * VCENC_HW_SLICE_MODE or VCENC_SW_SLICE_MODE.
 */
typedef struct vcenc_jpeg_config_t {
	uint16_t      width;
	uint16_t      height;
	vcenc_input_e in_type;
	/* Flexa input line-buffer config; zero out in non-flexa modes. */
	uint32_t      input_linebuf_depth;
	uint32_t      input_linebuf_loopback_en;
	uint32_t      input_linebuf_hw_mode_en;
	uint32_t      amount_per_loopback;
} vcenc_jpeg_config_t;

/**
 * @brief Per-frame JPEG encode configuration.
 *
 * Filled by the caller for every vcenc_jpeg_encode_frame() call to describe
 * the input picture buffer, the output bitstream buffer and a per-frame
 * quality knob.
 */
typedef struct vcenc_jpeg_frame_config_t {
	uint16_t width;
	uint16_t height;
	uint32_t in_buffer;
	uint32_t in_lines;
	uint32_t out_buffer;
	uint32_t out_len;
	uint8_t  quality;          /* 0..10 -> EncJpeg QuantLuminance index */
	uint32_t linebuf_wr_cnt;   /* flexa SW-slice initial write counter */
} vcenc_jpeg_frame_config_t;

#ifdef __cplusplus
}
#endif
