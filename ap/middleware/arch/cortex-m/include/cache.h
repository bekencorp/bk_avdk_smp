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

#include <common/bk_typedef.h>

#include "bk_arch.h"
#include "armv7m_v8m_cachel1.h"

void sram_dcache_map(void);
void enable_dcache(int enable);
void invalidate_icache(void);

/**
 * @brief Enable the i-cache
 *
 * Enable the instruction cache.
 */
void arch_icache_enable(void);

#define cache_instr_enable arch_icache_enable

/**
 * @brief Disable the i-cache
 *
 * Disable the instruction cache.
 */
void arch_icache_disable(void);

#define cache_instr_disable arch_icache_disable

/**
 * @brief Flush the i-cache
 *
 * Flush the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_all(void);

#define cache_instr_flush_all arch_icache_flush_all

/**
 * @brief Invalidate the i-cache
 *
 * Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_invd_all(void);

#define cache_instr_invd_all arch_icache_invd_all

/**
 * @brief Flush and Invalidate the i-cache
 *
 * Flush and Invalidate the whole instruction cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_and_invd_all(void);

#define cache_instr_flush_and_invd_all arch_icache_flush_and_invd_all

/**
 * @brief Flush an address range in the i-cache
 *
 * Flush the specified address range of the instruction cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed. This is usually
 *       not a problem because writing back is a non-destructive process that
 *       could be triggered by hardware at any time, so having an aligned
 *       @p addr or a padded @p size is not strictly necessary.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_range(void *addr, size_t size);

#define cache_instr_flush_range(addr, size) arch_icache_flush_range(addr, size)

/**
 * @brief Invalidate an address range in the i-cache
 *
 * Invalidate the specified address range of the instruction cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being invalidated, all the portions of the
 *       non-read-only data structures sharing the same line will be
 *       invalidated as well. This is a destructive process that could lead to
 *       data loss and/or corruption. When @p addr is not aligned to the cache
 *       line and/or @p size is not a multiple of the cache line size the
 *       behaviour is undefined.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_invd_range(void *addr, size_t size);

#define cache_instr_invd_range(addr, size) arch_icache_invd_range(addr, size)

/**
 * @brief Flush and Invalidate an address range in the i-cache
 *
 * Flush and Invalidate the specified address range of the instruction cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed before being
 *       invalidated. This is usually not a problem because writing back is a
 *       non-destructive process that could be triggered by hardware at any
 *       time, so having an aligned @p addr or a padded @p size is not strictly
 *       necessary.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_icache_flush_and_invd_range(void *addr, size_t size);

#define cache_instr_flush_and_invd_range(addr, size) \
	arch_icache_flush_and_invd_range(addr, size)

/**
 * @brief Enable the d-cache
 *
 * Enable the data cache.
 */
void arch_dcache_enable(void);

#define cache_data_enable arch_dcache_enable

/**
 * @brief Disable the d-cache
 *
 * Disable the data cache.
 */
void arch_dcache_disable(void);

#define cache_data_disable arch_dcache_disable

/**
 * @brief Flush the d-cache
 *
 * Flush the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_all(void);

#define cache_data_flush_all arch_dcache_flush_all

/**
 * @brief Invalidate the d-cache
 *
 * Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_invd_all(void);

#define cache_data_invd_all arch_dcache_invd_all

/**
 * @brief Flush and Invalidate the d-cache
 *
 * Flush and Invalidate the whole data cache.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_and_invd_all(void);

#define cache_data_flush_and_invd_all arch_dcache_flush_and_invd_all

/**
 * @brief Flush an address range in the d-cache
 *
 * Flush the specified address range of the data cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed. This is usually
 *       not a problem because writing back is a non-destructive process that
 *       could be triggered by hardware at any time, so having an aligned
 *       @p addr or a padded @p size is not strictly necessary.
 *
 * @param addr Starting address to flush.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_flush_range(void *addr, size_t size);

#define cache_data_flush_range(addr, size) arch_dcache_flush_range(addr, size)

/**
 * @brief Invalidate an address range in the d-cache
 *
 * Invalidate the specified address range of the data cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being invalidated, all the portions of the
 *       non-read-only data structures sharing the same line will be
 *       invalidated as well. This is a destructive process that could lead to
 *       data loss and/or corruption. When @p addr is not aligned to the cache
 *       line and/or @p size is not a multiple of the cache line size the
 *       behaviour is undefined.
 *
 * @param addr Starting address to invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */
int arch_dcache_invd_range(void *addr, size_t size);

#define cache_data_invd_range(addr, size) arch_dcache_invd_range(addr, size)

/**
 * @brief Flush and Invalidate an address range in the d-cache
 *
 * Flush and Invalidate the specified address range of the data cache.
 *
 * @note the cache operations act on cache line. When multiple data structures
 *       share the same cache line being flushed, all the portions of the
 *       data structures sharing the same line will be flushed before being
 *       invalidated. This is usually not a problem because writing back is a
 *       non-destructive process that could be triggered by hardware at any
 *       time, so having an aligned @p addr or a padded @p size is not strictly
 *       necessary.
 *
 * @param addr Starting address to flush and invalidate.
 * @param size Range size.
 *
 * @retval 0 If succeeded.
 * @retval -ENOTSUP If not supported.
 * @retval -errno Negative errno for other failures.
 */

int arch_dcache_flush_and_invd_range(void *addr, size_t size);

#define cache_data_flush_and_invd_range(addr, size) \
	arch_dcache_flush_and_invd_range(addr, size)

/**
 *
 * @brief Get the d-cache line size.
 *
 * The API is provided to dynamically detect the data cache line size at run
 * time.
 *
 * The function must be implemented only when CONFIG_DCACHE_LINE_SIZE_DETECT is
 * defined.
 *
 * @retval size Size of the d-cache line.
 * @retval 0 If the d-cache is not enabled.
 */
size_t arch_dcache_line_size_get(void);

#define cache_data_line_size_get arch_dcache_line_size_get

static inline void flush_dcache(void *va, long size)
{
	arch_dcache_flush_and_invd_range(va, size);
}

static inline void flush_all_dcache(void)
{
	arch_dcache_flush_and_invd_all();
}

#if CONFIG_SUPPORT_L1_CACHE
void unified_cache_enable_icache(void);
void unified_cache_disable_icache(void);
void unified_cache_enable_icache_without_invalidate(void);
#endif
