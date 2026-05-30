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
#include <driver/pwr_clk.h>
#include <driver/mailbox_channel.h>
#include <modules/pm.h>
#include "sys_driver.h"
#include "bk_pm_internal_api.h"
#include <driver/aon_rtc.h>
#include <os/mem.h>
#include <sys_sw_regs.h>
#if CONFIG_PSRAM
#include <driver/psram.h>
#endif
#if CONFIG_WDT_EN
#include "wdt_driver.h"
#endif
#include "driver/low_pwr_core.h"
#include "pm_debug.h"
#include "cache.h"
#if CONFIG_PSRAM
#include "pm_psram.h"
#endif
#define CONFIG_PM_SERVER                     (!CONFIG_PM_CLIENT)

/*=====================DEFINE  SECTION  START=====================*/
#define PM_SEND_CMD_CP1_RESPONSE_TIEM        (100)  //100ms

#define PM_CP1_RECOVERY_DEFAULT_VALUE        (0xFFFFFFFFFFFFFFFF)
#define PM_OPEN_CP1_TIMEOUT                  (20000) //20s
#define PM_SEMA_WAIT_FOREVER                 (0xFFFFFFFF)    /*Wait Forever*/

#define PM_CP_NOTIFY_AP_MAX_COUNT            (100)
#define PM_CP_NOTIFY_DELAY_TIME_US           (10)  //10us
#define PM_WAIT_AP_SLEEP_TIMEOUT_MS          (3000)
#define PM_WAIT_AP_SLEEP_POLL_MS             (1)
#define PM_CHNL_STATE_BUSY                   (1)
#define PM_CHNL_STATE_IDLE                   (0)

/*=====================DEFINE  SECTION  END=====================*/


/*=====================VARIABLE  SECTION  START=================*/
#if CONFIG_PM_CLIENT
static volatile  pm_mailbox_communication_state_e s_pm_cp1_pwr_finish            = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_clk_finish            = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_sleep_finish          = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_cpu_freq_finish       = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_init                  = 0;
static volatile  pm_mailbox_communication_state_e s_pm_external_ldo_ctrl         = 0;
static volatile  pm_mailbox_communication_state_e s_pm_psram_power_ctrl          = 0;
#else // CONFIG_PM_SERVER
static volatile  pm_mailbox_communication_state_e s_pm_cp1_boot_ready            = 0;
static volatile  uint32_t                         s_pm_cp1_boot_try_count        = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp1_psram_malloc_state    = 0;
static volatile  uint32_t                         s_pm_cp1_psram_malloc_count    = 0;
static volatile  uint64_t                         s_pm_cp1_module_recovery_state = PM_CP1_RECOVERY_DEFAULT_VALUE;

#endif
/*=====================VARIABLE  SECTION  END=================*/


/*================FUNCTION  DECLARATION  START========*/
#if CONFIG_PM_CLIENT
static void pm_cp1_mailbox_init();
bk_err_t pm_cp1_mailbox_response(uint32_t cmd, int ret);
bk_err_t bk_pm_cp1_ctrl_state_set(pm_mailbox_communication_state_e state);
pm_mailbox_communication_state_e bk_pm_cp1_ctrl_state_get();
static void pm_cp1_mailbox_send_data(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3);
#endif

#if CONFIG_PM_SERVER && (CONFIG_CPU_CNT > 1)
static void pm_cp0_mailbox_init();
static void pm_module_shutdown_cpu1(pm_power_module_name_e module);
static bk_err_t pm_cp1_vote_mutex_init(void);
bk_err_t bk_pm_cp1_recovery_module_state_ctrl(pm_cp1_prepare_close_module_name_e module,pm_cp1_module_recovery_state_e state);
static bk_err_t pm_cp0_mailbox_send_data(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3);
#endif
/*================FUNCTION  DECLARATION  END========*/

