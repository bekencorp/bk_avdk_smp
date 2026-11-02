/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2020-2022 Qualcomm Innovation Center, Inc.
 * Copyright (c) 2025-2026 Beken
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Cache manipulation
 *
 * This module contains functions for manipulation caches.
 */

#include "sdkconfig.h"
#include <stdbool.h>
#include <common/bk_include.h>
#include <common/bk_typedef.h>
#include <common/bk_kernel_err.h>
#include <os/os.h>
#include "cmsis_compiler.h"
#include "cache.h"
#include "ram_regions.h"

#if CONFIG_ARMV8_M_MAINLINE
#include "armv8m_reg.h"
#endif

#if CONFIG_L2_CACHE_ENABLE
#include "l2_cache.h"
#endif

#if CONFIG_AP_SECURE_CACHE_NSC
#include <common/ap_secure_cache_nsc.h>
#endif

#include "bk_arch.h"

#if CONFIG_AP_SECURE_CACHE_NSC
typedef struct {
	volatile uint32_t interested[2];
	volatile uint32_t turn;
	/* Core index + 1 while the critical section is held, 0 when free. */
	volatile uint32_t owner;
	uint32_t reserved[4];
} ap_cache_swlock_t;

/* The storage below must stay exactly one 32-byte cache line with no
 * neighbours in the same line, so a size change must not slip through. */
_Static_assert(sizeof(ap_cache_swlock_t) == 32U,
	       "ap_cache_swlock_t must be one 32-byte line");

/*
 * Reserve one complete cache line in .sram_spinlock_section, which the AP
 * linker places directly in the shared 0x3Cxxxxxx non-cacheable SRAM alias.
 * This avoids both false sharing and dependence on the PL310 being maintained.
 */
static ap_cache_swlock_t s_ap_cache_swlock_storage
	__attribute__((used, section(".sram_spinlock_section"), aligned(32))) = {
		.reserved = { 0x434C4B31U },
	};

static inline volatile ap_cache_swlock_t *ap_cache_swlock_get(void)
{
	return &s_ap_cache_swlock_storage;
}

/*
 * The L2 belongs to both AP cores, so whole-cache maintenance is issued from
 * the primary core only: the secondary core owns no L2 state of its own and
 * must not walk the cache a running peer is using. See the ownership contract
 * in common/ap_secure_cache_nsc.h.
 */
static inline bool ap_cache_l2_maintain_allowed(void)
{
	return (rtos_get_core_id() & 1U) == 0U;
}

int32_t ap_cache_swlock_irqsave(uint32_t *saved_primask)
{
	volatile ap_cache_swlock_t *lock = ap_cache_swlock_get();
	uint32_t core_id;
	uint32_t other_id;
	uint32_t wait_count;
	uint32_t primask;

	if (saved_primask == NULL) {
		return AP_SECURE_CACHE_ERR_PARAM;
	}

	primask = __get_PRIMASK();
	__disable_irq();
	__DSB();
	__ISB();

	core_id = rtos_get_core_id() & 1U;
	other_id = core_id ^ 1U;

	/*
	 * Peterson separates the two cores, not a core from itself.
	 * interested[self] is set as soon as this core starts a lock attempt, so it
	 * doubles as the "this core is already acquiring or holding" declaration:
	 *  - owner == self + 1: the critical section is already held (post-acquire);
	 *  - interested[self] != 0: this core claimed the lock but has not published
	 *    owner yet (pre-acquire window, e.g. a fault right after the claim).
	 * Without the second check a same-core re-entry would fall through the wait
	 * loop, a nested unlock would drop the outer critical section and a nested
	 * turn write could flip the state a spinning peer waits on. Report BUSY
	 * without touching turn/interested: the caller skips the maintenance, which
	 * is the right answer on a fault path and leaves the outer ownership intact.
	 * interested[self] is 0 on the first attempt, so an uncontended acquisition
	 * is never mistaken for a nested one; the timeout path clears it again, so a
	 * later attempt by the same core enters normally.
	 */
	if ((lock->owner == (core_id + 1U)) ||
	    (lock->interested[core_id] != 0U)) {
		__set_PRIMASK(primask);
		__ISB();
		return AP_SECURE_CACHE_ERR_BUSY;
	}

	lock->interested[core_id] = 1U;
	__DMB();
	lock->turn = other_id;
	__DMB();

	wait_count = AP_SECURE_CACHE_SWLOCK_WAIT_COUNT;
	while ((lock->interested[other_id] != 0U) &&
	       (lock->turn == other_id)) {
		if (--wait_count == 0U) {
			lock->interested[core_id] = 0U;
			__DMB();
			__set_PRIMASK(primask);
			__ISB();
			return AP_SECURE_CACHE_ERR_BUSY;
		}
		__asm__ volatile("yield" ::: "memory");
	}

	lock->owner = core_id + 1U;
	__DMB();
	*saved_primask = primask;
	return AP_SECURE_CACHE_OK;
}

