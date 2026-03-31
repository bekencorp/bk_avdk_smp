// Copyright 2023-2024 Beken
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

#include <common/bk_include.h>
#include <driver/irda_types.h>

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t bk_irda_driver_init(void);

bk_err_t bk_irda_driver_deinit(void);

bk_err_t bk_irda_init_tx(const irda_tx_init_config_t *tx_config);

bk_err_t bk_irda_init_rx(const irda_rx_init_config_t *rx_config);

/*
 * Write IRDA TX data as 16-bit words.
 * @data: Pointer to TX 16-bit entries buffer.
 * @entry_count: Number of 16-bit entries to send.
 */
bk_err_t bk_irda_write_words(const uint16_t *data, uint32_t entry_count);

/*
 * Read IRDA RX data as 16-bit words.
 * @data: Pointer to RX 16-bit entries buffer.
 * @entry_count: RX buffer capacity in 16-bit entries.
 * @timeout_ms: wait timeout in milliseconds.
 *              0 means no wait, BEKEN_WAIT_FOREVER means wait forever.
 * Return: received entry count on success, negative error code on failure.
 */
int bk_irda_read_words(uint16_t *data, uint32_t entry_count, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif