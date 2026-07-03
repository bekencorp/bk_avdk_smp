#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int dm_ble_gatt_client_demo_init(void);
int dm_ble_gatt_client_demo_read(uint16_t conn_id, uint16_t attr_handle, uint8_t *data, uint16_t len);
int dm_ble_gatt_client_demo_write_ccc(uint16_t conn_id, uint16_t ccc_handle, uint8_t enable);

#ifdef __cplusplus
}
#endif
