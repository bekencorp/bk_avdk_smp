#ifndef __BT_IPC_VENDOR_EVENT_HANDLER_H__
#define __BT_IPC_VENDOR_EVENT_HANDLER_H__

#include "bt_ipc_core.h"
#include "bt_ipc_vendor_opcode.h"

#define BT_IPC_VENDOR_EVENT_CB_MAX  (BT_VENDOR_SUB_OPCODE_COUNT - 1)

bk_err_t bt_ipc_vendor_event_handler_dispatch(const event_hdr_t *event_hdr);

#endif
