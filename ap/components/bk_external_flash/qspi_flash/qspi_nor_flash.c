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

#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include <driver/qspi_nor_dev.h>
#include <driver/int.h>
#include <os/mem.h>
#include "qspi_flash_log.h"
#include <components/bk_platform.h>
#include <driver/aon_rtc.h>
#include "bk_misc.h"

#define QSPI_POLL_FAST_US          50
#define QSPI_POLL_FAST_PHASE_US    1000

#define FLASH_STATUS_REG_SIZE      1
#define FLASH_READ_ID_SIZE         3  // Flash ID is 3 bytes: MID, ID15-ID8, ID7-ID0
#define QSPI_FIFO_LEN_MAX          256
#define FLASH_STATUS_REG2_SIZE     2                   // write S0-S7 + S8-S15 in one command
#define FLASH_READ_ID_BUF_SIZE     4                   // 3 ID bytes + 1 padding

/* Program page granularity == controller FIFO depth; NOR page is 256B. */
#define FLASH_PAGE_SIZE            0x100
#define FLASH_PAGE_MASK            (FLASH_PAGE_SIZE - 1)

#define QSPI_FLASH_MAX_SCK_HZ      80000000            // QSPI max configurable SCK is 80MHz

#define FLASH_WIP_BIT              BIT(0)              // Write In Progress bit in status register
#define FLASH_WIP_TIMEOUT_US      (2000ULL * 1000)    // wait WIP done timeout: 2s

#define FLASH_ERASE_32K_SIZE      (32 * 1024)
#define FLASH_ERASE_64K_SIZE      (64 * 1024)

/* Erase-type selectors used by bk_qspi_flash_erase()/erase_type() dispatch.
 * They are logical tags, not the wire opcode: the opcode actually sent comes
 * from the active descriptor (dev->cmd.erase_sector/erase_32k/erase_64k). */
#define FLASH_ERASE_SECTOR_SEL     0x20
#define FLASH_ERASE_32K_SEL        0x52
#define FLASH_ERASE_64K_SEL        0xd8

/* Fallback capacity when Read ID (9Fh) returns a part not in the registry. */
#ifdef CONFIG_QSPI_NOR_FLASH_SIZE
#define NOR_FALLBACK_TOTAL_SIZE    (CONFIG_QSPI_NOR_FLASH_SIZE)
#else
#define NOR_FALLBACK_TOTAL_SIZE    (16U * 1024U * 1024U)
#endif

/* CONFIG_QSPI_LINE_MODE is the single source of truth for the flash wire mode.
 * bk_qspi_flash_write/read dispatch through these aliases; the single/dual/quad
 * APIs themselves stay unconditionally available.
 * For QSPI NOR flash, CONFIG_QSPI_LINE_MODE must be at least 2 (dual or quad). */
#if   (CONFIG_QSPI_LINE_MODE == 4)
#define bk_qspi_flash_line_read     bk_qspi_flash_quad_read
#define bk_qspi_flash_line_program  bk_qspi_flash_quad_page_program
#else
#define bk_qspi_flash_line_read     bk_qspi_flash_dual_read
#define bk_qspi_flash_line_program  bk_qspi_flash_dual_page_program
#endif

static void bk_qspi_flash_wait_wip_done(qspi_id_t id);

/* Only write_s16_s23 is exported in qspi_flash.h; the other status-register
 * writers are file-local API used before their definition below. */
bk_err_t bk_qspi_flash_write_s0_s7(qspi_id_t id, uint8_t status_reg_data);
bk_err_t bk_qspi_flash_write_s8_s15(qspi_id_t id, uint8_t status_reg_data);

/* Built-in default descriptor: GD/Winbond-style 3-byte-addressed NOR. Used
 * until Read ID selects a registered descriptor, and for boards that register
 * none. Matches the parameters this driver hard-coded before the registry
 * existed; fallback capacity comes from CONFIG_QSPI_NOR_FLASH_SIZE.
 *
 * GD status-register bit layout:
 *   S7-S0 : SRP0 BP4 BP3 BP2 BP1 BP0 WEL WIP
 *   S15-S8: SUS1 CMP LB3 LB2 LB1 SUS2 QE SRP1
 */
static const qspi_nor_dev_t s_nor_dev_default = {
	.name = "generic",
	.id = {0},
	.id_len = 0,
	.geometry = {
		.total_size = NOR_FALLBACK_TOTAL_SIZE,
		.sector_size = 4U * 1024U,
		.page_size = 256U,
	},
	.cmd = {
		.read_id = 0x9F,
		.write_enable = 0x06,
		.read_sr1 = 0x05,
		.read_sr2 = 0x35,
		.read_sr3 = 0x15,
		.write_sr1 = 0x01,
		.write_sr2 = 0x31,
		.write_sr3 = 0x11,
		.page_program = 0x02,
		.quad_page_program = 0x32,
		.read = 0x03,
		.dual_read = 0x3B,
		.quad_read = 0xEB,
		.erase_sector = 0x20,
		.erase_32k = 0x52,
		.erase_64k = 0xD8,
		.enter_4byte = 0x00,     /* 3-byte addressing, no enter-4B needed */
		.addr_bytes = 3,
		.dual_read_dummy_cycle = 8,   /* 0x3B needs 8 dummy clocks */
		.dual_read_dummy_mode = 4,    /* dummy after opcode(1)+addr(3), no mode byte */
		.quad_read_dummy_cycle = 4,
		.quad_read_dummy_mode = 5,
	},
	.reg = {
		.qe_sr_index = 1,        /* QE lives in SR2 (S8-S15) */
		.qe_bit = BIT(1),
		.srp0_bit = BIT(7),      /* SRP0 in SR1 */
		.srp1_bit = BIT(0),      /* SRP1 in SR2 */
		.sr1_preserve_mask = 0x83,  /* keep SRP0/WEL/WIP; clear BP4-BP0 */
		.bp_shift = 2,           /* BP2/BP1/BP0 start at bit 2 */
		.bp_mask = 0x07,
	},
};

static const qspi_nor_dev_t *s_active_dev[QSPI_ID_MAX];

const qspi_nor_dev_t *bk_qspi_nor_dev(qspi_id_t id)
{
	if (id >= QSPI_ID_MAX || !s_active_dev[id]) {
		return &s_nor_dev_default;
	}
	return s_active_dev[id];
}

