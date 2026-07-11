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

#include <stddef.h>
#include <stdint.h>
#include "pm_debug.h"
#include "aon_pmu_hal.h"
#include <os/os.h>
#include "sys_ll.h"
#include "sys_types.h"

#if CONFIG_CKMN
#include <driver/ckmn.h>
#include "ckmn_reg.h"
#endif

/* Used unconditionally around deep LV entry; declared outside the
 * CONFIG_DEEP_LV_DEBUG_LOG block so the snapshot log can be disabled
 * while the deep-sleep entered flag still works. */
static volatile uint32_t s_lv_deep_sleep_entered = 0;

#if CONFIG_PM_CP_DEEP_LV_SRAM_CHECK
#define SYS_PM_SRAM_CRC_MAGIC                 (0x43504352u) /* "CPCR" */
#define SYS_PM_SRAM_CRC_STATUS_MAGIC_ERR      (1u << 0)
#define SYS_PM_SRAM_CRC_STATUS_RANGE_ERR      (1u << 1)
#define SYS_PM_SRAM_CRC_STATUS_VALUE_ERR      (1u << 2)
#define SYS_PM_SRAM_CRC_REGION_IRAM           (0)
#define SYS_PM_SRAM_CRC_REGION_DATA           (1)
#define SYS_PM_SRAM_CRC_REGION_BSS            (2)
#define SYS_PM_SRAM_CRC_REGION_NUM            (3)
#define SYS_PM_SRAM_CRC_BLOCK_SIZE            (0x4u)
#define SYS_PM_SRAM_CRC_BLOCK_NUM_MAX         (1024u)
#define SYS_PM_SRAM_CRC_SKIP_RANGE_NUM        (4)

typedef struct {
	uint32_t magic;
	uint32_t magic_inv;
	uint32_t start;
	uint32_t end;
	uint32_t length;
	uint32_t crc_before;
	uint32_t crc_before_inv;
	uint32_t crc_after;
	uint32_t last_status;
	uint32_t mismatch_count;
	uint32_t first_bad_start;
	uint32_t first_bad_end;
	uint32_t first_bad_crc_before;
	uint32_t first_bad_crc_after;
} sys_pm_sram_crc_record_t;

extern uint8_t __iram_start__;
extern uint8_t __iram_end__;
extern uint8_t __data_start__;
extern uint8_t __data_end__;
extern uint8_t _bss_start;
extern uint8_t _bss_end;

static void *s_sys_pm_crc_idle_stack_start;
static void *s_sys_pm_crc_idle_stack_end;
static sys_pm_sram_crc_record_t s_sys_pm_sram_crc_records[SYS_PM_SRAM_CRC_REGION_NUM];
static uint32_t s_sys_pm_sram_crc_blocks[SYS_PM_SRAM_CRC_REGION_NUM][SYS_PM_SRAM_CRC_BLOCK_NUM_MAX]
	__attribute__((section(".dtcm_sec_data")));

static uint32_t s_sys_pm_crc32_nibble_table[16] __attribute__((section(".dtcm_sec_data"))) = {
	0x00000000u, 0x1db71064u, 0x3b6e20c8u, 0x26d930acu,
	0x76dc4190u, 0x6b6b51f4u, 0x4db26158u, 0x5005713cu,
	0xedb88320u, 0xf00f9344u, 0xd6d6a3e8u, 0xcb61b38cu,
	0x9b64c2b0u, 0x86d3d2d4u, 0xa00ae278u, 0xbdbdf21cu,
};

__IRAM_PM static inline uint32_t sys_pm_hal_crc32_update_u8(uint32_t crc, uint8_t data)
{
	crc ^= data;
	crc = (crc >> 4) ^ s_sys_pm_crc32_nibble_table[crc & 0x0fu];
	crc = (crc >> 4) ^ s_sys_pm_crc32_nibble_table[crc & 0x0fu];

	return crc;
}

__IRAM_PM static uint32_t sys_pm_hal_crc32_update_buf(uint32_t crc, const void *data, uint32_t length)
{
	const uint32_t *word = (const uint32_t *)data;
	uint32_t words = length >> 2;

	while (words-- > 0) {
		uint32_t value = *word++;

		crc = sys_pm_hal_crc32_update_u8(crc, (uint8_t)(value));
		crc = sys_pm_hal_crc32_update_u8(crc, (uint8_t)(value >> 8));
		crc = sys_pm_hal_crc32_update_u8(crc, (uint8_t)(value >> 16));
		crc = sys_pm_hal_crc32_update_u8(crc, (uint8_t)(value >> 24));
	}

	const uint8_t *byte = (const uint8_t *)word;
	for (uint32_t i = 0; i < (length & 0x3u); i++) {
		crc = sys_pm_hal_crc32_update_u8(crc, byte[i]);
	}

	return crc;
}

__IRAM_PM static uint32_t sys_pm_hal_fast_crc32(const void *data, uint32_t length)
{
	uint32_t crc = sys_pm_hal_crc32_update_buf(0xffffffffu, data, length);

	return ~crc;
}

__IRAM_PM static void sys_pm_hal_crc32_update_range_skip(uint32_t *crc, uint32_t *cursor,
	uint32_t end, uint32_t skip_start, uint32_t skip_end)
{
	uint32_t start = *cursor;

	if ((skip_end <= start) || (skip_start >= end)) {
		return;
	}

	if (skip_start > start) {
		uint32_t update_end = (skip_start < end) ? skip_start : end;
		*crc = sys_pm_hal_crc32_update_buf(*crc, (const void *)(uintptr_t)start, update_end - start);
	}

	if (skip_end > *cursor) {
		*cursor = (skip_end < end) ? skip_end : end;
	}
}

