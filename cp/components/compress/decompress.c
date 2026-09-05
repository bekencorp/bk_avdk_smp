// Copyright 2022-2023 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>

#if (CONFIG_LZMA1900 | CONFIG_LZMA2301)
#include "lzma.h"
#endif

#include "decompress.h"

/*
 * .lzma file layout used by beken packager `lzma e`:
 *   [0..4]   5-byte LZMA props
 *   [5..12]  8-byte little-endian uncompressed size
 *   [13..]   compressed payload
 *
 * Beken LzmaDec.h defines LZMA_PROPS_SIZE as 13 (= 5 + 8), and
 * bk_lzma_decode() does: src_size = src_len - 13; stream = src + 13.
 * Do NOT redefine LZMA_PROPS_SIZE here.
 */
#define LZMA_FILE_SIZE_OFF        5u   /* uncompressed size field in .lzma */

uint8_t *decompress_in_memory(uint8_t *src, uint8_t *dest, uint32_t src_len,
			      uint32_t dest_cap, decompress_type_t decompressor)
{
	if (src == NULL || dest == NULL || dest_cap == 0) {
		DEC_LOGE("decompress: null arg\r\n");
		return NULL;
	}

	switch (decompressor) {
	case DECOMPRESS_BY_LZMA:
#if (CONFIG_LZMA1900 | CONFIG_LZMA2301)
	{
		/* Must be > LZMA_PROPS_SIZE(13); else bk_lzma_decode underflows. */
		if (src_len <= LZMA_PROPS_SIZE) {
			DEC_LOGE("decompress: src_len=%u too small (need > %u)\r\n",
				 (unsigned)src_len, (unsigned)LZMA_PROPS_SIZE);
			return NULL;
		}

		/* Packer stores real size in the first 4 of the 8-byte field. */
		uint32_t claimed = (uint32_t)src[LZMA_FILE_SIZE_OFF]
				 | ((uint32_t)src[LZMA_FILE_SIZE_OFF + 1] << 8)
				 | ((uint32_t)src[LZMA_FILE_SIZE_OFF + 2] << 16)
				 | ((uint32_t)src[LZMA_FILE_SIZE_OFF + 3] << 24);

		if (claimed == 0 || claimed > dest_cap) {
			DEC_LOGE("lzma uncomp_size=%u exceeds dest_cap=%u\r\n",
				 (unsigned)claimed, (unsigned)dest_cap);
			return NULL;
		}

		int ret = bk_lzma_decode(src, src_len, dest, claimed);
		if (ret) {
			DEC_LOGE("lzma error code : %d \r\n", ret);
			return NULL;
		}

		return dest;
	}
#endif
		break;

	default:
		DEC_LOGE("unknown decompressor !!! \r\n");
		break;
	}

	return NULL;
}
