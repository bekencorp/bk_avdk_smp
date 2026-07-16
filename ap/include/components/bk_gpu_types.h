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

#include <avdk_check.h>
#include <common/avdk_pixel_types.h>

#include <modules/vg_lite_gpu/vg_lite.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BK_GPU_IOCTL_SET_FLEXA_LINES_READY,
    BK_GPU_IOCTL_SET_NOTIFY,
    BK_GPU_IOCTL_REGISTER_BOND,
    BK_GPU_IOCTL_UNREGISTER_BOND,
    BK_GPU_IOCTL_FLEXA_ADDR_MAPPING,
    BK_GPU_IOCTL_FLEXA_ADDR_UNMAPPING,
    BK_GPU_IOCTL_LOCK,
    BK_GPU_IOCTL_UNLOCK,
    BK_GPU_IOCTL_SET_FLEXA_EVENT_READY,
} bk_gpu_ioctl_cmd_t;

typedef struct
{
    uint32_t frame_seq;
    uint32_t line_cnt;
} bk_gpu_flexa_event_t;

typedef struct
{
    uint16_t tess_width;
    uint16_t tess_height;
    uint16_t src_width;
    uint16_t src_height;
    uint16_t dst_width;
    uint16_t dst_height;
    bk_pixel_format_t src_format;
    bk_pixel_format_t dst_format;
    uint16_t rotate_degree;
    bool horizontal_mirror;
    uint8_t *src_buffer;
    uint8_t *dst_buffer;

    bool scale;
    bool compress;
    bool flexa;
    uint8_t flexa_lines;
    uint8_t flexa_buff_cnt;

    void *(*frame_malloc)(uint32_t size);
    avdk_err_t (*frame_free)(void *ptr);
    void (*flexa_line_done)(uint32_t done_lines, void *args);
    void *flexa_line_done_args;
    void (*frame_done)(void *frame, uint32_t frame_size, void *args);
    void *frame_done_args;
} bk_gpu_ctlr_config_t;

typedef struct
{
    uint16_t src_x;
    uint16_t src_y;
    uint16_t src_width;
    uint16_t src_height;
    bk_pixel_format_t src_format;
    uint16_t dst_x;
    uint16_t dst_y;
    uint16_t rotate_degree;
    void *args;
    void (*free)(void *frame, void *args);
} bk_gpu_blit_config_t;

typedef struct
{
    uint8_t *cmd;
    void *data;
    uint32_t size;
    uint8_t alpha;
    uint32_t color; /* ARGB888  bit 24~31: alpha, bit16~23: blue, bit8~15: green, bit0~7: red */
} bk_gpu_draw_path_set_t;

typedef struct bk_gpu_ctlr_t *bk_gpu_ctlr_handle_t;
typedef struct bk_gpu_ctlr_t bk_gpu_ctlr_t;

struct bk_gpu_ctlr_t
{
    avdk_err_t (*init)(bk_gpu_ctlr_t *controller);
    avdk_err_t (*open)(bk_gpu_ctlr_t *controller);
    avdk_err_t (*close)(bk_gpu_ctlr_t *controller);
    avdk_err_t (*deinit)(bk_gpu_ctlr_t *controller);
    avdk_err_t (*ioctl)(bk_gpu_ctlr_t *controller, uint32_t cmd, void *args);
    avdk_err_t (*del)(bk_gpu_ctlr_t *controller);
    avdk_err_t (*draw_path_clear)(bk_gpu_ctlr_t *controller);
    avdk_err_t (*draw_path_build)(bk_gpu_ctlr_t *controller, bk_gpu_draw_path_set_t *path_set);
    avdk_err_t (*blit_set)(bk_gpu_ctlr_t *controller, void *src_buffer, bk_gpu_blit_config_t *blit_config);
    avdk_err_t (*blit_clear)(bk_gpu_ctlr_t *controller);
};


#define DRAW_RECTANGLE_PATH_BUILD(_pcmd, _pdat, _xmin, _ymin, _xmax, _ymax)      \
do {                                                                             \
    *(_pcmd)++ = VLC_OP_MOVE;                                                    \
    *(_pcmd)++ = VLC_OP_LINE;                                                    \
    *(_pcmd)++ = VLC_OP_LINE;                                                    \
    *(_pcmd)++ = VLC_OP_LINE;                                                    \
    *(_pcmd)++ = VLC_OP_LINE;                                                    \
    *(_pcmd)++ = VLC_OP_CLOSE;                                                   \
    *(_pdat)++ = (typeof(*(_pdat)))_xmin;                                        \
    *(_pdat)++ = (typeof(*(_pdat)))_ymin;                                        \
    *(_pdat)++ = (typeof(*(_pdat)))_xmax;                                        \
    *(_pdat)++ = (typeof(*(_pdat)))_ymin;                                        \
    *(_pdat)++ = (typeof(*(_pdat)))_xmax;                                        \
    *(_pdat)++ = (typeof(*(_pdat)))_ymax;                                        \
    *(_pdat)++ = (typeof(*(_pdat)))_xmin;                                        \
    *(_pdat)++ = (typeof(*(_pdat)))_ymax;                                        \
    *(_pdat)++ = (typeof(*(_pdat)))_xmin;                                        \
    *(_pdat)++ = (typeof(*(_pdat)))_ymin;                                        \
} while (0);


#ifdef __cplusplus
}
#endif

