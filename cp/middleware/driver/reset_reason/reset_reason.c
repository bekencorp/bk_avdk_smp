// Copyright 2025-2026 Beken
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
#include "bk_sys_ctrl.h"
#include <modules/pm.h>
#include "reset_reason.h"
#include <components/log.h>
#include "aon_pmu_hal.h"
#include "reset_reason_hal.h"
#include "driver/gpio.h"
#include "driver/pwr_clk.h"
#include <sys_sw_regs.h>

#define TAG "init"
#define DISPLAY_START_TYPE_STR 1

#define EXCEPTION_REBOOT_INFO_MAGIC   0x45585231U
#define EXCEPTION_REBOOT_INFO_VERSION 1U

static volatile bool s_initialized = false;
static uint32_t s_start_type = 0;
static uint32_t s_misc_value_save = 0;
static uint32_t s_mem_value_save = 0;

typedef struct {
	uint32_t magic;
	uint32_t version;
	bk_exception_reboot_info_t info;
	uint32_t checksum;
} exception_reboot_record_t;

/* Survives a warm reset (NOLOAD, never zero-initialised at boot), so the
 * primary exception context stays readable even when the secondary watchdog
 * reset cuts the dump short. */
static volatile exception_reboot_record_t s_exception_reboot_record
	__attribute__((section(".noinit.exception_reboot_info"), aligned(32)));
static bk_exception_reboot_info_t s_exception_reboot_info_save;
static bool s_exception_reboot_info_valid;

/* Same storage, but owned on behalf of the AP domain. The AP cannot keep its own
 * record: the CP reloads AP SRAM when it restarts the AP, so whatever the AP
 * wrote there is already gone by the time the next boot could report it. The CP
 * publishes this slot's address through sys_sw_regs, the AP writes into it
 * directly, and the CP reports it below over the synchronous UART path that is
 * known to be up this early. */
static volatile exception_reboot_record_t s_ap_exception_reboot_record
	__attribute__((section(".noinit.exception_reboot_info"), aligned(32)));
static bk_exception_reboot_info_t s_ap_exception_reboot_info_save;
static bool s_ap_exception_reboot_info_valid;

static uint32_t exception_reboot_info_checksum(
	const bk_exception_reboot_info_t *info)
{
	return EXCEPTION_REBOOT_INFO_MAGIC ^ EXCEPTION_REBOOT_INFO_VERSION ^
		info->primary_reason ^ info->secondary_reason ^
		info->primary_core ^ info->secondary_core ^
		info->pc ^ info->lr ^ info->sp ^ info->cfsr ^ info->hfsr;
}

/* SRAM is never mapped into the d-cache, so a barrier is all that is needed to
 * order the magic/content/magic store sequence below; no cache maintenance. */
#define exception_reboot_record_commit() __asm volatile ("dsb" ::: "memory")

void bk_misc_persist_exception_reboot_info(
	const bk_exception_reboot_info_t *info)
{
	/* Invalidate first, so a reset landing mid-update is detected as invalid
	 * instead of reporting a half-written record. */
	s_exception_reboot_record.magic = 0U;
	exception_reboot_record_commit();

	s_exception_reboot_record.version = EXCEPTION_REBOOT_INFO_VERSION;
	s_exception_reboot_record.info = *info;
	s_exception_reboot_record.checksum =
		exception_reboot_info_checksum(info);
	exception_reboot_record_commit();

	s_exception_reboot_record.magic = EXCEPTION_REBOOT_INFO_MAGIC;
	exception_reboot_record_commit();
}

/* Reads one slot and always clears it, so the slot is free for the domain that
 * owns it to write a fresh record immediately after init. */
static bool exception_reboot_record_take(
	volatile exception_reboot_record_t *record,
	bk_exception_reboot_info_t *out)
{
	bk_exception_reboot_info_t info = record->info;
	uint32_t checksum = exception_reboot_info_checksum(&info);
	bool valid = (record->magic == EXCEPTION_REBOOT_INFO_MAGIC) &&
		(record->version == EXCEPTION_REBOOT_INFO_VERSION) &&
		(record->checksum == checksum);

	if (valid) {
		*out = info;
	}

	record->magic = 0U;
	exception_reboot_record_commit();

	return valid;
}

static void exception_reboot_info_restore(void)
{
	s_exception_reboot_info_valid = exception_reboot_record_take(
		&s_exception_reboot_record, &s_exception_reboot_info_save);
	s_ap_exception_reboot_info_valid = exception_reboot_record_take(
		&s_ap_exception_reboot_record, &s_ap_exception_reboot_info_save);
}

/* domain_tag is empty for the CP's own record and ",ap" for the AP slot, so the
 * CP line keeps the exact wording that existing logs already match. */
