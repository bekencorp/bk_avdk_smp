/**
 * KVS AWS CLI: select role (master/viewer) by command instead of macro.
 * Usage: kvs master [channel_name]  |  kvs viewer [channel_name]
 */
#include "bk_cli.h"
#include "cli.h"
#include <os/str.h>
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
    BK_LOG_RAW("kvs <role> [channel_name]\r\n");
    BK_LOG_RAW("  role: master | viewer\r\n");
    BK_LOG_RAW("  channel_name: optional, default \"" KVS_AWS_DEFAULT_CHANNEL_NAME "\"\r\n");
    BK_LOG_RAW("  e.g. kvs master\r\n");
    BK_LOG_RAW("       kvs viewer my_channel\r\n");
}

static void kvs_cli_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    INT32 ret;
    CHAR *kvs_argv[2];
    const char *channel = KVS_AWS_DEFAULT_CHANNEL_NAME;
    if (argc < 2) {
        kvs_cli_help(pcWriteBuffer, xWriteBufferLen);
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
        BK_LOG_RAW("unknown role, use: master | viewer\r\n");
        kvs_cli_help(pcWriteBuffer, xWriteBufferLen);
        return;
    }

    (void)ret;
}

static const struct cli_command s_kvs_commands[] = {
    {"kvs", "kvs master|viewer [channel_name] - start KVS AWS role", kvs_cli_cmd},
};

int kvs_cli_init(void)
{
    return cli_register_commands(s_kvs_commands, KVS_CMD_CNT);
}
