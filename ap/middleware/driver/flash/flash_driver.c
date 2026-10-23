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
 * AP middleware flash driver: thin adapter over the portable flash_core
 * (shared physical source in cp/middleware/soc/common/flash_core). Supplies the
 * AP NS concurrency port (HSPL + mailbox notify), the AP lazy-init / wait-for-CP
 * policy and the partition write-permission check.
 */

#include <common/bk_include.h>
#include <components/ate.h>
#include <driver/flash.h>
#include <os/os.h>
#include <os/mem.h>
#include "flash_driver.h"
#include "flash_hal.h"
#include "sys_driver.h"
#include "driver/flash_partition.h"
#include <modules/chip_support.h>
#include "flash_bypass.h"
#include "aspl_lock.h"

#if CONFIG_AP_EMUBOOT
#include "bk_misc.h"
#include "sys_sw_regs.h"
#endif

static bool s_flash_is_init = false;
static uint32_t s_flash_dev_version = 0;

#define FLASH_MAX_WAIT_CB_CNT (4)
static flash_wait_callback_t s_flash_wait_cb[FLASH_MAX_WAIT_CB_CNT] = {NULL};

extern bk_err_t    mb_flash_ipc_init(void);
extern bk_err_t    mb_flash_op_prepare(void);
extern bk_err_t    mb_flash_op_finish(void);

extern bk_err_t    bk_flash_partition_write_perm_check_by_addr(uint32_t addr, uint32_t size, uint32_t magic_code);

/* ------------------------------------------------------------------------- */
/* flash_core port: NS concurrency (HSPL + rtos_disable_int) + mb notify      */
/* ------------------------------------------------------------------------- */

static void ap_flash_op_prepare(void)
{
	mb_flash_op_prepare();  /* AP: LCD + camera + UART notify */
}

static void ap_flash_op_finish(void)
{
	mb_flash_op_finish();
}

static const flash_core_port_t s_ap_flash_port = {
	.enter_critical = bk_aspl_flash_enter_critical,
	.exit_critical  = bk_aspl_flash_exit_critical,
	.op_prepare     = ap_flash_op_prepare,
	.op_finish      = ap_flash_op_finish,
};

static bool ap_flash_perm_cb(uint32_t addr, uint32_t size)
{
	return bk_flash_partition_write_perm_check_by_addr(addr, size, FLASH_API_MAGIC_CODE) == BK_OK;
}

#if CONFIG_AP_EMUBOOT

#define CP_FLASH_INIT_WAIT_TIMEOUT_US 5000000U
#define CP_FLASH_INIT_WAIT_POLL_US    100U

static bk_err_t wait_for_cp_flash_init_done_in_flash_api(void)
{
	uint32_t waited_us = 0;

	while (bk_sys_sw_regs_get_flash_init_done() != BK_SYS_SW_REGS_FLASH_INIT_DONE) {
		if (waited_us >= CP_FLASH_INIT_WAIT_TIMEOUT_US) {
			FLASH_LOGW("wait cp flash init timeout\r\n");
			return BK_FAIL;
		}

		bk_delay_us(CP_FLASH_INIT_WAIT_POLL_US);
		waited_us += CP_FLASH_INIT_WAIT_POLL_US;
	}

	return BK_OK;
}
#endif

static bk_err_t ensure_flash_driver_initialized(void)
{

#if CONFIG_AP_EMUBOOT
	if (s_flash_is_init && (flash_core_get_cfg() != NULL)) {
		return BK_OK;
		}

		bk_err_t ret = wait_for_cp_flash_init_done_in_flash_api();
		if (ret != BK_OK) {
			return ret;
		}
#endif

	return bk_flash_driver_init();
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

	flash_core_set_port(&s_ap_flash_port);
	flash_core_set_perm_cb(ap_flash_perm_cb);
	flash_core_hal_init();
	hal_ptr = flash_core_hal();

	s_flash_dev_version = flash_hal_get_dev_version(hal_ptr);
	FLASH_LOGI("dev_version=0x%x\r\n", s_flash_dev_version);

	uint32_t int_level = flash_core_enter_critical();

	flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

	if (!flash_core_identify()) {
		for(int i = 0; i < 10; i++) {
			FLASH_LOGE("This flash is not identified, choose default config\r\n");
		}
	}
	FLASH_LOGD("id=0x%x\r\n", flash_core_get_id());

	if (flash_core_get_cfg() == NULL) {
		FLASH_LOGE("flash_cfg is NULL after flash_core_identify\r\n");
		flash_core_exit_critical(int_level);
		return BK_ERR_FLASH_NOT_INIT;
	}

	/* Enable flash write-protect at boot and persist it across reboot. */
	flash_core_set_runtime_protect_type(FLASH_PROTECT_ALL);
	flash_core_protect_nvol();

	// Set flash line mode to the default line mode
	flash_core_set_line_mode(flash_core_get_line_mode());

	flash_core_exit_critical(int_level);

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
	// Auto-initialize if not initialized
	if (!s_flash_is_init || flash_core_get_cfg() == NULL) {
		bk_err_t ret = ensure_flash_driver_initialized();
		if (ret != BK_OK) {
			FLASH_LOGW("erase error: flash driver init failed\r\n");
			return ret;
		}
	}

	if (address >= flash_core_get_total_size()) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	return flash_core_erase(address & (~(FLASH_SECTOR_SIZE - 1)), FLASH_SECTOR_SIZE);
}

