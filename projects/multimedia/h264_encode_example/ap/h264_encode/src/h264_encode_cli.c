#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>

#include <avdk_error.h>
#include <components/log.h>

#include "h264_encode_test.h"

#define TAG "h264_enc_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define USE_LEGACY_H264E 0

typedef enum {
    H264E_TEST_ID_API = 0,
    H264E_TEST_ID_FLX = 1,
    H264E_TEST_ID_VCENC_H264 = 2,
    H264E_TEST_ID_VCENC_H264_FLEXA = 3,
} h264e_test_id_t;


#ifdef CONFIG_BK_ENCODER
extern int vcenc_h264_frame_test(void);
extern int vcenc_h264_flexa_test(void);
#endif

#if USE_LEGACY_H264E
extern int bk_h264e_api_test(void);
extern int bk_h264e_flx_test(void);
#endif

/* Run the test case in a dedicated task to avoid CLI task stack/latency issues. */
#define H264E_TEST_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define H264E_TEST_TASK_STACK_SIZE  (1024 * 16)

static beken_thread_t s_h264e_test_thread = NULL;
static volatile uint8_t s_h264e_test_running = 0;

static void cli_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
    size_t msg_len = 0;

    if (pcWriteBuffer == NULL || xWriteBufferLen <= 0 || msg == NULL) {
        return;
    }

    msg_len = os_strlen(msg);
    if (msg_len >= (size_t)xWriteBufferLen) {
        msg_len = (size_t)xWriteBufferLen - 1;
    }

    os_memcpy(pcWriteBuffer, msg, msg_len);
    pcWriteBuffer[msg_len] = '\0';
}

static const char *h264_encode_mode_name(h264e_test_id_t test_id)
{
    switch (test_id) {
    case H264E_TEST_ID_VCENC_H264:
        return "frame";
    case H264E_TEST_ID_VCENC_H264_FLEXA:
        return "flexa";
    default:
        return "unknown";
    }
}

static void h264e_test_task_entry(void *arg)
{
    h264e_test_id_t test_id = (h264e_test_id_t)(uintptr_t)arg;
    int test_ret = BK_FAIL;

    LOGI("h264 encode task start, mode=%s\r\n", h264_encode_mode_name(test_id));

#ifdef CONFIG_BK_ENCODER
    if (test_id == H264E_TEST_ID_VCENC_H264) {
        test_ret = vcenc_h264_frame_test();
    } else if (test_id == H264E_TEST_ID_VCENC_H264_FLEXA) {
        test_ret = vcenc_h264_flexa_test();
    }
    else
#endif
#if USE_LEGACY_H264E
    if (test_id == H264E_TEST_ID_API) {
        test_ret = bk_h264e_api_test();
    } else if (test_id == H264E_TEST_ID_FLX) {
        test_ret = bk_h264e_flx_test();
    }
    else
#endif
    {
        LOGE("invalid test id=%u\r\n", (unsigned)test_id);
    }
    if (test_ret == BK_OK) {
        LOGI("h264 encode task success, mode=%s\r\n", h264_encode_mode_name(test_id));
    } else {
        LOGE("h264 encode task failed, mode=%s ret=%d\r\n",
             h264_encode_mode_name(test_id), test_ret);
    }

    s_h264e_test_running = 0;
    s_h264e_test_thread = NULL;

    /* Self-delete to release task resources. */
    rtos_delete_thread(NULL);
}

static void h264_encode_print_usage(void)
{
    bk_printf("Usage:\r\n");
    bk_printf("  h264_encode help | -h       - show this help\r\n");
    bk_printf("  h264_encode vcenc_h264e     - vcenc H.264 encode test, frame mode, 256x128 NV12\r\n");
    bk_printf("  h264_encode vcenc_h264e_flexa - vcenc H.264 encode test, FLEXA mode, 256x128 NV12\r\n");
}

void cli_h264_encode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = BK_OK;
    h264e_test_id_t test_id = H264E_TEST_ID_VCENC_H264;
    const char *task_name = "vcenc_h264e";

    if ((pcWriteBuffer == NULL) || (argv == NULL)) {
        ret = BK_FAIL;
        goto exit;
    }

    if (argc < 2) {
        LOGE("%s: invalid params\r\n", __func__);
        h264_encode_print_usage();
        ret = BK_FAIL;
        goto exit;
    }

    if ((os_strcmp(argv[1], "help") == 0) || (os_strcmp(argv[1], "-h") == 0)) {
        h264_encode_print_usage();
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
        return;
    }

#if CONFIG_BK_ENCODER
    if (os_strcmp(argv[1], "vcenc_h264e") == 0) {
        test_id = H264E_TEST_ID_VCENC_H264;
        task_name = "vcenc_h264e";
    } else if (os_strcmp(argv[1], "vcenc_h264e_flexa") == 0) {
        test_id = H264E_TEST_ID_VCENC_H264_FLEXA;
        task_name = "vcenc_h264e_flexa";
    }
    else
#endif
#if USE_LEGACY_H264E
    if (os_strcmp(argv[1], "h264e") == 0) {
        test_id = H264E_TEST_ID_API;
        task_name = "h264e";
    } else if (os_strcmp(argv[1], "h264e_flexa") == 0) {
        test_id = H264E_TEST_ID_FLX;
        task_name = "h264e_flexa";
    }
    else
#endif
    {
        LOGE("%s: unknown subcommand: %s\r\n", __func__, argv[1]);
        h264_encode_print_usage();
        ret = BK_FAIL;
        goto exit;
    }

exit:
    if (ret != BK_OK) {
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    if (s_h264e_test_running) {
        LOGE("h264_encode task is already running\r\n");
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    s_h264e_test_running = 1;
    ret = rtos_create_thread(&s_h264e_test_thread,
                             H264E_TEST_TASK_PRIORITY,
                             task_name,
                             (beken_thread_function_t)h264e_test_task_entry,
                             H264E_TEST_TASK_STACK_SIZE,
                             (beken_thread_arg_t)(uintptr_t)test_id);
    if (ret != BK_OK) {
        LOGE("create h264_encode task failed, ret=%d\r\n", ret);
        s_h264e_test_running = 0;
        s_h264e_test_thread = NULL;
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    LOGI("create h264_encode task, mode=%s\r\n", h264_encode_mode_name(test_id));
    cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
}

