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

#include "encoder_core.h"

typedef void * jpeg_encoder_handle_t;

typedef JpegEncOut jpeg_enc_out_t;
typedef JpegEncIn jpeg_enc_in_t;
typedef JpegEncInst jpeg_enc_inst_t;
typedef JpegEncCfg jpeg_enc_config_t;

typedef enum
{
	JPEG_ENC_FLEXA_MODE_NONE = 0,
	JPEG_ENC_FLEXA_MODE_SOFTWARE,
	JPEG_ENC_FLEXA_MODE_HARDWARE,
} jpeg_enc_flexa_mode_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t flexa_mode;
    uint32_t input_type;
} jpeg_encoder_config_t;

typedef struct
{
    uint32_t max_i_frame_size;
    uint32_t max_p_frame_size;
    uint32_t last_i_frame_size;
    uint32_t last_p_frame_size;
    uint32_t all_frame_size;
    uint32_t all_frame_count;
} jpeg_enc_debug_t;

typedef struct
{
    uint32_t  width;
    uint32_t  height;
    uint32_t  alignment;
    uint32_t  encPicYSize;
    uint32_t  encPicUVSize;
    uint32_t  encOutPPSize;
    uint8_t   encOutPPS[32];
    uint32_t  encOutBuf;
    uint32_t  encOutSize;
    VCEncGopPicConfig  gopPicCfg[MAX_GOP_PIC_CONFIG_NUM];
    uint32_t           lineBufMode;
    inputLineBufferCfg lineBufCfg;
    enc_out_callback encOutCallback;
    jpeg_enc_debug_t debug_info;
    uint32_t force_idr;
} jpeg_enc_buf_t;

typedef struct
{
    enc_flexa_done_callback fcb;
    enc_out_callback ocb;
    uint32_t param;
} jpeg_encoder_callback_t;

typedef struct
{
    jpeg_enc_inst_t enc_handle;
    jpeg_enc_buf_t enc_buf;
    jpeg_enc_in_t   enc_in;
    jpeg_enc_out_t  enc_out;
    uint32_t  err_code;
    jpeg_enc_config_t enc_config;
    uint8_t module;
    jpeg_encoder_config_t config;
    jpeg_encoder_callback_t callback;
} jpeg_encoder_context;

typedef struct
{
    uint32_t pic_buf;
    uint32_t pic_lines;
    uint32_t out_buf;
    uint32_t out_size;
} jpeg_encoder_parameters_t;

int32_t jpege_init(jpeg_encoder_handle_t* handle, jpeg_encoder_config_t* in_config);
int32_t jpege_deinit(jpeg_encoder_handle_t* handle);
bk_err_t jpege_register_callback(jpeg_encoder_handle_t* handle, jpeg_encoder_callback_t *callback);
bk_err_t jpege_deregister_callback(jpeg_encoder_handle_t* handle);
int32_t jpege_start_encode(jpeg_encoder_handle_t* handle, jpeg_encoder_parameters_t *para);

