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

#include <common/bk_include.h>
#include <components/ate.h>
#include <os/mem.h>
#include <driver/flash.h>
#include <os/os.h>
#include "bk_pm_model.h"
#include "flash_driver.h"
#include "flash_hal.h"
#include "sys_driver.h"
#include "driver/flash_partition.h"
#include <modules/chip_support.h>
#include "Driver_Flash.h"

#if CONFIG_FLASH_QUAD_ENABLE
extern UINT8 flash_get_line_mode(void);
extern void flash_set_line_mode(UINT8 mode);
#endif

typedef struct {
	flash_hal_t hal;
	uint32_t flash_id;
	const flash_config_t *flash_cfg;
} flash_driver_t;

#define FLASH_GET_PROTECT_CFG(cfg) ((cfg) & FLASH_STATUS_REG_PROTECT_MASK)
#define FLASH_GET_CMP_CFG(cfg)     (((cfg) >> FLASH_STATUS_REG_PROTECT_OFFSET) & FLASH_STATUS_REG_PROTECT_MASK)

#define FLASH_RETURN_ON_DRIVER_NOT_INIT() do {\
	if (!s_flash_is_init) {\
		return BK_ERR_FLASH_NOT_INIT;\
	}\
} while(0)

#define FLASH_RETURN_ON_WRITE_ADDR_OUT_OF_RANGE(addr, len) do {\
	if ((addr >= s_flash.flash_cfg->flash_size) ||\
		(len > s_flash.flash_cfg->flash_size) ||\
		((addr + len) > s_flash.flash_cfg->flash_size)) {\
		FLASH_LOGW("write error[addr:0x%x len:0x%x]\r\n", addr, len);\
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;\
	}\
} while(0)

