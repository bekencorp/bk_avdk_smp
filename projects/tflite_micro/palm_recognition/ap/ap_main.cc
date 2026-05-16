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
#include <lcd/lcd_mipi_hx8399c_1080x1920.h>
#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

#include "AvdkVideoReatorOSD.h"
#include "AvdkDetectionModel.h"
#include "PalmDetectionModel.h"
#include "box.h"
#include "servo.h"

static void bk_auxldo_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

static AvdkVideoReatorOSD *video_reator = NULL;
static PalmDetectionModel *model = NULL;

static void detection_box_cb(Box *boxes, int count)
{
    if (boxes == NULL || count <= 0) {
        box_detection_path_clear();
        return;
    }

    /* Pick the box with the largest area (w*h) to drive the servo.
     *
     * boxes[] arrives sorted by score (NMS output), but the highest-score box
     * is not always the largest one -- a small but very distinct palm in a
     * corner can outscore a partially-clipped larger palm in the center. For
     * servo tracking we want "the palm closest to the camera", and box area
     * is a robust proxy for that regardless of pose. When count==1 we skip
     * the scan entirely. */
    int target = 0;
    if (count > 1) {
        float best_area = boxes[0].w * boxes[0].h;
        for (int i = 1; i < count; i++) {
            float area = boxes[i].w * boxes[i].h;
            if (area > best_area) {
                best_area = area;
                target = i;
            }
        }
    }

    bk_printf("detection_box_cb: count=%d target=%d score=%.3f xywh=(%.2f,%.2f,%.2fx%.2f)\n",
              count, target, boxes[target].score,
              boxes[target].x, boxes[target].y, boxes[target].w, boxes[target].h);

    /* Draw ALL detected palms. The user can still see secondary palms on the
     * OSD even though the servo only follows the largest one.
     * src = model input size (256x256), dst = display canvas size (1088x1088). */
    box_detection_path_build(boxes, count, count, 0, model->getWidth(), model->getHeight(), 1088, 1088);

    /* Drive servo from the chosen box's center: top-left (x,y) + half size. */
    palm_track_servo(boxes[target].x + boxes[target].w * 0.5f,
                     boxes[target].y + boxes[target].h * 0.5f,
                     model->getWidth(), model->getHeight());
}

int ai_main_start()
{
    servo_init();
    servo_set_angle(SERVO_CENTER_ANGLE);

    model = new PalmDetectionModel();
    model->setBoxDetectionCallback(detection_box_cb);
    video_reator = new AvdkVideoReatorOSD(model);
    video_reator->init();
    video_reator->OpenISPCamera();
    video_reator->OpenDisplay();
    video_reator->start();

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
    gpu_board.flexa.tess_width = 1920 / 4;
    gpu_board.flexa.tess_height = 1080 / 4;

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


