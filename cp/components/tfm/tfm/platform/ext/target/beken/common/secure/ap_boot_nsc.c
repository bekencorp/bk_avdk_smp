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

/* Narrow AP secure-prepare NSC gateway.
 *
 * The CP Non-Secure world owns the AP start sequence (AON PMU, analog domain,
 * AP power-on, AP SYS/AHBP via the NS alias, the AP master security attribute in
 * PPRO reg0xF, and the final reset release). This gateway only performs the
 * Secure-privilege steps that the NS world cannot do: apply the AP MPC/PPHS and
 * install the verified boot shim. It accepts no address or attribute data from
 * the NS caller; the shim target comes from compile-time partition info. */

#include <stdint.h>
#include "security_defs.h"
#include "cmsis.h"
#include "bk_tfm_ap_power.h"

/* Keep in sync with enum tfm_ap_boot_reason in tfm_ap_boot_nsc.h. */
#define AP_BOOT_REASON_MAX 4u

/* Return codes. */
#define AP_SEC_OK          0
#define AP_SEC_ERR_PARAM  (-1)
#define AP_SEC_ERR_BUSY   (-2)
#define AP_SEC_ERR_STATE  (-3)
#define AP_SEC_ERR_HW     (-4)

static volatile uint8_t s_ap_busy;
static volatile uint8_t s_ap_prepared;
static volatile uint32_t s_ap_boot_addr;

/* Link anchor: referenced from tfm_hal_get_ns_entry_point() so --gc-sections
 * keeps the gateway veneers in tfm_s. */
void psa_ap_boot_nsc_stub(void)
{
}

/* Non-blocking test-and-set under masked interrupts so a re-entrant NS caller
 * cannot interleave two prepare/cancel operations. */
static int ap_prepare_lock(void)
{
	uint32_t primask = __get_PRIMASK();

	__disable_irq();
	if (s_ap_busy != 0u) {
		__set_PRIMASK(primask);
		return AP_SEC_ERR_BUSY;
	}
	s_ap_busy = 1u;
	__set_PRIMASK(primask);
	return AP_SEC_OK;
}

static void ap_prepare_unlock(void)
{
	__DMB();
	s_ap_busy = 0u;
}

__tz_c_veneer int psa_ap_secure_sys_open(void)
{
	int rc;

	if (ap_prepare_lock() != AP_SEC_OK) {
		return AP_SEC_ERR_BUSY;
	}

	rc = (bk_ap_sys_secure_open() == 0) ? AP_SEC_OK : AP_SEC_ERR_HW;

	ap_prepare_unlock();
	return rc;
}

__tz_c_veneer int psa_ap_secure_prepare(uint32_t boot_reason)
{
	uint32_t boot_addr = 0u;
	int rc = AP_SEC_ERR_HW;

	if (boot_reason >= AP_BOOT_REASON_MAX) {
		return AP_SEC_ERR_PARAM;
	}

	if (ap_prepare_lock() != AP_SEC_OK) {
		return AP_SEC_ERR_BUSY;
	}

	s_ap_prepared = 0u;
	s_ap_boot_addr = 0u;

	/* The AP domain must be powered by CP NS and both cores still in reset. */
	if (bk_ap_domain_and_reset_ready() != 0) {
		rc = AP_SEC_ERR_STATE;
		goto out;
	}

	if (bk_ap_secure_resources_prepare(&boot_addr) != 0) {
		goto out;
	}

	/* Re-check the AP core is still in reset after the shim install. */
	if (bk_ap_domain_and_reset_ready() != 0) {
		rc = AP_SEC_ERR_STATE;
		goto out;
	}

	s_ap_boot_addr = boot_addr;
	__DMB();
	s_ap_prepared = 1u;
	rc = AP_SEC_OK;

out:
	ap_prepare_unlock();
	return rc;
}

__tz_c_veneer int psa_ap_secure_cancel(void)
{
	if (ap_prepare_lock() != AP_SEC_OK) {
		return AP_SEC_ERR_BUSY;
	}

	s_ap_prepared = 0u;
	s_ap_boot_addr = 0u;

	ap_prepare_unlock();
	return AP_SEC_OK;
}

__tz_c_veneer uint32_t psa_ap_secure_boot_addr(void)
{
	if (s_ap_prepared == 0u) {
		return 0u;
	}
	return s_ap_boot_addr;
}