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

#include "components/bk24/bk_24g_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief bk24 APIs Version 1.0
 * @defgroup bk24_api_v1 New bk24 api group
 * @{
 */

/**
 * @brief     Initialize the bk24 2.4G controller.
 *
 * @attention This API powers on the controller, enables clock and interrupt,
 *            initializes XVR and configures default TX mode parameters.
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_init(void);

/**
 * @brief     Deinitialize the bk24 2.4G controller.
 *
 * @attention This API resets the controller and powers down the 2.4G module.
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_deinit(void);

/**
 * @brief     Send data with ACK enabled.
 *
 * @param
 *    - data: data buffer to be sent
 *    - len: data length in bytes, normally no more than 32 bytes
 *
 * @attention Must be used in TX mode. When long packet is enabled, len must be
 *            a multiple of 8 if it is greater than the FIFO payload size.
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_send_data(uint8_t *data, uint32_t len);

/**
 * @brief     Send data without requesting ACK.
 *
 * @param
 *    - data: data buffer to be sent
 *    - len: data length in bytes, normally no more than 32 bytes
 *
 * @attention Must be used in TX mode. When long packet is enabled, len must be
 *            a multiple of 8 if it is greater than the FIFO payload size.
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_send_data_no_ack(uint8_t *data, uint32_t len);

/**
 * @brief     Set bk24 data rate.
 *
 * @param
 *    - s: data rate, 0 means 1M, 1 means 2M
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_set_data_rate(uint8_t s);

/**
 * @brief     Switch bk24 between TX and RX mode.
 *
 * @param
 *    - rx: 0 means TX mode, 1 means RX mode
 *
 * @return
 *    - void
 */
void bk24_switch_to_tx_rx(uint8_t rx);

/**
 * @brief     Set ACK payload for the specified RX pipe.
 *
 * @param
 *    - data: ACK payload buffer
 *    - len: ACK payload length in bytes, normally no more than 32 bytes
 *    - pipe: RX pipe index, valid range is 0 to 5
 *
 * @attention Must be used in RX mode.
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_set_ack_payload(uint8_t *data, uint16_t len, uint8_t pipe);

/**
 * @brief     Set RF channel.
 *
 * @param
 *    - c: channel offset from 2.4GHz, step is 1MHz, valid range is 0 to 127
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_set_channel(uint8_t c);

/**
 * @brief     Set auto retransmission parameters.
 *
 * @param
 *    - delay: retransmission delay in us, range is 250 to 4000 and must be a multiple of 250
 *    - retran_count: retransmission count, valid range is 0 to 15
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_set_retran(uint16_t delay, uint8_t retran_count);

/**
 * @brief     Set TX address.
 *
 * @param
 *    - addr: TX address buffer
 *    - len: TX address length, must be 4 bytes
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_set_tx_addr(uint8_t *addr, uint8_t len);

/**
 * @brief     Set RX address for the specified pipe.
 *
 * @param
 *    - addr: RX address buffer
 *    - len: RX address length, must be 4 bytes
 *    - pipe: RX pipe index, valid range is 0 to 5
 *
 * @attention For pipe 2 to pipe 5, the address MSB shares pipe 1 MSB.
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_set_rx_addr(uint8_t *addr, uint8_t len, uint8_t pipe);

/**
 * @brief     Set TX timing parameter.
 *
 * @param
 *    - us: PLL lock time in us, supported values are 60, 70, 80, 90, 120, 200, 300 and 500
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_set_tx_setting(uint16_t us);

/**
 * @brief     Register TX result callback.
 *
 * @param
 *    - cb: send callback, see \ref bk24_send_cb_t
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_reg_send_callback(bk24_send_cb_t cb);

/**
 * @brief     Register RX data callback.
 *
 * @param
 *    - cb: receive callback.
 *
 * @return
 *    - 0: succeed
 *    - others: fail
 */
int32_t bk24_reg_recv_callback(bk24_recv_cb_t cb);

/**
 * @brief     Reset the bk24 2.4G controller.
 *
 * @attention This API resets the controller and flushes TX/RX FIFO.
 *
 * @return
 *    - void
 */
void bk24_reset(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
