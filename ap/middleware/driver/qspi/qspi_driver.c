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

#include <driver/int.h>
#include <os/mem.h>
#include <driver/qspi.h>
#include "bk_sys_ctrl.h"
#include "clock_driver.h"
#include "gpio_driver.h"
#include "power_driver.h"
#include "qspi_driver.h"
#include "qspi_statis.h"
#include "sys_driver.h"
#include <modules/pm.h>
#include <driver/gpio.h>

static qspi_driver_t s_qspi[SOC_QSPI_UNIT_NUM] = {
	{
		.hal.hw = (qspi_hw_t *)(SOC_QSPI0_REG_BASE),
	},
#if (SOC_QSPI_UNIT_NUM > 1)
	{
		.hal.hw = (qspi_hw_t *)(SOC_QSPI1_REG_BASE),
	}
#endif
};

#define QSPI_RETURN_ON_NOT_INIT() do {\
		if (!s_qspi_driver_is_init) {\
			QSPI_LOGE("QSPI driver not init\r\n");\
			return BK_ERR_QSPI_NOT_INIT;\
		}\
	} while(0)

#define QSPI_RETURN_ON_ID_NOT_INIT(id) do {\
		if (!s_qspi[id].id_init_bits) {\
			QSPI_LOGE("QSPI not init\r\n");\
			return BK_ERR_QSPI_ID_NOT_INIT;\
		}\
	} while(0)


static bool s_qspi_driver_is_init = false;
static qspi_callback_t s_qspi_tx_isr = {NULL};
static qspi_callback_t s_qspi_rx_isr = {NULL};

#if CONFIG_USR_GPIO_CFG_EN
#if(CONFIG_QSPI_LINE_MODE == 1)
#define QSPI_SET_PIN(id) do {\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_CSN);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_CLK);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_IO0);\
} while(0)
#elif (CONFIG_QSPI_LINE_MODE == 2)
#define QSPI_SET_PIN(id) do {\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_CSN);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_CLK);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_IO0);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_IO1);\
} while(0)
#else
#define QSPI_SET_PIN(id) do {\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_CSN);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_CLK);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_IO0);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_IO1);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_IO2);\
	gpio_dev_map_by_func(GPIO_DEV_QSPI##id##_IO3);\
} while(0)
#endif
#endif

static void qspi_init_gpio(qspi_id_t id)
{
#if CONFIG_USR_GPIO_CFG_EN
	switch (id) {
	case QSPI_ID_0:
		QSPI_SET_PIN(0);
		break;
#if (SOC_QSPI_UNIT_NUM > 1)
	case QSPI_ID_1:
		QSPI_SET_PIN(1);
		break;
#endif
#if (SOC_QSPI_UNIT_NUM > 2)
	case QSPI_ID_2:
		QSPI_SET_PIN(2);
		break;
#endif
	default:
		break;
	}
#endif
}

static void qspi_clock_enable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			bk_pm_clock_ctrl(CLK_PWR_ID_QSPI0, CLK_PWR_CTRL_PWR_UP);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			bk_pm_clock_ctrl(CLK_PWR_ID_QSPI1, CLK_PWR_CTRL_PWR_UP);
			break;
#endif
		default:
			break;
	}
}

static void qspi_clock_disable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			bk_pm_clock_ctrl(CLK_PWR_ID_QSPI0, CLK_PWR_CTRL_PWR_DOWN);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			bk_pm_clock_ctrl(CLK_PWR_ID_QSPI1, CLK_PWR_CTRL_PWR_DOWN);
			break;
#endif
		default:
			break;
	}
}

static void qspi_interrupt_enable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_set_int_en(0, INT_SRC_QSPI0, 1);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_set_int_en(0, INT_SRC_QSPI1, 1);
			break;
#endif
		default:
			break;
	}
}

static void qspi_interrupt_disable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_set_int_en(0, INT_SRC_QSPI0, 0);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_set_int_en(0, INT_SRC_QSPI1, 0);
			break;
#endif
		default:
			break;
	}
}

/*
 * 1. set clock
 * 2. set gpio as qspi
 * 3. enable interrupt(fiq_int_enable)
 */
