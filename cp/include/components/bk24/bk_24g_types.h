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

/**
 * @brief bk24 APIs Version 1.0
 * @addtogroup bk24_api_v1 New bk24 API group
 * @{
 */

/**
 * @brief for bk24 send callback.
 *
 * This callback reports the TX result after bk24_send_data or bk24_send_data_no_ack.
 *
 * @param
 *    - is_timeout: send result, 0 means send done, 1 means send timeout
 *
 * @return
 * - void
 */
typedef void (*bk24_send_cb_t)(uint8_t is_timeout);

/**
 * @brief for bk24 recv callback.
 *
 * @param
 *    - buf: payload
 *    - len: buf's len
 *    - pipe: pipe number
 *
 * @return
 * - void
 */
typedef void (*bk24_recv_cb_t)(uint8_t *buf, uint16_t len, uint8_t is_ack, uint8_t pipe);

/**
 * @}
 */
