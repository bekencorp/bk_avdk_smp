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
#include <sys_types.h>
#include "sys_driver.h"
#include "aon_pmu_driver.h"
#include <os/mem.h>
#include "pm_debug.h"
#include "driver/int_types.h"
#include <driver/gpio.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc_types.h>
#include <driver/aon_rtc.h>
#include "pm_wakeup_source.h"
#include "pm_sleep.h"

/*=====================DEFINE SECTION START=====================*/
#define PM_WAKEUP_SOURCE_MARK                                (WAKEUP_SOURCE_MARK)

/*=====================DEFINE SECTION END=====================*/

/*=====================VARIABLE SECTION START=================*/
static uint32_t s_pm_wakeup_source                          = 0;
static pm_wakeup_source_e s_pm_exit_low_vol_wakeup_source   = PM_WAKEUP_SOURCE_INT_NONE;
static pm_wakeup_source_e s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;

static touch_wakeup_param_t s_touch_wakeup_param;
static uint64_t s_sleep_wakeup_irq                   = 0;
static icu_int_src_t s_sleep_wakeup_irq_id           = INT_SRC_NONE;
static bk_pm_wakeup_reason_e s_sleep_rtc_wakeup_source      = BK_PM_WAKEUP_UNKNOWN;
/*=====================VARIABLE SECTION END=================*/

/*================FUNCTION DECLARATION SECTION START========*/
static void pm_core_rtc_callback(aon_rtc_id_t id, uint8_t *name_p, void *param);
static void pm_core_gpio_callback(gpio_id_t gpio_id);
static bk_err_t pm_core_rtc_wakeup_config(const pm_ap_core_msg_t *msg);
static bk_err_t pm_core_gpio_wakeup_config(const pm_ap_core_msg_t *msg);
/*================FUNCTION DECLARATION SECTION END========*/
static void pm_core_rtc_callback(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LV_WAKEUP,0x0,0x0);
	pm_ap_core_msg_t msg = {0};
	msg.event= PM_CP_CORE_RTC_WAKEUPED;
	bk_pm_send_msg(&msg);
}
static void pm_core_gpio_callback(gpio_id_t gpio_id)
{
	bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LV_WAKEUP,0x0,0x0);
	pm_ap_core_msg_t msg = {0};
	msg.event= PM_CP_CORE_GPIO_WAKEUPED;
	msg.param1 = gpio_id;
	bk_pm_send_msg(&msg);
}
static bk_err_t pm_core_rtc_wakeup_config(const pm_ap_core_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	pm_rtc_wakeup_config_t *rtc_cfg = (pm_rtc_wakeup_config_t*)msg->param3;

#if CONFIG_AON_RTC || CONFIG_ANA_RTC
	alarm_info_t lv_alarm = {
						PM_APP_RTC_ALARM_NAME,
						(rtc_cfg->rtc_period)*AON_RTC_MS_TICK_CNT,
						rtc_cfg->rtc_cnt,
						pm_core_rtc_callback,
						NULL
						};

	bk_alarm_unregister(AON_RTC_ID_1, lv_alarm.name);
	bk_alarm_register(AON_RTC_ID_1, &lv_alarm);
#endif //CONFIG_AON_RTC
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
	return ret;
}
static bk_err_t pm_core_gpio_wakeup_config(const pm_ap_core_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	int gpio_id;
	gpio_int_type_t int_type;
	if(msg == NULL)
	{
		return BK_FAIL;
	}
	gpio_id = msg->param3&0xFFFF;
	int_type = (msg->param3 >> 16)&0xFFFF;
	LOGD("gpio cfg[%d][%d]\r\n",gpio_id,int_type);
	#if CONFIG_GPIO_WAKEUP_SUPPORT || CONFIG_ANA_GPIO
	bk_gpio_register_isr(gpio_id , pm_core_gpio_callback);
	bk_gpio_register_wakeup_source(gpio_id ,int_type);
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
	#endif //CONFIG_GPIO_WAKEUP_SUPPORT
	return ret;
}
bk_err_t bk_pm_wakeup_source_set(pm_wakeup_source_e wakeup_source, void *source_param)
{
	GLOBAL_INT_DECLARATION();

	if (wakeup_source >= PM_WAKEUP_SOURCE_INT_NONE)
	{
		return BK_ERR_PARAM;
	}

	GLOBAL_INT_DISABLE();
	s_pm_wakeup_source |= 0x1 << wakeup_source;
	if (source_param == NULL)
	{
#if CONFIG_WAKEUP
		bk_wakeup_driver_source_set(wakeup_source);
#endif
		GLOBAL_INT_RESTORE();
		return BK_OK;
	}
	switch (wakeup_source)
	{
	case PM_WAKEUP_SOURCE_INT_GPIO:
		break;

	case PM_WAKEUP_SOURCE_INT_RTC:
		break;

	case PM_WAKEUP_SOURCE_INT_SYSTEM_WAKE: // wifi/bt
		break;

	case PM_WAKEUP_SOURCE_INT_USBPLUG:
		break;

	case PM_WAKEUP_SOURCE_INT_TOUCHED:
		os_memcpy(&s_touch_wakeup_param, (touch_wakeup_param_t *)source_param, sizeof(touch_wakeup_param_t));
		break;

	case PM_WAKEUP_SOURCE_INT_VAD:
		break;

	default:
		break;
	}
	GLOBAL_INT_RESTORE();
	return BK_OK;
}

