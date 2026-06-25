#pragma once

#include <stdint.h>

#define DM_BLE_BOARDING_ENABLE 1

int32_t dm_ble_boarding_init(void);
int32_t dm_ble_boarding_deinit(uint8_t deinit_bluetooth_future);
int32_t dm_ble_boarding_deinit_because_bluetooth_deinit_future(void);

int32_t dm_ble_boarding_notify(uint8_t *data, uint16_t len);
