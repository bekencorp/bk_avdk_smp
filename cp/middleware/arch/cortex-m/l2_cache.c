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
 * @file l2_cache.c
 * @brief L2 Cache and L1 Cache management implementation
 */

#include "l2_cache.h"
#include "cmsis_gcc.h"
#include "armcm52.h"

/*
 * CM52SUB L2 cache maintenance interface.
 *
 * This L2 cache controller is NOT PL310. It uses maintenance control registers
 * (ALL/LINES) and SECIRQ status/clear bits for completion notification.
 *
 * Reference: the provided M52SUB sample code (m52_cache.txt).
 */
#define L2C_M52_BASE_ADDR                  0xE0060000U

#define L2C_MAINT_CTRL_ALL_REG_OFFSET      0x20U
#define L2C_MAINT_CTRL_LINES_REG_OFFSET    0x24U
#define L2C_SECIRQSTAT_REG_OFFSET          0x100U
#define L2C_SECIRQCLR_REG_OFFSET           0x104U

#define L2C_CACHE_LINE_SIZE                32U

#define L2C_SECIRQSTAT_REQ_DONE_BIT        (1U << 2)
#define L2C_SECIRQSTAT_REQ_ERROR_BIT       (1U << 3)
#define L2C_SECIRQCLR_REQ_DONE_BIT         (1U << 2)

#define L2C_OP_WAIT_TIMEOUT                1000000U

#define L2C_M52_REG(offset) \
    (*((volatile uint32_t *)(L2C_M52_BASE_ADDR + (offset))))

static int32_t l2_cache_wait_req_complete(void)
{
    uint32_t timeout = L2C_OP_WAIT_TIMEOUT;

    while (timeout > 0U) {
        uint32_t stat = L2C_M52_REG(L2C_SECIRQSTAT_REG_OFFSET);
        if ((stat & L2C_SECIRQSTAT_REQ_DONE_BIT) != 0U) {
            if ((stat & L2C_SECIRQSTAT_REQ_ERROR_BIT) != 0U) {
                return -1;
            }

            /* Clear request-done interrupt/status (bit2 -> 0x4). */
            L2C_M52_REG(L2C_SECIRQCLR_REG_OFFSET) = L2C_SECIRQCLR_REQ_DONE_BIT;
            __DSB();
            __ISB();
            return 0;
        }
        timeout--;
    }

    return -1;
}

int32_t l2_cache_init(void)
{
    /*
     * For CM52SUB, L2 cache enable/config is handled by platform init.
     * This function is kept to match the common API.
     */
    __DSB();
    __ISB();
    return 0;
}

int32_t l2_cache_wait_enabled(void)
{
    /* CM52SUB has no PL310-like enable bit exposed here; treat as ready. */
    __DSB();
    __ISB();
    return 0;
}

int32_t l2_cache_enable(bool enable)
{
    (void)enable;

    /*
     * CM52SUB L2 cache enable/disable is not controlled from this driver.
     * Keep API for compatibility.
     */
    __DSB();
    __ISB();
    return 0;
}

bool l2_cache_is_enabled(void)
{
    /* Assume enabled when CONFIG_L2_CACHE_ENABLE is set on CM52SUB. */
    return true;
}

int32_t l2_cache_maintain_all(l2c_op_type_t operation)
{
    if ((operation != L2C_OP_CLEAN) &&
        (operation != L2C_OP_INVALID) &&
        (operation != L2C_OP_CLEAN_INVALID)) {
        return -1;
    }

    /*
     * bit0=1: clean all
     * bit1=1: invalidate all
     * bit0/bit1 can be set at the same time.
     */
    L2C_M52_REG(L2C_MAINT_CTRL_ALL_REG_OFFSET) = (uint32_t)operation;
    __DSB();
    __ISB();

    return l2_cache_wait_req_complete();
}

