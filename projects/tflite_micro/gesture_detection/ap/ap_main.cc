#include "bk_private/bk_init.h"
#include <components/system.h>
extern "C" {
#include <os/os.h>
}
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
#include <lcd/lcd_lt8912b_mipi_bridge.h>
#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

#include "AvdkVideoReator.h"
#include "AvdkDetectionModel.h"
#include "GestureDetectionModel.h"
#include "app_event.h"

#if CONFIG_LVGL
#include "lv_vendor.h"
#include "beken_ui.h"
#endif
#include "driver/drv_tp.h"
#include <components/bk_frame_buffer.h>


static void bk_auxldo_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

static AvdkVideoReator *video_reator = NULL;
static GestureDetectionModel *model = NULL;

/**
 * @brief Callback function for gesture detection results.
 *
 * This callback is called by GestureDetectionModel when a gesture is detected.
 * It forwards the gesture result to app_event module, which will check
 * if the game is in the correct state and phase to accept the gesture.
 *
 * @param gesture Detected gesture: GESTURE_ROCK, GESTURE_PAPER, GESTURE_SCISSORS, or GESTURE_MAX (invalid).
 */
static void gesture_result_callback(gesture_result_t gesture)
{
    /* Forward gesture result to app_event module.
     * app_event_on_gesture() will check:
     *   - Game state is APP_STATE_GAME
     *   - Game phase is GAME_PHASE_WAIT_GESTURE
     *   - gesture_accept_enabled is true
     *   - No gesture has been received yet for this round
     * If any condition is not met, the gesture will be ignored.
     */
    app_event_on_gesture((int)gesture);
}

/**
 * @brief Callback function for gesture detection image data.
 *
 * This callback is called by GestureDetectionModel after each detection
 * with the input image data that was fed to the model.
 * It forwards the image data to app_event module, which will backup
 * the image if needed (valid gesture detected before timeout, or after timeout).
 *
 * @param image_data Pointer to image pixel data (BGRA8888 format).
 * @param width Image width in pixels.
 * @param height Image height in pixels.
 * @param format Pixel format (bk_pixel_format_t).
 * @param data_size Total size of image data in bytes.
 */
static void gesture_image_callback(const uint8_t *image_data, uint32_t width, uint32_t height, bk_pixel_format_t format, uint32_t data_size)
{
    /* Forward image data to app_event module.
     * app_event_on_image() will check:
     *   - Game state is APP_STATE_GAME
     *   - Game phase is GAME_PHASE_WAIT_GESTURE
     *   - need_backup_image flag is set
     * If conditions are met, the image will be backed up for display.
     */
    app_event_on_image(image_data, width, height, (uint32_t)format, data_size);
}

int ai_main_start()
{
    model = new GestureDetectionModel();
    /* Register callback to receive gesture detection results */
    model->setResultCallback(gesture_result_callback);
    /* Register callback to receive gesture detection image data */
    model->setImageCallback(gesture_image_callback);
    video_reator = new AvdkVideoReator(model);
    video_reator->init_model();
#if CONFIG_USB_CAMERA
    video_reator->OpenUVCCameraWithDisplay();
#else
    video_reator->OpenCameraWithoutDisplay();
    video_reator->OpenDisplayWithoutGPU();
#endif
    //video_reator->start(AVDK_VIDEO_REATOR_MODE_NODISPLAY);

    return 0;
}

extern "C" int gesture_detection_start(void)
{
    if (video_reator == NULL)
    {
        return -1;
    }
    int ret = 0;

    /* Start inference thread after camera is opened */
    ret = video_reator->start_detect();
    if (ret != BK_OK)
    {
        bk_printf("start_detect failed, ret: %d\n", ret);
        return ret;
    }

    return 0;
}

extern "C" int gesture_detection_stop(void)
{
    if (video_reator == NULL)
    {
        return -1;
    }

    /* Stop inference thread first */
    int ret = video_reator->stop_detect();
    if (ret != BK_OK)
    {
        bk_printf("stop_detect failed, ret: %d\n", ret);
    }

    return 0;
}

static void bk_ui_flush_cb(void *args, void *frame_buffer, int (*cb)(void *args))
{
    bk_display_flush((bk_display_ctlr_handle_t)args, frame_buffer, cb);
}

static void lvgl_ui_init(void)
{
    lv_vnd_config_t lv_vnd_config = {0};

    #define WIDTH (1280)
    #define HEIGHT (720)

    lv_vnd_config.width = WIDTH;
    lv_vnd_config.height = HEIGHT;
    lv_vnd_config.render_mode = RENDER_PARTIAL_MODE;
    lv_vnd_config.draw_pixel_size = WIDTH * 36 * sizeof(bk_color_t);

    lv_vnd_config.rotation = ROTATE_NONE;
	lv_vnd_config.frame_buffer[0] = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, WIDTH * HEIGHT * sizeof(bk_color_t));
	// lv_vnd_config.frame_buffer[1] = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, WIDTH * HEIGHT * sizeof(bk_color_t));
    lv_vnd_config.args = app_mipi_lcd_handle_get();
    lv_vnd_config.flush_cb = bk_ui_flush_cb;

    lv_vendor_init(&lv_vnd_config);

#if (CONFIG_TP)
    drv_tp_open(lv_vnd_config.width, lv_vnd_config.height, TP_MIRROR_NONE);
#endif

    lv_vendor_disp_lock();
    beken_ui_init();
    lv_vendor_disp_unlock();

    lv_vendor_start();
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
    display_board.mipi.pin_reset = -1;
    display_board.mipi.pin_backlight = -1;
    display_board.mipi.pin_scl = GPIO_7;
    display_board.mipi.pin_sda = GPIO_60;
    display_board.mipi.panel = &lcd_device_lt8912b_mipi;
    display_board.dpu_video.enable = true;
    display_board.dpu_video.decompress = false;
    display_board.dpu_video.format = BK_PIXEL_FORMAT_RGB565;

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

    // Initialize app_event module for gesture detection game logic
    if (app_event_init() != BK_OK)
    {
        bk_printf("app_event_init failed\r\n");
    }
    else
    {
        // Register test CLI commands
        if (app_event_test_cli_init() != 0)
        {
            bk_printf("app_event_test_cli_init failed\r\n");
        }

        // Start first game round automatically after power-on.
        if (app_event_post(APP_EVENT_PROMPT_GET_READY, 0) != BK_OK)
        {
            bk_printf("post initial APP_EVENT_PROMPT_GET_READY failed\r\n");
        }
    }

    ai_main_start();

    lvgl_ui_init();

    int ret = video_reator->start_display();
    if (ret != BK_OK)
    {
        bk_printf("start_display failed, ret: %d\n", ret);
        return ret;
    }

    return 0;
}


