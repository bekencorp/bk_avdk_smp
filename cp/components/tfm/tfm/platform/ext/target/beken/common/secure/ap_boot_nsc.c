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

/* AP boot release NSC gateway. Power-on stays in TF-M isolation
 * (bk_ap_power_domain_on); this gateway only installs the shim and releases
 * the AP CPU reset when the NS cpu0_app is ready (after bk_init). */

#include <stdint.h>
#include "security_defs.h"
#include "bk_tfm_ap_power.h"

static volatile uint8_t s_ap_released;

/* Link anchor: referenced from tfm_hal_get_ns_entry_point() so --gc-sections
 * keeps the gateway veneers in tfm_s. */
void psa_ap_boot_nsc_stub(void)
{
}

__tz_c_veneer int psa_ap_boot(void)
{
	if (s_ap_released) {
		return 0;
	}

	bk_ap_release();
	s_ap_released = 1;
	return 0;
}
