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
#include <components/log.h>
#include <common/bk_err.h>
#include <components/system.h>
#include <driver/timer.h>
#include <driver/wdt.h>
#include "bk_misc.h"
#include "reset_reason.h"
#include "drv_model_pub.h"
#include "bk_wifi_types.h"
#include "bk_wifi.h"
#include "aon_pmu_driver.h"
#include "wdt_driver.h"
#include "driver/flash.h"
#include <modules/pm.h>

#if CONFIG_DUMP_BY_LOG_UART
extern void bk_coredump_writer_init(void);
extern void bk_coredump_write(const char *format, ...);
#define bk_reboot_writer_init() bk_coredump_writer_init()
#define bk_reboot_write(format, ...) bk_coredump_write(format, ##__VA_ARGS__)
#else
#define TAG "sys"

#define bk_reboot_writer_init()
#define bk_reboot_write(format, ...) BK_LOGD(TAG, format, ##__VA_ARGS__)
#endif

void bk_reboot_ex(uint32_t reset_reason)
{
	rtos_disable_int();
	bk_wdt_force_feed();
#if ((CONFIG_INT_WDT) || (CONFIG_TASK_WDT))
	/* close wdt timer to avoid wdt reset on CP0 during reboot */
	bk_timer_stop(TIMER_ID2);
#endif

	if (reset_reason < RESET_SOURCE_UNKNOWN) {
		bk_misc_set_cp_reset_reason(reset_reason);
		bk_misc_set_ap_reset_reason(reset_reason);
	}

	bk_reboot_writer_init();
	bk_reboot_write("bk_reboot\r\n");
	delay_ms(100); //add delay for bk_writer BEKEN_DO_REBOOT cmd
	// bk_pm_module_vote_cpu_freq(PM_DEV_ID_DEFAULT,PM_CPU_FRQ_60M);

	bk_reboot_write("wdt reboot\r\n");

	//fix reboot hang 16s issue
	bk_flash_power_saving_enter();
#if CONFIG_AON_WDT
	bk_wdt_force_reboot();
#endif

	while(1);
}

void bk_reboot(void)
{
	bk_reboot_ex(RESET_SOURCE_REBOOT);
}
