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
 * Portable flash core - single implementation of the bk7259 flash algorithm.
 * The public operational surface is read/write/erase; the status-register,
 * raw-op, line-mode and lock primitives are internal here. OS / sys_drv / PPC /
 * HSPL / WDT dependencies are pushed to the injected port.
 */

#include <common/bk_include.h>
#include "bk_flash_core.h"
#include "flash_ll.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

#define FLASH_GET_PROTECT_CFG(cfg) ((cfg) & FLASH_STATUS_REG_PROTECT_MASK)
#define FLASH_GET_CMP_CFG(cfg)     (((cfg) >> FLASH_STATUS_REG_PROTECT_OFFSET) & FLASH_STATUS_REG_PROTECT_MASK)

/* ------------------------------------------------------------------------- */
/* Core state (single flash controller instance)                             */
/* ------------------------------------------------------------------------- */

static flash_hal_t s_hal;
static uint32_t s_flash_id;
/* Runtime line mode: FLASH_LINE_MODE_TWO / FOUR, or FLASH_LINE_MODE_UNSET when
 * not yet synced with the controller (BSS / post deep-sleep). Not the same as
 * flash_core_get_line_mode(), which returns the config capability. */
static uint32_t s_flash_line_mode;
static const flash_config_t *s_flash_cfg;
static flash_protect_type_t s_runtime_protect_type = FLASH_PROTECT_ALL;
static flash_core_port_t s_port;
static flash_core_perm_cb_t s_perm_cb;

/* ------------------------------------------------------------------------- */
/* Port / critical section                                                   */
/* ------------------------------------------------------------------------- */

void flash_core_set_port(const flash_core_port_t *port)
{
	if (port) {
		s_port = *port;
	}
}

void flash_core_set_perm_cb(flash_core_perm_cb_t cb)
{
	s_perm_cb = cb;
}

uint32_t flash_core_enter_critical(void)
{
	if (s_port.enter_critical) {
		return s_port.enter_critical();
	}
	return 0;
}

void flash_core_exit_critical(uint32_t int_level)
{
	if (s_port.exit_critical) {
		s_port.exit_critical(int_level);
	}
}

/* Outer op lock: critical section + optional peripheral notify. */
static uint32_t flash_core_lock(void)
{
	uint32_t int_level = flash_core_enter_critical();

	if (s_port.op_prepare) {
		s_port.op_prepare();
	}
	return int_level;
}

static void flash_core_unlock(uint32_t int_level)
{
	if (s_port.op_finish) {
		s_port.op_finish();
	}
	flash_core_exit_critical(int_level);
}

static void flash_core_progress(void)
{
	if (s_port.op_progress) {
		s_port.op_progress();
	}
}

/* ------------------------------------------------------------------------- */
/* Setup / accessors                                                         */
/* ------------------------------------------------------------------------- */

void flash_core_hal_init(void)
{
	/* Inline of flash_hal_init() so this same core links in TF-M, which does
	 * not compile the SDK flash_hal.c. */
	s_hal.id = 0;
	s_hal.hw = (flash_hw_t *)FLASH_LL_REG_BASE(s_hal.id);
	flash_ll_init(s_hal.hw);
}

flash_hal_t *flash_core_hal(void)
{
	return &s_hal;
}

static uint32_t flash_core_read_id(void)
{
	uint32_t int_level = flash_core_enter_critical();
	s_flash_id = flash_hal_get_id(&s_hal);
	flash_core_exit_critical(int_level);
	return s_flash_id;
}

static bool flash_core_resolve_cfg(void)
{
	for (uint32_t i = 0; i < (flash_config_num - 1); i++) {
		if (s_flash_id == flash_config[i].flash_id) {
			s_flash_cfg = &flash_config[i];
			return true;
		}
	}

	s_flash_cfg = &flash_config[flash_config_num - 1];
	return false;
}

