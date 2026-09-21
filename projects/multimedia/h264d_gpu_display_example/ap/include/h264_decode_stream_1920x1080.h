// H.264 1920x1080 test stream data

#pragma once

#include <stdint.h>

/*
 * Embedded 1920x1080 Annex-B H.264 elementary stream (no B-frames).
 * Source: record-2026-06-11T08-57-35-981Z.h264
 * Main profile, 4 I + 54 P (58 frames), ~25 fps.
 */
extern const uint8_t  h264_decode_stream_1920x1080[];
extern const uint32_t h264_decode_stream_1920x1080_bytes;
