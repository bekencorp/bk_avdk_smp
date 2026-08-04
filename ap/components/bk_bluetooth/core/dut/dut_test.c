#include <stdint.h>
#include <string.h>
#include "bk_cli.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "components/bluetooth/bk_dm_gap_bt.h"
#include "components/bluetooth/bk_dm_gap_bt_types.h"

#define TAG "bt_dut"
#define DUT_DEVICE_NAME "BK_DUT_TEST"
#define DUT_RSP_OK "DUT TEST RSP:OK\r\n"
#define DUT_RSP_ERROR "DUT TEST RSP:ERROR\r\n"
#define HCI_ENABLE_DEVICE_UNDER_TEST_MODE_OPCODE 0x1803U
#define DUT_HCI_UART_INVALID 0xFFU

extern uint16_t hci_common_api_handler_no_params(uint16_t opcode);
extern void bk_ble_dut_start(uint8_t uart_id);
extern void bk_ble_dut_stop(void);

static void bt_dut_write_response(char *buffer, int buffer_len, const char *response)
{
    size_t response_len;

    if ((buffer == NULL) || (buffer_len <= 0) || (response == NULL))
    {
        return;
    }

    response_len = strlen(response);
    if (response_len >= (size_t)buffer_len)
    {
        response_len = buffer_len - 1;
    }

    memcpy(buffer, response, response_len);
    buffer[response_len] = '\0';
}

static bk_err_t bt_dut_set_scan_mode(bk_bt_conn_mode_t connectable, bk_bt_disc_mode_t discoverable)
{
    bk_err_t ret = bk_bt_gap_set_scan_mode(connectable, discoverable);

    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "set scan mode failed: %d\n", ret);
    }

    return ret;
}

static bk_err_t bt_dut_enable(void)
{
    bk_err_t ret;

    BK_LOGI(TAG, "enable DUT test\n");
    bk_ble_dut_start(DUT_HCI_UART_INVALID);

    ret = bt_dut_set_scan_mode(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
    if (ret != BK_OK)
    {
        bk_ble_dut_stop();
        return ret;
    }

    ret = hci_common_api_handler_no_params(HCI_ENABLE_DEVICE_UNDER_TEST_MODE_OPCODE);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "enable DUT mode failed: %d\n", ret);
        bt_dut_set_scan_mode(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        bk_ble_dut_stop();
    }

    return ret;
}

static void bt_dut_test_command(char *buffer, int buffer_len, int argc, char **argv)
{
    bk_err_t ret;

    if ((argc < 2) || (argv == NULL) || (argv[1] == NULL))
    {
        bt_dut_write_response(buffer, buffer_len, DUT_RSP_ERROR);
        return;
    }

    if (strcmp(argv[1], "enable") == 0)
    {
        ret = bt_dut_enable();
        if (ret != BK_OK)
        {
            bt_dut_write_response(buffer, buffer_len, DUT_RSP_ERROR);
            return;
        }
    }
    else if (strcmp(argv[1], "disable") == 0)
    {
        BK_LOGI(TAG, "disable DUT test\n");
        bk_ble_dut_stop();

        ret = bt_dut_set_scan_mode(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
        if (ret != BK_OK)
        {
            bt_dut_write_response(buffer, buffer_len, DUT_RSP_ERROR);
            return;
        }
    }
    else if (strcmp(argv[1], "scan_enable") == 0)
    {
        BK_LOGI(TAG, "enable DUT scan\n");
        ret = bt_dut_set_scan_mode(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
        if (ret != BK_OK)
        {
            bt_dut_write_response(buffer, buffer_len, DUT_RSP_ERROR);
            return;
        }
    }
    else
    {
        bt_dut_write_response(buffer, buffer_len, DUT_RSP_ERROR);
        return;
    }

    bt_dut_write_response(buffer, buffer_len, DUT_RSP_OK);
}

static void bt_dut_gap_callback(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    (void)param;

    switch (event)
    {
        case BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
            BK_LOGI(TAG, "DUT connected\n");
            bt_dut_set_scan_mode(BK_BT_NON_CONNECTABLE, BK_BT_NON_DISCOVERABLE);
            break;

        case BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
            BK_LOGI(TAG, "DUT disconnected\n");
            bt_dut_set_scan_mode(BK_BT_CONNECTABLE, BK_BT_DISCOVERABLE);
            break;

        default:
            break;
    }
}

static const struct cli_command s_dut_test_commands[] =
{
    {"bt_dut_test", "bt_dut_test <enable|disable|scan_enable>", bt_dut_test_command},
};

void dut_test_init(void)
{
    bk_err_t ret;

    ret = cli_register_commands(s_dut_test_commands,
                                sizeof(s_dut_test_commands) / sizeof(s_dut_test_commands[0]));
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "register CLI command failed: %d\n", ret);
    }

    ret = bk_bt_gap_set_local_name((uint8_t *)DUT_DEVICE_NAME, strlen(DUT_DEVICE_NAME));
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "set local name failed: %d\n", ret);
    }

    ret = bk_bt_gap_register_callback(bt_dut_gap_callback);
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "register GAP callback failed: %d\n", ret);
    }

    ret = bt_dut_enable();
    if (ret != BK_OK)
    {
        BK_LOGE(TAG, "enable DUT test by default failed: %d\n", ret);
        return;
    }

    BK_LOGI(TAG, "DUT test initialized and enabled\n");
}
