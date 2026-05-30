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

#include "multicore_hal.h"
#include "cpu_id.h"
#include "partitions.h"
#include "sys_driver.h"
#include "aon_pmu_ll.h"
#include "sys_ahbp_ll.h"
#include "bk_misc.h"
#include "wwdt_hal.h"
#include "soc/soc.h"
#include "cmsis_gcc.h"
#include <stdint.h>
#include <string.h>
#include "cache.h"
#include "sdkconfig.h"

#if CONFIG_SOC_SMP
extern uint32_t __vector_core1_table;
#endif

#if defined(__GNUC__) && !defined(__clang__)
#define BK_OPTIMIZE_O3 __attribute__((optimize("-O3")))
#else
#define BK_OPTIMIZE_O3
#endif

#define M55S_RAM_EMA_KEY_POS             (24U)
#define M55S_RAM_EMA_UNLOCK_KEY          (0x5AU)
#define M55S_RAM_EMA_LOCK_KEY            (0xA5U)
#define M55S_RAM_EMA_SP_CFG_POS          (10U)
#define M55S_RAM_EMA_SPS_CFG             (0x241U)
#define M55S_RAM_EMA_SPB_CFG             (0x441U)
#define M55S_RAM_EMA_STP_CFG             (0x901U)
#define M55S_RAM_EMA_SPSP_CFG            ((M55S_RAM_EMA_SPB_CFG << M55S_RAM_EMA_SP_CFG_POS) | M55S_RAM_EMA_SPS_CFG)
#define M55S_RAM_EMA_SET_KEY(key)        ((key) << M55S_RAM_EMA_KEY_POS)

static void multicore_hal_m55s_ram_ema_switch_to_high_speed(void)
{
	uint32_t spsp_cfg = M55S_RAM_EMA_SPSP_CFG;
	uint32_t stp_cfg = M55S_RAM_EMA_STP_CFG;

	sys_ahbp_ll_set_reg50_value(M55S_RAM_EMA_SET_KEY(M55S_RAM_EMA_UNLOCK_KEY) | spsp_cfg);
	sys_ahbp_ll_set_reg50_value(M55S_RAM_EMA_SET_KEY(M55S_RAM_EMA_LOCK_KEY) | spsp_cfg);
	sys_ahbp_ll_set_reg51_value(M55S_RAM_EMA_SET_KEY(M55S_RAM_EMA_UNLOCK_KEY) | stp_cfg);
	sys_ahbp_ll_set_reg51_value(M55S_RAM_EMA_SET_KEY(M55S_RAM_EMA_LOCK_KEY) | stp_cfg);
	sys_ahbp_ll_set_reg52_value(M55S_RAM_EMA_SET_KEY(M55S_RAM_EMA_UNLOCK_KEY) | spsp_cfg);
	sys_ahbp_ll_set_reg52_value(M55S_RAM_EMA_SET_KEY(M55S_RAM_EMA_LOCK_KEY) | spsp_cfg);
	sys_ahbp_ll_set_reg53_value(M55S_RAM_EMA_SET_KEY(M55S_RAM_EMA_UNLOCK_KEY) | stp_cfg);
	sys_ahbp_ll_set_reg53_value(M55S_RAM_EMA_SET_KEY(M55S_RAM_EMA_LOCK_KEY) | stp_cfg);
}

static void multicore_hal_m55_core_init_common(void)
{
	uint32_t reg_val = 0;
	volatile uint32_t *ppro_cfg = (volatile uint32_t *)0x44050000;

	if (aon_pmu_ll_get_r2_m55_auto_sel() == 1) {
		aon_pmu_ll_set_r2_m55_mem_auto_set(0); // m55 power seq on
	} else {
		aon_pmu_ll_set_r2_m55_mem3_pwd(0); // mem3 power on
		delay_ms(1);
		aon_pmu_ll_set_r2_m55_mem4_pwd(0); // mem4 power on
		delay_ms(1);
		aon_pmu_ll_set_r2_m55_mem5_pwd(0); // mem5 power on
		delay_ms(1);
		aon_pmu_ll_set_r2_m55_mem6_pwd(0); // mem6 power on
		delay_ms(1);
		aon_pmu_ll_set_r2_m55_cpu2_cache_pwd(0); // cpu2 power on
		delay_ms(1);
		aon_pmu_ll_set_r2_m55_cpu3_cache_pwd(0); // cpu3 power on
		delay_ms(1);
	}

	// reg_val = sys_ll_get_ana_reg10_value();
	// reg_val |= BIT(19);
	// sys_ll_set_ana_reg10_value(reg_val);

	while(aon_pmu_ll_get_r74_por_corehs_n() == 0x0);

	aon_pmu_ll_set_r2_m55_mem_ret(1); // mem ret release
	aon_pmu_ll_set_r2_m55_iso_en(0); // iso release
	aon_pmu_ll_set_r2_m55_clk_en(1); // clk enable
	aon_pmu_ll_set_r2_m55_rstn(1); // rstn release

	reg_val = sys_ahbp_ll_get_rega_value();
	reg_val |= 0x3F0001; // open cpu clk
	sys_ahbp_ll_set_rega_value(reg_val);

	/* M55S Access Secure - configure PPRO register */
	reg_val = ppro_cfg[0xF];
	reg_val &= ~((0x1 << 3) | (0x1 << 2));
	reg_val |= ((0 << 3) | (0 << 2));
	ppro_cfg[0xF] = reg_val;

	/* PSRAM Enable */
	sys_ll_set_ana_reg14_enpsram(1);

	/* M55S Memory EMA switch to 1 */
	multicore_hal_m55s_ram_ema_switch_to_high_speed();
}

