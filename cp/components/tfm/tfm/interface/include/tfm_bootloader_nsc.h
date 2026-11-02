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

/* Non-secure prototype for the CMSE bootloader-version gateway implemented in
 * the secure world (bootloader_nsc.c). No cmse_nonsecure_entry here: the NS side
 * calls it as an ordinary function and the linker resolves it to the SG-stub
 * veneer in libtfm_s_veneers.a. */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Encoded version of the running bootloader slot: major<<16 | minor<<8 | patch.
 * Returns BL2_VERSION_ABSENT (0xFFFFFFFF) when nothing is stamped (legacy
 * bootloader, erased slot, magic mismatch or read failure), and
 * BL2_VERSION_CORRUPT (0xFFFFFFFE) when the magic matched but the CRC failed.
 * A valid 0.0.0 returns 0 and stays distinguishable from both. Read-only,
 * display/comparison use only. */
uint32_t bk_bootloader_version_nsc(void);

#ifdef __cplusplus
}
#endif
