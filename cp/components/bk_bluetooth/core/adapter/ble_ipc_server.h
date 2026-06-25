#ifndef __BLE_IPC_SERVER_H__
#define __BLE_IPC_SERVER_H__

#include <stdint.h>
#include <common/bk_err.h>

bk_err_t ble_ipc_server_dispatch_vendor_cmd(uint16_t sub_opcode, const uint8_t *payload, uint16_t payload_len);

#endif
