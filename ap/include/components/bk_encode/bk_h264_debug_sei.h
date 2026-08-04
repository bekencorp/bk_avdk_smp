// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * H.264 debug SEI (Supplemental Enhancement Information) - v2.
 *
 * This module ONLY provides pure functions to build a debug SEI NAL; it does
 * NOT touch the encoder pipeline. The application, after obtaining one encoded
 * H.264 frame, calls these APIs right before sending it out, appends the SEI
 * NAL to the frame and transmits it together. This keeps the CRC/assembly work
 * off the encoder callback thread.
 *
 * The SEI carries a fixed per-frame snapshot so a receiver can detect transport
 * anomalies WITHOUT decoding the stream:
 *   - sequence     -> frame loss (gap) / reorder (rollback)
 *   - payload_len  -> truncation / concatenation
 *   - payload_crc  -> corruption / tampering (CRC32 over the raw stream)
 *
 * On-wire SEI layout (Annex-B):
 *   [00 00 00 01][0x06][0x05][payloadSize][UUID(16B)][payload][0x80]
 * The [UUID + payload] region is emulation-prevention encoded (0x03 insertion)
 * so a standard decoder can safely skip it.
 *
 * All multi-byte fields are little-endian (native to the Cortex-M AP).
 */

/** SEI structure version. Bump when the payload layout changes. */
#define BK_H264_SEI_VERSION      2U

/** Magic 'BK59' stored little-endian (bytes: 0x42 'B', 0x4B 'K', 0x35 '5', 0x39 '9'). */
#define BK_H264_SEI_MAGIC        0x39354B42U

/** Length of the user_data_unregistered UUID prefix, per H.264 spec. */
#define BK_H264_SEI_UUID_LEN     16U

/**
 * Max bytes a generated SEI NAL can occupy (worst-case emulation prevention on
 * UUID+payload plus start code/header/trailing, rounded up). Handy for sizing a
 * stack buffer when using bk_h264_debug_sei_generate().
 */
#define BK_H264_SEI_MAX_LEN      64U

/**
 * @brief Per-frame debug payload carried inside the SEI user data (v2).
 *
 * Packed and little-endian so the receiver can parse it directly.
 * sizeof == 17 bytes.
 */
typedef struct __attribute__((packed))
{
	uint32_t magic;       /**< BK_H264_SEI_MAGIC, secondary identifier */
	uint8_t  version;     /**< BK_H264_SEI_VERSION */
	uint32_t sequence;    /**< monotonically increasing frame index */
	uint32_t payload_len; /**< raw H.264 length of THIS frame, excluding SEI */
	uint32_t payload_crc; /**< CRC32 (zlib/IEEE) over the raw H.264 stream */
} bk_h264_sei_debug_t;

/**
 * @brief Compute a standard CRC32 (zlib/IEEE 802.3, reflected, poly 0xEDB88320).
 *
 * Matches Python binascii.crc32 / zlib.crc32 so the receiver can verify.
 *
 * @param data   input buffer
 * @param length number of bytes
 * @return CRC32 value
 */
uint32_t bk_h264_sei_crc32(const uint8_t *data, uint32_t length);

/**
 * @brief Build a standalone debug SEI NAL for a raw H.264 frame (does NOT touch
 *        the frame).
 *
 * Computes CRC32 over frame[0 .. frame_len), fills payload_len = frame_len and
 * the given sequence, applies emulation prevention, and writes the full Annex-B
 * SEI NAL to out[].
 *
 * @param frame     raw H.264 stream of this frame
 * @param frame_len raw stream length (excluding SEI)
 * @param sequence  frame index (from the encoder), for loss/reorder detection
 * @param out       output buffer for the SEI NAL bytes
 * @param out_cap   capacity of out in bytes (>= BK_H264_SEI_MAX_LEN recommended)
 * @return number of bytes written to out; 0 on invalid args or insufficient cap.
 */
uint32_t bk_h264_debug_sei_generate(const uint8_t *frame, uint32_t frame_len,
				    uint32_t sequence,
				    uint8_t *out, uint32_t out_cap);

/**
 * @brief Convenience: generate a debug SEI NAL and append it at the tail of buf
 *        in place.
 *
 * buf holds the raw stream in [0, frame_len); capacity is the total buffer size.
 * The caller must ensure buf is cache-coherent before the CRC read and must
 * flush the returned length afterwards.
 *
 * @param buf       frame output buffer (raw stream at [0, frame_len))
 * @param frame_len current encoded length (raw stream, excluding SEI)
 * @param capacity  total capacity of buf in bytes
 * @param sequence  frame index (from the encoder)
 * @return new total length including the SEI; equals frame_len if it was
 *         skipped (invalid args or not enough room).
 */
uint32_t bk_h264_debug_sei_append(uint8_t *buf, uint32_t frame_len,
				  uint32_t capacity, uint32_t sequence);

#ifdef __cplusplus
}
#endif
