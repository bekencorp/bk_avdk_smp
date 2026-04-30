/**
 * doorbell_kvs: multimedia bring-up, Wi-Fi via CLI (no BLE provisioning).
 */
#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include <stdint.h>
#include <stdlib.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "media_service.h"
#include "app_display.h"
#include "app_camera.h"
#include "devices_mgmt.h"
#include "app_gpu.h"
#include "avdk_monitor.h"
#include "doorbell_comm.h"
#include "doorbell_kvs_network_transfer.h"
#include <lcd/lcd_hx8399c_mipi_1080x1920.h>

extern int kvs_webrtc_cli_init(void);
extern void mm_test_cli_init(void);

static void kvs_set_aws_credentials_env(void)
{
	setenv("AWS_ACCESS_KEY_ID", "YOUR_ACCESS_KEY_ID", 1);
	setenv("AWS_SECRET_ACCESS_KEY", "YOUR_SECRET_ACCESS_KEY", 1);
	//setenv("AWS_DEFAULT_REGION", "us-west-2", 1);
}

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

	bk_printf("doorbell_kvs M55 main\r\n");

	camera_board_config_t camera_board = {0};
	display_board_config_t display_board = {0};
	gpu_board_config_t gpu_board = {0};

	camera_board.mipi.enable = true;
	camera_board.mipi.pin_scl = GPIO_69;
	camera_board.mipi.pin_sda = GPIO_70;
	camera_board.mipi.i2c_id = 1;
	camera_board.mipi.pin_reset = GPIO_71;
	camera_board.mipi.pin_pwdn = -1;
	camera_board.mipi.pin_xclk = GPIO_59;
	camera_board.mipi.sensor_max_width = 1920;
	camera_board.mipi.sensor_max_height = 1080;
	camera_board.mipi.sensor_fps = 25;
	camera_board.isp.mp_enable = true;
	camera_board.isp.mp_flexa = true;
	camera_board.isp.mp_width = 1920;
	camera_board.isp.mp_height = 1080;
	camera_board.isp.mp_format = BK_PIXEL_FORMAT_NV12;
	camera_board.isp.sp_enable = false;
	camera_board.isp.sp_flexa = false;
	display_board.mipi.enable = true;
	display_board.mipi.pin_reset = GPIO_60;
	display_board.mipi.pin_backlight = GPIO_7;
	display_board.mipi.panel = &lcd_device_hx8399c_mipi_1080x1920;
	display_board.dpu_video.enable = true;
	display_board.dpu_video.decompress = true;
	display_board.dpu_video.format = BK_PIXEL_FORMAT_ARGB8888;

	gpu_board.flexa.enable = true;
	gpu_board.flexa.degree = 90;
	gpu_board.flexa.src_width = 1920;
	gpu_board.flexa.src_height = 1080;
	gpu_board.flexa.dst_width = 1920;
	gpu_board.flexa.dst_height = 1080;
	gpu_board.flexa.src_format = BK_PIXEL_FORMAT_NV12;
	gpu_board.flexa.dst_format = BK_PIXEL_FORMAT_ARGB8888;
	gpu_board.flexa.dst_compress = true;
	gpu_board.flexa.scale = false;

	bk_auxldo_enable();
	bk_frame_buffer_init();

	app_camera_board_config_set(&camera_board);
	app_display_board_config_set(&display_board);
	app_gpu_board_config_set(&gpu_board);

	avdk_monitor_init();
	avdk_monitor_start();

	devices_mgmt_init();

	kvs_set_aws_credentials_env();

	doorbell_kvs_ntwk_init("kvs_service", NULL);
	doorbell_core_init();
	mm_test_cli_init();
	kvs_webrtc_cli_init();

	return 0;
}
