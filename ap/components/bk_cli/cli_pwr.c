#include "cli.h"
#include "bk_manual_ps.h"
#include "bk_mac_ps.h"
#include "bk_mcu_ps.h"
#include "bk_ps.h"
#include "bk_wifi.h"
#include "modules/pm.h"
#include "sys_driver.h"
#include "bk_pm_internal_api.h"
#include <driver/mailbox_channel.h>
#include <driver/gpio.h>
#include <driver/touch.h>
#include <driver/touch_types.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc_types.h>
#include <driver/aon_rtc.h>
#include <driver/timer.h>
#include <components/bk_platform.h>
#include <driver/pwr_clk.h>
#if CONFIG_SPI
#include <driver/spi.h>
#include "spi_hal.h"
#include "cmsis_gcc.h"
#endif
#include <driver/rosc_32k.h>
#include <driver/rosc_ppm.h>
#include <driver/pm_ap_core.h>
#if CONFIG_PM_DEMO_ENABLE
#include "pm_ap_demo.h"
#endif

#if 1//CONFIG_SYSTEM_CTRL
#define PM_MANUAL_LOW_VOL_VOTE_ENABLE          (0)
#define PM_DEEP_SLEEP_REGISTER_CALLBACK_ENABLE (0x1)

static UINT32 s_cli_sleep_mode = 0;
static UINT32 s_pm_vote1       = 0;
static UINT32 s_pm_vote2       = 0;
static UINT32 s_pm_vote3       = 0;

