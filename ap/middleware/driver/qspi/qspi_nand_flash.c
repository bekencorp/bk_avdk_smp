// Copyright 2024-2025 Beken
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

#include <soc/soc.h>
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include "qspi_hal.h"
#include <driver/int.h>
#include <os/mem.h>
#include "qspi_driver.h"
#include "qspi_statis.h"
#include "qspi_nand_flash.h"
#include <driver/aon_rtc.h>
#include "bk_misc.h"

#if CONFIG_QSPI_NAND_FLASH

#define QSPI_POLL_FAST_US          50
#define QSPI_POLL_FAST_PHASE_US    1000

/* CONFIG_QSPI_LINE_MODE is the single source of truth for the flash wire mode.
 * bk_qspi_flash_write/read dispatch through these aliases; NAND has no dual
 * mode, so anything other than 4-wire falls back to single-wire.
 * For QSPI NAND flash, CONFIG_QSPI_LINE_MODE must be at least 2 (not 1-wire). */
#if (CONFIG_QSPI_LINE_MODE == 4)
#define bk_qspi_flash_line_read     bk_qspi_flash_quad_read
#define bk_qspi_flash_line_program  bk_qspi_flash_quad_page_program
#else
#define bk_qspi_flash_line_read     bk_qspi_flash_single_read
#define bk_qspi_flash_line_program  bk_qspi_flash_single_page_program
#endif

static bk_err_t nand_wait_ready_internal(qspi_id_t id);

static inline bk_err_t nand_check_id(qspi_id_t id)
{
	return (id < QSPI_ID_MAX) ? BK_OK : BK_ERR_PARAM;
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
	bk_err_t ret = nand_wait_ready_internal(id);
	if (ret != BK_OK) {
		QSPI_LOGW("%s: wait ready timeout(%d)\n", __func__, ret);
	}
}

static bk_err_t nand_feature_get_internal(qspi_id_t id, uint8_t addr, uint8_t *value);
static bk_err_t nand_reset_internal(qspi_id_t id);

static bk_err_t nand_reset_internal(qspi_id_t id)
{
	qspi_cmd_t cmd = {0};

	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_RESET;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	/* Spec tRST: wait until device ready after Reset (FFh) */
	rtos_delay_milliseconds(1);
	return BK_OK;
}

bk_err_t bk_qspi_flash_init(qspi_id_t id)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	/* QSPI max configurable SCK is 80MHz */
	BK_LOG_ON_ERR(bk_qspi_init_by_freq(id, QSPI_FLASH_MAX_SCK_HZ));

	BK_LOG_ON_ERR(nand_reset_internal(id));

	bk_qspi_flash_set_protect_none(id);

	bk_qspi_flash_quad_enable(id);

	return BK_OK;
}

bk_err_t bk_qspi_flash_deinit(qspi_id_t id)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	BK_LOG_ON_ERR(bk_qspi_deinit(id));
	return BK_OK;
}

static bk_err_t nand_feature_get_internal(qspi_id_t id, uint8_t addr, uint8_t *value)
{
	qspi_cmd_t cmd = {0};
	uint32_t temp_val = 0;

	if (id >= QSPI_ID_MAX) {
		return BK_ERR_PARAM;
	}
	BK_RETURN_ON_NULL(value);

	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_READ;
	cmd.cmd = NAND_CMD_GET_FEATURE;
	cmd.data_len = NAND_FEATURE_DATA_LEN;
	cmd.addr = addr;
	cmd.addr_len = NAND_ADDR_LEN_FEATURE;
	cmd.addr_wire_mode = QSPI_1WIRE;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	BK_RETURN_ON_ERR(bk_qspi_read(id, &temp_val, NAND_FEATURE_DATA_LEN));
	*value = (uint8_t)temp_val;
	return BK_OK;
}

static bk_err_t nand_wait_ready_with_status_ms(qspi_id_t id, uint32_t timeout_ms, uint8_t *out_status)
{
	uint8_t status = 0;
	uint64_t start = bk_aon_rtc_get_us();
	const uint64_t timeout_us = (uint64_t)timeout_ms * 1000;

	while (bk_aon_rtc_get_us() - start <= timeout_us) {
		BK_RETURN_ON_ERR(nand_feature_get_internal(id, NAND_FEATURE_ADDR_STATUS, &status));
		if (!(status & NAND_STATUS_OIP)) {
			if (out_status) {
				*out_status = status;
			}
			return BK_OK;
		}
		qspi_poll_backoff(start);
	}

	if (out_status) {
		*out_status = status;
	}
	return BK_ERR_TIMEOUT;
}

static bk_err_t nand_wait_ready_with_status(qspi_id_t id, uint8_t *out_status)
{
	return nand_wait_ready_with_status_ms(id, NAND_DEFAULT_TIMEOUT_MS, out_status);
}

static bk_err_t nand_wait_ready_internal(qspi_id_t id)
{
	return nand_wait_ready_with_status(id, NULL);
}

static bk_err_t nand_write_enable_internal(qspi_id_t id)
{
	qspi_cmd_t cmd = {0};
	uint8_t status = 0;

	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_WRITE_ENABLE;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	BK_RETURN_ON_ERR(nand_feature_get_internal(id, NAND_FEATURE_ADDR_STATUS, &status));
	return (status & NAND_STATUS_WEL) ? BK_OK : BK_ERR_STATE;
}

static bk_err_t nand_write_enable_bare(qspi_id_t id)
{
	qspi_cmd_t cmd = {0};
	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_WRITE_ENABLE;
	return bk_qspi_command(id, &cmd);
}

static bk_err_t nand_feature_set_internal(qspi_id_t id, uint8_t addr, uint8_t value)
{
	BK_RETURN_ON_ERR(nand_write_enable_internal(id));

	qspi_cmd_t cmd = {0};

	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	/* SET FEATURE 1Fh: opcode + 1-byte feature address + 1-byte value.
	 * The value byte is clocked out through the FIFO data phase. */
	cmd.cmd = NAND_CMD_SET_FEATURE;
	cmd.addr = addr;
	cmd.addr_len = NAND_ADDR_LEN_FEATURE;
	cmd.addr_wire_mode = QSPI_1WIRE;
	cmd.data_len = NAND_FEATURE_DATA_LEN;

	uint32_t wdata = value;
	BK_RETURN_ON_ERR(bk_qspi_write(id, &wdata, NAND_FEATURE_DATA_LEN));
	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	return nand_wait_ready_internal(id);
}


static bk_err_t nand_block_erase_internal(qspi_id_t id, uint32_t block)
{
	qspi_cmd_t cmd = {0};
	uint32_t total_blocks = NAND_DEVICE_TOTAL_SIZE / NAND_BLOCK_SIZE_BYTES;

	if (block >= total_blocks) {
		QSPI_LOGE("Invalid block number: %u (max: %u)\r\n", block, total_blocks - 1);
		return BK_ERR_PARAM;
	}

	BK_RETURN_ON_ERR(nand_write_enable_internal(id));

	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_BLOCK_ERASE;
	cmd.addr = block * NAND_BLOCK_PAGE_COUNT;
	cmd.addr_len = NAND_ADDR_LEN_ROW;
	cmd.addr_wire_mode = QSPI_1WIRE;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));

	uint8_t status = 0;
	BK_RETURN_ON_ERR(nand_wait_ready_with_status_ms(id, NAND_ERASE_TIMEOUT_MS, &status));
	if (status & NAND_STATUS_E_FAIL) {
		QSPI_LOGE("block %u erase E-FAIL (status=0x%02x)\r\n", block, status);
		return BK_FAIL;
	}
	return BK_OK;
}

/* 13H: load flash page into on-die data buffer (D8H erase does not clear buffer). */
static bk_err_t nand_page_data_read_to_buffer(qspi_id_t id, uint32_t page)
{
	qspi_cmd_t cmd = {0};

	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_PAGE_READ;
	cmd.addr = page;
	cmd.addr_len = NAND_ADDR_LEN_ROW;
	cmd.addr_wire_mode = QSPI_1WIRE;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	return nand_wait_ready_internal(id);
}

/*
 * Spec 3.4: first 02H/32H load resets unused buffer bytes to FFh — full-page
 * program from column 0 does not need 13H after erase.
 * Partial-page update (column > 0) must 13H first to load existing flash data (RMW).
 */
static bk_err_t nand_page_program_prepare_buffer(qspi_id_t id, uint32_t page, uint32_t column)
{
	if (column > 0) {
		return nand_page_data_read_to_buffer(id, page);
	}
	return BK_OK;
}

