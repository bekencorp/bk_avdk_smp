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
#include <common/bk_compiler.h>
#include <os/mem.h>
#include "uart_driver.h"
#include "uart_hal.h"
#include <driver/uart.h>
#include "bk_uart.h"
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "bk_fifo.h"
#include <os/os.h>
#include "uart_statis.h"
#include <driver/int.h>
#include <driver/aon_rtc.h>
#include "icu_driver.h"
#include "power_driver.h"
#include "clock_driver.h"
#include <os/os.h>
#include "bk_arch.h"
#include <components/system.h>
#include <driver/sys_pm.h>

/* P0-3: bound the unsafe (coredump-context) UART byte write, aligned with the
 * CP driver. 0 disables the timeout (legacy busy-wait). */
#ifndef CONFIG_UART_UNSAFE_WRITE_TIMEOUT_MS
#define CONFIG_UART_UNSAFE_WRITE_TIMEOUT_MS 2000U
#endif

#include "sys_driver.h"
#include <modules/pm.h>
#if (CONFIG_UART_RX_DMA || CONFIG_UART_TX_DMA)
#include <driver/dma.h>
#endif
#if CONFIG_SPE
#include "security.h"
#endif
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t uart_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP

static void uart_isr_common(uart_id_t id) __BK_SECTION(".itcm");
static uint32_t uart_id_read_fifo_frame(uart_id_t id, const kfifo_ptr_t rx_ptr) __BK_SECTION(".itcm");

typedef struct {
	uart_hal_t hal;
	uint8_t id_init_bits;
	uint8_t id_sw_fifo_enable_bits;
#if CONFIG_UART_PM_CB_SUPPORT	//this macro config set to n
	uint32_t pm_backup[UART_PM_BACKUP_REG_NUM];
	uint8_t pm_bakeup_is_valid;
#endif
/*
 * !!! WARNING !!! !!! WARNING !!!
 * If APP wants to set rx_dma_en, please enable macro of CONFIG_UART_RX_DMA.
 *
 * Struct forces enable this elem(not controlled by CONFIG_UART_RX_DMA).
 * When build libs(the lib function calls this struct) it does not enable the macro of CONFIG_UART_RX_DMA,
 * but build UART Driver codes, it enables the macro of CONFIG_UART_RX_DMA.
 * Then the lib function call UART DRIVER hasn't set param of rx_dma_en, but UART driver API
 * init function will check this item and maybe set RX dma enable.
 * If defined these elems, the value will be 0 if APP not set it.
 */
#if 1
	bool rx_dma_enable;
	dma_id_t rx_dma_id;
	bool tx_dma_enable;
	dma_id_t tx_dma_id;
	bool rx_dma_stopped;              // Whether DMA is stopped
	bool rx_hw_stopped;              // Whether UART RX hardware is stopped
	uint32_t last_dma_len;           // Last DMA configured transfer length (critical!)
	uint32_t rx_dma_accounted_len;   // Bytes already accounted into kfifo in current DMA transfer
		/*
	 * Optional customer-specific RX DMA behavior.
	 * Keep it 0 by default. Set it to 1 only when the application drains the
	 * RX software FIFO in one read, and the peer will not send data while the
	 * application is reading and rewinding the RX DMA destination.
	 * Enabling it without meeting the above conditions may cause RX data abnormal.
	 */
	uint8_t rx_dma_rewind_when_fifo_empty;
#endif
#if 1
	bool sw_flow_ctrl_en;
	bool rx_int_enable;
	uint8_t rts_gpio;
	uint8_t cts_gpio;
	uint32_t sw_fifo_size;
	uint32_t sw_high_watermark;
	uint32_t sw_low_watermark;
	uint32_t hw_high_watermark;
	uint32_t hw_low_watermark;
#endif
} uart_driver_t;

typedef struct {
	bool rx_blocked;
	beken_semaphore_t rx_int_sema;
} uart_sema_t;

typedef struct
{
	uart_isr_t callback;
	void *param;
} uart_callback_t;

#define CONFIG_UART_MIN_BAUD_RATE (UART_CLOCK / (0xffff + 1))
#define CONFIG_UART_MAX_BAUD_RATE (UART_CLOCK / (4 + 1))
// RX_DMA Resume threshold: At least 25% free space
#define RESUME_THRESHOLD (s_uart_rx_kfifo[id]->size / 4)
#ifndef CONFIG_PRINTF_BUF_SIZE
#define CONFIG_PRINTF_BUF_SIZE    (128)
#endif

static uart_driver_t s_uart[SOC_UART_ID_NUM_PER_UNIT] = {
	{
		.hal.hw = (uart_hw_t *)SOC_UART0_REG_BASE,
		.hal.id = 0,
	},
	{
		.hal.hw = (uart_hw_t *)SOC_UART1_REG_BASE,
		.hal.id = 1,
	},
#if (SOC_UART_ID_NUM_PER_UNIT  >= 3)
	{
		.hal.hw = (uart_hw_t *)SOC_UART2_REG_BASE,
		.hal.id = 2,
	},
#endif
#if (SOC_UART_ID_NUM_PER_UNIT  >= 4)
	{
		.hal.hw = (uart_hw_t *)SOC_UART3_REG_BASE,
		.hal.id = 3,
	},
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 5)
	{
		.hal.hw = (uart_hw_t *)SOC_UART4_REG_BASE,
		.hal.id = 4,
	},
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 6)
	{
		.hal.hw = (uart_hw_t *)SOC_UART5_REG_BASE,
		.hal.id = 5,
	},
#endif
};

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
#define UART_FAST_PM_FIRST_ID        UART_ID_1
#define UART_FAST_PM_LAST_ID         UART_ID_5
#define UART_FAST_PM_BACKUP_REG_NUM  (6U)
#define UART_FAST_PM_QUIESCE_MS      (20U)

typedef struct {
	uint32_t active_mask;
	uint32_t backup_valid_mask;
	uint32_t backup[SOC_UART_ID_NUM_PER_UNIT][UART_FAST_PM_BACKUP_REG_NUM];
	volatile uint32_t tx_busy_count;
	volatile bool tx_suspended;
	bool dma_paused;
	bool registered;
} uart_fast_pm_context_t;

static uart_fast_pm_context_t s_uart_fast_pm;
static bk_err_t uart_fast_quiesce(void *arg);
static bk_err_t uart_fast_backup(void *arg);
static bk_err_t uart_fast_restore(void *arg);
static bk_err_t uart_fast_resume(void *arg);
#if CONFIG_UART_RX_DMA
static bk_err_t uart_rx_dma_restore(uart_id_t id);
#endif
#if CONFIG_UART_TX_DMA
static bk_err_t uart_tx_dma_restore(uart_id_t id);
#endif

static const pm_ap_fast_pm_ops_t s_uart_fast_ops = {
	.name = "uart",
	.quiesce = uart_fast_quiesce,
	.backup = uart_fast_backup,
	.restore = uart_fast_restore,
	.resume = uart_fast_resume,
	.arg = &s_uart_fast_pm,
	.priority = PM_AP_FAST_PRIORITY_PERIPHERAL,
};

static inline bool uart_fast_managed_id(uart_id_t id)
{
	return (id >= UART_FAST_PM_FIRST_ID) && (id <= UART_FAST_PM_LAST_ID);
}

static bk_err_t uart_fast_tx_enter(uart_id_t id)
{
	if (!uart_fast_managed_id(id)) {
		return BK_OK;
	}
	if (__atomic_load_n(&s_uart_fast_pm.tx_suspended, __ATOMIC_ACQUIRE)) {
		return BK_ERR_BUSY;
	}

	__atomic_add_fetch(&s_uart_fast_pm.tx_busy_count, 1U,
		__ATOMIC_ACQ_REL);
	if (__atomic_load_n(&s_uart_fast_pm.tx_suspended, __ATOMIC_ACQUIRE)) {
		__atomic_sub_fetch(&s_uart_fast_pm.tx_busy_count, 1U,
			__ATOMIC_RELEASE);
		return BK_ERR_BUSY;
	}
	return BK_OK;
}

static void uart_fast_tx_exit(uart_id_t id)
{
	if (uart_fast_managed_id(id)) {
		__atomic_sub_fetch(&s_uart_fast_pm.tx_busy_count, 1U,
			__ATOMIC_RELEASE);
	}
}

#define UART_FAST_TX_RETURN_ON_SUSPEND(id) do {\
		bk_err_t fast_pm_ret = uart_fast_tx_enter(id);\
		if (fast_pm_ret != BK_OK) {\
			return fast_pm_ret;\
		}\
	} while (0)
#define UART_FAST_TX_EXIT(id) uart_fast_tx_exit(id)
#else
static inline bk_err_t uart_fast_tx_enter(uart_id_t id)
{
	(void)id;
	return BK_OK;
}

#define UART_FAST_TX_RETURN_ON_SUSPEND(id)
#define UART_FAST_TX_EXIT(id)
#endif

static bool s_uart_driver_is_init = false;
static uart_callback_t s_uart_rx_isr[SOC_UART_ID_NUM_PER_UNIT] = {NULL};
static uart_callback_t s_uart_tx_isr[SOC_UART_ID_NUM_PER_UNIT] = {NULL};
static kfifo_ptr_t s_uart_rx_kfifo[SOC_UART_ID_NUM_PER_UNIT] = {NULL};
static uart_sema_t s_uart_sema[SOC_UART_ID_NUM_PER_UNIT] = {0};

#define UART_RETURN_ON_NOT_INIT() do {\
		if (!s_uart_driver_is_init) {\
			return BK_ERR_UART_NOT_INIT;\
		}\
	} while(0)

#define UART_RETURN_ON_INVALID_ID(id) do {\
		if ((id) >= SOC_UART_ID_NUM_PER_UNIT) {\
			return BK_ERR_UART_INVALID_ID;\
		}\
	} while(0)

#define UART_RETURN_ON_ID_NOT_INIT(id) do {\
		if (!(s_uart[id].id_init_bits & BIT((id)))) {\
			return BK_ERR_UART_ID_NOT_INIT;\
		}\
	} while(0)

#define UART_RETURN_ON_BAUD_RATE_NOT_SUPPORT(baud_rate) do {\
		if ((baud_rate) < CONFIG_UART_MIN_BAUD_RATE ||\
			(baud_rate) > CONFIG_UART_MAX_BAUD_RATE) {\
			return BK_ERR_UART_BAUD_RATE_NOT_SUPPORT;\
		}\
	} while(0)

#if CONFIG_UART_PM_CB_SUPPORT	//this macro config set to n
#define UART_PM_CHECK_RESTORE(id) do {\
	GLOBAL_INT_DECLARATION();\
	GLOBAL_INT_DISABLE();\
	switch (id) {\
	case UART_ID_0:\
		break;\
	case UART_ID_1:\
		if (bk_pm_module_lv_sleep_state_get(PM_DEV_ID_UART2)) {\
			bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_UART1, PM_POWER_MODULE_STATE_ON);\
			uart_pm_restore(0, (void *)id);\
			bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_UART2);\
		}\
		break;\
	case UART_ID_2:\
		if (bk_pm_module_lv_sleep_state_get(PM_DEV_ID_UART3)) {\
			bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_UART2, PM_POWER_MODULE_STATE_ON);\
			uart_pm_restore(0, (void *)id);\
			bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_UART3);\
		}\
		break;\
	default:\
		break;\
	}\
	GLOBAL_INT_RESTORE();\
} while(0)
#else
#define UART_PM_CHECK_RESTORE(id)
#endif

#if CONFIG_SPE
#define UART_CHECK_SECURE(id) do {\
	switch (id) {\
	case UART_ID_0:\
		BK_ASSERT(DEV_IS_SECURE(UART0) == 1);\
		break;\
	case UART_ID_1:\
		BK_ASSERT(DEV_IS_SECURE(UART1) == 1);\
		break;\
	case UART_ID_2:\
		BK_ASSERT(DEV_IS_SECURE(UART2) == 1);\
		break;\
	default:\
		break;\
	}\
} while(0)
#else
#define UART_CHECK_SECURE(id)
#endif

#if (CONFIG_DEBUG_VERSION)
#define DEAD_WHILE() do{\
		while(1);\
	} while(0)
#else
#define DEAD_WHILE() do{\
		BK_LOGD(NULL, "dead\r\n");\
	} while(0)
#endif

static inline uint32_t uart_enter_critical()
{
       uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
       spin_lock(&uart_spin_lock);
#endif // CONFIG_FREERTOS_SMP

       return flags;
}

static inline void uart_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
       spin_unlock(&uart_spin_lock);
