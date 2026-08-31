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

// IOCTL command definitions
typedef enum {
    BK_H264_ENCODE_IOCTL_DEBUG_START,    // Start debug logging, arg: uint32_t* (interval in ms)
    BK_H264_ENCODE_IOCTL_DEBUG_STOP,     // Stop debug logging, arg: NULL
    BK_H264_ENCODE_IOCTL_SET_GOP_FRAME_COUNT, // Set GOP frame count, arg: uint32_t*
    BK_H264_ENCODE_IOCTL_GET_GOP_FRAME_COUNT, // Get GOP frame count, arg: uint32_t*
    BK_H264_ENCODE_IOCTL_SET_FLEXA_LINES_READY,  // Software Flexa: set input line buffer write count, arg: uint32_t*
    BK_H264_ENCODE_IOCTL_SET_FRAME_READY,      // Software Flexa: set frame done, arg: uint32_t*
    BK_H264_ENCODE_IOCTL_REGISTER_BOND,      // arg: bk_h264_encode_sw_flexa_bond_ops_t* (see bk_encoder/h264e/include), or NULL
    BK_H264_ENCODE_IOCTL_UNREGISTER_BOND,      // arg: bk_h264_encode_sw_flexa_bond_ops_t* (see bk_encoder/h264e/include), or NULL
    BK_H264_ENCODE_IOCTL_STOP_ENCODE,      // arg: uint32_t*
    BK_H264_ENCODE_IOCTL_SET_RATE_CTRL,      // arg: bk_h264_encode_rate_ctrl_t*
    BK_H264_ENCODE_IOCTL_GET_RATE_CTRL,      // arg: bk_h264_encode_rate_ctrl_t*
    BK_H264_ENCODE_IOCTL_SET_OSD,            // arg: bk_h264_encode_osd_t*
    BK_H264_ENCODE_IOCTL_SET_INPUT_BUF,      // frame mode: set next-frame input buffer, arg: bk_h264_encode_input_t*
    BK_H264_ENCODE_IOCTL_GET_STREAM_INFO,    // frame mode: read last-frame stream stats, arg: bk_h264_encode_stream_info_t*
} bk_h264_encode_ioctl_cmd_t;

/**
 * @brief Per-frame input descriptor for frame-mode zero-copy encoding.
 *
 * Used with BK_H264_ENCODE_IOCTL_SET_INPUT_BUF.
 * The input must be contiguous NV12; the encoder derives the chroma-plane
 * address from the luma address and configured frame dimensions.
 */
typedef struct
{
    uint32_t input_buf;         /**< Luma (Y) plane base address of the frame to encode. */
    uint32_t input_size;        /**< Input frame size in bytes (or line count, per controller). */
} bk_h264_encode_input_t;

/**
 * @brief Per-frame encoder statistics used by adaptive pre-processing (e.g. NDR).
 *
 * Filled by BK_H264_ENCODE_IOCTL_GET_STREAM_INFO.
 */
typedef struct
{
    uint32_t intra_cu8_num;     /**< Number of intra-coded 8x8 CUs in the last frame (motion proxy). */
    uint32_t rd_cost;           /**< Aggregate rate-distortion cost of the last frame. */
} bk_h264_encode_stream_info_t;

typedef enum
{
    BK_H264_ENCODE_OVERLAY_FORMAT_ARGB8888 = 0,
    BK_H264_ENCODE_OVERLAY_FORMAT_NV12 = 1,
    BK_H264_ENCODE_OVERLAY_FORMAT_BITMAP = 2,
} bk_h264_encode_overlay_format_t;

typedef void (*bk_h264_encode_osd_buffer_free_cb_t)(void *buffer, void *free_arg);

typedef struct
{
    uint32_t index;     /* overlay slot index, 0..7 */
    void *buffer;       /* pixel buffer in memory visible to encoder */
    uint32_t format;    /* bk_h264_encode_overlay_format_t */
    uint8_t alpha;      /* global alpha for NV12/Bitmap; ignored for ARGB8888 */
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint8_t bitmap_y;   /* Bitmap foreground Y; valid when format == BITMAP */
    uint8_t bitmap_u;   /* Bitmap foreground U */
    uint8_t bitmap_v;   /* Bitmap foreground V */
    /*
     * Required when buffer != NULL. Encoder takes ownership after a successful
     * set_osd; caller must not modify the buffer afterwards. On overlay update
     * the previous buffer is released via this callback. All active buffers are
     * released when the encoder is closed.
     */
    bk_h264_encode_osd_buffer_free_cb_t buffer_free;
    void *free_arg;
} bk_h264_encode_osd_t;

typedef enum
{
	BK_H264_ENCODE_FLEXA_MODE_NONE = 0,
	BK_H264_ENCODE_FLEXA_MODE_SOFTWARE,
	BK_H264_ENCODE_FLEXA_MODE_HARDWARE,
} bk_h264_encode_flexa_mode_t;

typedef struct
{
    uint32_t bitrate;    /* non-zero: bitrate RC mode; zero: fixed QP mode */
    uint8_t qp_min_i;    /* I-frame min QP in RC mode, fixed I-frame QP in fixed mode */
    uint8_t qp_max_i;    /* I-frame max QP in RC mode; 0 keeps internal value, ignored in fixed mode */
    uint8_t qp_min_p;    /* P-frame min QP in RC mode, fixed P-frame QP in fixed mode */
    uint8_t qp_max_p;    /* P-frame max QP in RC mode; 0 keeps internal value, ignored in fixed mode */
} bk_h264_encode_rate_ctrl_t;

typedef struct
{
    void *outbuf;
    uint32_t length;
    uint32_t type;
    uint32_t status;
    uint32_t sequence;
    void *args;
} bk_h264_encode_outbuf_info_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t gop_frame_count;
    uint32_t input_format;
    uint32_t input_flexa_cnt;
    uint32_t input_buf;
    uint32_t input_size;
    void *(*outbuf_malloc)(uint32_t outbuf_size, void *args);
    void *outbuf_malloc_args;
    uint32_t (*outbuf_complete)(bk_h264_encode_outbuf_info_t *info);
    void *outbuf_complete_args;
} bk_h264_encode_frame_config_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t gop_frame_count;
    uint32_t input_format;
    uint32_t input_flexa_cnt;
    uint32_t input_buf;
    uint32_t input_size;
    void *(*outbuf_malloc)(uint32_t outbuf_size, void *args);
    void *outbuf_malloc_args;
    uint32_t (*outbuf_complete)(bk_h264_encode_outbuf_info_t *info);
    void *outbuf_complete_args;
} bk_h264_encode_hw_flexa_config_t;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t gop_frame_count;
    uint32_t input_format;
    uint32_t input_flexa_cnt;
    uint32_t input_buf;
    uint32_t input_size;
    void *(*outbuf_malloc)(uint32_t outbuf_size, void *args);
    void *outbuf_malloc_args;
    uint32_t (*outbuf_complete)(bk_h264_encode_outbuf_info_t *info);
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

