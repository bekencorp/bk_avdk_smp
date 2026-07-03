#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DM_BLE_GATT_SERVER_DEMO_SERVICE_UUID      0xFA00
#define DM_BLE_GATT_SERVER_DEMO_NOTIFY_CHAR_UUID 0xEA01
#define DM_BLE_GATT_SERVER_DEMO_WRITE_CHAR_UUID  0xEA02
#define DM_BLE_GATT_SERVER_DEMO_RW_N2_CHAR_UUID  0xEA05
#define DM_BLE_GATT_SERVER_DEMO_RW_N3_CHAR_UUID  0xEA06
#define DM_BLE_GATT_SERVER_DEMO_RW_N4_CHAR_UUID  0xEA07

int dm_ble_gatt_server_demo_init(void);
int dm_ble_gatt_server_demo_send_notify(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif
