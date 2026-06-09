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
#include <os/os.h>
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_hal_v2px.h"
#include "gpio_driver_base.h"
#include "sys_driver.h"
#if CONFIG_ANA_GPIO
#include "ana_gpio_driver.h"
#endif
#include "sys_sw_regs.h"
#include "bk_misc.h"
#if CONFIG_MAILBOX
#include "bk_api_ipc.h"
#endif
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#if CONFIG_USR_GPIO_CFG_EN
#include "gpio_driver.h"     /* SOC-specific helpers: convert_gpio_dev_to_iomx_code, etc */
#endif
#if CONFIG_USR_GPIO_CFG_EN
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

#if CONFIG_GPIO_WAKEUP_SUPPORT
typedef struct
{
	gpio_id_t id;
	gpio_int_type_t int_type;
} gpio_wakeup_t;
#endif

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
/* s_gpio_wakeup_gpio_id is no longer maintained on AP; the ID is owned by CP
 * and propagated to AP via pm_shared_info.gpio_id (see bk_gpio_get_wakeup_gpio_id). */
#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
static gpio_dynamic_wakeup_t s_gpio_dynamic_wakeup_source_map[CONFIG_GPIO_DYNAMIC_WAKEUP_SOURCE_MAX_CNT];
#endif

/* Latched + register-on-arrival GPIO-wakeup dispatch state.
 *
 * When AP is brought back up by a GPIO wake-source, the GPIO interrupt has
 * already been serviced and cleared by CP. To make the application-layer
 * callback fire once (and only once) symmetrically with the AP-online case,
 * the v2px driver "latches" the wake-source gpio id at boot time
 * (gpio_wakeup_latch_init) and delivers it the moment a matching
 * bk_gpio_register_isr() call arrives on AP - regardless of whether the
 * registration happens in bk_init, user_main, or any later runtime point.
 *
 * s_gpio_wakeup_pending[id] : true if id is the boot-time wake-source and
 *                             its callback has not been replayed yet.
 * s_gpio_wakeup_int_type[id]: int_type recorded by bk_gpio_set_wakeup(),
 *                             also used as a "this id was registered as
 *                             a wake source on AP" guard.
 */
static bool s_gpio_wakeup_pending[SOC_GPIO_NUM];
static gpio_int_type_t s_gpio_wakeup_int_type[SOC_GPIO_NUM];
static beken_thread_t s_gpio_wakeup_dispatcher_task;
static beken_queue_t s_gpio_wakeup_dispatcher_queue;
static bool s_gpio_wakeup_dispatcher_ready;

#define GPIO_WAKEUP_DISPATCHER_QUEUE_DEPTH (8)
#define GPIO_WAKEUP_DISPATCHER_STACK_SIZE  (1024)
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
static void gpio_wakeup_latch_init(void);
static void gpio_wakeup_dispatcher_thread(void *arg);
static bk_err_t gpio_wakeup_dispatcher_start(void);
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

#if CONFIG_GPIO_WAKEUP_SUPPORT && CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT && CONFIG_MAILBOX
BK_IPC_CHANNEL_DEF(gpio_ipc);
BK_IPC_CHANNEL_REGISTER(gpio_ipc, IPC_ROUTE_CPU0_CPU1, NULL, NULL, NULL);
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

	/* Start the wake-up callback dispatcher first, then latch the boot-time
	 * wake-source so that any pending event published into pm_shared_info by
	 * CP can be replayed once an application later calls bk_gpio_register_isr. */
	gpio_wakeup_dispatcher_start();
	gpio_wakeup_latch_init();
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

	//Move ISR to last to avoid other resouce doesn't finish but isr has came.
	//F.E:GPIO wakeup deepsleep.
#if CONFIG_TZ && (!CONFIG_SPE)
	bk_int_isr_register(INT_SRC_GPIO_NS, gpio_isr, NULL);
#else
	bk_int_isr_register(INT_SRC_GPIO, gpio_isr, NULL);
#endif

	//interrupt to CPU enable
#if CONFIG_TZ && (!CONFIG_SPE)
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPIO_NS, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO_NS, 1);
#endif
#else
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPIO, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO, 1);
#endif
#endif

	s_gpio_is_init = true;

