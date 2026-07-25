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

#ifdef __cplusplus
extern "C" {
#endif

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
bk_err_t bk_cpu_hp_offline_direct(uint32_t cpu_id);
bk_err_t bk_cpu_hp_online_direct(uint32_t cpu_id);
uint32_t bk_cpu_hp_enter_primary(void);
void bk_cpu_hp_exit_primary(uint32_t old_core_id);

void bk_cpu_hp_core_stop_hmb_isr(void);
void bk_cpu_hp_idle_handler(void);

#if CONFIG_CPU_HP_GOVERNOR

/*
 * Dynamic CPU load governor for the CP domain (CPU0 primary + CPU1 hotplug).
 *
 * Policy (see cpu_hp_governor.c for the full state machine):
 *   - online  CPU1 when CPU0 alone is saturated (load0 > UP_THRESHOLD) for a
 *     sustained window;
 *   - offline CPU1 when the combined work of both cores fits comfortably into a
 *     single core (load0 + load1 < DOWN_THRESHOLD) for a sustained window.
 *
 * Anti-flapping: asymmetric thresholds + per-direction debounce + a post-switch
 * cooldown. All hotplug actions run from a normal task context; the underlying
 * bk_cpu_hp_*() helpers migrate the work to the primary core internally.
 */

typedef struct {
	uint32_t enabled;       /* auto-adjust running                  */
	uint32_t load0;         /* smoothed load of CPU0 in percent     */
	uint32_t load1;         /* smoothed load of CPU1 in percent     */
	uint32_t cpu1_online;   /* 1 if CPU1 currently online           */
	uint32_t up_cnt;        /* consecutive "overloaded" samples     */
	uint32_t down_cnt;      /* consecutive "underloaded" samples    */
	uint32_t online_cnt;    /* number of auto online transitions    */
	uint32_t offline_cnt;   /* number of auto offline transitions   */
} bk_cpu_hp_governor_status_t;

bk_err_t bk_cpu_hp_governor_init(void);

bk_err_t bk_cpu_hp_governor_start(void);
bk_err_t bk_cpu_hp_governor_stop(void);

/* Snapshot the current governor status (loads, counters, state). */
void bk_cpu_hp_governor_get_status(bk_cpu_hp_governor_status_t *status);

#endif /* CONFIG_CPU_HP_GOVERNOR */

#if CONFIG_CPU_HP_VOTE

typedef struct cpu_hp_voter *cpu_hp_voter_handle_t;

/**
 * @brief Join the vote.
 *
 * @param name voter name (required, must not be NULL).
 * @return cpu_hp_voter_handle_t NULL if name is NULL or the voter table is full.
 */
cpu_hp_voter_handle_t bk_cpu_hp_vote_register(const char *name);
/**
 * @brief Leave the vote (== voting offline). May trigger power-down; returns its result.
 * 
 * @param voter voter handle
 * @return bk_err_t BK_OK if the vote is unregistered successfully, otherwise an error code.
 */
bk_err_t bk_cpu_hp_vote_unregister(cpu_hp_voter_handle_t voter);

bk_err_t bk_cpu_hp_vote_online(cpu_hp_voter_handle_t voter);
bk_err_t bk_cpu_hp_vote_offline(cpu_hp_voter_handle_t voter);

/**
 * @brief Get the wanted state of a voter.
 * 
 * @param voter voter handle
 * @param wanted pointer to the wanted state
 * @return bk_err_t BK_OK if the wanted state is retrieved successfully, otherwise an error code.
 */
bk_err_t bk_cpu_hp_vote_get_wanted(cpu_hp_voter_handle_t voter, uint32_t *wanted);
uint32_t bk_cpu_hp_vote_get_online_count(void);
uint32_t bk_cpu_hp_vote_get_voter_count(void);

#if CONFIG_CPU_HP_VOTE_FIND
cpu_hp_voter_handle_t bk_cpu_hp_vote_find(const char *name);
#endif /* CONFIG_CPU_HP_VOTE_FIND */

#if CONFIG_CPU_HP_VOTE_DUMP
void bk_cpu_hp_vote_dump(void);
#endif /* CONFIG_CPU_HP_VOTE_DUMP */

#endif /* CONFIG_CPU_HP_VOTE */

#endif /* CONFIG_CPU_HOTPLUG */

#ifdef __cplusplus
}
#endif