#if CONFIG_TOUCH
void cli_pm_touch_callback(void *param)
{
	if(s_cli_sleep_mode == PM_MODE_DEEP_SLEEP)//when wakeup from deep sleep, all thing initial
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
	}
	else if(s_cli_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
	}
	else
	{
		bk_pm_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote1,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote2,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote3,0x0,0x0);
	}
	BK_LOGD(NULL, "cli_pm_touch_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}
#endif
void cli_pm_gpio_callback(gpio_id_t gpio_id)
{
	pm_ap_core_msg_t msg = {0};

	if(s_cli_sleep_mode == PM_MODE_DEEP_SLEEP)//when wakeup from deep sleep, all thing initial
	{
		//bk_pm_ap_sleep_mode_set(PM_MODE_DEFAULT);
	}
	else if(s_cli_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		msg.event= PM_AP_CORE_SLEEP_DEMO_HANDLE;
		msg.param1 = PM_MODE_LOW_VOLTAGE;
		msg.param2 = PM_WAKEUP_SOURCE_INT_GPIO;
		msg.param3 = gpio_id;
		bk_pm_ap_core_send_msg(&msg);
	}
	else
	{
		//bk_pm_ap_sleep_mode_set(PM_MODE_DEFAULT);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote1,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote2,0x0,0x0);
		bk_pm_module_vote_sleep_ctrl(s_pm_vote3,0x0,0x0);
	}
	BK_LOGD(NULL, "cli_pm_gpio_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}
static bk_err_t cli_pm_rtc_sleep_wakeup_callback(pm_sleep_mode_e sleep_mode,pm_wakeup_source_e wake_source,void* param_p)
{
	pm_ap_core_msg_t msg = {0};
	msg.event= PM_AP_CORE_SLEEP_DEMO_HANDLE;
	msg.param1 = PM_MODE_LOW_VOLTAGE;
	msg.param2 = PM_WAKEUP_SOURCE_INT_RTC;
	msg.param3 = 0;
	bk_pm_ap_core_send_msg(&msg);
	BK_LOGD(NULL,"rtc sleep wakeup cb[mode:%d][src:%d][param_p:%p]\r\n",sleep_mode,wake_source,param_p);
    return BK_OK;
}
#define PM_MANUAL_LOW_VOL_VOTE_ENABLE    (0)
#define PM_DEEPSLEEP_RTC_THRESHOLD       (500)
#define PM_SHUTDOWN_RTC_THRESHOLD        (4)        //=500ms
#define PM_OLD_TOUCH_WAKE_SOURCE         (4)

static void cli_pm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_sleep_mode = 0;
	UINT32 pm_vote1 = 0,pm_vote2 = 0,pm_vote3=0;
	UINT32 pm_wake_source = 0;
	UINT32 pm_param1 = 0,pm_param2 = 0,pm_param3 = 0;
	//rtc_wakeup_param_t      rtc_wakeup_param         = {0};
	//system_wakeup_param_t   system_wakeup_param      = {0};
	#if CONFIG_TOUCH
	touch_wakeup_param_t    touch_wakeup_param       = {0};
	#endif
	//usbplug_wakeup_param_t  usbplug_wakeup_param     = {0};

	if (argc != 9) 
	{
		BK_LOGD(NULL, "set low power parameter invalid %d\r\n",argc);
		return;
	}
	pm_sleep_mode  = os_strtoul(argv[1], NULL, 10);
	pm_wake_source = os_strtoul(argv[2], NULL, 10);
	pm_vote1       = os_strtoul(argv[3], NULL, 10);
	pm_vote2       = os_strtoul(argv[4], NULL, 10);
	pm_vote3       = os_strtoul(argv[5], NULL, 10);
	pm_param1      = os_strtoul(argv[6], NULL, 10);
	pm_param2      = os_strtoul(argv[7], NULL, 10);
	pm_param3      = os_strtoul(argv[8], NULL, 10);

	BK_LOGD(NULL, "cli_pm_cmd %d %d %d %d %d %d %d!!! \r\n",
				pm_sleep_mode,
				pm_wake_source,
				pm_vote1,
				pm_vote2,
				pm_vote3,
				pm_param1,
				pm_param2);
	if((pm_sleep_mode > PM_MODE_DEFAULT)||(pm_wake_source > PM_WAKEUP_SOURCE_INT_NONE))
	{
		BK_LOGD(NULL, "set low power  parameter value  invalid\r\n");
		return;
	}

	if(pm_sleep_mode == PM_MODE_DEEP_SLEEP || pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if((pm_vote1 > PM_POWER_MODULE_NAME_NONE) ||(pm_vote2 > PM_POWER_MODULE_NAME_NONE) ||(pm_vote3 > PM_POWER_MODULE_NAME_NONE))
		{
			BK_LOGD(NULL, "set pm vote deepsleep parameter value invalid\r\n");
			return;
		}
	}

	if(pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		if((pm_vote1 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_vote2 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_vote3 > PM_SLEEP_MODULE_NAME_MAX))
		{
			BK_LOGD(NULL, "set pm vote low vol parameter value invalid\r\n");
			return;
		}
	}

	s_cli_sleep_mode = pm_sleep_mode;
	s_pm_vote1 = pm_vote1;
	s_pm_vote2 = pm_vote2;
	s_pm_vote3 = pm_vote3;

	/*set wakeup source*/
	if(pm_wake_source == PM_WAKEUP_SOURCE_INT_RTC)
	{
		if (pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
		{
			#if CONFIG_RTC_ANA_WAKEUP_SUPPORT
			if(pm_param1 < PM_SHUTDOWN_RTC_THRESHOLD)
			{
				BK_LOGD(NULL, "param %d invalid ! must > %d which means 500ms.\r\n",pm_param1,PM_SHUTDOWN_RTC_THRESHOLD);
				return;
			}
			//bk_rtc_ana_register_wakeup_source(pm_param1);
			/*workaround fix for unexpecting wakeup from super deep*/
			sys_drv_gpio_ana_wakeup_enable(1, GPIO_4, GPIO_INT_TYPE_MAX);
			#endif
		}
		else
		{
			if(pm_param2 == 0)
			{
				pm_param2 = 0x1;
			}
			// pm_ap_rtc_info_t low_power_info = {0};
			// low_power_info.period_tick                = pm_param1;
			// low_power_info.period_cnt                 = pm_param2;
			// low_power_info.callback                   = cli_pm_rtc_sleep_wakeup_callback;
			// low_power_info.param_p                    = NULL;
			// bk_pm_ap_rtc_regsiter_wakeup(pm_sleep_mode,&low_power_info);
		}
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_GPIO)
	{
		if (pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
		{
			#if CONFIG_GPIO_ANA_WAKEUP_SUPPORT
			bk_gpio_ana_register_wakeup_source(pm_param1,pm_param2);
			#endif
		}
		else
		{
			// pm_gpio_wakeup_config_t gpio_wakeup= {pm_param1,pm_param2};
			// bk_pm_ap_gpio_wakeup_source_config(PM_MODE_LOW_VOLTAGE,WAKEUP_SOURCE_INT_GPIO,&gpio_wakeup);
			#if CONFIG_GPIO_WAKEUP_SUPPORT
			bk_gpio_register_isr(pm_param1, cli_pm_gpio_callback);
			bk_gpio_register_wakeup_source(pm_param1,pm_param2);
			pm_gpio_wakeup_config_t gpio_wakeup = {pm_param1, pm_param2};
			bk_pm_ap_gpio_wakeup_source_config(pm_sleep_mode, PM_WAKEUP_SOURCE_INT_GPIO, &gpio_wakeup);
			#endif //CONFIG_GPIO_WAKEUP_SUPPORT
		}
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE)
	{   
		if(pm_param1 == WIFI_WAKEUP)
		{
			//system_wakeup_param.wifi_bt_wakeup = WIFI_WAKEUP;
		}
		else
		{
			//system_wakeup_param.wifi_bt_wakeup = BT_WAKEUP;
		}

		//bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE, &system_wakeup_param);
	}
	else if((pm_wake_source == PM_WAKEUP_SOURCE_INT_TOUCHED)
		||(pm_wake_source == PM_OLD_TOUCH_WAKE_SOURCE))//bk7256 touch wakeup source value is 4,in order to adapt new project for old cmd
	{
		#if CONFIG_TOUCH
		touch_wakeup_param.touch_channel = pm_param1;
		bk_touch_register_touch_isr((1<< touch_wakeup_param.touch_channel), cli_pm_touch_callback, NULL);
		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_TOUCHED, &touch_wakeup_param);
		#endif
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_USBPLUG)
	{
		//bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_USBPLUG, &usbplug_wakeup_param);
	}
	else
	{
		;
	}

	/*vote*/
	if(pm_sleep_mode == PM_MODE_DEEP_SLEEP || pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if(pm_vote3 == PM_POWER_MODULE_NAME_CPU1)
		{

		}
	}
	else if(pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		#if PM_MANUAL_LOW_VOL_VOTE_ENABLE
		if(pm_vote1 == PM_SLEEP_MODULE_NAME_APP)
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote1,0x1,pm_param3);
		}
		else
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote1,0x1,0x0);
		}

		if(pm_vote2 == PM_SLEEP_MODULE_NAME_APP)
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote2,0x1,pm_param3);
		}
		else
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote2,0x1,0x0);
		}

		if(pm_vote3 == PM_SLEEP_MODULE_NAME_APP)
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote3,0x1,pm_param3);
		}
		else
		{
			bk_pm_module_vote_sleep_ctrl(pm_vote3,0x1,0x0);
		}
		#endif

		if((pm_vote1 == PM_SLEEP_MODULE_NAME_APP)||(pm_vote2 == PM_SLEEP_MODULE_NAME_APP)||(pm_vote3 == PM_SLEEP_MODULE_NAME_APP))
		{
			bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x1,pm_param3);
		}
	}
	else
	{
		;//do something
	}

	//bk_pm_ap_sleep_mode_set(pm_sleep_mode);
}
static void cli_pm_debug(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_debug  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set low power debug parameter invalid %d\r\n",argc);
		return;
	}

	pm_debug = os_strtoul(argv[1], NULL, 10);

	//pm_debug_ctrl(pm_debug);

	if(pm_debug == PM_DEBUG_CTRL_STATE)
	{
		//pm_debug_pwr_clk_state();
		//pm_debug_lv_state();
		pm_debug_module_state();
		bk_pm_ap_power_prepare_dump();
		bk_pm_cpu_freq_dump();
	}
	/*for temp debug*/
	if(pm_debug == 16)
	{

	}

	if(pm_debug == 32)
	{

	}
}
static void cli_pm_vote_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_sleep_mode   = 0;
	UINT32 pm_vote         = 0;
	UINT32 pm_vote_value   = 0;
	UINT32 pm_sleep_time   = 0;
	if (argc != 5)
	{
		BK_LOGD(NULL, "set low power vote parameter invalid %d\r\n",argc);
		return;
	}
	pm_sleep_mode        = os_strtoul(argv[1], NULL, 10);
	pm_vote              = os_strtoul(argv[2], NULL, 10);
	pm_vote_value        = os_strtoul(argv[3], NULL, 10);
	pm_sleep_time        = os_strtoul(argv[4], NULL, 10);
	if((pm_sleep_mode > PM_MODE_DEFAULT)|| (pm_vote > PM_SLEEP_MODULE_NAME_MAX)||(pm_vote_value > 1))
	{
		BK_LOGD(NULL, "set low power vote parameter value  invalid\r\n");
		return;
	}
	/*vote*/
	if(pm_sleep_mode == LOW_POWER_DEEP_SLEEP)
	{
		if((pm_vote == POWER_MODULE_NAME_BTSP)||(pm_vote == POWER_MODULE_NAME_WIFIP_MAC))
		{
			bk_pm_module_vote_power_ctrl(pm_vote,pm_vote_value);
		}
	}
	else if(pm_sleep_mode == LOW_POWER_MODE_LOW_VOLTAGE)
	{
		bk_pm_module_vote_sleep_ctrl(pm_vote,pm_vote_value,pm_sleep_time);
	}
	else
	{
		;//do something
	}
    //pm_printf_current_temperature();
}
#if 1//CONFIG_DEBUG_VERSION
static void cli_pm_vol(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_vol  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm voltage parameter invalid %d\r\n",argc);
		return;
	}

	pm_vol = os_strtoul(argv[1], NULL, 10);
	if ((pm_vol < 0) || (pm_vol > 7))
	{
		BK_LOGD(NULL, "set pm voltage value invalid %d\r\n",pm_vol);
		return;
	}

	//bk_pm_lp_vol_set(pm_vol);

}
static void cli_pm_clk(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_clk_state  = 0;
	UINT32 pm_module_id  = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL, "set pm clk parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_clk_state = os_strtoul(argv[2], NULL, 10);
	if ((pm_clk_state < 0) || (pm_clk_state > 1) || (pm_module_id < 0) || (pm_module_id > PM_CLK_ID_NONE))
	{
		BK_LOGD(NULL, "set pm clk value invalid %d %d\r\n",pm_clk_state,pm_module_id);
		return;
	}
	bk_pm_clock_ctrl(pm_module_id,pm_clk_state);

}
static void cli_pm_power(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_power_state  = 0;
	UINT32 pm_module_id  = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL, "set pm power parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_power_state = os_strtoul(argv[2], NULL, 10);
	if (pm_power_state > 1)
	{
		BK_LOGD(NULL, "set pm power value invalid %d %d \r\n",pm_power_state,pm_module_id);
		return;
	}

	bk_pm_module_vote_power_ctrl(pm_module_id,pm_power_state);

}
static void cli_pm_freq(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret;
	UINT32 pm_freq  = 0;
	UINT32 pm_module_id  = 0;
	pm_cpu_freq_e module_freq = 0;
	pm_cpu_freq_e current_max_freq = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL, "set pm freq parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_freq = os_strtoul(argv[2], NULL, 10);
	if ((pm_freq > PM_CPU_FRQ_DEFAULT) || (pm_module_id >= PM_DEV_ID_MAX))
	{
		BK_LOGD(NULL, "set pm freq value invalid %d %d \r\n",pm_freq,pm_module_id);
		return;
	}

	ret = bk_pm_module_vote_cpu_freq(pm_module_id,pm_freq);
	if (ret != BK_OK)
	{
		BK_LOGD(NULL, "set pm freq failed: %d\r\n", ret);
		return;
	}

	module_freq = bk_pm_module_current_cpu_freq_get(pm_module_id);
	current_max_freq = bk_pm_current_max_cpu_freq_get();
	BK_LOGD(NULL, "PM AP cpu freq test id: %d; freq: %d; current max cpu freq: %d;\r\n",pm_module_id,module_freq,current_max_freq);

}
static const char *cli_pm_cp_cpu_freq_str(pm_cp_cpu_freq_e pm_freq)
{
	switch (pm_freq)
	{
		case PM_CP_CPU_FRQ_XTAL:
			return "XTAL (40M/26M)";
		case PM_CP_CPU_FRQ_60M:
			return "60M";
		case PM_CP_CPU_FRQ_80M:
			return "80M";
		case PM_CP_CPU_FRQ_120M:
			return "120M";
		case PM_CP_CPU_FRQ_160M:
			return "160M";
		case PM_CP_CPU_FRQ_240M:
			return "240M";
		case PM_CP_CPU_FRQ_HIGHEST:
			return "HIGHEST";
		case PM_CP_CPU_FRQ_DEFAULT:
			return "DEFAULT";
		default:
			return "UNKNOWN";
	}
}
static void cli_pm_cp_freq(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret;
	UINT32 pm_freq = 0;
	UINT32 pm_module_id = 0;

	if (argc != 3)
	{
		BK_LOGD(NULL, "set CP pm freq parameter invalid %d\r\n", argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_freq = os_strtoul(argv[2], NULL, 10);
	if ((pm_freq > PM_CP_CPU_FRQ_DEFAULT) || (pm_module_id >= PM_CP_DEV_ID_MAX))
	{
		BK_LOGD(NULL, "set CP pm freq value invalid %d %d\r\n", pm_freq, pm_module_id);
		return;
	}

	BK_LOGD(NULL, "PM CP module id: %d; pm_freq: %d; CPU freq: %s\r\n",
		pm_module_id, pm_freq, cli_pm_cp_cpu_freq_str((pm_cp_cpu_freq_e)pm_freq));

	ret = bk_pm_module_vote_cp_cpu_freq((pm_cp_dev_id_e)pm_module_id, (pm_cp_cpu_freq_e)pm_freq);
	if (ret != BK_OK)
	{
		BK_LOGD(NULL, "set CP pm freq failed: %d\r\n", ret);
		return;
	}

	BK_LOGD(NULL, "PM CP cpu freq test id: %d; freq: %d\r\n", pm_module_id, pm_freq);
}
static void cli_pm_lpo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if 1
	UINT32 pm_lpo  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm lpo parameter invalid %d\r\n",argc);
		return;
	}

	pm_lpo = os_strtoul(argv[1], NULL, 10);
	if ((pm_lpo < 0) || (pm_lpo > 3))
	{
		BK_LOGD(NULL, "set  pm lpo value invalid %d\r\n",pm_lpo);
		return;
	}

	//bk_pm_lpo_src_set(pm_lpo);
#endif
}
static void cli_pm_ctrl(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_ctrl  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm ctrl parameter invalid %d\r\n",argc);
		return;
	}

	pm_ctrl = os_strtoul(argv[1], NULL, 10);
	if ((pm_ctrl < 0) || (pm_ctrl > 1))
	{
		BK_LOGD(NULL, "set pm ctrl value invalid %d\r\n",pm_ctrl);
		return;
	}

	//bk_pm_mcu_pm_ctrl(pm_ctrl);

}
static void cli_pm_pwr_state(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_pwr_module        = 0;
	UINT32 pm_pwr_module_state  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm pwr state parameter invalid %d\r\n",argc);
		return;
	}

	pm_pwr_module = os_strtoul(argv[1], NULL, 10);
	if ((pm_pwr_module < 0) || (pm_pwr_module >= PM_POWER_MODULE_NAME_NONE))
	{
		BK_LOGD(NULL, "pm module[%d] not support ,get power state fail\r\n",pm_pwr_module);
		return;
	}

	//pm_pwr_module_state = bk_pm_module_power_state_get(pm_pwr_module);
	BK_LOGD(NULL, "Get module[%d] power state[%d] \r\n",pm_pwr_module,pm_pwr_module_state);

}
static void cli_pm_auto_vote(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_ctrl  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL, "set pm auto_vote parameter invalid %d\r\n",argc);
		return;
	}

	pm_ctrl = os_strtoul(argv[1], NULL, 10);
	if ((pm_ctrl < 0) || (pm_ctrl > 1))
	{
		BK_LOGD(NULL, "set pm auto vote value invalid %d\r\n",pm_ctrl);
		return;
	}
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
}

