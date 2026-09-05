// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compressed-overwrite (secureboot_overwrite) confirm flag.
 *
 * Stored as a {magic, confirm, crc} record at the END of the ota_control
 * partition (the BL2 resume journal grows from the start, so the two never
 * overlap). magic+crc gate the confirm word so an erased/garbage/partial value
 * is never taken as a real install request.
 *
 * The implementation (ota_confirm.c) is compiled into BOTH platform_bl2 (BL2 /
 * MCUboot) and platform_s (TF-M SPE) from one source, mirroring boot_param_ops.c,
 * so the two on-chip consumers cannot drift. The AP receiver
 * (ota_secure_overwrite.c) writes the same record; its layout and CRC MUST match
 * ota_confirm.c. */

/* Absolute flash offset of the confirm record, or 0 when ota_control is
 * missing/too small (fail-closed: read/write below bail out instead of
 * underflowing to a bogus flash address). */
uint32_t bk_boot_overwrite_confirm_off(void);

/* True iff a valid confirm record (magic + crc ok) whose confirm word == value
 * is present. BL2 uses it to decide whether to run the decompress-overwrite. */
bool bk_boot_read_ota_confirm(uint32_t value);

/* Write the {magic, value, crc} record: erase the confirm sector first, then
 * write and read-back-verify all fields with retries. Returns BK_OK/BK_FAIL
 * (plain int so callers need no Beken type dependency). */
int bk_boot_write_ota_confirm(uint32_t value);

/* Erase confirm sector if armed. Used by SPE (boot ok) and BL2 (OTA verify fail). */
void bk_ota_confirm_clear_if_armed(void);

#ifdef __cplusplus
}
#endif
