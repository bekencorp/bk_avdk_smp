// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: table-driven flash driver.
// Serves the secure-boot verify path and the BL2 serial-download backend.
//
// The flash erase/write/read algorithm, the status-register / QE handling, the
// per-op protection and the line-mode switch are all delegated to the shared
// portable core (cp/middleware/soc/common/flash_core/bk_flash_core.c) - the same
// implementation the CP/AP middleware and the aboot bootloader use, driven by the
// single shared flash_config[] table (flash_core_config.c). Only the secure-world
// / SoC-specific glue stays here: XIP cbus reads, DBUS security regions, the
// DIRECT_XIP remap/offset, the OTA-enable bit and the download-protocol host-width
// WRSR (RDSR uses flash_core_read_status_reg()).
//
// Concurrency: the secure world runs this path single-threaded and PPC isolation
// is handled one layer up (Driver_Flash.c), so no core port is installed and the
// core's default nop critical section preserves the previous no-locking behavior.
// Protection policy is PER_OP throughout (no session-wide unprotect):
//   - init applies FLASH_PROTECT_ALL (volatile) so the read-only secure-boot
//     verify path and normal DIRECT_XIP boot stay write-protected;
//   - every erase/write (and write_cbus) self-brackets unprotect -> op ->
//     re-protect(FLASH_PROTECT_ALL), so flash is re-protected after each op,
//     including throughout a BL2 serial-download session - each op unprotects
//     itself, so no handshake unprotect is needed.
// The shared core always RDSR before protect / QE RMW and refreshes its SR cache.

#include <stdint.h>
#include <stdbool.h>
#include <common/bk_include.h>
#include <driver/flash.h>
#include "flash_hal.h"
#include "flash_ll.h"
#include "flash_layout.h"
#include "tfm_flash_partition.h"  /* SOC_FLASH_BASE_ADDR */
#include "cmsis_gcc.h"
#include "flash_core_config.h"
#include "bk_flash_core.h"

extern void flush_all_dcache(void);

#ifndef SOC_SEC_ADDR_NUM
#define SOC_SEC_ADDR_NUM        4   /* DBUS security regions */
#endif

#define FLASH_MIN_SECTOR_MASK   0xfff   /* 4KB sector */

static bool s_flash_min_inited;

/* Not declared in driver/flash.h; used before its definition below. */
bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode);

bk_err_t bk_flash_driver_init(void)
{
	if (s_flash_min_inited) {
		return BK_OK;
	}

	/* Bind the HAL to the flash controller and soft-reset it (flash_ll_init).
	 * No core port is installed: the secure world drives this single-threaded,
	 * so the default nop critical section keeps the previous behavior. */
	flash_core_hal_init();

	/* BL2 leaves the flash device in QUAD continuous-read (for XIP). In that
	 * state the device ignores the distinct-opcode op_sw commands (RDID / RDSR),
	 * so a plain RDID busy-poll never clears -> the secure image hangs here
	 * (observed: markers reach 'R' then stop, no 'I'). Data reads still work in
	 * continuous-read: memory-mapped code fetch and op_sw READ both use the same
	 * read pattern the device is primed for. CRMR (clear_qwfr) is honored even
	 * while in continuous read, so issue it first to drop back to normal command
	 * mode before RDID. */
	flash_hal_clear_qwfr(flash_core_hal());

	flash_core_identify();
	s_flash_min_inited = true;

	/* Apply the default write-protect (FLASH_PROTECT_ALL) before restoring QUAD
	 * XIP. This is the steady state: erase/write self-unprotect PER_OP and
	 * re-protect to this runtime type after each op, so flash is protected
	 * between all ops (verify path, download session and OTA alike). */
	flash_core_set_runtime_protect_type(FLASH_PROTECT_ALL);
	flash_core_protect();

	/* Restore QUAD continuous-read so the rest of TF-M and the NS app run XIP at
	 * full speed, matching the state BL2 handed over. */
	bk_flash_set_line_mode(flash_core_get_line_mode());
	return BK_OK;
}

static void flash_min_ensure_init(void)
{
	if (!s_flash_min_inited) {
		bk_flash_driver_init();
	}
}

bk_err_t bk_flash_driver_deinit(void)
{
	s_flash_min_inited = false;
	return BK_OK;
}