#endif // CONFIG_FREERTOS_SMP

       rtos_enable_int(flags);
}


void uart_clock_enable(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_0:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART0, CLK_PWR_CTRL_PWR_UP);
			break;
		case UART_ID_1:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART1, CLK_PWR_CTRL_PWR_UP);
			break;
		case UART_ID_2:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART2, CLK_PWR_CTRL_PWR_UP);
			break;
#if (SOC_UART_ID_NUM_PER_UNIT >= 4)
		case UART_ID_3:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART3, CLK_PWR_CTRL_PWR_UP);
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 5)
		case UART_ID_4:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART4, CLK_PWR_CTRL_PWR_UP);
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 6)
		case UART_ID_5:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART5, CLK_PWR_CTRL_PWR_UP);
			break;
#endif
		default:
			break;
	}
}

void uart_clock_disable(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_0:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART0, CLK_PWR_CTRL_PWR_DOWN);
			break;
		case UART_ID_1:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART1, CLK_PWR_CTRL_PWR_DOWN);
			break;
		case UART_ID_2:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART2, CLK_PWR_CTRL_PWR_DOWN);
			break;
#if (SOC_UART_ID_NUM_PER_UNIT  >= 4)
		case UART_ID_3:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART3, CLK_PWR_CTRL_PWR_DOWN);
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 5)
		case UART_ID_4:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART4, CLK_PWR_CTRL_PWR_DOWN);
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 6)
		case UART_ID_5:
			bk_pm_clock_ctrl(CLK_PWR_ID_UART5, CLK_PWR_CTRL_PWR_DOWN);
			break;
#endif
		default:
			break;
	}
}

static void uart_interrupt_enable(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_0:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART0, 1);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART0, 1);
#endif
			break;
		case UART_ID_1:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART1, 1);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART1, 1);
#endif
			break;
		case UART_ID_2:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART2, 1);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART2, 1);
#endif
			break;
#if (SOC_UART_ID_NUM_PER_UNIT >= 4)
		case UART_ID_3:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART3, 1);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART3, 1);
#endif
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 5)
		case UART_ID_4:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART4, 1);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART4, 1);
#endif
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 6)
		case UART_ID_5:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART5, 1);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART5, 1);
#endif
			break;
#endif
		default:
			break;
	}
}

static void uart_interrupt_disable(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_0:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART0, 0);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART0, 0);
#endif
			break;
		case UART_ID_1:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART1, 0);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART1, 0);
#endif
			break;
		case UART_ID_2:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART2, 0);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART2, 0);
#endif
			break;
#if (SOC_UART_ID_NUM_PER_UNIT >= 4)
		case UART_ID_3:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART3, 0);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART3, 0);
#endif
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 5)
		case UART_ID_4:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART4, 0);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART4, 0);
#endif
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 6)
		case UART_ID_5:
#if CONFIG_SOC_SMP
			sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_UART5, 0);
#else
			sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_UART5, 0);
#endif
			break;
#endif
		default:
			break;
	}
}

/* Resolve the UART TXD/RXD device function for a UART id, or GPIO_DEV_NONE
 * when the UART has no configurable pad function. */
static gpio_dev_t uart_txd_dev(uart_id_t id)
{
	switch (id) {
	case UART_ID_0: return GPIO_DEV_UART0_TXD;
	case UART_ID_1: return GPIO_DEV_UART1_TXD;
	case UART_ID_2: return GPIO_DEV_UART2_TXD;
#if (SOC_UART_ID_NUM_PER_UNIT >= 6)
	case UART_ID_5: return GPIO_DEV_UART5_TXD;
#endif
	default: return GPIO_DEV_NONE;
	}
}

static gpio_dev_t uart_rxd_dev(uart_id_t id)
{
	switch (id) {
	case UART_ID_0: return GPIO_DEV_UART0_RXD;
	case UART_ID_1: return GPIO_DEV_UART1_RXD;
	case UART_ID_2: return GPIO_DEV_UART2_RXD;
#if (SOC_UART_ID_NUM_PER_UNIT >= 6)
	case UART_ID_5: return GPIO_DEV_UART5_RXD;
#endif
	default: return GPIO_DEV_NONE;
	}
}

/* Map/unmap a UART pad by its function. Only acts when the function is present
 * in the board's GPIO_DEFAULT_DEV_CONFIG (usr_gpio_cfg.h); otherwise silently
 * skips so UARTs not owned by this core's table are left untouched. The
 * function-selector write itself is idempotent inside gpio_dev_map_by_func(). */
static void uart_map_func(gpio_dev_t func)
{
	if (func != GPIO_DEV_NONE && gpio_get_id_by_func(func) < SOC_GPIO_NUM) {
		gpio_dev_map_by_func(func);
	}
}

static void uart_unmap_func(gpio_dev_t func)
{
	if (func != GPIO_DEV_NONE && gpio_get_id_by_func(func) < SOC_GPIO_NUM) {
		gpio_dev_unmap_by_func(func);
	}
}

/* Resolve a UART RX pin. The pin assignment lives in GPIO_DEFAULT_DEV_CONFIG
 * (usr_gpio_cfg.h) and is the single source of truth, so the pin is looked up
 * by its UART RXD function. Returns SOC_GPIO_NUM when the UART is not present
 * in the board's GPIO config table. */
static gpio_id_t uart_cfg_rx_pin(uart_id_t id)
{
	gpio_dev_t func = uart_rxd_dev(id);

	return (func != GPIO_DEV_NONE) ? gpio_get_id_by_func(func) : SOC_GPIO_NUM;
}

static void uart_init_gpio(uart_id_t id)
{
	/* UART pin assignment lives in GPIO_DEFAULT_DEV_CONFIG (usr_gpio_cfg.h).
	 * Re-apply the TX/RX mapping here so a UART re-init after a
	 * disable_tx/disable_rx (which drives the pad to high-Z) is restored. */
	uart_map_func(uart_txd_dev(id));
	uart_map_func(uart_rxd_dev(id));

#if CONFIG_UART0_FLOW_CTRL
	if (id == UART_ID_0) {
		bk_uart_set_hw_flow_ctrl(id, UART0_FLOW_CTRL_CNT);
	}
#endif

	if (uart_cfg_rx_pin(id) >= GPIO_64 && uart_cfg_rx_pin(id) <= GPIO_71)
	{
		/* Enable the 3 V auxiliary LDO before using GPIO64~71. The system
		 * driver resolves the correct Secure/Non-secure register alias. */
		(void)sys_drv_auxldo_enable(AUXLDOS_SEL_3V, 1);
	}
}

static void uart_deinit_tx_gpio(uart_id_t id)
{
	/* Drive the TX pad to a true high-impedance, low-power state. */
	uart_unmap_func(uart_txd_dev(id));
}

static void uart_deinit_rx_gpio(uart_id_t id)
{
	/* Drive the RX pad to a true high-impedance, low-power state. */
	uart_unmap_func(uart_rxd_dev(id));
}

static bk_err_t uart_id_init_kfifo(uart_id_t id)
{
	uint32_t fifo_size = CONFIG_KFIFO_SIZE;
//RX DMA needs bigger FIFO size when erase flash.
#if (CONFIG_UART_RX_DMA)
	fifo_size = CONFIG_UART_RX_DMA_KFIFO_SIZE;
#endif

	if (!s_uart_rx_kfifo[id]) {
		s_uart_rx_kfifo[id] = kfifo_alloc(fifo_size);
		if (!s_uart_rx_kfifo[id]) {
			UART_LOGE("uart(%d) rx kfifo alloc failed\n", id);
			return BK_ERR_NULL_PARAM;
		}
#if CONFIG_UART_SW_FLOW_CTRL
		s_uart[id].sw_fifo_size = fifo_size;
#endif
	}
	return BK_OK;
}

static void uart_id_deinit_kfifo(uart_id_t id)
{
	if (s_uart_rx_kfifo[id]) {
		kfifo_free(s_uart_rx_kfifo[id]);
	}
	s_uart_rx_kfifo[id] = NULL;
}

/* 1. power up uart
 * 2. set clock
 * 3. set gpio as uart
 */
static bk_err_t uart_id_init_common(uart_id_t id)
{
	bk_err_t ret = 0;

	uart_clock_enable(id);
	sys_drv_uart_select_clock(id, UART_SCLK_XTAL_26M);

	uart_init_gpio(id);
	ret = uart_id_init_kfifo(id);
	uart_statis_id_init(id);

	s_uart_sema[id].rx_blocked = false;
	if (s_uart_sema[id].rx_int_sema == NULL) {
		ret = rtos_init_semaphore(&(s_uart_sema[id].rx_int_sema), 1);
		BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
	}
	s_uart[id].id_init_bits |= BIT(id);
	s_uart[id].id_sw_fifo_enable_bits |= BIT(id);

#if CONFIG_UART_RX_DMA
	//default not enable rx dma
	s_uart[id].rx_dma_enable = 0;
	s_uart[id].rx_dma_rewind_when_fifo_empty = 0;
#endif
#if CONFIG_UART_TX_DMA
	//default not enable tx dma
	s_uart[id].tx_dma_enable = 0;
#endif

	return ret;
}

static void uart_id_deinit_common(uart_id_t id)
{
	s_uart[id].id_init_bits &= ~BIT(id);
#if CONFIG_UART_RX_DMA
	s_uart[id].rx_dma_enable = 0;
	s_uart[id].rx_dma_rewind_when_fifo_empty = 0;
#endif
#if CONFIG_UART_TX_DMA
	s_uart[id].tx_dma_enable = 0;
#endif

	uart_hal_stop_common(&s_uart[id].hal, id);
	uart_hal_reset_config_to_default(&s_uart[id].hal, id);
	uart_interrupt_disable(id);
	uart_clock_disable(id);

	uart_id_deinit_kfifo(id);
	if(s_uart_sema[id].rx_int_sema)
	{
		rtos_deinit_semaphore(&(s_uart_sema[id].rx_int_sema));
		s_uart_sema[id].rx_int_sema = NULL;
	}
}

static inline bool uart_id_is_sw_fifo_enabled(uart_id_t id)
{
	return !!(s_uart[id].id_sw_fifo_enable_bits & BIT(id));
}

#if CONFIG_UART_SW_FLOW_CTRL

static inline void usfc_gpio_init(uart_id_t id)
{
	bk_gpio_set_value(s_uart[id].rts_gpio, 0x0); //output low level
	bk_gpio_set_value(s_uart[id].cts_gpio, 0x3c);//input pull up

	UART_LOGD("%s \r\n", __func__);
}

static inline void usfc_gpio_deinit(uart_id_t id)
{
	bk_gpio_set_value(s_uart[id].rts_gpio, 0x8); //high resistance
	bk_gpio_set_value(s_uart[id].cts_gpio, 0x8);//high resistance
	s_uart[id].rts_gpio = GPIO_NUM_MAX;
	s_uart[id].cts_gpio = GPIO_NUM_MAX;

}

static uint8_t hw_rts,sw_rts;
void bk_usfc_hw_set_rts(uart_id_t id,uint32_t high)
{
	if (s_uart[id].sw_flow_ctrl_en && s_uart[id].rts_gpio != GPIO_NUM_MAX)
	{
		uint32_t int_level = uart_enter_critical();
		uint32_t pre = hw_rts || sw_rts;
		uint32_t cur = high || sw_rts;
		uart_exit_critical(int_level);
		if (cur != pre)
		{
			bk_gpio_set_output_value(s_uart[id].rts_gpio, cur);
			UART_LOGV("high=%d,hw_rts=%d,sw=%d,line=%d\r\n", high, hw_rts, sw_rts, __LINE__);
			UART_LOGV("pre=%d,cur=%d\r\n", pre, cur);
		}

		hw_rts = high;
	}
	
}

void bk_usfc_sw_set_rts(uart_id_t id,uint32_t high)
{
	if (s_uart[id].sw_flow_ctrl_en && s_uart[id].rts_gpio != GPIO_NUM_MAX)
	{
		uint32_t int_level = uart_enter_critical();
		uint32_t pre = hw_rts || sw_rts;
		uint32_t cur = high || hw_rts;

		uart_exit_critical(int_level);
		if (cur != pre)
		{
			bk_gpio_set_output_value(s_uart[id].rts_gpio, cur);
			UART_LOGV("high=%d,hw_rts=%d,sw=%d,line=%d\r\n", high, hw_rts, sw_rts, __LINE__);
			UART_LOGV("pre=%d,cur=%d\r\n", pre, cur);
		}

		sw_rts = high;
	}	
}

