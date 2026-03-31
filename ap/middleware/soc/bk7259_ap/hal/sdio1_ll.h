// Copyright 2022-2025 Beken
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
#include "sdio1_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDIO1_LL_REG_BASE   SOC_SDIO1_REG_BASE

//reg dev_id:

static inline void sdio1_ll_set_dev_id_value(uint32_t v) {
	sdio1_dev_id_t *r = (sdio1_dev_id_t*)(SOC_SDIO1_REG_BASE + (0x0 << 2));
	r->v = v;
}

static inline uint32_t sdio1_ll_get_dev_id_value(void) {
	sdio1_dev_id_t *r = (sdio1_dev_id_t*)(SOC_SDIO1_REG_BASE + (0x0 << 2));
	return r->v;
}

static inline uint32_t sdio1_ll_get_dev_id_deviceid(void) {
	sdio1_dev_id_t *r = (sdio1_dev_id_t*)(SOC_SDIO1_REG_BASE + (0x0 << 2));
	return r->deviceid;
}

//reg ver_id:

static inline void sdio1_ll_set_ver_id_value(uint32_t v) {
	sdio1_ver_id_t *r = (sdio1_ver_id_t*)(SOC_SDIO1_REG_BASE + (0x1 << 2));
	r->v = v;
}

static inline uint32_t sdio1_ll_get_ver_id_value(void) {
	sdio1_ver_id_t *r = (sdio1_ver_id_t*)(SOC_SDIO1_REG_BASE + (0x1 << 2));
	return r->v;
}

static inline uint32_t sdio1_ll_get_ver_id_versionid(void) {
	sdio1_ver_id_t *r = (sdio1_ver_id_t*)(SOC_SDIO1_REG_BASE + (0x1 << 2));
	return r->versionid;
}

//reg clkg_reset:

static inline void sdio1_ll_set_clkg_reset_value(uint32_t v) {
	sdio1_clkg_reset_t *r = (sdio1_clkg_reset_t*)(SOC_SDIO1_REG_BASE + (0x2 << 2));
	r->v = v;
}

static inline uint32_t sdio1_ll_get_clkg_reset_value(void) {
	sdio1_clkg_reset_t *r = (sdio1_clkg_reset_t*)(SOC_SDIO1_REG_BASE + (0x2 << 2));
	return r->v;
}

static inline void sdio1_ll_set_clkg_reset_soft_resetn(uint32_t v) {
	sdio1_clkg_reset_t *r = (sdio1_clkg_reset_t*)(SOC_SDIO1_REG_BASE + (0x2 << 2));
	r->soft_resetn = v;
}

static inline uint32_t sdio1_ll_get_clkg_reset_soft_resetn(void) {
	sdio1_clkg_reset_t *r = (sdio1_clkg_reset_t*)(SOC_SDIO1_REG_BASE + (0x2 << 2));
	return r->soft_resetn;
}

static inline void sdio1_ll_set_clkg_reset_bps_clkgate(uint32_t v) {
	sdio1_clkg_reset_t *r = (sdio1_clkg_reset_t*)(SOC_SDIO1_REG_BASE + (0x2 << 2));
	r->bps_clkgate = v;
}

static inline uint32_t sdio1_ll_get_clkg_reset_bps_clkgate(void) {
	sdio1_clkg_reset_t *r = (sdio1_clkg_reset_t*)(SOC_SDIO1_REG_BASE + (0x2 << 2));
	return r->bps_clkgate;
}

//reg status:

static inline void sdio1_ll_set_status_value(uint32_t v) {
	sdio1_status_t *r = (sdio1_status_t*)(SOC_SDIO1_REG_BASE + (0x3 << 2));
	r->v = v;
}

static inline uint32_t sdio1_ll_get_status_value(void) {
	sdio1_status_t *r = (sdio1_status_t*)(SOC_SDIO1_REG_BASE + (0x3 << 2));
	return r->v;
}

static inline uint32_t sdio1_ll_get_status_globalstatus(void) {
	sdio1_status_t *r = (sdio1_status_t*)(SOC_SDIO1_REG_BASE + (0x3 << 2));
	return r->globalstatus;
}

//reg div_ctrl:

static inline void sdio1_ll_set_div_ctrl_value(uint32_t v) {
	sdio1_div_ctrl_t *r = (sdio1_div_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x4 << 2));
	r->v = v;
}

static inline uint32_t sdio1_ll_get_div_ctrl_value(void) {
	sdio1_div_ctrl_t *r = (sdio1_div_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x4 << 2));
	return r->v;
}

static inline void sdio1_ll_set_div_ctrl_tmclk_div(uint32_t v) {
	sdio1_div_ctrl_t *r = (sdio1_div_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x4 << 2));
	r->tmclk_div = v;
}