bool flash_core_identify(void)
{
	/* RDID is an op_sw command: it is ignored while the device is in QUAD
	 * continuous-read, so self-bracket to two-line (mirrors erase/write) instead
	 * of relying on every caller. The critical section is re-entrant, so callers
	 * that already bracket their init sequence are unaffected. */
	uint32_t int_level = flash_core_enter_critical();
	/* NULL-cfg safe: FOUR is refused when s_flash_cfg is NULL, so pre-RDID
	 * set(TWO) never dereferences cfg. */
	flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_core_read_id();
	bool matched = flash_core_resolve_cfg();
	/* cfg is resolved now, so restoring FOUR is safe; at init old == TWO (no-op). */
	flash_core_set_line_mode(old);
	flash_core_exit_critical(int_level);
	return matched;
}

uint32_t flash_core_get_id(void)
{
	return s_flash_id;
}

const flash_config_t *flash_core_get_cfg(void)
{
	return s_flash_cfg;
}

/*
 * Inject the active config directly instead of resolving it from the built-in
 * table. Used by consumers that own their own device-selection policy (e.g. the
 * bootloader resolves the part from a per-device metadata table, not flash_id).
 */
void flash_core_set_cfg(const flash_config_t *cfg)
{
	s_flash_cfg = cfg;
}

uint32_t flash_core_get_total_size(void)
{
	return s_flash_cfg->flash_size;
}

flash_line_mode_t flash_core_get_line_mode(void)
{
	return s_flash_cfg->line_mode;
}

uint8_t flash_core_get_continuous_read_mode(void)
{
	return s_flash_cfg->coutinuous_read_mode_bits_val;
}

void flash_core_set_runtime_protect_type(flash_protect_type_t type)
{
	s_runtime_protect_type = type;
}

/* ------------------------------------------------------------------------- */
/* Status register                                                           */
/* ------------------------------------------------------------------------- */

/*
 * Raw RDSR primitive (file-local). Assumes the caller already holds the outer op
 * lock AND is in two-line mode (e.g. flash_set_protect_type_ex / flash_core_set_qe,
 * which run inside the protect/erase/write / set_line_mode brackets). Every caller
 * outside this file must use flash_core_read_status_reg() instead: in QUAD
 * continuous-read the device ignores the RDSR opcode and the busy-poll hangs, so
 * the two-line switch + RDSR must be one atomic critical section.
 */
FLASH_CORE_IRAM static uint32_t flash_core_read_status_reg_raw(void)
{
	uint32_t status_reg;
	uint32_t int_level;

	int_level = flash_core_enter_critical();
	status_reg = flash_hal_read_status_reg(&s_hal, s_flash_cfg->status_reg_size);
	flash_core_exit_critical(int_level);

	return status_reg;
}

/*
 * Atomic RDSR for external callers: take the outer op lock, drop to two-line,
 * RDSR, then restore the previous line mode. The whole sequence runs under one
 * (re-entrant, non-sleeping) critical section so another SMP cluster cannot flip
 * the device back to QUAD continuous-read between the line switch and the RDSR.
 */
FLASH_CORE_IRAM uint32_t flash_core_read_status_reg(void)
{
	uint32_t int_level = flash_core_lock();
	flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);
	uint32_t status_reg = flash_core_read_status_reg_raw();
	flash_core_set_line_mode(old);
	flash_core_unlock(int_level);
	return status_reg;
}

/*
 * Raw volatile WRSR (0x50 prefix). File-local — same preconditions as
 * flash_core_read_status_reg_raw(): caller already holds outer op lock and is
 * in two-line mode. Used by protect RMW only.
 */
FLASH_CORE_IRAM static void flash_core_write_status_reg_raw(uint32_t status_reg_val)
{
	uint32_t int_level = flash_core_enter_critical();

	flash_hal_write_status_reg(&s_hal, s_flash_cfg->status_reg_size, status_reg_val);
	flash_core_exit_critical(int_level);
}

/*
 * Raw non-volatile WRSR: value persists across power cycles. File-local —
 * same preconditions as flash_core_write_status_reg_raw(). Used by protect / QE.
 */