__IRAM_PM static void sys_pm_hal_sram_crc_skip_range_get(uint32_t index, uint32_t *start, uint32_t *end)
{
	switch (index) {
	case 0:
		*start = (uint32_t)(uintptr_t)s_sys_pm_crc32_nibble_table;
		*end = *start + sizeof(s_sys_pm_crc32_nibble_table);
		break;
	case 1:
		*start = (uint32_t)(uintptr_t)s_sys_pm_sram_crc_records;
		*end = *start + sizeof(s_sys_pm_sram_crc_records);
		break;
	case 2:
		*start = (uint32_t)(uintptr_t)s_sys_pm_sram_crc_blocks;
		*end = *start + sizeof(s_sys_pm_sram_crc_blocks);
		break;
	case 3:
		*start = (uint32_t)(uintptr_t)s_sys_pm_crc_idle_stack_start;
		*end = (uint32_t)(uintptr_t)s_sys_pm_crc_idle_stack_end;
		break;
	default:
		*start = 0;
		*end = 0;
		break;
	}
}

__IRAM_PM static uint32_t sys_pm_hal_region_crc32(uint32_t start, uint32_t end)
{
	uint32_t crc = 0xffffffffu;
	uint32_t cursor = start;

	if (end <= start) {
		return 0;
	}

	for (uint32_t handled = 0; handled < SYS_PM_SRAM_CRC_SKIP_RANGE_NUM; handled++) {
		uint32_t next_start = 0xffffffffu;
		uint32_t next_end = 0;

		for (uint32_t i = 0; i < SYS_PM_SRAM_CRC_SKIP_RANGE_NUM; i++) {
			uint32_t skip_start = 0;
			uint32_t skip_end = 0;
			sys_pm_hal_sram_crc_skip_range_get(i, &skip_start, &skip_end);
			if ((skip_end > cursor) && (skip_start < end) && (skip_start < next_start)) {
				next_start = skip_start;
				next_end = skip_end;
			}
		}

		if (next_start == 0xffffffffu) {
			break;
		}
		sys_pm_hal_crc32_update_range_skip(&crc, &cursor, end, next_start, next_end);
	}

	if (cursor < end) {
		crc = sys_pm_hal_crc32_update_buf(crc, (const void *)(uintptr_t)cursor, end - cursor);
	}

	return ~crc;
}

void sys_pm_hal_sram_crc_set_idle_stack(void *start, void *end)
{
	s_sys_pm_crc_idle_stack_start = start;
	s_sys_pm_crc_idle_stack_end = end;
}

void sys_pm_hal_sram_crc_get_idle_stack(void **start, void **end)
{
	if (start) {
		*start = s_sys_pm_crc_idle_stack_start;
	}
	if (end) {
		*end = s_sys_pm_crc_idle_stack_end;
	}
}

__IRAM_PM static uint32_t sys_pm_hal_sram_crc_block_count(uint32_t length)
{
	uint32_t blocks = (length + SYS_PM_SRAM_CRC_BLOCK_SIZE - 1) / SYS_PM_SRAM_CRC_BLOCK_SIZE;

	return (blocks > SYS_PM_SRAM_CRC_BLOCK_NUM_MAX) ? SYS_PM_SRAM_CRC_BLOCK_NUM_MAX : blocks;
}

__IRAM_PM static const char *sys_pm_hal_sram_crc_region_name(uint32_t region)
{
	switch (region) {
	case SYS_PM_SRAM_CRC_REGION_IRAM:
		return "IRAM";
	case SYS_PM_SRAM_CRC_REGION_DATA:
		return "DATA";
	case SYS_PM_SRAM_CRC_REGION_BSS:
		return "BSS";
	default:
		return "UNKNOWN";
	}
}

__IRAM_PM static void sys_pm_hal_sram_crc_region_get(uint32_t region, uint32_t *start, uint32_t *end)
{
	switch (region) {
	case SYS_PM_SRAM_CRC_REGION_IRAM:
		*start = (uint32_t)(uintptr_t)&__iram_start__;
		*end = (uint32_t)(uintptr_t)&__iram_end__;
		break;
	case SYS_PM_SRAM_CRC_REGION_DATA:
		*start = (uint32_t)(uintptr_t)&__data_start__;
		*end = (uint32_t)(uintptr_t)&__data_end__;
		break;
	case SYS_PM_SRAM_CRC_REGION_BSS:
		*start = (uint32_t)(uintptr_t)&_bss_start;
		*end = (uint32_t)(uintptr_t)&_bss_end;
		break;
	default:
		*start = 0;
		*end = 0;
		break;
	}
}

