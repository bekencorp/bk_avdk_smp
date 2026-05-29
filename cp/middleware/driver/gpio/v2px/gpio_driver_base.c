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

#include <common/bk_include.h>
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_hal_v2px.h"
#include "gpio_driver_base.h"
#include "sys_driver.h"
#if CONFIG_ANA_GPIO
#include "ana_gpio_driver.h"
#endif
#include "bk_misc.h"
#if CONFIG_USR_GPIO_CFG_EN
#include "gpio_driver.h"
#include "usr_gpio_cfg.h"
#endif

#define GPIO_REG_DEFAULT_VALUE                    (0x0)
#define GPIO_WAKE_SOURCE_IDLE_ID                  (GPIO_NUM_MAX)
#define GPIO_LOWPOWER_KEEP_STATUS_IDLE_ID         (GPIO_NUM_MAX)

#define GPIO_RETURN_ON_INVALID_ID(id) do {\
	if ((id) >= SOC_GPIO_NUM) {\
		return BK_ERR_GPIO_INVALID_ID;\
	}\
} while(0)

#define GPIO_RETURN_ON_INVALID_INT_TYPE_MODE(mode) do {\
	if ((mode) >= GPIO_INT_TYPE_MAX) {\
		return BK_ERR_GPIO_INVALID_INT_TYPE;\
	}\
} while(0)

#define GPIO_RETURN_ON_INVALID_IO_MODE(mode) do {\
	if (((mode)) >= GPIO_IO_INVALID) {\
		return BK_ERR_GPIO_INVALID_MODE;\
	}\
} while(0)

#define GPIO_RETURN_ON_INVALID_PULL_MODE(mode) do {\
	if (((mode)) >= GPIO_PULL_INVALID) {\
		return BK_ERR_GPIO_INVALID_MODE;\
	}\
} while(0)

typedef struct
{
	gpio_id_t id;
	gpio_int_type_t int_type;
	//gpio_isr_t isr;
} gpio_dynamic_wakeup_t;

typedef struct
{
	gpio_id_t gpio_id;
	gpio_config_t config;
} gpio_dynamic_keep_status_t;

static bool s_gpio_is_init = false;
static gpio_isr_t s_gpio_isr[SOC_GPIO_NUM] = {NULL};
static uint32_t s_gpio_baked_regs[SOC_GPIO_NUM] = {0};

#if CONFIG_GPIO_DUMP_MAP_DEV_DEBUG
/* Snapshot of every GPIO cfg register taken right before and right after the
 * gpio_default_map_init() call inside bk_gpio_driver_init(). They are kept in
 * RAM so the caller can later (once UART is up) confirm whether the table
 * GPIO_DEFAULT_DEV_CONFIG actually changed any pad. */
static uint32_t s_gpio_default_map_before[SOC_GPIO_NUM];
static uint32_t s_gpio_default_map_after[SOC_GPIO_NUM];
static bool     s_gpio_default_map_snapshot_valid;
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
static uint64_t s_gpio_is_setted_wake_status;
static gpio_id_t s_gpio_wakeup_gpio_id = SOC_GPIO_NUM;
#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
static gpio_dynamic_wakeup_t s_gpio_dynamic_wakeup_source_map[CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT];
#endif
#endif

#if CONFIG_GPIO_KPSTAT_SUPPORT
static uint64_t s_gpio_is_lowpower_keep_status;
#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
static gpio_dynamic_keep_status_t s_gpio_lowpower_keep_config[CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT];
#endif
#endif


#if CONFIG_USR_GPIO_CFG_EN
static void gpio_default_map_init(void);
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
static void gpio_wakeup_source_config(void);
static void gpio_record_wakeup_pin_id(void);
#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
static void gpio_dynamic_wakeup_source_init(void);
#endif
#endif

#if CONFIG_GPIO_KPSTAT_SUPPORT
static void gpio_keep_status_init(void);
#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
static void gpio_keep_status_config(void);
#endif
#endif

static void gpio_isr(void);
bk_err_t bk_gpio_driver_init(void)
{
	//avoid re-init caused some info lost
	if (s_gpio_is_init) {
		GPIO_LOGD("%s:has inited \r\n", __func__);
		return BK_OK;
	}

	gpio_hal_init();

#if CONFIG_GPIO_KPSTAT_SUPPORT
	gpio_keep_status_init();
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_record_wakeup_pin_id();

#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
	gpio_dynamic_wakeup_source_init();
#endif

#if CONFIG_ANA_GPIO
	ana_gpio_wakeup_init();
#endif
#endif

#if CONFIG_USR_GPIO_CFG_EN
#if CONFIG_GPIO_DUMP_MAP_DEV_DEBUG
	for (gpio_id_t id = GPIO_0; id < SOC_GPIO_NUM; id++) {
		s_gpio_default_map_before[id] = gpio_hal_get_value(id);
	}
#endif
	gpio_default_map_init();
#if CONFIG_GPIO_DUMP_MAP_DEV_DEBUG
	for (gpio_id_t id = GPIO_0; id < SOC_GPIO_NUM; id++) {
		s_gpio_default_map_after[id] = gpio_hal_get_value(id);
	}
	s_gpio_default_map_snapshot_valid = true;
#endif
#endif

#if CONFIG_PM_ONLY_CP_ENABLE
#if CONFIG_TZ && (!CONFIG_SPE)
	bk_int_isr_register(INT_SRC_GPIO_NS, gpio_isr, NULL);
#else
	bk_int_isr_register(INT_SRC_GPIO, gpio_isr, NULL);
#endif

	//interrupt to CPU enable
#if CONFIG_TZ && (!CONFIG_SPE)
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO_NS, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO, 1);
#endif
#endif

	s_gpio_is_init = true;

	return BK_OK;
}

