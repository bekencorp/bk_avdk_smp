// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: table-driven flash driver.
// Serves the secure-boot verify path and the BL2 serial-download backend.
// Reuses the bk7259 flash_config[] table (embedded below, from the non-secure
// cp/middleware/driver/flash/flash_driver.c) via the inline flash_ll.h.

#include <stdint.h>
#include <stdbool.h>
#include <common/bk_include.h>
#include <driver/flash.h>
#include "flash_hal.h"
#include "flash_ll.h"
#include "flash_layout.h"

#define FLASH_MIN_BYTES_CNT     32
#define FLASH_MIN_BUFFER_LEN    8
#define FLASH_MIN_ADDRESS_MASK  0x1f
#define FLASH_MIN_SECTOR_MASK   0xfff   /* 4KB sector */
#define FLASH_MIN_ERASED_VALUE  0xff

#ifndef SOC_SEC_ADDR_NUM
#define SOC_SEC_ADDR_NUM        4   /* DBUS security regions */
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)           (sizeof(a) / sizeof((a)[0]))
#endif

/* bk7259 flash_config_t + table, embedded from the non-secure driver (can't be
 * #included directly - that pulls in its OS/lock/sys deps). Note: the bk7236
 * reference struct differs (extra protect_half/mode_sel, different order). */
#define FLASH_SIZE_1M                    0x100000
#define FLASH_SIZE_2M                    0x200000
#define FLASH_SIZE_4M                    0x400000
#define FLASH_SIZE_8M                    0x800000
#define FLASH_SIZE_16M                   0x1000000
#define FLASH_STATUS_REG_PROTECT_MASK    0xff
#define FLASH_STATUS_REG_PROTECT_OFFSET  8
#define FLASH_CMP_MASK                   0x1

#define FLASH_GET_PROTECT_CFG(cfg) ((cfg) & FLASH_STATUS_REG_PROTECT_MASK)
#define FLASH_GET_CMP_CFG(cfg)     (((cfg) >> FLASH_STATUS_REG_PROTECT_OFFSET) & FLASH_STATUS_REG_PROTECT_MASK)

typedef struct {
	uint32_t flash_id;
	uint32_t flash_size;
	uint8_t status_reg_size;
	flash_line_mode_t line_mode;
	uint8_t cmp_post;
	uint8_t protect_post;
	uint8_t protect_mask;
	uint16_t protect_all;
	uint16_t protect_none;
	uint16_t unprotect_last_block;
	uint8_t quad_en_post;
	uint8_t quad_en_val;
	uint8_t coutinuous_read_mode_bits_val;
} flash_min_config_t;

