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
#if 0
#include <driver/qspi.h>
#if (CONFIG_QSPI_MST_FLASH) || (CONFIG_QSPI_NAND_FLASH)
#include <driver/qspi_flash.h>
#endif
#include <driver/qspi_psram.h>
#include "cli.h"
#include "qspi_hw.h"

#define PSRAM_TEST_START_ADDR(_id)         (QSPI_DCACHE_BASE_ADDR(_id))
#define PSRAM_TEST_LEN                (1024 * 10)

#if CONFIG_QSPI_NAND_FLASH
#define NAND_PAGE_SIZE_BYTES          2048U
#define NAND_BLOCK_PAGE_COUNT         64U
#define NAND_BLOCK_SIZE_BYTES         (NAND_PAGE_SIZE_BYTES * NAND_BLOCK_PAGE_COUNT)
#define NAND_WAIT_TIMEOUT_MS          100U
#define NAND_STATUS_BUSY_BIT          BIT(0)
#endif

static void cli_qspi_help(void)
{
	CLI_LOGD("qspi_driver init\r\n");
	CLI_LOGD("qspi_driver deinit\r\n");
	CLI_LOGD("qspi init\r\n");
	CLI_LOGD("qspi enter_quad_mode\r\n");
	CLI_LOGD("qspi exit_quad_mode\r\n");
	CLI_LOGD("qspi quad_write\r\n");
	CLI_LOGD("qspi quad_read\r\n");
	CLI_LOGD("qspi compare\r\n");
#if (CONFIG_QSPI_MST_FLASH)
	CLI_LOGD("qspi_flash get_id\r\n");
	CLI_LOGD("qspi_flash erase 0 256\r\n");
	CLI_LOGD("qspi_flash single_write 0 256\r\n");
	CLI_LOGD("qspi_flash single_read 0 256\r\n");
#endif
}

static void cli_qspi_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_qspi_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_qspi_driver_init());
		CLI_LOGD("qspi driver init\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		BK_LOG_ON_ERR(bk_qspi_driver_deinit());
		CLI_LOGD("qspi driver deinit\n");
	} else {
		cli_qspi_help();
		return;
	}
}

static bk_err_t cli_qspi_psram_8bit_increase_init_memory(uint8_t *buf, uint32_t count)
{
	BK_RETURN_ON_NULL(buf);
	uint8_t *ptr = buf;
	for (int i = 0; i < count; i++) {
		ptr[i] = i & 0xff;
	}
	return BK_OK;
}

static bk_err_t cli_qspi_psram_8bit_increase_compare(uint8_t *buf, uint32_t count)
{
	BK_RETURN_ON_NULL(buf);
	uint8_t *ptr = buf;
	for (int i = 0; i < count; i ++) {
		if ((i & 0xff) != ptr[i]) {
			CLI_LOGW("qspi dcache failed [%d] addr:0x%x, value:%x/%x\r\n", i, &ptr[i], (i & 0xff), ptr[i]);
			return -1;
		}
	}
	return BK_OK;
}

static bk_err_t cli_qspi_psram_8bit_init_fixed_value(uint8_t *buf, uint32_t count, uint8_t val)
{
	BK_RETURN_ON_NULL(buf);
	uint8_t *ptr = buf;
	for (int i = 0; i < count; i++) {
		ptr[i] = val;
	}
	return BK_OK;
}

static bk_err_t cli_qspi_psram_8bit_cmp_fixed_value(uint8_t *buf, uint32_t count, uint8_t val)
{
	BK_RETURN_ON_NULL(buf);
	uint8_t *ptr = buf;
	for (int i = 0; i < count; i ++) {
		if (val != ptr[i]) {
			CLI_LOGW("qspi dcache [%d] addr:0x%x, %x/%x\r\n", i, &ptr[i], val, ptr[i]);
			return -1;
		}
	}
	return BK_OK;
}

static void cli_qspi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_qspi_help();
		return;
	}

	uint32_t qspi_id = os_strtoul(argv[1], NULL, 10);
	CLI_LOGD("qspi_id:%08x\r\n",qspi_id);

	if (os_strcmp(argv[2], "init") == 0) {
		qspi_config_t config = {0};
		config.src_clk = os_strtoul(argv[3], NULL, 10);
		config.src_clk_div = os_strtoul(argv[4], NULL, 10);
		config.clk_div = os_strtoul(argv[5], NULL, 10);
		BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &config));
		CLI_LOGD("qspi init\r\n");