bk_err_t pm_core_wakeup_src_cfg_handle(const pm_ap_core_msg_t *msg)
{
	bk_err_t ret = BK_OK;
	if(msg == NULL)
	{
		return BK_FAIL;
	}
	pm_sleep_mode_e sleep_mode = msg->param1;
	pm_wakeup_source_e wakeup_source = msg->param2;

	switch(sleep_mode)
	{
		case PM_MODE_LOW_VOLTAGE:
			if(wakeup_source == WAKEUP_SOURCE_INT_RTC)
			{
				pm_core_rtc_wakeup_config(msg);
			}
			else if(wakeup_source == WAKEUP_SOURCE_INT_GPIO)
			{
				pm_core_gpio_wakeup_config(msg);
			}
		break;
		case PM_MODE_DEEP_SLEEP:
			if(wakeup_source == WAKEUP_SOURCE_INT_RTC)
			{
				pm_core_rtc_wakeup_config(msg);
			}
			else if(wakeup_source == WAKEUP_SOURCE_INT_GPIO)
			{
				pm_core_gpio_wakeup_config(msg);
			}
		break;
        default:
			break;
	}

	return ret;
}

pm_wakeup_source_e bk_pm_wakeup_source_get(void)
{
	return s_pm_wakeup_source;
}

void pm_touched_wakeup_low_voltage()
{
	sys_drv_touch_wakeup_enable(s_touch_wakeup_param.touch_channel);
}

void pm_rtc_wakeup_deep_sleep()
{
	aon_pmu_drv_set_wakeup_source(WAKEUP_SOURCE_INT_RTC);
}

void pm_touched_wakeup_deep_sleep()
{
	sys_drv_touch_wakeup_enable(s_touch_wakeup_param.touch_channel);
}

bk_err_t pm_wakeup_from_deepsleep_handle()
{
	uint32_t pmu_state = 0;
	if (aon_pmu_drv_reg_get(PMU_REG2) & BIT(BIT_SLEEP_FLAG_DEEP_SLEEP))
	{
		pmu_state = 0;
		pmu_state = aon_pmu_drv_reg_get(PMU_REG2);
		pmu_state &= ~(BIT(BIT_SLEEP_FLAG_DEEP_SLEEP));
		aon_pmu_drv_reg_set(PMU_REG2, pmu_state);
	}
	return BK_OK;
}

