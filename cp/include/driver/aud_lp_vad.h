// Copyright 2026 Beken
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

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*lp_vad_isr_t)(void);

/**
 * @brief Initialize LP VAD hardware
 *
 * Configure VAD analog registers (ana_reg23, ana_reg24).
 * Must be called before any other LP VAD APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_lp_vad_init(void);

/**
 * @brief Deinitialize LP VAD hardware
 *
 * Disable VAD interrupt, unregister ISR and reset internal state.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_DRV_NOT_INIT: not initialized
 */
bk_err_t bk_lp_vad_deinit(void);

/**
 * @brief Configure LP VAD parameters before entering low-voltage sleep
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_lp_vad_set_sleep_para_before_sleep(void);

/**
 * @brief Configure low-power parameters and enter deepsleep with VAD wakeup
 *
 * Set LDO voltages, power down peripherals, configure wakeup source,
 * adjust analog clocks and enter ARM sleep-deep mode (SCB SLEEPDEEP).
 * The system will be woken up by VAD interrupt when MIC detects sound.
 *
 * @attention bk_lp_vad_init() and bk_lp_vad_register_isr() must be called first.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_DRV_NOT_INIT: not initialized
 */
bk_err_t bk_lp_vad_deepsleep_enter(void);

/**
 * @brief Register VAD wakeup interrupt callback
 *
 * Register the ISR that will be called when VAD detects voice activity
 * during low-power sleep.
 *
 * @param isr  Callback function invoked on VAD interrupt (must not be NULL)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_DRV_NOT_INIT: not initialized
 */
bk_err_t bk_lp_vad_register_isr(lp_vad_isr_t isr);

#ifdef __cplusplus
}
#endif

