// Copyright     2023-2028 Beken
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

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flash 0x100 magic == "BK.SB" (sig_verify_en). Independent of eFuse. */
bool bk_boot_flash_sig_magic_enabled(void);

/* Whether BL2 must verify the image *signature*. Image hash is always
 * checked. TRUE if efuse bit3 is burned (secure_boot_supported) or the
 * flash 0x100 magic == "BK.SB". Implemented in hooks_bl2.c. */
bool bk_boot_verify_required(void);

#ifdef __cplusplus
}
#endif