bk_err_t bk_gpio_driver_deinit(void)
{
	if (!s_gpio_is_init)
	{
		GPIO_LOGD("%s:isn't init \r\n", __func__);
		return BK_OK;
	}

#if 0
#if CONFIG_TZ && (!CONFIG_SPE)
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO_NS, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO, 0);
#endif
#endif

	s_gpio_is_init = false;

	return BK_OK;
}

uint32_t bk_gpio_get_gpio_func_code(uint32_t gpio_id)
{
	return gpio_hal_get_func_code(gpio_id);
}

bk_err_t bk_gpio_set_gpio_func(uint32_t gpio_id, IOMX_CODE_T func_code)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	return gpio_hal_set_func_code(gpio_id, func_code);
}

void bk_gpio_set_value(gpio_id_t id, uint32_t v)
{
	gpio_hal_set_value(id, v);
}

uint32_t bk_gpio_get_value(gpio_id_t id)
{
	return gpio_hal_get_value(id);
}

bk_err_t bk_gpio_enable_output(gpio_id_t gpio_id)
{
	return gpio_hal_set_func_code(gpio_id, FUNC_CODE_OUTPUT);
}

bk_err_t bk_gpio_disable_output(gpio_id_t gpio_id)
{
	// do nothing
	return BK_OK;
}

bk_err_t bk_gpio_enable_input(gpio_id_t gpio_id)
{
	return gpio_hal_set_func_code(gpio_id, FUNC_CODE_INPUT);
}

bk_err_t bk_gpio_disable_input(gpio_id_t gpio_id)
{
	// do nothing
	return BK_OK;
}

bk_err_t bk_gpio_enable_pull(gpio_id_t gpio_id )
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_pull_enable(gpio_id, 1);
}

bk_err_t bk_gpio_disable_pull(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_pull_enable(gpio_id, 0);
}

bk_err_t bk_gpio_pull_up(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	gpio_hal_pull_enable(gpio_id, 1);
	return gpio_hal_pull_up_enable(gpio_id, 1);
}

bk_err_t bk_gpio_pull_down(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	gpio_hal_pull_enable(gpio_id, 1);
	return gpio_hal_pull_up_enable(gpio_id, 0);
}

bk_err_t bk_gpio_set_output_high(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_set_output_value(gpio_id, 1);
}

bk_err_t bk_gpio_set_output_low(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_set_output_value(gpio_id, 0);
}

bool bk_gpio_get_input(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_get_input(gpio_id);
}

//MAX capactiy:3
bool bk_gpio_set_capacity(gpio_id_t gpio_id, uint32 capacity)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	return gpio_hal_set_capacity(gpio_id, capacity);
}

bk_err_t bk_gpio_set_config(gpio_id_t gpio_id, const gpio_config_t *config)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	GPIO_RETURN_ON_INVALID_IO_MODE(config->io_mode);
	GPIO_RETURN_ON_INVALID_PULL_MODE(config->pull_mode);

	switch (config->io_mode) {
	case GPIO_OUTPUT_ENABLE:
		bk_gpio_set_gpio_func(gpio_id, FUNC_CODE_OUTPUT);
		break;

	case GPIO_INPUT_ENABLE:
		bk_gpio_set_gpio_func(gpio_id, FUNC_CODE_INPUT);
		break;

	case GPIO_IO_DISABLE:
		bk_gpio_set_gpio_func(gpio_id, FUNC_CODE_HIGH_Z);
		break;

	default:
		break;
	}

	switch (config->pull_mode) {
	case GPIO_PULL_DISABLE:
		bk_gpio_disable_pull(gpio_id);
		break;

	case GPIO_PULL_DOWN_EN:
		bk_gpio_pull_down(gpio_id);
		break;

	case GPIO_PULL_UP_EN:
		bk_gpio_pull_up(gpio_id);
		break;

	default:
		break;
	}

	return BK_OK;
}

/* Enable GPIO  interrupt.
*/
bk_err_t bk_gpio_register_isr(gpio_id_t gpio_id, gpio_isr_t isr)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	s_gpio_isr[gpio_id] = isr;

	return BK_OK;
}

//This function just enable the select GPIO can report IRQ to CPU
bk_err_t bk_gpio_enable_interrupt(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	//Before enable the interrupt,wait for the internal stability of the chip
	for (volatile int i = 0; i < 1000; i++);

	return gpio_hal_enable_interrupt(gpio_id);
}

bk_err_t bk_gpio_disable_interrupt(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	gpio_hal_disable_interrupt(gpio_id);
	return BK_OK;
}

bk_err_t bk_gpio_clear_interrupt(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	gpio_hal_clear_chan_interrupt_status(gpio_id);
	return BK_OK;
}

bk_err_t bk_gpio_set_interrupt_type(gpio_id_t gpio_id, gpio_int_type_t type)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	GPIO_RETURN_ON_INVALID_INT_TYPE_MODE(type);

	return gpio_hal_set_int_type(gpio_id, type);
}

