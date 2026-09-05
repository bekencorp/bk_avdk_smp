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
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
#include "cmsis_gcc.h"
#endif

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

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
#define QSPI_FAST_BACKUP_REG_NUM (28U)

typedef struct {
	uint32_t regs[SOC_QSPI_UNIT_NUM][QSPI_FAST_BACKUP_REG_NUM];
	qspi_config_t config[SOC_QSPI_UNIT_NUM];
	uint32_t valid_mask;
	volatile uint32_t busy_count[SOC_QSPI_UNIT_NUM];
	volatile bool suspended;
	bool registered;
} qspi_fast_pm_context_t;

static qspi_fast_pm_context_t s_qspi_fast_pm;
#endif

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

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
static bk_err_t qspi_fast_transfer_enter(qspi_id_t id)
{
	if (__atomic_load_n(&s_qspi_fast_pm.suspended, __ATOMIC_ACQUIRE)) {
		return BK_ERR_BUSY;
	}
	__atomic_add_fetch(&s_qspi_fast_pm.busy_count[id], 1U,
		__ATOMIC_ACQ_REL);
	if (__atomic_load_n(&s_qspi_fast_pm.suspended, __ATOMIC_ACQUIRE)) {
		__atomic_sub_fetch(&s_qspi_fast_pm.busy_count[id], 1U,
			__ATOMIC_RELEASE);
		return BK_ERR_BUSY;
	}
	return BK_OK;
}

static void qspi_fast_transfer_exit(qspi_id_t id)
{
	__atomic_sub_fetch(&s_qspi_fast_pm.busy_count[id], 1U,
		__ATOMIC_RELEASE);
}

static bk_err_t qspi_fast_quiesce(void *arg)
{
	qspi_fast_pm_context_t *ctx = arg;

	__atomic_store_n(&ctx->suspended, true, __ATOMIC_RELEASE);
	for (qspi_id_t id = QSPI_ID_0; id < QSPI_ID_MAX; id++) {
		qspi_hw_t *hw = s_qspi[id].hal.hw;
		if (__atomic_load_n(&ctx->busy_count[id], __ATOMIC_ACQUIRE) ||
			hw->status.rx_busy || hw->status.tx_busy ||
			hw->cmd_c_cfg2.cmd_start || hw->cmd_d_cfg2.cmd_start) {
			__atomic_store_n(&ctx->suspended, false,
				__ATOMIC_RELEASE);
			return BK_ERR_BUSY;
		}
	}
	return BK_OK;
}

static void qspi_fast_vote_power_on(qspi_id_t id)
{
	if (id == QSPI_ID_0) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI,
			PM_POWER_MODULE_STATE_ON);
#if (SOC_QSPI_UNIT_NUM > 1)
	} else if (id == QSPI_ID_1) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI1,
			PM_POWER_MODULE_STATE_ON);
#endif
	}
}

static void qspi_fast_backup_one(qspi_id_t id, uint32_t *backup)
{
	volatile uint32_t *raw = (volatile uint32_t *)s_qspi[id].hal.hw;

	backup[0] = raw[0x02];
	for (uint32_t i = 0U; i < 19U; i++) {
		backup[1U + i] = raw[0x08U + i];
	}
	for (uint32_t i = 0U; i < 8U; i++) {
		backup[20U + i] = raw[0x1DU + i];
	}
	raw[0x02] = 0U;
}

static void qspi_fast_restore_one(qspi_id_t id, const uint32_t *backup)
{
	volatile uint32_t *raw = (volatile uint32_t *)s_qspi[id].hal.hw;
	qspi_hw_t *hw = s_qspi[id].hal.hw;

	for (uint32_t i = 0U; i < 19U; i++) {
		raw[0x08U + i] = backup[1U + i];
	}
	for (uint32_t i = 0U; i < 8U; i++) {
		raw[0x1DU + i] = backup[20U + i];
	}
	raw[0x02] = backup[0];
	hw->glb_ctrl.bps_clkgate = 1;
}

static void qspi_fast_recover_sys(qspi_id_t id, const qspi_config_t *config)
{
	qspi_init_gpio(id);
	qspi_interrupt_enable(id);
	qspi_hal_set_clock_source(id, config->src_clk);
	sys_drv_qspi_set_src_clk_div(id, config->src_clk_div);
}

static bk_err_t qspi_fast_backup(void *arg)
{
	qspi_fast_pm_context_t *ctx = arg;

	ctx->valid_mask = 0U;
	for (qspi_id_t id = QSPI_ID_0; id < QSPI_ID_MAX; id++) {
		if (!s_qspi[id].id_init_bits) {
			continue;
		}
		qspi_fast_backup_one(id, ctx->regs[id]);
		ctx->valid_mask |= BIT(id);
	}
	__DMB();
	return BK_OK;
}

static bk_err_t qspi_fast_restore(void *arg)
{
	qspi_fast_pm_context_t *ctx = arg;

	for (qspi_id_t id = QSPI_ID_0; id < QSPI_ID_MAX; id++) {
		if (!(ctx->valid_mask & BIT(id))) {
			continue;
		}
		qspi_fast_vote_power_on(id);
		qspi_clock_enable(id);
		qspi_fast_restore_one(id, ctx->regs[id]);
	}
	__DMB();
	return BK_OK;
}