__IRAM_PM void sys_pm_hal_sram_crc_save(void)
{
	for (uint32_t i = 0; i < SYS_PM_SRAM_CRC_REGION_NUM; i++) {
		uint32_t start = 0;
		uint32_t end = 0;
		sys_pm_hal_sram_crc_region_get(i, &start, &end);
		uint32_t length = end - start;
		uint32_t crc = (i == SYS_PM_SRAM_CRC_REGION_IRAM) ?
			sys_pm_hal_fast_crc32((const void *)(uintptr_t)start, length) :
			sys_pm_hal_region_crc32(start, end);

		s_sys_pm_sram_crc_records[i].magic = SYS_PM_SRAM_CRC_MAGIC;
		s_sys_pm_sram_crc_records[i].magic_inv = ~SYS_PM_SRAM_CRC_MAGIC;
		s_sys_pm_sram_crc_records[i].start = start;
		s_sys_pm_sram_crc_records[i].end = end;
		s_sys_pm_sram_crc_records[i].length = length;
		s_sys_pm_sram_crc_records[i].crc_before = crc;
		s_sys_pm_sram_crc_records[i].crc_before_inv = ~crc;
		s_sys_pm_sram_crc_records[i].crc_after = 0;
		s_sys_pm_sram_crc_records[i].last_status = 0;
		s_sys_pm_sram_crc_records[i].first_bad_start = 0;
		s_sys_pm_sram_crc_records[i].first_bad_end = 0;
		s_sys_pm_sram_crc_records[i].first_bad_crc_before = 0;
		s_sys_pm_sram_crc_records[i].first_bad_crc_after = 0;

		uint32_t blocks = sys_pm_hal_sram_crc_block_count(length);
		for (uint32_t j = 0; j < blocks; j++) {
			uint32_t block_start = start + j * SYS_PM_SRAM_CRC_BLOCK_SIZE;
			uint32_t block_end = block_start + SYS_PM_SRAM_CRC_BLOCK_SIZE;
			if (block_end > end) {
				block_end = end;
			}
			s_sys_pm_sram_crc_blocks[i][j] = (i == SYS_PM_SRAM_CRC_REGION_IRAM) ?
				sys_pm_hal_fast_crc32((const void *)(uintptr_t)block_start, block_end - block_start) :
				sys_pm_hal_region_crc32(block_start, block_end);
		}
	}
}

__IRAM_PM uint32_t sys_pm_hal_sram_crc_check(void)
{
	uint32_t final_status = 0;

	for (uint32_t i = 0; i < SYS_PM_SRAM_CRC_REGION_NUM; i++) {
		uint32_t start = 0;
		uint32_t end = 0;
		uint32_t status = 0;
		sys_pm_hal_sram_crc_region_get(i, &start, &end);

		if ((s_sys_pm_sram_crc_records[i].magic != SYS_PM_SRAM_CRC_MAGIC) ||
			(s_sys_pm_sram_crc_records[i].magic_inv != ~SYS_PM_SRAM_CRC_MAGIC) ||
			(s_sys_pm_sram_crc_records[i].crc_before_inv != ~s_sys_pm_sram_crc_records[i].crc_before)) {
			status |= SYS_PM_SRAM_CRC_STATUS_MAGIC_ERR;
		}

		if ((s_sys_pm_sram_crc_records[i].start != start) ||
			(s_sys_pm_sram_crc_records[i].end != end) ||
			(s_sys_pm_sram_crc_records[i].length != (end - start))) {
			status |= SYS_PM_SRAM_CRC_STATUS_RANGE_ERR;
		}

		if (status == 0) {
			s_sys_pm_sram_crc_records[i].crc_after = (i == SYS_PM_SRAM_CRC_REGION_IRAM) ?
				sys_pm_hal_fast_crc32((const void *)(uintptr_t)start, s_sys_pm_sram_crc_records[i].length) :
				sys_pm_hal_region_crc32(start, end);
			if (s_sys_pm_sram_crc_records[i].crc_after != s_sys_pm_sram_crc_records[i].crc_before) {
				status |= SYS_PM_SRAM_CRC_STATUS_VALUE_ERR;
				uint32_t blocks = sys_pm_hal_sram_crc_block_count(s_sys_pm_sram_crc_records[i].length);
				for (uint32_t j = 0; j < blocks; j++) {
					uint32_t block_start = start + j * SYS_PM_SRAM_CRC_BLOCK_SIZE;
					uint32_t block_end = block_start + SYS_PM_SRAM_CRC_BLOCK_SIZE;
					if (block_end > end) {
						block_end = end;
					}
					uint32_t block_crc = (i == SYS_PM_SRAM_CRC_REGION_IRAM) ?
						sys_pm_hal_fast_crc32((const void *)(uintptr_t)block_start, block_end - block_start) :
						sys_pm_hal_region_crc32(block_start, block_end);
					if (block_crc != s_sys_pm_sram_crc_blocks[i][j]) {
						s_sys_pm_sram_crc_records[i].first_bad_start = block_start;
						s_sys_pm_sram_crc_records[i].first_bad_end = block_end;
						s_sys_pm_sram_crc_records[i].first_bad_crc_before = s_sys_pm_sram_crc_blocks[i][j];
						s_sys_pm_sram_crc_records[i].first_bad_crc_after = block_crc;
						break;
					}
				}
			}
		}

		if (status != 0) {
			s_sys_pm_sram_crc_records[i].mismatch_count++;
		}
		s_sys_pm_sram_crc_records[i].last_status = status;
		final_status |= status;
	}

	return final_status;
}

void sys_pm_hal_sram_crc_dump(void)
{
	for (uint32_t i = 0; i < SYS_PM_SRAM_CRC_REGION_NUM; i++) {
		if (s_sys_pm_sram_crc_records[i].last_status == 0) {
			continue;
		}

		LOGE("CP SRAM %s CRC check failed: status=0x%x pre=0x%x post=0x%x range=[0x%x,0x%x) count=%u first_bad=[0x%x,0x%x) block_pre=0x%x block_post=0x%x\r\n",
			sys_pm_hal_sram_crc_region_name(i),
			s_sys_pm_sram_crc_records[i].last_status,
			s_sys_pm_sram_crc_records[i].crc_before,
			s_sys_pm_sram_crc_records[i].crc_after,
			s_sys_pm_sram_crc_records[i].start,
			s_sys_pm_sram_crc_records[i].end,
			s_sys_pm_sram_crc_records[i].mismatch_count,
			s_sys_pm_sram_crc_records[i].first_bad_start,
			s_sys_pm_sram_crc_records[i].first_bad_end,
			s_sys_pm_sram_crc_records[i].first_bad_crc_before,
			s_sys_pm_sram_crc_records[i].first_bad_crc_after);
	}
}
#endif

