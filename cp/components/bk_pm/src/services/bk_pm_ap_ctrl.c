// Copyright 2021-2025 Beken
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
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <driver/mailbox_channel.h>
#include <driver/aon_rtc.h>
#include "sys_driver.h"
#include <os/mem.h>
#include <sys_sw_regs.h>
#include "cache.h"
#include "pm_debug.h"

extern void mb_ipc_reset_notify(u32 cpu_id, u32 power_on);

typedef struct ap_ctrl_callback_node {
	ap_ctrl_callback_t callback;
	void *arg;
	pm_ap_ctrl_cb_type_t type;
	struct ap_ctrl_callback_node *next;
} ap_ctrl_callback_node_t;


#define PM_SEND_CMD_CP1_RESPONSE_TIEM        (100)  //100ms
#define PM_WAIT_AP_SLEEP_TIMEOUT_MS          (3000)
#define PM_CP1_RECOVERY_DEFAULT_VALUE        (0xFFFFFFFFFFFFFFFFULL)


/*=====================VARIABLE  SECTION  START=================*/
#if (CONFIG_CPU_CNT > 1)
static uint32_t s_pm_cp1_ctrl_state                                              = 0;
static volatile  uint32_t                         s_pm_cp1_closing               = 0;
static volatile  uint32_t                         s_pm_cp1_sema_count            = 0;
static volatile  uint32_t                         s_pm_cp1_boot_try_count        = 0;
static volatile  uint32_t                         s_pm_cp1_psram_malloc_count    = 0;
static volatile  uint64_t                         s_pm_cp1_module_recovery_state = PM_CP1_RECOVERY_DEFAULT_VALUE;
static beken_mutex_t                              s_pm_cp1_vote_mutex            = NULL;
#endif

static ap_ctrl_callback_node_t *s_ap_ctrl_callback_head                          = NULL;

/*=====================VARIABLE  SECTION  END=================*/



/*
 * Example:
 * static void ap_poweroff_notify_cb(void *arg)
 * {
 *     uint32_t module_id = (uint32_t)(uintptr_t)arg;
 *     BK_LOGI(NULL, "module %u prepare for ap power off\r\n", module_id);
 * }
 *
 * void example_ap_ctrl_cb_register(void)
 * {
 *     // Register callback, arg will be passed back on callback execution
 *     bk_pm_ap_ctrl_callback_register(ap_poweroff_notify_cb, (void *)1, PM_AP_CTRL_CB_TYPE_POWER_OFF);
 *
 *     // If needed, unregister callback later
 *     // bk_pm_ap_ctrl_callback_unregister(ap_poweroff_notify_cb, PM_AP_CTRL_CB_TYPE_POWER_OFF);
 * }
 */
bk_err_t bk_pm_ap_ctrl_callback_register(ap_ctrl_callback_t callback, void *arg, pm_ap_ctrl_cb_type_t type)
{
	ap_ctrl_callback_node_t *new_node = NULL;
	uint32_t int_level = 0;

	if (!callback) {
		return -1;
	}

	new_node = (ap_ctrl_callback_node_t *)os_malloc(sizeof(ap_ctrl_callback_node_t));
	if (!new_node) {
		return -2;
	}

	new_node->callback = callback;
	new_node->arg = arg;
	new_node->type = type;

	int_level = rtos_disable_int();
	new_node->next = s_ap_ctrl_callback_head;
	s_ap_ctrl_callback_head = new_node;
	rtos_enable_int(int_level);

	return BK_OK;
}

bk_err_t bk_pm_ap_ctrl_callback_unregister(ap_ctrl_callback_t callback, pm_ap_ctrl_cb_type_t type)
{
	ap_ctrl_callback_node_t *curr = NULL;
	ap_ctrl_callback_node_t *prev = NULL;
	uint32_t int_level = 0;

	if (!callback) {
		return -1;
	}

	int_level = rtos_disable_int();

	curr = s_ap_ctrl_callback_head;
	while (curr) {
		if ((curr->callback == callback) && (curr->type == type)) {
			if (prev) {
				prev->next = curr->next;
			} else {
				s_ap_ctrl_callback_head = curr->next;
			}

			rtos_enable_int(int_level);
			os_free(curr);
			return BK_OK;
		}

		prev = curr;
		curr = curr->next;
	}

	rtos_enable_int(int_level);
	return -2;
}

