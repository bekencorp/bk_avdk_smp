// Copyright 2020-2026 Beken
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

#include <common/bk_err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Upper-layer resource IDs (device / OS)
 *
 * These IDs map to (hspl_instance, channel) pairs. The mapping is defined in
 * hspl_res_lock.c and can be adjusted to your platform needs.
 */
typedef enum {
	BK_HSPL_RES_FLASH = 0,
	BK_HSPL_RES_CLOCK = 1,
	BK_HSPL_RES_POWER = 2,
	BK_HSPL_RES_SYS = 3,
	BK_HSPL_RES_RTC = 4,
	BK_HSPL_RES_ANA = 5,
	BK_HSPL_RES_FUSE = 6,
	BK_HSPL_RES_TRNG = 7,
	BK_HSPL_RES_SYS_SW_REGS = 8,
	BK_HSPL_RES_SPI = 9,
	BK_HSPL_RES_GPIO = 10,
	BK_HSPL_RES_PWM = 11,
	BK_HSPL_RES_ADC = 12,
	BK_HSPL_RES_DAC = 13,
	BK_HSPL_RES_PMU = 14,
	BK_HSPL_RES_UART_LOG = 15,

	BK_HSPL_RES_OS = 16,
	BK_HSPL_RES_LVGL = 17,
	BK_HSPL_RES_AUDIO = 18,
	BK_HSPL_RES_VIDEO = 19,
	BK_HSPL_RES_GPU = 20,
	BK_HSPL_RES_NPU = 21,
	BK_HSPL_RES_DSP = 22,
	BK_HSPL_RES_ISP = 23,
	BK_HSPL_RES_VDEC = 24,
	BK_HSPL_RES_VENC = 25,
	BK_HSPL_RES_SDIO = 26,
	BK_HSPL_RES_SDIO_HS = 27,
	BK_HSPL_RES_SDIO_HS_HS = 28,
	BK_HSPL_RES_USB = 29,
	BK_HSPL_RES_PSRAM = 30,
	BK_HSPL_RES_USER2 = 31,

	BK_HSPL_RES_MAX = 32,
} bk_hspl_res_t;

/**
 * @brief Special timeout value for infinite wait (task context only)
 */
#define BK_HSPL_WAIT_FOREVER (0xFFFFFFFFU)

/**
 * @brief Lock a resource using HSPL (spin until success or timeout)
 *
 * @param res Resource id
 * @param timeout_us Timeout in microseconds.
 *                   Use 0 for try-lock once.
 *                   Use BK_HSPL_WAIT_FOREVER for infinite wait (task context only).
 * @return BK_OK on success, BK_ERR_TIMEOUT on timeout, others on param error.
 */
bk_err_t bk_hspl_res_lock(bk_hspl_res_t res, uint32_t timeout_us);

/**
 * @brief Try lock a resource once (no wait)
 */
bk_err_t bk_hspl_res_try_lock(bk_hspl_res_t res);

/**
 * @brief Lock a resource with local IRQ disabled (try-lock once)
 *
 * @note Task context only. If lock fails, IRQ will be restored and BK_ERR_TIMEOUT returned.
 */
bk_err_t bk_hspl_res_lock_irqsave(bk_hspl_res_t res, uint32_t *flags);

/**
 * @brief Unlock a resource and restore local IRQ
 */
bk_err_t bk_hspl_res_unlock_irqrestore(bk_hspl_res_t res, uint32_t flags);

/**
 * @brief Unlock a resource using HSPL
 */
bk_err_t bk_hspl_res_unlock(bk_hspl_res_t res);

/**
 * @brief Lock a resource using HSPL (blocking until acquired or timeout)
 *
 * @note FLASH uses CONFIG_HSPL_MUST_LOCK_TIMEOUT_MS_FLASH; other resources use
 *       CONFIG_HSPL_MUST_LOCK_TIMEOUT_MS_DEFAULT (0 = wait forever).
 *       On timeout BK_ASSERT is triggered.
 *
 * @param res Resource id
 * @return BK_OK on success, BK_ERR_PARAM / BK_ERR_NOT_SUPPORTED / BK_ERR_TIMEOUT
 */
bk_err_t bk_hspl_res_must_lock(bk_hspl_res_t res);

/**
 * @brief Residual lock state of one core
 *
 * @note One bit per resource id, so BK_HSPL_RES_MAX must stay <= 32.
 */
typedef struct {
	uint32_t hw_owned_mask;	/**< BIT(res) set when HSPL still reports this core as owner */
	uint32_t sw_held_mask;	/**< BIT(res) set when this core's recursion count is not zero */
} bk_hspl_res_core_state_t;

