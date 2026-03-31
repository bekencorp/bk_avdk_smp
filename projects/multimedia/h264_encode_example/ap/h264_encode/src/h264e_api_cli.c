#include "bk_private/bk_init.h"
#include <stddef.h>
#include <stdint.h>
#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>
#include "cli.h"
#include "h264_encode_test.h"

#define TAG "h264e_api_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

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

/*
 * Build requirement:
 * - Enable BK H264E component and its demo so `bk_h264e_api_test()` is linked.
 * If not enabled, the build will fail at link time due to missing symbol, which
 * exposes the root cause early.
 */
extern int bk_h264e_api_test(void);
extern int bk_h264e_flx_test(void);

/* Run the test case in a dedicated task to avoid CLI task stack/latency issues. */
#define H264E_API_TEST_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define H264E_API_TEST_TASK_STACK_SIZE  (1024 * 16)

static beken_thread_t s_h264e_api_test_thread = NULL;
static volatile uint8_t s_h264e_api_test_running = 0;

typedef enum {
    H264E_TEST_ID_API = 0,
    H264E_TEST_ID_FLX = 1,
} h264e_test_id_t;

static void h264e_api_test_task_entry(void *arg)
{
    h264e_test_id_t test_id = (h264e_test_id_t)(uintptr_t)arg;
    int test_ret = -1;

    if (test_id == H264E_TEST_ID_API) {
        test_ret = bk_h264e_api_test();
    } else if (test_id == H264E_TEST_ID_FLX) {
        test_ret = bk_h264e_flx_test();
    } else {
        LOGE("invalid test id=%u\n", (unsigned)test_id);
        test_ret = -1;
    }

    if (test_ret != 0) {
        LOGE("h264e test failed, id=%u ret=%d\n", (unsigned)test_id, test_ret);
    } else {
        LOGI("h264e test finished, id=%u\n", (unsigned)test_id);
    }

    s_h264e_api_test_running = 0;
    s_h264e_api_test_thread = NULL;

    /* Self-delete to release task resources. */
    rtos_delete_thread(NULL);
}

void cli_h264e_api_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    h264e_test_id_t test_id = H264E_TEST_ID_API;
    const char *task_name = "h264e_api";

    /* Command: h264e_api <api_test|flx_test> */
    if (argc < 2) {
        LOGE("Usage: h264e_api <api_test|flx_test>\n");
        ret = BK_FAIL;
        goto exit;
    }

    if (os_strcmp(argv[1], "api_test") == 0) {
        test_id = H264E_TEST_ID_API;
        task_name = "h264e_api";
    } else if (os_strcmp(argv[1], "flx_test") == 0) {
        test_id = H264E_TEST_ID_FLX;
        task_name = "h264e_flx";
    } else {
        LOGE("Usage: h264e_api <api_test|flx_test>\n");
        ret = BK_FAIL;
        goto exit;
    }

    /*
     * Run the component test case directly.
     * The test case provides its own input frame (static YUV) and output callback.
     */
    {
        if (s_h264e_api_test_running) {
            LOGE("h264e_api_test task is already running\n");
            ret = BK_FAIL;
            goto exit;
        }

        s_h264e_api_test_running = 1;
        ret = rtos_create_thread(&s_h264e_api_test_thread,
                                 H264E_API_TEST_TASK_PRIORITY,
                                 task_name,
                                 (beken_thread_function_t)h264e_api_test_task_entry,
                                 H264E_API_TEST_TASK_STACK_SIZE,
                                 (beken_thread_arg_t)(uintptr_t)test_id);
        if (ret != BK_OK) {
            LOGE("create h264e_api_test task failed, ret=%d\n", ret);
            s_h264e_api_test_running = 0;
            s_h264e_api_test_thread = NULL;
            goto exit;
        }
    }

exit:
    if (ret != BK_OK) {
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
}

