#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <components/bk_frame_buffer.h>
#include <stdint.h>
#include <math.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"
#include "media_service.h"
#include "app_display.h"
#include "app_camera.h"
#include "devices_mgmt.h"
#include "app_gpu.h"
#include "avdk_monitor.h"
#include <lcd/lcd_hx8399c_mipi_1080x1920.h>
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

/* ===================== Servo palm-tracking config ===================== */
/* Servo mechanical limits. Initial angle is also the "neutral" position. */
#define SERVO_CENTER_ANGLE      90
#define SERVO_MIN_ANGLE         0
#define SERVO_MAX_ANGLE         180

/* Gain that turns the normalized palm offset (in [-0.5, +0.5]) into a
 * per-frame angle offset. Larger -> turns more aggressively. */
#define SERVO_TRACK_GAIN        20.0f
/* Hard cap on |offset| each frame, to keep motion smooth. */
#define SERVO_TRACK_MAX_STEP    5
/* Dead band on normalized palm offset; below this the palm is considered
 * already centered and the servo holds its previous angle. */
#define SERVO_TRACK_DEADBAND    0.06f

/* Direction sign mapping "palm offset" -> "servo angle delta".
 *   +1 : palm-on-right (norm > 0) increases the servo angle.
 *   -1 : palm-on-right decreases the servo angle (flip if servo turns the
 *        wrong way physically). */
#define SERVO_TRACK_DIR         (+1)

/* Pick which model axis represents the palm's "left/right" relative to the
 * camera. With the current pipeline (camera -> model 256x256 -> GPU rotates
 * for display), the camera-horizontal direction is model X (cx).
 *   1 : use cy  (model Y)
 *   0 : use cx  (model X)  <-- default for this hardware */
#define SERVO_TRACK_USE_CY      0

/* Last-set servo angle. Treated as "current physical angle" so each frame
 * applies an offset on top of it. Initialised to the neutral position. */
static uint32_t s_servo_angle = SERVO_CENTER_ANGLE;

/**
 * Incremental palm-tracking servo controller.
 *
 *   1. Compute the palm's signed offset from the image center
 *      (norm in [-0.5, +0.5]; negative = left/up, positive = right/down).
 *   2. Convert it into a small per-frame angle offset.
 *   3. Add the offset on top of the last servo angle and update the servo.
 *
 * Effect: the servo follows the palm. Palm on the left -> servo rotates
 * left; palm on the right -> servo rotates right.
 *
 * @param palm_center_x  Palm center X in model coordinates (e.g. 256x256).
 * @param palm_center_y  Palm center Y in model coordinates.
 * @param img_w          Model image width  (e.g. model->getWidth()).
 * @param img_h          Model image height (e.g. model->getHeight()).
 */
static void palm_track_servo(float palm_center_x, float palm_center_y,
                             uint16_t img_w, uint16_t img_h)
{
#if SERVO_TRACK_USE_CY
    const char *axis = "cy";
    float pos = palm_center_y;
    float dim = (float)img_h;
#else
    const char *axis = "cx";
    float pos = palm_center_x;
    float dim = (float)img_w;
#endif
    if (dim <= 0.0f) return;

    int prev = (int)s_servo_angle;

    /* Step 1: signed normalized offset from center.
     *   norm < 0 : palm is on the LEFT  side of the center.
     *   norm > 0 : palm is on the RIGHT side of the center. */
    float norm = (pos - dim * 0.5f) / dim;

    /* Dead band: palm close enough to center -> hold last angle. */
    if (fabsf(norm) < SERVO_TRACK_DEADBAND) {
        bk_printf("[track] axis=%s pos=%.1f/%u norm=%+.3f deadband, hold=%d\n",
                  axis, pos, (unsigned)dim, norm, prev);
        return;
    }

    /* Step 2: convert offset to a signed per-frame angle delta and clamp. */
    int offset = (int)lroundf((float)SERVO_TRACK_DIR * norm * SERVO_TRACK_GAIN);
    if (offset >  SERVO_TRACK_MAX_STEP) offset =  SERVO_TRACK_MAX_STEP;
    if (offset < -SERVO_TRACK_MAX_STEP) offset = -SERVO_TRACK_MAX_STEP;

    /* Step 3: apply on top of the *last* angle, clamp to mechanical limits. */
    int next = prev + offset;
    if (next < SERVO_MIN_ANGLE) next = SERVO_MIN_ANGLE;
    if (next > SERVO_MAX_ANGLE) next = SERVO_MAX_ANGLE;

    bk_printf("[track] axis=%s pos=%.1f/%u norm=%+.3f offset=%+d  %d -> %d%s\n",
              axis, pos, (unsigned)dim, norm, offset, prev, next,
              (next == prev) ? " (saturated)" : "");

    if ((uint32_t)next == s_servo_angle) return;

    s_servo_angle = (uint32_t)next;
    servo_set_angle(s_servo_angle);
}

static void detection_result_cb(int has_palm, float cx, float cy, float w, float h, uint64_t timestamp)
{
    bk_printf("detection_result_cb: has_palm=%d, cx=%f, cy=%f, w=%f, h=%f\n", has_palm, cx, cy, w, h);

    if (has_palm == 0) {
        box_detection_path_clear();
        return;
    }
    /* cx/cy/w/h are in model coordinates (256x256); use truncf to drop the fraction explicitly. */
    Box faces[1];
    faces[0].x1    = (int)truncf(cx);
    faces[0].y1    = (int)truncf(cy);
    faces[0].x2    = (int)truncf(cx + w);
    faces[0].y2    = (int)truncf(cy + h);
    faces[0].score = (float)has_palm;

    /* src = model input size (256x256), dst = display canvas size (1088x1088). */
    box_detection_path_build(faces, 1, 1, 0, model->getWidth(), model->getHeight(), 1088, 1088);

    /* Use the box center (cx,cy are top-left in current usage) to drive servo. */
    palm_track_servo(cx + w * 0.5f, cy + h * 0.5f,
                     model->getWidth(), model->getHeight());
}

int ai_main_start()
{
    servo_init();
    servo_set_angle(90);

    model = new PalmDetectionModel();
    model->setResultCallback(detection_result_cb);
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


