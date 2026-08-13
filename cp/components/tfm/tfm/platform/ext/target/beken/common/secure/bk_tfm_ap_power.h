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

/* Secure-privilege AP start helpers.
 *
 * The AP power domain, analog high-voltage domain, the AP SYS/AHBP registers and
 * the AP master security attribute are driven by the CP Non-Secure world (the AP
 * has no independent AON PMU). The secure world only performs the steps that
 * need Secure privilege: the AP MPC/PPHS attributes and the boot-shim install.
 * These helpers back the psa_ap_secure_prepare / psa_ap_secure_cancel NSC
 * gateways. */

/* Report whether the AP power domain is up (isolation released, core clock on,
 * domain reset de-asserted). Reads only the CP-side AON PMU. */
bool bk_ap_domain_is_on(void);

/* Verify the AP domain is powered and both AP cores are still held in software
 * reset (AP SYS reg4/reg5 read through the NS alias). Returns 0 when ready. */
int bk_ap_domain_and_reset_ready(void);

/* Open the AP SYS/AHBP region to the Non-secure world: apply the AP MPC and the
 * AP PPHS (from the verified flash config) once the AP power domain is up. After
 * this, CP NS can program the AP SysCfg (clock/EMA/freq) through the NS alias.
 * Must be called right after CP NS powers the AP domain, before any CP NS AP SYS
 * access. Returns 0 on success, negative on failure. */
int bk_ap_sys_secure_open(void);

/* Install the verified boot shim into the reserved Secure RAM block. The AP
 * MPC/PPHS must already be applied via bk_ap_sys_secure_open(). The AP core is
 * left in reset. Returns 0 and the shim boot address on success. */
int bk_ap_secure_resources_prepare(uint32_t *boot_addr);