void ap_cache_swunlock_irqrestore(uint32_t saved_primask)
{
	volatile ap_cache_swlock_t *lock = ap_cache_swlock_get();
	uint32_t core_id = rtos_get_core_id() & 1U;

	__DMB();
	/*
	 * Release only what this core owns; a mismatched unlock must not free the
	 * real holder's critical section. Clear owner BEFORE interested[self]: the
	 * peer may acquire as soon as interested[self] reads 0 and publishes its own
	 * owner right after, so clearing owner afterwards would race with it and
	 * wipe the peer's ownership. interested[peer] would then stay set forever
	 * and every later attempt on either core would end in a BUSY timeout.
	 */
	if (lock->owner == (core_id + 1U)) {
		lock->owner = 0U;
		__DMB();
		lock->interested[core_id] = 0U;
	}
	__DSB();
	__SEV();

	__set_PRIMASK(saved_primask);
	__ISB();
}
#endif

void arch_dcache_enable(void)
{
#if CONFIG_L2_CACHE_ENABLE
	l1_cache_enable(CACHE_TYPE_DCACHE, true);
#elif CONFIG_DCACHE
	SCB_EnableDCache();
#endif
}

void arch_dcache_disable(void)
{
#if CONFIG_L2_CACHE_ENABLE
	l1_cache_enable(CACHE_TYPE_DCACHE, false);
#elif CONFIG_DCACHE
	SCB_DisableDCache();
#endif
}

int arch_dcache_flush_all(void)
{
#if CONFIG_L2_CACHE_ENABLE
	/* Clean L1 then L2 cache */
	return cache_clean_all(CACHE_TYPE_DCACHE, L2C_OP_CLEAN);
#elif CONFIG_AP_SECURE_CACHE_NSC
	/*
	 * Clean-all writes every line back before dropping it, so it cannot lose
	 * peer data and is safe wherever it runs. It is still limited to the
	 * primary core to keep a single owner for whole-cache walks: the secondary
	 * core owns no L2 state and must not start a walk on its own. See the
	 * ownership contract in common/ap_secure_cache_nsc.h.
	 */
	if (!ap_cache_l2_maintain_allowed()) {
		return 0;
	}
	return psa_ap_l2_clean_all();
#elif CONFIG_DCACHE
	SCB_CleanDCache();
	return 0;
#else
	return 0;
#endif
}

int arch_dcache_invd_all(void)
{
#if CONFIG_L2_CACHE_ENABLE
	/* Invalidate L1 then L2 cache */
	return cache_invalidate_all(CACHE_TYPE_DCACHE, L2C_OP_INVALID);
#elif CONFIG_AP_SECURE_CACHE_NSC
	/*
	 * Invalidate-all drops dirty lines without writeback. With the peer core
	 * live its PSRAM task stacks/queues (AP_PSRAM_CACHE_HEAP, L2 write-back)
	 * would revert to stale contents, so this must never run from the secondary
	 * core, whose only reachable call site is the per-core _soc_start(). The
	 * secondary core has no private L1 data lines in this configuration
	 * (CONFIG_DCACHE is off), so a no-op is equivalent and safe.
	 */
	if (!ap_cache_l2_maintain_allowed()) {
		return 0;
	}
	return psa_ap_l2_invalidate_all();
#elif CONFIG_DCACHE
	SCB_InvalidateDCache();
	return 0;
#else
	return 0;
#endif
}