/*================INITIAL FUNCTION  START========*/
bk_err_t bk_pm_mailbox_init()
{
#if CONFIG_PM_CLIENT
	/*cp1 mailbox init*/
	pm_cp1_mailbox_init();
#endif

#if CONFIG_PM_SERVER
	/*cp0 mailbox init*/
	#if (CONFIG_CPU_CNT > 1)
	pm_cp0_mailbox_init();
	#endif
#endif

	return BK_OK;
}
/*================INITIAL FUNCTION  END========*/

/*=====================PM_CLIENT  SECTION  START=================*/
#if CONFIG_PM_CLIENT
bk_err_t bk_pm_cp1_boot_ok_response_set()
{
	if(bk_pm_cp1_ctrl_state_get() == 0x0)
	{
		bk_pm_cp1_ctrl_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
		pm_cp1_mailbox_response(PM_CPU1_BOOT_READY_CMD, 0x1);
	}
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_pwr_ctrl_state_get()
{
	return s_pm_cp1_pwr_finish;
}
bk_err_t bk_pm_cp1_pwr_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_pwr_finish = state;
	return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_cp1_clk_ctrl_state_get()
{
	return s_pm_cp1_clk_finish;
}
 bk_err_t bk_pm_cp1_clk_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_clk_finish = state;
	return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_cp1_sleep_ctrl_state_get()
{
	return s_pm_cp1_sleep_finish;
}
bk_err_t bk_pm_cp1_sleep_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_sleep_finish = state;
	return BK_OK;
}

pm_mailbox_communication_state_e bk_pm_cp1_cpu_freq_ctrl_state_get()
{
	return s_pm_cp1_cpu_freq_finish;
}
bk_err_t bk_pm_cp1_cpu_freq_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_cpu_freq_finish = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_external_ldo_ctrl_state_get()
{
	return s_pm_external_ldo_ctrl;
}
bk_err_t bk_pm_cp1_external_ldo_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_external_ldo_ctrl = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_psram_power_state_get()
{
	return s_pm_psram_power_ctrl;
}
bk_err_t bk_pm_cp1_psram_power_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_psram_power_ctrl = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp1_ctrl_state_get()
{
	return s_pm_cp1_init;
}
bk_err_t bk_pm_cp1_ctrl_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_init = state;
	return BK_OK;
}
bk_err_t bk_pm_cp1_recovery_response(uint32_t cmd, pm_cp1_prepare_close_module_name_e module_name,pm_cp1_module_recovery_state_e state)
{
	pm_cp1_mailbox_send_data(cmd,module_name,state,0);
	return BK_OK;
}
static void pm_cp1_mailbox_send_data(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3)
{
	bk_err_t ret = BK_OK;
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = param1;
	mb_cmd.param2 = param2;
	mb_cmd.param3 = param3;
	ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();
	BK_LOGD(NULL, "cp1 send data %d\r\n",ret);
}
bk_err_t pm_cp1_mailbox_response(uint32_t cmd, int ret)
{
	mb_chnl_cmd_t mb_cmd = {0};
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = ret;
	mb_cmd.param2 = 0;
	mb_cmd.param3 = 0;
	mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
	GLOBAL_INT_RESTORE();
	return BK_OK;
}
static void pm_cp1_mailbox_tx_cmpl_isr(int *pm_mb, mb_chnl_ack_t *cmd_buf)
{
}
static void pm_cp1_mailbox_rx_isr(int *pm_mb, mb_chnl_cmd_t *cmd_buf)
{
	bk_err_t ret = BK_OK;
	uint32_t used_count;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	switch(cmd_buf->hdr.cmd) {
		case PM_POWER_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_pwr_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CLK_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_clk_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		 case PM_SLEEP_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_sleep_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		 case PM_CPU_FREQ_CTRL_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_cp1_cpu_freq_finish = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CTRL_EXTERNAL_LDO_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_external_ldo_ctrl = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CTRL_PSRAM_POWER_CMD:
			if(cmd_buf->param1 == BK_OK)
			{
				s_pm_psram_power_ctrl = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			else
			{
				ret = BK_FAIL;
			}
			break;
		case PM_CP1_PSRAM_MALLOC_STATE_CMD:
			used_count = bk_psram_heap_get_used_count();
			pm_cp1_mailbox_send_data(PM_CP1_PSRAM_MALLOC_STATE_CMD,0x1,used_count,0);
			//BK_LOGD(NULL, "cp1 bk_psram_heap_get_used_count[%d]\r\n", bk_psram_heap_get_used_count());
			break;
		case PM_CP1_DUMP_PSRAM_MALLOC_INFO_CMD:
			bk_psram_heap_get_used_state();
			break;
		case PM_CP1_RECOVERY_CMD:
			stop_cpu1_handle_notifications();
			bk_pm_cp1_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);
			break;
		default:
			break;
	}
	GLOBAL_INT_RESTORE();

	if(ret != BK_OK)
	{
		BK_LOGD(NULL, "cp1 resps:cp0 rev msg error\r\n");
	}
	//if(pm_debug_mode()&0x2)
	{
      if(cmd_buf->hdr.cmd != PM_CP1_PSRAM_MALLOC_STATE_CMD)
      {
		BK_LOGD(NULL, "cp1_mb_rx_isr %d %d %d\r\n",cmd_buf->hdr.cmd,cmd_buf->param1,cmd_buf->param2);
      }
	}
}
static void pm_cp1_mailbox_tx_isr(int *pm_mb)
{
}

