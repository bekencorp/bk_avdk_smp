#include <components/log.h>
#include "bt_ipc_core.h"
#include "bt_ipc_vendor_event_handler.h"
#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
#include "../include/bt_ipc_vendor_opcode.h"
#include "ble_ipc_client.h"
#endif

#define TAG  "bt_ipc"

#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)

typedef bk_err_t (*bt_ipc_vendor_event_cb_t)(uint16_t sub_opcode, const uint8_t *data, uint16_t len);

typedef struct
{
    uint16_t sub_opcode;
    bt_ipc_vendor_event_cb_t cb;
} bt_ipc_vendor_event_handler_t;

static bk_err_t bt_ipc_vendor_event_init_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len);
static bk_err_t bt_ipc_vendor_event_deinit_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len);
#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
static bk_err_t bt_ipc_vendor_event_ap_wakeup_trigger_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len);
static bk_err_t bt_ipc_vendor_event_ble_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len);
#endif

static bt_ipc_vendor_event_handler_t s_bt_ipc_vendor_event_handlers[BT_IPC_VENDOR_EVENT_CB_MAX] = {
    {BT_VENDOR_SUB_OPCODE_INIT, bt_ipc_vendor_event_init_cb},
    {BT_VENDOR_SUB_OPCODE_DEINIT, bt_ipc_vendor_event_deinit_cb},
#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
    {BT_VENDOR_SUB_OPCODE_AP_WAKEUP_TRIGGER, bt_ipc_vendor_event_ap_wakeup_trigger_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_CMD_EVT, bt_ipc_vendor_event_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_NOTICE_EVT, bt_ipc_vendor_event_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_QUERY_RSP, bt_ipc_vendor_event_ble_cb},
    {BT_VENDOR_SUB_OPCODE_BLE_FRAG, bt_ipc_vendor_event_ble_cb},
#endif
};

static bk_err_t bt_ipc_vendor_event_init_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len)
{
    (void)sub_opcode;

    if ((data == NULL) || (len < 1)) {
        return BK_ERR_PARAM;
    }

    if (data[0] == BT_EVENT_STATUS_NOERROR) {
        bk_bluetooth_init_deinit_compelete();
    }

    return BK_OK;
}

static bk_err_t bt_ipc_vendor_event_deinit_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len)
{
    return bt_ipc_vendor_event_init_cb(sub_opcode, data, len);
}

#if CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL
/* AP_WAKEUP_TRIGGER carries no payload and intentionally has no business
 * semantics on AP side: CP only sends it so that the send itself goes through
 * bt_ipc_mailbox_send_msg() and votes AP boot via the state-machine slow
 * path. By the time this handler runs AP is obviously up, so we just log and
 * return success to keep the upper dispatch loop quiet (no "no callback" or
 * "dispatch failed" warnings). Gated on AP-power-down config since that is
 * the only build where CP ever sends this opcode.
 */
static bk_err_t bt_ipc_vendor_event_ap_wakeup_trigger_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len)
{
    (void)sub_opcode;
    (void)data;
    (void)len;

    LOGI("ap wakeup trigger received!\r\n");
    return BK_OK;
}

static bk_err_t bt_ipc_vendor_event_ble_cb(uint16_t sub_opcode, const uint8_t *data, uint16_t len)
{
    return ble_ipc_client_handle_vendor_event(sub_opcode, data, len) ? BK_OK : BK_ERR_NOT_FOUND;
}
#endif

bk_err_t bt_ipc_vendor_event_handler_dispatch(const event_hdr_t *event_hdr)
{
    uint16_t i;
    uint16_t sub_opcode;
    uint16_t payload_len;
    const uint8_t *payload;

    if ((event_hdr == NULL) || (event_hdr->param_len < 2)) {
        LOGW("%s, invalid vendor event\r\n", __func__);
        return BK_ERR_PARAM;
    }

    sub_opcode = (event_hdr->param[0]) | (event_hdr->param[1] << 8);
    payload = &event_hdr->param[2];
    payload_len = event_hdr->param_len - 2;

    for (i = 0; i < BT_IPC_VENDOR_EVENT_CB_MAX; i++) {
        if ((s_bt_ipc_vendor_event_handlers[i].cb != NULL) &&
            (s_bt_ipc_vendor_event_handlers[i].sub_opcode == sub_opcode)) {
            return s_bt_ipc_vendor_event_handlers[i].cb(sub_opcode, payload, payload_len);
        }
    }

    LOGW("%s, no callback for sub opcode:0x%04x\r\n", __func__, sub_opcode);
    return BK_ERR_NOT_FOUND;
}
