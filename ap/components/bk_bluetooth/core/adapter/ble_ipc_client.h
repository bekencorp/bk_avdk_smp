#ifndef __BLE_IPC_CLIENT_H__
#define __BLE_IPC_CLIENT_H__

#include <stdbool.h>
#include <stdint.h>

void ble_ipc_client_init(void);
bool ble_ipc_client_handle_vendor_event(uint16_t sub_opcode, const uint8_t *payload, uint16_t payload_len);

#endif
