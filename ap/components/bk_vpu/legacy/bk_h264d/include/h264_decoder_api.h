// Copyright 2024-2025 Beken
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

#ifndef __H264_DECODER_API_H__
#define __H264_DECODER_API_H__

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

typedef enum
{
	VCDEC_FLEXA_MODE_NONE = 0,
	VCDEC_FLEXA_MODE_FLEXA,
    VCDEC_FLEXA_MODE_SLICE,
}VCDEC_FLEXA_MODE;

typedef enum
{
	VCDEC_OUT_IFRAME,
	VCDEC_OUT_PFRAME,
	VCDEC_OUT_BFRAME,
	VCDEC_OUT_INFO = 6,
	VCDEC_OUT_JFRAME = VCDEC_OUT_IFRAME,
}VCDEC_OUT_TYPE;

typedef void (*VCDecOutCallback)(uint8_t* y, uint8_t* cb, uint8_t* cr, uint32_t width, uint32_t height, uint32_t type);
typedef void (*VCDecFlexaDoneCallback)(uint32_t wrCnt);
typedef void (*VCDecHWReadyCallback)(uint32_t status);

void vcdec_memalloc_register(void* (*pmalloc)(size_t), void (*pfree)(void*));
void vcdec_flexa_input_linebuf_rdcnt_set(void* handle, uint32_t rdcnt);

int32_t h264_decoder_init(void** handle, uint32_t flexaMode, VCDecFlexaDoneCallback fcb, VCDecHWReadyCallback rcb, VCDecOutCallback ocb);
int32_t h264_decoder_deinit(void* handle);
int32_t h264_decoder_pp_config(void* handle, uint32_t outWidth, uint32_t outHeight, uint32_t rotation);
int32_t h264_decoder_osd_config(void* handle, uint32_t index, void* buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
int32_t h264_decoder_decode(void* handle, uint8_t* inBuf, uint32_t inSize, uint8_t* outBuf, uint32_t* usedSize);

int32_t jpeg_decoder_init(void** handle, uint32_t flexaMode, VCDecFlexaDoneCallback fcb, VCDecHWReadyCallback rcb, VCDecOutCallback ocb);
int32_t jpeg_decoder_deinit(void* handle);
int32_t jpeg_decoder_pp_config(void* handle, uint32_t outWidth, uint32_t outHeight, uint32_t rotation);
int32_t jpeg_decoder_osd_config(void* handle, uint32_t index, void* buffer, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
int32_t jpeg_decoder_decode(void* handle, uint8_t* inBuf, uint32_t inSize, uint8_t* outBuf, uint32_t outSize);
int32_t jpeg_decoder_reset(void* handle);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__H264_DECODER_API_H__
