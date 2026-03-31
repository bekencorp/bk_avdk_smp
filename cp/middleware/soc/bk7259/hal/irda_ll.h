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

#pragma once

#include <soc/soc.h>
#include "hal_port.h"
#include "irda_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IRDA_LL_REG_BASE   SOC_IRDA_REG_BASE

//reg device_id:

static inline void irda_ll_set_device_id_value(uint32_t v) {
	irda_device_id_t *r = (irda_device_id_t*)(SOC_IRDA_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_device_id_value(void) {
	irda_device_id_t *r = (irda_device_id_t*)(SOC_IRDA_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline void irda_ll_set_device_id_deviceid(uint32_t v) {
	irda_device_id_t *r = (irda_device_id_t*)(SOC_IRDA_REG_BASE + (0x0 << 2));
	r->deviceid = v;
}

static inline uint32_t irda_ll_get_device_id_deviceid(void) {
	irda_device_id_t *r = (irda_device_id_t*)(SOC_IRDA_REG_BASE + (0x0 << 2));
	return r->deviceid;
}

//reg version_id:

static inline void irda_ll_set_version_id_value(uint32_t v) {
	irda_version_id_t *r = (irda_version_id_t*)(SOC_IRDA_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_version_id_value(void) {
	irda_version_id_t *r = (irda_version_id_t*)(SOC_IRDA_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline void irda_ll_set_version_id_versionid(uint32_t v) {
	irda_version_id_t *r = (irda_version_id_t*)(SOC_IRDA_REG_BASE + (0x1 << 2));
	r->versionid = v;
}

static inline uint32_t irda_ll_get_version_id_versionid(void) {
	irda_version_id_t *r = (irda_version_id_t*)(SOC_IRDA_REG_BASE + (0x1 << 2));
	return r->versionid;
}

//reg soft_reset:

static inline void irda_ll_set_soft_reset_value(uint32_t v) {
	irda_soft_reset_t *r = (irda_soft_reset_t*)(SOC_IRDA_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_soft_reset_value(void) {
	irda_soft_reset_t *r = (irda_soft_reset_t*)(SOC_IRDA_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void irda_ll_set_soft_reset_soft_reset(uint32_t v) {
	irda_soft_reset_t *r = (irda_soft_reset_t*)(SOC_IRDA_REG_BASE + (0x2 << 2));
	r->soft_reset = v;
}

static inline uint32_t irda_ll_get_soft_reset_soft_reset(void) {
	irda_soft_reset_t *r = (irda_soft_reset_t*)(SOC_IRDA_REG_BASE + (0x2 << 2));
	return r->soft_reset;
}

static inline void irda_ll_set_soft_reset_clkg_bypass(uint32_t v) {
	irda_soft_reset_t *r = (irda_soft_reset_t*)(SOC_IRDA_REG_BASE + (0x2 << 2));
	r->clkg_bypass = v;
}

static inline uint32_t irda_ll_get_soft_reset_clkg_bypass(void) {
	irda_soft_reset_t *r = (irda_soft_reset_t*)(SOC_IRDA_REG_BASE + (0x2 << 2));
	return r->clkg_bypass;
}

//reg dev_status:

static inline void irda_ll_set_dev_status_value(uint32_t v) {
	irda_dev_status_t *r = (irda_dev_status_t*)(SOC_IRDA_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_dev_status_value(void) {
	irda_dev_status_t *r = (irda_dev_status_t*)(SOC_IRDA_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline void irda_ll_set_dev_status_devstatus(uint32_t v) {
	irda_dev_status_t *r = (irda_dev_status_t*)(SOC_IRDA_REG_BASE + (0x3 << 2));
	r->devstatus = v;
}

static inline uint32_t irda_ll_get_dev_status_devstatus(void) {
	irda_dev_status_t *r = (irda_dev_status_t*)(SOC_IRDA_REG_BASE + (0x3 << 2));
	return r->devstatus;
}

//reg reg0x4:

static inline void irda_ll_set_reg0x4_value(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_reg0x4_value(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void irda_ll_set_reg0x4_rxenable(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->rxenable = v;
}

static inline uint32_t irda_ll_get_reg0x4_rxenable(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->rxenable;
}

static inline void irda_ll_set_reg0x4_rx_initial_level(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->rx_initial_level = v;
}

static inline uint32_t irda_ll_get_reg0x4_rx_initial_level(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->rx_initial_level;
}

static inline void irda_ll_set_reg0x4_txenable(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->txenable = v;
}

static inline uint32_t irda_ll_get_reg0x4_txenable(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->txenable;
}

static inline void irda_ll_set_reg0x4_tx_initial_level(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->tx_initial_level = v;
}

static inline uint32_t irda_ll_get_reg0x4_tx_initial_level(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->tx_initial_level;
}

static inline void irda_ll_set_reg0x4_tx_start(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->tx_start = v;
}

static inline uint32_t irda_ll_get_reg0x4_tx_start(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->tx_start;
}

static inline void irda_ll_set_reg0x4_irda_pwd(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->irda_pwd = v;
}

static inline uint32_t irda_ll_get_reg0x4_irda_pwd(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->irda_pwd;
}

static inline void irda_ll_set_reg0x4_rfu7(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->rfu7 = v;
}

static inline uint32_t irda_ll_get_reg0x4_rfu7(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->rfu7;
}

static inline void irda_ll_set_reg0x4_clk_freq_in(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->clk_freq_in = v;
}

static inline uint32_t irda_ll_get_reg0x4_clk_freq_in(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->clk_freq_in;
}

static inline void irda_ll_set_reg0x4_rstn(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->rstn = v;
}

static inline uint32_t irda_ll_get_reg0x4_rstn(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->rstn;
}

static inline void irda_ll_set_reg0x4_txdata_num(uint32_t v) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	r->txdata_num = v;
}

static inline uint32_t irda_ll_get_reg0x4_txdata_num(void) {
	irda_reg0x4_t *r = (irda_reg0x4_t*)(SOC_IRDA_REG_BASE + (0x4 << 2));
	return r->txdata_num;
}

//reg reg0x5:

static inline void irda_ll_set_reg0x5_value(uint32_t v) {
	irda_reg0x5_t *r = (irda_reg0x5_t*)(SOC_IRDA_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_reg0x5_value(void) {
	irda_reg0x5_t *r = (irda_reg0x5_t*)(SOC_IRDA_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline void irda_ll_set_reg0x5_tx_fifo_threshold(uint32_t v) {
	irda_reg0x5_t *r = (irda_reg0x5_t*)(SOC_IRDA_REG_BASE + (0x5 << 2));
	r->tx_fifo_threshold = v;
}

static inline uint32_t irda_ll_get_reg0x5_tx_fifo_threshold(void) {
	irda_reg0x5_t *r = (irda_reg0x5_t*)(SOC_IRDA_REG_BASE + (0x5 << 2));
	return r->tx_fifo_threshold;
}

static inline void irda_ll_set_reg0x5_rx_fifo_threshold(uint32_t v) {
	irda_reg0x5_t *r = (irda_reg0x5_t*)(SOC_IRDA_REG_BASE + (0x5 << 2));
	r->rx_fifo_threshold = v;
}

static inline uint32_t irda_ll_get_reg0x5_rx_fifo_threshold(void) {
	irda_reg0x5_t *r = (irda_reg0x5_t*)(SOC_IRDA_REG_BASE + (0x5 << 2));
	return r->rx_fifo_threshold;
}

static inline void irda_ll_set_reg0x5_rx_timeout_cnt(uint32_t v) {
	irda_reg0x5_t *r = (irda_reg0x5_t*)(SOC_IRDA_REG_BASE + (0x5 << 2));
	r->rx_timeout_cnt = v;
}

static inline uint32_t irda_ll_get_reg0x5_rx_timeout_cnt(void) {
	irda_reg0x5_t *r = (irda_reg0x5_t*)(SOC_IRDA_REG_BASE + (0x5 << 2));
	return r->rx_timeout_cnt;
}

//reg reg0x6:

static inline void irda_ll_set_reg0x6_value(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_reg0x6_value(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->v;
}

static inline void irda_ll_set_reg0x6_tx_fifo_count(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->tx_fifo_count = v;
}

static inline uint32_t irda_ll_get_reg0x6_tx_fifo_count(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->tx_fifo_count;
}

static inline void irda_ll_set_reg0x6_rx_fifo_count(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->rx_fifo_count = v;
}

static inline uint32_t irda_ll_get_reg0x6_rx_fifo_count(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->rx_fifo_count;
}

static inline void irda_ll_set_reg0x6_tx_fifo_full(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->tx_fifo_full = v;
}

static inline uint32_t irda_ll_get_reg0x6_tx_fifo_full(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->tx_fifo_full;
}

static inline void irda_ll_set_reg0x6_tx_fifo_empty(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->tx_fifo_empty = v;
}

static inline uint32_t irda_ll_get_reg0x6_tx_fifo_empty(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->tx_fifo_empty;
}

static inline void irda_ll_set_reg0x6_rx_fifo_full(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->rx_fifo_full = v;
}

static inline uint32_t irda_ll_get_reg0x6_rx_fifo_full(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->rx_fifo_full;
}

static inline void irda_ll_set_reg0x6_rx_fifo_empty(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->rx_fifo_empty = v;
}

static inline uint32_t irda_ll_get_reg0x6_rx_fifo_empty(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->rx_fifo_empty;
}

static inline void irda_ll_set_reg0x6_fifo_wr_ready(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->fifo_wr_ready = v;
}

static inline uint32_t irda_ll_get_reg0x6_fifo_wr_ready(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->fifo_wr_ready;
}

static inline void irda_ll_set_reg0x6_fifo_rd_ready(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->fifo_rd_ready = v;
}

static inline uint32_t irda_ll_get_reg0x6_fifo_rd_ready(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->fifo_rd_ready;
}

static inline void irda_ll_set_reg0x6_rxdata_num(uint32_t v) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	r->rxdata_num = v;
}

static inline uint32_t irda_ll_get_reg0x6_rxdata_num(void) {
	irda_reg0x6_t *r = (irda_reg0x6_t*)(SOC_IRDA_REG_BASE + (0x6 << 2));
	return r->rxdata_num;
}

//reg reg0x7:

static inline void irda_ll_set_reg0x7_value(uint32_t v) {
	irda_reg0x7_t *r = (irda_reg0x7_t*)(SOC_IRDA_REG_BASE + (0x7 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_reg0x7_value(void) {
	irda_reg0x7_t *r = (irda_reg0x7_t*)(SOC_IRDA_REG_BASE + (0x7 << 2));
	return r->v;
}

static inline void irda_ll_set_reg0x7_fifo_data_rx(uint32_t v) {
	irda_reg0x7_t *r = (irda_reg0x7_t*)(SOC_IRDA_REG_BASE + (0x7 << 2));
	r->fifo_data_rx = v;
}

static inline uint32_t irda_ll_get_reg0x7_fifo_data_rx(void) {
	irda_reg0x7_t *r = (irda_reg0x7_t*)(SOC_IRDA_REG_BASE + (0x7 << 2));
	return r->fifo_data_rx;
}

static inline void irda_ll_set_reg0x7_fifo_data_tx(uint32_t v) {
	irda_reg0x7_t *r = (irda_reg0x7_t*)(SOC_IRDA_REG_BASE + (0x7 << 2));
	r->fifo_data_tx = v;
}

static inline uint32_t irda_ll_get_reg0x7_fifo_data_tx(void) {
	irda_reg0x7_t *r = (irda_reg0x7_t*)(SOC_IRDA_REG_BASE + (0x7 << 2));
	return r->fifo_data_tx;
}

//reg reg0x8:

static inline void irda_ll_set_reg0x8_value(uint32_t v) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_reg0x8_value(void) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	return r->v;
}

static inline void irda_ll_set_reg0x8_tx_need_wr_mask(uint32_t v) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	r->tx_need_wr_mask = v;
}

static inline uint32_t irda_ll_get_reg0x8_tx_need_wr_mask(void) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	return r->tx_need_wr_mask;
}

static inline void irda_ll_set_reg0x8_rx_need_rd_mask(uint32_t v) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	r->rx_need_rd_mask = v;
}

static inline uint32_t irda_ll_get_reg0x8_rx_need_rd_mask(void) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	return r->rx_need_rd_mask;
}

static inline void irda_ll_set_reg0x8_tx_done_mask(uint32_t v) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	r->tx_done_mask = v;
}

static inline uint32_t irda_ll_get_reg0x8_tx_done_mask(void) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	return r->tx_done_mask;
}

static inline void irda_ll_set_reg0x8_rx_timeout_mask(uint32_t v) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	r->rx_timeout_mask = v;
}

static inline uint32_t irda_ll_get_reg0x8_rx_timeout_mask(void) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	return r->rx_timeout_mask;
}

static inline void irda_ll_set_reg0x8_rx_overflow_status(uint32_t v) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	r->rx_overflow_status = v;
}

static inline uint32_t irda_ll_get_reg0x8_rx_overflow_status(void) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	return r->rx_overflow_status;
}

static inline void irda_ll_set_reg0x8_rfu6(uint32_t v) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	r->rfu6 = v;
}

static inline uint32_t irda_ll_get_reg0x8_rfu6(void) {
	irda_reg0x8_t *r = (irda_reg0x8_t*)(SOC_IRDA_REG_BASE + (0x8 << 2));
	return r->rfu6;
}

//reg reg0x9:

static inline void irda_ll_set_reg0x9_value(uint32_t v) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_reg0x9_value(void) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	return r->v;
}

static inline void irda_ll_set_reg0x9_tx_need_wr_state(uint32_t v) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	r->tx_need_wr_state = v;
}

static inline uint32_t irda_ll_get_reg0x9_tx_need_wr_state(void) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	return r->tx_need_wr_state;
}

static inline void irda_ll_set_reg0x9_rx_need_rd_state(uint32_t v) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	r->rx_need_rd_state = v;
}

static inline uint32_t irda_ll_get_reg0x9_rx_need_rd_state(void) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	return r->rx_need_rd_state;
}

static inline void irda_ll_set_reg0x9_tx_done_status(uint32_t v) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	r->tx_done_status = v;
}

static inline uint32_t irda_ll_get_reg0x9_tx_done_status(void) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	return r->tx_done_status;
}

static inline void irda_ll_set_reg0x9_rx_done_status(uint32_t v) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	r->rx_done_status = v;
}

static inline uint32_t irda_ll_get_reg0x9_rx_done_status(void) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	return r->rx_done_status;
}

static inline void irda_ll_set_reg0x9_rx_overflow_status(uint32_t v) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	r->rx_overflow_status = v;
}

static inline uint32_t irda_ll_get_reg0x9_rx_overflow_status(void) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	return r->rx_overflow_status;
}

static inline void irda_ll_set_reg0x9_rfu5(uint32_t v) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	r->rfu5 = v;
}

