/*
 * ota_boot_param.h - AP-side writer for the boot_param TRIAL record.
 *
 * Secure XIP A/B OTA arms a trial boot by committing an AB record to the
 * boot_param partition. Layout and ping-pong algorithm are shared with the
 * bootloader/SPE via ab_flag.h; this module only supplies the non-secure flash
 * driver + zlib CRC32. Records written here are byte-compatible with the CP-side
 * boot_param.h reader.
 */

#ifndef __OTA_BOOT_PARAM_H__
#define __OTA_BOOT_PARAM_H__

#include <stdint.h>
#include "ab_flag.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Read the freshest valid boot_param record (scans the ping-pong pair).
 * @param rec  out: receives the selected record.
 * @return 0 on success, -1 if the partition is virgin / doubly-corrupted. */
int ota_boot_param_read_latest(ab_flag_record_t *rec);

/* Arm a TRIAL boot for update_slot after a verified download: writes
 * exec_slot=running slot, boot_state=TRIAL, try_count=0 to the opposite
 * ping-pong sector (power-loss safe). BL2 bumps try_count and rolls back on
 * try_max. @return 0 on success, -1 on error (partition missing). */
int ota_boot_param_set_trial(uint8_t update_slot);

#ifdef __cplusplus
}
#endif

#endif /* __OTA_BOOT_PARAM_H__ */