#if 1
static const flash_config_t flash_config[] = {
	/* flash_id, status_reg_size, flash_size,    line_mode,           cmp_post, protect_post, protect_mask, protect_all, protect_none, protect_half, unprotect_last_block. quad_en_post, quad_en_val, coutinuous_read_mode_bits_val, mode_sel*/
	{0x1C7016,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x00,         0x16,         0x01B,                0,            0,           0xA5,                          0x01}, //en_25qh32b
	{0x1C7015,   1,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x00,         0x0d,         0x0d,                 0,            0,           0xA5,                          0x01}, //en_25qh16b
	{0x0B4014,   2,               FLASH_SIZE_1M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0C,         0x101,                9,            1,           0xA0,                          0x01}, //xtx_25f08b
	{0x0B4015,   2,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //xtx_25f16b
#if CONFIG_FLASH_QUAD_ENABLE
	{0x0B4016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,           1,            0xA0,                          0x02}, //xtx_25f32b
#else
	{0x0B4016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,            1,           0xA0,                          0x01}, //xtx_25f32b
#endif
	{0x0B4017,   2,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x05,        0x00,         0x0E,         0x109,                9,            1,           0xA0,                          0x01}, //xtx_25f64b
#if CONFIG_FLASH_QUAD_ENABLE
	{0x0B6018,   2,               FLASH_SIZE_16M, FLASH_LINE_MODE_FOUR,  0,	    2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                9,            1,           0xA0,                          0x02}, //xt_25q128d
#else
	{0x0B6018,   1,               FLASH_SIZE_16M, FLASH_LINE_MODE_TWO,   0,     2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                0,            0,           0xA0,                          0x01}, //xt_25q128d
#endif
	{0x0E4016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,            1,           0xA0,                          0x01}, //xtx_FT25H32
	{0x1C4116,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                0,            0,           0xA0,                          0x01}, //en_25qe32a(not support 4 line)
	{0x5E5018,   1,               FLASH_SIZE_16M, FLASH_LINE_MODE_TWO, 0, 	    2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                0,            0,           0xA0,                          0x01}, //zb_25lq128c
	{0xC84015,   2,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //gd_25q16c
	{0xC84017,   1,               FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //gd_25q16c
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC84016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                9,            1,           0xA0,                          0x02}, //gd_25q32c
#else
	{0xC84016,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                0,            0,           0xA0,                          0x01}, //gd_25q32c
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC86018,   2,               FLASH_SIZE_16M,FLASH_LINE_MODE_FOUR, 0,       2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                9,            1,           0xA0,                          0x02}, //gd_25lq128e
#else
	{0xC86018,   1,               FLASH_SIZE_16M,FLASH_LINE_MODE_TWO,  0,       2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                0,            0,           0xA0,                          0x01}, //gd_25lq128e
#endif
	{0xC86515,   2,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //gd_25w16e
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC86516,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_FOUR, 14,      2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                9,            1,           0xA0,                          0x02}, //en_25qe32a
#else
	{0xC86516,   1,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x1F,        0x00,         0x0E,         0x00E,                0,            0,           0xA0,                          0x01}, //en_25qe32a
#endif
	{0xEF4016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x00,         0x101,                9,            1,           0xA0,                          0x01}, //w_25q32(bfj)
#if CONFIG_FLASH_QUAD_ENABLE
	{0x204118,	 2, 			  FLASH_SIZE_16M,FLASH_LINE_MODE_FOUR, 0,		2,			  0x0F, 		0x0F,		 0x00,		   0x0A,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //xm_25qu128c
#else
	{0x204118,	 1, 			  FLASH_SIZE_16M,FLASH_LINE_MODE_TWO,  0,		2,			  0x0F, 		0x0F,		 0x00,		   0x0A,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //xm_25qu128c
#endif
	{0x204016,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0E,         0x101,                9,            1,           0xA0,                          0x01}, //xmc_25qh32b
	{0xC22315,   1,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 0,        2,            0x0F,         0x0F,        0x00,         0x0A,         0x00E,                6,            1,           0xA5,                          0x01}, //mx_25v16b
	{0xEB6015,   2,               FLASH_SIZE_2M, FLASH_LINE_MODE_TWO, 14,       2,            0x1F,         0x1F,        0x00,         0x0D,         0x101,                9,            1,           0xA0,                          0x01}, //zg_th25q16b
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC86517,	 2, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //gd_25Q32E
#else
	{0xC86517,	 1, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 0,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //gd_25Q32E
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	/* status_reg_size=2 per gerrit 101889 (was wrongly 3). */
	{0xCD6017,	 2, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //th_25q64ha
#else
	{0xCD6017,	 1, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 0,		2,			  0x1F, 		0x1F,		 0x00,		   0x0E,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //th_25q64ha
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0xCD7017,	 2, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,		2,			  0x1F, 		0x1F,		 0x00,		   0x09,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //TH25Q64UB
#else
	{0xCD7017,	 1, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 0,		2,			  0x1F, 		0x1F,		 0x00,		   0x09,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //TH25Q64UB
#endif
	{0x000000,   2,               FLASH_SIZE_4M, FLASH_LINE_MODE_TWO, 0,        2,            0x1F,         0x00,        0x00,         0x00,         0x000,                0,            0,           0x00,                          0x01}, //default
};
#else
static const flash_config_t flash_config[] = {
	/* flash_id, status_reg_size, flash_size,    line_mode,           cmp_post, protect_post, protect_mask, protect_all, protect_none, protect_half, unprotect_last_block. quad_en_post, quad_en_val, coutinuous_read_mode_bits_val, mode_sel*/
#if CONFIG_FLASH_QUAD_ENABLE
	{0xC86517,	 2, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,		2,			  0x1F, 		0x1F,		 0x00,		   0x09,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //gd_25Q32E (protect_half=9: protect first 128K)
#else
	{0xC86517,	 1, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 0,		2,			  0x1F, 		0x1F,		 0x00,		   0x09,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //gd_25Q32E (protect_half=9: protect first 128K)
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0xCD6017,	 2, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,		2,			  0x1F, 		0x1F,		 0x00,		   0x09,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //th_25q64ha
#else
	{0xCD6017,	 1, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 0,		2,			  0x1F, 		0x1F,		 0x00,		   0x09,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //th_25q64ha
#endif
#if CONFIG_FLASH_QUAD_ENABLE
	{0xCD7017,	 2, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_FOUR, 14,		2,			  0x1F, 		0x1F,		 0x00,		   0x09,		 0x00E, 			   9,			 1, 		  0xA0, 						 0x02}, //TH25Q64UB
#else
	{0xCD7017,	 1, 			  FLASH_SIZE_8M, FLASH_LINE_MODE_TWO, 0,		2,			  0x1F, 		0x1F,		 0x00,		   0x09,		 0x00E, 			   0,			 0, 		  0xA0, 						 0x01}, //TH25Q64UB
#endif
};
#endif
unsigned int flash_id = 0;
static flash_driver_t s_flash = {0};
static bool s_flash_is_init = false;
static flash_protect_type_t s_flash_runtime_protect_type = FLASH_PROTECT_NONE;

//static PM_STATUS flash_ps_status;
#if (CONFIG_SOC_BK7256XX)
static uint32_t s_hold_low_speed_status = 0;
#endif

#define FLASH_MAX_WAIT_CB_CNT (4)
 
#if 0    //optimize code size
static flash_wait_callback_t s_flash_wait_cb[FLASH_MAX_WAIT_CB_CNT] = {NULL};


__attribute__((section(".itcm_sec_code"))) void flash_waiting_cb(void)
{
	uint32_t i = 0;

	for(i = 0; i < FLASH_MAX_WAIT_CB_CNT; i++)
	{
		if(s_flash_wait_cb[i])
		{
			s_flash_wait_cb[i]();
		}
	}
}
#endif
void flash_switch_to_line_mode_two(void)
{
	if (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode())
		bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
}

void flash_restore_line_mode(void)
{
	if (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode())
		bk_flash_set_line_mode(FLASH_LINE_MODE_FOUR);
}
#if 0 //code size optimize
static PM_STATUS flash_ps_get_status(UINT32 flag)
{
	return flash_ps_status;
}

static DEV_PM_OPS_S flash_ps_ops = {
	.pm_init = NULL,
	.pm_deinit = NULL,
	.suspend = flash_switch_to_line_mode_two,
	.resume = flash_restore_line_mode,
	.status = flash_ps_get_status,
	.get_sleep_time = NULL,
};
#endif
static void flash_get_current_config(void)
{
	bool cfg_success = false;

	for (uint32_t i = 0; i < (ARRAY_SIZE(flash_config) - 1); i++) {
		if (s_flash.flash_id == flash_config[i].flash_id) {
			s_flash.flash_cfg = &flash_config[i];
			cfg_success = true;
			break;
		}
	}

	if (!cfg_success) {
		s_flash.flash_cfg = &flash_config[ARRAY_SIZE(flash_config) - 1];
		for(int i = 0; i < 10; i++) {
			FLASH_LOGE("This flash is not identified, choose default config\r\n");
		}
	}
}

static uint32_t flash_get_protect_cfg(flash_protect_type_t type)
{
	switch (type) {
	case FLASH_PROTECT_NONE:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_none);
	case FLASH_PROTECT_ALL:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_all);
	case FLASH_PROTECT_HALF:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_half);
	case FLASH_UNPROTECT_LAST_BLOCK:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->unprotect_last_block);
	default:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_all);
	}
}

static void flash_set_protect_cfg(uint32_t *status_reg_val, uint32_t new_protect_cfg)
{
	*status_reg_val &= ~(s_flash.flash_cfg->protect_mask << s_flash.flash_cfg->protect_post);
	*status_reg_val |= ((new_protect_cfg & s_flash.flash_cfg->protect_mask) << s_flash.flash_cfg->protect_post);
}

static uint32_t flash_get_cmp_cfg(flash_protect_type_t type)
{
	switch (type) {
	case FLASH_PROTECT_NONE:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->protect_none);
	case FLASH_PROTECT_ALL:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->protect_all);
	case FLASH_PROTECT_HALF:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->protect_half);
	case FLASH_UNPROTECT_LAST_BLOCK:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->unprotect_last_block);
	default:
		return FLASH_GET_CMP_CFG(s_flash.flash_cfg->protect_all);
	}
}

static void flash_set_cmp_cfg(uint32_t *status_reg_val, uint32_t new_cmp_cfg)
{
	*status_reg_val &= ~(FLASH_CMP_MASK << s_flash.flash_cfg->cmp_post);
	*status_reg_val |= ((new_cmp_cfg & FLASH_CMP_MASK) << s_flash.flash_cfg->cmp_post);
}

static bool flash_is_need_update_status_reg(uint32_t protect_cfg, uint32_t cmp_cfg, uint32_t status_reg_val)
{
	uint32_t cur_protect_val_in_status_reg = (status_reg_val >> s_flash.flash_cfg->protect_post) & s_flash.flash_cfg->protect_mask;
	uint32_t cur_cmp_val_in_status_reg = (status_reg_val >> s_flash.flash_cfg->cmp_post) & FLASH_CMP_MASK;

	if (cur_protect_val_in_status_reg != protect_cfg ||
		cur_cmp_val_in_status_reg != cmp_cfg) {
		return true;
	} else {
		return false;
	}
}

/*
 * Non-volatile status-register write (hardware WRSR).
 * For TH25Q64 (0xCD6017), flash_ll_write_status_reg() uses 0x01 then 0x31.
 * Do not use the volatile 0x50 bypass path.
 */
static void flash_write_status_reg_nonvolatile(uint8_t sr_width, uint32_t sr_data)
{
	flash_ll_write_status_reg(s_flash.hal.hw, sr_width, sr_data);
}

__attribute__((section(".iram"))) static void flash_write_status_reg(uint8_t sr_width, uint32_t status_reg)
{
	flash_write_status_reg_nonvolatile(sr_width, status_reg);
}

/* Read-modify-write the status register with an arbitrary protect/CMP config.
 * Shared by the enum-based flash_set_protect_type and the address-based dynamic
 * protection path. */
static void flash_apply_protect(uint32_t protect_cfg, uint32_t cmp_cfg)
{
	uint32_t status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);

	if (flash_is_need_update_status_reg(protect_cfg, cmp_cfg, status_reg)) {
		flash_set_protect_cfg(&status_reg, protect_cfg);
		flash_set_cmp_cfg(&status_reg, cmp_cfg);
		//FLASH_LOGD("write status reg:%x, status_reg_size:%d\r\n", status_reg, s_flash.flash_cfg->status_reg_size);
		flash_write_status_reg(s_flash.flash_cfg->status_reg_size, status_reg);
	}
}

