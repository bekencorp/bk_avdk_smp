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

#include "sys_pm_hal_debug.h"
#include "sys_ll.h"

#if CONFIG_PM_AP_SRAM_RETENTION_CHECK
#include <stdint.h>
#include "bk_arch.h"
#include <modules/ap_sram_retention_check.h>

typedef struct {
	uint32_t start;
	uint32_t size;
	uint32_t snapshot_offset;
} ap_sram_check_region_t;

static const ap_sram_check_region_t s_ap_sram_check_regions[AP_SRAM_CHECK_REGION_COUNT] = {
	{SOC_SRAM3_DATA_BASE, SOC_SRAM3_DATA_SIZE, 0x00000u},
	{SOC_SRAM4_DATA_BASE, SOC_SRAM4_DATA_SIZE, 0x40000u},
	{SOC_SRAM5_DATA_BASE, SOC_SRAM5_DATA_SIZE, 0x80000u},
	{SOC_SRAM6_DATA_BASE, SOC_SRAM6_DATA_SIZE, 0xc0000u},
};

extern uint8_t _sstack_core0;
extern uint8_t _estack_core0;
extern uint8_t _sstack_core1;
extern uint8_t _estack_core1;

static volatile ap_sram_check_shared_t *const s_ap_sram_check_shared =
	(volatile ap_sram_check_shared_t *)AP_SRAM_CHECK_PSRAM_BASE;
static uint8_t *const s_ap_sram_check_snapshot =
	(uint8_t *)AP_SRAM_CHECK_SNAPSHOT_BASE;

static inline uint32_t ap_sram_check_crc32_byte(uint32_t crc, uint8_t data)
{
	crc ^= data;
	for (uint32_t i = 0; i < 8u; i++) {
		crc = (crc >> 1) ^ ((crc & 1u) ? 0xedb88320u : 0u);
	}
	return crc;
}

static inline bool ap_sram_check_is_skipped(uintptr_t addr)
{
	uint32_t physical = SOC_SRAM_PERI_ADDR((uint32_t)addr);

	for (uint32_t i = 0; i < AP_SRAM_CHECK_SKIP_RANGE_COUNT; i++) {
		if ((physical >= s_ap_sram_check_shared->skip_start[i]) &&
			(physical < s_ap_sram_check_shared->skip_end[i])) {
			return true;
		}
	}
	return false;
}

static uint32_t ap_sram_check_region_crc32(const ap_sram_check_region_t *region)
{
	uint32_t crc = 0xffffffffu;
	volatile const uint8_t *data =
		(volatile const uint8_t *)(uintptr_t)region->start;

	for (uint32_t i = 0; i < region->size; i++) {
		uintptr_t addr = (uintptr_t)region->start + i;
		if (!ap_sram_check_is_skipped(addr)) {
			crc = ap_sram_check_crc32_byte(crc, data[i]);
		}
	}
	return ~crc;
}

void sys_pm_hal_ap_sram_check_set_idle_stack(void *start, void *end)
{
	s_ap_sram_check_shared->skip_start[0] =
		SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)start);
	s_ap_sram_check_shared->skip_end[0] =
		SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)end);
}

void sys_pm_hal_ap_sram_check_save(void)
{
	s_ap_sram_check_shared->magic = 0u;
	s_ap_sram_check_shared->magic_inv = 0u;
	s_ap_sram_check_shared->region_count = AP_SRAM_CHECK_REGION_COUNT;
	s_ap_sram_check_shared->generation++;
	s_ap_sram_check_shared->skip_start[1] =
		SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)&_sstack_core1);
	s_ap_sram_check_shared->skip_end[1] =
		SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)&_estack_core1);
	s_ap_sram_check_shared->skip_start[2] =
		SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)&_sstack_core0);
	s_ap_sram_check_shared->skip_end[2] =
		SOC_SRAM_PERI_ADDR((uint32_t)(uintptr_t)&_estack_core0);

	for (uint32_t i = 0; i < AP_SRAM_CHECK_REGION_COUNT; i++) {
		const ap_sram_check_region_t *region = &s_ap_sram_check_regions[i];
		volatile const uint32_t *src =
			(volatile const uint32_t *)(uintptr_t)region->start;
		uint32_t *dst = (uint32_t *)(void *)
			&s_ap_sram_check_snapshot[region->snapshot_offset];

		s_ap_sram_check_shared->crc_before[i] =
			ap_sram_check_region_crc32(region);
		s_ap_sram_check_shared->crc_before_inv[i] =
			~s_ap_sram_check_shared->crc_before[i];
		for (uint32_t word = 0; word < (region->size / sizeof(uint32_t)); word++) {
			uintptr_t addr = (uintptr_t)region->start +
				word * sizeof(uint32_t);
			if (!ap_sram_check_is_skipped(addr)) {
				dst[word] = src[word];
			}
		}
	}

	__DSB();
	s_ap_sram_check_shared->magic = AP_SRAM_CHECK_MAGIC;
	s_ap_sram_check_shared->magic_inv = ~AP_SRAM_CHECK_MAGIC;
	__DSB();
}
#endif

#if CONFIG_PM_HAL_DEBUG

static const uint8_t s_sys_regs[] = {
	8, 9, 0xa, 0xe, 0xf, 0x10, 0x12, 0x20, 0x21, 0x22, 0x23,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e
};

#define SYS_REG_NUM (sizeof(s_sys_regs) / sizeof(s_sys_regs[0]))
static uint32_t s_sys_regs_before_sleep[SYS_REG_NUM] = {0};
static uint32_t s_sys_regs_after_sleep[SYS_REG_NUM] = {0};

void sys_hal_debug_dump_sys_regs(void)
{
}

void sys_hal_debug_get_sys_regs(uint32_t *regs)
{
	uint8_t reg_id;

	for (int i = 0; i < SYS_REG_NUM; i++) {
		reg_id = s_sys_regs[i];

		if (reg_id >= 0x40) {
			regs[i] = sys_ll_get_analog_reg_value(SOC_SYS_REG_BASE + (reg_id << 2));
		} else {
			regs[i] = REG_READ(SOC_SYS_REG_BASE + (reg_id << 2));
		}
		PM_HAL_LOGV("sys_r0x%02x: %x\r\n", reg_id, regs[i]);
	}
}

void sys_hal_debug_get_sys_regs_before_sleep(void)
{
	sys_hal_debug_get_sys_regs(s_sys_regs_before_sleep);
}

void sys_hal_debug_get_sys_regs_after_waked(void)
{
	sys_hal_debug_get_sys_regs(s_sys_regs_after_sleep);
}

void sys_hal_debug_check_sys_regs(void)
{
	uint8_t reg_id;

	for (int i = 0; i < SYS_REG_NUM; i++) {
		reg_id = s_sys_regs[i];

		if (s_sys_regs_before_sleep[i] != s_sys_regs_after_sleep[i]) {
			PM_HAL_LOGV("sys_r0x%02x mismatch: %x != %x\r\n",
				reg_id, s_sys_regs_before_sleep[i], s_sys_regs_after_sleep[i]);
		}
	}
}

void sys_hal_debug_gpio_up(int id)
{
	GPIO_UP(id);
}

void sys_hal_debug_gpio_down(int id)
{
	GPIO_DOWN(id);
}

#endif