static inline uint32_t irda_ll_get_reg0x9_rfu5(void) {
	irda_reg0x9_t *r = (irda_reg0x9_t*)(SOC_IRDA_REG_BASE + (0x9 << 2));
	return r->rfu5;
}

//reg reg0xa:

static inline void irda_ll_set_reg0xa_value(uint32_t v) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_reg0xa_value(void) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	return r->v;
}

static inline void irda_ll_set_reg0xa_carrier_period(uint32_t v) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	r->carrier_period = v;
}

static inline uint32_t irda_ll_get_reg0xa_carrier_period(void) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	return r->carrier_period;
}

static inline void irda_ll_set_reg0xa_carrier_duty(uint32_t v) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	r->carrier_duty = v;
}

static inline uint32_t irda_ll_get_reg0xa_carrier_duty(void) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	return r->carrier_duty;
}

static inline void irda_ll_set_reg0xa_carrier_polarity(uint32_t v) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	r->carrier_polarity = v;
}

static inline uint32_t irda_ll_get_reg0xa_carrier_polarity(void) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	return r->carrier_polarity;
}

static inline void irda_ll_set_reg0xa_carrier_enable(uint32_t v) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	r->carrier_enable = v;
}

static inline uint32_t irda_ll_get_reg0xa_carrier_enable(void) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	return r->carrier_enable;
}

