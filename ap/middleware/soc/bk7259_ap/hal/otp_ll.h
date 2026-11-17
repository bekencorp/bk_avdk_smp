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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#pragma once

#include <soc/soc.h>
#include "hal_port.h"
#include "otp_hw.h"
#include "system_hw.h"
#include <modules/pm.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OTP_LL_REG_BASE(_otp_unit_id)    (SOC_OTP_APB_BASE)
#define OTP2_LL_REG_BASE(_otp_unit_id)    (SOC_OTP_AHB_BASE)
extern void bk_delay_us(UINT32 us);
#if CONFIG_MBEDTLS_ACCELERATOR || CONFIG_TRUSTENGINE
extern bool dubhe_inited;
#endif

/*
 * The OTP APB block sits in the CP-controlled WRLP_ENCP power domain, so the
 * AP PM library cannot switch it on by itself. Every OTP access has to ask the
 * CP to vote the sub domain on first; touching the block while it is unpowered
 * stalls the AP AHB bus and hangs the core.
 */
extern bool bk_pm_mailbox_is_ready(void);
extern bk_err_t bk_pm_module_vote_cp_power_ctrl(pm_power_module_name_e module,
                                                pm_power_module_state_e power_state);

static inline uint32_t otp_ll_check_busy(otp_hw_t *hw)
{
	return hw->status.busy;
}

/*
 * Every OTP wait runs with the FUSE HSPL lock held and interrupts disabled on
 * the calling core, so an unbounded spin can stall the system and trip the
 * watchdog. All waits are time bounded and report an overrun so the caller can
 * fail instead of hanging.
 */
#define OTP_BUSY_TIMEOUT_US       (5 * 1000)

/* One-shot factory/ATE commands (auto repair, PUF enroll, off-check, lock
 * bits) may take tens of milliseconds, keep a wider bound for them. */
#define OTP_LONG_BUSY_TIMEOUT_US  (100 * 1000)

static inline int otp_check_busy_timeout(otp_hw_t *hw, uint32_t timeout_us)
{
	uint32_t waited = 0;

	while (otp_ll_check_busy(hw)) {
		if (waited >= timeout_us) {
			BK_LOGE(NULL, "OTP busy timeout: status=0x%x, waited %uus\r\n",
			        hw->status.v, waited);
			return -1;
		}
		bk_delay_us(10);
		waited += 10;
	}
	return 0;
}

static inline int otp_check_busy(otp_hw_t *hw)
{
	return otp_check_busy_timeout(hw, OTP_BUSY_TIMEOUT_US);
}

static inline void otp_ll_sleep(otp_hw_t *hw)
{
	/* OTP1 writes already waited in otp_ll_write_otp(), but OTP2 (AHB) writes
	 * do not: otp2_ll_write_otp() only stores the data word and lets the AHB
	 * engine program asynchronously. Clearing pdstb/clkosc_en while that
	 * programming is still in flight leaves the row half written, and the
	 * read-back check in bk_otp_ahb_update() cannot detect it. Use the long
	 * bound because factory/ATE paths can program slowly. */
	(void)otp_check_busy_timeout(hw, OTP_LONG_BUSY_TIMEOUT_US);
	hw->hardware.pdstb = 0;
	hw->hardware.clkosc_en = 0;
}

static inline int otp_ll_active(otp_hw_t *hw)
{
	hw->hardware.pdstb = 1;
	hw->hardware.clkosc_en = 1;
	return otp_check_busy_timeout(hw, OTP_BUSY_TIMEOUT_US);
}

/*
 * Blocking part: ask the CP to power the OTP APB sub domain. The CP mailbox
 * round trip can take milliseconds, so it must be done OUTSIDE the FUSE HSPL
 * lock.
 */
static inline int otp_ll_power_on(otp_hw_t *hw)
{
	(void)hw;
	if (!bk_pm_mailbox_is_ready() ||
	    (bk_pm_module_vote_cp_power_ctrl(PM_POWER_SUB_DOMAIN_OTP_APB,
	                                     PM_POWER_MODULE_STATE_ON) != BK_OK)) {
		return -1;
	}
	return 0;
}

static inline void otp_ll_power_off(otp_hw_t *hw)
{
	(void)hw;
#if CONFIG_MBEDTLS_ACCELERATOR || CONFIG_TRUSTENGINE
	/* Keep the domain powered while dubhe/TE200 has not been initialized and
	 * may still own the OTP block. The CP side uses the same rule. */
	if (dubhe_inited == false) {
		return ;
	}
#endif
	bk_pm_module_vote_cp_power_ctrl(PM_POWER_SUB_DOMAIN_OTP_APB, PM_POWER_MODULE_STATE_OFF);
}

