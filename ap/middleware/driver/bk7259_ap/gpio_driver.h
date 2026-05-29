// Copyright 2020-2025 Beken
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

#include <components/log.h>
#include "gpio_hal_v2px.h"
#include "gpio_map.h"
#include <driver/gpio_types.h>

#define GPIO_TAG "gpio"
#define GPIO_LOGI(...) BK_LOGI(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGW(...) BK_LOGW(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGE(...) BK_LOGE(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGD(...) BK_LOGD(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGV(...) BK_LOGV(GPIO_TAG, ##__VA_ARGS__)

typedef enum {
	GPIO_JTAG_MAP_GROUP0 = 0,          /**<GPIO20~GPIO21 is used for jtag */
	GPIO_JTAG_MAP_GROUP1,              /**<GPIO0~GPIO1 is used for jtag */
	GPIO_JTAG_MAP_GROUP_MAX,           /**< Invalid mode*/
} gpio_jtag_map_group_t;

typedef enum {
	GPIO_SCR_MAP_GROUP0 = 0,          /**<GPIO0~GPIO3 is used for scr */
	GPIO_SCR_MAP_GROUP1,              /**<GPIO30~GPIO32,GPIO43 is used for scr */
	GPIO_SCR_MAP_GROUP2,              /**<GPIO40~GPIO43 is used for scr */
	GPIO_SCR_MAP_GROUP_MAX,           /**< Invalid mode*/
} gpio_scr_map_group_t;

typedef struct {
	gpio_id_t gpio_id;
	uint32_t ldo_state;
} gpio_ctrl_ldo_t;

#if CONFIG_USR_GPIO_CFG_EN
typedef struct {
	uint32_t gpio_id:				7;	//gpio_id_t (7 bits to cover GPIO_0..GPIO_71)

	/* if second func en,then second_func_dev value is valid */
	uint32_t second_func_en:		1;	//gpio_func_mode_t
	uint32_t second_func_dev:		12;	//gpio_dev_t

	uint32_t io_mode:				2;	//gpio_io_mode_t
	uint32_t pull_mode:				2;	//gpio_pull_mode_t

	/* if int en and then int_type is valid */
	uint32_t int_en:				1;	//gpio_int_mode_t
	uint32_t int_type:				2;	//gpio_int_type_t

	uint32_t low_power_io_ctrl:		2;	//gpio_lowpower_mode_t

	uint32_t driver_capacity:		2;	//gpio_driver_capacity_t
	uint32_t gpio_skip:             1; //gpio_skip_t
} gpio_default_map_t;
#endif

bk_err_t gpio_dev_map(gpio_id_t gpio_id, gpio_dev_t dev);
bk_err_t gpio_dev_unmap(gpio_id_t gpio_id);
bk_err_t gpio_dev_unprotect_map(gpio_id_t gpio_id, gpio_dev_t dev);
bk_err_t gpio_dev_unprotect_unmap(gpio_id_t gpio_id);
bk_err_t gpio_jtag_sel(gpio_jtag_map_group_t gpio_jtag_sel_mode);
bk_err_t gpio_scr_sel(gpio_scr_map_group_t mode);
IOMX_CODE_T convert_gpio_dev_to_iomx_code(gpio_dev_t dev);
bk_err_t bk_gpio_dump_pin_status(void);

#if CONFIG_GPIO_DUMP_MAP_DEV_DEBUG
bk_err_t bk_gpio_dump_default_map_init_effect(void);
#endif


#if CONFIG_GPIO_WAKEUP_SUPPORT
bk_err_t gpio_enter_low_power(void *param);
bk_err_t gpio_exit_low_power(void *param);
void gpio_get_interrupt_status(uint32_t *h_status, uint32_t *l_status);
bk_err_t gpio_enable_interrupt_mult_for_wake(void);
#endif

#if CONFIG_GPIO_SIMULATE_UART_WRITE
/**
 * @brief	  Uses specifies GPIO to simulate UART write data
 *
 * This API Uses specifies GPIO to simulate UART write data:
 *	 - Uses CPU poll wait to do delay, so it blocks CPU.
 *	 - The caller should confirm the specifies GPIO is not used by other APP.
 *
 * @param *buff  Which buffers will be write with GPIO.
 * @param len    How many bytes data will be wrote.
 * @param gpio_id  Which GPIO will be simulated as UART write data.
 * @param div    Baud rate == 1Mbps/(1+div)
 *
 * @attention 1. As this function just simulate uart write, it blocks the CPU,
 *               so please don't write too much data.
 *
 * @return
 */
void gpio_simulate_uart_write(unsigned char *buff, uint32_t len, gpio_id_t gpio_id, uint32_t div);
#endif