static void flash_set_protect_type(flash_protect_type_t type)
{
	flash_apply_protect(flash_get_protect_cfg(type), flash_get_cmp_cfg(type));
}

/* Apply a flash protection type by programming the protect/cmp bits into the
 * status register through the non-volatile write path. */
static void flash_set_protect_type_nonvolatile(flash_protect_type_t type)
{
	uint32_t protect_cfg = flash_get_protect_cfg(type);
	uint32_t cmp_cfg = flash_get_cmp_cfg(type);
	uint32_t status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);

	if (flash_is_need_update_status_reg(protect_cfg, cmp_cfg, status_reg)) {
		flash_set_protect_cfg(&status_reg, protect_cfg);
		flash_set_cmp_cfg(&status_reg, cmp_cfg);
		flash_write_status_reg_nonvolatile(s_flash.flash_cfg->status_reg_size, status_reg);
	}
}

/*
 * Address-based dynamic protection (CMP=0 Lower-block, Spec Table 4).
 *
 * Protect [0, boundary) where boundary is the largest standard Lower region
 * (flash_size >> k, k=1..6 -> 1/2 .. 1/64 of the chip) that is still <= addr,
 * so the op address itself always stays writable. The BP value for the region
 * "flash_size >> k" is (0x0F - k): k=1 -> 0x0E (1/2) ... k=6 -> 0x09 (1/64).
 * Derived from the density, so it holds for any flash size with neither a
 * per-part size table nor a flash-ID branch.
 */
