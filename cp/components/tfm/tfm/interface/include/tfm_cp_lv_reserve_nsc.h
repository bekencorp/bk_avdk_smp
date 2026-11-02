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

/* Non-secure prototype for the CP SRAM bad-point capture gateway implemented in
 * the secure world (cp_lv_reserve.c). The NS side calls it as an ordinary
 * function from its deep-LV path; the linker resolves it to the SG-stub veneer
 * in libtfm_s_veneers.a.
 *
 * This header states the interface; like the AP boot NSC, the CP NS caller
 * (soc/bk7259/hal/sys_pm_hal.c) declares the prototype locally instead of
 * including it, so a common CP driver does not pull in the TF-M interface
 * directory (which also ships mbedtls headers). Keep the two in sync.
 *
 * No address or value is passed: the secure world decodes the bad-point
 * addresses itself, so the NS caller cannot steer a secure write. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Capture the repaired CP SRAM words. Call from the deep-LV path, after the
 * memory content is final and before the flash clock is turned off (the veneer
 * and the secure handler are flash-resident). Returns 0 on success. */
int psa_cp_lv_reserve_save(void);

#ifdef __cplusplus
}
#endif
