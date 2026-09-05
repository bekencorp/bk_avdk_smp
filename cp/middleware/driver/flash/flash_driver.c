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
#include "flash_driver.h"
#include "flash_hal.h"
#include "sys_driver.h"
#include "driver/flash_partition.h"
#include <modules/chip_support.h>
#ifdef CONFIG_HSPL
#include "hspl_res_lock.h"
#endif

typedef struct {
	flash_hal_t            hal;
	uint32_t               flash_id;
	uint32_t               flash_status_reg_val;
	uint32_t               flash_line_mode;
	const flash_config_t * flash_cfg;
	uint32_t               dev_version;
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

static const flash_config_t flash_config[] = {
	/* flash_id, flash_size,    status_reg_size, line_mode,            cmp_post, protect_post, protect_mask, protect_all, protect_none, unprotect_last_block. quad_en_post, quad_en_val, coutinuous_read_mode_bits_val   */
	{0x1C7016,   FLASH_SIZE_4M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x1F,         0x1F,        0x00,         0x01B,                9,            1,           0xA5,                         }, //en_25qh32b
	{0x1C7015,   FLASH_SIZE_2M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x1F,         0x1F,        0x00,         0x0d,                 9,            1,           0xA5,                         }, //en_25qh16b
	{0x0B4014,   FLASH_SIZE_1M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xtx_25f08b
	{0x0B4015,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xtx_25f16b
	{0x0B4016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xtx_25f32b
	{0x0B4017,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x05,        0x00,         0x109,                9,            1,           0xA0,                         }, //xtx_25f64b
	{0x0B6017,   FLASH_SIZE_8M,   2,             FLASH_LINE_MODE_FOUR,   0,	       2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //xt_25q64d
	{0x0B6018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,	       2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //xt_25q128d
	{0x0B4018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,	       2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //xt_25F128F-W
	{0x0E4016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xtx_FT25H32
	{0x1C4116,   FLASH_SIZE_4M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //en_25qe32a(not support 4 line)
	{0x5E5018,   FLASH_SIZE_16M,  1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //zb_25lq128c
	{0xC84015,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //gd_25q16c
	{0xC84017,   FLASH_SIZE_8M,   1,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //gd_25q16c
	{0xC84016,   FLASH_SIZE_4M,   3,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //gd_25q32c
	{0xC86018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //gd_25lq128e
	{0xC86515,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //gd_25w16e
	{0xC86516,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //gd_25wq32e
	{0xEF4016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //w_25q32(bfj)
	{0x204118,	 FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   0,	       2,            0x0F,         0x0F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //xm_25qu128c
	{0x204016,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //xmc_25qh32b
	{0xC22315,   FLASH_SIZE_2M,   1,             FLASH_LINE_MODE_FOUR,   0,        2,            0x0F,         0x0F,        0x00,         0x00E,                6,            1,           0xA5,                         }, //mx_25v16b
	{0xEB6015,   FLASH_SIZE_2M,   2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x101,                9,            1,           0xA0,                         }, //zg_th25q16b
	{0xC86517,	 FLASH_SIZE_8M,   2, 			 FLASH_LINE_MODE_FOUR,   14,	   2,			 0x1F, 		   0x1F,		0x00,		  0x00E,		        9,			  1, 		   0xA0 		                 }, //gd_25Q32E
	{0xCD6017,   FLASH_SIZE_8M,   3,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //th_25q64ha
	{0x852018,   FLASH_SIZE_16M,  2,             FLASH_LINE_MODE_FOUR,   14,       2,            0x1F,         0x1F,        0x00,         0x00E,                9,            1,           0xA0,                         }, //py_25q129ha
	{0x000000,   FLASH_SIZE_4M,   2,             FLASH_LINE_MODE_TWO,    0,        2,            0x1F,         0x00,        0x00,         0x000,                0,            0,           0x00,                         }, //default
};

static flash_driver_t s_flash = {0};
static bool s_flash_is_init = false;
static flash_protect_type_t s_flash_runtime_protect_type = FLASH_PROTECT_ALL;


extern bk_err_t    mb_flash_ipc_init(void);
extern bk_err_t    mb_flash_op_prepare(void);
extern bk_err_t    mb_flash_op_finish(void);

extern bk_err_t    bk_flash_partition_write_perm_check_by_addr(uint32_t addr, uint32_t size, uint32_t magic_code);

extern int xTaskResumeAll( void );
extern void vTaskSuspendAll( void );

/* Recursive lock counter for hardware spinlock to prevent deadlock */
/* Shared between flash_lock/unlock and flash_enter_critical/exit_critical */
static volatile uint32_t s_flash_hspl_lock_count = 0;

/*
 * Recursion: supported on the same core (count only acquires HSPL when 0, releases when back to 0).
 * Cross-core: flash_lock/unlock must run to completion on the same core. int_level is per-CPU;
 * if the task migrates to another core before flash_unlock, restoring int_level on the new core
 * is wrong, and HSPL unlock must be done by the core that took it (bk_hspl_res_unlock asserts
 * per-core rec count). Do not migrate while holding flash lock.
 */
 static inline uint32_t flash_enter_critical(void)
 {
 
	 uint32_t flags = bk_aspl_flash_enter_critical();
	 
	 return flags;
 }
 
 static inline void flash_exit_critical(uint32_t flags)
 {
	 /* Release HSPL lock */
	 bk_aspl_flash_exit_critical(flags);
 }

#if 1
#ifdef CONFIG_FREERTOS_SMP
static beken_mutex_t s_flash_mutex = NULL;
#endif

static void flash_lock_init(void)
{
#ifdef CONFIG_FREERTOS_SMP
	int ret = rtos_init_mutex(&s_flash_mutex);
	BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
#endif
	s_flash_hspl_lock_count = 0;
}

#if 0
static void flash_lock_deinit(void)
{
	int ret = rtos_deinit_mutex(&s_flash_mutex);
	BK_ASSERT(kNoErr == ret); /* ASSERT VERIFIED */
}
#endif

static inline uint32_t flash_lock(void)
{
	uint32_t int_level = flash_enter_critical();

	mb_flash_op_prepare();

	return int_level;
}

static inline void flash_unlock(uint32_t int_level)
{
	mb_flash_op_finish();

	flash_exit_critical(int_level);
}

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

__attribute__((section(".iram"))) static uint32_t flash_read_status_reg(void)
{
	uint32_t status_reg;
    uint32_t int_level;

    int_level = flash_enter_critical();
    status_reg = flash_hal_read_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size);
    flash_exit_critical(int_level);

	return status_reg;
}

__attribute__((section(".iram"))) static void flash_write_status_reg(uint32_t status_reg_val)
{
	uint32_t int_level = flash_enter_critical();
	s_flash.flash_status_reg_val = status_reg_val;

	flash_hal_write_status_reg(&s_flash.hal, s_flash.flash_cfg->status_reg_size, status_reg_val);
	flash_exit_critical(int_level);
}

/* Non-volatile status-register write: the value persists across power cycles. */
__attribute__((section(".iram"))) static void flash_write_status_reg_nvol(uint32_t status_reg_val)
{
	uint32_t int_level = flash_enter_critical();
	s_flash.flash_status_reg_val = status_reg_val;

	flash_hal_write_status_reg_nvol(&s_flash.hal, s_flash.flash_cfg->status_reg_size, status_reg_val);
	flash_exit_critical(int_level);
}

static uint32_t flash_get_id(void)
{
	uint32_t flash_id;
	uint32_t int_level = flash_enter_critical();
	flash_id = flash_hal_get_id(&s_flash.hal);
	flash_exit_critical(int_level);
	return flash_id;
}

static uint32_t flash_get_protect_cfg(flash_protect_type_t type)
{
	switch (type) {
	case FLASH_PROTECT_NONE:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_none);
	case FLASH_PROTECT_ALL:
		return FLASH_GET_PROTECT_CFG(s_flash.flash_cfg->protect_all);
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

static __attribute__((unused)) flash_protect_type_t flash_get_protect_type(uint32_t sr_value)
{
	uint32_t type = 0;
	uint16_t protect_value = 0;
	uint16_t cmp;

	protect_value = sr_value >> s_flash.flash_cfg->protect_post;
	protect_value = protect_value & s_flash.flash_cfg->protect_mask;

	cmp = (sr_value >> s_flash.flash_cfg->cmp_post) & FLASH_CMP_MASK;
	protect_value |= cmp << FLASH_STATUS_REG_PROTECT_OFFSET;

	if (protect_value == s_flash.flash_cfg->protect_all)
		type = FLASH_PROTECT_ALL;
	else if (protect_value == s_flash.flash_cfg->protect_none)
		type = FLASH_PROTECT_NONE;
	else if (protect_value == s_flash.flash_cfg->unprotect_last_block)
		type = FLASH_UNPROTECT_LAST_BLOCK;
	else
		type = FLASH_PROTECT_ALL;  // FLASH_UNPROTECT_LAST_BLOCK ???

	return type;
}

static void flash_set_protect_type_ex(flash_protect_type_t type, bool nonvolatile)
{
	uint32_t protect_cfg;
	uint32_t cmp_cfg;
	uint32_t status_reg = s_flash.flash_status_reg_val;

	protect_cfg = flash_get_protect_cfg(type);
	cmp_cfg = flash_get_cmp_cfg(type);

	#if CONFIG_FLASH_SUPPORT_MULTI_PE  /* multiple process element. (multi-cores or SPE/NSPE) */
	status_reg = flash_read_status_reg();
	#endif

	if (flash_is_need_update_status_reg(protect_cfg, cmp_cfg, status_reg)) {
		flash_set_protect_cfg(&status_reg, protect_cfg);
		flash_set_cmp_cfg(&status_reg, cmp_cfg);

		//FLASH_LOGV("write status reg:%x, status_reg_size:%d\r\n", status_reg, s_flash.flash_cfg->status_reg_size);
		if (nonvolatile) {
			flash_write_status_reg_nvol(status_reg);
		} else {
			flash_write_status_reg(status_reg);
		}
	}
}

static void flash_set_protect_type(flash_protect_type_t type)
{
	flash_set_protect_type_ex(type, false);
}

/* Persist the protection setting across power cycles (non-volatile SR write). */
static void flash_set_protect_type_nvol(flash_protect_type_t type)
{
	flash_set_protect_type_ex(type, true);
}

__attribute__((section(".iram"))) static void flash_set_qe(void)
{
	uint32_t status_reg = s_flash.flash_status_reg_val;

	#if CONFIG_FLASH_SUPPORT_MULTI_PE
	status_reg = flash_read_status_reg();
	#endif
	if (((status_reg >> s_flash.flash_cfg->quad_en_post) & 0x01) == s_flash.flash_cfg->quad_en_val) {
		return;
	}

	if (1 == s_flash.flash_cfg->quad_en_val)
		status_reg |= (1 << s_flash.flash_cfg->quad_en_post);
	else
		status_reg &= ~(1 << s_flash.flash_cfg->quad_en_post);

	/* QE must persist across reboot -> non-volatile write. */
	flash_write_status_reg_nvol(status_reg);
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
        uint32_t int_level;

		/*disable interrupt for special device, contact with zili.guo/ling.zhou*/
		int_level = flash_enter_critical();
        flash_hal_set_op_cmd_read(&s_flash.hal, addr);
        addr += FLASH_BYTES_CNT;
        for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
            buf[i] = flash_hal_read_data(&s_flash.hal);
        }
        flash_exit_critical(int_level);

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

static void flash_read_word_common(uint32_t *buffer, uint32_t address, uint32_t len)
{
    uint32_t addr = address & (~FLASH_ADDRESS_MASK);
    uint32_t buf[FLASH_BUFFER_LEN] = {0};
    uint32_t *pb = (uint32_t *)&buf[0];

    if (len == 0) {
        return;
    }

    while (len) {
        uint32_t int_level;

		/*disable interrupt for special device, contact with zili.guo/ling.zhou*/
		int_level = flash_enter_critical();
        flash_hal_set_op_cmd_read(&s_flash.hal, addr);
        addr += FLASH_BYTES_CNT;
        for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
            buf[i] = flash_hal_read_data(&s_flash.hal);
        }

        flash_exit_critical(int_level);

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

		uint32_t int_level = flash_enter_critical();
		flash_hal_wait_op_done(&s_flash.hal);

		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			flash_hal_write_data(&s_flash.hal, buf[i]);
		}
		flash_hal_set_op_cmd_write(&s_flash.hal, addr);
		flash_exit_critical(int_level);

		addr += FLASH_BYTES_CNT;
	}
	return BK_OK;
}

static bk_err_t flash_erase_block(uint32_t address, int type)
{
	uint32_t int_level = flash_enter_critical();

	flash_hal_erase_block(&s_flash.hal, address, type);

	flash_exit_critical(int_level);

	return BK_OK;
}
__attribute__((section(".iram"))) static flash_line_mode_t flash_set_line_mode(flash_line_mode_t line_mode)
{
	uint32_t int_level = flash_enter_critical();

	flash_line_mode_t  new_line_mode;
	flash_line_mode_t  old_line_mode = s_flash.flash_line_mode;

#if CONFIG_FLASH_QUAD_ENABLE
	if ( (FLASH_LINE_MODE_FOUR == line_mode)
		&& (FLASH_LINE_MODE_FOUR == s_flash.flash_cfg->line_mode) )
	{
		new_line_mode = FLASH_LINE_MODE_FOUR;
	}
	else
#endif
	{
		new_line_mode = FLASH_LINE_MODE_TWO;
	}

	if(new_line_mode == old_line_mode)
	{
		flash_exit_critical(int_level);
		return old_line_mode;
	}

	flash_hal_clear_qwfr(&s_flash.hal);   // cmd CRMR (coutinuous_read_mode reset), quit QPI mode.

	if (FLASH_LINE_MODE_FOUR == new_line_mode)
	{
		flash_hal_set_quad_m_value(&s_flash.hal, s_flash.flash_cfg->coutinuous_read_mode_bits_val);
		flash_set_qe();
		flash_hal_set_mode(&s_flash.hal, FLASH_MODE_QUAD);  // enter QPI mode.
	}
	else
	{
		flash_hal_set_mode(&s_flash.hal, FLASH_MODE_DUAL);
	}

	s_flash.flash_line_mode = new_line_mode;

	flash_exit_critical(int_level);

	return old_line_mode;

}

flash_line_mode_t bk_flash_get_line_mode(void)
{
	return s_flash.flash_cfg->line_mode;
}

uint8_t bk_flash_get_coutinuous_read_mode(void)
{
	return s_flash.flash_cfg->coutinuous_read_mode_bits_val;
}

bk_err_t flash_clear_rd_residual_data(flash_hal_t *hal)
{
	uint32_t tmp_val;
	uint32_t residual_cnt, read_cnt;

	read_cnt = flash_hal_read_data_sw_flash_sel(hal);
	if(read_cnt){
		residual_cnt = FLASH_BUFFER_LEN - read_cnt;
		while(residual_cnt){
			tmp_val = flash_hal_read_data(hal);
			residual_cnt --;
		}
	}
	(void)tmp_val;

	return BK_OK;
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

    os_memset(&s_flash, 0, sizeof(s_flash));

	hal_ptr = &s_flash.hal;
	flash_hal_init(hal_ptr);

	s_flash.dev_version = flash_hal_get_dev_version(&s_flash.hal);
	FLASH_LOGI("dev_version=0x%x\r\n", s_flash.dev_version);

    flash_hal_disable_cpu_data_wr(hal_ptr);

	uint32_t int_level = flash_enter_critical();

    flash_set_line_mode(FLASH_LINE_MODE_TWO);

    s_flash.flash_id = flash_get_id();
    FLASH_LOGI("id=0x%x\r\n", s_flash.flash_id);

    flash_get_current_config();

    flash_hal_set_quad_m_value(hal_ptr, s_flash.flash_cfg->coutinuous_read_mode_bits_val);

    s_flash.flash_status_reg_val = flash_read_status_reg();

    /* Enable flash write-protect at boot and persist it across reboot. */
    s_flash_runtime_protect_type = FLASH_PROTECT_ALL;
    flash_set_protect_type_nvol(FLASH_PROTECT_ALL);

    flash_set_line_mode(s_flash.flash_cfg->line_mode);

    flash_hal_set_default_clk(hal_ptr);

	flash_exit_critical(int_level);

	if(s_flash.dev_version != 0x20000)
	{
		#if CONFIG_ATE_TEST
			#if (CONFIG_EXT_FLASH_CLK_FREQ == 40000000)
			if((0 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
				sys_drv_flash_set_clk_div(0); // XTAL= 40M
				sys_drv_flash_cksel(0);
			}
			#elif (CONFIG_EXT_FLASH_CLK_FREQ == 80000000)
			if((2 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
				sys_drv_flash_set_clk_div(0); // 80M div 1 = 80M
				sys_drv_flash_cksel(2);
			}
			#elif (CONFIG_EXT_FLASH_CLK_FREQ == 120000000)
			if((3 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
				sys_drv_flash_set_clk_div(0); // 120M div 1 = 120M
				sys_drv_flash_cksel(3);
			}
			#else
			if((0 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
				sys_drv_flash_set_clk_div(0); // XTAL= 40M
				sys_drv_flash_cksel(0);
			}
			#endif
    	#else
		if((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_GD || (s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_TH) {
			if((2 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
				sys_drv_flash_set_clk_div(0); // 80M div 1 = 80M
				sys_drv_flash_cksel(2);
			}
		} else {
			if((0 != sys_drv_flash_get_clk_sel()) || (0 != sys_drv_flash_get_clk_div())) {
				sys_drv_flash_set_clk_div(0); // XTAL= 40M
				sys_drv_flash_cksel(0);
			}
		}
    	#endif
		if((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_GD || (s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_TH) {
			if((1 != sys_drv_flash_get_clk_sel()) || (1 != sys_drv_flash_get_clk_div())) {
				#if (CONFIG_FLASH_CLK_120M)
				sys_drv_flash_set_clk_div(0); // dpll div 4 = 120M
				#else
				sys_drv_flash_set_clk_div(1); // dpll div 6 = 80M
				#endif
				sys_drv_flash_cksel(1);
			}
		}
		else
		{
			if((1 != sys_drv_flash_get_clk_sel()) || (3 != sys_drv_flash_get_clk_div())) {
				sys_drv_flash_set_clk_div(3); // dpll div 10 = 48M
				sys_drv_flash_cksel(1);
			}
		}

	}
	else {//for bk7259
		if((s_flash.flash_id >> FLASH_ManuFacID_POSI) == FLASH_ManuFacID_GD) {
			if((3 != sys_drv_flash_get_clk_sel()) || (3 != sys_drv_flash_get_clk_div())) {
				sys_drv_flash_set_clk_div(3); // 320M/4 = 80M
				sys_drv_flash_cksel(3);
			}
		} else {
			if((2 != sys_drv_flash_get_clk_sel()) || (4 != sys_drv_flash_get_clk_div())) {
				sys_drv_flash_set_clk_div(4); // 240M/5 = 48M
				sys_drv_flash_cksel(2);
			}
		}
	}

	sys_drv_set_sys2flsh_2wire(1);
	/* If start flash read, and the 8-word data was not fully read out
	 * from the flash fifo. and then a next flash read is initiated again
	 * the data will no be read correctly for hw fifo pointer is wrong.
	 */
	flash_clear_rd_residual_data(hal_ptr);

	flash_lock_init();

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

static bk_err_t flash_erase_no_lock(uint32_t address, int cmd)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	uint32_t  erase_size = 0;

	if(cmd == FLASH_OP_CMD_SE)
		erase_size = FLASH_SECTOR_SIZE;
	else if(cmd == FLASH_OP_CMD_BE1)
		erase_size = FLASH_BLOCK32_SIZE;
	else if(cmd == FLASH_OP_CMD_BE2)
		erase_size = FLASH_BLOCK_SIZE;
	else
		return BK_FAIL;

	uint32_t erase_addr = address & (~(erase_size - 1));

	bk_err_t    ret_val = BK_FAIL;

	flash_line_mode_t old_line_mode = flash_set_line_mode(FLASH_LINE_MODE_TWO);

	if(bk_flash_partition_write_perm_check_by_addr(erase_addr, erase_size, FLASH_API_MAGIC_CODE) == BK_OK)
	{
		flash_set_protect_type(FLASH_PROTECT_NONE);

		if(bk_flash_partition_write_perm_check_by_addr(erase_addr, erase_size, FLASH_API_MAGIC_CODE) == BK_OK)
			ret_val = flash_erase_block(address, cmd);
	}

	flash_set_protect_type(s_flash_runtime_protect_type);
	flash_set_line_mode(old_line_mode);

	return ret_val;
}

bk_err_t bk_flash_erase_sector(uint32_t address)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	uint32_t int_level = flash_lock();

	bk_err_t ret_val = flash_erase_no_lock(address, FLASH_OP_CMD_SE);

	flash_unlock(int_level);

	return ret_val;
}

bk_err_t bk_flash_erase_32k(uint32_t address)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	uint32_t int_level = flash_lock();

	bk_err_t  ret_val = flash_erase_no_lock(address, FLASH_OP_CMD_BE1);

	flash_unlock(int_level);

	return ret_val;
}

bk_err_t bk_flash_erase_block(uint32_t address)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("erase error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	uint32_t int_level = flash_lock();

	bk_err_t  ret_val = flash_erase_no_lock(address, FLASH_OP_CMD_BE2);

	flash_unlock(int_level);

	return ret_val;
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

bk_err_t bk_flash_read_word(uint32_t address, uint32_t *user_buf, uint32_t size)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("read error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}
	flash_read_word_common(user_buf, address, size);

	return BK_OK;
}

static bk_err_t flash_write_no_lock(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("write error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	bk_err_t    ret_val = BK_FAIL;

	flash_line_mode_t old_line_mode = flash_set_line_mode(FLASH_LINE_MODE_TWO);

	if(bk_flash_partition_write_perm_check_by_addr(address, size, FLASH_API_MAGIC_CODE) == BK_OK)
	{
		flash_set_protect_type(FLASH_PROTECT_NONE);

		if(bk_flash_partition_write_perm_check_by_addr(address, size, FLASH_API_MAGIC_CODE) == BK_OK)
			ret_val = flash_write_common(user_buf, address, size);
	}

	flash_set_protect_type(s_flash_runtime_protect_type);
	flash_set_line_mode(old_line_mode);

	return ret_val;
}

bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	if (address >= s_flash.flash_cfg->flash_size) {
		FLASH_LOGW("write error:invalid address 0x%x\r\n", address);
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	uint32_t int_level = flash_lock();

	bk_err_t    ret_val = flash_write_no_lock(address, user_buf, size);

	flash_unlock(int_level);

	return ret_val;
}

uint32_t bk_flash_get_id(void)
{
	return s_flash.flash_id;
}

bk_err_t bk_flash_set_clk_dpll(void)
{
	sys_drv_flash_set_dpll();
	flash_hal_set_clk_dpll(&s_flash.hal);

	return BK_OK;
}

bk_err_t bk_flash_set_clk_dco(void)
{
	sys_drv_flash_set_dco();
	bool ate_enabled = ate_is_enabled();
	flash_hal_set_clk_dco(&s_flash.hal, ate_enabled);

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
	#if CONFIG_FLASH_SUPPORT_MULTI_PE
	flash_line_mode_t old_line_mode = flash_set_line_mode(FLASH_LINE_MODE_TWO);
	uint16_t sr_data = flash_read_status_reg();
	flash_set_line_mode(old_line_mode);
	return sr_data;
	#else
	return s_flash.flash_status_reg_val;
	#endif
}

bk_err_t bk_flash_write_status_reg(uint16_t status_reg_data)
{
#if 0
	flash_line_mode_t old_line_mode = flash_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_write_status_reg(status_reg_data);
	flash_set_line_mode(old_line_mode);
#endif
	return BK_OK;
}

uint32_t bk_flash_get_crc_err_num(void)
{
	return flash_hal_get_crc_err_num(&s_flash.hal);
}
// #endif

void test_flash_set_protect_type_none(void)
{
	uint32_t int_level = flash_lock();
	flash_line_mode_t old_line_mode = flash_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_set_protect_type(FLASH_PROTECT_NONE);
	flash_set_line_mode(old_line_mode);
	flash_unlock(int_level);
}

void test_flash_set_protect_type_all(void)
{
	uint32_t int_level = flash_lock();
	flash_line_mode_t old_line_mode = flash_set_line_mode(FLASH_LINE_MODE_TWO);
	flash_set_protect_type(FLASH_PROTECT_ALL);
	flash_set_line_mode(old_line_mode);
	flash_unlock(int_level);
}

bool bk_flash_is_driver_inited()
{
	return s_flash_is_init;
}

uint32_t bk_flash_get_current_total_size(void)
{
	return s_flash.flash_cfg->flash_size;
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
static uint32_t s_pm_flash_saving_status_reg = 0;
bk_err_t bk_flash_power_saving_enter(void)
{
	// save flash ctrl setting to flash_ctrl_context;
	flash_set_line_mode(FLASH_LINE_MODE_TWO);

	/*
	 * Deep-LV does not power down the external flash, so its status/QE bits
	 * remain valid. Read them in command-safe dual-line mode before sleep and
	 * reuse the snapshot on wake to avoid RDSR on the wake critical path.
	 */
	s_pm_flash_saving_status_reg = flash_read_status_reg();

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
	s_flash.flash_status_reg_val = s_pm_flash_saving_status_reg;
	/* flash_set_line_mode(FLASH_LINE_MODE_TWO) updated this state on entry. */
	flash_set_line_mode(s_flash.flash_cfg->line_mode);

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

/* flash dump APIs are called in context of interrupt disabled. */
bk_err_t bk_flash_dump_erase_sector(uint32_t address)
{
	return flash_erase_no_lock(address, FLASH_OP_CMD_SE);
}

bk_err_t bk_flash_dump_write(uint32_t address, const uint8_t *user_buf, uint32_t size)
{
	return flash_write_no_lock(address, user_buf, size);
}

#if !CONFIG_SPE
/*
 * Keep the DBUS window helper after the legacy flash init/line-mode IRAM code.
 * The boot path is sensitive to IRAM layout changes around flash_set_line_mode().
 */
__attribute__((section(".iram")))
static bk_err_t flash_wait_op_done_with_busy_cb(bk_flash_busy_cb_t busy_cb,
						void *busy_arg)
{
	bool cb_done = false;
	bk_err_t ret = BK_OK;

	while (flash_hal_is_busy(&s_flash.hal)) {
		if (busy_cb && !cb_done) {
			ret = busy_cb(busy_arg);
			cb_done = true;
		}
	}

	if (busy_cb && !cb_done) {
		return BK_ERR_TIMEOUT;
	}

	return ret;
}

__attribute__((section(".iram")))
static bk_err_t flash_set_op_cmd_read_with_busy_cb(uint32_t read_addr,
						   bk_flash_busy_cb_t busy_cb,
						   void *busy_arg)
{
	flash_hw_t *hw = s_flash.hal.hw;

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
		uint32_t int_level = flash_enter_critical();

		ret = flash_set_op_cmd_read_with_busy_cb(addr, busy_cb,
			busy_arg);
		addr += FLASH_BYTES_CNT;
		busy_cb = NULL;
		busy_arg = NULL;
		if (ret != BK_OK) {
			flash_exit_critical(int_level);
			return ret;
		}

		for (uint32_t i = 0; i < FLASH_BUFFER_LEN; i++) {
			buf[i] = flash_hal_read_data(&s_flash.hal);
		}
		flash_exit_critical(int_level);

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
	FLASH_RETURN_ON_DRIVER_NOT_INIT();

	if (address >= s_flash.flash_cfg->flash_size) {
		return BK_ERR_FLASH_ADDR_OUT_OF_RANGE;
	}

	return flash_read_common_with_busy_cb(user_buf, address, size,
		busy_cb, busy_arg);
}
#endif