static bk_err_t qspi_fast_resume(void *arg)
{
	qspi_fast_pm_context_t *ctx = arg;

	for (qspi_id_t id = QSPI_ID_0; id < QSPI_ID_MAX; id++) {
		if (!(ctx->valid_mask & BIT(id))) {
			continue;
		}
		qspi_fast_recover_sys(id, &ctx->config[id]);
	}
	ctx->valid_mask = 0U;
	__DMB();
	__atomic_store_n(&ctx->suspended, false, __ATOMIC_RELEASE);
	return BK_OK;
}

static const pm_ap_fast_pm_ops_t s_qspi_fast_ops = {
	.name = "qspi",
	.quiesce = qspi_fast_quiesce,
	.backup = qspi_fast_backup,
	.restore = qspi_fast_restore,
	.resume = qspi_fast_resume,
	.arg = &s_qspi_fast_pm,
	.priority = PM_AP_FAST_PRIORITY_PERIPHERAL,
};
#endif

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

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	bk_err_t pm_ret = bk_pm_ap_fast_ops_register(&s_qspi_fast_ops);
	if (pm_ret != BK_OK) {
		return pm_ret;
	}
	s_qspi_fast_pm.registered = true;
#endif

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

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	if (s_qspi_fast_pm.registered) {
		bk_err_t pm_ret = bk_pm_ap_fast_ops_unregister(&s_qspi_fast_ops);
		if (pm_ret != BK_OK) {
			return pm_ret;
		}
		os_memset(&s_qspi_fast_pm, 0, sizeof(s_qspi_fast_pm));
	}
#endif

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
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	s_qspi_fast_pm.config[id] = *config;
#endif
	return BK_OK;
}

/* The QSPI internal clk_div is no longer effective in hardware, so SCK is
 * determined solely by the source clock divider:
 *   SCK = src_clock / (1 + src_clk_div)
 * Pick the achievable src_clk/src_clk_div combination whose frequency is
 * closest to clk_hz. src_clk_div is 4-bit but restricted to 2~15 here, so the
 * achievable SCK range is 10MHz~80MHz. clk_div is kept at 0. */
static void qspi_calc_clock_config(qspi_config_t *config, uint32_t clk_hz)
{
	static const struct {
		uint32_t hz;
		qspi_src_clk_t sel;
	} src_tbl[] = {
		{ 240000000u, QSPI_SCLK_240M },
		{ 160000000u, QSPI_SCLK_160M },
	};
	uint32_t best_diff = 0xFFFFFFFFu;

	/* fallback default = 240M / (1 + 3) = 60MHz */
	config->src_clk = QSPI_SCLK_240M;
	config->src_clk_div = 3;
	config->clk_div = 0;

	for (uint32_t s = 0; s < sizeof(src_tbl) / sizeof(src_tbl[0]); s++) {
		for (uint32_t sd = 2; sd <= 15; sd++) {
			uint32_t f = src_tbl[s].hz / (1u + sd);
			if (f > 80000000u) {
				continue;
			}
			uint32_t diff = (f > clk_hz) ? (f - clk_hz) : (clk_hz - f);
			if (diff < best_diff) {
				best_diff = diff;
				config->src_clk = src_tbl[s].sel;
				config->src_clk_div = sd;
				config->clk_div = 0;
			}
		}
	}
}

bk_err_t bk_qspi_init_by_freq(qspi_id_t id, uint32_t clk_hz)
{
	qspi_config_t config = {0};

	qspi_calc_clock_config(&config, clk_hz);
	uint32_t src_hz = (config.src_clk == QSPI_SCLK_240M) ? 240000000u : 160000000u;
	uint32_t actual_hz = src_hz / (1u + config.src_clk_div);
	QSPI_LOGI("init clk: target=%u Hz, src_clk=%uMHz, src_clk_div=%u, actual=%u Hz\r\n",
	          clk_hz, src_hz / 1000000u, config.src_clk_div, actual_hz);

	return bk_qspi_init(id, &config);
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
	bk_err_t ret;
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	ret = qspi_fast_transfer_enter(id);
	if (ret != BK_OK) {
		return ret;
	}
#endif
	ret = qspi_hal_command(&s_qspi[id].hal, cmd);
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	qspi_fast_transfer_exit(id);
#endif
	return ret;
}

bk_err_t bk_qspi_write(qspi_id_t id, const void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);
	QSPI_RETURN_ON_NOT_INIT();
	QSPI_RETURN_ON_ID_NOT_INIT(id);

	bk_err_t ret;
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	ret = qspi_fast_transfer_enter(id);
	if (ret != BK_OK) {
		return ret;
	}
#endif
	ret = qspi_hal_io_write(&s_qspi[id].hal, data, size);
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	qspi_fast_transfer_exit(id);
#endif
	return ret;
}

bk_err_t bk_qspi_read(qspi_id_t id, void *data, uint32_t size)
{
	BK_RETURN_ON_NULL(data);
	QSPI_RETURN_ON_NOT_INIT();
	QSPI_RETURN_ON_ID_NOT_INIT(id);

	bk_err_t ret;
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	ret = qspi_fast_transfer_enter(id);
	if (ret != BK_OK) {
		return ret;
	}
#endif
	ret = qspi_hal_io_read(&s_qspi[id].hal, data, size);
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	qspi_fast_transfer_exit(id);
#endif
	return ret;
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