uint32_t bk_qspi_flash_nor_total_size(qspi_id_t id)
{
	return bk_qspi_nor_dev(id)->geometry.total_size;
}

/* Read/write the status register selected by index (0=SR1,1=SR2,2=SR3) using
 * the public per-SR helpers so descriptor opcodes drive the actual command. */
static uint32_t nor_read_sr(qspi_id_t id, uint8_t index)
{
	switch (index) {
	case 0:  return bk_qspi_flash_read_s0_s7(id);
	case 1:  return bk_qspi_flash_read_s8_s15(id);
	case 2:  return bk_qspi_flash_read_s16_s23(id);
	default: return bk_qspi_flash_read_s8_s15(id);
	}
}

static bk_err_t nor_write_sr(qspi_id_t id, uint8_t index, uint8_t value)
{
	switch (index) {
	case 0:  return bk_qspi_flash_write_s0_s7(id, value);
	case 1:  return bk_qspi_flash_write_s8_s15(id, value);
	case 2:  return bk_qspi_flash_write_s16_s23(id, value);
	default: return bk_qspi_flash_write_s8_s15(id, value);
	}
}

static void bk_qspi_flash_wren(qspi_id_t id) {
	qspi_cmd_t wren_cmd = {0};

	wren_cmd.device = QSPI_FLASH;
	wren_cmd.data_wire_mode = QSPI_1WIRE;
	wren_cmd.work_mode = INDIRECT_MODE;
	wren_cmd.op = QSPI_WRITE;
	wren_cmd.cmd = bk_qspi_nor_dev(id)->cmd.write_enable;
	wren_cmd.data_len = 0;

	BK_LOG_ON_ERR(bk_qspi_command(id, &wren_cmd));
	bk_qspi_flash_wait_wip_done(id);
}

/* Enter 4-byte address mode (WREN + enter-4B opcode). Called at init for
 * descriptors that advertise addr_bytes==4 and a non-zero enter_4byte opcode. */
static void bk_qspi_flash_enter_4byte(qspi_id_t id)
{
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	qspi_cmd_t cmd = {0};

	bk_qspi_flash_wren(id);

	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = dev->cmd.enter_4byte;
	cmd.data_len = 0;

	BK_LOG_ON_ERR(bk_qspi_command(id, &cmd));
}

uint32_t bk_qspi_flash_read_s0_s7(qspi_id_t id)
{
	qspi_cmd_t read_status_cmd = {0};
	uint32_t status_reg_data = 0;

	read_status_cmd.device = QSPI_FLASH;
	read_status_cmd.data_wire_mode = QSPI_1WIRE;
	read_status_cmd.work_mode = INDIRECT_MODE;
	read_status_cmd.op = QSPI_READ;
	read_status_cmd.cmd = bk_qspi_nor_dev(id)->cmd.read_sr1;
	read_status_cmd.data_len = FLASH_STATUS_REG_SIZE;

	BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
	bk_qspi_read(id, &status_reg_data, FLASH_STATUS_REG_SIZE);
	QSPI_LOGV("[%s]: status_reg_data = 0x%x.\n", __func__, (uint8_t)status_reg_data);

	return status_reg_data;
}

uint32_t bk_qspi_flash_read_s8_s15(qspi_id_t id)
{
	qspi_cmd_t read_status_cmd = {0};
	uint32_t status_reg_data = 0;

	read_status_cmd.device = QSPI_FLASH;
	read_status_cmd.data_wire_mode = QSPI_1WIRE;
	read_status_cmd.work_mode = INDIRECT_MODE;
	read_status_cmd.op = QSPI_READ;
	read_status_cmd.cmd = bk_qspi_nor_dev(id)->cmd.read_sr2;
	read_status_cmd.data_len = FLASH_STATUS_REG_SIZE;

	BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
	bk_qspi_read(id, &status_reg_data, FLASH_STATUS_REG_SIZE);
	QSPI_LOGV("[%s]: status_reg_data = 0x%x.\n", __func__, (uint8_t)status_reg_data);

	return status_reg_data;
}

uint32_t bk_qspi_flash_read_s16_s23(qspi_id_t id)
{
	qspi_cmd_t read_status_cmd = {0};
	uint32_t status_reg_data = 0;

	read_status_cmd.device = QSPI_FLASH;
	read_status_cmd.data_wire_mode = QSPI_1WIRE;
	read_status_cmd.work_mode = INDIRECT_MODE;
	read_status_cmd.op = QSPI_READ;
	read_status_cmd.cmd = bk_qspi_nor_dev(id)->cmd.read_sr3;
	read_status_cmd.data_len = FLASH_STATUS_REG_SIZE;

	BK_LOG_ON_ERR(bk_qspi_command(id, &read_status_cmd));
	bk_qspi_read(id, &status_reg_data, FLASH_STATUS_REG_SIZE);
	QSPI_LOGV("[%s]: status_reg_data = 0x%x.\n", __func__, (uint8_t)status_reg_data);

	return status_reg_data;
}

