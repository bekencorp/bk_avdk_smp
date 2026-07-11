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
#include <driver/pwr_clk.h>
#include <driver/rosc_32k.h>
#include <driver/rosc_ppm.h>
#include "sys_pm_hal_debug.h"
#include "pm_debug.h"

#if 1//CONFIG_SYSTEM_CTRL
#define PM_MANUAL_LOW_VOL_VOTE_ENABLE          (0)
#define PM_DEEP_SLEEP_REGISTER_CALLBACK_ENABLE (0x1)

static UINT32 s_cli_sleep_mode      = 0;
static UINT32 s_pm_vote1            = 0;
static UINT32 s_pm_vote2            = 0;
static UINT32 s_pm_vote3            = 0;
UINT32 s_pm_rtc_sleep_count  = 0;


extern void stop_cpu1_core(void);
#if CONFIG_AON_RTC
static void cli_pm_rtc_callback(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
	pm_ap_core_msg_t msg;
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);

	//BK_LOGD(NULL,"cli_pm_rtc_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
	/* Always send message to ensure 10ms periodic processing */
	msg.event  = PM_CALLBACK_HANDLE_MSG;
	msg.param1 = PM_MODE_LOW_VOLTAGE;
	msg.param2 = PM_WAKEUP_SOURCE_INT_RTC;
	msg.param3 = 2;
	/* Non-blocking send, will drop if queue is full */
	bk_pm_send_msg(&msg);
}
#endif
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
	BK_LOGD(NULL,"cli_pm_touch_callback[%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
}
#endif
void cli_pm_gpio_callback(gpio_id_t gpio_id)
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
	BK_LOGD(NULL,"cli_pm_gpio_callback[%d], gpio_id: %d.\r\n",bk_pm_exit_low_vol_wakeup_source_get(), gpio_id);
}

#if (CONFIG_CPU_CNT > 1)
extern int mb_ipc_cpu_is_power_off(u32 cpu_id);

#define PM_BOOT_AP_STRESS_MAX_TASKS        (8)
#define PM_BOOT_AP_STRESS_TASK_STACK       (2048)
#define PM_BOOT_AP_STRESS_TASK_PRIO        (5)
#define PM_BOOT_AP_STRESS_CHECK_TIMEOUT_MS (3000)
#define PM_BOOT_AP_STRESS_CHECK_STEP_MS    (10)

typedef struct {
	UINT32 module_name;
	UINT32 power_state;
	bk_err_t ret;
	uint32_t worker_id;
} pm_boot_ap_stress_worker_t;

static beken_semaphore_t s_pm_boot_ap_stress_sema = NULL;

static bool cli_pm_boot_ap_wait_state(UINT32 power_state, UINT32 timeout_ms)
{
	UINT32 elapsed_ms = 0;
	bool ap_boot = false;
	int ap_off = 1;

	while (elapsed_ms <= timeout_ms)
	{
		ap_boot = bk_pm_ap_boot_success_get();
		ap_off = mb_ipc_cpu_is_power_off(1);

		if ((power_state == PM_POWER_MODULE_STATE_ON) && ap_boot)
		{
			return true;
		}

		if ((power_state == PM_POWER_MODULE_STATE_OFF) && !ap_boot && ap_off)
		{
			return true;
		}

		rtos_delay_milliseconds(PM_BOOT_AP_STRESS_CHECK_STEP_MS);
		elapsed_ms += PM_BOOT_AP_STRESS_CHECK_STEP_MS;
	}

	BK_LOGW(NULL, "pm_boot_ap stress wait timeout: state=%u timeout=%u ap_boot=%d ap_off=%d\r\n",
		power_state, timeout_ms, ap_boot, ap_off);
	return false;
}

static void cli_pm_boot_ap_stress_worker(void *arg)
{
	pm_boot_ap_stress_worker_t *worker = (pm_boot_ap_stress_worker_t *)arg;

	worker->ret = bk_pm_module_vote_boot_ap_ctrl(worker->module_name, worker->power_state);
	rtos_set_semaphore(&s_pm_boot_ap_stress_sema);
	rtos_delete_thread(NULL);
}

static uint32_t cli_pm_boot_ap_stress_run_phase(pm_boot_ap_stress_worker_t *workers,
	UINT32 start_module, UINT32 task_count, UINT32 power_state)
{
	uint32_t created_count = 0;
	uint32_t index;

	for (index = 0; index < task_count; index++)
	{
		beken_thread_t handle = NULL;

		workers[index].module_name = start_module + index;
		workers[index].power_state = power_state;
		workers[index].ret = BK_FAIL;
		workers[index].worker_id = index;

		if (rtos_create_thread(&handle,
			PM_BOOT_AP_STRESS_TASK_PRIO,
			"pm_ap_stress",
			(beken_thread_function_t)cli_pm_boot_ap_stress_worker,
			PM_BOOT_AP_STRESS_TASK_STACK,
			(beken_thread_arg_t)&workers[index]) != BK_OK)
		{
			BK_LOGE(NULL, "pm_boot_ap stress create task failed, idx=%u module=%u state=%u\r\n",
				index, workers[index].module_name, power_state);
			continue;
		}

		created_count++;
	}

	for (index = 0; index < created_count; index++)
	{
		rtos_get_semaphore(&s_pm_boot_ap_stress_sema, BEKEN_WAIT_FOREVER);
	}

	return created_count;
}