FLASH_CORE_IRAM static void flash_core_write_status_reg_nvol_raw(uint32_t status_reg_val)
{
	uint32_t int_level = flash_core_enter_critical();

	flash_hal_write_status_reg_nvol(&s_hal, s_flash_cfg->status_reg_size, status_reg_val);
	flash_core_exit_critical(int_level);
}

void flash_core_cpu_wr_enable(void)
{
	flash_hal_enable_cpu_data_wr(&s_hal);
}

void flash_core_cpu_wr_disable(void)
{
	flash_hal_disable_cpu_data_wr(&s_hal);
}

/* ------------------------------------------------------------------------- */
/* Protection                                                                */
/* ------------------------------------------------------------------------- */

static uint32_t flash_get_protect_cfg(flash_protect_type_t type)
{
	switch (type) {
	case FLASH_PROTECT_NONE:
		return FLASH_GET_PROTECT_CFG(s_flash_cfg->protect_none);
	case FLASH_PROTECT_ALL:
		return FLASH_GET_PROTECT_CFG(s_flash_cfg->protect_all);
	case FLASH_UNPROTECT_LAST_BLOCK:
		return FLASH_GET_PROTECT_CFG(s_flash_cfg->unprotect_last_block);
	default:
		return FLASH_GET_PROTECT_CFG(s_flash_cfg->protect_all);
	}
}

static void flash_set_protect_cfg(uint32_t *status_reg_val, uint32_t new_protect_cfg)
{
	*status_reg_val &= ~(s_flash_cfg->protect_mask << s_flash_cfg->protect_post);
	*status_reg_val |= ((new_protect_cfg & s_flash_cfg->protect_mask) << s_flash_cfg->protect_post);
}

static uint32_t flash_get_cmp_cfg(flash_protect_type_t type)
{
	switch (type) {
	case FLASH_PROTECT_NONE:
		return FLASH_GET_CMP_CFG(s_flash_cfg->protect_none);
	case FLASH_PROTECT_ALL:
		return FLASH_GET_CMP_CFG(s_flash_cfg->protect_all);
	case FLASH_UNPROTECT_LAST_BLOCK:
		return FLASH_GET_CMP_CFG(s_flash_cfg->unprotect_last_block);
	default:
		return FLASH_GET_CMP_CFG(s_flash_cfg->protect_all);
	}
}

static void flash_set_cmp_cfg(uint32_t *status_reg_val, uint32_t new_cmp_cfg)
{
	*status_reg_val &= ~(FLASH_CMP_MASK << s_flash_cfg->cmp_post);
	*status_reg_val |= ((new_cmp_cfg & FLASH_CMP_MASK) << s_flash_cfg->cmp_post);
}

static bool flash_is_need_update_status_reg(uint32_t protect_cfg, uint32_t cmp_cfg, uint32_t status_reg_val)
{
	uint32_t cur_protect_val_in_status_reg = (status_reg_val >> s_flash_cfg->protect_post) & s_flash_cfg->protect_mask;
	uint32_t cur_cmp_val_in_status_reg = (status_reg_val >> s_flash_cfg->cmp_post) & FLASH_CMP_MASK;

	if (cur_protect_val_in_status_reg != protect_cfg ||
		cur_cmp_val_in_status_reg != cmp_cfg) {
		return true;
	} else {
		return false;
	}
}

static void flash_set_protect_type_ex(flash_protect_type_t type, bool nonvolatile)
{
	uint32_t protect_cfg;
	uint32_t cmp_cfg;
	/* Always RDSR before RMW so QE / other bits set by another owner are kept. */
	uint32_t status_reg = flash_core_read_status_reg_raw();

	protect_cfg = flash_get_protect_cfg(type);
	cmp_cfg = flash_get_cmp_cfg(type);

	if (flash_is_need_update_status_reg(protect_cfg, cmp_cfg, status_reg)) {
		flash_set_protect_cfg(&status_reg, protect_cfg);
		flash_set_cmp_cfg(&status_reg, cmp_cfg);

		if (nonvolatile) {
			flash_core_write_status_reg_nvol_raw(status_reg);
		} else {
			flash_core_write_status_reg_raw(status_reg);
		}
	}
}

