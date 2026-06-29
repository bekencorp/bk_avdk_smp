#pragma once
#include <stdint.h>

/* Master switch for the HID-over-GATT (HOGP) device profile. Set to 0 to
 * compile the module out (all APIs become no-ops returning success/error). */
#define HOGPD_ENABLE 1

/*
 * Register the HID-over-GATT (HOGP) service database onto the public dm GATT
 * server. dm_gatts (bk_dm_prf_gatts_main) must already be initialised. Safe to call
 * once; a second call without a matching deinit returns an error.
 *
 * return: 0 on success, negative on error.
 */
int32_t bk_dm_prf_hogpd_init(void);

/*
 * Tear down the HOGP profile (unregister callbacks / mark uninitialised).
 *
 * deinit_bluetooth_future: non-zero if bluetooth itself will be deinitialised
 *                          afterwards, so the cached attribute table is also
 *                          dropped and rebuilt on the next init.
 * return: 0 on success, negative on error.
 */
int32_t bk_dm_prf_hogpd_deinit(uint8_t deinit_bluetooth_future);

/*
 * Reset the cached "db already created" state. Call this when bluetooth is
 * about to be deinitialised so the attribute table is recreated on the next
 * bk_dm_prf_hogpd_init().
 *
 * return: 0 on success.
 */
int32_t bk_dm_prf_hogpd_deinit_because_bluetooth_deinit_future(void);

/*
 * Send a HID input report to a connected host and block until the stack
 * confirms completion.
 *
 * gatt_conn_handle: connection handle of the target host.
 * data            : report payload.
 * len             : payload length in bytes.
 * is_notify       : non-zero to send as a notification, zero to send as an
 *                   indication (waits for the peer ack).
 * report_id       : HID report id selector (currently the input report).
 * return: 0 on success, negative on error/timeout.
 */
int32_t bk_dm_prf_hogpd_notify(uint16_t gatt_conn_handle, uint8_t *data, uint32_t len, uint8_t is_notify, uint8_t report_id);