static void gpio_isr(void)
{
	gpio_interrupt_status_t gpio_status;
	int gpio_id;

	gpio_hal_get_interrupt_status(&gpio_status);

	for (gpio_id = 0; gpio_id < SOC_GPIO_NUM; gpio_id++) {
		if (gpio_hal_is_interrupt_triggered(gpio_id, &gpio_status)) {
			//if gpio_id is not within default config, continue
#if CONFIG_USR_GPIO_CFG_EN
			const gpio_default_map_t default_map[] = GPIO_DEFAULT_DEV_CONFIG;
			int i = 0;
			for(i = 0; i < sizeof(default_map)/sizeof(gpio_default_map_t); i++) {
				if(gpio_id == default_map[i].gpio_id) {
					break;
				}
			}

			if(i == sizeof(default_map)/sizeof(gpio_default_map_t)) {
				continue;
			}
#endif
			if (s_gpio_isr[gpio_id]) {
				GPIO_LOGV("gpio int: index:%d \r\n",gpio_id);
				s_gpio_isr[gpio_id](gpio_id);
			}
			#if CONFIG_PM_ONLY_CP_ENABLE
			bk_gpio_clear_interrupt(gpio_id);
			#endif
		}
	}
}

/* ===========================================================================
 * Per-PIN status dump (v2px)
 *
 * Decode each GPIO's cfg register and print a compact human readable line so
 * that the actual function and electrical state of every pad is visible after
 * the board boots. Intended for production debug, not performance critical.
 * =========================================================================== */
static const char *gpio_v2px_pull_str(uint32_t pull_ena, uint32_t pull_mode)
{
	if (!pull_ena) {
		return "FLOAT";
	}
	return pull_mode ? "PU" : "PD";
}

static const char *gpio_v2px_int_type_str(uint32_t int_type)
{
	switch (int_type) {
	case GPIO_INT_TYPE_LOW_LEVEL:    return "LOW_LEVEL";
	case GPIO_INT_TYPE_HIGH_LEVEL:   return "HIGH_LEVEL";
	case GPIO_INT_TYPE_RISING_EDGE:  return "RISING";
	case GPIO_INT_TYPE_FALLING_EDGE: return "FALLING";
	default:                         return "UNKNOWN";
	}
}

/* Decode the function selection. fun_ena==0 means the pad is in GPIO mode
 * (controlled by input/output_ena), otherwise it is muxed to a peripheral
 * whose code is recorded in fun_sel (see IOMX_CODE_T). */
static const char *gpio_v2px_funcode_str(uint32_t code)
{
	switch (code) {
	case FUNC_CODE_HIGH_Z:       return "HIGH_Z";
	case FUNC_CODE_INPUT:        return "GPIO_IN";
	case FUNC_CODE_OUTPUT:       return "GPIO_OUT";
	case FUNC_CODE_INPUT_OUTPUT: return "GPIO_IO";
	case FUNC_CODE_UART0_RXD:    return "UART0_RX";
	case FUNC_CODE_UART0_TXD:    return "UART0_TX";
	case FUNC_CODE_UART0_CTS:    return "UART0_CTS";
	case FUNC_CODE_UART0_RTS:    return "UART0_RTS";
	case FUNC_CODE_UART1_RXD:    return "UART1_RX";
	case FUNC_CODE_UART1_TXD:    return "UART1_TX";
	case FUNC_CODE_UART2_RXD:    return "UART2_RX";
	case FUNC_CODE_UART2_TXD:    return "UART2_TX";
	case FUNC_CODE_I2C0_SCL:     return "I2C0_SCL";
	case FUNC_CODE_I2C0_SDA:     return "I2C0_SDA";
	case FUNC_CODE_I2C1_SCL:     return "I2C1_SCL";
	case FUNC_CODE_I2C1_SDA:     return "I2C1_SDA";
	case FUNC_CODE_SPI0_SCK:     return "SPI0_SCK";
	case FUNC_CODE_SPI0_NSS:     return "SPI0_CSN";
	case FUNC_CODE_SPI0_MOSI:    return "SPI0_MOSI";
	case FUNC_CODE_SPI0_MISO:    return "SPI0_MISO";
	case FUNC_CODE_SPI1_SCK:     return "SPI1_SCK";
	case FUNC_CODE_SPI1_NSS:     return "SPI1_CSN";
	case FUNC_CODE_SPI1_MOSI:    return "SPI1_MOSI";
	case FUNC_CODE_SPI1_MISO:    return "SPI1_MISO";
	case FUNC_CODE_SWCLK:        return "SWCLK";
	case FUNC_CODE_SWDIO:        return "SWDIO";
	default:                     return "ALT";
	}
}

