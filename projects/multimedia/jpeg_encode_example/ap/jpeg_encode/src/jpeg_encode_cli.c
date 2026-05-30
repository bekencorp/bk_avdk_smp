#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>

#include <avdk_error.h>
#include <components/log.h>

#include "cli.h"

#include "jpeg_encode_test.h"

#define TAG "jpeg_enc_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define JPEG_ENCODE_TEST_TASK_PRIO    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define JPEG_ENCODE_TEST_TASK_STACK   (1024 * 16)
#define JPEG_ENCODE_TEST_MODE_FRAME   (0U)
#define JPEG_ENCODE_TEST_MODE_FLEXA   (1U)

static beken_thread_t s_jpeg_test_thread;
static volatile uint8_t s_jpeg_test_running;

static void cli_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
	size_t msg_len;

	if (pcWriteBuffer == NULL || xWriteBufferLen <= 0 || msg == NULL)
		return;

	msg_len = os_strlen(msg);
	if (msg_len >= (size_t)xWriteBufferLen)
		msg_len = (size_t)xWriteBufferLen - 1;

	os_memcpy(pcWriteBuffer, msg, msg_len);
	pcWriteBuffer[msg_len] = '\0';
}

static void jpeg_encode_test_task_entry(void *arg)
{
	uint32_t mode = (uint32_t)(uintptr_t)arg;

	LOGI("jpeg encode task start\r\n");
	switch (mode) {
	case JPEG_ENCODE_TEST_MODE_FRAME:
		(void)bk_jpeg_encode_frame_test();
		break;
	case JPEG_ENCODE_TEST_MODE_FLEXA:
		(void)bk_jpeg_encode_sw_flexa_test();
		break;
	default:
		LOGE("unknown jpeg encode test mode=%u\r\n", (unsigned)mode);
		break;
	}

	s_jpeg_test_running = 0;
	s_jpeg_test_thread = NULL;
	rtos_delete_thread(NULL);
}

static void jpeg_encode_print_usage(void)
{
	bk_printf("Usage:\r\n");
	bk_printf("  jpeg_encode help | -h     - show help\r\n");
	bk_printf("  jpeg_encode frame         - VCENC JPEG frame mode, 1920x1080 NV12\r\n");
	bk_printf("  jpeg_encode flexa         - VCENC JPEG sw flexa mode, 1920x1080 NV12\r\n");
}

void cli_jpeg_encode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t mode;

	if ((pcWriteBuffer == NULL) || (argv == NULL)) {
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	if (argc < 2) {
		jpeg_encode_print_usage();
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	if ((os_strcmp(argv[1], "help") == 0) || (os_strcmp(argv[1], "-h") == 0)) {
		jpeg_encode_print_usage();
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
		return;
	}

#ifndef CONFIG_BK_ENCODER
	LOGE("CONFIG_BK_ENCODER is disabled\r\n");
	cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
	return;
#else
	if (os_strcmp(argv[1], "frame") != 0 && os_strcmp(argv[1], "flexa") != 0) {
		LOGE("%s: unknown subcommand: %s\r\n", __func__, argv[1]);
		jpeg_encode_print_usage();
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}
	mode = (os_strcmp(argv[1], "flexa") == 0) ?
	       JPEG_ENCODE_TEST_MODE_FLEXA : JPEG_ENCODE_TEST_MODE_FRAME;

	if (s_jpeg_test_running) {
		LOGE("jpeg_encode task already running\r\n");
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	s_jpeg_test_running = 1;
	ret = rtos_create_thread(&s_jpeg_test_thread,
				 JPEG_ENCODE_TEST_TASK_PRIO,
				 "jpeg_enc_test",
				 (beken_thread_function_t)jpeg_encode_test_task_entry,
				 JPEG_ENCODE_TEST_TASK_STACK,
				 (void *)(uintptr_t)mode);
	if (ret != BK_OK) {
		LOGE("create thread failed, ret=%d\r\n", ret);
		s_jpeg_test_running = 0;
		s_jpeg_test_thread = NULL;
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	LOGI("jpeg_encode %s task created\r\n",
	     (mode == JPEG_ENCODE_TEST_MODE_FLEXA) ? "flexa" : "frame");
	cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
#endif
}

static const struct cli_command s_jpeg_cmds[] = {
	{"jpeg_encode", "JPEG encode test (bk_jpeg_encode_*)", cli_jpeg_encode_cmd},
};

int cli_jpeg_encode_init(void)
{
	return cli_register_commands(s_jpeg_cmds,
				   sizeof(s_jpeg_cmds) / sizeof(s_jpeg_cmds[0]));
}