static void exception_reboot_info_report(
	const char *domain_tag, const bk_exception_reboot_info_t *info)
{
	BK_DUMP_OUT(
		"@PRIMARY_EXCEPTION(prev boot%s) reason=0x%x core=%u pc=0x%08x lr=0x%08x sp=0x%08x CFSR=0x%08x HFSR=0x%08x\r\n",
		domain_tag, info->primary_reason, info->primary_core, info->pc,
		info->lr, info->sp, info->cfsr, info->hfsr);
	BK_DUMP_OUT("@SECONDARY_EXCEPTION(prev boot%s) reason=0x%x core=%u\r\n",
		domain_tag, info->secondary_reason, info->secondary_core);
}

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

void reboot_tag_set(void)
{
	rr_hal_set_reboot_tag_val(REBOOT_TAG_REQ);
}

uint32_t reboot_tag_is_reboot(void)
{
	return (REBOOT_TAG_REQ == rr_hal_get_reboot_tag_val());
}

void reboot_tag_init(void)
{
	rr_hal_set_reboot_tag_val(0);
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

	if((s_start_type == RESET_SOURCE_POWERON)
	|| (s_start_type == RESET_SOURCE_REBOOT)
	|| (s_start_type == RESET_SOURCE_WATCHDOG)
	|| (s_start_type == RESET_SOURCE_DEEPPS_GPIO)
	|| (s_start_type == RESET_SOURCE_DEEPPS_RTC)
	|| (s_start_type == RESET_SOURCE_DEEPPS_TOUCH)
	|| (s_start_type == RESET_SOURCE_DEEPPS_VAD))
	{
		bk_pm_ap_first_boot_set(true);
	}

	BK_LOGD(TAG, "regs - %x, %x, %x\r\n", s_start_type, s_misc_value_save, s_mem_value_save);

	/* Report the exception context recorded before the previous reset. Uses the
	 * synchronous dump path (not BK_LOGx) so a reduced log level cannot drop the
	 * only surviving evidence of a dump that was cut short. The record itself was
	 * already cleared by exception_reboot_record_take(); clearing the valid flag
	 * here stops a later "starttype" CLI call from re-reporting it. */
	if (s_exception_reboot_info_valid) {
		exception_reboot_info_report("", &s_exception_reboot_info_save);
		s_exception_reboot_info_valid = false;
	}
	if (s_ap_exception_reboot_info_valid) {
		exception_reboot_info_report(",ap", &s_ap_exception_reboot_info_save);
		s_ap_exception_reboot_info_valid = false;
	}
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
		BK_DUMP_OUT("Invalid cp rr type: 0x%x", type);
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
		BK_LOGE(TAG, "Invalid ap rr type: 0x%x\r\n", type);
		return;
	}

	/* use PMU_REG0 bit[24:30] for reset reason */
	uint32_t misc_value = aon_pmu_hal_get_r0();

	BK_LOGI(TAG, "set ap rr: 0x%x\r\n", type);
	/* clear last reset reason */
	misc_value &= ~(0x7f << 24);

	misc_value |= ((type & 0x7f) << 24);
	aon_pmu_hal_set_r0(misc_value);
}

void bk_misc_set_reset_reason(uint32_t type)
{
	bk_misc_set_cp_reset_reason(type);
}


uint32_t reset_reason_deep_sleep_check(void)
{
	uint32_t misc_value = 0;

	if(s_start_type != RESET_SOURCE_SUPER_DEEP)
		return misc_value;

	misc_value = aon_pmu_hal_get_wakeup_source();
	switch (misc_value)
	{
	case 0x1: // gpio
		misc_value = RESET_SOURCE_DEEPPS_GPIO;
		break;
	case 0x2: // rtc
		misc_value = RESET_SOURCE_DEEPPS_RTC;
		break;
	case 0x10: // usbplug
		misc_value = RESET_SOURCE_DEEPPS_USB;
		break;
	case 0x20: // touch
		misc_value = RESET_SOURCE_DEEPPS_TOUCH;
		break;
	case 0x40: // vad
		misc_value = RESET_SOURCE_DEEPPS_VAD;
		break;
	default:
		misc_value = 0;
		break;
	}

	return misc_value;
}

uint32_t reset_reason_init(void)
{
	uint32_t misc_value;
	uint32_t cp_reset_reason = 0;
	uint32_t ap_reset_reason = 0;

	if (s_initialized != 0) {
		return s_start_type;
	}

	exception_reboot_info_restore();
	/* Publish before the CP starts the AP, so the AP always has somewhere to
	 * persist its exception context. */
	bk_sys_sw_regs_set_ap_exception_record_ptr(
		(uint32_t)(uintptr_t)&s_ap_exception_reboot_record);
	misc_value = aon_pmu_hal_get_reset_reason();
	cp_reset_reason = ((misc_value >> 5) & 0x7f);
	ap_reset_reason = ((misc_value >> 24) & 0x7f);
	
	s_start_type = cp_reset_reason;
	s_misc_value_save = misc_value;


	bk_sys_sw_regs_set_cp_reset_reason(cp_reset_reason);
	bk_sys_sw_regs_set_ap_reset_reason(ap_reset_reason);

	bk_misc_set_cp_reset_reason(RESET_SOURCE_POWERON);
	bk_misc_set_ap_reset_reason(RESET_SOURCE_POWERON);

	s_initialized = true;
	return s_start_type;
}
