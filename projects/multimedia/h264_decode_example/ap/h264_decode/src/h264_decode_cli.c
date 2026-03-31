#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>

#include <avdk_error.h>
#include <components/log.h>

#include "h264_decode_test.h"

#define TAG "h264_dec_cli"

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
 * - Enable BK H264D component and its demo so `h264_decoder_test()` and
 *   `jpeg_decoder_test()` are linked.
 * If not enabled, the build will fail at link time due to missing symbols,
 * which exposes the root cause early.
 */
extern void h264_decoder_test(void);
extern void jpeg_decoder_test(void);
extern void h264_decoder_flexa_test(void);
extern void jpeg_decoder_flexa_test(void);
#ifdef CONFIG_BK_DECODER
extern void vcdec_jpeg_test(void);
extern void vcdec_jpeg_flexa_test(void);
#endif

/* Run the test case in a dedicated task to avoid CLI task stack/latency issues. */
#define H264D_TEST_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define H264D_TEST_TASK_STACK_SIZE  (1024 * 16)

static beken_thread_t s_h264d_test_thread = NULL;
static volatile uint8_t s_h264d_test_running = 0;

typedef enum {
    H264D_TEST_ID_H264 = 0,
    H264D_TEST_ID_H264_FLEXA = 1,
    H264D_TEST_ID_JPEG = 2,
    H264D_TEST_ID_JPEG_FLEXA = 3,
    H264D_TEST_ID_VCDEC_JPEG = 4,
    H264D_TEST_ID_VCDEC_JPEG_FLEXA = 5,
} h264d_test_id_t;

static void h264d_test_task_entry(void *arg)
{
    h264d_test_id_t test_id = (h264d_test_id_t)(uintptr_t)arg;

    if (test_id == H264D_TEST_ID_H264) {
        h264_decoder_test();
    } else if (test_id == H264D_TEST_ID_H264_FLEXA) {
        h264_decoder_flexa_test();
    } else if (test_id == H264D_TEST_ID_JPEG) {
        jpeg_decoder_test();
    } else if (test_id == H264D_TEST_ID_JPEG_FLEXA) {
        jpeg_decoder_flexa_test();
#ifdef CONFIG_BK_DECODER
    } else if (test_id == H264D_TEST_ID_VCDEC_JPEG) {
        vcdec_jpeg_test();
    } else if (test_id == H264D_TEST_ID_VCDEC_JPEG_FLEXA) {
        vcdec_jpeg_flexa_test();
#endif
    } else {
        LOGE("invalid test id=%u\r\n", (unsigned)test_id);
    }

    s_h264d_test_running = 0;
    s_h264d_test_thread = NULL;

    /* Self-delete to release task resources. */
    rtos_delete_thread(NULL);
}

static void h264_decode_print_usage(void)
{
    bk_printf("Usage:\r\n");
    bk_printf("  h264_decode help | -h    - show this help\r\n");
    bk_printf("  h264_decode h264d        - H.264 decode test (internal stream)\r\n");
    bk_printf("  h264_decode h264d_flexa  - H.264 decode test (FLEXA)\r\n");
    bk_printf("  h264_decode jpegd        - JPEG decode test (internal stream)\r\n");
    bk_printf("  h264_decode jpegd_flexa  - JPEG decode test (FLEXA)\r\n");
#ifdef CONFIG_BK_DECODER
    bk_printf("  h264_decode vcdec_jpegd  - vcdec JPEG decode test (register HAL)\r\n");
    bk_printf("  h264_decode vcdec_jpegd_flexa  - vcdec JPEG decode test (FLEXA + PP rb)\r\n");
#endif
}

void cli_h264_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = BK_OK;
    h264d_test_id_t test_id = H264D_TEST_ID_H264;
    const char *task_name = "h264d_test";

    if ((pcWriteBuffer == NULL) || (argv == NULL)) {
        /* pcWriteBuffer is required by CLI framework to return command response. */
        ret = BK_FAIL;
        goto exit;
    }

    if (argc < 2) {
        LOGE("%s: invalid params\r\n", __func__);
        h264_decode_print_usage();
        ret = BK_FAIL;
        goto exit;
    }

    if ((os_strcmp(argv[1], "help") == 0) || (os_strcmp(argv[1], "-h") == 0)) {
        h264_decode_print_usage();
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
        return;
    }

    if (os_strcmp(argv[1], "h264d") == 0) {
        test_id = H264D_TEST_ID_H264;
        task_name = "h264d_test";
    } else if (os_strcmp(argv[1], "h264d_flexa") == 0) {
        test_id = H264D_TEST_ID_H264_FLEXA;
        task_name = "h264d_flexa_test";
    } else if (os_strcmp(argv[1], "jpegd") == 0) {
        test_id = H264D_TEST_ID_JPEG;
        task_name = "jpegd_test";
    } else if (os_strcmp(argv[1], "jpegd_flexa") == 0) {
        test_id = H264D_TEST_ID_JPEG_FLEXA;
        task_name = "jpegd_flexa_test";
#ifdef CONFIG_BK_DECODER
    } else if (os_strcmp(argv[1], "vcdec_jpegd") == 0) {
        test_id = H264D_TEST_ID_VCDEC_JPEG;
        task_name = "vcdec_jpegd_test";
    } else if (os_strcmp(argv[1], "vcdec_jpegd_flexa") == 0) {
        test_id = H264D_TEST_ID_VCDEC_JPEG_FLEXA;
        task_name = "vcdec_jpegd_flexa_test";
#endif
    } else {
        LOGE("%s: unknown subcommand: %s\r\n", __func__, argv[1]);
        h264_decode_print_usage();
        ret = BK_FAIL;
        goto exit;
    }

exit:
    if (ret != BK_OK) {
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    if (s_h264d_test_running) {
        LOGE("h264_decode task is already running\r\n");
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    s_h264d_test_running = 1;
    ret = rtos_create_thread(&s_h264d_test_thread,
                             H264D_TEST_TASK_PRIORITY,
                             task_name,
                             (beken_thread_function_t)h264d_test_task_entry,
                             H264D_TEST_TASK_STACK_SIZE,
                             (beken_thread_arg_t)(uintptr_t)test_id);
    if (ret != BK_OK) {
        LOGE("create h264_decode task failed, ret=%d\r\n", ret);
        s_h264d_test_running = 0;
        s_h264d_test_thread = NULL;
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
}

