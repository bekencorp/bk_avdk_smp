/*
 * BK7259 BL2-download flash adapter.
 *
 * The bk7236n download shipped its own SPI/flash driver providing these symbols;
 * that file was not part of this download drop. On bk7259 the real flash HAL
 * (flash_hal.h / flash_ll.h, cp/middleware/soc/...) is only reachable from the
 * armino_min target's include paths, NOT from this download CMake target. So the
 * HAL-touching implementation lives in bk7259/armino_min/hal/flash_min.c as
 * bk_flash_min_* helpers, and this file maps the symbol names expected by the
 * download protocol / boot update code onto those helpers.
 *
 * Real flash read/write also route through flash_min.c:
 *   flash_read_data()  -> bk_flash_read_bytes()   (already real)
 *   flash_write_data() -> bk_flash_write_bytes()  (now real, 32B page program)
 */

#include <stdint.h>
#include <common/bk_err.h>

/* Real HAL-backed helpers, implemented in bk7259/armino_min/hal/flash_min.c. */
extern void     bk_flash_min_erase(uint32_t address, int type);
extern uint16_t bk_flash_min_read_sr(uint8_t sr_width);
extern void     bk_flash_min_write_sr(uint8_t sr_width, uint16_t sr_data);
extern uint32_t bk_flash_min_get_id(void);

/* download_boot.h: flash_get_id() -> bk_flash_get_id(). Read on demand by the
 * FLASH_CMD_SPI_OPERATE (RDID) handler; no session state, no handshake needed
 * (RDID self-brackets to two-line internally). */
uint32_t bk_flash_get_id(void)
{
	return bk_flash_min_get_id();
}

/* download_boot.h: flash_erase_cmd(addr, cmd) -> bk_flash_erase_cmd(addr, type).
 * download FLASH_OPCODE_SE/BE1/BE2 (13/14/15) == flash_op_cmd_t FLASH_OP_CMD_*. */
bk_err_t bk_flash_erase_cmd(uint32_t address, int type)
{
	bk_flash_min_erase(address, type);
	return BK_OK;
}

/* download_boot.h: flash_read_sr(byte) -> bk_flash_read_sr(byte). */
uint16_t bk_flash_read_sr(unsigned char byte)
{
	return bk_flash_min_read_sr((uint8_t)byte);
}

/* download_boot.h: flash_write_sr(bytes, data) -> bk_flash_write_sr(...). */
bk_err_t bk_flash_write_sr(unsigned char bytes, uint16_t status_reg_data)
{
	bk_flash_min_write_sr((uint8_t)bytes, status_reg_data);
	return BK_OK;
}