uint32_t bk_flash_get_current_total_size(void)
{
	/* Matched device size; unknown parts fall back to the default table entry. */
	flash_min_ensure_init();
	return flash_core_get_total_size();
}

bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	if (!user_buf) {
		return BK_ERR_NULL_PARAM;
	}
	if (size == 0) {
		return BK_OK;
	}
	return flash_core_read(user_buf, address, size);
}

__attribute__((section(".iram"))) void bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size)
{
	/* Cacheable (XIP) view, XTS-decrypted on the fly. Secure base only
	 * (SOC_FLASH_BASE_ADDR), not SOC_FLASH_DATA_BASE / NS alias. */
	const volatile uint8_t *src = (const volatile uint8_t *)(SOC_FLASH_BASE_ADDR + address);
	uint8_t *dst = (uint8_t *)user_buf;
	for (uint32_t i = 0; i < size; i++) {
		dst[i] = src[i];
	}
}

#if CONFIG_OTA_OVERWRITE
/* Only the compressed/encrypted overwrite-OTA path needs the CPU-write window. */
static void flash_wt_copy(volatile uint8_t *dst8, const uint8_t *user_buf, uint32_t size)
{
	if (((((uintptr_t)dst8) | ((uintptr_t)user_buf)) & 3u) == 0u) {
		volatile uint32_t *d32 = (volatile uint32_t *)dst8;
		const uint32_t *s32 = (const uint32_t *)user_buf;
		uint32_t words = size >> 2;
		for (uint32_t i = 0; i < words; i++) {
			d32[i] = s32[i];
		}
		for (uint32_t i = words << 2; i < size; i++) {
			dst8[i] = user_buf[i];
		}
	} else {
		for (uint32_t i = 0; i < size; i++) {
			dst8[i] = user_buf[i];
		}
	}
}

/* Cacheable (XIP) write window: the flash controller XTS-encrypts on the fly.
 * Used by the BL2 compressed/encrypted overwrite-OTA path (decompress_bl2.c).
 * Uses the shared core HAL handle for the CPU-write window and op-done wait.
 *
 * Self-brackets BOTH axes (PER_OP), so this path is fully self-contained and
 * needs no session state from the caller (the old switch_line_mode_two +
 * unprotect_once helpers are gone): drop to two-line (the CPU-write window
 * programs in two-line, like op_sw PP), volatile-unprotect, program, re-protect,
 * restore the ambient line mode. Order matters for cost: set(TWO) first so the
 * unprotect/protect internal line brackets collapse to cache no-ops. The
 * brackets sit OUTSIDE the __disable_irq window (their WRSR/line pokes are fine
 * with IRQs enabled in the single-threaded secure world); IRQ-off wraps only the
 * CPU-write burst. */
__attribute__((section(".iram"))) void bk_flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	volatile uint8_t *dst8 = (volatile uint8_t *)(SOC_FLASH_BASE_ADDR + address);

	flash_line_mode_t old_lm = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_core_unprotect();

	uint32_t primask = __get_PRIMASK();
	__disable_irq();

	flash_core_cpu_wr_enable();
	flash_wt_copy(dst8, user_buf, size);
	flash_hal_wait_op_done(flash_core_hal());
	flash_core_cpu_wr_disable();

	/* Fresh ciphertext in flash; drop L1+L2 so readback/hash re-fetches. */
	__DSB();
	flush_all_dcache();
	if (!primask) {
		__enable_irq();
	}

	flash_core_protect();
	flash_core_set_line_mode(old_lm);
}
#endif /* CONFIG_OTA_OVERWRITE */

flash_line_mode_t bk_flash_get_line_mode(void)
{
	flash_min_ensure_init();
	return flash_core_get_line_mode();
}

bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode)
{
	flash_core_set_line_mode(line_mode);
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

	/* Page-program in 32-byte units. PER_OP policy: flash_core_write brackets
	 * both the line mode AND the protection (unprotect -> PP -> re-protect); the
	 * read-only secure-boot path never calls this. */
	return flash_core_write(address, user_buf, size);
}

bk_err_t bk_flash_erase_sector(uint32_t address)
{
	/* 4KB sector erase; caller must be in non-continuous line mode. */
	return flash_core_erase(address & (~FLASH_MIN_SECTOR_MASK), FLASH_SECTOR_SIZE);
}