static bk_err_t nand_program_load_internal(qspi_id_t id, uint32_t column, const uint8_t *buf, uint32_t len)
{
	bool first = true;
	uint32_t temp_buf[QSPI_FIFO_LEN_MAX / 4];

	while (len) {
		uint32_t chunk = len > QSPI_FIFO_LEN_MAX ? QSPI_FIFO_LEN_MAX : len;
		qspi_cmd_t cmd = {0};

		if (!first) {
			BK_RETURN_ON_ERR(nand_write_enable_internal(id));
		}

		os_memcpy(temp_buf, buf, chunk);

		cmd.device = QSPI_FLASH;
		cmd.data_wire_mode = QSPI_1WIRE;
		cmd.work_mode = INDIRECT_MODE;
		cmd.op = QSPI_WRITE;
		cmd.cmd = first ? NAND_CMD_PROGRAM_LOAD : NAND_CMD_PROGRAM_LOAD_RANDOM;
		cmd.addr = column;
		cmd.addr_len = NAND_ADDR_LEN_COLUMN;
		cmd.addr_wire_mode = QSPI_1WIRE;
		cmd.dummy_cycle = 0;
		cmd.data_len = chunk;

		BK_RETURN_ON_ERR(bk_qspi_write(id, temp_buf, chunk));
		BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));

		buf += chunk;
		column += chunk;
		len -= chunk;
		first = false;
	}

	return BK_OK;
}

static bk_err_t nand_page_program_internal(qspi_id_t id, uint32_t page, uint32_t column, const uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_NULL(buf);

	if (len == 0) {
		QSPI_LOGE("nand_page_program_internal: len=0\r\n");
		return BK_ERR_PARAM;
	}
	if (column >= NAND_PAGE_SIZE_BYTES) {
		QSPI_LOGE("nand_page_program_internal: column(%u) >= page_size(%u)\r\n", column, NAND_PAGE_SIZE_BYTES);
		return BK_ERR_PARAM;
	}
	if ((column + len) > NAND_PAGE_SIZE_BYTES) {
		QSPI_LOGE("nand_page_program_internal: column(%u) + len(%u) = %u > page_size(%u)\r\n",
		          column, len, column + len, NAND_PAGE_SIZE_BYTES);
		return BK_ERR_PARAM;
	}

	BK_RETURN_ON_ERR(nand_page_program_prepare_buffer(id, page, column));
	BK_RETURN_ON_ERR(nand_write_enable_internal(id));
	BK_RETURN_ON_ERR(nand_program_load_internal(id, column, buf, len));

	qspi_cmd_t cmd = {0};
	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_PROGRAM_EXECUTE;
	cmd.addr = page;
	cmd.addr_len = NAND_ADDR_LEN_ROW;
	cmd.addr_wire_mode = QSPI_1WIRE;
	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));

	uint8_t status = 0;
	BK_RETURN_ON_ERR(nand_wait_ready_with_status(id, &status));
	if (status & NAND_STATUS_P_FAIL) {
		QSPI_LOGE("page %u program P-FAIL (status=0x%02x)\r\n", page, status);
		return BK_FAIL;
	}

	return BK_OK;
}

static bk_err_t nand_page_read_internal(qspi_id_t id, uint32_t page, uint32_t column, uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_NULL(buf);
	if (!len || (column + len) > NAND_PAGE_SIZE_BYTES) {
		return BK_ERR_PARAM;
	}

	qspi_cmd_t cmd = {0};

	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_PAGE_READ;
	cmd.addr = page;
	cmd.addr_len = NAND_ADDR_LEN_ROW;
	cmd.addr_wire_mode = QSPI_1WIRE;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	BK_RETURN_ON_ERR(nand_wait_ready_internal(id));

	uint32_t current_column = column;
	uint32_t remaining = len;

	while (remaining) {
		uint32_t chunk = remaining > QSPI_FIFO_LEN_MAX ? QSPI_FIFO_LEN_MAX : remaining;

		cmd.device = QSPI_FLASH;
		cmd.data_wire_mode = QSPI_1WIRE;
		cmd.work_mode = INDIRECT_MODE;
		cmd.op = QSPI_READ;
		cmd.cmd = NAND_CMD_READ_FROM_CACHE;
		cmd.addr = current_column;
		cmd.addr_len = NAND_ADDR_LEN_COLUMN;
		cmd.addr_wire_mode = QSPI_1WIRE;
		cmd.dummy_cycle = NAND_READ_DUMMY_CYCLE;
		cmd.dummy_mode = NAND_READ_DUMMY_MODE;
		cmd.data_len = chunk;

		BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
		BK_RETURN_ON_ERR(bk_qspi_read(id, buf, chunk));

		buf += chunk;
		current_column += chunk;
		remaining -= chunk;
	}

	return BK_OK;
}

