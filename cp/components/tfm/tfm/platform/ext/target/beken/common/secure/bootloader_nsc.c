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

/* bootloader_nsc: ARMv8-M CMSE Non-Secure-Callable gateway exposing the BL2
 * (bootloader) version to the CP Non-Secure world.
 *
 * The manifest slot carrying the version stamp lies in the flash region TF-M
 * marks Secure on the DBUS, so the NS side cannot read it directly; this
 * gateway performs the read in the secure world. The parsing itself lives in
 * the shared bl2_version.c (one copy for BL2 and SPE) so both worlds always
 * report the same version.
 *
 * Keep the body minimal: it runs at the S<->NS boundary and the SDK log path is
 * unsafe to call here (see reg_nsc.c). __tz_c_veneer expands to
 * cmse_nonsecure_entry, so GCC emits an SG stub into .gnu.sgstubs (SAU-marked
 * NSC) and exports the symbol into libtfm_s_veneers.a. */

#include "security_defs.h"
#include "cmsis_gcc.h"
#include "bl2_version.h"
#include "bk_tfm_ppc.h"

/* Link anchor: referenced from tfm_hal_get_ns_entry_point() under
 * CONFIG_BOOTLOADER_VERSION_NSC so --gc-sections keeps this TU (and its
 * gateway veneer) in tfm_s. */
void psa_bootloader_nsc_stub(void)
{
	return;
}

__tz_c_veneer uint32_t bk_bootloader_version_nsc(void)
{
	uint32_t primask = __get_PRIMASK();
	uint32_t flash_ns_flag;
	uint32_t version;

	/* The runtime leaves the flash PPC bit Non-Secure so the NS OTA path can
	 * program its own partition; a Secure access through the secure flash alias
	 * would then BusFault. Re-attribute flash to Secure for the read and restore
	 * it afterwards (same bracket as Driver_Flash.c). bk_ppc_lock_flash() also
	 * repoints the SYS base to the Secure alias, so keep CP interrupts masked
	 * for the window: a NS ISR taken here would touch a Secure-only SYS. */
	__disable_irq();
	flash_ns_flag = bk_ppc_lock_flash();
	version = bl2_get_version();
	bk_ppc_unlock_flash(flash_ns_flag);
	__set_PRIMASK(primask);

	return version;
}