static uint32_t flash_addr_to_lower_bp(uint32_t addr, uint32_t flash_size)
{
	for (uint32_t k = 1; k <= 6; k++) {
		uint32_t region = flash_size >> k;
		if ((region != 0) && (region <= addr)) {
			return 0x0F - k;
		}
	}

	/* addr < flash_size/64: cannot protect anything below it without
	 * protecting the op address itself. */
	return 0x00;
}

/*
 * Unprotect strategy before erase/write, decided purely by the start address:
 * dynamically protect the largest Lower region (CMP=0) whose size is <= addr,
 * so [0, boundary) stays protected and addr remains writable. No flash-ID
 * special-casing and no fixed 128K threshold.
 */
static void flash_set_protect_before_op(uint32_t addr)
{
	flash_apply_protect(flash_addr_to_lower_bp(addr, s_flash.flash_cfg->flash_size), 0);
}

static void flash_set_qe(void)
{
	uint32_t status_reg;

	flash_hal_wait_op_done(&s_flash.hal);

	status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
	if (status_reg & (s_flash.flash_cfg->quad_en_val << s_flash.flash_cfg->quad_en_post)) {
		return;
	}

	status_reg |= s_flash.flash_cfg->quad_en_val << s_flash.flash_cfg->quad_en_post;
	flash_write_status_reg(s_flash.flash_cfg->status_reg_size, status_reg);
}

static void flash_set_qwfr(void)
{
	flash_hal_set_mode(&s_flash.hal, s_flash.flash_cfg->mode_sel);
}

static void flash_read_common(uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	uint32_t buf[FLASH_BUFFER_LEN] = {0};
	uint8_t *pb = (uint8_t *)&buf[0];

	if (len == 0) {
		return;
	}

	while (len) {
		SYS_LOCK_DECLARATION();
		SYS_LOCK();
		flash_hal_wait_op_done(&s_flash.hal);

		flash_hal_set_op_cmd_read(&s_flash.hal, addr);
		addr += FLASH_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_flash.hal);
		}
		SYS_UNLOCK();

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

