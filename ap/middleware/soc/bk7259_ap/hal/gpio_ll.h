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
#include "gpio_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_LL_REG_BASE   SOC_AON_GPIO_REG_BASE
#define GPIO_NUM_MAX       (SOC_GPIO_NUM)

typedef volatile struct {
	uint32_t gpio_0_31_int_status;
	uint32_t gpio_32_63_int_status;
	uint32_t gpio_64_71_int_status;
} gpio_interrupt_status_t;

//reg cfg:

static inline void gpio_ll_set_cfg_value(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->v = v;
}

static inline uint32_t gpio_ll_get_cfg_value(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->v;
}

static inline uint32_t gpio_ll_get_cfg_gpio_input(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_input;
}

static inline void gpio_ll_set_cfg_gpio_output(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->gpio_output = v;
}

static inline uint32_t gpio_ll_get_cfg_gpio_output(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_output;
}

static inline uint32_t gpio_ll_get_cfg_gpio_input_ena(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_input_ena;
}

static inline uint32_t gpio_ll_get_cfg_gpio_output_ena(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_output_ena;
}

static inline void gpio_ll_set_cfg_gpio_pull_mode(uint32_t id,uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->gpio_pull_mode = v;
}

static inline uint32_t gpio_ll_get_cfg_gpio_pull_mode(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_pull_mode;
}

static inline void gpio_ll_set_cfg_gpio_pull_ena(uint32_t id,uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->gpio_pull_ena = v;
}

static inline uint32_t gpio_ll_get_cfg_gpio_pull_ena(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_pull_ena;
}

static inline uint32_t gpio_ll_get_cfg_gpio_fun_ena(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_fun_ena;
}

static inline void gpio_ll_set_cfg_input_monitor(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->input_monitor = v;
}

static inline uint32_t gpio_ll_get_cfg_input_monitor(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->input_monitor;
}

static inline void gpio_ll_set_cfg_gpio_capacity(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->gpio_capacity = v;
}

static inline uint32_t gpio_ll_get_cfg_gpio_capacity(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_capacity;
}

static inline void gpio_ll_set_cfg_gpio_int_type(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->gpio_int_type = v;
}

static inline uint32_t gpio_ll_get_cfg_gpio_int_type(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_int_type;
}

static inline void gpio_ll_set_cfg_gpio_int_ena(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->gpio_int_ena = v;
}

static inline uint32_t gpio_ll_get_cfg_gpio_int_ena(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_int_ena;
}

static inline void gpio_ll_set_cfg_gpio_int_clear(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->gpio_int_clear = v;
}

static inline uint32_t gpio_ll_get_cfg_gpio_int_clear(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_int_clear;
}

static inline void gpio_ll_set_cfg_reserved_14_15(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->reserved_14_15 = v;
}

static inline uint32_t gpio_ll_get_cfg_reserved_14_15(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->reserved_14_15;
}

static inline uint32_t gpio_ll_get_cfg_gpio_state(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_state;
}

static inline void gpio_ll_set_cfg_reserved_21_23(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->reserved_21_23 = v;
}

static inline uint32_t gpio_ll_get_cfg_reserved_21_23(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->reserved_21_23;
}

static inline void gpio_ll_set_cfg_gpio_fun_sel(uint32_t id, uint32_t v) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	r->gpio_fun_sel = v;
}

static inline uint32_t gpio_ll_get_cfg_gpio_fun_sel(uint32_t id) {
	gpio_cfg_t *r = (gpio_cfg_t*)(GPIO_LL_REG_BASE + ((0x0 + id) << 2));
	return r->gpio_fun_sel;
}

//reg intsta0:

static inline void gpio_ll_set_intsta0_value(uint32_t v) {
	gpio_intsta0_t *r = (gpio_intsta0_t*)(GPIO_LL_REG_BASE + (0x78 << 2));
	r->v = v;
}

static inline uint32_t gpio_ll_get_intsta0_value(void) {
	gpio_intsta0_t *r = (gpio_intsta0_t*)(GPIO_LL_REG_BASE + (0x78 << 2));
	return r->v;
}

static inline uint32_t gpio_ll_get_intsta0_gpio_intsta0(void) {
	gpio_intsta0_t *r = (gpio_intsta0_t*)(GPIO_LL_REG_BASE + (0x78 << 2));
	return r->gpio_intsta0;
}

//reg intsta1:

static inline void gpio_ll_set_intsta1_value(uint32_t v) {
	gpio_intsta1_t *r = (gpio_intsta1_t*)(GPIO_LL_REG_BASE + (0x79 << 2));
	r->v = v;
}

