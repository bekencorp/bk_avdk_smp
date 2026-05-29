#ifndef __BT_IPC_CORE_H__
#define __BT_IPC_CORE_H__

#include <stdint.h>
#include <common/bk_err.h>
#include <driver/mailbox_channel.h>
#include "bt_ipc_vendor_opcode.h"

/* bt_ipc_env.state lifecycle on CP side:
 *
 *   BT_IPC_STATE_IDLE       - bt_ipc_init() has not run yet (or failed).
 *
 *   BT_IPC_STATE_LOCAL_READY- CP-side IPC infrastructure (queue/thread/mailbox/
 *                             semaphores) is up, but the peer (AP) has not yet
 *                             confirmed it is alive. CP-originated sends in
 *                             this state still have to go through the wakeup/
 *                             wait path (bt_ipc_wait_ap_ble_ready) so that AP
 *                             can be voted on if it is powered down.
 *
 *   BT_IPC_STATE_PEEP_READY - The peer (AP) has confirmed it is alive by
 *                             issuing BT_VENDOR_SUB_OPCODE_INIT to CP. From
 *                             this point bt_ipc_mailbox_send_msg() can write
 *                             to the mailbox directly without going through
 *                             bt_ipc_wait_ap_ble_ready. Transitions back to
 *                             LOCAL_READY when AP signals DEINIT.
 */
enum {
    BT_IPC_STATE_IDLE,
    BT_IPC_STATE_LOCAL_READY,
    BT_IPC_STATE_PEEP_READY,
};

#define BT_IPC_QUEUE_LEN      64
#define BT_IPC_TASK_PRIO       4

#define BT_EVENT_STATUS_NOERROR 0x00

typedef struct
{
    uint8_t type;
    uint32_t param;
} bt_ipc_msg_t;

typedef struct __attribute__((packed))
{
    uint16_t opcode;
    uint8_t param_len;
    uint8_t param[];
}cmd_hdr_t;

typedef struct __attribute__((packed))
{
    uint8_t event_code;
    uint8_t param_len;
    uint8_t param[];
}event_hdr_t;

typedef struct __attribute__((packed))
{
    uint16_t hdl_flags;
    uint16_t datalen;
    uint8_t param[];
}acl_hdr_t;

typedef struct __attribute__((packed))
{
    uint16_t conhdl_psf;
    uint8_t datalen;
    uint8_t param[];
}sco_hdr_t;

typedef struct __attribute__((packed))
{
    mb_chnl_hdr_t hdr;
    uint8_t pkt_type;
#if 0
    union
    {
        cmd_hdr_t *cmd_hdr;
        event_hdr_t *event_hdr;
    };
#else
    uint32_t hdr_ptr;
#endif
} hci_hdr_t;

typedef union
{
    hci_hdr_t hci_hdr;
    mb_chnl_cmd_t mb_cmd;
} bt_ipc_cmd_t;

enum
{
    HCI_COMMAND_PKT = 0x1,//A core
    HCI_ACL_DATA_PKT = 0x2,
    HCI_SCO_DATA_PKT = 0x3,
    HCI_EVENT_PKT = 0x4, //M core
    HCI_FREE_PKT = 0xa,
};

typedef void (*bt_hci_send_cb_t)(uint8_t *buf, uint16_t len);

int32_t bt_ipc_init(void);
void bt_ipc_set_state(uint8_t state);
uint8_t bt_ipc_get_state(void);
/* Notify bt_ipc that the AP has been powered off outside the normal
 * BT_VENDOR_SUB_OPCODE_DEINIT handshake (e.g. PM framework, debug CLI).
 * Always provided so generic callers (e.g. cli_pwr) do not need to know
 * about CONFIG_BLUETOOTH_SUPPORT_AP_PWD_ALL; on builds where AP power
 * management is not used the call is effectively a no-op because state
 * never reaches PEEP_READY. */
void bt_ipc_notify_ap_power_off(void);
/* Weak hook invoked from bt_ipc_notify_ap_power_off(). Adapters that need
 * to be told about an external AP power-off should provide a strong
 * definition; the default implementation is a no-op. */
void bt_ipc_on_ap_power_off_hook(void);
void bt_ipc_hci_send_vendor_event(uint8_t *data, uint16_t len);
void bt_ipc_hci_send_vendor_cmd(uint8_t *data, uint16_t len);
void bt_ipc_hci_send_complete_event(uint8_t *data, uint16_t len);
void bt_ipc_hci_send_acl_data(uint16_t hdl_flags, uint8_t *data, uint16_t len);
void bt_ipc_hci_send_event(uint8_t event_code, uint8_t *data, uint16_t len);
void bt_ipc_register_hci_send_callback(bt_hci_send_cb_t cb);
void bt_ipc_hci_send_sco_data(uint16_t hdl_flags, uint8_t *data, uint16_t len);
#endif