#if 0//code size optimize
static void flash_read_word_common(uint32_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);
	uint32_t buf[FLASH_BUFFER_LEN] = {0};
	//nt8_t *pb = (uint8_t *)&buf[0];
	uint32_t *pb = (uint32_t *)&buf[0];

	if (len == 0) {
		return;
	}

	while (len) {

		flash_hal_wait_op_done(&s_flash.hal);

		flash_hal_set_op_cmd_read(&s_flash.hal, addr);
		addr += FLASH_BYTES_CNT;
		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_flash.hal);
		}

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
#endif
#if 0
//extern part_flag update_part_flag;
bool flash_is_area_write_disable(uint32_t addr)
{
	uint32_t firmware_area_end_address = 0;
	bk_logic_partition_t * flash_pt = NULL;

	flash_pt = bk_flash_partition_get_info(BK_PARTITION_BOOTLOADER);
	if(!flash_pt)
	{
		FLASH_LOGE("get partition ota fail\r\n");
		return true;
	}
	firmware_area_end_address = flash_pt->partition_start_addr + flash_pt->partition_length;
	if (addr < firmware_area_end_address) {
		FLASH_LOGE("valid write/erase start address = 0x%x, but current address = 0x%x.\r\n", firmware_area_end_address, addr);
		BK_ASSERT(addr >= firmware_area_end_address);
		return true;
	}
	return false;
}
#else
static bool flash_is_area_write_disable(uint32_t addr)
{
	return false;
}
#endif

static bk_err_t flash_write_common(const uint8_t *buffer, uint32_t address, uint32_t len)
{
	uint32_t buf[FLASH_BUFFER_LEN];
	uint8_t *pb = (uint8_t *)&buf[0];
	uint32_t addr = address & (~FLASH_ADDRESS_MASK);

	FLASH_RETURN_ON_WRITE_ADDR_OUT_OF_RANGE(addr, len);

	while (len) {
		os_memset(pb, 0xFF, FLASH_BYTES_CNT);
		for (uint32_t i = address % FLASH_BYTES_CNT; i < FLASH_BYTES_CNT; i++) {
			pb[i] = *buffer++;
			address++;
			len--;
			if (len == 0) {
				break;
			}
		}

		SYS_LOCK_DECLARATION();
		SYS_LOCK();
		flash_hal_wait_op_done(&s_flash.hal);

		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			flash_hal_write_data(&s_flash.hal, buf[i]);
		}
		flash_hal_set_op_cmd_write(&s_flash.hal, addr);
		SYS_UNLOCK();

		addr += FLASH_BYTES_CNT;
	}
	return BK_OK;
}

static void flash_enable_line_mode_switch(bool enable)
{
	if (enable) {
		sys_drv_set_sys2flsh_2wire(0);
	} else {
		sys_drv_set_sys2flsh_2wire(1);
	}
}

bk_err_t bk_flash_set_line_mode(flash_line_mode_t line_mode)
{
	flash_hal_clear_qwfr(&s_flash.hal);
	flash_enable_line_mode_switch(true);
	if (FLASH_LINE_MODE_TWO == line_mode) {
		flash_hal_set_dual_mode(&s_flash.hal);
	} else if (FLASH_LINE_MODE_FOUR == line_mode) {
		flash_hal_set_quad_m_value(&s_flash.hal, s_flash.flash_cfg->coutinuous_read_mode_bits_val);
		if (1 == s_flash.flash_cfg->quad_en_val) {
			flash_set_qe();
		}
		flash_set_qwfr();
	}
	flash_enable_line_mode_switch(false);
	return BK_OK;
}

__attribute__((section(".iram"))) bk_err_t bk_flash_cpu_write_enable(void)   //optimize code size
{
        flash_hal_enable_cpu_data_wr(&s_flash.hal);
        return BK_OK;
}

__attribute__((section(".iram"))) bk_err_t bk_flash_cpu_write_disable(void)
{
        flash_hal_disable_cpu_data_wr(&s_flash.hal);
        return BK_OK;
}

bk_err_t bk_flash_driver_init(void)
{
	if (s_flash_is_init) {
		return BK_OK;
	}
#if CONFIG_FLASH_QUAD_ENABLE
#if CONFIG_FLASH_ORIGIN_API
	if (FLASH_LINE_MODE_FOUR == flash_get_line_mode())
		flash_set_line_mode(FLASH_LINE_MODE_TWO);
#endif
#endif
	os_memset(&s_flash, 0, sizeof(s_flash));
	flash_hal_init(&s_flash.hal);
	bk_flash_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_id = s_flash.flash_id = flash_hal_get_id(&s_flash.hal);
	FLASH_LOGI("id=0x%x\r\n", s_flash.flash_id);
	flash_get_current_config();
	s_flash_runtime_protect_type = FLASH_PROTECT_ALL;
	flash_set_protect_type_nonvolatile(FLASH_PROTECT_ALL);
#if (0 == CONFIG_JTAG)
	flash_hal_disable_cpu_data_wr(&s_flash.hal);
#endif
	bk_flash_set_line_mode(s_flash.flash_cfg->line_mode);
	flash_hal_set_default_clk(&s_flash.hal);

#if (CONFIG_SOC_BK7256XX)
	#if CONFIG_ATE_TEST
	bk_flash_clk_switch(FLASH_SPEED_LOW, 0);
	#else
	bk_flash_clk_switch(FLASH_SPEED_HIGH, 0);
	#endif
#endif

	flash_enable_line_mode_switch(false);

	//flash_init_common();
	s_flash_is_init = true;

	return BK_OK;
}

