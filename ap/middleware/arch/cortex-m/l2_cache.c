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
#include <os/os.h>
#include <string.h>
#include "cmsis_gcc.h"
#include "bk_arch.h"

/* L2 Cache PL310 register definitions */
#define L2C_PL310_BASE                0xA0000000U
#define L2C_CACHE_LINE_SIZE           32U
#define L2C_CONTROL_REG_OFFSET        0x100U
#define L2C_AUX_CTRL_REG_OFFSET       0x104U
#define L2C_ADDR_FILTER_REG_OFFSET    0xC00U
#define L2C_INT_CLEAR_REG_OFFSET      0x220U
#define L2C_INT_MASK_REG_OFFSET       0x214U
#define L2C_INVALID_WAY_REG_OFFSET    0x77CU
#define L2C_CLEAN_PA_REG_OFFSET       0x7B0U
#define L2C_CLEAN_WAY_REG_OFFSET      0x7BCU
#define L2C_INVALID_PA_REG_OFFSET     0x770U
#define L2C_CLEAN_INV_PA_REG_OFFSET   0x7F0U
#define L2C_CLEAN_INV_WAY_REG_OFFSET  0x7FCU
#define L2C_CACHE_SYNC_REG_OFFSET     0x730U

#define L2C_PL310_REG(offset)         (*((volatile uint32_t*)(L2C_PL310_BASE + (offset))))

/* L2 Cache control register bit definitions */
#define L2C_CONTROL_ENABLE_BIT        (1U << 0)

/* Timeout for waiting L2 cache enable (in iterations) */
#define L2C_ENABLE_WAIT_TIMEOUT       1000000U

/* PMU register for memory retention check */
/* Note: This needs to be verified with actual PMU register definition */
/* For now, we assume there's a bit indicating L2 cache retention recovery */
static bool check_pmu_l2_retention_recovery(void)
{
    /* TODO: Implement PMU retention check
     * According to the documentation, PMU has 1 bit to indicate if
     * L2 cache is recovered from memory retention. If recovered,
     * invalidate operation is not needed.
     * 
     * This function should read the PMU register and return true
     * if L2 cache is recovered from retention, false otherwise.
     */
    return false; /* Default: not from retention, need invalidate */
}

static int32_t l2_cache_invalidate_all_internal(void)
{
    L2C_PL310_REG(L2C_INVALID_WAY_REG_OFFSET) = 0xFFFFU;
    while (L2C_PL310_REG(L2C_INVALID_WAY_REG_OFFSET) != 0U) {
        /* Wait for invalidate to complete */
    }
    return 0;
}

int32_t l2_cache_init(void)
{
    volatile uint32_t control_reg;
    
    /* Check if L2 cache is already enabled */
    control_reg = L2C_PL310_REG(L2C_CONTROL_REG_OFFSET);
    if (control_reg & L2C_CONTROL_ENABLE_BIT) {
        /* Already enabled, return success */
        return 0;
    }
    
    /* Configure auxiliary control register only if not enabled */
    L2C_PL310_REG(L2C_AUX_CTRL_REG_OFFSET) = 0x02020000U;
    
    /* Close address filtering */
    L2C_PL310_REG(L2C_ADDR_FILTER_REG_OFFSET) = 0x0U;
    
    /* Clear interrupts */
    L2C_PL310_REG(L2C_INT_CLEAR_REG_OFFSET) = 0x1FFU;
    
    /* Enable all interrupts */
    L2C_PL310_REG(L2C_INT_MASK_REG_OFFSET) = 0x1FFU;
    
    /* Check PMU retention flag */
    if (!check_pmu_l2_retention_recovery()) {
        /* Not from retention, need to invalidate all L2 cache */
        if (l2_cache_invalidate_all_internal() != 0) {
            return -1;
        }
        
        /* Clear interrupts again after invalidate */
        L2C_PL310_REG(L2C_INT_CLEAR_REG_OFFSET) = 0x1FFU;
    }
    
    /* Enable L2 cache */
    L2C_PL310_REG(L2C_CONTROL_REG_OFFSET) = L2C_CONTROL_ENABLE_BIT;
    
    /* Memory barrier to ensure write completion */
    __DSB();
    __ISB();
    
    return 0;
}

int32_t l2_cache_wait_enabled(void)
{
    uint32_t timeout = L2C_ENABLE_WAIT_TIMEOUT;
    volatile uint32_t control_reg;
    
    /* Wait for L2 cache to be enabled by polling control register */
    while (timeout > 0U) {
        control_reg = L2C_PL310_REG(L2C_CONTROL_REG_OFFSET);
        if (control_reg & L2C_CONTROL_ENABLE_BIT) {
            /* Memory barrier after detecting enable */
            __DSB();
            __ISB();
            return 0;
        }
        timeout--;
    }
    
    /* Timeout */
    return -1;
}

int32_t l2_cache_enable(bool enable)
{
    volatile uint32_t control_reg;
    
    control_reg = L2C_PL310_REG(L2C_CONTROL_REG_OFFSET);
    
    if (enable) {
        if (control_reg & L2C_CONTROL_ENABLE_BIT) {
            /* Already enabled */
            return 0;
        }
        L2C_PL310_REG(L2C_CONTROL_REG_OFFSET) = L2C_CONTROL_ENABLE_BIT;
    } else {
        if (!(control_reg & L2C_CONTROL_ENABLE_BIT)) {
            /* Already disabled */
            return 0;
        }
        L2C_PL310_REG(L2C_CONTROL_REG_OFFSET) = 0x0U;
    }
    
    __DSB();
    __ISB();
    
    return 0;
}

bool l2_cache_is_enabled(void)
{
    volatile uint32_t control_reg;
    control_reg = L2C_PL310_REG(L2C_CONTROL_REG_OFFSET);
    return ((control_reg & L2C_CONTROL_ENABLE_BIT) != 0U);
}

