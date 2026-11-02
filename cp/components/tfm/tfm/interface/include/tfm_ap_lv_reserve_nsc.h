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

/* Non-secure prototypes for the AP bad-point gateways implemented in the secure
 * world (ap_lv_reserve.c). The linker resolves them to the SG-stub veneers in
 * libtfm_s_veneers.a.
 *
 * This header states the interface; like the CP and AP boot NSC, the CP NS
 * callers declare the prototypes locally instead of including it, so a common CP
 * driver does not pull in the TF-M interface directory (which also ships mbedtls
 * headers). Keep in sync with multicore_driver.c and bk_pm_ap_ctrl.c.
 *
 * No address or value is passed: the secure world decodes the repair points and
 * reads the OTP itself, so the NS caller cannot steer a secure access. */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* AP power-up, before the AP core is released. Must precede
 * psa_ap_lv_reserve_restore(). Returns 0, or -1 on an OTP read error. */
int psa_ap_mem_check_restore_from_otp(void);

/* AP power-up, after the import above. Returns 0. */
int psa_ap_lv_reserve_restore(void);

/* AP power-off, before the AP domain is cut. Returns 0. */
int psa_ap_lv_reserve_save(void);

#ifdef __cplusplus
}
#endif
