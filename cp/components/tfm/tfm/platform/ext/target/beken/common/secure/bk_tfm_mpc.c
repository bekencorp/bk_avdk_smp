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

#include "components/log.h"
#include "driver/mpc.h"
#include "armino_config.h"
#include "bk_tfm_mpc.h"
#include "cmsis.h"
#include "tfm_flash_partition.h"
#include "partitions_gen.h"
#include "ram_regions.h"
#include "hal_hw_fih.h"
#include "hal_sw_fih.h"
#include "bk_tfm_ppc.h"

#define TAG "mpc"

/* ---------------------------------------------------------------------------
 * AP-domain MPC
 *
 * The AP AHBP MPC controllers live at 0x4821xxxx~0x4829xxxx (step 0x10000) and
 * are only reachable once the AP power domain is up. They are not modelled by
 * the CP MPC driver, so they are programmed by direct register access.
 *
 * The AP runs Non-Secure, so its SRAM/PSRAM/QSPI blocks are marked Non-Secure
 * and the AP reaches them through the Non-Secure aliases (0x38xxxxxx /
 * 0x74xxxxxx). The only exception is the first block of smem3, which is kept
 * Secure to hold the AP boot shim that runs before the core switches to the
 * Non-Secure world.
 * ------------------------------------------------------------------------- */
#define MPC_CTRL_OFF      0x00u
#define MPC_BLK_MAX_OFF   0x10u
#define MPC_BLK_IDX_OFF   0x18u
#define MPC_BLK_LUT_OFF   0x1Cu

#if (CONFIG_AP_SPE_RAM_ADDR != 0x28100000u)
#error "AP_SPE_RAM must start at the smem3 base"
#endif

#if (CONFIG_AP_SPE_RAM_SIZE != 0x1000u)
#error "AP_SPE_RAM must occupy one MPC block"
#endif

/* Index of the AP boot shim controller inside s_ap_mpc_base (smem3). */
#define AP_MPC_SMEM3_IDX  0u

static const uint32_t s_ap_mpc_base[] = {
	0x48210000u, /* smem3 */
	0x48220000u, /* smem4 */
	0x48230000u, /* smem5 */
	0x48240000u, /* smem6 */
	0x48250000u, /* psram0 data 0x60000000 */
	0x48260000u, /* psram1 data 0x64000000 */
	0x48270000u, /* psram virtual / code */
	0x48280000u, /* qspi0 */
	0x48290000u, /* qspi1 */
};

/* Program every block of one AP MPC controller Non-Secure (LUT bit=1 -> NS). */
static void ap_mpc_all_ns(uint32_t base)
{
	volatile uint32_t *ctrl    = (volatile uint32_t *)(base + MPC_CTRL_OFF);
	volatile uint32_t *blk_max = (volatile uint32_t *)(base + MPC_BLK_MAX_OFF);
	volatile uint32_t *blk_idx = (volatile uint32_t *)(base + MPC_BLK_IDX_OFF);
	volatile uint32_t *blk_lut = (volatile uint32_t *)(base + MPC_BLK_LUT_OFF);
	uint32_t max = *blk_max;
	uint32_t i;

	*ctrl &= ~(1u << 8);              /* auto_increase off */
	for (i = 0; i <= max; i++) {
		*blk_idx = i;
		*blk_lut = 0xFFFFFFFFu;   /* 32 blocks per LUT word -> Non-Secure */
	}
}

/* smem3: keep the first block (4K boot shim) Secure, the rest Non-Secure. */
static void ap_mpc_smem3(uint32_t base)
{
	volatile uint32_t *ctrl    = (volatile uint32_t *)(base + MPC_CTRL_OFF);
	volatile uint32_t *blk_max = (volatile uint32_t *)(base + MPC_BLK_MAX_OFF);
	volatile uint32_t *blk_idx = (volatile uint32_t *)(base + MPC_BLK_IDX_OFF);
	volatile uint32_t *blk_lut = (volatile uint32_t *)(base + MPC_BLK_LUT_OFF);
	uint32_t max = *blk_max;
	uint32_t i;

	*ctrl &= ~(1u << 8);              /* auto_increase off */
	for (i = 0; i <= max; i++) {
		*blk_idx = i;
		/* LUT word 0 bit0 -> block0 Secure (shim), all other blocks NS. */
		*blk_lut = (i == 0u) ? 0xFFFFFFFEu : 0xFFFFFFFFu;
	}
}

static void ap_mpc_cfg(void)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(s_ap_mpc_base); i++) {
		if (i == AP_MPC_SMEM3_IDX) {
			ap_mpc_smem3(s_ap_mpc_base[i]);
		} else {
			ap_mpc_all_ns(s_ap_mpc_base[i]);
		}
	}
}

/* Lock each AP-domain MPC so its attributes can no longer be changed until the
 * next reset (ctrl.sec_lock, bit31). The blocks stay Secure. */
#define MPC_CTRL_SEC_LOCK (1u << 31)

static void __attribute__((unused)) ap_mpc_lockdown(void)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(s_ap_mpc_base); i++) {
		volatile uint32_t *ctrl = (volatile uint32_t *)(s_ap_mpc_base[i] + MPC_CTRL_OFF);
		*ctrl |= MPC_CTRL_SEC_LOCK;
	}
}