bk_err_t bk_flash_erase_32k(uint32_t address)
{
	// Auto-initialize if not initialized
	if (!s_flash_is_init || flash_core_get_cfg() == NULL) {
		bk_err_t ret = ensure_flash_driver_initialized();
		if (ret != BK_OK) {
			FLASH_LOGW("erase error: flash driver init failed\r\n");
			return ret;
		}
	}

	if (address >= flash_core_get_total_size()) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	return flash_core_erase(address & (~(FLASH_BLOCK32_SIZE - 1)), FLASH_BLOCK32_SIZE);
}

bk_err_t bk_flash_erase_block(uint32_t address)
{
	// Auto-initialize if not initialized
	if (!s_flash_is_init || flash_core_get_cfg() == NULL) {
		bk_err_t ret = ensure_flash_driver_initialized();
		if (ret != BK_OK) {
			FLASH_LOGW("erase error: flash driver init failed\r\n");
			return ret;
		}
	}

	if (address >= flash_core_get_total_size()) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	return flash_core_erase(address & (~(FLASH_BLOCK_SIZE - 1)), FLASH_BLOCK_SIZE);
}

bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	// Auto-initialize if not initialized
	if (!s_flash_is_init || flash_core_get_cfg() == NULL) {
		bk_err_t ret = ensure_flash_driver_initialized();
		if (ret != BK_OK) {
			FLASH_LOGW("read error: flash driver init failed\r\n");
			return ret;
		}
	}

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
	// Auto-initialize if not initialized
	if (!s_flash_is_init || flash_core_get_cfg() == NULL) {
		bk_err_t ret = ensure_flash_driver_initialized();
		if (ret != BK_OK) {
			FLASH_LOGW("write error: flash driver init failed\r\n");
			return ret;
		}
	}

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

#if CONFIG_FLASH_TEST
uint32_t bk_flash_get_crc_err_num(void)
{
	return flash_hal_get_crc_err_num(flash_core_hal());
}
#endif

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

bk_err_t bk_flash_register_wait_cb(flash_wait_callback_t wait_cb)
{
	uint32_t i = 0;

	for (i = 0; i < FLASH_MAX_WAIT_CB_CNT; i++) {
		if (s_flash_wait_cb[i] == NULL) {
			s_flash_wait_cb[i] = wait_cb;
			break;
		}
	}

	if (i == FLASH_MAX_WAIT_CB_CNT) {
		FLASH_LOGE("cb is full\r\n");
		return BK_ERR_FLASH_WAIT_CB_FULL;
	}

	return BK_OK;
}

bk_err_t bk_flash_unregister_wait_cb(flash_wait_callback_t wait_cb)
{
	uint32_t i = 0;

	for (i = 0; i < FLASH_MAX_WAIT_CB_CNT; i++) {
		if (s_flash_wait_cb[i] == wait_cb) {
			s_flash_wait_cb[i] = NULL;
			break;
		}
	}

	if (i == FLASH_MAX_WAIT_CB_CNT) {
		FLASH_LOGE("cb isn't registered\r\n");
		return BK_ERR_FLASH_WAIT_CB_NOT_REGISTER;
	}

	return BK_OK;
}

__attribute__((section(".itcm_sec_code"))) void flash_waiting_cb(void)
{
	uint32_t i = 0;

	for (i = 0; i < FLASH_MAX_WAIT_CB_CNT; i++) {
		if (s_flash_wait_cb[i]) {
			s_flash_wait_cb[i]();
		}
	}
}

bk_err_t bk_flash_register_ps_suspend_callback(flash_ps_callback_t ps_suspend_cb)
{
	return BK_OK;
}

