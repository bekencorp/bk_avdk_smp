// H.264 1920x1080 long-P-chain test stream data

#pragma once

#include <stdint.h>

/*
 * Video track of recordh2641080.mp4 (1920x1080 display, 1920x1088 coded,
 * Main profile, no B-frames), converted from AVCC to Annex-B.
 * Embedded size: 1661655 bytes, 195 frames (7 IDR + 188 P), GOP 30.
 *
 * Chosen over the 58-frame stream because its 29-P runs keep referencing
 * earlier P frames, which is what stresses reference-frame reads.
 */
extern const uint8_t  h264_decode_stream_1920x1080_gop30[];
extern const uint32_t h264_decode_stream_1920x1080_gop30_bytes;
