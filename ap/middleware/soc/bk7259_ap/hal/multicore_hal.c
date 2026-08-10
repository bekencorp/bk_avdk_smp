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
#include "ram_regions.h"
#include "sys_driver.h"
#include "aon_pmu_ll.h"
#include "sys_ahbp_ll.h"
#include "bk_misc.h"
#include "wwdt_hal.h"
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
#include "cache.h"
#include "soc/soc.h"
#include "multicore_driver.h"
#endif

#if CONFIG_SOC_SMP
extern uint32_t __vector_core1_table;
#if CONFIG_SPE && CONFIG_PM_AP_FAST_BOOT_ENABLE
#define CPU3_BOOT_VECTOR_WORDS (__VECTOR_SYS_ENTRY_CNT + __INT_NUMBER_MAX)
/*
 * Keep this in normal SRAM BSS. An orphan noinit section can be placed across
 * the end of the 64-KiB AP0 DTCM backup region by the linker. Normal BSS
 * allocation guarantees that both objects have distinct storage; the vector
 * is rebuilt immediately before every CPU3 reset release.
 */
static __attribute__((aligned(256)))
	uint32_t s_cpu3_boot_vector[CPU3_BOOT_VECTOR_WORDS];
#endif
#endif

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
static bool multicore_hal_m55_sram_is_powered(void)
{
	return (aon_pmu_ll_get_r2_m55_mem3_pwd() == 0U) &&
		(aon_pmu_ll_get_r2_m55_mem4_pwd() == 0U) &&
		(aon_pmu_ll_get_r2_m55_mem5_pwd() == 0U) &&
		(aon_pmu_ll_get_r2_m55_mem6_pwd() == 0U) &&
		(aon_pmu_ll_get_r2_m55_cpu2_cache_pwd() == 0U) &&
		(aon_pmu_ll_get_r2_m55_cpu3_cache_pwd() == 0U);
}
#endif

static void multicore_hal_m55_core_init_common(bool reuse_retained_sram)
{
	uint32_t reg_val = 0;

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	if (!reuse_retained_sram || !multicore_hal_m55_sram_is_powered()) {
#endif
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
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
	}
#else
	(void)reuse_retained_sram;
#endif

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
}

void multicore_hal_set_cpu_id(uint32_t cpu_id)
{

}

uint32_t multicore_hal_get_cpu_id(void)
{
	return wwdt_hal_get_cpu_id();
}