bk_err_t bk_gpio_dump_pin_status(void)
{
	GPIO_LOGI("===== GPIO PIN STATUS (v2px, SOC_GPIO_NUM=%d) =====\r\n", SOC_GPIO_NUM);
	GPIO_LOGI("ID   CFG       FUN_EN FUN_SEL(NAME)         DIR    PULL  LVL DRV INT(TYPE)\r\n");
	GPIO_LOGI("---- --------- ------ --------------------- ------ ----- --- --- ---------------\r\n");

	for (gpio_id_t id = GPIO_0; id < SOC_GPIO_NUM; id++) {
		uint32_t cfg     = gpio_hal_get_value(id);
		uint32_t in_ena  = (cfg >> 2) & 0x1;
		uint32_t out_ena = (cfg >> 3) & 0x1;
		uint32_t pul_mod = (cfg >> 4) & 0x1;
		uint32_t pul_ena = (cfg >> 5) & 0x1;
		uint32_t fun_ena = (cfg >> 6) & 0x1;
		uint32_t cap     = (cfg >> 8) & 0x3;
		uint32_t int_typ = (cfg >> 10) & 0x3;
		uint32_t int_ena = (cfg >> 12) & 0x1;
		uint32_t fun_sel = (cfg >> 24) & 0xFF;
		uint32_t in_lvl  =  cfg        & 0x1;
		uint32_t out_lvl = (cfg >> 1)  & 0x1;

		const char *dir;
		uint32_t    lvl;
		if (fun_ena) {
			dir = "ALT";
			lvl = in_lvl;
		} else if (out_ena && in_ena) {
			dir = "IO";
			lvl = in_lvl;
		} else if (out_ena) {
			dir = "OUT";
			lvl = out_lvl;
		} else if (in_ena) {
			dir = "IN";
			lvl = in_lvl;
		} else {
			dir = "HI-Z";
			lvl = in_lvl;
		}

		GPIO_LOGI("%-4d 0x%08x %-6d %3u(%-10s) %-6s %-5s %-3u %-3u %s(%s)\r\n",
			id, cfg, fun_ena,
			fun_sel, gpio_v2px_funcode_str(fun_sel),
			dir,
			gpio_v2px_pull_str(pul_ena, pul_mod),
			lvl, cap,
			int_ena ? "EN" : "DIS",
			gpio_v2px_int_type_str(int_typ));
	}

	GPIO_LOGI("===== GPIO PIN STATUS END =====\r\n");
	return BK_OK;
}

#if CONFIG_GPIO_DUMP_MAP_DEV_DEBUG
bk_err_t bk_gpio_dump_default_map_init_effect(void)
{
	if (!s_gpio_default_map_snapshot_valid) {
		GPIO_LOGI("default_map snapshot is not valid (driver init not run?)\r\n");
		return BK_FAIL;
	}

	uint32_t changed = 0;

	GPIO_LOGI("===== gpio_default_map_init() BEFORE/AFTER diff =====\r\n");
	GPIO_LOGI("ID   BEFORE_CFG  AFTER_CFG   CHANGED\r\n");
	GPIO_LOGI("---- ----------- ----------- -------\r\n");

	for (gpio_id_t id = GPIO_0; id < SOC_GPIO_NUM; id++) {
		uint32_t b = s_gpio_default_map_before[id];
		uint32_t a = s_gpio_default_map_after[id];
		/* bit0 (gpio_input) is the live-sampled input level, which may toggle
		 * naturally between the two snapshots even when nothing was written.
		 * Mask it out before comparing so we only count software-driven diffs. */
		uint32_t b_cmp = b & ~0x1u;
		uint32_t a_cmp = a & ~0x1u;
		bool diff = (b_cmp != a_cmp);

		GPIO_LOGI("%-4d 0x%08x  0x%08x  %s\r\n",
			id, b, a, diff ? "YES" : "");
		if (diff) {
			changed++;
		}
	}

	GPIO_LOGI("===== diff end: %u/%u pads changed by default_map_init =====\r\n",
		(unsigned)changed, (unsigned)SOC_GPIO_NUM);
	return BK_OK;
}
#endif

bk_err_t gpio_backup_gpio_configs(void)
{
	return gpio_hal_bakup_configs(s_gpio_baked_regs);
}

bk_err_t gpio_restore_gpio_configs(void)
{
	return gpio_hal_restore_configs(s_gpio_baked_regs);
}

void gpio_dump_regs(bool config, bool overview, bool atpg)
{
#if 0
	gpio_id_t gpio_id;

	GPIO_LOGV("%s[+]\r\n", __func__);

	if (config)
	{
		for(gpio_id = GPIO_0; gpio_id < SOC_GPIO_NUM; gpio_id++)
		{
			///gpio_struct_dump(gpio_id);
			GPIO_LOGV("gpio[%d]=0x%x\r\n", gpio_id, *(volatile uint32_t*)(GPIO_LL_REG_BASE + 4*gpio_id));
		}
	}

#if 0
	if (overview)
	{
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_INTSTA_ADDR, REG_READ(IOMX_GPIO_INTSTA_ADDR));
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_INPUT_ADDR,  REG_READ(IOMX_GPIO_INPUT_ADDR));
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_OUTPUT_ADDR, REG_READ(IOMX_GPIO_OUTPUT_ADDR));
	}

	if (atpg)
	{
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_FUNC_O_ATPG_ADDR,   REG_READ(IOMX_GPIO_FUNC_O_ATPG_ADDR));
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_FUNC_IE_ATPG_ADDR,  REG_READ(IOMX_GPIO_FUNC_IE_ATPG_ADDR));
		GPIO_LOGV("REG0x%x=0x%x\r\n", IOMX_GPIO_FUNC_OEN_ATPG_ADDR, REG_READ(IOMX_GPIO_FUNC_OEN_ATPG_ADDR));
	}

#endif
	GPIO_LOGV("%s[-]\r\n", __func__);
#endif
}