static uint32_t usfc_tx_is_enable(uart_id_t id)
{
	if (s_uart[id].sw_flow_ctrl_en && s_uart[id].cts_gpio != GPIO_NUM_MAX)
	{
		return (!bk_gpio_get_input(s_uart[id].cts_gpio));
	}
	return true;
}

static void usfc_sw_fifo_will_full(uart_id_t id)
{
	if (!s_uart[id].sw_flow_ctrl_en)
	{
		return;
	}

	if(kfifo_data_size(s_uart_rx_kfifo[id]) > s_uart[id].sw_high_watermark)
	{
		bk_usfc_sw_set_rts(id,1);
	}
}

static void usfc_sw_fifo_will_empty(uart_id_t id)
{
	if (!s_uart[id].sw_flow_ctrl_en)
	{
		return;
	}

	if(kfifo_data_size(s_uart_rx_kfifo[id]) < s_uart[id].sw_low_watermark)
	{
		bk_usfc_sw_set_rts(id, 0);
	}
}

static void usfc_hw_fifo_will_full(uart_id_t id)
{
	if (!s_uart[id].sw_flow_ctrl_en)
	{
		return;
	}
	
	if(uart_hal_get_rx_fifo_cnt(&s_uart[id].hal, id) > s_uart[id].hw_high_watermark)
	{
		bk_usfc_hw_set_rts(id, 1);
	}
}

static void usfc_hw_fifo_will_empty(uart_id_t id)
{
	if (!s_uart[id].sw_flow_ctrl_en)
	{
		return;
	}
	
	if(uart_hal_get_rx_fifo_cnt(&s_uart[id].hal, id) < s_uart[id].hw_low_watermark)
	{
		bk_usfc_hw_set_rts(id, 0);
	}
}

static void uart_adjust_sw_flow_control(uart_id_t id, uint32_t baud_rate)
{
	uint32_t bits_per_byte = 10;
	uint32_t byte_time_us = (bits_per_byte * 1000000) / baud_rate;
	uint32_t max_delay_us = 300;
	uint32_t max_burst_bytes = max_delay_us/byte_time_us;
	s_uart[id].sw_high_watermark = s_uart[id].sw_fifo_size * 70 / 100 ;
	s_uart[id].sw_low_watermark = s_uart[id].sw_fifo_size * 20 / 100;
	s_uart[id].hw_high_watermark = UART_HW_FIFO_SIZE - max_burst_bytes;
	s_uart[id].hw_low_watermark = USFC_RX_UART_EMPTY_THROHOLD;
}
#endif

#if CONFIG_UART_RX_DMA
static uint32_t uart_id_dma_read_fifo_frame(uart_id_t id, const kfifo_ptr_t rx_ptr)
{
#if CONFIG_UART_SW_FLOW_CTRL
	usfc_sw_fifo_will_full(id);
	usfc_hw_fifo_will_empty(id);
#endif
	//DMA stop
	bk_dma_stop(s_uart[id].rx_dma_id);

	//actual_trans length
	uint16_t actual_trans_len = 0;

	//update WRITE-Pointer by DMA write length
	uint16_t dma_remain_length = bk_dma_get_remain_len(s_uart[id].rx_dma_id);

	//check kfifo used data length
	int before_kfifo_unused_size = kfifo_unused(s_uart_rx_kfifo[id]);

	UART_LOGV("uart_id_dma_read_fifo_frame dma_remain_length[%d], before_kfifo_unused_size:%d\n",
	 dma_remain_length, before_kfifo_unused_size);
	if(before_kfifo_unused_size > 0) {
		actual_trans_len = (before_kfifo_unused_size - dma_remain_length);
	}

	UART_LOGV("uart_id_dma_read_fifo_frame id[%d], actual_trans_len:%d\n", id, actual_trans_len);
	//buffer over-wrap:
	//i.e:when erase flash,CPU can't get instruction from flash then can't handle this function.
	//after Flash erase complete, UART handler come but DMA has copy more then s_uart_rx_kfifo[id]->size bytes data
	if(actual_trans_len > s_uart_rx_kfifo[id]->size)
	{
		//TODO:
		BK_ASSERT(0);
	}
	else
	{
		__attribute__((__unused__)) uart_statis_t *uart_statis = uart_statis_get_statis(id);

		//data has been saved in buffer by DMA, only update write pointer
		rx_ptr->in += actual_trans_len;
		rx_ptr->in = rx_ptr->in & rx_ptr->mask;

		UART_STATIS_SET(uart_statis->kfifo_status.in, rx_ptr->in);
	}

	int after_kfifo_unused_size = kfifo_unused(s_uart_rx_kfifo[id]);
	if(after_kfifo_unused_size > 0) {
		uint32_t dma_start_addr = (uint32_t)s_uart_rx_kfifo[id]->buffer + rx_ptr->in;
		bk_dma_set_dest_start_addr(s_uart[id].rx_dma_id, dma_start_addr);
		BK_LOG_ON_ERR(bk_dma_set_transfer_len(s_uart[id].rx_dma_id, (uint32_t)after_kfifo_unused_size));
		bk_dma_start(s_uart[id].rx_dma_id);
	} else {
		UART_LOGW("Software FIFO is full, please read the data\r\n");
		bk_uart_set_enable_rx(id, 0);
	}

	return actual_trans_len;
}
#endif

static uint32_t uart_id_read_fifo_frame(uart_id_t id, const kfifo_ptr_t rx_ptr)
{
	uint8_t read_val = 0;
	uint32_t rx_count = sizeof(read_val);
	uint32_t unused = kfifo_unused(rx_ptr);
	uint32_t kfifo_put_cnt = 0;
	__attribute__((__unused__)) uart_statis_t *uart_statis = uart_statis_get_statis(id);
#if CONFIG_UART_SW_FLOW_CTRL
	usfc_sw_fifo_will_full(id);
#endif

	//TODO: optimize flow ctrl
	while (uart_hal_is_fifo_read_ready(&s_uart[id].hal, id)) {
		if (rx_count > unused) {
#if !CONFIG_UART_SW_FLOW_CTRL
			read_val = uart_hal_read_byte(&s_uart[id].hal, id);
#endif
			if (!uart_hal_is_flow_control_enabled(&s_uart[id].hal, id)) {
				UART_LOGW("rx kfifo is full, out/in:%d/%d, unused:%d\n", rx_ptr->out, rx_ptr->in, unused);
				UART_STATIS_INC(uart_statis->kfifo_status.full_cnt);
#if CONFIG_UART_SW_FLOW_CTRL
				usfc_sw_fifo_will_full(id);
				bk_uart_disable_rx_interrupt(id);
				s_uart[id].rx_int_enable = false;
#endif
#if CFG_CLI_DEBUG
				extern void cli_show_running_command(void);
				cli_show_running_command();
#endif
				break;
			}
		}
		read_val = uart_hal_read_byte(&s_uart[id].hal, id);
		kfifo_put_cnt += kfifo_put(rx_ptr, &read_val, sizeof(read_val));
		UART_STATIS_INC(uart_statis->kfifo_status.put_cnt);
		UART_STATIS_SET(uart_statis->kfifo_status.last_value, read_val);
		unused = kfifo_unused(rx_ptr);
#if CONFIG_UART_SW_FLOW_CTRL
		uint32_t high_water_unused = s_uart[id].sw_fifo_size - s_uart[id].sw_high_watermark;
		if (unused <= high_water_unused)
		{
			usfc_sw_fifo_will_full(id);
		}
#endif
	}
	//读之后判断软件FIFO
#if CONFIG_UART_SW_FLOW_CTRL
	usfc_sw_fifo_will_full(id);
	usfc_hw_fifo_will_empty(id);
#endif
	UART_STATIS_SET(uart_statis->kfifo_status.in, rx_ptr->in);
	UART_STATIS_SET(uart_statis->kfifo_status.out, rx_ptr->out);

	return kfifo_put_cnt;
}

void print_hex_dump(const char *prefix, const void *buf, int len)
{
	int i;
	const u8 *b = buf;

	if (prefix)
		BK_LOG_RAW("%s", prefix);
	for (i = 0; i < len; i++)
		BK_LOG_RAW("%02X ", b[i]);
	BK_LOG_RAW("\n");
}

static bk_err_t uart_write_byte_raw(uart_id_t id, uint8_t data)
{
	/* wait for fifo write ready
	 * optimize it when write very fast
	 * wait for fifo write ready will waste CPU performance
	 */
	BK_WHILE (!uart_hal_is_fifo_write_ready(&s_uart[id].hal, id));
	uart_hal_write_byte(&s_uart[id].hal, id, data);
	return BK_OK;
}

bk_err_t uart_write_byte(uart_id_t id, uint8_t data)
{
	bk_err_t ret;

	UART_FAST_TX_RETURN_ON_SUSPEND(id);
	ret = uart_write_byte_raw(id, data);
	UART_FAST_TX_EXIT(id);
	return ret;
}

typedef struct {
	struct {uart_hw_t *hw;} hal;
} uart_unsafe_t;
const uart_unsafe_t s_uart_unsafe_hw[SOC_UART_ID_NUM_PER_UNIT] = {
	{
		.hal.hw = (uart_hw_t *)SOC_UART0_REG_BASE,
	},
	{
		.hal.hw = (uart_hw_t *)SOC_UART1_REG_BASE,
	},
#if (SOC_UART_ID_NUM_PER_UNIT  >= 3)
	{
		.hal.hw = (uart_hw_t *)SOC_UART2_REG_BASE,
	},
#endif
#if (SOC_UART_ID_NUM_PER_UNIT  >= 4)
	{
		.hal.hw = (uart_hw_t *)SOC_UART3_REG_BASE,
	},
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 5)
	{
		.hal.hw = (uart_hw_t *)SOC_UART4_REG_BASE,
	},
#endif
#if (SOC_UART_ID_NUM_PER_UNIT >= 6)
	{
		.hal.hw = (uart_hw_t *)SOC_UART5_REG_BASE,
	},
#endif
};

void bk_uart_snapshot_unsafe(uart_id_t id, bk_uart_unsafe_snapshot_t *snapshot)
{
	uart_hw_t *hw = s_uart_unsafe_hw[id].hal.hw;

	snapshot->timestamp_ms = bk_aon_rtc_get_ms();
	snapshot->global_ctrl = hw->global_ctrl.v;
	snapshot->config = hw->config.v;
	snapshot->fifo_config = hw->fifo_config.v;
	snapshot->fifo_status = hw->fifo_status.v;
	snapshot->int_enable = hw->int_enable.v;
	snapshot->int_status = hw->int_status.v;
	snapshot->flow_ctrl_config = hw->flow_ctrl_config.v;
	snapshot->wake_config = hw->wake_config.v;
}

void bk_uart_recover_unsafe(uart_id_t id, const bk_uart_unsafe_snapshot_t *snapshot)
{
	uart_hw_t *hw = s_uart_unsafe_hw[id].hal.hw;

	hw->int_enable.v = 0U;
	hw->config.tx_enable = 0U;
	hw->global_ctrl.soft_reset = 1U;
	__DSB();

	hw->config.v = snapshot->config & ~1U;
	hw->fifo_config.v = snapshot->fifo_config;
	hw->flow_ctrl_config.v = 0U;
	hw->wake_config.v = 0U;
	hw->int_status.v = 0xFFU;
	hw->global_ctrl.clk_gate_bypass = 1U;
	__DSB();

	hw->config.tx_enable = 1U;
	__DSB();
}

bk_err_t bk_uart_write_byte_unsafe(uart_id_t id, uint8_t data)
{
	uint64_t start_ms = bk_aon_rtc_get_ms();

	while (!uart_hal_is_fifo_write_ready(&s_uart_unsafe_hw[id].hal, id)) {
		if ((bk_aon_rtc_get_ms() - start_ms) >= CONFIG_UART_UNSAFE_WRITE_TIMEOUT_MS) {
			return BK_ERR_TIMEOUT;
		}
	}
	uart_hal_write_byte(&s_uart_unsafe_hw[id].hal, id, data);
	return BK_OK;
}

void uart_write_byte_for_ate(uart_id_t id, uint8_t *data, uint8_t cnt)
{
    int i;
    if (uart_fast_tx_enter(id) != BK_OK) {
        return;
    }
    for(i = 0; i < cnt; i ++)
    {
        BK_WHILE (!uart_hal_is_fifo_write_ready(&s_uart[id].hal, id));
        uart_hal_write_byte(&s_uart[id].hal, id, data[i]);
    }
    UART_FAST_TX_EXIT(id);
}

