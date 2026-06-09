#pragma once

/**
 * @file vcdec_jpeg_types.h
 * @brief JPEG-decoder-specific public types.
 *
 * Holds types that only apply to the JPEG decoder front-end (e.g. per-frame
 * decode configuration). Common decoder types (return codes, handle,
 * callbacks, flexa mode, init config) live in vcdec_types.h.
 */

#include <stdint.h>
#include "vcdec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-frame JPEG decode configuration.
 *
 * Filled by the caller for every vcdec_jpeg_decode_frame() invocation.
 * Describes the input bitstream buffer, the output frame buffer and the
 * tiled/segmented output layout used by the post-processor.
 */
typedef struct vcdec_jpeg_decode_config_t {
	uint8_t *input_stream;      /* JPEG bitstream buffer */
	uint32_t input_stream_len;  /* JPEG bitstream length in bytes */
	uint8_t *output_buffer;     /* Output frame buffer */
	uint32_t output_size;       /* Output frame buffer size in bytes */
	uint16_t width;             /* Output frame width in pixels */
	uint16_t height;            /* Output frame height in pixels */
	uint16_t segment_height;    /* Output ring-buffer segment height (in MB rows) */
	uint8_t  segment_number;    /* Number of output ring-buffer segments */
} vcdec_jpeg_decode_config_t;

#ifdef __cplusplus
}
#endif
