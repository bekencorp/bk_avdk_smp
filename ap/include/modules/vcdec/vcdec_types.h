#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VCDEC_OK = 0,
	VCDEC_FRAME_READY = (1 << 0),
	VCDEC_SLICE_READY  = (1 << 1),
	VCDEC_ERROR = -1,
	VCDEC_NULL_ARGUMENT = -2,
	VCDEC_INVALID_ARGUMENT = -3,
	VCDEC_MEMORY_ERROR = -4,
	VCDEC_HW_TIMEOUT = -5,
	VCDEC_HW_ERROR = -6,
	VCDEC_HW_BUS_ERROR = -7,
	VCDEC_SW_ABORT = -8,
} vcdec_ret_e;

typedef enum {
	VCDEC_FLEXA_MODE_NONE = 0,
	VCDEC_FLEXA_MODE_FLEXA,
} vcdec_flexa_mode_e;

typedef void *vcdec_handle;
typedef void (*vcdec_frame_done_cb)(int status, void *args);
typedef void (*vcdec_flexa_done_cb)(uint32_t line_cnt, void *args);

typedef struct vcdec_decode_config_t {
	uint8_t *input_stream;
	uint32_t input_stream_len;
	uint8_t *output_buffer;
	uint32_t output_size;
	uint16_t width;
	uint16_t height;
	uint16_t segment_height;
	uint8_t segment_number;
} vcdec_decode_config_t;

typedef struct vcdec_config_t {
	vcdec_flexa_mode_e mode;
	uint32_t timeout_ms;
	vcdec_frame_done_cb frame_done_cb;
	vcdec_flexa_done_cb flexa_done_cb;
	void *args;
} vcdec_config_t;

#ifdef __cplusplus
}
#endif