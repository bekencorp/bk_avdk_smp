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
#include <sdkconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <common/bk_include.h>
#include "ram_regions.h"
#include "bk_private/components_init.h"
#include "bk_private/bk_driver.h"
#include "rtos_init.h"
#include <os/os.h>
#include "sys_driver.h"
#include "gpio_driver.h"
#if CONFIG_TASK_WDT || CONFIG_SUPPORT_WWDT
#include <bk_wdt.h>
#endif
#if CONFIG_SUPPORT_WWDT
#include "wwdt_driver.h"
#endif
#if (CONFIG_CPU_CNT > 1)
#include "mb_ipc_cmd.h"
#endif
#include "driver/pm_ap_core.h"
#include "bk_rtos_debug.h"
#include <driver/flash_partition.h>
#include <driver/flash.h>
#include <driver/wwdt.h>

#if CONFIG_FREERTOS_TRACE
#include "trcRecorder.h"
#endif

#include "stack_base.h"
#if CONFIG_GCOV
#include "gcov_public.h"
#endif

#include "soc/soc.h"
#include "soc_debug.h"

#if CONFIG_HSPL
extern bk_err_t bk_hspl_driver_early_init(void);
#endif

static beken_thread_function_t s_user_app_entry = NULL;
beken_semaphore_t user_app_sema = NULL;

#if CONFIG_DEBUG_AP_STARTUP
volatile uint32_t  g_ap_startup_flag;
#endif

void rtos_set_user_app_entry(beken_thread_function_t entry)
{
	s_user_app_entry = entry;
}

void rtos_user_app_preinit(void)
{
    int ret = rtos_init_semaphore(&user_app_sema, 1);
	if(ret < 0){
		BK_LOGD(NULL, "init queue failed");
	}
}

void rtos_user_app_launch_over(void)
{
	int ret;

	ret = rtos_set_semaphore(&user_app_sema);
	if(ret < 0){
		BK_LOGD(NULL, "set sema failed");
	}
}

void rtos_user_app_waiting_for_launch(void)
{
	int ret;

	ret = rtos_get_semaphore(&user_app_sema, BEKEN_WAIT_FOREVER);
	if(ret < 0){
		BK_LOGD(NULL, "get sema failed");
	}

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_APP_ENTRY_TIME);
#endif
}


__attribute__((weak)) void bk_module_init(void) {
	
}

#if (CONFIG_CPU_CNT > 1)
#define MAX_STOP_CPU1_NOTIFICATION_CNT (4)
static stop_cpu1_notification s_stop_cpu1_notifications_array[MAX_STOP_CPU1_NOTIFICATION_CNT];
static void *s_stop_cpu1_notifications_param_array[MAX_STOP_CPU1_NOTIFICATION_CNT];

void stop_cpu1_register_notification(stop_cpu1_notification notification, void *param)
{
	int i;

	for(i = 0; i < MAX_STOP_CPU1_NOTIFICATION_CNT; i++)
	{
		if ((s_stop_cpu1_notifications_array[i] == NULL))
		{
			s_stop_cpu1_notifications_array[i] = notification;
			s_stop_cpu1_notifications_param_array[i] = param;
			break;
		}
	}

	if(i >= MAX_STOP_CPU1_NOTIFICATION_CNT)
		BK_LOGD(NULL, "Err:%s:%p", __func__, notification);
}

void stop_cpu1_unregister_notification(stop_cpu1_notification notification)
{
	int i;
	for(i = 0; i < MAX_STOP_CPU1_NOTIFICATION_CNT; i++)
	{
		if ((s_stop_cpu1_notifications_array[i] == notification))
		{
			s_stop_cpu1_notifications_array[i] = NULL;
			s_stop_cpu1_notifications_param_array[i] = NULL;
			break;
		}
	}

	if(i >= MAX_STOP_CPU1_NOTIFICATION_CNT)
		BK_LOGD(NULL, "Err:%s:%p", __func__, notification);
}

void stop_cpu1_handle_notifications()		//CPU1 handle stop notications
{
	for(int i = 0; i < MAX_STOP_CPU1_NOTIFICATION_CNT; i++)
	{
		if (s_stop_cpu1_notifications_array[i])
		{
			s_stop_cpu1_notifications_array[i](s_stop_cpu1_notifications_param_array[i]);
		}
	}
}

static uint32 get_partition_addr(uint32 cpu_id)
{
	(void)cpu_id;

	bk_logic_partition_t *pt = NULL;
	bk_partition_t   part_id = -1;
	uint32    addr = -1;

	switch(cpu_id) {
		case 1:
		{
			part_id = BK_PARTITION_APPLICATION1;
			break;
		}
		default:
			return 0;
	}

	pt = bk_flash_partition_get_info(part_id);
	if((pt != NULL) && ((pt->partition_start_addr % 34) == 0))
	{
		addr = (pt->partition_start_addr / 34) * 32;   // CRC16 appended every 32 bytes in flash.  (32 bytes -> 34 bytes).
	}
	else
	{
		BK_LOGD(NULL, "slave core start addr not valid.\r\n");
	}

	return addr;
}

