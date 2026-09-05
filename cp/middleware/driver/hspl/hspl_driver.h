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

#include <common/bk_include.h>
#include <soc/soc.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief HSPL channel count
 */
#define HSPL_CHANNEL_MAX (16)

/**
 * @brief HSPL unlock magic value (write to LOCK register to release lock)
 */
#define HSPL_UNLOCK_MAGIC (0xA55A80AF)

/**
 * @brief HSPL lock state info
 */
typedef struct {
	uint8_t locked;      /**< 1: channel is locked, 0: unlocked */
	uint8_t owner_valid; /**< 1: owner_id valid, 0: no owner */
	uint8_t owner_id;    /**< 0..15 */
} hspl_state_t;

/**
 * @brief HSPL HW IDs (two blocks, different base addresses)
 *
 * BK7259 has two equivalent HSPL blocks:
 * - HSPL_ID_0: base 0x45010000 (M52-side mapping)
 * - HSPL_ID_1: base 0x480C0000 (M55-side mapping)
 */
typedef enum {
	BK_HSPL_ID_0 = 0, /**< base: 0x45010000 */
	BK_HSPL_ID_1 = 1, /**< base: 0x480C0000 */
	BK_HSPL_ID_MAX,
} bk_hspl_id_t;

/**
 * @brief HSPL timeout callback
 * @param channel Monitored channel that triggers timeout (0..15)
 * @param param User parameter
 */
typedef void (*hspl_timeout_callback_t)(uint8_t channel, void *param);

/**
 * @brief Initialize HSPL driver
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_hspl_driver_init(void);

/**
 * @brief Deinitialize HSPL driver
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_hspl_driver_deinit(void);

#if CONFIG_DEEP_LV
/**
 * @brief Reinitialize HSPL0 hardware after CP Deep-LV wakeup
 *
 * Deep-LV retains the driver software state but resets HSPL0 registers.
 *
 * @return BK_OK on success, BK_FAIL otherwise
 */
bk_err_t bk_hspl_deep_lv_resume_reinit(void);
#endif

/**
 * @brief Try to lock a HSPL channel by reading its LOCK register
 *
 * Hardware behavior:
 * - If read returns 0x1, lock succeeds (unlock -> lock).
 * - If read returns 6'b1xxxx0 (bit5=1, bit0=0), the channel is already locked,
 *   and bits[4:1] indicate owner core_id.
 *
 * @param channel 0..15
 * @param owner_id Optional output owner ID when lock fails (can be NULL)
 * @return BK_OK if lock succeeds, BK_FAIL otherwise
 */
bk_err_t bk_hspl_try_lock(bk_hspl_id_t hspl_id, uint8_t channel, uint8_t *owner_id);

/**
 * @brief Unlock a HSPL channel by writing HSPL_UNLOCK_MAGIC to its LOCK register
 * @param channel 0..15
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_hspl_unlock(bk_hspl_id_t hspl_id, uint8_t channel);

/**
 * @brief Get HSPL channel state from STA register (read-only helper register)
 * @param channel 0..15
 * @param state Output state (must not be NULL)
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_hspl_get_state(bk_hspl_id_t hspl_id, uint8_t channel, hspl_state_t *state);

/**
 * @brief Read raw LOCK register value (note: reading LOCK triggers lock attempt)
 * @param channel 0..15
 * @return Raw register value
 */
uint32_t bk_hspl_read_lock_raw(bk_hspl_id_t hspl_id, uint8_t channel);

/**
 * @brief Read raw STA register value (does not change LOCK state)
 * @param channel 0..15
 * @return Raw register value
 */
uint32_t bk_hspl_read_sta_raw(bk_hspl_id_t hspl_id, uint8_t channel);

/**
 * @brief Configure timeout monitor (for debug)
 *
 * Mapping per BK7259 HSPL tests:
 * - timeout_threshold: bus clock cycles (written to bits[19:0])
 * - hspl_sel: channel select (written to bits[23:20])
 * - timeout_en: enable (bit24)
 *
 * @param channel 0..15
 * @param threshold_cycles Timeout threshold in bus clock cycles (0 disables threshold)
 * @param enable 0/1
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_hspl_timeout_config(bk_hspl_id_t hspl_id, uint8_t channel, uint32_t threshold_cycles, bool enable);

/**
 * @brief Enable/disable timeout interrupt generation (for debug)
 * @param enable 0/1
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_hspl_timeout_irq_enable(bk_hspl_id_t hspl_id, bool enable);

/**
 * @brief Clear timeout interrupt pending (for debug)
 */
void bk_hspl_timeout_irq_clear(bk_hspl_id_t hspl_id);

/**
 * @brief Register timeout callback (called from ISR context)
 * @param cb Callback (can be NULL)
 * @param param User param
 * @return BK_OK on success
 */
bk_err_t bk_hspl_register_timeout_callback(bk_hspl_id_t hspl_id, hspl_timeout_callback_t cb, void *param);

/**
 * @brief HSPL interrupt handler dispatch
 * @note Registered by bk_hspl_driver_init()
 */
void bk_hspl_isr_dispatch(void);

#ifdef __cplusplus
}
#endif