bk_err_t bk_flash_register_ps_resume_callback(flash_ps_callback_t ps_resume_cb)
{
	return BK_OK;
}

bk_err_t bk_flash_power_saving_enter(void)
{
	// save flash ctrl setting to flash_ctrl_context;
	flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

	return BK_OK;
}

bk_err_t bk_flash_power_saving_exit(void)
{
	// restore flash ctrl setting from flash_ctrl_context;
	// the restore API must run in SRAM/ITCM.
	// don't access flash before restoring setting, especially for A/B image project.

	flash_core_reset_line_mode();
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

#if CONFIG_FLASH_OTP_OPERATION
/* ------------------------------------------------------------------------- */
/* Security-register (OTP) operations                                          */
/* ------------------------------------------------------------------------- */

#define FLASH_OTP_IDX_ADDR(idx)    ((idx) << 12)
#define FLASH_OTP_LOCK_BIT(idx)    (10 + (idx))  /* LB1/LB2/LB3: SR bit 11/12/13 */
#define FLASH_OTP_RETRY_MAX        (3)

/* 32-byte aligned chunk read; same op_sw data path as flash_core_read_bytes_raw(). */
static void flash_otp_read_chunks(flash_hal_t *hal, uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	uint32_t tmp[FLASH_BUFFER_LEN] = {0};
	uint8_t *pb = (uint8_t *)&tmp[0];

	while (len) {
		flash_hal_set_op_cmd_read_scr(hal, addr);
		addr += FLASH_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			tmp[i] = flash_hal_read_data(hal);
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
}

/* 32-byte aligned chunk write; same op_sw data path as flash_core_write_common_raw(). */
static void flash_otp_write_chunks(flash_hal_t *hal, const uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	uint32_t tmp[FLASH_BUFFER_LEN];
	uint8_t *pb = (uint8_t *)&tmp[0];

	while (len) {
		for (uint32_t i = 0; i < FLASH_BYTES_CNT; i++) {
			pb[i] = 0xFF;
		}
		for (uint32_t i = address % FLASH_BYTES_CNT; i < FLASH_BYTES_CNT; i++) {
			pb[i] = *buffer++;
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}

		flash_hal_wait_op_done(hal);
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			flash_hal_write_data(hal, tmp[i]);
		}
		flash_hal_set_op_cmd_write(hal, addr);
		addr += FLASH_BYTES_CNT;
	}
}

/* One erase + verify attempt in one atomic bracket. Returns 0 when the whole
 * block reads back 0xFF, 1 when the verify failed (worth retrying: the
 * device can silently drop an erase, like the bypass test observed). */
static int flash_otp_erase_once(flash_hal_t *hal, uint32_t otp_idx, uint8_t *verify)
{
	int ret;

	mb_flash_op_prepare();
	uint32_t int_level = flash_core_enter_critical();
	flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

	flash_hal_otp_opcodes_set(hal);

	flash_hal_erase_block(hal, FLASH_OTP_IDX_ADDR(otp_idx), FLASH_OP_CMD_SE);

	flash_otp_read_chunks(hal, verify, FLASH_OTP_IDX_ADDR(otp_idx), FLASH_OTP_BLOCK_LENGTH);

	flash_hal_otp_opcodes_clear(hal);

	ret = 0;
	for (uint32_t i = 0; i < FLASH_OTP_BLOCK_LENGTH; i++) {
		if (verify[i] != 0xFF) {
			ret = 1;
			break;
		}
	}

	flash_core_set_line_mode(old);
	flash_core_exit_critical(int_level);
	mb_flash_op_finish();
	return ret;
}

bk_err_t bk_flash_otp_read(uint32_t otp_idx, uint32_t offset, uint32_t len, uint8_t *buf)
{
	if ((otp_idx < 1) || (otp_idx > FLASH_OTP_IDX_MAX) ||
		(len == 0) || (offset + len > FLASH_OTP_BLOCK_LENGTH)) {
		return BK_ERR_PARAM;
	}

	if (!s_flash_is_init || flash_core_get_cfg() == NULL) {
		bk_err_t ret = ensure_flash_driver_initialized();
		if (ret != BK_OK) {
			FLASH_LOGW("otp read error: flash driver init failed\r\n");
			return ret;
		}
	}

	flash_hal_t *hal = flash_core_hal();

	mb_flash_op_prepare();
	uint32_t int_level = flash_core_enter_critical();
	flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

	flash_otp_read_chunks(hal, buf, FLASH_OTP_IDX_ADDR(otp_idx) + offset, len);

	flash_core_set_line_mode(old);
	flash_core_exit_critical(int_level);
	mb_flash_op_finish();
	return BK_OK;
}

/* OTP write: opcode 0x42 rides the page-program op; no read window is
 * needed, the PP slot is never used by XIP fetches. */
bk_err_t bk_flash_otp_write(uint32_t otp_idx, uint32_t offset, uint32_t len, const uint8_t *buf)
{
	if ((otp_idx < 1) || (otp_idx > FLASH_OTP_IDX_MAX) ||
		(len == 0) || (offset + len > FLASH_OTP_BLOCK_LENGTH)) {
		return BK_ERR_PARAM;
	}

	if (!s_flash_is_init || flash_core_get_cfg() == NULL) {
		bk_err_t ret = ensure_flash_driver_initialized();
		if (ret != BK_OK) {
			FLASH_LOGW("otp write error: flash driver init failed\r\n");
			return ret;
		}
	}

	flash_hal_t *hal = flash_core_hal();

	mb_flash_op_prepare();
	uint32_t int_level = flash_core_enter_critical();
	flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

	flash_hal_otp_opcodes_set(hal);
	flash_otp_write_chunks(hal, buf, FLASH_OTP_IDX_ADDR(otp_idx) + offset, len);
	flash_hal_otp_opcodes_clear(hal);

	flash_core_set_line_mode(old);
	flash_core_exit_critical(int_level);
	mb_flash_op_finish();
	return BK_OK;
}

/* OTP erase: opcode 0x44 rides the sector-erase op; erase-then-verify with
 * retry, like the bypass test (the device can silently drop an erase). */
bk_err_t bk_flash_otp_erase(uint32_t otp_idx)
{
	if ((otp_idx < 1) || (otp_idx > FLASH_OTP_IDX_MAX)) {
		return BK_ERR_PARAM;
	}

	if (!s_flash_is_init || flash_core_get_cfg() == NULL) {
		bk_err_t ret = ensure_flash_driver_initialized();
		if (ret != BK_OK) {
			FLASH_LOGW("otp erase error: flash driver init failed\r\n");
			return ret;
		}
	}

	flash_hal_t *hal = flash_core_hal();
	uint8_t *verify = os_malloc(FLASH_OTP_BLOCK_LENGTH);

	if (verify == NULL) {
		return BK_ERR_NO_MEM;
	}

	int ok = 0;
	for (uint32_t i = 0; i < FLASH_OTP_RETRY_MAX; i++) {
		if (flash_otp_erase_once(hal, otp_idx, verify) == 0) {
			ok = 1;
			break;
		}
	}
	os_free(verify);

	if (!ok) {
		FLASH_LOGE("otp idx=%d erase verify not all-FF\r\n", otp_idx);
		return BK_FAIL;
	}
	return BK_OK;
}

/* OTP lock: set the LB bit of this block in the status register. LB bits are
 * non-volatile, so use the standard WREN + WRSR path (no 0x50 prefix). */
bk_err_t bk_flash_otp_lock(uint32_t otp_idx)
{
	if ((otp_idx < 1) || (otp_idx > FLASH_OTP_IDX_MAX)) {
		return BK_ERR_PARAM;
	}

	if (!s_flash_is_init || flash_core_get_cfg() == NULL) {
		bk_err_t ret = ensure_flash_driver_initialized();
		if (ret != BK_OK) {
			FLASH_LOGW("otp lock error: flash driver init failed\r\n");
			return ret;
		}
	}

	const flash_config_t *cfg = flash_core_get_cfg();

	/* LB1-3 live in SR2 (S8-15): needs at least a 2-byte status register. */
	if (cfg->status_reg_size < 2) {
		FLASH_LOGE("otp lock needs a 2-byte status register\r\n");
		return BK_FAIL;
	}

	flash_hal_t *hal = flash_core_hal();
	uint32_t lock_bit = FLASH_OTP_LOCK_BIT(otp_idx);
	bk_err_t ret = BK_FAIL;

	for (uint32_t i = 0; i < FLASH_OTP_RETRY_MAX; i++) {
		uint32_t int_level = flash_core_enter_critical();
		flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

		uint32_t sr = flash_hal_read_status_reg(hal, cfg->status_reg_size);
		sr |= (1u << lock_bit);
		flash_hal_write_status_reg_nvol(hal, cfg->status_reg_size, sr);
		sr = flash_hal_read_status_reg(hal, cfg->status_reg_size);

		flash_core_set_line_mode(old);
		flash_core_exit_critical(int_level);

		if (sr & (1u << lock_bit)) {
			ret = BK_OK;
			break;
		}
	}
	return ret;
}
#endif
