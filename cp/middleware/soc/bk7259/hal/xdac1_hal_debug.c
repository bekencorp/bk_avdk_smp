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
#include "xdac1_hw.h"
#include "xdac1_hal.h"

typedef void (*xdac1_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	xdac1_dump_fn_t fn;
} xdac1_reg_fn_map_t;

static void xdac1_dump_reg0(void)
{
	SOC_LOGI("reg0: %8x\r\n", REG_READ(SOC_XDAC1_REG_BASE + (0x0 << 2)));
}

static void xdac1_dump_reg1(void)
{
	SOC_LOGI("reg1: %8x\r\n", REG_READ(SOC_XDAC1_REG_BASE + (0x1 << 2)));
}

static void xdac1_dump_reg2(void)
{
	xdac1_reg2_t *r = (xdac1_reg2_t *)(SOC_XDAC1_REG_BASE + (0x2 << 2));

	SOC_LOGI("reg2: %8x\r\n", REG_READ(SOC_XDAC1_REG_BASE + (0x2 << 2)));
	SOC_LOGI("	soft_reset: %8x\r\n", r->soft_reset);
	SOC_LOGI("	clkg_bypass: %8x\r\n", r->clkg_bypass);
	SOC_LOGI("	reserved_bit_2_31: %8x\r\n", r->reserved_bit_2_31);
}

static void xdac1_dump_reg3(void)
{
	SOC_LOGI("reg3: %8x\r\n", REG_READ(SOC_XDAC1_REG_BASE + (0x3 << 2)));
}

static void xdac1_dump_reg4(void)
{
	xdac1_reg4_t *r = (xdac1_reg4_t *)(SOC_XDAC1_REG_BASE + (0x4 << 2));

	SOC_LOGI("reg4: %8x\r\n", REG_READ(SOC_XDAC1_REG_BASE + (0x4 << 2)));
	SOC_LOGI("	dac_enable: %8x\r\n", r->dac_enable);
	SOC_LOGI("	dac_clk_en: %8x\r\n", r->dac_clk_en);
	SOC_LOGI("	dac_mode: %8x\r\n", r->dac_mode);
	SOC_LOGI("	fifo_enable: %8x\r\n", r->fifo_enable);
	SOC_LOGI("	reserved_4_15: %8x\r\n", r->reserved_4_15);
	SOC_LOGI("	dac_clk_div: %8x\r\n", r->dac_clk_div);
}

static void xdac1_dump_reg5(void)
{
	xdac1_reg5_t *r = (xdac1_reg5_t *)(SOC_XDAC1_REG_BASE + (0x5 << 2));

	SOC_LOGI("reg5: %8x\r\n", REG_READ(SOC_XDAC1_REG_BASE + (0x5 << 2)));
	SOC_LOGI("	fifo_empty_int: %8x\r\n", r->fifo_empty_int);
	SOC_LOGI("	fifo_full_int: %8x\r\n", r->fifo_full_int);
	SOC_LOGI("	fifo_near_full_int: %8x\r\n", r->fifo_near_full_int);
	SOC_LOGI("	fifo_near_empty_int: %8x\r\n", r->fifo_near_empty_int);
	SOC_LOGI("	reserved_4_31: %8x\r\n", r->reserved_4_31);
}

static void xdac1_dump_reg6(void)
{
	xdac1_reg6_t *r = (xdac1_reg6_t *)(SOC_XDAC1_REG_BASE + (0x6 << 2));

	SOC_LOGI("reg6: %8x\r\n", REG_READ(SOC_XDAC1_REG_BASE + (0x6 << 2)));
	SOC_LOGI("	fifo_empty_int_en: %8x\r\n", r->fifo_empty_int_en);
	SOC_LOGI("	fifo_full_int_en: %8x\r\n", r->fifo_full_int_en);
	SOC_LOGI("	fifo_near_full_int_en: %8x\r\n", r->fifo_near_full_int_en);
	SOC_LOGI("	fifo_near_empty_int_en: %8x\r\n", r->fifo_near_empty_int_en);
	SOC_LOGI("	reserved_4_31: %8x\r\n", r->reserved_4_31);
}

static void xdac1_dump_reg7(void)
{
	xdac1_reg7_t *r = (xdac1_reg7_t *)(SOC_XDAC1_REG_BASE + (0x7 << 2));

	SOC_LOGI("reg7: %8x\r\n", REG_READ(SOC_XDAC1_REG_BASE + (0x7 << 2)));
	SOC_LOGI("	dac_rthrd: %8x\r\n", r->dac_rthrd);
	SOC_LOGI("	dac_wthrd: %8x\r\n", r->dac_wthrd);
	SOC_LOGI("	reserved_10_31: %8x\r\n", r->reserved_10_31);
}

static void xdac1_dump_reg8(void)
{
	xdac1_reg8_t *r = (xdac1_reg8_t *)(SOC_XDAC1_REG_BASE + (0x8 << 2));

	SOC_LOGI("reg8: %8x\r\n", REG_READ(SOC_XDAC1_REG_BASE + (0x8 << 2)));
	SOC_LOGI("	tx_fifo_wr_data: %8x\r\n", r->tx_fifo_wr_data);
	SOC_LOGI("	reserved_12_31: %8x\r\n", r->reserved_12_31);
}

static xdac1_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, xdac1_dump_reg0},
	{0x1, 0x1, xdac1_dump_reg1},
	{0x2, 0x2, xdac1_dump_reg2},
	{0x3, 0x3, xdac1_dump_reg3},
	{0x4, 0x4, xdac1_dump_reg4},
	{0x5, 0x5, xdac1_dump_reg5},
	{0x6, 0x6, xdac1_dump_reg6},
	{0x7, 0x7, xdac1_dump_reg7},
	{0x8, 0x8, xdac1_dump_reg8},
	{-1, -1, 0}
};

void xdac1_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