#define CLI_DVFS_FREQUNCY_DIV_MAX      (15)
#define CLI_DVFS_FREQUNCY_DIV_BUS_MAX  (1)
static void cli_dvfs_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 cksel_core = 0;
	UINT32 ckdiv_core = 0;
	UINT32 ckdiv_bus  = 0;
	UINT32 ckdiv_cpu0 = 0;
	UINT32 ckdiv_cpu1 = 0;

	if (argc != 6) 
	{
		BK_LOGD(NULL, "set dvfs parameter invalid %d\r\n",argc);
		return;
	}

	GLOBAL_INT_DECLARATION();
	cksel_core   = os_strtoul(argv[1], NULL, 10);
	ckdiv_core   = os_strtoul(argv[2], NULL, 10);
	ckdiv_bus    = os_strtoul(argv[3], NULL, 10);
	ckdiv_cpu0   = os_strtoul(argv[4], NULL, 10);
	ckdiv_cpu1   = os_strtoul(argv[5], NULL, 10);

	BK_LOGD(NULL, "cli_dvfs_cmd %d %d %d %d %d !!! \r\n",
				cksel_core,
				ckdiv_core,
				ckdiv_bus,
				ckdiv_cpu0,
				ckdiv_cpu1);
	GLOBAL_INT_DISABLE();
	if(cksel_core > 3)
	{
		BK_LOGD(NULL, "set dvfs cksel core > 3 invalid %d\r\n",cksel_core);
		GLOBAL_INT_RESTORE();
		return;
	}

	if((ckdiv_core > CLI_DVFS_FREQUNCY_DIV_MAX) || (ckdiv_bus > CLI_DVFS_FREQUNCY_DIV_BUS_MAX)||(ckdiv_cpu0 > CLI_DVFS_FREQUNCY_DIV_MAX)||(ckdiv_cpu0 > CLI_DVFS_FREQUNCY_DIV_MAX))
	{
		BK_LOGD(NULL, "set dvfs ckdiv_core ckdiv_bus ckdiv_cpu0  ckdiv_cpu0  > 15 invalid\r\n");
		GLOBAL_INT_RESTORE();
		return;
	}
	//pm_core_bus_clock_ctrl(cksel_core, ckdiv_core,ckdiv_bus, ckdiv_cpu0,ckdiv_cpu1);
	GLOBAL_INT_RESTORE();
	BK_LOGD(NULL, "switch cpu frequency ok 0x%x 0x%x 0x%x\r\n",sys_drv_all_modules_clk_div_get(CLK_DIV_REG0),sys_drv_cpu_clk_div_get(0),sys_drv_cpu_clk_div_get(1));
}

