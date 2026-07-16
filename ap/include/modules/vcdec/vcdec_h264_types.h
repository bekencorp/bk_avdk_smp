#pragma once

/**
 * @file vcdec_h264_types.h
 * @brief H264-decoder-specific public types.
 *
 * Holds types that only apply to the H264 decoder front-end (e.g. parsed
 * frame info, per-frame decode configuration). Common decoder types
 * (return codes, handle, callbacks, flexa mode, init config) live in
 * vcdec_types.h.
 */

#include <stdint.h>
#include "vcdec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief H264 frame / slice type reported by the parser.
 */
typedef enum {
	VCDEC_H264_FRAME_UNKNOWN = 0,
	VCDEC_H264_FRAME_IDR,
	VCDEC_H264_FRAME_I,
	VCDEC_H264_FRAME_P,
	VCDEC_H264_FRAME_B,
} vcdec_h264_frame_type_t;

/**
 * @brief Parsed H264 frame info.
 *
 * Filled by vcdec_h264_get_info() and carries metadata extracted from the
 * current access unit so that the caller can drive its frame management.
 */
typedef struct vcdec_h264_info_t {
	uint32_t width;
	uint32_t height;
	uint8_t *input_stream;
	uint32_t input_stream_len;
	vcdec_h264_frame_type_t frame_type;
	uint8_t  is_reference;
	uint8_t  nal_ref_idc;
} vcdec_h264_info_t;

/**
 * @brief Per-frame H264 decode configuration.
 *
 * Filled by the caller for every vcdec_h264_decode_frame() invocation.
 * Describes the input bitstream buffer, the output frame buffer and the
 * tiled/segmented output layout used by the post-processor.
 */
/** @deprecated alias; use vcdec_pp_out_format_e. */
typedef vcdec_pp_out_format_e vcdec_h264_out_format_e;

typedef vcdec_pp_osd_config_t vcdec_h264_osd_config_t;

typedef struct vcdec_h264_decode_config_t {
	uint8_t *input_stream;      /* H264 bitstream buffer */
	uint32_t input_stream_len;  /* H264 bitstream length in bytes */
	uint8_t *output_buffer;     /* Output frame buffer */
	uint32_t output_size;       /* Output frame buffer size in bytes */
	uint16_t out_width;         /* PP output width; 0 = match coded width */
	uint16_t out_height;        /* PP output height; 0 = match coded height */
	vcdec_h264_out_format_e out_format; /* NV12 zero-copy or PP RGB output */
	uint16_t segment_height;    /* Output ring-buffer segment height (in MB rows) */
	uint8_t  segment_number;    /* Number of output ring-buffer segments */
	vcdec_h264_osd_config_t osd[2]; /* Optional PP alpha blend overlays */
} vcdec_h264_decode_config_t;

#ifdef __cplusplus
}
#endif