bk_err_t bk_flash_erase_block_32k(uint32_t address)
{
	return flash_core_erase(address & (~(FLASH_BLOCK32_SIZE - 1)), FLASH_BLOCK32_SIZE);
}

bk_err_t bk_flash_erase_block_64k(uint32_t address)
{
	return flash_core_erase(address & (~(FLASH_BLOCK_SIZE - 1)), FLASH_BLOCK_SIZE);
}

/* BL2 serial-download backend helpers. The download CMake target lacks the SDK
 * soc/hal include paths, so the HAL-touching code lives here and
 * common/download/src/flash/download_flash_adapter.c just forwards to these.
 *
 * No session-level line-mode bracket is exposed anymore: every op_sw verb
 * (erase / write / read_sr / write_sr / get_id) self-brackets to two-line and
 * restores the ambient QUAD continuous-read, and data reads work in four-line,
 * so callers just issue ops directly. */

void bk_flash_min_erase(uint32_t address, int type)
{
	/* download FLASH_OPCODE_SE/BE1/BE2 == flash_op_cmd_t values. */
	uint32_t size;

	switch (type) {
	case FLASH_OP_CMD_BE1:
		size = FLASH_BLOCK32_SIZE;
		break;
	case FLASH_OP_CMD_BE2:
		size = FLASH_BLOCK_SIZE;
		break;
	case FLASH_OP_CMD_SE:
	default:
		size = FLASH_SECTOR_SIZE;
		break;
	}
	flash_core_erase(address & ~(size - 1), size);
}

uint16_t bk_flash_min_read_sr(uint8_t sr_width)
{
	/* Align with aboot: use the public atomic RDSR (lock + two-line + RDSR +
	 * restore). Width comes from the resolved config's status_reg_size; the
	 * download protocol currently hard-codes flash_read_sr(1) and only uses
	 * the low byte. */
	(void)sr_width;
	return (uint16_t)flash_core_read_status_reg();
}

void bk_flash_min_write_sr(uint8_t sr_width, uint16_t sr_data)
{
	/* Self-bracket like flash_core_read_status_reg: WRSR is ignored in QUAD
	 * continuous-read. Keep host width (download REG_WRITE). TF-M is
	 * single-threaded, so no outer op lock. Core has no public host-width WRSR. */
	flash_line_mode_t old = flash_core_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_ll_write_status_reg(flash_core_hal()->hw, sr_width, sr_data);
	flash_core_set_line_mode(old);
}

uint32_t bk_flash_min_get_id(void)
{
	flash_core_identify();
	return flash_core_get_id();
}

void flash_set_xip_offset(uint32_t primary_start, uint32_t secondary_start,
			  uint32_t code_size)
{
	/* Init-independent (BL2 may program the remap before flash_core_hal_init): keep
	 * the direct controller base rather than the core's HAL handle. */
	flash_hw_t *hw = (flash_hw_t *)FLASH_LL_REG_BASE(0);

	flash_ll_set_offset_addr_begin(hw, primary_start);
	flash_ll_set_offset_addr_end(hw, primary_start + code_size);
	flash_ll_set_addr_offset(hw, secondary_start - primary_start);
}

/*实现一个函数，设置falsh 的0xb[25] 置1 的函数*/
void flash_set_ota_enable(bool enable)
{
	flash_ll_set_ota_enable(flash_core_hal()->hw, enable);
}

bool flash_get_ota_enable_value(void)
{
	return flash_ll_get_ota_enable_value(flash_core_hal()->hw);
}

int bk_flash_set_dbus_security_region(uint32_t id, uint32_t start, uint32_t end, bool secure)
{
	if (id >= SOC_SEC_ADDR_NUM) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	if (end <= start) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	flash_hw_t *hw = flash_core_hal()->hw;
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
		flash_hal_offset_enable(flash_core_hal());
	} else {
		flash_hal_offset_disable(flash_core_hal());
	}
	flush_all_dcache();
	__enable_irq();
}


uint32_t flash_get_excute_enable()
{
	return flash_hal_read_offset_enable(flash_core_hal());
}

/* DIRECT_XIP A/B debug: expose the flash remap delta (secondary_start -
 * primary_start) so BL2 can verify the remap is actually programmed before it
 * reads the secondary slot through the primary XIP window. */
uint32_t flash_get_addr_offset(void)
{
	return flash_ll_get_addr_offset(flash_core_hal()->hw);
}