static bk_err_t nand_program_load_quad_internal(qspi_id_t id, uint32_t column, const uint8_t *buf, uint32_t len)
{
	bool first = true;
	uint32_t temp_buf[QSPI_FIFO_LEN_MAX / 4];

	while (len) {
		uint32_t chunk = len > QSPI_FIFO_LEN_MAX ? QSPI_FIFO_LEN_MAX : len;
		qspi_cmd_t cmd = {0};

		if (!first) {
			BK_RETURN_ON_ERR(nand_write_enable_internal(id));
		}

		os_memcpy(temp_buf, buf, chunk);

		cmd.device = QSPI_FLASH;
		cmd.work_mode = INDIRECT_MODE;
		cmd.op = QSPI_WRITE;
		cmd.data_wire_mode = QSPI_4WIRE;
		cmd.cmd = first ? NAND_CMD_PRORAM_LOAD_QUAD : NAND_CMD_PRORAM_LOAD_RANDOM_QUAD;
		cmd.addr = column;
		cmd.addr_len = NAND_ADDR_LEN_COLUMN;
		cmd.addr_wire_mode = QSPI_1WIRE;
		cmd.data_len = chunk;

		BK_RETURN_ON_ERR(bk_qspi_write(id, temp_buf, chunk));
		BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));

		buf += chunk;
		column += chunk;
		len -= chunk;
		first = false;
	}

	return BK_OK;
}

static bk_err_t nand_page_program_quad_internal(qspi_id_t id, uint32_t page, uint32_t column, const uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_NULL(buf);

	if (len == 0) {
		return BK_ERR_PARAM;
	}
	if (column >= NAND_PAGE_SIZE_BYTES) {
		return BK_ERR_PARAM;
	}
	if ((column + len) > NAND_PAGE_SIZE_BYTES) {
		return BK_ERR_PARAM;
	}

	BK_RETURN_ON_ERR(nand_page_program_prepare_buffer(id, page, column));
	BK_RETURN_ON_ERR(nand_write_enable_internal(id));
	BK_RETURN_ON_ERR(nand_program_load_quad_internal(id, column, buf, len));

	qspi_cmd_t cmd = {0};
	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_PROGRAM_EXECUTE;
	cmd.addr = page;
	cmd.addr_len = NAND_ADDR_LEN_ROW;
	cmd.addr_wire_mode = QSPI_1WIRE;
	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));

	uint8_t status = 0;
	BK_RETURN_ON_ERR(nand_wait_ready_with_status(id, &status));
	if (status & NAND_STATUS_P_FAIL) {
		QSPI_LOGE("page %u program quad P-FAIL (status=0x%02x)\r\n", page, status);
		return BK_FAIL;
	}

	return BK_OK;
}

static bk_err_t nand_page_read_quad_internal(qspi_id_t id, uint32_t page, uint32_t column, uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_NULL(buf);
	if (!len || (column + len) > NAND_PAGE_SIZE_BYTES) {
		return BK_ERR_PARAM;
	}

	qspi_cmd_t cmd = {0};

	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_PAGE_READ;
	cmd.addr = page;
	cmd.addr_len = NAND_ADDR_LEN_ROW;
	cmd.addr_wire_mode = QSPI_1WIRE;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	BK_RETURN_ON_ERR(nand_wait_ready_internal(id));

	uint32_t current_column = column;
	uint32_t remaining = len;

	while (remaining) {
		uint32_t chunk = remaining > QSPI_FIFO_LEN_MAX ? QSPI_FIFO_LEN_MAX : remaining;

		cmd.device = QSPI_FLASH;
		cmd.data_wire_mode = QSPI_4WIRE;
		cmd.work_mode = INDIRECT_MODE;
		cmd.op = QSPI_READ;
		cmd.cmd = NAND_CMD_READ_FROM_CACHE_X4;
		cmd.addr = current_column;
		cmd.addr_len = NAND_ADDR_LEN_COLUMN;
		cmd.addr_wire_mode = QSPI_1WIRE;
		cmd.dummy_cycle = NAND_READ_DUMMY_CYCLE;
		cmd.dummy_mode = NAND_READ_DUMMY_MODE;
		cmd.data_len = chunk;

		BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
		BK_RETURN_ON_ERR(bk_qspi_read(id, buf, chunk));

		buf += chunk;
		current_column += chunk;
		remaining -= chunk;
	}

	return BK_OK;
}