void pm_deep_sleep_wakeup_source_set()
{
	uint32_t pmu_state = 0;
	if (aon_pmu_drv_reg_get(PMU_REG2) & BIT(BIT_SLEEP_FLAG_DEEP_SLEEP))
	{
		pmu_state = 0;
		pmu_state = aon_pmu_drv_reg_get(PMU_REG0x71);
		pmu_state = (pmu_state >> 20) & PM_WAKEUP_SOURCE_MARK;

		switch (pmu_state)
		{
		case 0x1: // gpio
			bk_misc_set_reset_reason(RESET_SOURCE_DEEPPS_GPIO);
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_GPIO;
			break;
		case 0x2: // rtc
			bk_misc_set_reset_reason(RESET_SOURCE_DEEPPS_RTC);
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_RTC;
			break;
		case 0x10: // usbplug
			bk_misc_set_reset_reason(RESET_SOURCE_DEEPPS_USB);
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_USBPLUG;
			break;
		case 0x20: // touch
			bk_misc_set_reset_reason(RESET_SOURCE_DEEPPS_TOUCH);
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_TOUCHED;
			break;
		case 0x40: // vad
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_VAD;
			break;
		default:
			s_pm_exit_deepsleep_wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
			break;
		}
	}
}

pm_wakeup_source_e bk_pm_deep_sleep_wakeup_source_get()
{
	return s_pm_exit_deepsleep_wakeup_source;
}

pm_wakeup_source_e bk_pm_exit_low_vol_wakeup_source_get()
{
	return s_pm_exit_low_vol_wakeup_source;
}

bk_err_t bk_pm_exit_low_vol_wakeup_source_set()
{
	uint32_t pmu_state = 0;

	pmu_state = 0;
	pmu_state = aon_pmu_drv_reg_get(PMU_REG0x71);
	pmu_state = (pmu_state >> 20) & PM_WAKEUP_SOURCE_MARK;

	switch (pmu_state)
	{
	case 0x1: // gpio
		s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_GPIO;
		break;
	case 0x2: // rtc
		s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_RTC;
		break;
	case 0x4: // WIFI wakeup
		s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_WIFI;
		break;
	case 0x8: // BT wakeup
		s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_BT;
		break;
	case 0x10: // usbplug wakeup
		s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_USBPLUG;
		break;
	case 0x20: // touch wakeup
		s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_TOUCHED;
		break;
	case 0x40: // vad wakeup
		s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_VAD;
		break;
	default:
		s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
		break;
	}

	return BK_OK;
}

bk_err_t bk_pm_exit_low_vol_wakeup_source_clear()
{
	/*clear the wakeup source*/
	uint32_t pmu_state = 0;
	pmu_state = aon_pmu_drv_reg_get(PMU_REG0x43);
	pmu_state |= (0x1 << 17);
	aon_pmu_drv_reg_set(PMU_REG0x43, pmu_state);

	pmu_state = aon_pmu_drv_reg_get(PMU_REG0x43);
	pmu_state &= ~(0x1 << 17);
	aon_pmu_drv_reg_set(PMU_REG0x43, pmu_state);
	s_pm_exit_low_vol_wakeup_source = PM_WAKEUP_SOURCE_INT_NONE;
	return BK_OK;
}

__attribute__((section(".iram")))  bk_err_t bk_pm_sleep_wakeup_reason_clear()
{
	s_sleep_wakeup_irq    = 0;
	s_sleep_wakeup_irq_id = 0;
	s_sleep_rtc_wakeup_source    = BK_PM_WAKEUP_UNKNOWN;
	return BK_OK;
}