/*
 * Register-level part. The clock gate and the standby bits belong to the OTP
 * controller shared with the CP, so both sides run these with the
 * BK_HSPL_RES_FUSE HSPL lock held.
 */
static inline int otp_ll_hw_active(otp_hw_t *hw)
{
	sys_ll_set_cpu_device_clk_enable_otp_cken(1);
	bk_delay_us(10);
	return otp_ll_active(hw);
}

static inline void otp_ll_hw_sleep(otp_hw_t *hw)
{
	otp_ll_sleep(hw);
	sys_ll_set_cpu_device_clk_enable_otp_cken(0);
	bk_delay_us(10);
}

static inline uint32_t otp_ll_read_tmlck(otp_hw_t *hw)
{
	return hw->module_flag.tmlck;
}

static inline uint32_t otp_ll_read_puflck(otp_hw_t *hw)
{
	return hw->module_flag.puflck;
}

static inline uint32_t otp_ll_read_otplck(otp_hw_t *hw)
{
	return hw->module_flag.otplck;
}

static inline uint32_t otp_ll_read_shfwen(otp_hw_t *hw)
{
	return hw->module_flag.shfwen;
}

static inline uint32_t otp_ll_read_shfren(otp_hw_t *hw)
{
	return hw->module_flag.shfren;
}

static inline uint32_t otp_ll_read_pgmprt(otp_hw_t *hw)
{
	return hw->module_flag.pgmprt;
}

static inline uint32_t otp_ll_read_enroll(otp_hw_t *hw)
{
	return hw->module_flag.enroll;
}

static inline void otp_ll_write_otp_security(otp_hw_t *hw, uint32_t location)
{
	hw->sectrl.v |= (0x7 << (location / 64 * 3));
	otp_check_busy(hw);
}

static inline uint32_t otp_ll_read_otp_security(otp_hw_t *hw, uint32_t location)
{
	return hw->sectrl.v >> (location / 64 * 3) & 0x7;
}

static inline void otp_ll_write_puf_security(otp_hw_t *hw, uint32_t location)
{
	hw->sectrl.v |= (0x7 << (location / 8 * 3 + 12));
	otp_check_busy(hw);
}

static inline uint32_t otp_ll_read_puf_security(otp_hw_t *hw, uint32_t location)
{
	return hw->sectrl.v >> (location / 8 * 3 + 12) & 0x7;
}

static inline void otp_ll_enable_security_protection(otp_hw_t *hw)
{
	hw->sectrl.secr_prot_en = 0xF;
	otp_check_busy(hw);
}

static inline uint32_t otp_ll_read_security_protection(otp_hw_t *hw)
{
	return hw->sectrl.secr_prot_en;
}

static inline uint32_t otp_ll_read_puf_zeroized_flag(otp_hw_t *hw, uint32_t location)
{
	return hw->zeroized_puf.v << (location / 8 * 2) & 0x3;
}

static inline uint32_t otp_ll_read_otp_zeroized_flag(otp_hw_t *hw, uint32_t location)
{
	return hw->zeroized_otp[location / 128].v >> (location % 128 / 8 * 2) & 0x3;
}

static inline void otp_ll_write_otp2_permission(otp_hw_t *hw, uint32_t location, uint32_t value)
{
	hw->rolck[location / 256].v |= (value << (location % 256 / 32) * 4);
	otp_check_busy(hw);
}

static inline uint32_t otp_ll_read_otp2_permission(otp_hw_t *hw, uint32_t location)
{
	return (hw->rolck[location / 256].v >> (location % 256 / 32) * 4) & 0xF;
}

static inline void otp_ll_write_puf_permission(otp_hw_t *hw, uint32_t location)
{
	hw->puf_lckwd[location / 8].v |= (0xF << (location % 8) * 4);
	otp_check_busy(hw);
}

static inline uint32_t otp_ll_read_puf_permission(otp_hw_t *hw, uint32_t location)
{
	return hw->puf_lckwd[location / 8].v >> ((location % 8) * 4) & 0xF;
}

static inline void otp_ll_write_otp_permission(otp_hw_t *hw, uint32_t location, uint32_t value)
{
	hw->otp_lckwd[location / 8].v |= (value << (location % 8) * 4);
	otp_check_busy(hw);
}

static inline uint32_t otp_ll_read_otp_permission(otp_hw_t *hw, uint32_t location)
{
	return (hw->otp_lckwd[location / 8].v >> (location % 8 * 4)) & 0xF;
}

