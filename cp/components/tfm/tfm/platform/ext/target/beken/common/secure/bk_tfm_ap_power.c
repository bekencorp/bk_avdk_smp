// Copyright     2023-2028 Beken
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

#include "bk_tfm_ap_power.h"
#include "bk_tfm_ap_shim.h"
#include "cmsis.h"
#include "aon_pmu_ll.h"
#include "partitions_gen.h"
#include "sys_ahbp_ll.h"
#include "sys_ll.h"
#include "tfm_flash_partition.h"

extern void bk_delay_us(uint32_t us);

/* PPHS marks SYS Non-Secure before CP enters NS; post-PPHS Secure access to
 * AP SysCfg/AHBP must target the NS alias. */
#define SYS_AHBP_NS_BASE  SOC_GET_NS_ADDR(SOC_SYS_AHBP_REG_BASE)

static inline volatile uint32_t *sys_ahbp_ns_reg(uint32_t idx)
{
	return (volatile uint32_t *)(SYS_AHBP_NS_BASE + (idx << 2));
}

void bk_ap_power_domain_on(void)
{
	uint32_t reg_val;
	uint32_t por_wait;

	/* AP high-speed LDO / power switch bring-up (BL2 only loads the base analog
	 * table, enhspw stays 0). */
	sys_ll_set_ana_reg10_spi_latch1v(1);
	sys_ll_set_ana_reg9_pwd_hsldo(1);
	bk_delay_us(20);
	sys_ll_set_ana_reg9_pwd_hsldo(0);
	bk_delay_us(200);
	sys_ll_set_ana_reg16_enhspw(1);
	bk_delay_us(200);
	sys_ll_set_ana_reg16_vcorehssel(0xA);
	bk_delay_us(200);
	sys_ll_set_ana_reg10_spi_latch1v(0);

	if (aon_pmu_ll_get_r2_m55_auto_sel() == 1) {
		aon_pmu_ll_set_r2_m55_mem_auto_set(0);
	} else {
		aon_pmu_ll_set_r2_m55_mem3_pwd(0);
		bk_delay_us(1000);
		aon_pmu_ll_set_r2_m55_mem4_pwd(0);
		bk_delay_us(1000);
		aon_pmu_ll_set_r2_m55_mem5_pwd(0);
		bk_delay_us(1000);
		aon_pmu_ll_set_r2_m55_mem6_pwd(0);
		bk_delay_us(1000);
		aon_pmu_ll_set_r2_m55_cpu2_cache_pwd(0);
		bk_delay_us(1000);
		aon_pmu_ll_set_r2_m55_cpu3_cache_pwd(0);
		bk_delay_us(1000);
	}

	/* Wait for the AP high-speed power-on reset to de-assert. */
	for (por_wait = 0; por_wait < 100000; por_wait++) {
		if (aon_pmu_ll_get_r74_por_corehs_n() != 0x0) {
			break;
		}
		bk_delay_us(10);
	}

	aon_pmu_ll_set_r2_m55_mem_ret(1);
	aon_pmu_ll_set_r2_m55_iso_en(0);
	aon_pmu_ll_set_r2_m55_clk_en(1);
	aon_pmu_ll_set_r2_m55_rstn(1);

	reg_val = sys_ahbp_ll_get_rega_value();
	reg_val |= 0x3F0001;
	sys_ahbp_ll_set_rega_value(reg_val);

	/* Enable the PSRAM analog LDO; the NS PSRAM driver relies on it. */
	sys_ll_set_ana_reg14_enpsram(1);

	/* AP core clk select + AHBP bus_ls divider. The PSRAM controller sits on the
	 * AHBP bus; its read-capture timing depends on this bus clock. */
	sys_ahbp_ll_set_reg8_cksel_core(1);
	sys_ahbp_ll_set_reg8_ckdiv_core(0);
	sys_ahbp_ll_set_reg8_ckdiv_bus_ls(1);
	bk_delay_us(20);

	__DSB();
	__ISB();
}

void bk_ap_release(void)
{
	/* The AP application is linked Non-Secure but the core resets Secure, so it
	 * boots into the Secure shim (reserved Secure RAM block). The shim switches
	 * the core to the Non-Secure state and branches to the AP Non-Secure vector.
	 * The shim also serves core1 (released later by the AP), branching it to its
	 * own Non-Secure vector selected by core id. */
	uint32_t boot_addr = bk_ap_shim_install(AP_CORE1_NS_VECTOR);
	sys_ahbp_reg4_t *reg4 = (sys_ahbp_reg4_t *)sys_ahbp_ns_reg(0x4u);

	reg4->cpu0_sw_rstn = 0;

	/* Open AP master-access gate (PPRO reg0xF, Secure-only). */
	{
		volatile uint32_t *ppro_cfg = (volatile uint32_t *)0x44050000u;
		ppro_cfg[0xF] &= ~((0x1u << 3) | (0x1u << 2));
	}

	/* AP RAM high-speed EMA (key-protected reg50..53). */
	{
		const uint32_t spsp_cfg = ((0x441u) << 10) | 0x241u;
		const uint32_t stp_cfg = 0x901u;

		*sys_ahbp_ns_reg(0x50u) = (0x5Au << 24) | spsp_cfg;
		*sys_ahbp_ns_reg(0x50u) = (0xA5u << 24) | spsp_cfg;
		*sys_ahbp_ns_reg(0x51u) = (0x5Au << 24) | stp_cfg;
		*sys_ahbp_ns_reg(0x51u) = (0xA5u << 24) | stp_cfg;
		*sys_ahbp_ns_reg(0x52u) = (0x5Au << 24) | spsp_cfg;
		*sys_ahbp_ns_reg(0x52u) = (0xA5u << 24) | spsp_cfg;
		*sys_ahbp_ns_reg(0x53u) = (0x5Au << 24) | stp_cfg;
		*sys_ahbp_ns_reg(0x53u) = (0xA5u << 24) | stp_cfg;
	}

	reg4->cpu0_offset = boot_addr >> 8;
	reg4->cpu0_init_dtcm_en = 1;

	__DSB();
	__ISB();

	reg4->cpu0_sw_rstn = 1;

	__DSB();
	__ISB();
}
