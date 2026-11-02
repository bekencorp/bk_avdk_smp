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

/* CP (cpu0_app, Non-Secure) API to query the bootloader version.
 *
 * Security firmware projects: the stamp lives at the tail of the BL2 manifest
 * slot, in a flash region TF-M marks Secure on the DBUS, so the NS side reads it
 * through the secure world (tfm_bootloader_nsc.h). Projects without security
 * firmware run aboot: the stamp sits right after the aboot vector table in the
 * bootloader partition and is read straight from flash. Read-only;
 * display/comparison use only, it does not take part in anti-rollback. */

#pragma once

#include <stdint.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Sentinels returned by bk_bootloader_get_version() when no trustworthy version
 * is available. Must match BL2_VERSION_* in the secure side bl2_version.h:
 *   ABSENT  - no stamp: legacy bootloader (erased/0xFF slot), magic mismatch or
 *             flash read failure.
 *   CORRUPT - a stamp is there but its CRC does not check out.
 * A real version always leaves the top byte clear, so neither sentinel can be
 * confused with a version; a valid 0.0.0 stamp returns 0. */
#define BK_BOOTLOADER_VERSION_ABSENT 0xFFFFFFFFu
#define BK_BOOTLOADER_VERSION_CORRUPT 0xFFFFFFFEu

/* Running bootloader version, encoded as major<<16 | minor<<8 | patch.
 * Returns one of the BK_BOOTLOADER_VERSION_* sentinels above when no valid
 * version is available; a valid 0.0.0 stamp returns 0. */
uint32_t bk_bootloader_get_version(void);

/* Format the running bootloader version into buf as "major.minor.patch".
 * Returns BK_OK on success; BK_FAIL when buf is NULL, len is too small for the
 * longest form ("255.255.255" plus the terminator) or no valid version is
 * available. */
bk_err_t bk_bootloader_get_version_string(char *buf, uint32_t len);

#ifdef __cplusplus
}
#endif
