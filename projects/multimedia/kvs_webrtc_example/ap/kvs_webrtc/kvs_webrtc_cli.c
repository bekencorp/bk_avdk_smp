#include "bk_cli.h"
#include "cli.h"
#include <os/str.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <stdlib.h>
#include "kvs_common.h"

extern INT32 kvs_webrtc_master_main(INT32 argc, CHAR *argv[]);

#ifndef KVS_DOORBELL_DEFAULT_CHANNEL
#define KVS_DOORBELL_DEFAULT_CHANNEL "kvs_doorbell_channel"
#endif

#define TAG "kvs_db_cli"

static beken_thread_t s_kvs_master_thread;
static volatile bool s_kvs_master_task_running;

static void kvs_webrtc_master_task_entry(beken_thread_arg_t arg)
{
	CHAR *channel = (CHAR *)arg;
	CHAR *argv[2] = { (CHAR *)"kvs_db", channel };
	INT32 ret;

	ret = kvs_webrtc_master_main(2, argv);
	if (ret != 0) {
		BK_LOGW(TAG, "kvs_webrtc_master_main exited %d\r\n", (int)ret);
	}
	os_free(channel);
	s_kvs_master_task_running = false;
	rtos_delete_thread(NULL);
}

static bk_err_t kvs_webrtc_start_master_task(const char *channel)
{
	bk_err_t err;
	CHAR *ch_copy;

	if (s_kvs_master_task_running) {
		BK_LOGW(TAG, "KVS master task already running\r\n");
		return BK_FAIL;
	}

	ch_copy = os_strdup(channel);
	if (ch_copy == NULL) {
		BK_LOGE(TAG, "os_strdup channel failed\r\n");
		return BK_ERR_NO_MEM;
	}

	s_kvs_master_task_running = true;
	err = rtos_create_thread(&s_kvs_master_thread,
							BEKEN_DEFAULT_WORKER_PRIORITY,
							"kvs_mas",
							(beken_thread_function_t)kvs_webrtc_master_task_entry,
							(48 * 1024),
							(beken_thread_arg_t)ch_copy);
	if (err != BK_OK) {
		s_kvs_master_task_running = false;
		os_free(ch_copy);
		BK_LOGE(TAG, "rtos_create_thread failed %d\r\n", (int)err);
		return err;
	}

	BK_LOGI(TAG, "KVS master task started, channel: %s\r\n", channel);
	return BK_OK;
}

static void kvs_webrtc_cli_help(void)
{
	BK_LOG_RAW("kvs_wb master [channel_name]\r\n");
	BK_LOG_RAW("kvs_wb cred <access_key> <secret_key> [region]\r\n");
	BK_LOG_RAW("kvs_wb cred show | clear\r\n");
}

/* Runtime AWS credentials via CLI (RAM only, not persisted). Keeps secrets out
 * of the source tree; set them before running "kvs_wb master". */
static void kvs_webrtc_cred_cmd(int argc, char **argv)
{
	if (argc >= 3 && os_strcmp(argv[2], "show") == 0) {
		char *ak = getenv("AWS_ACCESS_KEY_ID");
		char *sk = getenv("AWS_SECRET_ACCESS_KEY");
		char *rg = getenv("AWS_DEFAULT_REGION");

		BK_LOG_RAW("AWS_ACCESS_KEY_ID: %s\r\n", (ak && ak[0]) ? ak : "(unset)");
		if (sk && sk[0]) {
			BK_LOG_RAW("AWS_SECRET_ACCESS_KEY: set (%d chars)\r\n", (int)os_strlen(sk));
		} else {
			BK_LOG_RAW("AWS_SECRET_ACCESS_KEY: (unset)\r\n");
		}
		BK_LOG_RAW("AWS_DEFAULT_REGION: %s\r\n", (rg && rg[0]) ? rg : "(unset)");
		return;
	}

	if (argc >= 3 && os_strcmp(argv[2], "clear") == 0) {
		setenv("AWS_ACCESS_KEY_ID", "", 1);
		setenv("AWS_SECRET_ACCESS_KEY", "", 1);
		setenv("AWS_DEFAULT_REGION", "", 1);
		BK_LOG_RAW("AWS credentials cleared\r\n");
		return;
	}

	if (argc < 4) {
		BK_LOG_RAW("usage: kvs_wb cred <access_key> <secret_key> [region]\r\n");
		return;
	}

	setenv("AWS_ACCESS_KEY_ID", argv[2], 1);
	setenv("AWS_SECRET_ACCESS_KEY", argv[3], 1);
	if (argc >= 5) {
		setenv("AWS_DEFAULT_REGION", argv[4], 1);
	}
	BK_LOG_RAW("AWS credentials set (RAM). Now run: kvs_wb master\r\n");
}

static void kvs_webrtc_cli_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	const char *channel = KVS_DOORBELL_DEFAULT_CHANNEL;
	bk_err_t ret;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		kvs_webrtc_cli_help();
		return;
	}

	if (os_strcmp(argv[1], "cred") == 0) {
		kvs_webrtc_cred_cmd(argc, argv);
		return;
	}

	if (argc >= 3) {
		channel = argv[2];
	}

	if (os_strcmp(argv[1], "master") == 0) {
		ret = kvs_webrtc_start_master_task(channel);
		if (ret != BK_OK) {
			BK_LOGE(TAG, "start KVS master task failed %d\r\n", (int)ret);
		}
	} else {
		kvs_webrtc_cli_help();
	}
}

static const struct cli_command s_cmds[] = {
	{"kvs_wb", "kvs_wb master [channel] | cred <ak> <sk> [region] - AWS KVS for kvs_webrtc", kvs_webrtc_cli_cmd},
};

int kvs_webrtc_cli_init(void)
{
	return cli_register_commands(s_cmds, sizeof(s_cmds) / sizeof(s_cmds[0]));
}
