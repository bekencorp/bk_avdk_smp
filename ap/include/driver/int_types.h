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
//
#pragma once
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <driver/hal/hal_int_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_INT_DEVICE_NONE                  (BK_ERR_INT_BASE - 1) /**< icu device number is invalid */
#define BK_ERR_INT_NOT_EXIST                    (BK_ERR_INT_BASE - 2) /**< icu device number is invalid */

#include "int_types_impl.h"

typedef void (*int_mac_ps_callback_t)(uint32_t status);

#ifdef __cplusplus
}
#endif