#if (CONFIG_QSPI_MST_FLASH)
	} else if (os_strcmp(argv[2], "flash_test") == 0) {
		extern void test_qspi_flash(uint32_t id, uint32_t base_addr, uint32_t buf_len);
		uint32_t base_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t buf_len = os_strtoul(argv[4], NULL, 10);
		test_qspi_flash(qspi_id, base_addr, buf_len);
		CLI_LOGD("qspi flash test end\r\n");
#endif
	} else if (os_strcmp(argv[2], "enter_quad_mode") == 0) {
		BK_LOG_ON_ERR(bk_qspi_psram_enter_quad_mode(qspi_id));
		CLI_LOGD("qspi enter quad mode\r\n");
	} else if (os_strcmp(argv[2], "exit_quad_mode") == 0) {
		BK_LOG_ON_ERR(bk_qspi_psram_exit_quad_mode(qspi_id));
		CLI_LOGD("qspi exit quad mode\r\n");
	} else if (os_strcmp(argv[2], "quad_write") == 0) {
		BK_LOG_ON_ERR(bk_qspi_psram_quad_write(qspi_id));
		CLI_LOGD("qspi psram quad write mode\r\n");
	} else if (os_strcmp(argv[2], "quad_read") == 0) {
		BK_LOG_ON_ERR(bk_qspi_psram_quad_read(qspi_id));
		CLI_LOGD("qspi psram quad read mode\r\n");
	} else if (os_strcmp(argv[2], "single_write") == 0) {
		BK_LOG_ON_ERR(bk_qspi_psram_single_write(qspi_id));
		CLI_LOGD("qspi psram single write mode\r\n");
	} else if (os_strcmp(argv[2], "single_read") == 0) {
		BK_LOG_ON_ERR(bk_qspi_psram_single_read(qspi_id));
		CLI_LOGD("qspi psram single read mode\r\n");
	} else if (os_strcmp(argv[2], "compare") == 0) {
		cli_qspi_psram_8bit_increase_init_memory((uint8_t *)PSRAM_TEST_START_ADDR(qspi_id), PSRAM_TEST_LEN);
		cli_qspi_psram_8bit_increase_compare((uint8_t *)PSRAM_TEST_START_ADDR(qspi_id), PSRAM_TEST_LEN);

		cli_qspi_psram_8bit_init_fixed_value((uint8_t *)PSRAM_TEST_START_ADDR(qspi_id), PSRAM_TEST_LEN, 0xff);
		cli_qspi_psram_8bit_cmp_fixed_value((uint8_t *)PSRAM_TEST_START_ADDR(qspi_id), PSRAM_TEST_LEN, 0xff);

		cli_qspi_psram_8bit_init_fixed_value((uint8_t *)PSRAM_TEST_START_ADDR(qspi_id), PSRAM_TEST_LEN, 0x5a);
		cli_qspi_psram_8bit_cmp_fixed_value((uint8_t *)PSRAM_TEST_START_ADDR(qspi_id), PSRAM_TEST_LEN, 0x5a);
		CLI_LOGD("qspi psram write and read ok\r\n");
	} else if (os_strcmp(argv[2], "write") == 0) {
		uint32_t base_addr = os_strtoul(argv[3], NULL, 16);
		uint8_t write_data = os_strtoul(argv[4], NULL, 16);
		uint32_t write_size = os_strtoul(argv[5], NULL, 10);
		uint8_t *wr_buf = (uint8_t *)os_malloc(write_size);
		if (!wr_buf) {
			CLI_LOGE("qspi write buff malloc failed\r\n");
			return;
		}
		os_memset(wr_buf, write_data, write_size);
		BK_LOG_ON_ERR(bk_qspi_psram_write(qspi_id, base_addr, wr_buf, write_size));
		if (wr_buf) {
			os_free(wr_buf);
			wr_buf = NULL;
		}
		CLI_LOGD("qspi psram write\r\n");
	} else if (os_strcmp(argv[2], "read") == 0) {
		uint32_t base_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t read_size = os_strtoul(argv[4], NULL, 10);
		uint8_t *rd_buf = (uint8_t *)os_zalloc(read_size);
		if (!rd_buf) {
			CLI_LOGE("qspi read buff malloc failed\r\n");
			return;
		}
		BK_LOG_ON_ERR(bk_qspi_psram_read(qspi_id, base_addr, rd_buf, read_size));
		for (int i = 0; i < read_size; i++) {
			BK_DUMP_OUT("read_buf[%d]=%x\r\n", i, rd_buf[i]);
		}
		if (rd_buf) {
			os_free(rd_buf);
			rd_buf = NULL;
		}
		CLI_LOGD("qspi psram read\r\n");
	} else {
		cli_qspi_help();
	}
}

