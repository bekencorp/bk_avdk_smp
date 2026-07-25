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
#include <modules/zbar/zbar_qr.h>
#include "app_camera.h"
#include "app_display.h"
#include "app_gpu.h"

#define TAG "qr"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR   "CMDRSP:ERROR\r\n"

#define QR_WIDTH             640
#define QR_HEIGHT            360
#define QR_FORMAT            BK_PIXEL_FORMAT_NV12
#define QR_GRAY_SIZE         (QR_WIDTH * QR_HEIGHT)
#define QR_THREAD_PRIO       BEKEN_DEFAULT_WORKER_PRIORITY
#define QR_THREAD_STACK      (1024 * 8)
#define QR_READ_TIMEOUT      1000
#define QR_SCAN_INTERVAL_MS  100
#define QR_ZBAR_MAX_RESULTS  4
#define QR_ZBAR_PAYLOAD_SIZE 8896

typedef struct {
    void *isp_gpu_bond;
    beken_thread_t input_thread;
    uint8_t *input_frame;
    uint32_t input_frame_size;
    uint32_t frame_index;
    zbar_qr_context_t *zbar_decoder;
    zbar_qr_result_t zbar_results[QR_ZBAR_MAX_RESULTS];
    uint8_t *zbar_payloads;
    volatile uint8_t input_thread_running;
    uint8_t input_channel;
    const char *input_name;
    uint8_t saved_isp_valid;
    uint8_t saved_mp_enable;
    uint8_t saved_mp_flexa;
    uint16_t saved_mp_width;
    uint16_t saved_mp_height;
    bk_pixel_format_t saved_mp_format;
    uint8_t saved_sp_enable;
    uint8_t opened;
} qr_demo_ctx_t;

static qr_demo_ctx_t s_qr_ctx;

static void qr_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
    if (pcWriteBuffer != NULL && xWriteBufferLen > 0) {
        os_snprintf(pcWriteBuffer, xWriteBufferLen, "%s", msg);
    }
}

static void qr_process_zbar(const uint8_t *frame)
{
    unsigned long long start_us = bk_aon_rtc_get_us();
    int count = zbar_qr_scan(s_qr_ctx.zbar_decoder,
                             frame,
                             QR_WIDTH,
                             QR_HEIGHT,
                             QR_WIDTH,
                             s_qr_ctx.zbar_results,
                             QR_ZBAR_MAX_RESULTS);

    ++s_qr_ctx.frame_index;
    if (count < 0) {
        LOGW("zbar scan failed=%d\r\n", count);
        return;
    }
    if (count == 0) {
        return;
    }

    LOGI("frame=%u found %d QR code(s), scan=%llu us\r\n",
         (unsigned)s_qr_ctx.frame_index,
         count,
         bk_aon_rtc_get_us() - start_us);

    for (int i = 0; i < count; ++i) {
        const zbar_qr_result_t *result = &s_qr_ctx.zbar_results[i];

        LOGI("QR[%d] corners=(%d,%d),(%d,%d),(%d,%d),(%d,%d)\r\n",
             i,
             result->corners[0].x, result->corners[0].y,
             result->corners[1].x, result->corners[1].y,
             result->corners[2].x, result->corners[2].y,
             result->corners[3].x, result->corners[3].y);
        LOGI("QR[%d] len=%u data=%.*s\r\n",
             i,
             (unsigned)result->payload_len,
             (int)result->payload_len,
             (const char *)result->payload);
    }
}

static void qr_process(const uint8_t *frame, uint32_t size)
{
    if (frame == NULL || size < QR_GRAY_SIZE) {
        LOGE("invalid %s frame, frame=%p size=%u\r\n",
             s_qr_ctx.input_name, frame, (unsigned)size);
        return;
    }

    if (s_qr_ctx.zbar_decoder == NULL || s_qr_ctx.zbar_payloads == NULL) {
        LOGE("zbar decoder is not ready\r\n");
        return;
    }
    qr_process_zbar(frame);
}

static void qr_input_thread_entry(void *arg)
{
    qr_demo_ctx_t *ctx = (qr_demo_ctx_t *)arg;

    LOGI("QR %s thread start\r\n", ctx->input_name);
    rtos_delay_milliseconds(1000);

    while (ctx->input_thread_running != 0U) {
        int ret = app_isp_camera_channel_read(ctx->input_channel,
                                              ctx->input_frame,
                                              ctx->input_frame_size,
                                              QR_READ_TIMEOUT);
        if (ret != AVDK_ERR_OK) {
            if (ctx->input_thread_running != 0U) {
                LOGE("read %s frame failed=%d\r\n", ctx->input_name, ret);
                rtos_delay_milliseconds(10);
            }
            continue;
        }

        qr_process(ctx->input_frame, ctx->input_frame_size);
        rtos_delay_milliseconds(QR_SCAN_INTERVAL_MS);
    }

    LOGI("QR %s thread exit\r\n", ctx->input_name);
    ctx->input_thread = NULL;
    rtos_delete_thread(NULL);
}

