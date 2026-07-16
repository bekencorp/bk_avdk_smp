#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>

#include <avdk_error.h>
#include <components/log.h>

#include "pp_test.h"

#define TAG "pp_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#ifdef CONFIG_BK_DECODER
extern void pp_nv12_rgb565_test(void);
extern void pp_nv12_rgb888_test(void);
extern void pp_nv12_scale_down_test(void);
extern void pp_nv12_scale_up_test(void);
extern void pp_nv12_rgb565_down_test(void);
extern void pp_nv12_rgb888_down_test(void);
extern void pp_nv12_rgb565_up_test(void);
extern void pp_nv12_rgb888_up_test(void);
#endif

#define PP_EXAMPLE_TEST_TASK_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define PP_EXAMPLE_TEST_TASK_STACK_SIZE  (1024 * 16)

static beken_thread_t s_pp_test_thread = NULL;
static volatile uint8_t s_pp_test_running = 0;

typedef enum {
	PP_TEST_ID_NV12_RGB565 = 0,
	PP_TEST_ID_NV12_RGB888,
	PP_TEST_ID_NV12_SCALE_DOWN,
	PP_TEST_ID_NV12_SCALE_UP,
	PP_TEST_ID_NV12_RGB565_DOWN,
	PP_TEST_ID_NV12_RGB888_DOWN,
	PP_TEST_ID_NV12_RGB565_UP,
	PP_TEST_ID_NV12_RGB888_UP,
} pp_test_id_t;

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

static void pp_test_task_entry(void *arg)
{
	pp_test_id_t test_id = (pp_test_id_t)(uintptr_t)arg;

#ifdef CONFIG_BK_DECODER
	switch (test_id) {
	case PP_TEST_ID_NV12_RGB565:
		pp_nv12_rgb565_test();
		break;
	case PP_TEST_ID_NV12_RGB888:
		pp_nv12_rgb888_test();
		break;
	case PP_TEST_ID_NV12_SCALE_DOWN:
		pp_nv12_scale_down_test();
		break;
	case PP_TEST_ID_NV12_SCALE_UP:
		pp_nv12_scale_up_test();
		break;
	case PP_TEST_ID_NV12_RGB565_DOWN:
		pp_nv12_rgb565_down_test();
		break;
	case PP_TEST_ID_NV12_RGB888_DOWN:
		pp_nv12_rgb888_down_test();
		break;
	case PP_TEST_ID_NV12_RGB565_UP:
		pp_nv12_rgb565_up_test();
		break;
	case PP_TEST_ID_NV12_RGB888_UP:
		pp_nv12_rgb888_up_test();
		break;
	default:
		LOGE("invalid test id=%u\r\n", (unsigned)test_id);
		break;
	}
#else
	(void)test_id;
	LOGE("CONFIG_BK_DECODER is disabled\r\n");
#endif

	s_pp_test_running = 0;
	s_pp_test_thread = NULL;
	rtos_delete_thread(NULL);
}

static void pp_print_usage(void)
{
	bk_printf("Usage (source: embedded 1280x720 H.264 -> NV12):\r\n");
	bk_printf("  pp help | -h         - show this help\r\n");
	bk_printf("  pp nv12_rgb565       - NV12 -> RGB565 1280x720\r\n");
	bk_printf("  pp nv12_rgb888       - NV12 -> RGB888 1280x720\r\n");
	bk_printf("  pp nv12_scale_down   - NV12 1280x720 -> 640x360\r\n");
	bk_printf("  pp nv12_scale_up     - NV12 1280x720 -> 1920x1080\r\n");
	bk_printf("  pp nv12_rgb565_down  - NV12 -> RGB565 640x360\r\n");
	bk_printf("  pp nv12_rgb888_down  - NV12 -> RGB888 640x360\r\n");
	bk_printf("  pp nv12_rgb565_up    - NV12 -> RGB565 1920x1080\r\n");
	bk_printf("  pp nv12_rgb888_up    - NV12 -> RGB888 1920x1080\r\n");
}

void cli_pp_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	avdk_err_t ret = BK_OK;
	pp_test_id_t test_id = PP_TEST_ID_NV12_RGB565;
	const char *task_name = "pp_test";

	if ((pcWriteBuffer == NULL) || (argv == NULL)) {
		ret = BK_FAIL;
		goto exit;
	}

	if (argc < 2) {
		LOGE("%s: invalid params\r\n", __func__);
		pp_print_usage();
		ret = BK_FAIL;
		goto exit;
	}

	if ((os_strcmp(argv[1], "help") == 0) || (os_strcmp(argv[1], "-h") == 0)) {
		pp_print_usage();
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
		return;
	}

#if CONFIG_BK_DECODER
	if (os_strcmp(argv[1], "nv12_rgb565") == 0) {
		test_id = PP_TEST_ID_NV12_RGB565;
		task_name = "pp_rgb565_test";
	} else if (os_strcmp(argv[1], "nv12_rgb888") == 0) {
		test_id = PP_TEST_ID_NV12_RGB888;
		task_name = "pp_rgb888_test";
	} else if (os_strcmp(argv[1], "nv12_scale_down") == 0) {
		test_id = PP_TEST_ID_NV12_SCALE_DOWN;
		task_name = "pp_scale_down";
	} else if (os_strcmp(argv[1], "nv12_scale_up") == 0) {
		test_id = PP_TEST_ID_NV12_SCALE_UP;
		task_name = "pp_scale_up";
	} else if (os_strcmp(argv[1], "nv12_rgb565_down") == 0) {
		test_id = PP_TEST_ID_NV12_RGB565_DOWN;
		task_name = "pp_rgb565_down";
	} else if (os_strcmp(argv[1], "nv12_rgb888_down") == 0) {
		test_id = PP_TEST_ID_NV12_RGB888_DOWN;
		task_name = "pp_rgb888_down";
	} else if (os_strcmp(argv[1], "nv12_rgb565_up") == 0) {
		test_id = PP_TEST_ID_NV12_RGB565_UP;
		task_name = "pp_rgb565_up";
	} else if (os_strcmp(argv[1], "nv12_rgb888_up") == 0) {
		test_id = PP_TEST_ID_NV12_RGB888_UP;
		task_name = "pp_rgb888_up";
	} else
#endif
	{
		LOGE("%s: unknown subcommand: %s\r\n", __func__, argv[1]);
		pp_print_usage();
		ret = BK_FAIL;
		goto exit;
	}

exit:
	if (ret != BK_OK) {
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	if (s_pp_test_running) {
		LOGE("pp task is already running\r\n");
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	s_pp_test_running = 1;
	ret = rtos_create_thread(&s_pp_test_thread,
				 PP_EXAMPLE_TEST_TASK_PRIORITY,
				 task_name,
				 (beken_thread_function_t)pp_test_task_entry,
				 PP_EXAMPLE_TEST_TASK_STACK_SIZE,
				 (beken_thread_arg_t)(uintptr_t)test_id);
	if (ret != BK_OK) {
		LOGE("create pp task failed, ret=%d\r\n", ret);
		s_pp_test_running = 0;
		s_pp_test_thread = NULL;
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
}