void uart_write_byte_for_fr(uart_id_t id, uint8_t *data, uint8_t cnt)
{
    int i;

    int port = id;

    if (bk_get_printf_port() == port)
        BK_LOGD(NULL, "!UART_ID_2\n");

    BK_ASSERT (port != bk_get_printf_port());

    if (uart_fast_tx_enter(id) != BK_OK) {
        return;
    }
    for(i = 0; i < cnt; i ++)
    {
        BK_WHILE (!uart_hal_is_fifo_write_ready(&s_uart[port].hal, port));
        uart_hal_write_byte(&s_uart[port].hal, port, data[i]);
    }
    UART_FAST_TX_EXIT(id);
}


bk_err_t uart_write_ready(uart_id_t id)
{
	/* wait for fifo write ready
	 * optimize it when write very fast
	 * wait for fifo write ready will waste CPU performance
	 */
	if (!uart_hal_is_fifo_write_ready(&s_uart[id].hal, id)) {
		return BK_FAIL;
	}

	return BK_OK;
}

bk_err_t uart_write_string(uart_id_t id, const char *string)
{
	const char *p = string;

	UART_FAST_TX_RETURN_ON_SUSPEND(id);
	while (*string) {
		if (*string == '\n') {
			if (p == string || *(string - 1) != '\r')
				uart_write_byte_raw(id, '\r'); /* append '\r' */
		}
		uart_write_byte_raw(id, *string++);
	}

	UART_FAST_TX_EXIT(id);
	return BK_OK;
}

bk_err_t uart_read_ready(uart_id_t id)
{
	if (!uart_hal_is_fifo_read_ready(&s_uart[id].hal, id)) {
		return BK_FAIL;
	}

	return BK_OK;
}

int uart_read_byte(uart_id_t id)
{
	int val = -1;
	if (uart_hal_is_fifo_read_ready(&s_uart[id].hal, id)) {
		val = uart_hal_read_byte(&s_uart[id].hal, id);
	}
	return val;
}

int uart_read_byte_ex(uart_id_t id, uint8_t *ch)
{
	int val = -1;
	if (uart_hal_is_fifo_read_ready(&s_uart[id].hal, id)) {
		*ch = uart_hal_read_byte(&s_uart[id].hal, id);
		val = 0;
	}
	return val;
}

uint32_t uart_get_length_in_buffer(uart_id_t id)
{
	return kfifo_data_size(s_uart_rx_kfifo[id]);
}

static void uart_isr_register_functions(uart_id_t id)
{
	switch(id)
	{
		case UART_ID_0:
			bk_int_isr_register(INT_SRC_UART0, uart0_isr, NULL);
			break;
		case UART_ID_1:
			bk_int_isr_register(INT_SRC_UART1, uart1_isr, NULL);
			break;
		case UART_ID_2:
			bk_int_isr_register(INT_SRC_UART2, uart2_isr, NULL);
			break;
#if (SOC_UART_ID_NUM_PER_UNIT  >= 4)
		case UART_ID_3:
			bk_int_isr_register(INT_SRC_UART3, uart3_isr, NULL);
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT  >= 5)
		case UART_ID_4:
			bk_int_isr_register(INT_SRC_UART4, uart4_isr, NULL);
			break;
#endif
#if (SOC_UART_ID_NUM_PER_UNIT  >= 6)
		case UART_ID_5:
			bk_int_isr_register(INT_SRC_UART5, uart5_isr, NULL);
			break;
#endif
		default:
			break;
	}
}

uint32_t uart_id_to_pm_uart_id(uint32_t uart_id)
{
	switch (uart_id)
	{
		case UART_ID_0:
			return PM_DEV_ID_UART1;

		case UART_ID_1:
			return PM_DEV_ID_UART2;

		case UART_ID_2:
			return PM_DEV_ID_UART3;

		default:
			return PM_DEV_ID_UART1;
	}
}

static bk_err_t uart_enter_deep_sleep(uint64_t sleep_time, void *args)
{
	uart_id_t uart_id = (uart_id_t)args;

	// disable TX firstly, then set tx_stopped to 1.
	bk_uart_disable_tx_interrupt(uart_id);

	// suspend, tx stopped after fifo empty.
	while((bk_uart_is_tx_over(uart_id) == 0))
	{
	}
	bk_uart_set_enable_tx(uart_id, 0);

	bk_uart_set_enable_rx(uart_id, 0);

	return BK_OK;
}

#if CONFIG_UART_PM_CB_SUPPORT	//this macro config set to n

#if CONFIG_UART_RX_DMA
static bk_err_t uart_rx_dma_restore(uart_id_t id);
#endif
#if CONFIG_UART_TX_DMA
static bk_err_t uart_tx_dma_restore(uart_id_t id);
#endif
static bk_err_t uart_pm_backup(uint64_t sleep_time, void *args)
{
	uart_id_t uart_id = (uart_id_t)args;

	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(uart_id);
	UART_RETURN_ON_ID_NOT_INIT(uart_id);

	if (!s_uart[uart_id].pm_bakeup_is_valid)
	{
		uart_hal_backup(&s_uart[uart_id].hal, s_uart[uart_id].pm_backup);
		s_uart[uart_id].pm_bakeup_is_valid = 1;
	}
	return BK_OK;
}

static bk_err_t uart_pm_restore(uint64_t sleep_time, void *args)
{
	uart_id_t uart_id = (uart_id_t)args;

	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(uart_id);
	UART_RETURN_ON_ID_NOT_INIT(uart_id);

	if (s_uart[uart_id].pm_bakeup_is_valid)
	{
		uart_hal_restore(&s_uart[uart_id].hal, s_uart[uart_id].pm_backup);

		/*
		 * Low voltage wake-up only restores UART HAL registers; the DMA
		 * channel registers (src/dst addr, transfer len, isr, finish-int
		 * enable, start bit) are not preserved. Re-configure and restart
		 * the DMA channel here so that RX path works immediately after
		 * wake-up without depending on a later API call.
		 */
		#if CONFIG_BK_MODEM
		extern uint8_t bk_modem_get_state(void);
		if (bk_modem_get_state() == 5)
		{
			#if CONFIG_UART_RX_DMA
			if (s_uart[uart_id].rx_dma_enable)
			{
				uart_rx_dma_restore(uart_id);
			}
			#endif
			#if CONFIG_UART_TX_DMA
			if (s_uart[uart_id].tx_dma_enable)
			{
				uart_tx_dma_restore(uart_id);
			}
			#endif
		}
		#endif

		s_uart[uart_id].pm_bakeup_is_valid = 0;
	}

	return BK_OK;
}

bk_err_t bk_uart_pm_backup(uart_id_t id)
{
	if ((id == UART_ID_1) || (id == UART_ID_2))
	{
		uart_pm_backup(0, (void *)id);
		return BK_OK;
	}
	return BK_FAIL;
}

bk_err_t bk_uart_pm_restore(uart_id_t id)
{
	uart_pm_restore(0, (void *)id);
	return BK_OK;
}
#else
bk_err_t bk_uart_pm_backup(uart_id_t id)
{
	return BK_OK;
}

bk_err_t bk_uart_pm_restore(uart_id_t id)
{
	return BK_OK;
}
#endif

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
static void uart_fast_backup_one(uart_id_t id, uint32_t *backup)
{
	uart_hw_t *hw = s_uart[id].hal.hw;

	backup[0] = hw->config.v;
	backup[1] = hw->fifo_config.v;
	backup[2] = hw->int_enable.v;
	backup[3] = hw->flow_ctrl_config.v;
	backup[4] = hw->wake_config.v;
	backup[5] = hw->global_ctrl.v;
	hw->global_ctrl.v = backup[5] & ~1U;
}

static void uart_fast_restore_one(uart_id_t id, const uint32_t *backup)
{
	uart_hw_t *hw = s_uart[id].hal.hw;

	hw->config.v = backup[0];
	hw->fifo_config.v = backup[1];
	hw->int_enable.v = backup[2];
	hw->flow_ctrl_config.v = backup[3];
	hw->wake_config.v = backup[4];
	hw->global_ctrl.v = backup[5];
}

/*
 * AP power-off drops BAKP, so any UART DMA bytes in flight or arriving
 * while AP is down are discarded. Stop the channels here so GDMA quiesce
 * sees them idle; resume re-inits from kfifo start (same contract as the
 * legacy LV uart_rx_dma_restore path).
 */
static void uart_fast_pause_dma(uint32_t active)
{
	for (uart_id_t id = UART_FAST_PM_FIRST_ID;
		id <= UART_FAST_PM_LAST_ID; id++) {
		if (!(active & BIT(id))) {
			continue;
		}
#if CONFIG_UART_RX_DMA
		if (s_uart[id].rx_dma_enable &&
			(s_uart[id].rx_dma_id < DMA_ID_MAX)) {
			bk_dma_stop(s_uart[id].rx_dma_id);
			s_uart[id].rx_dma_stopped = true;
		}
#endif
#if CONFIG_UART_TX_DMA
		if (s_uart[id].tx_dma_enable &&
			(s_uart[id].tx_dma_id < DMA_ID_MAX)) {
			bk_dma_stop(s_uart[id].tx_dma_id);
		}
#endif
	}
}

static void uart_fast_resume_dma(uint32_t active)
{
	for (uart_id_t id = UART_FAST_PM_FIRST_ID;
		id <= UART_FAST_PM_LAST_ID; id++) {
		if (!(active & BIT(id))) {
			continue;
		}
#if CONFIG_UART_RX_DMA
		if (s_uart[id].rx_dma_enable) {
			(void)uart_rx_dma_restore(id);
		}
#endif
#if CONFIG_UART_TX_DMA
		if (s_uart[id].tx_dma_enable) {
			(void)uart_tx_dma_restore(id);
		}
#endif
	}
}

static bk_err_t uart_fast_wait_tx_idle(
	uart_fast_pm_context_t *ctx, uint32_t start_ms)
{
	while (__atomic_load_n(&ctx->tx_busy_count, __ATOMIC_ACQUIRE) != 0U) {
		if ((rtos_get_time() - start_ms) >=
			UART_FAST_PM_QUIESCE_MS) {
			return BK_ERR_TIMEOUT;
		}
		rtos_delay_milliseconds(1);
	}
	return BK_OK;
}

static bk_err_t uart_fast_wait_tx_complete(uart_id_t id, uint32_t start_ms)
{
	uart_hw_t *hw = s_uart[id].hal.hw;
	uint32_t uart_clk = clk_get_uart_clk(id) ?
		UART_CLOCK_FREQ_120M : UART_CLOCK_FREQ_26M;
	uint32_t baud_rate = uart_clk / (hw->config.clk_div + 1U);
	uint32_t frame_bits = 1U + 5U + hw->config.data_bits +
		(hw->config.parity_en ? 1U : 0U) +
		(hw->config.stop_bits ? 2U : 1U);
	uint32_t frame_ms = MAX(
		(frame_bits * 1000U + baud_rate - 1U) / baud_rate, 1U);

	while (!uart_hal_is_tx_fifo_empty(&s_uart[id].hal, id)) {
		if ((rtos_get_time() - start_ms) >=
			UART_FAST_PM_QUIESCE_MS) {
			return BK_ERR_TIMEOUT;
		}
		rtos_delay_milliseconds(1);
	}
	if (((rtos_get_time() - start_ms) + frame_ms) >
		UART_FAST_PM_QUIESCE_MS) {
		return BK_ERR_TIMEOUT;
	}
	rtos_delay_milliseconds(frame_ms);
	return BK_OK;
}

static bk_err_t uart_fast_quiesce(void *arg)
{
	uart_fast_pm_context_t *ctx = arg;
	uint32_t active = __atomic_load_n(&ctx->active_mask,
		__ATOMIC_ACQUIRE);
	uint32_t start_ms = rtos_get_time();
	bk_err_t ret;

	__atomic_store_n(&ctx->tx_suspended, true, __ATOMIC_RELEASE);
	ret = uart_fast_wait_tx_idle(ctx, start_ms);
	if (ret != BK_OK) {
		__atomic_store_n(&ctx->tx_suspended, false, __ATOMIC_RELEASE);
		return ret;
	}

	active = __atomic_load_n(&ctx->active_mask, __ATOMIC_ACQUIRE);
	for (uart_id_t id = UART_FAST_PM_FIRST_ID;
		id <= UART_FAST_PM_LAST_ID; id++) {
		if (!(active & BIT(id))) {
			continue;
		}
		ret = uart_fast_wait_tx_complete(id, start_ms);
		if (ret != BK_OK) {
			__atomic_store_n(&ctx->tx_suspended, false,
				__ATOMIC_RELEASE);
			return ret;
		}
	}
	uart_fast_pause_dma(active);
	ctx->dma_paused = true;
	return BK_OK;
}

