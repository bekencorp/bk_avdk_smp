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
#include <stdbool.h>
#include <string.h>
#include "../download_internal.h"

#define FLASH_PAGE_SIZE              (0x100)

typedef u32 (*cmd_handler_t)(u8 *cmd_param, u16 param_len);

typedef struct {
	u16 cmd_id;
	cmd_handler_t cmd_handler;
} cmd_hdlr_tbl_t;

static u32 flash_cmd_write_handler(u8 *cmd_param, u16 param_len)
{
	u32 addr;
	u8 ret_val = 0;

	if (param_len < 4)
		return 1;

	memcpy(&addr, cmd_param, 4);

	printf("param_len :0x%x, param_len - 4 :0x%x \r\n ", param_len, param_len - 4);
	if (download_record_dl_flag == 0) {
		if ((addr >= CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET) &&
			(addr < CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET)) {
			ret_val = OPERATE_PROTECTED_AREA;
		}
	}
	if (bl_forbid_operate_boot_partition(addr, param_len - 4) != true) {
		ret_val = OPERATE_PROTECTED_AREA;
	} else {
		flash_write_data(&cmd_param[4], addr, param_len - 4);
	}

	cmd_param[4] = param_len - 4;
	tx_rsp_for_flash_cmd(FLASH_CMD_WRITE, ret_val, cmd_param, 5);

	return 0;
}

u32 flash_cmd_sector_write(rx_frm_ctrl_t *frm_ctrl)
{
	u32 addr;
	u8 ret_val = 0;

	if (frm_ctrl->cmd_len != 0x1005)
		frm_ctrl->status = PACK_LEN_ERROR;

	if (frm_ctrl->status != 0) {
		frm_ctrl->read_idx = frm_ctrl->cmd_len - 1;
		return 1;
	}

	if (frm_ctrl->write_idx < 4)
		return 0;

	if (frm_ctrl->read_idx == 0)
		frm_ctrl->read_idx = 4;

	if (frm_ctrl->write_idx < (frm_ctrl->read_idx + FLASH_PAGE_SIZE)) {
		return 0;
	}

	u8 *cmd_param = (u8 *)&frm_ctrl->cmd_param[0];

	memcpy(&addr, cmd_param, 4);

	if (download_record_dl_flag == 0) {
		if ((addr >= CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET) &&
			(addr < CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET)) {
			frm_ctrl->status = OPERATE_PROTECTED_AREA;
			return 2;
		}
	}

	if (bl_forbid_operate_boot_partition(addr, FLASH_PAGE_SIZE) != true) {
		frm_ctrl->status = OPERATE_PROTECTED_AREA;
		return 2;
	}

	if (addr & (FLASH_PAGE_SIZE - 1)) {
		frm_ctrl->status = PARAM_ERROR;
		return 2;
	}

	addr += frm_ctrl->read_idx - 4;
	cmd_param += frm_ctrl->read_idx;

	while (frm_ctrl->write_idx >= (frm_ctrl->read_idx + FLASH_PAGE_SIZE)) {
		wdt_time_set(DOWNLOAD_WDT_VALUE);

		if (download_record_dl_flag == 0) {
			if ((addr >= CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET) &&
				(addr < CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET)) {
				frm_ctrl->status = OPERATE_PROTECTED_AREA;
				printf("the protected addr! \r\n");
				return 2;
			}
		}

		if (bl_forbid_operate_boot_partition(addr, FLASH_PAGE_SIZE) != true) {
			frm_ctrl->status = OPERATE_PROTECTED_AREA;
			printf("the protected addr \r\n");
			return 2;
		}

		flash_write_data(cmd_param, addr, FLASH_PAGE_SIZE);

		addr += FLASH_PAGE_SIZE;
		cmd_param += FLASH_PAGE_SIZE;
		frm_ctrl->read_idx += FLASH_PAGE_SIZE;
	}

	if (ret_val != 0) {
		frm_ctrl->status = ret_val;
		return 3;
	}

	return 0;
}

