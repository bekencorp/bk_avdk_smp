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

#include <driver/irda_types.h>
#include "irda_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

bk_err_t irda_hal_init(void);
bk_err_t irda_hal_init_tx(const irda_tx_init_config_t *tx_config);
bk_err_t irda_hal_init_rx(const irda_rx_init_config_t *rx_config);
bk_err_t irda_hal_set_tx_enable(bool enable);
bk_err_t irda_hal_set_rx_enable(bool enable);
bk_err_t irda_hal_set_tx_need_wr_int(bool enable);
bk_err_t irda_hal_set_tx_done_int(bool enable);
bk_err_t irda_hal_set_rx_timeout_int(bool enable);
bk_err_t irda_hal_set_rx_int(bool enable);
bk_err_t irda_hal_set_tx_data_num(uint32_t num);
bk_err_t irda_hal_set_tx_fifo_threshold(uint32_t threshold);
bk_err_t irda_hal_start_tx(void);
bk_err_t irda_hal_write_byte(uint32_t data);
uint32_t irda_hal_read_byte(void);
uint32_t irda_hal_get_tx_fifo_count(void);
uint32_t irda_hal_get_rx_fifo_count(void);
bool irda_hal_is_rx_fifo_empty(void);
bool irda_hal_is_rx_fifo_read_ready(void);
uint32_t irda_hal_get_rx_data_num(void);

uint32_t irda_hal_get_int_status(void);
bk_err_t irda_hal_clear_int_status(uint32_t int_status);
bool irda_hal_is_tx_need_wr_int_triggered(uint32_t int_status);
bool irda_hal_is_tx_done_int_triggered(uint32_t int_status);
bool irda_hal_is_rx_done_int_triggered(uint32_t int_status);
bool irda_hal_is_rx_need_rd_int_triggered(uint32_t int_status);

#ifdef __cplusplus
}
#endif