static bk_err_t uart_fast_backup(void *arg)
{
	uart_fast_pm_context_t *ctx = arg;
	uint32_t active = __atomic_load_n(&ctx->active_mask,
		__ATOMIC_ACQUIRE);

	ctx->backup_valid_mask = 0U;
	for (uart_id_t id = UART_FAST_PM_FIRST_ID;
		id <= UART_FAST_PM_LAST_ID; id++) {
		if (active & BIT(id)) {
			uart_fast_backup_one(id, ctx->backup[id]);
			ctx->backup_valid_mask |= BIT(id);
		}
	}
	__DMB();
	return BK_OK;
}

static bk_err_t uart_fast_restore(void *arg)
{
	uart_fast_pm_context_t *ctx = arg;
	uint32_t valid = ctx->backup_valid_mask;

	for (uart_id_t id = UART_FAST_PM_FIRST_ID;
		id <= UART_FAST_PM_LAST_ID; id++) {
		if (valid & BIT(id)) {
			uart_fast_restore_one(id, ctx->backup[id]);
		}
	}
	__DMB();
	return BK_OK;
}

static bk_err_t uart_fast_resume(void *arg)
{
	uart_fast_pm_context_t *ctx = arg;
	uint32_t active = __atomic_load_n(&ctx->active_mask,
		__ATOMIC_ACQUIRE);

	if (ctx->dma_paused) {
		uart_fast_resume_dma(active);
		ctx->dma_paused = false;
	}
	ctx->backup_valid_mask = 0U;
	__atomic_store_n(&ctx->tx_suspended, false, __ATOMIC_RELEASE);
	return BK_OK;
}
#endif

bk_err_t bk_uart_driver_init(void)
{
	if (s_uart_driver_is_init) {
		return BK_OK;
	}

	os_memset(&s_uart_rx_isr, 0, sizeof(s_uart_rx_isr));
	os_memset(&s_uart_tx_isr, 0, sizeof(s_uart_tx_isr));
	uart_statis_init();
	s_uart_driver_is_init = true;

#ifndef CONFIG_BK_PRINTF_DISABLE
	bk_printf_init();
#endif

#if (CONFIG_CLI && CONFIG_UART_API_TEST)
        int bk_uart_api_register_cli_test_feature(void);
        bk_uart_api_register_cli_test_feature();
#endif

#if (CONFIG_CLI && CONFIG_UART_TEST)
        int bk_uart_register_cli_test_feature(void);
        bk_uart_register_cli_test_feature();
#endif

	return BK_OK;
}

bk_err_t bk_uart_driver_deinit(void)
{
	if (!s_uart_driver_is_init)
		return BK_OK;

	for (uart_id_t id = UART_ID_0; id < SOC_UART_ID_NUM_PER_UNIT; id++) {
		bk_uart_deinit(id);
	}

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	if (s_uart_fast_pm.registered) {
		bk_err_t ret = bk_pm_ap_fast_ops_unregister(&s_uart_fast_ops);
		if (ret != BK_OK) {
			return ret;
		}
		s_uart_fast_pm.registered = false;
	}
#endif

	s_uart_driver_is_init = false;

	return BK_OK;
}

int bk_uart_is_in_used(uart_id_t id)
{
	return (s_uart[id].id_init_bits & BIT((id)));
}

int bk_uart_is_rx_dma_enabled(uart_id_t id)
{
	return (s_uart[id].rx_dma_enable);
}

#if (CONFIG_UART_RX_DMA || CONFIG_UART_TX_DMA)
static inline dma_dev_t uart_id_to_dma_dev(uart_id_t id, bool rx)
{
	uint32_t rx_id_offset = 0;
	if(rx)
		rx_id_offset = 1;
	switch(id)
	{
		case UART_ID_0:
			return DMA_DEV_UART1 + rx_id_offset;

		case UART_ID_1:
			return DMA_DEV_UART2 + rx_id_offset;

		case UART_ID_2:
			return DMA_DEV_UART3 + rx_id_offset;

		default:
			return DMA_DEV_UART1 + rx_id_offset;
	}
}
#endif

#if (CONFIG_UART_RX_DMA)
static inline uart_id_t uart_rx_dma_id_to_uart_id(dma_id_t dma_id)
{
    for (uart_id_t id = UART_ID_0; id < SOC_UART_ID_NUM_PER_UNIT; id++) {
        if (s_uart[id].rx_dma_enable && s_uart[id].rx_dma_id == dma_id) {
            return id;
        }
    }
    return UART_ID_MAX;
}

static void uart_rx_dma_fifo_full(dma_id_t dma_id)
{
	uart_id_t id = uart_rx_dma_id_to_uart_id(dma_id);
	
	if (id >= SOC_UART_ID_NUM_PER_UNIT) {
		UART_LOGE("FATAL: Unknown DMA ID %d in RX ISR\n", dma_id);
		return;
	}
	
	// ========================================
	// Step 1: Update write pointer
	// ========================================
	uint32_t completed_len = s_uart[id].last_dma_len;
	uint32_t accounted_len = s_uart[id].rx_dma_accounted_len;
	uint32_t unprocessed_len = completed_len - accounted_len;
	
	s_uart_rx_kfifo[id]->in += unprocessed_len;
	s_uart[id].rx_dma_accounted_len = 0;
	
	// ========================================
	// Step 2: Calculate available space and decide whether to restart DMA
	// ========================================
	uint32_t unused = kfifo_unused(s_uart_rx_kfifo[id]);
	
	if (unused > 0) {
		// ========================================
		// Has space: Reconfigure and start DMA
		// ========================================
		uint32_t in = s_uart_rx_kfifo[id]->in;
		uint32_t write_pos = in & s_uart_rx_kfifo[id]->mask;
		uint32_t space_to_end = s_uart_rx_kfifo[id]->size - write_pos;
		uint32_t next_dma_len = min(unused, space_to_end);
		uint32_t dma_dest_addr = (uint32_t)s_uart_rx_kfifo[id]->buffer + write_pos;
		
		bk_dma_set_dest_start_addr(s_uart[id].rx_dma_id, dma_dest_addr);
		bk_dma_set_transfer_len(s_uart[id].rx_dma_id, next_dma_len);
		bk_dma_start(s_uart[id].rx_dma_id);
		
		s_uart[id].rx_dma_stopped = false;
		s_uart[id].last_dma_len = next_dma_len;
		
	} else {
		// ========================================
		// Software FIFO full: Stop DMA and UART RX
		// ========================================

		// [Critical] Immediately disable UART RX to protect hardware FIFO data
		bk_uart_set_enable_rx(id, 0);

		// Disable RX interrupt (no need to trigger again)
		bk_uart_disable_rx_interrupt(id);

		s_uart[id].rx_hw_stopped = true;
		s_uart[id].rx_dma_stopped = true;

		// ========================================
		// Step 3: Wake up application thread
		// ========================================
		if (s_uart_sema[id].rx_int_sema && s_uart_sema[id].rx_blocked) {
			rtos_set_semaphore(&(s_uart_sema[id].rx_int_sema));
			s_uart_sema[id].rx_blocked = false;
		}
		
		// ========================================
		// Step 4: User callback
		// ========================================
		if (s_uart_rx_isr[id].callback) {
			s_uart_rx_isr[id].callback(id, s_uart_rx_isr[id].param);
		}

		UART_LOGW("UART%d: DMA stopped, RX disabled\n", id);
		
	}
	
}


static void uart_rx_dma_reset_dst_addr(uart_id_t id, uint32_t dma_start_addr, uint32_t len)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_dma_stop(s_uart[id].rx_dma_id);
	bk_dma_set_dest_start_addr(s_uart[id].rx_dma_id, dma_start_addr);
	bk_dma_set_transfer_len(s_uart[id].rx_dma_id, len);
	GLOBAL_INT_RESTORE();
	bk_dma_start(s_uart[id].rx_dma_id);
}

static void uart_rx_dma_rewind_when_fifo_empty(uart_id_t id)
{
	s_uart_rx_kfifo[id]->in = 0;
	s_uart_rx_kfifo[id]->out = 0;
	uart_rx_dma_reset_dst_addr(id, (uint32_t)s_uart_rx_kfifo[id]->buffer, s_uart_rx_kfifo[id]->size);
	s_uart[id].last_dma_len = s_uart_rx_kfifo[id]->size;
	s_uart[id].rx_dma_accounted_len = 0;
	s_uart[id].rx_dma_stopped = false;
}

static inline void uart_rx_dma_src_port_config(uart_id_t id, dma_port_config_t *cfg_ptr)
{
	cfg_ptr->width = DMA_DATA_WIDTH_8BITS;
	cfg_ptr->addr_inc_en = DMA_ADDR_INC_DISABLE,
	cfg_ptr->addr_loop_en = DMA_ADDR_LOOP_DISABLE,
	cfg_ptr->dev = uart_id_to_dma_dev(id, 1);
	BK_LOGD(NULL, "src dev=%d\r\n", cfg_ptr->dev);
	cfg_ptr->end_addr = cfg_ptr->start_addr = (uint32_t)uart_hal_get_read_data_addr(&s_uart[id].hal, id) & 0xffffffffc;
}

bk_err_t uart_rx_dma_init(uart_id_t id)
{
	bk_err_t ret = BK_OK;
	dma_config_t dma_cfg = {0};

	//DMA DST config:Memory
	dma_port_config_t dma_mem_port_config = {
						.dev = DMA_DEV_DTCM,
						.width = DMA_DATA_WIDTH_32BITS,
						.addr_inc_en = DMA_ADDR_INC_ENABLE,
						.addr_loop_en = DMA_ADDR_LOOP_DISABLE,
						.start_addr = 0,
						.end_addr = 0,
					};

	dma_cfg.dst = dma_mem_port_config;
	//force enable sw fifo
	bk_uart_enable_sw_fifo(id);
	if(s_uart_rx_kfifo[id] == NULL)
	{
		ret = uart_id_init_kfifo(id);
		if(ret != BK_OK)
			return ret;
	}
	dma_cfg.dst.start_addr = (uint32_t)s_uart_rx_kfifo[id]->buffer;
	if(dma_cfg.dst.start_addr == 0)
	{
		//TODO:
		BK_ASSERT(0);
	}
	dma_cfg.dst.end_addr = dma_cfg.dst.start_addr + s_uart_rx_kfifo[id]->size;

	//DMA SRC config:UART RX read port
	uart_rx_dma_src_port_config(id, &dma_cfg.src);

	dma_cfg.mode = DMA_WORK_MODE_SINGLE;
	dma_cfg.chan_prio = 0;	//UART speed is slow, so no need high priority

	//init DMA config
	dma_id_t dma_id = bk_dma_alloc(uart_id_to_dma_dev(id, 1));
	if(dma_id < DMA_ID_MAX)
	{
		BK_LOG_ON_ERR(bk_dma_init(dma_id, &dma_cfg));
		BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_id, s_uart_rx_kfifo[id]->size));
#if (CONFIG_SPE)
		BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(dma_id, DMA_ATTR_SEC));
		BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(dma_id, DMA_ATTR_SEC));
#endif
		bk_dma_register_isr(dma_id, NULL, uart_rx_dma_fifo_full);
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_id));

		s_uart[id].rx_dma_stopped = false;
		s_uart[id].rx_hw_stopped = false;
		s_uart[id].last_dma_len = s_uart_rx_kfifo[id]->size;
		s_uart[id].rx_dma_accounted_len = 0;

		BK_LOG_ON_ERR(bk_dma_start(dma_id));
	}
	else
	{
		BK_LOGD(NULL, "Err:uart rx dma alloc fail\r\n");
		return BK_FAIL;
	}

	s_uart[id].rx_dma_id = dma_id;
	s_uart[id].rx_dma_enable = 1;

	return BK_OK;
}

bk_err_t uart_rx_dma_deinit(uart_id_t id)
{
	dma_id_t dma_id = s_uart[id].rx_dma_id;

	if (dma_id < DMA_ID_MAX)
	{
		bk_dma_stop(dma_id);
	}

	s_uart[id].rx_dma_id = 0;
	s_uart[id].rx_dma_enable = 0;
	s_uart[id].rx_dma_stopped = false;
	s_uart[id].rx_hw_stopped = false;
	s_uart[id].last_dma_len = 0;
	s_uart[id].rx_dma_accounted_len = 0;

	return bk_dma_free(uart_id_to_dma_dev(id, 1), dma_id);
}