#if CONFIG_PM_CLOCK_VOTE_RECORD
#define PM_CLOCK_VOTE_RECORD_NUM             (64)
#define PM_CLOCK_VOTE_RECORD_MODULE_START    (CLK_PWR_ID_AUDIO)
#define PM_CLOCK_VOTE_RECORD_MODULE_END      (CLK_PWR_ID_OFDM)

typedef struct
{
	uint32_t module;
	uint32_t clock_state;
	uint32_t return_address;
	uint32_t clk_status;
	uint32_t module_clk_status;
} pm_clock_vote_record_t;

static volatile uint32_t s_pm_clock_vote_record_idx = 0;
static volatile pm_clock_vote_record_t s_pm_clock_vote_records[PM_CLOCK_VOTE_RECORD_NUM];

extern uint32_t sys_hal_clk_pwr_status_get(dev_clk_pwr_id_t dev);
extern uint32_t sys_hal_clk_pwr_is_enabled(dev_clk_pwr_id_t dev);

void sys_hal_pm_clock_vote_record(uint32_t module, uint32_t clock_state, uint32_t return_address)
{
	uint32_t index = s_pm_clock_vote_record_idx;

	if ((module < PM_CLOCK_VOTE_RECORD_MODULE_START) || (module > PM_CLOCK_VOTE_RECORD_MODULE_END))
	{
		return;
	}

	s_pm_clock_vote_records[index].module = module;
	s_pm_clock_vote_records[index].clock_state = clock_state;
	s_pm_clock_vote_records[index].return_address = return_address;
	s_pm_clock_vote_records[index].clk_status = sys_hal_clk_pwr_status_get((dev_clk_pwr_id_t)module);
	s_pm_clock_vote_records[index].module_clk_status = sys_hal_clk_pwr_is_enabled((dev_clk_pwr_id_t)module);
	s_pm_clock_vote_record_idx = (s_pm_clock_vote_record_idx + 1) % PM_CLOCK_VOTE_RECORD_NUM;

	PM_HAL_LOGV("pm_clk_vote_rec: mod = %d, op = %d, ret = %p, clk_sta = %x, mod_clk_sta = %d\n",
		module, clock_state, (void *)return_address, s_pm_clock_vote_records[index].clk_status,
		s_pm_clock_vote_records[index].module_clk_status);
}
#endif

#if CONFIG_PM_POWER_VOTE_RECORD
#define PM_POWER_VOTE_RECORD_NUM             (64)

typedef struct {
	uint32_t module;
	uint32_t domain;
	uint32_t submodule;
	uint32_t power_state;
	uint32_t return_address;
	uint32_t power_status;
} pm_power_vote_record_t;

static volatile uint32_t s_pm_power_vote_record_idx = 0;
static volatile pm_power_vote_record_t s_pm_power_vote_records[PM_POWER_VOTE_RECORD_NUM];

extern int32 sys_hal_module_power_state_get(power_module_name_t module);

void sys_hal_pm_power_vote_record(uint32_t module, uint32_t power_state, uint32_t return_address, uint32_t filter_domain)
{
	uint32_t domain = module / PM_MODULE_SUB_POWER_DOMAIN_MAX;
	uint32_t submodule = module % PM_MODULE_SUB_POWER_DOMAIN_MAX;
	uint32_t index = s_pm_power_vote_record_idx;

	if ((domain > PM_POWER_DOMAIN_4) || (domain != filter_domain))
	{
		return;
	}

	s_pm_power_vote_records[index].module = module;
	s_pm_power_vote_records[index].domain = domain;
	s_pm_power_vote_records[index].submodule = submodule;
	s_pm_power_vote_records[index].power_state = power_state;
	s_pm_power_vote_records[index].return_address = return_address;
	s_pm_power_vote_records[index].power_status = sys_hal_module_power_state_get((power_module_name_t)domain);
	s_pm_power_vote_record_idx = (s_pm_power_vote_record_idx + 1) % PM_POWER_VOTE_RECORD_NUM;

	PM_HAL_LOGV("pm_pow_vote_rec: mod = %d, dom = %d, sub = %d, op = %d, ret = %p, pow_sta = %d\n",
		module, domain, submodule, power_state, (void *)return_address,
		s_pm_power_vote_records[index].power_status);
}
#endif

__IRAM_PM void sys_hal_lv_deep_sleep_enter_clear(void)
{
	s_lv_deep_sleep_entered = 0;
}

__IRAM_PM void sys_hal_lv_deep_sleep_enter_set(void)
{
	s_lv_deep_sleep_entered = 1;
}

#if CONFIG_DEEP_LV_DEBUG_LOG
#define SYS_PM_LV_AON_SNAP_MAGIC 0x4C56444Du

