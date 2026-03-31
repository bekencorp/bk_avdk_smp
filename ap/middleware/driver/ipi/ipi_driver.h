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

#include <common/bk_include.h>
#include <soc/soc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPI Core ID enumeration
 */
typedef enum {
	IPI_CP_CORE0 = 0,  /**< CPU Core 0 */
	IPI_CP_CORE1 = 1,  /**< CPU Core 1 */
	IPI_AP_CORE0 = 2,  /**< CPU Core 2 */
	IPI_AP_CORE1 = 3,  /**< CPU Core 3 */
	IPI_DSP_CORE = 4,  /**< DSP Core (CPU4) */
	IPI_CORE_MAX       /**< Maximum core number */
} ipi_core_id_t;

/**
 * @brief IPI callback function type
 * @param core_id The core ID that received the interrupt
 * @param value The value field from IPIG register (31:1 bits)
 * @param param User parameter
 */
typedef void (*ipi_callback_t)(ipi_core_id_t core_id, uint32_t value, void *param);

/**
 * @brief Initialize IPI driver
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_driver_init(void);

/**
 * @brief Deinitialize IPI driver
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_driver_deinit(void);

/**
 * @brief Send IPI interrupt to specified core
 * @param core_id Target core ID (IPI_CP_CORE0, IPI_CP_CORE1, IPI_AP_CORE0, IPI_AP_CORE1, IPI_DSP_CORE)
 * @param value Value to send (will be stored in bits 31:1 of IPIG register)
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_send(ipi_core_id_t core_id, uint32_t value);

/**
 * @brief Get IPI interrupt status for specified core
 * @param core_id Core ID to check (IPI_CP_CORE0, IPI_CP_CORE1, IPI_AP_CORE0, IPI_AP_CORE1, IPI_DSP_CORE)
 * @return 1 if interrupt is pending, 0 if no interrupt
 */
uint32_t bk_ipi_get_status(ipi_core_id_t core_id);

/**
 * @brief Get IPI interrupt status for all cores
 * @return Bitmask where bit 0-4 correspond to core 0-4 interrupt status
 */
uint32_t bk_ipi_get_all_status(void);

/**
 * @brief Clear IPI interrupt for specified core
 * @param core_id Core ID to clear interrupt for (IPI_CP_CORE0, IPI_CP_CORE1, IPI_AP_CORE0, IPI_AP_CORE1, IPI_DSP_CORE)
 * @return BK_OK on success, BK_FAIL on failure
 * @note This function reads the current IPIG value and writes it to IPIC to clear
 */
bk_err_t bk_ipi_clear(ipi_core_id_t core_id);

/**
 * @brief Enable IPI interrupt for specified core
 * @param core_id Core ID to enable interrupt for (IPI_CP_CORE0, IPI_CP_CORE1, IPI_AP_CORE0, IPI_AP_CORE1, IPI_DSP_CORE)
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_enable(ipi_core_id_t core_id);

/**
 * @brief Disable IPI interrupt for specified core
 * @param core_id Core ID to disable interrupt for (IPI_CP_CORE0, IPI_CP_CORE1, IPI_AP_CORE0, IPI_AP_CORE1, IPI_DSP_CORE)
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_disable(ipi_core_id_t core_id);

/**
 * @brief Register callback function for IPI interrupt
 * @param core_id Core ID to register callback for
 * @param callback Callback function pointer
 * @param param User parameter to pass to callback
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_register_callback(ipi_core_id_t core_id, ipi_callback_t callback, void *param);

/**
 * @brief Unregister callback function for IPI interrupt
 * @param core_id Core ID to unregister callback for
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_unregister_callback(ipi_core_id_t core_id);

/**
 * @brief Get device status
 * @return Device status value
 */
uint32_t bk_ipi_get_device_status(void);

/**
 * @brief Dump IPI callbacks information
 * @note This function prints all registered callback information for debugging
 */
void bk_ipi_dump_info(void);

/**
 * @brief IPI interrupt handler (called from ISR)
 * @note This function should be called from the IPI interrupt service routine
 */
void bk_ipi_isr_dispatch(void);

#ifdef __cplusplus
}
#endif