#if CONFIG_CLI && CONFIG_GPIO_TEST
	int bk_gpio_register_cli_test_feature(void);
	bk_gpio_register_cli_test_feature();
#endif

	return BK_OK;
}

bk_err_t bk_gpio_driver_deinit(void)
{
	if (!s_gpio_is_init)
	{
		GPIO_LOGD("%s:isn't init \r\n", __func__);
		return BK_OK;
	}

#if CONFIG_TZ && (!CONFIG_SPE)
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPIO_NS, 0);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_GPIO_NS, 0);
#endif
#else
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_GPIO, 0);
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

bool bk_gpio_get_output(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	return gpio_hal_get_output(gpio_id);
}

bool bk_gpio_get_input(gpio_id_t gpio_id)
{
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

#if CONFIG_GPIO_WAKEUP_SUPPORT
	/* Latched wake-up replay: if AP boot was triggered by a GPIO wake-source
	 * and the application is now (potentially much later than driver init)
	 * registering its callback, push the gpio_id into the dispatcher queue
	 * so the callback is invoked exactly once on a task context. NULL ISR
	 * is treated as an explicit unregister - do not replay in that case. */
	if (isr && s_gpio_wakeup_pending[gpio_id] && s_gpio_wakeup_dispatcher_ready) {
		gpio_id_t pending_id = gpio_id;
		s_gpio_wakeup_pending[gpio_id] = false;
		if (rtos_push_to_queue(&s_gpio_wakeup_dispatcher_queue, &pending_id,
					BEKEN_NO_WAIT) != kNoErr) {
			GPIO_LOGW("%s:dispatch wake replay failed gpio_id=%d\r\n",
				__func__, gpio_id);
		}
	}
#endif

	return BK_OK;
}

bk_err_t bk_gpio_unregister_isr(gpio_id_t gpio_id)
{
	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	s_gpio_isr[gpio_id] = NULL;

#if CONFIG_GPIO_WAKEUP_SUPPORT
	/* Drop any pending replay so a future register_isr does not deliver
	 * a stale wake-up to the wrong owner. */
	s_gpio_wakeup_pending[gpio_id] = false;
#endif

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
#if CONFIG_GPIO_WAKEUP_SUPPORT
			/* If the firing GPIO has been registered on AP as a wake source,
			 * publish its id into pm_shared_info so bk_gpio_get_wakeup_gpio_id()
			 * keeps working symmetrically when the wake-up ISR runs on AP
			 * instead of CP. CP also performs the same update from its own
			 * gpio_isr; the shared-memory window holds the most recent value. */
			if (s_gpio_is_setted_wake_status & ((uint64_t)0x1 << gpio_id)) {
				pm_shared_info_t info = {0};
				info.gpio_id = (uint8_t)gpio_id;
				bk_sys_sw_regs_update_pm_shared_info(&info,
					BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_GPIO_ID,
					BK_SYS_SW_REGS_LOCK_DISABLE);
			}
#endif
			bk_gpio_clear_interrupt(gpio_id);
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
	return pull_mode ? "PULL_UP" : "PULL_DOWN";
}

/* Interrupt state: a disabled interrupt is reported as a distinct state rather
 * than a (meaningless) trigger type. */
static const char *gpio_v2px_int_str(uint32_t int_ena, uint32_t int_type)
{
	if (!int_ena) {
		return "DISABLED";
	}
	switch (int_type) {
	case GPIO_INT_TYPE_LOW_LEVEL:    return "LOW_LEVEL";
	case GPIO_INT_TYPE_HIGH_LEVEL:   return "HIGH_LEVEL";
	case GPIO_INT_TYPE_RISING_EDGE:  return "RISING_EDGE";
	case GPIO_INT_TYPE_FALLING_EDGE: return "FALLING_EDGE";
	default:                         return "UNKNOWN";
	}
}

/* On v2px the whole pad direction/function lives in the fun_sel field:
 *   0=HIGH-Z 1=INPUT 2=OUTPUT 3=IN+OUT  >=4=peripheral(FUNC). */
