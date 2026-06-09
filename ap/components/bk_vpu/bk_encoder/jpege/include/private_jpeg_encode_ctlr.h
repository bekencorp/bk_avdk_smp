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

#include "components/bk_encode/bk_jpeg_encode_types.h"
#include "bk_flexa_bond_types.h"
#include "modules/vcenc/vcenc_jpeg_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bk_jpeg_encode_frame_config_t config;
	vcenc_handle               handle;
	vcenc_jpeg_frame_config_t  frame_cfg;
	vcenc_ret_e last_ret;
	uint8_t opened;

	beken_thread_t thread;
	beken_semaphore_t sem;
	beken_semaphore_t enc_start_sem;
	beken_semaphore_t enc_done_sem;
	uint8_t enc_status;
	bk_jpeg_encode_input_t pending_input;

	bk_jpeg_encode_ctlr_t ops;
} private_jpeg_encode_frame_ctlr_t;

typedef struct {
	bk_jpeg_encode_hw_flexa_config_t config;
	vcenc_handle               handle;
	vcenc_jpeg_frame_config_t  frame_cfg;
	vcenc_ret_e last_ret;
	uint8_t opened;

	beken_thread_t thread;
	beken_semaphore_t sem;
	beken_semaphore_t enc_start_sem;
	beken_semaphore_t enc_done_sem;
	uint8_t enc_status;
	uint32_t encode_result;
	bk_flexa_bond_t *bond;

	bk_jpeg_encode_ctlr_t ops;
} private_jpeg_encode_hw_flexa_ctlr_t;

typedef struct {
	bk_jpeg_encode_sw_flexa_config_t config;
	vcenc_handle               handle;
	vcenc_jpeg_frame_config_t  frame_cfg;
	vcenc_ret_e last_ret;
	uint8_t opened;

	beken_thread_t thread;
	beken_semaphore_t sem;
	beken_semaphore_t enc_start_sem;
	beken_semaphore_t enc_done_sem;
	uint8_t enc_status;
	uint32_t encode_result;
	bk_flexa_bond_t *bond;
	uint32_t flexa_blocks_per_frame;
	uint32_t last_flexa_line;

	bk_jpeg_encode_ctlr_t ops;
} private_jpeg_encode_sw_flexa_ctlr_t;

avdk_err_t bk_jpeg_encode_hw_flexa_ctlr_new(bk_jpeg_encode_ctlr_handle_t *handle,
					    bk_jpeg_encode_hw_flexa_config_t *config);
avdk_err_t bk_jpeg_encode_sw_flexa_ctlr_new(bk_jpeg_encode_ctlr_handle_t *handle,
					    bk_jpeg_encode_sw_flexa_config_t *config);

#ifdef __cplusplus
}
#endif