static void qspi_id_init_common(qspi_id_t id)
{
	qspi_clock_enable(id);
	qspi_init_gpio(id);
	qspi_interrupt_enable(id);

	qspi_hal_init_common(&s_qspi[id].hal);
}

static void qspi_id_deinit_common(qspi_id_t id)
{
	qspi_hal_deinit_common(&s_qspi[id].hal);
	qspi_interrupt_disable(id);
	qspi_clock_disable(id);
}

bk_err_t bk_qspi_driver_init(void)
{
	if (s_qspi_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_qspi, 0, sizeof(s_qspi));
	for (int id = QSPI_ID_0; id < QSPI_ID_MAX; id++) {
		s_qspi[id].hal.id = id;
		qspi_hal_init(&s_qspi[id].hal);
	}

	qspi_statis_init();
	s_qspi_driver_is_init = true;

#if CONFIG_CLI && CONFIG_QSPI_TEST
	int bk_qspi_register_cli_test_feature(void);
	bk_qspi_register_cli_test_feature();
#endif

	return BK_OK;
}

bk_err_t bk_qspi_driver_deinit(void)
{
	if (!s_qspi_driver_is_init) {
		return BK_OK;
	}

	for (int id = QSPI_ID_0; id < QSPI_ID_MAX; id++) {
		qspi_id_deinit_common(id);
	}

	s_qspi_driver_is_init = false;

	return BK_OK;
}

bk_err_t bk_qspi_init(qspi_id_t id, const qspi_config_t *config)
{
	BK_RETURN_ON_NULL(config);
	QSPI_RETURN_ON_NOT_INIT();

	if (id == QSPI_ID_0) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI, PM_POWER_MODULE_STATE_ON);
#if (SOC_QSPI_UNIT_NUM > 1)
	} else if (id == QSPI_ID_1) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI1, PM_POWER_MODULE_STATE_ON);
#endif
	}

	qspi_id_init_common(id);
	qspi_hal_set_clock_source(id, config->src_clk);
	sys_drv_qspi_set_src_clk_div(id, config->src_clk_div);
	qspi_hal_set_clk_div(&s_qspi[id].hal, config->clk_div);
	s_qspi[id].id_init_bits |= BIT(0);
	return BK_OK;
}

bk_err_t bk_qspi_deinit(qspi_id_t id)
{
	qspi_id_deinit_common(id);
	s_qspi[id].id_init_bits &= ~BIT(0);

	if (id == QSPI_ID_0) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI, PM_POWER_MODULE_STATE_OFF);
#if (SOC_QSPI_UNIT_NUM > 1)
	} else if (id == QSPI_ID_1) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI1, PM_POWER_MODULE_STATE_OFF);
#endif
	}

	return BK_OK;
}

bk_err_t bk_qspi_command(qspi_id_t id, const qspi_cmd_t *cmd)
{
	BK_RETURN_ON_NULL(cmd);
	QSPI_RETURN_ON_ID_NOT_INIT(id);
	qspi_hal_command(&s_qspi[id].hal, cmd);
	return BK_OK;
}

bk_err_t bk_qspi_write(qspi_id_t id, const void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);
	QSPI_RETURN_ON_NOT_INIT();
	QSPI_RETURN_ON_ID_NOT_INIT(id);

	qspi_hal_io_write(&s_qspi[id].hal, data, size);

	return BK_OK;
}

bk_err_t bk_qspi_read(qspi_id_t id, void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);

	qspi_hal_io_read(&s_qspi[id].hal, data, size);

	return BK_OK;
}

bk_err_t bk_qspi_register_tx_isr(qspi_isr_t isr, void *param)
{
	uint32_t int_level = rtos_enter_critical();
	s_qspi_tx_isr.callback = isr;
	s_qspi_tx_isr.param = param;
	rtos_exit_critical(int_level);
	return BK_OK;
}

bk_err_t bk_qspi_register_rx_isr(qspi_isr_t isr, void *param)
{
	uint32_t int_level = rtos_enter_critical();
	s_qspi_rx_isr.callback = isr;
	s_qspi_rx_isr.param = param;
	rtos_exit_critical(int_level);
	return BK_OK;
}
//eof
