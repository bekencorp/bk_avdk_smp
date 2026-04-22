// Copyright 2020-2025 Beken
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

/*
 * Public IPI driver API header for CP side.
 * Keep consistent with other drivers' <driver/xxx.h> include style.
 *
 * Note: keep ALL public declarations here. Headers under
 * `cp/middleware/driver/ipi/` are treated as private/internal.
 */

#include <common/bk_include.h>
#include <soc/soc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	IPI_CP_CORE0 = 0,
	IPI_CP_CORE1 = 1,
	IPI_AP_CORE0 = 2,
	IPI_AP_CORE1 = 3,
	IPI_DSP_CORE = 4,
	IPI_CORE_MAX
} ipi_core_id_t;

/**
 * @brief IPI interrupt callback type
 *
 * @param core_id Core/channel ID whose IPI interrupt is pending
 * @param value   Payload value carried by the IPI (driver-specific encoding may apply)
 * @param param   User argument registered with bk_ipi_register_callback()
 */
typedef void (*ipi_callback_t)(ipi_core_id_t core_id, uint32_t value, void *param);

/**
 * @brief Initialize IPI driver and register ISR
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_driver_init(void);

/**
 * @brief Deinitialize IPI driver
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_driver_deinit(void);

/**
 * @brief Send an IPI interrupt to target core/channel
 *
 * @param core_id Target IPI core/channel ID
 * @param value   Payload value to send
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_send(ipi_core_id_t core_id, uint32_t value);

/**
 * @brief Get pending status for one IPI core/channel
 *
 * @param core_id IPI core/channel ID to query
 *
 * @return 1 if pending, 0 if clear/invalid/not initialized
 */
uint32_t bk_ipi_get_status(ipi_core_id_t core_id);

/**
 * @brief Get pending status bitmap for all IPI cores/channels
 *
 * @return Bitmask where bit0..bit4 correspond to IPI_CP_CORE0..IPI_DSP_CORE
 */
uint32_t bk_ipi_get_all_status(void);

/**
 * @brief Clear pending IPI interrupt for the specified core/channel
 *
 * @param core_id IPI core/channel ID to clear
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_clear(ipi_core_id_t core_id);

/**
 * @brief Enable IPI interrupt for the specified core/channel
 *
 * @param core_id IPI core/channel ID to enable
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_enable(ipi_core_id_t core_id);

/**
 * @brief Disable IPI interrupt for the specified core/channel
 *
 * @param core_id IPI core/channel ID to disable
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_disable(ipi_core_id_t core_id);

/**
 * @brief Register callback for the specified core/channel
 *
 * @param core_id  IPI core/channel ID
 * @param callback Callback function (NULL is allowed to clear)
 * @param param    User argument passed back to callback
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_register_callback(ipi_core_id_t core_id, ipi_callback_t callback, void *param);

/**
 * @brief Unregister callback for the specified core/channel
 *
 * @param core_id IPI core/channel ID
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_unregister_callback(ipi_core_id_t core_id);

/**
 * @brief Get IPI device status register value
 *
 * @return Device status register value, or 0 if not initialized
 */
uint32_t bk_ipi_get_device_status(void);

/**
 * @brief Dump driver state and registered callbacks (for debugging)
 */
#if CONFIG_IPI_DUMP
void bk_ipi_dump_info(void);
#endif

#ifdef __cplusplus
}
#endif

