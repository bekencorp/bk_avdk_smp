#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>

#include <avdk_error.h>
#include <components/log.h>

#include "jpeg_decode_test.h"

#define TAG "jpeg_dec_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define USE_LEGACY_JPEGD 0

/*
 * Build requirement:
 * - Enable BK JPEG component and its demo so `jpeg_decoder_test()` and
 *   `jpeg_decoder_flexa_test()` are linked.
 * If not enabled, the build will fail at link time due to missing symbols,
 * which exposes the root cause early.
 */
#if USE_LEGACY_JPEGD
extern void jpeg_decoder_test(void);
extern void jpeg_decoder_flexa_test(void);
#endif
#ifdef CONFIG_BK_DECODER
extern void vcdec_jpeg_frame_test(void);
extern void vcdec_jpeg_flexa_test(void);
#endif

/* Run the test case in a dedicated task to avoid CLI task stack/latency issues. */
#define JPEGD_TEST_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define JPEGD_TEST_TASK_STACK_SIZE  (1024 * 16)

static beken_thread_t s_jpegd_test_thread = NULL;
static volatile uint8_t s_jpegd_test_running = 0;

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

typedef enum {
    JPEGD_TEST_ID_JPEG = 0,
    JPEGD_TEST_ID_JPEG_FLEXA = 1,
    JPEGD_TEST_ID_VCDEC_JPEG = 2,
    JPEGD_TEST_ID_VCDEC_JPEG_FLEXA = 3,
} jpegd_test_id_t;

static void jpegd_test_task_entry(void *arg)
{
    jpegd_test_id_t test_id = (jpegd_test_id_t)(uintptr_t)arg;

#ifdef CONFIG_BK_DECODER
    if (test_id == JPEGD_TEST_ID_VCDEC_JPEG) {
        vcdec_jpeg_frame_test();
    } else if (test_id == JPEGD_TEST_ID_VCDEC_JPEG_FLEXA) {
        vcdec_jpeg_flexa_test();
    }
    else
#endif
#if USE_LEGACY_JPEGD
    if (test_id == JPEGD_TEST_ID_JPEG) {
        jpeg_decoder_test();
    } else if (test_id == JPEGD_TEST_ID_JPEG_FLEXA) {
        jpeg_decoder_flexa_test();
    }
    else
#endif
    {
        LOGE("invalid test id=%u\r\n", (unsigned)test_id);
    }

    s_jpegd_test_running = 0;
    s_jpegd_test_thread = NULL;

    /* Self-delete to release task resources. */
    rtos_delete_thread(NULL);
}

static void jpeg_decode_print_usage(void)
{
    bk_printf("Usage:\r\n");
    bk_printf("  jpeg_decode help | -h    - show this help\r\n");
    bk_printf("  jpeg_decode jpegd        - JPEG decode test (internal stream)\r\n");
    bk_printf("  jpeg_decode jpegd_flexa  - JPEG decode test (FLEXA)\r\n");
    bk_printf("  jpeg_decode vcdec_jpegd  - vcdec JPEG decode test (register HAL)\r\n");
    bk_printf("  jpeg_decode vcdec_jpegd_flexa  - vcdec JPEG decode test (FLEXA)\r\n");
}

void cli_jpeg_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = BK_OK;
    jpegd_test_id_t test_id = JPEGD_TEST_ID_JPEG;
    const char *task_name = "jpegd_test";

    if ((pcWriteBuffer == NULL) || (argv == NULL)) {
        /* pcWriteBuffer is required by CLI framework to return command response. */
        ret = BK_FAIL;
        goto exit;
    }

    if (argc < 2) {
        LOGE("%s: invalid params\r\n", __func__);
        jpeg_decode_print_usage();
        ret = BK_FAIL;
        goto exit;
    }

    if ((os_strcmp(argv[1], "help") == 0) || (os_strcmp(argv[1], "-h") == 0)) {
        jpeg_decode_print_usage();
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
        return;
    }

#if CONFIG_BK_DECODER
    if (os_strcmp(argv[1], "vcdec_jpegd") == 0) {
        test_id = JPEGD_TEST_ID_VCDEC_JPEG;
        task_name = "vcdec_jpegd_test";
    } else if (os_strcmp(argv[1], "vcdec_jpegd_flexa") == 0) {
        test_id = JPEGD_TEST_ID_VCDEC_JPEG_FLEXA;
        task_name = "vcdec_jpegd_flexa_test";
    }
    else
#endif
#if USE_LEGACY_JPEGD
    if (os_strcmp(argv[1], "jpegd") == 0) {
        test_id = JPEGD_TEST_ID_JPEG;
        task_name = "jpegd_test";
    } else if (os_strcmp(argv[1], "jpegd_flexa") == 0) {
        test_id = JPEGD_TEST_ID_JPEG_FLEXA;
        task_name = "jpegd_flexa_test";
    }
    else
#endif
    {
        LOGE("%s: unknown subcommand: %s\r\n", __func__, argv[1]);
        jpeg_decode_print_usage();
        ret = BK_FAIL;
        goto exit;
    }

exit:
    if (ret != BK_OK) {
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    if (s_jpegd_test_running) {
        LOGE("jpeg_decode task is already running\r\n");
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    s_jpegd_test_running = 1;
    ret = rtos_create_thread(&s_jpegd_test_thread,
                             JPEGD_TEST_TASK_PRIORITY,
                             task_name,
                             (beken_thread_function_t)jpegd_test_task_entry,
                             JPEGD_TEST_TASK_STACK_SIZE,
                             (beken_thread_arg_t)(uintptr_t)test_id);
    if (ret != BK_OK) {
        LOGE("create jpeg_decode task failed, ret=%d\r\n", ret);
        s_jpegd_test_running = 0;
        s_jpegd_test_thread = NULL;
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
}