static void cli_pm_boot_ap_stress(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 start_module = 0;
	UINT32 task_count = 0;
	UINT32 loop_count = 0;
	UINT32 on_hold_ms = 0;
	UINT32 off_hold_ms = 0;
	UINT32 loop;
	uint32_t total_count = 0;
	uint32_t success_count = 0;
	uint32_t fail_count = 0;
	pm_boot_ap_stress_worker_t workers[PM_BOOT_AP_STRESS_MAX_TASKS] = {0};

	if ((argc != 6) && (argc != 7))
	{
		BK_LOGI(NULL, "usage: pm_boot_ap stress [start_module] [task_count] [loop_count] [on_hold_ms] [off_hold_ms]\r\n");
		return;
	}

	start_module = os_strtoul(argv[2], NULL, 10);
	task_count = os_strtoul(argv[3], NULL, 10);
	loop_count = os_strtoul(argv[4], NULL, 10);
	on_hold_ms = os_strtoul(argv[5], NULL, 10);
	if (argc == 7)
	{
		off_hold_ms = os_strtoul(argv[6], NULL, 10);
	}

	if ((task_count == 0) || (task_count > PM_BOOT_AP_STRESS_MAX_TASKS) ||
		(loop_count == 0) || ((start_module + task_count) > PM_BOOT_AP_MODULE_NAME_MAX))
	{
		BK_LOGE(NULL, "pm_boot_ap stress invalid param: start=%u tasks=%u loops=%u max_tasks=%u module_max=%u\r\n",
			start_module, task_count, loop_count, PM_BOOT_AP_STRESS_MAX_TASKS, PM_BOOT_AP_MODULE_NAME_MAX);
		return;
	}

	if (s_pm_boot_ap_stress_sema != NULL)
	{
		BK_LOGE(NULL, "pm_boot_ap stress already running\r\n");
		return;
	}

	if (rtos_init_semaphore_ex(&s_pm_boot_ap_stress_sema, task_count, 0) != BK_OK)
	{
		BK_LOGE(NULL, "pm_boot_ap stress init semaphore failed\r\n");
		return;
	}

	BK_LOGI(NULL, "pm_boot_ap stress start: start_module=%u tasks=%u loops=%u on_hold_ms=%u off_hold_ms=%u\r\n",
		start_module, task_count, loop_count, on_hold_ms, off_hold_ms);

	for (loop = 0; loop < loop_count; loop++)
	{
		uint32_t index;
		uint32_t created_count;
		bool phase_ok;

		created_count = cli_pm_boot_ap_stress_run_phase(workers, start_module, task_count, PM_POWER_MODULE_STATE_ON);
		phase_ok = (created_count == task_count) &&
			cli_pm_boot_ap_wait_state(PM_POWER_MODULE_STATE_ON, PM_BOOT_AP_STRESS_CHECK_TIMEOUT_MS);

		for (index = 0; index < task_count; index++)
		{
			total_count++;
			if ((index < created_count) && (workers[index].ret == BK_OK) && phase_ok)
			{
				success_count++;
			}
			else
			{
				fail_count++;
			}
		}

		BK_LOGI(NULL, "pm_boot_ap stress loop=%u ON result=%d total=%u success=%u fail=%u\r\n",
			loop, phase_ok, total_count, success_count, fail_count);

		if (on_hold_ms)
		{
			rtos_delay_milliseconds(on_hold_ms);
		}

		created_count = cli_pm_boot_ap_stress_run_phase(workers, start_module, task_count, PM_POWER_MODULE_STATE_OFF);
		phase_ok = (created_count == task_count) &&
			cli_pm_boot_ap_wait_state(PM_POWER_MODULE_STATE_OFF, PM_BOOT_AP_STRESS_CHECK_TIMEOUT_MS);

		for (index = 0; index < task_count; index++)
		{
			total_count++;
			if ((index < created_count) && (workers[index].ret == BK_OK) && phase_ok)
			{
				success_count++;
			}
			else
			{
				fail_count++;
			}
		}

		BK_LOGI(NULL, "pm_boot_ap stress loop=%u OFF result=%d total=%u success=%u fail=%u\r\n",
			loop, phase_ok, total_count, success_count, fail_count);

		if (off_hold_ms && ((loop + 1) < loop_count))
		{
			rtos_delay_milliseconds(off_hold_ms);
		}
	}

	BK_LOGI(NULL, "pm_boot_ap stress done: total=%u success=%u fail=%u\r\n",
		total_count, success_count, fail_count);

	rtos_deinit_semaphore(&s_pm_boot_ap_stress_sema);
	s_pm_boot_ap_stress_sema = NULL;
}
#endif

#define PM_MANUAL_LOW_VOL_VOTE_ENABLE    (0)
#define PM_DEEPSLEEP_RTC_THRESHOLD       (500)
#define PM_SHUTDOWN_RTC_THRESHOLD        (4)        //=500ms
#define PM_OLD_TOUCH_WAKE_SOURCE         (4)
extern void stop_cpu1_core(void);

