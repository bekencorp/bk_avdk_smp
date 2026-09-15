// Copyright 2025-2026 Beken
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

/**
 * @file l2_cache.h
 * @brief L2 Cache and L1 Cache management API
 * @author Beken
 * @date 2025-09-19
 * @version 1.0
 */

#ifndef L2_CACHE_H
#define L2_CACHE_H

#include <common/bk_include.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief L2 cache operation type
 */
typedef enum {
    L2C_OP_CLEAN = 1,           /**< Clean operation */
    L2C_OP_INVALID = 2,         /**< Invalidate operation */
    L2C_OP_CLEAN_INVALID = 3    /**< Clean and invalidate operation */
} l2c_op_type_t;

/**
 * @brief Cache type
 */
typedef enum {
    CACHE_TYPE_ICACHE = 1,      /**< Instruction cache */
    CACHE_TYPE_DCACHE = 2        /**< Data cache */
} cache_type_t;

/**
 * @brief Initialize L2 cache
 * 
 * This function initializes the L2 cache with multi-core coordination.
 * Only one CPU should call this function to enable L2 cache, while the other
 * CPU should wait by polling L2 cache control register until it's enabled.
 * 
 * @note This function checks if L2 cache is already enabled to avoid
 *       triggering bus error. It also checks PMU retention flag to determine
 *       if invalidate operation is needed.
 * 
 * @return 0 on success, -1 on failure
 */
int32_t l2_cache_init(void);

/**
 * @brief Wait for L2 cache to be enabled (for secondary CPU)
 * 
 * This function should be called by the secondary CPU to wait for L2 cache
 * to be enabled by the primary CPU. During this wait, the CPU can only
 * access L2 cache registers, not other data.
 * 
 * @return 0 on success, -1 on timeout
 */
int32_t l2_cache_wait_enabled(void);

/**
 * @brief Enable or disable L2 cache
 * 
 * @param enable true to enable, false to disable
 * @return 0 on success, -1 on failure
 */
int32_t l2_cache_enable(bool enable);

/**
 * @brief Check if L2 cache is enabled
 * 
 * @return true if enabled, false otherwise
 */
bool l2_cache_is_enabled(void);

/**
 * @brief Maintain entire L2 cache
 * 
 * @param operation Operation type (clean/invalidate/clean_invalidate)
 * @return 0 on success, -1 on failure
 */
int32_t l2_cache_maintain_all(l2c_op_type_t operation);

/**
 * @brief Maintain L2 cache by address range
 * 
 * @param operation Operation type (clean/invalidate/clean_invalidate)
 * @param start_addr Start address (must be cache line aligned)
 * @param end_addr End address (must be cache line aligned)
 * @return 0 on success, -1 on failure
 */
int32_t l2_cache_maintain_range(l2c_op_type_t operation,
                                 uint32_t start_addr,
                                 uint32_t end_addr);

/**
 * @brief Enable or disable L1 cache
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param enable true to enable, false to disable
 * @return 0 on success, -1 on failure
 */
int32_t l1_cache_enable(cache_type_t type, bool enable);

/**
 * @brief Clean entire L1 cache
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @return 0 on success, -1 on failure
 */
int32_t l1_cache_clean_all(cache_type_t type);

/**
 * @brief Invalidate entire L1 cache
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @return 0 on success, -1 on failure
 */
int32_t l1_cache_invalidate_all(cache_type_t type);

/**
 * @brief Clean and invalidate entire L1 cache
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @return 0 on success, -1 on failure
 */
int32_t l1_cache_clean_invalidate_all(cache_type_t type);

/**
 * @brief Clean L1 cache by address range
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param start_addr Start address
 * @param size Size in bytes
 * @return 0 on success, -1 on failure
 */
int32_t l1_cache_clean_range(cache_type_t type,
                             uint32_t start_addr,
                             uint32_t size);

/**
 * @brief Invalidate L1 cache by address range
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param start_addr Start address
 * @param size Size in bytes
 * @return 0 on success, -1 on failure
 */
int32_t l1_cache_invalidate_range(cache_type_t type,
                                   uint32_t start_addr,
                                   uint32_t size);

/**
 * @brief Clean and invalidate L1 cache by address range
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param start_addr Start address
 * @param size Size in bytes
 * @return 0 on success, -1 on failure
 */
int32_t l1_cache_clean_invalidate_range(cache_type_t type,
                                         uint32_t start_addr,
                                         uint32_t size);

/**
 * @brief Combined cache maintenance: clean L1 then L2
 * 
 * This function performs cache maintenance on both L1 and L2 cache.
 * For clean operations, it cleans L1 first, then L2.
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param operation L2 operation type
 * @return 0 on success, -1 on failure
 */
int32_t cache_clean_all(cache_type_t type, l2c_op_type_t operation);

/**
 * @brief Combined cache maintenance: invalidate L1 then L2
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param operation L2 operation type
 * @return 0 on success, -1 on failure
 */
int32_t cache_invalidate_all(cache_type_t type, l2c_op_type_t operation);

/**
 * @brief Combined cache maintenance: clean and invalidate L1 then L2
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param operation L2 operation type
 * @return 0 on success, -1 on failure
 */
int32_t cache_clean_invalidate_all(cache_type_t type,
                                    l2c_op_type_t operation);

/**
 * @brief Combined cache maintenance by range: clean L1 then L2
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param operation L2 operation type
 * @param start_addr Start address
 * @param size Size in bytes
 * @return 0 on success, -1 on failure
 */
int32_t cache_clean_range(cache_type_t type,
                           l2c_op_type_t operation,
                           uint32_t start_addr,
                           uint32_t size);

/**
 * @brief Combined cache maintenance by range: invalidate L1 then L2
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param operation L2 operation type
 * @param start_addr Start address
 * @param size Size in bytes
 * @return 0 on success, -1 on failure
 */
int32_t cache_invalidate_range(cache_type_t type,
                                l2c_op_type_t operation,
                                uint32_t start_addr,
                                uint32_t size);

/**
 * @brief Combined cache maintenance by range: clean and invalidate L1 then L2
 * 
 * @param type Cache type (ICACHE or DCACHE)
 * @param operation L2 operation type
 * @param start_addr Start address
 * @param size Size in bytes
 * @return 0 on success, -1 on failure
 */
int32_t cache_clean_invalidate_range(cache_type_t type,
                                      l2c_op_type_t operation,
                                      uint32_t start_addr,
                                      uint32_t size);

#ifdef __cplusplus
}
#endif

#endif /* L2_CACHE_H */

