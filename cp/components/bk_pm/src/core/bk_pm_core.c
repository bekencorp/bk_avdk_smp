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
#include <os/str.h>
#include <os/mem.h>
#include "sys_driver.h"
#include "aon_pmu_driver.h"
#include <driver/aon_rtc.h>

#include "pm_power.h"
#include "pm_sleep.h"
#include "pm_wakeup_source.h"
#include "pm_debug.h"
#include "sys_pm_hal_debug.h"


#if CONFIG_PM_PROTECT_TIME_CHECK
#define ENTER_LOW_VOLTAGE_PROTECT_TIME                  (3) // 3ms
#define ENTER_LOW_VOLTAGE_WAKEUP_PROTECT_TIME           (5) // 5m = 3ms(dpll+xtal 26m stability time) + 2ms(mtime interval),mainly for bt wakeup handle interrupt
#endif


static pm_sleep_mode_e s_pm_sleep_mode        = PM_MODE_DEFAULT;

static uint32_t s_pm_on_modules               = 0;
static uint32_t s_pm_off_modules              = 0;
static uint64_t s_pm_sleeped_modules          = 0;

static uint64_t s_pm_enter_normal_sleep_modules = 0;
static uint64_t s_pm_enter_low_vol_modules    = 0;
static uint32_t s_pm_enter_deep_sleep_modules = 0;

#if CONFIG_PM_PROTECT_TIME_CHECK
static uint64_t s_bt_need_wakeup_time         = 0;
static system_wakeup_param_t s_bt_system_wakeup_param;
#endif

static uint64_t pm_state_machine(void);
static uint64_t pm_check_and_ctrl_sleep(void);
static void pm_enter_normal_sleep_modules_config(void);
static void pm_enter_low_vol_modules_config(void);
static void pm_enter_deep_sleep_modules_config(void);
static void pm_core_debug(void);

void pm_hardware_init(void)
{
	sys_drv_low_power_hardware_init();

	/*config vote for entering normal sleep modules*/
	pm_enter_normal_sleep_modules_config();

	/*config vote for entering low vol modules*/
	pm_enter_low_vol_modules_config();

	/*config vote for entering deepsleep modules*/
	pm_enter_deep_sleep_modules_config();

	/*add sleep vote*/
	pm_sleep_vote_init();

	/*set the wakeup wakeup source to misc module*/
	pm_deep_sleep_wakeup_source_set();

#if CONFIG_BAKP_POWER_DOMAIN_PM_CONTROL
	/*pm vote power on ticket for bakp module*/
	//bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_BAKP_PM, PM_POWER_MODULE_STATE_ON);
#endif

	bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_BAKP_PM, PM_POWER_MODULE_STATE_ON);
}

/*=========================SLEEP STATE MACHINE START========================*/
uint64_t pm_management(uint32_t sleep_ticks)
{
	uint64_t missed_ticks = 0ULL;
#if CONFIG_PM_CLIENT
	if (bk_pm_cp1_ctrl_state_get() == 0x0)
	{
		bk_pm_cp1_ctrl_state_set(PM_MAILBOX_COMMUNICATION_FINISH);
		pm_cp1_mailbox_response(PM_CPU1_BOOT_READY_CMD, 0x1);
		// BK_LOGD(NULL, "cpu1 already\r\n");
	}
	missed_ticks = pm_enter_cpu_wfi();
#elif CONFIG_MCU_PS
#if CONFIG_PM_LIGHT_SLEEP
	return pm_light_sleep(sleep_ticks);
#else // CONFIG_PM_LIGHT_SLEEP

	if (bk_pm_mcu_pm_state_get())
	{
		return BK_OK;
	}
	missed_ticks = pm_state_machine();
	if (missed_ticks != 0)
	{
		// bk_update_tick(missed_ticks);
	}
#endif // CONFIG_PM_LIGHT_SLEEP
#else
	missed_ticks = pm_cpu_wfi_process();
#endif
	return missed_ticks;
}

