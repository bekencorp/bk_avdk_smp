/**
 * KVS AWS CLI: select role (master/viewer) by command instead of macro.
 * Usage:
 *   kvs master [channel_name]  |  kvs viewer [channel_name]
 *   kvs cred <access_key> <secret_key> [region]
 *   kvs cred show | clear
 */
#include "bk_cli.h"
#include "cli.h"
#include <os/str.h>
#include <stdlib.h>
#include <components/log.h>

#include "Samples.h"

extern INT32 kvs_aws_master_main(INT32 argc, CHAR *argv[]);
extern INT32 kvs_aws_viewer_main(INT32 argc, CHAR *argv[]);

#ifndef KVS_AWS_DEFAULT_CHANNEL_NAME
#define KVS_AWS_DEFAULT_CHANNEL_NAME "kvs_aws_channel"
#endif

#define KVS_CLI_TAG "kvs_cli"

#define KVS_CMD_CNT (sizeof(s_kvs_commands) / sizeof(struct cli_command))

static void kvs_cli_help(char *pcWriteBuffer, int xWriteBufferLen)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	BK_LOG_RAW("kvs <role> [channel_name]\r\n");
	BK_LOG_RAW("  role: master | viewer\r\n");
	BK_LOG_RAW("  channel_name: optional, default \"" KVS_AWS_DEFAULT_CHANNEL_NAME "\"\r\n");
	BK_LOG_RAW("kvs cred <access_key> <secret_key> [region]\r\n");
	BK_LOG_RAW("kvs cred show | clear\r\n");
	BK_LOG_RAW("  e.g. kvs master\r\n");
	BK_LOG_RAW("       kvs viewer my_channel\r\n");
	BK_LOG_RAW("       kvs cred <ak> <sk> [region]\r\n");
}

/* Runtime AWS credentials via CLI (RAM only, not persisted). Keeps secrets out
 * of the source tree; set them before running "kvs master" / "kvs viewer". */
static void kvs_cred_cmd(int argc, char **argv)
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
		BK_LOG_RAW("usage: kvs cred <access_key> <secret_key> [region]\r\n");
		return;
	}

	setenv("AWS_ACCESS_KEY_ID", argv[2], 1);
	setenv("AWS_SECRET_ACCESS_KEY", argv[3], 1);
	if (argc >= 5) {
		setenv("AWS_DEFAULT_REGION", argv[4], 1);
	}
	BK_LOG_RAW("AWS credentials set (RAM). Now run: kvs master | kvs viewer\r\n");
}

static void kvs_cli_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	INT32 ret;
	CHAR *kvs_argv[2];
	const char *channel = KVS_AWS_DEFAULT_CHANNEL_NAME;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		kvs_cli_help(pcWriteBuffer, xWriteBufferLen);
		return;
	}

	if (os_strcmp(argv[1], "cred") == 0) {
		kvs_cred_cmd(argc, argv);
		return;
	}

	if (argc >= 3)
		channel = argv[2];

	kvs_argv[0] = (CHAR *)"kvs_app";
	kvs_argv[1] = (CHAR *)channel;
	if (os_strcmp(argv[1], "master") == 0) {
		BK_LOGI(KVS_CLI_TAG, "start KVS master, channel: %s\r\n", channel);
		ret = kvs_aws_master_main(2, kvs_argv);
	} else if (os_strcmp(argv[1], "viewer") == 0) {
		BK_LOGI(KVS_CLI_TAG, "start KVS viewer, channel: %s\r\n", channel);
		ret = kvs_aws_viewer_main(2, kvs_argv);
	} else {
		BK_LOG_RAW("unknown role, use: master | viewer | cred\r\n");
		kvs_cli_help(pcWriteBuffer, xWriteBufferLen);
		return;
	}

	(void)ret;
}

static const struct cli_command s_kvs_commands[] = {
	{"kvs", "kvs master|viewer [channel] | cred <ak> <sk> [region] - AWS KVS sample", kvs_cli_cmd},
};

int kvs_cli_init(void)
{
	return cli_register_commands(s_kvs_commands, KVS_CMD_CNT);
}
