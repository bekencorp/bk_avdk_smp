#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <driver/mailbox.h>
#include "cli.h"
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc_types.h>
#include <driver/aon_rtc.h>
#include "bk_pm_internal_api.h"
#include "pm_wakeup_source.h"

/*=====================DEFINE  SECTION  START=====================*/
#define TAG "pm"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define PM_THREAD_STACK_SIZE              (1536)  /* Keep 1024 due to limited SRAM */
#define PM_QUEUE_NUMBER_OF_MESSAGE        (20)   /* Reduced to 20 to save SRAM (~240B queue) */
#define PM_THREAD_PRIORITY                (BEKEN_DEFAULT_WORKER_PRIORITY - 3)/* With priority 3 thread, 200ms buffer (20*10ms) is sufficient */

/*=====================DEFINE  SECTION  END=====================*/
/*=====================STRUCT AND ENUM  SECTION  START==========*/

/*=====================STRUCT AND ENUM  SECTION  END=============*/
/*=====================VARIABLE  SECTION  START==================*/
static  beken_thread_t s_thd;
static  beken_queue_t  s_queue;
/* Statistics for monitoring queue status */
static volatile uint32_t s_msg_send_success = 0;
static volatile uint32_t s_msg_send_fail    = 0;

extern UINT32 s_pm_rtc_sleep_count;
/*=====================VARIABLE  SECTION  END==================*/

/*================FUNCTION DECLARATION  SECTION  START==========*/
bk_err_t bk_pm_send_msg(pm_ap_core_msg_t *msg);
/*================FUNCTION DECLARATION  SECTION  END===========*/

#if CONFIG_AON_RTC
static void pm_deep_lv_rtc_callback(aon_rtc_id_t id, uint8_t *name_p, void *param)
{
	pm_ap_core_msg_t msg;
	msg.event  = PM_CALLBACK_HANDLE_MSG;
	msg.param1 = PM_MODE_LOW_VOLTAGE;
	msg.param2 = PM_WAKEUP_SOURCE_INT_RTC;
	msg.param3 = 2;
	bk_pm_send_msg(&msg);
}
#endif
static bk_err_t pm_rtc_sleep_wakeup_callback(pm_sleep_mode_e sleep_mode,pm_wakeup_source_e wake_source,void* param_p)
{
    pm_ap_core_msg_t msg = {0};
    msg.event= PM_CALLBACK_HANDLE_MSG;
    msg.param1 = PM_MODE_LOW_VOLTAGE;
    msg.param2 = PM_WAKEUP_SOURCE_INT_RTC;
    msg.param3 = 2;
    bk_pm_send_msg(&msg);
    return BK_OK;
}
void pm_gpio_callback(gpio_id_t gpio_id)
{
    pm_ap_core_msg_t msg = {0};
    msg.event= PM_CALLBACK_HANDLE_MSG;
    msg.param1 = PM_MODE_LOW_VOLTAGE;
    msg.param2 = PM_WAKEUP_SOURCE_INT_GPIO;
    msg.param3 = gpio_id;
    bk_pm_send_msg(&msg);
}
static bk_err_t pm_sleep_wakeup_callback(void* param1,uint32_t param2)
{
    LOGD("%s\r\n",__func__);
    return BK_OK;
}

static bk_err_t pm_thread_init(void)
{
    LOGI("%s\r\n",__func__);
    return BK_OK;
}

bk_err_t bk_pm_send_msg(pm_ap_core_msg_t *msg)
{
    bk_err_t ret = BK_OK;
	if(msg == NULL)
	{
	    LOGE("Pm core send msg error\r\n");
		return BK_FAIL;
	}

    if (s_queue)
    {
        /* Optimization: Using BEKEN_NO_WAIT in interrupt context is correct,
         * but need to handle queue full gracefully */
        ret = rtos_push_to_queue(&s_queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            /* Queue full or other error, silently drop message to avoid log flooding */
            s_msg_send_fail++;
            /* Log only once per 1000 failures to avoid log storm */
            if ((s_msg_send_fail % 1000) == 0) {
                LOGE("Pm msg failed (total: %u, success: %u)\n", s_msg_send_fail, s_msg_send_success);
            }
            return BK_FAIL;
        }
        s_msg_send_success++;
        return ret;
    }
    return ret;
}

