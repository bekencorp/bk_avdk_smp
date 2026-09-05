// Copyright 2020-2022 Beken
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

#include <stdio.h>
#include <string.h>
#include "../download_internal.h"
#include "crc32.h"

#ifndef REG_XVR_BASE_ADDR
#define REG_XVR_BASE_ADDR            (0x4a800000)
#endif
#define SYS_VERSION_ID_ADDRESS       (0x44010004)

typedef u32 (*cmd_handler_t)(u8 *cmd_param, u16 param_len);

typedef struct {
	u16 cmd_id;
	cmd_handler_t cmd_handler;
} cmd_hdlr_tbl_t;

u8 uart_link_check_flag = 0;
u32 download_record_dl_flag = 0;

/* Shared flash core (linked into BL2). Declared here because the download
 * CMake target lacks the SDK soc/hal include paths. */
extern void flash_core_unprotect(void);

#if 0 //unused
static u32 cmd_link_check_handler(u8 *cmd_param, u16 param_len)
{
	u8 temp = 0;

	uart_link_check_flag = 1;
	// not use the COMMON_CMD_LINK_CHECK as the parameter, because a bug in old version of ROM code.
	tx_rsp_for_common_cmd(COMMON_RSP_LINK_CHECK, &temp, 1);
	return 0;
}
#endif

volatile u8 record_times = 0;

static u32 cmd_bl2_link_check_handler(u8 *cmd_param, u16 param_len)
{
	u8 temp = 0;

	uart_link_check_flag = 1;

	++record_times;

	if (record_times == 1) {
		printf("enter cmd_bl2_link_check_handler \r\n");
		/* Handshake success: unprotect the flash once (init left it
		 * FLASH_PROTECT_ALL). PER_OP still re-protects after each erase/PP, so
		 * this is an explicit gesture; actual writes are carried by each op's
		 * own self-unprotect. Reboot after flashing re-applies the protection. */
		flash_core_unprotect();
	}
	tx_rsp_for_common_cmd(COMMON_RSP_BL2_CMD_LINK_CHECK, &temp, 1);
	return 0;
}

#define ANA_XVR_NUM                  16
static u32 XVR_ANALOG_REG_BAK[ANA_XVR_NUM];

static u32 cmd_reg_read_handler(u8 *cmd_param, u16 param_len)
{
	u32 reg_addr = 0, reg_data = 0;

	if (param_len < sizeof(u32))
		return 1;

	memcpy(&reg_addr, cmd_param, sizeof(u32));

	s32 reg_index = (reg_addr - REG_XVR_BASE_ADDR) / 4;

	if ((reg_index >= 0) && (reg_index < ANA_XVR_NUM)) {
		reg_data = XVR_ANALOG_REG_BAK[reg_index];
	} else {
		reg_data = *((volatile u32 *)reg_addr);
	}

	reg_data = *((volatile u32 *)SYS_VERSION_ID_ADDRESS);

	memcpy(&cmd_param[4], &reg_data, 4);
	tx_rsp_for_common_cmd(COMMON_CMD_REG_READ, cmd_param, param_len + 4);

	return 0;
}

static u32 cmd_reboot_handler(u8 *cmd_param, u16 param_len)
{
#if 1
	u8 temp = 0xA5;

	if (param_len < 1)
		return 1;

	if (cmd_param[0] != 0xA5) {
		tx_rsp_for_common_cmd(COMMON_CMD_REBOOT, &temp, 1);
	} else {
		download_uart_disable();
		wdt_time_set(0x5);
		while (1) {
		}
	}
#endif
	return 0;
}

static u32 cmd_reset_handler(u8 *cmd_param, u16 param_len)
{
	if (param_len < 4)
		return 1;

	if ((cmd_param[0] != 0x53) ||
		(cmd_param[1] != 0x45) ||
		(cmd_param[2] != 0x41) ||
		(cmd_param[3] != 0x4E)) {
		tx_rsp_for_common_cmd(COMMON_CMD_RESET, cmd_param, param_len);
	} else {
		download_uart_disable();
		wdt_time_set(0x5);
		while (1) {
		}
	}

	return 0;
}

static u32 cmd_stay_rom_handler(u8 *cmd_param, u16 param_len)
{
	if (param_len < 1)
		return 1;

	if (cmd_param[0] != 0x55)
		return 2;

	uart_link_check_flag = 1;
	tx_rsp_for_common_cmd(COMMON_CMD_STAY_ROM, cmd_param, param_len);

	return 0;
}