#define AP_DTCM_BASE 0x20000000
#define AP_DTCM_SIZE 0x10000
#define AP_DTCM_OFFSET 0x08200000
static uint32_t addr_is_ap_dtcm(uint32_t addr)
{
	return (addr >= AP_DTCM_BASE) && (addr < (AP_DTCM_BASE + AP_DTCM_SIZE));
}

extern void data_copy_block_ram(uint32_t *dest, const uint32_t *src, uint32_t word_cnt);

BK_OPTIMIZE_O3 __IRAM_SEC
static void multicore_hal_m55_core_copy_code_and_data(uint32_t boot_addr, bool is_init_dtcm)
{
	enum {
		COPY_TABLE_START_OFFSET = (17U * sizeof(uint32_t)),
		COPY_TABLE_END_OFFSET = (18U * sizeof(uint32_t)),
		ZERO_TABLE_START_OFFSET = (19U * sizeof(uint32_t)),
		ZERO_TABLE_END_OFFSET = (20U * sizeof(uint32_t)),
		COPY_TABLE_ENTRY_SIZE = (3U * sizeof(uint32_t)),
		ZERO_TABLE_ENTRY_SIZE = (2U * sizeof(uint32_t)),
	};

	uint32_t copy_table_start = REG_READ(boot_addr + COPY_TABLE_START_OFFSET);
	uint32_t copy_table_end = REG_READ(boot_addr + COPY_TABLE_END_OFFSET);

	SOC_LOGD("copy_table_start: 0x%08X, copy_table_end: 0x%08X\n", copy_table_start, copy_table_end);
	if ((copy_table_start == 0U) || (copy_table_end == 0U) ||
		(copy_table_end < copy_table_start) ||
		((copy_table_start & 0x3U) != 0U) || ((copy_table_end & 0x3U) != 0U) ||
		(((copy_table_end - copy_table_start) % COPY_TABLE_ENTRY_SIZE) != 0U)) {
		SOC_LOGE("invalid m55 copy table: start=0x%08x end=0x%08x\r\n",
				 copy_table_start, copy_table_end);
		return;
	}

	for (uint32_t table = copy_table_start; table < copy_table_end; table += COPY_TABLE_ENTRY_SIZE) {
		uint32_t data_src = REG_READ(table);
		uint32_t data_dst = REG_READ(table + sizeof(uint32_t));
		uint32_t data_wlen = REG_READ(table + (2U * sizeof(uint32_t)));

		if (data_wlen == 0U) {
			continue;
		}

		if (addr_is_ap_dtcm(data_dst)) {
			if (is_init_dtcm) {
			data_dst += AP_DTCM_OFFSET;
			SOC_LOGD("dtcm data_src: 0x%08X, data_dst: 0x%08X, data_wlen: 0x%08X\n",
				data_src, data_dst, data_wlen);
			} else {
				continue;
			}
		} else {
			if (is_init_dtcm) {
				continue;
			}
			SOC_LOGD("data_src: 0x%08X, data_dst: 0x%08X, data_wlen: 0x%08X\n",
					data_src, data_dst, data_wlen);
		}

		data_copy_block_ram((void *)(uintptr_t)data_dst, (void *)(uintptr_t)data_src,
			   (size_t)data_wlen);
	}

	if (!is_init_dtcm) {
		uint32_t zero_table_start = REG_READ(boot_addr + ZERO_TABLE_START_OFFSET);
		uint32_t zero_table_end = REG_READ(boot_addr + ZERO_TABLE_END_OFFSET);

		if ((zero_table_start == 0U) || (zero_table_end == 0U) ||
			(zero_table_end < zero_table_start) ||
			((zero_table_start & 0x3U) != 0U) || ((zero_table_end & 0x3U) != 0U) ||
			(((zero_table_end - zero_table_start) % ZERO_TABLE_ENTRY_SIZE) != 0U)) {
			SOC_LOGE("invalid m55 zero table: start=0x%08x end=0x%08x\r\n",
					zero_table_start, zero_table_end);
			return;
		}

		for (uint32_t table = zero_table_start; table < zero_table_end; table += ZERO_TABLE_ENTRY_SIZE) {
			uint32_t bss_addr = REG_READ(table);
			uint32_t bss_wlen = REG_READ(table + sizeof(uint32_t));

			if (bss_wlen == 0U) {
				continue;
			}

			// SOC_LOGD("bss_addr: 0x%08X, bss_wlen: 0x%08X\n", bss_addr, bss_wlen);
			memset((void *)(uintptr_t)bss_addr, 0,
				(size_t)bss_wlen * sizeof(uint32_t));
		}
	}

#if CONFIG_DCACHE
	flush_all_dcache();
#endif

	/*
	 * Ensure the initialization writes are globally visible before releasing
	 * the M55 reset.
	 */
	__DSB();
	__ISB();
}