void reset_cpu1_core(uint32 offset, uint32_t start_flag)
{
	BK_LOGD(NULL, "reset_cpu1_core at: %08x, start=%d\r\n", offset, start_flag);

	sys_drv_set_cpu1_pwr_dw(0);
	sys_drv_set_cpu1_rxevt_sel(1); // for cpu0 + smp(cpu1,cpu2)
	sys_drv_set_cpu1_boot_address_offset(offset >> 8);
	sys_drv_set_cpu1_reset(start_flag);

}

extern void mb_ipc_reset_notify(u32 cpu_id, u32 power_on);

void start_cpu1_core(void)
{
#if CONFIG_SPE
	uint32  addr = get_partition_addr(1);
	reset_cpu1_core(SOC_FLASH_DATA_BASE + addr, 1);
#else
	/* Non-Secure SMP: secondary core enters the Secure boot shim in SRAM. */
	reset_cpu1_core(CONFIG_AP_SPE_RAM_ADDR, 1);
#endif

	mb_ipc_reset_notify(1, 1);
}

void stop_cpu1_core(void)
{
	reset_cpu1_core(0, 0);
	mb_ipc_reset_notify(1, 0);
}


void reset_cpu2_core(uint32 offset, uint32_t start_flag)
{
	BK_LOGD(NULL, "reset_cpu2_core at: %08x, start=%d\r\n", offset, start_flag);

	sys_drv_set_cpu2_pwr_dw(0);
	sys_drv_set_cpu2_rxevt_sel(1); // for cpu0 + smp(cpu1,cpu2)
	sys_drv_set_cpu2_boot_address_offset(offset >> 8);
	sys_drv_set_cpu2_reset(start_flag);

}

void start_cpu2_core(void)
{
#if (CONFIG_CPU_CNT > 2)
	uint32  addr = get_partition_addr(2);

	reset_cpu2_core(SOC_FLASH_DATA_BASE + addr, 1);
#endif
}

void stop_cpu2_core(void)
{
	reset_cpu2_core(0, 0);
}

#if (CONFIG_CPU_CNT > 2)
static uint32_t s_cpu2_users_id;
static beken_mutex_t s_mutex_cpu2_users;
bk_err_t management_cpu2_init(void)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if(s_mutex_cpu2_users == NULL) {
		bk_err_t error_state = rtos_init_mutex(&s_mutex_cpu2_users);
		if (error_state != BK_OK) {
			GLOBAL_INT_RESTORE();
			BK_ASSERT(0);
		}
	}
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

int32_t vote_start_cpu2_core(cpu2_user_id_t user_id)
{
	int32_t ret = user_id;
	management_cpu2_init();

	rtos_lock_mutex(&s_mutex_cpu2_users);
	if(s_cpu2_users_id == 0) {
		bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_ON);
		start_cpu2_core();
	}
	else
		ret = -1;
	s_cpu2_users_id |= (0x1 << user_id);
	rtos_unlock_mutex(&s_mutex_cpu2_users);

	return ret;
}

int32_t vote_stop_cpu2_core(cpu2_user_id_t user_id)
{
	int32_t ret = user_id;
	management_cpu2_init();

	rtos_lock_mutex(&s_mutex_cpu2_users);
	s_cpu2_users_id &= ~(0x1 << user_id);
	if(s_cpu2_users_id == 0) {
		stop_cpu2_core();
		bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_OFF);
	}
	else
		ret = -1;
	rtos_unlock_mutex(&s_mutex_cpu2_users);

	return ret;
}
#endif	// (CONFIG_CPU_CNT > 2)

#endif // (CONFIG_CPU_CNT > 1)


/* Enter SWD debug mode from the AP side.
 *
 * The SWD pads and the debug-port routing are owned by the CP, so the AP only
 * stops its own watchdogs (so a halted AP core cannot trip them) and then asks
 * the CP, over the mailbox IPC, to switch the shared port/pins to SWD. The
 * watchdog stops are idempotent register/flag writes, safe before driver init. */
void bk_set_swd_mode(void) {
#if CONFIG_DEBUG_VERSION || CONFIG_SWD_DEBUG_MODE

	/* Stop the AP-local watchdogs. */
#if CONFIG_SUPPORT_WWDT
	bk_wwdt_close();
#endif
#if CONFIG_TASK_WDT
	bk_task_wdt_stop();
#endif

	/* Ask the CP to enter SWD mode (route the debug port + map GPIO20/21 to
	 * SWCLK/SWDIO + stop the CP watchdogs). */
#if (CONFIG_CPU_CNT > 1)
	(void)ipc_send_set_swd_mode();
#endif
#endif
}

/* Legacy API, kept for source compatibility with the old
 * bk_set_jtag_mode(cpu_id, group_id); both arguments are now ignored because the
 * SWD switch is delegated to the CP via bk_set_swd_mode(). */