static bk_err_t nand_read_id_internal(qspi_id_t id, uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_NULL(buf);
	if (!len || len > FLASH_READ_ID_SIZE) {
		return BK_ERR_PARAM;
	}

	qspi_cmd_t cmd = {0};
	cmd.device = QSPI_FLASH;
	cmd.data_wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_READ;
	cmd.cmd = FLASH_READ_ID_CMD;
	cmd.addr = 0;
	cmd.addr_len = NAND_ADDR_LEN_FEATURE;
	cmd.addr_wire_mode = QSPI_1WIRE;
	cmd.dummy_cycle = NAND_READ_DUMMY_CYCLE;
	cmd.dummy_mode = NAND_READ_DUMMY_MODE;
	cmd.data_len = len;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	return bk_qspi_read(id, buf, len);
}


bk_err_t bk_qspi_flash_single_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
	const uint8_t *buf8 = (const uint8_t *)data;
	BK_RETURN_ON_ERR(nand_check_id(id));
	BK_RETURN_ON_NULL(buf8);

	uint32_t remaining = size;
	uint32_t current_addr = addr;

	while (remaining) {
		uint32_t page = current_addr / NAND_PAGE_SIZE_BYTES;
		uint32_t column = current_addr % NAND_PAGE_SIZE_BYTES;
		uint32_t chunk = NAND_PAGE_SIZE_BYTES - column;
		if (chunk > remaining) {
			chunk = remaining;
		}

		BK_RETURN_ON_ERR(nand_page_program_internal(id, page, column, buf8, chunk));

		buf8 += chunk;
		current_addr += chunk;
		remaining -= chunk;
	}

	return BK_OK;
}


bk_err_t bk_qspi_flash_single_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
	uint8_t *buf8 = (uint8_t *)data;
	BK_RETURN_ON_ERR(nand_check_id(id));
	BK_RETURN_ON_NULL(buf8);

	uint32_t remaining = size;
	uint32_t current_addr = addr;

	while (remaining) {
		uint32_t page = current_addr / NAND_PAGE_SIZE_BYTES;
		uint32_t column = current_addr % NAND_PAGE_SIZE_BYTES;
		uint32_t chunk = NAND_PAGE_SIZE_BYTES - column;
		if (chunk > remaining) {
			chunk = remaining;
		}

		BK_RETURN_ON_ERR(nand_page_read_internal(id, page, column, buf8, chunk));

		buf8 += chunk;
		current_addr += chunk;
		remaining -= chunk;
	}

	return BK_OK;
}

bk_err_t bk_qspi_flash_quad_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
	const uint8_t *buf8 = (const uint8_t *)data;
	BK_RETURN_ON_ERR(nand_check_id(id));
	BK_RETURN_ON_NULL(buf8);

	uint32_t remaining = size;
	uint32_t current_addr = addr;

	while (remaining) {
		uint32_t page = current_addr / NAND_PAGE_SIZE_BYTES;
		uint32_t column = current_addr % NAND_PAGE_SIZE_BYTES;
		uint32_t chunk = NAND_PAGE_SIZE_BYTES - column;
		if (chunk > remaining) {
			chunk = remaining;
		}

		BK_RETURN_ON_ERR(nand_page_program_quad_internal(id, page, column, buf8, chunk));

		buf8 += chunk;
		current_addr += chunk;
		remaining -= chunk;
	}

	return BK_OK;
}

bk_err_t bk_qspi_flash_quad_read(qspi_id_t id, uint32_t addr, void *data, uint32_t size)
{
	uint8_t *buf8 = (uint8_t *)data;
	BK_RETURN_ON_ERR(nand_check_id(id));
	BK_RETURN_ON_NULL(buf8);

	uint32_t remaining = size;
	uint32_t current_addr = addr;

	while (remaining) {
		uint32_t page = current_addr / NAND_PAGE_SIZE_BYTES;
		uint32_t column = current_addr % NAND_PAGE_SIZE_BYTES;
		uint32_t chunk = NAND_PAGE_SIZE_BYTES - column;
		if (chunk > remaining) {
			chunk = remaining;
		}

		BK_RETURN_ON_ERR(nand_page_read_quad_internal(id, page, column, buf8, chunk));

		buf8 += chunk;
		current_addr += chunk;
		remaining -= chunk;
	}

	return BK_OK;
}


