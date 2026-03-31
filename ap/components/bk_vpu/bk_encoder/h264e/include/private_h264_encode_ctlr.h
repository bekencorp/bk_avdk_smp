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

#include "components/bk_encode/bk_h264_encode_types.h"
#include "bk_flexa_bond_types.h"
#include "h264e_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	ENCODER_CORE_INITING,
	ENCODER_CORE_INITED,
	ENCODER_CORE_DEINITING,
	ENCODER_CORE_DEINITED,
} encoder_core_status_t;

typedef struct
{
    void *h264e_handler;              // H.264编码器句柄
    beken_thread_t thread;            // 编码线程
    beken_semaphore_t sem;            // 同步信号量
    beken_semaphore_t enc_start_sem;  // 编码启动信号量
    uint32_t enc_start_flag;          // 编码启动标志
    uint32_t enc_start_first;         // 首次编码标志
    uint32_t enc_line_cnt;            // 编码行计数
    uint8_t enc_status;               // 编码状态
    beken_timer_t debug_timer;        // 调试定时器
    uint32_t debug_time_ms;           // 调试时间间隔
    enc_h264_debug_t last_debug_info; // 上次调试信息
    h264_encoder_parameters_t *h264_encoder_param;  // 编码器参数指针
    beken_semaphore_t enc_done_sem;   // Signalled when one frame encode completes (in h264e_end_cb)
    uint32_t soft_flexa_pic_buf;      // Software FLEXA: pic_buf set at init from config.param
    uint32_t soft_flexa_pic_lines;    // Software FLEXA: pic_lines set at init from config.param

    bk_h264_encode_frame_config_t config;   // 编码器配置
    bk_h264_encode_ctlr_t ops;        // 操作接口
} private_h264_encode_frame_ctlr_t;

typedef struct
{
    void *h264e_handler;              // H.264编码器句柄
    beken_thread_t thread;            // 编码线程
    beken_semaphore_t sem;            // 同步信号量
    beken_semaphore_t enc_start_sem;  // 编码启动信号量
    uint32_t enc_start_flag;          // 编码启动标志
    uint32_t enc_start_first;         // 首次编码标志
    uint32_t enc_line_cnt;            // 编码行计数
    uint8_t enc_status;               // 编码状态
    beken_timer_t debug_timer;        // 调试定时器
    uint32_t debug_time_ms;           // 调试时间间隔
    enc_h264_debug_t last_debug_info; // 上次调试信息
    h264_encoder_parameters_t *h264_encoder_param;  // 编码器参数指针
    uint32_t encode_result;            // 编码结果
    beken_semaphore_t enc_done_sem;   // Signalled when one frame encode completes (in h264e_end_cb)
    uint32_t soft_flexa_pic_buf;      // Software FLEXA: pic_buf set at init from config.param
    uint32_t soft_flexa_pic_lines;    // Software FLEXA: pic_lines set at init from config.param

    bk_flexa_bond_t *bond;      // Bond operations

    bk_h264_encode_hw_flexa_config_t config;   // 编码器配置
    bk_h264_encode_ctlr_t ops;        // 操作接口
} private_h264_encode_hw_flexa_ctlr_t;

typedef struct
{
    void *h264e_handler;              // H.264编码器句柄
    beken_thread_t thread;            // 编码线程
    beken_semaphore_t sem;            // 同步信号量
    beken_semaphore_t enc_start_sem;  // 编码启动信号量
    uint32_t enc_start_flag;          // 编码启动标志
    uint32_t enc_start_first;         // 首次编码标志
    uint32_t enc_line_cnt;            // 编码行计数
    uint8_t enc_status;               // 编码状态
    beken_timer_t debug_timer;        // 调试定时器
    uint32_t debug_time_ms;           // 调试时间间隔
    enc_h264_debug_t last_debug_info; // 上次调试信息
    h264_encoder_parameters_t *h264_encoder_param;  // 编码器参数指针
    beken_semaphore_t enc_done_sem;   // Signalled when one frame encode completes (in h264e_end_cb)
    uint32_t soft_flexa_pic_buf;      // Software FLEXA: pic_buf set at init from config.param
    uint32_t soft_flexa_pic_lines;    // Software FLEXA: pic_lines set at init from config.param

    bk_flexa_bond_t *bond;      // Bond operations
    /** 每帧 Flexa 块数（height / 16），供 rd_blocks 钳位 */
    uint32_t flexa_blocks_per_frame;
    /** 上一档 flexa_done 给出的 rd_blocks，用于检测行回绕 */
    uint32_t last_flexa_line;

    bk_h264_encode_sw_flexa_config_t config;   // 编码器配置
    bk_h264_encode_ctlr_t ops;        // 操作接口
} private_h264_encode_sw_flexa_ctlr_t;

avdk_err_t bk_h264_encode_frame_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_frame_config_t *config);
avdk_err_t bk_h264_encode_hw_flexa_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_hw_flexa_config_t *config);
avdk_err_t bk_h264_encode_sw_flexa_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_sw_flexa_config_t *config);

#ifdef __cplusplus
}
#endif