typedef struct {
	uint32_t magic;
	uint32_t entry_cnt;
	uint32_t ana0;
	uint32_t ana2;
	uint32_t ana3;
	uint32_t ana5;
	uint32_t ana7;
	uint32_t ana8;
	uint32_t ana9;
	uint32_t valoldosel;
	uint32_t ana10;
	uint32_t ana11;
	uint32_t ana12;
	uint32_t ana13;
	uint32_t ana14;
	uint32_t aon_r0;
	uint32_t aon_r2;
	uint32_t aon_r40;
	uint32_t aon_r41;
	uint32_t aon_r42;
	uint32_t aon_r7b;
	uint32_t sys_r10;
	uint32_t dlv_r7b;
	uint32_t dlv_r0;
	uint32_t deep_sleep_entered;
	uint32_t ckmn_ctrl;
	uint32_t ckmn_rc32k_ctrl;
	uint32_t ckmn_corr_cfg;
	uint32_t ckmn_bkp_ctrl;
	uint32_t ckmn_bkp_rc32k;
	uint32_t ckmn_bkp_corr;
	/* AON-domain peripheral state (GPIO / WDT) */
	uint32_t aon_wdt_ctrl;
	uint32_t gpio_int_mask;
	uint32_t gpio_intsta0;
	uint32_t gpio_intsta1;
	uint32_t gpio_intsta2;
	uint32_t gpio22_cfg;
	uint32_t gpio23_cfg;
	uint32_t gpio24_cfg;
	uint32_t ana8_val;
	uint32_t ana11_val;
	/* audio/touch/USB analog regs (probed to find live AON modules) */
	uint32_t ana20;
	uint32_t ana21;
	uint32_t ana27;
	uint32_t ana28;
	uint32_t ana29;
	uint32_t ana30;
	uint32_t ana32;
	uint32_t ana41;
	uint32_t ana42;
	uint32_t ana43;
	/* AON digital: peripheral SRAM retention + power domain */
	uint32_t sys_reg0xf;   /* peripheral mem_ret bits */
	uint32_t sys_reg0xc;   /* device clock enable */
	uint32_t ldo_log_valid;
	uint32_t ldo_pre;
	uint32_t ldo_sleep_cfg;
	uint32_t ldo_after_sleep_set;
	uint32_t ldo_after_ramp;
	uint32_t ldo_backup;
	uint32_t ldo_after_ana_restore;
	uint32_t ldo_final;
} sys_pm_lv_aon_snap_t;

static uint32_t s_lv_sleep_enter_cnt = 0;

static struct {
	sys_pm_lv_aon_snap_t pre;
	sys_pm_lv_aon_snap_t at;
	sys_pm_lv_aon_snap_t post;
	volatile uint8_t pre_valid;
	volatile uint8_t at_valid;
	volatile uint8_t post_valid;
} s_lv_aon_debug;

__IRAM_PM static void sys_hal_lv_aon_snap_fill(sys_pm_lv_aon_snap_t *snap)
{
#if CONFIG_CKMN
	uint32_t ckmn_ctrl = REG_READ(CKMN_CTRL_ADDR);
	uint32_t ckmn_rc32k_ctrl = REG_READ(CKMN_RC32K_CTRL_ADDR);
	uint32_t ckmn_corr_cfg = REG_READ(CKMN_CORR_CFG_ADDR);
#else
	uint32_t ckmn_ctrl = 0;
	uint32_t ckmn_rc32k_ctrl = 0;
	uint32_t ckmn_corr_cfg = 0;
#endif

	snap->magic = SYS_PM_LV_AON_SNAP_MAGIC;
	snap->entry_cnt = s_lv_sleep_enter_cnt;
	snap->ana0 = sys_ll_get_ana_reg0_value();
	snap->ana2 = sys_ll_get_ana_reg2_value();
	snap->ana3 = sys_ll_get_ana_reg3_value();
	snap->ana5 = sys_ll_get_ana_reg5_value();
	snap->ana7 = sys_ll_get_ana_reg7_value();
	snap->ana8 = sys_ll_get_ana_reg8_value();
	snap->ana9 = sys_ll_get_ana_reg9_value();
	snap->valoldosel = sys_ll_get_ana_reg9_valoldosel();
	snap->ana10 = sys_ll_get_ana_reg10_value();
	snap->ana11 = sys_ll_get_ana_reg11_value();
	snap->ana12 = sys_ll_get_ana_reg12_value();
	snap->ana13 = sys_ll_get_ana_reg13_value();
	snap->ana14 = sys_ll_get_ana_reg14_value();
	snap->aon_r0 = aon_pmu_ll_get_r0();
	snap->aon_r2 = aon_pmu_ll_get_r2();
	snap->aon_r40 = aon_pmu_ll_get_r40();
	snap->aon_r41 = aon_pmu_ll_get_r41();
	snap->aon_r42 = aon_pmu_ll_get_r42_value();
	snap->aon_r7b = REG_READ(SOC_AON_PMU_REG_BASE + (0x7b << 2));
	snap->sys_r10 = sys_ll_get_reserver_reg0x10_value();
	snap->dlv_r7b = aon_pmu_hal_get_dlv_startup_iram();
	snap->dlv_r0 = aon_pmu_ll_get_r0_dlv_startup();
	snap->deep_sleep_entered = s_lv_deep_sleep_entered;
	snap->ckmn_ctrl = ckmn_ctrl;
	snap->ckmn_rc32k_ctrl = ckmn_rc32k_ctrl;
	snap->ckmn_corr_cfg = ckmn_corr_cfg;
#if CONFIG_CKMN
	bk_ckmn_sleep_regs_get_backup(&snap->ckmn_bkp_ctrl, &snap->ckmn_bkp_rc32k,
		&snap->ckmn_bkp_corr);
#else
	snap->ckmn_bkp_ctrl = 0;
	snap->ckmn_bkp_rc32k = 0;
	snap->ckmn_bkp_corr = 0;
#endif
	/* AON-domain peripheral snapshots: GPIO / WDT */
	snap->aon_wdt_ctrl = REG_READ(SOC_AON_WDT_REG_BASE);
	snap->gpio_int_mask = REG_READ(SOC_AON_GPIO_REG_BASE + (0x7c << 2));
	snap->gpio_intsta0 = REG_READ(SOC_AON_GPIO_REG_BASE + (0x78 << 2));
	snap->gpio_intsta1 = REG_READ(SOC_AON_GPIO_REG_BASE + (0x79 << 2));
	snap->gpio_intsta2 = REG_READ(SOC_AON_GPIO_REG_BASE + (0x7a << 2));
	snap->gpio22_cfg = REG_READ(SOC_AON_GPIO_REG_BASE + (22u << 2));
	snap->gpio23_cfg = REG_READ(SOC_AON_GPIO_REG_BASE + (23u << 2));
	snap->gpio24_cfg = REG_READ(SOC_AON_GPIO_REG_BASE + (24u << 2));
	snap->ana8_val = sys_ll_get_ana_reg8_value();
	snap->ana11_val = sys_ll_get_ana_reg11_value();
	snap->ana20 = sys_ll_get_ana_reg20_value();
	snap->ana21 = sys_ll_get_ana_reg21_value();
	snap->ana27 = sys_ll_get_ana_reg27_value();
	snap->ana28 = sys_ll_get_ana_reg28_value();
	snap->ana29 = sys_ll_get_ana_reg29_value();
	snap->ana30 = sys_ll_get_ana_reg30_value();
	snap->ana32 = sys_ll_get_ana_reg32_value();
	snap->ana41 = sys_ll_get_ana_reg41_value();
	snap->ana42 = sys_ll_get_ana_reg42_value();
	snap->ana43 = sys_ll_get_ana_reg43_value();
	snap->sys_reg0xf = sys_ll_get_reserver_reg0xf_value();
	snap->sys_reg0xc = sys_ll_get_cpu_device_clk_enable_value();
	snap->ldo_log_valid = 0;
}