static avdk_err_t qr_open_camera(void)
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

    LOGI("camera opened: sensor=%ux%u@%u MP=%ux%u\r\n",
         (unsigned)config->mipi.sensor_max_width,
         (unsigned)config->mipi.sensor_max_height,
         (unsigned)config->mipi.sensor_fps,
         (unsigned)config->isp.mp_width,
         (unsigned)config->isp.mp_height);
    return ret;
}

static avdk_err_t qr_open_camera_mp_only(void)
{
    camera_board_config_t *config = app_camera_board_config_get();
    if (config == NULL) {
        LOGE("camera board config is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    s_qr_ctx.saved_isp_valid = 1U;
    s_qr_ctx.saved_mp_enable = config->isp.mp_enable;
    s_qr_ctx.saved_mp_flexa = config->isp.mp_flexa;
    s_qr_ctx.saved_mp_width = config->isp.mp_width;
    s_qr_ctx.saved_mp_height = config->isp.mp_height;
    s_qr_ctx.saved_mp_format = config->isp.mp_format;
    s_qr_ctx.saved_sp_enable = config->isp.sp_enable;

    config->isp.mp_enable = true;
    config->isp.mp_flexa = false;
    config->isp.mp_width = QR_WIDTH;
    config->isp.mp_height = QR_HEIGHT;
    config->isp.mp_format = QR_FORMAT;
    config->isp.sp_enable = false;

    return qr_open_camera();
}

static void qr_restore_camera_config(void)
{
    camera_board_config_t *config = app_camera_board_config_get();

    if (config == NULL || s_qr_ctx.saved_isp_valid == 0U) {
        return;
    }

    config->isp.mp_enable = s_qr_ctx.saved_mp_enable;
    config->isp.mp_flexa = s_qr_ctx.saved_mp_flexa;
    config->isp.mp_width = s_qr_ctx.saved_mp_width;
    config->isp.mp_height = s_qr_ctx.saved_mp_height;
    config->isp.mp_format = s_qr_ctx.saved_mp_format;
    config->isp.sp_enable = s_qr_ctx.saved_sp_enable;
    s_qr_ctx.saved_isp_valid = 0U;
}

static avdk_err_t qr_open_sp_channel(void)
{
    camera_board_config_t *config = app_camera_board_config_get();
    if (config == NULL) {
        LOGE("camera board config is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    config->isp.sp_enable = true;
    config->isp.sp_flexa = false;
    config->isp.sp_width = QR_WIDTH;
    config->isp.sp_height = QR_HEIGHT;
    config->isp.sp_format = QR_FORMAT;

    avdk_err_t ret = app_isp_camera_sp_channel_turn_on(config);
    if (ret != AVDK_ERR_OK) {
        LOGE("app_isp_camera_sp_channel_turn_on failed=%d\r\n", (int)ret);
        return ret;
    }

    LOGI("SP channel opened: %ux%u NV12\r\n", QR_WIDTH, QR_HEIGHT);
    return AVDK_ERR_OK;
}

static avdk_err_t qr_open_buffers(uint8_t channel, const char *name)
{
    s_qr_ctx.input_frame_size = bk_image_size_get(QR_WIDTH, QR_HEIGHT, QR_FORMAT);
    if (s_qr_ctx.input_frame_size < QR_GRAY_SIZE) {
        LOGE("invalid %s frame size=%u\r\n",
             name, (unsigned)s_qr_ctx.input_frame_size);
        return AVDK_ERR_INVAL;
    }

    s_qr_ctx.input_frame = (uint8_t *)bk_frame_buffer_malloc(
        MEM_SLAB_HEAP_UNCODED, s_qr_ctx.input_frame_size);
    if (s_qr_ctx.input_frame == NULL) {
        LOGE("malloc %s frame failed, size=%u\r\n",
             name, (unsigned)s_qr_ctx.input_frame_size);
        return AVDK_ERR_NOMEM;
    }

    size_t payloads_size = QR_ZBAR_MAX_RESULTS * QR_ZBAR_PAYLOAD_SIZE;

    s_qr_ctx.zbar_payloads = (uint8_t *)os_malloc(payloads_size);
    if (s_qr_ctx.zbar_payloads == NULL) {
        LOGE("malloc zbar payload buffers failed, size=%u\r\n",
             (unsigned)payloads_size);
        return AVDK_ERR_NOMEM;
    }
    for (int i = 0; i < QR_ZBAR_MAX_RESULTS; ++i) {
        s_qr_ctx.zbar_results[i].payload =
            s_qr_ctx.zbar_payloads + i * QR_ZBAR_PAYLOAD_SIZE;
        s_qr_ctx.zbar_results[i].payload_capacity = QR_ZBAR_PAYLOAD_SIZE;
    }

    s_qr_ctx.zbar_decoder = zbar_qr_create();
    if (s_qr_ctx.zbar_decoder == NULL) {
        LOGE("zbar_qr_create failed\r\n");
        return AVDK_ERR_NOMEM;
    }

    s_qr_ctx.input_channel = channel;
    s_qr_ctx.input_name = name;
    LOGI("%s input ready: decoder=zbar %ux%u NV12 size=%u, gray=%u\r\n",
         name, QR_WIDTH, QR_HEIGHT,
         (unsigned)s_qr_ctx.input_frame_size, (unsigned)QR_GRAY_SIZE);
    return AVDK_ERR_OK;
}

static avdk_err_t qr_start_input_thread(void)
{
    if (s_qr_ctx.input_thread != NULL ||
        s_qr_ctx.input_frame == NULL ||
        s_qr_ctx.zbar_decoder == NULL) {
        return AVDK_ERR_INVAL;
    }

    s_qr_ctx.input_thread_running = 1U;
    avdk_err_t ret = rtos_create_hsram_thread(
        &s_qr_ctx.input_thread,
        QR_THREAD_PRIO,
        "qr_thread",
        (beken_thread_function_t)qr_input_thread_entry,
        QR_THREAD_STACK,
        &s_qr_ctx);
    if (ret != AVDK_ERR_OK) {
        LOGE("create QR %s thread failed=%d\r\n",
             s_qr_ctx.input_name, (int)ret);
        s_qr_ctx.input_thread_running = 0U;
    }
    return ret;
}

static avdk_err_t qr_open_display(void)
{
    display_board_config_t *config = app_display_board_config_get();
    if (config == NULL) {
        LOGE("display board config is NULL\r\n");
        return AVDK_ERR_INVAL;
    }

    avdk_err_t ret = app_mipi_lcd_turn_on(config);
    if (ret != AVDK_ERR_OK) {
        LOGE("app_mipi_lcd_turn_on failed=%d\r\n", (int)ret);
    }
    return ret;
}

static avdk_err_t qr_open_gpu(void)
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

    ret = bk_flexa_isp_gpu_bond_start(&s_qr_ctx.isp_gpu_bond,
                                      app_isp_handle_get(),
                                      app_gpu_handle_get());
    if (ret != AVDK_ERR_OK) {
        LOGE("isp-gpu bond start failed=%d\r\n", (int)ret);
    }
    return ret;
}

static void qr_close_gpu(void)
{
    if (s_qr_ctx.isp_gpu_bond != NULL) {
        bk_flexa_isp_gpu_bond_stop(s_qr_ctx.isp_gpu_bond);
        s_qr_ctx.isp_gpu_bond = NULL;
    }

    bk_gpu_ctlr_handle_t gpu = app_gpu_handle_get();
    if (gpu != NULL) {
        (void)app_gpu_turn_off(gpu);
    }
}

static void qr_close_display(void)
{
    if (app_mipi_lcd_state_get()) {
        (void)app_mipi_lcd_turn_off();
    }
}

static void qr_close_input(void)
{
    if (s_qr_ctx.input_thread != NULL) {
        s_qr_ctx.input_thread_running = 0U;
        while (s_qr_ctx.input_thread != NULL) {
            rtos_delay_milliseconds(10);
        }
    }

    if (s_qr_ctx.zbar_decoder != NULL) {
        zbar_qr_destroy(s_qr_ctx.zbar_decoder);
        s_qr_ctx.zbar_decoder = NULL;
    }
    if (s_qr_ctx.zbar_payloads != NULL) {
        os_free(s_qr_ctx.zbar_payloads);
        s_qr_ctx.zbar_payloads = NULL;
    }
    if (s_qr_ctx.input_frame != NULL) {
        bk_frame_buffer_free(s_qr_ctx.input_frame);
        s_qr_ctx.input_frame = NULL;
    }
    s_qr_ctx.input_frame_size = 0U;
}

static void qr_close_camera(void)
{
    if (app_isp_handle_get() != NULL) {
        (void)app_isp_camera_turn_off();
    }
}

static avdk_err_t qr_demo_open(void)
{
    if (s_qr_ctx.opened != 0U) {
        LOGW("QR demo already opened\r\n");
        return AVDK_ERR_BUSY;
    }

    os_memset(&s_qr_ctx, 0, sizeof(s_qr_ctx));

    avdk_err_t ret = qr_open_camera();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = qr_open_sp_channel();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = qr_open_buffers(APP_ISP_SP_CHN_ID, "SP");
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = qr_open_display();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = qr_open_gpu();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = qr_start_input_thread();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }

    s_qr_ctx.opened = 1U;
    LOGI("QR preview started; decoder=zbar recognition uses SP %ux%u Y plane\r\n",
         QR_WIDTH, QR_HEIGHT);
    return AVDK_ERR_OK;

error:
    qr_close_gpu();
    qr_close_display();
    qr_close_input();
    qr_close_camera();
    os_memset(&s_qr_ctx, 0, sizeof(s_qr_ctx));
    LOGE("QR preview start failed=%d\r\n", (int)ret);
    return ret;
}

static avdk_err_t qr_demo_open_mp(void)
{
    if (s_qr_ctx.opened != 0U) {
        LOGW("QR demo already opened\r\n");
        return AVDK_ERR_BUSY;
    }

    os_memset(&s_qr_ctx, 0, sizeof(s_qr_ctx));

    avdk_err_t ret = qr_open_camera_mp_only();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = qr_open_buffers(APP_ISP_MP_CHN_ID, "MP");
    if (ret != AVDK_ERR_OK) {
        goto error;
    }
    ret = qr_start_input_thread();
    if (ret != AVDK_ERR_OK) {
        goto error;
    }

    s_qr_ctx.opened = 1U;
    LOGI("QR MP-only started; decoder=zbar at %ux%u NV12\r\n",
         QR_WIDTH, QR_HEIGHT);
    return AVDK_ERR_OK;

error:
    qr_close_input();
    qr_close_camera();
    qr_restore_camera_config();
    os_memset(&s_qr_ctx, 0, sizeof(s_qr_ctx));
    LOGE("QR MP-only start failed=%d\r\n", (int)ret);
    return ret;
}

static avdk_err_t qr_demo_close(void)
{
    if (s_qr_ctx.opened == 0U &&
        s_qr_ctx.isp_gpu_bond == NULL &&
        s_qr_ctx.input_thread == NULL &&
        app_gpu_handle_get() == NULL &&
        !app_mipi_lcd_state_get() &&
        app_isp_handle_get() == NULL) {
        LOGI("QR demo already closed\r\n");
        return AVDK_ERR_OK;
    }

    qr_close_gpu();
    qr_close_display();
    qr_close_input();
    qr_close_camera();
    qr_restore_camera_config();
    os_memset(&s_qr_ctx, 0, sizeof(s_qr_ctx));
    LOGI("QR demo stopped\r\n");
    return AVDK_ERR_OK;
}

static void qr_print_usage(void)
{
    bk_printf("qr help\r\n");
    bk_printf("qr start     - display MP preview; scan SP with zbar\r\n");
    bk_printf("qr start_mp  - scan MP with zbar without display\r\n");
    bk_printf("qr stop\r\n");
}

static void cli_qr_cmd(char *pcWriteBuffer, int xWriteBufferLen,
                       int argc, char **argv)
{
    avdk_err_t ret;

    if (argc < 2 ||
        os_strcmp(argv[1], "help") == 0 ||
        os_strcmp(argv[1], "-h") == 0) {
        qr_print_usage();
        qr_write_rsp(pcWriteBuffer, xWriteBufferLen,
                     (argc < 2) ? CLI_CMD_RSP_ERROR : CLI_CMD_RSP_SUCCEED);
        return;
    }

    if (argc != 2) {
        LOGE("bad arg count\r\n");
        qr_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        ret = qr_demo_open();
    } else if (os_strcmp(argv[1], "start_mp") == 0) {
        ret = qr_demo_open_mp();
    } else if (os_strcmp(argv[1], "stop") == 0) {
        ret = qr_demo_close();
    } else {
        qr_print_usage();
        qr_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    qr_write_rsp(pcWriteBuffer, xWriteBufferLen,
                 (ret == AVDK_ERR_OK) ? CLI_CMD_RSP_SUCCEED : CLI_CMD_RSP_ERROR);
}

int cli_qr_demo_init(void)
{
    static const struct cli_command s_qr_cmds[] = {
        {"qr", "qr help|start|start_mp|stop",
         cli_qr_cmd},
    };

    return cli_register_commands(s_qr_cmds,
                                 sizeof(s_qr_cmds) / sizeof(s_qr_cmds[0]));
}
