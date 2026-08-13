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

/* Non-secure prototypes for the AP secure-prepare gateway (ap_boot_nsc.c). The
 * NS side calls these as ordinary functions; the linker resolves them to the
 * SG-stub veneers in libtfm_s_veneers.a. */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tfm_ap_boot_reason {
	TFM_AP_BOOT_REASON_COLD = 0,
	TFM_AP_BOOT_REASON_REQUEST,
	TFM_AP_BOOT_REASON_TIMER,
	TFM_AP_BOOT_REASON_NETWORK,
};

/* Open the AP SYS/AHBP region to the Non-secure world: apply the AP MPC/PPHS
 * once CP NS has powered the AP domain. Must be called before CP NS touches the
 * AP SysCfg (clock/EMA/freq) through the NS alias. Returns 0 on success. */
int psa_ap_secure_sys_open(void);

/* Perform the remaining Secure-privilege AP start step: install the verified
 * boot shim. The AP MPC/PPHS must already be applied via psa_ap_secure_sys_open,
 * the AP power domain up (driven by CP NS) and both AP cores held in reset. The
 * AP core is left in reset; CP NS writes the boot offset and releases it.
 * Returns 0 on success, negative on error. */
int psa_ap_secure_prepare(enum tfm_ap_boot_reason boot_reason);

/* Roll back a prepare (close the Secure master gate) after a CP NS start error
 * before the AP core is released. Returns 0 on success. */
int psa_ap_secure_cancel(void);

/* Return the shim boot address set by a successful psa_ap_secure_prepare(), or
 * 0 if not prepared. CP NS programs this into the AP boot offset. */
uint32_t psa_ap_secure_boot_addr(void);

#ifdef __cplusplus
}
#endif
