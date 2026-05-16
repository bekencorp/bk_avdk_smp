#ifndef __BT_IPC_VENDOR_CMD_HANDLER_H__
#define __BT_IPC_VENDOR_CMD_HANDLER_H__

#include "bt_ipc_core.h"
#include "bt_ipc_vendor_opcode.h"

#define BT_IPC_VENDOR_CMD_CB_MAX  (BT_VENDOR_SUB_OPCODE_COUNT - 1)

bk_err_t bt_ipc_vendor_cmd_handler_dispatch(const cmd_hdr_t *cmd_hdr);

#endif
