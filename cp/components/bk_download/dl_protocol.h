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

#ifndef _dl_protocol_h_
#define _dl_protocol_h_

#ifdef __cplusplus
extern "C" {
#endif
#include <common/bk_err.h>
#include <driver/qspi.h>
#include "driver/qspi_flash.h"
#include "driver/flash.h"
#include "dl_uart.h"
enum
{
	// comon type cmd. distinguished by cmd_type.
	COMMON_CMD_LINK_CHECK   = 0x00,
	COMMON_RSP_LINK_CHECK   = 0x01,	// used as rsp, not command.
	COMMON_CMD_REG_WRITE    = 0x01,	// it is a command.
	COMMON_CMD_REG_READ     = 0x03,
	COMMON_CMD_REBOOT       = 0x0E,
	COMMON_CMD_SET_BAUDRATE = 0x0F,
	COMMON_CMD_CHECK_CRC32  = 0x10,
	COMMON_CMD_RESET        = 0x70,
	COMMON_CMD_STAY_ROM     = 0xAA,

	COMMON_CMD_EXT_REG_WRITE= 0x11,
	COMMON_CMD_EXT_REG_READ = 0x13,
	COMMON_CMD_STARTUP      = 0xFE,		// it is a startup indication.
	// flash type cmd. distinguished by cmd_type.
	FLASH_CMD_WRITE         = 0x06,
	FLASH_CMD_SECTOR_WRITE  = 0x07,
	FLASH_CMD_READ          = 0x08,
	FLASH_CMD_SECTOR_READ   = 0x09,
	FLASH_CMD_CHIP_ERASE    = 0x0A,
	FLASH_CMD_SECTOR_ERASE  = 0x0B,
	FLASH_CMD_REG_READ      = 0x0C,
	FLASH_CMD_REG_WRITE     = 0x0D,
	FLASH_CMD_SPI_OPERATE   = 0x0E,
	FLASH_CMD_SIZE_ERASE    = 0x0F,
	// can be flash type cmd or common type cmd. 
	// distinguished by cmd_type.
	EXT_CMD_RAM_WRITE       = 0x21,
	EXT_CMD_RAM_READ        = 0x23,
	EXT_CMD_JUMP            = 0x25,
};

typedef enum {
	FLASH_OPCODE_WREN    = 1,
	FLASH_OPCODE_WRDI    = 2,
	FLASH_OPCODE_RDSR    = 3,
	FLASH_OPCODE_WRSR    = 4,
	FLASH_OPCODE_READ    = 5,
	FLASH_OPCODE_RDSR2   = 6,
	FLASH_OPCODE_WRSR2   = 7,
	FLASH_OPCODE_PP      = 12,
	FLASH_OPCODE_SE      = 13,
	FLASH_OPCODE_BE1     = 14,
	FLASH_OPCODE_BE2     = 15,
	FLASH_OPCODE_CE      = 16,
	FLASH_OPCODE_DP      = 17,
	FLASH_OPCODE_RFDP    = 18,
	FLASH_OPCODE_RDID    = 20,
	FLASH_OPCODE_HPM     = 21,
	FLASH_OPCODE_CRMR    = 22,
	FLASH_OPCODE_CRMR2   = 23
}FLASH_OPCODE;

typedef enum{
	QSPI_FLASH_ERASE_SECTOR_CMD = 0x20,
	QSPI_FLASH_ERASE_32K_CMD    = 0x52,
	QSPI_FLASH_ERASE_64K_CMD    = 0xd8,
}QSPI_FLASH_OPCODE;

enum{	
	STATUS_OK = 0,
	FLASH_STATUS_BUSY = 1,
	SPI_OP_T_OUT = 2,
	FLASH_OP_T_OUT = 3,
	PACK_LEN_ERROR = 4,
	PACK_PAYLOAD_LACK = 5,
	PARAM_ERROR = 6,
	UNKNOW_CMD = 7,
};

#define RX_FRM_BUFF_SIZE	(4200)

typedef struct
{
	u32					rx_buf[RX_FRM_BUFF_SIZE / sizeof(u32)];		// buffer align with 32-bits.
	u16					read_idx;
	volatile u16		write_idx;
} rx_link_buf_t;

extern rx_link_buf_t rx_link_buf;

extern uint32_t bk_qspi_flash_read_s0_s7(qspi_id_t id);
extern uint32_t bk_qspi_flash_read_s8_s15(qspi_id_t id);
extern bk_err_t bk_qspi_flash_write_s0_s7(qspi_id_t id, uint8_t status_reg_data);
extern bk_err_t bk_qspi_flash_write_s8_s15(qspi_id_t id, uint8_t status_reg_data);
extern bk_err_t bk_qspi_flash_write_s0_s15(qspi_id_t id, uint16_t status_reg_data);

#define	flash_read_data(user_buf, address, size)    bk_qspi_flash_read(CONFIG_DL_QSPI_ID_NUMBER,  address, user_buf, size)
#define	flash_write_data(user_buf, address, size)   bk_qspi_flash_write(CONFIG_DL_QSPI_ID_NUMBER, address, user_buf, size)
#define flash_erase_cmd(address, cmd)               bk_qspi_flash_erase(CONFIG_DL_QSPI_ID_NUMBER, address, cmd)
#define	FLASH_READ_SR0_SR7                          bk_qspi_flash_read_s0_s7(CONFIG_DL_QSPI_ID_NUMBER)
#define	FLASH_READ_SR8_SR15                         bk_qspi_flash_read_s8_s15(CONFIG_DL_QSPI_ID_NUMBER)

#define	flash_set_protect_type(type) \
			do {\
				bk_qspi_flash_set_protect_none(CONFIG_DL_QSPI_ID_NUMBER);\
				bk_qspi_flash_quad_enable(CONFIG_DL_QSPI_ID_NUMBER);	\
			}while(0)

#define	FLASH_READ_SR \
			do {\
				bk_qspi_flash_read_s0_s7(CONFIG_DL_QSPI_ID_NUMBER);\
				bk_qspi_flash_read_s8_s15(CONFIG_DL_QSPI_ID_NUMBER);\
			}while(0)

#define	flash_write_sr(status_reg_data) \
			do {\
			 	bk_qspi_flash_write_s0_s15(CONFIG_DL_QSPI_ID_NUMBER,  status_reg_data);\
			 	bk_qspi_flash_write_s0_s7(CONFIG_DL_QSPI_ID_NUMBER,   (status_reg_data & 0xFF));\
			 	bk_qspi_flash_write_s8_s15(CONFIG_DL_QSPI_ID_NUMBER,  ((status_reg_data >> 8) & 0xFF));\
			 }while(0)

u32 boot_rx_frm_handler(void);
void boot_tx_startup_indication(void);
#ifdef __cplusplus
}
#endif

#endif /* _dl_protocol_h_ */