bk_err_t bk_qspi_flash_write(qspi_id_t id, uint32_t base_addr, const void *data, uint32_t size)
{
	BK_RETURN_ON_ERR(nand_check_id(id));
	BK_RETURN_ON_NULL(data);
	if (size == 0) {
		return BK_OK;
	}

	/* single/quad_page_program already handle byte-granular start column, page
	 * crossing, the QSPI 256-byte FIFO split and the column>0 read-modify-write
	 * (0x13 page load) internally, so just delegate. */
	return bk_qspi_flash_line_program(id, base_addr, data, size);
}


bk_err_t bk_qspi_flash_read(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
	BK_RETURN_ON_ERR(nand_check_id(id));
	BK_RETURN_ON_NULL(data);
	if (size == 0) {
		return BK_OK;
	}

	/* single/quad_read already handle byte-granular start column, page crossing
	 * and the QSPI 256-byte FIFO split internally, so just delegate. */
	return bk_qspi_flash_line_read(id, base_addr, data, size);
}


bk_err_t bk_qspi_flash_nand_get_feature(qspi_id_t id, uint8_t addr, uint8_t *value)
{
	BK_RETURN_ON_ERR(nand_check_id(id));
	BK_RETURN_ON_NULL(value);

	return nand_feature_get_internal(id, addr, value);
}

bk_err_t bk_qspi_flash_nand_get_id(qspi_id_t id, uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	return nand_read_id_internal(id, buf, len);
}

bk_err_t bk_qspi_flash_nand_set_feature(qspi_id_t id, uint8_t addr, uint8_t value)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	return nand_feature_set_internal(id, addr, value);
}

bk_err_t bk_qspi_flash_nand_get_block_lock(qspi_id_t id, uint8_t *value)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	return bk_qspi_flash_nand_get_feature(id, NAND_FEATURE_ADDR_BLOCK_LOCK, value);
}

bk_err_t bk_qspi_flash_nand_set_block_lock(qspi_id_t id, uint8_t value)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	return bk_qspi_flash_nand_set_feature(id, NAND_FEATURE_ADDR_BLOCK_LOCK, value);
}

bk_err_t bk_qspi_flash_nand_get_status(qspi_id_t id, uint8_t *value)
{
	BK_RETURN_ON_ERR(nand_check_id(id));
	BK_RETURN_ON_NULL(value);

	return bk_qspi_flash_nand_get_feature(id, NAND_FEATURE_ADDR_STATUS, value);
}

bk_err_t bk_qspi_flash_nand_get_feature_register(qspi_id_t id, uint8_t *value)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	return bk_qspi_flash_nand_get_feature(id, NAND_FEATURE_ADDR_DRIVE, value);
}

bk_err_t bk_qspi_flash_nand_set_feature_register(qspi_id_t id, uint8_t value)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	return bk_qspi_flash_nand_set_feature(id, NAND_FEATURE_ADDR_DRIVE, value);
}

bk_err_t bk_qspi_flash_nand_set_protect_none(qspi_id_t id)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	uint8_t lock = 0;
	BK_RETURN_ON_ERR(bk_qspi_flash_nand_set_block_lock(id, NAND_BLOCK_LOCK_NONE));
	BK_RETURN_ON_ERR(bk_qspi_flash_nand_get_block_lock(id, &lock));
	if (lock != NAND_BLOCK_LOCK_NONE) {
		return BK_ERR_STATE;
	}
	return BK_OK;
}

bk_err_t bk_qspi_flash_nand_block_erase(qspi_id_t id, uint32_t block)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	return nand_block_erase_internal(id, block);
}

bk_err_t bk_qspi_flash_erase(qspi_id_t id, uint32_t addr, uint32_t size)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	uint32_t block_start = addr / NAND_BLOCK_SIZE_BYTES;
	uint32_t block_end = (addr + size - 1) / NAND_BLOCK_SIZE_BYTES;

	for (uint32_t i = block_start; i <= block_end; i++) {
		BK_RETURN_ON_ERR(bk_qspi_flash_nand_block_erase(id, i));
	}
	return BK_OK;
}