/**
 * @brief Check that a core no longer holds any HSPL resource
 *
 * Covers both instances: HSPL_0 (res 0..15, CP<->AP, survives an AP power
 * cycle) and HSPL_1 (res 16..31, AP internal, orphans the surviving AP core
 * when its peer is reset). Call this before a core is reset or its power
 * domain is cut.
 *
 * @note Read-only probe. It samples the STA registers, which report the owner
 *       without attempting an acquire, plus the local recursion counters.
 *       It never releases a lock on behalf of the core: an orphaned lock must
 *       be prevented, not forcibly cleared.
 *       Safe to call from an ISR or with local interrupts disabled.
 *
 * @param hw_owner_id   Hardware owner id of the core (e.g. CPU3_CORE_ID)
 * @param sw_core_index Index of the core in the local recursion table (SMP core id)
 * @param state         Optional, receives the residual masks
 *
 * @return BK_OK when nothing is held, BK_ERR_BUSY when a residual lock is found,
 *         BK_ERR_PARAM when sw_core_index is out of range
 */
bk_err_t bk_hspl_res_check_core_released(uint8_t hw_owner_id, uint8_t sw_core_index,
	bk_hspl_res_core_state_t *state);

void bk_hspl_res_dbg_set_owner(bk_hspl_res_t res, uint8_t core, uint32_t pc);
void bk_hspl_res_dbg_clear_owner(bk_hspl_res_t res);

#if CONFIG_HSPL_LEAK_DEBUG
#define BK_HSPL_RES3_TRACE_DEPTH 64U

typedef enum {
	BK_HSPL_RES3_TRACE_ENTER_REQ = 1,
	BK_HSPL_RES3_TRACE_ENTER_GOT = 2,
	BK_HSPL_RES3_TRACE_OWNER_SET = 3,
	BK_HSPL_RES3_TRACE_EXIT_REQ = 4,
	BK_HSPL_RES3_TRACE_OWNER_CLEAR = 5,
	BK_HSPL_RES3_TRACE_EXIT_DONE = 6,
	BK_HSPL_RES3_TRACE_RECUR_ENTER = 7,
	BK_HSPL_RES3_TRACE_RECUR_EXIT = 8,
	BK_HSPL_RES3_TRACE_TIMEOUT = 9,
} bk_hspl_res3_trace_action_t;

typedef struct {
	volatile uint32_t seq;
	volatile uint32_t tick_ms;
	volatile uint32_t action;
	volatile uint32_t core;
	volatile uint32_t pc;
	volatile uint32_t rec_before;
	volatile uint32_t rec_after;
	volatile uint32_t ret;
} bk_hspl_res3_trace_entry_t;

typedef struct {
	volatile uint32_t magic;
	volatile uint32_t version;
	volatile uint32_t entry_size;
	volatile uint32_t depth;
	volatile uint32_t write_index;
	volatile uint32_t wrapped;
	volatile uint32_t registered;
	volatile uint32_t reserved;
	volatile bk_hspl_res3_trace_entry_t entries[BK_HSPL_RES3_TRACE_DEPTH];
} bk_hspl_res3_trace_buffer_t;

void bk_hspl_res3_trace_record(bk_hspl_res_t res, bk_hspl_res3_trace_action_t action,
	uint32_t pc, uint32_t ret);
const volatile bk_hspl_res3_trace_buffer_t *bk_hspl_res3_trace_get(void);
void bk_hspl_res3_trace_clear(void);
#endif

/**
 * @brief Get mapping info for debug
 */
bk_err_t bk_hspl_res_get_map(bk_hspl_res_t res, uint8_t *hspl_id, uint8_t *channel);

/**
 * @brief Enter critical section for driver (disable IRQ and acquire HSPL lock)
 *
 * @note This function disables interrupts and acquires the SYS resource lock.
 *       Must be paired with bk_hspl_driver_exit_critical().
 *
 * @return Interrupt state flags (must be passed to exit_critical)
 */
uint32_t bk_hspl_driver_enter_critical(void);

/**
 * @brief Exit critical section for driver (release HSPL lock and restore IRQ)
 *
 * @param flags Interrupt state flags returned by bk_hspl_driver_enter_critical()
 */
void bk_hspl_driver_exit_critical(uint32_t flags);

/**
 * @brief Enter critical section for UART log
 *
 * @note This function disables interrupts and acquires the UART_LOG resource lock.
 *       Must be paired with bk_hspl_uart_log_unlock().
 */
void bk_hspl_uart_log_lock(void);

/**
 * @brief Unlock UART log (release UART_LOG lock)
 */
void bk_hspl_uart_log_unlock(void);

/**
 * @brief Enter critical section for UART log
 *
 * @note This function disables interrupts and acquires the UART_LOG resource lock.
 *       Must be paired with bk_hspl_uart_log_exit_critical().
 */
uint32_t bk_hspl_uart_log_enter_critical(void);

/**
 * @brief Exit critical section for UART log
 *
 * @param flags Interrupt state flags returned by bk_hspl_uart_log_enter_critical()
 */
void bk_hspl_uart_log_exit_critical(uint32_t flags);

/**
 * @brief Lock SYS_SW_REGS (acquire SYS_SW_REGS lock)
 */
void bk_hspl_sys_sw_regs_lock(void);

/**
 * @brief Unlock SYS_SW_REGS (release SYS_SW_REGS lock)
 */
void bk_hspl_sys_sw_regs_unlock(void);

#ifdef __cplusplus
}
#endif
