// Copyright 2020-2024 Beken
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
#include <driver/wwdt_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief WWDT API
 * @defgroup bk_api_wwdt WWDT API group
 * @{
 */

/**
 * @brief     Init the WWDT driver
 *
 * This API init the resoure common:
 *   - Init WWDT driver control memory
 *
 * @attention 1. This API should be called before any other WWDT APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_wwdt_driver_init(void);

/**
 * @brief     Deinit the WWDT driver
 *
 * This API free all resource related to WWDT and power down WWDT.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_wwdt_driver_deinit(void);

/**
 * @brief     Start the WWDT
 *
 * This API start the WWDT:
 *  - Power up the WWDT
 *  - Init the watch dog timer period
 *
 * @param timeout_ms: WWDT period
 * @param is_enable_window: WWDT enable window feature
 * @param window_val: window period
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_WWDT_DRIVER_NOT_INIT: WWDT driver not init
 *    - BK_ERR_WWDT_INVALID_PERIOD: WWDT invalid period
 *    - others: other errors.
 */
bk_err_t bk_wwdt_start(uint32_t timeout_ms, bool is_enable_window, uint32_t window_val);

/**
 * @brief     Stop the WWDT
 *
 * This API stop the WWDT:
 *   - Reset all configuration of WWDT to default value
 *   - Power down the WWDT
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
__attribute__((section(".itcm_sec_code"))) bk_err_t bk_wwdt_stop(void);

/**
 * @brief     Feed the WWDT
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_wwdt_feed(void);

/**
 * @brief   Get feed watchdog time
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
uint32_t bk_wwdt_get_feed_time(void);

/**
 * @brief  set feed watchdog time
 *
 * @return
 *    - NULL
 *
 */
void bk_wwdt_set_feed_time(uint32_t dw_set_time);

/**
 * @brief   Get wwdt driver init flag
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bool bk_wwdt_is_driver_inited(void);

/**
 * @brief   Get cpu id
 *
 * @return
 *    - uint32_t: id
 *    - 0xFF: invalid index value
 */
uint32_t bk_wwdt_get_cpu_id(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