bk_err_t bk_qspi_flash_write_s0_s7(qspi_id_t id, uint8_t status_reg_data)
{
	qspi_cmd_t write_status_cmd = {0};

	bk_qspi_flash_wren(id);
	write_status_cmd.device = QSPI_FLASH;
	write_status_cmd.data_wire_mode = QSPI_1WIRE;
	write_status_cmd.work_mode = INDIRECT_MODE;
	write_status_cmd.op = QSPI_WRITE;
	write_status_cmd.cmd = bk_qspi_nor_dev(id)->cmd.write_sr1;
	write_status_cmd.data_len = FLASH_STATUS_REG_SIZE;

	bk_qspi_write(id, (void *)&status_reg_data, FLASH_STATUS_REG_SIZE);
	BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
	QSPI_LOGD("[%s]: status_reg_data to be writen is 0x%x.\n", __func__, status_reg_data);
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_write_s8_s15(qspi_id_t id, uint8_t status_reg_data)
{
	qspi_cmd_t write_status_cmd = {0};
	bk_qspi_flash_wren(id);
	write_status_cmd.device = QSPI_FLASH;
	write_status_cmd.data_wire_mode = QSPI_1WIRE;
	write_status_cmd.work_mode = INDIRECT_MODE;
	write_status_cmd.op = QSPI_WRITE;
	write_status_cmd.cmd = bk_qspi_nor_dev(id)->cmd.write_sr2;
	write_status_cmd.data_len = FLASH_STATUS_REG_SIZE;

	bk_qspi_write(id, (void *)&status_reg_data, FLASH_STATUS_REG_SIZE);
	BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
	QSPI_LOGD("[%s]: status_reg_data to be writen is 0x%x.\n", __func__, status_reg_data);
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_write_s16_s23(qspi_id_t id, uint8_t status_reg_data)
{
	qspi_cmd_t write_status_cmd = {0};
	bk_qspi_flash_wren(id);
	write_status_cmd.device = QSPI_FLASH;
	write_status_cmd.data_wire_mode = QSPI_1WIRE;
	write_status_cmd.work_mode = INDIRECT_MODE;
	write_status_cmd.op = QSPI_WRITE;
	write_status_cmd.cmd = bk_qspi_nor_dev(id)->cmd.write_sr3;
	write_status_cmd.data_len = FLASH_STATUS_REG_SIZE;

	bk_qspi_write(id, (void *)&status_reg_data, FLASH_STATUS_REG_SIZE);
	BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
	QSPI_LOGD("[%s]: status_reg_data to be writen is 0x%x.\n", __func__, status_reg_data);
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_write_s0_s15(qspi_id_t id, uint16_t status_reg_data)
{
	qspi_cmd_t write_status_cmd = {0};

	bk_qspi_flash_wren(id);
	write_status_cmd.device = QSPI_FLASH;
	write_status_cmd.data_wire_mode = QSPI_1WIRE;
	write_status_cmd.work_mode = INDIRECT_MODE;
	write_status_cmd.op = QSPI_WRITE;
	write_status_cmd.cmd = bk_qspi_nor_dev(id)->cmd.write_sr1;
	write_status_cmd.data_len = FLASH_STATUS_REG2_SIZE;

	bk_qspi_write(id, (void *)&status_reg_data, write_status_cmd.data_len);
	BK_LOG_ON_ERR(bk_qspi_command(id, &write_status_cmd));
	QSPI_LOGD("[%s]: status_reg_data to be writen is 0x%x.\n", __func__, status_reg_data);
	QSPI_LOGD("[%s]: write_status_cmd.cmd is 0x%x.\n", __func__, write_status_cmd.cmd);
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

static inline void qspi_poll_backoff(uint64_t start_us)
{
	if (bk_aon_rtc_get_us() - start_us < QSPI_POLL_FAST_PHASE_US) {
		bk_delay_us(QSPI_POLL_FAST_US);
	} else {
		rtos_delay_milliseconds(1);
	}
}

static void bk_qspi_flash_wait_wip_done(qspi_id_t id)
{
	uint32_t status_reg_data = 0;
	uint64_t start = bk_aon_rtc_get_us();
	const uint64_t timeout_us = FLASH_WIP_TIMEOUT_US;

	while (bk_aon_rtc_get_us() - start <= timeout_us) {
		status_reg_data = bk_qspi_flash_read_s0_s7(id);
		if (0 == (status_reg_data & FLASH_WIP_BIT)) {
			return;
		}
		qspi_poll_backoff(start);
	}

	QSPI_LOGW("[%s]: wait write_in_progress done timeout.\n", __func__);
}

/**
 * @brief Erase a 4KB sector in Flash
 *
 * @param id QSPI ID
 * @param addr Physical address in Flash (not sector number). Flash will erase
 *             the entire 4KB sector containing this address. The address should
 *             typically be aligned to sector boundary (4KB = 0x1000) for clarity,
 *             but any address within the sector will work.
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_qspi_flash_erase_sector(qspi_id_t id, uint32_t addr)
{
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	qspi_cmd_t erase_sector_cmd = {0};

	bk_qspi_flash_wren(id);

	erase_sector_cmd.device = QSPI_FLASH;
	erase_sector_cmd.data_wire_mode = QSPI_1WIRE;
	erase_sector_cmd.work_mode = INDIRECT_MODE;
	erase_sector_cmd.op = QSPI_WRITE;
	erase_sector_cmd.cmd = dev->cmd.erase_sector;
	erase_sector_cmd.addr = addr;  // Physical address, Flash will erase the sector containing this address
	erase_sector_cmd.addr_len = dev->cmd.addr_bytes;
	erase_sector_cmd.addr_wire_mode = QSPI_1WIRE;

	BK_LOG_ON_ERR(bk_qspi_command(id, &erase_sector_cmd));
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_erase_32k(qspi_id_t id, uint32_t addr)
{
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	qspi_cmd_t erase_sector_cmd = {0};

	bk_qspi_flash_wren(id);

	erase_sector_cmd.device = QSPI_FLASH;
	erase_sector_cmd.data_wire_mode = QSPI_1WIRE;
	erase_sector_cmd.work_mode = INDIRECT_MODE;
	erase_sector_cmd.op = QSPI_WRITE;
	erase_sector_cmd.cmd = dev->cmd.erase_32k;
	erase_sector_cmd.addr = addr;
	erase_sector_cmd.addr_len = dev->cmd.addr_bytes;
	erase_sector_cmd.addr_wire_mode = QSPI_1WIRE;

	BK_LOG_ON_ERR(bk_qspi_command(id, &erase_sector_cmd));
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_erase_64k(qspi_id_t id, uint32_t addr)
{
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	qspi_cmd_t erase_sector_cmd = {0};

	bk_qspi_flash_wren(id);

	erase_sector_cmd.device = QSPI_FLASH;
	erase_sector_cmd.data_wire_mode = QSPI_1WIRE;
	erase_sector_cmd.work_mode = INDIRECT_MODE;
	erase_sector_cmd.op = QSPI_WRITE;
	erase_sector_cmd.cmd = dev->cmd.erase_64k;
	erase_sector_cmd.addr = addr;
	erase_sector_cmd.addr_len = dev->cmd.addr_bytes;
	erase_sector_cmd.addr_wire_mode = QSPI_1WIRE;

	BK_LOG_ON_ERR(bk_qspi_command(id, &erase_sector_cmd));
	bk_qspi_flash_wait_wip_done(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_erase_type(qspi_id_t id, uint32_t addr, uint32_t type)
{
	bk_err_t ret = BK_OK;

	switch(type)
	{
		case FLASH_ERASE_SECTOR_SEL:
			bk_qspi_flash_erase_sector(id, addr);
		break;
		case FLASH_ERASE_32K_SEL:
			bk_qspi_flash_erase_32k(id, addr);
			break;
		case FLASH_ERASE_64K_SEL:
			bk_qspi_flash_erase_64k(id, addr);
		break;
		default:
			bk_qspi_flash_erase_sector(id, addr);
		break;
	}

	return ret;
}

bk_err_t bk_qspi_flash_erase(qspi_id_t id, uint32_t addr, uint32_t size)
{
    bk_err_t ret = BK_OK;
    uint32_t capacity = bk_qspi_nor_dev(id)->geometry.total_size;
    uint32_t sector_size = bk_qspi_nor_dev(id)->geometry.sector_size;

    QSPI_LOGV("[%s]addr=0x%08X, size=%u.\r\n", __func__, addr, size);

    if ( (addr >= capacity) || (size > capacity) || ((addr + size) > capacity) )
    {
        QSPI_LOGE("[%s] addr or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    if ( 0 == size )
    {
        QSPI_LOGE("[%s] buff or size paras error!\r\n", __func__);
        return BK_FAIL;
    }

    int left_size = (int)size;
    while (left_size > 0)
    {
        uint32_t erase_size = 0, erase_mode;

        #if (CONFIG_TASK_WDT)
            extern void bk_task_wdt_feed(void);
            bk_task_wdt_feed();
        #endif

        if(left_size <= (int)sector_size)
        {
            erase_size = sector_size;
            erase_mode = FLASH_ERASE_SECTOR_SEL;
        }
        else if(size <= FLASH_ERASE_32K_SIZE)
        {
            erase_size = FLASH_ERASE_32K_SIZE;
            erase_mode = FLASH_ERASE_32K_SEL;
        }
        else
        {
            erase_size = FLASH_ERASE_64K_SIZE;
            erase_mode = FLASH_ERASE_64K_SEL;
        }

        ret = bk_qspi_flash_erase_type(id, addr, erase_mode);
        if (BK_OK != ret)
        {
            QSPI_LOGE("[%s] bk_qspi_flash_erase_type fail, addr=0x%x, erase_mode=%d.\r\n", __func__, addr, erase_mode);
            return BK_FAIL;
        }

        if(addr & (erase_size - 1))
        {
            size = erase_size - (addr & (erase_size - 1));
        }
        else
        {
            size = erase_size;
        }

        QSPI_LOGV("addr:%d,erase_mode:%d,left_size:%d,size:%d\r\n",addr,erase_mode,left_size,size);

        left_size -= size;
        addr += size;
    }

    return BK_OK;
}


bk_err_t bk_qspi_flash_single_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	qspi_cmd_t page_program_cmd = {0};
	uint32_t cmd_data_len = 0;
	uint32_t *cmd_data = (uint32_t *)data;

	page_program_cmd.device = QSPI_FLASH;
	page_program_cmd.data_wire_mode = QSPI_1WIRE;
	page_program_cmd.work_mode = INDIRECT_MODE;
	page_program_cmd.op = QSPI_WRITE;
	page_program_cmd.cmd = dev->cmd.page_program;
	page_program_cmd.addr_len = dev->cmd.addr_bytes;
	page_program_cmd.addr_wire_mode = QSPI_1WIRE;

	while(0 < size) {
		bk_qspi_flash_wren(id);

		cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
		bk_qspi_write(id, cmd_data, cmd_data_len);
		page_program_cmd.addr = addr;
		page_program_cmd.data_len = cmd_data_len;
		BK_LOG_ON_ERR(bk_qspi_command(id, &page_program_cmd));
		addr += cmd_data_len;
		cmd_data += cmd_data_len;
		size -= cmd_data_len;

		bk_qspi_flash_wait_wip_done(id);
	}

	return BK_OK;
}

bk_err_t bk_qspi_flash_single_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	qspi_cmd_t single_read_cmd = {0};
	uint32_t cmd_data_len = 0;
	uint32_t *cmd_data = data;

	single_read_cmd.device = QSPI_FLASH;
	single_read_cmd.data_wire_mode = QSPI_1WIRE;
	single_read_cmd.work_mode = INDIRECT_MODE;
	single_read_cmd.op = QSPI_READ;
	single_read_cmd.cmd = dev->cmd.read;
	single_read_cmd.addr_len = dev->cmd.addr_bytes;
	single_read_cmd.addr_wire_mode = QSPI_1WIRE;

	while(0 < size) {
		cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
		single_read_cmd.addr = addr;
		single_read_cmd.data_len = cmd_data_len;
		BK_LOG_ON_ERR(bk_qspi_command(id, &single_read_cmd));
		bk_qspi_read(id, cmd_data, cmd_data_len);

		addr += cmd_data_len;
		cmd_data += cmd_data_len;
		size -= cmd_data_len;
	}
	return BK_OK;
}

bk_err_t bk_qspi_flash_quad_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	bk_err_t ret = BK_OK;
	qspi_cmd_t page_program_cmd = {0};
	uint32_t cmd_data_len = 0;
	uint32_t *cmd_data = (uint32_t *)data;

	page_program_cmd.device = QSPI_FLASH;
	page_program_cmd.data_wire_mode = QSPI_4WIRE;
	page_program_cmd.work_mode = INDIRECT_MODE;
	page_program_cmd.op = QSPI_WRITE;
	page_program_cmd.cmd = dev->cmd.quad_page_program;
	page_program_cmd.dummy_cycle = 0;  // Quad write doesn't need dummy cycles
	page_program_cmd.addr_len = dev->cmd.addr_bytes;
	page_program_cmd.addr_wire_mode = QSPI_1WIRE;

	//QSPI_LOGD("[%s] Start: addr=0x%08X, size=%d\r\n", __func__, addr, size);

	while(0 < size) {
		// Write Enable
		bk_qspi_flash_wren(id);

		cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;

		// Write data to FIFO first
		ret = bk_qspi_write(id, cmd_data, cmd_data_len);
		if (ret != BK_OK) {
			QSPI_LOGE("[%s] bk_qspi_write failed: %d\r\n", __func__, ret);
			return ret;
		}

		// Then send command with address and data length
		page_program_cmd.addr = addr;
		page_program_cmd.data_len = cmd_data_len;
		ret = bk_qspi_command(id, &page_program_cmd);
		if (ret != BK_OK) {
			QSPI_LOGE("[%s] bk_qspi_command failed: %d, addr=0x%08X, len=%d\r\n", __func__, ret, addr, cmd_data_len);
			return ret;
		}

		//QSPI_LOGD("[%s] Written: addr=0x%08X, len=%d\r\n", __func__, addr, cmd_data_len);

		// Wait for write to complete
		bk_qspi_flash_wait_wip_done(id);
		//QSPI_LOGD("[%s] Write operation completed for addr=0x%08X, len=%d\r\n", __func__, addr, cmd_data_len);

		addr += cmd_data_len;
		cmd_data += cmd_data_len;
		size -= cmd_data_len;
	}

	//QSPI_LOGD("[%s] Complete\r\n", __func__);
	return BK_OK;
}

bk_err_t bk_qspi_flash_quad_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	qspi_cmd_t quad_read_cmd = {0};

	/* Some parts (e.g. 4-byte-address NOR) must read single-line because the
	 * controller's quad read corrupts data with 4-byte addressing. */
	if (dev->cmd.read_force_single) {
		return bk_qspi_flash_single_read(id, addr, data, size);
	}
	uint32_t cmd_data_len = 0;
	uint32_t *cmd_data = data;

	quad_read_cmd.device = QSPI_FLASH;
	quad_read_cmd.data_wire_mode = QSPI_4WIRE;
	quad_read_cmd.work_mode = INDIRECT_MODE;
	quad_read_cmd.op = QSPI_READ;
	quad_read_cmd.cmd = dev->cmd.quad_read;
	quad_read_cmd.dummy_cycle = dev->cmd.quad_read_dummy_cycle;
	quad_read_cmd.dummy_mode = dev->cmd.quad_read_dummy_mode;
	quad_read_cmd.addr_len = dev->cmd.addr_bytes;
	/* Quad Output Read (0x6B/0x6C) clocks the address on a single wire (no 4-wire
	 * mode byte); Quad I/O Read (0xEB/0xEC) clocks it on four wires. */
	quad_read_cmd.addr_wire_mode = dev->cmd.quad_read_addr_1wire ? QSPI_1WIRE : QSPI_4WIRE;

	while(0 < size) {
		cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
		quad_read_cmd.addr = addr;
		quad_read_cmd.data_len = cmd_data_len;
		BK_LOG_ON_ERR(bk_qspi_command(id, &quad_read_cmd));
		bk_qspi_read(id, cmd_data, cmd_data_len);

		addr += cmd_data_len;
		cmd_data += cmd_data_len;
		size -= cmd_data_len;
	}
	return BK_OK;
}

bk_err_t bk_qspi_flash_dual_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	qspi_cmd_t dual_read_cmd = {0};
	uint32_t cmd_data_len = 0;
	uint32_t *cmd_data = data;

	dual_read_cmd.device = QSPI_FLASH;
	dual_read_cmd.data_wire_mode = QSPI_2WIRE;
	dual_read_cmd.work_mode = INDIRECT_MODE;
	dual_read_cmd.op = QSPI_READ;
	dual_read_cmd.cmd = dev->cmd.dual_read;
	dual_read_cmd.dummy_cycle = dev->cmd.dual_read_dummy_cycle;
	dual_read_cmd.dummy_mode = dev->cmd.dual_read_dummy_mode;
	dual_read_cmd.addr_len = dev->cmd.addr_bytes;
	dual_read_cmd.addr_wire_mode = QSPI_1WIRE;

	while(0 < size) {
		cmd_data_len = (size < QSPI_FIFO_LEN_MAX) ? size : QSPI_FIFO_LEN_MAX;
		dual_read_cmd.addr = addr;
		dual_read_cmd.data_len = cmd_data_len;
		BK_LOG_ON_ERR(bk_qspi_command(id, &dual_read_cmd));
		bk_qspi_read(id, cmd_data, cmd_data_len);

		addr += cmd_data_len;
		cmd_data += cmd_data_len;
		size -= cmd_data_len;
	}
	return BK_OK;
}

bk_err_t bk_qspi_flash_dual_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
	/* Standard SPI NOR has no dual-input page program; fall back to single-line 0x02. */
	return bk_qspi_flash_single_page_program(id, addr, data, size);
}

bk_err_t bk_qspi_flash_write(qspi_id_t id, uint32_t base_addr, const void *data, uint32_t size)
{
	uint32_t capacity = bk_qspi_nor_dev(id)->geometry.total_size;
	uint8_t buf[QSPI_FIFO_LEN_MAX] = {0};
	uint32_t left_len = size;
	uint32_t write_len= 0;
	uint32_t write_addr = 0;
	uint32_t offset = 0;
	uint32_t page_write_len = 0;
	bk_err_t ret = BK_OK;

	QSPI_LOGD("[%s] Called with base_addr=0x%08X, size=%d\r\n", __func__, base_addr, size);

	if (NULL == data) {
		QSPI_LOGE("[%s] data is NULL!\r\n", __func__);
		return BK_ERR_NULL_PARAM;
	}
	if (0 == size) {
		QSPI_LOGE("[%s] size is 0!\r\n", __func__);
		return BK_ERR_PARAM;
	}
	if ((base_addr >= capacity) || (size > capacity) || ((base_addr + size) > capacity)) {
		QSPI_LOGE("[%s] addr/size out of range[addr=0x%08X, size=%u]!\r\n", __func__, base_addr, size);
		return BK_ERR_PARAM;
	}

	if(0 != (base_addr & FLASH_PAGE_MASK)) {
		write_addr = base_addr & (~FLASH_PAGE_MASK);
		QSPI_LOGD("[%s] Address not page-aligned, aligning to 0x%08X, reading page first\r\n", __func__, write_addr);
		ret = bk_qspi_flash_line_read(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
		if (BK_OK != ret) {
			QSPI_LOGE("[%s] quad_read head fail[ret=%d, addr=0x%08X]!\r\n", __func__, ret, write_addr);
			return ret;
		}
		page_write_len = (QSPI_FIFO_LEN_MAX - (base_addr & FLASH_PAGE_MASK));
		write_len = (page_write_len > left_len) ? left_len : page_write_len;
		QSPI_LOGD("[%s] Copying %d bytes to buffer offset 0x%02X\r\n", __func__, write_len, (base_addr & FLASH_PAGE_MASK));
		os_memcpy(buf + (base_addr & FLASH_PAGE_MASK), data, write_len);
		QSPI_LOGD("[%s] Writing full page (256 bytes) to address 0x%08X\r\n", __func__, write_addr);
		ret = bk_qspi_flash_line_program(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
		if (BK_OK != ret) {
			QSPI_LOGE("[%s] page_program head fail[ret=%d, addr=0x%08X]!\r\n", __func__, ret, write_addr);
			return ret;
		}
		offset += write_len;
		left_len -= write_len;
		if(left_len == 0) {
			return BK_OK;
		}
	}

	for (write_addr = base_addr + offset; write_addr < ((base_addr + size) & (~FLASH_PAGE_MASK)); write_addr += write_len) {
		write_len = (left_len > QSPI_FIFO_LEN_MAX) ? QSPI_FIFO_LEN_MAX : left_len;
		ret = bk_qspi_flash_line_program(id, write_addr, data + offset, write_len);
		if (BK_OK != ret) {
			QSPI_LOGE("[%s] page_program middle fail[ret=%d, addr=0x%08X]!\r\n", __func__, ret, write_addr);
			return ret;
		}
		offset += write_len;
		left_len -= write_len;
	}

	if(left_len == 0) {
		return BK_OK;
	}
	if(0 != ((base_addr + size) & FLASH_PAGE_MASK)) {
		// Last partial page: read entire page, merge data, write entire page
		write_addr = (base_addr + offset) & (~FLASH_PAGE_MASK);
		QSPI_LOGD("[%s] Last partial page, reading page at 0x%08X\r\n", __func__, write_addr);
		ret = bk_qspi_flash_line_read(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
		if (BK_OK != ret) {
			QSPI_LOGE("[%s] quad_read tail fail[ret=%d, addr=0x%08X]!\r\n", __func__, ret, write_addr);
			return ret;
		}
		write_len = (base_addr + size) & FLASH_PAGE_MASK;
		QSPI_LOGD("[%s] Copying %d bytes to buffer offset 0x%02X\r\n", __func__, write_len, ((base_addr + offset) & FLASH_PAGE_MASK));
		os_memcpy(buf + ((base_addr + offset) & FLASH_PAGE_MASK), data + offset, write_len);
		QSPI_LOGD("[%s] Writing full page (256 bytes) to address 0x%08X\r\n", __func__, write_addr);
		ret = bk_qspi_flash_line_program(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
		if (BK_OK != ret) {
			QSPI_LOGE("[%s] page_program tail fail[ret=%d, addr=0x%08X]!\r\n", __func__, ret, write_addr);
			return ret;
		}
	}

	return BK_OK;
}

bk_err_t bk_qspi_flash_read(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
	uint32_t capacity = bk_qspi_nor_dev(id)->geometry.total_size;
	uint8_t buf[QSPI_FIFO_LEN_MAX] = {0};
	uint32_t left_len = size;
	uint32_t read_len= 0;
	uint32_t offset = 0;
	bk_err_t ret = BK_OK;

	if (NULL == data) {
		QSPI_LOGE("[%s] data is NULL!\r\n", __func__);
		return BK_ERR_NULL_PARAM;
	}
	if (0 == size) {
		QSPI_LOGE("[%s] size is 0!\r\n", __func__);
		return BK_ERR_PARAM;
	}
	if ((base_addr >= capacity) || (size > capacity) || ((base_addr + size) > capacity)) {
		QSPI_LOGE("[%s] addr/size out of range[addr=0x%08X, size=%u]!\r\n", __func__, base_addr, size);
		return BK_ERR_PARAM;
	}

	for (uint32_t addr = base_addr; addr < (base_addr + size); addr += QSPI_FIFO_LEN_MAX) {
		offset += read_len;
		read_len = (left_len >= QSPI_FIFO_LEN_MAX) ? QSPI_FIFO_LEN_MAX : left_len;
		ret = bk_qspi_flash_line_read(id, addr, buf, QSPI_FIFO_LEN_MAX);
		if (BK_OK != ret) {
			QSPI_LOGE("[%s] quad_read fail[ret=%d, addr=0x%08X]!\r\n", __func__, ret, addr);
			return ret;
		}
		os_memcpy(data + offset, buf, read_len);
		left_len -= QSPI_FIFO_LEN_MAX;
	}

	return BK_OK;
}

bk_err_t bk_qspi_flash_quad_enable(qspi_id_t id) {
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	uint8_t qe_bit = dev->reg.qe_bit;
	uint8_t qe_idx = dev->reg.qe_sr_index;
	bk_err_t ret = BK_OK;
	uint32_t status_reg_data = 0;
	uint32_t status_reg_read = 0;

	// Read current QE status register
	status_reg_data = (uint8_t)nor_read_sr(id, qe_idx);
	QSPI_LOGD("[%s] Before: QE-SR[%d]=0x%02X\r\n", __func__, qe_idx, (uint8_t)status_reg_data);

	// Check if QE bit is already set
	if (status_reg_data & qe_bit) {
		QSPI_LOGD("[%s] QE bit already set, no need to write\r\n", __func__);
		return BK_OK;
	}

	// Set QE bit
	uint8_t original_status = status_reg_data;
	status_reg_data |= qe_bit;
	QSPI_LOGD("[%s] Setting QE bit: QE-SR[%d]=0x%02X -> 0x%02X\r\n", __func__,
	          qe_idx, original_status, (uint8_t)status_reg_data);

	ret = nor_write_sr(id, qe_idx, (uint8_t)status_reg_data);
	if (ret != BK_OK) {
		QSPI_LOGE("[%s] Write QE-SR[%d] failed: %d\r\n", __func__, qe_idx, ret);
		return ret;
	}
	// Verify QE bit is set
	status_reg_read = (uint8_t)nor_read_sr(id, qe_idx);
	QSPI_LOGD("[%s] After: QE-SR[%d]=0x%02X (expected QE bit set)\r\n", __func__, qe_idx, (uint8_t)status_reg_read);
	if (!(status_reg_read & qe_bit)) {
		QSPI_LOGE("[%s] QE bit not set! QE-SR[%d]=0x%02X\r\n", __func__, qe_idx, (uint8_t)status_reg_read);
		return BK_FAIL;
	}
	return BK_OK;
}

bk_err_t bk_qspi_flash_set_protect_none(qspi_id_t id) {
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	uint8_t qe_bit = dev->reg.qe_bit;
	uint8_t qe_idx = dev->reg.qe_sr_index;
	bk_err_t ret = BK_OK;
	uint32_t status_reg_s0_s7 = 0;
	uint32_t status_reg_s8_s15 = 0;
	uint32_t status_reg_read = 0;

	// To unlock all sectors: BP bits in SR1 must be cleared. The exact layout
	// (SRP0/SRP1 positions, preserve mask, BP field) comes from the descriptor.

	// Read current status registers
	status_reg_s0_s7 = bk_qspi_flash_read_s0_s7(id);
	status_reg_s8_s15 = (uint8_t)bk_qspi_flash_read_s8_s15(id);
	QSPI_LOGD("[%s] Before: S0-S7=0x%02X, S8-S15=0x%02X\r\n", __func__,
	          (uint8_t)status_reg_s0_s7, (uint8_t)status_reg_s8_s15);

	// Check SRP1 and SRP0 to determine if status register can be written
	uint8_t srp1 = (status_reg_s8_s15 & dev->reg.srp1_bit) ? 1 : 0;
	uint8_t srp0 = (status_reg_s0_s7 & dev->reg.srp0_bit) ? 1 : 0;
	QSPI_LOGD("[%s] SRP1=%d, SRP0=%d\r\n", __func__, srp1, srp0);

	if (srp1 == 1 && srp0 == 1) {
		QSPI_LOGE("[%s] Status register is hardware protected (SRP1=1, SRP0=1)!\r\n", __func__);
		return BK_FAIL;
	}

	// Preserve the QE bit (may live in a different SR than SR1).
	uint8_t qe_preserve = (uint8_t)nor_read_sr(id, qe_idx) & qe_bit;

	// Clear BP protection bits in SR1, keeping SRP0/WEL/WIP via the preserve mask.
	status_reg_s0_s7 &= dev->reg.sr1_preserve_mask;

	if ((status_reg_s0_s7 & dev->reg.srp0_bit) != 0) {
		QSPI_LOGW("[%s] Warning: SRP0 is already set (S0-S7=0x%02X). "
		          "Writing may cause hardware protection if SRP1 is also set.\r\n",
		          __func__, (uint8_t)status_reg_s0_s7);
		// Continue anyway since we're only clearing BP bits, not setting SRP0
	}

	QSPI_LOGD("[%s] Writing S0-S7=0x%02X (BP cleared, SRP0 preserved)\r\n", __func__, (uint8_t)status_reg_s0_s7);
	ret = bk_qspi_flash_write_s0_s7(id, status_reg_s0_s7);
	if (ret != BK_OK) {
		QSPI_LOGE("[%s] Write S0-S7 failed: %d\r\n", __func__, ret);
		return ret;
	}

	// Verify protection bits are cleared
	status_reg_read = bk_qspi_flash_read_s0_s7(id);
	QSPI_LOGD("[%s] After: S0-S7=0x%02X\r\n", __func__, (uint8_t)status_reg_read);

	uint8_t bp_bits = (status_reg_read >> dev->reg.bp_shift) & dev->reg.bp_mask;
	if (bp_bits != 0) {
		QSPI_LOGW("[%s] Protection bits not cleared! BP=0x%02X (S0-S7=0x%02X)\r\n",
		          __func__, bp_bits, (uint8_t)status_reg_read);
		return BK_FAIL;
	}

	/* Writing SR1 may clear QE on some parts; restore QE via its own SR. */
	if (qe_preserve) {
		uint8_t qe_sr = (uint8_t)nor_read_sr(id, qe_idx);
		if (!(qe_sr & qe_bit)) {
			QSPI_LOGD("[%s] Restoring QE bit after SR1 write (QE-SR[%d] was 0x%02X)\r\n",
			          __func__, qe_idx, qe_sr);
			ret = nor_write_sr(id, qe_idx, qe_sr | qe_bit);
			if (ret != BK_OK) {
				return ret;
			}
		}
	}

	QSPI_LOGD("[%s] All protection bits cleared successfully\r\n", __func__);
	return BK_OK;
}

uint32_t bk_qspi_flash_read_id(qspi_id_t id) {
	qspi_cmd_t read_id_cmd = {0};
	uint8_t read_id_buf[FLASH_READ_ID_BUF_SIZE] = {0};  // Buffer for 3 bytes + 1 padding
	uint32_t read_id_data = 0;

	read_id_cmd.device = QSPI_FLASH;
	read_id_cmd.data_wire_mode = QSPI_1WIRE;
	read_id_cmd.work_mode = INDIRECT_MODE;
	read_id_cmd.op = QSPI_READ;
	read_id_cmd.cmd = bk_qspi_nor_dev(id)->cmd.read_id;
	read_id_cmd.data_len = FLASH_READ_ID_SIZE;  // 3 bytes

	BK_LOG_ON_ERR(bk_qspi_command(id, &read_id_cmd));

	// Read 3 bytes from FIFO
	bk_qspi_read(id, read_id_buf, FLASH_READ_ID_SIZE);

	// Flash ID format: MID7-MID0, ID15-ID8, ID7-ID0
	// Pack into 32-bit value: 0x00MID_ID15-8_ID7-0
	read_id_data = ((uint32_t)read_id_buf[0] << 16) |  // MID in bits 23-16
	               ((uint32_t)read_id_buf[1] << 8)  |  // ID15-ID8 in bits 15-8
	               ((uint32_t)read_id_buf[2]);          // ID7-ID0 in bits 7-0

	QSPI_LOGD("Flash ID raw bytes: 0x%02X 0x%02X 0x%02X -> 0x%06X\r\n",
	          read_id_buf[0], read_id_buf[1], read_id_buf[2], read_id_data & 0xFFFFFF);

	return read_id_data;
}

/* Read JEDEC ID (9Fh) and select the first registered descriptor whose id
 * prefix matches. On no match the built-in default stays active. */
static bk_err_t nor_probe(qspi_id_t id)
{
	uint32_t raw = bk_qspi_flash_read_id(id);
	uint8_t id_buf[QSPI_NOR_ID_MAX_LEN];

	if (id >= QSPI_ID_MAX) {
		return BK_ERR_PARAM;
	}

	s_active_dev[id] = NULL;

	id_buf[0] = (uint8_t)(raw >> 16);   /* MID */
	id_buf[1] = (uint8_t)(raw >> 8);    /* ID15-8 */
	id_buf[2] = (uint8_t)(raw);         /* ID7-0 */

	for (qspi_nor_device_entry_t *e = &__qspi_nor_device_array_start;
	     e < &__qspi_nor_device_array_end; e++) {
		const qspi_nor_dev_t *dev = e->dev;

		if (!dev || !dev->id_len || dev->id_len > QSPI_NOR_ID_MAX_LEN) {
			continue;
		}
		if (os_memcmp(id_buf, dev->id, dev->id_len) != 0) {
			continue;
		}

		s_active_dev[id] = dev;
		QSPI_LOGI("nor %s detected, %uMB\r\n", dev->name,
		          dev->geometry.total_size / (1024U * 1024U));
		return BK_OK;
	}

	QSPI_LOGW("nor not identified (id=0x%06X), using default cfg, %uMB\r\n",
	          raw & 0xFFFFFF, bk_qspi_nor_dev(id)->geometry.total_size / (1024U * 1024U));
	return BK_ERR_NOT_FOUND;
}

bk_err_t bk_qspi_flash_init(qspi_id_t id)
{
	bk_err_t ret = BK_OK;

	/* QSPI max configurable SCK is 80MHz */
	ret = bk_qspi_init_by_freq(id, QSPI_FLASH_MAX_SCK_HZ);
	if (ret != BK_OK) {
		QSPI_LOGE("bk_qspi_init failed: %d\r\n", ret);
		return ret;
	}

	/* Identify the connected part by JEDEC ID; fall back to the default cfg. */
	if (nor_probe(id) != BK_OK) {
#if CONFIG_QSPI_NOR_ALLOW_UNKNOWN
		QSPI_LOGW("nor unidentified, continue with default cfg\r\n");
#else
		QSPI_LOGE("nor unidentified, init aborted (ALLOW_UNKNOWN=n)\r\n");
		return BK_ERR_NOT_FOUND;
#endif
	}

	/* Switch to 4-byte addressing before any address-phase command if the
	 * active descriptor needs it (capacity > 16MB). */
	const qspi_nor_dev_t *dev = bk_qspi_nor_dev(id);
	if (dev->cmd.addr_bytes == 4 && dev->cmd.enter_4byte) {
		bk_qspi_flash_enter_4byte(id);
	}

	// Try to set protect none, but don't fail if Flash is not connected
	ret = bk_qspi_flash_set_protect_none(id);
	if (ret != BK_OK) {
		QSPI_LOGW("bk_qspi_flash_set_protect_none failed: %d (Flash may not be connected)\r\n", ret);
		// Continue anyway, as this might be called during startup before Flash is ready
	}

	// Try to enable quad mode, but don't fail if Flash is not connected
	ret = bk_qspi_flash_quad_enable(id);
	if (ret != BK_OK) {
		QSPI_LOGW("bk_qspi_flash_quad_enable failed: %d (Flash may not be connected)\r\n", ret);
		// Continue anyway, as this might be called during startup before Flash is ready
	}

	return BK_OK;
}

bk_err_t bk_qspi_flash_deinit(qspi_id_t id)
{
	BK_LOG_ON_ERR(bk_qspi_deinit(id));
	if (id < QSPI_ID_MAX) {
		s_active_dev[id] = NULL;
	}
	return BK_OK;
}

void qspi_flash_test_case(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
	uint32_t read_id = 0;
	uint32_t *read_data = (uint32_t *)os_zalloc(size);
	uint32_t *origin_data = (uint32_t *)data;
	if (read_data == NULL) {
		QSPI_LOGE("send buffer malloc failed\r\n");
		return;
	}

	read_id = bk_qspi_flash_read_id(id);
	QSPI_LOGD("%s read_id = 0x%x\n", __func__, read_id);

	bk_qspi_flash_set_protect_none(id);
	bk_qspi_flash_quad_enable(id);

	/* quad write, then quad/single read to check data*/
	bk_qspi_flash_erase_sector(id, base_addr);
	bk_qspi_flash_read(id, base_addr, read_data, size);

	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != 0xFFFFFFFF) {
			QSPI_LOGD("[ERASE ERROR]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
		}
		QSPI_LOGV("[ERASE DBG]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
	}

	bk_qspi_flash_write(id, base_addr, data, size);
	bk_qspi_flash_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != origin_data[i]) {
			QSPI_LOGD("[QUAD WRITE - QUAD READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
		}
		QSPI_LOGV("[QUAD WRITE - QUAD READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
	}

	bk_qspi_flash_single_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != origin_data[i]) {
			QSPI_LOGD("[QUAD WRITE - SINGLE READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
		}
		QSPI_LOGV("[QUAD WRITE - SINGLE READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
	}

	/* singel write, then single/quad read to check data*/
	bk_qspi_flash_erase_sector(id, base_addr);
	bk_qspi_flash_single_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != 0xFFFFFFFF) {
			QSPI_LOGD("[ERASE ERROR]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
		}
		QSPI_LOGV("[ERASE DBG]: read_data[%d]=0x%x, should be 0xFFFFFFFF\n", i, read_data[i]);
	}

	bk_qspi_flash_single_page_program(id, base_addr, data, size);
	bk_qspi_flash_single_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != origin_data[i]) {
			QSPI_LOGD("[SINGLE WRITE - SINGLE READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
		}
		QSPI_LOGV("[SINGLE WRITE - SINGLE READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
	}

	bk_qspi_flash_read(id, base_addr, read_data, size);
	for (int i = 0; i < size/4; i++) {
		if(read_data[i] != origin_data[i]) {
			QSPI_LOGD("[SINGLE WRITE - QUAD READ ERROR]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
		}
		QSPI_LOGV("[SINGLE WRITE - QUAD READ DBG]: read_data[%d]=0x%x, origin data[%d]=0x%x\n", i, read_data[i], i, origin_data[i]);
	}

	if (read_data) {
		os_free(read_data);
		read_data = NULL;
	}
}

void test_qspi_flash(qspi_id_t id, uint32_t base_addr, uint32_t buf_len)
{
	uint32_t *send_data = (uint32_t *)os_zalloc(buf_len);
//	uint32_t rand_val = bk_rand() % (0x100000000);

	if (send_data == NULL) {
		QSPI_LOGE("send buffer malloc failed\r\n");
		return;
	}
	for (int i = 0; i < (buf_len/4); i++) {
		send_data[i] = (0x03020100 + i*0x04040404) & 0xffffffff;
	}
	qspi_flash_test_case(id, base_addr, send_data, buf_len);

	if (send_data) {
		os_free(send_data);
		send_data = NULL;
	}
}