bk_err_t bk_pm_ap_ctrl_callback_execute(pm_ap_ctrl_cb_type_t type)
{
	ap_ctrl_callback_node_t *curr = NULL;
	uint32_t int_level = 0;

	int_level = rtos_disable_int();
	curr = s_ap_ctrl_callback_head;
	rtos_enable_int(int_level);

	while (curr) {
		if (curr->callback && (curr->type == type)) {
			curr->callback(curr->arg);
		}
		curr = curr->next;
	}

	return BK_OK;
}

bk_err_t bk_pm_ap_first_boot_set(bool is_first_boot)
{
	pm_shared_info_t shared_info = {0};

	shared_info.pm_ap_first_boot = is_first_boot;
	bk_sys_sw_regs_update_pm_shared_info(&shared_info, BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP_FIRST_BOOT, BK_SYS_SW_REGS_LOCK_ENABLE);
	__DSB();
	flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info, sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
	__DSB();
	return BK_OK;
}


#if (CONFIG_CPU_CNT > 1)
bk_err_t bk_pm_module_check_cp1_shutdown(void);
pm_mailbox_communication_state_e bk_pm_cp0_psram_malloc_state_get(void);
bk_err_t bk_pm_cp0_psram_malloc_state_set(pm_mailbox_communication_state_e state);
#if CONFIG_DEEP_LV
extern void sys_hal_mailbox_regs_backup();
extern void sys_hal_mailbox_saved_regs_dump();
#endif
static bk_err_t pm_cp0_mailbox_send_data(uint32_t cmd, uint32_t param1, uint32_t param2, uint32_t param3)
{
	mb_chnl_cmd_t mb_cmd = {0};
	bk_err_t ret = BK_OK;

	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = param1;
	mb_cmd.param2 = param2;
	mb_cmd.param3 = param3;

	ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	LOGD("pm_dbg mb_send cmd=0x%x p1=0x%x p2=0x%x p3=0x%x ret=%d\r\n",
		cmd, param1, param2, param3, ret);
	return ret;
}

bk_err_t bk_pm_cp1_recovery_module_state_ctrl(pm_cp1_prepare_close_module_name_e module,pm_cp1_module_recovery_state_e state)
{
	if(state == PM_CP1_MODULE_RECOVERY_STATE_INIT)
	{
		s_pm_cp1_module_recovery_state &= ~(0x1ULL << module);
	}
	else
	{
		s_pm_cp1_module_recovery_state |= (0x1ULL << module);
	}
	LOGD("pm_cp1_rcv:0x%llx %d %d %d\r\n",s_pm_cp1_module_recovery_state,bk_pm_cp1_work_state_get(),bk_pm_cp1_recovery_all_state_get(),s_pm_cp1_ctrl_state);
	if(bk_pm_cp1_recovery_all_state_get())
	{
		bk_pm_module_check_cp1_shutdown();
	}
	return BK_OK;
}

