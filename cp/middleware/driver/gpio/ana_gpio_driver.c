// Copyright 2020-2021 Beken
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

#include <common/bk_include.h>
#include <soc/soc.h>

#include "ana_gpio_driver.h"
#include "sys_driver.h"
#if CONFIG_ANA_GPIO
#include "io_matrix_driver.h"
#include "bk_intc.h"
#include "sys_hal.h"
#include "aon_pmu_hal.h"
#endif

#if CONFIG_ANA_GPIO
gpio_id_t ana_gpio_get_wakeup_pin(void)
{
	gpio_id_t gpio_id = SOC_GPIO_NUM;
	uint32_t gpio_sta = aon_pmu_hal_get_ana_gpio_status();

	for (uint32_t i = 0; i < SOC_GPIO_NUM; i++)
	{
		if (gpio_sta & BIT(i))
		{
			if (gpio_id == SOC_GPIO_NUM) {
				gpio_id = i;
				sys_drv_wakeup_source_clear();
				break;
			} else {
				// TODO: let user know multiple GPIOs were detected triggering wake-up at the same time
				// BK_ASSERT(0);
			}
		}
	}

	return gpio_id;
}

static void __BK_IRQ ana_gpio_isr_handler(void)
{
	uint32_t gpio_id = SOC_GPIO_NUM, gpio_idx;
	uint32_t gpio_sta = aon_pmu_hal_get_ana_gpio_status();

	for (uint32_t i = 0; i < SOC_GPIO_NUM; i++)
	{
		if (gpio_sta & BIT(i))
		{
			gpio_id = i;
			ANA_GPIO_LOGV("gpio int: index:%d \r\n", gpio_id);
			iomx_exec_iomx_gpio_isr(gpio_id);
		}
	}

	if (gpio_id == SOC_GPIO_NUM)
	{
		gpio_idx = sys_hal_get_ana_reg11_gpiowk();
		ANA_GPIO_LOGW("unexpected gpio int: idx:%x sta:%x\r\n", gpio_idx, gpio_sta);
	}

	ana_gpio_clear_wakeup_source();
}

bk_err_t ana_gpio_wakeup_init(void)
{
	bk_int_isr_register(INT_SRC_ANA_GPIO, ana_gpio_isr_handler, NULL);

	return BK_OK;
}

bk_err_t ana_gpio_wakeup_deinit(void)
{
	bk_int_isr_unregister(INT_SRC_ANA_GPIO);
	// bk_int_isr_unregister(INT_SRC_ABNORMAL_GPIO);

	return BK_OK;
}

bk_err_t ana_gpio_config_wakeup_source(uint64_t gpio_bitmap)
{
	sys_hal_set_ana_reg8_spi_latch1v(1);
	sys_hal_set_ana_reg11_gpiowk((uint32_t)gpio_bitmap);
	sys_hal_set_ana_reg8_spi_latch1v(0);

	return BK_OK;
}

bk_err_t ana_gpio_clear_wakeup_source(void)
{
	sys_hal_set_ana_reg8_spi_latch1v(1);
	sys_hal_set_ana_reg11_gpiowk(0);
	sys_hal_set_ana_reg8_spi_latch1v(0);

	return BK_OK;
}
#endif

#if CONFIG_GPIO_ANA_WAKEUP_SUPPORT
#define GPIO_ANA_WAKEUP_MAX (2)

static uint32_t s_wkup_cnt;
static gpio_wakeup_config_t s_wkup_cfg[GPIO_ANA_WAKEUP_MAX];

static int gpio_ana_enter_cb(uint64_t sleep_time, void *args)
{
	for (uint32_t i = 0; i < MIN(s_wkup_cnt, GPIO_ANA_WAKEUP_MAX); i++) {
		sys_drv_gpio_ana_wakeup_enable(i, s_wkup_cfg[i].id,
			(uint32_t)s_wkup_cfg[i].int_type);
	}

	return 0;
}

bk_err_t bk_gpio_ana_register_wakeup_source(gpio_id_t gpio_id, gpio_int_type_t int_type)
{
	pm_cb_conf_t enter_conf;

	if (gpio_id >= SOC_GPIO_NUM) {
		return BK_ERR_GPIO_CHAN_ID;
	}

	if (int_type >= GPIO_INT_TYPE_MAX) {
		return BK_ERR_GPIO_INVALID_INT_TYPE;
	}

	if (gpio_id > GPIO_15 || int_type > GPIO_INT_TYPE_HIGH_LEVEL) {
		ANA_GPIO_LOGE("gpio ana wakeup source not support id: %d type: %d\r\n", gpio_id, int_type);
		return BK_ERR_ANA_GPIO_TYPE_NOT_SUPPORT;
	}

	for (uint32_t i = 0; i < s_wkup_cnt; i++) {
		if (s_wkup_cfg[i].id == gpio_id) {
			s_wkup_cfg[i].int_type = int_type;
			ANA_GPIO_LOGI("update ana wakeup gpio id: %d type: %d\r\n", gpio_id, int_type);
			return BK_OK;
		}
	}

	if (s_wkup_cnt >= GPIO_ANA_WAKEUP_MAX) {
		ANA_GPIO_LOGE("too many ana gpio wakeup sources, max: %d\r\n", GPIO_ANA_WAKEUP_MAX);
		return BK_ERR_GPIO_WAKESOURCE_OVER_MAX_CNT;
	}

	s_wkup_cfg[s_wkup_cnt].id = gpio_id;
	s_wkup_cfg[s_wkup_cnt].int_type = int_type;
	s_wkup_cfg[s_wkup_cnt].valid = 1;
	s_wkup_cnt++;
	ANA_GPIO_LOGI("regist wakeup source gpio id: %d type: %d\r\n", gpio_id, int_type);

	enter_conf.cb = gpio_ana_enter_cb;
	enter_conf.args = NULL;

	return bk_pm_sleep_register_cb(PM_MODE_SUPER_DEEP_SLEEP, PM_DEV_ID_GPIO, &enter_conf, NULL);
}
#endif