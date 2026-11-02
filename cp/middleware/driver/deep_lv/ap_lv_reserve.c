// Copyright 2023-2025 Beken
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
#include <common/bk_include.h>
#include "soc/soc.h"
#include "bk_arch.h"
#include "ap_lv_reserve.h"
#include "deep_lv_reserve_layout.h"
#include <stdint.h>

#if CONFIG_SPE && CONFIG_PM_AP_FAST_BOOT_ENABLE

#define AP_LV_RESERVE_MAGIC     0x414C5652u /* "ALVR" */

typedef struct {
	uint32_t addr;
	uint32_t words;
	uint32_t values[AP_MEM_CHECK_POINT_MAX_WORDS];
} ap_lv_reserve_entry_t;

typedef struct {
	uint32_t magic;
	ap_lv_reserve_entry_t entry[AP_MEM_CHECK_POINT_NUM];
} ap_lv_reserve_t;

static volatile ap_lv_reserve_t s_ap_lv_reserve;

void ap_lv_reserve_save(void)
{
	volatile ap_lv_reserve_t *tbl = &s_ap_lv_reserve;

	tbl->magic = 0u;

	for (uint32_t i = 0; i < AP_MEM_CHECK_POINT_NUM; i++) {
		uint32_t addr = ap_mem_check_bad_point_addr_get(i);
		uint32_t words = (addr != 0u) ? ap_mem_check_bad_point_words_get(i) : 0u;

		tbl->entry[i].addr = addr;
		tbl->entry[i].words = words;
		for (uint32_t w = 0; w < words; w++) {
			tbl->entry[i].values[w] = ((volatile uint32_t *)addr)[w];
		}
	}

	__DSB();

	/* Magic last: a capture that did not complete must never be consumed. */
	tbl->magic = AP_LV_RESERVE_MAGIC;
	__DSB();
}

void ap_lv_reserve_restore(void)
{
	volatile ap_lv_reserve_t *tbl = &s_ap_lv_reserve;

	if (tbl->magic != AP_LV_RESERVE_MAGIC) {
		return;
	}

	tbl->magic = 0u;

	for (uint32_t i = 0; i < AP_MEM_CHECK_POINT_NUM; i++) {
		uint32_t addr = tbl->entry[i].addr;
		uint32_t words = tbl->entry[i].words;

		if (!ap_mem_check_addr_in_ap_ram(addr) ||
			(words > AP_MEM_CHECK_POINT_MAX_WORDS)) {
			continue;
		}

		for (uint32_t w = 0; w < words; w++) {
			((volatile uint32_t *)addr)[w] = tbl->entry[i].values[w];
		}
	}

	__DSB();
}

#endif /* CONFIG_SPE && CONFIG_PM_AP_FAST_BOOT_ENABLE */
