// Copyright 2020-2021 Beken
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

/*
 * CP middleware flash driver: thin adapter over the portable flash_core.
 * The flash table and the erase/write/protect algorithm live in flash_core;
 * this file only supplies the NS concurrency port (HSPL + mailbox notify), the
 * CP hardware init (clock / 2-wire / residual drain) and the partition
 * write-permission policy.
 */

#include <common/bk_include.h>
#include <components/ate.h>
#include <driver/flash.h>
#include <os/os.h>
#include "flash_driver.h"
#include "flash_hal.h"
#include "sys_driver.h"
#include "driver/flash_partition.h"
#include <modules/chip_support.h>
#include "aspl_lock.h"
#ifdef CONFIG_HSPL
#include "hspl_res_lock.h"
#endif

static bool s_flash_is_init = false;
static uint32_t s_flash_dev_version = 0;

extern bk_err_t    mb_flash_ipc_init(void);
extern bk_err_t    mb_flash_op_prepare(void);
extern bk_err_t    mb_flash_op_finish(void);

extern bk_err_t    bk_flash_partition_write_perm_check_by_addr(uint32_t addr, uint32_t size, uint32_t magic_code);

/* ------------------------------------------------------------------------- */
/* flash_core port: NS concurrency (HSPL + rtos_disable_int) + mb notify      */
/* ------------------------------------------------------------------------- */

static void cp_flash_op_prepare(void)
{
	mb_flash_op_prepare();  /* coordinate LCD etc. */
}

static void cp_flash_op_finish(void)
{
	mb_flash_op_finish();
}

static const flash_core_port_t s_cp_flash_port = {
	.enter_critical = bk_aspl_flash_enter_critical,
	.exit_critical  = bk_aspl_flash_exit_critical,
	.op_prepare     = cp_flash_op_prepare,
	.op_finish      = cp_flash_op_finish,
};

static bool cp_flash_perm_cb(uint32_t addr, uint32_t size)
{
	return bk_flash_partition_write_perm_check_by_addr(addr, size, FLASH_API_MAGIC_CODE) == BK_OK;
}

/* ------------------------------------------------------------------------- */

flash_line_mode_t bk_flash_get_line_mode(void)
{
	return flash_core_get_line_mode();
}

uint8_t bk_flash_get_coutinuous_read_mode(void)
{
	return flash_core_get_continuous_read_mode();
}

/* Kept as a stable (non-static) symbol; forwards to the core. */
bk_err_t flash_clear_rd_residual_data(flash_hal_t *hal)
{
	(void)hal;
	return flash_core_clear_rd_residual();
}

bk_err_t bk_flash_driver_init(void)
{
	flash_hal_t *hal_ptr;

	if (s_flash_is_init) {
		return BK_OK;
	}

	bk_err_t ret_code = mb_flash_ipc_init();  /* used for projects with LCD. */
	if(ret_code != BK_OK)
		return ret_code;

	flash_core_set_port(&s_cp_flash_port);
	flash_core_set_perm_cb(cp_flash_perm_cb);
	flash_core_hal_init();
	hal_ptr = flash_core_hal();

	s_flash_dev_version = flash_hal_get_dev_version(hal_ptr);
	FLASH_LOGI("dev_version=0x%x\r\n", s_flash_dev_version);

	flash_core_cpu_wr_disable();

	uint32_t int_level = flash_core_enter_critical();

	flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

	if (!flash_core_identify()) {
		for(int i = 0; i < 10; i++) {
			FLASH_LOGE("This flash is not identified, choose default config\r\n");
		}
	}
	FLASH_LOGI("id=0x%x\r\n", flash_core_get_id());

	flash_hal_set_quad_m_value(hal_ptr, flash_core_get_continuous_read_mode());

	/* Enable flash write-protect at boot and persist it across reboot. */
	flash_core_set_runtime_protect_type(FLASH_PROTECT_ALL);
	flash_core_protect_nvol();

	flash_core_set_line_mode(flash_core_get_line_mode());

	flash_core_exit_critical(int_level);

	/* BK7259 flash ctrl (dev 0x20000): 320M/4 = 80M */
	if ((3 != sys_drv_flash_get_clk_sel()) || (3 != sys_drv_flash_get_clk_div())) {
		sys_drv_flash_set_clk_div(3);
		sys_drv_flash_cksel(3);
	}

	sys_drv_set_sys2flsh_2wire(1);
	/* If start flash read, and the 8-word data was not fully read out
	 * from the flash fifo. and then a next flash read is initiated again
	 * the data will no be read correctly for hw fifo pointer is wrong.
	 */
	flash_core_clear_rd_residual();

	s_flash_is_init = true;

#if CONFIG_FLASH_TEST
    int bk_flash_wr_register_cli_test_feature(void);
    bk_flash_wr_register_cli_test_feature();
#endif

    return BK_OK;
}


