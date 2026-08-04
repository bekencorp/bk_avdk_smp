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

// Notes:
// This file only contain OS-independent components initialization code,
// For OS-dependently initialization, put them to OSK or bk_system.

#include <common/bk_include.h>
#include "bk_sys_ctrl.h"
#include "stdlib.h"
#include <components/ate.h>
#include <common/sys_config.h>
#include "sys_driver.h"
#include "bk_pm_model.h"
#include "bk_private/bk_init.h"
#include "bk_rtos_debug.h"

#if CONFIG_FULLY_HOSTED
#include "mmgmt.h"
#endif

#if CONFIG_TASK_WDT
#include "bk_wdt.h"
#endif

#include <components/bk_platform.h>
#include "reset_reason.h"
#include <driver/pwr_clk.h>

#if CONFIG_EASY_FLASH
#include "easyflash.h"
#include "bk_ef.h"
#endif

#include <components/log.h>
#include <components/sensor.h>

#include "bk_arch.h"
#include "bk_private/bk_driver.h"

#include "mb_ipc_cmd.h"
#include "interrupt_base.h"
#include "soc_debug.h"

#if (CONFIG_PSRAM)
#include <driver/psram.h>
#endif
#if (CONFIG_OTP_V1)
#include <driver/otp.h>
#endif
#if (CONFIG_SUPPORT_WWDT)
#include <driver/wwdt.h>
#endif

#define TAG "init"


//TODO move to better place
#if CONFIG_MEM_DEBUG
/* memory leak timer at 1sec */
static beken_timer_t memleak_timer = {0};
int bmsg_memleak_check_sender();

void memleak_timer_cb(void *arg)
{
	bmsg_memleak_check_sender();
}

void mem_debug_start_timer(void)
{
	bk_err_t err;

	err = rtos_init_timer(&memleak_timer,
						  1000,
						  memleak_timer_cb,
						  (void *)0);
	if(kNoErr != err)
	{
		BK_LOGE(TAG, "rtos_init_timer fail\r\n");
		return;
	}
	err = rtos_start_timer(&memleak_timer);
	if(kNoErr != err)
	{
		BK_LOGE(TAG, "rtos_start_timer fail\r\n");
		return;
	}
}
#endif

int bandgap_init(void)
{
	return BK_OK;
}

int random_init(void)
{
	BK_LOGV(TAG, "create srand seed\r\n");
	srand((unsigned)rand());
	return BK_OK;
}

__IRAM_SEC int wdt_init(void)
{
#if CONFIG_TASK_WDT
	bk_task_wdt_start();
	BK_LOGV(TAG, "task watchdog enabled, period=%u\r\n", CONFIG_TASK_WDT_PERIOD_MS);
#endif

	return BK_OK;
}

int memory_debug_todo(void)
{
#if CONFIG_MEM_DEBUG
	mem_debug_start_timer();
#endif
	return BK_OK;
}

__attribute__((unused)) static int pm_init(void)
{
#if CONFIG_DEEP_PS
	bk_init_deep_wakeup_gpio_status();
#endif

	return BK_OK;
}

static inline void show_sdk_version(void)
{
#if (CONFIG_CMAKE)
	//BK_LOGD(TAG, "armino rev: %s\r\n", ARMINO_VER);
	BK_LOGD(TAG, "armino rev: %s\r\n", "");
#else
	//BK_LOGD(TAG, "armino rev: %s\r\n", BEKEN_SDK_REV);
	BK_LOGD(TAG, "armino rev: %s\r\n", "");
#endif
}

static inline void show_chip_id(void)
{
	// BK_LOGD(TAG, "armino soc id:%x_%x\r\n",sddev_control(DD_DEV_TYPE_SCTRL,CMD_GET_DEVICE_ID, NULL),
	// 	sddev_control(DD_DEV_TYPE_SCTRL,CMD_GET_CHIP_ID, NULL));
	BK_LOGD(TAG, "armino soc id:%x_%x\r\n", sys_drv_get_device_id(), sys_drv_get_chip_id());
}

static inline void show_sdk_lib_version(void)
{

}

static void show_armino_version(void)
{
	show_sdk_version();
	show_chip_id();
}

static void show_init_info(void)
{
	show_reset_reason();
	show_armino_version();
}

void *__stack_chk_guard = NULL;

// Intialize random stack guard
void bk_stack_guard_setup(void)
{
	BK_LOGD(TAG, "Intialize random stack guard.\r\n");
	__stack_chk_guard = (void *)(uintptr_t)(unsigned)rand();
}

#if CONFIG_UT_REG
static void ut_reg_init(void)
{
	extern void (*__ut_reg_start)(void);
	extern void (*__ut_reg_end)(void);

	void (*p)(void);

	for (p = (void*)((uint32_t)&__ut_reg_start + 1); (uint32_t)p < (uint32_t)&__ut_reg_end; p += 12) {
		BK_LOGV(TAG, "calling init function: %p\r\n", p);
		p();
	}
}
#endif

void __stack_chk_fail (void)
{
    BK_DUMP_OUT("Stack guard warning, local buffer overflow!!!\r\n");
    BK_ASSERT(0);
}


int components_early_init(void)
{
    set_ap_startup_index(AP_ENTER_COMPONTENT_EARLY_INIT);
	interrupt_init();
#if CONFIG_RESET_REASON
	reset_reason_init();
#endif
	app_phy_init();

	if(driver_early_init())
		return BK_FAIL;

	pm_init();

	bandgap_init();
	random_init();

	bk_stack_guard_setup();
    set_ap_startup_index(AP_EXIT_COMPONTENT_EARLY_INIT);
	return BK_OK;
}

__attribute__((weak)) void bk_module_init(void) {

}

// Run in task environment
int components_init(void)
{
	if(driver_init())
		return BK_FAIL;

	if(wdt_init())
		return BK_FAIL;

#if CONFIG_UT_REG
	ut_reg_init();
#endif
	bk_module_init();

	show_init_info();

	return BK_OK;
}