u32 flash_cmd_sector_write_done(rx_frm_ctrl_t *frm_ctrl)
{
	flash_cmd_sector_write(frm_ctrl);

	u8 *cmd_param = (u8 *)&frm_ctrl->cmd_param[0];

	tx_rsp_for_flash_cmd(FLASH_CMD_SECTOR_WRITE, frm_ctrl->status, cmd_param, 4);

	return 0;
}

static u32 flash_cmd_read_handler(u8 *cmd_param, u16 param_len)
{
	u32 addr;
	u16 size;
	u8 ret_val = STATUS_OK;

	memcpy(&addr, cmd_param, 4);
	size = cmd_param[4];

	if (param_len == 5) {
		flash_read_data(&cmd_param[4], addr, size);
	} else {
		ret_val = PACK_LEN_ERROR;
	}

	if (ret_val != 0)
		size = 0;

	tx_rsp_for_flash_cmd(FLASH_CMD_READ, ret_val, cmd_param, size + 4);

	return 0;
}

static u32 flash_cmd_sector_read_handler(u8 *cmd_param, u16 param_len)
{
	u32 start_addr = 0, read_cnt = 0;
	u8 ret_val = STATUS_OK;

	memcpy(&start_addr, cmd_param, 4);

	if (param_len == 4) {
		ret_val = STATUS_OK;
	} else {
		ret_val = PACK_LEN_ERROR;
	}

	tx_rsp_for_flash_cmd_hdr(FLASH_CMD_SECTOR_READ, ret_val, 0x1000 + 4);
	tx_rsp_data((u8 *)&start_addr, 4);

	while (read_cnt < 0x1000) {
		wdt_time_set(DOWNLOAD_WDT_VALUE);

		flash_read_data(cmd_param, start_addr, 128);

		if (ret_val == STATUS_OK) {
			tx_rsp_data(cmd_param, 128);
			start_addr += 128;
			read_cnt += 128;
		} else {
			break;
		}
	}

	return 0;
}

static u32 flash_cmd_sector_erase_handler(u8 *cmd_param, u16 param_len)
{
	u32 addr;
	u8 ret_val = 0;

	memcpy(&addr, cmd_param, 4);
#if CONFIG_BL2_UPDATE_WITH_PC
	if ((addr >= CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET) &&
		(addr < (CONFIG_BL2_B_PHY_PARTITION_OFFSET + CONFIG_BL2_B_PHY_PARTITION_SIZE))) {
		download_record_dl_flag = 1;
	}

	if (download_record_dl_flag == 0) {
		if ((addr >= CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET) &&
			(addr < CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET)) {
			ret_val = OPERATE_PROTECTED_AREA;
		}
	}
#endif
	if (bl_forbid_operate_boot_partition(addr, ERASE_4KB_LENGTH) != true) {
		ret_val = OPERATE_PROTECTED_AREA;
	} else if (param_len == 4) {
		flash_erase_cmd(addr, FLASH_OPCODE_SE);
	} else {
		ret_val = PACK_LEN_ERROR;
	}

	tx_rsp_for_flash_cmd((u16)FLASH_CMD_SECTOR_ERASE, ret_val, cmd_param, param_len);

	return 0;
}

static u32 flash_cmd_size_erase_handler(u8 *cmd_param, u16 param_len)
{
	u32 addr;
	u8 ret_val = 0;
	u8 size_cmd;

	size_cmd = cmd_param[0];
	memcpy(&addr, cmd_param + 1, 4);
#if CONFIG_BL2_UPDATE_WITH_PC
	if ((addr >= CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET) &&
		(addr < (CONFIG_BL2_B_PHY_PARTITION_OFFSET + CONFIG_BL2_B_PHY_PARTITION_SIZE))) {
		download_record_dl_flag = 1;
	}

	if (download_record_dl_flag == 0) {
		if ((addr >= CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET) &&
			(addr < CONFIG_PRIMARY_TFM_S_PHY_PARTITION_OFFSET)) {
			ret_val = OPERATE_PROTECTED_AREA;
		}
	}
#endif

	if (bl_forbid_erase_boot_partition(addr, size_cmd) != true) {
		ret_val = OPERATE_PROTECTED_AREA;
	} else if (param_len == 5) {
		if (size_cmd == 0x20) {
			flash_erase_cmd(addr, FLASH_OPCODE_SE);
		} else if (size_cmd == 0xd8) {
			flash_erase_cmd(addr, FLASH_OPCODE_BE2);
		} else if (size_cmd == 0x52) {
			flash_erase_cmd(addr, FLASH_OPCODE_BE1);
		}
	} else {
		ret_val = PACK_LEN_ERROR;
	}

	tx_rsp_for_flash_cmd(FLASH_CMD_SIZE_ERASE, ret_val, cmd_param, param_len);

	return 0;
}

