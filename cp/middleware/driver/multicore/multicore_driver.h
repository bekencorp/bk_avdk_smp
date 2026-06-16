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

#include <components/log.h>
#include "cpu_id.h"

#define MULTICORE_TAG "multicore"
#define MULTICORE_LOGI(...) BK_LOGI(MULTICORE_TAG, ##__VA_ARGS__)
#define MULTICORE_LOGW(...) BK_LOGW(MULTICORE_TAG, ##__VA_ARGS__)
#define MULTICORE_LOGE(...) BK_LOGE(MULTICORE_TAG, ##__VA_ARGS__)
#define MULTICORE_LOGD(...) BK_LOGD(MULTICORE_TAG, ##__VA_ARGS__)

void bk_multicore_set_cpu_id(uint32_t cpu_id);
uint32_t bk_multicore_get_cpu_id(void);
bk_err_t bk_multicore_start(uint32_t cpu_id);
bk_err_t bk_multicore_reset(uint32_t cpu_id);
bk_err_t bk_multicore_stop(uint32_t cpu_id);

#if CONFIG_CPU_HOTPLUG
typedef enum {
	BK_CPU_HP_STATE_ONLINE = 0,
	BK_CPU_HP_STATE_OFFLINE_REQUESTED,
	BK_CPU_HP_STATE_SCHEDULER_DRAINING,
	BK_CPU_HP_STATE_IRQ_MIGRATING,
	BK_CPU_HP_STATE_QUIESCE,
	BK_CPU_HP_STATE_RESET_HOLD,
	BK_CPU_HP_STATE_POWER_OFF,
	BK_CPU_HP_STATE_OFFLINE,
	BK_CPU_HP_STATE_POWER_ON,
	BK_CPU_HP_STATE_BOOT_PREPARE,
	BK_CPU_HP_STATE_RESET_RELEASE,
	BK_CPU_HP_STATE_SECONDARY_BOOT,
	BK_CPU_HP_STATE_JOIN_SCHEDULER,
} bk_cpu_hp_state_t;

uint32_t bk_cpu_hp_is_online(uint32_t cpu_id);
uint32_t bk_cpu_hp_is_active(uint32_t cpu_id);
bk_cpu_hp_state_t bk_cpu_hp_get_state(uint32_t cpu_id);
const char *bk_cpu_hp_get_state_name(uint32_t cpu_id);
uint32_t bk_cpu_hp_get_domain_possible_mask(uint32_t cpu_id);
uint32_t bk_cpu_hp_get_domain_online_mask(uint32_t cpu_id);
uint32_t bk_cpu_hp_get_domain_active_mask(uint32_t cpu_id);
uint32_t bk_cpu_hp_get_domain_dying_mask(uint32_t cpu_id);
uint32_t bk_cpu_hp_get_domain_offline_mask(uint32_t cpu_id);

bk_err_t bk_cpu_hp_offline(uint32_t cpu_id);
bk_err_t bk_cpu_hp_online(uint32_t cpu_id);
uint32_t bk_cpu_hp_enter_primary(void);
void bk_cpu_hp_exit_primary(uint32_t old_core_id);

void bk_cpu_hp_core_online(void);
void bk_cpu_hp_core_stop_hmb_isr(void);
void bk_cpu_hp_idle_handler(void);

#endif /* CONFIG_CPU_HOTPLUG */
