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
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Beken eFuse helpers (definitions in common/secure/bk_efuse.c). */
int      bk_efuse_init(void);
uint32_t efuse_get_value(void);
void     dump_efuse(void);

/* Secure-boot policy derived from the cached eFuse value. */
bool efuse_is_secureboot_enabled(void);
bool efuse_is_spe_debug_enabled(void);
bool efuse_is_info_log_enabled(void);
bool efuse_is_err_log_enabled(void);
bool efuse_is_secure_download_enabled(void);

#ifdef __cplusplus
}
#endif
