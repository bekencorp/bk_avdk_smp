// Copyright 2020-2021 Beken
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

#include "hal_config.h"

#ifdef __cplusplus
extern "C" {
#endif

void hal_mcheck(void);

#if CONFIG_OTP_V1 && CONFIG_SPE
/* Writes the AP repair addresses from the AP memory-check OTP region into the
 * AP MEM_CHECK registers. Must run after the AP subsystem leaves
 * isolation/reset and before the AP core leaves reset; the registers are
 * cleared by every AP power-down. */
bk_err_t hal_ap_mem_check_restore_from_otp(void);
#endif

#ifdef __cplusplus
}
#endif


