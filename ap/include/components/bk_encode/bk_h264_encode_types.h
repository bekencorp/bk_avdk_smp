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

#include "components/avdk_utils/avdk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	FLEXA_STREAM_ID_Y = 0x14,
	FLEXA_STREAM_ID_CB = 0x15,
	FLEXA_STREAM_ID_CR = 0x16,
} flexa_stream_id_t;

// ioctl 命令定义
typedef enum {
    BK_H264_ENCODE_IOCTL_DEBUG_START,    // 启动调试, arg: uint32_t* (时间间隔ms)
    BK_H264_ENCODE_IOCTL_DEBUG_STOP,     // 停止调试, arg: NULL
    BK_H264_ENCODE_IOCTL_SET_PARAM,      // 设置参数
    BK_H264_ENCODE_IOCTL_SET_FLEXA_LINES_READY,  // Software Flexa: set input line buffer write count, arg: uint32_t*
    BK_H264_ENCODE_IOCTL_SET_FRAME_READY,      // Software Flexa: set frame done, arg: uint32_t*
    BK_H264_ENCODE_IOCTL_REGISTER_BOND,      // arg: bk_h264_encode_sw_flexa_bond_ops_t*（见 bk_encoder/h264e/include），或 NULL
    BK_H264_ENCODE_IOCTL_UNREGISTER_BOND,      // arg: bk_h264_encode_sw_flexa_bond_ops_t*（见 bk_encoder/h264e/include），或 NULL
    BK_H264_ENCODE_IOCTL_STOP_ENCODE,      // arg: uint32_t*
} bk_h264_encode_ioctl_cmd_t;

typedef enum
{
	BK_H264_ENCODE_FLEXA_MODE_NONE = 0,
	BK_H264_ENCODE_FLEXA_MODE_SOFTWARE,
	BK_H264_ENCODE_FLEXA_MODE_HARDWARE,
} bk_h264_encode_flexa_mode_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t pframe_number;
    uint32_t input_format;
    uint32_t input_flexa_cnt;
    uint32_t input_buf;
    uint32_t input_size;
    void *(*outbuf_malloc)(uint32_t outbuf_size, void *args);
    void *outbuf_malloc_args;
    uint32_t (*outbuf_complete)(void *outbuf, uint32_t status, void *args);
    void *outbuf_complete_args;
} bk_h264_encode_frame_config_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t pframe_number;
    uint32_t input_format;
    uint32_t input_flexa_cnt;
    uint32_t input_buf;
    uint32_t input_size;
    void *(*outbuf_malloc)(uint32_t outbuf_size, void *args);
    void *outbuf_malloc_args;
    uint32_t (*outbuf_complete)(void *outbuf, uint32_t status, void *args);
    void *outbuf_complete_args;
} bk_h264_encode_hw_flexa_config_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t pframe_number;
    uint32_t input_format;
    uint32_t input_flexa_cnt;
    uint32_t input_buf;
    uint32_t input_size;
    void *(*outbuf_malloc)(uint32_t outbuf_size, void *args);
    void *outbuf_malloc_args;
    uint32_t (*outbuf_complete)(void *outbuf, uint32_t status, void *args);
    void *outbuf_complete_args;
    void (*flexa_done)(uint32_t rd_blocks, void *arg);
    void *flexa_done_arg;
} bk_h264_encode_sw_flexa_config_t;

typedef struct bk_h264_encode_ctlr_t *bk_h264_encode_ctlr_handle_t;
typedef struct bk_h264_encode_ctlr_t bk_h264_encode_ctlr_t;

struct bk_h264_encode_ctlr_t
{
    avdk_err_t (*init)(bk_h264_encode_ctlr_t *controller);
    avdk_err_t (*open)(bk_h264_encode_ctlr_t *controller);
    avdk_err_t (*encode)(bk_h264_encode_ctlr_t *controller);
    avdk_err_t (*close)(bk_h264_encode_ctlr_t *controller);
    avdk_err_t (*deinit)(bk_h264_encode_ctlr_t *controller);
    avdk_err_t (*ioctl)(bk_h264_encode_ctlr_t *controller, uint32_t cmd, void *arg);
    avdk_err_t (*force_idr)(bk_h264_encode_ctlr_t *controller);
    avdk_err_t (*del)(bk_h264_encode_ctlr_t *controller);
};


#ifdef __cplusplus
}
#endif