static void gpio_low_power_config(void);
bk_err_t gpio_enter_low_power(void *param)
{
#if 1
	GPIO_LOGV("%s[+]\r\n", __func__);


	gpio_backup_gpio_configs();

	gpio_dump_regs(true, false, false);

	//NOTES:force disable all int to avoid config gpio caused error isr
	gpio_hal_disable_all_interrupts();

	// setup gpio configs for sleep
#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
	gpio_keep_status_config();
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_wakeup_source_config();
#endif

	gpio_low_power_config();


	GPIO_LOGV("%s[-]\r\n", __func__);
#endif
	return BK_OK;
}

bk_err_t gpio_exit_low_power(void *param)
{
#if 1
	GPIO_LOGV("%s[+]\r\n", __func__);

	gpio_hal_disable_all_interrupts();

#if CONFIG_GPIO_WAKEUP_SUPPORT
#if CONFIG_ANA_GPIO
	// another workaround fix for unexpected gpio interrupt
	ana_gpio_clear_wakeup_source();
#endif
#endif

	gpio_dump_regs(true, false, false);
	gpio_hal_restore_configs(s_gpio_baked_regs);

	gpio_dump_regs(true, false, false);

	GPIO_LOGV("%s[-]\r\n", __func__);
#endif
	return BK_OK;
}

bk_err_t gpio_hal_switch_to_low_power_status(uint64_t skip_io)
{
	for (gpio_id_t i = GPIO_0; i < GPIO_NUM_MAX; i++)
	{
		if (skip_io & (0x1ULL << i))
			continue;
		bk_gpio_set_value(i, GPIO_REG_DEFAULT_VALUE);
	}

	return BK_OK;
}

static void gpio_low_power_config(void)
{
	uint64_t skip_io = 0;

#if CONFIG_GPIO_WAKEUP_SUPPORT
	skip_io |= s_gpio_is_setted_wake_status;
#endif

#if CONFIG_GPIO_KPSTAT_SUPPORT
	skip_io |= s_gpio_is_lowpower_keep_status;
#endif

	gpio_hal_switch_to_low_power_status(skip_io);
}

#if CONFIG_GPIO_WAKEUP_SUPPORT
gpio_id_t bk_gpio_get_wakeup_gpio_id(void)
{
	return s_gpio_wakeup_gpio_id;
}

static void gpio_record_wakeup_pin_id(void)
{
#if CONFIG_ANA_GPIO
	s_gpio_wakeup_gpio_id = ana_gpio_get_wakeup_pin();
#endif
}

bk_err_t gpio_enable_interrupt_mult_for_wake(void)
{
#if CONFIG_ANA_GPIO
	ana_gpio_config_wakeup_source(s_gpio_is_setted_wake_status);
#endif

	return gpio_hal_enable_multi_interrupts(s_gpio_is_setted_wake_status);
}

static void gpio_set_wakeup_config(gpio_id_t gpio_id, gpio_int_type_t int_type)
{
	// setup gpio as input mode
	bk_gpio_set_value(gpio_id, GPIO_REG_DEFAULT_VALUE);
	bk_gpio_set_gpio_func(gpio_id, FUNC_CODE_INPUT);

	switch(int_type)
	{
		case GPIO_INT_TYPE_LOW_LEVEL:
		case GPIO_INT_TYPE_FALLING_EDGE:
			bk_gpio_pull_up(gpio_id);
			GPIO_LOGV("%s GPIO %d Pull_up!\r\n", __func__, gpio_id);
			break;
		case GPIO_INT_TYPE_HIGH_LEVEL:
		case GPIO_INT_TYPE_RISING_EDGE:
			bk_gpio_pull_down(gpio_id);
			GPIO_LOGV("%s GPIO %d Pull_down!\r\n", __func__, gpio_id);
			break;
		default:
			GPIO_LOGD("%s Please set fill in the mode correctly!\r\n", __func__);
			break;
	}

	bk_gpio_set_interrupt_type(gpio_id, int_type);
}

static void gpio_wakeup_source_config(void)
{
	uint32_t i = 0;

	GPIO_LOGV("%s[+]\r\n", __func__);

	s_gpio_is_setted_wake_status = 0;

	gpio_wakeup_t gpio_wakeup_map[] = GPIO_STATIC_WAKEUP_SOURCE_MAP;
	for (i = 0; i < sizeof(gpio_wakeup_map)/sizeof(gpio_wakeup_t); i++)
	{
		gpio_set_wakeup_config(gpio_wakeup_map[i].id, gpio_wakeup_map[i].int_type);
		s_gpio_is_setted_wake_status |= ((uint64_t)1 << gpio_wakeup_map[i].id);
	}

#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		if (s_gpio_dynamic_wakeup_source_map[i].id != GPIO_WAKE_SOURCE_IDLE_ID) {
			//maybe the PIN is re-used as SECOND_FUNCTION and GPIO,F.E:UART RXD re-uses as wakeup PIN
			gpio_set_wakeup_config(s_gpio_dynamic_wakeup_source_map[i].id, s_gpio_dynamic_wakeup_source_map[i].int_type);
			s_gpio_is_setted_wake_status |= ((uint64_t)1 << s_gpio_dynamic_wakeup_source_map[i].id);
		}
	}
#endif

	GPIO_LOGV("%s[-]set wake src h=0x%0x, l=0x%0x\r\n", __func__, (uint32_t)(s_gpio_is_setted_wake_status>>32), (uint32_t)s_gpio_is_setted_wake_status);
}

