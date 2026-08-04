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

/* reg_nsc: ARMv8-M CMSE Non-Secure-Callable register access for BK7259.
 *
 * The CP core runs TF-M as the Secure world (SPE) and the cpu0_app as the
 * Non-Secure world (NSPE). These gateways let the NS side read/write registers
 * through the secure world via SG-instruction veneers (see tfm_reg_nsc.h for the
 * NS prototypes). __tz_c_veneer expands to cmse_nonsecure_entry, so GCC emits an
 * SG stub into .gnu.sgstubs (SAU-marked NSC) and exports the symbol into the
 * CMSE import library (libtfm_s_veneers.a) linked by the NS image.
 *
 * Keep the gateway bodies minimal: they run at the S<->NS boundary and the SDK
 * log path (BK_LOGI) is NOT safe to call here (it runs through the NS driver
 * stack). When CONFIG_REG_NSC_DIAG_LOG is set we only emit bytes by raw UART
 * register access (see below). */

#include "soc/soc.h"
#include "security_defs.h"

/* Link anchor: referenced from tfm_hal_get_ns_entry_point() under
 * CONFIG_REG_ACCESS_NSC so --gc-sections keeps this translation unit (and thus
 * the gateway veneers) in tfm_s. */
void psa_reg_nsc_stub(void)
{
	return;
}

__tz_c_veneer uint32_t psa_reg_read(uint32_t addr)
{
	return REG_READ(addr);
}

__tz_c_veneer void psa_reg_write(uint32_t addr, uint32_t value)
{
	REG_WRITE(addr, value);
}

__tz_c_veneer void psa_reg_dump(uint32_t addr, uint32_t size)
{
	uint32_t words = (size + 3) / 4;

	for (uint32_t i = 0; i < words; i++) {
		(void)REG_READ(addr);
		addr += 4;
	}
}
