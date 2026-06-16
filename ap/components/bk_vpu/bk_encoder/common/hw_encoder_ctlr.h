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
	HW_ENCODER_TYPE_H264,
	HW_ENCODER_TYPE_JPEG,
	HW_ENCODER_TYPE_MAX,
} hw_encoder_type_t;

typedef enum {
    HW_ENCODER_MSG_ENCODE,
    HW_ENCODER_MSG_CONFIG,
    HW_ENCODER_MSG_RESET,
    HW_ENCODER_MSG_EXIT,
} hw_encoder_msg_type_t;

// Hardware encoder message callback
typedef avdk_err_t (*hw_encoder_msg_cb_t)(void *param);

// Hardware encoder message
typedef struct {
	hw_encoder_msg_type_t type;
	hw_encoder_type_t encoder_type;
	hw_encoder_msg_cb_t callback;
	void *param;
	beken_semaphore_t *sem;
} hw_encoder_msg_t;

/**
 * @brief Register an encoder with the hardware encoder controller.
 *
 * @param type Encoder type (H.264 or JPEG).
 * @param encoder_id Opaque client handle used to identify this registration.
 *
 * @return avdk_err_t
 */
avdk_err_t hw_encoder_register(hw_encoder_type_t type, void *encoder_id);

/**
 * @brief Unregister an encoder from the hardware encoder controller.
 *
 * @param encoder_id Same handle passed to hw_encoder_register().
 *
 * @return avdk_err_t
 */
avdk_err_t hw_encoder_unregister(void *encoder_id);

/**
 * @brief Post a message to the hardware encoder worker task.
 *
 * @param msg Message payload.
 * @param timeout Queue push timeout.
 *
 * @return avdk_err_t
 */
avdk_err_t hw_encoder_send_msg(hw_encoder_msg_t *msg, uintptr_t timeout);

#ifdef __cplusplus
}
#endif