bool bk_pm_cp1_recovery_all_state_get()
{
	bool cp1_all_module_recovery = false;
	if(bk_pm_cp1_work_state_get())
	{
		cp1_all_module_recovery = (s_pm_cp1_module_recovery_state == PM_CP1_RECOVERY_DEFAULT_VALUE);
	}
	return cp1_all_module_recovery;
}
#if CONFIG_DEEP_LV
extern uint32_t g_enter_sleep;
#endif
static void pm_module_bootup_cpu1(pm_power_module_name_e module)
{
	// uint64_t previous_tick = 0;
	// uint64_t current_tick   = 0;
	//if(PM_POWER_MODULE_STATE_OFF == sys_drv_module_power_state_get(module))
	{
		if(module == POWER_SUB_DOMAIN_NAME_AP_CPU)
		{
//boot_cp1:
			#if CONFIG_PM_AP_POWERDOWN_WHEN_LV
			bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_CPU1, 0, 0);
			#endif
			#if CONFIG_DEEP_LV
			if(g_enter_sleep == 0x1)
			{
				extern void sys_hal_mailbox_regs_restore(void);
				sys_hal_mailbox_regs_restore();
				sys_hal_mailbox_saved_regs_dump();
				mb_ipc_reset_notify(1, 1);
				g_enter_sleep = 0x0;
			}
			#endif
            bk_pm_module_vote_power_ctrl(POWER_SUB_DOMAIN_NAME_AP_CPU, PM_POWER_MODULE_STATE_ON);
			/* Keep mailbox heartbeat state machine aligned with AP power transitions. */

			LOGI("pm_dbg ap_power_on: vote_on + reset_notify(on)\r\n");
			// #if defined(RECV_LOG_FROM_MBOX)
			// void reset_forward_log_status(void);
			// // reset cpu1's log transfer status on cpu0.
			// reset_forward_log_status();
			// #endif
			extern void bk_delay_us(UINT32 us);
			bk_delay_us(200);
			#if CONFIG_PSRAM

			bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_MEDIA, PM_POWER_MODULE_STATE_ON);
            #endif
			bk_delay_us(1000);

#if 0//CONFIG_PSRAM
			{
				volatile uint32_t *psram_test_addr = (volatile uint32_t *)psram_malloc(sizeof(uint32_t));
				const uint32_t test_value = 0x5A5AA5A5;
				uint32_t read_value = 0;

				if (psram_test_addr == NULL) {
					BK_LOGE(NULL, "psram self test failed: malloc null\r\n");
				} else {
					*psram_test_addr = test_value;
					read_value = *psram_test_addr;
					if (read_value == test_value) {
						BK_LOGI(NULL, "psram self test pass: addr=0x%x val=0x%x\r\n",
								(uint32_t)psram_test_addr, read_value);
					} else {
						BK_LOGE(NULL, "psram self test failed: addr=0x%x wr=0x%x rd=0x%x\r\n",
								(uint32_t)psram_test_addr, test_value, read_value);
					}
					psram_free((void *)psram_test_addr);
				}
			}
#endif
			extern void bk_start_ap_system(void);
			bk_start_ap_system();
			LOGI("pm_dbg ap_power_on: bk_start_ap_system done\r\n");
			bk_pm_ap_ctrl_callback_execute(PM_AP_CTRL_CB_TYPE_POWER_ON);
			#if 0
			previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
			current_tick = previous_tick;
			while((current_tick - previous_tick) < (PM_BOOT_CP1_WAITING_TIEM*AON_RTC_MS_TICK_CNT))
			{
				if (bk_pm_cp1_work_state_get()) // wait the cp1 response
				{
					break;
				}
				current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
			}

			if(!bk_pm_cp1_work_state_get())
			{
				BK_LOGD(NULL, "cp0 boot cp1[%d] time out, boot cp1 fail!!!\r\n",s_pm_cp1_boot_try_count);

				/*Reset psram*/
#if CONFIG_PSRAM
				//bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_MEDIA, PM_POWER_MODULE_STATE_OFF);
				//bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_MEDIA, PM_POWER_MODULE_STATE_ON);
#endif
				s_pm_cp1_boot_try_count++;
				if(s_pm_cp1_boot_try_count < PM_BOOT_CP1_TRY_COUNT)
				{
					goto boot_cp1;
				}
				if(s_pm_cp1_boot_try_count == PM_BOOT_CP1_TRY_COUNT)
				{
					#if CONFIG_WDT_EN
					bk_wdt_force_reboot();//try 3 times, if fail ,reboot.
					#endif
				}
			}
			#endif
		}
	}
}

static bk_err_t pm_cp1_vote_mutex_init(void)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if (s_pm_cp1_vote_mutex == NULL)
	{
		bk_err_t error_state = rtos_init_mutex(&s_pm_cp1_vote_mutex);
		if (error_state != BK_OK)
		{
			GLOBAL_INT_RESTORE();
			return BK_FAIL;
		}
	}
	GLOBAL_INT_RESTORE();
	return BK_OK;
}
bk_err_t bk_pm_module_check_cp1_shutdown()
{
	// if(0x0 == s_pm_cp1_ctrl_state)
	// {
	// 	pm_module_shutdown_cpu1(POWER_SUB_DOMAIN_NAME_AP_CPU);
	// }
    return BK_OK;
}
static void pm_module_shutdown_cpu1(pm_power_module_name_e module)
{
	bk_err_t ret = BK_OK;
	GLOBAL_INT_DECLARATION();
	//if(PM_POWER_MODULE_STATE_ON == sys_drv_module_power_state_get(module))
	{
		if(module == POWER_SUB_DOMAIN_NAME_AP_CPU)
		{
			#if CONFIG_PM_AP_POWERDOWN_WHEN_LV
			bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_MEDIA, PM_POWER_MODULE_STATE_OFF);
			#endif

			bk_pm_module_vote_power_ctrl(POWER_SUB_DOMAIN_NAME_AP_CPU, PM_POWER_MODULE_STATE_OFF);
			/* AP power is cut, force heartbeat state to OFF immediately. */
			mb_ipc_reset_notify(1, 0);
			LOGI("pm_dbg ap_power_off: vote_off + reset_notify(off)\r\n");
			//bk_pm_module_vote_cpu_freq(PM_DEV_ID_CPU1,PM_CPU_FRQ_DEFAULT);

			GLOBAL_INT_DISABLE();
			bk_pm_cp1_work_state_set(PM_MAILBOX_COMMUNICATION_INIT);
			s_pm_cp1_closing = 0;
			s_pm_cp1_boot_try_count = 0;
			pm_shared_info_t shared_info = {0};

			shared_info.pm_cp0_sleep_state = 0;
			bk_sys_sw_regs_update_pm_shared_info(&shared_info, BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_CP0_SLEEP_STATE, BK_SYS_SW_REGS_LOCK_DISABLE);
			__DSB();
			flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info, sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
			__DSB();

			bk_pm_ap_first_boot_set(false);
			GLOBAL_INT_RESTORE();

			#if CONFIG_PM_AP_POWERDOWN_WHEN_LV
			bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_CPU1, 1, 0);
			#endif
			bk_printf_nonblock(4,NULL,"Shutdown_cp1[%d][%d][%d]\r\n",s_pm_cp1_closing,ret,s_pm_cp1_sema_count); //4:BK_LOG_DEBUG
			LOGI("pm_dbg ap_power_off: shutdown done closing=%d sema=%d\r\n", s_pm_cp1_closing, s_pm_cp1_sema_count);
		}
	}
}

bk_err_t bk_pm_module_vote_boot_ap_ctrl(pm_boot_ap_module_name_e module,pm_power_module_state_e power_state)
{
	bk_err_t ret = BK_OK;
	GLOBAL_INT_DECLARATION();

	if (pm_cp1_vote_mutex_init() != BK_OK)
	{
		BK_LOGE(NULL, "cp1 vote mutex init failed\r\n");
		return BK_FAIL;
	}

	rtos_lock_mutex(&s_pm_cp1_vote_mutex);

	BK_LOGD(NULL, "boot_cp1 %d %d 0x%x [%d][0x%x]E_1\r\n",module, power_state,s_pm_cp1_ctrl_state,s_pm_cp1_closing,&s_pm_cp1_vote_mutex);

	BK_LOGD(NULL, "boot_cp1 %d %d 0x%x [%d]E_2\r\n",module, power_state,s_pm_cp1_ctrl_state,ret);
    if(power_state == PM_POWER_MODULE_STATE_ON)//power on
    {
		if(s_pm_cp1_ctrl_state == 0)
		{
			LOGD("boot_cp1 %d %d 0x%x [%d]E_3\r\n",module, power_state,s_pm_cp1_ctrl_state,ret);
			pm_module_bootup_cpu1(POWER_SUB_DOMAIN_NAME_AP_CPU);
		}
		GLOBAL_INT_DISABLE();
		s_pm_cp1_ctrl_state |= 0x1 << (module);
		GLOBAL_INT_RESTORE();
    }
    else //power down
    {
		if(s_pm_cp1_ctrl_state&(0x1 << (module)))
		{
			GLOBAL_INT_DISABLE();
			s_pm_cp1_ctrl_state &= ~(0x1 << (module));
			GLOBAL_INT_RESTORE();
			if(0x0 == s_pm_cp1_ctrl_state)
			{
				s_pm_cp1_closing = 1;
				BK_LOGD(NULL, "boot_cp1 %d %d close 0x%llx %d\r\n",module, power_state,s_pm_cp1_module_recovery_state,bk_pm_cp1_work_state_get());
				/* Ask AP to run registered stop notifications before power-off. */
				pm_cp0_mailbox_send_data(PM_CP1_RECOVERY_CMD,0,0,0);
				LOGI("pm_dbg ap_close: send recovery cmd\r\n");

				pm_shared_info_t shared_info = {0};

				shared_info.pm_cp0_sleep_state = 1;
				bk_sys_sw_regs_update_pm_shared_info(&shared_info, BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_CP0_SLEEP_STATE, BK_SYS_SW_REGS_LOCK_DISABLE);
				__DSB();
				flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info, sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
				__DSB();

				LOGD("pm_cp0_sleep_state: %d,ap0_sleep_state: %d\r\n", shared_info.pm_cp0_sleep_state, shared_info.pm_ap0_sleep_state);

				uint64_t previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
				uint64_t current_tick = previous_tick;
				bool ap_sleep_ready = false;
				uint64_t next_log_tick = previous_tick + (500 * AON_RTC_MS_TICK_CNT);

				while ((current_tick - previous_tick) < (PM_WAIT_AP_SLEEP_TIMEOUT_MS * AON_RTC_MS_TICK_CNT))
				{
					__DSB();
					flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info, sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
					__DSB();
					bk_sys_sw_regs_get_pm_shared_info(&shared_info);
					__DSB();

					if (shared_info.pm_ap0_sleep_state == 0x1)
					{
						#if CONFIG_DEEP_LV
						sys_hal_mailbox_regs_backup();
						sys_hal_mailbox_saved_regs_dump();
						#endif
						LOGI("pm_dbg ap_close: ap_sleep_state ready, start shutdown\r\n");
						pm_module_shutdown_cpu1(POWER_SUB_DOMAIN_NAME_AP_CPU);
						bk_pm_ap_ctrl_callback_execute(PM_AP_CTRL_CB_TYPE_POWER_OFF);
						LOGD("ap power off!!!\r\n");
						ap_sleep_ready = true;
						s_pm_cp1_closing = 0;
						break;
					}
					current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
					if (current_tick >= next_log_tick)
					{
						LOGD("pm_dbg ap_close_wait: cp0_sleep=%d ap0_sleep=%d elapsed_ms=%llu\r\n",
							shared_info.pm_cp0_sleep_state, shared_info.pm_ap0_sleep_state,
							(current_tick - previous_tick) / AON_RTC_MS_TICK_CNT);
						next_log_tick += (500 * AON_RTC_MS_TICK_CNT);
					}
				}

				if (!ap_sleep_ready)
				{
					LOGE("wait ap0_sleep_state timeout, cp0_sleep_state:%d ap0_sleep_state:%d\r\n",
						shared_info.pm_cp0_sleep_state, shared_info.pm_ap0_sleep_state);
				}
			}
    	}
    }
	rtos_unlock_mutex(&s_pm_cp1_vote_mutex);
    return BK_OK;
}
bk_err_t bk_pm_cp_wakeup_ap_from_wfi(uint8_t core_id)
{
	int ret                       = BK_OK;
#if CONFIG_PM_LV_SUBCORES_ON && !CONFIG_PM_AP_POWERDOWN_WHEN_LV
	mb_chnl_cmd_t mb_cmd          = {0};

	mb_cmd.hdr.cmd = PM_SLEEP_WAKEUP_NOTIFY_CMD;
	mb_cmd.param1 = 0;
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	if(ret == BK_ERR_BUSY)
	{
		BK_LOGI(NULL,"Mb busy[%d]wait next wakeup\r\n",ret);
		ret = BK_FAIL;
	}
	else if(ret == BK_OK)
	{
	}
	else
	{
		BK_LOGE(NULL,"Mb write error[%d]\r\n",ret);
	}
	FIXED_ADDR_WAKEUP_CP_COUNT += 1;

#endif
	return ret;
}
/*Get the cp1 heap malloc count*/
uint32_t bk_pm_get_cp1_psram_malloc_count(uint32_t using_psram_type)
{
	uint64_t previous_tick = 0;
	uint64_t current_tick   = 0;
	if(bk_pm_cp1_work_state_get())
	{
		bk_pm_cp0_psram_malloc_state_set(PM_MAILBOX_COMMUNICATION_INIT);
		pm_cp0_mailbox_send_data(PM_CP1_PSRAM_MALLOC_STATE_CMD,using_psram_type,0,0);
		if(using_psram_type == 0x0)
		{
			s_pm_cp1_psram_malloc_count = 0;
			previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
			current_tick = previous_tick;
			while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*AON_RTC_MS_TICK_CNT))
			{
				if (bk_pm_cp0_psram_malloc_state_get()) // wait the cp1 response
				{
					break;
				}
				current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
			}
			if(!bk_pm_cp0_psram_malloc_state_get())
			{
				BK_LOGD(NULL,"cp0 get the psram malloc state[%d] time out > 100ms\r\n",using_psram_type);
			}

			return s_pm_cp1_psram_malloc_count;
		}
	}
	else
	{
		return 0;
	}
	return 0;
}

/*trigger the cp1 heap malloc dump*/
bk_err_t bk_pm_dump_cp1_psram_malloc_info()
{
	if(bk_pm_cp1_work_state_get())
	{
		pm_cp0_mailbox_send_data(PM_CP1_DUMP_PSRAM_MALLOC_INFO_CMD,0,0,0);
	}
    return BK_OK;
}
#endif