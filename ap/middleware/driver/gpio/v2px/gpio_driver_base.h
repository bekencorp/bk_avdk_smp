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

#define GPIO_TAG "gpio"
#define GPIO_LOGI(...) BK_LOGI(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGW(...) BK_LOGW(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGE(...) BK_LOGE(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGD(...) BK_LOGD(GPIO_TAG, ##__VA_ARGS__)
#define GPIO_LOGV(...) BK_LOGV(GPIO_TAG, ##__VA_ARGS__)

bk_err_t bk_gpio_set_gpio_func(uint32_t gpio_id, IOMX_CODE_T func_code);
uint32_t bk_gpio_get_gpio_func_code(uint32_t gpio_id);