void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id) {
	(void)cpu_id;
	(void)group_id;
	bk_set_swd_mode();
}

static void user_app_thread( void *arg )
{
	rtos_user_app_waiting_for_launch();
	/* add your user_main*/
	BK_LOGD(NULL, "user app entry(0x%0x)\r\n", s_user_app_entry);
	if(NULL != s_user_app_entry) {
		s_user_app_entry(0);
	}

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_APP_FINISH_TIME);
#endif

	rtos_deinit_semaphore(&user_app_sema);

	rtos_delete_thread( NULL );
}

static void start_user_app_thread(void)
{
	BK_LOGD(NULL, "start user app thread.\r\n");
	rtos_create_sram_thread(NULL,
					BEKEN_APPLICATION_PRIORITY,
					"app",
					(beken_thread_function_t)user_app_thread,
					CONFIG_APP_MAIN_TASK_STACK_SIZE,
					(beken_thread_arg_t)0);
}

#if CONFIG_SUPPORT_MATTER
beken_thread_t matter_thread_handle = NULL;
extern void ChipTest(void);
static void matter_thread( void *arg ) {
#ifdef CONFIG_MATTER_EXAMPLE
    if (CONFIG_MATTER_EXAMPLE[0] != '\0')
	ChipTest();
#endif
    rtos_delete_thread(NULL);
}

void start_matter(void) {
    BK_LOGD(NULL, "start matter\r\n");
    rtos_create_thread(&matter_thread_handle,
        BEKEN_DEFAULT_WORKER_PRIORITY,
         "matter",
        (beken_thread_function_t)matter_thread,
        8192,
        0);
}
#endif // CONFIG_SUPPORT_MATTER

extern int main(void);
extern bool ate_is_enabled(void);
extern void rtos_init_base_time(void);

static void app_main_thread(void *arg)
{
    set_ap_startup_index(AP_ENTER_APP_MAIN_THREAD);
#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_MAIN_ENTRY_TIME);
#endif

#if CONFIG_CP_HANG_DUMP_BY_AP
	extern bk_err_t bk_cp_hang_dump_by_ap_init(void);
	bk_cp_hang_dump_by_ap_init();
#endif

#ifdef RTOS_FUNC_TEST
	/*rtos thread func test, for bk7256 bringup.*/
	rtos_thread_func_test();
	//if nessary ,close the main() function.
#endif

	bk_pm_ap_thread_main();

	main();
	BK_LOGI(NULL, "AP main running...\r\n");

#if CONFIG_MATTER_START && CONFIG_SUPPORT_MATTER
#ifdef CONFIG_MATTER_EXAMPLE
	if (CONFIG_MATTER_EXAMPLE[0] != '\0')
	    start_matter();
#endif
#endif // CONFIG_MATTER_START && CONFIG_SUPPORT_MATTER
    // if(ate_is_enabled())
    // {
    //     BK_LOGD(NULL, "ATE enabled = 1\r\n");
    // }

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_MIAN_FINISH_TIME);
#endif
    set_ap_startup_index(AP_EXIT_APP_MAIN_THREAD);
	rtos_delete_thread(NULL);
}

void start_app_main_thread(void)
{
	rtos_create_sram_thread(NULL, CONFIG_APP_MAIN_TASK_PRIO,
		"main",
		(beken_thread_function_t)app_main_thread,
		CONFIG_APP_MAIN_TASK_STACK_SIZE,
		(beken_thread_arg_t)0);
}

void entry_main(void)
{
#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_MAIN_ENTRY_TIME);
#endif
    set_ap_startup_index(AP_ENTER_ENTRY_MAIN);
#if CONFIG_HSPL
	bk_hspl_driver_early_init();
#endif
	rtos_init();

#if CONFIG_GCOV
	__gcov_call_constructors();
#endif

#if (CONFIG_ATE_TEST)
	bk_set_printf_enable(0);
#endif

	if(components_early_init())
		return;

#if (CONFIG_FREERTOS_TRACE)
	xTraceEnable(TRC_START);
	uint32_t trace_addr = (uint32_t)xTraceGetTraceBuffer();
	uint32_t trace_size = uiTraceGetTraceBufferSize();

	rtos_regist_plat_dump_hook(trace_addr, trace_size);
#endif

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_INIT_DRIVER_TIME);
#endif

    bk_module_init();

	start_app_main_thread();
	// start_user_app_thread();

	rtos_init_base_time();

#if CONFIG_SAVE_BOOT_TIME_POINT
	save_mtime_point(CPU_START_SCHE_TIME);
#endif

    set_ap_startup_index(AP_ENTER_RTOS_START_SCHEDULER);
#if CONFIG_SUPPORT_WWDT
	BK_LOG_ON_ERR(bk_wwdt_start(CONFIG_INT_WWDT_PERIOD_MS, false, 0));
	BK_LOGD(NULL, "boot core wwdt enabled, period=%u\r\n", CONFIG_INT_WWDT_PERIOD_MS);
#endif
	rtos_start_scheduler();
}
// eof

