#include <os/mem.h>
#include <components/log.h>
#include "bt_ipc_core.h"
#include "bt_ipc_vendor_cmd_handler.h"
#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
#include "ble_ipc_server.h"
#endif
#include "components/bluetooth/bk_dm_bluetooth.h"
#include "components/bluetooth/bk_ble.h"

#define TAG  "bt_ipc"

#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

typedef bk_err_t (*bt_ipc_vendor_cmd_cb_t)(uint16_t sub_opcode, const uint8_t *data, uint16_t len);

typedef struct
{
    uint16_t sub_opcode;
    bt_ipc_vendor_cmd_cb_t cb;
} bt_ipc_vendor_cmd_handler_t;

static bk_err_t bt_ipc_vendor_cmd_init_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len);
static bk_err_t bt_ipc_vendor_cmd_deinit_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len);
static bk_err_t bt_ipc_vendor_cmd_setpwr_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len);
#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
static bk_err_t bt_ipc_vendor_cmd_ble_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len);
#endif
static void bt_ipc_vendor_cmd_send_status(uint16_t sub_opcode, uint8_t status);

static bt_ipc_vendor_cmd_handler_t s_bt_ipc_vendor_cmd_handlers[BT_IPC_VENDOR_CMD_CB_MAX] = {
    {BT_VENDOR_SUB_OPCODE_INIT, bt_ipc_vendor_cmd_init_cb},
    {BT_VENDOR_SUB_OPCODE_DEINIT, bt_ipc_vendor_cmd_deinit_cb},
    {BT_VENDOR_SUB_OPCODE_SETPWR, bt_ipc_vendor_cmd_setpwr_cb},
#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
    {BT_VENDOR_SUB_OPCODE_BLE_CREATE_DB, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_CREATE_ADV, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_SET_ADV_DATA, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_SET_SCAN_RSP_DATA, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_START_ADV, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_STOP_ADV, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_DELETE_ADV, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_SET_ADV_RANDOM_ADDR, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_SET_NOTICE, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_GET_IDLE_ACTV_IDX, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_GET_MAX_ACTV_IDX, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_READ_RESPONSE_VALUE, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_UPDATE_PARAM, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_GATT_MTU_CHANGE, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_SET_MAX_MTU, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_SEND_NOTI_VALUE, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_SEND_IND_VALUE, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_GET_CONNECT_STATE, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_FIND_CONN_IDX_FROM_ADDR, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_DISCONNECT, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_GET_MAX_CONN_IDX, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_FIND_ACTV_STATE_IDX, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_FIND_MASTER_STATE_IDX, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_GET_BT_ADDRESS, bt_ipc_vendor_cmd_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_FRAG, bt_ipc_vendor_cmd_ble_cb},
#endif
};

static bk_err_t bt_ipc_vendor_cmd_init_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;

    bk_err_t ret = bk_bluetooth_init();
    if (ret != BK_OK)
    {
        LOGW("%s, bk_bluetooth_init failed, ret:%d\r\n", __func__, ret);
    }

    bt_ipc_vendor_cmd_send_status(sub_opcode, BT_EVENT_STATUS_NOERROR);
    return ret;
}

static bk_err_t bt_ipc_vendor_cmd_deinit_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len)
{
    (void)data;
    (void)len;

    bk_err_t ret = bk_bluetooth_deinit();
    if (ret != BK_OK)
    {
        LOGW("%s, bk_bluetooth_deinit failed, ret:%d\r\n", __func__, ret);
    }

    bt_ipc_vendor_cmd_send_status(sub_opcode, BT_EVENT_STATUS_NOERROR);
    return ret;
}

static bk_err_t bt_ipc_vendor_cmd_setpwr_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len)
{
    (void)sub_opcode;

    if (len < sizeof(float))
    {
        LOGW("%s, invalid len:%d\r\n", __func__, len);
        return BK_ERR_PARAM;
    }

    float pwr_gain = 0;
    os_memcpy(&pwr_gain, data, sizeof(float));
    LOGD("pwr_gain :%f\n", pwr_gain);

    return bk_ble_tx_power_set(pwr_gain);
}

#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
static bk_err_t bt_ipc_vendor_cmd_ble_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len)
{
    return ble_ipc_server_dispatch_vendor_cmd(sub_opcode, data, len);
}
#endif

static void bt_ipc_vendor_cmd_send_status(uint16_t sub_opcode, uint8_t status)
{
    uint8_t vendor_data[3];

    vendor_data[0] = sub_opcode & 0xff;
    vendor_data[1] = sub_opcode >> 8;
    vendor_data[2] = status;
    bt_ipc_hci_send_vendor_event(vendor_data, sizeof(vendor_data));
}

bk_err_t bt_ipc_vendor_cmd_handler_dispatch(const cmd_hdr_t *cmd_hdr)
{
    uint16_t i;
    uint16_t sub_opcode;
    uint16_t payload_len;
    const uint8_t *payload;

    if ((cmd_hdr == NULL) || (cmd_hdr->param_len < 2))
    {
        LOGW("%s, invalid vendor command\r\n", __func__);
        return BK_ERR_PARAM;
    }

    sub_opcode = (cmd_hdr->param[0] << 8) | cmd_hdr->param[1];
    payload = &cmd_hdr->param[2];
    payload_len = cmd_hdr->param_len - 2;

    LOGD("sub opcode :0x%04x\n", sub_opcode);

    for (i = 0; i < BT_IPC_VENDOR_CMD_CB_MAX; i++)
    {
        if ((s_bt_ipc_vendor_cmd_handlers[i].cb != NULL)
            && (s_bt_ipc_vendor_cmd_handlers[i].sub_opcode == sub_opcode))
        {
            return s_bt_ipc_vendor_cmd_handlers[i].cb(sub_opcode, payload, payload_len);
        }
    }

    LOGW("%s, no callback for sub opcode:0x%04x\r\n", __func__, sub_opcode);
    return BK_ERR_NOT_FOUND;
}