int32_t l2_cache_maintain_all(l2c_op_type_t operation)
{
    switch (operation) {
    case L2C_OP_CLEAN:
        L2C_PL310_REG(L2C_CLEAN_WAY_REG_OFFSET) = 0xFFFFU;
        while (L2C_PL310_REG(L2C_CLEAN_WAY_REG_OFFSET) != 0U) {
            /* Wait for clean to complete */
        }
        break;
        
    case L2C_OP_INVALID:
        L2C_PL310_REG(L2C_INVALID_WAY_REG_OFFSET) = 0xFFFFU;
        while (L2C_PL310_REG(L2C_INVALID_WAY_REG_OFFSET) != 0U) {
            /* Wait for invalidate to complete */
        }
        break;
        
    case L2C_OP_CLEAN_INVALID:
        L2C_PL310_REG(L2C_CLEAN_INV_WAY_REG_OFFSET) = 0xFFFFU;
        while (L2C_PL310_REG(L2C_CLEAN_INV_WAY_REG_OFFSET) != 0U) {
            /* Wait for clean and invalidate to complete */
        }
        break;
        
    default:
        return -1;
    }

    /* Synchronize cache operations */
    L2C_PL310_REG(L2C_CACHE_SYNC_REG_OFFSET) = 0U;
    __DSB();
    __ISB();
    
    return 0;
}

int32_t l2_cache_maintain_range(l2c_op_type_t operation,
                                 uint32_t start_addr,
                                 uint32_t end_addr)
{
    uint32_t addr;
    
    /* Align start address to cache line */
    start_addr = start_addr & ~(L2C_CACHE_LINE_SIZE - 1U);
    
    /* Process each cache line */
    for (addr = start_addr; addr < end_addr; addr += L2C_CACHE_LINE_SIZE) {
        switch (operation) {
        case L2C_OP_CLEAN:
            L2C_PL310_REG(L2C_CLEAN_PA_REG_OFFSET) = addr;
            break;
            
        case L2C_OP_INVALID:
            L2C_PL310_REG(L2C_INVALID_PA_REG_OFFSET) = addr;
            break;
            
        case L2C_OP_CLEAN_INVALID:
            L2C_PL310_REG(L2C_CLEAN_INV_PA_REG_OFFSET) = addr;
            break;
            
        default:
            return -1;
        }
    }
    
    /* Synchronize cache operations */
    L2C_PL310_REG(L2C_CACHE_SYNC_REG_OFFSET) = 0U;
    
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
    int32_t ret;
    
    /* Clean L1 cache first */
    ret = l1_cache_clean_all(type);
    if (ret != 0) {
        return ret;
    }
    
    /* Then clean L2 cache */
    if (operation == L2C_OP_CLEAN || operation == L2C_OP_CLEAN_INVALID) {
        ret = l2_cache_maintain_all(operation);
    }
    
    return ret;
}

int32_t cache_invalidate_all(cache_type_t type, l2c_op_type_t operation)
{
    int32_t ret;
    
    /* Invalidate L1 cache first */
    ret = l1_cache_invalidate_all(type);
    if (ret != 0) {
        return ret;
    }
    
    /* Then invalidate L2 cache */
    if (operation == L2C_OP_INVALID || operation == L2C_OP_CLEAN_INVALID) {
        ret = l2_cache_maintain_all(operation);
    }
    
    return ret;
}

int32_t cache_clean_invalidate_all(cache_type_t type,
                                    l2c_op_type_t operation)
{
    int32_t ret;
    
    /* Clean and invalidate L1 cache first */
    ret = l1_cache_clean_invalidate_all(type);
    if (ret != 0) {
        return ret;
    }
    
    /* Then clean and invalidate L2 cache */
    ret = l2_cache_maintain_all(operation);
    
    return ret;
}

int32_t cache_clean_range(cache_type_t type,
                          l2c_op_type_t operation,
                          uint32_t start_addr,
                          uint32_t size)
{
    int32_t ret;
    uint32_t end_addr = start_addr + size;
    
    /* Clean L1 cache first */
    ret = l1_cache_clean_range(type, start_addr, size);
    if (ret != 0) {
        return ret;
    }
    
    /* Then clean L2 cache */
    if (operation == L2C_OP_CLEAN || operation == L2C_OP_CLEAN_INVALID) {
        ret = l2_cache_maintain_range(operation, start_addr, end_addr);
    }
    
    return ret;
}

int32_t cache_invalidate_range(cache_type_t type,
                                l2c_op_type_t operation,
                                uint32_t start_addr,
                                uint32_t size)
{
    int32_t ret;
    uint32_t end_addr = start_addr + size;
    
    /* Invalidate L1 cache first */
    ret = l1_cache_invalidate_range(type, start_addr, size);
    if (ret != 0) {
        return ret;
    }
    
    /* Then invalidate L2 cache */
    if (operation == L2C_OP_INVALID || operation == L2C_OP_CLEAN_INVALID) {
        ret = l2_cache_maintain_range(operation, start_addr, end_addr);
    }
    
    return ret;
}

int32_t cache_clean_invalidate_range(cache_type_t type,
                                      l2c_op_type_t operation,
                                      uint32_t start_addr,
                                      uint32_t size)
{
    int32_t ret;
    uint32_t end_addr = start_addr + size;
    
    /* Clean and invalidate L1 cache first */
    ret = l1_cache_clean_invalidate_range(type, start_addr, size);
    if (ret != 0) {
        return ret;
    }
    
    /* Then clean and invalidate L2 cache */
    ret = l2_cache_maintain_range(operation, start_addr, end_addr);
    
    return ret;
}

