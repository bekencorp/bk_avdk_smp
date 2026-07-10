// Copyright 2022-2023 Beken
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

#include <soc/soc.h>

#define PM_HAL_LOGD HAL_LOGD
#define PM_HAL_LOGV HAL_LOGV

void sys_hal_lv_deep_sleep_enter_clear(void);
void sys_hal_lv_deep_sleep_enter_set(void);

#if CONFIG_PM_CLOCK_VOTE_RECORD
void sys_hal_pm_clock_vote_record(uint32_t module, uint32_t clock_state, uint32_t return_address);
#endif

#if CONFIG_PM_POWER_VOTE_RECORD
void sys_hal_pm_power_vote_record(uint32_t module, uint32_t power_state, uint32_t return_address, uint32_t filter_domain);
#endif

#if CONFIG_DEEP_LV_DEBUG_LOG
void sys_hal_lv_aon_snap_pre_record(void);
void sys_hal_lv_aon_snap_at_sleep_record(void);
void sys_hal_lv_aon_snap_post_record(void);
void sys_hal_lv_aon_ldo_record(uint32_t pre, uint32_t sleep_cfg, uint32_t after_sleep_set,
	uint32_t after_ramp, uint32_t backup, uint32_t after_ana_restore, uint32_t final);
void sys_hal_lv_aon_debug_flush(void);
#endif
