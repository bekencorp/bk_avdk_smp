#include "bk_cli.h"
#include "cli.h"
#include <os/str.h>
#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
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
	{"kvs_wb", "kvs_wb master [channel] - AWS KVS master for kvs_webrtc", kvs_webrtc_cli_cmd},
};

int kvs_webrtc_cli_init(void)
{
	return cli_register_commands(s_cmds, sizeof(s_cmds) / sizeof(s_cmds[0]));
}