static const char *gpio_v2px_dir_str(uint32_t fun_sel)
{
	switch (fun_sel) {
	case FUNC_CODE_HIGH_Z:       return "HIGH-Z";
	case FUNC_CODE_INPUT:        return "INPUT";
	case FUNC_CODE_OUTPUT:       return "OUTPUT";
	case FUNC_CODE_INPUT_OUTPUT: return "IN+OUT";
	default:                     return "FUNC";
	}
}

/* Render the meaningful pad level into a fixed width (5) centered field:
 *   INPUT/IN+OUT -> pad input bit, OUTPUT -> output bit, otherwise "-". */
static void gpio_v2px_level_str(char *buf, uint32_t len, uint32_t fun_sel,
				uint32_t in_lvl, uint32_t out_lvl)
{
	switch (fun_sel) {
	case FUNC_CODE_INPUT:
	case FUNC_CODE_INPUT_OUTPUT:
		snprintf(buf, len, "  %u  ", in_lvl);
		break;
	case FUNC_CODE_OUTPUT:
		snprintf(buf, len, "  %u  ", out_lvl);
		break;
	default:
		snprintf(buf, len, "  -  ");
		break;
	}
}

bk_err_t bk_gpio_dump_pin_status(void)
{
	GPIO_LOGI("================== GPIO STATUS DUMP ( SOC_GPIO_NUM=%d ) ==================\r\n", SOC_GPIO_NUM);
	GPIO_LOGI(" GPIO | Function         | Direction | Pull        | Drive | Level | Interrupt\r\n");
	GPIO_LOGI("------+------------------+-----------+-------------+-------+-------+-------------\r\n");

	for (gpio_id_t id = GPIO_0; id < SOC_GPIO_NUM; id++) {
		uint32_t cfg     = gpio_hal_get_value(id);
		uint32_t pul_mod = (cfg >> 4) & 0x1;
		uint32_t pul_ena = (cfg >> 5) & 0x1;
		uint32_t cap     = (cfg >> 8) & 0x3;
		uint32_t int_typ = (cfg >> 10) & 0x3;
		uint32_t int_ena = (cfg >> 12) & 0x1;
		uint32_t fun_sel = (cfg >> 24) & 0xFF;
		uint32_t in_lvl  =  cfg        & 0x1;
		uint32_t out_lvl = (cfg >> 1)  & 0x1;

		char drive_str[8];
		char level_str[8];
		snprintf(drive_str, sizeof(drive_str), "  %u  ", cap);
		gpio_v2px_level_str(level_str, sizeof(level_str), fun_sel, in_lvl, out_lvl);

		GPIO_LOGI(" %-4d | %-16s | %-9s | %-11s | %s | %s | %s\r\n",
			id,
			bk_gpio_func_name(id, fun_sel),
			gpio_v2px_dir_str(fun_sel),
			gpio_v2px_pull_str(pul_ena, pul_mod),
			drive_str,
			level_str,
			gpio_v2px_int_str(int_ena, int_typ));
	}

	GPIO_LOGI("=========================================================================\r\n");
	return BK_OK;
}

bk_err_t bk_gpio_dump_pin_detail(gpio_id_t id)
{
	if (id >= SOC_GPIO_NUM) {
		GPIO_LOGI("invalid gpio id %d (valid 0~%d)\r\n", id, SOC_GPIO_NUM - 1);
		return BK_ERR_GPIO_INVALID_ID;
	}

	uint32_t cfg     = gpio_hal_get_value(id);
	uint32_t pul_mod = (cfg >> 4) & 0x1;
	uint32_t pul_ena = (cfg >> 5) & 0x1;
	uint32_t cap     = (cfg >> 8) & 0x3;
	uint32_t int_typ = (cfg >> 10) & 0x3;
	uint32_t int_ena = (cfg >> 12) & 0x1;
	uint32_t fun_sel = (cfg >> 24) & 0xFF;
	uint32_t in_lvl  =  cfg        & 0x1;
	uint32_t out_lvl = (cfg >> 1)  & 0x1;

	char level_str[8];
	gpio_v2px_level_str(level_str, sizeof(level_str), fun_sel, in_lvl, out_lvl);

	GPIO_LOGI("================= GPIO[%d] CONFIG =================\r\n", id);
	GPIO_LOGI("  Function : %-16s (fun_sel=0x%02x)\r\n", bk_gpio_func_name(id, fun_sel), fun_sel);
	GPIO_LOGI("  Direction: %s\r\n", gpio_v2px_dir_str(fun_sel));
	GPIO_LOGI("  Pull     : %s\r\n", gpio_v2px_pull_str(pul_ena, pul_mod));
	GPIO_LOGI("  Drive    : %u  (level 0~3)\r\n", cap);
	GPIO_LOGI("  Level    : %s\r\n", level_str);
	GPIO_LOGI("  Interrupt: %s\r\n", gpio_v2px_int_str(int_ena, int_typ));
	GPIO_LOGI("  Raw CFG  : 0x%08x\r\n", cfg);
	GPIO_LOGI("==================================================\r\n");
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
#if CONFIG_GPIO_WAKEUP_DEBUG
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

	GPIO_LOGV("%s[-]\r\n", __func__);
#endif
}

