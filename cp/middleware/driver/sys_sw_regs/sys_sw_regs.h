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
 * @file sys_sw_regs.h
 * @brief System software registers for multi-core shared configuration
 * @author Beken
 * @date 2026-03-05
 * @version 1.0
 */

#ifndef BK7259_SYS_SW_REGS_H
#define BK7259_SYS_SW_REGS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory section name for sys_sw_regs shared data.
 *        Modify this macro to change the target linker section.
 */
#define SYS_SW_REGS_SECTION_NAME    ".shared_memory"
#define SYS_SW_REGS_SECTION         __attribute__((section(SYS_SW_REGS_SECTION_NAME)))

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

/** Must match sys_sw_regs_shared.h / RISC-V firmware */
#define RISCV_USB_PROBE_PIPE_NUM 16U

typedef struct {
    volatile uint32_t magic;
    volatile uint32_t owner;
    volatile uint32_t irq_seq;
    volatile uint32_t event;
    volatile uint32_t event_data;
    volatile uint32_t g_musb_hcd_addr;
    volatile uint32_t usb_ep0_state_addr;
    volatile uint32_t pending_ep0;
    volatile uint32_t pending_pipe_tx[RISCV_USB_PROBE_PIPE_NUM];
    volatile uint32_t pending_pipe_rx[RISCV_USB_PROBE_PIPE_NUM];
} riscv_usb_probe_t;

typedef union {
    struct {
        volatile sspl_data_t sspl_list[32];    /**< SSPL list */
        volatile uint32_t psram_power_down; /**< PSRAM power-down flag */
        volatile uint32_t cp_reset_reason;  /**< CP reset reason code  */
        volatile uint32_t ap_reset_reason;  /**< AP reset reason code  */
        volatile riscv_usb_probe_t riscv_usb_probe; /**< AP/RISC-V USB host probe context */
    };
    volatile uint32_t reserved[256];        /**< Reserved for future use */
} sys_sw_regs_t;

/* Maximum CPU count for this configuration is 4 */
#if CONFIG_CPU_CNT > 4
#error "CPU count exceeds maximum supported value (4). Please check CONFIG_CPU_CNT setting."
#endif

/* --------------------------------------------------------------------------
 * Read API (no lock required - single 32-bit volatile reads are atomic)
 * -------------------------------------------------------------------------- */

/**
 * @brief Get the PSRAM power-down flag.
 * @return Current value of psram_power_down.
 */
uint32_t bk_sys_sw_regs_get_psram_power_down(void);

/**
 * @brief Get the CP reset reason.
 * @return Current value of cp_reset_reason.
 */
uint32_t bk_sys_sw_regs_get_cp_reset_reason(void);

/**
 * @brief Get the AP reset reason.
 * @return Current value of ap_reset_reason.
 */
uint32_t bk_sys_sw_regs_get_ap_reset_reason(void);

/* --------------------------------------------------------------------------
 * Write API (protected by lock)
 * -------------------------------------------------------------------------- */

/**
 * @brief Set the PSRAM power-down flag.
 * @param value Value to write.
 */
void bk_sys_sw_regs_set_psram_power_down(uint32_t value);

/**
 * @brief Set the CP reset reason.
 * @param value Value to write.
 */
void bk_sys_sw_regs_set_cp_reset_reason(uint32_t value);

/**
 * @brief Set the AP reset reason.
 * @param value Value to write.
 */
void bk_sys_sw_regs_set_ap_reset_reason(uint32_t value);

/**
 * @brief Get the SSPL list.
 * @return Pointer to the SSPL list.
 */
void *bk_sys_sw_regs_get_sspl_list(void);
volatile sys_sw_regs_t *bk_sys_sw_regs_ptr(void);

#ifdef __cplusplus
}
#endif

#endif /* BK7259_SYS_SW_REGS_H */
