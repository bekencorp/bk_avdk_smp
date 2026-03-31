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

#include "modules/vcenc/vcenc_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	H264_ENC_OUT_IFRAME,
	H264_ENC_OUT_PFRAME,
	H264_ENC_OUT_BFRAME,
	H264_ENC_OUT_HEADER,
	H264_ENC_OUT_ENDING,
} h264_enc_out_type_t;

typedef enum
{
	H264_ENC_FLEXA_MODE_NONE = 0,
	H264_ENC_FLEXA_MODE_SOFTWARE,
	H264_ENC_FLEXA_MODE_HARDWARE,
} h264_enc_flexa_mode_t;

typedef enum {
  /** YYYY... UUUU... VVVV... */
  H264_ENC_YUV420_PLANAR = 0,
  /** YYYY... UVUVUV... */
  H264_ENC_YUV420_SEMIPLANAR = 1,
  /** YYYY... VUVUVU... */
  H264_ENC_YUV420_SEMIPLANAR_VU = 2,
  H264_ENC_PIXFMT_FORMAT_MAX
} h264_enc_format_t;

typedef enum
{
    H264_ENC_TAG_INIT = (0xA55A),
    H264_ENC_TAG_DEINIT = (0x5AA5),
} h264_enc_tag_t;

typedef enum
{
    BK_H264_ENCODER_STATUS_STOP = 0,
    BK_H264_ENCODER_STATUS_START = 1,
} bk_h264_encoder_frame_status_t;


typedef uint32_t (*enc_flexa_done_callback)(uint8_t *, uint8_t *, uint8_t *, uint32_t);
typedef void (*enc_out_callback)(void *, uint32_t, uint32_t, uint32_t, uint32_t);

typedef void * h264_encoder_handle_t;


typedef struct
{
    uint32_t max_i_frame_size;
    uint32_t max_p_frame_size;
    uint32_t last_i_frame_size;
    uint32_t last_p_frame_size;
    uint32_t all_frame_size;
    uint32_t all_frame_count;
    uint32_t dec_frame_err_cnt;
} enc_h264_debug_t;

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
    enc_out_callback encOutCallback;
    enc_h264_debug_t debug_info;
    uint32_t force_idr;
} enc_buf_t;

typedef struct
{
    enc_flexa_done_callback fcb;
    enc_out_callback ocb;
    uint32_t param;
} h264_encoder_callback_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t idr_interval;
    uint32_t flexa_mode;
    uint32_t input_type;
    uint32_t buf_cnt;
} h264_encoder_config_t;

typedef struct
{
    uint16_t           width;
    uint16_t           height;
    uint8_t            qp_min;
    uint8_t            qp_max;
    uint8_t            fps;
    uint32_t           bitrate;
} h264_encoder_rate_ctrl_t;

typedef struct
{
    uint32_t enc_tag;
    h264_enc_param_t param;
    enc_h264_debug_t debug_info;
    uint8_t module;
    uint32_t force_idr;
    h264_encoder_config_t config;
    h264_encoder_callback_t callback;
} h264_encoder_context;

typedef struct
{
    uint32_t pic_buf;
    uint32_t pic_lines;
    uint32_t coding_type;
    uint32_t out_buf;
    uint32_t out_size;
} h264_encoder_parameters_t;

bk_err_t h264e_init(h264_encoder_handle_t* handle, h264_encoder_config_t* in_config);
bk_err_t h264e_deinit(h264_encoder_handle_t* handle);
bk_err_t h264e_register_callback(h264_encoder_handle_t* handle, h264_encoder_callback_t *callback);
bk_err_t h264e_deregister_callback(h264_encoder_handle_t* handle);
bk_err_t h264e_open(h264_encoder_handle_t* handle);
bk_err_t h264e_close(h264_encoder_handle_t* handle);
bk_err_t h264e_start_encode(h264_encoder_handle_t* handle, h264_encoder_parameters_t* para);
bk_err_t h264e_stop_encode(h264_encoder_handle_t* handle);
bk_err_t h264e_set_force_idr(h264_encoder_handle_t* handle);

bk_err_t h264e_set_rate_ctrl(h264_encoder_handle_t* handle, h264_encoder_rate_ctrl_t* rate_ctrl);
bk_err_t h264e_get_rate_ctrl(h264_encoder_handle_t* handle, h264_encoder_rate_ctrl_t* rate_ctrl);

bk_err_t h264e_get_debug_info(h264_encoder_handle_t* handle, enc_h264_debug_t **debug_info);
bk_err_t h264e_flexa_input_linebuf_wrcnt_set(h264_encoder_handle_t* handle, uint32_t wrcnt);

uint32_t h264e_get_flexa_mode(h264_encoder_handle_t* handle);
uint32_t h264e_get_encoded_lines(void);

bk_err_t h264e_osd_config(h264_encoder_handle_t* handle, uint32_t index, void* buffer, uint32_t format, uint8_t alpha, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif

