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
#include <modules/pm.h>
#include "sys_driver.h"
#include "bk_pm_internal_api.h"
#include <driver/pwr_clk.h>
#include <sys/types.h>
#include "pm_power.h"

#include "pm_mailbox.h"
#include "pm_debug.h"
#include "pm_sleep.h"

#define PM_CP1_SOC_AON_RTC_REG_BASE          (SOC_AON_RTC_REG_BASE)
#define PM_AON_RTC_CNT_VAL_L_OFFSET          (0x3*4)
#define PM_AON_RTC_CNT_VAL_H_OFFSET          (0xa*4)

#define PM_AON_RTC_CNT_VAL_L_ADDR            (PM_CP1_SOC_AON_RTC_REG_BASE + PM_AON_RTC_CNT_VAL_L_OFFSET)
#define PM_AON_RTC_CNT_VAL_H_ADDR            (PM_CP1_SOC_AON_RTC_REG_BASE + PM_AON_RTC_CNT_VAL_H_OFFSET)

#define PM_SEND_CMD_CP0_RESPONSE_TIME_OUT    (100) //100ms
#define PM_SEND_CMD_CP1_RESPONSE_TIEM        (100)  //100ms
#define PM_BOOT_CP1_WAITING_TIEM             (500) // 0.5s

static uint32_t s_pm_on_modules               = 0;
static uint32_t s_pm_off_modules              = 0;
static uint64_t s_pm_sleeped_modules          = 0;
//static uint8_t  s_debug_en               = 0;

bk_err_t bk_pm_sleep_register_cb(pm_sleep_mode_e sleep_mode, pm_dev_id_e dev_id, pm_cb_conf_t *enter_config, pm_cb_conf_t *exit_config)
{
// 	GLOBAL_INT_DECLARATION();
// 	GLOBAL_INT_DISABLE();
// 	if (sleep_mode == PM_MODE_LOW_VOLTAGE)
// 	{
// 		if (enter_config != NULL)
// 		{
// 			if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_MAX))
// 			{
// 				s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].cb = enter_config->cb;
// 				s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].args = enter_config->args;
// 			}
// 		}

// 		if (exit_config != NULL)
// 		{
// 			if ((exit_config->cb != NULL) && (dev_id < PM_DEV_ID_MAX))
// 			{
// 				s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].cb = exit_config->cb;
// 				s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].args = exit_config->args;
// 			}
// 		}
// 	}
// 	else if (sleep_mode == PM_MODE_DEEP_SLEEP)
// 	{
// 		if (enter_config != NULL)
// 		{
// 			pm_sleep_cb_t cb_item = {dev_id, *enter_config};
// 			// use exit_config to indicate the execution priority when entering sleep
// 			if ((enter_config->cb != NULL) && (exit_config != NULL) && ((pm_cb_priority_e)exit_config->args < PM_CB_PRIORITY_MAX))
// 			{
// 				pm_sleep_cb_push_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[(pm_cb_priority_e)exit_config->args], cb_item);
// 			}
// 			else if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_DEFAULT))
// 			{
// 				pm_sleep_cb_push_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[PM_CB_PRIORITY_0], cb_item);
// 			}
// 			else if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_MAX))
// 			{
// 				pm_sleep_cb_push_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[PM_CB_PRIORITY_1], cb_item);
// 			}
// 		}
// 	}
// #if CONFIG_PM_SUPER_DEEP_SLEEP
// 	else if (sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
// 	{
// 		if (enter_config != NULL)
// 		{
// 			pm_sleep_cb_t cb_item = {dev_id, *enter_config};
// 			if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_DEFAULT))
// 			{
// 				pm_sleep_cb_push_item(s_pm_superdeep_enter_cb_conf, &s_pm_superdeep_enter_cb_cnt[PM_CB_PRIORITY_0], cb_item);
// 			}
// 			else if ((enter_config->cb != NULL) && (dev_id < PM_DEV_ID_MAX))
// 			{
// 				pm_sleep_cb_push_item(s_pm_superdeep_enter_cb_conf, &s_pm_superdeep_enter_cb_cnt[PM_CB_PRIORITY_1], cb_item);
// 			}
// 		}
// 	}
// #endif
// 	else
// 	{
// 		BK_LOGD(NULL, "The sleep mode[%d] not support register call back \r\n", sleep_mode);
// 	}
// 	GLOBAL_INT_RESTORE();
	return BK_OK;
}

bk_err_t bk_pm_sleep_unregister_cb(pm_sleep_mode_e sleep_mode, pm_dev_id_e dev_id, bool enter_cb, bool exit_cb)
{
	// GLOBAL_INT_DECLARATION();
	// GLOBAL_INT_DISABLE();
// 	if (sleep_mode == PM_MODE_LOW_VOLTAGE)
// 	{
// 		if ((enter_cb == true) && (dev_id < PM_DEV_ID_MAX))
// 		{
// 			s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].cb = NULL;
// 			s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_ENTER_LOWVOL_INDEX][dev_id].args = NULL;
// 		}

// 		if (exit_cb == true)
// 		{
// 			s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].cb = NULL;
// 			s_pm_lowvol_enter_exit_cb_conf[PM_SLEEP_CB_EXIT_LOWVOL_INDEX][dev_id].args = NULL;
// 		}
// 	}
// 	else if (sleep_mode == PM_MODE_DEEP_SLEEP)
// 	{
// 		if ((enter_cb == true) && (dev_id < PM_DEV_ID_DEFAULT))
// 		{
// 			for (uint8_t i = PM_SLEEP_CB_IND_PRI_0; i < PM_SLEEP_CB_IND_PRI_1; i++)
// 			{
// 				if (s_pm_deepsleep_enter_cb_conf[i].id == dev_id)
// 				{
// 					pm_sleep_cb_pop_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[PM_CB_PRIORITY_0], i);
// 					break;
// 				}
// 			}
// 		}
// 		else if ((enter_cb == true) && (dev_id < PM_DEV_ID_MAX))
// 		{
// 			for (uint8_t i = PM_SLEEP_CB_IND_PRI_1; i < PM_DEEPSLEEP_CB_SIZE; i++)
// 			{
// 				if (s_pm_deepsleep_enter_cb_conf[i].id == dev_id)
// 				{
// 					pm_sleep_cb_pop_item(s_pm_deepsleep_enter_cb_conf, &s_pm_deepsleep_enter_cb_cnt[PM_CB_PRIORITY_1], i);
// 					break;
// 				}
// 			}
// 		}
// 	}
// #if CONFIG_PM_SUPER_DEEP_SLEEP
// 	else if (sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
// 	{
// 		if ((enter_cb == true) && (dev_id < PM_DEV_ID_DEFAULT))
// 		{
// 			for (uint8_t i = PM_SLEEP_CB_IND_PRI_0; i < PM_SLEEP_CB_IND_PRI_1; i++)
// 			{
// 				if (s_pm_superdeep_enter_cb_conf[i].id == dev_id)
// 				{
// 					pm_sleep_cb_pop_item(s_pm_superdeep_enter_cb_conf, &s_pm_superdeep_enter_cb_cnt[PM_CB_PRIORITY_0], i);
// 					break;
// 				}
// 			}
// 		}
// 		else if ((enter_cb == true) && (dev_id < PM_DEV_ID_MAX))
// 		{
// 			for (uint8_t i = PM_SLEEP_CB_IND_PRI_1; i < PM_SUPERDEEP_CB_SIZE; i++)
// 			{
// 				if (s_pm_superdeep_enter_cb_conf[i].id == dev_id)
// 				{
// 					pm_sleep_cb_pop_item(s_pm_superdeep_enter_cb_conf, &s_pm_superdeep_enter_cb_cnt[PM_CB_PRIORITY_1], i);
// 					break;
// 				}
// 			}
// 		}
// 	}
// #endif
// 	else
// 	{
// 		BK_LOGD(NULL, "The sleep mode[%d] not support unregister call back \r\n", sleep_mode);
// 	}
// 	GLOBAL_INT_RESTORE();

	return BK_OK;
}

bk_err_t bk_pm_module_vote_power_ctrl(pm_power_module_name_e module, pm_power_module_state_e power_state)
{
#if CONFIG_PM_AP_POWER_VOTE_CLIENT
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_pwr_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_POWER_CTRL_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = power_state;
	mb_cmd.param3 = 0;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP0_RESPONSE_TIME_OUT*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_pwr_ctrl_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_pwr_ctrl_state_get())
	{
	    BK_LOGD(NULL, "cp1 power_C:%d time out\r\n",module);
	}

	if (s_debug_en & 0x2)
		BK_LOGD(NULL, "cp1 vote power\r\n");
#endif //CONFIG_MAILBOX
#endif //CONFIG_PM_AP_POWER_VOTE_CLIENT

	if (power_state == PM_POWER_MODULE_STATE_ON) // power on
	{
		bk_pm_module_power_on(&s_pm_off_modules, &s_pm_on_modules, &s_pm_sleeped_modules, module);
	}
	else // power off
	{
		bk_pm_module_power_off(&s_pm_off_modules, &s_pm_on_modules, &s_pm_sleeped_modules, module);
	}

	pm_check_power_off_module(&s_pm_off_modules, &s_pm_on_modules, &s_pm_sleeped_modules);

	return BK_OK;
}

bk_err_t bk_pm_module_vote_sleep_ctrl(pm_sleep_module_name_e module, uint32_t sleep_state, uint32_t sleep_time)
{
#if 1//CONFIG_PM_AP_CPU_FREQ_VOTE_CLIENT
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;

	if(module == PM_SLEEP_MODULE_NAME_LOG)
	{
		return BK_OK;
	}

	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_sleep_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_SLEEP_CTRL_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = sleep_state;
	mb_cmd.param3 = sleep_time;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP0_RESPONSE_TIME_OUT*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_sleep_ctrl_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_sleep_ctrl_state_get())
	{
	    BK_LOGD(NULL, "cp1 wait cp0 vote sleep[%d] time out\r\n",module);
	}
#endif //CONFIG_MAILBOX
#endif //CONFIG_PM_AP_CPU_FREQ_VOTE_CLIENT
	return BK_OK;
}

// bk_err_t bk_pm_clock_ctrl(pm_dev_clk_e module, pm_dev_clk_pwr_e clock_state)
// {
// #if 0//CONFIG_MAILBOX
// 	uint64_t previous_tick  = 0;
// 	uint64_t current_tick   = 0;
// 	mb_chnl_cmd_t mb_cmd    = {0};

// 	GLOBAL_INT_DECLARATION();
// 	GLOBAL_INT_DISABLE();
// 	bk_pm_cp1_clk_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
// 	mb_cmd.hdr.cmd = PM_CLK_CTRL_CMD;
// 	mb_cmd.param1 = module;
// 	mb_cmd.param2 = clock_state;
// 	mb_cmd.param3 = 0;
// 	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
// 	GLOBAL_INT_RESTORE();

// 	previous_tick = pm_cp1_aon_rtc_counter_get();
// 	current_tick = previous_tick;
// 	while((current_tick - previous_tick) < (PM_SEND_CMD_CP0_RESPONSE_TIME_OUT*PM_AON_RTC_DEFAULT_TICK_COUNT))
// 	{
// 	    if (bk_pm_cp1_clk_ctrl_state_get()) // wait the cp0 response
// 	    {
// 			break;
// 	    }
// 	    current_tick = pm_cp1_aon_rtc_counter_get();
// 	}

// 	if(!bk_pm_cp1_clk_ctrl_state_get())
// 	{
// 	    BK_LOGD(NULL, "cp1 vote freq[%d] time out\r\n",module);
// 	}
// #endif
// 	sys_drv_dev_clk_pwr_ctrl(module, clock_state);
// 	return BK_OK;
// }

// bk_err_t bk_pm_module_vote_cpu_freq(pm_dev_id_e module, pm_cpu_freq_e cpu_freq)
// {
// #if CONFIG_PM_AP_CPU_FREQ_VOTE_CLIENT
// #if CONFIG_MAILBOX
// 	uint64_t previous_tick  = 0;
// 	uint64_t current_tick   = 0;

// 	mb_chnl_cmd_t mb_cmd    = {0};
// 	GLOBAL_INT_DECLARATION();
// 	GLOBAL_INT_DISABLE();
// 	bk_pm_cp1_cpu_freq_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
// 	mb_cmd.hdr.cmd = PM_CPU_FREQ_CTRL_CMD;
// 	mb_cmd.param1 = module;
// 	mb_cmd.param2 = cpu_freq;
// 	mb_cmd.param3 = 0;
// 	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
// 	GLOBAL_INT_RESTORE();

