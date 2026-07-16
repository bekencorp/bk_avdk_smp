#include <stdio.h>
#include <string.h>

#include <bk_cli.h>
#include <components/bluetooth/bk_dm_bluetooth.h>
#include <components/log.h>

#include "ble_mesh_led_sample.h"
#include "ble_mesh_provision_sample.h"
#include "ble_mesh_tmall_spirit_sample.h"

#define TAG "ble_mesh"
#define BLE_MESH_CMD_RSP_SUCCEED "CMDRSP:OK\r\n"
#define BLE_MESH_CMD_RSP_ERROR "CMDRSP:ERROR\r\n"

static void mesh_cli_response(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
    if (pcWriteBuffer && xWriteBufferLen > 0 && msg)
    {
        snprintf(pcWriteBuffer, xWriteBufferLen, "%s", msg);
    }
}

static void mesh_cli_usage(void)
{
    BK_LOGI(TAG, "ble_mesh provision init provisioner\n");
    BK_LOGI(TAG, "ble_mesh provision init provisionee\n");
    BK_LOGI(TAG, "ble_mesh provision deprovision <MAC>\n");
    BK_LOGI(TAG, "ble_mesh provision send_count [interval_ms]\n");
    BK_LOGI(TAG, "ble_mesh tmall init <product_id> <device_secret>\n");
    BK_LOGI(TAG, "ble_mesh tmall click\n");
    BK_LOGI(TAG, "ble_mesh led init [type]\n");
    BK_LOGI(TAG, "ble_mesh led click\n");
    BK_LOGI(TAG, "ble_mesh led send_count [interval_ms]\n");
}

static int mesh_cli_prepare(void)
{
    int ret;

    ret = bk_bluetooth_init();
    if (ret)
    {
        BK_LOGE(TAG, "bk_bluetooth_init failed ret=%d\n", ret);
        return ret;
    }

    return 0;
}

static void cmd_ble_mesh(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int ret;
    const char *rsp = BLE_MESH_CMD_RSP_SUCCEED;

    if (argc < 2 || !strcasecmp(argv[1], "help"))
    {
        mesh_cli_usage();
        mesh_cli_response(pcWriteBuffer, xWriteBufferLen, rsp);
        return;
    }

    ret = mesh_cli_prepare();
    if (ret)
    {
        mesh_cli_response(pcWriteBuffer, xWriteBufferLen, BLE_MESH_CMD_RSP_ERROR);
        return;
    }

    if (!strcasecmp(argv[1], "provision"))
    {
        bt_mesh_provision_sample_shell(argc - 2, &argv[2]);
    }
    else if (!strcasecmp(argv[1], "tmall"))
    {
        ble_mesh_tmall_spirit_sample_shell(argc - 2, &argv[2]);
    }
    else if (!strcasecmp(argv[1], "led"))
    {
        ble_mesh_led_sample_shell(argc - 2, &argv[2]);
    }
    else
    {
        BK_LOGE(TAG, "unknown subcmd %s\n", argv[1]);
        mesh_cli_usage();
        rsp = BLE_MESH_CMD_RSP_ERROR;
    }

    mesh_cli_response(pcWriteBuffer, xWriteBufferLen, rsp);
}

static const struct cli_command s_ble_mesh_commands[] =
{
    {"ble_mesh", "ble_mesh help", cmd_ble_mesh},
};

int cli_ble_mesh_init(void)
{
    return cli_register_commands(s_ble_mesh_commands,
                                 sizeof(s_ble_mesh_commands) / sizeof(s_ble_mesh_commands[0]));
}
