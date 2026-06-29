#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Configure and start BLE advertising for the hogpd device demo.
 *
 * Builds a legacy connectable advertisement carrying the local name
 * (BK_HID-xxxxxx, derived from the BLE identity address) plus the Beken
 * manufacturer id, then enables advertising. Call once after the GATT
 * server (bk_dm_prf_gatts_main) has been initialised.
 */
void ble_demo_init(void);

/* Start/stop advertising at runtime. enable==0 stops, otherwise (re)starts. */
int ble_demo_adv_enable(uint8_t enable);

#ifdef __cplusplus
}
#endif
