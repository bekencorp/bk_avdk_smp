#include <common/bk_include.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>
#include <driver/aon_rtc.h>

#include "cli.h"
#include "bk_private/bk_cli.h"

#include <avdk_error.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_flexa_bond.h>
#include "modules/motion_detect.h"
#include "app_camera.h"
#include "app_display.h"
#include "app_gpu.h"

#define TAG "motion_detect"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR   "CMDRSP:ERROR\r\n"

#define MOTION_DETECT_WIDTH             320
#define MOTION_DETECT_HEIGHT            180
#define MOTION_DETECT_FORMAT            BK_PIXEL_FORMAT_NV12
#define MOTION_DETECT_FRAME_SIZE        (MOTION_DETECT_WIDTH * MOTION_DETECT_HEIGHT)
#define MOTION_DETECT_THREAD_PRIO       BEKEN_DEFAULT_WORKER_PRIORITY
#define MOTION_DETECT_THREAD_STACK      (1024 * 4)
#define MOTION_DETECT_READ_TIMEOUT      100
#define MOTION_DETECT_DUMP_DELAY_BYTES  4
#define MOTION_DETECT_DIFF_THRESHOLD    64
#define MOTION_DETECT_COUNT_THRESHOLD   128

typedef struct {
    void *isp_gpu_bond;
    beken_thread_t input_thread;
    uint8_t *input_frame[2];
    uint8_t *old_frame;
    uint8_t *motion_counts;
    uint32_t input_frame_size;
    uint32_t frame_index;
    volatile uint8_t input_thread_running;
    volatile uint8_t dump_next_frame;
    uint8_t input_write_index;
    uint8_t input_channel;
    const char *input_name;
    uint8_t has_old_frame;
    uint8_t saved_isp_valid;
    uint8_t saved_mp_enable;
    uint8_t saved_mp_flexa;
    uint16_t saved_mp_width;
    uint16_t saved_mp_height;
    bk_pixel_format_t saved_mp_format;
    uint8_t saved_sp_enable;
    uint8_t opened;
} motion_detect_demo_ctx_t;

static motion_detect_demo_ctx_t s_motion_ctx;

static void motion_detect_log_interval(const char *prefix, unsigned long long start_us)
{
    unsigned long long elapsed_us = bk_aon_rtc_get_us() - start_us;
    LOGI("%s: %llu us\r\n", prefix, elapsed_us);
}

static void motion_detect_dump_hex(const char *name, const uint8_t *buf, int len)
{
    bk_printf_raw(BK_LOG_INFO, NULL, "==== dump %s len=%d ====\r\n", name, len);
    int i = 0;
    for (; i + 12 <= len; i += 12) {
        bk_printf_raw(BK_LOG_INFO, NULL,
                      "0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X\r\n",
                      buf[i + 0], buf[i + 1], buf[i + 2], buf[i + 3],
                      buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7],
                      buf[i + 8], buf[i + 9], buf[i + 10], buf[i + 11]);
        if ((i + 12) % MOTION_DETECT_DUMP_DELAY_BYTES == 0) {
            rtos_delay_milliseconds(20);
        }
    }
    for (; i < len; ++i) {
        bk_printf_raw(BK_LOG_INFO, NULL, "0x%02X ", buf[i]);
    }
    bk_printf_raw(BK_LOG_INFO, NULL, "\r\n==== dump %s end ====\r\n", name);
}

static void motion_detect_process(uint8_t *frame, uint32_t size)
{
    unsigned long long detect_start_us;
    motion_detect_config_t config = {
        .width = MOTION_DETECT_WIDTH,
        .height = MOTION_DETECT_HEIGHT,
        .diff_threshold = MOTION_DETECT_DIFF_THRESHOLD,
        .count_threshold = MOTION_DETECT_COUNT_THRESHOLD,
    };
    motion_detect_result_t result;
    int ret;

    if (s_motion_ctx.dump_next_frame != 0U) {
        s_motion_ctx.dump_next_frame = 0U;
        if (frame != NULL && size >= MOTION_DETECT_FRAME_SIZE) {
            motion_detect_dump_hex("motion_frame", frame, MOTION_DETECT_FRAME_SIZE);
        } else {
            LOGE("invalid %s frame for dump, frame=%p size=%u\r\n",
                 s_motion_ctx.input_name, frame, (unsigned)size);
        }
    }

    if (frame == NULL ||
        size < MOTION_DETECT_FRAME_SIZE ||
        s_motion_ctx.old_frame == NULL ||
        s_motion_ctx.motion_counts == NULL) {
        LOGE("invalid %s frame for motion detect, frame=%p size=%u old=%p counts=%p\r\n",
             s_motion_ctx.input_name, frame, (unsigned)size,
             s_motion_ctx.old_frame, s_motion_ctx.motion_counts);
        return;
    }

    if (s_motion_ctx.has_old_frame == 0U) {
        os_memcpy(s_motion_ctx.old_frame, frame, MOTION_DETECT_FRAME_SIZE);
        s_motion_ctx.has_old_frame = 1U;
        s_motion_ctx.frame_index = 1U;
        LOGI("motion_detect first frame saved: %ux%u\r\n",
             MOTION_DETECT_WIDTH, MOTION_DETECT_HEIGHT);
        return;
    }

    detect_start_us = bk_aon_rtc_get_us();
    ret = motion_detect_compare_gray(&config,
                                     frame,
                                     s_motion_ctx.old_frame,
                                     s_motion_ctx.motion_counts,
                                     &result);
    motion_detect_log_interval("motion_detect core process", detect_start_us);
    if (ret != MOTION_DETECT_OK) {
        LOGE("motion detect core failed=%d\r\n", ret);
        return;
    }

    if (result.moving != 0U) {
        os_memcpy(s_motion_ctx.old_frame, frame, MOTION_DETECT_FRAME_SIZE);
    }

    ++s_motion_ctx.frame_index;
    LOGI("motion_detect frame=%u state=%s total=%u max_3x3[%u]=%u "
         "rows=%u/%u/%u cols=%u/%u/%u\r\n",
         (unsigned)s_motion_ctx.frame_index,
         result.moving ? "moving" : "still",
         (unsigned)result.total_count,
         (unsigned)result.max_block_index,
         (unsigned)result.max_block_count,
         (unsigned)result.row_counts[0],
         (unsigned)result.row_counts[1],
         (unsigned)result.row_counts[2],
         (unsigned)result.col_counts[0],
         (unsigned)result.col_counts[1],
         (unsigned)result.col_counts[2]);
}

static void motion_detect_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
    if (pcWriteBuffer != NULL && xWriteBufferLen > 0) {
        os_snprintf(pcWriteBuffer, xWriteBufferLen, "%s", msg);
    }
}

static void motion_detect_input_thread_entry(void *arg)
{
    motion_detect_demo_ctx_t *ctx = (motion_detect_demo_ctx_t *)arg;
    int ret;

    LOGI("motion_detect %s thread start\r\n", ctx->input_name);

    rtos_delay_milliseconds(1000);

    while (ctx->input_thread_running != 0U) {
        uint8_t *input_frame = ctx->input_frame[ctx->input_write_index];

        if (input_frame == NULL) {
            LOGE("%s frame buffer %u is NULL\r\n",
                 ctx->input_name, (unsigned)ctx->input_write_index);
            rtos_delay_milliseconds(10);
            continue;
        }

        ret = app_isp_camera_channel_read(ctx->input_channel,
                                          input_frame,
                                          ctx->input_frame_size,
                                          MOTION_DETECT_READ_TIMEOUT);
        if (ret != AVDK_ERR_OK) {
            if (ctx->input_thread_running != 0U) {
                LOGE("read %s frame failed=%d\r\n", ctx->input_name, ret);
                rtos_delay_milliseconds(10);
            }
            continue;
        }


        motion_detect_process(input_frame, ctx->input_frame_size);
        ctx->input_write_index ^= 1U;

        rtos_delay_milliseconds(1000);
    }

    LOGI("motion_detect %s thread exit\r\n", ctx->input_name);
    ctx->input_thread = NULL;
    rtos_delete_thread(NULL);
}

