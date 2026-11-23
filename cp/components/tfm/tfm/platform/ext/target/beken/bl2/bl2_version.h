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

#ifdef __cplusplus
extern "C" {
#endif

/* BL2 (bootloader) version stamp reader. The stamp is written at the tail of
 * the running manifest slot by beken_utils/scripts/bl1_sign.py.
 *
 * Returns the encoded version (major<<16|minor<<8|patch) on success, or:
 *   BL2_VERSION_ABSENT  - nothing to read: legacy bootloader (erased/0xFF slot),
 *                         a magic mismatch or a flash read failure.
 *   BL2_VERSION_CORRUPT - a stamp is present (magic matched) but its CRC failed.
 * A valid version always leaves the top byte clear (each field is 8 bits), so
 * neither sentinel can collide with a real version; a valid 0.0.0 returns 0.
 */
#define BL2_VERSION_ABSENT  0xFFFFFFFFu
#define BL2_VERSION_CORRUPT 0xFFFFFFFEu

uint32_t bl2_get_version(void);

#ifdef __cplusplus
}
#endif
