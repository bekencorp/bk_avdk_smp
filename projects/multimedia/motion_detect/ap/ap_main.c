#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/bk_frame_buffer.h>
#include <driver/gpio_types.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "app_camera.h"
#include "app_display.h"
#include "app_gpu.h"
#include <lcd/lcd_mipi_er68576b_720x1280.h>
#include "media_service.h"
#include "include/motion_detect_demo.h"

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

    bk_auxldo_enable();
    bk_frame_buffer_init();

    camera_board_config_t camera_board = {0};
    display_board_config_t display_board = {0};
    gpu_board_config_t gpu_board = {0};

    camera_board.mipi.enable = true;
    camera_board.mipi.pin_scl = GPIO_69;
    camera_board.mipi.pin_sda = GPIO_70;
    camera_board.mipi.i2c_id = 1;
    camera_board.mipi.pin_reset = GPIO_71;
    camera_board.mipi.pin_pwdn = 0xFF;
    camera_board.mipi.pin_xclk = GPIO_59;
    camera_board.mipi.sensor_max_width = 1280;
    camera_board.mipi.sensor_max_height = 720;
    camera_board.mipi.sensor_fps = 20;
    camera_board.isp.mp_enable = true;
    camera_board.isp.mp_flexa = true;
    camera_board.isp.mp_width = 1280;
    camera_board.isp.mp_height = 720;
    camera_board.isp.mp_format = BK_PIXEL_FORMAT_NV12;

    display_board.mipi.enable = true;
    display_board.mipi.pin_reset = GPIO_60;
    display_board.mipi.pin_backlight = GPIO_7;
    display_board.mipi.panel = &lcd_device_er68576b_mipi_720x1280;
    display_board.dpu_video.enable = true;
    display_board.dpu_video.decompress = true;
    display_board.dpu_video.format = BK_PIXEL_FORMAT_ARGB8888;

    gpu_board.flexa.enable = true;
    gpu_board.flexa.degree = 90;
    gpu_board.flexa.src_width = camera_board.isp.mp_width;
    gpu_board.flexa.src_height = camera_board.isp.mp_height;
    gpu_board.flexa.dst_width = camera_board.isp.mp_width;
    gpu_board.flexa.dst_height = camera_board.isp.mp_height;
    gpu_board.flexa.src_format = camera_board.isp.mp_format;
    gpu_board.flexa.dst_format = display_board.dpu_video.format;
    gpu_board.flexa.dst_compress = display_board.dpu_video.decompress;
    gpu_board.flexa.scale = false;

    app_camera_board_config_set(&camera_board);
    app_display_board_config_set(&display_board);
    app_gpu_board_config_set(&gpu_board);

    cli_motion_detect_demo_init();
    (void)motion_detect_demo_boot_start();

    return 0;
}