static uint64_t pm_state_machine()
{
	uint64_t missed_ticks       = 0ULL;
	uint64_t sleep_tick         = 0ULL;
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	uint64_t entry_tick         = 0ULL;
	entry_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	#endif

	pm_check_power_on_module(&s_pm_off_modules, &s_pm_on_modules, &s_pm_sleeped_modules);
	pm_wakeup_from_deepsleep_handle();
	pm_core_debug();

	#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	uint64_t exit_tick          = 0ULL;
	exit_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	sleep_tick = exit_tick - entry_tick;
	if(exit_tick - entry_tick < 0)
	{
		sleep_tick = 0ULL;
	}
	else
	{
		sleep_tick = exit_tick - entry_tick;
	}
	#endif
	GLOBAL_INT_RESTORE();

	missed_ticks = pm_check_and_ctrl_sleep() + sleep_tick;

#if PM_SLEEP_TIME_COMPENSATION_ENABLE
	missed_ticks = (missed_ticks) / (bk_rtc_get_ms_tick_count() * (rtos_get_ms_per_tick()));
#endif

	return missed_ticks;
}

static uint64_t pm_check_and_ctrl_sleep()
{
	uint64_t sleep_tick = 0;

	if (s_pm_sleep_mode == PM_MODE_NORMAL_SLEEP)
	{
		if ((s_pm_sleeped_modules & s_pm_enter_normal_sleep_modules) == s_pm_enter_normal_sleep_modules)
		{
			sleep_tick = pm_normal_sleep_process();
			return sleep_tick;
		}
	}
	else if (s_pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		if ((s_pm_sleeped_modules & s_pm_enter_low_vol_modules) == s_pm_enter_low_vol_modules)
		{
			sleep_tick = pm_low_voltage_process();
			return sleep_tick;
		}
	}
	else if (s_pm_sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		if ((s_pm_off_modules & s_pm_enter_deep_sleep_modules) == s_pm_enter_deep_sleep_modules)
		{
			sleep_tick = pm_deep_sleep_process();
			return sleep_tick;
		}
	}
#if CONFIG_PM_SUPER_DEEP_SLEEP
	else if (s_pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if ((s_pm_off_modules & s_pm_enter_deep_sleep_modules) == s_pm_enter_deep_sleep_modules)
		{
			pm_super_deep_sleep_process();
		}
	}
#endif
	else
	{
		if ((s_pm_sleeped_modules & s_pm_enter_low_vol_modules) == s_pm_enter_low_vol_modules)
		{
			sleep_tick = pm_low_voltage_process();
			return sleep_tick;
		}
	}
	sleep_tick = pm_cpu_wfi_process();
	return sleep_tick;
}

static void pm_enter_normal_sleep_modules_config()
{
	uint32_t i = 0;
	pm_sleep_module_name_e enter_normal_sleep_modules[] = PM_ENTER_NORMAL_SLEEP_MODULES_CONFIG;

	for (i = 0; i < sizeof(enter_normal_sleep_modules) / sizeof(pm_sleep_module_name_e); i++)
	{
		s_pm_enter_normal_sleep_modules |= 0x1ULL << enter_normal_sleep_modules[i];
	}
}

static void pm_enter_low_vol_modules_config()
{
	uint32_t i = 0;
	pm_sleep_module_name_e enter_low_vol_modules[] = PM_ENTER_LOW_VOL_MODULES_CONFIG;

	for (i = 0; i < sizeof(enter_low_vol_modules) / sizeof(pm_sleep_module_name_e); i++)
	{
		s_pm_enter_low_vol_modules |= 0x1ULL << enter_low_vol_modules[i];
	}
}

