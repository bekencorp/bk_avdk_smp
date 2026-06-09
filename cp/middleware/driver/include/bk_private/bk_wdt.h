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

#include <common/bk_include.h>
#include <common/bk_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WDT_DEV_NAME                "wdt"

void bk_int_wdt_feed(void);
bk_err_t bk_task_wdt_driver_init(void);
bk_err_t bk_task_wdt_driver_deinit(void);
void bk_task_wdt_start(void);
__attribute__((section(".itcm_sec_code")))void bk_task_wdt_stop(void);
bk_err_t bk_task_wdt_set_feed_bits(uint32_t core_id, bool set_flag);
void bk_task_wdt_feed(void);
void bk_task_wdt_timeout_check(void);
void bk_task_wdt_systick_check(void);
void bk_wdt_feed_handle(void);

#if CONFIG_TASK_WDT_TEST
bk_err_t bk_task_wdt_set_skip_feed_core(uint32_t core_id, bool skip);
uint32_t bk_task_wdt_get_feed_bits(void);
uint32_t bk_task_wdt_get_skip_feed_bits(void);
uint64_t bk_task_wdt_get_last_feed_tick(uint32_t core_id);
#endif

#ifdef __cplusplus
}
#endif