static void cli_pm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_sleep_mode = 0;
	UINT32 pm_vote1 = 0,pm_vote2 = 0,pm_vote3=0;
	UINT32 pm_wake_source = 0;
	UINT32 pm_param1 = 0,pm_param2 = 0,pm_param3 = 0;
	//rtc_wakeup_param_t      rtc_wakeup_param         = {0};
	system_wakeup_param_t   system_wakeup_param      = {0};
	#if CONFIG_TOUCH
	touch_wakeup_param_t    touch_wakeup_param       = {0};
	#endif
	usbplug_wakeup_param_t  usbplug_wakeup_param     = {0};

	if (argc != 9) 
	{
		BK_LOGD(NULL,"set low power parameter invalid %d\r\n",argc);
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
	
	BK_LOGD(NULL,"cli_pm_cmd %d %d %d %d %d %d %d!!! \r\n",
				pm_sleep_mode,
				pm_wake_source,
				pm_vote1,
				pm_vote2,
				pm_vote3,
				pm_param1,
				pm_param2);
	if((pm_sleep_mode > PM_MODE_DEFAULT)||(pm_wake_source > PM_WAKEUP_SOURCE_INT_NONE))
	{
		BK_LOGD(NULL,"set low power  parameter value  invalid\r\n");
		return;
	}

	if(pm_sleep_mode == PM_MODE_DEEP_SLEEP || pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if((pm_vote1 > PM_POWER_MODULE_NAME_NONE) ||(pm_vote2 > PM_POWER_MODULE_NAME_NONE) ||(pm_vote3 > PM_POWER_MODULE_NAME_NONE))
		{
			BK_LOGD(NULL,"set pm vote deepsleep parameter value invalid\r\n");
			return;
		}
	}

	if(pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		if((pm_vote1 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_vote2 > PM_SLEEP_MODULE_NAME_MAX) ||(pm_vote3 > PM_SLEEP_MODULE_NAME_MAX))
		{
			BK_LOGD(NULL,"set pm vote low vol parameter value invalid\r\n");
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
				BK_LOGD(NULL,"param %d invalid ! must > %d which means 500ms.\r\n",pm_param1,PM_SHUTDOWN_RTC_THRESHOLD);
				return;
			}
			bk_rtc_ana_register_wakeup_source(pm_param1);
			/*workaround fix for unexpecting wakeup from super deep*/
			sys_drv_gpio_ana_wakeup_enable(1, GPIO_4, GPIO_INT_TYPE_MAX);
			#endif
		}
		else
		{
			#if CONFIG_AON_RTC
			alarm_info_t low_valtage_alarm = {0};
			memcpy(low_valtage_alarm.name, "low_vol", sizeof("low_vol"));
			low_valtage_alarm.period_tick = pm_param1*AON_RTC_MS_TICK_CNT;
			low_valtage_alarm.period_cnt = 1;
			low_valtage_alarm.callback = cli_pm_rtc_callback;
			low_valtage_alarm.param_p = NULL;
			if(pm_param1 < PM_DEEPSLEEP_RTC_THRESHOLD)
			{
				BK_LOGD(NULL,"param %d invalid ! must > %dms.\r\n",pm_param1,PM_DEEPSLEEP_RTC_THRESHOLD);
				return;
			}
			s_pm_rtc_sleep_count = pm_param2;
			//force unregister previous if doesn't finish.
			bk_alarm_unregister(AON_RTC_ID_1, low_valtage_alarm.name);
			bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);
			#endif //CONFIG_AON_RTC
			bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
		}
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_GPIO)
	{
		if (pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
		{
			#if CONFIG_GPIO_ANA_WAKEUP_SUPPORT
			//bk_gpio_ana_register_wakeup_source(pm_param1,pm_param2);
			#endif
		}
		else
		{
			#if CONFIG_GPIO_WAKEUP_SUPPORT
			bk_gpio_register_isr(pm_param1, cli_pm_gpio_callback);
			bk_gpio_register_wakeup_source(pm_param1,pm_param2);
			bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
			#endif //CONFIG_GPIO_WAKEUP_SUPPORT
		}
	}
	else if(pm_wake_source == PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE)
	{   
		if(pm_param1 == WIFI_WAKEUP)
		{
			system_wakeup_param.wifi_bt_wakeup = WIFI_WAKEUP;
		}
		else
		{
			system_wakeup_param.wifi_bt_wakeup = BT_WAKEUP;
		}

		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE, &system_wakeup_param);
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
		bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_USBPLUG, &usbplug_wakeup_param);
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
			#if 1 && (CONFIG_CPU_CNT > 1)
				stop_cpu1_core();
			#endif
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

	bk_pm_sleep_mode_set(pm_sleep_mode);

	pm_printf_current_temperature();

}
static void cli_pm_debug(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_debug  = 0;
	UINT32 aon_flush = 0;

	if ((argc != 2) && (argc != 3))
	{
		BK_LOGD(NULL,"set low power debug parameter invalid %d\r\n",argc);
		return;
	}

	pm_debug = os_strtoul(argv[1], NULL, 10);
	if (argc == 3)
	{
		aon_flush = os_strtoul(argv[2], NULL, 10);
	}

	pm_debug_ctrl(pm_debug);

	if(pm_debug == PM_DEBUG_CTRL_STATE)
	{
		pm_debug_pwr_clk_state();
		pm_debug_lv_state();
		bk_pm_cpu_freq_dump();

		if(aon_flush == 1)
		{
			pm_debug_lv_aon_flush();
		}
	}
	/*for temp debug*/
	if(pm_debug == 16)
	{

	}

	if(pm_debug == 32)
	{

	}
#endif
}
static void cli_pm_vote_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_sleep_mode   = 0;
	UINT32 pm_vote         = 0;
	UINT32 pm_vote_value   = 0;
	UINT32 pm_sleep_time   = 0;
	if (argc != 5)
	{
		BK_LOGD(NULL,"set low power vote parameter invalid %d\r\n",argc);
		return;
	}
	pm_sleep_mode        = os_strtoul(argv[1], NULL, 10);
	pm_vote              = os_strtoul(argv[2], NULL, 10);
	pm_vote_value        = os_strtoul(argv[3], NULL, 10);
	pm_sleep_time        = os_strtoul(argv[4], NULL, 10);
	if((pm_sleep_mode > PM_MODE_DEFAULT)|| (pm_vote > PM_SLEEP_MODULE_NAME_MAX)||(pm_vote_value > 1))
	{
		BK_LOGD(NULL,"set low power vote parameter value  invalid\r\n");
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
	pm_printf_current_temperature();
}
#if 1//CONFIG_DEBUG_VERSION
static void cli_pm_vol(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_vol  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL,"set pm voltage parameter invalid %d\r\n",argc);
		return;
	}

	pm_vol = os_strtoul(argv[1], NULL, 10);
	if ((pm_vol < 0) || (pm_vol > 7))
	{
		BK_LOGD(NULL,"set pm voltage value invalid %d\r\n",pm_vol);
		return;
	}

	bk_pm_lp_vol_set(pm_vol);

}
static void cli_pm_clk(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_clk_state  = 0;
	UINT32 pm_module_id  = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL,"set pm clk parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_clk_state = os_strtoul(argv[2], NULL, 10);
	if ((pm_clk_state < 0) || (pm_clk_state > 1) || (pm_module_id < 0) || (pm_module_id > PM_CLK_ID_NONE))
	{
		BK_LOGD(NULL,"set pm clk value invalid %d %d\r\n",pm_clk_state,pm_module_id);
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
		BK_LOGD(NULL,"set pm power parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	pm_power_state = os_strtoul(argv[2], NULL, 10);
	if (pm_power_state > 1)
	{
		BK_LOGD(NULL,"set pm power value invalid %d %d \r\n",pm_power_state,pm_module_id);
		return;
	}
	bk_printf("cli_pm_power: module: %d, power_state: %d\r\n", pm_module_id, pm_power_state);
	bk_pm_module_vote_power_ctrl(pm_module_id,pm_power_state);

}

#define CLI_PM_FREQ_RANDOM_TEST_COUNT      (64)
#define CLI_PM_FREQ_RANDOM_TEST_MAX_COUNT  (1000000)

static const pm_cpu_freq_e s_cli_pm_cpu_freq_test_list[] = {
	PM_CPU_FRQ_XTAL,
	PM_CPU_FRQ_60M,
	PM_CPU_FRQ_80M,
	PM_CPU_FRQ_120M,
	PM_CPU_FRQ_160M,
	PM_CPU_FRQ_240M,
};

static uint32_t s_cli_pm_freq_rand_seed = 0x12345678;

static uint32_t cli_pm_freq_soft_rand(void)
{
	s_cli_pm_freq_rand_seed = s_cli_pm_freq_rand_seed * 1103515245 + 12345;
	return s_cli_pm_freq_rand_seed;
}

static int cli_pm_cpu_freq_from_reg(pm_cpu_freq_e *cpu_freq, uint32_t *freq_reg)
{
	uint32_t reg = sys_drv_all_modules_clk_div_get(CLK_DIV_REG0);
	uint32_t cksel_core = reg & 0x3;
	uint32_t ckdiv_core = (reg >> 2) & 0xF;
	uint32_t cp0_div = ckdiv_core + 1;

	if(freq_reg != NULL)
	{
		*freq_reg = reg;
	}

	switch(cksel_core)
	{
		case 0:
			*cpu_freq = PM_CPU_FRQ_XTAL;
			return 0;

		case 1:
			if(cp0_div == 4)
			{
				*cpu_freq = PM_CPU_FRQ_60M;
				return 0;
			}
			else if(cp0_div == 3)
			{
				*cpu_freq = PM_CPU_FRQ_80M;
				return 0;
			}
			else if(cp0_div == 2)
			{
				*cpu_freq = PM_CPU_FRQ_120M;
				return 0;
			}
			break;

		case 2:
			if(cp0_div == 2)
			{
				*cpu_freq = PM_CPU_FRQ_160M;
				return 0;
			}
			break;

		case 3:
			if(cp0_div == 2)
			{
				*cpu_freq = PM_CPU_FRQ_240M;
				return 0;
			}
			else if(cp0_div == 4)
			{
				*cpu_freq = PM_CPU_FRQ_120M;
				return 0;
			}
			else if(cp0_div == 6)
			{
				*cpu_freq = PM_CPU_FRQ_80M;
				return 0;
			}
			else if(cp0_div == 8)
			{
				*cpu_freq = PM_CPU_FRQ_60M;
				return 0;
			}
			break;

		default:
			break;
	}

	return -1;
}

static pm_cpu_freq_e cli_pm_cpu_freq_expected_freq(pm_cpu_freq_e cpu_freq)
{
	if(cpu_freq == PM_CPU_FRQ_HIGHEST)
	{
		return (pm_cpu_freq_e)CONFIG_PM_CPU_FRQ_HIGHEST;
	}

	return cpu_freq;
}

static bk_err_t cli_pm_check_cpu_freq(pm_cpu_freq_e expected_freq)
{
	pm_cpu_freq_e reg_freq = 0;
	uint32_t freq_reg = 0;

	expected_freq = cli_pm_cpu_freq_expected_freq(expected_freq);
	if(cli_pm_cpu_freq_from_reg(&reg_freq, &freq_reg) != 0)
	{
		BK_LOGD(NULL,"pm cpu freq check error: unknown freq reg: 0x%x\r\n", freq_reg);
		return BK_FAIL;
	}

	if(reg_freq == expected_freq)
	{
		BK_LOGD(NULL,"pm cpu freq check correct: reg freq: %d, expected freq: %d, reg: 0x%x\r\n",
			reg_freq, expected_freq, freq_reg);
		return BK_OK;
	}

	BK_LOGD(NULL,"pm cpu freq check error: reg freq: %d, expected freq: %d, reg: 0x%x\r\n",
		reg_freq, expected_freq, freq_reg);
	return BK_FAIL;
}

static int cli_pm_is_dec_string(const char *str)
{
	if((str == NULL) || (*str == '\0'))
		return 0;

	while(*str != '\0')
	{
		if((*str < '0') || (*str > '9'))
			return 0;

		str++;
	}

	return 1;
}

typedef struct {
	uint32_t total;
	uint32_t success;
	uint32_t fail;
} cli_pm_freq_test_stat_t;

static void cli_pm_freq_test_stat_update(cli_pm_freq_test_stat_t *stat, bk_err_t ret)
{
	stat->total++;
	if(ret == BK_OK)
	{
		stat->success++;
	}
	else
	{
		stat->fail++;
	}
}

static void cli_pm_freq_test_stat_dump(const char *test_name, const cli_pm_freq_test_stat_t *stat)
{
	BK_LOGD(NULL,"pm cpu freq %s test stat: total: %d, success: %d, fail: %d\r\n",
		test_name, stat->total, stat->success, stat->fail);
}

static bk_err_t cli_pm_vote_cpu_freq_once(UINT32 pm_module_id, pm_cpu_freq_e pm_freq)
{
	bk_err_t ret;
	bk_err_t check_ret = BK_OK;
	pm_cpu_freq_e module_freq = 0;
	pm_cpu_freq_e current_max_freq = 0;

	GPIO_UP(36);
	GPIO_DOWN(36);
	ret = bk_pm_module_vote_cpu_freq((pm_dev_id_e)pm_module_id, pm_freq);
	GPIO_UP(36);
	GPIO_DOWN(36);

	module_freq = bk_pm_module_current_cpu_freq_get((pm_dev_id_e)pm_module_id);
	current_max_freq = bk_pm_current_max_cpu_freq_get();

	BK_LOGD(NULL,"pm cpu freq test id: %d; vote freq: %d; module freq: %d; current max cpu freq: %d; ret: %d\r\n",
		pm_module_id, pm_freq, module_freq, current_max_freq, ret);
	if(ret == BK_OK)
	{
		check_ret = cli_pm_check_cpu_freq(current_max_freq);
	}

	return (ret == BK_OK) ? check_ret : ret;
}

static void cli_pm_freq_test_up(UINT32 pm_module_id)
{
	uint32_t i;
	bk_err_t ret;
	cli_pm_freq_test_stat_t stat = {0};

	BK_LOGD(NULL,"pm cpu freq up test start, module id: %d\r\n", pm_module_id);
	for(i = 0; i < sizeof(s_cli_pm_cpu_freq_test_list) / sizeof(s_cli_pm_cpu_freq_test_list[0]); i++)
	{
		ret = cli_pm_vote_cpu_freq_once(pm_module_id, s_cli_pm_cpu_freq_test_list[i]);
		cli_pm_freq_test_stat_update(&stat, ret);
		if(ret != BK_OK)
		{
			BK_LOGD(NULL,"pm cpu freq up test fail at index: %d\r\n", i);
		}
	}
	cli_pm_freq_test_stat_dump("up", &stat);
	BK_LOGD(NULL,"pm cpu freq up test done\r\n");
}

static void cli_pm_freq_test_down(UINT32 pm_module_id)
{
	int32_t i;
	bk_err_t ret;
	cli_pm_freq_test_stat_t stat = {0};

	BK_LOGD(NULL,"pm cpu freq down test start, module id: %d\r\n", pm_module_id);
	for(i = (int32_t)(sizeof(s_cli_pm_cpu_freq_test_list) / sizeof(s_cli_pm_cpu_freq_test_list[0])) - 1; i >= 0; i--)
	{
		ret = cli_pm_vote_cpu_freq_once(pm_module_id, s_cli_pm_cpu_freq_test_list[i]);
		cli_pm_freq_test_stat_update(&stat, ret);
		if(ret != BK_OK)
		{
			BK_LOGD(NULL,"pm cpu freq down test fail at index: %d\r\n", i);
		}
	}
	cli_pm_freq_test_stat_dump("down", &stat);
	BK_LOGD(NULL,"pm cpu freq down test done\r\n");
}

static void cli_pm_freq_test_random(UINT32 pm_module_id, uint32_t test_count)
{
	uint32_t i;
	uint32_t rand_num;
	uint32_t freq_count = sizeof(s_cli_pm_cpu_freq_test_list) / sizeof(s_cli_pm_cpu_freq_test_list[0]);
	bk_err_t ret;
	cli_pm_freq_test_stat_t stat = {0};

	BK_LOGD(NULL,"pm cpu freq random test start, module id: %d, count: %d\r\n", pm_module_id, test_count);
	s_cli_pm_freq_rand_seed ^= (pm_module_id << 16) ^ test_count;

	for(i = 0; i < test_count; i++)
	{
		rand_num = cli_pm_freq_soft_rand() % freq_count;
		ret = cli_pm_vote_cpu_freq_once(pm_module_id, s_cli_pm_cpu_freq_test_list[rand_num]);
		cli_pm_freq_test_stat_update(&stat, ret);
		if(ret != BK_OK)
		{
			BK_LOGD(NULL,"pm cpu freq random test fail at index: %d, rand: %d\r\n", i, rand_num);
		}
	}
	cli_pm_freq_test_stat_dump("random", &stat);
	BK_LOGD(NULL,"pm cpu freq random test done\r\n");
}

typedef struct {
	pm_dev_id_e module;
	pm_cpu_freq_e freq;
} cli_pm_freq_multi_vote_t;

static const cli_pm_freq_multi_vote_t s_cli_pm_freq_multi_votes[][4] = {
	{
		{PM_DEV_ID_TIMER_0, PM_CPU_FRQ_60M},
		{PM_DEV_ID_I2C1, PM_CPU_FRQ_80M},
		{PM_DEV_ID_SPI_1, PM_CPU_FRQ_120M},
		{PM_DEV_ID_UART1, PM_CPU_FRQ_240M},
	},
	{
		{PM_DEV_ID_TIMER_0, PM_CPU_FRQ_240M},
		{PM_DEV_ID_I2C1, PM_CPU_FRQ_60M},
		{PM_DEV_ID_SPI_1, PM_CPU_FRQ_80M},
		{PM_DEV_ID_UART1, PM_CPU_FRQ_160M},
	},
	{
		{PM_DEV_ID_TIMER_0, PM_CPU_FRQ_80M},
		{PM_DEV_ID_I2C1, PM_CPU_FRQ_240M},
		{PM_DEV_ID_SPI_1, PM_CPU_FRQ_60M},
		{PM_DEV_ID_UART1, PM_CPU_FRQ_160M},
	},
	{
		{PM_DEV_ID_TIMER_0, PM_CPU_FRQ_160M},
		{PM_DEV_ID_I2C1, PM_CPU_FRQ_120M},
		{PM_DEV_ID_SPI_1, PM_CPU_FRQ_240M},
		{PM_DEV_ID_UART1, PM_CPU_FRQ_80M},
	},
};

static pm_cpu_freq_e cli_pm_freq_multi_expected_freq(const cli_pm_freq_multi_vote_t *votes, uint32_t vote_count)
{
	uint32_t i;
	pm_cpu_freq_e expected_freq = PM_CPU_FRQ_XTAL;

	for(i = 0; i < vote_count; i++)
	{
		if(expected_freq < votes[i].freq)
		{
			expected_freq = votes[i].freq;
		}
	}

	return expected_freq;
}

static bk_err_t cli_pm_freq_multi_run_one(uint32_t index, const cli_pm_freq_multi_vote_t *votes, uint32_t vote_count)
{
	uint32_t i;
	bk_err_t ret = BK_OK;
	pm_cpu_freq_e expected_freq = cli_pm_freq_multi_expected_freq(votes, vote_count);
	pm_cpu_freq_e current_max_freq = 0;

	BK_LOGD(NULL,"pm cpu freq multi test case %d start, expected max freq: %d\r\n", index, expected_freq);
	for(i = 0; i < vote_count; i++)
	{
		ret = bk_pm_module_vote_cpu_freq(votes[i].module, votes[i].freq);
		BK_LOGD(NULL,"pm cpu freq multi vote case: %d, index: %d, module: %d, freq: %d, ret: %d\r\n",
			index, i, votes[i].module, votes[i].freq, ret);
		if(ret != BK_OK)
		{
			return ret;
		}
	}

	current_max_freq = cli_pm_cpu_freq_expected_freq(bk_pm_current_max_cpu_freq_get());
	if(current_max_freq != expected_freq)
	{
		BK_LOGD(NULL,"pm cpu freq multi check error: current max freq: %d, expected max freq: %d\r\n",
			current_max_freq, expected_freq);
		return BK_FAIL;
	}

	ret = cli_pm_check_cpu_freq(expected_freq);
	if(ret == BK_OK)
	{
		BK_LOGD(NULL,"pm cpu freq multi check correct: case: %d, expected max freq: %d\r\n",
			index, expected_freq);
	}
	else
	{
		BK_LOGD(NULL,"pm cpu freq multi check error: case: %d, expected max freq: %d\r\n",
			index, expected_freq);
	}

	return ret;
}

static void cli_pm_freq_test_multi(void)
{
	uint32_t i;
	bk_err_t ret;
	cli_pm_freq_test_stat_t stat = {0};
	uint32_t case_count = sizeof(s_cli_pm_freq_multi_votes) / sizeof(s_cli_pm_freq_multi_votes[0]);

	BK_LOGD(NULL,"pm cpu freq multi test start, case count: %d\r\n", case_count);
	for(i = 0; i < case_count; i++)
	{
		ret = cli_pm_freq_multi_run_one(i, s_cli_pm_freq_multi_votes[i],
			sizeof(s_cli_pm_freq_multi_votes[i]) / sizeof(s_cli_pm_freq_multi_votes[i][0]));
		cli_pm_freq_test_stat_update(&stat, ret);
	}
	cli_pm_freq_test_stat_dump("multi", &stat);
	BK_LOGD(NULL,"pm cpu freq multi test done\r\n");
}

static void cli_pm_freq(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_freq  = 0;
	UINT32 pm_module_id  = 0;
	uint32_t test_count = CLI_PM_FREQ_RANDOM_TEST_COUNT;
	bk_err_t ret;
	cli_pm_freq_test_stat_t stat = {0};

	if ((argc < 2) || (argc > 4))
	{
		BK_LOGD(NULL,"set pm freq parameter invalid %d\r\n",argc);
		return;
	}

	if (os_strcmp(argv[1], "multi") == 0)
	{
		if(argc != 2)
		{
			BK_LOGD(NULL,"set pm freq multi parameter invalid %d\r\n",argc);
			return;
		}

		cli_pm_freq_test_multi();
		return;
	}

	if ((argc != 3) && (argc != 4))
	{
		BK_LOGD(NULL,"set pm freq parameter invalid %d\r\n",argc);
		return;
	}

	pm_module_id = os_strtoul(argv[1], NULL, 10);
	if (pm_module_id >= PM_DEV_ID_MAX)
	{
		BK_LOGD(NULL,"set pm freq module invalid %d\r\n",pm_module_id);
		return;
	}

	if (os_strcmp(argv[2], "up") == 0)
	{
		if(argc != 3)
		{
			BK_LOGD(NULL,"set pm freq up parameter invalid %d\r\n",argc);
			return;
		}

		cli_pm_freq_test_up(pm_module_id);
		return;
	}
	else if (os_strcmp(argv[2], "down") == 0)
	{
		if(argc != 3)
		{
			BK_LOGD(NULL,"set pm freq down parameter invalid %d\r\n",argc);
			return;
		}

		cli_pm_freq_test_down(pm_module_id);
		return;
	}
	else if (os_strcmp(argv[2], "random") == 0)
	{
		if(argc == 4)
		{
			if(!cli_pm_is_dec_string(argv[3]))
			{
				BK_LOGD(NULL,"set pm freq random count invalid %s\r\n", argv[3]);
				return;
			}

			test_count = os_strtoul(argv[3], NULL, 10);
			if((test_count == 0) || (test_count > CLI_PM_FREQ_RANDOM_TEST_MAX_COUNT))
			{
				BK_LOGD(NULL,"set pm freq random count invalid %d\r\n", test_count);
				return;
			}
		}

		cli_pm_freq_test_random(pm_module_id, test_count);
		return;
	}

	if((argc != 3) || !cli_pm_is_dec_string(argv[2]))
	{
		BK_LOGD(NULL,"set pm freq value invalid %s\r\n", argv[2]);
		return;
	}

	pm_freq = os_strtoul(argv[2], NULL, 10);
	if (pm_freq > PM_CPU_FRQ_DEFAULT)
	{
		BK_LOGD(NULL,"set pm freq value invalid %d %d \r\n",pm_freq,pm_module_id);
		return;
	}

	ret = cli_pm_vote_cpu_freq_once(pm_module_id, (pm_cpu_freq_e)pm_freq);
	cli_pm_freq_test_stat_update(&stat, ret);
	cli_pm_freq_test_stat_dump("single", &stat);
}
static void cli_pm_lpo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if 1
	UINT32 pm_lpo  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL,"set pm lpo parameter invalid %d\r\n",argc);
		return;
	}

	pm_lpo = os_strtoul(argv[1], NULL, 10);
	if ((pm_lpo < 0) || (pm_lpo > 3))
	{
		BK_LOGD(NULL,"set  pm lpo value invalid %d\r\n",pm_lpo);
		return;
	}

	bk_pm_lpo_src_set(pm_lpo);