int arch_dcache_flush_and_invd_all(void)
{
#if CONFIG_L2_CACHE_ENABLE
	/* Clean and invalidate L1 then L2 cache */
	return cache_clean_invalidate_all(CACHE_TYPE_DCACHE, L2C_OP_CLEAN_INVALID);
#elif CONFIG_AP_SECURE_CACHE_NSC
	/*
	 * Clean+invalidate writes the lines back before dropping them, so it does
	 * not lose peer data. It is still limited to the primary core for the same
	 * reason as invalidate-all: a core that owns no L2 state must not start a
	 * whole-cache walk while the other core is running.
	 */
	if (!ap_cache_l2_maintain_allowed()) {
		return 0;
	}
	return psa_ap_l2_clean_invalidate_all();
#elif CONFIG_DCACHE && defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	/* Check if DCache is present and enabled */
	/* CLIDR bit[1] indicates DCache presence, CCR bit[16] indicates DCache enable */
	if (SCB->CLIDR & (1UL << 1U)) {
		if (SCB->CCR & (1UL << 16U)) {
			SCB_CleanInvalidateDCache();
			return 0;
		}
	}
#endif
	return 0;
}

int arch_dcache_flush_range(void *start_addr, size_t size)
{
#if CONFIG_L2_CACHE_ENABLE
	/* Clean L1 then L2 cache by range */
	return cache_clean_range(CACHE_TYPE_DCACHE, L2C_OP_CLEAN,
	                         (uint32_t)start_addr, size);
#elif CONFIG_DCACHE
	SCB_CleanDCache_by_Addr(start_addr, size);
	return 0;
#else
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);
	return 0;
#endif
}

int arch_dcache_invd_range(void *start_addr, size_t size)
{
#if CONFIG_L2_CACHE_ENABLE
	/* Invalidate L1 then L2 cache by range */
	return cache_invalidate_range(CACHE_TYPE_DCACHE, L2C_OP_INVALID,
	                              (uint32_t)start_addr, size);
#elif CONFIG_DCACHE
	SCB_InvalidateDCache_by_Addr(start_addr, size);
	return 0;
#else
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);
	return 0;
#endif
}

int arch_dcache_flush_and_invd_range(void *start_addr, size_t size)
{
#if CONFIG_L2_CACHE_ENABLE
	/* Clean and invalidate L1 then L2 cache by range */
	return cache_clean_invalidate_range(CACHE_TYPE_DCACHE, L2C_OP_CLEAN_INVALID,
	                                    (uint32_t)start_addr, size);
#elif CONFIG_DCACHE
	SCB_CleanInvalidateDCache_by_Addr(start_addr, size);
	return 0;
#else
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);
	return 0;
#endif
}

void arch_icache_enable(void)
{
#if CONFIG_L2_CACHE_ENABLE
	l1_cache_enable(CACHE_TYPE_ICACHE, true);
#elif CONFIG_ICACHE
	SCB_EnableICache();
#endif
}

void arch_icache_disable(void)
{
#if CONFIG_L2_CACHE_ENABLE
	l1_cache_enable(CACHE_TYPE_ICACHE, false);
#elif CONFIG_ICACHE
	SCB_DisableICache();
#endif
}

int arch_icache_flush_all(void)
{
	/* ICache flush is not supported on ARM Cortex-M */
	return -kUnsupportedErr;
}

int arch_icache_invd_all(void)
{
#if CONFIG_L2_CACHE_ENABLE
	/* Invalidate L1 then L2 cache */
	return cache_invalidate_all(CACHE_TYPE_ICACHE, L2C_OP_INVALID);
#elif CONFIG_ICACHE
	SCB_InvalidateICache();
	return 0;
#else
	return 0;
#endif
}

int arch_icache_flush_and_invd_all(void)
{
	/* ICache flush is not supported on ARM Cortex-M */
	return -kUnsupportedErr;
}

int arch_icache_flush_range(void *start_addr, size_t size)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);

	/* ICache flush is not supported on ARM Cortex-M */
	return -kUnsupportedErr;
}

int arch_icache_invd_range(void *start_addr, size_t size)
{
#if CONFIG_L2_CACHE_ENABLE
	/* Invalidate L1 then L2 cache by range */
	return cache_invalidate_range(CACHE_TYPE_ICACHE, L2C_OP_INVALID,
	                              (uint32_t)start_addr, size);
#elif CONFIG_ICACHE
	SCB_InvalidateICache_by_Addr(start_addr, size);
	return 0;
#else
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);
	return 0;
#endif
}

int arch_icache_flush_and_invd_range(void *start_addr, size_t size)
{
	ARG_UNUSED(start_addr);
	ARG_UNUSED(size);

	/* ICache flush is not supported on ARM Cortex-M */
	return -kUnsupportedErr;
}

