#pragma once

#include "vcdec_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VCDEC_H264_FRAME_UNKNOWN = 0,
	VCDEC_H264_FRAME_IDR,
	VCDEC_H264_FRAME_I,
	VCDEC_H264_FRAME_P,
} vcdec_h264_frame_type_t;

typedef struct {
	uint32_t width;
	uint32_t height;
	uint8_t *input_stream;
	uint32_t input_stream_len;
	vcdec_h264_frame_type_t frame_type;
	uint8_t is_reference;
	uint8_t nal_ref_idc;
} vcdec_h264_info_t;

typedef struct {
	uint8_t *input_stream;
	uint32_t input_stream_len;
	uint8_t *output_buffer;
	uint32_t output_size;
	uint16_t segment_height;
	uint8_t segment_number;
} vcdec_h264_decode_config_t;

#ifdef __cplusplus
}
#endif
