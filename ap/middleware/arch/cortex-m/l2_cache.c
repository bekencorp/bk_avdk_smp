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
#include <common/bk_err.h>
#include <common/bk_assert.h>
#include <driver/hal/hal_int_types.h>
#include <soc/bk7259/int_types_impl.h>

extern bk_err_t bk_int_isr_register(icu_int_src_t dev, int_group_isr_t isr, void *arg);
extern int32_t sys_drv_set_int_en(uint32_t core_id, uint32_t int_num, uint32_t int_en);

/* L2 Cache PL310 register definitions */
#define L2C_PL310_BASE                0xA0000000U
#define L2C_CACHE_LINE_SIZE           32U
#define L2C_CONTROL_REG_OFFSET        0x100U
#define L2C_AUX_CTRL_REG_OFFSET       0x104U
#define L2C_ADDR_FILTER_REG_OFFSET    0xC00U
#define L2C_INT_CLEAR_REG_OFFSET      0x220U
#define L2C_INT_MASK_REG_OFFSET       0x214U
#define L2C_INT_MASK_STATUS_REG_OFFSET 0x218U
#define L2C_INT_RAW_STATUS_REG_OFFSET 0x21CU
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
#define L2C_ERROR_INT_MASK            0x1FEU

/* Timeout for waiting L2 cache enable (in iterations) */
#define L2C_ENABLE_WAIT_TIMEOUT       1000000U

typedef struct {
    uint32_t raw_status;
    uint32_t masked_status;
    uint32_t active_status;
    uint32_t control;
    uint32_t aux_control;
    uint32_t core_id;
    uint32_t ipsr;
    uint32_t count;
} l2_cache_error_record_t;

volatile l2_cache_error_record_t g_l2_cache_error_record;

static bool s_l2_cache_error_monitor_enabled;

static void l2_cache_error_isr(void)
{
    uint32_t raw_status = L2C_PL310_REG(L2C_INT_RAW_STATUS_REG_OFFSET);
    uint32_t masked_status = L2C_PL310_REG(L2C_INT_MASK_STATUS_REG_OFFSET);
    uint32_t active_status = (masked_status | raw_status) & L2C_ERROR_INT_MASK;

    /* Stop any further L2 error interrupt storm before capturing state. */
    L2C_PL310_REG(L2C_INT_MASK_REG_OFFSET) = 0U;
    if (active_status != 0U) {
        L2C_PL310_REG(L2C_INT_CLEAR_REG_OFFSET) = active_status;
    }
    __DSB();
    __ISB();

    g_l2_cache_error_record.raw_status = raw_status;
    g_l2_cache_error_record.masked_status = masked_status;
    g_l2_cache_error_record.active_status = active_status;
    g_l2_cache_error_record.control = L2C_PL310_REG(L2C_CONTROL_REG_OFFSET);
    g_l2_cache_error_record.aux_control = L2C_PL310_REG(L2C_AUX_CTRL_REG_OFFSET);
    g_l2_cache_error_record.core_id = rtos_get_core_id();
    g_l2_cache_error_record.ipsr = __get_IPSR();
    g_l2_cache_error_record.count++;

    BK_ASSERT(0);
    while (1) {
        __BKPT(0);
    }
}

int32_t l2_cache_error_monitor_enable(void)
{
    if (!s_l2_cache_error_monitor_enabled) {
        if (bk_int_isr_register(INT_SRC_L2CACHE_ERR, l2_cache_error_isr, NULL) != BK_OK) {
            return -1;
        }

#if CONFIG_SOC_SMP
        (void)sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_L2CACHE_ERR, 1);
#else
        (void)sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_L2CACHE_ERR, 1);
#endif

        s_l2_cache_error_monitor_enabled = true;
    }

    L2C_PL310_REG(L2C_INT_CLEAR_REG_OFFSET) = L2C_ERROR_INT_MASK;
    L2C_PL310_REG(L2C_INT_MASK_REG_OFFSET) = L2C_ERROR_INT_MASK;
    __DSB();
    __ISB();

    return 0;
}

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

    /* Clear L2 error interrupts. ECNTR is a performance event, not an error. */
    L2C_PL310_REG(L2C_INT_CLEAR_REG_OFFSET) = L2C_ERROR_INT_MASK;

    /* Keep error interrupts masked until interrupt controller is ready. */
    L2C_PL310_REG(L2C_INT_MASK_REG_OFFSET) = 0U;

    if (control_reg & L2C_CONTROL_ENABLE_BIT) {
        if (s_l2_cache_error_monitor_enabled) {
            L2C_PL310_REG(L2C_INT_MASK_REG_OFFSET) = L2C_ERROR_INT_MASK;
        }
        __DSB();
        __ISB();
        return 0;
    }
    
    /* Configure auxiliary control register only if not enabled */
    L2C_PL310_REG(L2C_AUX_CTRL_REG_OFFSET) = 0x02020000U;
    
    /* Close address filtering */
    L2C_PL310_REG(L2C_ADDR_FILTER_REG_OFFSET) = 0x0U;
    
    /* Check PMU retention flag */
    if (!check_pmu_l2_retention_recovery()) {
        /* Not from retention, need to invalidate all L2 cache */
        if (l2_cache_invalidate_all_internal() != 0) {
            return -1;
        }
        
        /* Clear L2 error interrupts again after invalidate. */
        L2C_PL310_REG(L2C_INT_CLEAR_REG_OFFSET) = L2C_ERROR_INT_MASK;
    }
    
    /* Enable L2 cache */
    L2C_PL310_REG(L2C_CONTROL_REG_OFFSET) = L2C_CONTROL_ENABLE_BIT;

    if (s_l2_cache_error_monitor_enabled) {
        L2C_PL310_REG(L2C_INT_MASK_REG_OFFSET) = L2C_ERROR_INT_MASK;
    }
    
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
    
    __DSB();

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

    /* Ensure the maintenance is complete and ordered before subsequent
     * accesses / cross-master notifications, so range-based callers
     * (cache_*_range, flush_dcache) are self-contained and the upper layer
     * does not need to add its own barrier. Matches l2_cache_maintain_all. */
    __DSB();
    __ISB();

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