__attribute__((section(".iram")))  bk_err_t bk_pm_sleep_wakeup_reason_set(uint64_t wakeup_irq)
{
	s_sleep_wakeup_irq = wakeup_irq;
	for(int i = 0; i < INT_SRC_NONE; i++)
	{
		if(wakeup_irq & (1ULL << i))
		{
			s_sleep_wakeup_irq_id = i;
		}
	}
	return BK_OK;
}
bk_err_t bk_pm_rtc_wakeup_reason_parse()
{
	bk_pm_wakeup_reason_e wakeup_reason = BK_PM_WAKEUP_UNKNOWN;
	uint32_t int_src = 0;
	#if CONFIG_ANA_RTC
	int_src =  INT_SRC_ANA_RTC;
	#else
	int_src = INT_SRC_RTC;
	#endif

	if(s_sleep_wakeup_irq_id == int_src)
	{
		uint8_t *alarm_name = bk_rtc_get_first_alarm_name();

		if(alarm_name != NULL)
		{
			const char *alarm_name_str = (const char *)alarm_name;

			if(strncmp(alarm_name_str, PM_WIFI_RTC_ALARM_NAME, sizeof(PM_WIFI_RTC_ALARM_NAME)) == 0)
			{
				wakeup_reason = BK_PM_WAKEUP_WIFI;
			}
			else if(strncmp(alarm_name_str, PM_BT_RTC_ALARM_NAME, sizeof(PM_BT_RTC_ALARM_NAME)) == 0)
			{
				wakeup_reason = BK_PM_WAKEUP_BLE;
			}
			else if(strncmp(alarm_name_str, PM_APP_RTC_ALARM_NAME, sizeof(PM_APP_RTC_ALARM_NAME)) == 0)
			{
				wakeup_reason = BK_PM_WAKEUP_HW_TIMER;
			}
			else if(strncmp(alarm_name_str, PM_MM_RTC_ALARM_NAME, sizeof(PM_MM_RTC_ALARM_NAME)) == 0)
			{
				/*mm rtc alarm is on wifi on, it is not a wakeup source, ignore it*/
			}
			else
			{
				wakeup_reason = BK_PM_WAKEUP_HW_TIMER;
			}
			//LOGI("bk_pm_sleep_wakeup_reason_get: alarm name=%s\r\n", alarm_name_str);
		}
		else
		{
			//LOGI("bk_pm_sleep_wakeup_reason_get: no alarm name\r\n");
			wakeup_reason = BK_PM_WAKEUP_HW_TIMER;
		}
		s_sleep_rtc_wakeup_source = wakeup_reason;
	}
	return BK_OK;
}
bk_pm_wakeup_reason_e bk_pm_sleep_wakeup_reason_get()
{
	if(s_sleep_wakeup_irq_id != INT_SRC_NONE)
	{
		/*Debug*/
		//LOGI("NS wakeup irq: %d,0x%llx", s_sleep_wakeup_irq_id,s_sleep_wakeup_irq);
	}
	bk_pm_wakeup_reason_e wakeup_reason = BK_PM_WAKEUP_UNKNOWN;
	switch (s_sleep_wakeup_irq_id)
	{
		#if CONFIG_ANA_GPIO
		case INT_SRC_ANA_GPIO:
		#endif
		case INT_SRC_GPIO:
		case INT_SRC_GPIO_NS:
		    wakeup_reason = BK_PM_WAKEUP_GPIO;
			break;
		#if CONFIG_ANA_RTC
		case INT_SRC_ANA_RTC:
		#else
		case INT_SRC_RTC:
		#endif
		if(s_sleep_rtc_wakeup_source != BK_PM_WAKEUP_UNKNOWN)
		{
			wakeup_reason = s_sleep_rtc_wakeup_source;
		}
		else
		{
		}
			break;
		case INT_SRC_MAC_GENERAL:
		    wakeup_reason = BK_PM_WAKEUP_WIFI;
			break;
		case INT_SRC_BT:
		    wakeup_reason = BK_PM_WAKEUP_BLE;
			break;
		case INT_SRC_UART1:
		    wakeup_reason = BK_PM_WAKEUP_UART_CONSOLE;
			break;
		case INT_SRC_UART2:
		    wakeup_reason = BK_PM_WAKEUP_UART_TTYS2;
			break;
		default:
		   wakeup_reason =  BK_PM_WAKEUP_UNKNOWN;
			break;
	}
	return wakeup_reason;
}