#if (CONFIG_QSPI_MST_FLASH)
#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_SIZE 0x1000

static void cli_qspi_flash_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_qspi_help();
		return;
	}

	uint32_t qspi_id = os_strtoul(argv[1], NULL, 10);
	uint32_t start_addr = os_strtoul(argv[3], NULL, 16);
	uint32_t len = os_strtoul(argv[4], NULL, 10);

	if (os_strcmp(argv[2], "erase") == 0) {
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_SECTOR_SIZE) {
			bk_qspi_flash_erase_sector(qspi_id, addr);
		}

	} else if (os_strcmp(argv[2], "read") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			os_memset(buf, 0, FLASH_PAGE_SIZE);
			bk_qspi_flash_read(qspi_id, addr, buf, FLASH_PAGE_SIZE);
			CLI_LOGD("flash read addr:%x\r\n", addr);

			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_DUMP_OUT("%02x ", buf[i * 16 + j]);
				}
				BK_DUMP_OUT("\r\n");
			}
		}
	} else if (os_strcmp(argv[2], "write") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
			buf[i] = i;
		}

		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			bk_qspi_flash_write(qspi_id, addr, buf, FLASH_PAGE_SIZE);
		}
	} else if (os_strcmp(argv[2], "single_read") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			os_memset(buf, 0, FLASH_PAGE_SIZE);
			bk_qspi_flash_single_read(qspi_id, addr, buf, FLASH_PAGE_SIZE);
			CLI_LOGD("flash read addr:%x\r\n", addr);

			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_DUMP_OUT("%02x ", buf[i * 16 + j]);
				}
				BK_DUMP_OUT("\r\n");
			}
		}
	} else if (os_strcmp(argv[2], "single_write") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
			buf[i] = i;
		}

		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			bk_qspi_flash_single_page_program(qspi_id, addr, buf, FLASH_PAGE_SIZE);
		}

	} else if (os_strcmp(argv[2], "get_id") == 0) {
		uint32_t flash_id = bk_qspi_flash_read_id(qspi_id);
		bk_qspi_flash_set_protect_none(qspi_id);
#if CONFIG_QSPI_QUAD_WIRE
		bk_qspi_flash_quad_enable(qspi_id);
#endif
		CLI_LOGD("flash_id:%x\r\n", flash_id);
	}
}
#endif

#if CONFIG_QSPI_NAND_FLASH
static bool s_nand_initialized = false;

static void cli_nand_usage(void)
{
	CLI_LOGI("qspi_nand init\r\n");
	CLI_LOGI("qspi_nand get_id\r\n");
	CLI_LOGI("qspi_nand protect_none\r\n");
	CLI_LOGI("qspi_nand quad_enable - Enable Quad (4-wire) mode\r\n");
	CLI_LOGI("qspi_nand get_feature {addr}\r\n");
	CLI_LOGI("qspi_nand set_feature {addr} {value}\r\n");
	CLI_LOGI("qspi_nand block_erase {block}\r\n");
	CLI_LOGI("qspi_nand page_program {page} {column} {pattern} {len}\r\n");
	CLI_LOGI("qspi_nand page_read {page} {column} {len}\r\n");
	CLI_LOGI("qspi_nand page_program_quad {page} {column} {pattern} {len}\r\n");
	CLI_LOGI("qspi_nand page_read_quad {page} {column} {len}\r\n");
	CLI_LOGI("qspi_nand test_page {page} - Test erase/write/read on a page\r\n");
	CLI_LOGI("qspi_nand page_test {page} - Single page erase/write/read verify\r\n");
	CLI_LOGI("qspi_nand block_test {block} - Full block erase/write/read verify\r\n");
}