void arch_cache_init(void)
{
#if CONFIG_L2_CACHE_ENABLE
	/* L2 cache initialization is handled in system.c */
	/* This function is kept for compatibility */
#endif
}

#if CONFIG_SUPPORT_L1_CACHE
void invalidate_icache(void)
{
    /* L1 unified cache*/
    SCB_InvalidateDCache();
}

void unified_cache_enable_icache(void)
{
    /*cache access through CCR and MSCR registers. it reused
     * relevant bit of data cache,including CCR.DC and MSCR.DCACTIVE
     */
    SCB_EnableDCache();
}

void unified_cache_enable_icache_without_invalidate(void)
{
    SCB_EnableDCacheWithoutInvalidate();
}

void unified_cache_disable_icache(void)
{
    /* L1 unified cache*/
    SCB_DisableDCache();
}
#else
void invalidate_icache(void)
{
    SCB_InvalidateICache();
}
#endif

/*
 * ------------------------------------------------------------------
 * Cross-core / DMA D-cache coherency helpers (see cache.h and the
 * 8131 shared-PSRAM coherency guide).
 * ------------------------------------------------------------------
 */

/* A PSRAM MPU region is D-cacheable unless its attribute index selects the
 * non-cacheable (1) or device (2) memory type in mpu_attrs[]. */
#define BK_MPU_ATTR_IS_DCACHEABLE(a)   (((a) != 1U) && ((a) != 2U))

#if CONFIG_SPE
#define BK_DC_REGION_BASE(idx)  CONFIG_PSRAM_MPU_REGION_##idx##_BASE
#define BK_DC_REGION_LIMIT(idx) CONFIG_PSRAM_MPU_REGION_##idx##_LIMIT
#else
#define BK_DC_REGION_BASE(idx)  CONFIG_PSRAM_MPU_NS_REGION_##idx##_BASE
#define BK_DC_REGION_LIMIT(idx) CONFIG_PSRAM_MPU_NS_REGION_##idx##_LIMIT
#endif

bool bk_dcache_addr_is_cacheable(const void *addr, size_t len)
{
	if ((addr == 0) || (len == 0U)) {
		return false;
	}

	uint32_t s = (uint32_t)addr;
	uint32_t e = s + (uint32_t)len;

#define BK_DC_RANGE_HIT(idx)                                                  \
	(BK_MPU_ATTR_IS_DCACHEABLE(CONFIG_PSRAM_MPU_REGION_##idx##_ATTR) &&        \
	 (s < (uint32_t)BK_DC_REGION_LIMIT(idx)) &&                                \
	 (e > (uint32_t)BK_DC_REGION_BASE(idx)))

#if defined(CONFIG_PSRAM_MPU_REGION_COUNT) && (CONFIG_PSRAM_MPU_REGION_COUNT > 0)
	if (BK_DC_RANGE_HIT(0)) return true;
#endif
#if defined(CONFIG_PSRAM_MPU_REGION_COUNT) && (CONFIG_PSRAM_MPU_REGION_COUNT > 1)
	if (BK_DC_RANGE_HIT(1)) return true;
#endif
#if defined(CONFIG_PSRAM_MPU_REGION_COUNT) && (CONFIG_PSRAM_MPU_REGION_COUNT > 2)
	if (BK_DC_RANGE_HIT(2)) return true;
#endif
#if defined(CONFIG_PSRAM_MPU_REGION_COUNT) && (CONFIG_PSRAM_MPU_REGION_COUNT > 3)
	if (BK_DC_RANGE_HIT(3)) return true;
#endif

#undef BK_DC_RANGE_HIT
#undef BK_DC_REGION_LIMIT
#undef BK_DC_REGION_BASE

	return false;
}

void bk_dcache_clean_for_producer(void *addr, size_t len)
{
#if CONFIG_CACHE_MAINTENANCE
	if (!bk_dcache_addr_is_cacheable(addr, len)) {
		return;
	}
	(void)arch_dcache_flush_range(addr, len);   /* clean-only (write-back) */
	__DSB();
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(len);
#endif
}

void bk_dcache_invalidate_for_consumer(void *addr, size_t len)
{
#if CONFIG_CACHE_MAINTENANCE
	if (!bk_dcache_addr_is_cacheable(addr, len)) {
		return;
	}
	(void)arch_dcache_invd_range(addr, len);    /* invalidate-only */
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(len);
#endif
}