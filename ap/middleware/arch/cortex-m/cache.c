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
#include "cmsis_compiler.h"
#include "cache.h"
#include "ram_regions.h"

#if CONFIG_ARMV8_M_MAINLINE
#include "armv8m_reg.h"
#endif

#if CONFIG_L2_CACHE_ENABLE
#include "l2_cache.h"
#endif

#include "bk_arch.h"

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

bool bk_dcache_addr_is_cacheable(const void *addr, size_t len)
{
	if ((addr == 0) || (len == 0U)) {
		return false;
	}

	uint32_t s = (uint32_t)addr;
	uint32_t e = s + (uint32_t)len;

#define BK_DC_RANGE_HIT(idx)                                                  \
	(BK_MPU_ATTR_IS_DCACHEABLE(CONFIG_PSRAM_MPU_REGION_##idx##_ATTR) &&        \
	 (s < (uint32_t)CONFIG_PSRAM_MPU_REGION_##idx##_LIMIT) &&                  \
	 (e > (uint32_t)CONFIG_PSRAM_MPU_REGION_##idx##_BASE))

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