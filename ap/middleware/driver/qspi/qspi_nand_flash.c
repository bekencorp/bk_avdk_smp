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

#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include "qspi_hal.h"
#include <driver/int.h>
#include <os/mem.h>
#include "qspi_driver.h"
#include "qspi_statis.h"
#include "qspi_nand_flash.h"

#if CONFIG_QSPI_NAND_FLASH

static bk_err_t nand_wait_ready_internal(qspi_id_t id);

static void bk_qspi_flash_wait_wip_done(qspi_id_t id)
{
	bk_err_t ret = nand_wait_ready_internal(id);
	if (ret != BK_OK) {
		QSPI_LOGW("%s: wait ready timeout(%d)\n", __func__, ret);
	}
}

static bk_err_t nand_feature_get_internal(qspi_id_t id, uint8_t addr, uint8_t *value);

bk_err_t bk_qspi_flash_init(qspi_id_t id)
{
	qspi_config_t config = {0};
	config.src_clk = QSPI_SCLK_240M;
	config.src_clk_div = 0xF;
	config.clk_div = 0x2;
	BK_LOG_ON_ERR(bk_qspi_init(id, &config));

	{
		qspi_cmd_t cmd = {0};
		cmd.device = QSPI_FLASH;
		cmd.wire_mode = QSPI_1WIRE;
		cmd.work_mode = INDIRECT_MODE;
		cmd.op = QSPI_WRITE;
		cmd.cmd = 0xFF;
		BK_LOG_ON_ERR(bk_qspi_command(id, &cmd));
		rtos_delay_milliseconds(2);
	}

	bk_qspi_flash_set_protect_none(id);
	{
		uint8_t prot = 0xFF;
		nand_feature_get_internal(id, 0xA0, &prot);
		QSPI_LOGI("init: Prot(A0h)=0x%02x (expect 0x00)\r\n", prot);
	}

#if CONFIG_QSPI_QUAD_WIRE
	bk_qspi_flash_quad_enable(id);
#endif
	{
		uint8_t cfg = 0;
		nand_feature_get_internal(id, 0xB0, &cfg);
		QSPI_LOGI("init: Cfg(B0h)=0x%02x (expect 0x11 with QE+ECC)\r\n", cfg);
	}

	return BK_OK;
}

bk_err_t bk_qspi_flash_deinit(qspi_id_t id)
{
	BK_LOG_ON_ERR(bk_qspi_deinit(id));
	return BK_OK;
}

static bk_err_t nand_feature_get_internal(qspi_id_t id, uint8_t addr, uint8_t *value)
{
	qspi_cmd_t cmd = {0};
	uint32_t temp_val = 0;

	if (id >= QSPI_ID_MAX) {
		QSPI_LOGE("nand_feature_get_internal: Invalid ID=%u\r\n", id);
		return BK_ERR_PARAM;
	}

	cmd.device = QSPI_FLASH;
	cmd.wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_READ;
	cmd.cmd = NAND_CMD_GET_FEATURE;
	cmd.data_len = 1;
	cmd.addr = addr;

	bk_err_t ret = bk_qspi_command(id, &cmd);
	if (ret != BK_OK) {
		return ret;
	}

	ret = bk_qspi_read(id, &temp_val, 1);
	*value = (uint8_t)temp_val;
	return ret;
}