static void pm_cp1_mailbox_init()
{
	mb_chnl_open(MB_CHNL_PWC, NULL);
	if (pm_cp1_mailbox_rx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_RX_ISR, pm_cp1_mailbox_rx_isr);
	if (pm_cp1_mailbox_tx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_ISR, pm_cp1_mailbox_tx_isr);
	if (pm_cp1_mailbox_tx_cmpl_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_CMPL_ISR, pm_cp1_mailbox_tx_cmpl_isr);
}
#endif // CONFIG_PM_CLIENT
/*=====================PM_CLIENT  SECTION  END=================*/


/*=====================PM_SERVER  SECTION  START=================*/
#if CONFIG_PM_SERVER
#if (CONFIG_CPU_CNT > 1)
pm_mailbox_communication_state_e bk_pm_cp1_work_state_get()
{
	return s_pm_cp1_boot_ready;
}
bk_err_t bk_pm_cp1_work_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_boot_ready = state;
	return BK_OK;
}
pm_mailbox_communication_state_e bk_pm_cp0_psram_malloc_state_get()
{
	return s_pm_cp1_psram_malloc_state;
}
bk_err_t bk_pm_cp0_psram_malloc_state_set(pm_mailbox_communication_state_e state)
{
	s_pm_cp1_psram_malloc_state = state;
	return BK_OK;
}
#if CONFIG_MAILBOX
static bk_err_t pm_cp0_send_msg(uint32_t event, uint32_t param1,uint32_t param2,uint32_t param3)
{
	pm_ap_core_msg_t msg = {0};
	msg.event= event;
	msg.param1 = param1;
	msg.param2 = param2;
	msg.param3 = param3;
	return bk_pm_send_msg(&msg);
}
bk_err_t bk_pm_cp0_response_cp1(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3)
{
	return pm_cp0_mailbox_send_data(cmd,param1,param2,param3);
}
static bk_err_t pm_cp0_mailbox_send_data(uint32_t cmd, uint32_t param1,uint32_t param2,uint32_t param3)
{
	mb_chnl_cmd_t mb_cmd = {0};
	int ret              = 0;
	uint8_t  retry_count = 0;
	//bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_MAILBOX,0,0);
	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = param1;
	mb_cmd.param2 = param2;
	mb_cmd.param3 = param3;
	ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
    while(ret != BK_OK)
	{
	    retry_count++;
		ret = mb_chnl_write(MB_CHNL_PWC, &mb_cmd);
		rtos_delay_milliseconds(2);
        if(retry_count > 5)
        {
            LOGE("Mailbox send data fail[ret:%d]\r\n",ret);
			//bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_MAILBOX,1,0);
            return ret;
        }
	}
	//bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_MAILBOX,1,0);
	return BK_OK;
}