static const flash_min_config_t s_flash_config[] = {
	/* flash_id, flash_size,    status_reg_size, line_mode,            cmp_post, protect_post, protect_mask, protect_all, protect_none, unprotect_last_block. quad_en_post, quad_en_val, coutinuous_read_mode_bits_val */
	{0x1C7016,   FLASH_SIZE_4M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x1F,         0x1F,        0x00,         0x01B,                9,            1,           0xA5}, //en_25qh32b
	{0x1C7015,   FLASH_SIZE_2M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x1F,         0x1F,        0x00,         0x0d,                 9,            1,           0xA5}, //en_25qh16b
	{0x0B4014,   FLASH_SIZE_1M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //xtx_25f08b
	{0x0B4015,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //xtx_25f16b
	{0x0B4016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //xtx_25f32b
	{0x0B4017,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x05,        0x00,         0x109,                9,            1,           0xA0}, //xtx_25f64b
	{0x0B6017,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0}, //xt_25q64d
	{0x0B6018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0}, //xt_25q128d
	{0x0B4018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0}, //xt_25F128F-W
	{0x0E4016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //xtx_FT25H32
	{0x1C4116,   FLASH_SIZE_4M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0}, //en_25qe32a
	{0x5E5018,   FLASH_SIZE_16M,  1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0}, //zb_25lq128c
	{0xC84015,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //gd_25q16c
	{0xC84017,   FLASH_SIZE_8M,   1,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //gd_25q16c
	{0xC84016,   FLASH_SIZE_4M,   3,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0}, //gd_25q32c
	{0xC86018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0}, //gd_25lq128e
	{0xC86515,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //gd_25w16e
	{0xC86516,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0}, //gd_25wq32e
	{0xEF4016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //w_25q32(bfj)
	{0x204118,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0}, //xm_25qu128c
	{0x204016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //xmc_25qh32b
	{0xC22315,   FLASH_SIZE_2M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                6,            1,           0xA5}, //mx_25v16b
	{0xEB6015,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0}, //zg_th25q16b
	{0xC86517,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0}, //gd_25Q32E
	{0xCD6017,   FLASH_SIZE_8M,   3,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0}, //th_25q64ha
	{0x000000,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_TWO,    0,        2,            0x1F,         0x00,        0x00,         0x000,                0,            0,           0x00}, //default
};

static flash_hal_t s_flash_hal;
static bool s_flash_min_inited;
static uint32_t s_flash_id;
static const flash_min_config_t *s_flash_cfg;

/* Not declared in driver/flash.h; used before its definition below. */
bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode);

/* Match RDID against the table; last entry is the catch-all default. */
static void flash_min_resolve_cfg(void)
{
	s_flash_id = flash_ll_get_id(s_flash_hal.hw);

	for (uint32_t i = 0; i < (ARRAY_SIZE(s_flash_config) - 1); i++) {
		if (s_flash_id == s_flash_config[i].flash_id) {
			s_flash_cfg = &s_flash_config[i];
			return;
		}
	}
	s_flash_cfg = &s_flash_config[ARRAY_SIZE(s_flash_config) - 1];
}

static const flash_min_config_t *flash_min_cfg(void)
{
	if (!s_flash_min_inited) {
		bk_flash_driver_init();
	}
	if (!s_flash_cfg) {
		flash_min_resolve_cfg();
	}
	return s_flash_cfg;
}

bk_err_t bk_flash_driver_init(void)
{
	if (s_flash_min_inited) {
		return BK_OK;
	}
	s_flash_hal.id = 0;
	/* Inline flash_hal_init() to avoid pulling in the SDK flash_hal.c. */
	s_flash_hal.hw = (flash_hw_t *)FLASH_LL_REG_BASE(s_flash_hal.id);
	flash_ll_init(s_flash_hal.hw);
	s_flash_min_inited = true;
	/* BL2 leaves the flash device in QUAD continuous-read (for XIP). In that
	 * state the device ignores op_sw opcodes (RDID / READ / RDSR), so
	 * flash_ll_get_id()'s busy poll never clears -> the secure image hangs here
	 * (observed: markers reach 'R' then stop, no 'I'). CRMR (clear_qwfr) is a
	 * continuous-read mode reset the device honors even while in continuous
	 * read, so issue it first to drop back to normal command mode; memory-mapped
	 * code fetch keeps working via standard reads. Mirrors the SDK flash_driver.c
	 * sequence (flash_set_line_mode(TWO) -> flash_get_id). */
	flash_ll_clear_qwfr(s_flash_hal.hw);
	flash_min_resolve_cfg();
	/* Restore QUAD continuous-read so the rest of TF-M and the NS app run XIP at
	 * full speed, matching the state BL2 handed over. */
	bk_flash_set_line_mode(s_flash_cfg->line_mode);
	/* Do NOT unprotect here: the secure-boot verify path (this init + reads) must
	 * keep the persistent flash write protection. Flash is unprotected only when a
	 * serial-download session starts, via bk_flash_min_unprotect_once() called from
	 * the download handshake (flash_op_enable_ctrl -> download_flash_adapter.c). */
	return BK_OK;
}

bk_err_t bk_flash_driver_deinit(void)
{
	s_flash_min_inited = false;
	return BK_OK;
}

uint32_t bk_flash_get_current_total_size(void)
{
	/* Matched device size; unknown parts fall back to the default entry. */
	return flash_min_cfg()->flash_size;
}

bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	if (!user_buf) {
		return BK_ERR_NULL_PARAM;
	}
	if (size == 0) {
		return BK_OK;
	}

	uint32_t addr = address & (~FLASH_MIN_ADDRESS_MASK);
	uint32_t buf[FLASH_MIN_BUFFER_LEN] = {0};
	uint8_t *pb = (uint8_t *)&buf[0];
	uint32_t len = size;

	while (len) {
		flash_hal_set_op_cmd_read(&s_flash_hal, addr);
		addr += FLASH_MIN_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_MIN_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_flash_hal);
		}

		for (uint32_t i = address % FLASH_MIN_BYTES_CNT; i < FLASH_MIN_BYTES_CNT; i++) {
			*user_buf++ = pb[i];
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}
	}
	return BK_OK;
}

extern void *memcpy(void *dest, const void *src, size_t n);
__attribute__((section(".iram"))) void bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size)
{
	/* Cacheable (XIP) view, XTS-decrypted on the fly. */
	//memcpy(user_buf, (const void *)(SOC_FLASH_DATA_BASE + address), size);
	const volatile uint8_t *src = (const volatile uint8_t *)(0x04000000 + address);
	uint8_t *dst = (uint8_t *)user_buf;
	for (uint32_t i = 0; i < size; i++) {
		dst[i] = src[i];
	}
}

/* protect helpers, ported from flash_driver.c. bk7259 table has no
 * protect_half, so FLASH_PROTECT_HALF folds into the protect_all default. */
static uint32_t flash_get_protect_cfg(flash_protect_type_t type)
{
	const flash_min_config_t *cfg = flash_min_cfg();

	switch (type) {
	case FLASH_PROTECT_NONE:
		return FLASH_GET_PROTECT_CFG(cfg->protect_none);
	case FLASH_UNPROTECT_LAST_BLOCK:
		return FLASH_GET_PROTECT_CFG(cfg->unprotect_last_block);
	case FLASH_PROTECT_ALL:
	default:
		return FLASH_GET_PROTECT_CFG(cfg->protect_all);
	}
}

static uint32_t flash_get_cmp_cfg(flash_protect_type_t type)
{
	const flash_min_config_t *cfg = flash_min_cfg();

	switch (type) {
	case FLASH_PROTECT_NONE:
		return FLASH_GET_CMP_CFG(cfg->protect_none);
	case FLASH_UNPROTECT_LAST_BLOCK:
		return FLASH_GET_CMP_CFG(cfg->unprotect_last_block);
	case FLASH_PROTECT_ALL:
	default:
		return FLASH_GET_CMP_CFG(cfg->protect_all);
	}
}

static void flash_set_protect_cfg(uint32_t *status_reg_val, uint32_t new_protect_cfg)
{
	const flash_min_config_t *cfg = flash_min_cfg();

	*status_reg_val &= ~(cfg->protect_mask << cfg->protect_post);
	*status_reg_val |= ((new_protect_cfg & cfg->protect_mask) << cfg->protect_post);
}

static void flash_set_cmp_cfg(uint32_t *status_reg_val, uint32_t new_cmp_cfg)
{
	const flash_min_config_t *cfg = flash_min_cfg();

	*status_reg_val &= ~(FLASH_CMP_MASK << cfg->cmp_post);
	*status_reg_val |= ((new_cmp_cfg & FLASH_CMP_MASK) << cfg->cmp_post);
}

static bool flash_is_need_update_status_reg(uint32_t protect_cfg, uint32_t cmp_cfg, uint32_t status_reg_val)
{
	const flash_min_config_t *cfg = flash_min_cfg();
	uint32_t cur_protect = (status_reg_val >> cfg->protect_post) & cfg->protect_mask;
	uint32_t cur_cmp = (status_reg_val >> cfg->cmp_post) & FLASH_CMP_MASK;

	return (cur_protect != protect_cfg) || (cur_cmp != cmp_cfg);
}

static void flash_set_protect_type(flash_protect_type_t type)
{
	const flash_min_config_t *cfg = flash_min_cfg();
	uint32_t protect_cfg = flash_get_protect_cfg(type);
	uint32_t cmp_cfg = flash_get_cmp_cfg(type);
	uint32_t status_reg = flash_ll_read_status_reg(s_flash_hal.hw, cfg->status_reg_size);

	if (flash_is_need_update_status_reg(protect_cfg, cmp_cfg, status_reg)) {
		flash_set_protect_cfg(&status_reg, protect_cfg);
		flash_set_cmp_cfg(&status_reg, cmp_cfg);
		flash_ll_write_status_reg(s_flash_hal.hw, cfg->status_reg_size, status_reg);
	}
}

/* Set the QE bit so the device accepts quad IO (before restoring QUAD read). */
static void flash_set_qe(void)
{
	const flash_min_config_t *cfg = flash_min_cfg();
	uint32_t status_reg;

	flash_ll_wait_op_done(s_flash_hal.hw);
	status_reg = flash_ll_read_status_reg(s_flash_hal.hw, cfg->status_reg_size);
	if (status_reg & (cfg->quad_en_val << cfg->quad_en_post)) {
		return;
	}
	status_reg |= cfg->quad_en_val << cfg->quad_en_post;
	flash_ll_write_status_reg(s_flash_hal.hw, cfg->status_reg_size, status_reg);
}

/* Unprotect the whole device once, on the first serial-download handshake, so
 * download/BL2 erase/program can write any sector. Deferred out of
 * bk_flash_driver_init() so the read-only secure-boot verify path keeps the
 * persistent write protection; normal (DIRECT_XIP, no-swap) boot never writes
 * flash and stays protected.
 *
 * WRSR is an op_sw command: it MUST be issued in two-line mode - while the
 * device is in QUAD continuous-read it ignores op_sw and the busy poll never
 * clears, hanging the CPU. So bracket the status-register write: switch to
 * two-line, write, then restore the original (configured) line mode. The caller
 * (flash_op_enable_ctrl in download_flash_adapter.c) invokes this while flash is
 * still in the configured line mode, before switching the session to two-line.
 * Idempotent: guarded so repeated download commands only issue the WRSR once. */
void bk_flash_min_unprotect_once(void)
{
	static uint8_t s_flash_is_unlocked;

	if (!s_flash_is_unlocked) {
		bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
		flash_set_protect_type(FLASH_PROTECT_NONE);
		s_flash_is_unlocked = 1;
		bk_flash_set_line_mode(flash_min_cfg()->line_mode);
	}
}

flash_line_mode_t bk_flash_get_line_mode(void)
{
	return flash_min_cfg()->line_mode;
}

bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode)
{
	/* Ported from flash_driver.c, minus sys_drv_set_sys2flsh_2wire() (that
	 * sys_driver dep is not in armino_min; clear_qwfr + mode_sel suffices). */
	const flash_min_config_t *cfg = flash_min_cfg();

	flash_ll_clear_qwfr(s_flash_hal.hw);
	if (line_mode == FLASH_LINE_MODE_TWO) {
		flash_ll_set_dual_mode(s_flash_hal.hw);
	} else if (line_mode == FLASH_LINE_MODE_FOUR) {
		flash_ll_set_quad_m_value(s_flash_hal.hw, cfg->coutinuous_read_mode_bits_val);
		if (cfg->quad_en_val == 1) {
			flash_set_qe();
		}
		flash_ll_set_mode(s_flash_hal.hw, FLASH_MODE_QUAD);
	}
	return BK_OK;
}


bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	if (!user_buf) {
		return BK_ERR_NULL_PARAM;
	}
	if (size == 0) {
		return BK_OK;
	}

	/* Page-program in 32-byte units (mirrors flash_write_common): fill the
	 * 8-word FIFO, then issue PP (op_sw drives WREN + waits for busy). Partial
	 * units are padded with 0xFF. Caller must be out of QUAD continuous-read
	 * (download does this via bk_flash_min_switch_line_mode_two()). */
	uint32_t buf[FLASH_MIN_BUFFER_LEN];
	uint8_t *pb = (uint8_t *)&buf[0];
	uint32_t addr = address & (~FLASH_MIN_ADDRESS_MASK);
	uint32_t len = size;

	while (len) {
		for (uint32_t i = 0; i < FLASH_MIN_BYTES_CNT; i++) {
			pb[i] = FLASH_MIN_ERASED_VALUE;
		}
		for (uint32_t i = address % FLASH_MIN_BYTES_CNT; i < FLASH_MIN_BYTES_CNT; i++) {
			pb[i] = *user_buf++;
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}

		flash_hal_wait_op_done(&s_flash_hal);
		for (uint32_t i = 0; i < FLASH_MIN_BUFFER_LEN; i++) {
			flash_hal_write_data(&s_flash_hal, buf[i]);
		}
		flash_hal_set_op_cmd_write(&s_flash_hal, addr);

		addr += FLASH_MIN_BYTES_CNT;
	}
	return BK_OK;
}

bk_err_t bk_flash_erase_sector(uint32_t address)
{
	/* 4KB sector erase; caller must be in non-continuous line mode. */
	flash_hal_erase_block(&s_flash_hal, address & (~FLASH_MIN_SECTOR_MASK), FLASH_OP_CMD_SE);
	return BK_OK;
}

/* BL2 serial-download backend helpers. The download CMake target lacks the SDK
 * soc/hal include paths, so the HAL-touching code lives here and
 * common/download/src/flash/download_flash_adapter.c just forwards to these. */

void bk_flash_min_switch_line_mode_two(void)
{
	/* Leave QUAD continuous-read so op_sw erase/PP/SR are accepted. */
	if (flash_min_cfg()->line_mode == FLASH_LINE_MODE_FOUR) {
		bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
	}
}

void bk_flash_min_restore_line_mode(void)
{
	if (flash_min_cfg()->line_mode == FLASH_LINE_MODE_FOUR) {
		bk_flash_set_line_mode(FLASH_LINE_MODE_FOUR);
	}
}

void bk_flash_min_erase(uint32_t address, int type)
{
	/* download FLASH_OPCODE_SE/BE1/BE2 == flash_op_cmd_t values. */
	flash_ll_erase_block(s_flash_hal.hw, address, type);
}

uint16_t bk_flash_min_read_sr(uint8_t sr_width)
{
	return (uint16_t)flash_ll_read_status_reg(s_flash_hal.hw, sr_width);
}

void bk_flash_min_write_sr(uint8_t sr_width, uint16_t sr_data)
{
	flash_ll_write_status_reg(s_flash_hal.hw, sr_width, sr_data);
}

uint32_t bk_flash_min_get_id(void)
{
	return flash_ll_get_id(s_flash_hal.hw);
}

void flash_set_xip_offset(uint32_t primary_start, uint32_t secondary_start,
			  uint32_t code_size)
{
	flash_hw_t *hw = (flash_hw_t *)FLASH_LL_REG_BASE(0);

	flash_ll_set_offset_addr_begin(hw, primary_start);
	flash_ll_set_offset_addr_end(hw, primary_start + code_size);
	flash_ll_set_addr_offset(hw, secondary_start - primary_start);
}

/*实现一个函数，设置falsh 的0xb[25] 置1 的函数*/
void flash_set_ota_enable(bool enable)
{
	flash_ll_set_ota_enable(s_flash_hal.hw, enable);
}

bool flash_get_ota_enable_value(void)
{
	return flash_ll_get_ota_enable_value(s_flash_hal.hw);
}

int bk_flash_set_dbus_security_region(uint32_t id, uint32_t start, uint32_t end, bool secure)
{
	if (id >= SOC_SEC_ADDR_NUM) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	if (end <= start) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	flash_hw_t *hw = s_flash_hal.hw;
	hw->sec_addr[id].sec_addr_start.flash_sec_start_addr = start & 0xFFFFFF;
	hw->sec_addr[id].sec_addr_end.flash_sec_end_addr = end & 0xFFFFFF;
	hw->sec_addr[id].sec_addr_start.flash_sec_addr_en = secure ? 1 : 0;
	return BK_OK;
}


#include "cmsis_gcc.h"
extern void flush_all_dcache(void);
void flash_set_excute_enable(int enable)
{
    __disable_irq();
	/* bk7259 flash_hal.h has no set_offset_enable(hal, en); use the pair. */
	if (enable) {
		flash_hal_offset_enable(&s_flash_hal);
	} else {
		flash_hal_offset_disable(&s_flash_hal);
	}
	flush_all_dcache();
	__enable_irq();
}


uint32_t flash_get_excute_enable()
{
	return flash_hal_read_offset_enable(&s_flash_hal);
}

/* DIRECT_XIP A/B debug: expose the flash remap delta (secondary_start -
 * primary_start) so BL2 can verify the remap is actually programmed before it
 * reads the secondary slot through the primary XIP window. */
uint32_t flash_get_addr_offset(void)
{
	return flash_ll_get_addr_offset(s_flash_hal.hw);
}