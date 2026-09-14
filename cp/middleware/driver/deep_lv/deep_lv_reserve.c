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

#include "deep_lv_reserve.h"
#include "sdkconfig.h"
#include "reg_base.h"
#include "soc/soc.h"
#include <stdint.h>

#if CONFIG_DEEP_LV

#define MEM_CHECK_BLOCK_NUM               3
#define MEM_CHECK_POINT_PER_BLOCK         4
#define MEM_CHECK_BST_POINT_NUM           (MEM_CHECK_BLOCK_NUM * MEM_CHECK_POINT_PER_BLOCK)
#define MEM_CHECK_REG_IDX_FIRST           8
#define MEM_CHECK_BST_VALID_MASK          0x8000u
#define MEM_CHECK_BST_ADDR_MASK           0x7FFFu

#define DEEP_LV_RESERVE_MAGIC             0x444C5652u /* "DLVR" */

typedef struct {
	uint32_t magic;
	uint32_t values[MEM_CHECK_BST_POINT_NUM];
} deep_lv_reserve_t;

#if CONFIG_SOC_SMP
extern uint32_t __StackLimitCore1;
#define DEEP_LV_RESERVE_BASE          ((uint32_t)&__StackLimitCore1)
#else
extern uint32_t __StackLimit;
#define DEEP_LV_RESERVE_BASE          ((uint32_t)&__StackLimit)
#endif

#define DEEP_LV_RESERVE()             ((volatile deep_lv_reserve_t *)DEEP_LV_RESERVE_BASE)

static inline __attribute__((always_inline)) uint32_t mem_check_bad_point_addr_get(uint32_t idx)
{
	volatile uint32_t *reg_base = (volatile uint32_t *)SOC_MEM_CHECK_REG_BASE;
	/* The repair point sits in the upper half of the register. */
	uint16_t bst_high = (uint16_t)(reg_base[MEM_CHECK_REG_IDX_FIRST + idx] >> 16);
	uint32_t mem_base;
	uint32_t mem_size;

	if ((bst_high & MEM_CHECK_BST_VALID_MASK) == 0) {
		return 0;
	}

	switch (idx / MEM_CHECK_POINT_PER_BLOCK) {
	case 0:
		mem_base = SOC_SRAM0_DATA_BASE;
		mem_size = SOC_SRAM0_DATA_SIZE;
		break;
	case 1:
		mem_base = SOC_SRAM1_DATA_BASE;
		mem_size = SOC_SRAM1_DATA_SIZE;
		break;
	default:
		mem_base = SOC_SRAM2_DATA_BASE;
		mem_size = SOC_SRAM2_DATA_SIZE;
		break;
	}

	uint32_t addr = mem_base + (uint32_t)(bst_high & MEM_CHECK_BST_ADDR_MASK) * 4;

	if (addr >= (mem_base + mem_size)) {
		return 0;
	}

	/* Writing back inside the table would clobber the data being restored. */
	if ((addr >= DEEP_LV_RESERVE_BASE) &&
		(addr < (DEEP_LV_RESERVE_BASE + sizeof(deep_lv_reserve_t)))) {
		return 0;
	}

	return addr;
}

__IRAM_PM void sys_hal_mem_check_bad_point_value_save(void)
{
	volatile deep_lv_reserve_t *tbl = DEEP_LV_RESERVE();

	for (uint32_t i = 0; i < MEM_CHECK_BST_POINT_NUM; i++) {
		uint32_t bad_addr = mem_check_bad_point_addr_get(i);

		tbl->values[i] = (bad_addr != 0) ? *(volatile uint32_t *)bad_addr : 0;
	}

	tbl->magic = DEEP_LV_RESERVE_MAGIC;
}

void sys_hal_mem_check_bad_point_value_restore(void)
{
	volatile deep_lv_reserve_t *tbl = DEEP_LV_RESERVE();

	if (tbl->magic != DEEP_LV_RESERVE_MAGIC) {
		return;
	}

	tbl->magic = 0;

	for (uint32_t i = 0; i < MEM_CHECK_BST_POINT_NUM; i++) {
		uint32_t bad_addr = mem_check_bad_point_addr_get(i);

		if (bad_addr != 0) {
			*(volatile uint32_t *)bad_addr = tbl->values[i];
		}
	}
}

#endif /* CONFIG_DEEP_LV */
