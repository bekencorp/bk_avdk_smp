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
	BK_HSPL_RES_USER1 = 30,
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
 * @brief Lock a resource using HSPL with busy-wait (can be used in interrupt context)
 *
 * @note This function will spin until the lock is acquired. It can be used in
 *       interrupt context but may cause system hang if the lock is held for too long.
 *       Use with caution and ensure the lock holder releases it quickly.
 *
 * @param res Resource id
 * @return BK_OK on success, BK_ERR_PARAM on invalid parameter
 */
bk_err_t bk_hspl_res_must_lock(bk_hspl_res_t res);

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
