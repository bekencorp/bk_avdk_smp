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

#include "bk_decode_pp_helper.h"

#include "common/avdk_pixel_types.h"

vcdec_pp_out_format_e bk_decode_pp_map_out_format(uint32_t fmt)
{
	switch (fmt) {
	case BK_PIXEL_FORMAT_RGB565:
		return VCDEC_PP_OUT_RGB565;
	case BK_PIXEL_FORMAT_RGB888:
		return VCDEC_PP_OUT_RGB888;
	default:
		return VCDEC_PP_OUT_NV12;
	}
}

uint32_t bk_decode_pp_output_size(uint32_t fmt, uint16_t width, uint16_t height)
{
	switch (fmt) {
	case BK_PIXEL_FORMAT_RGB565:
		return (uint32_t)width * (uint32_t)height * 2U;
	case BK_PIXEL_FORMAT_RGB888:
		return (uint32_t)width * (uint32_t)height * 4U;
	default:
		return (uint32_t)width * (uint32_t)height * 3U / 2U;
	}
}

uint32_t bk_decode_pp_flexa_rb_size(uint16_t width, uint16_t segment_height_mb,
				    uint8_t segment_num)
{
	const uint32_t seg_h = (uint32_t)segment_height_mb * 16U;

	return (uint32_t)width * seg_h * 3U / 2U * (uint32_t)segment_num;
}
