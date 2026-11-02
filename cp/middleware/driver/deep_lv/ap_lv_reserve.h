// Copyright 2023-2025 Beken
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

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AP SRAM bad-point retention, CP-driven (Secure CP). The repair addresses are
 * imported separately by hal_ap_mem_check_restore_from_otp(), which must run
 * before ap_lv_reserve_restore(). */

#if CONFIG_SPE && CONFIG_PM_AP_FAST_BOOT_ENABLE

/* Write the words captured before the last power-down back. */
void ap_lv_reserve_restore(void);

/* Capture the repaired words before the AP domain loses power. */
void ap_lv_reserve_save(void);

#endif /* CONFIG_SPE && CONFIG_PM_AP_FAST_BOOT_ENABLE */

#ifdef __cplusplus
}
#endif
