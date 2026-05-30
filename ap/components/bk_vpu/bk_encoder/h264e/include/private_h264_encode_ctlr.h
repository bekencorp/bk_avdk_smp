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
    void *h264e_handler;              /* H.264 encoder driver handle */
    beken_thread_t thread;            /* Encoder worker thread */
    beken_semaphore_t sem;            /* Thread startup handshake */
    beken_semaphore_t enc_start_sem;  /* Kick off one encode */
    uint32_t enc_start_flag;          /* Encode start flag */
    uint32_t enc_start_first;         /* First-frame flag */
    uint32_t enc_line_cnt;            /* Encoded line counter */
    uint8_t enc_status;               /* Encode state */
    beken_timer_t debug_timer;        /* Debug timer */
    uint32_t debug_time_ms;           /* Debug interval (ms) */
    enc_h264_debug_t last_debug_info; /* Last debug snapshot */
    h264_encoder_parameters_t *h264_encoder_param;  /* Encoder parameters */
    beken_semaphore_t enc_done_sem;   /* Posted when one frame encode completes (h264e_end_cb). */

    bk_flexa_bond_t *bond;            /* Bond callbacks */

    bk_h264_encode_frame_config_t config;   /* User configuration */
    bk_h264_encode_ctlr_t ops;        /* Control vtable */
} private_h264_encode_frame_ctlr_t;

typedef struct
{
    void *h264e_handler;              /* H.264 encoder driver handle */
    beken_thread_t thread;            /* Encoder worker thread */
    beken_semaphore_t sem;            /* Thread startup handshake */
    beken_semaphore_t enc_start_sem;  /* Kick off one encode */
    uint32_t enc_start_flag;          /* Encode start flag */
    uint32_t enc_start_first;         /* First-frame flag */
    uint32_t enc_line_cnt;            /* Encoded line counter */
    uint8_t enc_status;               /* Encode state */
    beken_timer_t debug_timer;        /* Debug timer */
    uint32_t debug_time_ms;           /* Debug interval (ms) */
    enc_h264_debug_t last_debug_info; /* Last debug snapshot */
    h264_encoder_parameters_t *h264_encoder_param;  /* Encoder parameters */
    uint32_t encode_result;            /* Last encode result */
    beken_semaphore_t enc_done_sem;   /* Posted when one frame encode completes (h264e_end_cb). */

    bk_flexa_bond_t *bond;            /* Bond callbacks */

    bk_h264_encode_hw_flexa_config_t config;   /* User configuration */
    bk_h264_encode_ctlr_t ops;        /* Control vtable */
} private_h264_encode_hw_flexa_ctlr_t;

typedef struct
{
    void *h264e_handler;              /* H.264 encoder driver handle */
    beken_thread_t thread;            /* Encoder worker thread */
    beken_semaphore_t sem;            /* Thread startup handshake */
    beken_semaphore_t enc_start_sem;  /* Kick off one encode */
    uint32_t enc_start_flag;          /* Encode start flag */
    uint32_t enc_start_first;         /* First-frame flag */
    uint32_t enc_line_cnt;            /* Encoded line counter */
    uint8_t enc_status;               /* Encode state */
    beken_timer_t debug_timer;        /* Debug timer */
    uint32_t debug_time_ms;           /* Debug interval (ms) */
    enc_h264_debug_t last_debug_info; /* Last debug snapshot */
    h264_encoder_parameters_t *h264_encoder_param;  /* Encoder parameters */
    uint32_t encode_result;            /* Last encode result, set by h264e_end_cb */
    beken_semaphore_t enc_done_sem;   /* Posted when one frame encode completes (h264e_end_cb). */

    bk_flexa_bond_t *bond;            /* Bond callbacks */
    /** Flexa blocks per frame (height / 16); used to clamp rd_blocks */
    uint32_t flexa_blocks_per_frame;
    /** Last rd_blocks from flexa_done; used to detect line counter wrap */
    uint32_t last_flexa_line;

    bk_h264_encode_sw_flexa_config_t config;   /* User configuration */
    bk_h264_encode_ctlr_t ops;        /* Control vtable */
} private_h264_encode_sw_flexa_ctlr_t;

avdk_err_t bk_h264_encode_frame_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_frame_config_t *config);
avdk_err_t bk_h264_encode_hw_flexa_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_hw_flexa_config_t *config);
avdk_err_t bk_h264_encode_sw_flexa_ctlr_new(bk_h264_encode_ctlr_handle_t *handle, bk_h264_encode_sw_flexa_config_t *config);

#ifdef __cplusplus
}
#endif