int32_t l2_cache_maintain_range(l2c_op_type_t operation,
                                 uint32_t start_addr,
                                 uint32_t end_addr)
{
    uint32_t addr;

    if (end_addr <= start_addr) {
        return 0;
    }

    if ((operation != L2C_OP_CLEAN) &&
        (operation != L2C_OP_INVALID) &&
        (operation != L2C_OP_CLEAN_INVALID)) {
        return -1;
    }

    /* Align range to cache line boundaries to fully cover the interval. */
    start_addr = start_addr & ~(L2C_CACHE_LINE_SIZE - 1U);
    end_addr = (end_addr + (L2C_CACHE_LINE_SIZE - 1U)) &
               ~(L2C_CACHE_LINE_SIZE - 1U);

    for (addr = start_addr; addr < end_addr; addr += L2C_CACHE_LINE_SIZE) {
        uint32_t maint_val = addr & ~(L2C_CACHE_LINE_SIZE - 1U);

        /*
         * MAINT_CTRL_LINES:
         * - Address is in [31:5] (cache line aligned).
         * - bit0/bit1 select clean/invalidate.
         */
        maint_val |= (uint32_t)operation;
        L2C_M52_REG(L2C_MAINT_CTRL_LINES_REG_OFFSET) = maint_val;
        __DSB();
        __ISB();

        if (l2_cache_wait_req_complete() != 0) {
            return -1;
        }
    }

    return 0;
}

int32_t l1_cache_enable(cache_type_t type, bool enable)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if (type == CACHE_TYPE_ICACHE) {
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
        if (enable) {
            SCB_EnableICache();
        } else {
            SCB_DisableICache();
        }
        return 0;
#else
        return -1;
#endif
    } else if (type == CACHE_TYPE_DCACHE) {
        if (enable) {
            SCB_EnableDCache();
        } else {
            SCB_DisableDCache();
        }
        return 0;
    }
#endif
    return -1;
}

int32_t l1_cache_clean_all(cache_type_t type)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if (type == CACHE_TYPE_DCACHE) {
        SCB_CleanDCache();
        return 0;
    }
#endif
    return -1;
}

int32_t l1_cache_invalidate_all(cache_type_t type)
{
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
    if (type == CACHE_TYPE_ICACHE) {
        SCB_InvalidateICache();
        return 0;
    }
#endif
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if (type == CACHE_TYPE_DCACHE) {
        SCB_InvalidateDCache();
        return 0;
    }
#endif
    return -1;
}

int32_t l1_cache_clean_invalidate_all(cache_type_t type)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if (type == CACHE_TYPE_DCACHE) {
        SCB_CleanInvalidateDCache();
        return 0;
    }
#endif
    return -1;
}

int32_t l1_cache_clean_range(cache_type_t type,
                              uint32_t start_addr,
                              uint32_t size)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if (type == CACHE_TYPE_DCACHE) {
        SCB_CleanDCache_by_Addr((void*)start_addr, size);
        return 0;
    }
#endif
    return -1;
}

int32_t l1_cache_invalidate_range(cache_type_t type,
                                   uint32_t start_addr,
                                   uint32_t size)
{
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
    if (type == CACHE_TYPE_ICACHE) {
        SCB_InvalidateICache_by_Addr((void*)start_addr, size);
        return 0;
    }
#endif
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if (type == CACHE_TYPE_DCACHE) {
        SCB_InvalidateDCache_by_Addr((void*)start_addr, size);
        return 0;
    }
#endif
    return -1;
}

int32_t l1_cache_clean_invalidate_range(cache_type_t type,
                                         uint32_t start_addr,
                                         uint32_t size)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    if (type == CACHE_TYPE_DCACHE) {
        SCB_CleanInvalidateDCache_by_Addr((void*)start_addr, size);
        return 0;
    }
#endif
    return -1;
}

int32_t cache_clean_all(cache_type_t type, l2c_op_type_t operation)
{
    int32_t ret = 0;

    if (type != CACHE_TYPE_DCACHE) {
        return -1;
    }

#if CONFIG_DCACHE
    {
        ret = l1_cache_clean_all(type);
        if (ret != 0) {
            return ret;
        }
    }
#endif
    
#if CONFIG_L2_CACHE_ENABLE
    if (operation == L2C_OP_CLEAN || operation == L2C_OP_CLEAN_INVALID) {
        ret = l2_cache_maintain_all(operation);
    }
#else
    (void)operation;
#endif
    
    return ret;
}

