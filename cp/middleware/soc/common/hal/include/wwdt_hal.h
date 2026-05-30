// Copyright 2020-2024 Beken
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

#include "hal_config.h"
#include "wwdt_hw.h"
#include "wwdt_ll.h"
#include <driver/hal/hal_wwdt_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CPU_WWDT_ID = 0,  /**< CPU_WWDT_ID */
	CPU_WWDT_INVALID_ID = 0xFF
} wwdt_id_t;

typedef struct {
	wwdt_hw_t *hw;
	wwdt_unit_t id;
}wwdt_hal_t;

#define WWDT_CLK_SRC              (32 * 1000)
#define MULTI_VAL_MS              (WWDT_CLK_SRC / 1000)

#define wwdt_hal_reset_config_to_default(hal) wwdt_ll_reset_config_to_default((hal)->hw)
#define wwdt_hal_set_smb_clkrst_clkg_bypass(val) wwdt_ll_set_smb_clkrst_clkg_bypass(val)
#define wwdt_hal_set_smb_clkrst_soft_reset(val)  wwdt_ll_set_smb_clkrst_soft_reset(val)
#define wwdt_hal_1st_set_wdt_config_period(val)  wwdt_ll_1st_set_wdt_config_period(val)
#define wwdt_hal_2nd_set_wdt_config_period(val)  wwdt_ll_2nd_set_wdt_config_period(val)
#define wwdt_hal_set_wdt_win_set_win_val(val)    wwdt_ll_set_wdt_win_set_win_val(val)
#define wwdt_hal_set_wdt_win_1st_set_win_val(val) wwdt_ll_set_wdt_win_1st_set_win_val(val)
#define wwdt_hal_set_wdt_win_2nd_set_win_val(val) wwdt_ll_set_wdt_win_2nd_set_win_val(val)
#define wwdt_hal_get_wdt_win_get_win_val()        wwdt_ll_get_wdt_win_set_win_val()
#define wwdt_hal_get_cpuid_magic_word()           wwdt_ll_get_cpuid_magic_word()
#define wwdt_hal_get_cpuid_cpu_id()               wwdt_ll_get_cpuid_cpu_id()
#define wwdt_hal_set_wdt_win_set_win_en(val)      wwdt_ll_set_wdt_win_set_win_en(val)

bk_err_t wwdt_hal_init(wwdt_hal_t *hal);
bk_err_t wwdt_hal_init_wwdt(wwdt_hal_t *hal, uint32_t timeout);
void wwdt_hal_close(void);
void wwdt_hal_force_feed(void);
void wwdt_hal_force_reboot(void);
uint32_t wwdt_hal_get_cpu_id(void);

#if CFG_HAL_DEBUG_WWDT
void wwdt_struct_dump(uint32_t start, uint32_t end);
#else
#define wwdt_struct_dump(start, end)
#endif

#ifdef __cplusplus
}
#endif

