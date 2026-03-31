// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <os/os.h>
#include <components/avdk_utils/avdk_error.h>

typedef enum {
	BK_FLEXA_TYPE_H264E = 0,
	BK_FLEXA_TYPE_GPU = 1,
	BK_FLEXA_TYPE_ISP = 2,
	BK_FLEXA_TYPE_MJPEG = 3,
	BK_FLEXA_TYPE_H264D = 4,
} bk_flexa_bond_kind_t;

typedef struct bk_flexa_bond_config {
	void *bond;
	void *in_stream;
	void *out_stream;
	uint32_t set_sbi_flag;
	uint32_t flexa_sbi;
	beken_semaphore_t sem;
	bk_flexa_bond_kind_t in_stream_type;
	bk_flexa_bond_kind_t out_stream_type;
} bk_flexa_bond_config_t;

typedef struct bk_flexa_bond {
	uint32_t max_lines_per_frame;

	void *handle;
	uint32_t last_lines;
	void (*flexa_done)(uint32_t lines, void *arg);
	void (*frame_start)(void *arg);
	void (*frame_done)(uint32_t status, void *arg);

	void (*error)(uint32_t reason, void *arg);

	bk_flexa_bond_config_t *bond_config;
} bk_flexa_bond_t;

#ifdef __cplusplus
}
#endif
