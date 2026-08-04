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

#include <os/mem.h>
#include "components/bk_encode/bk_h264_debug_sei.h"

/*
 * Fixed 16-byte UUID identifying a BK7259 debug SEI (v2):
 *   "BK7259-SEI-DBG" + {0x00, 0x02}
 * Receivers match this to distinguish our SEI from any other user data.
 */
static const uint8_t s_bk_h264_sei_uuid[BK_H264_SEI_UUID_LEN] = {
	0x42, 0x4B, 0x37, 0x32, 0x35, 0x39, 0x2D, 0x53, /* B K 7 2 5 9 - S */
	0x45, 0x49, 0x2D, 0x44, 0x42, 0x47, 0x00, 0x02, /* E I - D B G .. */
};

/* Lazily-initialized CRC32 table (zlib/IEEE, reflected poly 0xEDB88320). */
static uint32_t s_crc32_table[256];
static uint8_t  s_crc32_ready;

static void bk_h264_sei_crc32_build_table(void)
{
	for (uint32_t i = 0; i < 256U; i++) {
		uint32_t c = i;
		for (uint32_t k = 0; k < 8U; k++) {
			c = (c & 1U) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
		}
		s_crc32_table[i] = c;
	}
	s_crc32_ready = 1U;
}

uint32_t bk_h264_sei_crc32(const uint8_t *data, uint32_t length)
{
	uint32_t crc = 0xFFFFFFFFU;

	if (data == NULL) {
		return 0U;
	}
	if (!s_crc32_ready) {
		bk_h264_sei_crc32_build_table();
	}

	while (length-- > 0U) {
		crc = s_crc32_table[(crc ^ *data++) & 0xFFU] ^ (crc >> 8);
	}

	return crc ^ 0xFFFFFFFFU;
}

/*
 * Apply H.264 emulation prevention over an RBSP byte run: insert 0x03 before
 * any byte <= 0x03 that follows two consecutive 0x00 bytes. Returns the encoded
 * length written to dst. dst must hold up to n * 3 / 2 + 1 bytes worst case.
 */
static uint32_t bk_h264_sei_apply_ep(const uint8_t *src, uint32_t n, uint8_t *dst)
{
	uint32_t zeros = 0;
	uint32_t o = 0;

	for (uint32_t i = 0; i < n; i++) {
		uint8_t b = src[i];

		if (zeros >= 2U && b <= 0x03U) {
			dst[o++] = 0x03U;
			zeros = 0;
		}
		dst[o++] = b;
		zeros = (b == 0x00U) ? (zeros + 1U) : 0U;
	}

	return o;
}

uint32_t bk_h264_debug_sei_generate(const uint8_t *frame, uint32_t frame_len,
				    uint32_t sequence,
				    uint8_t *out, uint32_t out_cap)
{
	/* pre-EP RBSP: nal_header + payloadType + payloadSize + UUID + struct */
	uint8_t raw[3U + BK_H264_SEI_UUID_LEN + sizeof(bk_h264_sei_debug_t)];
	/* EP worst case grows by 50%; plus a small margin. */
	uint8_t ep[sizeof(raw) * 3U / 2U + 4U];
	/* Zero-init first: no uninitialized stack data leaks onto the wire. */
	bk_h264_sei_debug_t payload = {0};
	uint32_t r = 0;
	uint32_t ep_len;
	uint32_t total;
	const uint8_t payload_size = (uint8_t)(BK_H264_SEI_UUID_LEN + sizeof(bk_h264_sei_debug_t));

	if (frame == NULL || frame_len == 0U || out == NULL) {
		return 0U;
	}

	payload.magic       = BK_H264_SEI_MAGIC;
	payload.version     = (uint8_t)BK_H264_SEI_VERSION;
	payload.sequence    = sequence;
	payload.payload_len = frame_len;
	payload.payload_crc = bk_h264_sei_crc32(frame, frame_len);

	raw[r++] = 0x06U;         /* NAL header: forbidden=0, ref_idc=0, type=6 (SEI) */
	raw[r++] = 0x05U;         /* payloadType = 5 (user_data_unregistered) */
	raw[r++] = payload_size;  /* payloadSize (< 128, single byte) */
	os_memcpy(&raw[r], s_bk_h264_sei_uuid, BK_H264_SEI_UUID_LEN);
	r += BK_H264_SEI_UUID_LEN;
	os_memcpy(&raw[r], &payload, sizeof(payload));
	r += sizeof(payload);

	ep_len = bk_h264_sei_apply_ep(raw, r, ep);

	/* start code (4) + EP-encoded RBSP + rbsp_trailing_bits (1) */
	total = 4U + ep_len + 1U;
	if (total > out_cap) {
		return 0U;
	}

	out[0] = 0x00U;
	out[1] = 0x00U;
	out[2] = 0x00U;
	out[3] = 0x01U;
	os_memcpy(&out[4], ep, ep_len);
	out[4U + ep_len] = 0x80U; /* rbsp_trailing_bits */

	return total;
}

uint32_t bk_h264_debug_sei_append(uint8_t *buf, uint32_t frame_len,
				  uint32_t capacity, uint32_t sequence)
{
	uint32_t room;
	uint32_t sei_len;

	if (buf == NULL || frame_len == 0U || capacity <= frame_len) {
		return frame_len;
	}

	room = capacity - frame_len;
	sei_len = bk_h264_debug_sei_generate(buf, frame_len, sequence,
					     buf + frame_len, room);
	if (sei_len == 0U) {
		return frame_len; /* not enough room; leave the frame untouched */
	}

	return frame_len + sei_len;
}
