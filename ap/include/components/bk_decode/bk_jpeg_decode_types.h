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

#include "os/os.h"
#include "components/avdk_utils/avdk_error.h"
#ifdef __cplusplus
extern "C" {
#endif

/** JPEG decode result callback: (status, args). */
typedef void (*bk_jpeg_decode_frame_done_cb)(int status, void *args);

/** Flexa done callback: (wr_ptr, args). */
typedef void (*bk_jpeg_decode_flexa_done_cb)(uint32_t wr_ptr, void *args);

typedef enum {
    BK_JPEG_DECODE_IOCTL_GET_INFO,                    /*!< Get image info see bk_jpeg_decode_img_info_t*/    
    BK_JPEG_DECODE_IOCTL_ABORT,                       /*!< Abort decode */
    BK_JPEG_DECODE_IOCTL_PORT_SET_RD_PTR,             /*!< Set read pointer see bk_jpeg_decode_port_rd_t*/
    BK_JPEG_DECODE_IOCTL_REGISTER_BOND,               /*!< Register bond should be ptr for the bond*/
    BK_JPEG_DECODE_IOCTL_UNREGISTER_BOND,             /*!< Unregister bond should be ptr for the bond*/
    BK_JPEG_DECODE_IOCTL_FLEXA_NOTIFY_PORT_DONE,      /*!< Notify port done should be ptr for the port*/
} bk_jpeg_decode_ioctl_cmd_t;

#define BK_JPEG_DECODE_RD_PORT_MAX (2U)

typedef enum {
    BK_JPEG_DECODE_RD_PORT_GPU = 0,
    BK_JPEG_DECODE_RD_PORT_H264E = 1,
} bk_jpeg_decode_rd_port_id_t;

/** this is for bond*/
typedef struct {
    void *port_ptr;
    uint32_t rd_blocks;
} bk_jpeg_decode_port_rd_t;

typedef enum {
    BK_JPEG_DECODE_FLEXA_MODE_NONE = 0,
    BK_JPEG_DECODE_FLEXA_MODE_FLEXA,
} bk_jpeg_decode_flexa_mode_t;

typedef struct {
    uint32_t timeout_ms;                                /*!< timeout in milliseconds */
    uint16_t out_width;                                 /*!< output width */
    uint16_t out_height;                                /*!< output height */
    uint32_t out_format;                                /*!< output format */
    bk_jpeg_decode_frame_done_cb frame_done_cb;         /*!< frame done callback */
    void *frame_done_args;                              /*!< frame done arguments */
} bk_jpeg_decode_frame_config_t;

typedef struct {
    uint32_t timeout_ms;                                /*!< timeout in milliseconds */
    uint16_t out_width;                                 /*!< output width */
    uint16_t out_height;                                /*!< output height */
    uint32_t out_format;                                /*!< output format */
    uint16_t segment_height;                            /*!< segment height */
    uint8_t segment_number;                             /*!< segment number */
    bk_jpeg_decode_frame_done_cb frame_done_cb;         /*!< frame done callback */
    void *frame_done_args;                              /*!< frame done arguments */
    bk_jpeg_decode_flexa_done_cb flexa_done_cb;         /*!< flexa done callback */
    void *flexa_done_args;                              /*!< flexa done arguments */
} bk_jpeg_decode_flexa_config_t;

typedef struct {
    uint8_t *stream;                                    /*!< input stream */
    uint32_t stream_len;                                /*!< input stream length */
    uint8_t *out_buffer;                                /*!< output buffer */
    uint32_t out_buffer_size;                           /*!< output buffer size */
} bk_jpeg_decode_input_t;

typedef enum
{
    BK_JPEG_DECODE_IMG_FMT_ERR,           /*!< Invalid or unsupported image format */
    BK_JPEG_DECODE_IMG_FMT_YUV444,        /*!< YUV 4:4:4 format - full chroma resolution, no subsampling */
    BK_JPEG_DECODE_IMG_FMT_YUV422,        /*!< YUV 4:2:2 format - horizontal chroma subsampling by 2:1 */
    BK_JPEG_DECODE_IMG_FMT_YUV420,        /*!< YUV 4:2:0 format - horizontal and vertical chroma subsampling by 2:1 */
    BK_JPEG_DECODE_IMG_FMT_YUV400,        /*!< YUV 4:0:0 format - grayscale image with only luma component */
} bk_jpeg_decode_img_fmt_t;

typedef struct bk_jpeg_decode_img_info
{
    uint8_t *input_stream;              /*!< Input: JPEG stream buffer */
    uint32_t input_stream_length;       /*!< Input: JPEG stream length */
    uint32_t width;                     /*!< Output: Image width in pixels */
    uint32_t height;                    /*!< Output: Image height in pixels */
    bk_jpeg_decode_img_fmt_t format;    /*!< Output: Image format (see bk_jpeg_decode_img_fmt_t) */
} bk_jpeg_decode_img_info_t;

typedef struct bk_jpeg_decode_ctlr_t *bk_jpeg_decode_ctlr_handle_t;


typedef struct bk_jpeg_decode_ctlr_t bk_jpeg_decode_ctlr_t;
struct bk_jpeg_decode_ctlr_t {
    avdk_err_t (*init)(bk_jpeg_decode_ctlr_t *controller);
    avdk_err_t (*open)(bk_jpeg_decode_ctlr_t *controller);
    avdk_err_t (*decode_frame)(bk_jpeg_decode_ctlr_t *controller, bk_jpeg_decode_input_t *input);
    avdk_err_t (*close)(bk_jpeg_decode_ctlr_t *controller);
    avdk_err_t (*deinit)(bk_jpeg_decode_ctlr_t *controller);
    avdk_err_t (*ioctl)(bk_jpeg_decode_ctlr_t *controller, uint32_t cmd, void *arg);
    avdk_err_t (*del)(bk_jpeg_decode_ctlr_t *controller);
};

#define DEFAULT_JPEG_DECODE_FLEXA_CONFIG {    \
    .timeout_ms = 1000U,                      \
    .out_width = 1280,                        \
    .out_height = 720,                        \
    .out_format = BK_PIXEL_FORMAT_NV12,       \
    .segment_height = 16,                     \
    .segment_number = 2,                      \
    .frame_done_cb = NULL,                    \
    .frame_done_args = NULL,                  \
    .flexa_done_cb = NULL,                    \
    .frame_done_args = NULL,                  \
}

#define DEFAULT_JPEG_DECODE_FRAME_CONFIG {    \
    .timeout_ms = 1000U,                      \
    .out_width = 1280,                        \
    .out_height = 720,                        \
    .out_format = BK_PIXEL_FORMAT_NV12,       \
    .frame_done_cb = NULL,                    \
    .frame_done_args = NULL,                  \
}

#ifdef __cplusplus
}
#endif
