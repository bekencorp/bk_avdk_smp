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

#include "video_player_cli.h"
#include "video_recorder_cli.h"

#include "video_player_common.h"

#if CONFIG_BK_DECODER && CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
#include "components/bk_decode/bk_h264_decode_ctlr.h"
#include "components/bk_decode/bk_h264_decode_types.h"
#include "components/bk_video_player/video_decoder/bk_video_player_hw_h264_decoder.h"
#endif

/*
 * GPU rotation applied by the H264 decoder during NV12 -> compressed ARGB
 * blit. Pick to match (source orientation, panel orientation):
 *   1080x1920 portrait source on 1080x1920 portrait panel -> 0
 *   1920x1080 landscape source on 1080x1920 portrait panel -> 90 (CW)
 * 180/270 are also supported by VG-Lite.
 *
 * The new H264 path runs the rotation on the GPU as part of the same blit
 * that produces the compressed ARGB output, so the cost is essentially
 * free compared to the old CPU-side rotation.
 */
#define VIDEO_PLAYER_H264_DECODER_OUTPUT_ROTATION_DEG   0U

#define SYS_ANA_REG_BASE    (0x44010000)
#define LDO_ANA_REG         (0x69)

#define TAG "ap_main"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)


static void bk_auxldo_enable(void)
{
    uint32_t reg = REG_READ(SYS_ANA_REG_BASE + LDO_ANA_REG * 4);
    reg |= (0xF << 28) | (0x2 << 23) | (0x7 << 19) | (0x7 << 15);
    reg &= ~(0xF << 11);
    reg |= (0x8 << 11);
    REG_WRITE(SYS_ANA_REG_BASE + LDO_ANA_REG * 4, reg);
}

#if CONFIG_BK_DECODER && CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
/*
 * Pre-warm the H264 hardware decoder once at boot. Without this prewarm,
 * the first vcdec_h264_init() call from the video_player path fails because
 * some hardware/driver lazy initialization has not been triggered, which
 * surfaces as `vcdec_h2:E: parse NALU type=7 failed` on every AU and
 * eventually a bk_mem_slab_free assert from the leaking annexb buffers.
 * h264_decode_example does the same thing implicitly via vcdec_h264_run_boot_demo().
 *
 * We use the frame-mode controller for the prewarm even though the runtime
 * path is flexa-mode: the underlying vcdec IP is the same; the prewarm
 * exists to fault in the kernel-side resources, not to exercise a specific
 * mode.
 */
static void h264_hw_decoder_prewarm(void)
{
    LOGI("====== h264_prewarm: ENTER, free heap=%u ======\r\n",
              (unsigned)rtos_get_free_heap_size());

    bk_h264_decode_ctlr_handle_t handle = NULL;
    bk_h264_decode_frame_config_t cfg = DEFAULT_H264_DECODE_FRAME_CONFIG;
    cfg.timeout_ms      = 1000U;
    cfg.out_width       = 1280U;
    cfg.out_height      = 720U;
    cfg.out_format      = BK_PIXEL_FORMAT_NV12;
    cfg.frame_done_cb   = NULL;
    cfg.frame_done_args = NULL;

    if (bk_h264_decode_frame_ctlr_new(&handle, &cfg) != BK_OK || handle == NULL) {
        LOGI("====== h264_prewarm: ctlr_new FAILED ======\r\n");
        return;
    }
    LOGI("====== h264_prewarm: ctlr_new OK, free heap=%u ======\r\n",
              (unsigned)rtos_get_free_heap_size());

    if (bk_h264_decode_init(handle) != BK_OK) {
        LOGI("====== h264_prewarm: init FAILED, free heap=%u ======\r\n",
                  (unsigned)rtos_get_free_heap_size());
        (void)bk_h264_decode_delete(handle);
        return;
    }
    LOGI("====== h264_prewarm: init OK, free heap=%u ======\r\n",
              (unsigned)rtos_get_free_heap_size());

    (void)bk_h264_decode_deinit(handle);
    (void)bk_h264_decode_delete(handle);
    LOGI("====== h264_prewarm: DONE, free heap=%u ======\r\n",
              (unsigned)rtos_get_free_heap_size());
}
#endif

int main(void)
{
    bk_init();
    media_service_init();

    LOGI("M55 main running...\r\n");

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
    gpu_board.flexa.tess_width = 0;
    gpu_board.flexa.tess_height = 0;


    bk_auxldo_enable();
    bk_frame_buffer_init();

#if CONFIG_BK_DECODER && CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    /* Prewarm AFTER LDO/frame_buffer init but BEFORE board config / devices_mgmt. */
    h264_hw_decoder_prewarm();

#endif

    /* Board config for Multimedia config */
    app_camera_board_config_set(&camera_board);
    app_display_board_config_set(&display_board);
    app_gpu_board_config_set(&gpu_board);

    /* Debug config for Multimedia */
    avdk_monitor_init();
    avdk_monitor_start();

    devices_mgmt_init();

    cli_video_player_init();
    cli_video_recorder_init();

    return 0;
}
