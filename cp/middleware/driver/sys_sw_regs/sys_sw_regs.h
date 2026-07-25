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
#include <modules/pm.h>
#include "sys_sw_regs_layout.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Memory section name for sys_sw_regs shared data.
 *        Modify this macro to change the target linker section.
 */
#define SYS_SW_REGS_SECTION_NAME    ".shared_memory"
#define SYS_SW_REGS_SECTION         __attribute__((section(SYS_SW_REGS_SECTION_NAME)))

#define BK_SYS_SW_REGS_LOCK_DISABLE  (0U)
#define BK_SYS_SW_REGS_LOCK_ENABLE   (1U)

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

/**
 * @brief Read the AP heap dump window for the selected heap pool.
 * @param id Heap pool identifier.
 * @param info Output buffer for the shared register contents.
 * @return 1 if a valid heap dump window exists, otherwise 0.
 */
uint32_t bk_sys_sw_regs_get_ap_heap_dump(bk_sys_sw_regs_ap_heap_id_t id, ap_heap_dump_info_t *info);
uint32_t bk_sys_sw_regs_get_ap_extra_dump(uint32_t index, ap_extra_dump_info_t *info);
uint32_t bk_sys_sw_regs_get_hspl_owner(uint8_t res, uint8_t *core, uint32_t *pc);
uint32_t bk_sys_sw_regs_get_ap_cp_hang_dumping(void);
uint32_t bk_sys_sw_regs_get_cp_coredump_active(void);
/**
 * @brief Read PM info snapshot from shared registers.
 * @param info Output buffer for PM info.
 * @return BK_OK on success, BK_ERR_PARAM if info is NULL.
 */
bk_err_t bk_sys_sw_regs_get_pm_shared_info(pm_shared_info_t *info);
uint32_t bk_sys_sw_regs_get_adc_key_sample(adc_key_sample_info_t *info);

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

#if CONFIG_AP_EMUBOOT
/**
 * @brief Get the CP flash init completion flag.
 * @return Current value of flash_init_done.
 */
uint32_t bk_sys_sw_regs_get_flash_init_done(void);

/**
 * @brief Set the CP flash init completion flag.
 * @param value Value to write.
 */
void bk_sys_sw_regs_set_flash_init_done(uint32_t value);
#endif

/**
 * @brief Update the AP heap dump window for the selected heap pool.
 * @param id Heap pool identifier.
 * @param pool_base Heap pool start address.
 * @param max_alloc_end Monotonic allocation high-water end address.
 */
void bk_sys_sw_regs_update_ap_heap_dump(bk_sys_sw_regs_ap_heap_id_t id, uint32_t pool_base, uint32_t max_alloc_end);
void bk_sys_sw_regs_update_ap_extra_dump(uint32_t index, uint32_t start_addr, uint32_t size);
void bk_sys_sw_regs_set_adc_key_sample(uint16_t raw, uint16_t mv, uint8_t status, uint8_t channel, uint32_t sample_period_ms, uint32_t sample_tick);
void bk_sys_sw_regs_set_hspl_owner(uint8_t res, uint8_t core, uint32_t pc);
void bk_sys_sw_regs_clear_hspl_owner(uint8_t res);
void bk_sys_sw_regs_set_ap_cp_hang_dumping(uint32_t value);
void bk_sys_sw_regs_set_cp_coredump_active(uint32_t value);
/**
 * @brief Publish the address of the CP system heap free counter
 *        (FreeRTOS xFreeBytesRemaining) so AP can read it cross-core.
 * @param addr Address of the size_t free-bytes counter (constant after link).
 */
void bk_sys_sw_regs_set_cp_heap_free_ptr(uint32_t addr);
/**
 * @brief Publish the address of the CP cp_mem_addr_info_t snapshot
 *        (lwIP stats + heap total/reserve addresses) so AP can read it
 *        cross-core without an IPC handshake.
 * @param addr Address of the CP-side static cp_mem_addr_info_t instance.
 */
void bk_sys_sw_regs_set_cp_lwip_mem_info_ptr(uint32_t addr);
/**
 * @brief Publish the address of the CP chip UID snapshot (bk_uid_snapshot_t)
 *        so AP can read the 32-byte UID cross-core without an OTP re-read.
 * @param addr Address of the CP-side static bk_uid_snapshot_t instance; 0 clears.
 */
void bk_sys_sw_regs_set_cp_uid_ptr(uint32_t addr);
/**
 * @brief Update PM info fields selected by mask.
 * @param info Input PM info values.
 * @param field_mask Bitmask of bk_sys_sw_regs_pm_shared_info_field_t to update.
 * @param use_lock BK_SYS_SW_REGS_LOCK_ENABLE to use lock, BK_SYS_SW_REGS_LOCK_DISABLE for early init.
 * @return BK_OK on success, BK_ERR_PARAM if input is invalid.
 */
bk_err_t bk_sys_sw_regs_update_pm_shared_info(const pm_shared_info_t *info, uint32_t field_mask, uint8_t use_lock);
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