typedef struct{
	uint32_t cksel_core;  // 0:XTAL       1 : clk_DCO      2 : 320M      3 : 480M
	uint32_t ckdiv_core;  // Frequency division : F/(1+N), N is the data of the reg value 0--15
	uint32_t ckdiv_bus;   // Frequency division : F/(1+N), N is the data of the reg value:0--1
	uint32_t ckdiv_cpu0;  // Frequency division : F/(1+N), N is the data of the reg value:0--15
	uint32_t ckdiv_cpu1;  // 0: cpu0,cpu1 and bus  sel same clock frequence 1: cpu0_clk and  bus_clk is half cpu1_clk
}core_bus_clock_ctrl_t;

#define CORE_BUS_CLOCK_AUTO_TEST 0
#if CORE_BUS_CLOCK_AUTO_TEST
#define CORE_BUS_CLOCK_MAP \
{ \
	{0x0,0x0,0x0,0x0,0x0 },  /*0:XTAL */\
	{0x2,0x1,0x0,0x0,0x0 },  /*2 : 320M  ckdiv_cpu1 = 0 */ \
	{0x2,0x2,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x3,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x4,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x5,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x6,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x7,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x8,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x9,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xA,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xB,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xC,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xD,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xE,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0xF,0x0,0x0,0x0 },  /*2 : 320M */ \
	{0x2,0x0,0x0,0x0,0x1 },  /*2 : 320M  ckdiv_cpu1 = 1*/ \
	{0x2,0x1,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x2,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x3,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x4,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x5,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x6,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x7,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x8,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0x9,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xA,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xB,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xC,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xD,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xE,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x2,0xF,0x0,0x0,0x1 },  /*2 : 320M */ \
	{0x3,0x2,0x0,0x0,0x0 },  /*3 : 480M ckdiv_cpu1 = 0*/ \
	{0x3,0x3,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x4,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x5,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x6,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x7,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x8,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x9,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xA,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xB,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xC,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xD,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xE,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0xF,0x0,0x0,0x0 },  /*3 : 480M */ \
	{0x3,0x1,0x0,0x0,0x1 },  /*3 : 480M ckdiv_cpu1 = 1*/ \
	{0x3,0x2,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x3,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x4,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x5,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x6,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x7,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x8,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0x9,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xA,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xB,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xC,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xD,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xE,0x0,0x0,0x1 },  /*3 : 480M */ \
	{0x3,0xF,0x0,0x0,0x1 },  /*3 : 480M */ \
}
#else
#define CORE_BUS_CLOCK_MAP \
{ \
	{0x0,0x0,0x0,0x0,0x0 },  /*0:XTAL */\
	{0x2,0x1,0x0,0x0,0x0 },  /*2 : 320M  ckdiv_cpu1 = 0 */ \
	{0x3,0x2,0x0,0x0,0x0 },  /*3 : 480M  ckdiv_cpu1 = 0 */ \
	{0x3,0x1,0x0,0x0,0x1 },  /*3 : 480M  ckdiv_cpu1 = 1 */ \
}
#endif
#define DVFS_AUTO_TEST_COUNT (2)
extern void bk_delay_us(uint32 num);
static void cli_dvfs_auto_test_timer_isr(timer_id_t chan)
{
	// uint8_t rand_num;
	// uint8_t i;
	// for(i=0; i<DVFS_AUTO_TEST_COUNT; i++)
	// {
	// 	rand_num = (uint32_t)bk_rand()%PM_CPU_FRQ_DEFAULT;
	// 	//BK_LOGD(NULL, "dvfs random %d \r\n",rand_num);
	// 	bk_delay_us(5);
	// 	sys_drv_switch_cpu_bus_freq(rand_num);
	// }
}

core_bus_clock_ctrl_t core_bus_clock[] = CORE_BUS_CLOCK_MAP;
static void cli_dvfs_auto_test_all_timer_isr(timer_id_t chan)
{
#if 0
	uint8_t rand_num;
	uint8_t i;
	
	for(i=0; i<DVFS_AUTO_TEST_COUNT; i++)
	{
		rand_num = (uint32_t)bk_rand()%(sizeof(core_bus_clock)/sizeof(core_bus_clock_ctrl_t));
		//BK_LOGD(NULL, "dvfs random %d \r\n",rand_num);
		//BK_LOGD(NULL, "[cksel:%d] [ckdiv_core:%d] [ckdiv_bus:%d] [ckdiv_cpu0:%d] [ckdiv_cpu1:%d]\r\n",
			//core_bus_clock[rand_num].cksel_core, core_bus_clock[rand_num].ckdiv_core, core_bus_clock[rand_num].ckdiv_bus, core_bus_clock[rand_num].ckdiv_cpu0, core_bus_clock[rand_num].ckdiv_cpu1);
		bk_delay_us(5);
		pm_core_bus_clock_ctrl(core_bus_clock[rand_num].cksel_core, core_bus_clock[rand_num].ckdiv_core, core_bus_clock[rand_num].ckdiv_bus, core_bus_clock[rand_num].ckdiv_cpu0, core_bus_clock[rand_num].ckdiv_cpu1);
	}
#endif
}