/* Table-driven QE bit update. Internal — called from set_line_mode(FOUR). */
FLASH_CORE_IRAM static void flash_core_set_qe(void)
{
	/* Always RDSR before RMW. */
	uint32_t status_reg = flash_core_read_status_reg_raw();

	if (((status_reg >> s_flash_cfg->quad_en_post) & 0x01) == s_flash_cfg->quad_en_val) {
		return;
	}

	if (1 == s_flash_cfg->quad_en_val)
		status_reg |= (1 << s_flash_cfg->quad_en_post);
	else
		status_reg &= ~(1 << s_flash_cfg->quad_en_post);

	/* QE must persist across reboot -> non-volatile write. */
	flash_core_write_status_reg_nvol_raw(status_reg);
}

/* ------------------------------------------------------------------------- */
/* Line mode                                                                 */
/* ------------------------------------------------------------------------- */

/* Distinct from FLASH_LINE_MODE_TWO(2) / FOUR(4): software cache not synced. */
#define FLASH_LINE_MODE_UNSET  0u

FLASH_CORE_IRAM flash_line_mode_t flash_core_set_line_mode(flash_line_mode_t line_mode)
{
	uint32_t int_level = flash_core_enter_critical();

	flash_line_mode_t new_line_mode;
	flash_line_mode_t old_line_mode = (flash_line_mode_t)s_flash_line_mode;

#if CONFIG_FLASH_QUAD_ENABLE
	/* Cap FOUR by config; NULL cfg (pre-identify) must not dereference. */
	if ((FLASH_LINE_MODE_FOUR == line_mode) &&
	    (s_flash_cfg != NULL) &&
	    (FLASH_LINE_MODE_FOUR == s_flash_cfg->line_mode)) {
		new_line_mode = FLASH_LINE_MODE_FOUR;
	} else
#endif
	{
		new_line_mode = FLASH_LINE_MODE_TWO;
	}

	/* Skip HW only when cache is valid and already at the target. UNSET means
	 * HW may still be in continuous-read after handoff / deep-sleep — must
	 * clear_qwfr and re-apply, or the next op_sw can busy-poll forever. */
	if ((s_flash_line_mode != FLASH_LINE_MODE_UNSET) &&
	    (new_line_mode == old_line_mode)) {
		flash_core_exit_critical(int_level);
		return old_line_mode;
	}

	flash_hal_clear_qwfr(&s_hal);   // cmd CRMR (coutinuous_read_mode reset), quit QPI mode.

	if (FLASH_LINE_MODE_FOUR == new_line_mode) {
		flash_hal_set_quad_m_value(&s_hal, s_flash_cfg->coutinuous_read_mode_bits_val);
		flash_core_set_qe();
		flash_hal_set_mode(&s_hal, FLASH_MODE_QUAD);  // enter QPI mode.
	} else {
		flash_hal_set_mode(&s_hal, FLASH_MODE_DUAL);
	}

	s_flash_line_mode = new_line_mode;

	flash_core_exit_critical(int_level);

	return old_line_mode;
}

void flash_core_reset_line_mode(void)
{
	s_flash_line_mode = FLASH_LINE_MODE_UNSET;
}

/* ------------------------------------------------------------------------- */
/* Session protect brackets                                                  */
/* ------------------------------------------------------------------------- */

/* Volatile SR write is ignored while the device is in QUAD continuous-read, so
 * bracket the protect change: switch to two-line, WRSR, restore the line mode. */
void flash_core_unprotect(void)
{
	uint32_t int_level = flash_core_lock();
	flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_set_protect_type_ex(FLASH_PROTECT_NONE, false);
	flash_core_set_line_mode(old);
	flash_core_unlock(int_level);
}

void flash_core_protect(void)
{
	uint32_t int_level = flash_core_lock();
	flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_set_protect_type_ex(s_runtime_protect_type, false);
	flash_core_set_line_mode(old);
	flash_core_unlock(int_level);
}