#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
bk_err_t bk_gpio_register_wakeup_source(gpio_id_t gpio_id, gpio_int_type_t int_type)
{
	uint32_t i = 0;

	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	GPIO_RETURN_ON_INVALID_INT_TYPE_MODE(int_type);

	//search the same id and replace it.
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		if (s_gpio_dynamic_wakeup_source_map[i].id == gpio_id)
		{
			s_gpio_dynamic_wakeup_source_map[i].int_type = int_type;

			//NOTES:If doesn't set int type, exit lowpower, if the ISR has reported by rising/falling type
			//Then restore to level type, entry GPIO ISR caused the int status lost.
			bk_gpio_set_interrupt_type(gpio_id, int_type);
			//s_gpio_dynamic_wakeup_source_map[i].isr = isr;

			GPIO_LOGV("gpio=%d,int_type=%d replace previous wake src\r\n", gpio_id, int_type);
			return BK_OK;
		}
	}

	//serach the first idle id
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		if (s_gpio_dynamic_wakeup_source_map[i].id == GPIO_WAKE_SOURCE_IDLE_ID)
		{
			s_gpio_dynamic_wakeup_source_map[i].id = gpio_id;
			s_gpio_dynamic_wakeup_source_map[i].int_type = int_type;

			//NOTES:If doesn't set int type, exit lowpower, if the ISR has reported by rising/falling type
			//Then restore to level type, entry GPIO ISR caused the int status lost.
			bk_gpio_set_interrupt_type(gpio_id, int_type);
			//s_gpio_dynamic_wakeup_source_map[i].isr = isr;
			s_gpio_is_setted_wake_status |= ((uint64_t)1 << s_gpio_dynamic_wakeup_source_map[i].id);

			GPIO_LOGV("gpio=%d,int_type=%d register wake src\r\n", gpio_id, int_type);

			return BK_OK;
		}
	}

	GPIO_LOGE("too much(%d) GPIO is setted wake src\r\n", CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT);
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		GPIO_LOGE("gpio id:%d is using \r\n", s_gpio_dynamic_wakeup_source_map[i].id);
	}
	return BK_FAIL;
}

bk_err_t bk_gpio_unregister_wakeup_source(gpio_id_t gpio_id)
{
	uint32_t i = 0;

	// GPIO_RETURN_ON_INVALID_ID(gpio_id);

	/* search the same id and replace it.*/
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		if (s_gpio_dynamic_wakeup_source_map[i].id == gpio_id)
		{
			s_gpio_is_setted_wake_status &= ~(((uint64_t)1 << s_gpio_dynamic_wakeup_source_map[i].id));
			s_gpio_dynamic_wakeup_source_map[i].id = GPIO_WAKE_SOURCE_IDLE_ID;
			s_gpio_dynamic_wakeup_source_map[i].int_type = GPIO_INT_TYPE_MAX;
			//s_gpio_dynamic_wakeup_source_map[i].isr = NULL;

			/* Clear the hardware status during deregister */
			bk_gpio_disable_input(gpio_id);
			bk_gpio_disable_interrupt(gpio_id);

			GPIO_LOGV("%s[-]gpioid=%d\r\n", __func__, gpio_id);

			return BK_OK;
		}
	}

	GPIO_LOGE("gpio id:%d is not using \r\n", gpio_id);
	return BK_FAIL;
}

static void gpio_dynamic_wakeup_source_init(void)
{
	uint32_t i = 0;

	GPIO_LOGV("%s[+]gpio wakecnt=%d\r\n", __func__, CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT);
	//search the same id and replace it.
	for (i = 0; i < CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT; i++)
	{
		s_gpio_dynamic_wakeup_source_map[i].id = GPIO_WAKE_SOURCE_IDLE_ID;
	}

	GPIO_LOGV("%s[-]\r\n", __func__);
}
#else
bk_err_t bk_gpio_register_wakeup_source(gpio_id_t gpio_id,
                                                 gpio_int_type_t int_type)
{
	return BK_OK;
}

bk_err_t bk_gpio_unregister_wakeup_source(gpio_id_t gpio_id)
{
	return BK_OK;
}
#endif
#endif

#if CONFIG_GPIO_KPSTAT_SUPPORT
static void gpio_keep_status_init(void)
{
	//has configured in default map with static mode
#if CONFIG_USR_GPIO_CFG_EN
	const gpio_default_map_t default_map[] = GPIO_DEFAULT_DEV_CONFIG;

	for (uint32_t i = 0; i < sizeof(default_map)/sizeof(gpio_default_map_t); i++)
	{
		//uses equal to avoid some guy maybe write other value
		if ((default_map[i].low_power_io_ctrl == GPIO_LOW_POWER_KEEP_INPUT_STATUS) ||
			(default_map[i].low_power_io_ctrl == GPIO_LOW_POWER_KEEP_OUTPUT_STATUS))
		{
			s_gpio_is_lowpower_keep_status |= ((uint64_t)0x1 << default_map[i].gpio_id);
		}
	}
#endif

#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
	GPIO_LOGV("%s[+]gpio wakecnt=%d\r\n", __func__, CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT);
	for (uint32_t i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		s_gpio_lowpower_keep_config[i].gpio_id = GPIO_LOWPOWER_KEEP_STATUS_IDLE_ID;
	}
#endif

	GPIO_LOGV("%s[-]\r\n", __func__);
}