bk_err_t bk_qspi_flash_nand_page_program(qspi_id_t id, uint32_t page, uint32_t column, const uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	if (buf == NULL) {
		return BK_ERR_NULL_PARAM;
	}
	if (len == 0) {
		return BK_ERR_PARAM;
	}
	if (column >= NAND_PAGE_SIZE_BYTES) {
		return BK_ERR_PARAM;
	}
	if (len > (NAND_PAGE_SIZE_BYTES - column)) {
		QSPI_LOGE("bk_qspi_flash_nand_page_program: column(%u) + len(%u) > page_size(%u)\r\n",
		          column, len, NAND_PAGE_SIZE_BYTES);
		return BK_ERR_PARAM;
	}

	return nand_page_program_internal(id, page, column, buf, len);
}

bk_err_t bk_qspi_flash_nand_page_read(qspi_id_t id, uint32_t page, uint32_t column, uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	if (buf == NULL) {
		return BK_ERR_NULL_PARAM;
	}
	if (len == 0) {
		return BK_ERR_PARAM;
	}
	if (column >= NAND_PAGE_SIZE_BYTES) {
		return BK_ERR_PARAM;
	}
	if (len > (NAND_PAGE_SIZE_BYTES - column)) {
		QSPI_LOGE("bk_qspi_flash_nand_page_read: column(%u) + len(%u) > page_size(%u)\r\n",
		          column, len, NAND_PAGE_SIZE_BYTES);
		return BK_ERR_PARAM;
	}

	return nand_page_read_internal(id, page, column, buf, len);
}

bk_err_t bk_qspi_flash_nand_page_program_quad(qspi_id_t id, uint32_t page, uint32_t column, const uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	if (buf == NULL) {
		return BK_ERR_NULL_PARAM;
	}
	if (len == 0) {
		return BK_ERR_PARAM;
	}
	if (column >= NAND_PAGE_SIZE_BYTES) {
		return BK_ERR_PARAM;
	}
	if (len > (NAND_PAGE_SIZE_BYTES - column)) {
		QSPI_LOGE("bk_qspi_flash_nand_page_program_quad: column(%u) + len(%u) > page_size(%u)\r\n",
		          column, len, NAND_PAGE_SIZE_BYTES);
		return BK_ERR_PARAM;
	}

	return nand_page_program_quad_internal(id, page, column, buf, len);
}

bk_err_t bk_qspi_flash_nand_page_read_quad(qspi_id_t id, uint32_t page, uint32_t column, uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	if (buf == NULL) {
		return BK_ERR_NULL_PARAM;
	}
	if (len == 0) {
		return BK_ERR_PARAM;
	}
	if (column >= NAND_PAGE_SIZE_BYTES) {
		return BK_ERR_PARAM;
	}
	if (len > (NAND_PAGE_SIZE_BYTES - column)) {
		QSPI_LOGE("bk_qspi_flash_nand_page_read_quad: column(%u) + len(%u) > page_size(%u)\r\n",
		          column, len, NAND_PAGE_SIZE_BYTES);
		return BK_ERR_PARAM;
	}

	return nand_page_read_quad_internal(id, page, column, buf, len);
}

bk_err_t bk_qspi_flash_quad_enable(qspi_id_t id)
{
	uint8_t prot = 0;
	uint8_t cfg = 0;

	BK_RETURN_ON_ERR(nand_check_id(id));

	BK_RETURN_ON_ERR(bk_qspi_flash_nand_get_block_lock(id, &prot));

	if (prot & NAND_PROT_WP_E_BIT) {
		BK_RETURN_ON_ERR(bk_qspi_flash_nand_set_block_lock(id, prot & ~NAND_PROT_WP_E_BIT));
	}

	BK_RETURN_ON_ERR(bk_qspi_flash_nand_get_block_lock(id, &prot));
	if (prot & NAND_PROT_WP_E_BIT) {
		QSPI_LOGE("quad_enable: WP-E still set (A0h=0x%02x)\r\n", prot);
		return BK_ERR_STATE;
	}

	BK_RETURN_ON_ERR(nand_feature_get_internal(id, NAND_FEATURE_ADDR_DRIVE, &cfg));

	if (!(cfg & NAND_CFG_QE_BIT)) {
		BK_RETURN_ON_ERR(nand_feature_set_internal(id, NAND_FEATURE_ADDR_DRIVE, cfg | NAND_CFG_QE_BIT));
	}

	BK_RETURN_ON_ERR(nand_feature_get_internal(id, NAND_FEATURE_ADDR_DRIVE, &cfg));
	if (!(cfg & NAND_CFG_QE_BIT)) {
		QSPI_LOGE("quad_enable: QE still clear (B0h=0x%02x)\r\n", cfg);
		return BK_ERR_STATE;
	}

	QSPI_LOGI("quad_enable: WP-E=0, QE=1 (B0h=0x%02x), quad IO ready\r\n", cfg);
	return BK_OK;
}