static inline uint32_t otp_ll_read_random_number(otp_hw_t *hw)
{
	return hw->random_value.v;
}

static inline uint32_t otp_ll_read_intrpt_status(otp_hw_t *hw)
{
	return hw->intrpt.intrpt_st;
}

static inline void otp_ll_write_intrpt_enable(otp_hw_t *hw, uint32_t value)
{
	hw->intrpt.intrpt_en = value;
}

static inline uint32_t otp_ll_read_intrpt_enable(otp_hw_t *hw)
{
	return hw->intrpt.intrpt_en;
}

static inline void otp_ll_write_otp2_mask(otp_hw_t *hw, uint32_t location, uint32_t mask)
{
	hw->cde_mask[location / 512].v |= mask << (location % 512 / 32 * 2);
	otp_check_busy(hw);
}

static inline uint32_t otp_ll_read_otp2_mask(otp_hw_t *hw, uint32_t location)
{
	return hw->cde_mask[location / 512].v >> (location % 512 / 32 * 2) & 0x3;
}

static inline void otp_ll_write_otp_mask(otp_hw_t *hw, uint32_t location, uint32_t mask)
{
	hw->otp_mask[location / 128].v |= mask << (location % 128 / 8 * 2);
	otp_check_busy(hw);
}

static inline uint32_t otp_ll_read_otp_mask(otp_hw_t *hw, uint32_t location)
{
	return hw->otp_mask[location / 128].v >> (location % 128 / 8 * 2) & 0x3;
}

static inline void otp_ll_write_puf_mask(otp_hw_t *hw, uint32_t location)
{
	hw->puf_mask.v |= 0x3 << (location / 8 * 2);
}

static inline uint32_t otp_ll_read_puf_mask(otp_hw_t *hw, uint32_t location)
{
	return hw->puf_mask.v >> (location / 8 * 2) & 0x3;
}

static inline void otp_ll_enable_mask_lck(otp_hw_t *hw)
{
	hw->puf_mask.lck_psmsk = 0xFF;
}

static inline uint32_t otp_ll_read_mask_lck(otp_hw_t *hw)
{
	return hw->puf_mask.lck_psmsk;
}

static inline uint32_t otp_ll_read_version(otp_hw_t *hw)
{
	return hw->version.v;
}

static inline uint32_t otp_ll_read_status(otp_hw_t *hw)
{
	return hw->status.v & 0xF1F;
}

static inline uint32_t otp_ll_get_failcnt(otp_hw_t *hw)
{
	return hw->status.failcnt;
}

static inline uint32_t otp_ll_do_auto_repair(otp_hw_t *hw)
{
	hw->auto_repair.auto_repair = 0xE9;
	(void)otp_check_busy_timeout(hw, OTP_LONG_BUSY_TIMEOUT_US);
	return hw->status.v & (0xF1F);
}

/*Temperature < 85 centigrade*/
static inline uint32_t otp_ll_init_off_check(otp_hw_t *hw)
{
	hw->ini_off_check.v = 0x5A;
	(void)otp_check_busy_timeout(hw, OTP_LONG_BUSY_TIMEOUT_US);
	return hw->status.v & 0x1F;
}

static inline void otp_ll_enable_off_margin_read(otp_hw_t *hw)
{
	hw->hardware.rd_mode = 0x1;
	(void)otp_check_busy(hw);
}

static inline void otp_ll_disable_off_margin_read(otp_hw_t *hw)
{
	hw->hardware.rd_mode = 0x0;
	(void)otp_check_busy(hw);
}

static inline void otp_ll_enable_on_margin_read(otp_hw_t *hw)
{
	hw->hardware.rd_mode = 0x2;
	(void)otp_check_busy(hw);
}

static inline void otp_ll_disable_on_margin_read(otp_hw_t *hw)
{
	hw->hardware.rd_mode = 0x0;
	(void)otp_check_busy(hw);
}

static inline uint32_t otp_ll_do_puf_quality_check(otp_hw_t *hw)
{
	hw->puf_qty_chk.v = 0x0;
	(void)otp_check_busy_timeout(hw, OTP_LONG_BUSY_TIMEOUT_US);
	return hw->status.v & 0x1F;
}

static inline uint32_t otp_ll_do_puf_health_check(otp_hw_t *hw)
{
	/* OTP reg 0xAA (puf_hck_cfg): vendor-required pre-configuration for the
	 * PUF health check. The dummy read before/after the write keeps the
	 * original APB access sequence used to flush state and commit the
	 * configuration. */
	(void)hw->puf_hck_cfg.v;
	hw->puf_hck_cfg.v = 0x31D0303;
	(void)hw->puf_hck_cfg.v;
	hw->puf_qty_chk.v = 0xdc;
	(void)otp_check_busy_timeout(hw, OTP_LONG_BUSY_TIMEOUT_US);
	return hw->status.v & 0x1F;
}

