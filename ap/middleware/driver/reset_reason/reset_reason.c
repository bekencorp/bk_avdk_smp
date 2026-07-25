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

#include <common/bk_include.h>
#include "bk_icu.h"
#include "bk_uart.h"

#include "bk_arm_arch.h"
#include "arch_interrupt.h"
#include "bk_sys_ctrl.h"

#include "reset_reason.h"
#include <components/log.h>
#include "aon_pmu_hal.h"
#include "reset_reason_hal.h"
#include "driver/gpio.h"
#include "driver/pwr_clk.h"
#include "sys_sw_regs.h"
#define TAG "init"
#define DISPLAY_START_TYPE_STR 1


static volatile bool s_initialized = false;
static uint32_t s_start_type = 0;
static uint32_t s_misc_value_save = 0;
static uint32_t s_mem_value_save = 0;

uint32_t bk_misc_get_reset_reason(void)
{
	return s_start_type;
}

void persist_memory_init(void)
{
	rr_hal_set_persist_mem_val((uint32_t)CRASH_ILLEGAL_JUMP_VALUE);
}

uint32_t persist_memory_get(void)
{
	return rr_hal_get_persist_mem_val();
}


bool persist_memory_is_lost(void)
{
	if ((uint32_t)CRASH_ILLEGAL_JUMP_VALUE == persist_memory_get())
		return false;
	else
		return true;
}

static char *misc_get_start_type_str(uint32_t start_type)
{
#if DISPLAY_START_TYPE_STR
	switch (start_type) {
	case RESET_SOURCE_POWERON:
		return "power on";

	case RESET_SOURCE_REBOOT:
		return "software reboot";

	case RESET_SOURCE_WATCHDOG:
		return "interrupt watchdog";

	case RESET_SOURCE_DEEPPS_GPIO:
		return "deep sleep gpio";

	case RESET_SOURCE_DEEPPS_RTC:
		return "deep sleep rtc";

	case RESET_SOURCE_DEEPPS_TOUCH:
		return "deep sleep touch";

	case RESET_SOURCE_CRASH_ILLEGAL_JUMP:
		return "illegal jump";

	case RESET_SOURCE_CRASH_UNDEFINED:
		return "undefined";

	case RESET_SOURCE_CRASH_PREFETCH_ABORT:
		return "prefetch abort";

	case RESET_SOURCE_CRASH_DATA_ABORT:
		return "data abort";

	case RESET_SOURCE_CRASH_UNUSED:
		return "unused";

	case RESET_SOURCE_CRASH_ILLEGAL_INSTRUCTION:
		return "illegal instruction";

	case RESET_SOURCE_CRASH_MISALIGNED:
		return "misaligned";

	case RESET_SOURCE_CRASH_ASSERT:
		return "assert";

	case RESET_SOURCE_DEEPPS_USB:
		return "deep sleep usb";

	case RESET_SOURCE_SUPER_DEEP:
		return "super deep sleep";

	case RESET_SOURCE_NMI_WDT:
		return "nmi watchdog";

	case RESET_SOURCE_HARD_FAULT:
		return "hard fault";

	case RESET_SOURCE_MPU_FAULT:
		return "mpu fault";

	case RESET_SOURCE_BUS_FAULT:
		return "bus fault";

	case RESET_SOURCE_USAGE_FAULT:
		return "usage fault";

	case RESET_SOURCE_SECURE_FAULT:
		return "secure fault";

	case RESET_SOURCE_DEFAULT_EXCEPTION:
		return "default exception";

	case RESET_SOURCE_OTA_REBOOT:
		return "ota reboot";

	case RESET_SOURCE_FORCE_DEEPSLEEP:
		return "enter deep sleep";

	case RESET_SOURCE_UNKNOWN:
	default:
		return "unknown";
	}
#else
	return "";
#endif
}

void show_reset_reason(void)
{
	BK_LOGD(TAG, "reason - %s\r\n", misc_get_start_type_str(s_start_type));
	if(RESET_SOURCE_DEEPPS_GPIO == s_start_type)
	{
#if CONFIG_DEEP_PS
		BK_LOGD(TAG, "by gpio - %d\r\n", bk_misc_wakeup_get_gpio_num());
#else
#ifdef CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
		BK_LOGD(TAG, "by gpio - %d\r\n", bk_gpio_get_wakeup_gpio_id());
#endif
#endif
	}
	BK_LOGD(TAG, "regs - %x, %x, %x\r\n", s_start_type, s_misc_value_save, s_mem_value_save);
}

// typedef volatile union {
// 	struct {
// 		uint32_t memchk_bps               :  1; /**<bit[0 : 0] */
// 		uint32_t fast_boot                :  1; /**<bit[1 : 1] */
// 		uint32_t ota_finish               :  1; /**<bit[2 : 2] */
// 		uint32_t bl2_deep_sleep           :  1; /**<bit[3 : 3] */
// 		uint32_t dlv_startup              :  1; /**<bit[4 : 4] */
// 		uint32_t reset_reason_cp          :  7; /**<bit[5 : 11] */
// 		uint32_t gpio_retention_bitmap    :  8; /**<bit[12 : 19] */
// 		uint32_t reset_count              :  4 ;/**<bit[20 : 23] */
// 		uint32_t reset_reason_ap          :  7; /**<bit[24 : 30] */
// 		uint32_t gpio_sleep               :  1; /**<bit[31 : 31] */
// 	};
// 	uint32_t v;
// } aon_pmu_r0_t;

void bk_misc_set_cp_reset_reason(uint32_t type)
{
	if (type > 0x7f) {
		BK_LOGE(TAG, "Invalid cp rr type: 0x%x", type);
		return;
	}

	/* use PMU_REG0 bit[5:11] for cp reset reason */
	uint32_t misc_value = aon_pmu_hal_get_r0();

	/* clear last reset reason */
	misc_value &= ~(0x7f << 5);

	misc_value |= ((type & 0x7f) << 5);
	aon_pmu_hal_set_r0(misc_value);
}

void bk_misc_set_ap_reset_reason(uint32_t type)
{
	if (type > 0x7f) {
		BK_DUMP_OUT("Invalid ap rr type: 0x%x\r\n", type);
		return;
	}

	/* use PMU_REG0 bit[24:30] for reset reason */
	uint32_t misc_value = aon_pmu_hal_get_r0();

	/* clear last reset reason */
	misc_value &= ~(0x7f << 24);

	misc_value |= ((type & 0x7f) << 24);
	aon_pmu_hal_set_r0(misc_value);
}

void bk_misc_set_reset_reason(uint32_t type)
{
	bk_misc_set_ap_reset_reason(type);
}


uint32_t reset_reason_init(void)
{
	uint32_t cp_reset_reason = 0;
	uint32_t ap_reset_reason = 0;

	if (s_initialized != 0) {
		return s_start_type;
	}

	cp_reset_reason = bk_sys_sw_regs_get_cp_reset_reason();
	ap_reset_reason = bk_sys_sw_regs_get_ap_reset_reason();

	s_start_type = ap_reset_reason;
	s_misc_value_save = cp_reset_reason;

	s_initialized = true;
	return s_start_type;
}