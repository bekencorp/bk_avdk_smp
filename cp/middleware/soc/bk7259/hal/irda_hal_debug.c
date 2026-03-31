// Copyright 2022-2023 Beken
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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#include "hal_config.h"
#include "irda_hw.h"
#include "irda_hal.h"

typedef void (*irda_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	irda_dump_fn_t fn;
} irda_reg_fn_map_t;

static void irda_dump_device_id(void)
{
	SOC_LOGD("device_id: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x0 << 2)));
}

static void irda_dump_version_id(void)
{
	SOC_LOGD("version_id: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x1 << 2)));
}

static void irda_dump_soft_reset(void)
{
	irda_soft_reset_t *r = (irda_soft_reset_t *)(SOC_IRDA_REG_BASE + (0x2 << 2));

	SOC_LOGD("soft_reset: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x2 << 2)));
	SOC_LOGD("	soft_reset: %8x\r\n", r->soft_reset);
	SOC_LOGD("	clkg_bypass: %8x\r\n", r->clkg_bypass);
	SOC_LOGD("	reserved_bit_2_31: %8x\r\n", r->reserved_bit_2_31);
}

static void irda_dump_dev_status(void)
{
	SOC_LOGD("dev_status: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x3 << 2)));
}

static void irda_dump_reg0x4(void)
{
	irda_reg0x4_t *r = (irda_reg0x4_t *)(SOC_IRDA_REG_BASE + (0x4 << 2));

	SOC_LOGD("reg0x4: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x4 << 2)));
	SOC_LOGD("	rxenable: %8x\r\n", r->rxenable);
	SOC_LOGD("	rx_initial_level: %8x\r\n", r->rx_initial_level);
	SOC_LOGD("	txenable: %8x\r\n", r->txenable);
	SOC_LOGD("	tx_initial_level: %8x\r\n", r->tx_initial_level);
	SOC_LOGD("	tx_start: %8x\r\n", r->tx_start);
	SOC_LOGD("	irda_pwd: %8x\r\n", r->irda_pwd);
	SOC_LOGD("	rfu7: %8x\r\n", r->rfu7);
	SOC_LOGD("	clk_freq_in: %8x\r\n", r->clk_freq_in);
	SOC_LOGD("	rstn: %8x\r\n", r->rstn);
	SOC_LOGD("	txdata_num: %8x\r\n", r->txdata_num);
	SOC_LOGD("	reserved_bit_26_31: %8x\r\n", r->reserved_bit_26_31);
}

static void irda_dump_reg0x5(void)
{
	irda_reg0x5_t *r = (irda_reg0x5_t *)(SOC_IRDA_REG_BASE + (0x5 << 2));

	SOC_LOGD("reg0x5: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x5 << 2)));
	SOC_LOGD("	tx_fifo_threshold: %8x\r\n", r->tx_fifo_threshold);
	SOC_LOGD("	rx_fifo_threshold: %8x\r\n", r->rx_fifo_threshold);
	SOC_LOGD("	rx_timeout_cnt: %8x\r\n", r->rx_timeout_cnt);
}

static void irda_dump_reg0x6(void)
{
	irda_reg0x6_t *r = (irda_reg0x6_t *)(SOC_IRDA_REG_BASE + (0x6 << 2));

	SOC_LOGD("reg0x6: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x6 << 2)));
	SOC_LOGD("	tx_fifo_count: %8x\r\n", r->tx_fifo_count);
	SOC_LOGD("	rx_fifo_count: %8x\r\n", r->rx_fifo_count);
	SOC_LOGD("	tx_fifo_full: %8x\r\n", r->tx_fifo_full);
	SOC_LOGD("	tx_fifo_empty: %8x\r\n", r->tx_fifo_empty);
	SOC_LOGD("	rx_fifo_full: %8x\r\n", r->rx_fifo_full);
	SOC_LOGD("	rx_fifo_empty: %8x\r\n", r->rx_fifo_empty);
	SOC_LOGD("	fifo_wr_ready: %8x\r\n", r->fifo_wr_ready);
	SOC_LOGD("	fifo_rd_ready: %8x\r\n", r->fifo_rd_ready);
	SOC_LOGD("	rxdata_num: %8x\r\n", r->rxdata_num);
}

static void irda_dump_reg0x7(void)
{
	irda_reg0x7_t *r = (irda_reg0x7_t *)(SOC_IRDA_REG_BASE + (0x7 << 2));

	SOC_LOGD("reg0x7: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x7 << 2)));
	SOC_LOGD("	fifo_data_rx: %8x\r\n", r->fifo_data_rx);
	SOC_LOGD("	fifo_data_tx: %8x\r\n", r->fifo_data_tx);
}

static void irda_dump_reg0x8(void)
{
	irda_reg0x8_t *r = (irda_reg0x8_t *)(SOC_IRDA_REG_BASE + (0x8 << 2));

	SOC_LOGD("reg0x8: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x8 << 2)));
	SOC_LOGD("	tx_need_wr_mask: %8x\r\n", r->tx_need_wr_mask);
	SOC_LOGD("	rx_need_rd_mask: %8x\r\n", r->rx_need_rd_mask);
	SOC_LOGD("	tx_done_mask: %8x\r\n", r->tx_done_mask);
	SOC_LOGD("	rx_timeout_mask: %8x\r\n", r->rx_timeout_mask);
	SOC_LOGD("	rx_overflow_status: %8x\r\n", r->rx_overflow_status);
	SOC_LOGD("	rfu6: %8x\r\n", r->rfu6);
}

static void irda_dump_reg0x9(void)
{
	irda_reg0x9_t *r = (irda_reg0x9_t *)(SOC_IRDA_REG_BASE + (0x9 << 2));

	SOC_LOGD("reg0x9: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0x9 << 2)));
	SOC_LOGD("	tx_need_wr_state: %8x\r\n", r->tx_need_wr_state);
	SOC_LOGD("	rx_need_rd_state: %8x\r\n", r->rx_need_rd_state);
	SOC_LOGD("	tx_done_status: %8x\r\n", r->tx_done_status);
	SOC_LOGD("	rx_done_status: %8x\r\n", r->rx_done_status);
	SOC_LOGD("	rx_overflow_status: %8x\r\n", r->rx_overflow_status);
	SOC_LOGD("	rfu5: %8x\r\n", r->rfu5);
}

static void irda_dump_reg0xa(void)
{
	irda_reg0xa_t *r = (irda_reg0xa_t *)(SOC_IRDA_REG_BASE + (0xa << 2));

	SOC_LOGD("reg0xa: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0xa << 2)));
	SOC_LOGD("	carrier_period: %8x\r\n", r->carrier_period);
	SOC_LOGD("	carrier_duty: %8x\r\n", r->carrier_duty);
	SOC_LOGD("	carrier_polarity: %8x\r\n", r->carrier_polarity);
	SOC_LOGD("	carrier_enable: %8x\r\n", r->carrier_enable);
	SOC_LOGD("	rfu4: %8x\r\n", r->rfu4);
	SOC_LOGD("	rx_start_thr: %8x\r\n", r->rx_start_thr);
}

static void irda_dump_reg0xb(void)
{
	irda_reg0xb_t *r = (irda_reg0xb_t *)(SOC_IRDA_REG_BASE + (0xb << 2));

	SOC_LOGD("reg0xb: %8x\r\n", REG_READ(SOC_IRDA_REG_BASE + (0xb << 2)));
	SOC_LOGD("	glitch_enable: %8x\r\n", r->glitch_enable);
	SOC_LOGD("	rfu0: %8x\r\n", r->rfu0);
	SOC_LOGD("	rfu1: %8x\r\n", r->rfu1);
	SOC_LOGD("	rfu2: %8x\r\n", r->rfu2);
	SOC_LOGD("	glitch_thr: %8x\r\n", r->glitch_thr);
	SOC_LOGD("	rfu3: %8x\r\n", r->rfu3);
}

static irda_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, irda_dump_device_id},
	{0x1, 0x1, irda_dump_version_id},
	{0x2, 0x2, irda_dump_soft_reset},
	{0x3, 0x3, irda_dump_dev_status},
	{0x4, 0x4, irda_dump_reg0x4},
	{0x5, 0x5, irda_dump_reg0x5},
	{0x6, 0x6, irda_dump_reg0x6},
	{0x7, 0x7, irda_dump_reg0x7},
	{0x8, 0x8, irda_dump_reg0x8},
	{0x9, 0x9, irda_dump_reg0x9},
	{0xa, 0xa, irda_dump_reg0xa},
	{0xb, 0xb, irda_dump_reg0xb},
	{-1, -1, 0}
};

void irda_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
