#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>

#include <avdk_error.h>
#include <components/log.h>

#include "h264_decode_test.h"

#define TAG "h264_dec_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* Run the test case in a dedicated task to avoid CLI task stack/latency issues. */
#define H264D_TEST_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define H264D_TEST_TASK_STACK_SIZE  (1024 * 16)

static beken_thread_t s_h264d_test_thread = NULL;
static volatile uint8_t s_h264d_test_running = 0;

typedef enum {
    H264D_TEST_ID_VCDEC_H264 = 0,
    H264D_TEST_ID_VCDEC_H264_FLEXA = 1,
    H264D_TEST_ID_VCDEC_H264_FRAME_ZC = 2,
    H264D_TEST_ID_VCDEC_H264_FRAME_RGB = 3,
} h264d_test_id_t;

#define H264D_TEST_THREAD_ARG(test_id, stream_id) \
    ((uintptr_t)(((uint32_t)(test_id) & 0xFFU) | (((uint32_t)(stream_id) & 0xFFU) << 8U)))
#define H264D_TEST_THREAD_ID(arg) \
    ((h264d_test_id_t)((uint32_t)(uintptr_t)(arg) & 0xFFU))
#define H264D_TEST_THREAD_STREAM(arg) \
    ((h264_decode_test_stream_t)(((uint32_t)(uintptr_t)(arg) >> 8U) & 0xFFU))

#ifdef CONFIG_BK_DECODER
extern void vcdec_h264_frame_test(h264_decode_test_stream_t stream);
extern void vcdec_h264_flexa_test(h264_decode_test_stream_t stream);
extern void vcdec_h264_frame_zerocopy_test(h264_decode_test_stream_t stream);
extern void vcdec_h264_frame_rgb_test(void);
#endif

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

static const char *h264_decode_stream_name(h264_decode_test_stream_t stream_id)
{
    switch (stream_id) {
    case H264_DECODE_TEST_STREAM_1280X720_1I30P:
        return "1280x720_1i30p";
    case H264_DECODE_TEST_STREAM_1280X720_IBBP:
        return "1280x720_ibbp";
    default:
        return "unknown";
    }
}

static int h264_decode_parse_stream_arg(const char *arg, h264_decode_test_stream_t *stream_id)
{
    if (arg == NULL || stream_id == NULL) {
        return -1;
    }

    if (os_strcmp(arg, "1280x720_1i30p") == 0) {
        *stream_id = H264_DECODE_TEST_STREAM_1280X720_1I30P;
        return 0;
    }

    if (os_strcmp(arg, "1280x720_ibbp") == 0) {
        *stream_id = H264_DECODE_TEST_STREAM_1280X720_IBBP;
        return 0;
    }

    /* `1280x720` is kept as an alias for the IBBP (B-frame) stream. */
    if (os_strcmp(arg, "1280x720") == 0) {
        *stream_id = H264_DECODE_TEST_STREAM_1280X720_IBBP;
        return 0;
    }

    return -1;
}

