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
#include "app_camera.h"
#include "devices_mgmt.h"
#include "avdk_monitor.h"
#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

#include "AvdkVideoReator.h"
#include "tflm_person_detect.h"

static void bk_auxldo_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

static AvdkVideoReator *video_reator = NULL;
static PersonDetectModel *model = NULL;

static void person_detection_box_cb(Box *boxes, int count)
{
    if (boxes == NULL || count <= 0) {
        bk_printf("PersonDetection no target\r\n");
        return;
    }

    bk_printf("PersonDetection count=%d top score=%.3f xywh=(%.1f,%.1f,%.1fx%.1f)\r\n",
              count,
              boxes[0].score,
              boxes[0].x,
              boxes[0].y,
              boxes[0].w,
              boxes[0].h);
}

int ai_main_start()
{
    model = new PersonDetectModel();
    if (model == NULL) {
        bk_printf("PersonDetectModel alloc failed\r\n");
        return -1;
    }
    model->setBoxDetectionCallback(person_detection_box_cb);

    video_reator = new AvdkVideoReator(model);
    if (video_reator == NULL) {
        bk_printf("AvdkVideoReator alloc failed\r\n");
        return -1;
    }
    if (video_reator->init_model() != BK_OK) {
        bk_printf("PersonDetection model init failed\r\n");
        return -1;
    }
    if (video_reator->OpenCameraWithoutDisplay() != BK_OK) {
        bk_printf("PersonDetection camera open failed\r\n");
        return -1;
    }
    if (video_reator->start_infer() != BK_OK) {
        bk_printf("PersonDetection infer start failed\r\n");
        return -1;
    }

    return 0;
}

int main(void)
{
    bk_init();
    media_service_init();

    bk_printf("M55 main running...\r\n");

    camera_board_config_t camera_board = {0};

    camera_board.mipi.enable = true;
    camera_board.mipi.pin_scl = GPIO_69;
    camera_board.mipi.pin_sda = GPIO_70;
    camera_board.mipi.i2c_id = 1;
    camera_board.mipi.pin_reset = GPIO_71;
    camera_board.mipi.pin_pwdn = -1;
    camera_board.mipi.pin_xclk = GPIO_59;
    camera_board.mipi.sensor_max_width = 1280;
    camera_board.mipi.sensor_max_height = 720;
    camera_board.mipi.sensor_fps = 20;
    camera_board.isp.mp_enable = true;
    camera_board.isp.mp_flexa = false;
    camera_board.isp.mp_width = 320;
    camera_board.isp.mp_height = 180;
#if CONFIG_TFLM_PERSON_DETECTION_RGB
    camera_board.isp.mp_format = BK_PIXEL_FORMAT_BGRA8888;
#elif CONFIG_TFLM_PERSON_DETECTION_GRAY
    camera_board.isp.mp_format = BK_PIXEL_FORMAT_NV12;
#else
#error "CONFIG_TFLM_PERSON_DETECTION_RGB or CONFIG_TFLM_PERSON_DETECTION_GRAY must be enabled"
#endif
    camera_board.isp.sp_enable = false;
    camera_board.isp.sp_flexa = false;

    bk_auxldo_enable();
    bk_frame_buffer_init();

    /* Board config for Multimedia config */
    app_camera_board_config_set(&camera_board);

    /* Debug config for Multimedia */
    //avdk_monitor_init();
    //avdk_monitor_start();
    devices_mgmt_init();

    extern int ai_main_start(void);
    ai_main_start();

    return 0;
}