static inline void otp_ll_do_puf_enroll(otp_hw_t *hw)
{
	if(otp_ll_read_tmlck(hw) == 0 && otp_ll_read_puflck(hw) == 0){
		hw->puf_enroll.v = 0xA7;
	}
	(void)otp_check_busy_timeout(hw, OTP_LONG_BUSY_TIMEOUT_US);
}

static inline void otp_ll_zeroize_puf(otp_hw_t *hw, uint32_t location)
{
	if(otp_ll_read_puflck(hw) == 0){
		switch (location / 8)
		{
		case 0:
			hw->puf_zeroize.v = 0x4B;
			break;

		case 1:
			hw->puf_zeroize.v = 0xAD;
			break;

		case 2:
			hw->puf_zeroize.v = 0xD2;
			break;

		case 3:
			hw->puf_zeroize.v = 0x34;
			break;

		default:
			break;
		}
	}
}

static inline void otp_ll_zeroize_otp(otp_hw_t *hw, uint32_t location)
{
	if(otp_ll_read_puflck(hw) == 0){
		hw->otp_zeroize.v = 0x80 + (location / 8);
	}
}

static inline void otp_ll_set_flag(otp_hw_t *hw, uint32_t flag)
{
	switch (flag)
	{
	case 0xb6:
		/* enable the propragm protect function */
		hw->set_flag.v = 0xB6;
		break;

	case 0x71:
		/* enable the Test Mode Lock function */
		hw->set_flag.v = 0x71;
		break;

	case 0x2c:
		/* enable the PUF Lock function */
		hw->set_flag.v = 0x2C;
		break;

	case 0x65:
		/* enable the OTP Lock function */
		hw->set_flag.v = 0x65;
		break;

	case 0x99:
		/* enable the shuffle function for OTP/PUF read */
		hw->set_flag.v = 0x99;
		break;

	case 0xc2:
		/* enable the shuffle function for OTP/PUF write */
		hw->set_flag.v = 0xC2;
		break;

	default:
		break;
	}
	(void)otp_check_busy_timeout(hw, OTP_LONG_BUSY_TIMEOUT_US);
}

static inline uint32_t otp_ll_read_puf(otp_hw_t *hw, uint32_t location)
{
	return hw->puf[location].v;
}

static inline uint32_t otp_ll_read_puf_entropy(otp_hw_t *hw, uint32_t location)
{
	return hw->puf[location].v;
}

static inline uint32_t otp_ll_read_otp(otp_hw_t *hw, uint32_t location)
{
	return hw->otp[location].v;
}

static inline void otp_ll_write_otp(otp_hw_t *hw, uint32_t location, uint32_t value)
{
	hw->otp[location].v = value;
	otp_check_busy(hw);
}

static inline void otp_ll_enable_pdstb(otp_hw_t *hw)
{
	hw->hardware.pdstb = 1;
}

static inline void otp_ll_enable_clkosc_en(otp_hw_t *hw)
{
	hw->hardware.clkosc_en = 1;
}

static inline void otp_ll_set_fre_cont(otp_hw_t *hw, uint32_t value)
{
	hw->hardware.fre_cont = value;
}

static inline void otp_ll_set_time_to_active(otp_hw_t *hw, uint32_t value)
{
	hw->hardware.rd_to_sel = value;
}

static inline void otp_ll_write_test_row(otp_hw_t *hw, uint32_t location, uint32_t value)
{
	hw->test_row[location].v = value;
}

static inline uint32_t otp_ll_read_test_row(otp_hw_t *hw, uint32_t location)
{
	return hw->test_row[location].v;
}

static inline void otp_ll_write_test_column(otp_hw_t *hw, uint32_t location, uint32_t value)
{
	hw->test_column[location].v = value;
}

static inline uint32_t otp_ll_read_test_column(otp_hw_t *hw, uint32_t location)
{
	return hw->test_column[location].v;
}

static inline void otp_ll_set_ptc_page(otp_hw_t *hw, uint32_t value)
{
	hw->hardware.ptc_page = value;
}

static inline uint32_t otp2_ll_read_otp(otp2_hw_t *hw, uint32_t location)
{
	return hw->otp2[location].v;
}

static inline void otp2_ll_write_otp(otp2_hw_t *hw, uint32_t location, uint32_t value)
{
	hw->otp2[location].v = value;
}

#ifdef __cplusplus
}
#endif