__IRAM_PM void sys_hal_lv_aon_snap_pre_record(void)
{
	s_lv_sleep_enter_cnt++;
	sys_hal_lv_aon_snap_fill(&s_lv_aon_debug.pre);
	s_lv_aon_debug.pre_valid = 1;
}

__IRAM_PM void sys_hal_lv_aon_snap_at_sleep_record(void)
{
	sys_hal_lv_aon_snap_fill(&s_lv_aon_debug.at);
	s_lv_aon_debug.at_valid = 1;
}

__IRAM_PM void sys_hal_lv_aon_snap_post_record(void)
{
	sys_hal_lv_aon_snap_fill(&s_lv_aon_debug.post);
	s_lv_aon_debug.post_valid = 1;
}

__IRAM_PM void sys_hal_lv_aon_ldo_record(uint32_t pre, uint32_t sleep_cfg, uint32_t after_sleep_set,
	uint32_t after_ramp, uint32_t backup, uint32_t after_ana_restore, uint32_t final)
{
	s_lv_aon_debug.post.ldo_pre = pre;
	s_lv_aon_debug.post.ldo_sleep_cfg = sleep_cfg;
	s_lv_aon_debug.post.ldo_after_sleep_set = after_sleep_set;
	s_lv_aon_debug.post.ldo_after_ramp = after_ramp;
	s_lv_aon_debug.post.ldo_backup = backup;
	s_lv_aon_debug.post.ldo_after_ana_restore = after_ana_restore;
	s_lv_aon_debug.post.ldo_final = final;
	s_lv_aon_debug.post.ldo_log_valid = 1;
}

