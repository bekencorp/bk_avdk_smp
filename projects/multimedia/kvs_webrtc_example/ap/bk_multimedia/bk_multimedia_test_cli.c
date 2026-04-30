#include <os/os.h>
#include <os/str.h>
#include <common/bk_err.h>
#include "cli.h"

#include "doorbell_devices.h"
#include "doorbell_comm.h"

#define TAG "dbkvs-cam"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/**
 * Same as doorbell db_cam, plus:
 *   db_cam transfer on|off  — start/stop encoded frame pump to ntwk_trans (KVS path).
 */
static void bk_mm_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2 || argv[1] == NULL) {
		LOGE("Usage: db_cam open ... | close | transfer on|off\n");
		return;
	}

	if (os_strcmp(argv[1], "transfer") == 0) {
		if (argc < 3) {
			LOGE("Usage: db_cam transfer on|off\n");
			return;
		}
		if (os_strcmp(argv[2], "on") == 0) {
			int r = doorbell_video_transfer_turn_on();
			LOGI("db_cam transfer on ret=%d\n", r);
		} else if (os_strcmp(argv[2], "off") == 0) {
			int r = doorbell_video_transfer_turn_off();
			LOGI("db_cam transfer off ret=%d\n", r);
		} else {
			LOGE("Usage: db_cam transfer on|off\n");
		}
		return;
	}

	if (os_strcmp(argv[1], "close") == 0) {
		int ret = doorbell_camera_turn_off();
		if (ret != BK_OK) {
			LOGE("db_cam close failed, ret=%d\n", ret);
		} else {
			LOGI("db_cam close ok\n");
		}
		return;
	}

	if (os_strcmp(argv[1], "open") != 0) {
		LOGE("Usage: db_cam open uvc ... | open isp | close | transfer on|off\n");
		return;
	}

	if (argc < 3 || argv[2] == NULL) {
		LOGE("Usage: db_cam open uvc <w> <h> <mjpeg|h264> | db_cam open isp\n");
		return;
	}

	camera_parameters_t param;
	os_memset(&param, 0, sizeof(param));
	param.protocol = 0;
	param.rotate = 0;

	if (os_strcmp(argv[2], "uvc") == 0) {
#if !defined(CONFIG_USB_CAMERA)
		LOGE("db_cam: UVC needs CONFIG_USB_CAMERA\n");
		return;
#else
		if (argc < 6) {
			LOGE("Usage: db_cam open uvc <w> <h> <mjpeg|h264>\n");
			return;
		}
		param.id = UVC_DEVICE_ID;
		param.width = (uint16_t)os_strtoul(argv[3], NULL, 10);
		param.height = (uint16_t)os_strtoul(argv[4], NULL, 10);
		if (os_strcmp(argv[5], "h264") == 0) {
			param.format = 1;
		} else if (os_strcmp(argv[5], "mjpeg") == 0) {
			param.format = 0;
		} else {
			LOGE("format must be mjpeg or h264\n");
			return;
		}
#endif
	} else if (os_strcmp(argv[2], "isp") == 0) {
		param.id = 0;
		param.width = 1920;
		param.height = 1080;
		param.format = 0;
	} else {
		LOGE("Usage: db_cam open uvc <w> <h> <mjpeg|h264> | db_cam open isp\n");
		return;
	}

	int ret = doorbell_camera_turn_on(&param);
	if (ret != BK_OK) {
		LOGE("doorbell_camera_turn_on failed, ret=%d\n", ret);
	}
}

static const struct cli_command s_cmds[] = {
	{"mm_test", "mm_test open uvc|isp / close / transfer on|off", bk_mm_test_cmd},
};

void mm_test_cli_init(void)
{
	cli_register_commands(s_cmds, sizeof(s_cmds) / sizeof(s_cmds[0]));
}