#endif
}
static void cli_pm_ctrl(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_ctrl  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL,"set pm ctrl parameter invalid %d\r\n",argc);
		return;
	}

	pm_ctrl = os_strtoul(argv[1], NULL, 10);
	if ((pm_ctrl < 0) || (pm_ctrl > 1))
	{
		BK_LOGD(NULL,"set pm ctrl value invalid %d\r\n",pm_ctrl);
		return;
	}

	bk_pm_mcu_pm_ctrl(pm_ctrl);

}
static void cli_pm_pwr_state(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_pwr_module        = 0;
	UINT32 pm_pwr_module_state  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL,"set pm pwr state parameter invalid %d\r\n",argc);
		return;
	}

	pm_pwr_module = os_strtoul(argv[1], NULL, 10);
	if ((pm_pwr_module < 0) || (pm_pwr_module >= PM_POWER_MODULE_NAME_NONE))
	{
		BK_LOGD(NULL,"pm module[%d] not support ,get power state fail\r\n",pm_pwr_module);
		return;
	}

	pm_pwr_module_state = bk_pm_module_power_state_get(pm_pwr_module);
	BK_LOGD(NULL,"Get module[%d] power state[%d] \r\n",pm_pwr_module,pm_pwr_module_state);

}
static void cli_pm_auto_vote(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 pm_ctrl  = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL,"set pm auto_vote parameter invalid %d\r\n",argc);
		return;
	}

	pm_ctrl = os_strtoul(argv[1], NULL, 10);
	if ((pm_ctrl < 0) || (pm_ctrl > 1))
	{
		BK_LOGD(NULL,"set pm auto vote value invalid %d\r\n",pm_ctrl);
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
		BK_LOGD(NULL,"set dvfs parameter invalid %d\r\n",argc);
		return;
	}

	GLOBAL_INT_DECLARATION();
	cksel_core   = os_strtoul(argv[1], NULL, 10);
	ckdiv_core   = os_strtoul(argv[2], NULL, 10);
	ckdiv_bus    = os_strtoul(argv[3], NULL, 10);
	ckdiv_cpu0   = os_strtoul(argv[4], NULL, 10);
	ckdiv_cpu1   = os_strtoul(argv[5], NULL, 10);

	BK_LOGD(NULL,"cli_dvfs_cmd %d %d %d %d %d !!! \r\n",
				cksel_core,
				ckdiv_core,
				ckdiv_bus,
				ckdiv_cpu0,
				ckdiv_cpu1);
	GLOBAL_INT_DISABLE();
	if(cksel_core > 3)
	{
		BK_LOGD(NULL,"set dvfs cksel core > 3 invalid %d\r\n",cksel_core);
		GLOBAL_INT_RESTORE();
		return;
	}

	if((ckdiv_core > CLI_DVFS_FREQUNCY_DIV_MAX) || (ckdiv_bus > CLI_DVFS_FREQUNCY_DIV_BUS_MAX)||(ckdiv_cpu0 > CLI_DVFS_FREQUNCY_DIV_MAX)||(ckdiv_cpu0 > CLI_DVFS_FREQUNCY_DIV_MAX))
	{
		BK_LOGD(NULL,"set dvfs ckdiv_core ckdiv_bus ckdiv_cpu0  ckdiv_cpu0  > 15 invalid\r\n");
		GLOBAL_INT_RESTORE();
		return;
	}
	pm_core_bus_clock_ctrl(cksel_core, ckdiv_core,ckdiv_bus, ckdiv_cpu0,ckdiv_cpu1);
	GLOBAL_INT_RESTORE();
	BK_LOGD(NULL,"switch cpu frequency ok 0x%x 0x%x 0x%x\r\n",sys_drv_all_modules_clk_div_get(CLK_DIV_REG0),sys_drv_cpu_clk_div_get(0),sys_drv_cpu_clk_div_get(1));
}