/*
 * Re-configure and restart the RX DMA channel on an already-allocated
 * rx_dma_id after power-loss (LV restore or AP Fast Boot resume). The
 * DMA channel is NOT re-allocated. Destination is reset to kfifo start,
 * so unread bytes from before the power-off window are discarded.
 */
static bk_err_t uart_rx_dma_restore(uart_id_t id)
{
	dma_id_t dma_id = s_uart[id].rx_dma_id;
	dma_config_t dma_cfg = {0};

	if (dma_id >= DMA_ID_MAX)
		return BK_FAIL;

	if (s_uart_rx_kfifo[id] == NULL)
		return BK_FAIL;

	bk_dma_stop(dma_id);

	dma_cfg.dst.dev          = DMA_DEV_DTCM;
	dma_cfg.dst.width        = DMA_DATA_WIDTH_32BITS;
	dma_cfg.dst.addr_inc_en  = DMA_ADDR_INC_ENABLE;
	dma_cfg.dst.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
	dma_cfg.dst.start_addr   = (uint32_t)s_uart_rx_kfifo[id]->buffer;
	dma_cfg.dst.end_addr     = dma_cfg.dst.start_addr + s_uart_rx_kfifo[id]->size;

	uart_rx_dma_src_port_config(id, &dma_cfg.src);

	dma_cfg.mode      = DMA_WORK_MODE_SINGLE;
	dma_cfg.chan_prio = 0;

	BK_LOG_ON_ERR(bk_dma_init(dma_id, &dma_cfg));
	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_id, s_uart_rx_kfifo[id]->size));
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(dma_id, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(dma_id, DMA_ATTR_SEC));
#endif
	bk_dma_register_isr(dma_id, NULL, uart_rx_dma_fifo_full);
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_id));

	/*
	 * DMA destination is reset to kfifo buffer start, so align kfifo
	 * pointers and software DMA accounting. Any unread bytes buffered
	 * before low voltage are discarded, which is acceptable on wake-up.
	 */
	s_uart_rx_kfifo[id]->in          = 0;
	s_uart_rx_kfifo[id]->out         = 0;
	s_uart[id].last_dma_len          = s_uart_rx_kfifo[id]->size;
	s_uart[id].rx_dma_accounted_len  = 0;
	s_uart[id].rx_dma_stopped        = false;
	s_uart[id].rx_hw_stopped         = false;

	BK_LOG_ON_ERR(bk_dma_start(dma_id));

	return BK_OK;
}
#endif

#if (CONFIG_UART_TX_DMA)
static inline void uart_tx_dma_dst_port_config(uart_id_t id, dma_port_config_t *cfg_ptr)
{
	cfg_ptr->width = DMA_DATA_WIDTH_8BITS;
	cfg_ptr->addr_inc_en = DMA_ADDR_INC_DISABLE,
	cfg_ptr->addr_loop_en = DMA_ADDR_LOOP_DISABLE,
	cfg_ptr->dev = uart_id_to_dma_dev(id, 0);
	//BK_LOGD(NULL, "%s dst dev=%d\r\n", __func__, cfg_ptr->dev);
	uint32_t fifo_address = (uint32_t)uart_hal_get_write_data_addr(&s_uart[id].hal, id) & 0xfffffffff;
	//BK_LOGD(NULL, "%s dst fifo_address=0x%x\r\n", __func__, fifo_address);
	cfg_ptr->start_addr = fifo_address;
	cfg_ptr->end_addr = fifo_address;
}

static void uart_tx_dma_write_done(dma_id_t dma_id)
{
	UART_LOGV("%s:dma_id=%d\r\n", __func__, dma_id);
	
}

static bk_err_t uart_tx_dma_write_to_fifo(uart_id_t id, uint32_t data_address, uint32_t size)
{
	dma_id_t dma_id = s_uart[id].tx_dma_id;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

	BK_LOG_ON_ERR(bk_dma_set_transfer_len(dma_id, size));
	BK_LOG_ON_ERR(bk_dma_set_src_addr(dma_id, data_address, data_address + size));
	GLOBAL_INT_RESTORE();

	BK_LOG_ON_ERR(bk_dma_start(dma_id));

	return BK_OK;
}
static bk_err_t uart_tx_dma_init(uart_id_t id)
{
	//DMA malloc chn
	dma_id_t dma_id = bk_dma_alloc(uart_id_to_dma_dev(id, 0));
	if(dma_id < DMA_ID_MAX) {
		dma_config_t dma_cfg = {0};
		uint32_t tx_dma_test_buffer[8] = {0};
		//DMA DST config:UART TX write port
		uart_tx_dma_dst_port_config(id, &dma_cfg.dst);
		
		//DMA SRC config:Memory
		dma_port_config_t dma_mem_port_config = {
							.dev = DMA_DEV_DTCM,
							.width = DMA_DATA_WIDTH_32BITS,
							.addr_inc_en = DMA_ADDR_INC_ENABLE,
							.addr_loop_en = DMA_ADDR_LOOP_DISABLE,
							.start_addr = (uint32_t)(&tx_dma_test_buffer[0]),
							.end_addr = (uint32_t)(&tx_dma_test_buffer[0]) + 8,
						};
		
		dma_cfg.src = dma_mem_port_config;
		dma_cfg.mode = DMA_WORK_MODE_SINGLE;
		dma_cfg.chan_prio = 0;	//UART speed is slow, so no need high priority
		dma_cfg.dest_wr_intlv = 8;
		BK_LOG_ON_ERR(bk_dma_init(dma_id, &dma_cfg));
		bk_dma_set_dest_burst_len(dma_id, BURST_LEN_SINGLE);
		bk_dma_set_src_burst_len(dma_id, BURST_LEN_SINGLE);
#if (CONFIG_SPE)
		BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(dma_id, DMA_ATTR_SEC));
		BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(dma_id, DMA_ATTR_SEC));
#endif
		BK_LOG_ON_ERR(bk_dma_register_isr(dma_id, NULL, uart_tx_dma_write_done));
		BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_id));

	} else {
		BK_LOGD(NULL, "Err:uart tx dma alloc fail\r\n");
		return BK_FAIL;
	}
	s_uart[id].tx_dma_id = dma_id;
	s_uart[id].tx_dma_enable = 1;

	return BK_OK;
}

static bk_err_t uart_tx_dma_deinit(uart_id_t id)
{
	dma_id_t dma_id = s_uart[id].tx_dma_id;
	s_uart[id].tx_dma_id = 0;
	s_uart[id].tx_dma_enable = 0;
	return bk_dma_free(uart_id_to_dma_dev(id, 0), dma_id);
}

/*
 * Re-configure the TX DMA channel on an already-allocated tx_dma_id.
 * TX DMA is started per-transfer by uart_tx_dma_write_to_fifo(), so this
 * function only restores channel configuration without calling start.
 */
static bk_err_t uart_tx_dma_restore(uart_id_t id)
{
	dma_id_t dma_id = s_uart[id].tx_dma_id;
	dma_config_t dma_cfg = {0};
	uint32_t tx_dma_dummy_buffer[8] = {0};

	if (dma_id >= DMA_ID_MAX)
		return BK_FAIL;

	bk_dma_stop(dma_id);

	uart_tx_dma_dst_port_config(id, &dma_cfg.dst);

	dma_cfg.src.dev          = DMA_DEV_DTCM;
	dma_cfg.src.width        = DMA_DATA_WIDTH_32BITS;
	dma_cfg.src.addr_inc_en  = DMA_ADDR_INC_ENABLE;
	dma_cfg.src.addr_loop_en = DMA_ADDR_LOOP_DISABLE;
	dma_cfg.src.start_addr   = (uint32_t)(&tx_dma_dummy_buffer[0]);
	dma_cfg.src.end_addr     = (uint32_t)(&tx_dma_dummy_buffer[0]) + 8;

	dma_cfg.mode          = DMA_WORK_MODE_SINGLE;
	dma_cfg.chan_prio     = 0;
	dma_cfg.dest_wr_intlv = 8;

	BK_LOG_ON_ERR(bk_dma_init(dma_id, &dma_cfg));
	bk_dma_set_dest_burst_len(dma_id, BURST_LEN_SINGLE);
	bk_dma_set_src_burst_len(dma_id, BURST_LEN_SINGLE);
#if (CONFIG_SPE)
	BK_LOG_ON_ERR(bk_dma_set_dest_sec_attr(dma_id, DMA_ATTR_SEC));
	BK_LOG_ON_ERR(bk_dma_set_src_sec_attr(dma_id, DMA_ATTR_SEC));
#endif
	BK_LOG_ON_ERR(bk_dma_register_isr(dma_id, NULL, uart_tx_dma_write_done));
	BK_LOG_ON_ERR(bk_dma_enable_finish_interrupt(dma_id));

	return BK_OK;
}
#endif

bk_err_t bk_uart_init(uart_id_t id, const uart_config_t *config)
{
	UART_RETURN_ON_NOT_INIT();
	BK_RETURN_ON_NULL(config);
	UART_RETURN_ON_INVALID_ID(id);
	if (config->src_clk == UART_SCLK_80M) {
		uint32_t clock_ratio = UART_CLOCK_FREQ_80M / UART_CLOCK;  // 80M / 40M = 2
		UART_RETURN_ON_BAUD_RATE_NOT_SUPPORT(config->baud_rate/clock_ratio);
	} else {
		UART_RETURN_ON_BAUD_RATE_NOT_SUPPORT(config->baud_rate);
	}
	UART_CHECK_SECURE(id);

	/* If UART is already initialized, return OK directly */
	if (s_uart[id].id_init_bits & BIT(id)) {

		return BK_OK;
	}

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	if (uart_fast_managed_id(id) && !s_uart_fast_pm.registered) {
		bk_err_t ret = bk_pm_ap_fast_ops_register(&s_uart_fast_ops);
		if (ret != BK_OK) {
			return ret;
		}
		s_uart_fast_pm.registered = true;
	}
#endif

#if CONFIG_UART_PM_CB_SUPPORT	//this macro config set to n
	pm_cb_conf_t uart_enter_config = {
		.cb = (pm_cb)uart_pm_backup,
		.args = (void *)id
	};

	if (id == UART_ID_1) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_UART1, PM_POWER_MODULE_STATE_ON);
		bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_UART2, &uart_enter_config, NULL);
		bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_UART2);
	} else if (id == UART_ID_2) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_UART2, PM_POWER_MODULE_STATE_ON);
		bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_UART3, &uart_enter_config, NULL);
		bk_pm_module_lv_sleep_state_clear(PM_DEV_ID_UART3);
	}
#endif

#if CONFIG_UART_PM_CB_SUPPORT
	pm_cb_conf_t enter_config = {
		.cb = (pm_cb)uart_enter_deep_sleep,
		.args = (void *)id
	};
	pm_cb_conf_t exit_config = {
		.cb = NULL,
		.args = (void *)PM_CB_PRIORITY_1
	};
	u8 pm_uart_port = uart_id_to_pm_uart_id(id);

	bk_pm_sleep_register_cb(PM_MODE_DEEP_SLEEP, pm_uart_port, &enter_config, &exit_config);
#endif


	uart_hal_disable_tx_interrupt(&s_uart[id].hal, id);
	uart_hal_clear_id_tx_interrupt_status(&s_uart[id].hal, id);
	uart_isr_register_functions(id);
	s_uart[id].hal.id = id;
	uart_hal_init(&s_uart[id].hal);


	uart_interrupt_enable(id);
	uart_id_init_common(id);
	if (config->src_clk == UART_SCLK_APLL)
		sys_drv_uart_select_clock(id, UART_SCLK_APLL);
	else
		sys_drv_uart_select_clock(id, UART_SCLK_XTAL_26M);

#if CONFIG_UART_RX_DMA
	if(config->rx_dma_en)
	{
		uart_rx_dma_init(id);
	}
	s_uart[id].rx_dma_rewind_when_fifo_empty = (config->rx_dma_en && config->rx_dma_rewind_when_fifo_empty);
#else	//avoid set err parameter.
	if(config->rx_dma_en)
	{
		UART_LOGW("uart(%d)Please enable MACRO CONFIG_UART_RX_DMA then set DMA enable parameter\n", id);
	}
#endif

#if CONFIG_UART_TX_DMA
	if(config->tx_dma_en)
	{
		//DMA init TX
		uart_tx_dma_init(id);
	}
#else	//avoid set err parameter.
	if(config->tx_dma_en)
	{
		UART_LOGW("uart(%d)Please enable MACRO CONFIG_UART_TX_DMA then set DMA enable parameter\n", id);
	}
#endif