// 	previous_tick = pm_cp1_aon_rtc_counter_get();
// 	current_tick = previous_tick;
// 	BK_LOGD(NULL, "cp1 vote freq_B[%lld]\r\n",previous_tick);
// 	while((current_tick - previous_tick) < (PM_SEND_CMD_CP0_RESPONSE_TIME_OUT*PM_AON_RTC_DEFAULT_TICK_COUNT))
// 	{
// 	    if (bk_pm_cp1_cpu_freq_ctrl_state_get()) // wait the cp0 response
// 	    {
// 			break;
// 	    }
// 	    current_tick = pm_cp1_aon_rtc_counter_get();
// 	}

// 	if(!bk_pm_cp1_cpu_freq_ctrl_state_get())
// 	{
// 	    BK_LOGD(NULL, "cp1 vote freq[%d]time out\r\n",module);
// 	}
// 	BK_LOGD(NULL, "cp1 vote freq_E[%lld]\r\n",current_tick);

// 	if(s_debug_en&0x2)
// 		BK_LOGD(NULL, "cpu1 vote cpu freq\r\n");
// #endif
// #endif //CONFIG_PM_AP_CPU_FREQ_VOTE_CLIENT
// 	return BK_OK;
// }


#if 0	///TODO: multiple definition of `bk_pm_module_vote_psram_ctrl
bk_err_t bk_pm_module_vote_psram_ctrl(pm_power_psram_module_name_e module,pm_power_module_state_e power_state)
{
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;

	bk_err_t ret = BK_OK;
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_psram_power_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_CTRL_PSRAM_POWER_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = power_state;
	ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();
	BK_LOGD(NULL, "cp1 vote psram_P B:%d\r\n",ret);
	while(ret != BK_OK)
	{
		ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
		rtos_delay_milliseconds(4);
	}

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_psram_power_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_psram_power_state_get())
	{
	    BK_LOGD(NULL, "cp1 get psram sta timeout\r\n");
	}

	BK_LOGD(NULL, "cp1 vote psram_P E\r\n");
#endif

	return BK_OK;
}
#endif

bk_err_t bk_pm_module_vote_ctrl_external_ldo(gpio_ctrl_ldo_module_e module,gpio_id_t gpio_id,gpio_output_state_e value)
{
#if CONFIG_MAILBOX
	uint64_t previous_tick  = 0;
	uint64_t current_tick   = 0;

	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	bk_pm_cp1_external_ldo_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
	mb_cmd.hdr.cmd = PM_CTRL_EXTERNAL_LDO_CMD;
	mb_cmd.param1 = module;
	mb_cmd.param2 = gpio_id;
	mb_cmd.param3 = value;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();

	previous_tick = pm_cp1_aon_rtc_counter_get();
	current_tick = previous_tick;
	while((current_tick - previous_tick) < (PM_BOOT_CP1_WAITING_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
	{
	    if (bk_pm_cp1_external_ldo_ctrl_state_get()) // wait the cp0 response
	    {
			break;
	    }
	    current_tick = pm_cp1_aon_rtc_counter_get();
	}

	if(!bk_pm_cp1_external_ldo_ctrl_state_get())
	{
	    BK_LOGD(NULL, "cp1 ctr extLdo timeout\r\n");
	}

	BK_LOGD(NULL, "cp1 vote ctr_extLdo\r\n");
#endif

	return BK_OK;
}

#if CONFIG_MAILBOX
uint64_t pm_cp1_aon_rtc_counter_get()
{
	volatile uint32_t val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
	volatile uint32_t val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);

	while (REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR) != val
		|| REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR) != val_hi)
	{
		val = REG_READ(PM_AON_RTC_CNT_VAL_L_ADDR);
		val_hi = REG_READ(PM_AON_RTC_CNT_VAL_H_ADDR);
	}
	return (((uint64_t)(val_hi) << 32) + val);
}
#endif

/*=========================DEBUG/TEST CTRL START========================*/
uint32_t pm_debug_mode()
{
	//return s_debug_en;
	return 0;
}

void pm_debug_ctrl(uint32_t debug_en)
{
	//s_debug_en = debug_en;
}

/*=========================DEBUG/TEST CTRL END========================*/