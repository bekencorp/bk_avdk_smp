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

#include "irda_hw.h"
#include "irda_hal.h"
#include "bk_misc.h"

// #define IRDA_RX_TIMEOUT_CNT        8000
#define IRDA_RX_START_THRESHOLD    1000
#define IRDA_GLITCH_THRESHOLD      41
#define IRDA_RX_FIFO_THRESHOLD     20

bk_err_t irda_hal_init(void)
{
	irda_ll_set_soft_reset_soft_reset(0);
	delay_ms(20);
	irda_ll_set_soft_reset_soft_reset(1);
	return BK_OK;
}

bk_err_t irda_hal_init_tx(const irda_tx_init_config_t *tx_config)
{
	irda_ll_set_reg0x4_tx_initial_level(0);
	irda_ll_set_reg0xa_carrier_polarity(0);
	irda_ll_set_reg0x4_clk_freq_in(tx_config->clk_freq_input);
	irda_ll_set_reg0xa_carrier_duty(tx_config->carrier_duty_cycle);
	irda_ll_set_reg0xa_carrier_period(tx_config->carrier_period_cycle);
	// irda_ll_set_reg0x5_tx_fifo_threshold(1);
	return BK_OK;
}

bk_err_t irda_hal_init_rx(const irda_rx_init_config_t *rx_config)
{
	irda_ll_set_reg0x4_rx_initial_level(0);
	irda_ll_set_reg0x4_clk_freq_in(rx_config->clk_freq_input);
	irda_ll_set_reg0x5_rx_timeout_cnt(rx_config->rx_timeout_us);
	irda_ll_set_reg0xa_rx_start_thr(rx_config->rx_start_threshold_us);
	irda_ll_set_reg0xb_glitch_enable(1);
	irda_ll_set_reg0xb_rfu2(8);
	irda_ll_set_reg0xb_glitch_thr(IRDA_GLITCH_THRESHOLD);
	irda_ll_set_reg0x5_rx_fifo_threshold(IRDA_RX_FIFO_THRESHOLD);
	return BK_OK;
}

bk_err_t irda_hal_set_tx_enable(bool enable)
{
	irda_ll_set_reg0x4_txenable(enable);
	return BK_OK;
}

bk_err_t irda_hal_set_rx_enable(bool enable)
{
	irda_ll_set_reg0x4_rxenable(enable);
	return BK_OK;
}

bk_err_t irda_hal_set_tx_done_int(bool enable)
{
	irda_ll_set_reg0x8_tx_done_mask(enable);
	return BK_OK;
}

bk_err_t irda_hal_set_rx_timeout_int(bool enable)
{
	irda_ll_set_reg0x8_rx_timeout_mask(enable);
	return BK_OK;
}

bk_err_t irda_hal_set_rx_int(bool enable)
{
	irda_ll_set_reg0x8_tx_done_mask(enable);
	irda_ll_set_reg0x8_rx_need_rd_mask(enable);
	irda_ll_set_reg0x8_rx_timeout_mask(enable);
	return BK_OK;
}

bk_err_t irda_hal_set_tx_data_num(uint32_t num)
{
	irda_ll_set_reg0x4_txdata_num(num);
	return BK_OK;
}

bk_err_t irda_hal_set_tx_fifo_threshold(uint32_t threshold)
{
	irda_ll_set_reg0x5_tx_fifo_threshold(threshold);
	return BK_OK;
}

bk_err_t irda_hal_start_tx(void)
{
	irda_ll_set_reg0x4_tx_start(0);
	delay_ms(2);
	irda_ll_set_reg0x4_tx_start(1);
	delay_ms(2);
	return BK_OK;
}

bk_err_t irda_hal_write_byte(uint32_t data)
{
	irda_ll_set_reg0x7_fifo_data_tx(data & IRDA_REG0X7_FIFO_DATA_TX_MASK);
	return BK_OK;
}

uint32_t irda_hal_read_byte(void)
{
	return irda_ll_get_reg0x7_fifo_data_rx();
}

uint32_t irda_hal_get_rx_data_num(void)
{
	return irda_ll_get_reg0x6_rxdata_num();
}

uint32_t irda_hal_get_int_status(void)
{
	return irda_ll_get_reg0x9_value();
}

bk_err_t irda_hal_clear_int_status(uint32_t int_status)
{
	irda_ll_set_reg0x9_value(int_status);
	return BK_OK;
}

bool irda_hal_is_tx_done_int_triggered(uint32_t int_status)
{
	return !!(int_status & BIT(IRDA_REG0X9_TX_DONE_STATUS_POS));
}

bool irda_hal_is_rx_done_int_triggered(uint32_t int_status)
{
	return !!(int_status & BIT(IRDA_REG0X9_RX_DONE_STATUS_POS));
}

bool irda_hal_is_rx_need_rd_int_triggered(uint32_t int_status)
{
	return !!(int_status & BIT(IRDA_REG0X9_RX_NEED_RD_STATE_POS));
}
