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

/* GDMA0 (BAKP DMA) secure/privileged access-attribute restore gateway.
 *
 * Across deep-LV sleep the GDMA0 register context resets to defaults: the
 * per-channel masks secure_attr (REG 0x10) / privileged_attr (REG 0x14) come
 * back 0xFF, locking every channel register window to secure/privileged-only
 * access, so non-secure accesses of the channel pages (e.g. 0x55020140)
 * BusFault after wakeup. Only the secure world can clear these masks.
 *
 * Both worlds share this single gateway:
 *   - CP NS: dma_driver.c lazily calls it on the driver entry paths after a
 *     wakeup;
 *   - AP: psa_ap_secure_prepare() calls it before the pure non-secure AP core
 *     is released from reset.
 *
 * This translation unit has its own switch, CONFIG_DMA_ATTR_NSC (forwarded
 * from CONFIG_TFM_DMA_ATTR_NSC), and is intentionally NOT gated by
 * CONFIG_REG_ACCESS_NSC: the generic register gateway (reg_nsc.c) is expected
 * to be turned off in production builds, while this fix must survive that.
 *
 * No prio_mode.soft_reset write here: this gateway only restores the access
 * masks; the DMA control state is re-programmed by the normal driver init.
 */

#include <stddef.h>
#include "cmsis.h"
#include "soc/soc.h"
#include "dma_struct.h"
#include "security_defs.h"

/* Link anchor: referenced from tfm_hal_get_ns_entry_point() under
 * CONFIG_DMA_ATTR_NSC so --gc-sections keeps this translation unit (and thus
 * the gateway veneer) in tfm_s. */
void psa_dma_attr_nsc_stub(void)
{
}

__tz_c_veneer int psa_gdma_attr_restore(void)
{
	uint32_t gdma_s_base = SOC_GET_S_ADDR(SOC_GENER_DMA_REG_BASE);

	*(volatile uint32_t *)(gdma_s_base + offsetof(dma_hw_t, prio_mode)) = 0u;
	*(volatile uint32_t *)(gdma_s_base + offsetof(dma_hw_t, prio_mode)) = 1u;
	*(volatile uint32_t *)(gdma_s_base + offsetof(dma_hw_t, secure_attr)) = 0u;
	*(volatile uint32_t *)(gdma_s_base + offsetof(dma_hw_t, privileged_attr)) = 0u;
	*(volatile uint32_t *)(gdma_s_base + offsetof(dma_hw_t, int_allocate)) = 0xAA00u;

	__DSB();

	return 0;
}
