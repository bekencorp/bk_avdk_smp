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