static avdk_err_t motion_detect_open_camera(void)
{
    camera_board_config_t *config = app_camera_board_config_get();
    if (config == NULL) {
        LOGE("camera board config is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    avdk_err_t ret = app_isp_mipi_camera_turn_on(config);
    if (ret != AVDK_ERR_OK) {
        LOGE("app_isp_mipi_camera_turn_on failed=%d\r\n", (int)ret);
        return ret;
    }

    LOGI("camera opened by app_camera: sensor=%ux%u@%u MP=%ux%u\r\n",
         (unsigned)config->mipi.sensor_max_width,
         (unsigned)config->mipi.sensor_max_height,
         (unsigned)config->mipi.sensor_fps,
         (unsigned)config->isp.mp_width,
         (unsigned)config->isp.mp_height);
    return ret;
}

static avdk_err_t motion_detect_open_camera_mp_only(void)
{
    camera_board_config_t *config = app_camera_board_config_get();
    if (config == NULL) {
        LOGE("camera board config is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    s_motion_ctx.saved_isp_valid = 1U;
    s_motion_ctx.saved_mp_enable = config->isp.mp_enable;
    s_motion_ctx.saved_mp_flexa = config->isp.mp_flexa;
    s_motion_ctx.saved_mp_width = config->isp.mp_width;
    s_motion_ctx.saved_mp_height = config->isp.mp_height;
    s_motion_ctx.saved_mp_format = config->isp.mp_format;
    s_motion_ctx.saved_sp_enable = config->isp.sp_enable;

    config->isp.mp_enable = true;
    config->isp.mp_flexa = false;
    config->isp.mp_width = MOTION_DETECT_WIDTH;
    config->isp.mp_height = MOTION_DETECT_HEIGHT;
    config->isp.mp_format = MOTION_DETECT_FORMAT;
    config->isp.sp_enable = false;

    return motion_detect_open_camera();
}

static void motion_detect_restore_camera_config(void)
{
    camera_board_config_t *config = app_camera_board_config_get();

    if (config == NULL || s_motion_ctx.saved_isp_valid == 0U) {
        return;
    }

    config->isp.mp_enable = s_motion_ctx.saved_mp_enable;
    config->isp.mp_flexa = s_motion_ctx.saved_mp_flexa;
    config->isp.mp_width = s_motion_ctx.saved_mp_width;
    config->isp.mp_height = s_motion_ctx.saved_mp_height;
    config->isp.mp_format = s_motion_ctx.saved_mp_format;
    config->isp.sp_enable = s_motion_ctx.saved_sp_enable;
    s_motion_ctx.saved_isp_valid = 0U;
}

static avdk_err_t motion_detect_open_input_buffers(uint8_t channel, const char *name)
{
    avdk_err_t ret;
    uint32_t motion_count_size;

    if (s_motion_ctx.input_thread != NULL) {
        LOGW("motion_detect %s thread already started\r\n", name);
        return AVDK_ERR_BUSY;
    }

    s_motion_ctx.input_frame_size = bk_image_size_get(MOTION_DETECT_WIDTH,
                                                      MOTION_DETECT_HEIGHT,
                                                      MOTION_DETECT_FORMAT);
    if (s_motion_ctx.input_frame_size == 0U) {
        LOGE("invalid %s frame size\r\n", name);
        return AVDK_ERR_INVAL;
    }

    for (uint32_t i = 0; i < 2U; ++i) {
        s_motion_ctx.input_frame[i] = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                                                        s_motion_ctx.input_frame_size);
        if (s_motion_ctx.input_frame[i] == NULL) {
            LOGE("malloc %s frame[%u] failed, size=%u\r\n",
                 name, (unsigned)i, (unsigned)s_motion_ctx.input_frame_size);
            ret = AVDK_ERR_NOMEM;
            goto error;
        }
    }

    s_motion_ctx.old_frame = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                                              MOTION_DETECT_FRAME_SIZE);
    if (s_motion_ctx.old_frame == NULL) {
        LOGE("malloc old frame failed, size=%u\r\n", (unsigned)MOTION_DETECT_FRAME_SIZE);
        ret = AVDK_ERR_NOMEM;
        goto error;
    }

    s_motion_ctx.input_write_index = 0U;
    s_motion_ctx.input_channel = channel;
    s_motion_ctx.input_name = name;

    motion_count_size = motion_detect_get_count_buffer_size(MOTION_DETECT_WIDTH,
                                                            MOTION_DETECT_HEIGHT);
    if (motion_count_size == 0U) {
        LOGE("invalid motion count size, width=%u height=%u\r\n",
             (unsigned)MOTION_DETECT_WIDTH, (unsigned)MOTION_DETECT_HEIGHT);
        ret = AVDK_ERR_INVAL;
        goto error;
    }

    s_motion_ctx.motion_counts = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                                                   motion_count_size);
    if (s_motion_ctx.motion_counts == NULL) {
        LOGE("malloc motion count failed, size=%u\r\n", (unsigned)motion_count_size);
        ret = AVDK_ERR_NOMEM;
        goto error;
    }

    LOGI("%s buffers ready: %ux%u fmt=%u size=%u\r\n",
         name,
         MOTION_DETECT_WIDTH,
         MOTION_DETECT_HEIGHT,
         MOTION_DETECT_FORMAT,
         (unsigned)s_motion_ctx.input_frame_size);
    return AVDK_ERR_OK;

error:
    if (s_motion_ctx.motion_counts != NULL) {
        bk_frame_buffer_free(s_motion_ctx.motion_counts);
        s_motion_ctx.motion_counts = NULL;
    }
    if (s_motion_ctx.old_frame != NULL) {
        bk_frame_buffer_free(s_motion_ctx.old_frame);
        s_motion_ctx.old_frame = NULL;
    }
    for (uint32_t i = 0; i < 2U; ++i) {
        if (s_motion_ctx.input_frame[i] != NULL) {
            bk_frame_buffer_free(s_motion_ctx.input_frame[i]);
            s_motion_ctx.input_frame[i] = NULL;
        }
    }
    s_motion_ctx.input_frame_size = 0U;
    return ret;
}

static avdk_err_t motion_detect_open_camera_sp_channel(void)
{
    camera_board_config_t *config = app_camera_board_config_get();
    avdk_err_t ret;

    if (config == NULL) {
        LOGE("camera board config is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    config->isp.sp_enable = true;
    config->isp.sp_flexa = false;
    config->isp.sp_width = MOTION_DETECT_WIDTH;
    config->isp.sp_height = MOTION_DETECT_HEIGHT;
    config->isp.sp_format = MOTION_DETECT_FORMAT;

    ret = app_isp_camera_sp_channel_turn_on(config);
    if (ret != AVDK_ERR_OK) {
        LOGE("app_isp_camera_sp_channel_turn_on failed=%d\r\n", (int)ret);
        return ret;
    }

    LOGI("SP channel opened: %ux%u fmt=%u\r\n",
         MOTION_DETECT_WIDTH, MOTION_DETECT_HEIGHT, MOTION_DETECT_FORMAT);
    return AVDK_ERR_OK;
}

static avdk_err_t motion_detect_start_input_thread(void)
{
    if (s_motion_ctx.input_thread != NULL) {
        LOGW("motion_detect %s thread already started\r\n", s_motion_ctx.input_name);
        return AVDK_ERR_BUSY;
    }

    if (s_motion_ctx.input_frame[0] == NULL ||
        s_motion_ctx.input_frame[1] == NULL ||
        s_motion_ctx.input_frame_size == 0U) {
        LOGE("motion_detect %s frame is not ready\r\n", s_motion_ctx.input_name);
        return AVDK_ERR_INVAL;
    }

    s_motion_ctx.input_thread_running = 1U;
    avdk_err_t ret = rtos_create_hsram_thread(&s_motion_ctx.input_thread,
                                              MOTION_DETECT_THREAD_PRIO,
                                              "motion_detect_thread",
                                              (beken_thread_function_t)motion_detect_input_thread_entry,
                                              MOTION_DETECT_THREAD_STACK,
                                              &s_motion_ctx);
    if (ret != AVDK_ERR_OK) {
        LOGE("create motion %s thread failed=%d\r\n", s_motion_ctx.input_name, (int)ret);
        s_motion_ctx.input_thread_running = 0U;
        return ret;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t motion_detect_open_display(void)
{
    display_board_config_t *config = app_display_board_config_get();
    if (config == NULL) {
        LOGE("display board config is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    avdk_err_t ret = app_mipi_lcd_turn_on(config);
    if (ret != AVDK_ERR_OK) {
        LOGE("app_mipi_lcd_turn_on failed=%d\r\n", (int)ret);
        return ret;
    }

    LOGI("display opened by app_display\r\n");
    return ret;
}

static avdk_err_t motion_detect_open_gpu(void)
{
    gpu_board_config_t *config = app_gpu_board_config_get();
    if (config == NULL) {
        LOGE("gpu board config is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    avdk_err_t ret = app_gpu_turn_on(config);
    if (ret != AVDK_ERR_OK) {
        LOGE("app_gpu_turn_on failed=%d\r\n", (int)ret);
        return ret;
    }

    ret = bk_flexa_isp_gpu_bond_start(&s_motion_ctx.isp_gpu_bond,
                                      app_isp_handle_get(),
                                      app_gpu_handle_get());
    if (ret != AVDK_ERR_OK) {
        LOGE("isp-gpu bond start failed=%d\r\n", (int)ret);
        return ret;
    }

    LOGI("gpu opened by app_gpu: src=%ux%u dst=%ux%u rotate=%u\r\n",
         (unsigned)config->flexa.src_width,
         (unsigned)config->flexa.src_height,
         (unsigned)config->flexa.dst_width,
         (unsigned)config->flexa.dst_height,
         (unsigned)config->flexa.degree);
    return ret;
}

static void motion_detect_close_gpu(void)
{
    if (s_motion_ctx.isp_gpu_bond != NULL) {
        bk_flexa_isp_gpu_bond_stop(s_motion_ctx.isp_gpu_bond);
        s_motion_ctx.isp_gpu_bond = NULL;
    }

    bk_gpu_ctlr_handle_t gpu = app_gpu_handle_get();
    if (gpu != NULL) {
        (void)app_gpu_turn_off(gpu);
    }
}

static void motion_detect_close_display(void)
{
    if (app_mipi_lcd_state_get()) {
        (void)app_mipi_lcd_turn_off();
    }
}

static void motion_detect_close_input(void)
{
    if (s_motion_ctx.input_thread != NULL) {
        s_motion_ctx.input_thread_running = 0U;
        while (s_motion_ctx.input_thread != NULL) {
            rtos_delay_milliseconds(10);
        }
    }

    if (s_motion_ctx.input_thread == NULL) {
        for (uint32_t i = 0; i < 2U; ++i) {
            if (s_motion_ctx.input_frame[i] != NULL) {
                bk_frame_buffer_free(s_motion_ctx.input_frame[i]);
                s_motion_ctx.input_frame[i] = NULL;
            }
        }
        s_motion_ctx.input_frame_size = 0U;
        if (s_motion_ctx.old_frame != NULL) {
            bk_frame_buffer_free(s_motion_ctx.old_frame);
            s_motion_ctx.old_frame = NULL;
        }
        s_motion_ctx.has_old_frame = 0U;
        s_motion_ctx.frame_index = 0U;
        s_motion_ctx.input_write_index = 0U;
    }

    if (s_motion_ctx.input_thread == NULL && s_motion_ctx.motion_counts != NULL) {
        bk_frame_buffer_free(s_motion_ctx.motion_counts);
        s_motion_ctx.motion_counts = NULL;
    }
}

static void motion_detect_close_camera(void)
{
    if (app_isp_handle_get() != NULL) {
        (void)app_isp_camera_turn_off();
    }
}

static avdk_err_t motion_detect_demo_open(void)
{
    if (s_motion_ctx.opened != 0U) {
        LOGW("motion_detect already opened\r\n");
        return AVDK_ERR_BUSY;
    }

    os_memset(&s_motion_ctx, 0, sizeof(s_motion_ctx));

    avdk_err_t ret = motion_detect_open_camera();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = motion_detect_open_camera_sp_channel();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = motion_detect_open_input_buffers(APP_ISP_SP_CHN_ID, "SP");
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = motion_detect_open_display();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = motion_detect_open_gpu();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = motion_detect_start_input_thread();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }

    s_motion_ctx.opened = 1U;
    LOGI("motion_detect preview started\r\n");
    return AVDK_ERR_OK;

error:
    motion_detect_close_gpu();
    motion_detect_close_display();
    motion_detect_close_input();
    motion_detect_close_camera();
    os_memset(&s_motion_ctx, 0, sizeof(s_motion_ctx));
    LOGE("motion_detect preview start failed=%d\r\n", (int)ret);
    return ret;
}

static avdk_err_t motion_detect_demo_open_mp(void)
{
    if (s_motion_ctx.opened != 0U) {
        LOGW("motion_detect already opened\r\n");
        return AVDK_ERR_BUSY;
    }

    os_memset(&s_motion_ctx, 0, sizeof(s_motion_ctx));

    avdk_err_t ret = motion_detect_open_camera_mp_only();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = motion_detect_open_input_buffers(APP_ISP_MP_CHN_ID, "MP");
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = motion_detect_start_input_thread();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }

    s_motion_ctx.opened = 1U;
    LOGI("motion_detect MP-only started\r\n");
    return AVDK_ERR_OK;

error:
    motion_detect_close_input();
    motion_detect_close_camera();
    motion_detect_restore_camera_config();
    os_memset(&s_motion_ctx, 0, sizeof(s_motion_ctx));
    LOGE("motion_detect MP-only start failed=%d\r\n", (int)ret);
    return ret;
}

static avdk_err_t motion_detect_demo_close(void)
{
    if (s_motion_ctx.opened == 0U &&
        s_motion_ctx.isp_gpu_bond == NULL &&
        app_gpu_handle_get() == NULL &&
        !app_mipi_lcd_state_get() &&
        s_motion_ctx.input_thread == NULL &&
        app_isp_handle_get() == NULL) {
        LOGI("motion_detect already closed\r\n");
        return AVDK_ERR_OK;
    }

    motion_detect_close_gpu();
    motion_detect_close_display();
    motion_detect_close_input();
    motion_detect_close_camera();
    motion_detect_restore_camera_config();
    os_memset(&s_motion_ctx, 0, sizeof(s_motion_ctx));
    LOGI("motion_detect stopped\r\n");
    return AVDK_ERR_OK;
}

static void motion_detect_print_usage(void)
{
    camera_board_config_t *camera = app_camera_board_config_get();
    display_board_config_t *display = app_display_board_config_get();
    gpu_board_config_t *gpu = app_gpu_board_config_get();

    bk_printf("motion_detect help\r\n");
    bk_printf("motion_detect start\r\n");
    bk_printf("motion_detect start_mp\r\n");
    bk_printf("motion_detect stop\r\n");
    bk_printf("motion_detect dump\r\n");
    if (camera != NULL) {
        bk_printf("  camera: %ux%u@%u, ISP MP-only %ux%u NV12, SP detect %ux%u NV12\r\n",
                  (unsigned)camera->mipi.sensor_max_width,
                  (unsigned)camera->mipi.sensor_max_height,
                  (unsigned)camera->mipi.sensor_fps,
                  MOTION_DETECT_WIDTH,
                  MOTION_DETECT_HEIGHT,
                  MOTION_DETECT_WIDTH,
                  MOTION_DETECT_HEIGHT);
    }
    if (display != NULL && display->mipi.panel != NULL) {
        bk_printf("  display: %s\r\n", display->mipi.panel->name);
    }
    if (gpu != NULL) {
        bk_printf("  gpu: rotate=%u dst=%ux%u\r\n",
                  (unsigned)gpu->flexa.degree,
                  (unsigned)gpu->flexa.dst_width,
                  (unsigned)gpu->flexa.dst_height);
    }
}

static void cli_motion_detect_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (argc < 2 ||
        os_strcmp(argv[1], "help") == 0 ||
        os_strcmp(argv[1], "-h") == 0) {
        motion_detect_print_usage();
        motion_detect_write_rsp(pcWriteBuffer, xWriteBufferLen,
                                (argc < 2) ? CLI_CMD_RSP_ERROR : CLI_CMD_RSP_SUCCEED);
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        if (argc != 2) {
            LOGE("bad arg count, expect start\r\n");
            motion_detect_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
            return;
        }

        ret = motion_detect_demo_open();
        motion_detect_write_rsp(pcWriteBuffer, xWriteBufferLen,
                                (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR);
        return;
    }

    if (os_strcmp(argv[1], "start_mp") == 0) {
        if (argc != 2) {
            LOGE("bad arg count, expect start_mp\r\n");
            motion_detect_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
            return;
        }

        ret = motion_detect_demo_open_mp();
        motion_detect_write_rsp(pcWriteBuffer, xWriteBufferLen,
                                (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR);
        return;
    }

    if (os_strcmp(argv[1], "stop") == 0) {
        ret = motion_detect_demo_close();
        motion_detect_write_rsp(pcWriteBuffer, xWriteBufferLen,
                                (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR);
        return;
    }

    if (os_strcmp(argv[1], "dump") == 0) {
        if (argc != 2) {
            LOGE("bad arg count, expect dump\r\n");
            motion_detect_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
            return;
        }

        s_motion_ctx.dump_next_frame = 1U;
        LOGI("will dump next motion_detect process frame\r\n");
        motion_detect_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
        return;
    }

    motion_detect_print_usage();
    motion_detect_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
}

int cli_motion_detect_demo_init(void)
{
    static const struct cli_command s_motion_detect_cmds[] = {
        {"motion_detect",
         "motion_detect help|start|start_mp|stop|dump",
         cli_motion_detect_cmd},
    };

    return cli_register_commands(s_motion_detect_cmds,
                                 sizeof(s_motion_detect_cmds) / sizeof(s_motion_detect_cmds[0]));
}