int32_t cache_invalidate_all(cache_type_t type, l2c_op_type_t operation)
{
    int32_t ret = 0;
    bool maintain_l1 = false;

    if ((type != CACHE_TYPE_ICACHE) && (type != CACHE_TYPE_DCACHE)) {
        return -1;
    }

#if CONFIG_ICACHE
    maintain_l1 = maintain_l1 || (type == CACHE_TYPE_ICACHE);
#endif
#if CONFIG_DCACHE
    maintain_l1 = maintain_l1 || (type == CACHE_TYPE_DCACHE);
#endif

    if (maintain_l1) {
        ret = l1_cache_invalidate_all(type);
        if (ret != 0) {
            return ret;
        }
    }
    
#if CONFIG_L2_CACHE_ENABLE
    if (operation == L2C_OP_INVALID || operation == L2C_OP_CLEAN_INVALID) {
        ret = l2_cache_maintain_all(operation);
    }
#else
    (void)operation;
#endif
    
    return ret;
}

int32_t cache_clean_invalidate_all(cache_type_t type,
                                    l2c_op_type_t operation)
{
    int32_t ret = 0;

    if (type != CACHE_TYPE_DCACHE) {
        return -1;
    }

#if CONFIG_DCACHE
    {
        ret = l1_cache_clean_invalidate_all(type);
        if (ret != 0) {
            return ret;
        }
    }
#endif
    
#if CONFIG_L2_CACHE_ENABLE
    ret = l2_cache_maintain_all(operation);
#else
    (void)operation;
#endif
    
    return ret;
}

int32_t cache_clean_range(cache_type_t type,
                          l2c_op_type_t operation,
                          uint32_t start_addr,
                          uint32_t size)
{
    int32_t ret = 0;
    uint32_t end_addr = start_addr + size;

    if (type != CACHE_TYPE_DCACHE) {
        return -1;
    }

#if CONFIG_DCACHE
    {
        ret = l1_cache_clean_range(type, start_addr, size);
        if (ret != 0) {
            return ret;
        }
    }
#endif
    
#if CONFIG_L2_CACHE_ENABLE
    if (operation == L2C_OP_CLEAN || operation == L2C_OP_CLEAN_INVALID) {
        ret = l2_cache_maintain_range(operation, start_addr, end_addr);
    }
#else
    (void)operation;
    (void)end_addr;
#endif
    
    return ret;
}

int32_t cache_invalidate_range(cache_type_t type,
                                l2c_op_type_t operation,
                                uint32_t start_addr,
                                uint32_t size)
{
    int32_t ret = 0;
    uint32_t end_addr = start_addr + size;
    bool maintain_l1 = false;

    if ((type != CACHE_TYPE_ICACHE) && (type != CACHE_TYPE_DCACHE)) {
        return -1;
    }

#if CONFIG_ICACHE
    maintain_l1 = maintain_l1 || (type == CACHE_TYPE_ICACHE);
#endif
#if CONFIG_DCACHE
    maintain_l1 = maintain_l1 || (type == CACHE_TYPE_DCACHE);
#endif

    if (maintain_l1) {
        ret = l1_cache_invalidate_range(type, start_addr, size);
        if (ret != 0) {
            return ret;
        }
    }
    
#if CONFIG_L2_CACHE_ENABLE
    if (operation == L2C_OP_INVALID || operation == L2C_OP_CLEAN_INVALID) {
        ret = l2_cache_maintain_range(operation, start_addr, end_addr);
    }
#else
    (void)operation;
    (void)end_addr;
#endif
    
    return ret;
}

int32_t cache_clean_invalidate_range(cache_type_t type,
                                      l2c_op_type_t operation,
                                      uint32_t start_addr,
                                      uint32_t size)
{
    int32_t ret = 0;
    uint32_t end_addr = start_addr + size;

    if (type != CACHE_TYPE_DCACHE) {
        return -1;
    }

#if CONFIG_DCACHE
    {
        ret = l1_cache_clean_invalidate_range(type, start_addr, size);
        if (ret != 0) {
            return ret;
        }
    }
#endif
    
#if CONFIG_L2_CACHE_ENABLE
    ret = l2_cache_maintain_range(operation, start_addr, end_addr);
#else
    (void)operation;
    (void)end_addr;
#endif
    
    return ret;
}