static void pm_cp0_mailbox_response(uint32_t cmd, int ret)
{
	pm_cp0_mailbox_send_data(cmd,ret,0,0);
}

static void pm_cp0_mailbox_tx_cmpl_isr(int *pm_mb, mb_chnl_ack_t *cmd_buf)
{
}

static void pm_cp0_mailbox_rx_isr(int *pm_mb, mb_chnl_cmd_t *cmd_buf)
{
	bk_err_t ret = BK_OK;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	switch(cmd_buf->hdr.cmd) {
		case PM_POWER_CTRL_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_POWER_CTRL, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_CLK_CTRL_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_CLK_CTRL, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_SLEEP_CTRL_CMD:
			//bk_pm_cp0_response_cp1(PM_SLEEP_CTRL_CMD,BK_OK,0,0);//for more quick when enter lv
			ret = pm_cp0_send_msg(PM_CP_CORE_SLEEP_CTRL, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_CPU_FREQ_CTRL_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_FREQ_CTRL, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_CTRL_EXTERNAL_LDO_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_EXTERNAL_LDO, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_CTRL_PSRAM_POWER_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_PSRAM_POWER, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_CPU1_BOOT_READY_CMD:
			if(cmd_buf->param1 == 0x1)
			{
				s_pm_cp1_boot_ready = PM_MAILBOX_COMMUNICATION_FINISH;
			}
			//if(pm_debug_mode()&0x2)//for temp debug
				BK_LOGD(NULL,"cpu0 receive the cpu1 boot success event [%d]\r\n",cmd_buf->param1);
			break;
		case PM_CP1_PSRAM_MALLOC_STATE_CMD:
			if(cmd_buf->param1 == PM_CP1_PSRAM_MALLOC_STATE_CMD)//Get the psram malloc count
			{
				s_pm_cp1_psram_malloc_count = cmd_buf->param2;
			}
			bk_pm_cp0_psram_malloc_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
			break;
		case PM_CP1_RECOVERY_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_CP2_RECOVERY, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_RTC_DEEPSLEEP_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_RTC_DEEPSLEEP, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_GET_PM_DATA_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_GET_CP_DATA, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_CTRL_AP_STATE_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_CTRL_CP2_STATE, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_ENTER_DEEP_SLEEP_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_STATE_ENTER_DEEPSLEEP, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		case PM_WAKEUP_CONFIG_CMD:
			ret = pm_cp0_send_msg(PM_CP_CORE_WAKEUP_SRC_CFG, cmd_buf->param1,cmd_buf->param2,cmd_buf->param3);
			break;
		default:
			break;
	}
	GLOBAL_INT_RESTORE();
	if(ret != BK_OK)
	{
		BK_LOGD(NULL,"cp0 handle cp1 message error\r\n");
	}

	//if(pm_debug_mode()&0x2)
	{
		if(cmd_buf->hdr.cmd != PM_CP1_PSRAM_MALLOC_STATE_CMD)
		{
			BK_LOGD(NULL,"cp0_mb_rx_isr %d %d %d %d %d\r\n",cmd_buf->hdr.cmd,cmd_buf->param1,cmd_buf->param2,cmd_buf->param3,ret);
		}
	}

}
static void pm_cp0_mailbox_tx_isr(int *pm_mb)
{
}
static void pm_cp0_mailbox_init()
{
	mb_chnl_open(MB_CHNL_PWC, NULL);
	if (pm_cp0_mailbox_rx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_RX_ISR, pm_cp0_mailbox_rx_isr);
	if (pm_cp0_mailbox_tx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_ISR, pm_cp0_mailbox_tx_isr);
	if (pm_cp0_mailbox_tx_cmpl_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_PWC, MB_CHNL_SET_TX_CMPL_ISR, pm_cp0_mailbox_tx_cmpl_isr);
}
#endif //CONFIG_MAILBOX
#endif //(CONFIG_CPU_CNT > 1)


#if (CONFIG_CPU_CNT > 2)
static volatile  pm_mailbox_communication_state_e s_pm_cp2_boot_ready        = 0;
static uint32_t s_pm_cp2_ctrl_state           = 0;
extern void start_cpu2_core(void);
extern void stop_cpu2_core(void);
static void pm_module_bootup_cpu2(pm_power_module_name_e module)
{
	if(PM_POWER_MODULE_STATE_OFF == sys_drv_module_power_state_get(module))
	{
		if(module == PM_POWER_MODULE_NAME_CPU2)
		{
            bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_ON);
            start_cpu2_core();
            //while(!s_pm_cp2_boot_ready);
		}
	}
}
static void pm_module_shutdown_cpu2(pm_power_module_name_e module)
{
	GLOBAL_INT_DECLARATION();
	if(PM_POWER_MODULE_STATE_ON == sys_drv_module_power_state_get(module))
	{
		if(module == PM_POWER_MODULE_NAME_CPU2)
		{
            stop_cpu2_core();
		    bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_OFF);
			GLOBAL_INT_DISABLE();
			s_pm_cp2_boot_ready = 0;
			GLOBAL_INT_RESTORE();
		}
	}
}
bk_err_t bk_pm_module_vote_boot_cp2_ctrl(pm_boot_cp2_module_name_e module,pm_power_module_state_e power_state)
{
	GLOBAL_INT_DECLARATION();

    if(power_state == PM_POWER_MODULE_STATE_ON)//power on
    {
        GLOBAL_INT_DISABLE();
        s_pm_cp2_ctrl_state |= 0x1 << (module);
        GLOBAL_INT_RESTORE();
        pm_module_bootup_cpu2(PM_POWER_MODULE_NAME_CPU2);
    }
    else //power down
    {
		GLOBAL_INT_DISABLE();
		s_pm_cp2_ctrl_state &= ~(0x1 << (module));
		GLOBAL_INT_RESTORE();
		if(0x0 == s_pm_cp2_ctrl_state)
		{
			pm_module_shutdown_cpu2(PM_POWER_MODULE_NAME_CPU2);
		}
    }
    return BK_OK;
}
#endif

