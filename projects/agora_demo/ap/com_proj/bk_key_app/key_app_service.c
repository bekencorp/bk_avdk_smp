#include <common/sys_config.h>
#include <components/log.h>
#include <string.h>

#include <key_app_service.h>
#include <key_app_config.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#if (0)
#include "audio_config.h"
#include "aud_intf.h"
#include "bk_factory_config.h"
#if CONFIG_BK_SMART_CONFIG
#include "bk_genie_comm.h"
#include "bk_smart_config.h"
#endif
#endif
#if (CONFIG_USR_KEY_CFG_EN)
#include "usr_key_cfg.h"
#endif

#include "app_event.h"
#include <common/bk_include.h>
#include "components/bluetooth/bk_dm_bluetooth.h"

#if (CONFIG_BK_WSS_TRANS || CONFIG_BK_WSS_TRANS_NOPSRAM)
#include "bk_wss/bk_wss.h"
#endif

#if CONFIG_BK_SMART_CONFIG
#include "bk_smart_config.h"
#endif

#if CONFIG_BK_AUDIO_ENGINE
#include "audio_engine.h"
#endif

#include "bk_factory_config.h"

#define TAG "key_service"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#if CONFIG_BUTTON

static void power_off(void)
{
    LOGI(" power_off\r\n");
    LOGW(" ************TODO:Just force deep sleep for Demo!\r\n");
    //extern bk_err_t audio_turn_off(void);
    //extern bk_err_t video_turn_off(void);
    //audio_turn_off();
    //video_turn_off();
    bk_reboot_ex(RESET_SOURCE_FORCE_DEEPSLEEP);
}

static void power_on(void)
{
    LOGI("power_on\r\n");
}

static void ai_agent_config(void)
{
    //BK_LOGW(TAG, " ************TODO:AI Agent doesn't complete!\r\n");
}

static KeyConfig_t key_config[] = KEY_DEFAULT_CONFIG_TABLE;

/*Do not execute blocking or time-consuming long code in event handler
 functions. The reason is that key_thread processes messages in a
 single task in sequence. If a handler function blocks or takes too
 long to execute, it will cause subsequent key events to be responded to untimely.*/

static void handle_system_event(uint8_t event)
{
    if (IS_INVALID_EVENT(event))
    {
        LOGI("Invalid event: %d\r\n", event);
        return;
    }
    
    uint32_t time;

    switch (event)
    {
        case VOLUME_UP:
        {
            LOGI("VOLUME_UP\r\n");
            audio_engine_volume_increase();
        }
        break;
        case VOLUME_DOWN:
        {
            LOGI("VOLUME_DOWN\r\n");
            audio_engine_volume_decrease();
        }
        break;
        case SHUT_DOWN:
            time = rtos_get_time(); //long press more than 6s
            if (time < 9000)
            {
                break;
            }
            power_off();
            break;
        case POWER_ON:
            power_on();
            break;
        case AI_AGENT_CONFIG:
            LOGI("AI_AGENT_CONFIG\r\n");
            // ai_agent_config();
            break;
        case CONFIG_NETWORK:
            LOGI("CONFIG_NETWORK\r\n");
            #if CONFIG_BK_SMART_CONFIG
            bk_sconf_prepare_for_smart_config();
            #endif
            break;
        case IR_MODE_SWITCH:	////image recognition mode switch
            LOGI("IR_MODE_SWITCH\r\n");
            app_event_send_msg(APP_EVT_IR_MODE_SWITCH, 0);
            break;
        case FACTORY_RESET:
            LOGI("trigger factory config reset\r\n");
            bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_BTSP, PM_POWER_MODULE_STATE_OFF);//power off btsp
            bk_factory_reset();
            #if CONFIG_BK_SMART_CONFIG
            bk_sconf_erase_smart_config();
            #endif
            bk_reboot();
            break;
#if CONFIG_BK_WSS_TRANS_NOPSRAM
        case AUDIO_BUF_APPEND:
            LOGI("audio append start!\r\n");
            app_event_send_msg(APP_EVT_ASR_WAKEUP, 0);
            bk_wss_state_event(WSS_EVENT_RECORDING_START, NULL);
            break;
#endif
        // 其他事件处理...
        default:
            break;
    }
}

void bk_key_register_wakeup_source(void)
{
#if CONFIG_GPIO_DYNAMIC_WAKEUP_SUPPORT
    for (uint8_t i = 0; i < sizeof(key_config) / sizeof(KeyConfig_t); i++)
    {
        if ((key_config[i].short_event == POWER_ON) || (key_config[i].double_event == POWER_ON) || (key_config[i].long_event == POWER_ON))
        {
            if (key_config[i].active_level == LOW_LEVEL_TRIGGER)
            {
                bk_gpio_register_wakeup_source(key_config[i].gpio_id, GPIO_INT_TYPE_FALLING_EDGE);
                LOGI("register wakeup source %d falling edge\r\n", key_config[i].gpio_id);
            }
            else
            {
                bk_gpio_register_wakeup_source(key_config[i].gpio_id, GPIO_INT_TYPE_RISING_EDGE);
                LOGI("register wakeup source %d rising edge\r\n", key_config[i].gpio_id);
            }
        }
    }
#else
    LOGW("GPIO dynamic wakeup not supported, skip wakeup source registration\r\n");
#endif
}


void bk_key_service_init(void)
{
    bk_key_register_event_handler(handle_system_event);

    bk_key_driver_init(key_config, sizeof(key_config) / sizeof(KeyConfig_t));
}


#endif
