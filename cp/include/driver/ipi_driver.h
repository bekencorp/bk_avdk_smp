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
 * @brief Software domains multiplexed over one target-core IPI channel.
 *
 * The IPI hardware provides one pending/value register per target core, not a
 * FIFO or a set of independent logical channels. Domain encoding lets the
 * common IPI ISR dispatch one hardware interrupt source to multiple software
 * users without those users overwriting each other's per-core callback.
 *
 * Encoded domain IPI values use this layout:
 *   bits 30..27: source CPU ID
 *   bits 26..24: software domain
 *   bits 23..16: domain-specific event ID
 *   bits 15..0 : domain-specific compact payload
 *
 * Domain 0 is reserved as an invalid domain, so every public IPI user must use
 * an explicitly assigned domain. HEARTBEAT is for AP/CP liveness notifications,
 * SMP is for cross-core scheduler/OS kicks, USB is reserved for USB/RISC-V
 * bridge notifications, and TEST is kept for CLI/debug traffic.
 */
typedef enum {
	IPI_DOMAIN_RESERVED = 0,
	IPI_DOMAIN_HEARTBEAT = 1,
	IPI_DOMAIN_SMP = 2,
	IPI_DOMAIN_USB = 3,
	IPI_DOMAIN_CP_HANG_DEBUG = 4,
	IPI_DOMAIN_TEST = 7,
	IPI_DOMAIN_MAX = 8
} ipi_domain_t;

#define IPI_VALUE_PAYLOAD_MASK  0x0000FFFFU
#define IPI_VALUE_EVENT_MASK    0x00FF0000U
#define IPI_VALUE_DOMAIN_MASK   0x07000000U
#define IPI_VALUE_SRC_MASK      0x78000000U
#define IPI_VALUE_EVENT_POS     16
#define IPI_VALUE_DOMAIN_POS    24
#define IPI_VALUE_SRC_POS       27

#define IPI_VALUE_MAKE(src, domain, event, payload) \
	((((uint32_t)(src) << IPI_VALUE_SRC_POS) & IPI_VALUE_SRC_MASK) | \
	 (((uint32_t)(domain) << IPI_VALUE_DOMAIN_POS) & IPI_VALUE_DOMAIN_MASK) | \
	 (((uint32_t)(event) << IPI_VALUE_EVENT_POS) & IPI_VALUE_EVENT_MASK) | \
	 ((uint32_t)(payload) & IPI_VALUE_PAYLOAD_MASK))

#define IPI_VALUE_GET_SRC(value)     (((value) & IPI_VALUE_SRC_MASK) >> IPI_VALUE_SRC_POS)
#define IPI_VALUE_GET_DOMAIN(value)  (((value) & IPI_VALUE_DOMAIN_MASK) >> IPI_VALUE_DOMAIN_POS)
#define IPI_VALUE_GET_EVENT(value)   (((value) & IPI_VALUE_EVENT_MASK) >> IPI_VALUE_EVENT_POS)
#define IPI_VALUE_GET_PAYLOAD(value) ((value) & IPI_VALUE_PAYLOAD_MASK)

/**
 * @brief Domain-level IPI callback.
 *
 * IPI is a doorbell, not a FIFO. Domain users should keep durable state in
 * shared memory/bitmaps and use the IPI value only as a compact notification.
 */
typedef void (*ipi_domain_callback_t)(ipi_core_id_t core_id, uint32_t value,
	uint8_t src_cpu, uint8_t event, uint16_t payload, void *param);

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
 * @brief Send an encoded domain IPI interrupt to target core/channel
 *
 * The current CPU ID is encoded as the source field.
 *
 * @param core_id Target IPI core/channel ID
 * @param domain  Domain identifier, see ipi_domain_t
 * @param event   Domain-specific event ID
 * @param payload Domain-specific compact payload
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_send_domain(ipi_core_id_t core_id, ipi_domain_t domain, uint8_t event, uint16_t payload);

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
 * @brief Register a shared domain callback
 *
 * Only one owner can register each domain. Use domain callbacks for shared IPI
 * use cases such as heartbeat and SMP kicks, so multiple users do not overwrite
 * each other.
 *
 * @param domain   Domain identifier, see ipi_domain_t
 * @param callback Callback function
 * @param param    User argument passed back to callback
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_register_domain_callback(ipi_domain_t domain, ipi_domain_callback_t callback, void *param);

/**
 * @brief Unregister a shared domain callback
 *
 * @param domain Domain identifier, see ipi_domain_t
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
bk_err_t bk_ipi_unregister_domain_callback(ipi_domain_t domain);

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