static bk_err_t pm_message_handle(void)
{
	bk_err_t ret = BK_OK;
	pm_ap_core_msg_t msg;

	pm_thread_init();

	while (1)
	{
		ret = rtos_pop_from_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret)
		{
			switch (msg.event)
			{
				case PM_ENTER_LOW_VOLTAGE_MSG:
				{
					/* Reserved for future use */
				}
				break;

				case PM_ENTER_DEEP_SLEEP_MSG:
				{
					/* Reserved for future use */
				}
				break;

				case PM_CALLBACK_HANDLE_MSG:
				{
					/* Fast path: Handle RTC periodic callbacks efficiently */
					if (msg.param2 == PM_WAKEUP_SOURCE_INT_RTC)
					{
						/* RTC periodic callback (10ms) - minimal processing */
						if (msg.param1 == PM_MODE_LOW_VOLTAGE)
						{
							/* For periodic callbacks (param3 == 0), use fast path */
							if (msg.param3 == 0) {
								/* Just maintain the vote state, no delay */
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0x0);
							}
							else if (msg.param3 == 1)
							{
								/* State changed - log and process */
								LOGI("LV RTC wakeup[reason:%d]\r\n", bk_pm_sleep_wakeup_reason_get());
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
								rtos_delay_milliseconds(2);
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0x0);
							}
							else if (msg.param3 == 2)
							{
								/* State changed - log and process */
								// LOGI("Deep_LV RTC wakeup\r\n");
								bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP, PM_POWER_MODULE_STATE_ON);
								rtos_delay_milliseconds(2000);
								bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP, PM_POWER_MODULE_STATE_OFF);
								#if CONFIG_AON_RTC
								alarm_info_t low_valtage_alarm = {0};
								memcpy(low_valtage_alarm.name, "low_vol", sizeof("low_vol"));
								low_valtage_alarm.period_tick = 1000*AON_RTC_MS_TICK_CNT;
								low_valtage_alarm.period_cnt = 1;
								low_valtage_alarm.callback = pm_deep_lv_rtc_callback;
								low_valtage_alarm.param_p = NULL;

								bk_alarm_unregister(AON_RTC_ID_1, low_valtage_alarm.name);
								bk_alarm_register(AON_RTC_ID_1, &low_valtage_alarm);
								#endif //CONFIG_AON_RTC

								bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_RTC, NULL);
								rtos_delay_milliseconds(2);
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0x0);
							}
						}
						else if (msg.param1 == PM_MODE_NORMAL_SLEEP)
						{
							/* Handle based on param3 flag from interrupt */
							if (msg.param3 == BK_PM_WAKEUP_HW_TIMER) {
								/* Hardware timer wakeup */
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
							} else if (msg.param3 == 1) {
								/* State changed - full processing */
								LOGI("NS RTC wakeup[reason:%d]\r\n",bk_pm_sleep_wakeup_reason_get());
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
								rtos_delay_milliseconds(2);
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0x0);
							} else {
								/* Periodic callback - fast path */
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
								bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x1, 0x0);
							}
						}
					}
					/* GPIO wakeup - less frequent, can use full processing */
					else if (msg.param2 == PM_WAKEUP_SOURCE_INT_GPIO)
					{
						bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);

						if (msg.param1 == PM_MODE_LOW_VOLTAGE) {
							bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LOG, 0x0, 0x0);
							LOGI("LV GPIO wakeup[reason:%d][id:%d]\r\n", bk_pm_sleep_wakeup_reason_get(), msg.param3);
							rtos_delay_milliseconds(2000);
							bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LOG, 0x1, 0x0);
						} else if (msg.param1 == PM_MODE_NORMAL_SLEEP) {
							LOGI("NS GPIO wakeup[reason:%d][id:%d]\r\n", bk_pm_sleep_wakeup_reason_get(), msg.param3);
							//rtos_delay_milliseconds(2000);
							//bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LOG, 0x1, 0x0);
						}

					}
					else
					{
						/* Other wakeup sources */
						bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 0x0, 0x0);
					}
				}
				break;
				case PM_CP_CORE_STATE_ENTER_DEEPSLEEP:
				{
					bk_pm_cp0_response_cp1(PM_ENTER_DEEP_SLEEP_CMD,BK_OK,0,0);
					bk_pm_sleep_mode_set(msg.param1);
				}
				break;
				case PM_CP_CORE_CTRL_AP_STATE:
				{
					bk_pm_cp0_response_cp1(PM_CTRL_AP_STATE_CMD, BK_OK,0,0);
					bk_pm_module_vote_boot_ap_ctrl(msg.param1,msg.param2);
				}
				break;
				case PM_CP_CORE_AP_RECOVERY:
				{
					bk_pm_cp1_recovery_module_state_ctrl(msg.param1,msg.param2);
				}
				break;
				case PM_CP_CORE_RTC_DEEPSLEEP:
				{
					bk_pm_cp0_response_cp1(PM_RTC_DEEPSLEEP_CMD, BK_OK,0,0);
					bk_low_pwr_misc_rtc_enter_deepsleep(msg.param2,NULL);
				}
				break;
				case PM_CP_CORE_GET_CP_DATA:
				{
					uint32_t cp_pm_data = 0;
					pm_ap_get_cp_data_type_e data_type = msg.param1;
					switch(data_type)
					{
						case PM_CP_DATE_TYPE_TIME_INTERVAL_FROM_STARTUP:
							bk_low_pwr_misc_get_time_interval_from_startup(&cp_pm_data);
						break;
						case PM_CP_DATE_TYPE_DEEP_SLEEP_WAKEUP_SOURCE:
							cp_pm_data = bk_pm_deep_sleep_wakeup_source_get();
						break;
						case PM_CP_DATE_TYPE_EXIT_LOW_VOL_WAKEUP_SOURCE:
							cp_pm_data =  bk_pm_exit_low_vol_wakeup_source_get();
						break;
						default:
						break;
					}
					bk_pm_cp0_response_cp1(PM_GET_PM_DATA_CMD,data_type,cp_pm_data,0);
				}
				break;
				case PM_CP_CORE_PSRAM_POWER:
				{
					ret = bk_pm_module_vote_psram_ctrl(msg.param1,msg.param2);
					bk_pm_cp0_response_cp1(PM_CTRL_PSRAM_POWER_CMD, msg.param2,0,0);
				}
				break;
				case PM_CP_CORE_POWER_CTRL:
				{
					ret = bk_pm_module_vote_power_ctrl(msg.param1,msg.param2);
					bk_pm_cp0_response_cp1(PM_POWER_CTRL_CMD,ret,0,0);
				}
				break;
				case PM_CP_CORE_CLK_CTRL:
				{
					ret = bk_pm_clock_ctrl(msg.param1,msg.param2);
					bk_pm_cp0_response_cp1(PM_CLK_CTRL_CMD, ret,0,0);
				}
				break;
				case PM_CP_CORE_SLEEP_CTRL:
				{
					bk_pm_cp0_response_cp1(PM_SLEEP_CTRL_CMD,BK_OK,0,0);
					ret = bk_pm_module_vote_sleep_ctrl(msg.param1,msg.param2,msg.param3);
				}
				break;
				case PM_CP_CORE_FREQ_CTRL:
				{
					ret = bk_pm_module_vote_cpu_freq(msg.param1,msg.param2);
					bk_pm_cp0_response_cp1(PM_CPU_FREQ_CTRL_CMD, ret,0,0);
				}
				break;
				case PM_CP_CORE_EXTERNAL_LDO:
				{
					ret = bk_pm_module_vote_ctrl_external_ldo(msg.param1,msg.param2,msg.param3);
					bk_pm_cp0_response_cp1(PM_CTRL_EXTERNAL_LDO_CMD, ret,0,0);
				}
				break;
				case PM_CP_CORE_WAKEUP_SRC_CFG:
				{
					bk_pm_cp0_response_cp1(PM_WAKEUP_CONFIG_CMD, ret,0,0);
					pm_core_wakeup_src_cfg_handle(&msg);
				}
				break;
				case PM_CP_CORE_RTC_WAKEUPED:
				{
					LOGD("rtc_cb[%d][%d][%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get(),bk_pm_ap_boot_success_get(),bk_pm_sleep_wakeup_reason_get());
					if(!bk_pm_ap_boot_success_get())
					{
						bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);
					}
					bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LV_WAKEUP,0x1,0x0);
				}
				break;
				case PM_CP_CORE_GPIO_WAKEUPED:
				{
					LOGD("gpio_cb[%d][%d]\r\n",bk_pm_exit_low_vol_wakeup_source_get(),msg.param1,bk_pm_sleep_wakeup_reason_get());
					if(!bk_pm_ap_boot_success_get())
					{
						bk_pm_module_vote_boot_ap_ctrl(PM_BOOT_AP_MODULE_NAME_APP,PM_POWER_MODULE_STATE_ON);
					}
					bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_LV_WAKEUP,0x1,0x0);
				}
				break;
				default:
					break;
			}
		}
	}

	return  ret;
}

bk_err_t pm_thread_main(void)
{
    bk_err_t ret = BK_OK;
	ret = rtos_init_queue(&s_queue,
                          "pm_queue",
                          sizeof(pm_ap_core_msg_t),
                          PM_QUEUE_NUMBER_OF_MESSAGE);

    if (ret != BK_OK)
    {
        LOGE("create pm que fail\n");
    }

	/* Optimization: Use higher priority (lower number = higher priority)
	 * to ensure 10ms messages are processed promptly.
	 * Changed from BEKEN_DEFAULT_WORKER_PRIORITY to 3 for time-critical processing.
	 */
	ret = rtos_create_thread(&s_thd,
                             PM_THREAD_PRIORITY, /* Higher priority for 10ms interrupt handling */
                             "pm_thd",
                             (beken_thread_function_t)pm_message_handle,
                             PM_THREAD_STACK_SIZE,
                             NULL);
    if (ret != BK_OK)
    {
        LOGE("create pm thrd fail\n");
    }
	return 0;
}