static void h264d_test_task_entry(void *arg)
{
    h264d_test_id_t test_id = H264D_TEST_THREAD_ID(arg);
    h264_decode_test_stream_t stream_id = H264D_TEST_THREAD_STREAM(arg);

    LOGI("h264 test task start, mode=%u stream=%s\r\n",
         (unsigned)test_id, h264_decode_stream_name(stream_id));

#ifdef CONFIG_BK_DECODER
    if (test_id == H264D_TEST_ID_VCDEC_H264) {
        vcdec_h264_frame_test(stream_id);
    } else if (test_id == H264D_TEST_ID_VCDEC_H264_FLEXA) {
        vcdec_h264_flexa_test(stream_id);
    } else if (test_id == H264D_TEST_ID_VCDEC_H264_FRAME_ZC) {
        vcdec_h264_frame_zerocopy_test(stream_id);
    } else if (test_id == H264D_TEST_ID_VCDEC_H264_FRAME_RGB) {
        vcdec_h264_frame_rgb_test();
    }
    else
#endif
    {
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
#ifdef CONFIG_BK_DECODER
    bk_printf("  h264_decode vcdec_h264d [1280x720_1i30p|1280x720_ibbp]        - vcdec H.264 frame decode test (whole-frame, non-B)\r\n");
    bk_printf("  h264_decode vcdec_h264d_flexa [1280x720_1i30p|1280x720_ibbp]  - vcdec H.264 decode test (FLEXA, non-B)\r\n");
    bk_printf("  h264_decode vcdec_h264d_frame_zerocopy [1280x720_1i30p|1280x720_ibbp] - vcdec H.264 zero-copy/B-frame frame decode test\r\n");
    bk_printf("    (stream defaults to 1280x720_ibbp; `1280x720` is an alias for it)\r\n");
    bk_printf("  h264_decode vcdec_h264d_frame_rgb                 - vcdec H.264 frame RGB565/RGB888 format test\r\n");
#endif
}

void cli_h264_decode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    avdk_err_t ret = BK_OK;
    h264d_test_id_t test_id = H264D_TEST_ID_VCDEC_H264;
    h264_decode_test_stream_t stream_id = H264_DECODE_TEST_STREAM_1280X720_IBBP;
    const char *task_name = "h264d_test";
    uint8_t need_stream_arg = 0U;

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

#ifdef CONFIG_BK_DECODER
    if (os_strcmp(argv[1], "vcdec_h264d") == 0) {
        test_id = H264D_TEST_ID_VCDEC_H264;
        task_name = "vcdec_h264d_test";
        need_stream_arg = 1U;
    } else if (os_strcmp(argv[1], "vcdec_h264d_flexa") == 0) {
        test_id = H264D_TEST_ID_VCDEC_H264_FLEXA;
        task_name = "vcdec_h264d_flexa_test";
        need_stream_arg = 1U;
    } else if (os_strcmp(argv[1], "vcdec_h264d_frame_zerocopy") == 0) {
        test_id = H264D_TEST_ID_VCDEC_H264_FRAME_ZC;
        task_name = "vcdec_h264d_fzc_test";
        need_stream_arg = 1U;
    } else if (os_strcmp(argv[1], "vcdec_h264d_frame_rgb") == 0) {
        test_id = H264D_TEST_ID_VCDEC_H264_FRAME_RGB;
        task_name = "vcdec_h264d_frgb_test";
    }
    else
#endif
    {
        LOGE("%s: unknown subcommand: %s\r\n", __func__, argv[1]);
        h264_decode_print_usage();
        ret = BK_FAIL;
        goto exit;
    }

    if (need_stream_arg) {
        if (argc >= 3) {
            if (h264_decode_parse_stream_arg(argv[2], &stream_id) != 0) {
                LOGE("%s: unknown stream: %s\r\n", __func__, argv[2]);
                h264_decode_print_usage();
                ret = BK_FAIL;
                goto exit;
            }
        }
    } else if (argc >= 3) {
        LOGE("%s: subcommand %s does not accept stream argument\r\n", __func__, argv[1]);
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
    {
        ret = rtos_create_thread(&s_h264d_test_thread,
                                 H264D_TEST_TASK_PRIORITY,
                                 task_name,
                                 (beken_thread_function_t)h264d_test_task_entry,
                                 H264D_TEST_TASK_STACK_SIZE,
                                 (beken_thread_arg_t)H264D_TEST_THREAD_ARG(test_id, (uint32_t)stream_id));
    }
    if (ret != BK_OK) {
        LOGE("create h264_decode task failed, ret=%d\r\n", ret);
        s_h264d_test_running = 0;
        s_h264d_test_thread = NULL;
        cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    LOGI("create h264_decode task, mode=%s stream=%s\r\n",
         (test_id == H264D_TEST_ID_VCDEC_H264_FLEXA) ? "flexa" : "frame",
         h264_decode_stream_name(stream_id));
    cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
}

