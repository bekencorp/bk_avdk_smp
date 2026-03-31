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

#include "hal_config.h"
#include "sdio1_hw.h"

typedef void (*sdio1_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	sdio1_dump_fn_t fn;
} sdio1_reg_fn_map_t;

static void sdio1_dump_dev_id(void)
{
	SOC_LOGI("dev_id: %8x\r\n", REG_READ(SOC_SDIO1_REG_BASE + (0x0 << 2)));
}

static void sdio1_dump_ver_id(void)
{
	SOC_LOGI("ver_id: %8x\r\n", REG_READ(SOC_SDIO1_REG_BASE + (0x1 << 2)));
}

static void sdio1_dump_clkg_reset(void)
{
	sdio1_clkg_reset_t *r = (sdio1_clkg_reset_t *)(SOC_SDIO1_REG_BASE + (0x2 << 2));

	SOC_LOGI("clkg_reset: %8x\r\n", REG_READ(SOC_SDIO1_REG_BASE + (0x2 << 2)));
	SOC_LOGI("	soft_resetn: %8x\r\n", r->soft_resetn);
	SOC_LOGI("	bps_clkgate: %8x\r\n", r->bps_clkgate);
	SOC_LOGI("	reserved_bit_2_31: %8x\r\n", r->reserved_bit_2_31);
}

static void sdio1_dump_status(void)
{
	SOC_LOGI("status: %8x\r\n", REG_READ(SOC_SDIO1_REG_BASE + (0x3 << 2)));
}

static void sdio1_dump_div_ctrl(void)
{
	sdio1_div_ctrl_t *r = (sdio1_div_ctrl_t *)(SOC_SDIO1_REG_BASE + (0x4 << 2));

	SOC_LOGI("div_ctrl: %8x\r\n", REG_READ(SOC_SDIO1_REG_BASE + (0x4 << 2)));
	SOC_LOGI("	tmclk_div: %8x\r\n", r->tmclk_div);
	SOC_LOGI("	cqet_mclk_div: %8x\r\n", r->cqet_mclk_div);
	SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void sdio1_dump_sdio_ctrl(void)
{
	sdio1_sdio_ctrl_t *r = (sdio1_sdio_ctrl_t *)(SOC_SDIO1_REG_BASE + (0x5 << 2));

	SOC_LOGI("sdio_ctrl: %8x\r\n", REG_READ(SOC_SDIO1_REG_BASE + (0x5 << 2)));
	SOC_LOGI("	card_write_prot: %8x\r\n", r->card_write_prot);
	SOC_LOGI("	card_detect_n: %8x\r\n", r->card_detect_n);
	SOC_LOGI("	led_control: %8x\r\n", r->led_control);
	SOC_LOGI("	sd_datxfer_width: %8x\r\n", r->sd_datxfer_width);
	SOC_LOGI("	tuning_rx_sel0: %8x\r\n", r->tuning_rx_sel0);
	SOC_LOGI("	tuning_rx_sel1: %8x\r\n", r->tuning_rx_sel1);
	SOC_LOGI("	reserved_11_13: %8x\r\n", r->reserved_11_13);
	SOC_LOGI("	sample_rx_sel0: %8x\r\n", r->sample_rx_sel0);
	SOC_LOGI("	sample_rx_sel1: %8x\r\n", r->sample_rx_sel1);
	SOC_LOGI("	reserved_16_16: %8x\r\n", r->reserved_16_16);
	SOC_LOGI("	tuning_tx_sel0: %8x\r\n", r->tuning_tx_sel0);
	SOC_LOGI("	tuning_tx_sel1: %8x\r\n", r->tuning_tx_sel1);
	SOC_LOGI("	reserved_23_25: %8x\r\n", r->reserved_23_25);
	SOC_LOGI("	sample_tx_sel0: %8x\r\n", r->sample_tx_sel0);
	SOC_LOGI("	sample_tx_sel1: %8x\r\n", r->sample_tx_sel1);
	SOC_LOGI("	reserved_28_28: %8x\r\n", r->reserved_28_28);
	SOC_LOGI("	clk_drv_negedge_sel: %8x\r\n", r->clk_drv_negedge_sel);
	SOC_LOGI("	reserved_30_31: %8x\r\n", r->reserved_30_31);
}

static void sdio1_dump_prot_ctrl(void)
{
	sdio1_prot_ctrl_t *r = (sdio1_prot_ctrl_t *)(SOC_SDIO1_REG_BASE + (0x6 << 2));

	SOC_LOGI("prot_ctrl: %8x\r\n", REG_READ(SOC_SDIO1_REG_BASE + (0x6 << 2)));
	SOC_LOGI("	mhprot: %8x\r\n", r->mhprot);
	SOC_LOGI("	mhprot_sel: %8x\r\n", r->mhprot_sel);
	SOC_LOGI("	reserved_5_31: %8x\r\n", r->reserved_5_31);
}

static sdio1_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, sdio1_dump_dev_id},
	{0x1, 0x1, sdio1_dump_ver_id},
	{0x2, 0x2, sdio1_dump_clkg_reset},
	{0x3, 0x3, sdio1_dump_status},
	{0x4, 0x4, sdio1_dump_div_ctrl},
	{0x5, 0x5, sdio1_dump_sdio_ctrl},
	{0x6, 0x6, sdio1_dump_prot_ctrl},
	{-1, -1, 0}
};

void sdio1_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
