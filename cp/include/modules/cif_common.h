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

#include <common/sys_config.h>
#include <stdint.h>
#include <stdbool.h>
#include <common/bk_err.h>
#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bk_msg_hdr
{
    uint32_t rsv0;
    uint16_t cmd_id;
    uint16_t cmd_sn;
    uint16_t rsv1;
    uint16_t len;//msg payload length
};

typedef int(*cif_customer_msg_cb_t)(struct bk_msg_hdr *msg);

/**
 * @brief Send customer command confirmation
 *
 * @param data    the data to send
 * @param len    the length of the data
 * @param msg    the message header
 *
 * @return
 *    - BK_OK: succeed
 *    - otherwise: fail
 */
bk_err_t cif_send_customer_cmd_cfm(uint8_t *data, uint16_t len, struct bk_msg_hdr *msg);

/**
 * @brief Register customer message handler
 *
 * @param func    the function to handle the customer message
 *
 * @return
 *    - BK_OK: succeed
 *    - otherwise: fail
 */
void cif_register_customer_msg_handler(cif_customer_msg_cb_t func);

/**
 * @brief Send customer event
 *
 * @param data    the data to send
 * @param len    the length of the data
 *
 * @return
 *    - BK_OK: succeed
 *    - otherwise: fail
 */
bk_err_t cif_send_customer_event(uint8_t *data, uint16_t len);

/**
 * @brief Exit sleep
 *
 * @return
 *    - BK_OK: succeed
 *    - otherwise: fail
 */
bk_err_t cif_exit_sleep(void);

/**
 * @brief Power up host
 *
 * @return
 *    - BK_OK: succeed
 *    - otherwise: fail
 */
bk_err_t cif_power_up_host(void);

/**
 * @brief Power down host
 *
 * @return
 *    - BK_OK: succeed
 *    - otherwise: fail
 */
bk_err_t cif_power_down_host(void);

/**
 * @brief Start deep sleep
 *
 * @return
 *    - BK_OK: succeed
 *    - otherwise: fail
 */
void cif_start_deep_sleep(void);

/**
 * @brief Start low voltage sleep
 *
 * @return
 *    - BK_OK: succeed
 *    - otherwise: fail
 */
void cif_start_lv_sleep(void);

/**
 * @brief Add customer filter
 *
 * @param ip    the ip to filter
 * @param port    the port to filter
 *
 * @return
 *    - BK_OK: succeed
 *    - otherwise: fail
 */
void cif_filter_add_customer_filter(uint32_t ip, uint16_t port);

#ifdef __cplusplus
}
#endif
