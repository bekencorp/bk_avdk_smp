// Copyright 2023-2024 Beken
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

#include "sdkconfig.h"
#include "deep_lv_reserve.h"
#include "reg_base.h"
#include "soc/soc.h"
#include "deep_lv_reserve_layout.h"
#include <assert.h>
#include <stdint.h>

#if CONFIG_DEEP_LV && CONFIG_SPE

#define DEEP_LV_RESERVE_MAGIC             0x444C5652u /* "DLVR" */

typedef struct {
	uint32_t magic;
	uint32_t values[CP_MEM_CHECK_POINT_NUM];
	uint32_t point_mask;
} deep_lv_reserve_t;

#if CONFIG_SOC_SMP
extern uint32_t __StackLimitCore1;
#define DEEP_LV_RESERVE_BASE          ((uint32_t)&__StackLimitCore1)
#else
extern uint32_t __StackLimit;
#define DEEP_LV_RESERVE_BASE          ((uint32_t)&__StackLimit)
#endif

#define DEEP_LV_RESERVE()             ((volatile deep_lv_reserve_t *)DEEP_LV_RESERVE_BASE)

/* Geometry and decode come from deep_lv_reserve_layout.h, shared with the
 * secure build's cp_lv_reserve.c; everything folds to immediates so the IRAM
 * capture below never reads flash. */
__IRAM_PM void sys_hal_mem_check_bad_point_value_save(void)
{
	volatile deep_lv_reserve_t *tbl = DEEP_LV_RESERVE();
	uint32_t point_mask = 0;

	for (uint32_t i = 0; i < CP_MEM_CHECK_POINT_NUM; i++) {
		uint32_t bad_addr = cp_mem_check_bad_point_addr_get(i);

		/* Such a board must not sleep on this capture: the slot would be
		 * written back as raw garbage on wake. */
		if ((bad_addr >= DEEP_LV_RESERVE_BASE) &&
		    (bad_addr < (DEEP_LV_RESERVE_BASE + sizeof(deep_lv_reserve_t)))) {
			assert(0);
			return; /* only reached when asserts are compiled out */
		}

		if (bad_addr != 0) {
			point_mask |= 1u << i;
		}

		tbl->values[i] = (bad_addr != 0) ? *(volatile uint32_t *)bad_addr : 0;
	}

	tbl->point_mask = point_mask;
	tbl->magic = DEEP_LV_RESERVE_MAGIC;
}

__attribute__((optimize("O2"))) void sys_hal_mem_check_bad_point_value_restore(void)
{
	volatile deep_lv_reserve_t *tbl = DEEP_LV_RESERVE();
	uint32_t point_mask;

	if (tbl->magic != DEEP_LV_RESERVE_MAGIC) {
		return;
	}

	point_mask = tbl->point_mask;
	tbl->magic = 0;

	while (point_mask != 0) {
		uint32_t i = (uint32_t)__builtin_ctz(point_mask);
		uint32_t bad_addr;

		point_mask &= point_mask - 1u;

		bad_addr = cp_mem_check_bad_point_addr_get(i);
		if (bad_addr != 0) {
			*(volatile uint32_t *)bad_addr = tbl->values[i];
		}
	}
}

#endif /* CONFIG_DEEP_LV && CONFIG_SPE */