bk_err_t multicore_hal_start(uint32_t id)
{
	uint32_t boot_addr = 0;

	switch (id) {
	case CPU1_CORE_ID:
		sys_ll_set_reserver_reg0x33_cpu0_dbgen_l2_rst_dis(1);
		sys_ll_set_reserver_reg0x33_cpu1_dbgen_l2_rst_dis(1);
		boot_addr = SOC_FLASH_DATA_BASE + CONFIG_CP_VIRTUAL_PARTITION_OFFSET;
		sys_drv_set_cpu1_boot_address_offset((boot_addr) >> 8);
		sys_drv_set_cpu1_reset(1);
		break;
	case CPU2_CORE_ID:
		multicore_hal_m55_core_init_common(false);
		boot_addr = SOC_FLASH_DATA_BASE + CONFIG_AP_VIRTUAL_PARTITION_OFFSET;
		sys_ahbp_ll_set_reg4_cpu0_offset((boot_addr) >> 8);
		sys_ahbp_ll_set_reg4_cpu0_init_dtcm_en(1);
		sys_ahbp_ll_set_reg4_cpu0_sw_rstn(1);
		break;
	case CPU3_CORE_ID:
		multicore_hal_m55_core_init_common(
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
			bk_cpu3_fast_resume_is_pending() != 0U
#else
			false
#endif
		);
#if !CONFIG_SPE
		/* Non-Secure build: the secondary core also resets in the Secure state
		 * and must enter the Secure boot shim first, which sets up the SAU and
		 * branches to the core's Non-Secure vector. Pointing it directly at the
		 * Non-Secure vector would execute Non-Secure code in the Secure state. */
		boot_addr = CONFIG_AP_SPE_RAM_ADDR;
#elif CONFIG_SOC_SMP
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
		if (bk_cpu3_fast_resume_is_pending() != 0U) {
			/*
			 * CPU2 owns the retained INT_CONTROLLER_ID_SECONDARY runtime
			 * vector mirror at this VMA. It is restored with AP0 DTCM and
			 * contains registered handlers (notably IRQ3/IPI), unlike the
			 * Flash boot template whose IRQ slots still contain copy/zero
			 * table metadata.
			 */
			const uint32_t *runtime_vector =
				(const uint32_t *)(uintptr_t)&__vector_core1_table;

			for (uint32_t i = 0; i < CPU3_BOOT_VECTOR_WORDS; i++) {
				s_cpu3_boot_vector[i] = runtime_vector[i];
			}
			flush_dcache(s_cpu3_boot_vector, sizeof(s_cpu3_boot_vector));
			__DSB();
			__ISB();
			boot_addr = (uint32_t)(uintptr_t)s_cpu3_boot_vector;
		} else {
			/*
			 * Preserve the proven native cold-boot path. CPU3 sees its
			 * own DTCM at this VMA after the hardware DTCM image init.
			 */
			boot_addr = (uint32_t)(uintptr_t)&__vector_core1_table;
		}
#else
		boot_addr = (uint32_t)&__vector_core1_table;
#endif
#else
		boot_addr = SOC_FLASH_DATA_BASE + CONFIG_AP_VIRTUAL_PARTITION_OFFSET;
#endif
		sys_ahbp_ll_set_reg5_cpu1_sw_rstn(0);
		sys_ahbp_ll_set_reg5_cpu1_offset((boot_addr) >> 8);
#if CONFIG_SPE
		sys_ahbp_ll_set_reg5_cpu1_init_dtcm_en(1);
#else
		/* NS AP: TCM is not used; keep DTCM disabled on the secondary core. */
		sys_ahbp_ll_set_reg5_cpu1_init_dtcm_en(0);
#endif
		sys_ahbp_ll_set_reg5_cpu1_wait(0);
		sys_ahbp_ll_set_reg5_cpu1_wfe_pulse(0);
		sys_ahbp_ll_set_reg5_cpu1_sw_rstn(1);
		break;
	default:
		return BK_ERR_PARAM;
	}

	return BK_OK;
}

bk_err_t multicore_hal_reset(uint32_t id)
{
	switch (id) {
	case CPU2_CORE_ID:
		sys_ahbp_ll_set_reg4_cpu0_sw_rstn(0);
		delay_ms(1);
		sys_ahbp_ll_set_reg4_cpu0_sw_rstn(1);
		break;
	case CPU3_CORE_ID:
		sys_ahbp_ll_set_reg5_cpu1_sw_rstn(0);
		delay_ms(1);
		sys_ahbp_ll_set_reg5_cpu1_sw_rstn(1);
		break;
	default:
		return BK_ERR_PARAM;
	}

	return BK_OK;
}

bk_err_t multicore_hal_stop(uint32_t id)
{
	switch (id) {
	case CPU3_CORE_ID:
		sys_ahbp_ll_set_reg5_cpu1_wfe_pulse(0);
		sys_ahbp_ll_set_reg5_cpu1_wait(0);
		sys_ahbp_ll_set_reg5_cpu1_sw_rstn(0);
		break;
	case CPU2_CORE_ID:
		sys_ahbp_ll_set_reg4_cpu0_wfe_pulse(0);
		sys_ahbp_ll_set_reg4_cpu0_wait(0);
		sys_ahbp_ll_set_reg4_cpu0_sw_rstn(0);
		break;
	default:
		return BK_ERR_PARAM;
	}

	return BK_OK;
}