bk_err_t bk_qspi_flash_set_protect_none(qspi_id_t id)
{
	BK_RETURN_ON_ERR(nand_check_id(id));

	return bk_qspi_flash_nand_set_protect_none(id);
}

uint32_t bk_qspi_flash_read_id(qspi_id_t id)
{
	uint8_t id_buf[FLASH_READ_ID_SIZE] = {0};

	if (nand_check_id(id) != BK_OK) {
		return 0;
	}

	if (nand_read_id_internal(id, id_buf, sizeof(id_buf)) != BK_OK)
	{
		return 0;
	}

	return id_buf[0] | ((uint32_t)id_buf[1] << 8);
}


void qspi_flash_test_case(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
	uint32_t page_addr = (base_addr / NAND_PAGE_SIZE_BYTES) * NAND_PAGE_SIZE_BYTES;
	uint8_t *buf = (uint8_t *)os_zalloc(NAND_PAGE_SIZE_BYTES);
	uint8_t *verify = (uint8_t *)os_zalloc(NAND_PAGE_SIZE_BYTES);
	uint8_t id_buf[FLASH_READ_ID_SIZE] = {0};

	if (!buf || !verify) {
		QSPI_LOGE("qspi flash test: buffer alloc failed\r\n");
		goto exit;
	}

	if (bk_qspi_flash_nand_get_id(id, id_buf, sizeof(id_buf)) == BK_OK) {
		QSPI_LOGI("%s zb35 id = %02x %02x (expect %02x %02x)\n", __func__,
		          id_buf[0], id_buf[1],
		          NAND_JEDEC_MFG_ID_ZBIT, NAND_JEDEC_DEV_ID_ZB35Q01);
	}

	BK_LOG_ON_ERR(bk_qspi_flash_nand_set_protect_none(id));
	BK_LOG_ON_ERR(bk_qspi_flash_nand_block_erase(id, base_addr / NAND_BLOCK_SIZE_BYTES));

	BK_LOG_ON_ERR(bk_qspi_flash_single_read(id, page_addr, verify, NAND_PAGE_SIZE_BYTES));
	for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
		if (verify[i] != 0xFF) {
			QSPI_LOGI("[ZB35 TEST] erase mismatch idx:%u val:%02x\r\n", i, verify[i]);
			break;
		}
	}

	os_memset(buf, 0xAA, NAND_PAGE_SIZE_BYTES);
	BK_LOG_ON_ERR(bk_qspi_flash_single_page_program(id, page_addr, buf, NAND_PAGE_SIZE_BYTES));
	BK_LOG_ON_ERR(bk_qspi_flash_single_read(id, page_addr, verify, NAND_PAGE_SIZE_BYTES));
	for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
		if (verify[i] != 0xAA) {
			QSPI_LOGI("[ZB35 TEST] 0xAA mismatch idx:%u val:%02x\r\n", i, verify[i]);
			break;
		}
	}

	os_memset(buf, 0x55, NAND_PAGE_SIZE_BYTES);
	BK_LOG_ON_ERR(bk_qspi_flash_single_page_program(id, page_addr, buf, NAND_PAGE_SIZE_BYTES));
	BK_LOG_ON_ERR(bk_qspi_flash_single_read(id, page_addr, verify, NAND_PAGE_SIZE_BYTES));
	for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
		if (verify[i] != 0x55) {
			QSPI_LOGI("[ZB35 TEST] 0x55 mismatch idx:%u val:%02x\r\n", i, verify[i]);
			break;
		}
	}

exit:
	if (buf) {
		os_free(buf);
	}
	if (verify) {
		os_free(verify);
	}
}

void test_qspi_flash(qspi_id_t id, uint32_t base_addr, uint32_t buf_len)
{
	uint32_t *send_data = (uint32_t *)os_zalloc(buf_len);

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

#endif
