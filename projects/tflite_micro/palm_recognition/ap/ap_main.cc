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
#include "app_display.h"
#include "app_camera.h"
#include "devices_mgmt.h"
#include "app_gpu.h"
#include "avdk_monitor.h"
#include "aov_detection.h"
#include <lcd/lcd_hx8399c_mipi_1080x1920.h>
#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

#include "AvdkVideoReator.h"
#include "AvdkDetectionModel.h"
#include "PalmDetectionModel.h"
#include "app_event.h"
#include "car_control.h"

static void bk_auxldo_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

static AvdkVideoReator *video_reator = NULL;
static PalmDetectionModel *model = NULL;

int ai_main_start()
{
    /* Init UART for car/gimbal control (optional; skip if not connected). */
    const car_control_config_t car_config = DEFAULT_CAR_CONTROL_CONFIG();
    if (car_control_init(&car_config) != BK_OK)
    {
        bk_printf("car_control_init failed\n");
    }

    /* Init palm event module (queue + tracking task). Pass NULL for default center/thresholds. */
    if (app_event_init(NULL) != BK_OK)
    {
        bk_printf("app_event_init failed\n");
    }

    model = new PalmDetectionModel();
    model->setResultCallback(app_event_get_result_callback());
    video_reator = new AvdkVideoReator(model);
    video_reator->start(AVDK_VIDEO_REATOR_MODE_DISPLAY);
    //video_reator->start(AVDK_VIDEO_REATOR_MODE_NODISPLAY);

    return 0;
}


int main(void)
{
    bk_init();
    media_service_init();

    bk_printf("M55 main running...\r\n");

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
    camera_board.mipi.sensor_max_width = 1088;
    camera_board.mipi.sensor_max_height = 1088;
    camera_board.mipi.sensor_fps = 15;
    camera_board.isp.mp_enable = true;
    camera_board.isp.mp_flexa = true;
    camera_board.isp.mp_width = 1088;
    camera_board.isp.mp_height = 1088;
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
    gpu_board.flexa.src_width = 1088;
    gpu_board.flexa.src_height = 1088;
    gpu_board.flexa.dst_width = 1920;
    gpu_board.flexa.dst_height = 1080;
    gpu_board.flexa.src_format = BK_PIXEL_FORMAT_NV12;
    gpu_board.flexa.dst_format = BK_PIXEL_FORMAT_ARGB8888;
    gpu_board.flexa.dst_compress = true;
    gpu_board.flexa.scale = false;

    bk_auxldo_enable();
    bk_frame_buffer_init();

    /* Board config for Multimedia config */
    app_camera_board_config_set(&camera_board);
    app_display_board_config_set(&display_board);
    app_gpu_board_config_set(&gpu_board);

    /* Debug config for Multimedia */
    //avdk_monitor_init();
    //avdk_monitor_start();

    devices_mgmt_init();

    extern int ai_main_start(void);
    ai_main_start();

    return 0;
}


