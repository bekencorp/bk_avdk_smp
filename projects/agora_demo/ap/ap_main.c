#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <media_service.h>
#include <components/bk_frame_buffer.h>
#include "sys_driver.h"
#include "sys_hal.h"

#if CONFIG_BK_NETWORK_TRANSFER
#include "network_transfer.h"
#endif

#if CONFIG_BK_AUDIO_ENGINE
#include "audio_engine.h"
#endif

#if CONFIG_BK_VIDEO_ENGINE
#include "video_engine.h"
#if CONFIG_USB_CAMERA
#include "devices_mgmt.h"
#endif
#if CONFIG_VIDEO_ENGINE_USE_DVP_CAMERA
#include "app_camera.h"
#include "app_camera_types.h"
#endif
#endif

#if CONFIG_BK_SMART_CONFIG
#include "bk_smart_config.h"
#endif

#if CONFIG_APP_EVT
#include "app_event.h"
#endif

#if CONFIG_BUTTON
#include <key_app_service.h>
#endif

#include "bk_factory_config.h"

#if CONFIG_LED_BLINK
#include "led_blink.h"
#endif

#if CONFIG_MOTOR
#include "motor.h"
#endif

#if CONFIG_NET_PAN
#include "bluetooth_storage.h"
#endif

#define TAG "ap_main"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#ifdef CONFIG_LDO3V3_ENABLE
#ifndef LDO3V3_CTRL_GPIO
#ifdef CONFIG_LDO3V3_CTRL_GPIO
#define LDO3V3_CTRL_GPIO    CONFIG_LDO3V3_CTRL_GPIO
#else
#define LDO3V3_CTRL_GPIO    GPIO_52
#endif
#endif
#endif

static const uint32_t s_user_value2 = 10;

const struct factory_config_t s_user_config[] = {
    {"user_key1", (void *)"user_value1", 11, BK_FALSE, 0},
    {"user_key2", (void *)&s_user_value2, 4, BK_TRUE, 4},
};

#if CONFIG_BK_VIDEO_ENGINE && CONFIG_VIDEO_ENGINE_USE_DVP_CAMERA
#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

static void bk_auxldo_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

static void video_engine_camera_board_init(void)
{
    camera_board_config_t camera_board = {0};

    camera_board.mipi.enable = true;
    camera_board.mipi.pin_scl = GPIO_69;
    camera_board.mipi.pin_sda = GPIO_70;
    camera_board.mipi.i2c_id = 1;
    camera_board.mipi.pin_reset = GPIO_71;
    camera_board.mipi.pin_pwdn = (uint8_t)-1;
    camera_board.mipi.pin_xclk = GPIO_59;
    camera_board.mipi.sensor_max_width = CONFIG_VIDEO_ENGINE_RESOLUTION_WIDTH;
    camera_board.mipi.sensor_max_height = CONFIG_VIDEO_ENGINE_RESOLUTION_HEIGHT;
    camera_board.mipi.sensor_fps = 20;

    camera_board.isp.mp_enable = true;
    camera_board.isp.mp_flexa = true;
    camera_board.isp.mp_width = CONFIG_VIDEO_ENGINE_RESOLUTION_WIDTH;
    camera_board.isp.mp_height = CONFIG_VIDEO_ENGINE_RESOLUTION_HEIGHT;
    camera_board.isp.mp_format = BK_PIXEL_FORMAT_NV12;
    camera_board.isp.sp_enable = false;
    camera_board.isp.sp_flexa = false;

    BK_LOG_ON_ERR(app_camera_board_config_set(&camera_board));
}
#endif

int main(void)
{
        bk_init();

        media_service_init();

    #ifdef CONFIG_LDO3V3_ENABLE
        BK_LOG_ON_ERR(gpio_dev_unmap(LDO3V3_CTRL_GPIO));
        bk_gpio_disable_pull(LDO3V3_CTRL_GPIO);
        bk_gpio_enable_output(LDO3V3_CTRL_GPIO);
        bk_gpio_set_output_high(LDO3V3_CTRL_GPIO);
    #endif

    #if CONFIG_BK_VIDEO_ENGINE
        bk_frame_buffer_init();
    #endif

    #if CONFIG_BK_VIDEO_ENGINE && CONFIG_USB_CAMERA
        devices_mgmt_init();
    #endif

    #if CONFIG_BK_VIDEO_ENGINE && CONFIG_VIDEO_ENGINE_USE_DVP_CAMERA
        bk_auxldo_enable();
        video_engine_camera_board_init();
    #endif

    bk_regist_factory_user_config((const struct factory_config_t *)&s_user_config,
                                    sizeof(s_user_config)/sizeof(s_user_config[0]));
    bk_factory_init();

    #if CONFIG_LED_BLINK
        led_driver_init();
        led_app_set(LED_ON_GREEN,LED_LAST_FOREVER);
    #endif

    #if (CONFIG_NFC_ENABLE)
        void nfc_get_id_task(void);
        nfc_get_id_task();
    #endif

#if CONFIG_BK_AUDIO_ENGINE
        audio_engine_init();
#endif

    #if CONFIG_APP_EVT
        app_event_init();
    #endif

    #if CONFIG_BK_NETWORK_TRANSFER
        ntwk_trans_init();
    #endif

    #if CONFIG_BK_SMART_CONFIG
        bk_sconf_init();
    #endif


    //Notice need to wait other services to initialize
    #if CONFIG_BUTTON
        bk_key_service_init();
    #endif

    #if CONFIG_BAT_MONITOR
        extern void battery_monitor_init(void);
        battery_monitor_init();
    #endif

    #if CONFIG_USBD_MSC
        extern void msc_storage_init(void);
        msc_storage_init();
    #endif

    // }
    // else
    // {
    //     bk_init();
    //     bk_enter_deepsleep();
    // }

        return 0;

}
