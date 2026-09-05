// Copyright 2020-2021 Beken
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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * flash_core port: the environment-specific hooks the portable flash core needs.
 *
 * The core itself never references OS / sys_drv / PPC / HSPL / WDT. Each consumer
 * (CP/AP middleware, TF-M, aboot bootloader) supplies these callbacks so the
 * same erase/write algorithm runs correctly under the local concurrency model:
 *
 *   - middleware (NS)  : enter_critical = HSPL lock + rtos_disable_int
 *                        op_prepare/op_finish = mailbox peripheral notify
 *   - TF-M   (secure)  : enter_critical = NULL (single-threaded; PPC one layer up)
 *                        op_prepare/op_finish = NULL
 *   - bootloader       : enter_critical = NULL, op_prepare/op_finish = NULL
 *                        op_progress = watchdog feed for long range erases
 *
 * enter_critical/exit_critical MUST be re-entrant: the core nests an inner
 * critical section (per 32-byte HAL op) inside the outer op lock, matching the
 * historical flash_enter_critical() nesting.
 */
typedef struct {
	uint32_t (*enter_critical)(void);        /**< returns an opaque int_level; NULL => nop */
	void     (*exit_critical)(uint32_t int_level); /**< consumes int_level; NULL => nop */
	void     (*op_prepare)(void);            /**< optional peripheral notify; may be NULL */
	void     (*op_finish)(void);             /**< optional peripheral notify; may be NULL */
	void     (*op_progress)(void);           /**< optional per-sub-op progress (e.g. WDT feed); may be NULL */
} flash_core_port_t;

#ifdef __cplusplus
}
#endif