#if CONFIG_UART_SW_FLOW_CTRL
	if (config->enable_sw_flow_ctrl) {
		s_uart[id].sw_flow_ctrl_en = true;
		s_uart[id].rts_gpio = config->rts_gpio;
		s_uart[id].cts_gpio = config->cts_gpio;
		s_uart[id].rx_int_enable = true;
		bk_uart_enable_sw_fifo(id);
		uart_adjust_sw_flow_control(id, config->baud_rate);

		usfc_gpio_init(id);
	} else {
		s_uart[id].sw_flow_ctrl_en = false;
		s_uart[id].rts_gpio = GPIO_NUM_MAX;
		s_uart[id].cts_gpio = GPIO_NUM_MAX;
	}
#endif

	uart_hal_init_uart(&s_uart[id].hal, id, config);
	uart_hal_start_common(&s_uart[id].hal, id);

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	if (uart_fast_managed_id(id)) {
		__atomic_or_fetch(&s_uart_fast_pm.active_mask, BIT(id),
			__ATOMIC_RELEASE);
	}
#endif

	return BK_OK;
}

bk_err_t bk_uart_deinit(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);

	/* If UART is already deinitialized, return OK directly */
	if (!(s_uart[id].id_init_bits & BIT(id))) {

		return BK_OK;
	}
	
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	if (uart_fast_managed_id(id)) {
		__atomic_and_fetch(&s_uart_fast_pm.active_mask, ~BIT(id),
			__ATOMIC_RELEASE);
	}
#endif

#if CONFIG_UART_RX_DMA
	uart_rx_dma_deinit(id);
#endif
#if CONFIG_UART_TX_DMA
	uart_tx_dma_deinit(id);
#endif

	uart_id_deinit_common(id);

#if CONFIG_UART_SW_FLOW_CTRL
		usfc_gpio_deinit(id);
#endif

#if CONFIG_UART_PM_CB_SUPPORT	//this macro config set to n
	if (id == UART_ID_1) {
		bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_UART2, true, false);
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_UART1, PM_POWER_MODULE_STATE_OFF);
	} else if (id == UART_ID_2) {
		bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_UART3, true, false);
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_UART2, PM_POWER_MODULE_STATE_OFF);
	}
#endif

	return BK_OK;
}

bk_err_t bk_uart_set_baud_rate(uart_id_t id, uint32_t baud_rate)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	UART_RETURN_ON_BAUD_RATE_NOT_SUPPORT(baud_rate);
	uint32_t uart_clk = clk_get_uart_clk(id);
#if (SOC_UART_ID_NUM_PER_UNIT >= 6)
	// Force UART5 to use 26MHz clock like UART0
	// sys_hal_uart_select_clock_get returns 0 for BK7259, so we need to explicitly set it
	if (id == UART_ID_5) {
		uart_clk = UART_SCLK_APLL;
	}
#endif
	uart_hal_set_baud_rate(&s_uart[id].hal, id, uart_clk, baud_rate);
	return BK_OK;
}

bk_err_t bk_uart_set_data_bits(uart_id_t id, uart_data_bits_t data_bits)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_data_bits(&s_uart[id].hal, id, data_bits);
	return BK_OK;
}

bk_err_t bk_uart_set_stop_bits(uart_id_t id, uart_stop_bits_t stop_bits)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_stop_bits(&s_uart[id].hal, id, stop_bits);
	return BK_OK;
}

bk_err_t bk_uart_set_parity(uart_id_t id, uart_parity_t partiy)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_parity(&s_uart[id].hal, id, partiy);
	return BK_OK;
}

#if CONFIG_ENABLE_FILTER_GLITCH
bk_err_t bk_uart_set_glitch_width(uart_id_t id, uint32_t width)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_glitch_width(&s_uart[id].hal, width);
	return BK_OK;
}

uint32_t bk_uart_get_glitch_width(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	return uart_hal_get_glitch_width(&s_uart[id].hal);
}

bk_err_t bk_uart_enable_filter_glitch(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_enable_glitch_cancel(&s_uart[id].hal);

	return BK_OK;
}

bk_err_t bk_uart_disable_filter_glitch(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_disable_glitch_cancel(&s_uart[id].hal);

	return BK_OK;
}
#endif

