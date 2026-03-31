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
#include <driver/qspi_psram.h>
#include "cli.h"
#include "qspi_hw.h"

#define PSRAM_TEST_START_ADDR(_id)         (QSPI_DCACHE_BASE_ADDR(_id))
#define PSRAM_TEST_LEN                (1024 * 10)

static void cli_qspi_help(void)
{
	CLI_LOGD("qspi_driver init\r\n");
	CLI_LOGD("qspi_driver deinit\r\n");
	CLI_LOGD("qspi <id> init <src_clk> <src_clk_div> <clk_div> - Initialize QSPI with clock configuration\r\n");
	CLI_LOGD("  src_clk: 0=160MHz, 1=240MHz\r\n");
	CLI_LOGD("  src_clk_div: 0~15 (actual divider = 1 + src_clk_div)\r\n");
	CLI_LOGD("  clk_div: 0~4 (divider factors: 1, 2, 4, 6, 8)\r\n");
	CLI_LOGD("  Example: qspi 0 init 0 3 1  (160M/(1+3)/2 = 20MHz)\r\n");
	CLI_LOGD("qspi enter_quad_mode\r\n");
	CLI_LOGD("qspi exit_quad_mode\r\n");
	CLI_LOGD("qspi quad_write\r\n");
	CLI_LOGD("qspi quad_read\r\n");
	CLI_LOGD("qspi compare\r\n");
	CLI_LOGD("qspi_flash get_id\r\n");
	CLI_LOGD("qspi_flash erase 0 256\r\n");
	CLI_LOGD("qspi_flash single_write 0 256\r\n");
	CLI_LOGD("qspi_flash single_read 0 256\r\n");
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
		if (argc < 6) {
			CLI_LOGE("Usage: qspi <id> init <src_clk> <src_clk_div> <clk_div>\r\n");
			CLI_LOGE("  src_clk: 0=160MHz, 1=240MHz\r\n");
			CLI_LOGE("  src_clk_div: 0~15 (actual divider = 1 + src_clk_div)\r\n");
			CLI_LOGE("  clk_div: 0~4 (divider factors: 1, 2, 4, 6, 8)\r\n");
			CLI_LOGE("  Example: qspi 0 init 0 3 1  (160M/(1+3)/2 = 20MHz)\r\n");
			return;
		}

		// Ensure driver is initialized first
		BK_LOG_ON_ERR(bk_qspi_driver_init());

		uint32_t src_clk = os_strtoul(argv[3], NULL, 10);
		uint32_t src_clk_div = os_strtoul(argv[4], NULL, 10);
		uint32_t clk_div = os_strtoul(argv[5], NULL, 10);

		// Validate parameters
		if (src_clk > 1) {
			CLI_LOGE("Invalid src_clk: %d (must be 0 or 1)\r\n", src_clk);
			return;
		}
		if (src_clk_div > 15) {
			CLI_LOGE("Invalid src_clk_div: %d (must be 0~15)\r\n", src_clk_div);
			return;
		}
		if (clk_div > 4) {
			CLI_LOGE("Invalid clk_div: %d (must be 0~4)\r\n", clk_div);
			return;
		}

		qspi_config_t config = {0};
		config.src_clk = (qspi_src_clk_t)src_clk;
		config.src_clk_div = src_clk_div;
		config.clk_div = clk_div;

		// Calculate and display actual frequency
		uint32_t base_clocks[] = {160, 240}; // MHz
		uint32_t clk_div_factors[] = {1, 2, 4, 6, 8};
		uint32_t base_clk = base_clocks[src_clk];
		uint32_t first_div_clk = base_clk / (1 + src_clk_div);
		uint32_t actual_freq = first_div_clk / clk_div_factors[clk_div];

		CLI_LOGD("Config: src_clk=%s, src_clk_div=%d, clk_div=%d\r\n",
		         (src_clk == 0) ? "160M" : "240M", src_clk_div, clk_div);
		CLI_LOGD("Calculation: %dM / (1+%d) / %d = %dMHz\r\n",
		         base_clk, src_clk_div, clk_div_factors[clk_div], actual_freq);

		BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &config));
		CLI_LOGD("qspi init success\r\n");
	} else if (os_strcmp(argv[2], "flash_test") == 0) {
		extern void test_qspi_flash(uint32_t id, uint32_t base_addr, uint32_t buf_len);
		uint32_t base_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t buf_len = os_strtoul(argv[4], NULL, 10);
		test_qspi_flash(qspi_id, base_addr, buf_len);
		CLI_LOGD("qspi flash test end\r\n");
	} else if (os_strcmp(argv[2], "flash_write_read_test") == 0) {
		if (argc < 5) {
			CLI_LOGE("Usage: qspi <id> flash_write_read_test <addr> <size>\r\n");
			CLI_LOGE("  Example: qspi 0 flash_write_read_test 0x0 256\r\n");
			return;
		}

		uint32_t test_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t test_size = os_strtoul(argv[4], NULL, 10);

		if (test_size == 0 || test_size > (64 * 1024)) {
			CLI_LOGE("Invalid test size: %d (max: 64KB)\r\n", test_size);
			return;
		}

		// Allocate buffers
		uint8_t *write_buf = (uint8_t *)os_malloc(test_size);
		uint8_t *read_buf = (uint8_t *)os_zalloc(test_size);

		if (!write_buf || !read_buf) {
			CLI_LOGE("Failed to allocate buffers (size: %d)\r\n", test_size);
			if (write_buf) os_free(write_buf);
			if (read_buf) os_free(read_buf);
			return;
		}

		// Generate test pattern (incrementing pattern)
		for (uint32_t i = 0; i < test_size; i++) {
			write_buf[i] = (uint8_t)(i & 0xFF);
		}

		CLI_LOGD("Flash Write/Read Test: addr=0x%08X, size=%d bytes\r\n", test_addr, test_size);

		// Step 0: Read Flash ID
		CLI_LOGD("Step 0: Reading Flash ID...\r\n");
		uint32_t flash_id = bk_qspi_flash_read_id(qspi_id);
		CLI_LOGD("Flash ID: 0x%06X\r\n", flash_id & 0xFFFFFF);

		// Step 1: Clear protection
		CLI_LOGD("Step 1: Clearing Flash protection...\r\n");
		bk_err_t ret = bk_qspi_flash_set_protect_none(qspi_id);
		if (ret != BK_OK) {
			CLI_LOGE("Clear protection failed: %d\r\n", ret);
			goto cleanup;
		}
		CLI_LOGD("Protection cleared\r\n");

		// Verify protection bits are cleared by reading status register
		uint32_t status_s0_s7 = bk_qspi_flash_read_s0_s7(qspi_id);
		uint32_t status_s8_s15 = bk_qspi_flash_read_s8_s15(qspi_id);
		uint32_t status_s16_s23 = bk_qspi_flash_read_s16_s23(qspi_id);
		uint8_t bp_bits = (status_s0_s7 >> 2) & 0x1F;  // BP4-BP0 (bits 6-2)
		CLI_LOGD("Status after protection clear: S0-S7=0x%02X, S8-S15=0x%02X, S16-S23=0x%02X, BP bits=0x%02X\r\n",
		         (uint8_t)status_s0_s7, (uint8_t)status_s8_s15, (uint8_t)status_s16_s23, bp_bits);
		if (bp_bits != 0) {
			CLI_LOGE("Warning: Protection bits not fully cleared! BP bits=0x%02X\r\n", bp_bits);
		}

		// Read and modify DRV1/DRV0 bits (S22/S21) in S16-S23 status register
		// S22 = bit 6, S21 = bit 5 (S16 is bit 0)
		CLI_LOGD("Step 1.1: Reading DRV1/DRV0 bits from S16-S23 status register...\r\n");
		uint8_t drv1 = (status_s16_s23 >> 6) & 0x01;  // S22 (bit 6)
		uint8_t drv0 = (status_s16_s23 >> 5) & 0x01;  // S21 (bit 5)
		CLI_LOGD("Before modification: DRV1=%d, DRV0=%d (S16-S23=0x%02X)\r\n", drv1, drv0, (uint8_t)status_s16_s23);

		// Clear DRV1 and DRV0 bits (set to 0)
		uint8_t modified_s16_s23 = (uint8_t)status_s16_s23;
		modified_s16_s23 &= ~(0x03 << 5);  // Clear bits 6 and 5 (DRV1 and DRV0)
		CLI_LOGD("Step 1.2: Clearing DRV1 and DRV0 bits (writing S16-S23=0x%02X)...\r\n", modified_s16_s23);
		ret = bk_qspi_flash_write_s16_s23(qspi_id, modified_s16_s23);
		if (ret != BK_OK) {
			CLI_LOGE("Write S16-S23 failed: %d\r\n", ret);
			goto cleanup;
		}
		CLI_LOGD("Write S16-S23 completed\r\n");

		// Read back to verify DRV1 and DRV0 are cleared
		CLI_LOGD("Step 1.3: Reading back to verify DRV1/DRV0 bits...\r\n");
		uint32_t status_s16_s23_after = bk_qspi_flash_read_s16_s23(qspi_id);
		uint8_t drv1_after = (status_s16_s23_after >> 6) & 0x01;  // S22 (bit 6)
		uint8_t drv0_after = (status_s16_s23_after >> 5) & 0x01;  // S21 (bit 5)
		CLI_LOGD("After modification: DRV1=%d, DRV0=%d (S16-S23=0x%02X)\r\n",
		         drv1_after, drv0_after, (uint8_t)status_s16_s23_after);
		if (drv1_after != 0 || drv0_after != 0) {
			CLI_LOGE("Warning: DRV1/DRV0 bits not cleared! DRV1=%d, DRV0=%d\r\n", drv1_after, drv0_after);
		} else {
			CLI_LOGD("DRV1 and DRV0 successfully cleared\r\n");
		}

		// Step 1.5: Enable Quad mode
		CLI_LOGD("Step 1.5: Enabling Quad mode...\r\n");
		ret = bk_qspi_flash_quad_enable(qspi_id);
		if (ret != BK_OK) {
			CLI_LOGE("Quad enable failed: %d\r\n", ret);
			goto cleanup;
		}
		CLI_LOGD("Quad mode enabled\r\n");

		// Step 2: Erase sector
		// Flash sector size is 4KB (0x1000), erase command will erase the entire sector
		// containing the test address
		uint32_t sector_base = (test_addr / 0x1000) * 0x1000;  // Align to sector boundary

		// Read data before erase for debugging
		uint8_t pre_erase_buf[4] = {0};
		ret = bk_qspi_flash_read(qspi_id, sector_base, pre_erase_buf, 4);
		if (ret == BK_OK) {
			CLI_LOGD("Step 2: Before erase at 0x%08X: %02X %02X %02X %02X\r\n",
			         sector_base, pre_erase_buf[0], pre_erase_buf[1], pre_erase_buf[2], pre_erase_buf[3]);
		}

		CLI_LOGD("Step 2: Erasing sector at 0x%08X (sector base: 0x%08X)...\r\n", test_addr, sector_base);
		ret = bk_qspi_flash_erase_sector(qspi_id, sector_base);
		if (ret != BK_OK) {
			CLI_LOGE("Erase failed: %d\r\n", ret);
			goto cleanup;
		}
		CLI_LOGD("Erase completed\r\n");

		// Step 2.5: Verify erase by reading first 4 bytes at sector base
		CLI_LOGD("Step 2.5: Verifying erase (reading first 4 bytes at sector base 0x%08X)...\r\n", sector_base);
		uint8_t verify_buf[4] = {0};
		ret = bk_qspi_flash_read(qspi_id, sector_base, verify_buf, 4);
		if (ret != BK_OK) {
			CLI_LOGE("Verify read at sector base failed: %d\r\n", ret);
			goto cleanup;
		}
		CLI_LOGD("After erase at 0x%08X: %02X %02X %02X %02X\r\n",
		         sector_base, verify_buf[0], verify_buf[1], verify_buf[2], verify_buf[3]);

		// Check if all bytes are 0xFF
		bool erase_ok = true;
		for (int i = 0; i < 4; i++) {
			if (verify_buf[i] != 0xFF) {
				erase_ok = false;
				break;
			}
		}

		if (!erase_ok) {
			CLI_LOGE("Erase verification failed at sector base 0x%08X: %02X %02X %02X %02X (expected all 0xFF)\r\n",
			         sector_base, verify_buf[0], verify_buf[1], verify_buf[2], verify_buf[3]);
			CLI_LOGE("Flash may be protected or erase command failed. Check protection bits.\r\n");
			goto cleanup;
		}

		// Also verify at test address if different from sector base
		if (test_addr != sector_base) {
			CLI_LOGD("Step 2.5: Verifying erase at test address 0x%08X...\r\n", test_addr);
			ret = bk_qspi_flash_read(qspi_id, test_addr, verify_buf, 2);
			if (ret != BK_OK) {
				CLI_LOGE("Verify read at test address failed: %d\r\n", ret);
				goto cleanup;
			}
			if (verify_buf[0] != 0xFF || verify_buf[1] != 0xFF) {
				CLI_LOGE("Erase verification failed at test address: first byte=0x%02X, second byte=0x%02X (expected 0xFF)\r\n",
				         verify_buf[0], verify_buf[1]);
				goto cleanup;
			}
		}
		CLI_LOGD("Erase verification passed: first 4 bytes are 0xFF\r\n");

		// Step 3: Write data
		CLI_LOGD("Step 3: Writing %d bytes to 0x%08X...\r\n", test_size, test_addr);
		CLI_LOGD("First 16 bytes to write: ");
		for (int i = 0; i < 16 && i < test_size; i++) {
			CLI_LOGD("%02X ", write_buf[i]);
		}
		CLI_LOGD("\r\n");

		ret = bk_qspi_flash_write(qspi_id, test_addr, write_buf, test_size);
		if (ret != BK_OK) {
			CLI_LOGE("Write failed: %d\r\n", ret);
			goto cleanup;
		}
		CLI_LOGD("Write completed\r\n");

		// Verify write immediately after write
		CLI_LOGD("Step 3.5: Verifying write immediately after write...\r\n");
		// Wait a bit more to ensure write is fully completed
		rtos_delay_milliseconds(10);
		uint8_t verify_write_buf[16] = {0};
		ret = bk_qspi_flash_read(qspi_id, test_addr, verify_write_buf, 16);
		if (ret == BK_OK) {
			CLI_LOGD("First 16 bytes read after write: ");
			for (int i = 0; i < 16; i++) {
				CLI_LOGD("%02X ", verify_write_buf[i]);
			}
			CLI_LOGD("\r\n");
			// Check if write was successful
			if (verify_write_buf[0] != write_buf[0]) {
				CLI_LOGE("Warning: Write may have failed! Expected 0x%02X, got 0x%02X\r\n",
				         write_buf[0], verify_write_buf[0]);
				CLI_LOGE("Check Flash protection bits (SRP0/SRP1) and Quad mode (QE bit)\r\n");
			}
		}

		// Step 4: Read data
		CLI_LOGD("Step 4: Reading %d bytes from 0x%08X...\r\n", test_size, test_addr);
		ret = bk_qspi_flash_read(qspi_id, test_addr, read_buf, test_size);
		if (ret != BK_OK) {
			CLI_LOGE("Read failed: %d\r\n", ret);
			goto cleanup;
		}
		CLI_LOGD("Read completed\r\n");

		// Step 5: Verify data
		CLI_LOGD("Step 5: Verifying data...\r\n");
		uint32_t error_count = 0;
		uint32_t first_error_offset = 0;
		uint8_t first_error_write = 0;
		uint8_t first_error_read = 0;

		for (uint32_t i = 0; i < test_size; i++) {
			if (write_buf[i] != read_buf[i]) {
				if (error_count == 0) {
					first_error_offset = i;
					first_error_write = write_buf[i];
					first_error_read = read_buf[i];
				}
				error_count++;
			}
		}

		// Print results
		if (error_count == 0) {
			CLI_LOGD("✓ Test PASSED: All %d bytes verified successfully\r\n", test_size);
		} else {
			CLI_LOGE("✗ Test FAILED: %d errors found\r\n", error_count);
			CLI_LOGE("  First error at offset 0x%X: wrote 0x%02X, read 0x%02X\r\n",
			         first_error_offset, first_error_write, first_error_read);

			// Print first 16 bytes of both buffers for debugging
			CLI_LOGD("  First 16 bytes written: ");
			for (uint32_t i = 0; i < 16 && i < test_size; i++) {
				CLI_LOGD("%02X ", write_buf[i]);
			}
			CLI_LOGD("\r\n");
			CLI_LOGD("  First 16 bytes read:    ");
			for (uint32_t i = 0; i < 16 && i < test_size; i++) {
				CLI_LOGD("%02X ", read_buf[i]);
			}
			CLI_LOGD("\r\n");
		}

cleanup:
		if (write_buf) os_free(write_buf);
		if (read_buf) os_free(read_buf);
		CLI_LOGD("Flash Write/Read Test completed\r\n");
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
			CLI_LOGD("read_buf[%d]=%x\r\n", i, rd_buf[i]);
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

			CLI_LOGD("dump read flash data:\r\n");
			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_DUMP_OUT(NULL, "%02x ", buf[i * 16 + j]);
				}
				BK_DUMP_OUT(NULL, "\r\n");
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

			CLI_LOGD("dump read flash data:\r\n");
			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_DUMP_OUT(NULL, "%02x ", buf[i * 16 + j]);
				}
				BK_DUMP_OUT(NULL, "\r\n");
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
		bk_qspi_flash_quad_enable(qspi_id);
		CLI_LOGD("flash_id:%x\r\n", flash_id);
	}
}

#define QSPI_CMD_CNT (sizeof(s_qspi_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_qspi_commands[] = {
	{"qspi_driver", "qspi_driver {init|deinit}", cli_qspi_driver_cmd},
	{"qspi", "qspi {init|write|read}", cli_qspi_cmd},
	{"qspi_flash", "qspi_flash {write|read}", cli_qspi_flash_cmd},
};

int bk_qspi_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_qspi_driver_init());
	return cli_register_module_test_feature(s_qspi_commands, QSPI_CMD_CNT);
}

