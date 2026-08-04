// Copyright     2023-2028 Beken
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
#include <driver/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif


/* @brief Overview about this anti-tamper API header
 *
 */

/**
 * @brief anti-tamper API
 * @defgroup bk_api_anti_tamper anti-tamper API group
 * @{
 */

/**
 * @brief     Enable anti-tamper
 *
 * This API enables anti-tamper GPIO ports
 *
 * @param tx_port anti-tamper tx GPIO port
 * @param rx_port anti-tamper tx GPIO port
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_anti_tamper_enable(gpio_id_t tx_port, gpio_id_t rx_port);

/**
 * @brief     Disable anti-tamper
 *
 * This API disables anti-tamper
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_anti_tamper_disable(void);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