static u32 cmd_set_br_handler(u8 *cmd_param, u16 param_len)
{
	u32 rate;
	u8 delay_ms;

	if (param_len < 5) {
		return 1;
	}

	memcpy(&rate, cmd_param, 4);
	delay_ms = cmd_param[4];

	download_uart_set_baudrate(rate);
	timer_delay_ms(delay_ms);

	tx_rsp_for_common_cmd(COMMON_CMD_SET_BAUDRATE, cmd_param, param_len);

	return 0;
}

static u32 cmd_check_crc_handler(u8 *cmd_param, u16 param_len)
{
	u32 start_addr = 0, end_addr = 0, data_len;
	u32 data_crc32 = 0xffffffff;

	if (param_len < (sizeof(u32) * 2))
		return 1;

	memcpy(&start_addr, cmd_param, 4);
	memcpy(&end_addr, cmd_param + 4, 4);

	make_crc32_table();

	while (start_addr <= end_addr) {
		wdt_time_set(DOWNLOAD_WDT_VALUE);

		data_len = end_addr - start_addr + 1;
		if (data_len > 256) {
			data_len = 256;
		}

		flash_read_data(cmd_param, start_addr, data_len);
		data_crc32 = crc32(data_crc32, cmd_param, data_len);
		start_addr += data_len;
	}

	tx_rsp_for_common_cmd(COMMON_CMD_CHECK_CRC32, (u8 *)&data_crc32, sizeof(data_crc32));

	return 0;
}

static u32 cmd_unkown_handler(u16 cmd_id)
{
	u8 temp = UNKNOW_CMD;

	tx_rsp_for_common_cmd(cmd_id, &temp, 1);

	return 0;
}

static u32 cmd_set_boot_flag_handler(u8 *cmd_param, u16 param_len)
{
	if (download_record_dl_flag == 1) {
		printf(" dl boot need refresh boot_flag!!!!! \n");
#if CONFIG_BL2_UPDATE_WITH_PC
		bl_set_boot_flag_value();
#endif
		download_record_dl_flag = 0;
	}

	tx_rsp_for_common_cmd(COMMON_CMD_SET_BOOT_FLAG, NULL, 0);

	return 0;
}

static u32 cmd_reps_current_exec_boot_handler(u8 *cmd_param, u16 param_len)
{
	u8 ret_boot_part_val = 0;

	ret_boot_part_val = bl_get_current_boot_execute_partition();
	tx_rsp_for_common_cmd(COMMON_CMD_RPS_CURRENT_BOOT_PART, &ret_boot_part_val, 1);

	return 0;
}

static const cmd_hdlr_tbl_t common_cmd_hdlr_tbl[] = {
	//{ COMMON_CMD_LINK_CHECK,    cmd_link_check_handler    },
	{ COMMON_BL2_CMD_LINK_CHECK, cmd_bl2_link_check_handler },
	{ COMMON_CMD_REG_READ,       cmd_reg_read_handler       },
	{ COMMON_CMD_REBOOT,         cmd_reboot_handler         },
	{ COMMON_CMD_SET_BAUDRATE,   cmd_set_br_handler         },
	{ COMMON_CMD_CHECK_CRC32,    cmd_check_crc_handler      },
	{ COMMON_CMD_RESET,          cmd_reset_handler          },
	{ COMMON_CMD_STAY_ROM,       cmd_stay_rom_handler       },
	{ COMMON_CMD_SET_BOOT_FLAG,  cmd_set_boot_flag_handler  },
	{ COMMON_CMD_RPS_CURRENT_BOOT_PART, cmd_reps_current_exec_boot_handler },
};

u32 common_cmd_process(rx_frm_ctrl_t *frm_ctrl)
{
	u16 i;

	for (i = 0; i < TBL_SIZE(common_cmd_hdlr_tbl); i++) {
		if (frm_ctrl->cmd_id == common_cmd_hdlr_tbl[i].cmd_id) {
			common_cmd_hdlr_tbl[i].cmd_handler((u8 *)&frm_ctrl->cmd_param[0],
				frm_ctrl->cmd_len - 1);
			return 0;
		}
	}

	cmd_unkown_handler(frm_ctrl->cmd_id);

	return 1;
}