static void pm_enter_deep_sleep_modules_config()
{
	uint32_t i = 0;
	pm_power_module_name_e enter_deep_sleep_modules[] = PM_ENTER_DEEP_SLEEP_MODULES_CONFIG;

	for (i = 0; i < sizeof(enter_deep_sleep_modules) / sizeof(pm_power_module_name_e); i++)
	{
		s_pm_enter_deep_sleep_modules |= 0x1 << enter_deep_sleep_modules[i];
	}
}
/*=========================SLEEP STATE MACHINE END========================*/

/*=========================COMMON PM API START========================*/
bk_err_t bk_pm_sleep_mode_set(pm_sleep_mode_e sleep_mode)
{
	s_pm_sleep_mode = sleep_mode;

	if (s_pm_sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		pm_deep_sleep_prepare();
	}
#if CONFIG_PM_SUPER_DEEP_SLEEP
	else if (s_pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		pm_super_deep_sleep_prepare();
	}
#endif

	return BK_OK;
}

bk_err_t bk_pm_module_vote_power_ctrl(pm_power_module_name_e module, pm_power_module_state_e power_state)
{
	if (power_state == PM_POWER_MODULE_STATE_ON) // power on
	{
		bk_pm_module_power_on(&s_pm_off_modules, &s_pm_on_modules, &s_pm_sleeped_modules, module);
	}
	else // power off
	{
		bk_pm_module_power_off(&s_pm_off_modules, &s_pm_on_modules, &s_pm_sleeped_modules, module);
	}

	pm_check_power_off_module(&s_pm_off_modules, &s_pm_on_modules, &s_pm_sleeped_modules);

	if (pm_debug_mode() & 0x2)
		BK_LOGD(NULL, "cpu0 vote power 0x%X 0x%X\r\n", s_pm_on_modules, s_pm_off_modules);

#if CONFIG_PM_POWER_VOTE_RECORD
	sys_hal_pm_power_vote_record(module, power_state, (uint32_t)__builtin_return_address(0), PM_POWER_DOMAIN_2);
#endif
	return BK_OK;
}

bk_err_t bk_pm_module_vote_sleep_ctrl(pm_sleep_module_name_e module, uint32_t sleep_state, uint32_t sleep_time)
{
	if (module >= PM_SLEEP_MODULE_NAME_MAX)
	{
		return BK_ERR_PARAM;
	}

	if((module == PM_SLEEP_MODULE_NAME_AT) && (sleep_state == PM_POWER_MODULE_STATE_ON))
	{
		return BK_OK;
	}
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	if (sleep_state == 0x1) // enter sleep
	{
		if (module == PM_SLEEP_MODULE_NAME_BTSP)
		{
#if CONFIG_PM_PROTECT_TIME_CHECK
			s_bt_system_wakeup_param.wifi_bt_wakeup = BT_WAKEUP;
			bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE, NULL);
			s_bt_system_wakeup_param.sleep_time = ((sleep_time * bk_rtc_get_ms_tick_count()) / 1000) + 1; // bt provide sleep time convent to 32k tick;+1:protect the precision miss
#if CONFIG_AON_RTC || CONFIG_ANA_RTC
			s_bt_need_wakeup_time = bk_aon_rtc_get_current_tick(AON_RTC_ID_1) + s_bt_system_wakeup_param.sleep_time;
#endif
			/*set bt wakeup source*/
#endif
			sys_drv_enable_bt_wakeup_source();
		}
		else if (module == PM_SLEEP_MODULE_NAME_WIFIP_MAC)
		{
#if CONFIG_PM_PROTECT_TIME_CHECK
			// s_wifi_system_wakeup_param.wifi_bt_wakeup = WIFI_WAKEUP;
			bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE, NULL);
#endif
			sys_drv_enable_mac_wakeup_source();
		}
		else if (module == PM_SLEEP_MODULE_NAME_APP)
		{
			pm_lv_enter_time_out_clear();
		}
		s_pm_sleeped_modules |= 0x1ULL << module;
	}
	else // exit sleep
	{
		if (module == PM_SLEEP_MODULE_NAME_BTSP)
		{
#if CONFIG_PM_PROTECT_TIME_CHECK
			s_bt_system_wakeup_param.wifi_bt_wakeup = BT_WAKEUP;
			s_bt_system_wakeup_param.sleep_time = 0;
			s_bt_need_wakeup_time = 0;
#endif
			/*disable bt wakeup source*/
			aon_pmu_drv_clear_wakeup_source(WAKEUP_SOURCE_INT_BT);
			// BK_LOGD(NULL, "bt exit sleep\r\n");
		}
		else if (module == PM_SLEEP_MODULE_NAME_WIFIP_MAC)
		{
			/*disable WIFI wakeup source*/
			aon_pmu_drv_clear_wakeup_source(WAKEUP_SOURCE_INT_WIFI);
		}
		else if (module == PM_SLEEP_MODULE_NAME_APP)
		{
		}
		else if (module == PM_SLEEP_MODULE_NAME_LOG)
		{
		}
		s_pm_sleeped_modules &= ~(0x1ULL << module);
	}
	if (pm_debug_mode() & 0x1)
		BK_LOGD(NULL, "pm sleep state 0x%llX 0x%x %d\r\n", s_pm_sleeped_modules, s_pm_on_modules, module);
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