void flash_core_protect_nvol(void)
{
	uint32_t int_level = flash_core_lock();
	flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_set_protect_type_ex(s_runtime_protect_type, true);
	flash_core_set_line_mode(old);
	flash_core_unlock(int_level);
}

/* ------------------------------------------------------------------------- */
/* Raw data path (internal)                                                  */
/* ------------------------------------------------------------------------- */

static void flash_core_read_bytes_raw(uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	uint32_t buf[FLASH_BUFFER_LEN] = {0};
	uint8_t *pb = (uint8_t *)&buf[0];

	if (len == 0) {
		return;
	}

	while (len) {
		uint32_t int_level;

		/*disable interrupt for special device, contact with zili.guo/ling.zhou*/
		int_level = flash_core_enter_critical();
		flash_hal_set_op_cmd_read(&s_hal, addr);
		addr += FLASH_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_hal);
		}
		flash_core_exit_critical(int_level);

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

static void flash_core_read_word_raw(uint32_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	uint32_t buf[FLASH_BUFFER_LEN] = {0};
	uint32_t *pb = (uint32_t *)&buf[0];

	if (len == 0) {
		return;
	}

	while (len) {
		uint32_t int_level;

		int_level = flash_core_enter_critical();
		flash_hal_set_op_cmd_read(&s_hal, addr);
		addr += FLASH_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_hal);
		}

		flash_core_exit_critical(int_level);

		for (uint32_t i = address % (FLASH_BYTES_CNT/4); i < (FLASH_BYTES_CNT/4); i++) {
			*buffer++ = pb[i];
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}
	}
}

static bk_err_t flash_core_write_common_raw(const uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t buf[FLASH_BUFFER_LEN];
	uint8_t *pb = (uint8_t *)&buf[0];
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);

	if ((addr >= s_flash_cfg->flash_size) ||
		(len > s_flash_cfg->flash_size) ||
		((addr + len) > s_flash_cfg->flash_size)) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

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

		uint32_t int_level = flash_core_enter_critical();
		flash_hal_wait_op_done(&s_hal);

		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			flash_hal_write_data(&s_hal, buf[i]);
		}
		flash_hal_set_op_cmd_write(&s_hal, addr);
		flash_core_exit_critical(int_level);

		addr += FLASH_BYTES_CNT;
	}
	return BK_OK;
}

static bk_err_t flash_core_erase_block_raw(uint32_t address, int type)
{
	uint32_t int_level = flash_core_enter_critical();

	flash_hal_erase_block(&s_hal, address, type);

	flash_core_exit_critical(int_level);

	return BK_OK;
}

bk_err_t flash_core_clear_rd_residual(void)
{
	uint32_t tmp_val = 0;
	uint32_t residual_cnt, read_cnt;

	read_cnt = flash_hal_read_data_sw_flash_sel(&s_hal);
	if(read_cnt){
		residual_cnt = FLASH_BUFFER_LEN - read_cnt;
		while(residual_cnt){
			tmp_val = flash_hal_read_data(&s_hal);
			residual_cnt --;
		}
	}
	(void)tmp_val;

	return BK_OK;
}

/* ------------------------------------------------------------------------- */
/* Primary verbs: read / write / erase                                       */
/* ------------------------------------------------------------------------- */

bk_err_t flash_core_read(uint8_t *buffer, uint32_t address, uint32_t len)
{
	if (address >= s_flash_cfg->flash_size) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	/* Hold the outer op lock across all 32-byte chunks so another cluster
	 * cannot write/erase / switch line mode in the gaps. */
	uint32_t int_level = flash_core_lock();
	flash_core_read_bytes_raw(buffer, address, len);
	flash_core_unlock(int_level);
	return BK_OK;
}

bk_err_t flash_core_read_word(uint32_t *buffer, uint32_t address, uint32_t len)
{
	if (address >= s_flash_cfg->flash_size) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	uint32_t int_level = flash_core_lock();
	flash_core_read_word_raw(buffer, address, len);
	flash_core_unlock(int_level);
	return BK_OK;
}