/* ---------------------------------------------------------------------------
 * CP-domain MPC
 *
 * The CP MPC controllers (FLASH / SMEM0..2 / OTP2) live at 0x448Bxxxx~
 * 0x448Fxxxx and are programmed through the CP MPC driver.
 * ------------------------------------------------------------------------- */
static void flash_mpc_cfg(void)
{
	uint32_t block_sz = bk_mpc_get_block_size(MPC_DEV_FLASH);
	uint32_t max_block_num = 32 * (bk_mpc_get_max_block_index(MPC_DEV_FLASH) + 1);
	uint32_t nspe_phy_offset = partition_get_phy_offset(PARTITION_PRIMARY_CPU0_APP);
	(void)max_block_num; (void)block_sz;

	/* The Non-Secure world is the CPU0 application; mark its flash region
	 * Non-Secure so the SPM can read the NS MSP/VTOR from that alias. */
	BK_LOGI(TAG, "flash NS carve start phy=%x\r\n", nspe_phy_offset);
	if (nspe_phy_offset == 0) {
		BK_LOGI(TAG, "no NS image, skip flash NS carve (flash stays secure)\r\n");
		return;
	}
	/* Carve Non-Secure from the first partition after the TF-M image to the end
	 * of flash: every partition past TF-M is used by Non-Secure masters, so a
	 * Non-Secure erase/write there must not be blocked by the flash controller. */
	{
		uint32_t ns_vir = FLASH_PHY2VIRTUAL(nspe_phy_offset);
		uint32_t start_blk = ns_vir / block_sz;
		uint32_t ns_blk = (start_blk < max_block_num) ? (max_block_num - start_blk) : 0;
		BK_LOGI(TAG, "flash NS: vir=%x start_blk=%x ns_blk=%x max=%x\r\n", ns_vir, start_blk, ns_blk, max_block_num);
		if (ns_blk > 0 && ns_blk <= max_block_num) {
			BK_LOG_ON_ERR(bk_mpc_set_secure_attribute(MPC_DEV_FLASH, ns_vir, ns_blk, MPC_BLOCK_NON_SECURE));
		}
		return;
	}
}

static void ram_mpc_cfg(void)
{
	uint32_t ram_size[] = {KB(128), KB(128), KB(128)};

	uint32_t total_remain_s_size = CONFIG_TFM_RAM_SIZE;
	uint32_t s_size = 0;
	uint32_t s_block_num = 0;
	uint32_t ns_block_num = 0;
	uint32_t max_block_num = 0;
	uint32_t block_sz;
	uint32_t idx;
	uint32_t iter_count = 0;

	/* The CP owns SMEM0..2 (3x128K) as secure-capable RAM MPC blocks; the SPE
	 * carve uses CONFIG_TFM_RAM_SIZE, the remainder is Non-Secure. */
	const uint32_t ram_mpc_dev_end = MPC_DEV_SMEM2 + 1;

	for (uint32_t dev = MPC_DEV_SMEM0; dev < ram_mpc_dev_end; dev++) {
		iter_count++;
		idx = dev - MPC_DEV_SMEM0;
		block_sz = bk_mpc_get_block_size(dev);

		if (total_remain_s_size > ram_size[idx]) {
			s_size = ram_size[idx];
		} else {
			s_size = total_remain_s_size;
		}
		total_remain_s_size -= s_size;

		max_block_num = ram_size[idx] / block_sz;
		s_block_num = s_size / block_sz;
		if (s_block_num > 0) {
			BK_LOG_ON_ERR(bk_mpc_set_secure_attribute(dev, 0, s_block_num, MPC_BLOCK_SECURE));
		}

		ns_block_num = max_block_num - s_block_num;
		if (ns_block_num > 0) {
			BK_LOG_ON_ERR(bk_mpc_set_secure_attribute(dev, s_size, ns_block_num, MPC_BLOCK_NON_SECURE));
		}
	}
	FIH_ASSERT4(iter_count == (ram_mpc_dev_end - MPC_DEV_SMEM0));
}

static void cp_mpc_cfg(void)
{
	BK_LOG_ON_ERR(bk_mpc_driver_init());
	bk_sw_fih_set_data(FIH_SW_INDEX5);
	bk_fih_set_src(FIH_DATA_MPC, 0xaa);

#if CONFIG_TFM_S_JUMP_TO_CPU0_APP || CONFIG_TFM_S_JUMP_TO_TFM_NS
	ram_mpc_cfg();
	flash_mpc_cfg();
#endif

#if (0 == CONFIG_ENABLE_DEBUG)
	BK_LOG_ON_ERR(bk_mpc_lockdown(MPC_DEV_FLASH));
	BK_LOG_ON_ERR(bk_mpc_lockdown(MPC_DEV_SMEM0));
	BK_LOG_ON_ERR(bk_mpc_lockdown(MPC_DEV_SMEM1));
	BK_LOG_ON_ERR(bk_mpc_lockdown(MPC_DEV_SMEM2));
#endif
}

int bk_mpc_cfg(void)
{
	/* The AP power domain is powered on by bk_ap_power_domain_on() (called from
	 * tfm_hal_set_up_static_boundaries before this), so the AP AHBP MPC
	 * instances are accessible here. */
	ap_mpc_cfg();
	cp_mpc_cfg();

	__DSB();
	__ISB();

	return BK_OK;
}
