// H.264 1280x720 test stream data

#pragma once

#include <stdint.h>

/*
 * Two independent 1280x720 elementary streams are embedded simultaneously, each
 * with its own symbol, so all decode tests can run against either one without
 * rebuilding the firmware:
 *   - h264_decode_stream_1280x720_1i30p: baseline 1 IDR + 30 P frames, no
 *     B-frames (provided by h264_decode_stream_1280x720.c).
 *   - h264_decode_stream_1280x720_ibbp : Main profile stream WITH B-frames
 *     (raw_1280x720_30f_IBBP.h264, provided by
 *     h264_decode_stream_1280x720_ibbp.c).
 *
 * Both streams are linked into flash at the same time; pick the desired stream
 * at run time via the h264_decode test stream id / CLI argument.
 */
extern const uint8_t  h264_decode_stream_1280x720_1i30p[];
extern const uint32_t h264_decode_stream_1280x720_1i30p_bytes;

extern const uint8_t  h264_decode_stream_1280x720_ibbp[];
extern const uint32_t h264_decode_stream_1280x720_ibbp_bytes;