#if CONFIG_AON_RTC
static UINT32 s_pre_tick;
static void cli_pm_timer_isr(timer_id_t chan)
{
	UINT32 current_tick = 0;
	double current_freq = 0;
	INT32 current_ppm = 0;
	INT32 current_delta = 0;

#if CONFIG_CKMN
	current_freq = bk_rosc_32k_get_freq();
	current_ppm = bk_rosc_32k_get_ppm();
#endif
	current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	current_delta = current_tick - s_pre_tick;

	BK_LOGD(NULL,"rosc %d %8.3f %d %d\r\n",
						current_delta,
						current_freq,
						current_ppm,
						current_tick);
	s_pre_tick = current_tick;
}
#if CONFIG_CKMN
static uint32_t prog_intval = 32;
static void cli_pm_rosc_timer_isr(timer_id_t chan)
{
	bk_rosc_32k_ckest_prog(prog_intval);
}
#endif
#endif
static void cli_pm_rosc_accuracy(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_AON_RTC
	UINT32 timer_count_interval = 0;

	if (argc != 2)
	{
		BK_LOGD(NULL,"set rosc_accuracy parameter invalid %d\r\n",argc);
		return;
	}

	timer_count_interval   = os_strtoul(argv[1], NULL, 10);
	bk_timer_start(TIMER_ID1, timer_count_interval, cli_pm_timer_isr);
#endif
}
static void cli_pm_rosc_cali(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 cali_interval = 0;
	UINT32 cali_mode = 0;

	if (argc < 3)
	{
		BK_LOGD(NULL,"set rosc cali parameter invalid %d\r\n",argc);
		return;
	}

	cali_mode   = os_strtoul(argv[1], NULL, 10);
	cali_interval   = os_strtoul(argv[2], NULL, 10);
	if (cali_mode == 4) {
#if CONFIG_CKMN
		prog_intval = os_strtoul(argv[3], NULL, 10);
		bk_timer_start(1, cali_interval, cli_pm_rosc_timer_isr);
#endif
	} else {
		bk_pm_rosc_calibration(cali_mode, cali_interval);
	}
}
static void cli_pm_clk_pin(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 lpo_clk = 0;
	UINT32 lpo_digital;
	UINT32 reg;
	//dco_cali_speed_e lpo_clk;
	#define SYSTEM_BASE_ADDR    (0x44010000)
	#define AON_GPIO_BASE_ADDR  (0x44000400)
	#define AON_PMU_BASE_ADDR   (0x44000000)

	if (argc < 3)
	{
		BK_LOGD(NULL,"parameter invalid %d\r\n", argc);
		return;
	}

	// lpo_clk      0:rosc    1:xtall    2:dco
	// lpo_digital  0:analog  1:digital
	lpo_clk     = os_strtoul(argv[1], NULL, 10);
	lpo_digital = os_strtoul(argv[2], NULL, 10);

	if (lpo_digital == 1) //gpio24 output digital clock
	{
	}
	else  //gpio24 output analog clock
	{
		REG_WRITE((AON_GPIO_BASE_ADDR + 0x18 * 4), 0x8); // 0x44000460

		// system clock test enable
		reg = REG_READ(SYSTEM_BASE_ADDR + 0x44 * 4);     // 0x44010110
		reg |= BIT(22);
		REG_WRITE((SYSTEM_BASE_ADDR + 0x44 * 4), reg);   // 0x44010110

		if (lpo_clk == 0) { // output rosc clock

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(21);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(12);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114
			BK_LOGD(NULL,"gpio 24 output analog rosc 32k\r\n");

		} else if (lpo_clk == 1) { // output xtall clock
			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(1);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(20);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114
			BK_LOGD(NULL,"gpio 24 output analog xtall 32.768k\r\n");

		}else if (lpo_clk == 2) {  // output dco clock
			reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
			reg &= ~ BIT(26);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x45 * 4);     // 0x44010114
			reg |= BIT(2);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x45 * 4), reg);   // 0x44010114

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
			reg |= (0x3 << 11);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104

			reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
			reg |= (0x127 << 16);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104
			// frequency division 3
			reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
			reg |= (0x2 << 27);
			REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104

			for(int i = 0; i < 2; i++)
			{
				reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
				reg &= ~ BIT(15);
				REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104
				reg = REG_READ(SYSTEM_BASE_ADDR + 0x41 * 4);     // 0x44010104
				reg |= BIT(15);
				REG_WRITE((SYSTEM_BASE_ADDR + 0x41 * 4), reg);   // 0x44010104
			}
			BK_LOGD(NULL,"gpio 24 output analog DCO 20M digital core 80M.\r\n");
		}
		//clock test signal selection rosc/xtall/dco
		reg = REG_READ(SYSTEM_BASE_ADDR + 0x44 * 4);     // 0x44010110
		reg |= (lpo_clk << 20);
		REG_WRITE((SYSTEM_BASE_ADDR + 0x44 * 4), reg);   // 0x44010110

	}
	
}
static void cli_pm_wakeup_source(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 sleep_mode = 0;

	if (argc != 2)
	{
		BK_LOGD(NULL,"set get wakeup source parameter invalid %d\r\n",argc);
		return;
	}

	sleep_mode   = os_strtoul(argv[1], NULL, 10);
	if(sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		#if 1
		BK_LOGD(NULL,"low voltage wakeup source [%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get());
		#endif
	}
	else if(sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		#if 1
		BK_LOGD(NULL,"deepsleep wakeup source [%d]\r\n",bk_pm_deep_sleep_wakeup_source_get());
		#endif
	}
	else
	{
		BK_LOGD(NULL,"it not support the sleep mode[%d] for wakeup source \r\n",sleep_mode);
	}

}

static void cli_pm_rosc_ppm(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if 1
	UINT32 timer_interval, count= 0;
	INT32  ppm_val;

	if (argc < 3)
	{
		BK_LOGD(NULL,"parameter invalid %d\r\n", argc);
		return;
	}

	timer_interval = os_strtoul(argv[1], NULL, 10);
	count          = os_strtoul(argv[2], NULL, 10);

	bk_rosc_ppm_statistic_start(TIMER_ID1, timer_interval, count);
	rtos_delay_milliseconds(timer_interval * count + 100);
	bk_rosc_ppm_statistics_get(&ppm_val);
	BK_LOGD(NULL,"ppm val = %d\r\n", ppm_val);

	return ;
#endif
}

static void cli_pm_boot_ap(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if 1 && (CONFIG_CPU_CNT > 1)
	UINT32 boot_ap_state = 0;
	UINT32 module_name    = 0;

	if ((argc >= 2) && (os_strcmp(argv[1], "stress") == 0))
	{
		cli_pm_boot_ap_stress(pcWriteBuffer, xWriteBufferLen, argc, argv);
		return;
	}

	if (argc != 3)
	{
		BK_LOGD(NULL,"usage: pm_boot_ap [module_name] [ctrl_state:0:on 1:off]\r\n");
		BK_LOGD(NULL,"usage: pm_boot_ap stress [start_module] [task_count] [loop_count] [on_hold_ms] [off_hold_ms]\r\n");
		return;
	}
	module_name   = os_strtoul(argv[1], NULL, 10);
	boot_ap_state   = os_strtoul(argv[2], NULL, 10);
	bk_pm_module_vote_boot_ap_ctrl(module_name,boot_ap_state);
#endif
}
#if (CONFIG_CPU_CNT > 2)
static void cli_pm_boot_cp2(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if 1
	UINT32 boot_cp2_state = 0;
	UINT32 module_name    = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL,"cp2 ctrl parameter invalid %d\r\n",argc);
		return;
	}
	module_name   = os_strtoul(argv[1], NULL, 10);
	boot_cp2_state   = os_strtoul(argv[2], NULL, 10);
	bk_pm_module_vote_boot_cp2_ctrl(module_name,boot_cp2_state);