static bk_err_t flash_erase_block(uint32_t erase_addr, int type)
{
	FLASH_LOGI("erase: %x.%d\r\n", erase_addr, type);
	if (erase_addr >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", erase_addr);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	if (flash_is_area_write_disable(erase_addr)) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	/* Download sets runtime to NONE; do not re-apply PROTECT_ALL after each erase. */
	if (s_flash_runtime_protect_type != FLASH_PROTECT_NONE) {
		flash_set_protect_before_op(erase_addr);
	}
	flash_hal_erase_block(&s_flash.hal, erase_addr, type);
	if (s_flash_runtime_protect_type != FLASH_PROTECT_NONE) {
		flash_set_protect_type(s_flash_runtime_protect_type);
	}
	flash_restore_line_mode();
	SYS_UNLOCK();

	return BK_OK;
}

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_flash_erase_sector(uint32_t address)
{
	return flash_erase_block(address & FLASH_ERASE_SECTOR_MASK, FLASH_OP_CMD_SE);
}

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_flash_erase_block_64k(uint32_t address)
{
	return flash_erase_block(address & FLASH_ERASE_BLOCK_64K_MASK, FLASH_OP_CMD_BE2);
}

__attribute__((section(".itcm_sec_code"))) bk_err_t bk_flash_erase_block_32k(uint32_t address)
{
	return flash_erase_block(address & FLASH_ERASE_BLOCK_32K_MASK, FLASH_OP_CMD_BE1);
}

bk_err_t bk_flash_erase_cmd(uint32_t address, int type)
{
	switch(type)
	{
		case FLASH_OP_CMD_SE:
			bk_flash_erase_sector(address);
		break;
		case FLASH_OP_CMD_BE2:
			bk_flash_erase_block_64k(address);
		break;
		case FLASH_OP_CMD_BE1:
			bk_flash_erase_block_32k(address);
		break;
	}
}


bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("read error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	flash_read_common(user_buf, address, size);

	return BK_OK;
}

bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("write error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	if (flash_is_area_write_disable(address)) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	/* Download sets runtime to NONE; do not re-apply PROTECT_ALL after each write. */
	if (s_flash_runtime_protect_type != FLASH_PROTECT_NONE) {
		flash_set_protect_before_op(address);
	}
	flash_write_common(user_buf, address, size);
	if (s_flash_runtime_protect_type != FLASH_PROTECT_NONE) {
		flash_set_protect_type(s_flash_runtime_protect_type);
	}
	flash_restore_line_mode();
	SYS_UNLOCK();

	return BK_OK;
}

#if 0//!CONFIG_SECUREBOOT
__attribute__((section(".iram")))
static void *flash_memcpy(void *d, const void *s, size_t n)
{
        /* attempt word-sized copying only if buffers have identical alignment */

        unsigned char *d_byte = (unsigned char *)d;
        const unsigned char *s_byte = (const unsigned char *)s;
        const uint32_t mask = sizeof(uint32_t) - 1;

        if ((((uint32_t)d ^ (uint32_t)s_byte) & mask) == 0) {

                /* do byte-sized copying until word-aligned or finished */

                while (((uint32_t)d_byte) & mask) {
                        if (n == 0) {
                                return d;
                        }
                        *(d_byte++) = *(s_byte++);
                        n--;
                };

                /* do word-sized copying as long as possible */

                uint32_t *d_word = (uint32_t *)d_byte;
                const uint32_t *s_word = (const uint32_t *)s_byte;

                while (n >= sizeof(uint32_t)) {
                        *(d_word++) = *(s_word++);
                        n -= sizeof(uint32_t);
                }

                d_byte = (unsigned char *)d_word;
                s_byte = (unsigned char *)s_word;
        }

        /* do byte-sized copying until finished */

        while (n > 0) {
                *(d_byte++) = *(s_byte++);
                n--;
        }

        return d;
}
#endif
extern void *memcpy(void *dest, const void *src, size_t n);
__attribute__((section(".iram")))
void bk_flash_read_cbus(uint32_t address, void *user_buf, uint32_t size)
{
	memcpy((void*)user_buf, (const void*)(0x02000000+address),size);
}

#if 0
__attribute__((section(".iram")))
void bk_flash_write_cbus(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();

	bk_flash_cpu_write_enable();

#if CONFIG_SECUREBOOT
	memcpy((void*)(0x02000000+address), (const void*)user_buf,size);
#else
	flash_memcpy((char*)(0x02000000+address), (const char*)user_buf,size);
#endif

	bk_flash_cpu_write_disable();
	flash_restore_line_mode();
	SYS_UNLOCK();
}
#endif

flash_line_mode_t bk_flash_get_line_mode(void)
{
	return s_flash.flash_cfg->line_mode;
}
bk_err_t bk_flash_driver_deinit(void)
{
	if (!s_flash_is_init) {
		return BK_OK;
	}
	//flash_deinit_common();
	s_flash_is_init = false;

	return BK_OK;
}
#if 0//code size optimize

bk_err_t bk_flash_register_wait_cb(flash_wait_callback_t wait_cb)
{
	uint32_t i = 0;

	for(i = 0; i < FLASH_MAX_WAIT_CB_CNT; i++)
	{
		if(s_flash_wait_cb[i] == NULL)
		{
			s_flash_wait_cb[i] = wait_cb;
			break;
		}
	}

	if(i == FLASH_MAX_WAIT_CB_CNT)
	{
		FLASH_LOGE("cb is full\r\n");
		return BK_ERR_FLASH_WAIT_CB_FULL;
	}

	return BK_OK;
}

bk_err_t bk_flash_unregister_wait_cb(flash_wait_callback_t wait_cb)
{
	uint32_t i = 0;

	for(i = 0; i < FLASH_MAX_WAIT_CB_CNT; i++)
	{
		if(s_flash_wait_cb[i] == wait_cb)
		{
			s_flash_wait_cb[i] = NULL;
			break;
		}
	}

	if(i == FLASH_MAX_WAIT_CB_CNT)
	{
		FLASH_LOGE("cb isn't registered\r\n");
		return BK_ERR_FLASH_WAIT_CB_NOT_REGISTER;
	}

	return BK_OK;
}

static void flash_init_common(void)
{
	return 0;
}

static void flash_deinit_common(void)
{
	return 0;
}

bk_err_t bk_flash_read_word(uint32_t address, uint32_t *user_buf, uint32_t size)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("read error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	flash_read_word_common(user_buf, address, size);

	return BK_OK;
}

uint32_t bk_flash_get_id(void)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	s_flash.flash_id = flash_hal_get_id(&s_flash.hal);
	flash_restore_line_mode();
	SYS_UNLOCK();
	return s_flash.flash_id;
}

bk_err_t bk_flash_set_clk_dpll(void)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	sys_drv_flash_set_dpll();
	flash_hal_set_clk_dpll(&s_flash.hal);
	flash_restore_line_mode();
	SYS_UNLOCK();
	
	return BK_OK;
}

bk_err_t bk_flash_set_clk_dco(void)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	sys_drv_flash_set_dco();
	bool ate_enabled = ate_is_enabled();
	flash_hal_set_clk_dco(&s_flash.hal, ate_enabled);
	flash_restore_line_mode();
	SYS_UNLOCK();

	return BK_OK;
}

bk_err_t bk_flash_write_enable(void)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	flash_hal_write_enable(&s_flash.hal);
	flash_restore_line_mode();
	SYS_UNLOCK();

	return BK_OK;
}

bk_err_t bk_flash_write_disable(void)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	flash_hal_write_disable(&s_flash.hal);
	flash_restore_line_mode();
	SYS_UNLOCK();
	return BK_OK;
}

uint16_t bk_flash_read_status_reg(void)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	uint16_t sr_data = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
	flash_restore_line_mode();
	SYS_UNLOCK();

	return sr_data;
}
#endif//code size optimize
uint16_t bk_flash_read_sr(unsigned char byte)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	uint16_t sr_data = flash_hal_read_status_reg(&s_flash.hal, byte);
	flash_restore_line_mode();
	SYS_UNLOCK();
	return sr_data;
}
#if 0//code size optimize
bk_err_t bk_flash_write_status_reg(uint16_t status_reg_data)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	flash_hal_write_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size, status_reg_data);
	flash_restore_line_mode();
	SYS_UNLOCK();
	return BK_OK;
}
#endif//code size optimize
bk_err_t bk_flash_write_sr(unsigned char bytes,  uint16_t status_reg_data)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	flash_write_status_reg(bytes, status_reg_data);
	flash_restore_line_mode();
	SYS_UNLOCK();
	return BK_OK;
}

bk_err_t bk_flash_set_protect_type_none(void)
{
	SYS_LOCK_DECLARATION();
	SYS_LOCK();
	flash_switch_to_line_mode_two();
	/* Keep steady-state as NONE so erase/write do not restore PROTECT_ALL. */
	s_flash_runtime_protect_type = FLASH_PROTECT_NONE;
	flash_set_protect_type(FLASH_PROTECT_NONE);
	flash_restore_line_mode();
	SYS_UNLOCK();

	return BK_OK;
}