static void gpio_low_power_config(void);
bk_err_t gpio_enter_low_power(void *param)
{
#if 0
	GPIO_LOGV("%s[+]\r\n", __func__);

	gpio_dump_regs(true, true, true);

	gpio_backup_gpio_configs();

	iomx_dump_baked_regs();

	//NOTES:force disable all int to avoid config gpio caused error isr
	iomx_disable_all_interrupts();

	// setup gpio configs for sleep
#if CONFIG_GPIO_DYNAMIC_KPSTAT_SUPPORT
	gpio_keep_status_config();
#endif

#if CONFIG_GPIO_WAKEUP_SUPPORT
	gpio_wakeup_source_config();
#endif

	gpio_low_power_config();

	iomx_dump_regs(true, true, true);

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

/*
 * The wake-source GPIO ID is detected on CP and propagated through shared
 * memory; AP simply reads it back so this getter is always available even when
 * CONFIG_GPIO_WAKEUP_SUPPORT is not enabled on the AP side.
 */
gpio_id_t bk_gpio_get_wakeup_gpio_id(void)
{
	pm_shared_info_t info = {0};

	if (bk_sys_sw_regs_get_pm_shared_info(&info) != BK_OK) {
		return SOC_GPIO_NUM;
	}
	return (gpio_id_t)info.gpio_id;
}

#if CONFIG_GPIO_WAKEUP_SUPPORT
static void gpio_record_wakeup_pin_id(void)
{
	/* AP does not scan wakeup GPIO; CP fills pm_shared_info.gpio_id via shared memory */
}

/* Worker that delivers latched GPIO wake-up events to the application
 * callback in a task context. Sharing s_gpio_isr[] with the AP-online ISR
 * keeps the application-visible callback path identical between the two
 * scenarios. */
static void gpio_wakeup_dispatcher_thread(void *arg)
{
	gpio_id_t id = SOC_GPIO_NUM;

	(void)arg;

	for (;;) {
		if (rtos_pop_from_queue(&s_gpio_wakeup_dispatcher_queue, &id,
					BEKEN_WAIT_FOREVER) != kNoErr) {
			continue;
		}

		if (id >= SOC_GPIO_NUM) {
			continue;
		}

		gpio_isr_t cb = s_gpio_isr[id];
		if (cb) {
			GPIO_LOGD("%s:replay wake-up callback gpio_id=%d\r\n", __func__, id);
			cb(id);
		}
	}
}

static bk_err_t gpio_wakeup_dispatcher_start(void)
{
	bk_err_t ret;

	if (s_gpio_wakeup_dispatcher_ready) {
		return BK_OK;
	}

	ret = rtos_init_queue(&s_gpio_wakeup_dispatcher_queue,
				"gpio_wk_q",
				sizeof(gpio_id_t),
				GPIO_WAKEUP_DISPATCHER_QUEUE_DEPTH);
	if (ret != BK_OK) {
		GPIO_LOGE("%s:queue init failed=%d\r\n", __func__, ret);
		return ret;
	}

	ret = rtos_create_thread(&s_gpio_wakeup_dispatcher_task,
				BEKEN_DEFAULT_WORKER_PRIORITY,
				"gpio_wk_thd",
				(beken_thread_function_t)gpio_wakeup_dispatcher_thread,
				GPIO_WAKEUP_DISPATCHER_STACK_SIZE,
				NULL);
	if (ret != BK_OK) {
		GPIO_LOGE("%s:thread create failed=%d\r\n", __func__, ret);
		rtos_deinit_queue(&s_gpio_wakeup_dispatcher_queue);
		return ret;
	}

	s_gpio_wakeup_dispatcher_ready = true;
	return BK_OK;
}

/* Mark the boot-time wake-source GPIO as "callback pending". The
 * application's bk_gpio_register_isr() will detect this and trigger a
 * one-shot delivery via the dispatcher queue. Cold boots set
 * PM_AP_WORK_STATE_FIRST_BOOT and skip latching. */
static void gpio_wakeup_latch_init(void)
{
	if (bk_pm_ap_first_boot_get()) {
		return;
	}

	gpio_id_t id = bk_gpio_get_wakeup_gpio_id();
	if (id >= SOC_GPIO_NUM) {
		return;
	}

	s_gpio_wakeup_pending[id] = true;
	GPIO_LOGD("%s:latched wake-source gpio_id=%d\r\n", __func__, id);
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
	gpio_hal_set_value(gpio_id, GPIO_REG_DEFAULT_VALUE);
	gpio_hal_set_func_code(gpio_id, FUNC_CODE_INPUT);

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
bk_err_t bk_gpio_register_wakeup_source(gpio_id_t gpio_id,
                                                 gpio_int_type_t int_type)
{
	bk_err_t ret = BK_OK;
#if CONFIG_MAILBOX
	gpio_lowerpower_t lowerpower_info = {
		.header = {
			.gpio_id = gpio_id,
			.event = GPIO_WAKEUP_UP_EVENT
		},
		.data.int_type = int_type
	};
	GPIO_LOGD("%s:register wakeup source gpio_id = %d, int_type = %d \r\n", __func__, gpio_id, int_type);
	ret = bk_ipc_send(&gpio_ipc, &lowerpower_info, sizeof(lowerpower_info), MIPC_CHAN_SEND_FLAG_SYNC, 0);
	if (ret != BK_OK) {
		GPIO_LOGW("%s:register wakeup source gpio_id = %d, int_type = %d failed \r\n", __func__, gpio_id, int_type);
	}
#endif
	return ret;
}

bk_err_t bk_gpio_unregister_wakeup_source(gpio_id_t gpio_id)
{
	bk_err_t ret = BK_OK;
#if CONFIG_MAILBOX
	gpio_lowerpower_t lowerpower_info = {
		.header = {
			.gpio_id = gpio_id,
			.event = GPIO_CANCEL_WAKEUP_EVENT
		}
	};
	ret = bk_ipc_send(&gpio_ipc, &lowerpower_info, sizeof(lowerpower_info), MIPC_CHAN_SEND_FLAG_SYNC, 0);
	if (ret != BK_OK) {
		GPIO_LOGW("%s:unregister wakeup source gpio_id = %d failed \r\n", __func__, gpio_id);
	}
#endif
	return ret;
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

/* One-call helper that wires up everything required for a GPIO to act
 * as a wake-up source under the unified API model:
 *
 *   1. Push the dynamic wake source row into CP via gpio_ipc, so CP keeps
 *      the wake table refreshed before AP enters low-voltage.
 *   2. Tell CP-PM (via MB_CHNL_PWC) to install pm_core_gpio_callback on
 *      this pin, so a GPIO edge after AP power-off can vote AP back on.
 *   3. Mirror the AP-side bookkeeping (s_gpio_is_setted_wake_status and
 *      s_gpio_wakeup_int_type[]) so that the AP-online gpio_isr also
 *      publishes the latest wake-source id into pm_shared_info.
 *
 * Pairing this call with bk_gpio_register_isr() lets the application use
 * one callback for both the AP-online interrupt path and the AP-offline
 * latched-replay path (see gpio_wakeup_latch_init / register_isr).
 */
bk_err_t bk_gpio_set_wakeup(gpio_id_t gpio_id, gpio_int_type_t int_type, bool enable)
{
	bk_err_t ret;

	GPIO_RETURN_ON_INVALID_ID(gpio_id);

	if (enable) {
		GPIO_RETURN_ON_INVALID_INT_TYPE_MODE(int_type);

		ret = bk_gpio_register_wakeup_source(gpio_id, int_type);
		if (ret != BK_OK) {
			GPIO_LOGW("%s:register wake source failed=%d gpio_id=%d\r\n",
				__func__, ret, gpio_id);
			return ret;
		}

#if CONFIG_PM_CLIENT
		/* CONFIG_PM_CLIENT is the linkable side of bk_pm_ap_gpio_wakeup_source_config
		 * on AP (defined in bk_pm/src/clients/bk_pm_client_mailbox.c). When the
		 * client side of PM is disabled the helper is unavailable, so we only
		 * push the IPC dynamic wake row and let the application know it must
		 * provide its own LV wake-up bring-up on AP. */
		pm_gpio_wakeup_config_t gpio_wakeup = {
			.gpio_id = (uint16_t)gpio_id,
			.int_type = (uint16_t)int_type,
		};
		ret = bk_pm_ap_gpio_wakeup_source_config(PM_MODE_LOW_VOLTAGE,
				PM_WAKEUP_SOURCE_INT_GPIO,
				&gpio_wakeup);
		if (ret != BK_OK) {
			GPIO_LOGW("%s:cp pm gpio wake config failed=%d gpio_id=%d\r\n",
				__func__, ret, gpio_id);
			bk_gpio_unregister_wakeup_source(gpio_id);
			return ret;
		}
#endif

		s_gpio_is_setted_wake_status |= ((uint64_t)0x1 << gpio_id);
		s_gpio_wakeup_int_type[gpio_id] = int_type;
	} else {
		ret = bk_gpio_unregister_wakeup_source(gpio_id);
		if (ret != BK_OK) {
			GPIO_LOGW("%s:unregister wake source failed=%d gpio_id=%d\r\n",
				__func__, ret, gpio_id);
		}

		s_gpio_is_setted_wake_status &= ~((uint64_t)0x1 << gpio_id);
		s_gpio_wakeup_int_type[gpio_id] = 0;
		s_gpio_wakeup_pending[gpio_id] = false;
	}

	return ret;
}
#else /* CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT */
bk_err_t bk_gpio_register_wakeup_source(gpio_id_t gpio_id,
                                                 gpio_int_type_t int_type)
{
	return BK_OK;
}

bk_err_t bk_gpio_unregister_wakeup_source(gpio_id_t gpio_id)
{
	return BK_OK;
}

bk_err_t bk_gpio_set_wakeup(gpio_id_t gpio_id, gpio_int_type_t int_type, bool enable)
{
	(void)gpio_id;
	(void)int_type;
	(void)enable;
	return BK_ERR_NOT_SUPPORT;
}
#endif
#else /* CONFIG_GPIO_WAKEUP_SUPPORT */
bk_err_t bk_gpio_set_wakeup(gpio_id_t gpio_id, gpio_int_type_t int_type, bool enable)
{
	(void)gpio_id;
	(void)int_type;
	(void)enable;
	return BK_ERR_NOT_SUPPORT;
}
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

		/* 1. function / direction selection.
		 *    For second-function pins we must go through gpio_dev_unprotect_map():
		 *    it consults both GPIO_DEV_TO_IOMX_CODE_MAP (flexible mux: UART/I2C/
		 *    SPI/PWM/...) and MAP_FUNC_CODE_FIX_GPIO (fixed mux: SDIO1/USB/JTAG/
		 *    LCD-DPI/...). Using convert_gpio_dev_to_iomx_code() alone silently
		 *    drops fixed-mux devs (e.g. SDIO1_HOST_CLK/CMD/DATA0 on P14-P16,
		 *    which only exist in the fixed map as FUNC_CODE_129), leaving the
		 *    function selector untouched. */
		if (m->second_func_en) {
			(void)gpio_dev_unprotect_map(id, (gpio_dev_t)m->second_func_dev);
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
