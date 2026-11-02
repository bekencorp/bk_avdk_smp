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

/* Non-secure prototypes for the SRAM bad-point gateways implemented in the
 * secure world (tfm/.../beken/common/secure/ap_lv_reserve.c and
 * cp_lv_reserve.c). The linker resolves them to the SG-stub veneers in
 * libtfm_s_veneers.a.
 *
 * The authoritative statements of this interface live in the TF-M tree:
 *   tfm/interface/include/tfm_ap_lv_reserve_nsc.h
 *   tfm/interface/include/tfm_cp_lv_reserve_nsc.h
 * They are mirrored here because the common CP drivers that call the gateways
 * must not pull in the TF-M interface directory (which also ships mbedtls
 * headers). Keep the two copies in sync.
 *
 * No address or value is passed: the secure world decodes the repair points and
 * reads the OTP itself, so the NS caller cannot steer a secure access. */

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_TFM_AP_BOOT_NSC
/* AP power-up, before the AP core is released. Must precede
 * psa_ap_lv_reserve_restore(). Returns 0, or -1 on an OTP read error. */
int psa_ap_mem_check_restore_from_otp(void);

/* AP power-up, after the import above. Returns 0. */
int psa_ap_lv_reserve_restore(void);

/* AP power-off, before the AP domain is cut. Returns 0. */
int psa_ap_lv_reserve_save(void);
#endif /* CONFIG_TFM_AP_BOOT_NSC */

#if CONFIG_TFM_SLEEP_RETENTION_NSC
/* Capture the repaired CP SRAM words. Call from the deep-LV path, after the
 * memory content is final and before the flash clock is turned off (the veneer
 * and the secure handler are flash-resident). Returns 0 on success. */
int psa_cp_lv_reserve_save(void);
#endif /* CONFIG_TFM_SLEEP_RETENTION_NSC */

#ifdef __cplusplus
}
#endif