static void cli_dvfs_auto_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t period_us;
	
	if (argc != 3)
	{
		BK_LOGD(NULL, "set dvfs_auto_test parameter invalid %d\r\n",argc);
		return;
	}
	period_us = os_strtoul(argv[1], NULL, 10);
	BK_LOGD(NULL, "dvfs auto test period set %d us!\r\n",period_us);

	//bk_trng_driver_init();
	//bk_trng_start();
	
	if (os_strcmp(argv[2], "default") == 0)
	{
		bk_timer_delay_with_callback(TIMER_ID5,period_us,cli_dvfs_auto_test_timer_isr);
	}
	else if (os_strcmp(argv[2], "all") == 0)
	{
		bk_timer_delay_with_callback(TIMER_ID5,period_us,cli_dvfs_auto_test_all_timer_isr);
	}
	else
	{
		bk_timer_delay_with_callback(TIMER_ID5,period_us,cli_dvfs_auto_test_timer_isr);
	}
}

static void cli_pm_wakeup_source(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 sleep_mode = 0;

	if (argc != 2)
	{
		BK_LOGD(NULL, "set get wakeup source parameter invalid %d\r\n",argc);
		return;
	}

	sleep_mode   = os_strtoul(argv[1], NULL, 10);
	if(sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		#if 0
		BK_LOGD(NULL, "low voltage wakeup source [%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
		#endif
	}
	else if(sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		#if 0
		BK_LOGD(NULL, "deepsleep wakeup source [%d]\r\n",bk_pm_deep_sleep_wakeup_source_get());
		#endif
	}
	else
	{
		BK_LOGD(NULL, "it not support the sleep mode[%d] for wakeup source \r\n",sleep_mode);
	}

}
static void cli_pm_auxldo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 ldo = 0;
	UINT32 out = 0;
	UINT32 state = 0;
	UINT32 user = 0;
	
	if (argc != 5)
	{
		BK_LOGD(NULL, "auxldo parameter invalid %d\r\n",argc);
		return;
	}
	ldo = os_strtoul(argv[1], NULL, 10);
	out = os_strtoul(argv[2], NULL, 10);
	state = os_strtoul(argv[3], NULL, 10);
	user = os_strtoul(argv[4], NULL, 10);

	pm_auxldo_ctrl_cfg_t auxldo_cfg = {0};
	auxldo_cfg.ldo = (auxldo_sel_t)ldo;
	auxldo_cfg.out = (uint32_t)out;
	auxldo_cfg.state = (pm_auxldo_enable_t)state;
	auxldo_cfg.user = (pm_auxldo_user_t)user;
	bk_pm_auxldo_ctrl_vote(&auxldo_cfg);
}
#if (CONFIG_CPU_CNT > 2)
static void cli_pm_boot_cp2(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	// UINT32 boot_cp2_state = 0;
	// UINT32 module_name    = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL, "cp2 ctrl parameter invalid %d\r\n",argc);
		return;
	}
	// module_name   = os_strtoul(argv[1], NULL, 10);
	// boot_cp2_state   = os_strtoul(argv[2], NULL, 10);
	//bk_pm_module_vote_boot_cp2_ctrl(module_name,boot_cp2_state);
}
#endif//CONFIG_CPU_CNT > 2
static void cli_pm_boot_ap(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
// #if 1 && (CONFIG_CPU_CNT > 1)
// 	UINT32 boot_ap_state = 0;
// 	UINT32 module_name    = 0;
// 	if (argc != 3)
// 	{
// 		BK_LOGD(NULL, "ap ctrl parameter invalid %d\r\n",argc);
// 		return;
// 	}
// 	module_name   = os_strtoul(argv[1], NULL, 10);
// 	boot_ap_state   = os_strtoul(argv[2], NULL, 10);
// 	bk_pm_module_vote_boot_ap_ctrl(module_name,boot_ap_state);
// #endif
}
#endif//CONFIG_DEBUG_VERSION

#endif//CONFIG_SYSTEM_CTRL

static void cli_pm_ap_demo_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "init") == 0)
	{
		CLI_LOGD("pm_ap_demo init\r\n");
		pm_demo_thread_main();
	} 
	else if (os_strcmp(argv[1], "deep_sleep") == 0) 
	{
		CLI_LOGD("pm_ap_demo deep sleep\r\n");

		if (argc != 6)
		{
			BK_LOGD(NULL, "set pm_ap_demo deep sleep parameter invalid %d\r\n",argc);
			return;
		}

		#if CONFIG_PM_DEMO_ENABLE
		UINT32 rtc_sleep_time         = 0;
		UINT32 rtc_sleep_repeat_count = 0;
		UINT32 gpio_id                = 0;
		UINT32 gpio_wakeup_int_type   = 0;
		rtc_sleep_time                = os_strtoul(argv[2], NULL, 10);
		rtc_sleep_repeat_count        = os_strtoul(argv[3], NULL, 10);
		gpio_id                       = os_strtoul(argv[4], NULL, 10);
		gpio_wakeup_int_type          = os_strtoul(argv[5], NULL, 10);

		pm_ap_core_msg_t msg = {0};
		msg.event  = PM_DEMO_ENTER_DEEP_SLEEP;
		msg.param1 = rtc_sleep_time;
		msg.param2 = rtc_sleep_repeat_count;
		msg.param3 = gpio_id;
		msg.param4 = gpio_wakeup_int_type;
		bk_pm_demo_send_msg(&msg);
		#endif
	} 
	else if (os_strcmp(argv[1], "sleep") == 0)
	{
		CLI_LOGD("pm_ap_demo sleep\r\n");
		if (argc != 6)
		{
			BK_LOGD(NULL, "set pm_ap_demo sleep parameter invalid %d\r\n",argc);
			return;
		}
		#if CONFIG_PM_DEMO_ENABLE
		UINT32 rtc_sleep_time         = 0;
		UINT32 rtc_sleep_repeat_count = 0;
		UINT32 gpio_id                = 0;
		UINT32 gpio_wakeup_int_type   = 0;
		rtc_sleep_time                = os_strtoul(argv[2], NULL, 10);
		rtc_sleep_repeat_count        = os_strtoul(argv[3], NULL, 10);
		gpio_id                       = os_strtoul(argv[4], NULL, 10);
		gpio_wakeup_int_type          = os_strtoul(argv[5], NULL, 10);
		pm_ap_core_msg_t msg          = {0};
		msg.event  = PM_DEMO_ENTER_LOW_VOLTAGE;
		msg.param1 = rtc_sleep_time;
		msg.param2 = rtc_sleep_repeat_count;
		msg.param3 = gpio_id;
		msg.param4 = gpio_wakeup_int_type;
		bk_pm_demo_send_msg(&msg);
		#endif
	} 
	else 
	{
		CLI_LOGD("pm_ap_demo unknown cmd\r\n");
		return;
	}
}