/* Map an exact aligned size to its native single-op opcode, else -1 (range). */
static int flash_core_single_erase_cmd(uint32_t address, uint32_t size)
{
	if (size == FLASH_SECTOR_SIZE && (address & (FLASH_SECTOR_SIZE - 1)) == 0)
		return FLASH_OP_CMD_SE;
	if (size == FLASH_BLOCK32_SIZE && (address & (FLASH_BLOCK32_SIZE - 1)) == 0)
		return FLASH_OP_CMD_BE1;
	if (size == FLASH_BLOCK_SIZE && (address & (FLASH_BLOCK_SIZE - 1)) == 0)
		return FLASH_OP_CMD_BE2;
	return -1;
}

/* Erase the sectors/64K-blocks covering [address, address+size), feeding the
 * optional progress hook between sub-ops (matches the historical bootloader
 * flash_erase() sector + 64K-block optimization). */
static bk_err_t flash_core_erase_range_raw(uint32_t address, uint32_t size)
{
	uint32_t cur = address & (~(FLASH_SECTOR_SIZE - 1));
	uint32_t end = address + size;
	bk_err_t ret = BK_OK;

	while (cur < end) {
		int cmd;
		uint32_t step;

		if ((cur & (FLASH_BLOCK_SIZE - 1)) == 0 && (cur + FLASH_BLOCK_SIZE) <= end) {
			cmd = FLASH_OP_CMD_BE2;
			step = FLASH_BLOCK_SIZE;
		} else {
			cmd = FLASH_OP_CMD_SE;
			step = FLASH_SECTOR_SIZE;
		}

		ret = flash_core_erase_block_raw(cur, cmd);
		flash_core_progress();
		cur += step;
	}
	return ret;
}

/* Bracketed erase WITHOUT the outer op lock (caller holds it, or is in an
 * IRQ-disabled dump context). Line mode -> two-line; protection per policy. */
static bk_err_t flash_core_erase_op(uint32_t address, uint32_t size)
{
	if (size == 0) {
		return BK_OK;
	}
	if (address >= s_flash_cfg->flash_size) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	bk_err_t ret = BK_FAIL;
	flash_line_mode_t old_line_mode = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

	if ((s_perm_cb == NULL) || s_perm_cb(address, size)) {
		flash_set_protect_type_ex(FLASH_PROTECT_NONE, false);

		int cmd = flash_core_single_erase_cmd(address, size);
		if (cmd >= 0) {
			ret = flash_core_erase_block_raw(address, cmd);
			flash_core_progress();
		} else {
			ret = flash_core_erase_range_raw(address, size);
		}
	}

	flash_set_protect_type_ex(s_runtime_protect_type, false);
	flash_core_set_line_mode(old_line_mode);

	return ret;
}

/* Bracketed write WITHOUT the outer op lock. */
static bk_err_t flash_core_write_op(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	if (address >= s_flash_cfg->flash_size) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	bk_err_t ret = BK_FAIL;
	flash_line_mode_t old_line_mode = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);

	if ((s_perm_cb == NULL) || s_perm_cb(address, size)) {
		flash_set_protect_type_ex(FLASH_PROTECT_NONE, false);

		ret = flash_core_write_common_raw(user_buf, address, size);
	}

	flash_set_protect_type_ex(s_runtime_protect_type, false);
	flash_core_set_line_mode(old_line_mode);

	return ret;
}

bk_err_t flash_core_erase(uint32_t address, uint32_t size)
{
	uint32_t int_level = flash_core_lock();
	bk_err_t ret = flash_core_erase_op(address, size);
	flash_core_unlock(int_level);
	return ret;
}

bk_err_t flash_core_write(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	uint32_t int_level = flash_core_lock();
	bk_err_t ret = flash_core_write_op(address, user_buf, size);
	flash_core_unlock(int_level);
	return ret;
}
