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

#include <components/avdk_utils/avdk_error.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	HW_DECODER_TYPE_JPEG,
	HW_DECODER_TYPE_H264,
	HW_DECODER_TYPE_PP,   /* standalone PP (memory-in/memory-out), shares VCDec HW */
	HW_DECODER_TYPE_MAX,
} hw_decoder_type_t;

typedef enum {
	HW_DECODER_MSG_DECODE,
	HW_DECODER_MSG_CONFIG,
	HW_DECODER_MSG_RESET,
	HW_DECODER_MSG_EXIT,
} hw_decoder_msg_type_t;

typedef avdk_err_t (*hw_decoder_msg_cb_t)(void *param);

typedef struct {
	hw_decoder_type_t decoder_type;
	hw_decoder_msg_type_t type;
	hw_decoder_msg_cb_t callback;
	void *param;
	beken_semaphore_t *sem;
} hw_decoder_msg_t;

avdk_err_t hw_decoder_register(hw_decoder_type_t type, void *decoder_id);
avdk_err_t hw_decoder_unregister(void *decoder_id);
avdk_err_t hw_decoder_send_msg(hw_decoder_msg_t *msg, uintptr_t timeout);

#ifdef __cplusplus
}
#endif