#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
static void gpio_keep_status_config(void)
{
	uint32_t index;
	gpio_id_t gpio_id;
	gpio_config_t config;

	config.io_mode = GPIO_IO_DISABLE;
	config.pull_mode = GPIO_PULL_DISABLE;
	config.func_mode = GPIO_SECOND_FUNC_DISABLE;

	for(index = 0; index < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; index++)
	{
		if(s_gpio_is_lowpower_keep_status & ((uint64_t)1 << s_gpio_lowpower_keep_config[index].gpio_id)) {
			gpio_id = s_gpio_lowpower_keep_config[index].gpio_id;
			config.io_mode = s_gpio_lowpower_keep_config[index].config.io_mode;
			config.pull_mode = s_gpio_lowpower_keep_config[index].config.pull_mode;
			config.func_mode = s_gpio_lowpower_keep_config[index].config.func_mode;
			bk_gpio_set_config(gpio_id, &config);
			BK_LOGD(NULL, "set config %d %d %d %x\r\n", config.io_mode, config.pull_mode, config.func_mode, bk_gpio_get_value(gpio_id));
		}
	}
}

bk_err_t bk_gpio_register_lowpower_keep_status(gpio_id_t gpio_id,
                                                 const gpio_config_t *config)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);
	GPIO_RETURN_ON_INVALID_IO_MODE(config->io_mode);
	GPIO_RETURN_ON_INVALID_PULL_MODE(config->pull_mode);

	uint32_t i = 0;
	GPIO_LOGD("[+]gpio=%d io_mode=%d pull_mode=%d func_mode=%d\r\n",
		gpio_id, config->io_mode, config->pull_mode, config->func_mode);

#if CONFIG_GPIO_RETENTION_SUPPORT
	if (config->io_mode == GPIO_OUTPUT_ENABLE && config->pull_mode == GPIO_PULL_UP_EN)
	{
		gpio_retention_map_set(gpio_id, GPIO_OUTPUT_STATE_HIGH);
	}
	else if (config->io_mode == GPIO_OUTPUT_ENABLE && config->pull_mode == GPIO_PULL_DOWN_EN)
	{
		gpio_retention_map_set(gpio_id, GPIO_OUTPUT_STATE_LOW);
	}
#endif

	//search the same id and replace it.
	for(i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		if(s_gpio_lowpower_keep_config[i].gpio_id == gpio_id) {
			s_gpio_lowpower_keep_config[i].config.io_mode = config->io_mode;
			s_gpio_lowpower_keep_config[i].config.pull_mode = config->pull_mode;
			s_gpio_lowpower_keep_config[i].config.func_mode = config->func_mode;
			s_gpio_is_lowpower_keep_status |= ((uint64_t)1 << gpio_id);

			GPIO_LOGV("gpio=%d io_mode=%d pull_mode=%d func_mode=%d\r\n",
				gpio_id, config->io_mode, config->pull_mode, config->func_mode);
			return BK_OK;
		}
	}

	//serach the first idle id
	for(i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		if(s_gpio_lowpower_keep_config[i].gpio_id == GPIO_LOWPOWER_KEEP_STATUS_IDLE_ID) {
			s_gpio_lowpower_keep_config[i].gpio_id = gpio_id;
			s_gpio_lowpower_keep_config[i].config.io_mode = config->io_mode;
			s_gpio_lowpower_keep_config[i].config.pull_mode = config->pull_mode;
			s_gpio_lowpower_keep_config[i].config.func_mode = config->func_mode;
			s_gpio_is_lowpower_keep_status |= ((uint64_t)1 << gpio_id);

			GPIO_LOGD("gpio=%d io_mode=%d pull_mode=%d func_mode=%d\r\n",
				gpio_id, config->io_mode, config->pull_mode, config->func_mode);

			return BK_OK;
		}
	}

	GPIO_LOGE("too much(%d) GPIO is setted keep status\r\n", CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT);
	for(i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		GPIO_LOGE("gpio id:%d is using \r\n", s_gpio_lowpower_keep_config[i].gpio_id);
	}
	return BK_FAIL;
}

bk_err_t bk_gpio_unregister_lowpower_keep_status(gpio_id_t gpio_id)
{
	gpio_config_t config;
	uint32_t i = 0;

	config.io_mode = GPIO_IO_DISABLE;
	config.pull_mode = GPIO_PULL_DISABLE;
	config.func_mode = GPIO_SECOND_FUNC_DISABLE;

	/* search the same id and replace it.*/
	for(i = 0; i < CONFIG_GPIO_DYNAMIC_KEEP_STATUS_MAX_CNT; i++)
	{
		if(s_gpio_lowpower_keep_config[i].gpio_id == gpio_id) {
			s_gpio_lowpower_keep_config[i].gpio_id = GPIO_LOWPOWER_KEEP_STATUS_IDLE_ID;
			s_gpio_is_lowpower_keep_status &= ~(((uint64_t)1 << gpio_id));
			bk_gpio_set_config(gpio_id, &config);
			s_gpio_lowpower_keep_config[i].config.io_mode = config.io_mode;
			s_gpio_lowpower_keep_config[i].config.pull_mode = config.pull_mode;
			s_gpio_lowpower_keep_config[i].config.func_mode = config.func_mode;

			GPIO_LOGV("%s[-]gpioid=%d\r\n", __func__, gpio_id);

			return BK_OK;
		}
	}

	GPIO_LOGE("gpio id:%d is not using \r\n", gpio_id);
	return BK_FAIL;
}
#endif
#endif

