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
#include "bk_tfm_mpc.h"
#include "bk_tfm_ppc.h"
#include "cmsis.h"
#include "aon_pmu_ll.h"
#include "partitions_gen.h"
#include "ram_regions.h"
#include "sys_ahbp_ll.h"
#include "common/bk_err.h"

/* AP HPDMA (DMA1) controller-wide Secure setup, implemented in tfm_hal_platform.c.
 * Runs in the AP power domain, so it is deferred to the AP secure-prepare path. */
void tfm_hal_ap_dma_init(void);

/* CP PPRO marks the CP AON/sys region Non-Secure (aon_ahb_sys_nsec) and AP PPHS
 * marks the AP sys region Non-Secure (ahbp_ahb_sys_nsec) before/while CP runs NS.
 * Once a region is Non-Secure the secure world must access it through the NS
 * alias; using the secure alias raises a bus fault. */
#define AON_PMU_NS_BASE   SOC_GET_NS_ADDR(SOC_AON_PMU_REG_BASE)
#define SYS_AHBP_NS_BASE  SOC_GET_NS_ADDR(SOC_SYS_AHBP_REG_BASE)

static inline volatile uint32_t *sys_ahbp_ns_reg(uint32_t idx)
{
	return (volatile uint32_t *)(SYS_AHBP_NS_BASE + (idx << 2));
}

bool bk_ap_domain_is_on(void)
{
	volatile aon_pmu_r2_t *r2 =
		(volatile aon_pmu_r2_t *)(AON_PMU_NS_BASE + (0x2u << 2));

	return (r2->m55_iso_en == 0u) && (r2->m55_clk_en == 1u) &&
	       (r2->m55_rstn == 1u);
}

int bk_ap_domain_and_reset_ready(void)
{
	sys_ahbp_reg4_t *reg4 = (sys_ahbp_reg4_t *)sys_ahbp_ns_reg(0x4u);
	sys_ahbp_reg5_t *reg5 = (sys_ahbp_reg5_t *)sys_ahbp_ns_reg(0x5u);

	if (!bk_ap_domain_is_on()) {
		return -1;
	}
	if ((reg4->cpu0_sw_rstn != 0u) || (reg5->cpu1_sw_rstn != 0u)) {
		return -1;
	}

	return 0;
}

int bk_ap_sys_secure_open(void)
{
	/* The AP power domain must already be up (CP NS drives the AON PMU). */
	if (!bk_ap_domain_is_on()) {
		return -1;
	}

	/* Enable the AP AHBP peripheral clocks before touching the AP MPC/PPHS. The
	 * AP MPC controllers and the PPHS live on the AP AHBP sub-bus, which is
	 * unclocked right after the domain powers up; accessing them first raises a
	 * bus fault. The AP SYS region is still Secure here (the PPHS runs below), so
	 * the AHBP clock register is written through the Secure alias. */
	*(volatile uint32_t *)(SOC_SYS_AHBP_REG_BASE + (0xAu << 2)) = 0xFFFFFFFFu;
	__DSB();
	__ISB();

	/* Apply the AP MPC and the AP PPHS from the RAM-cached config (loaded at cold
	 * boot while flash was Secure). The PPHS marks the AP SYS/AHBP region
	 * Non-secure (ahbp_ahb_sys_nsec) so CP NS can program the AP SysCfg
	 * (clock/EMA/freq) through the NS alias, and so the later secure AP prepare
	 * can read AP SYS through the NS alias too. */
	if (bk_mpc_ap_cfg() != BK_OK) {
		return -1;
	}
	if (bk_pphs_apply_ap_config_from_flash() != BK_OK) {
		return -1;
	}

	__DSB();
	__ISB();

	return 0;
}

int bk_ap_secure_resources_prepare(uint32_t *boot_addr)
{
	uint32_t addr;

	/* The AP MPC/PPHS are applied earlier by bk_ap_sys_secure_open() (right after
	 * CP NS powers the AP domain). By now the AP SYS clocks are up, so program the
	 * AP HPDMA (DMA1) controller-wide Secure registers here: this lives in the AP
	 * power domain and could not be set during TF-M init (domain off). The soft
	 * reset clears secure_attr so the NS AP can program the DMA channels. */
	tfm_hal_ap_dma_init();

	/* Install the verified boot shim into the reserved Secure RAM block. The boot
	 * target comes from compile-time partition info, never from the Non-Secure
	 * caller. */
	addr = bk_ap_shim_install(AP_CORE1_NS_VECTOR);
	if (addr != (uint32_t)CONFIG_AP_SPE_RAM_ADDR) {
		return -1;
	}

	__DSB();
	__ISB();

	/* The AP core stays in reset; CP NS programs the boot offset and releases
	 * it, and owns the AP master security attribute (PPRO reg0xF). */
	*boot_addr = addr;
	return 0;
}