uint64_t pm_sleeped_modules_get(void)
{
	return s_pm_sleeped_modules;
}

uint32_t bk_pm_low_vol_vote_state_get(void)
{
	return ((s_pm_sleeped_modules & s_pm_enter_low_vol_modules) == s_pm_enter_low_vol_modules);
}

int32_t bk_pm_module_sleep_state_get(pm_sleep_module_name_e module)
{
	if (module >= PM_SLEEP_MODULE_NAME_MAX)
	{
		return 0;
	}

	return !!(s_pm_sleeped_modules & (0x1ULL << module));
}

bk_err_t bk_pm_clear_deep_sleep_modules_config(pm_power_module_name_e module_name)
{
	if (module_name >= 32)
	{
		return BK_ERR_PARAM;
	}

	s_pm_enter_deep_sleep_modules &= ~(0x1 << module_name);
	return BK_OK;
}
/*=========================COMMON PM API END========================*/

/*=========================PM FEATURE START========================*/
#if CONFIG_PM_LIGHT_SLEEP
uint64_t pm_light_sleep(uint32_t sleep_ticks)
{
	uint64_t previous_tick = 0;
	uint64_t current_tick = 0;
	uint64_t missed_ticks = 0;
	int ret = 0;
	if (bk_pm_mcu_pm_state_get())
	{
		return 0ULL;
	}

	if ((s_pm_light_sleep_enter_cb_conf.cb == NULL) || (s_pm_light_sleep_exit_cb_conf.cb == NULL))
	{

		return 0ULL;
	}
	ret = s_pm_light_sleep_enter_cb_conf.cb(sleep_ticks * rtos_get_ms_per_tick(), s_pm_light_sleep_enter_cb_conf.args);
	if (ret)
	{
		return 0ULL;
	}
	previous_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	current_tick = previous_tick;
	while (((current_tick - previous_tick)) < (sleep_ticks * rtos_get_ms_per_tick() * bk_rtc_get_ms_tick_count()))
	{
		current_tick = bk_aon_rtc_get_current_tick(AON_RTC_ID_1);
	}

	// BK_LOGD(NULL, "idle task after:%d \r\n",sleep_ticks);
	missed_ticks = pm_state_machine();
	missed_ticks = sleep_ticks;
	// bk_update_tick(missed_ticks);

	s_pm_light_sleep_exit_cb_conf.cb(sleep_ticks * rtos_get_ms_per_tick(), s_pm_light_sleep_exit_cb_conf.args);
	return missed_ticks;
}
#endif