#if CONFIG_USR_GPIO_CFG_EN
/* Apply every entry of GPIO_DEFAULT_DEV_CONFIG to the v2px GPIO IP.
 *
 * NOTE: v2px IP only exposes a single "function selector" field (gpio_fun_sel)
 * combined with a few side-bits (pull/capacity/int). FUNC_CODE_HIGH_Z/INPUT/
 * OUTPUT/INPUT_OUTPUT cover the pure-GPIO modes, anything >=4 selects a
 * peripheral. So we drive the pad direction through gpio_hal_set_func_code()
 * rather than the v1px-era bk_iomx_* writes (whose implementation is not
 * linked when CONFIG_SUPPORT_IO_MATRIX is disabled, as on bk7259). */
static void gpio_default_map_init(void)
{
	const gpio_default_map_t default_map[] = GPIO_DEFAULT_DEV_CONFIG;

	for (uint32_t i = 0; i < sizeof(default_map) / sizeof(default_map[0]); i++) {
		const gpio_default_map_t *m = &default_map[i];
		gpio_id_t id = (gpio_id_t)m->gpio_id;

		if (id >= SOC_GPIO_NUM) {
			continue;
		}

		if (m->gpio_skip == GPIO_INIT_DISABLE)
		{
			continue;
		}

		/* Detach IRQ first so a transient state during reconfig does not fire. */
		gpio_hal_disable_interrupt(id);

		/* 1. function / direction selection */
		if (m->second_func_en) {
			IOMX_CODE_T code = convert_gpio_dev_to_iomx_code((gpio_dev_t)m->second_func_dev);
			if (code != FUNC_CODE_INVALID) {
				gpio_hal_set_func_code(id, code);
			}
		} else {
			switch (m->io_mode) {
			case GPIO_IO_DISABLE:
				gpio_hal_set_func_code(id, FUNC_CODE_HIGH_Z);
				break;
			case GPIO_INPUT_ENABLE:
				gpio_hal_set_func_code(id, FUNC_CODE_INPUT);
				break;
			case GPIO_OUTPUT_ENABLE:
				gpio_hal_set_func_code(id, FUNC_CODE_OUTPUT);
				break;
			default:
				break;
			}
		}

		/* 2. initial output level (only meaningful in pure GPIO_OUTPUT mode).
		 *    Reuses the pull_mode field as the initial level hint, matching the
		 *    legacy semantics: PULL_UP_EN=>drive HIGH, PULL_DOWN_EN=>drive LOW. */
		if (!m->second_func_en && m->io_mode == GPIO_OUTPUT_ENABLE) {
			if (m->pull_mode == GPIO_PULL_UP_EN) {
				gpio_hal_set_output_value(id, 1);
			} else if (m->pull_mode == GPIO_PULL_DOWN_EN) {
				gpio_hal_set_output_value(id, 0);
			}
		}

		/* 3. pull-up / pull-down */
		switch (m->pull_mode) {
		case GPIO_PULL_DISABLE:
			gpio_hal_pull_enable(id, 0);
			break;
		case GPIO_PULL_DOWN_EN:
			gpio_hal_pull_enable(id, 1);
			gpio_hal_pull_up_enable(id, 0);
			break;
		case GPIO_PULL_UP_EN:
			gpio_hal_pull_enable(id, 1);
			gpio_hal_pull_up_enable(id, 1);
			break;
		default:
			break;
		}

		/* 4. drive capacity */
		gpio_hal_set_capacity(id, m->driver_capacity);

		/* 5. (re)enable interrupt if the map requests it */
		if (m->int_en) {
			gpio_hal_set_int_type(id, m->int_type);
			/* small settle delay before re-enabling, copied from legacy code */
			for (volatile int j = 0; j < 1000; j++) {
				;
			}
			gpio_hal_enable_interrupt(id);
		}
	}
}
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
void gpio_simulate_uart_write(unsigned char *buff, uint32_t len, gpio_id_t gpio_id, uint32_t div)
{
	volatile unsigned char c, n;
	UINT32 param;
	uint32_t div_cnt = div+1;

	BK_LOG_ON_ERR(bk_gpio_disable_input(gpio_id));
	BK_LOG_ON_ERR(bk_gpio_enable_output(gpio_id));

	bk_gpio_set_output_high(gpio_id);
	bk_delay_us(div_cnt);

	while (len--) {
		//in while loop, to avoid disable IRQ too much time, release it if finish one byte.
		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();

		//UART start bit
		bk_gpio_set_output_low(gpio_id);
		bk_delay_us(div_cnt);

		//char value
		c = *buff++;
		n = 8;
		while (n--) {
			param = c & 0x01;
			if (param) {
				bk_gpio_set_output_high(gpio_id);
			} else {
				bk_gpio_set_output_low(gpio_id);
			}

			bk_delay_us(div_cnt);
			c >>= 1;
		}

		//UART stop bit
		bk_gpio_set_output_high(gpio_id);
		bk_delay_us(div_cnt);

		GLOBAL_INT_RESTORE();
	}
}
#endif