#endif
}
#endif//CONFIG_CPU_CNT > 2
static void cli_pm_ldo(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
//#if 1 && (CONFIG_CPU_CNT > 1)
	UINT32 gpio_id                = 0;
	UINT32 module_name            = 0;
	UINT32 gpio_output_state_e    = 0;
	if (argc != 4)
	{
		BK_LOGD(NULL,"cp1 ctrl parameter invalid %d\r\n",argc);
		return;
	}
	module_name           = os_strtoul(argv[1], NULL, 10);
	gpio_id               = os_strtoul(argv[2], NULL, 10);
	gpio_output_state_e   = os_strtoul(argv[3], NULL, 10);
	bk_pm_module_vote_ctrl_external_ldo(module_name,gpio_id,gpio_output_state_e);
//#endif
}
static void cli_pm_psram(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
#if CONFIG_PSRAM
	UINT32 module_name            = 0;
	UINT32 power_psram_state      = 0;
	if (argc != 3)
	{
		BK_LOGD(NULL,"psram ctrl parameter invalid %d\r\n",argc);
		return;
	}
	module_name         = os_strtoul(argv[1], NULL, 10);
	power_psram_state   = os_strtoul(argv[2], NULL, 10);
	bk_pm_module_vote_psram_ctrl(module_name,power_psram_state);
#endif
}
#endif//CONFIG_DEBUG_VERSION