#if CONFIG_SPI
/*
 * AP fast-suspend SPI callback demo
 *
 * This demo deliberately does not call bk_spi_init() or claim GPIO/DMA
 * resources. Register it only after the target SPI instance has been
 * initialized by its real owner:
 *
 *   pm_spi_fast_demo register 1
 *   pm_spi_fast_demo status
 *   pm_spi_fast_demo busy 1     # simulate an in-flight transfer
 *   pm_spi_fast_demo busy 0
 *   pm_spi_fast_demo prepare 1  # force prepare_power_off to return BUSY
 *   pm_spi_fast_demo prepare 0  # let prepare_power_off return OK
 *   pm_spi_fast_demo unregister
 *
 * Driver integration rules demonstrated here:
 * 1. prepare_power_off is the common normal/Fast Boot drain gate. On its
 *    first idle probe it atomically rejects new submissions; prepare_busy
 *    models a driver-owned lock-free pending counter. It returns BUSY until
 *    normal tasks drain existing work, then the next idle probe returns OK.
 * 2. resume must idempotently reopen the submission gate after cancellation
 *    or Fast Boot recovery. quiesce/app_resume run in task context and may use
 *    RTOS APIs; any quiesce wait must have a finite timeout.
 * 3. backup/restore run with CPU3 offline and CPU2 interrupts disabled. They
 *    must only access retained memory and MMIO: no log, allocation, mutex,
 *    semaphore, delay or other blocking API.
 *    prepare_power_off follows the same non-blocking restrictions.
 * 4. Clock/power dependencies must use PLATFORM/BUS priority callbacks so
 *    they restore before this PERIPHERAL callback.
 * 5. The PM framework never reads driver-private queues or busy flags. WiFi,
 *    BT/BLE and other owners must maintain these states and register callbacks.
 * 6. The callback descriptor and backup storage must remain valid until
 *    bk_pm_ap_power_ops_unregister() succeeds.
 */
#define CLI_SPI_FAST_QUIESCE_TIMEOUT_MS (20U)
#define CLI_SPI_FAST_BACKUP_REG_NUM      (3U)

typedef struct {
	spi_hal_t hal;
	uint32_t regs[CLI_SPI_FAST_BACKUP_REG_NUM];
	volatile bool accepting;
	volatile bool transfer_busy;
	volatile bool prepare_busy;
	volatile bool backup_valid;
	uint32_t prepare_count;
	uint32_t app_resume_count;
	bool registered;
} cli_spi_fast_demo_t;

static cli_spi_fast_demo_t s_cli_spi_fast_demo;

static bk_err_t cli_spi_prepare_power_off(void *arg)
{
	cli_spi_fast_demo_t *demo = (cli_spi_fast_demo_t *)arg;

	/*
	 * Called from the idle path with interrupts disabled. Only close the
	 * submission gate and inspect retained lock-free state. prepare_busy is a
	 * test stand-in for a real module's queue/worker/transaction pending count;
	 * the PM framework intentionally has no knowledge of that private state.
	 */
	__atomic_store_n(&demo->accepting, false, __ATOMIC_RELEASE);
	__atomic_add_fetch(&demo->prepare_count, 1U, __ATOMIC_RELAXED);
	return __atomic_load_n(&demo->prepare_busy, __ATOMIC_ACQUIRE) ?
		BK_ERR_BUSY : BK_OK;
}

static bk_err_t cli_spi_fast_quiesce(void *arg)
{
	cli_spi_fast_demo_t *demo = (cli_spi_fast_demo_t *)arg;
	uint32_t start_ms = rtos_get_time();

	/* A real driver sets this gate before checking its DMA/IRQ busy state. */
	__atomic_store_n(&demo->accepting, false, __ATOMIC_RELEASE);
	while (__atomic_load_n(&demo->transfer_busy, __ATOMIC_ACQUIRE)) {
		if ((rtos_get_time() - start_ms) >=
			CLI_SPI_FAST_QUIESCE_TIMEOUT_MS) {
			__atomic_store_n(&demo->accepting, true, __ATOMIC_RELEASE);
			return BK_ERR_TIMEOUT;
		}
		rtos_delay_milliseconds(1);
	}

	return BK_OK;
}

static bk_err_t cli_spi_fast_backup(void *arg)
{
	cli_spi_fast_demo_t *demo = (cli_spi_fast_demo_t *)arg;
	spi_hw_t *hw = demo->hal.hw;

	/*
	 * Atomic hardware snapshot. The PM framework has already disabled CPU2
	 * interrupts; these are the same configuration registers saved by the
	 * BK7259 SPI HAL. FIFO/data/status registers are intentionally excluded.
	 */
	demo->regs[0] = hw->global_ctrl.v;
	demo->regs[1] = hw->ctrl.v;
	demo->regs[2] = hw->cfg.v;
	__DMB();
	demo->backup_valid = true;
	return BK_OK;
}

static bk_err_t cli_spi_fast_restore(void *arg)
{
	cli_spi_fast_demo_t *demo = (cli_spi_fast_demo_t *)arg;
	spi_hw_t *hw = demo->hal.hw;

	/*
	 * Clock/power must already be available through an earlier PLATFORM/BUS
	 * restore callback. Do not call ordinary SPI driver APIs in this phase.
	 */
	if (demo->backup_valid) {
		hw->global_ctrl.v = demo->regs[0];
		hw->ctrl.v = demo->regs[1];
		hw->cfg.v = demo->regs[2];
		__DMB();
		demo->backup_valid = false;
	}
	return BK_OK;
}

static bk_err_t cli_spi_fast_resume(void *arg)
{
	cli_spi_fast_demo_t *demo = (cli_spi_fast_demo_t *)arg;

	/* Real drivers re-enable software submissions and restart deferred work. */
	__atomic_store_n(&demo->accepting, true, __ATOMIC_RELEASE);
	return BK_OK;
}

static bk_err_t cli_spi_fast_app_resume(void *arg)
{
	cli_spi_fast_demo_t *demo = (cli_spi_fast_demo_t *)arg;
	uint32_t count = __atomic_add_fetch(&demo->app_resume_count, 1U,
		__ATOMIC_RELAXED);

	/*
	 * This phase runs only after AP0/AP1, low-level resources and CP-to-AP
	 * business mailbox communication are ready. Application work may use
	 * normal RTOS, logging, driver and cross-core communication APIs here.
	 */
	CLI_LOGI("SPI fast PM demo app resume:%u\r\n", count);
	return BK_OK;
}

static const pm_ap_power_ops_t s_cli_spi_fast_ops = {
	.name = "cli_spi_demo",
	.prepare_power_off = cli_spi_prepare_power_off,
	.quiesce = cli_spi_fast_quiesce,
	.backup = cli_spi_fast_backup,
	.restore = cli_spi_fast_restore,
	.resume = cli_spi_fast_resume,
	.app_resume = cli_spi_fast_app_resume,
	.arg = &s_cli_spi_fast_demo,
	.priority = PM_AP_POWER_PRIORITY_PERIPHERAL,
};

