#include <os/os.h>
#include <os/str.h>
#include <os/mem.h>

#include <avdk_error.h>
#include <components/log.h>

#include "cli.h"

#include "jpeg_encode_test.h"
#include "jpeg_encode_display.h"

#define TAG "jpeg_enc_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define JPEG_ENCODE_TEST_TASK_PRIO    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define JPEG_ENCODE_TEST_TASK_STACK   (1024 * 16)
#define JPEG_ENCODE_TEST_MODE_FRAME   (0U)
#define JPEG_ENCODE_TEST_MODE_FLEXA   (1U)
#define JPEG_ENCODE_TEST_MODE_OSD     (2U)
#define JPEG_ENCODE_TEST_MODE_OSD_DISPLAY (3U)
#define JPEG_ENCODE_TEST_MODE_OSD_BENCH   (4U)
#define JPEG_ENCODE_TEST_DISPLAY_SHIFT (8U)
#define JPEG_ENCODE_BENCH_DEFAULT_W    (640U)
#define JPEG_ENCODE_BENCH_DEFAULT_H    (360U)

static beken_thread_t s_jpeg_test_thread;
static volatile uint8_t s_jpeg_test_running;
static uint32_t s_bench_width = JPEG_ENCODE_BENCH_DEFAULT_W;
static uint32_t s_bench_height = JPEG_ENCODE_BENCH_DEFAULT_H;

static const char *jpeg_encode_mode_name(uint32_t mode)
{
	switch (mode) {
	case JPEG_ENCODE_TEST_MODE_FRAME:
		return "frame";
	case JPEG_ENCODE_TEST_MODE_FLEXA:
		return "flexa";
	case JPEG_ENCODE_TEST_MODE_OSD:
		return "osd";
	case JPEG_ENCODE_TEST_MODE_OSD_DISPLAY:
		return "osd_display";
	case JPEG_ENCODE_TEST_MODE_OSD_BENCH:
		return "osd_bench";
	default:
		return "unknown";
	}
}

static const char *jpeg_encode_display_mode_name(uint32_t mode)
{
	switch (mode) {
	case JPEG_ENCODE_OSD_DISPLAY_FRAME:
		return "frame";
	case JPEG_ENCODE_OSD_DISPLAY_FLEXA:
		return "flexa";
	case JPEG_ENCODE_OSD_DISPLAY_SW_FRAME:
		return "sw_frame";
	case JPEG_ENCODE_OSD_DISPLAY_ALL:
		return "all";
	default:
		return "unknown";
	}
}

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
	uint32_t arg_val = (uint32_t)(uintptr_t)arg;
	uint32_t mode = arg_val & 0xFFU;
	uint32_t display_mode = (arg_val >> JPEG_ENCODE_TEST_DISPLAY_SHIFT) & 0xFFU;

	LOGI("jpeg encode task start\r\n");
	switch (mode) {
	case JPEG_ENCODE_TEST_MODE_FRAME:
		(void)bk_jpeg_encode_frame_test();
		break;
	case JPEG_ENCODE_TEST_MODE_FLEXA:
		(void)bk_jpeg_encode_sw_flexa_test();
		break;
	case JPEG_ENCODE_TEST_MODE_OSD:
		(void)bk_jpeg_encode_osd_test();
		break;
	case JPEG_ENCODE_TEST_MODE_OSD_DISPLAY:
		(void)bk_jpeg_encode_osd_display_test((jpeg_encode_osd_display_mode_t)display_mode);
		break;
	case JPEG_ENCODE_TEST_MODE_OSD_BENCH:
		(void)bk_jpeg_encode_osd_bench_test(s_bench_width, s_bench_height);
		break;
	default:
		LOGE("unknown jpeg encode test mode=%u\r\n", (unsigned)mode);
		break;
	}

	s_jpeg_test_running = 0;
	s_jpeg_test_thread = NULL;
	rtos_delete_thread(NULL);
}

/*
 * Parse the osd_bench input size: "<width>x<height>" or the "1080p" shortcut.
 * The bench itself rejects sizes that are not 8-pixel aligned.
 */
static int jpeg_encode_parse_resolution(const char *text, uint32_t *width, uint32_t *height)
{
	uint32_t w = 0U;
	uint32_t h = 0U;
	const char *p = text;

	if (text == NULL) {
		return -1;
	}

	if (os_strcmp(text, "1080p") == 0) {
		*width = 1920U;
		*height = 1080U;
		return 0;
	}

	if (*p < '0' || *p > '9') {
		return -1;
	}
	while (*p >= '0' && *p <= '9') {
		w = w * 10U + (uint32_t)(*p - '0');
		p++;
	}
	if (*p != 'x' && *p != 'X') {
		return -1;
	}
	p++;
	if (*p < '0' || *p > '9') {
		return -1;
	}
	while (*p >= '0' && *p <= '9') {
		h = h * 10U + (uint32_t)(*p - '0');
		p++;
	}
	if (*p != '\0') {
		return -1;
	}

	*width = w;
	*height = h;
	return 0;
}