bk_err_t bk_flash_driver_deinit(void)
{
	if (!s_flash_is_init) {
		return BK_OK;
	}

	s_flash_is_init = false;

	return BK_OK;
}

bk_err_t bk_flash_erase_sector(uint32_t address)
{
	if (address >= flash_core_get_total_size()) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	return flash_core_erase(address & (~(FLASH_SECTOR_SIZE - 1)), FLASH_SECTOR_SIZE);
}

bk_err_t bk_flash_erase_32k(uint32_t address)
{
	if (address >= flash_core_get_total_size()) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	return flash_core_erase(address & (~(FLASH_BLOCK32_SIZE - 1)), FLASH_BLOCK32_SIZE);
}

bk_err_t bk_flash_erase_block(uint32_t address)
{
	if (address >= flash_core_get_total_size()) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	return flash_core_erase(address & (~(FLASH_BLOCK_SIZE - 1)), FLASH_BLOCK_SIZE);
}

bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	if (address >= flash_core_get_total_size()) {
		FLASH_LOGW("read error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	return flash_core_read(user_buf, address, size);
}

bk_err_t bk_flash_read_word(uint32_t address, uint32_t *user_buf, uint32_t size)
{
	if (address >= flash_core_get_total_size()) {
		FLASH_LOGW("read error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	return flash_core_read_word(user_buf, address, size);
}

bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	if (address >= flash_core_get_total_size()) {
		FLASH_LOGW("write error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	return flash_core_write(address, user_buf, size);
}

uint32_t bk_flash_get_id(void)
{
	return flash_core_get_id();
}

bk_err_t bk_flash_set_clk_dpll(void)
{
	return BK_OK;
}

bk_err_t bk_flash_set_clk_dco(void)
{
	return BK_OK;
}


// #if CONFIG_FLASH_TEST
bk_err_t bk_flash_write_enable(void)
{
	return BK_OK;
}

bk_err_t bk_flash_write_disable(void)
{
	return BK_OK;
}

uint16_t bk_flash_read_status_reg(void)
{
	/* Atomic in the core: lock + two-line + RDSR + restore. Splitting the line
	 * switch and the RDSR here would let the other SMP cluster flip back to QUAD
	 * continuous-read in between and hang the RDSR. */
	return (uint16_t)flash_core_read_status_reg();
}

bk_err_t bk_flash_write_status_reg(uint16_t status_reg_data)
{
	(void)status_reg_data;
	return BK_OK;
}

uint32_t bk_flash_get_crc_err_num(void)
{
	return flash_hal_get_crc_err_num(flash_core_hal());
}
// #endif

void test_flash_set_protect_type_none(void)
{
	flash_core_unprotect();
}

void test_flash_set_protect_type_all(void)
{
	flash_core_set_runtime_protect_type(FLASH_PROTECT_ALL);
	flash_core_protect();
}

bool bk_flash_is_driver_inited()
{
	return s_flash_is_init;
}

uint32_t bk_flash_get_current_total_size(void)
{
	return flash_core_get_total_size();
}

bk_err_t bk_flash_register_ps_suspend_callback(flash_ps_callback_t ps_suspend_cb)
{
	return BK_OK;
}

bk_err_t bk_flash_register_ps_resume_callback(flash_ps_callback_t ps_resume_cb)
{
	return BK_OK;
}

#if CONFIG_DEEP_LV
uint32_t g_pm_flash_saving_regs[23] = {0};
#endif
bk_err_t bk_flash_power_saving_enter(void)
{
	// save flash ctrl setting to flash_ctrl_context;
	flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

#if CONFIG_DEEP_LV
	g_pm_flash_saving_regs[0] = REG_READ(SOC_FLASH_REG_BASE+0x4*4);
	g_pm_flash_saving_regs[1] =REG_READ(SOC_FLASH_REG_BASE+0x7*4);
	g_pm_flash_saving_regs[2] =REG_READ(SOC_FLASH_REG_BASE+0x9*4);
	g_pm_flash_saving_regs[3] =REG_READ(SOC_FLASH_REG_BASE+0xa*4);
#if CONFIG_SPE
	g_pm_flash_saving_regs[4] =REG_READ(SOC_FLASH_REG_BASE+0xd*4);
	g_pm_flash_saving_regs[5] =REG_READ(SOC_FLASH_REG_BASE+0xe*4);
	g_pm_flash_saving_regs[6] =REG_READ(SOC_FLASH_REG_BASE+0xf*4);
	g_pm_flash_saving_regs[7] =REG_READ(SOC_FLASH_REG_BASE+0x10*4);
	g_pm_flash_saving_regs[8] =REG_READ(SOC_FLASH_REG_BASE+0x11*4);
	g_pm_flash_saving_regs[9] =REG_READ(SOC_FLASH_REG_BASE+0x12*4);
	g_pm_flash_saving_regs[10] =REG_READ(SOC_FLASH_REG_BASE+0x13*4);
	g_pm_flash_saving_regs[11] =REG_READ(SOC_FLASH_REG_BASE+0x14*4);
#endif
	g_pm_flash_saving_regs[12] =REG_READ(SOC_FLASH_REG_BASE+0x15*4);
	g_pm_flash_saving_regs[13] =REG_READ(SOC_FLASH_REG_BASE+0x16*4);
	g_pm_flash_saving_regs[14] =REG_READ(SOC_FLASH_REG_BASE+0x17*4);
	g_pm_flash_saving_regs[15] =REG_READ(SOC_FLASH_REG_BASE+0x18*4);
	g_pm_flash_saving_regs[16] =REG_READ(SOC_FLASH_REG_BASE+0x19*4);
	g_pm_flash_saving_regs[17] =REG_READ(SOC_FLASH_REG_BASE+0x1a*4);
	g_pm_flash_saving_regs[18] =REG_READ(SOC_FLASH_REG_BASE+0x1b*4);
	g_pm_flash_saving_regs[19] =REG_READ(SOC_FLASH_REG_BASE+0x1c*4);
	g_pm_flash_saving_regs[20] =REG_READ(SOC_FLASH_REG_BASE+0x1d*4);
	g_pm_flash_saving_regs[21] =REG_READ(SOC_FLASH_REG_BASE+0x1e*4);
	g_pm_flash_saving_regs[22] =REG_READ(SOC_FLASH_REG_BASE+0x1f*4);
#endif
	return BK_OK;
}
__attribute__((section(".iram"))) bk_err_t bk_flash_power_saving_exit(void)
{
#if CONFIG_DEEP_LV
#if CONFIG_SPE
	REG_WRITE(SOC_FLASH_REG_BASE+0x4*4, g_pm_flash_saving_regs[0]);
#endif
	REG_WRITE(SOC_FLASH_REG_BASE+0x7*4, g_pm_flash_saving_regs[1]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x9*4, g_pm_flash_saving_regs[2]);
	REG_WRITE(SOC_FLASH_REG_BASE+0xa*4, g_pm_flash_saving_regs[3]);
#if CONFIG_SPE
	REG_WRITE(SOC_FLASH_REG_BASE+0xd*4, g_pm_flash_saving_regs[4]);
	REG_WRITE(SOC_FLASH_REG_BASE+0xe*4, g_pm_flash_saving_regs[5]);
	REG_WRITE(SOC_FLASH_REG_BASE+0xf*4, g_pm_flash_saving_regs[6]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x10*4, g_pm_flash_saving_regs[7]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x11*4, g_pm_flash_saving_regs[8]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x12*4, g_pm_flash_saving_regs[9]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x13*4, g_pm_flash_saving_regs[10]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x14*4, g_pm_flash_saving_regs[11]);
#endif
	REG_WRITE(SOC_FLASH_REG_BASE+0x15*4, g_pm_flash_saving_regs[12]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x16*4, g_pm_flash_saving_regs[13]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x17*4, g_pm_flash_saving_regs[14]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x18*4, g_pm_flash_saving_regs[15]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x19*4, g_pm_flash_saving_regs[16]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x1a*4, g_pm_flash_saving_regs[17]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x1b*4, g_pm_flash_saving_regs[18]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x1c*4, g_pm_flash_saving_regs[19]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x1d*4, g_pm_flash_saving_regs[20]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x1e*4, g_pm_flash_saving_regs[21]);
	REG_WRITE(SOC_FLASH_REG_BASE+0x1f*4, g_pm_flash_saving_regs[22]);
#endif

	// restore flash ctrl setting from flash_ctrl_context;
	// the restore API must run in SRAM/ITCM.
	// don't access flash before restoring setting, especially for A/B image project.
	/* flash_core_set_line_mode(FLASH_LINE_MODE_TWO) updated this state on entry. */
	flash_core_set_line_mode(flash_core_get_line_mode());

	return BK_OK;
}

__attribute__((section(".iram"))) bk_err_t bk_flash_enter_deep_sleep(void)
{
	return BK_FAIL;
}

__attribute__((section(".iram"))) bk_err_t bk_flash_exit_deep_sleep(void)
{
	return BK_FAIL;
}

#if !CONFIG_SPE
/*
 * DBUS window helper for NS CPU-freq switch (BK7259SW-3249). Must stay after
 * the IRAM power-saving path: the boot path is sensitive to IRAM layout
 * around flash_core_set_line_mode(). flash_hal_set_op_cmd_read() waits until
 * done, so this path kicks the controller itself and runs busy_cb once.
 */
__attribute__((section(".iram")))
static bk_err_t flash_wait_op_done_with_busy_cb(bk_flash_busy_cb_t busy_cb,
						void *busy_arg)
{
	flash_hal_t *hal = flash_core_hal();
	bool cb_done = false;
	bk_err_t ret = BK_OK;

	while (flash_hal_is_busy(hal)) {
		if (busy_cb && !cb_done) {
			ret = busy_cb(busy_arg);
			cb_done = true;
		}
	}

	/* The busy pulse can be too short to sample (op already completed). Still
	 * run the callback exactly once so the freq switch is never skipped: the op
	 * is done here and this path runs from IRAM with IRQ off, so switching the
	 * clock outside the busy window does not risk an XIP-fetch hang. Never
	 * return early on a missed busy window - the caller must always fall through
	 * to drain the read FIFO. */
	if (busy_cb && !cb_done) {
		ret = busy_cb(busy_arg);
		cb_done = true;
	}

	return ret;
}

__attribute__((section(".iram")))
static bk_err_t flash_set_op_cmd_read_with_busy_cb(uint32_t read_addr,
						   bk_flash_busy_cb_t busy_cb,
						   void *busy_arg)
{
	flash_hw_t *hw = flash_core_hal()->hw;

	hw->op_cmd.addr_sw_reg = read_addr;
	hw->op_cmd.op_type_sw = FLASH_OP_CMD_READ;
	hw->op_ctrl.op_sw = 1;
	return flash_wait_op_done_with_busy_cb(busy_cb, busy_arg);
}

__attribute__((section(".iram")))
static bk_err_t flash_read_common_with_busy_cb(uint8_t *buffer,
					       uint32_t address,
					       uint32_t len,
					       bk_flash_busy_cb_t busy_cb,
					       void *busy_arg)
{
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	uint32_t buf[FLASH_BUFFER_LEN] = {0};
	uint8_t *pb = (uint8_t *)&buf[0];
	bk_err_t ret;

	if (len == 0) {
		return BK_OK;
	}

	while (len) {
		uint32_t int_level = flash_core_enter_critical();

		ret = flash_set_op_cmd_read_with_busy_cb(addr, busy_cb,
			busy_arg);
		addr += FLASH_BYTES_CNT;
		busy_cb = NULL;
		busy_arg = NULL;

		/* op_sw READ was triggered and has completed (wait returns only when
		 * not busy), so the 8-word FIFO is filled. Always drain it - even if
		 * the callback reported an error - so the controller is left clean for
		 * the next read / XIP fetch. Leaving the FIFO undrained corrupts the hw
		 * FIFO pointer (see flash_core_clear_rd_residual) and hangs later XIP. */
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(flash_core_hal());
		}
		flash_core_exit_critical(int_level);

		if (ret != BK_OK) {
			return ret;
		}

		for (uint32_t i = address % FLASH_BYTES_CNT; i < FLASH_BYTES_CNT; i++) {
			*buffer++ = pb[i];
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}
	}

	return BK_OK;
}

__attribute__((section(".iram")))
bk_err_t bk_flash_read_bytes_with_busy_cb(uint32_t address, uint8_t *user_buf,
					  uint32_t size,
					  bk_flash_busy_cb_t busy_cb,
					  void *busy_arg)
{
	if (!s_flash_is_init) {
		return BK_ERR_FLASH_NOT_INIT;
	}

	if (address >= flash_core_get_total_size()) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	/* Does not go through flash_core_read(); self-bracket the outer op lock. */
	uint32_t int_level = flash_core_enter_critical();
	cp_flash_op_prepare();
	bk_err_t ret = flash_read_common_with_busy_cb(user_buf, address, size,
		busy_cb, busy_arg);
	cp_flash_op_finish();
	flash_core_exit_critical(int_level);
	return ret;
}
#endif