#if 0//code size optimize
/* This API is not used in bk7256xx */
void flash_ps_pm_init(void)
{
	PM_STATUS *flash_ps_status_ptr = &flash_ps_status;

	bk_flash_set_clk_dco();
	bk_flash_get_id();

	flash_ps_status_ptr->bits.unconditional_ps_support = 1;
	flash_ps_status_ptr->bits.unconditional_ps_suspend_allow = 1;
	flash_ps_status_ptr->bits.unconditional_ps_resume_allow = 1;
	flash_ps_status_ptr->bits.unconditional_ps_sleeped = 0;
	flash_ps_status_ptr->bits.normal_ps_support = 1;
	flash_ps_status_ptr->bits.normal_ps_suspend_allow = 1;
	flash_ps_status_ptr->bits.normal_ps_resume_allow = 1;
	flash_ps_status_ptr->bits.normal_ps_sleeped = 0;
	flash_ps_status_ptr->bits.lowvol_ps_support = 1;
	flash_ps_status_ptr->bits.lowvol_ps_suspend_allow = 1;
	flash_ps_status_ptr->bits.lowvol_ps_resume_allow = 1;
	flash_ps_status_ptr->bits.lowvol_ps_sleeped = 0;
	flash_ps_status_ptr->bits.deep_ps_support = 1;
	flash_ps_status_ptr->bits.deep_ps_suspend_allow = 1;
	flash_ps_status_ptr->bits.deep_ps_resume_allow = 1;
	flash_ps_status_ptr->bits.deep_ps_sleeped = 0;

	dev_pm_register(PM_ID_FLASH, "flash", &flash_ps_ops);
}
#endif

bool bk_flash_is_driver_inited()
{
	return s_flash_is_init;
}

uint32_t bk_flash_get_current_total_size(void)
{
	return s_flash.flash_cfg->flash_size;
}

#define FLASH_OPERATE_SIZE_AND_OFFSET    (4096)
bk_err_t bk_spec_flash_write_bytes(bk_partition_t partition, const uint8_t *user_buf, uint32_t size,uint32_t offset)
{
	bk_logic_partition_t *bk_ptr = NULL;
	u8 *save_flashdata_buff  = NULL;

	bk_ptr = bk_flash_partition_get_info(partition);
	if((size + offset) > FLASH_OPERATE_SIZE_AND_OFFSET)
		return BK_FAIL;
	
	save_flashdata_buff= os_malloc(bk_ptr->partition_length);
	if(save_flashdata_buff == NULL)
	{
		os_printf("save_flashdata_buff malloc err\r\n");
		return BK_FAIL;
	}

	bk_flash_read_bytes((bk_ptr->partition_start_addr),(uint8_t *)save_flashdata_buff, bk_ptr->partition_length);

	bk_flash_erase_sector(bk_ptr->partition_start_addr);
	os_memcpy((save_flashdata_buff + offset), user_buf, size);
	bk_flash_write_bytes(bk_ptr->partition_start_addr ,(uint8_t *)save_flashdata_buff, bk_ptr->partition_length);
        
	os_free(save_flashdata_buff);
	save_flashdata_buff = NULL;
    
	return BK_OK;

}

void bk_flash_set_base_addr(uint32_t addr)
{
	flash_hal_set_base_addr(&s_flash.hal, addr);
}

int bk_flash_set_dbus_security_region(uint32_t id, uint32_t start, uint32_t end, bool secure)
{
	if (id >= FLASH_DBUS_REGION_MAX) {
		return BK_ERR_FLASH_DBUS_REGION;
	}

	if (end <= start) {
		return BK_ERR_FLASH_ADDR;
	}

	if (end >= s_flash.flash_cfg->flash_size) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	flash_hal_set_dbus_region(&s_flash.hal, id, start, end, secure);
	return BK_OK;
}

void flash_set_xip_offset(uint32_t primary_start, uint32_t secondary_start, uint32_t code_size)
{
	flash_hal_set_offset_begin(&s_flash.hal,primary_start);
	flash_hal_set_offset_end(&s_flash.hal,primary_start+code_size);
	flash_hal_set_addr_offset(&s_flash.hal,secondary_start-primary_start);
}

#include "cmsis_gcc.h"
extern void flush_all_dcache(void);
void flash_set_excute_enable(int enable)
{
    __disable_irq();
	flash_hal_set_offset_enable(&s_flash.hal, enable);
	flush_all_dcache();
	__enable_irq();
}


uint32_t flash_get_excute_enable()
{
	return flash_hal_read_offset_enable(&s_flash.hal);
}