static void cli_qspi_nand_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	(void)pcWriteBuffer;
	(void)xWriteBufferLen;

	if (argc < 2) {
		cli_nand_usage();
		return;
	}

	uint32_t qspi_id = QSPI_ID_0;
	const char *subcmd = argv[1];

	if (os_strcmp(subcmd, "init") == 0) {
		BK_LOG_ON_ERR(bk_qspi_driver_init());
		BK_LOG_ON_ERR(bk_qspi_flash_init(qspi_id));
		s_nand_initialized = true;
		CLI_LOGI("nand init done\r\n");
	} else if (os_strcmp(subcmd, "get_id") == 0) {
		uint8_t id_buf[2] = {0};
		bk_err_t ret = bk_qspi_flash_nand_get_id(qspi_id, id_buf, sizeof(id_buf));
		if (ret == BK_OK) {
			CLI_LOGI("nand id: %02x %02x\n", id_buf[0], id_buf[1]);
		} else {
			CLI_LOGE("nand get id fail(%d)\n", ret);
		}
	} else if (os_strcmp(subcmd, "protect_none") == 0) {
		BK_LOG_ON_ERR(bk_qspi_flash_nand_set_protect_none(qspi_id));
		CLI_LOGI("nand protect none\r\n");
	} else if (os_strcmp(subcmd, "quad_enable") == 0) {
		if (!s_nand_initialized) {
			CLI_LOGE("QSPI not initialized! Please run 'qspi_nand init' first\r\n");
			return;
		}
#if CONFIG_QSPI_QUAD_WIRE
		bk_qspi_flash_quad_enable(qspi_id);
		CLI_LOGI("nand quad (4-wire) mode enabled\r\n");
#else
		CLI_LOGI("QSPI_QUAD_WIRE not enabled\r\n");
#endif
	} else if (os_strcmp(subcmd, "get_feature") == 0) {
		if (argc < 3) { cli_nand_usage(); return; }
		uint32_t addr = os_strtoul(argv[2], NULL, 0);
		uint8_t value = 0;
		BK_LOG_ON_ERR(bk_qspi_flash_nand_get_feature(qspi_id, addr & 0xFF, &value));
		CLI_LOGI("feature[0x%02x]=0x%02x\r\n", (uint32_t)(addr & 0xFF), value);
	} else if (os_strcmp(subcmd, "set_feature") == 0) {
		if (argc < 4) { cli_nand_usage(); return; }
		uint32_t addr = os_strtoul(argv[2], NULL, 0);
		uint32_t value = os_strtoul(argv[3], NULL, 0);
		BK_LOG_ON_ERR(bk_qspi_flash_nand_set_feature(qspi_id, addr & 0xFF, value & 0xFF));
		CLI_LOGI("set feature[0x%02x]=0x%02x\r\n", (uint32_t)(addr & 0xFF), (uint32_t)(value & 0xFF));
	} else if (os_strcmp(subcmd, "block_erase") == 0) {
		if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
		if (argc < 3) { cli_nand_usage(); return; }
		uint32_t block = os_strtoul(argv[2], NULL, 0);
		bk_err_t ret = bk_qspi_flash_nand_block_erase(qspi_id, block);
		CLI_LOGI("block %u erase %s\r\n", block, (ret == BK_OK) ? "done" : "FAIL");
	} else if (os_strcmp(subcmd, "page_program") == 0) {
		if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
		if (argc < 6) { cli_nand_usage(); return; }
		uint32_t page = os_strtoul(argv[2], NULL, 0);
		uint32_t column = os_strtoul(argv[3], NULL, 0);
		uint32_t pattern = os_strtoul(argv[4], NULL, 0);
		uint32_t len = os_strtoul(argv[5], NULL, 0);
		if ((column >= NAND_PAGE_SIZE_BYTES) || (len == 0) || ((column + len) > NAND_PAGE_SIZE_BYTES)) {
			CLI_LOGE("invalid params\r\n"); return;
		}
		uint8_t *buf = (uint8_t *)os_malloc(len);
		if (!buf) { CLI_LOGE("no mem\r\n"); return; }
		os_memset(buf, pattern & 0xFF, len);
		bk_err_t ret = bk_qspi_flash_nand_page_program(qspi_id, page, column, buf, len);
		os_free(buf);
		CLI_LOGI("page_program page:%u col:0x%x len:%u %s\r\n", page, column, len,
		         (ret == BK_OK) ? "OK" : "FAIL");
	} else if (os_strcmp(subcmd, "page_read") == 0) {
		if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
		if (argc < 5) { cli_nand_usage(); return; }
		uint32_t page = os_strtoul(argv[2], NULL, 0);
		uint32_t column = os_strtoul(argv[3], NULL, 0);
		uint32_t len = os_strtoul(argv[4], NULL, 0);
		if (!len || (column >= NAND_PAGE_SIZE_BYTES) || ((column + len) > NAND_PAGE_SIZE_BYTES)) {
			CLI_LOGE("invalid params\r\n"); return;
		}
		uint8_t *buffer = (uint8_t *)os_malloc(len);
		if (!buffer) { CLI_LOGE("no mem\r\n"); return; }
		os_memset(buffer, 0, len);
		uint32_t addr = page * NAND_PAGE_SIZE_BYTES + column;
		bk_err_t ret = bk_qspi_flash_single_read(qspi_id, addr, buffer, len);
		if (ret == BK_OK) {
			CLI_LOGI("page %u col 0x%x len %u:\r\n", page, column, len);
			for (uint32_t i = 0; i < len; i++) {
				CLI_LOGI("%02x%s", buffer[i], ((i + 1) % 16) ? " " : "\r\n");
			}
			if (len % 16) CLI_LOGI("\r\n");
		} else {
			CLI_LOGE("page read failed: %d\r\n", ret);
		}
		os_free(buffer);
#if CONFIG_QSPI_QUAD_WIRE
	} else if (os_strcmp(subcmd, "page_program_quad") == 0) {
		if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
		if (argc < 6) { cli_nand_usage(); return; }
		uint32_t page = os_strtoul(argv[2], NULL, 0);
		uint32_t column = os_strtoul(argv[3], NULL, 0);
		uint32_t pattern = os_strtoul(argv[4], NULL, 0);
		uint32_t len = os_strtoul(argv[5], NULL, 0);
		if ((column >= NAND_PAGE_SIZE_BYTES) || (len == 0) || ((column + len) > NAND_PAGE_SIZE_BYTES)) {
			CLI_LOGE("invalid params\r\n"); return;
		}
		uint8_t *buf = (uint8_t *)os_malloc(len);
		if (!buf) { CLI_LOGE("no mem\r\n"); return; }
		os_memset(buf, pattern & 0xFF, len);
		bk_err_t ret = bk_qspi_flash_nand_page_program_quad(qspi_id, page, column, buf, len);
		os_free(buf);
		CLI_LOGI("page_program_quad page:%u col:0x%x len:%u %s\r\n", page, column, len,
		         (ret == BK_OK) ? "OK" : "FAIL");
	} else if (os_strcmp(subcmd, "page_read_quad") == 0) {
		if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
		if (argc < 5) { cli_nand_usage(); return; }
		uint32_t page = os_strtoul(argv[2], NULL, 0);
		uint32_t column = os_strtoul(argv[3], NULL, 0);
		uint32_t len = os_strtoul(argv[4], NULL, 0);
		if (!len || (column >= NAND_PAGE_SIZE_BYTES) || ((column + len) > NAND_PAGE_SIZE_BYTES)) {
			CLI_LOGE("invalid params\r\n"); return;
		}
		uint8_t *buffer = (uint8_t *)os_malloc(len);
		if (!buffer) { CLI_LOGE("no mem\r\n"); return; }
		os_memset(buffer, 0, len);
		bk_err_t ret = bk_qspi_flash_nand_page_read_quad(qspi_id, page, column, buffer, len);
		if (ret == BK_OK) {
			CLI_LOGI("page %u col 0x%x len %u (quad):\r\n", page, column, len);
			for (uint32_t i = 0; i < len; i++) {
				CLI_LOGI("%02x%s", buffer[i], ((i + 1) % 16) ? " " : "\r\n");
			}
			if (len % 16) CLI_LOGI("\r\n");
		} else {
			CLI_LOGE("page read quad failed: %d\r\n", ret);
		}
		os_free(buffer);
#endif /* CONFIG_QSPI_QUAD_WIRE */
	} else if (os_strcmp(subcmd, "page_test") == 0) {
		if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
		if (argc < 3) { CLI_LOGI("Usage: qspi_nand page_test <page>\r\n"); return; }
		uint32_t page = os_strtoul(argv[2], NULL, 0);
		uint32_t block = page / NAND_BLOCK_PAGE_COUNT;
		bk_err_t ret;
		CLI_LOGI("=== PAGE_TEST: page=%u block=%u ===\r\n", page, block);
		uint8_t *wr_buf = (uint8_t *)os_malloc(NAND_PAGE_SIZE_BYTES);
		uint8_t *rd_buf = (uint8_t *)os_malloc(NAND_PAGE_SIZE_BYTES);
		if (!wr_buf || !rd_buf) {
			CLI_LOGE("malloc failed\r\n");
			if (wr_buf) os_free(wr_buf);
			if (rd_buf) os_free(rd_buf);
			return;
		}
		ret = bk_qspi_flash_nand_block_erase(qspi_id, block);
		if (ret != BK_OK) { CLI_LOGE("erase fail\r\n"); goto pt_end; }
		os_memset(wr_buf, 0xAA, NAND_PAGE_SIZE_BYTES);
#if CONFIG_QSPI_QUAD_WIRE
		ret = bk_qspi_flash_nand_page_program_quad(qspi_id, page, 0, wr_buf, NAND_PAGE_SIZE_BYTES);
#else
		ret = bk_qspi_flash_nand_page_program(qspi_id, page, 0, wr_buf, NAND_PAGE_SIZE_BYTES);
#endif
		if (ret != BK_OK) { CLI_LOGE("program fail\r\n"); goto pt_end; }
		os_memset(rd_buf, 0, NAND_PAGE_SIZE_BYTES);
#if CONFIG_QSPI_QUAD_WIRE
		ret = bk_qspi_flash_nand_page_read_quad(qspi_id, page, 0, rd_buf, NAND_PAGE_SIZE_BYTES);
#else
		ret = bk_qspi_flash_nand_page_read(qspi_id, page, 0, rd_buf, NAND_PAGE_SIZE_BYTES);
#endif
		if (ret != BK_OK) { CLI_LOGE("read fail\r\n"); goto pt_end; }
		{
			bool pass = true;
			for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
				if (rd_buf[i] != 0xAA) { CLI_LOGE("0xAA fail at %u: 0x%02x\r\n", i, rd_buf[i]); pass = false; break; }
			}
			CLI_LOGI("0xAA verify: %s\r\n", pass ? "PASS" : "FAIL");
			if (!pass) goto pt_end;
		}
		ret = bk_qspi_flash_nand_block_erase(qspi_id, block);
		if (ret != BK_OK) { CLI_LOGE("erase fail\r\n"); goto pt_end; }
		os_memset(wr_buf, 0x55, NAND_PAGE_SIZE_BYTES);
