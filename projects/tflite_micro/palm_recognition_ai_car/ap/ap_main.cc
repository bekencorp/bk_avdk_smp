#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include <stdint.h>
#include <stdlib.h>
#include "media_service.h"
#include "app_display.h"
#include "app_camera.h"
#include "devices_mgmt.h"
#include "app_gpu.h"
#include "avdk_monitor.h"
#include <lcd/lcd_mipi_hx8399c_1080x1920.h>
#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

/* Include GPIO driver headers in extern "C" block before AvdkVideoReator.h
 * to ensure C linkage for gpio_dev_unmap and other GPIO functions.
 * AvdkVideoReator.h also includes gpio_driver.h, but this ensures it's
 * included with C linkage first.
 */
extern "C" {
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
}

#include "AvdkVideoReator.h"
#include "AvdkDetectionModel.h"
#include "PalmDetectionModel.h"
#include "app_event.h"
#include "car_control.h"
#include "avi_play.h"

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

/**
 * @brief Adapter: new Box-based detection callback -> old palm-result callback.
 *
 * `PalmDetectionModel` now publishes results through the generic
 * `boxDetectionCallbackT(Box *boxes, int count)` interface inherited from
 * `AvdkDetectionModel`. Per `PalmDetectionModel::run()` it always emits
 * exactly one `Box`, with `score` repurposed as the `has_palm` flag (1.0f =
 * palm detected, 0.0f = none).
 *
 * The car/gimbal business layer in app_event.c was written against the older
 * `(has_palm, cx, cy, w, h)` callback shape and we want to keep it untouched,
 * so this small adapter does the unpacking and forwards to the legacy callback
 * obtained from `app_event_get_result_callback()`.
 */
static void on_box_detection(Box *boxes, int count)
{
    static palm_result_callback_t cb = nullptr;
    if (cb == nullptr) {
        cb = app_event_get_result_callback();
    }
    if (cb == nullptr || boxes == nullptr || count <= 0) {
        return;
    }
    /* Center coordinates are already in model space (e.g. 256x256); see
     * PalmDetectionModel.cc which writes palm_cx/cy/w/h directly into the Box. */
    int has_palm = (boxes[0].score > 0.5f) ? 1 : 0;
    cb(has_palm, boxes[0].x, boxes[0].y, boxes[0].w, boxes[0].h);
}


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
    model->setBoxDetectionCallback(on_box_detection);
    video_reator = new AvdkVideoReator(model);
    video_reator->init_model();
    video_reator->OpenCameraWithoutDisplay();
    video_reator->start_infer();

    return 0;
}

int main(void)
{
    bk_init();
    media_service_init();

    /* U19 power on enable */
    gpio_dev_unmap(GPIO_1);
    bk_gpio_enable_output(GPIO_1);
    bk_gpio_set_output_high(GPIO_1);

    /* system power on enable */
    gpio_dev_unmap(GPIO_41);
    bk_gpio_enable_output(GPIO_41);
    bk_gpio_set_output_high(GPIO_41);

    /* DVD 3.3V enable */
    gpio_dev_unmap(GPIO_30);
    bk_gpio_enable_output(GPIO_30);
    bk_gpio_set_output_high(GPIO_30);


    /* VBAT enable */
    gpio_dev_unmap(GPIO_43);
    bk_gpio_enable_output(GPIO_43);
    bk_gpio_set_output_high(GPIO_43);

    /* enable power VBAT -> 3.3V */
    gpio_dev_unmap(GPIO_47);
    bk_gpio_enable_output(GPIO_47);
    bk_gpio_set_output_high(GPIO_47);

    /* enable power for csi camera */
    gpio_dev_unmap(GPIO_44);
    bk_gpio_enable_output(GPIO_44);
    bk_gpio_set_output_high(GPIO_44);

    /* csi reset */
    gpio_dev_unmap(GPIO_46);
    bk_gpio_enable_output(GPIO_46);
    bk_gpio_set_output_high(GPIO_46);

    /* csi pwdn */
    gpio_dev_unmap(GPIO_45);
    bk_gpio_enable_output(GPIO_45);
    bk_gpio_set_output_high(GPIO_45);

    gpio_dev_unmap(GPIO_53);
    bk_gpio_enable_output(GPIO_53);
    bk_gpio_set_output_low(GPIO_53);

    bk_printf("M55 main running...\r\n");

    camera_board_config_t camera_board = {0};
    display_board_config_t display_board = {0};
    gpu_board_config_t gpu_board = {0};

    camera_board.mipi.enable = true;
    camera_board.mipi.pin_scl = GPIO_21;
    camera_board.mipi.pin_sda = GPIO_20;
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
    gpu_board.flexa.tess_width = 0;
    gpu_board.flexa.tess_height = 0;

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

    bk_avi_player_start("1:/animation_240_304.avi");

    extern int ai_main_start(void);
    ai_main_start();

    return 0;
}