#if CONFIG_PSRAM
static uint32_t s_pm_psram_ctrl_state     = 0;

#endif
static bk_err_t pm_psram_power_ctrl(pm_power_psram_module_name_e module,pm_power_module_state_e power_state)
{
#if CONFIG_PSRAM
	bk_err_t ret = BK_OK;
	GLOBAL_INT_DECLARATION();
	//BK_LOGD(NULL,"%s %d %d 0x%x\r\n",__func__, module, power_state,s_pm_psram_ctrl_state);
    if(power_state == PM_POWER_MODULE_STATE_ON)//power on
    {
		if(s_pm_psram_ctrl_state == 0)
		{
			bk_pm_module_vote_vdddig_ctrl(PM_VDDDIG_MODULE_PSRAM,PM_VDDDIG_HIGH_STATE_ON);
		}
		ret = bk_psram_init();
		if(ret != BK_OK)
		{
			LOGE("Psram_I err0:%d",ret);
			bk_psram_deinit();
			ret = bk_psram_init();
			if(ret != BK_OK)
			{
				LOGE("Psram_I err1:%d",ret);
				bk_psram_deinit();
				ret = bk_psram_init();
				if(ret != BK_OK)
				{
					LOGE("Psram_I err2:%d",ret);
					#if CONFIG_WDT_EN
					bk_wdt_force_reboot();//try 3 times, if fail ,reboot.
					#endif
				}
			}
		}
		GLOBAL_INT_DISABLE();
        s_pm_psram_ctrl_state |= 0x1 << (module);
        GLOBAL_INT_RESTORE();
	}
    else //power down
    {
		if(s_pm_psram_ctrl_state&(0x1 << (module)))
		{
			GLOBAL_INT_DISABLE();
			s_pm_psram_ctrl_state &= ~(0x1 << (module));
			GLOBAL_INT_RESTORE();
			#if !CONFIG_PM_PSRAM_FORCE_ON
			if(0x0 == s_pm_psram_ctrl_state)
			{
				bk_psram_deinit();
				bk_pm_module_vote_vdddig_ctrl(PM_VDDDIG_MODULE_PSRAM,PM_VDDDIG_HIGH_STATE_OFF);
				bk_sys_sw_regs_set_psram_power_down(PM_PSRAM_POWER_DOWN_MAGIC);
                bk_pm_get_cp1_psram_malloc_count(0x1);
			}
			#endif
		}
	}
#endif
	return BK_OK;
}
bk_err_t pm_debug_pwr_clk_state()
{
#if CONFIG_PSRAM
    pm_debug_psram_state();
#endif
	BK_LOGD(NULL, "pm_cp1_boot_ready:0x%x 0x%x\r\n",s_pm_cp1_boot_ready,s_pm_cp1_module_recovery_state);

	return BK_OK;
}
uint32_t bk_pm_get_psram_ctrl_state()
{
	uint32_t psram_ctrl_state = 0x1;//Default psram used and power on
	#if CONFIG_PSRAM
	if(s_pm_psram_ctrl_state == 0x0)
	{
		psram_ctrl_state = 0x0;//psram state:power off
	}
	#endif
	return psram_ctrl_state;
}

