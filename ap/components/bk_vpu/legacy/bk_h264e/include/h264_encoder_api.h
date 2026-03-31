/**
 **************************************************************************************
 * @file    h264_encoder_api.h
 * @brief   API wrapper for H265 encoder
 *
 * @author  Aixing.Li
 * @version V1.0.0
 *
 * &copy; 2023 BEKEN Corporation Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __H264_ENCODER_API_H__
#define __H264_ENCODER_API_H__

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

typedef enum
{
	VCENC_FLEXA_MODE_NONE = 0,
	VCENC_FLEXA_MODE_SOFTWARE,
	VCENC_FLEXA_MODE_HARDWARE,
}VCENC_FLEXA_MODE;

typedef enum
{
	VCENC_OUT_IFRAME,
	VCENC_OUT_PFRAME,
	VCENC_OUT_BFRAME,
	VCENC_OUT_HEADER,
	VCENC_OUT_ENDING,
}VCENC_OUT_TYPE;

typedef enum
{
	FLEXA_STREAM_ID_Y = 0x14,
	FLEXA_STREAM_ID_CB = 0x15,
	FLEXA_STREAM_ID_CR = 0x16,
}FLEXA_STREAM_ID;

typedef void     (*VCEncStartCallback)(void);
typedef void     (*VCEncOutCallback)(uint8_t*, uint32_t, uint32_t);
typedef uint32_t (*VCEncFlexaDoneCallback)(uint8_t*, uint8_t*, uint8_t*);

void vcenc_memalloc_register(void* (*pmalloc)(size_t), void (*pfree)(void*));
void vcenc_read_yuv420_planar_frame(uint32_t dest, uint32_t picture, uint32_t width, uint32_t height, uint32_t alignment);
void vcenc_read_yuv420_semi_planar_frame(uint32_t dest, uint32_t picture, uint32_t width, uint32_t height, uint32_t alignment);
void vcenc_flexa_input_linebuf_wrcnt_set(void* handle, uint32_t wrcnt);
void vcenc_asic_encode_start_callback_set(void* handle, VCEncStartCallback scb);

int32_t h264_encoder_init(void** handle, uint32_t width, uint32_t height, uint32_t flexaMode, VCEncFlexaDoneCallback fcb, VCEncOutCallback ocb);
int32_t h264_encoder_deinit(void* handle);
int32_t h264_encoder_pps_data_get(void* handle, uint8_t** data, uint32_t* size);
int32_t h264_encoder_encode(void* handle, uint32_t picBuf, uint32_t picLines, uint32_t codingType, uint32_t outBuf, uint32_t outSize);

int32_t jpeg_encoder_init(void** handle, uint32_t width, uint32_t height, uint32_t flexaMode, VCEncFlexaDoneCallback fcb, VCEncOutCallback ocb);
int32_t jpeg_encoder_deinit(void* handle);
int32_t jpeg_encoder_encode(void* handle, uint32_t picBuf, uint32_t picLines, uint32_t outBuf, uint32_t outSize);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__H264_ENCODER_API_H__