static inline uint32_t gpio_ll_get_intsta1_value(void) {
	gpio_intsta1_t *r = (gpio_intsta1_t*)(GPIO_LL_REG_BASE + (0x79 << 2));
	return r->v;
}

static inline uint32_t gpio_ll_get_intsta1_gpio_intsta1(void) {
	gpio_intsta1_t *r = (gpio_intsta1_t*)(GPIO_LL_REG_BASE + (0x79 << 2));
	return r->gpio_intsta1;
}

//reg intsta2:

static inline void gpio_ll_set_intsta2_value(uint32_t v) {
	gpio_intsta2_t *r = (gpio_intsta2_t*)(GPIO_LL_REG_BASE + (0x7a << 2));
	r->v = v;
}

static inline uint32_t gpio_ll_get_intsta2_value(void) {
	gpio_intsta2_t *r = (gpio_intsta2_t*)(GPIO_LL_REG_BASE + (0x7a << 2));
	return r->v;
}

static inline uint32_t gpio_ll_get_intsta2_gpio_intsta2(void) {
	gpio_intsta2_t *r = (gpio_intsta2_t*)(GPIO_LL_REG_BASE + (0x7a << 2));
	return r->gpio_intsta2;
}

//reg int_mask:

static inline void gpio_ll_set_int_mask_value(uint32_t v) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	r->v = v;
}

static inline uint32_t gpio_ll_get_int_mask_value(void) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	return r->v;
}

static inline void gpio_ll_set_int_mask_m52_gpoup_int_mask(uint32_t v) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	r->m52_gpoup_int_mask = v;
}

static inline uint32_t gpio_ll_get_int_mask_m52_gpoup_int_mask(void) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	return r->m52_gpoup_int_mask;
}

static inline void gpio_ll_set_int_mask_m52_even_int_mask(uint32_t v) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	r->m52_even_int_mask = v;
}

static inline uint32_t gpio_ll_get_int_mask_m52_even_int_mask(void) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	return r->m52_even_int_mask;
}

static inline void gpio_ll_set_int_mask_m52_odd_int_mask(uint32_t v) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	r->m52_odd_int_mask = v;
}

static inline uint32_t gpio_ll_get_int_mask_m52_odd_int_mask(void) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	return r->m52_odd_int_mask;
}

static inline void gpio_ll_set_int_mask_reserved_11_15(uint32_t v) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	r->reserved_11_15 = v;
}

static inline uint32_t gpio_ll_get_int_mask_reserved_11_15(void) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	return r->reserved_11_15;
}

static inline void gpio_ll_set_int_mask_m55_gpoup_int_mask(uint32_t v) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	r->m55_gpoup_int_mask = v;
}

static inline uint32_t gpio_ll_get_int_mask_m55_gpoup_int_mask(void) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	return r->m55_gpoup_int_mask;
}

static inline void gpio_ll_set_int_mask_m55_even_int_mask(uint32_t v) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	r->m55_even_int_mask = v;
}

static inline uint32_t gpio_ll_get_int_mask_m55_even_int_mask(void) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	return r->m55_even_int_mask;
}

static inline void gpio_ll_set_int_mask_m55_odd_int_mask(uint32_t v) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	r->m55_odd_int_mask = v;
}

static inline uint32_t gpio_ll_get_int_mask_m55_odd_int_mask(void) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	return r->m55_odd_int_mask;
}

static inline void gpio_ll_set_int_mask_reserved_27_31(uint32_t v) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	r->reserved_27_31 = v;
}

static inline uint32_t gpio_ll_get_int_mask_reserved_27_31(void) {
	gpio_int_mask_t *r = (gpio_int_mask_t*)(GPIO_LL_REG_BASE + (0x7c << 2));
	return r->reserved_27_31;
}

static inline void gpio_ll_get_interrupt_status(gpio_interrupt_status_t *gpio_status)
{
	gpio_status->gpio_0_31_int_status = gpio_ll_get_intsta0_gpio_intsta0();
	gpio_status->gpio_32_63_int_status = gpio_ll_get_intsta1_gpio_intsta1();
	gpio_status->gpio_64_71_int_status = gpio_ll_get_intsta2_gpio_intsta2();
}

static inline bool gpio_ll_is_interrupt_triggered(uint32_t index, gpio_interrupt_status_t *gpio_status)
{
	if (index < 32) {
		return !!((gpio_status->gpio_0_31_int_status) & (0x1 << index));
	} else if (index < 64) {
		return !!((gpio_status->gpio_32_63_int_status) & (0x1 << (index - 32)));
	} else {
		return !!((gpio_status->gpio_64_71_int_status) & (0x1 << (index - 64)));
	}
}

#ifdef __cplusplus
}
#endif