#if CONFIG_BUCK_ENABLE
/**
 * BK7236XX use buck power supply as default and should not close buck any time during use.
 * This function can only be used when buck power supply is unstable (hardware v4).
 * Please remove it when buck is stable (next hardware version).
*/
extern void sys_hal_buck_switch(uint32_t flag);
static void cli_pm_buck(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	UINT32 flag = 0;
	if (argc != 2)
	{
		BK_LOGD(NULL,"set buck power supply invalid %d\r\n",argc);
		return;
	}

	flag = os_strtoul(argv[1], NULL, 10);

	sys_hal_buck_switch(flag);
}
#endif //CONFIG_BUCK_ENABLE

#define PWR_CMD_CNT (sizeof(s_pwr_commands) / sizeof(struct cli_command))
static const struct cli_command s_pwr_commands[] = {
#if 1//CONFIG_SYSTEM_CTRL
#if 1//CONFIG_DEBUG_VERSION
	{"pm", "pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]", cli_pm_cmd},
	{"dvfs", "dvfs [cksel_core] [ckdiv_core] [ckdiv_bus] [ckdiv_cpu0] [ckdiv_cpu1]", cli_dvfs_cmd},
	{"pm_vote", "pm_vote [pm_sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]", cli_pm_vote_cmd},
	{"pm_debug", "pm_debug [debug_en_value]", cli_pm_debug},
	{"pm_lpo", "pm_lpo [lpo_type]", cli_pm_lpo},
	{"pm_vol", "pm_vol [vol_value]", cli_pm_vol},
	{"pm_clk", "pm_clk [module_name][clk_state]", cli_pm_clk},
	{"pm_power", "pm_power [module_name][ power state]", cli_pm_power},
	{"pm_freq", "pm_freq [module_name] [frequency|up|down|random] [random_count] | pm_freq multi", cli_pm_freq},
	{"pm_ctrl", "pm_ctrl [ctrl_value]", cli_pm_ctrl},
	{"pm_pwr_state", "pm_pwr_state [pwr_state]", cli_pm_pwr_state},
	{"pm_auto_vote", "pm_auto_vote [auto_vote_value]", cli_pm_auto_vote},
	{"pm_rosc", "pm_rosc [rosc_accuracy_count_interval]", cli_pm_rosc_accuracy},
	{"pm_rosc_cali", "pm_rosc_cali [cali_mode][cal_intval]", cli_pm_rosc_cali},
	{"pm_rosc_pin", "pm_rosc_pin [lpo_clk:0:ana;1:dig]", cli_pm_clk_pin},
	{"pm_wakeup_source", "pm_wakeup_source [pm_sleep_mode]", cli_pm_wakeup_source},
	{"pm_rosc_ppm", "pm_rosc_ppm [interval] [count]", cli_pm_rosc_ppm},
	{"pm_boot_ap", "pm_boot_ap [module_name] [ctrl_state:0:on 1:off] | pm_boot_ap stress [start_module] [task_count] [loop_count] [on_hold_ms] [off_hold_ms]", cli_pm_boot_ap},
#if (CONFIG_CPU_CNT > 2)
	{"pm_boot_cp2", "pm_boot_cp2 [module_name] [ctrl_state:0x0:bootup; 0x1:shutdowm]", cli_pm_boot_cp2},
#endif
	{"pm_ldo", "pm_ldo[module_name][gpio id][gpio_output_state:0x0->low voltage, 0x1->high voltage]", cli_pm_ldo},
	{"pm_psram", "pm_psram[module_name][ctrl_state:0x0:power&clk on; 0x1:power&clk off]", cli_pm_psram},
#if CONFIG_BUCK_ENABLE  //temp mofify
	{"pm_buck", "pm_buck [1/0]", cli_pm_buck},
#endif

#else
	{"pm", "pm [sleep_mode] [wake_source] [vote1] [vote2] [vote3] [param1] [param2] [param3]", cli_pm_cmd},
	{"pm_vote", "pm_vote [pm_sleep_mode] [pm_vote] [pm_vote_value] [pm_sleep_time]", cli_pm_vote_cmd},
	{"pm_debug", "pm_debug [debug_en_value]", cli_pm_debug},
#endif //CONFIG_DEBUG_VERSION
#endif //CONFIG_SYSTEM_CTRL

};

int cli_pwr_init(void)
{
	return cli_register_commands(s_pwr_commands, PWR_CMD_CNT);
}
