#pragma once

/**
 * @file vcdec_jpeg_types.h
 * @brief JPEG-decoder-specific public types.
 */

#include <stdint.h>
#include "vcdec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vcdec_jpeg_decode_config_t {
	uint8_t *input_stream;
	uint32_t input_stream_len;
	uint8_t *output_buffer;
	uint32_t output_size;
	uint16_t out_width;
	uint16_t out_height;
	vcdec_pp_out_format_e out_format;
	uint16_t segment_height;
	uint8_t  segment_number;
} vcdec_jpeg_decode_config_t;

#ifdef __cplusplus
}
#endif