bk_err_t bk_uart_enable_tx_interrupt(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_enable_tx_interrupt(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_disable_tx_interrupt(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_disable_tx_interrupt(&s_uart[id].hal, id);
	uart_hal_clear_id_tx_interrupt_status(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_enable_rx_interrupt(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_enable_rx_interrupt(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_disable_rx_interrupt(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_disable_rx_interrupt(&s_uart[id].hal, id);
	uart_hal_clear_id_rx_interrupt_status(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_register_rx_isr(uart_id_t id, uart_isr_t isr, void *param)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	s_uart_rx_isr[id].callback = isr;
	s_uart_rx_isr[id].param = param;
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

bk_err_t bk_uart_register_tx_isr(uart_id_t id, uart_isr_t isr, void *param)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	s_uart_tx_isr[id].callback = isr;
	s_uart_tx_isr[id].param = param;
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

static uart_callback_t s_last_uart_rx_isr = {0};

bk_err_t bk_uart_take_rx_isr(uart_id_t id, uart_isr_t isr, void *param)
{
	UART_PM_CHECK_RESTORE(id);

	s_last_uart_rx_isr.callback = s_uart_rx_isr[id].callback;
	s_last_uart_rx_isr.param = s_uart_rx_isr[id].param;

	BK_RETURN_ON_ERR(bk_uart_disable_sw_fifo(id));
	BK_RETURN_ON_ERR(bk_uart_register_rx_isr(id, isr, param));

	return BK_OK;
}

bk_err_t bk_uart_recover_rx_isr(uart_id_t id)
{
	UART_PM_CHECK_RESTORE(id);

	bk_uart_register_rx_isr(id, s_last_uart_rx_isr.callback, s_last_uart_rx_isr.param);

#if (!CONFIG_SHELL_ASYNCLOG)
	bk_uart_enable_sw_fifo(id);
#else
	if (id != bk_get_printf_port()) {
		bk_uart_enable_sw_fifo(id);
	}
#endif

	s_last_uart_rx_isr.callback = NULL;
	s_last_uart_rx_isr.param = NULL;

	return BK_OK;
}

bk_err_t bk_uart_write_bytes(uart_id_t id, const void *data, uint32_t size)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_RETURN_ON_ID_NOT_INIT(id);
	UART_FAST_TX_RETURN_ON_SUSPEND(id);
	UART_PM_CHECK_RESTORE(id);
#if CONFIG_UART_SW_FLOW_CTRL
	const uint8_t *data_ptr = (const uint8_t *)data;
	uint32_t bytes_sent = 0;
	extern void vPortYield( void );
	while (bytes_sent < size)
	{
		while (!usfc_tx_is_enable(id))
		{
			vPortYield();
		}

		uint32_t tx_fifo_cnt = uart_hal_get_tx_fifo_cnt(&s_uart[id].hal, id);
		while (tx_fifo_cnt > 64)
		{
			vPortYield();
			tx_fifo_cnt = uart_hal_get_tx_fifo_cnt(&s_uart[id].hal, id);
		}
		
		uint32_t available_space = UART_TX_HW_FIFO_THRESHOLD - tx_fifo_cnt;
		uint32_t to_send = MIN(size - bytes_sent, available_space);

		for (uint32_t i = 0; i < to_send; i++)
		{
			uart_write_byte_raw(id, data_ptr[bytes_sent + i]);
		}
	
		bytes_sent += to_send;
		UART_LOGV("uart_write_bytes id:%d bytes_sent:%d\r\n", id, bytes_sent);
	}
#else
#if (CONFIG_UART_TX_DMA)
	if(s_uart[id].tx_dma_enable) {
		//UART_LOGW("%s id:%d data:0x%x &data[0]:0x%x size:%d\r\n", __func__, id, data, &((uint8 *)data)[0], size);
		uart_tx_dma_write_to_fifo(id, (uint32_t)data, size);

	} else
#endif
	{
		for (int i = 0; i < size; i++) {
			uart_write_byte_raw(id, ((uint8 *)data)[i]);
		}
	}
#endif
	UART_FAST_TX_EXIT(id);
	return BK_OK;
}

bk_err_t bk_uart_read_bytes(uart_id_t id, void *data, uint32_t size, uint32_t timeout_ms)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_RETURN_ON_ID_NOT_INIT(id);
	UART_PM_CHECK_RESTORE(id);

	if (uart_id_is_sw_fifo_enabled(id)) {
		__attribute__((__unused__)) uart_statis_t* uart_statis = uart_statis_get_statis(id);
		GLOBAL_INT_DECLARATION();
		GLOBAL_INT_DISABLE();
		uint32_t kfifo_data_len = kfifo_data_size(s_uart_rx_kfifo[id]);
		/* Only kfifo_data_len=0, wait for semaphore */
		if (kfifo_data_len == 0) {
			UART_LOGV("kfifo is empty, wait for recv data\r\n");
			/* when sema_cnt=0, rx_blocked=true, otherwise rx_blocked=false */
			s_uart_sema[id].rx_blocked = true;
			GLOBAL_INT_RESTORE();
#if CONFIG_RTC_TIMER_PRECISION_TEST
			uint32_t ret = rtos_get_semaphore(&(s_uart_sema[id].rx_int_sema), BEKEN_WAIT_FOREVER);
#else
			uint32_t ret = rtos_get_semaphore(&(s_uart_sema[id].rx_int_sema), timeout_ms);
#endif
			if (ret == kTimeoutErr) {
				if (!s_uart_sema[id].rx_blocked) {
					rtos_get_semaphore(&(s_uart_sema[id].rx_int_sema), timeout_ms);
				}
				GLOBAL_INT_DISABLE();
				s_uart_sema[id].rx_blocked = false;
				GLOBAL_INT_RESTORE();
				UART_LOGW("recv data timeout:%d\n", timeout_ms);
				UART_STATIS_INC(uart_statis->recv_timeout_cnt);
				return BK_ERR_UART_RX_TIMEOUT;
			}
		} else {
			GLOBAL_INT_RESTORE();
		}

		// ========================================
		// Step 1: Read data
		// ========================================
		kfifo_data_len = kfifo_data_size(s_uart_rx_kfifo[id]);
		uint32_t read_len = min(size, kfifo_data_len);

#if CONFIG_UART_RX_DMA
		if (s_uart[id].rx_dma_enable) {
			read_len = kfifo_get_no_reset(s_uart_rx_kfifo[id], (uint8_t *)data, read_len);

			if (s_uart[id].rx_dma_rewind_when_fifo_empty && (kfifo_data_size(s_uart_rx_kfifo[id]) == 0)) {
				uart_rx_dma_rewind_when_fifo_empty(id);
			}
		} else
#endif
		{
			read_len = kfifo_get(s_uart_rx_kfifo[id], (uint8_t *)data, read_len);
		}

		
		// ========================================
		// Step 2: Check and resume DMA and UART RX
		// ========================================
	#if CONFIG_UART_RX_DMA
		if (s_uart[id].rx_dma_stopped || s_uart[id].rx_hw_stopped) 
		{
			// uint32_t data_size = kfifo_data_size(s_uart_rx_kfifo[id]);
			uint32_t unused = kfifo_unused(s_uart_rx_kfifo[id]);
			
			if (unused >= RESUME_THRESHOLD) 
			{
				GLOBAL_INT_DISABLE();

				// ========================================
				// Step 3: Resume DMA (if stopped)
				// ========================================
				if (s_uart[id].rx_dma_stopped && unused > 0) {
					uint32_t write_pos = s_uart_rx_kfifo[id]->in & s_uart_rx_kfifo[id]->mask;
					uint32_t space_to_end = s_uart_rx_kfifo[id]->size - write_pos;
					uint32_t dma_len = min(unused, space_to_end);
					uint32_t dma_addr = (uint32_t)s_uart_rx_kfifo[id]->buffer + write_pos;
					
					// Reconfigure and start DMA
					bk_dma_set_dest_start_addr(s_uart[id].rx_dma_id, dma_addr);
					bk_dma_set_transfer_len(s_uart[id].rx_dma_id, dma_len);
					bk_dma_start(s_uart[id].rx_dma_id);
					
					s_uart[id].rx_dma_stopped = false;
					s_uart[id].last_dma_len = dma_len;
					s_uart[id].rx_dma_accounted_len = 0;
					
					UART_LOGD("UART%d: DMA resumed - pos=%u, len=%u\n", 
							id, write_pos, dma_len);
				}
				
				// ========================================
				// Step 4: Resume UART RX (enable last)
				// ========================================
				if (s_uart[id].rx_hw_stopped) {
					// Re-enable RX interrupt
					bk_uart_enable_rx_interrupt(id);
					
					// Enable UART RX (start receiving new data)
					bk_uart_set_enable_rx(id, 1);
					s_uart[id].rx_hw_stopped = false;
					
					UART_LOGD("UART%d: UART RX resumed! Buffer unused=%d bytes\n", 
							id, unused);
					
				}
				
				GLOBAL_INT_RESTORE();
			} else {
				UART_LOGW("UART%d: DMA not stopped, Buffer unused=%d bytes\n", 
							id, unused);
			}
		}
	#endif

		return read_len;
	}else {
		int ret = 0;
		uint8_t rx_data;
		int read_count = 0;
		uint8_t *read_buffer = (uint8_t *)data;
	    int actual_bytes_to_read = size;

		/* read all data from rx-FIFO. */
	 	while (actual_bytes_to_read) {
			ret = uart_read_byte_ex(id, &rx_data);
			if (ret == -1)
				break;

			read_buffer[read_count] = rx_data;
			read_count++;

			actual_bytes_to_read--;
		}

		return read_count;
	}
}

bk_err_t bk_uart_set_rx_full_threshold(uart_id_t id, uint8_t threshold)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_rx_fifo_threshold(&s_uart[id].hal, id, threshold);
	return BK_OK;
}

bk_err_t bk_uart_set_tx_empty_threshold(uart_id_t id, uint8_t threshold)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_tx_fifo_threshold(&s_uart[id].hal, id, threshold);
	return BK_OK;
}

bk_err_t bk_uart_set_rx_timeout(uart_id_t id, uart_rx_stop_detect_time_t timeout_thresh)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_rx_stop_detect_time(&s_uart[id].hal, id, timeout_thresh);
	return BK_OK;
}

bk_err_t bk_uart_disable_rx(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_disable_rx(&s_uart[id].hal, id);
	uart_deinit_rx_gpio(id);

	return BK_OK;
}

bk_err_t bk_uart_disable_tx(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_disable_tx(&s_uart[id].hal, id);
	uart_deinit_tx_gpio(id);

	return BK_OK;
}

bk_err_t bk_uart_set_enable_rx(uart_id_t id, bool enable)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_rx_enable(&s_uart[id].hal, id, enable);

	return BK_OK;
}

bk_err_t bk_uart_set_enable_tx(uart_id_t id, bool enable)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_tx_enable(&s_uart[id].hal, id, enable);

	return BK_OK;
}

bk_err_t bk_uart_set_hw_flow_ctrl(uart_id_t id, uint8_t rx_threshold)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_set_hw_flow_ctrl(&s_uart[id].hal, id, rx_threshold);
	uart_hal_enable_flow_control(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_disable_hw_flow_ctrl(uart_id_t id)
{
	UART_RETURN_ON_NOT_INIT();
	UART_RETURN_ON_INVALID_ID(id);
	UART_PM_CHECK_RESTORE(id);
	uart_hal_disable_hw_flow_ctrl(&s_uart[id].hal, id);
	return BK_OK;
}

bk_err_t bk_uart_enable_sw_fifo(uart_id_t id)
{
	UART_RETURN_ON_INVALID_ID(id);
	s_uart[id].id_sw_fifo_enable_bits |= BIT(id);
	return BK_OK;
}

bk_err_t bk_uart_disable_sw_fifo(uart_id_t id)
{
	UART_RETURN_ON_INVALID_ID(id);
	s_uart[id].id_sw_fifo_enable_bits &= ~BIT(id);
	return BK_OK;
}

gpio_id_t bk_uart_get_rx_gpio(uart_id_t id)
{
	return uart_cfg_rx_pin(id);
}

bool bk_uart_is_tx_over(uart_id_t id)
{
	return uart_hal_is_tx_fifo_empty(&s_uart[id].hal, id);
}

void bk_uart_wait_tx_over(uart_id_t id)
{
	uint32_t uart_wait_us;
	uint32_t baudrate;
	uint32_t tx_fifo_count;
	uart_hw_t *hw = (uart_hw_t *)UART_LL_REG_BASE(id);
	extern void bk_delay_us(UINT32 us);

	tx_fifo_count = hw->fifo_status.tx_fifo_count + 1;
	baudrate = UART_CLOCK / (hw->config.clk_div + 1);
	uart_wait_us = 1000000 * tx_fifo_count * 10 / baudrate;

	bk_delay_us(uart_wait_us);
}

uint32_t uart_wait_tx_over(void)
{
	return uart_hal_wait_tx_over();
}

uint32_t uart_get_int_enable_status(uart_id_t id)
{
	UART_PM_CHECK_RESTORE(id);
	return uart_hal_get_int_enable_status(&s_uart[id].hal, id);
}

uint32_t uart_get_interrupt_status(uart_id_t id)
{
	return uart_hal_get_interrupt_status(&s_uart[id].hal, id);
}

void uart_clear_interrupt_status(uart_id_t id, uint32_t int_status)
{
	uart_hal_clear_interrupt_status(&s_uart[id].hal, id, int_status);
}

#if CONFIG_UART_RX_DMA
static void uart_rx_idle_isr(uart_id_t id)
{
	// ========================================
	// Step 1: Read DMA current progress
	// ========================================
	uint32_t dma_remain_len = bk_dma_get_remain_len(s_uart[id].rx_dma_id);
	uint32_t last_dma_len = s_uart[id].last_dma_len;
	uint32_t accounted_len = s_uart[id].rx_dma_accounted_len;
	
	// ========================================
	// Step 2: Calculate newly added data length (critical!)
	// ========================================
	// Current transferred = Total length - Remaining length
	uint32_t current_transferred = last_dma_len - dma_remain_len;
	
	// New data = Current transferred - Already accounted
	// This avoids duplicate counting
	uint32_t new_data_len = current_transferred - accounted_len;
	
	
	UART_LOGV("UART%d: Idle ISR - total=%u, remain=%u, accounted=%u, new=%u\n",
			id, last_dma_len, dma_remain_len, accounted_len, new_data_len);
	
	// ========================================
	// Step 3: Update in pointer (only update newly added part)
	// ========================================
	s_uart_rx_kfifo[id]->in += new_data_len;
	// ========================================
	// Step 4: Record accounted length (prevent DMA completion interrupt duplicate counting)
	// ========================================
	s_uart[id].rx_dma_accounted_len = current_transferred;
	
	// ========================================
	// Step 5: Wake up application thread
	// ========================================
	if (s_uart_sema[id].rx_int_sema && s_uart_sema[id].rx_blocked) 
	{
		rtos_set_semaphore(&(s_uart_sema[id].rx_int_sema));
		s_uart_sema[id].rx_blocked = false;
	}
	
	if (s_uart_rx_isr[id].callback)
	{
		s_uart_rx_isr[id].callback(id, s_uart_rx_isr[id].param);
	}
}
#endif

/* read int enable status
 * read int status
 * clear int status
 */
static void uart_isr_common(uart_id_t id)
{
	uint32_t int_status = 0;
	uint32_t int_enable_status = 0;
	uint32_t status = 0;
	UART_STATIS_DEC();

	int_status = uart_hal_get_interrupt_status(&s_uart[id].hal, id);
	int_enable_status = uart_hal_get_int_enable_status(&s_uart[id].hal, id);
	status = int_status & int_enable_status;
	uart_hal_clear_interrupt_status(&s_uart[id].hal, id, int_status);
	UART_STATIS_GET(uart_statis, id);
	UART_STATIS_INC(uart_statis->uart_isr_cnt);

	if (uart_hal_is_rx_interrupt_triggered(&s_uart[id].hal, id, status))	//rx end or rx fifo full
	{
		//overflow,rx_para_err,rx_stop_err
		if(int_status & (BIT(2) | BIT(3) | BIT(4)))
		{
#if (CONFIG_UART_RX_DMA)
			//TODO:Discard the RX FIFO data,set WR_PTR to RD_PTR
			if(s_uart[id].rx_dma_enable)
			{
#if CONFIG_UART_ERR_INTERRUPT
				uint32_t discard_len = s_uart_rx_kfifo[id]->out - s_uart_rx_kfifo[id]->in;
#endif
				s_uart_rx_kfifo[id]->in = s_uart_rx_kfifo[id]->out;
#if CONFIG_UART_ERR_INTERRUPT
				UART_LOGW("uart rx error(0x%x)!,discard_len=%d\r\n", (int_status & (BIT(2) | BIT(3) | BIT(4))), discard_len);
#endif
			}
			else
#endif
			{
				int ret = 0;
				uint8_t rx_data;

				/* read all data from rx-FIFO. */
				while (1)
				{
					ret = uart_read_byte_ex(id, &rx_data);
					if (ret == -1)
					{
						#if CONFIG_UART_ERR_INTERRUPT
						UART_LOGW("uart rx error(0x%x) triggered!\r\n", (int_status & (BIT(2) | BIT(3) | BIT(4))));
						#endif
						break;
					}
				}
			}
		}

		UART_STATIS_INC(uart_statis->rx_isr_cnt);
		UART_STATIS_SET(uart_statis->rx_fifo_cnt, uart_hal_get_rx_fifo_cnt(&s_uart[id].hal, id));
		if (uart_id_is_sw_fifo_enabled(id))
		{
#if CONFIG_UART_SW_FLOW_CTRL
			usfc_hw_fifo_will_full(id);
#endif
#if CONFIG_UART_RX_DMA
			if(s_uart[id].rx_dma_enable)
			{
					// ========================================
					// DMA mode
					// ========================================
				
				if (s_uart[id].rx_dma_stopped) {
					// ========================================
					// DMA stopped, RX interrupt should be disabled in theory
					// This branch should not be executed
					// ========================================
					UART_LOGW("UART%d: Unexpected RX INT while DMA stopped\n", id);
					
				} else {
					// ========================================
					// DMA running, only handle idle interrupt
					// ========================================
					// Idle interrupt: Process scattered data packets
					bk_dma_flush_src_buffer(s_uart[id].rx_dma_id);
					uart_rx_idle_isr(id);
				}
			}
			else
			{
#endif
				if (uart_id_read_fifo_frame(id, s_uart_rx_kfifo[id]) > 0)
				{
					if (s_uart_sema[id].rx_int_sema && s_uart_sema[id].rx_blocked)
					{
						rtos_set_semaphore(&(s_uart_sema[id].rx_int_sema));
						s_uart_sema[id].rx_blocked = false;
					}
				}

				if (s_uart_rx_isr[id].callback)
				{
					s_uart_rx_isr[id].callback(id, s_uart_rx_isr[id].param);
				}
#if CONFIG_UART_RX_DMA
			}
#endif
		}
		else if (s_uart_rx_isr[id].callback)
		{
			s_uart_rx_isr[id].callback(id, s_uart_rx_isr[id].param);
		}
		else
		{
			int ret = 0;
			uint8_t rx_data;

			/* read all data from rx-FIFO. */
			while (1)
			{
				ret = uart_read_byte_ex(id, &rx_data);
				if (ret == -1)
				{
					break;
				}
			}
		}
	}

	if (uart_hal_is_tx_interrupt_triggered(&s_uart[id].hal, id, status))
	{
		if (s_uart_tx_isr[id].callback)
		{
			s_uart_tx_isr[id].callback(id, s_uart_tx_isr[id].param);
		} else {
			uart_hal_disable_tx_interrupt(&s_uart[id].hal, id);
			uart_hal_clear_id_tx_interrupt_status(&s_uart[id].hal, id);
		}
	}
}

/* GCC 14+ requires compilation with general-regs-only for interrupt handlers
 * when FPU is enabled; the function attribute alone does not satisfy -Werror.
 */
#pragma GCC push_options
#pragma GCC target("general-regs-only")

void __BK_IRQ uart0_isr(void)
{
	uart_isr_common(UART_ID_0);
}

void __BK_IRQ uart1_isr(void)
{
	uart_isr_common(UART_ID_1);
}

void __BK_IRQ uart2_isr(void)
{
	uart_isr_common(UART_ID_2);
}

#if (SOC_UART_ID_NUM_PER_UNIT  >= 4)
void __BK_IRQ uart3_isr(void)
{
	uart_isr_common(UART_ID_3);
}
#endif

#if (SOC_UART_ID_NUM_PER_UNIT  >= 5)
void __BK_IRQ uart4_isr(void)
{
	uart_isr_common(UART_ID_4);
}
#endif

#if (SOC_UART_ID_NUM_PER_UNIT  >= 6)
void __BK_IRQ uart5_isr(void)
{
	uart_isr_common(UART_ID_5);
}
#endif

#pragma GCC pop_options
// eof