static bk_err_t nand_wait_ready_with_status(qspi_id_t id, uint8_t *out_status)
{
	uint8_t status = 0;

	for (uint32_t elapsed = 0; elapsed <= NAND_DEFAULT_TIMEOUT_MS; elapsed++) {
		BK_RETURN_ON_ERR(nand_feature_get_internal(id, NAND_FEATURE_ADDR_STATUS, &status));
		if (!(status & NAND_STATUS_OIP)) {
			if (out_status) {
				*out_status = status;
			}
			return BK_OK;
		}
		rtos_delay_milliseconds(1);
	}

	return BK_ERR_TIMEOUT;
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
	cmd.wire_mode = QSPI_1WIRE;
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
	cmd.wire_mode = QSPI_1WIRE;
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
	cmd.wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = ((uint32_t)value << (2 * QSPI_CMD1_LEN)) |
		((uint32_t)addr << QSPI_CMD1_LEN) |
		NAND_CMD_SET_FEATURE;

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

	BK_RETURN_ON_ERR(nand_write_enable_bare(id));

	cmd.device = QSPI_FLASH;
	cmd.wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_BLOCK_ERASE;
	cmd.addr = block * NAND_BLOCK_PAGE_COUNT;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));

	uint8_t status = 0;
	BK_RETURN_ON_ERR(nand_wait_ready_with_status(id, &status));
	if (status & NAND_STATUS_E_FAIL) {
		QSPI_LOGE("block %u erase E-FAIL (status=0x%02x)\r\n", block, status);
		return BK_FAIL;
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

		os_memcpy(temp_buf, buf, chunk);

		cmd.device = QSPI_FLASH;
		cmd.wire_mode = QSPI_1WIRE;
		cmd.work_mode = INDIRECT_MODE;
		cmd.op = QSPI_WRITE;
		cmd.cmd = first ? NAND_CMD_PROGRAM_LOAD : NAND_CMD_PROGRAM_LOAD_RANDOM;
		cmd.addr = column;
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

	BK_RETURN_ON_ERR(nand_write_enable_bare(id));
	BK_RETURN_ON_ERR(nand_program_load_internal(id, column, buf, len));

	qspi_cmd_t cmd = {0};
	cmd.device = QSPI_FLASH;
	cmd.wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_PROGRAM_EXECUTE;
	cmd.addr = page;
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
	cmd.wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_PAGE_READ;
	cmd.addr = page;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	BK_RETURN_ON_ERR(nand_wait_ready_internal(id));

	uint32_t current_column = column;
	uint32_t remaining = len;

	while (remaining) {
		uint32_t chunk = remaining > QSPI_FIFO_LEN_MAX ? QSPI_FIFO_LEN_MAX : remaining;

		cmd.device = QSPI_FLASH;
		cmd.wire_mode = QSPI_1WIRE;
		cmd.work_mode = INDIRECT_MODE;
		cmd.op = QSPI_READ;
		cmd.cmd = NAND_CMD_READ_FROM_CACHE;
		cmd.addr = current_column;
		cmd.dummy_cycle = 0;
		cmd.data_len = chunk;

		BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
		BK_RETURN_ON_ERR(bk_qspi_read(id, buf, chunk));

		buf += chunk;
		current_column += chunk;
		remaining -= chunk;
	}

	return BK_OK;
}

