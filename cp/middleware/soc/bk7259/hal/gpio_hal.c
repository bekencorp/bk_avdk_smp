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

#include "gpio_hw.h"
#include <driver/gpio_types.h>
#include "gpio_hal_v2px.h"
#include "gpio_ll.h"

#define GPIO_CFG_RESERVED_16_23_MASK    (0x00FF0000U)
#define GPIO_CFG_RESTORE_MASK           (0xFF001FB2U)

#define GPIO_FOREACH(i) \
	for (gpio_id_t i = 0; i < GPIO_NUM_MAX; i++)

bk_err_t gpio_hal_init(void)
{
	return BK_OK;
}

bk_err_t gpio_hal_deinit(void)
{
	return BK_OK;
}

void gpio_hal_set_value(gpio_id_t id, uint32_t v)
{
	gpio_ll_set_cfg_value(id, v);
}

uint32_t gpio_hal_get_value(gpio_id_t id)
{
	return gpio_ll_get_cfg_value(id);
}

bk_err_t gpio_hal_set_func_code(gpio_id_t gpio_id, uint32_t code)
{
	gpio_ll_set_cfg_gpio_fun_sel(gpio_id, code);

	return BK_OK;
}

uint32_t gpio_hal_get_func_code(gpio_id_t gpio_id)
{
	return gpio_ll_get_cfg_gpio_fun_sel(gpio_id);
}

bk_err_t gpio_hal_pull_up_enable(gpio_id_t gpio_id, uint32 enable)
{
	gpio_ll_set_cfg_gpio_pull_mode(gpio_id, enable);

	return BK_OK;
}

bk_err_t gpio_hal_pull_enable(gpio_id_t gpio_id, uint32 enable)
{
	gpio_ll_set_cfg_gpio_pull_ena(gpio_id, enable);

	return BK_OK;
}

bk_err_t gpio_hal_monitor_input_enable(gpio_id_t gpio_id, uint32 enable)
{
	gpio_ll_set_cfg_input_monitor(gpio_id, enable);

	return BK_OK;
}

bk_err_t gpio_hal_set_capacity(gpio_id_t gpio_id, uint32 capacity)
{
	gpio_ll_set_cfg_gpio_capacity(gpio_id, capacity);

	return BK_OK;
}

bk_err_t gpio_hal_set_output_value(gpio_id_t gpio_id, uint32 output_value)
{
	gpio_ll_set_cfg_gpio_output(gpio_id, output_value);

	return BK_OK;
}

bk_err_t gpio_hal_get_input(gpio_id_t gpio_id)
{
	return gpio_ll_get_cfg_gpio_input(gpio_id);
}

bk_err_t gpio_hal_set_int_type(gpio_id_t gpio_id, uint32_t type)
{
	gpio_ll_set_cfg_gpio_int_type(gpio_id, type);

	return BK_OK;
}

bk_err_t gpio_hal_enable_interrupt(gpio_id_t gpio_id)
{
	gpio_ll_set_cfg_gpio_int_ena(gpio_id, 1);

	return BK_OK;
}

bk_err_t gpio_hal_disable_interrupt(gpio_id_t gpio_id)
{
	gpio_ll_set_cfg_gpio_int_ena(gpio_id, 0);

	return BK_OK;
}

bk_err_t gpio_hal_enable_multi_interrupts(uint64_t gpio_idx)
{
	GPIO_FOREACH(i) {
		if (gpio_idx & BIT64(i)) {
			gpio_ll_set_cfg_gpio_int_ena(i, 1);
		}
	}

	return BK_OK;
}

bk_err_t gpio_hal_disable_all_interrupts(void)
{
	GPIO_FOREACH(i) {
		gpio_ll_set_cfg_gpio_int_ena(i, 0);
		// gpio_ll_set_cfg_gpio_int_clear(i, 1);
		// gpio_ll_set_cfg_gpio_int_clear(i, 0);
	}

	return BK_OK;
}

bk_err_t gpio_hal_get_interrupt_overview(void)
{
	// return iomx_ll_get_gpio_intsta_overview_value();
	return 0;
}

bk_err_t gpio_hal_bakup_configs(uint32_t *gpio_cfgs)
{
	if (!gpio_cfgs) {
		return BK_FAIL;
	}

	GPIO_FOREACH(i) {
		gpio_cfgs[i] = gpio_ll_get_cfg_value(i);
	}

	return BK_OK;
}
bk_err_t gpio_hal_restore_configs(uint32_t *gpio_cfgs)
{
	if (!gpio_cfgs) {
		return BK_FAIL;
	}

	for (gpio_id_t i = 0; i < GPIO_NUM_MAX; i++) {
		uint32_t current_cfg = gpio_ll_get_cfg_value(i);
		uint32_t restore_cfg = (current_cfg & ~GPIO_CFG_RESTORE_MASK) | (gpio_cfgs[i] & GPIO_CFG_RESTORE_MASK);
		gpio_ll_set_cfg_value(i, restore_cfg);
	}

	return BK_OK;
}