#if CONFIG_PM_PROTECT_TIME_CHECK
uint32_t pm_check_protect_time(uint64_t current_tick, uint64_t previous_tick)
{
	if (s_bt_need_wakeup_time > current_tick)
	{
		if ((s_bt_need_wakeup_time - current_tick) < (ENTER_LOW_VOLTAGE_WAKEUP_PROTECT_TIME * bk_rtc_get_ms_tick_count()))
		{
			if (pm_debug_mode() & 0x1)
			{
				BK_LOGD(NULL, "protect_time1 %lld %lld %d\r\n", s_bt_need_wakeup_time, current_tick, s_bt_system_wakeup_param.sleep_time);
			}
			return 0;
		}
	}
	else // s_bt_need_wakeup_time not set, it will be 0,or not clear value after bt wakeup
	{
		if (pm_debug_mode() & 0x1)
		{
			BK_LOGD(NULL, "protect_time2 %lld %lld %d\r\n", s_bt_need_wakeup_time, current_tick, s_bt_system_wakeup_param.sleep_time);
		}
		return 0; // do something
	}
	return 1;
}
#endif
/*=========================PM FEATURE END========================*/

/*=========================DEBUG/TEST CTRL START========================*/
#define PM_SLEEP_MODULES_DIFF_BUF_SIZE    (256)

static void pm_sleep_modules_mask_to_string(char *buf, uint32_t buf_size, uint64_t modules_mask)
{
	uint32_t i;
	char *ptr = buf;
	uint32_t remain = buf_size;
	int len;

	if ((buf == NULL) || (buf_size == 0)) {
		return;
	}

	buf[0] = '\0';

	if (modules_mask == 0) {
		os_snprintf(buf, buf_size, "none");
		return;
	}

	for (i = 0; i < PM_SLEEP_MODULE_NAME_MAX; i++) {
		if (modules_mask & (0x1ULL << i)) {
			len = os_snprintf(ptr, remain, "%s%s", (ptr == buf) ? "" : ",",
				pm_sleep_module_name_to_string((pm_sleep_module_name_e)i));
			if (len <= 0 || (uint32_t)len >= remain) {
				break;
			}
			ptr += len;
			remain -= len;
		}
	}
}

void pm_core_dump(void)
{
	char *not_sleeped_buf = NULL;
	char *extra_sleeped_buf = NULL;
	uint64_t not_sleeped_modules = s_pm_enter_low_vol_modules & ~s_pm_sleeped_modules;
	uint64_t extra_sleeped_modules = s_pm_sleeped_modules & ~s_pm_enter_low_vol_modules;

	not_sleeped_buf = (char *)os_malloc(PM_SLEEP_MODULES_DIFF_BUF_SIZE);
	extra_sleeped_buf = (char *)os_malloc(PM_SLEEP_MODULES_DIFF_BUF_SIZE);
	if ((not_sleeped_buf != NULL) && (extra_sleeped_buf != NULL)) {
		pm_sleep_modules_mask_to_string(not_sleeped_buf, PM_SLEEP_MODULES_DIFF_BUF_SIZE, not_sleeped_modules);
		pm_sleep_modules_mask_to_string(extra_sleeped_buf, PM_SLEEP_MODULES_DIFF_BUF_SIZE, extra_sleeped_modules);
		LOGI("pm low vol[module:0x%llx][need module:0x%llx][not_sleeped:%s][extra_sleeped:%s]\r\n",
			s_pm_sleeped_modules, s_pm_enter_low_vol_modules, not_sleeped_buf, extra_sleeped_buf);
	} else {
		LOGI("pm low vol[module:0x%llx][need module:0x%llx][not_sleeped:malloc_fail][extra_sleeped:malloc_fail]\r\n",
			s_pm_sleeped_modules, s_pm_enter_low_vol_modules);
	}

	if (not_sleeped_buf != NULL) {
		os_free(not_sleeped_buf);
	}
	if (extra_sleeped_buf != NULL) {
		os_free(extra_sleeped_buf);
	}

	LOGI("pm deepsleep[module:0x%x][need module:0x%x]\r\n",s_pm_off_modules,s_pm_enter_deep_sleep_modules);
	LOGI("pm normal sleep[module:0x%llx][need module:0x%llx]\r\n",s_pm_sleeped_modules,s_pm_enter_normal_sleep_modules);
	LOGI("pm normal sleep wakeup src:%d\r\n",bk_pm_sleep_wakeup_reason_get());
}