void multicore_hal_set_cpu_id(uint32_t cpu_id)
{

}

uint32_t multicore_hal_get_cpu_id(void)
{
	return wwdt_hal_get_cpu_id();
}

__IRAM_SEC bk_err_t multicore_hal_start(uint32_t id)
{
	uint32_t boot_addr = 0;

	switch (id) {
	case CPU1_CORE_ID:
		sys_ll_set_reserver_reg0x33_cpu0_dbgen_l2_rst_dis(1);
		sys_ll_set_reserver_reg0x33_cpu1_dbgen_l2_rst_dis(1);
#if CONFIG_SOC_SMP
		boot_addr = (uint32_t)&__vector_core1_table;
#else
		boot_addr = SOC_FLASH_DATA_BASE + CONFIG_CP_VIRTUAL_PARTITION_OFFSET;
#endif
		sys_drv_set_cpu1_boot_address_offset((boot_addr) >> 8);
		sys_drv_set_cpu1_reset(1);
		break;
	case CPU2_CORE_ID:
		multicore_hal_m55_core_init_common();
		boot_addr = SOC_FLASH_DATA_BASE + CONFIG_AP_VIRTUAL_PARTITION_OFFSET;
		/*
		 * Keep AP in reset while CP patches the image into AP-visible memories.
		 * On warm boot, sw_rstn may remain released from previous run.
		 */
		sys_ahbp_ll_set_reg4_cpu0_sw_rstn(0);
		sys_ahbp_ll_set_reg4_cpu0_offset((boot_addr) >> 8);
		sys_ahbp_ll_set_reg4_cpu0_init_dtcm_en(1);
		multicore_hal_m55_core_copy_code_and_data(boot_addr, false);
		sys_ahbp_ll_set_reg4_cpu0_sw_rstn(1);

		sys_ahbp_ll_set_reg4_cpu0_wait(1); // wait for AP to finish init
		sys_ahbp_ll_set_reg4_cpu0_init_dtcm_en(1);
		multicore_hal_m55_core_copy_code_and_data(boot_addr, true);
		sys_ahbp_ll_set_reg4_cpu0_wait(0); // wait for AP to finish init
		sys_ahbp_ll_set_reg4_cpu0_sw_rstn(1);
		break;
	case CPU3_CORE_ID:
		multicore_hal_m55_core_init_common();
		boot_addr = SOC_FLASH_DATA_BASE + CONFIG_AP_VIRTUAL_PARTITION_OFFSET;
		sys_ahbp_ll_set_reg5_cpu1_sw_rstn(0);
		sys_ahbp_ll_set_reg5_cpu1_offset((boot_addr) >> 8);
		sys_ahbp_ll_set_reg5_cpu1_init_dtcm_en(1);
		multicore_hal_m55_core_copy_code_and_data(boot_addr, false);
		sys_ahbp_ll_set_reg5_cpu1_sw_rstn(1);

		sys_ahbp_ll_set_reg5_cpu1_wait(1); // wait for AP to finish init
		sys_ahbp_ll_set_reg5_cpu1_init_dtcm_en(1);
		multicore_hal_m55_core_copy_code_and_data(boot_addr, true);
		sys_ahbp_ll_set_reg5_cpu1_wait(0); // wait for AP to finish init
		sys_ahbp_ll_set_reg5_cpu1_sw_rstn(1);
		break;
	default:
		break;
	}

	return BK_OK;
}

bk_err_t multicore_hal_reset(uint32_t id)
{
	return BK_OK;
}

bk_err_t multicore_hal_stop(uint32_t id)
{
	return BK_OK;
}
