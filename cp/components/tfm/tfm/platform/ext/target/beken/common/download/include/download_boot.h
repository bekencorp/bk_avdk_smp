/*
 * BK_HCI_protocol.h
 *
 *  Created on: 2017-5-8
 *      Author: gang.cheng
 */

#ifndef _DOWNLOAD_BOOT_H_
#define _DOWNLOAD_BOOT_H_
#include <stdbool.h>
#include <stdint.h>
#include <common/bk_err.h>
#include "type.h"
#include "bl_bk_reg.h"
#include "region_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLASH_DEVICE_BASE_OFFSET       (SOC_FLASH_MEM_BASE)
#define __PACKED_POST__  __attribute__((packed))

#define CMD_4KB_ERASE            (0x20)
#define CMD_32KB_ERASE          (0x52)
#define CMD_64KB_ERASE          (0xd8)
#define ERASE_4KB_LENGTH      (0x1000)
#define ERASE_32KB_LENGTH    (0x8000)
#define ERASE_64KB_LENGTH    (0x10000)
#define ALLOCATED_VERSION_LEN    (32)

/*
 * The bk7236n reference generates this value during build. The bk7259 bring-up
 * tree does not have that generator yet, so keep a static placeholder here.
 * boot update code stores it in a partition field sized by
 * ALLOCATED_VERSION_LEN, so keep it shorter than 32 bytes.
 */
#ifndef BL2_VERSION
#define BL2_VERSION "bk7259-bl2-0.0.0"
#endif

typedef enum boot_flag_t
{
    BOOT_FLAG_INVALID  	= 0,
    BOOT_FLAG_PRIMARY  	= 1,
    BOOT_FLAG_SECONDARY = 2,
}BOOT_FLAG;

typedef enum current_exec_part_t
{
    EXEC_BOOT_A_PARTITION = 1,
    EXEC_BOOT_B_PARTITION = 2,
}CURRENT_EXEC_PART;

typedef enum
{
	FLASH_PROTECT_NONE = 0,     /**< flash protect type none */
	FLASH_PROTECT_ALL,          /**< flash protect type all */
	FLASH_PROTECT_HALF,         /**< flash protect type half */
	FLASH_UNPROTECT_LAST_BLOCK, /**< flash protect type unprotect last block */
} flash_protect_type_t;

typedef struct {
    char *partition_name;
    u32 partition_offset;
    u32 partition_size;
} __PACKED_POST__ PARTITION_STRUCT;

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

enum
{
	// comon type cmd. distinguished by cmd_type.
	COMMON_CMD_LINK_CHECK   = 0x00,
	COMMON_RSP_LINK_CHECK   = 0x01,	// used as rsp, not command.
	COMMON_BL2_CMD_LINK_CHECK = 0x02,
	COMMON_RSP_BL2_CMD_LINK_CHECK = 0x03,

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
	COMMON_CMD_SET_BOOT_FLAG = 0xF0,
	COMMON_CMD_RPS_CURRENT_BOOT_PART = 0xF1,

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

	SECURE_AES_KEY          = 0x26,
	SECURE_RANDOM_KEY       = 0x27,
	SEC_RANDOM_KEY_REV_OVER = 0x28,
	SECURE_BOOT_ENABLE      = 0x29,
};

typedef struct
{
    u32 flash_id;
    u8  sr_size;
    u16 protect_all;
    u16 protect_none;
} flash_config_t;

#define RX_FRM_BUFF_SIZE    (4200)
#define FLASH_4K_SIZE       (0x1000)
#define FLASH_32K_SIZE      (0x8000)
#define FLASH_64K_SIZE      (0x10000)

typedef struct
{
	u32 rx_buf[RX_FRM_BUFF_SIZE / 4];		// buffer align with 32-bits.
	u16 read_idx;
	u16 write_idx;
} rx_link_buf_t;

extern rx_link_buf_t rx_link_buf;

extern bk_err_t bk_flash_read_bytes(uint32_t address, uint8_t *user_buf, uint32_t size);
extern bk_err_t bk_flash_write_bytes(uint32_t address, const uint8_t *user_buf, uint32_t size);
extern bk_err_t bk_flash_erase_cmd(uint32_t address, int type);
extern uint16_t bk_flash_read_sr(unsigned char byte);
extern bk_err_t bk_flash_write_sr(unsigned char bytes,  uint16_t status_reg_data);
extern uint32_t bk_flash_get_id(void);

#define   flash_read_data(user_buf, address, size)    bk_flash_read_bytes(address,user_buf,size)
#define   flash_write_data(user_buf, address, size)   bk_flash_write_bytes(address,user_buf,size)
#define   flash_erase_cmd(address, cmd)                 bk_flash_erase_cmd(address,cmd)
#define   flash_read_sr(byte)                                   bk_flash_read_sr(byte)
#define   flash_write_sr(bytes,  status_reg_data)     bk_flash_write_sr(bytes,  status_reg_data)
#define   flash_get_id()                                        bk_flash_get_id()

uint8_t bl_get_boot_flag_value(void);
uint8_t bl_set_boot_flag_value(void);
uint8_t bl_set_aon_pmu_bit3_for_deepsleep(void);
bool    bl_forbid_operate_boot_partition(uint32_t addr, uint32_t len);
bool    bl_forbid_erase_boot_partition(uint32_t addr, u8   size_cmd);
uint8_t bl_get_current_boot_execute_partition(void);

u32 boot_rx_frm_handler(void);
void boot_tx_startup_indication(void);
void legacy_boot_main_adapt(void);

#ifdef __cplusplus
}
#endif

#endif /* _DOWNLOAD_BOOT_H_ */
