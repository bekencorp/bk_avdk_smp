// Copyright 2025 Beken
//
// BK7259 TF-M armino_min: minimal EFUSE read driver.
//
// Secure boot reads a few efuse bytes (secure-boot policy bits). This is the
// read path of the SDK efuse_hal/efuse_driver, reusing the pure inline
// efuse_ll.h register accessors. Write path is provided as a symbol but is not
// exercised during boot. The upper-level efuse policy helpers
// (efuse_is_secureboot_enabled / efuse_get_value / bk_efuse_init) live in
// common/secure/bk_efuse.c and call bk_efuse_driver_init + bk_efuse_read_byte.

#include <stdint.h>
#include <common/bk_include.h>
#include <driver/efuse.h>
#include "efuse_ll.h"

static efuse_hw_t *const s_efuse_hw = (efuse_hw_t *)SOC_EFUSE_REG_BASE;

bk_err_t bk_efuse_driver_init(void)
{
	efuse_ll_init(s_efuse_hw);
	return BK_OK;
}

bk_err_t bk_efuse_driver_deinit(void)
{
	return BK_OK;
}

bk_err_t bk_efuse_read_byte(uint8_t addr, uint8_t *data)
{
	if (!data) {
		return BK_ERR_NULL_PARAM;
	}

	efuse_ll_set_direction_read(s_efuse_hw);
	efuse_ll_set_addr(s_efuse_hw, addr);
	efuse_ll_enable(s_efuse_hw);

	while (!efuse_ll_is_operate_finished(s_efuse_hw)) {
		;
	}

	if (efuse_ll_is_rd_data_valid(s_efuse_hw)) {
		*data = efuse_ll_get_rd_data(s_efuse_hw);
		return BK_OK;
	}
	return BK_ERR_EFUSE_READ_FAIL;
}

bk_err_t bk_efuse_write_byte(uint8_t addr, uint8_t data)
{
	/* Secure boot never writes efuse; provide the contract symbol only. */
	(void)addr;
	(void)data;
	return BK_OK;
}