static void cli_pm_spi_fast_demo(char *pcWriteBuffer,
	int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		CLI_LOGI("usage: pm_spi_fast_demo {register <id>|unregister|busy <0|1>|prepare <0|1>|status}\r\n");
		return;
	}

	if (os_strcmp(argv[1], "register") == 0) {
		uint32_t id;

		if (argc != 3) {
			CLI_LOGI("usage: pm_spi_fast_demo register <spi_id>\r\n");
			return;
		}
		if (s_cli_spi_fast_demo.registered) {
			CLI_LOGW("SPI fast PM demo already registered\r\n");
			return;
		}

		id = os_strtoul(argv[2], NULL, 0);
		if (id >= SPI_ID_MAX) {
			CLI_LOGE("invalid SPI id:%u\r\n", id);
			return;
		}

		os_memset(&s_cli_spi_fast_demo, 0,
			sizeof(s_cli_spi_fast_demo));
		s_cli_spi_fast_demo.hal.id = (spi_unit_t)id;
		s_cli_spi_fast_demo.hal.hw =
			(spi_hw_t *)SPI_LL_REG_BASE(id);
		s_cli_spi_fast_demo.accepting = true;

		ret = bk_pm_ap_power_ops_register(&s_cli_spi_fast_ops);
		if (ret == BK_OK) {
			s_cli_spi_fast_demo.registered = true;
			CLI_LOGI("SPI%u fast PM demo registered\r\n", id);
		} else {
			CLI_LOGE("register SPI fast PM demo failed:%d\r\n", ret);
		}
		return;
	}

	if (!s_cli_spi_fast_demo.registered) {
		CLI_LOGW("SPI fast PM demo is not registered; run: pm_spi_fast_demo register <spi_id>\r\n");
		return;
	}

	if (os_strcmp(argv[1], "unregister") == 0) {
		ret = bk_pm_ap_power_ops_unregister(&s_cli_spi_fast_ops);
		if (ret == BK_OK) {
			s_cli_spi_fast_demo.registered = false;
			CLI_LOGI("SPI fast PM demo unregistered\r\n");
		} else {
			CLI_LOGE("unregister SPI fast PM demo failed:%d\r\n", ret);
		}
		return;
	}

	if (os_strcmp(argv[1], "busy") == 0) {
		if (argc != 3) {
			CLI_LOGI("usage: pm_spi_fast_demo busy <0|1>\r\n");
			return;
		}
		__atomic_store_n(&s_cli_spi_fast_demo.transfer_busy,
			os_strtoul(argv[2], NULL, 0) != 0U,
			__ATOMIC_RELEASE);
		CLI_LOGI("SPI fast PM demo busy:%u\r\n",
			s_cli_spi_fast_demo.transfer_busy);
		return;
	}

	if (os_strcmp(argv[1], "prepare") == 0) {
		if (argc != 3) {
			CLI_LOGI("usage: pm_spi_fast_demo prepare <0|1>\r\n");
			return;
		}
		__atomic_store_n(&s_cli_spi_fast_demo.prepare_busy,
			os_strtoul(argv[2], NULL, 0) != 0U,
			__ATOMIC_RELEASE);
		CLI_LOGI("SPI fast PM demo prepare busy:%u\r\n",
			s_cli_spi_fast_demo.prepare_busy);
		return;
	}

	if (os_strcmp(argv[1], "status") == 0) {
		CLI_LOGI("SPI fast PM demo: registered=%u id=%u accepting=%u transfer_busy=%u prepare_busy=%u prepare_count=%u backup=%u app_resume=%u\r\n",
			s_cli_spi_fast_demo.registered,
			s_cli_spi_fast_demo.hal.id,
			s_cli_spi_fast_demo.accepting,
			s_cli_spi_fast_demo.transfer_busy,
			s_cli_spi_fast_demo.prepare_busy,
			s_cli_spi_fast_demo.prepare_count,
			s_cli_spi_fast_demo.backup_valid,
			s_cli_spi_fast_demo.app_resume_count);
		return;
	}

	CLI_LOGI("usage: pm_spi_fast_demo {register <id>|unregister|busy <0|1>|prepare <0|1>|status}\r\n");
}
#endif

/*
 * Generic non-Fast-Boot AP power-off drain demo.
 *
 * The auto_drain worker represents existing WiFi/BLE work. prepare_power_off
 * closes the submission gate and only observes retained lock-free state; the
 * worker clears pending later in normal task context. The next idle probe can
 * then continue AP power-off. If CP cancels or times out, resume reopens the
 * submission gate.
 */
#define CLI_POWER_PREPARE_DRAIN_TASK_PRIO  (4)
#define CLI_POWER_PREPARE_DRAIN_TASK_STACK (1024)

typedef struct {
	volatile bool accepting;
	volatile bool pending;
	volatile bool drain_running;
	volatile uint32_t prepare_count;
	volatile uint32_t resume_count;
	uint32_t drain_delay_ms;
	beken_thread_t drain_thread;
	bool registered;
} cli_power_prepare_demo_t;

static cli_power_prepare_demo_t s_cli_power_prepare_demo;

static bk_err_t cli_power_prepare_demo_prepare(void *arg)
{
	cli_power_prepare_demo_t *demo = (cli_power_prepare_demo_t *)arg;

	__atomic_store_n(&demo->accepting, false, __ATOMIC_RELEASE);
	__atomic_add_fetch(&demo->prepare_count, 1U, __ATOMIC_RELAXED);
	return __atomic_load_n(&demo->pending, __ATOMIC_ACQUIRE) ?
		BK_ERR_BUSY : BK_OK;
}

static bk_err_t cli_power_prepare_demo_resume(void *arg)
{
	cli_power_prepare_demo_t *demo = (cli_power_prepare_demo_t *)arg;

	/* Non-Fast-Boot prepare abort can run from Idle with IRQs disabled. */
	__atomic_store_n(&demo->accepting, true, __ATOMIC_RELEASE);
	__atomic_add_fetch(&demo->resume_count, 1U, __ATOMIC_RELAXED);
	return BK_OK;
}

static const pm_ap_power_ops_t s_cli_power_prepare_ops = {
	.name = "cli_power_demo",
	.prepare_power_off = cli_power_prepare_demo_prepare,
	.resume = cli_power_prepare_demo_resume,
	.arg = &s_cli_power_prepare_demo,
	.priority = PM_AP_POWER_PRIORITY_SERVICE,
};

static void cli_power_prepare_demo_drain_worker(void *arg)
{
	cli_power_prepare_demo_t *demo = (cli_power_prepare_demo_t *)arg;
	uint32_t delay_ms = demo->drain_delay_ms;

	rtos_delay_milliseconds(delay_ms);
	__atomic_store_n(&demo->pending, false, __ATOMIC_RELEASE);
	__atomic_store_n(&demo->drain_running, false, __ATOMIC_RELEASE);
	demo->drain_thread = NULL;
	CLI_LOGI("AP power prepare demo drained after %u ms\r\n", delay_ms);
	rtos_delete_thread(NULL);
}