static u32 flash_cmd_reg_read_handler(u8 *cmd_param, u16 param_len)
{
	u8 ret_val = 0;

	if (param_len < 1)
		return 1;

	cmd_param[1] = flash_read_sr(1);

	tx_rsp_for_flash_cmd(FLASH_CMD_REG_READ, ret_val, cmd_param, 2);

	return 0;
}

static u32 flash_cmd_reg_write_handler(u8 *cmd_param, u16 param_len)
{
	u8 ret_val = 0;
	u16 reg_val;
	u8 sr_bytes = 1;

	if (param_len < 2)
		return 1;

	reg_val = cmd_param[1];

	if (param_len > 2) {
		sr_bytes = 2;
		memcpy(&reg_val, cmd_param + 1, 2);
	}

	flash_write_sr(sr_bytes, reg_val);
	tx_rsp_for_flash_cmd(FLASH_CMD_REG_WRITE, ret_val, cmd_param, param_len);

	return 0;
}

static u32 flash_cmd_spi_op_handler(u8 *cmd_param, u16 param_len)
{
	u8 ret_val = 0;

	if (param_len > 2048)
		return 1;

	u8 *p_rx = cmd_param + 2100;

	if (cmd_param[0] != 0x9F)
		ret_val = PARAM_ERROR;

	p_rx[0] = (flash_id >> 24);
	p_rx[1] = (flash_id >> 16);
	p_rx[2] = (flash_id >> 8);
	p_rx[3] = (flash_id & 0xff);
	printf("FLASH_CMD_SPI_OPERATE: flash_id = 0x%x\r\n", flash_id);

	tx_rsp_for_flash_cmd(FLASH_CMD_SPI_OPERATE, ret_val, p_rx, param_len);

	return 0;
}

static u32 flash_cmd_unkown_handler(u16 cmd_id)
{
	u8 temp = UNKNOW_CMD;

	tx_rsp_for_flash_cmd(cmd_id, temp, NULL, 0);

	return 0;
}

static const cmd_hdlr_tbl_t flash_cmd_hdlr_tbl[] = {
	{ FLASH_CMD_WRITE,        flash_cmd_write_handler        },
//	{ FLASH_CMD_SECTOR_WRITE, flash_cmd_sector_write_handler },
	{ FLASH_CMD_READ,         flash_cmd_read_handler         },
	{ FLASH_CMD_SECTOR_READ,  flash_cmd_sector_read_handler  },
//	{ FLASH_CMD_CHIP_ERASE,   flash_cmd_chip_erase_handler   },
	{ FLASH_CMD_SECTOR_ERASE, flash_cmd_sector_erase_handler },
	{ FLASH_CMD_REG_READ,     flash_cmd_reg_read_handler     },
	{ FLASH_CMD_REG_WRITE,    flash_cmd_reg_write_handler    },
	{ FLASH_CMD_SPI_OPERATE,  flash_cmd_spi_op_handler       },
	{ FLASH_CMD_SIZE_ERASE,   flash_cmd_size_erase_handler   },
};

u32 flash_cmd_process(rx_frm_ctrl_t *frm_ctrl)
{
	u16 i;

	for (i = 0; i < TBL_SIZE(flash_cmd_hdlr_tbl); i++) {
		if (frm_ctrl->cmd_id == flash_cmd_hdlr_tbl[i].cmd_id) {
			flash_cmd_hdlr_tbl[i].cmd_handler((u8 *)&frm_ctrl->cmd_param[0],
				frm_ctrl->cmd_len - 1);
			return 0;
		}
	}

	flash_cmd_unkown_handler(frm_ctrl->cmd_id);

	return 1;
}
