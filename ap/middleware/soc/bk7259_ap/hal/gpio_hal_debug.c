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
#include "gpio_hw.h"
#include "gpio_hal_v2px.h"

typedef void (*gpio_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	gpio_dump_fn_t fn;
} gpio_reg_fn_map_t;

static void gpio_dump_cfg(void)
{
	gpio_cfg_t *r = (gpio_cfg_t *)(SOC_AON_GPIO_REG_BASE + (0x0 << 2));

	SOC_LOGI("cfg: %8x\r\n", REG_READ(SOC_AON_GPIO_REG_BASE + (0x0 << 2)));
	SOC_LOGI("	gpio_input: %8x\r\n", r->gpio_input);
	SOC_LOGI("	gpio_output: %8x\r\n", r->gpio_output);
	SOC_LOGI("	gpio_input_ena: %8x\r\n", r->gpio_input_ena);
	SOC_LOGI("	gpio_output_ena: %8x\r\n", r->gpio_output_ena);
	SOC_LOGI("	gpio_pull_mode: %8x\r\n", r->gpio_pull_mode);
	SOC_LOGI("	gpio_pull_ena: %8x\r\n", r->gpio_pull_ena);
	SOC_LOGI("	gpio_fun_ena: %8x\r\n", r->gpio_fun_ena);
	SOC_LOGI("	input_monitor: %8x\r\n", r->input_monitor);
	SOC_LOGI("	gpio_capacity: %8x\r\n", r->gpio_capacity);
	SOC_LOGI("	gpio_int_type: %8x\r\n", r->gpio_int_type);
	SOC_LOGI("	gpio_int_ena: %8x\r\n", r->gpio_int_ena);
	SOC_LOGI("	gpio_int_clear: %8x\r\n", r->gpio_int_clear);
	SOC_LOGI("	reserved_14_15: %8x\r\n", r->reserved_14_15);
	SOC_LOGI("	gpio_state: %8x\r\n", r->gpio_state);
	SOC_LOGI("	reserved_21_23: %8x\r\n", r->reserved_21_23);
	SOC_LOGI("	gpio_fun_sel: %8x\r\n", r->gpio_fun_sel);
}

static void gpio_dump_intsta0(void)
{
	SOC_LOGI("intsta0: %8x\r\n", REG_READ(SOC_AON_GPIO_REG_BASE + (0x78 << 2)));
}

static void gpio_dump_intsta1(void)
{
	SOC_LOGI("intsta1: %8x\r\n", REG_READ(SOC_AON_GPIO_REG_BASE + (0x79 << 2)));
}

static void gpio_dump_intsta2(void)
{
	gpio_intsta2_t *r = (gpio_intsta2_t *)(SOC_AON_GPIO_REG_BASE + (0x7a << 2));

	SOC_LOGI("intsta2: %8x\r\n", REG_READ(SOC_AON_GPIO_REG_BASE + (0x7a << 2)));
	SOC_LOGI("	gpio_intsta2: %8x\r\n", r->gpio_intsta2);
	SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void gpio_dump_rsv_7b_7b(void)
{
	for (uint32_t idx = 0; idx < 1; idx++) {
		SOC_LOGI("rsv_7b_7b: %8x\r\n", REG_READ(SOC_AON_GPIO_REG_BASE + ((0x7b + idx) << 2)));
	}
}

static void gpio_dump_int_mask(void)
{
	gpio_int_mask_t *r = (gpio_int_mask_t *)(SOC_AON_GPIO_REG_BASE + (0x7c << 2));

	SOC_LOGI("int_mask: %8x\r\n", REG_READ(SOC_AON_GPIO_REG_BASE + (0x7c << 2)));
	SOC_LOGI("	m52_gpoup_int_mask: %8x\r\n", r->m52_gpoup_int_mask);
	SOC_LOGI("	m52_even_int_mask: %8x\r\n", r->m52_even_int_mask);
	SOC_LOGI("	m52_odd_int_mask: %8x\r\n", r->m52_odd_int_mask);
	SOC_LOGI("	reserved_11_15: %8x\r\n", r->reserved_11_15);
	SOC_LOGI("	m55_gpoup_int_mask: %8x\r\n", r->m55_gpoup_int_mask);
	SOC_LOGI("	m55_even_int_mask: %8x\r\n", r->m55_even_int_mask);
	SOC_LOGI("	m55_odd_int_mask: %8x\r\n", r->m55_odd_int_mask);
	SOC_LOGI("	reserved_27_31: %8x\r\n", r->reserved_27_31);
}

#if CFG_HAL_DEBUG_GPIO
static gpio_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, gpio_dump_cfg},
	{0x78, 0x78, gpio_dump_intsta0},
	{0x79, 0x79, gpio_dump_intsta1},
	{0x7a, 0x7a, gpio_dump_intsta2},
	{0x7b, 0x7c, gpio_dump_rsv_7b_7b},
	{0x7c, 0x7c, gpio_dump_int_mask},
	{-1, -1, 0}
};

void gpio_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
#endif