#if CONFIG_QSPI_QUAD_WIRE
		ret = bk_qspi_flash_nand_page_program_quad(qspi_id, page, 0, wr_buf, NAND_PAGE_SIZE_BYTES);
#else
		ret = bk_qspi_flash_nand_page_program(qspi_id, page, 0, wr_buf, NAND_PAGE_SIZE_BYTES);
#endif
		if (ret != BK_OK) { CLI_LOGE("program fail\r\n"); goto pt_end; }
		os_memset(rd_buf, 0, NAND_PAGE_SIZE_BYTES);
#if CONFIG_QSPI_QUAD_WIRE
		ret = bk_qspi_flash_nand_page_read_quad(qspi_id, page, 0, rd_buf, NAND_PAGE_SIZE_BYTES);
#else
		ret = bk_qspi_flash_nand_page_read(qspi_id, page, 0, rd_buf, NAND_PAGE_SIZE_BYTES);
#endif
		if (ret != BK_OK) { CLI_LOGE("read fail\r\n"); goto pt_end; }
		{
			bool pass = true;
			for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
				if (rd_buf[i] != 0x55) { CLI_LOGE("0x55 fail at %u: 0x%02x\r\n", i, rd_buf[i]); pass = false; break; }
			}
			CLI_LOGI("0x55 verify: %s\r\n", pass ? "PASS" : "FAIL");
			if (!pass) goto pt_end;
		}
		bk_qspi_flash_nand_block_erase(qspi_id, block);
		CLI_LOGI("=== PAGE_TEST PASS ===\r\n");
