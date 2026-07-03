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

#include <components/bk_gpu_types.h>

#include <bk_list.h>

#include <driver/hpdma.h>
#include <driver/hal/hal_hpdma_types.h>

#include <bk_flexa_bond_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    vg_lite_buffer_t src_buf;
    vg_lite_buffer_t dst_buf;
    vg_lite_matrix_t matrix;
    uint16_t input_width;
    uint16_t input_height;
    uint16_t output_width;
    uint16_t output_height;
    uint16_t flexa_index;
    uint16_t read_lines;
    uint16_t dst_buf_idx;
    uint32_t need_lines;
    hpdma_id_t gdma;
    void *link_dma_list_table;
    uintptr_t buffers[2];
    uintptr_t pingpong_raw_addr;
    uint8_t *dpu_frame_buffers;
    /* Cached values for performance */
    uint32_t output_width_x_flexa_lines;     /* output_width * FLEXA_LINES */
    uint32_t flexa_lines_x_4;                 /* FLEXA_LINES * BYTES_PER_PIXEL_BGRA */
    uint32_t input_width_x_height;            /* input_width * input_height */
    float scale_x;                            /* output_width / input_width */
    float scale_y;                            /* output_height / input_height */

    uint8_t draw_enable;
    vg_lite_path_t draw_path;
    vg_lite_matrix_t draw_matrix;
    beken_mutex_t draw_mutex;
    beken_semaphore_t transfer_sem;
} gpu_flex_data_t;

typedef struct
{
    beken_thread_t flexa_thd;
    uint32_t line_cnt;
    uint32_t line_frame_seq;
    uint32_t active_frame_seq;
    uint32_t line_err_flag;

    beken_semaphore_t gpu_process_sem;
    beken_semaphore_t gpu_start_sem;

    volatile bool flexa_stop;
    bool flexa_frame_active;
    uint8_t *gpu_contiguous_buffer;

    bool blit_enable;
    void *display_blit_buffer;
    bk_gpu_blit_config_t display_blit_config;
    void *update_blit_buffer;
    bk_gpu_blit_config_t update_blit_config;
    beken_mutex_t blit_mutex;
    beken_mutex_t gpu_mutex;

    bk_gpu_ctlr_config_t config;
    gpu_flex_data_t flex;
    bk_gpu_ctlr_t ops;

    LIST_HEADER_T draw_cmd_set_list;
    bk_flexa_bond_t *bond;
} gpu_vn_ctlr_t;

#ifdef __cplusplus
}
#endif