bk_err_t bk_pm_module_vote_ctrl_external_ldo(uint32_t module,gpio_id_t gpio_id,gpio_output_state_e value)
{
	bk_gpio_ctrl_external_ldo(module,gpio_id,value);
	return BK_OK;
}
bk_err_t bk_pm_module_vote_vdddig_ctrl(pm_vdddig_module_e module,pm_vdddig_high_state_e state)
{
#if CONFIG_SYS_CPU0
	if(state == PM_VDDDIG_HIGH_STATE_ON)
	{
		/*The VDDDIG voltage must be ramped up prior to PRRAM power-on. During CPU operation at high frequencies, the voltage should be increased in conjunction with CPU frequency scaling events.*/
		if((module == PM_VDDDIG_MODULE_PSRAM)&&(s_pm_vdddig_ctrl_state == 0x0))
		{
			sys_hal_set_vdddig_h_vol(PM_VDDDIG_095);
		}
		s_pm_vdddig_ctrl_state |= 0x1 << module;
	}
	else
	{
		s_pm_vdddig_ctrl_state &= ~(0x1 << module);
		if((module == PM_VDDDIG_MODULE_PSRAM)&&(s_pm_vdddig_ctrl_state == 0x0))
		{
			pm_cpu_freq_e  cpu_freq = bk_pm_current_max_cpu_freq_get();
			const cpu_freq_vdddig_t cpu_freq_vdddig_map[] = CPU_FREQ_VDDDIG_MAP;

			for(int i = 0; i < sizeof(cpu_freq_vdddig_map)/sizeof(cpu_freq_vdddig_t); i++)
			{
				if(cpu_freq == cpu_freq_vdddig_map[i].cpu_freq)
				{
					sys_hal_set_vdddig_h_vol(cpu_freq_vdddig_map[i].vdddig);
				}
			}
		}
	}
#endif
	return BK_OK;
}
uint8_t bk_pm_cp_mb_busy(void)
{
    uint8_t state = 0;
    mb_chnl_ctrl(MB_CHNL_PWC,MB_CHNL_GET_STATUS, &state);
    return (state == PM_CHNL_STATE_BUSY) ? 1 : 0;
}
#endif // CONFIG_PM_SERVER
/*=====================PM_SERVER  SECTION  END=================*/