static void jpeg_encode_print_usage(void)
{
	bk_printf("Usage:\r\n");
	bk_printf("  jpeg_encode help | -h     - show help\r\n");
	bk_printf("  jpeg_encode frame         - VCENC JPEG frame mode, 1920x1080 NV12\r\n");
	bk_printf("  jpeg_encode flexa         - VCENC JPEG sw flexa mode, 1920x1080 NV12\r\n");
	bk_printf("  jpeg_encode osd           - 8-slot OSD (ARGB/NV12/Bitmap) on frame, sw flexa and sw frame\r\n");
	bk_printf("  jpeg_encode osd_display [frame|flexa|sw_frame|all]\r\n");
	bk_printf("                            - OSD encode -> JPEG decode -> GPU -> MIPI LCD (1080x1920)\r\n");
	bk_printf("  jpeg_encode osd_bench [<w>x<h>|1080p]\r\n");
	bk_printf("                            - software OSD per-frame time (ARGB8888 + BITMAP), default %ux%u\r\n",
		   (unsigned)JPEG_ENCODE_BENCH_DEFAULT_W, (unsigned)JPEG_ENCODE_BENCH_DEFAULT_H);
}

void cli_jpeg_encode_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint32_t mode = 0U;
	uint32_t display_mode = (uint32_t)JPEG_ENCODE_OSD_DISPLAY_ALL;

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
	if (os_strcmp(argv[1], "frame") == 0) {
		mode = JPEG_ENCODE_TEST_MODE_FRAME;
	} else if (os_strcmp(argv[1], "flexa") == 0) {
		mode = JPEG_ENCODE_TEST_MODE_FLEXA;
	} else if (os_strcmp(argv[1], "osd") == 0) {
		mode = JPEG_ENCODE_TEST_MODE_OSD;
	} else if (os_strcmp(argv[1], "osd_bench") == 0) {
		mode = JPEG_ENCODE_TEST_MODE_OSD_BENCH;

		s_bench_width = JPEG_ENCODE_BENCH_DEFAULT_W;
		s_bench_height = JPEG_ENCODE_BENCH_DEFAULT_H;
		if (argc > 2) {
			if (jpeg_encode_parse_resolution(argv[2], &s_bench_width,
							 &s_bench_height) != 0) {
				LOGE("%s: invalid osd_bench resolution: %s\r\n", __func__, argv[2]);
				jpeg_encode_print_usage();
				cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
				return;
			}
		}
	} else if (os_strcmp(argv[1], "osd_display") == 0) {
		mode = JPEG_ENCODE_TEST_MODE_OSD_DISPLAY;

		if (argc > 2) {
			if (os_strcmp(argv[2], "frame") == 0) {
				display_mode = (uint32_t)JPEG_ENCODE_OSD_DISPLAY_FRAME;
			} else if ((os_strcmp(argv[2], "flexa") == 0) ||
				   (os_strcmp(argv[2], "sw_flexa") == 0)) {
				display_mode = (uint32_t)JPEG_ENCODE_OSD_DISPLAY_FLEXA;
			} else if (os_strcmp(argv[2], "sw_frame") == 0) {
				display_mode = (uint32_t)JPEG_ENCODE_OSD_DISPLAY_SW_FRAME;
			} else if (os_strcmp(argv[2], "all") == 0) {
				display_mode = (uint32_t)JPEG_ENCODE_OSD_DISPLAY_ALL;
			} else {
				LOGE("%s: unknown osd_display mode: %s\r\n", __func__, argv[2]);
				jpeg_encode_print_usage();
				cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
				return;
			}
		}
	} else {
		LOGE("%s: unknown subcommand: %s\r\n", __func__, argv[1]);
		jpeg_encode_print_usage();
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

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
				 (void *)(uintptr_t)(mode |
						     (display_mode << JPEG_ENCODE_TEST_DISPLAY_SHIFT)));
	if (ret != BK_OK) {
		LOGE("create thread failed, ret=%d\r\n", ret);
		s_jpeg_test_running = 0;
		s_jpeg_test_thread = NULL;
		cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
		return;
	}

	if (mode == JPEG_ENCODE_TEST_MODE_OSD_DISPLAY)
		LOGI("jpeg_encode osd_display(%s) task created\r\n",
		     jpeg_encode_display_mode_name(display_mode));
	else if (mode == JPEG_ENCODE_TEST_MODE_OSD_BENCH)
		LOGI("jpeg_encode osd_bench(%ux%u) task created\r\n",
		     (unsigned)s_bench_width, (unsigned)s_bench_height);
	else
		LOGI("jpeg_encode %s task created\r\n", jpeg_encode_mode_name(mode));
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