static void cli_pm_power_prepare_demo(char *pcWriteBuffer,
	int xWriteBufferLen, int argc, char **argv)
{
	cli_power_prepare_demo_t *demo = &s_cli_power_prepare_demo;
	bk_err_t ret;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		CLI_LOGI("usage: pm_power_prepare_demo {register|unregister|busy <0|1>|auto_drain <ms>|status}\r\n");
		return;
	}

	if (os_strcmp(argv[1], "register") == 0) {
		if (demo->registered) {
			CLI_LOGW("AP power prepare demo already registered\r\n");
			return;
		}
		os_memset(demo, 0, sizeof(*demo));
		demo->accepting = true;
		ret = bk_pm_ap_power_ops_register(&s_cli_power_prepare_ops);
		if (ret == BK_OK) {
			demo->registered = true;
			CLI_LOGI("AP power prepare demo registered\r\n");
		} else {
			CLI_LOGE("register AP power prepare demo failed:%d\r\n", ret);
		}
		return;
	}

	if (!demo->registered) {
		CLI_LOGW("AP power prepare demo is not registered; run: pm_power_prepare_demo register\r\n");
		return;
	}

	if (os_strcmp(argv[1], "unregister") == 0) {
		if (__atomic_load_n(&demo->drain_running, __ATOMIC_ACQUIRE)) {
			CLI_LOGW("AP power prepare demo drain worker is running\r\n");
			return;
		}
		ret = bk_pm_ap_power_ops_unregister(&s_cli_power_prepare_ops);
		if (ret == BK_OK) {
			demo->registered = false;
			CLI_LOGI("AP power prepare demo unregistered\r\n");
		} else {
			CLI_LOGE("unregister AP power prepare demo failed:%d\r\n", ret);
		}
		return;
	}

	if ((os_strcmp(argv[1], "busy") == 0) ||
		(os_strcmp(argv[1], "pending") == 0)) {
		if (argc != 3) {
			CLI_LOGI("usage: pm_power_prepare_demo busy <0|1>\r\n");
			return;
		}
		if (__atomic_load_n(&demo->drain_running, __ATOMIC_ACQUIRE)) {
			CLI_LOGW("AP power prepare demo drain worker is running\r\n");
			return;
		}
		__atomic_store_n(&demo->pending,
			os_strtoul(argv[2], NULL, 0) != 0U, __ATOMIC_RELEASE);
		CLI_LOGI("AP power prepare demo state:%s\r\n",
			demo->pending ? "BUSY" : "IDLE");
		return;
	}

	if (os_strcmp(argv[1], "auto_drain") == 0) {
		if (argc != 3) {
			CLI_LOGI("usage: pm_power_prepare_demo auto_drain <ms>\r\n");
			return;
		}
		if (__atomic_load_n(&demo->drain_running, __ATOMIC_ACQUIRE)) {
			CLI_LOGW("AP power prepare demo drain worker is already running\r\n");
			return;
		}
		demo->drain_delay_ms = os_strtoul(argv[2], NULL, 0);
		__atomic_store_n(&demo->pending, true, __ATOMIC_RELEASE);
		__atomic_store_n(&demo->drain_running, true, __ATOMIC_RELEASE);
		ret = rtos_create_thread(&demo->drain_thread,
			CLI_POWER_PREPARE_DRAIN_TASK_PRIO,
			"pm_drain",
			(beken_thread_function_t)cli_power_prepare_demo_drain_worker,
			CLI_POWER_PREPARE_DRAIN_TASK_STACK,
			demo);
		if (ret != BK_OK) {
			demo->drain_thread = NULL;
			__atomic_store_n(&demo->drain_running, false,
				__ATOMIC_RELEASE);
			__atomic_store_n(&demo->pending, false, __ATOMIC_RELEASE);
			CLI_LOGE("create AP power prepare drain worker failed:%d\r\n",
				ret);
		} else {
			CLI_LOGI("AP power prepare demo pending, auto drain after %u ms\r\n",
				demo->drain_delay_ms);
		}
		return;
	}

	if (os_strcmp(argv[1], "status") == 0) {
		CLI_LOGI("AP power prepare demo: registered=%u state=%s accepting=%u pending=%u drain_running=%u prepare_count=%u resume_count=%u\r\n",
			demo->registered, demo->pending ? "BUSY" : "IDLE",
			demo->accepting, demo->pending,
			demo->drain_running, demo->prepare_count,
			demo->resume_count);
		return;
	}

	CLI_LOGI("usage: pm_power_prepare_demo {register|unregister|busy <0|1>|auto_drain <ms>|status}\r\n");
}

#define PWR_CMD_CNT (sizeof(s_pwr_commands) / sizeof(struct cli_command))
static const struct cli_command s_pwr_commands[] = {
#if 1//CONFIG_SYSTEM_CTRL
#if 1//CONFIG_DEBUG_VERSION
	{"pm", "pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]", cli_pm_cmd},
	{"dvfs", "dvfs [cksel_core] [ckdiv_core] [ckdiv_bus] [ckdiv_cpu0] [ckdiv_cpu1]", cli_dvfs_cmd},
	{"dvfs_auto_test", "dvfs_auto_test [period]", cli_dvfs_auto_test},
	{"pm_vote", "pm_vote [pm_sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]", cli_pm_vote_cmd},
	{"pm_debug", "pm_debug [debug_en_value]", cli_pm_debug},
	{"pm_lpo", "pm_lpo [lpo_type]", cli_pm_lpo},
	{"pm_vol", "pm_vol [vol_value]", cli_pm_vol},
	{"pm_clk", "pm_clk [module_name][clk_state]", cli_pm_clk},
	{"pm_power", "pm_power [module_name][ power state]", cli_pm_power},
	{"pm_freq", "pm_freq [module_name][ frequency]", cli_pm_freq},
	{"pm_cp_freq", "pm_cp_freq [module_name][frequency]", cli_pm_cp_freq},
	{"pm_ctrl", "pm_ctrl [ctrl_value]", cli_pm_ctrl},
	{"pm_pwr_state", "pm_pwr_state [pwr_state]", cli_pm_pwr_state},
	{"pm_auto_vote", "pm_auto_vote [auto_vote_value]", cli_pm_auto_vote},
	{"pm_wakeup_source", "pm_wakeup_source [pm_sleep_mode]", cli_pm_wakeup_source},
	{"pm_auxldo", "pm_auxldo [ldo] [out] [state] [user]", cli_pm_auxldo},
#if (CONFIG_CPU_CNT > 2)
	{"pm_boot_cp2", "pm_boot_cp2 [module_name] [ctrl_state:0x0:bootup; 0x1:shutdowm]", cli_pm_boot_cp2},
#endif
	{"pm_boot_ap", "pm_boot_ap [module_name] [ctrl_state:0x0:bootup; 0x1:shutdowm]", cli_pm_boot_ap},
	{"pm_ap_demo", "pm_ap_demo {init|sleep|deep_sleep}", cli_pm_ap_demo_cmd},
#if CONFIG_SPI
	{"pm_spi_fast_demo", "pm_spi_fast_demo {register <id>|unregister|busy <0|1>|prepare <0|1>|status}", cli_pm_spi_fast_demo},
#endif
	{"pm_power_prepare_demo", "pm_power_prepare_demo {register|unregister|busy <0|1>|auto_drain <ms>|status}", cli_pm_power_prepare_demo},
#else
	{"pm", "pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]", cli_pm_cmd},
	{"pm_vote", "pm_vote [pm_sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]", cli_pm_vote_cmd},
	{"pm_debug", "pm_debug [debug_en_value]", cli_pm_debug},
	{"pm_ap_demo", "pm_ap_demo {init|sleep|deep_sleep}", cli_pm_ap_demo_cmd},
	{"pm_power_prepare_demo", "pm_power_prepare_demo {register|unregister|busy <0|1>|auto_drain <ms>|status}", cli_pm_power_prepare_demo},
#endif //CONFIG_DEBUG_VERSION
#endif //CONFIG_SYSTEM_CTRL
};

int cli_pwr_init(void)
{
	return cli_register_commands(s_pwr_commands, PWR_CMD_CNT);
}
