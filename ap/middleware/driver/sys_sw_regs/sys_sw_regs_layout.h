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

#ifndef BK7259_SYS_SW_REGS_LAYOUT_H
#define BK7259_SYS_SW_REGS_LAYOUT_H

#include <stddef.h>
#include <stdint.h>
#include <modules/pm.h>

#define BK_SWR_SA_(a, b) a##b
#define BK_SWR_SA(a, b)  BK_SWR_SA_(a, b)
#define BK_SWR_STATIC_ASSERT(cond) \
    typedef char BK_SWR_SA(bk_swr_sa_, __LINE__)[(cond) ? 1 : -1]

/**
 * @brief Shared configuration registers accessible by all cores.
 *
 * All members are volatile to ensure multi-core visibility.
 * The structure instance is placed in SYS_SW_REGS_SECTION so the
 * linker script can map it to a shared memory region.
 */
#define CPU_CNT_MAX 4
#define SSPL_SIZE (CPU_CNT_MAX + 1)
typedef uint32_t sspl_data_t[SSPL_SIZE];

#define BK_SYS_SW_REGS_AP_HEAP_DUMP_VALID 0x41504844U
#define BK_SYS_SW_REGS_AP_EXTRA_DUMP_VALID_MASK 0xFFFF0000U
#define BK_SYS_SW_REGS_AP_EXTRA_DUMP_VALID      0x41500000U
#define BK_SYS_SW_REGS_AP_EXTRA_DUMP_SEQ_MASK   0x0000FFFFU
#define BK_SYS_SW_REGS_AP_EXTRA_DUMP_MAX        8U
#define BK_SYS_SW_REGS_ADC_KEY_VALID            0x41444B59U

#if CONFIG_AP_EMUBOOT
#define BK_SYS_SW_REGS_FLASH_INIT_NOT_DONE 0U
#define BK_SYS_SW_REGS_FLASH_INIT_DONE     1U
#endif

typedef enum {
    BK_SYS_SW_REGS_AP_HEAP_SRAM = 0,
    BK_SYS_SW_REGS_AP_HEAP_HSRAM,
    BK_SYS_SW_REGS_AP_HEAP_PSRAM,
    BK_SYS_SW_REGS_AP_HEAP_MAX,
} bk_sys_sw_regs_ap_heap_id_t;

typedef struct {
    volatile uint32_t valid;
    volatile uint32_t pool_base;
    volatile uint32_t max_alloc_end;
    volatile uint32_t reserved;
} ap_heap_dump_info_t;

typedef struct {
    volatile uint32_t valid_seq;
    volatile uint32_t start_addr;
    volatile uint32_t size;
} ap_extra_dump_info_t;

typedef struct {
    volatile uint32_t valid;
    volatile uint32_t seq;
    volatile uint32_t sample_tick;
    volatile uint16_t raw;
    volatile uint16_t mv;
    volatile uint8_t status;
    volatile uint8_t channel;
    volatile uint16_t reserved0;
    volatile uint32_t sample_period_ms;
} adc_key_sample_info_t;

typedef union {
    struct {
        volatile sspl_data_t sspl_list[32]; /**< SSPL list */
        volatile uint32_t psram_power_down; /**< PSRAM power-down flag */
        volatile uint32_t cp_reset_reason;  /**< CP reset reason code */
        volatile uint32_t ap_reset_reason;  /**< AP reset reason code */
        volatile void *riscv_swap; /**< AP/RISC-V USB host probe context */
        volatile ap_heap_dump_info_t ap_heap_dump[BK_SYS_SW_REGS_AP_HEAP_MAX]; /**< AP heap dump windows */
        volatile ap_extra_dump_info_t ap_extra_dump[BK_SYS_SW_REGS_AP_EXTRA_DUMP_MAX]; /**< AP extra dump windows */
        volatile pm_shared_info_t pm_shared_info;
        volatile adc_key_sample_info_t adc_key_sample; /**< CP ADC key latest sample */
        volatile uint8_t flash_init_done; /**< CP flash init completion flag */
        volatile uint8_t ap_cp_hang_dumping; /**< AP is dumping CP-hang context and owns UART output */
        volatile uint8_t reserved0[2];
        volatile uint32_t hspl_owner_pc[32]; /**< HSPL owner caller PC shadow, 0 means free */
        volatile uint8_t hspl_owner_core[32]; /**< HSPL owner core shadow */
        volatile uint32_t cp_heap_size_ptr; /**< Addr of CP system heap xFreeBytesRemaining (size_t); 0 = not published */
        volatile uint32_t cp_lwip_mem_info_ptr; /**< Addr of CP cp_mem_addr_info_t snapshot (lwIP/heap addrs); 0 = not published */
    };
    volatile uint32_t reserved[256]; /**< Reserved for future use */
} sys_sw_regs_t;

/* RISC-V-visible head ABI shared with sys_sw_regs_shared.h. */
#if UINTPTR_MAX == 0xFFFFFFFFU
BK_SWR_STATIC_ASSERT(sizeof(sspl_data_t) == 20U);
BK_SWR_STATIC_ASSERT(offsetof(sys_sw_regs_t, sspl_list) == 0U);
BK_SWR_STATIC_ASSERT(offsetof(sys_sw_regs_t, psram_power_down) == 640U);
BK_SWR_STATIC_ASSERT(offsetof(sys_sw_regs_t, cp_reset_reason) == 644U);
BK_SWR_STATIC_ASSERT(offsetof(sys_sw_regs_t, ap_reset_reason) == 648U);
BK_SWR_STATIC_ASSERT(offsetof(sys_sw_regs_t, riscv_swap) == 652U);
BK_SWR_STATIC_ASSERT(sizeof(sys_sw_regs_t) == 1024U);
#endif

#endif /* BK7259_SYS_SW_REGS_LAYOUT_H */
