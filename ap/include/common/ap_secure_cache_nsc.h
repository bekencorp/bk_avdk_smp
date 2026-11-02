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

#pragma once

#include <stdint.h>
#include "ram_regions.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Stable ABI between the AP Non-Secure image and the resident AP Secure shim.
 * The function pointer targets the copied AP Secure RAM image, not the source
 * blob linked into the CP TF-M image.
 *
 * Ownership contract for every caller of psa_ap_l2_*():
 *  - Whole-cache INVALIDATE discards lines without writing them back, so it is
 *    only legal while the other AP core is offline. arch_dcache_invd_all()
 *    additionally restricts it to the primary core.
 *  - Whole-cache CLEAN and CLEAN+INVALIDATE write the lines back before
 *    dropping them, so they are data-safe wherever they run. The
 *    implementation nevertheless issues them from the primary core only,
 *    because the secondary core owns no L2 state and must not start a
 *    whole-cache walk.
 *  - The software lock is advisory: it serializes cooperative callers, it does
 *    not stop Non-Secure code from entering the veneer directly.
 *  - The lock word must stay in non-cacheable SRAM: the project linker script
 *    maps .sram_spinlock_section to the non-cacheable alias. A cacheable lock
 *    breaks mutual exclusion. Placement is enforced by the linker script; a
 *    link-time ASSERT on the section attributes is tracked as a follow-up
 *    change.
 */
#define AP_SECURE_CACHE_NSC_ABI_VERSION       1U
#define AP_SECURE_CACHE_NSC_ENTRY_OFFSET      0x800U
#define AP_SECURE_CACHE_NSC_ENTRY_ADDR        \
	((uintptr_t)CONFIG_AP_SPE_RAM_ADDR + AP_SECURE_CACHE_NSC_ENTRY_OFFSET)

/*
 * One lock attempt spins for AP_SECURE_CACHE_SWLOCK_WAIT_COUNT iterations with
 * IRQs masked, so thread mode may make AP_SECURE_CACHE_BUSY_RETRY_LIMIT
 * attempts before it reports BUSY:
 *
 *   total wait = 0x1000 * 0x400 = 4M lock iterations
 *
 * Size the budget by time, not by iteration count. An NS spin iteration costs
 * about 6-10 cycles (load, compare, yield), whereas the Secure side spends its
 * AP_L2_WAIT_COUNT (0x00100000) budget on PL310 register polls that cost about
 * 20-50 cycles each, so both budgets land in the same order of magnitude in
 * time (tens of milliseconds). Invariant: the total iteration count must stay
 * above AP_L2_WAIT_COUNT, so a normal whole-L2 operation on the peer never
 * turns into a false BUSY. With a smaller limit a normal whole-L2 clean times
 * out, returns BUSY and gets dropped by callers that ignore the return value,
 * i.e. a silent "maintenance skipped" regression instead of a safety
 * improvement.
 * The full budget is only consumed when the peer is genuinely stuck; a normal
 * contention exits on the first or second attempt, and IRQs are re-enabled
 * between attempts because only the inner spin masks them.
 * Keep the inner count small so each IRQ-masked burst stays short, and keep
 * both constants in step with AP_L2_WAIT_COUNT if that budget ever changes.
 */
#define AP_SECURE_CACHE_SWLOCK_WAIT_COUNT     0x00001000U
#define AP_SECURE_CACHE_BUSY_RETRY_LIMIT      0x00000400U

typedef enum {
	AP_SECURE_CACHE_OP_CLEAN_ALL = 0,
	AP_SECURE_CACHE_OP_INVALIDATE_ALL = 1,
	AP_SECURE_CACHE_OP_CLEAN_INVALIDATE_ALL = 2,
} ap_secure_cache_op_t;

enum {
	AP_SECURE_CACHE_OK = 0,
	AP_SECURE_CACHE_ERR_PARAM = -1,
	AP_SECURE_CACHE_ERR_BUSY = -2,
	AP_SECURE_CACHE_ERR_STATE = -3,
	AP_SECURE_CACHE_ERR_TIMEOUT = -4,
};

typedef int32_t (*ap_secure_cache_nsc_fn_t)(uint32_t operation);

int32_t ap_cache_swlock_irqsave(uint32_t *saved_primask);
void ap_cache_swunlock_irqrestore(uint32_t saved_primask);

static inline int32_t
ap_secure_cache_nsc_maintain(ap_secure_cache_op_t operation)
{
	ap_secure_cache_nsc_fn_t entry =
		(ap_secure_cache_nsc_fn_t)(AP_SECURE_CACHE_NSC_ENTRY_ADDR | 1U);
	int32_t status;
	uint32_t retries = 0;
	uint32_t saved_primask;

	/*
	 * The software lock lives in shared Non-Secure non-cacheable SRAM and
	 * covers the complete NS-to-Secure cache-maintenance call. Handler mode
	 * takes one attempt only: a fault taken while this core owns the lock must
	 * degrade to "maintenance skipped", never to a nested critical section.
	 * Thread mode retries until the total wait exceeds the Secure side's own
	 * single-operation budget (see AP_SECURE_CACHE_BUSY_RETRY_LIMIT).
	 */
	do {
		status = ap_cache_swlock_irqsave(&saved_primask);
		if (status == AP_SECURE_CACHE_OK) {
			status = entry((uint32_t)operation);
			ap_cache_swunlock_irqrestore(saved_primask);
		}

		if (status == AP_SECURE_CACHE_ERR_BUSY) {
			uint32_t exception_number;

			__asm__ volatile("mrs %0, ipsr" : "=r"(exception_number));
			if ((exception_number != 0U) ||
			    (++retries >= AP_SECURE_CACHE_BUSY_RETRY_LIMIT)) {
				break;
			}
			__asm__ volatile("yield" ::: "memory");
		}
	} while (status == AP_SECURE_CACHE_ERR_BUSY);

	return status;
}

static inline int32_t psa_ap_l2_clean_all(void)
{
	return ap_secure_cache_nsc_maintain(AP_SECURE_CACHE_OP_CLEAN_ALL);
}

static inline int32_t psa_ap_l2_invalidate_all(void)
{
	return ap_secure_cache_nsc_maintain(AP_SECURE_CACHE_OP_INVALIDATE_ALL);
}

static inline int32_t psa_ap_l2_clean_invalidate_all(void)
{
	return ap_secure_cache_nsc_maintain(
		AP_SECURE_CACHE_OP_CLEAN_INVALIDATE_ALL);
}

#ifdef __cplusplus
}
#endif