#if CONFIG_QSPI_QUAD_WIRE
static bk_err_t nand_program_load_quad_internal(qspi_id_t id, uint32_t column, const uint8_t *buf, uint32_t len)
{
	bool first = true;
	uint32_t temp_buf[QSPI_FIFO_LEN_MAX / 4];

	while (len) {
		uint32_t chunk = len > QSPI_FIFO_LEN_MAX ? QSPI_FIFO_LEN_MAX : len;
		qspi_cmd_t cmd = {0};

		os_memcpy(temp_buf, buf, chunk);

		cmd.device = QSPI_FLASH;
		cmd.work_mode = INDIRECT_MODE;
		cmd.op = QSPI_WRITE;

		if (first) {
			cmd.wire_mode = QSPI_4WIRE;
			cmd.cmd = NAND_CMD_PRORAM_LOAD_QUAD;
		} else {
			cmd.wire_mode = QSPI_4WIRE;
			cmd.cmd = NAND_CMD_PRORAM_LOAD_RANDOM_QUAD;
		}
		cmd.addr = column;
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

	BK_RETURN_ON_ERR(nand_write_enable_bare(id));
	BK_RETURN_ON_ERR(nand_program_load_quad_internal(id, column, buf, len));

	qspi_cmd_t cmd = {0};
	cmd.device = QSPI_FLASH;
	cmd.wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_PROGRAM_EXECUTE;
	cmd.addr = page;
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
	uint32_t temp_buf[QSPI_FIFO_LEN_MAX / 4];

	cmd.device = QSPI_FLASH;
	cmd.wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_WRITE;
	cmd.cmd = NAND_CMD_PAGE_READ;
	cmd.addr = page;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	BK_RETURN_ON_ERR(nand_wait_ready_internal(id));

	uint32_t current_column = column;
	uint32_t remaining = len;

	while (remaining) {
		uint32_t chunk = remaining > QSPI_FIFO_LEN_MAX ? QSPI_FIFO_LEN_MAX : remaining;

		cmd.device = QSPI_FLASH;
		cmd.wire_mode = QSPI_4WIRE;
		cmd.work_mode = INDIRECT_MODE;
		cmd.op = QSPI_READ;
		cmd.cmd = NAND_CMD_READ_FROM_CACHE_X4;
		cmd.addr = current_column;
		cmd.dummy_cycle = 8;
		cmd.data_len = chunk;

		BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
		BK_RETURN_ON_ERR(bk_qspi_read(id, temp_buf, chunk));

		os_memcpy(buf, temp_buf, chunk);

		buf += chunk;
		current_column += chunk;
		remaining -= chunk;
	}

	return BK_OK;
}
#endif /* CONFIG_QSPI_QUAD_WIRE */

static bk_err_t nand_read_id_internal(qspi_id_t id, uint8_t *buf, uint32_t len)
{
	BK_RETURN_ON_NULL(buf);
	if (!len || len > FLASH_READ_ID_SIZE) {
		return BK_ERR_PARAM;
	}

	qspi_cmd_t cmd = {0};
	cmd.device = QSPI_FLASH;
	cmd.wire_mode = QSPI_1WIRE;
	cmd.work_mode = INDIRECT_MODE;
	cmd.op = QSPI_READ;
	cmd.cmd = FLASH_READ_ID_CMD;
	cmd.addr = 0;
	cmd.data_len = len;

	BK_RETURN_ON_ERR(bk_qspi_command(id, &cmd));
	return bk_qspi_read(id, buf, len);
}


bk_err_t bk_qspi_flash_single_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
	const uint8_t *buf8 = (const uint8_t *)data;
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

#if CONFIG_QSPI_QUAD_WIRE
bk_err_t bk_qspi_flash_quad_page_program(qspi_id_t id, uint32_t addr, const void *data, uint32_t size)
{
	const uint8_t *buf8 = (const uint8_t *)data;
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
#endif /* CONFIG_QSPI_QUAD_WIRE */


bk_err_t bk_qspi_flash_write(qspi_id_t id, uint32_t base_addr, const void *data, uint32_t size)
{
	uint8_t buf[QSPI_FIFO_LEN_MAX] = {0};
	uint32_t left_len = size;
	uint32_t write_len= 0;
	uint32_t write_addr = 0;
	uint32_t offset = 0;
	uint32_t page_write_len = 0;

	if(0 != (base_addr & FLASH_PAGE_MASK)) {
		write_addr = base_addr & (~FLASH_PAGE_MASK);
#if CONFIG_QSPI_QUAD_WIRE
		bk_qspi_flash_quad_read(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
#else
		bk_qspi_flash_single_read(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
#endif
		page_write_len = (QSPI_FIFO_LEN_MAX - (base_addr & FLASH_PAGE_MASK));
		write_len = (page_write_len > left_len) ? left_len : page_write_len;
		os_memcpy(buf + (base_addr & FLASH_PAGE_MASK), data, write_len);
#if CONFIG_QSPI_QUAD_WIRE
		bk_qspi_flash_quad_page_program(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
#else
		bk_qspi_flash_single_page_program(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
#endif
		offset += write_len;
		left_len -= write_len;
		if(left_len == 0) {
			return BK_OK;
		}
	}

	for (write_addr = base_addr + offset; write_addr < ((base_addr + size) & (~FLASH_PAGE_MASK)); write_addr += write_len) {
		write_len = (left_len > QSPI_FIFO_LEN_MAX) ? QSPI_FIFO_LEN_MAX : left_len;
#if CONFIG_QSPI_QUAD_WIRE
		bk_qspi_flash_quad_page_program(id, write_addr, data + offset, write_len);
#else
		bk_qspi_flash_single_page_program(id, write_addr, data + offset, write_len);
#endif
		offset += write_len;
		left_len -= write_len;
	}

	if(left_len == 0) {
		return BK_OK;
	}
	if(0 != ((base_addr + size) & FLASH_PAGE_MASK)) {
#if CONFIG_QSPI_QUAD_WIRE
		bk_qspi_flash_quad_read(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
#else
		bk_qspi_flash_single_read(id, write_addr, buf, QSPI_FIFO_LEN_MAX);
#endif
		write_len = (base_addr + size) & FLASH_PAGE_MASK;
		os_memcpy(buf, data + offset, write_len);
#if CONFIG_QSPI_QUAD_WIRE
		bk_qspi_flash_quad_page_program(id, write_addr, buf, write_len);
#else
		bk_qspi_flash_single_page_program(id, write_addr, buf, write_len);
#endif
	}

	return BK_OK;
}


bk_err_t bk_qspi_flash_read(qspi_id_t id, uint32_t base_addr, void *data, uint32_t size)
{
	uint8_t buf[QSPI_FIFO_LEN_MAX] = {0};
	uint32_t left_len = size;
	uint32_t read_len= 0;
	uint32_t offset = 0;

	for (uint32_t addr = base_addr; addr < (base_addr + size); addr += QSPI_FIFO_LEN_MAX) {
		offset += read_len;
		read_len = (left_len >= QSPI_FIFO_LEN_MAX) ? QSPI_FIFO_LEN_MAX : left_len;
#if CONFIG_QSPI_QUAD_WIRE
		bk_qspi_flash_quad_read(id, addr, buf, QSPI_FIFO_LEN_MAX);
#else
		bk_qspi_flash_single_read(id, addr, buf, QSPI_FIFO_LEN_MAX);
#endif
		os_memcpy(data + offset, buf, read_len);
		left_len -= QSPI_FIFO_LEN_MAX;
	}

	return BK_OK;
}


bk_err_t bk_qspi_flash_nand_get_feature(qspi_id_t id, uint8_t addr, uint8_t *value)
{
	BK_RETURN_ON_NULL(value);
	return nand_feature_get_internal(id, addr, value);
}

bk_err_t bk_qspi_flash_nand_get_id(qspi_id_t id, uint8_t *buf, uint32_t len)
{
	return nand_read_id_internal(id, buf, len);
}

bk_err_t bk_qspi_flash_nand_set_feature(qspi_id_t id, uint8_t addr, uint8_t value)
{
	return nand_feature_set_internal(id, addr, value);
}

bk_err_t bk_qspi_flash_nand_get_block_lock(qspi_id_t id, uint8_t *value)
{
	return bk_qspi_flash_nand_get_feature(id, NAND_FEATURE_ADDR_BLOCK_LOCK, value);
}

bk_err_t bk_qspi_flash_nand_set_block_lock(qspi_id_t id, uint8_t value)
{
	return bk_qspi_flash_nand_set_feature(id, NAND_FEATURE_ADDR_BLOCK_LOCK, value);
}

bk_err_t bk_qspi_flash_nand_get_status(qspi_id_t id, uint8_t *value)
{
	return bk_qspi_flash_nand_get_feature(id, NAND_FEATURE_ADDR_STATUS, value);
}

bk_err_t bk_qspi_flash_nand_get_feature_register(qspi_id_t id, uint8_t *value)
{
	if (id >= QSPI_ID_MAX) {
		QSPI_LOGE("get_feature_reg: Invalid ID=%u\r\n", id);
	}
	return bk_qspi_flash_nand_get_feature(id, NAND_FEATURE_ADDR_DRIVE, value);
}

bk_err_t bk_qspi_flash_nand_set_feature_register(qspi_id_t id, uint8_t value)
{
	return bk_qspi_flash_nand_set_feature(id, NAND_FEATURE_ADDR_DRIVE, value);
}

bk_err_t bk_qspi_flash_nand_set_protect_none(qspi_id_t id)
{
	uint8_t lock = 0;
	BK_RETURN_ON_ERR(bk_qspi_flash_nand_set_block_lock(id, 0x00));
	BK_RETURN_ON_ERR(bk_qspi_flash_nand_get_block_lock(id, &lock));
	if (lock != 0x00) {
		return BK_ERR_STATE;
	}
	return BK_OK;
}

bk_err_t bk_qspi_flash_nand_block_erase(qspi_id_t id, uint32_t block)
{
	return nand_block_erase_internal(id, block);
}

bk_err_t bk_qspi_flash_erase(qspi_id_t id, uint32_t addr, uint32_t size)
{
	uint32_t block_start = addr / NAND_BLOCK_SIZE_BYTES;
	uint32_t block_end = (addr + size - 1) / NAND_BLOCK_SIZE_BYTES;
	bk_err_t ret = BK_OK;

	for (uint32_t i = block_start; i <= block_end; i++) {
		ret = bk_qspi_flash_nand_block_erase(id, i);
		if (ret != BK_OK) {
			QSPI_LOGE("%s: erase block %d failed\n", __func__, i);
			return ret;
		}
	}
	return BK_OK;
}

bk_err_t bk_qspi_flash_nand_page_program(qspi_id_t id, uint32_t page, uint32_t column, const uint8_t *buf, uint32_t len)
{
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

#if CONFIG_QSPI_QUAD_WIRE
bk_err_t bk_qspi_flash_nand_page_program_quad(qspi_id_t id, uint32_t page, uint32_t column, const uint8_t *buf, uint32_t len)
{
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
	if (id >= QSPI_ID_MAX) {
		QSPI_LOGE("quad_enable: Invalid ID=%u\r\n", id);
		return BK_ERR_PARAM;
	}
	uint8_t cfg = 0;
	bk_err_t ret = bk_qspi_flash_nand_get_feature_register(id, &cfg);
	if (ret != BK_OK) {
		QSPI_LOGW("%s: read cfg fail(%d)\n", __func__, ret);
		return ret;
	}

	if (!(cfg & NAND_CFG_QE_BIT)) {
		ret = bk_qspi_flash_nand_set_feature_register(id, cfg | NAND_CFG_QE_BIT);
		if (ret != BK_OK) {
			QSPI_LOGW("%s: set cfg fail(%d)\n", __func__, ret);
			return ret;
		}
	}

	return BK_OK;
}
#endif /* CONFIG_QSPI_QUAD_WIRE */

bk_err_t bk_qspi_flash_set_protect_none(qspi_id_t id)
{
	return bk_qspi_flash_nand_set_protect_none(id);
}

uint32_t bk_qspi_flash_read_id(qspi_id_t id)
{
	uint8_t id_buf[FLASH_READ_ID_SIZE] = {0};
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
		QSPI_LOGI("%s xt26 id = 0x%02x%02x\n", __func__, id_buf[1], id_buf[0]);
	}

	BK_LOG_ON_ERR(bk_qspi_flash_nand_set_protect_none(id));
	BK_LOG_ON_ERR(bk_qspi_flash_nand_block_erase(id, base_addr / NAND_BLOCK_SIZE_BYTES));

	BK_LOG_ON_ERR(bk_qspi_flash_single_read(id, page_addr, verify, NAND_PAGE_SIZE_BYTES));
	for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
		if (verify[i] != 0xFF) {
			QSPI_LOGI("[XT26 TEST] erase mismatch idx:%u val:%02x\r\n", i, verify[i]);
			break;
		}
	}

	os_memset(buf, 0xAA, NAND_PAGE_SIZE_BYTES);
	BK_LOG_ON_ERR(bk_qspi_flash_single_page_program(id, page_addr, buf, NAND_PAGE_SIZE_BYTES));
	BK_LOG_ON_ERR(bk_qspi_flash_single_read(id, page_addr, verify, NAND_PAGE_SIZE_BYTES));
	for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
		if (verify[i] != 0xAA) {
			QSPI_LOGI("[XT26 TEST] 0xAA mismatch idx:%u val:%02x\r\n", i, verify[i]);
			break;
		}
	}

	os_memset(buf, 0x55, NAND_PAGE_SIZE_BYTES);
	BK_LOG_ON_ERR(bk_qspi_flash_single_page_program(id, page_addr, buf, NAND_PAGE_SIZE_BYTES));
	BK_LOG_ON_ERR(bk_qspi_flash_single_read(id, page_addr, verify, NAND_PAGE_SIZE_BYTES));
	for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
		if (verify[i] != 0x55) {
			QSPI_LOGI("[XT26 TEST] 0x55 mismatch idx:%u val:%02x\r\n", i, verify[i]);
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