static inline uint32_t sdio1_ll_get_div_ctrl_tmclk_div(void) {
	sdio1_div_ctrl_t *r = (sdio1_div_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x4 << 2));
	return r->tmclk_div;
}

static inline void sdio1_ll_set_div_ctrl_cqet_mclk_div(uint32_t v) {
	sdio1_div_ctrl_t *r = (sdio1_div_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x4 << 2));
	r->cqet_mclk_div = v;
}

static inline uint32_t sdio1_ll_get_div_ctrl_cqet_mclk_div(void) {
	sdio1_div_ctrl_t *r = (sdio1_div_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x4 << 2));
	return r->cqet_mclk_div;
}

//reg sdio_ctrl:

static inline void sdio1_ll_set_sdio_ctrl_value(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->v = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_value(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->v;
}

static inline void sdio1_ll_set_sdio_ctrl_card_write_prot(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->card_write_prot = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_card_write_prot(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->card_write_prot;
}

static inline void sdio1_ll_set_sdio_ctrl_card_detect_n(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->card_detect_n = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_card_detect_n(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->card_detect_n;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_led_control(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->led_control;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_sd_datxfer_width(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->sd_datxfer_width;
}

static inline void sdio1_ll_set_sdio_ctrl_tuning_rx_sel0(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->tuning_rx_sel0 = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_tuning_rx_sel0(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->tuning_rx_sel0;
}

static inline void sdio1_ll_set_sdio_ctrl_tuning_rx_sel1(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->tuning_rx_sel1 = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_tuning_rx_sel1(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->tuning_rx_sel1;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_reserved_11_13(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->reserved_11_13;
}

static inline void sdio1_ll_set_sdio_ctrl_sample_rx_sel0(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->sample_rx_sel0 = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_sample_rx_sel0(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->sample_rx_sel0;
}

static inline void sdio1_ll_set_sdio_ctrl_sample_rx_sel1(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->sample_rx_sel1 = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_sample_rx_sel1(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->sample_rx_sel1;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_reserved_16_16(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->reserved_16_16;
}

static inline void sdio1_ll_set_sdio_ctrl_tuning_tx_sel0(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->tuning_tx_sel0 = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_tuning_tx_sel0(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->tuning_tx_sel0;
}

static inline void sdio1_ll_set_sdio_ctrl_tuning_tx_sel1(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->tuning_tx_sel1 = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_tuning_tx_sel1(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->tuning_tx_sel1;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_reserved_23_25(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->reserved_23_25;
}

static inline void sdio1_ll_set_sdio_ctrl_sample_tx_sel0(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->sample_tx_sel0 = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_sample_tx_sel0(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->sample_tx_sel0;
}

static inline void sdio1_ll_set_sdio_ctrl_sample_tx_sel1(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->sample_tx_sel1 = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_sample_tx_sel1(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->sample_tx_sel1;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_reserved_28_28(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->reserved_28_28;
}

static inline void sdio1_ll_set_sdio_ctrl_clk_drv_negedge_sel(uint32_t v) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	r->clk_drv_negedge_sel = v;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_clk_drv_negedge_sel(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->clk_drv_negedge_sel;
}

static inline uint32_t sdio1_ll_get_sdio_ctrl_reserved_30_31(void) {
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x5 << 2));
	return r->reserved_30_31;
}

//reg prot_ctrl:

static inline void sdio1_ll_set_prot_ctrl_value(uint32_t v) {
	sdio1_prot_ctrl_t *r = (sdio1_prot_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x6 << 2));
	r->v = v;
}

static inline uint32_t sdio1_ll_get_prot_ctrl_value(void) {
	sdio1_prot_ctrl_t *r = (sdio1_prot_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x6 << 2));
	return r->v;
}

static inline void sdio1_ll_set_prot_ctrl_mhprot(uint32_t v) {
	sdio1_prot_ctrl_t *r = (sdio1_prot_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x6 << 2));
	r->mhprot = v;
}

static inline uint32_t sdio1_ll_get_prot_ctrl_mhprot(void) {
	sdio1_prot_ctrl_t *r = (sdio1_prot_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x6 << 2));
	return r->mhprot;
}

static inline void sdio1_ll_set_prot_ctrl_mhprot_sel(uint32_t v) {
	sdio1_prot_ctrl_t *r = (sdio1_prot_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x6 << 2));
	r->mhprot_sel = v;
}

static inline uint32_t sdio1_ll_get_prot_ctrl_mhprot_sel(void) {
	sdio1_prot_ctrl_t *r = (sdio1_prot_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x6 << 2));
	return r->mhprot_sel;
}

static inline uint32_t sdio1_ll_get_prot_ctrl_reserved_5_31(void) {
	sdio1_prot_ctrl_t *r = (sdio1_prot_ctrl_t*)(SOC_SDIO1_REG_BASE + (0x6 << 2));
	return r->reserved_5_31;
}
#ifdef __cplusplus
}
#endif
