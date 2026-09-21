// H.264 1920x1080 test stream data

#pragma once

#include <stdint.h>

/*
 * Same stream as h264d_gpu_display_example:
 * record-2026-06-11T08-57-35-981Z.h264, Main, no B-frames,
 * 4 I + 54 P (58 frames), 528017 bytes.
 */
extern const uint8_t  h264_decode_stream_1920x1080[];
extern const uint32_t h264_decode_stream_1920x1080_bytes;