static void sys_hal_lv_aon_snap_print(const char *stage, const sys_pm_lv_aon_snap_t *snap)
{
	PM_HAL_LOGD("[LV AON %s] entry#%u\r\n", stage, snap->entry_cnt);
	PM_HAL_LOGD("  ana0=0x%08x ana2=0x%08x ana3=0x%08x\r\n",
		snap->ana0, snap->ana2, snap->ana3);
	PM_HAL_LOGD("  ana5=0x%08x ana7=0x%08x ana8=0x%08x\r\n",
		snap->ana5, snap->ana7, snap->ana8);
	PM_HAL_LOGD("  ana9=0x%08x valoldo=%u alopow=%u aloldohp=%u ana10=0x%08x\r\n",
		snap->ana9, snap->valoldosel,
		(snap->ana9 >> 19) & 1U, (snap->ana9 >> 21) & 1U,
		snap->ana10);
	PM_HAL_LOGD("  ana11=0x%08x ana12=0x%08x ana13=0x%08x ana14=0x%08x\r\n",
		snap->ana11, snap->ana12, snap->ana13, snap->ana14);
	PM_HAL_LOGD("  aon r0=0x%08x r2=0x%08x r40=0x%08x\r\n",
		snap->aon_r0, snap->aon_r2, snap->aon_r40);
	PM_HAL_LOGD("  aon r41=0x%08x r42=0x%08x r7b=0x%08x\r\n",
		snap->aon_r41, snap->aon_r42, snap->aon_r7b);
	PM_HAL_LOGD("  r42 pwd_blp=%u pwd_wlp=%u iso_blp=%u iso_wlp=%u\r\n",
		(snap->aon_r42 >> 0) & 1U, (snap->aon_r42 >> 1) & 1U,
		(snap->aon_r42 >> 2) & 1U, (snap->aon_r42 >> 3) & 1U);
	PM_HAL_LOGD("  r41 halt_lpo=%u sram0=%u sram1=%u sram2=%u mem_ret=%u cache=%u\r\n",
		(snap->aon_r41 >> 24) & 1U, (snap->aon_r41 >> 25) & 1U,
		(snap->aon_r41 >> 26) & 1U, (snap->aon_r41 >> 27) & 1U,
		(snap->aon_r41 >> 28) & 1U, (snap->aon_r41 >> 29) & 1U);
	PM_HAL_LOGD("  r40 halt_volt=%u xtal=%u core=%u flash=%u rosc=%u resten=%u iso=%u clkena=%u\r\n",
		(snap->aon_r40 >> 24) & 1U, (snap->aon_r40 >> 25) & 1U,
		(snap->aon_r40 >> 26) & 1U, (snap->aon_r40 >> 27) & 1U,
		(snap->aon_r40 >> 28) & 1U, (snap->aon_r40 >> 29) & 1U,
		(snap->aon_r40 >> 30) & 1U, (snap->aon_r40 >> 31) & 1U);
	PM_HAL_LOGD("  sys_r10=0x%08x dlv_r7b=%u dlv_r0=%u deep=%u\r\n",
		snap->sys_r10, snap->dlv_r7b, snap->dlv_r0, snap->deep_sleep_entered);
	PM_HAL_LOGD("  ckmn live ctrl=0x%08x rc32k=0x%08x corr=0x%08x\r\n",
		snap->ckmn_ctrl, snap->ckmn_rc32k_ctrl, snap->ckmn_corr_cfg);
	PM_HAL_LOGD("  ckmn bkp  ctrl=0x%08x rc32k=0x%08x corr=0x%08x\r\n",
		snap->ckmn_bkp_ctrl, snap->ckmn_bkp_rc32k, snap->ckmn_bkp_corr);
	PM_HAL_LOGD("  aon_wdt_ctrl=0x%08x gpio_int_mask=0x%08x\r\n",
		snap->aon_wdt_ctrl, snap->gpio_int_mask);
	PM_HAL_LOGD("  gpio_intsta 0=0x%08x 1=0x%08x 2=0x%08x\r\n",
		snap->gpio_intsta0, snap->gpio_intsta1, snap->gpio_intsta2);
	PM_HAL_LOGD("  gpio22_cfg=0x%08x gpio23_cfg=0x%08x gpio24_cfg=0x%08x\r\n",
		snap->gpio22_cfg, snap->gpio23_cfg, snap->gpio24_cfg);
	PM_HAL_LOGD("  ana8=0x%08x ana11=0x%08x\r\n",
		snap->ana8_val, snap->ana11_val);
	PM_HAL_LOGD("  audio: ana20=0x%08x ana21=0x%08x ana27=0x%08x\r\n",
		snap->ana20, snap->ana21, snap->ana27);
	PM_HAL_LOGD("  audio: ana28=0x%08x ana29=0x%08x ana30=0x%08x\r\n",
		snap->ana28, snap->ana29, snap->ana30);
	PM_HAL_LOGD("  touch: ana32=0x%08x  auxldo: ana41=0x%08x\r\n",
		snap->ana32, snap->ana41);
	PM_HAL_LOGD("  usb/cmp: ana42=0x%08x ana43=0x%08x\r\n",
		snap->ana42, snap->ana43);
	PM_HAL_LOGD("  sys_reg0xf(peri_mem_ret)=0x%08x\r\n", snap->sys_reg0xf);
	PM_HAL_LOGD("  sys_reg0xc(dev_clk_en)=0x%08x  r2 m55_iso=%u m55_rstn=%u m55_mem_ret=%u\r\n",
		snap->sys_reg0xc,
		(snap->aon_r2 >> 16) & 1U, (snap->aon_r2 >> 17) & 1U,
		(snap->aon_r2 >> 18) & 1U);
	PM_HAL_LOGD("  r2 m55_clk=%u mem_pwd[3:6]=%u%u%u%u cache_pwd[2:3]=%u%u mem_auto=%u auto_sel=%u\r\n",
		(snap->aon_r2 >> 20) & 1U,
		(snap->aon_r2 >> 19) & 1U, (snap->aon_r2 >> 21) & 1U,
		(snap->aon_r2 >> 22) & 1U, (snap->aon_r2 >> 23) & 1U,
		(snap->aon_r2 >> 24) & 1U, (snap->aon_r2 >> 25) & 1U,
		(snap->aon_r2 >> 28) & 1U, (snap->aon_r2 >> 29) & 1U);
	if (snap->ldo_log_valid) {
		PM_HAL_LOGD("  lv aon ldo: pre=%u sleep_cfg=%u after_sleep=%u ramp=%u backup=%u after_ana_restore=%u final=%u\r\n",
			snap->ldo_pre, snap->ldo_sleep_cfg, snap->ldo_after_sleep_set,
			snap->ldo_after_ramp, snap->ldo_backup, snap->ldo_after_ana_restore,
			snap->ldo_final);
	}
}

/* ---- auto-diff: compare entry#N with entry#1 ---- */
static struct {
	sys_pm_lv_aon_snap_t pre;
	sys_pm_lv_aon_snap_t at;
	sys_pm_lv_aon_snap_t post;
	uint8_t valid;
} s_lv_aon_first;

typedef struct { const char *name; uint32_t off; } snap_field_t;

#define SNAP_FIELD(n) { #n, (uint32_t)(offsetof(sys_pm_lv_aon_snap_t, n)) }