static void pm_core_debug(void)
{
	if (s_pm_sleep_mode == PM_MODE_NORMAL_SLEEP)
	{
		if (pm_debug_mode() & 0x1)
		{
			LOGI("normal1 0x%X 0x%llX 0x%llX\r\n", s_pm_sleep_mode, s_pm_sleeped_modules, s_pm_enter_normal_sleep_modules);
			// BK_LOGD(NULL, "normal2 0x%X 0x%X 0x%X 0x%X\r\n", s_pm_ahpb_pm_state, s_pm_video_pm_state, s_pm_audio_pm_state, s_pm_bakp_pm_state);
			pm_power_modules_dump_with_sleep_mode(s_pm_sleep_mode);
		}
	}
	else if (s_pm_sleep_mode == PM_MODE_LOW_VOLTAGE)
	{
		if (pm_debug_mode() & 0x1)
		{
			LOGI("lowvol1 0x%X 0x%llX 0x%llX\r\n", s_pm_sleep_mode, s_pm_sleeped_modules, s_pm_enter_low_vol_modules);
			// BK_LOGD(NULL, "lowvol2 0x%X 0x%X 0x%X 0x%X\r\n", s_pm_ahpb_pm_state, s_pm_video_pm_state, s_pm_audio_pm_state, s_pm_bakp_pm_state);
			pm_power_modules_dump_with_sleep_mode(s_pm_sleep_mode);
		}
	}
	else if (s_pm_sleep_mode == PM_MODE_DEEP_SLEEP)
	{
		if (pm_debug_mode() & 0x2)
		{
			LOGI("deepsleep1 0x%X 0x%X\r\n", s_pm_off_modules, s_pm_enter_deep_sleep_modules);
			// BK_LOGD(NULL, "deepsleep2 0x%X 0x%X 0x%X 0x%X\r\n", s_pm_ahpb_pm_state, s_pm_video_pm_state, s_pm_audio_pm_state, s_pm_bakp_pm_state);
			pm_power_modules_dump_with_sleep_mode(s_pm_sleep_mode);
		}
	}
#if CONFIG_PM_SUPER_DEEP_SLEEP
	else if (s_pm_sleep_mode == PM_MODE_SUPER_DEEP_SLEEP)
	{
		if (pm_debug_mode() & 0x2)
		{
			LOGI("superdeepsleep1 0x%X 0x%X\r\n", s_pm_off_modules, s_pm_enter_deep_sleep_modules);
			// BK_LOGD(NULL, "superdeepsleep2 0x%X 0x%X 0x%X\r\n", s_pm_ahpb_pm_state, s_pm_video_pm_state, s_pm_audio_pm_state);
			pm_power_modules_dump_with_sleep_mode(s_pm_sleep_mode);
		}
	}
#endif
	else
	{
		if (pm_debug_mode() & 0x1)
		{
			LOGI("lowvol1 0x%X 0x%llX 0x%llX\r\n", s_pm_sleep_mode, s_pm_sleeped_modules, s_pm_enter_low_vol_modules);
			// BK_LOGD(NULL, "lowvol2 0x%X 0x%X\r\n", s_pm_video_pm_state, s_pm_audio_pm_state);
			pm_power_modules_dump_with_sleep_mode(s_pm_sleep_mode);
		}
	}
}
/*=========================DEBUG/TEST CTRL END========================*/