static inline void irda_ll_set_reg0xa_rfu4(uint32_t v) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	r->rfu4 = v;
}

static inline uint32_t irda_ll_get_reg0xa_rfu4(void) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	return r->rfu4;
}

static inline void irda_ll_set_reg0xa_rx_start_thr(uint32_t v) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	r->rx_start_thr = v;
}

static inline uint32_t irda_ll_get_reg0xa_rx_start_thr(void) {
	irda_reg0xa_t *r = (irda_reg0xa_t*)(SOC_IRDA_REG_BASE + (0xa << 2));
	return r->rx_start_thr;
}

//reg reg0xb:

static inline void irda_ll_set_reg0xb_value(uint32_t v) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	r->v = v;
}

static inline uint32_t irda_ll_get_reg0xb_value(void) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	return r->v;
}

static inline void irda_ll_set_reg0xb_glitch_enable(uint32_t v) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	r->glitch_enable = v;
}

static inline uint32_t irda_ll_get_reg0xb_glitch_enable(void) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	return r->glitch_enable;
}

static inline void irda_ll_set_reg0xb_rfu0(uint32_t v) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	r->rfu0 = v;
}

static inline uint32_t irda_ll_get_reg0xb_rfu0(void) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	return r->rfu0;
}

static inline void irda_ll_set_reg0xb_rfu1(uint32_t v) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	r->rfu1 = v;
}

static inline uint32_t irda_ll_get_reg0xb_rfu1(void) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	return r->rfu1;
}

static inline void irda_ll_set_reg0xb_rfu2(uint32_t v) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	r->rfu2 = v;
}

static inline uint32_t irda_ll_get_reg0xb_rfu2(void) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	return r->rfu2;
}

static inline void irda_ll_set_reg0xb_glitch_thr(uint32_t v) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	r->glitch_thr = v;
}

static inline uint32_t irda_ll_get_reg0xb_glitch_thr(void) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	return r->glitch_thr;
}

static inline void irda_ll_set_reg0xb_rfu3(uint32_t v) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	r->rfu3 = v;
}

static inline uint32_t irda_ll_get_reg0xb_rfu3(void) {
	irda_reg0xb_t *r = (irda_reg0xb_t*)(SOC_IRDA_REG_BASE + (0xb << 2));
	return r->rfu3;
}
#ifdef __cplusplus
}
#endif