static const snap_field_t s_snap_fields[] = {
	SNAP_FIELD(ana0), SNAP_FIELD(ana2), SNAP_FIELD(ana3), SNAP_FIELD(ana5),
	SNAP_FIELD(ana7), SNAP_FIELD(ana8), SNAP_FIELD(ana9), SNAP_FIELD(valoldosel),
	SNAP_FIELD(ana10), SNAP_FIELD(ana11), SNAP_FIELD(ana12), SNAP_FIELD(ana13), SNAP_FIELD(ana14),
	SNAP_FIELD(aon_r0), SNAP_FIELD(aon_r2), SNAP_FIELD(aon_r40), SNAP_FIELD(aon_r41),
	SNAP_FIELD(aon_r42), SNAP_FIELD(aon_r7b), SNAP_FIELD(sys_r10),
	SNAP_FIELD(dlv_r7b), SNAP_FIELD(dlv_r0), SNAP_FIELD(deep_sleep_entered),
	SNAP_FIELD(ckmn_ctrl), SNAP_FIELD(ckmn_rc32k_ctrl), SNAP_FIELD(ckmn_corr_cfg),
	SNAP_FIELD(ckmn_bkp_ctrl), SNAP_FIELD(ckmn_bkp_rc32k), SNAP_FIELD(ckmn_bkp_corr),
	SNAP_FIELD(aon_wdt_ctrl), SNAP_FIELD(gpio_int_mask),
	SNAP_FIELD(gpio_intsta0), SNAP_FIELD(gpio_intsta1), SNAP_FIELD(gpio_intsta2),
	SNAP_FIELD(gpio22_cfg), SNAP_FIELD(gpio23_cfg), SNAP_FIELD(gpio24_cfg),
	SNAP_FIELD(ana8_val), SNAP_FIELD(ana11_val),
	SNAP_FIELD(ana20), SNAP_FIELD(ana21), SNAP_FIELD(ana27),
	SNAP_FIELD(ana28), SNAP_FIELD(ana29), SNAP_FIELD(ana30),
	SNAP_FIELD(ana32), SNAP_FIELD(ana41), SNAP_FIELD(ana42), SNAP_FIELD(ana43),
	SNAP_FIELD(sys_reg0xf), SNAP_FIELD(sys_reg0xc),
};

static void sys_hal_lv_aon_snap_diff(const char *stage,
	const sys_pm_lv_aon_snap_t *cur, const sys_pm_lv_aon_snap_t *base)
{
	uint8_t any = 0;
	for (uint32_t i = 0; i < sizeof(s_snap_fields)/sizeof(s_snap_fields[0]); i++) {
		uint32_t cv = *(const uint32_t*)((const uint8_t*)cur + s_snap_fields[i].off);
		uint32_t bv = *(const uint32_t*)((const uint8_t*)base + s_snap_fields[i].off);
		if (cv != bv) {
			if (!any) {
				PM_HAL_LOGD("[LV AON diff %s] entry#%u vs entry#1:\r\n",
					stage, cur->entry_cnt);
				any = 1;
			}
			PM_HAL_LOGD("  %s: 0x%08x -> 0x%08x\r\n",
				s_snap_fields[i].name, bv, cv);
		}
	}
	if (!any) {
		PM_HAL_LOGD("[LV AON diff %s] entry#%u IDENTICAL to entry#1\r\n",
			stage, cur->entry_cnt);
	}
}

void sys_hal_lv_aon_debug_flush(void)
{
	if (s_lv_aon_debug.pre_valid &&
		s_lv_aon_debug.pre.magic == SYS_PM_LV_AON_SNAP_MAGIC) {
		sys_hal_lv_aon_snap_print("pre-sleep", &s_lv_aon_debug.pre);
	}
	if (s_lv_aon_debug.at_valid &&
		s_lv_aon_debug.at.magic == SYS_PM_LV_AON_SNAP_MAGIC) {
		sys_hal_lv_aon_snap_print("at-sleep", &s_lv_aon_debug.at);
	}
	if (s_lv_aon_debug.post_valid &&
		s_lv_aon_debug.post.magic == SYS_PM_LV_AON_SNAP_MAGIC) {
		sys_hal_lv_aon_snap_print("post-wake", &s_lv_aon_debug.post);
	}

	/* save entry#1 baseline for diff */
	if (s_lv_sleep_enter_cnt == 1 && !s_lv_aon_first.valid) {
		if (s_lv_aon_debug.pre_valid) s_lv_aon_first.pre = s_lv_aon_debug.pre;
		if (s_lv_aon_debug.at_valid)  s_lv_aon_first.at  = s_lv_aon_debug.at;
		if (s_lv_aon_debug.post_valid) s_lv_aon_first.post = s_lv_aon_debug.post;
		s_lv_aon_first.valid = 1;
	} else if (s_lv_sleep_enter_cnt > 1 && s_lv_aon_first.valid) {
		PM_HAL_LOGD("[LV AON diff] === entry#%u vs entry#1 ===\r\n",
			s_lv_sleep_enter_cnt);
		if (s_lv_aon_debug.pre_valid && s_lv_aon_first.pre.magic == SYS_PM_LV_AON_SNAP_MAGIC)
			sys_hal_lv_aon_snap_diff("pre-sleep", &s_lv_aon_debug.pre, &s_lv_aon_first.pre);
		if (s_lv_aon_debug.at_valid && s_lv_aon_first.at.magic == SYS_PM_LV_AON_SNAP_MAGIC)
			sys_hal_lv_aon_snap_diff("at-sleep", &s_lv_aon_debug.at, &s_lv_aon_first.at);
		if (s_lv_aon_debug.post_valid && s_lv_aon_first.post.magic == SYS_PM_LV_AON_SNAP_MAGIC)
			sys_hal_lv_aon_snap_diff("post-wake", &s_lv_aon_debug.post, &s_lv_aon_first.post);
	}

	s_lv_aon_debug.pre_valid = 0;
	s_lv_aon_debug.at_valid = 0;
	s_lv_aon_debug.post_valid = 0;
}
#endif
