#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include <stdint.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "media_service.h"
#if 0
#include "app_display.h"
#include "app_camera.h"
#include "devices_mgmt.h"
#include "app_gpu.h"
#include "avdk_monitor.h"
#ifdef CONFIG_INTEGRATION_DOORBELL
#include "doorbell_comm.h"
#include "bk_smart_config.h"
#endif
#include <lcd/lcd_hx8399c_mipi_1080x1920.h>
#endif
#include <media_service.h>
#include "cli_player_service.h"
#if (CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE)
#include "cli_voice_service.h"
#endif

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
int main(void)
{
	bk_init();
    media_service_init();

    bk_printf("M55 main running...\r\n");
#if 0
    camera_board_config_t config = {
        .mipi = {
            .enable = true,
            .pin_scl = GPIO_69,
            .pin_sda = GPIO_70,
            .i2c_id = 1,
            .pin_reset = GPIO_71,
            .pin_pwdn = -1,
            .pin_xclk = GPIO_59,
            .sensor_max_width = 1280,
            .sensor_max_height = 720,
            .sensor_fps = 20,
        },
        .isp = {
            .mp_enable = true,
            .mp_width = 1280,
            .mp_height = 720,
            .sp_enable = false,
        },
    };

    display_board_config_t display_config = {
        .mipi = {
            .enable = true,
            .pin_reset = GPIO_60,
            .pin_backlight = GPIO_7,
            .panel = &lcd_device_hx8399c_mipi_1080x1920,
        },

        .dpu_video = {
            .enable = true,
            .decompress = true,
            .format = BK_PIXEL_FORMAT_ARGB8888,
        },
    };

    gpu_board_config_t gpu_config = {
        .flexa = {
            .enable = true,
            .degree = 90,
            .src_width = 1280,
            .src_height = 720,
            .dst_width = 1920,
            .dst_height = 1080,
            .src_format = BK_PIXEL_FORMAT_NV12,
            .dst_format = BK_PIXEL_FORMAT_ARGB8888,
            .dst_compress = true,
            .scale = true,
            .tess_width = 0,
            .tess_height = 0,
        },
    };
#endif
    bk_auxldo_enable();
    bk_frame_buffer_init();

#if 0
    /* Board config for Multimedia config */
    app_camera_board_config_set(&config);
    app_display_board_config_set(&display_config);
    app_gpu_board_config_set(&gpu_config);


    /* Debug config for Multimedia */
    avdk_monitor_init();
    avdk_monitor_start();

    devices_mgmt_init();

#if (defined(CONFIG_INTEGRATION_DOORBELL))
    bk_smart_config_init();
    doorbell_core_init();
#endif
#endif
    cli_player_service_init();
#if (CONFIG_VOICE_SERVICE_TEST && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE)
    cli_voice_service_init();
#endif

	return 0;
}