pt_end:
		os_free(wr_buf);
		os_free(rd_buf);
	} else if (os_strcmp(subcmd, "block_test") == 0) {
		if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
		if (argc < 3) { CLI_LOGI("Usage: qspi_nand block_test <block>\r\n"); return; }
		uint32_t block = os_strtoul(argv[2], NULL, 0);
		bk_err_t ret;
		uint32_t first_page = block * NAND_BLOCK_PAGE_COUNT;
		CLI_LOGI("=== BLOCK_TEST: block=%u pages=%u~%u ===\r\n", block, first_page, first_page + NAND_BLOCK_PAGE_COUNT - 1);
		uint8_t *buf = (uint8_t *)os_malloc(NAND_PAGE_SIZE_BYTES);
		if (!buf) { CLI_LOGE("malloc failed\r\n"); return; }
		ret = bk_qspi_flash_nand_block_erase(qspi_id, block);
		if (ret != BK_OK) { CLI_LOGE("erase fail\r\n"); goto bt_end; }
		for (uint32_t p = 0; p < NAND_BLOCK_PAGE_COUNT; p++) {
			for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) buf[i] = (uint8_t)((p + i) & 0xFF);
#if CONFIG_QSPI_QUAD_WIRE
			ret = bk_qspi_flash_nand_page_program_quad(qspi_id, first_page + p, 0, buf, NAND_PAGE_SIZE_BYTES);
#else
			ret = bk_qspi_flash_nand_page_program(qspi_id, first_page + p, 0, buf, NAND_PAGE_SIZE_BYTES);
#endif
			if (ret != BK_OK) { CLI_LOGE("program page %u fail\r\n", first_page + p); goto bt_end; }
		}
		{
			bool all_pass = true;
			for (uint32_t p = 0; p < NAND_BLOCK_PAGE_COUNT; p++) {
				os_memset(buf, 0, NAND_PAGE_SIZE_BYTES);
#if CONFIG_QSPI_QUAD_WIRE
				ret = bk_qspi_flash_nand_page_read_quad(qspi_id, first_page + p, 0, buf, NAND_PAGE_SIZE_BYTES);
#else
				ret = bk_qspi_flash_nand_page_read(qspi_id, first_page + p, 0, buf, NAND_PAGE_SIZE_BYTES);
#endif
				if (ret != BK_OK) { CLI_LOGE("read page %u fail\r\n", first_page + p); all_pass = false; break; }
				for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
					if (buf[i] != (uint8_t)((p + i) & 0xFF)) {
						CLI_LOGE("page %u offset %u: expect 0x%02x got 0x%02x\r\n", first_page + p, i, (uint8_t)((p + i) & 0xFF), buf[i]);
						all_pass = false; break;
					}
				}
				if (!all_pass) break;
			}
			CLI_LOGI("Verify: %s\r\n", all_pass ? "PASS" : "FAIL");
		}
		bk_qspi_flash_nand_block_erase(qspi_id, block);
		CLI_LOGI("=== BLOCK_TEST PASS ===\r\n");
bt_end:
		os_free(buf);
	} else {
		cli_nand_usage();
	}
}
#endif /* CONFIG_QSPI_NAND_FLASH */

#define QSPI_CMD_CNT (sizeof(s_qspi_commands) / sizeof(struct cli_command))
static const struct cli_command s_qspi_commands[] = {
	{"qspi_driver", "qspi_driver {init|deinit}", cli_qspi_driver_cmd},
	{"qspi", "qspi {init|write|read}", cli_qspi_cmd},
#if CONFIG_QSPI_NAND_FLASH
	{"qspi_nand", "qspi_nand {init|get_id|...}", cli_qspi_nand_cmd},
#elif (CONFIG_QSPI_MST_FLASH)
	{"qspi_flash", "qspi_flash {write|read}", cli_qspi_flash_cmd},
#endif
};

int cli_qspi_init(void)
{
	BK_LOG_ON_ERR(bk_qspi_driver_init());
	return cli_register_commands(s_qspi_commands, QSPI_CMD_CNT);
}

#endif