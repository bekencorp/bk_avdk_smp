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

#include <components/log.h>
#include <driver/timer.h>

#define TIMER_TAG "timer"
#define TIMER_LOGI(...) BK_LOGI(TIMER_TAG, ##__VA_ARGS__)
#define TIMER_LOGW(...) BK_LOGW(TIMER_TAG, ##__VA_ARGS__)
#define TIMER_LOGE(...) BK_LOGE(TIMER_TAG, ##__VA_ARGS__)
#define TIMER_LOGD(...) BK_LOGD(TIMER_TAG, ##__VA_ARGS__)
#define TIMER_LOGV(...) BK_LOGV(TIMER_TAG, ##__VA_ARGS__)

uint32_t bk_timer_get_clock_src(timer_id_t timer_id);
void bk_timer_clock_select(timer_id_t id, timer_src_clk_t mode);
void bk_timer_delay_us(uint32_t us);
// eof
