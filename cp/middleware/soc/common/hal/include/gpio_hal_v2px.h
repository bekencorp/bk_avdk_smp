// Copyright 2020-2024 Beken
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

#include "hal_config.h"
#include "gpio_hw.h"
#include "gpio_ll.h"

#include <driver/hal/hal_io_matrix_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IOMUX_CFG_GPIO_CNT             (4)
#define IOMUX_CFG_GPIO_FIELD_BITS      (8)
#define IOMUX_FUNC_CODE_MASK           (0xFF)

#if CONFIG_GPIO_WAKEUP_SUPPORT
typedef struct {
	gpio_id_t id;
	gpio_int_type_t int_type;
} gpio_wakeup_t;
#endif

bk_err_t gpio_hal_init(void);
bk_err_t gpio_hal_deinit(void);
void gpio_hal_set_value(gpio_id_t id, uint32_t v);
uint32_t gpio_hal_get_value(gpio_id_t id);
bk_err_t gpio_hal_set_func_code(gpio_id_t gpio_id, uint32_t code);
uint32_t gpio_hal_get_func_code(gpio_id_t gpio_id);
bk_err_t gpio_hal_pull_up_enable(gpio_id_t gpio_id, uint32 enable);
bk_err_t gpio_hal_pull_enable(gpio_id_t gpio_id, uint32 enable);
bk_err_t gpio_hal_monitor_input_enable(gpio_id_t gpio_id, uint32 enable);
bk_err_t gpio_hal_set_capacity(gpio_id_t gpio_id, uint32 capacity);
bk_err_t gpio_hal_set_output_value(gpio_id_t gpio_id, uint32 output_value);
bk_err_t gpio_hal_get_input(gpio_id_t gpio_id);
bk_err_t gpio_hal_set_int_type(gpio_id_t gpio_id, uint32_t type);
bk_err_t gpio_hal_enable_interrupt(gpio_id_t gpio_id);
bk_err_t gpio_hal_disable_interrupt(gpio_id_t gpio_id);
bk_err_t gpio_hal_enable_multi_interrupts(uint64_t gpio_idx);
bk_err_t gpio_hal_disable_all_interrupts(void);
bk_err_t gpio_hal_get_interrupt_overview(void);
bk_err_t gpio_hal_bakup_configs(uint32_t *iomx_gpio_cfgs);
bk_err_t gpio_hal_restore_configs(uint32_t *iomx_gpio_cfgs);

#define gpio_hal_get_interrupt_status(status)           gpio_ll_get_interrupt_status(status)
#define gpio_hal_clear_interrupt_status(tatus)
#define gpio_hal_is_interrupt_triggered(id, status)     gpio_ll_is_interrupt_triggered(id, status)
#define gpio_hal_clear_chan_interrupt_status(id)        gpio_ll_set_cfg_gpio_int_clear(id, 1); \
                                                        gpio_ll_set_cfg_gpio_int_clear(id, 0) // workaround fix

#if CFG_HAL_DEBUG_GPIO
void gpio_regs_dump(void);
void gpio_struct_dump(uint32_t start, uint32_t end);
#else
#define gpio_regs_dump()
#define gpio_struct_dump(start, end)
#endif

#ifdef __cplusplus
}
#endif
