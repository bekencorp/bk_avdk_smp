// Copyright 2020-2025 Beken
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

// QSPI NOR/NAND flash CLI test commands.
//
// Moved out of middleware/driver/qspi/qspi_test.c together with the flash and
// NAND protocol engines: the engines now live in this component, so keeping the
// flash/NAND CLI here avoids middleware depending back on bk_external_flash. The
// controller-level and QSPI-PSRAM CLI stays in middleware qspi_test.c.
//
// The command tables use DRV_CLI_CMD_EXPORT: they are auto-registered by the
// CLI core walking the .cli_cmdtabl linker section, no explicit register call
// is required.

#include <soc/soc.h>
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "cli.h"
#if CONFIG_QSPI_NAND_FLASH
#include "qspi_nand_flash.h"
#include <driver/qspi_nand_dev.h>
#include <driver/qspi_nand_bbm.h>
#include <driver/aon_rtc.h>
#if CONFIG_TASK_WDT
#include "bk_wdt.h"
#endif
#endif

#if (CONFIG_QSPI_NOR_FLASH)
#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_SIZE 0x1000

/* Status-register write helpers are non-static in qspi_flash.c but only
 * write_s16_s23 is exported in qspi_flash.h; declare the rest locally. */
extern bk_err_t bk_qspi_flash_write_s0_s7(qspi_id_t id, uint8_t status_reg_data);
extern bk_err_t bk_qspi_flash_write_s8_s15(qspi_id_t id, uint8_t status_reg_data);
extern void test_qspi_flash(qspi_id_t id, uint32_t base_addr, uint32_t buf_len);

static void cli_qspi_flash_usage(void)
{
	CLI_LOGD("qspi_flash <id> get_id\r\n");
	CLI_LOGD("qspi_flash <id> erase <addr> <len>\r\n");
	CLI_LOGD("qspi_flash <id> read <addr> <len>\r\n");
	CLI_LOGD("qspi_flash <id> write <addr> <len>\r\n");
	CLI_LOGD("qspi_flash <id> single_read <addr> <len>\r\n");
	CLI_LOGD("qspi_flash <id> single_write <addr> <len>\r\n");
	CLI_LOGD("qspi_flash <id> read_sr [reg]   - read status reg (reg 0=S0-S7,1=S8-S15,2=S16-S23; omit=all)\r\n");
	CLI_LOGD("qspi_flash <id> write_sr <reg> <value_hex> - write status reg, e.g. write_sr 1 02\r\n");
	CLI_LOGD("qspi_flash <id> flash_test <addr> <len>\r\n");
	CLI_LOGD("qspi_flash <id> flash_write_read_test <addr> <size>\r\n");
}

static void cli_qspi_flash_write_read_test(uint32_t qspi_id, int argc, char **argv)
{
	if (argc < 5) {
		CLI_LOGE("Usage: qspi_flash <id> flash_write_read_test <addr> <size>\r\n");
		CLI_LOGE("  Example: qspi_flash 0 flash_write_read_test 0x0 256\r\n");
		return;
	}

	uint32_t test_addr = os_strtoul(argv[3], NULL, 16);
	uint32_t test_size = os_strtoul(argv[4], NULL, 10);

	if (test_size == 0 || test_size > (64 * 1024)) {
		CLI_LOGE("Invalid test size: %d (max: 64KB)\r\n", test_size);
		return;
	}

	uint8_t *write_buf = (uint8_t *)os_malloc(test_size);
	uint8_t *read_buf = (uint8_t *)os_zalloc(test_size);

	if (!write_buf || !read_buf) {
		CLI_LOGE("Failed to allocate buffers (size: %d)\r\n", test_size);
		if (write_buf) os_free(write_buf);
		if (read_buf) os_free(read_buf);
		return;
	}

	for (uint32_t i = 0; i < test_size; i++) {
		write_buf[i] = (uint8_t)(i & 0xFF);
	}

	CLI_LOGD("Flash Write/Read Test: addr=0x%08X, size=%d bytes\r\n", test_addr, test_size);

	CLI_LOGD("Step 0: Reading Flash ID...\r\n");
	uint32_t flash_id = bk_qspi_flash_read_id(qspi_id);
	CLI_LOGD("Flash ID: 0x%06X\r\n", flash_id & 0xFFFFFF);

	CLI_LOGD("Step 1: Clearing Flash protection...\r\n");
	bk_err_t ret = bk_qspi_flash_set_protect_none(qspi_id);
	if (ret != BK_OK) {
		CLI_LOGE("Clear protection failed: %d\r\n", ret);
		goto cleanup;
	}
	CLI_LOGD("Protection cleared\r\n");

	uint32_t status_s0_s7 = bk_qspi_flash_read_s0_s7(qspi_id);
	uint32_t status_s8_s15 = bk_qspi_flash_read_s8_s15(qspi_id);
	uint32_t status_s16_s23 = bk_qspi_flash_read_s16_s23(qspi_id);
	uint8_t bp_bits = (status_s0_s7 >> 2) & 0x1F;
	CLI_LOGD("Status after protection clear: S0-S7=0x%02X, S8-S15=0x%02X, S16-S23=0x%02X, BP bits=0x%02X\r\n",
	         (uint8_t)status_s0_s7, (uint8_t)status_s8_s15, (uint8_t)status_s16_s23, bp_bits);
	if (bp_bits != 0) {
		CLI_LOGE("Warning: Protection bits not fully cleared! BP bits=0x%02X\r\n", bp_bits);
	}

	CLI_LOGD("Step 1.1: Reading DRV1/DRV0 bits from S16-S23 status register...\r\n");
	uint8_t drv1 = (status_s16_s23 >> 6) & 0x01;
	uint8_t drv0 = (status_s16_s23 >> 5) & 0x01;
	CLI_LOGD("Before modification: DRV1=%d, DRV0=%d (S16-S23=0x%02X)\r\n", drv1, drv0, (uint8_t)status_s16_s23);

	uint8_t modified_s16_s23 = (uint8_t)status_s16_s23;
	modified_s16_s23 &= ~(0x03 << 5);
	CLI_LOGD("Step 1.2: Clearing DRV1 and DRV0 bits (writing S16-S23=0x%02X)...\r\n", modified_s16_s23);
	ret = bk_qspi_flash_write_s16_s23(qspi_id, modified_s16_s23);
	if (ret != BK_OK) {
		CLI_LOGE("Write S16-S23 failed: %d\r\n", ret);
		goto cleanup;
	}
	CLI_LOGD("Write S16-S23 completed\r\n");

	CLI_LOGD("Step 1.3: Reading back to verify DRV1/DRV0 bits...\r\n");
	uint32_t status_s16_s23_after = bk_qspi_flash_read_s16_s23(qspi_id);
	uint8_t drv1_after = (status_s16_s23_after >> 6) & 0x01;
	uint8_t drv0_after = (status_s16_s23_after >> 5) & 0x01;
	CLI_LOGD("After modification: DRV1=%d, DRV0=%d (S16-S23=0x%02X)\r\n",
	         drv1_after, drv0_after, (uint8_t)status_s16_s23_after);
	if (drv1_after != 0 || drv0_after != 0) {
		CLI_LOGE("Warning: DRV1/DRV0 bits not cleared! DRV1=%d, DRV0=%d\r\n", drv1_after, drv0_after);
	} else {
		CLI_LOGD("DRV1 and DRV0 successfully cleared\r\n");
	}

	CLI_LOGD("Step 1.5: Enabling Quad mode...\r\n");
	ret = bk_qspi_flash_quad_enable(qspi_id);
	if (ret != BK_OK) {
		CLI_LOGE("Quad enable failed: %d\r\n", ret);
		goto cleanup;
	}
	CLI_LOGD("Quad mode enabled\r\n");

	uint32_t sector_base = (test_addr / 0x1000) * 0x1000;

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

	CLI_LOGD("Step 2.5: Verifying erase (reading first 4 bytes at sector base 0x%08X)...\r\n", sector_base);
	uint8_t verify_buf[4] = {0};
	ret = bk_qspi_flash_read(qspi_id, sector_base, verify_buf, 4);
	if (ret != BK_OK) {
		CLI_LOGE("Verify read at sector base failed: %d\r\n", ret);
		goto cleanup;
	}
	CLI_LOGD("After erase at 0x%08X: %02X %02X %02X %02X\r\n",
	         sector_base, verify_buf[0], verify_buf[1], verify_buf[2], verify_buf[3]);

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

	CLI_LOGD("Step 3.5: Verifying write immediately after write...\r\n");
	uint8_t verify_write_buf[16] = {0};
	ret = bk_qspi_flash_read(qspi_id, test_addr, verify_write_buf, 16);
	if (ret == BK_OK) {
		CLI_LOGD("First 16 bytes read after write: ");
		for (int i = 0; i < 16; i++) {
			CLI_LOGD("%02X ", verify_write_buf[i]);
		}
		CLI_LOGD("\r\n");
		if (verify_write_buf[0] != write_buf[0]) {
			CLI_LOGE("Warning: Write may have failed! Expected 0x%02X, got 0x%02X\r\n",
			         write_buf[0], verify_write_buf[0]);
			CLI_LOGE("Check Flash protection bits (SRP0/SRP1) and Quad mode (QE bit)\r\n");
		}
	}

	CLI_LOGD("Step 4: Reading %d bytes from 0x%08X...\r\n", test_size, test_addr);
	ret = bk_qspi_flash_read(qspi_id, test_addr, read_buf, test_size);
	if (ret != BK_OK) {
		CLI_LOGE("Read failed: %d\r\n", ret);
		goto cleanup;
	}
	CLI_LOGD("Read completed\r\n");

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

	if (error_count == 0) {
		CLI_LOGD("Test PASSED: All %d bytes verified successfully\r\n", test_size);
	} else {
		CLI_LOGE("Test FAILED: %d errors found\r\n", error_count);
		CLI_LOGE("  First error at offset 0x%X: wrote 0x%02X, read 0x%02X\r\n",
		         first_error_offset, first_error_write, first_error_read);

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
}

static void cli_qspi_flash_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
		cli_qspi_flash_usage();
		return;
	}

	uint32_t qspi_id = os_strtoul(argv[1], NULL, 10);

	if (os_strcmp(argv[2], "get_id") == 0) {
		uint32_t flash_id = bk_qspi_flash_read_id(qspi_id);
		bk_qspi_flash_set_protect_none(qspi_id);
		bk_qspi_flash_quad_enable(qspi_id);
		CLI_LOGD("flash_id:%x\r\n", flash_id);
		return;
	}

	if (os_strcmp(argv[2], "read_sr") == 0) {
		/* Optional reg index (0=S0-S7, 1=S8-S15, 2=S16-S23); no arg = read all. */
		uint32_t s0_s7 = bk_qspi_flash_read_s0_s7(qspi_id);
		uint32_t s8_s15 = bk_qspi_flash_read_s8_s15(qspi_id);
		uint32_t s16_s23 = bk_qspi_flash_read_s16_s23(qspi_id);
		if (argc >= 4) {
			uint32_t reg = os_strtoul(argv[3], NULL, 10);
			switch (reg) {
			case 0: CLI_LOGD("flash SR0 (S0-S7)  =0x%02X\r\n", (uint8_t)s0_s7); break;
			case 1: CLI_LOGD("flash SR1 (S8-S15) =0x%02X\r\n", (uint8_t)s8_s15); break;
			case 2: CLI_LOGD("flash SR2 (S16-S23)=0x%02X\r\n", (uint8_t)s16_s23); break;
			default: CLI_LOGE("Invalid reg %d (0=S0-S7,1=S8-S15,2=S16-S23)\r\n", reg); break;
			}
		} else {
			CLI_LOGD("flash status: S0-S7=0x%02X S8-S15=0x%02X S16-S23=0x%02X\r\n",
			         (uint8_t)s0_s7, (uint8_t)s8_s15, (uint8_t)s16_s23);
		}
		return;
	}

	if (os_strcmp(argv[2], "write_sr") == 0) {
		if (argc < 5) {
			CLI_LOGE("Usage: qspi_flash <id> write_sr <reg:0|1|2> <value_hex>\r\n");
			CLI_LOGE("  reg 0=S0-S7, 1=S8-S15, 2=S16-S23\r\n");
			CLI_LOGE("  Example: qspi_flash 0 write_sr 1 02  (set QE bit)\r\n");
			return;
		}
		uint32_t reg = os_strtoul(argv[3], NULL, 10);
		uint8_t value = (uint8_t)os_strtoul(argv[4], NULL, 16);
		bk_err_t ret;
		switch (reg) {
		case 0: ret = bk_qspi_flash_write_s0_s7(qspi_id, value); break;
		case 1: ret = bk_qspi_flash_write_s8_s15(qspi_id, value); break;
		case 2: ret = bk_qspi_flash_write_s16_s23(qspi_id, value); break;
		default:
			CLI_LOGE("Invalid reg %d (0=S0-S7,1=S8-S15,2=S16-S23)\r\n", reg);
			return;
		}
		if (ret != BK_OK) {
			CLI_LOGE("write_sr reg %d = 0x%02X failed: %d\r\n", reg, value, ret);
			return;
		}
		uint8_t readback = 0;
		switch (reg) {
		case 0: readback = (uint8_t)bk_qspi_flash_read_s0_s7(qspi_id); break;
		case 1: readback = (uint8_t)bk_qspi_flash_read_s8_s15(qspi_id); break;
		case 2: readback = (uint8_t)bk_qspi_flash_read_s16_s23(qspi_id); break;
		}
		CLI_LOGD("write_sr reg %d = 0x%02X done, readback=0x%02X\r\n", reg, value, readback);
		return;
	}

	if (os_strcmp(argv[2], "flash_test") == 0) {
		if (argc < 5) {
			CLI_LOGE("Usage: qspi_flash <id> flash_test <addr> <len>\r\n");
			return;
		}
		uint32_t base_addr = os_strtoul(argv[3], NULL, 16);
		uint32_t buf_len = os_strtoul(argv[4], NULL, 10);
		test_qspi_flash(qspi_id, base_addr, buf_len);
		CLI_LOGD("qspi flash test end\r\n");
		return;
	}

	if (os_strcmp(argv[2], "flash_write_read_test") == 0) {
		cli_qspi_flash_write_read_test(qspi_id, argc, argv);
		return;
	}

	if (argc < 5) {
		cli_qspi_flash_usage();
		return;
	}

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

			CLI_LOGD("dump read flash data:\r\n");
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

	} else {
		cli_qspi_flash_usage();
	}
}
#endif /* CONFIG_QSPI_NOR_FLASH */

#if CONFIG_QSPI_NAND_FLASH
static bool s_nand_initialized = false;

/*
 * Render bytes as "xx xx xx" into a caller-supplied buffer.
 *
 * The log backend prefixes every CLI_LOGI call, so emitting one byte per call
 * turns a 16-byte dump into 16 prefixed fragments. Build the line first and
 * print it once. @p out must hold at least len*3 bytes.
 */
static void nand_hex_line(char *out, const uint8_t *buf, uint32_t len)
{
	static const char hex[] = "0123456789abcdef";
	uint32_t n = 0;

	for (uint32_t i = 0; i < len; i++) {
		out[n++] = hex[buf[i] >> 4];
		out[n++] = hex[buf[i] & 0x0F];
		out[n++] = ' ';
	}
	out[n ? n - 1 : 0] = '\0';
}

/*
 * Whole-die sweep used for incoming-material inspection.
 *
 * The filesystem-level tests only ever touch the littleFS partition (blocks
 * 0..7, 1.6% of a 512-block die) and concentrate all wear there, which is the
 * wrong shape for qualifying a batch of parts. This walks every block instead:
 * erase, program a per-page unique pattern, read back and compare, then erase
 * again so the part is handed over clean. Coverage is 100% at roughly two P/E
 * cycles per block, so a full pass costs about a minute and a negligible slice
 * of the 80K-cycle endurance budget.
 *
 * The same walk also runs as two halves for data-retention work: WRITE fills
 * the die and leaves the data in place, then VERIFY reads it back after the
 * part has been baked or power-cycled. The pattern is a pure function of
 * (tag, block, page), so the verify side needs no state carried across the
 * power cycle - it recomputes what every page should hold.
 */
typedef enum {
	NAND_SWEEP_FULL = 0,   /* erase -> program -> verify -> erase */
	NAND_SWEEP_WRITE,      /* erase -> program, data deliberately left behind */
	NAND_SWEEP_VERIFY,     /* read + compare only: no erase, no program */
} nand_sweep_phase_t;

typedef struct {
	uint32_t blocks_tested;
	uint32_t blocks_skipped;   /* factory-bad, never erased */
	uint32_t blocks_unwritten; /* verify: block still erased, nothing to check */
	uint32_t erase_fail;
	uint32_t program_fail;
	uint32_t read_fail;
	uint32_t verify_fail;
	uint32_t first_fail_block;
	bool     any_fail;
	uint64_t write_bytes;
	uint64_t read_bytes;
	uint64_t write_us;
	uint64_t read_us;
} nand_sweep_stat_t;

static inline uint32_t nand_sweep_lcg(uint32_t *s)
{
	*s = (*s * 1103515245u) + 12345u;
	return *s;
}

/* Unique payload per (tag, block, page): a constant or block-wide pattern
 * cannot catch a mis-addressed access, since every page would compare equal.
 * @p tag is the round index for a full sweep and the generation number for a
 * write/verify pair; either way the same tag reproduces the same bytes. */
static void nand_sweep_fill(uint8_t *buf, uint32_t tag, uint32_t block, uint32_t page)
{
	uint32_t seed = (tag * 2654435761u) ^ (block << 8) ^ page ^ 1u;

	for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
		buf[i] = (uint8_t)(nand_sweep_lcg(&seed) >> 24);
	}
}

static bool nand_sweep_page_is_erased(const uint8_t *buf)
{
	for (uint32_t i = 0; i < NAND_PAGE_SIZE_BYTES; i++) {
		if (buf[i] != 0xFF) {
			return false;
		}
	}
	return true;
}

/*
 * Feed the task watchdog and, crucially, yield.
 *
 * A whole-die pass is ~50 s of back-to-back QSPI transfers. Running that as an
 * uninterrupted CLI loop starved the tick path and the 2 s interrupt watchdog
 * (CONFIG_INT_WWDT_PERIOD_MS) reset the chip partway through. Feeding the 60 s
 * task watchdog alone is not enough - the loop has to give the scheduler air,
 * the same way the VFS soak task does between rounds. One millisecond per
 * block costs ~0.5 s over 512 blocks, about 1% of the sweep.
 */
static void nand_sweep_yield(void)
{
#if CONFIG_TASK_WDT
	bk_task_wdt_feed();
#endif
	rtos_delay_milliseconds(1);
}

static void nand_sweep_kbps(uint64_t bytes, uint64_t us, uint32_t *whole, uint32_t *tenth)
{
	uint64_t v;

	if (!us) {
		*whole = 0;
		*tenth = 0;
		return;
	}
	v = (bytes * 10000000ull) / us / 1024ull;
	*whole = (uint32_t)(v / 10ull);
	*tenth = (uint32_t)(v % 10ull);
}

/* One block, one phase. Never aborts the caller's walk; a failing block is
 * recorded and skipped so the operator gets the full bad-block list in one run. */
static void nand_sweep_block(qspi_id_t id, nand_sweep_phase_t phase, uint32_t tag,
                             uint32_t block, uint8_t *wr, uint8_t *rd,
                             nand_sweep_stat_t *st)
{
	uint32_t first_page = block * NAND_BLOCK_PAGE_COUNT;
	uint64_t t0;
	bool is_bad = false;

	/* Factory-bad blocks must never be erased: the marking may be
	 * unrecoverable afterwards (GD5F05 datasheet sec 12). VERIFY applies the
	 * same test so it walks exactly the set of blocks WRITE programmed. */
	if (bk_qspi_flash_nand_is_factory_bad(id, block, &is_bad) != BK_OK || is_bad) {
		st->blocks_skipped++;
		return;
	}

	if (phase != NAND_SWEEP_VERIFY) {
		if (bk_qspi_flash_nand_block_erase(id, block) != BK_OK) {
			CLI_LOGE("sweep: block %u erase FAIL\r\n", block);
			st->erase_fail++;
			goto failed;
		}

		for (uint32_t p = 0; p < NAND_BLOCK_PAGE_COUNT; p++) {
			nand_sweep_fill(wr, tag, block, p);

			t0 = bk_aon_rtc_get_us();
			bk_err_t ret = bk_qspi_flash_nand_page_program_quad(id, first_page + p, 0,
			                                                    wr, NAND_PAGE_SIZE_BYTES);
			st->write_us += bk_aon_rtc_get_us() - t0;
			if (ret != BK_OK) {
				CLI_LOGE("sweep: block %u page %u program FAIL\r\n", block, first_page + p);
				st->program_fail++;
				goto failed;
			}
			st->write_bytes += NAND_PAGE_SIZE_BYTES;
		}
	}

	if (phase != NAND_SWEEP_WRITE) {
		for (uint32_t p = 0; p < NAND_BLOCK_PAGE_COUNT; p++) {
			t0 = bk_aon_rtc_get_us();
			bk_err_t ret = bk_qspi_flash_nand_page_read_quad(id, first_page + p, 0,
			                                                 rd, NAND_PAGE_SIZE_BYTES);
			st->read_us += bk_aon_rtc_get_us() - t0;
			if (ret != BK_OK) {
				CLI_LOGE("sweep: block %u page %u read FAIL(%d)\r\n", block, first_page + p, ret);
				st->read_fail++;
				goto failed;
			}
			st->read_bytes += NAND_PAGE_SIZE_BYTES;

			nand_sweep_fill(wr, tag, block, p);
			if (os_memcmp(rd, wr, NAND_PAGE_SIZE_BYTES) != 0) {
				/* A block WRITE never reached still reads erased. Counting it
				 * separately keeps a range mismatch (or a tag typo) from being
				 * reported as retention corruption. Only VERIFY may do this:
				 * in the other phases the block was just programmed, so an
				 * erased page 0 is a program failure and must stay one. */
				if ((phase == NAND_SWEEP_VERIFY) && (p == 0) && nand_sweep_page_is_erased(rd)) {
					st->blocks_unwritten++;
					return;
				}

				/* Report the first differing byte only; a whole-die sweep must
				 * not drown the console on a single bad page. */
				uint32_t i = 0;
				while (i < NAND_PAGE_SIZE_BYTES && rd[i] == wr[i]) {
					i++;
				}
				CLI_LOGE("sweep: block %u page %u MISMATCH at %u exp 0x%02x got 0x%02x\r\n",
				         block, first_page + p, i, wr[i], rd[i]);
				st->verify_fail++;
				goto failed;
			}
		}
	}

	/* Hand the block back erased so the part is ready for a filesystem. WRITE
	 * must not do this - its whole point is to leave the data behind - and
	 * VERIFY must not touch the data it was asked to check. */
	if (phase == NAND_SWEEP_FULL) {
		BK_LOG_ON_ERR(bk_qspi_flash_nand_block_erase(id, block));
	}
	st->blocks_tested++;
	return;

failed:
	if (!st->any_fail) {
		st->any_fail = true;
		st->first_fail_block = block;
	}
	st->blocks_tested++;
}

static const char *nand_sweep_phase_name(nand_sweep_phase_t phase)
{
	switch (phase) {
	case NAND_SWEEP_WRITE:  return "SWEEP_WRITE";
	case NAND_SWEEP_VERIFY: return "SWEEP_VERIFY";
	default:                return "SWEEP";
	}
}

/*
 * Shared driver for `sweep`, `sweep_write` and `sweep_verify`.
 *
 * argv[2]/argv[3] are the block range. argv[4] is the round count for a full
 * sweep and the pattern tag for the write/verify pair: a verify must be given
 * the same tag as the write it checks, or every page mismatches.
 */
static void cli_nand_sweep(qspi_id_t id, nand_sweep_phase_t phase, int argc, char **argv)
{
	if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }

	uint32_t total_blocks = bk_qspi_flash_nand_total_size(id) / NAND_BLOCK_SIZE_BYTES;
	if (!total_blocks) { CLI_LOGE("sweep: unknown device size\r\n"); return; }

	const char *name = nand_sweep_phase_name(phase);
	bool full = (phase == NAND_SWEEP_FULL);
	uint32_t from = 0;
	uint32_t to = total_blocks - 1;
	uint32_t rounds = 1;
	uint32_t tag = 0;

	if (argc >= 3) { from = os_strtoul(argv[2], NULL, 0); }
	if (argc >= 4) { to = os_strtoul(argv[3], NULL, 0); }
	if (argc >= 5) {
		if (full) {
			rounds = os_strtoul(argv[4], NULL, 0);
		} else {
			tag = os_strtoul(argv[4], NULL, 0);
		}
	}

	if (from > to || to >= total_blocks || !rounds) {
		CLI_LOGE("%s: need 0 <= from <= to <= %u and rounds >= 1\r\n",
		         name, total_blocks - 1);
		return;
	}

	uint8_t *wr = (uint8_t *)os_malloc(NAND_PAGE_SIZE_BYTES);
	uint8_t *rd = (uint8_t *)os_malloc(NAND_PAGE_SIZE_BYTES);
	if (!wr || !rd) {
		CLI_LOGE("%s: no mem\r\n", name);
		if (wr) os_free(wr);
		if (rd) os_free(rd);
		return;
	}

	uint32_t ecc_corr_before = 0, ecc_unc_before = 0;
	uint32_t ecc_corr_after = 0, ecc_unc_after = 0;

	/* Snapshot rather than reset: another measurement may be in progress. */
	bk_qspi_flash_nand_get_ecc_stat(&ecc_corr_before, &ecc_unc_before);

	switch (phase) {
	case NAND_SWEEP_WRITE:
		CLI_LOGW("sweep_write: DESTRUCTIVE - erases and fills blocks %u..%u (tag %u), data left in place\r\n",
		         from, to, tag);
		break;
	case NAND_SWEEP_VERIFY:
		CLI_LOGI("sweep_verify: read-only, expecting tag %u pattern in blocks %u..%u\r\n",
		         tag, from, to);
		break;
	default:
		CLI_LOGW("sweep: DESTRUCTIVE - erases blocks %u..%u (%u round(s))\r\n",
		         from, to, rounds);
		break;
	}

	nand_sweep_stat_t st;
	os_memset(&st, 0, sizeof(st));
	uint64_t t_start = bk_aon_rtc_get_us();

	for (uint32_t r = 0; r < rounds; r++) {
		for (uint32_t b = from; b <= to; b++) {
			nand_sweep_block(id, phase, full ? r : tag, b, wr, rd, &st);
			nand_sweep_yield();

			/* Keep the operator informed without flooding the console. */
			if (((b - from) % 64U) == 63U) {
				CLI_LOGI("sweep: round %u/%u block %u/%u\r\n",
				         r + 1, rounds, b, to);
			}
		}
	}

	uint64_t elapsed = bk_aon_rtc_get_us() - t_start;
	bk_qspi_flash_nand_get_ecc_stat(&ecc_corr_after, &ecc_unc_after);

	uint32_t w_kbps, w_t, r_kbps, r_t;
	nand_sweep_kbps(st.write_bytes, st.write_us, &w_kbps, &w_t);
	nand_sweep_kbps(st.read_bytes, st.read_us, &r_kbps, &r_t);

	if (full) {
		CLI_LOGI("=== NAND SWEEP: blocks %u..%u x%u ===\r\n", from, to, rounds);
	} else {
		CLI_LOGI("=== NAND %s: blocks %u..%u tag %u ===\r\n", name, from, to, tag);
	}
	CLI_LOGI("  tested=%u factory_bad_skipped=%u elapsed=%llu us\r\n",
	         st.blocks_tested, st.blocks_skipped, (unsigned long long)elapsed);
	if (phase == NAND_SWEEP_VERIFY) {
		/* Printed even when zero: a retention verify is only meaningful if the
		 * operator can see that every block really did hold data to check. */
		CLI_LOGI("  unwritten(still erased)=%u\r\n", st.blocks_unwritten);
	}
	if (full || (st.write_bytes != 0)) {
		CLI_LOGI("  Write: %llu B, avg %u.%u KB/s\r\n",
		         (unsigned long long)st.write_bytes, w_kbps, w_t);
	}
	if (full || (st.read_bytes != 0)) {
		CLI_LOGI("  Read : %llu B, avg %u.%u KB/s\r\n",
		         (unsigned long long)st.read_bytes, r_kbps, r_t);
	}
	CLI_LOGI("  Fails: erase=%u program=%u read=%u verify=%u\r\n",
	         st.erase_fail, st.program_fail, st.read_fail, st.verify_fail);
	CLI_LOGI("  ECC delta: corrected=%u uncorrectable=%u\r\n",
	         ecc_corr_after - ecc_corr_before, ecc_unc_after - ecc_unc_before);
	if (st.any_fail) {
		CLI_LOGI("  first failing block: %u\r\n", st.first_fail_block);
	}
	if (st.blocks_unwritten) {
		CLI_LOGW("  %u block(s) were still erased: check the range and tag match the write phase\r\n",
		         st.blocks_unwritten);
	}
	CLI_LOGI("=== %s RESULT: %s ===\r\n", name, st.any_fail ? "FAIL" : "PASS");

	os_free(wr);
	os_free(rd);
}

static void nand_range_mark_fail(nand_sweep_stat_t *st, uint32_t block)
{
	if (!st->any_fail) {
		st->any_fail = true;
		st->first_fail_block = block;
	}
}

static bool nand_range_skip_bad(qspi_id_t id, uint32_t block, nand_sweep_stat_t *st)
{
	bool is_bad = false;

	if (bk_qspi_flash_nand_is_factory_bad(id, block, &is_bad) != BK_OK || is_bad) {
		st->blocks_skipped++;
		return true;
	}
	return false;
}

static bool nand_range_read_block(qspi_id_t id, uint32_t block, uint8_t *rd, nand_sweep_stat_t *st)
{
	uint32_t first_page = block * NAND_BLOCK_PAGE_COUNT;

	for (uint32_t p = 0; p < NAND_BLOCK_PAGE_COUNT; p++) {
		uint64_t t0 = bk_aon_rtc_get_us();
		bk_err_t ret = bk_qspi_flash_nand_page_read_quad(id, first_page + p, 0,
		                                                 rd, NAND_PAGE_SIZE_BYTES);
		st->read_us += bk_aon_rtc_get_us() - t0;
		if (ret != BK_OK) {
			CLI_LOGE("range_loop: block %u page %u pre-read FAIL(%d)\r\n", block, first_page + p, ret);
			st->read_fail++;
			nand_range_mark_fail(st, block);
			return false;
		}
		st->read_bytes += NAND_PAGE_SIZE_BYTES;
	}
	return true;
}

static bool nand_range_verify_erased(qspi_id_t id, uint32_t block, uint8_t *rd,
                                     nand_sweep_stat_t *st)
{
	uint32_t first_page = block * NAND_BLOCK_PAGE_COUNT;

	for (uint32_t p = 0; p < NAND_BLOCK_PAGE_COUNT; p++) {
		uint64_t t0 = bk_aon_rtc_get_us();
		bk_err_t ret = bk_qspi_flash_nand_page_read_quad(id, first_page + p, 0,
		                                                 rd, NAND_PAGE_SIZE_BYTES);
		st->read_us += bk_aon_rtc_get_us() - t0;
		if (ret != BK_OK) {
			CLI_LOGE("range_loop: block %u page %u erase-read FAIL(%d)\r\n", block, first_page + p, ret);
			st->read_fail++;
			nand_range_mark_fail(st, block);
			return false;
		}
		st->read_bytes += NAND_PAGE_SIZE_BYTES;
		if (!nand_sweep_page_is_erased(rd)) {
			CLI_LOGE("range_loop: block %u page %u erase verify FAIL\r\n", block, first_page + p);
			st->verify_fail++;
			nand_range_mark_fail(st, block);
			return false;
		}
	}
	return true;
}

static bool nand_range_program_block(qspi_id_t id, uint32_t tag, uint32_t block,
                                     uint8_t *wr, nand_sweep_stat_t *st)
{
	uint32_t first_page = block * NAND_BLOCK_PAGE_COUNT;

	for (uint32_t p = 0; p < NAND_BLOCK_PAGE_COUNT; p++) {
		nand_sweep_fill(wr, tag, block, p);
		uint64_t t0 = bk_aon_rtc_get_us();
		bk_err_t ret = bk_qspi_flash_nand_page_program_quad(id, first_page + p, 0,
		                                                    wr, NAND_PAGE_SIZE_BYTES);
		st->write_us += bk_aon_rtc_get_us() - t0;
		if (ret != BK_OK) {
			CLI_LOGE("range_loop: block %u page %u program FAIL(%d)\r\n", block, first_page + p, ret);
			st->program_fail++;
			nand_range_mark_fail(st, block);
			return false;
		}
		st->write_bytes += NAND_PAGE_SIZE_BYTES;
	}
	return true;
}

static bool nand_range_verify_block(qspi_id_t id, uint32_t tag, uint32_t block,
                                    uint8_t *wr, uint8_t *rd, nand_sweep_stat_t *st)
{
	uint32_t first_page = block * NAND_BLOCK_PAGE_COUNT;

	for (uint32_t p = 0; p < NAND_BLOCK_PAGE_COUNT; p++) {
		uint64_t t0 = bk_aon_rtc_get_us();
		bk_err_t ret = bk_qspi_flash_nand_page_read_quad(id, first_page + p, 0,
		                                                 rd, NAND_PAGE_SIZE_BYTES);
		st->read_us += bk_aon_rtc_get_us() - t0;
		if (ret != BK_OK) {
			CLI_LOGE("range_loop: block %u page %u readback FAIL(%d)\r\n", block, first_page + p, ret);
			st->read_fail++;
			nand_range_mark_fail(st, block);
			return false;
		}
		st->read_bytes += NAND_PAGE_SIZE_BYTES;
		nand_sweep_fill(wr, tag, block, p);
		if (os_memcmp(rd, wr, NAND_PAGE_SIZE_BYTES) != 0) {
			uint32_t i = 0;
			while (i < NAND_PAGE_SIZE_BYTES && rd[i] == wr[i]) {
				i++;
			}
			CLI_LOGE("range_loop: block %u page %u MISMATCH at %u exp 0x%02x got 0x%02x\r\n",
			         block, first_page + p, i, wr[i], rd[i]);
			st->verify_fail++;
			nand_range_mark_fail(st, block);
			return false;
		}
	}
	return true;
}

static void nand_range_loop_block(qspi_id_t id, uint32_t tag, uint32_t block,
                                  uint8_t *wr, uint8_t *rd, nand_sweep_stat_t *st)
{
	if (nand_range_skip_bad(id, block, st)) {
		return;
	}
	if (!nand_range_read_block(id, block, rd, st)) {
		st->blocks_tested++;
		return;
	}
	if (bk_qspi_flash_nand_block_erase(id, block) != BK_OK) {
		CLI_LOGE("range_loop: block %u erase FAIL\r\n", block);
		st->erase_fail++;
		nand_range_mark_fail(st, block);
		st->blocks_tested++;
		return;
	}
	if (nand_range_verify_erased(id, block, rd, st) &&
	    nand_range_program_block(id, tag, block, wr, st)) {
		(void)nand_range_verify_block(id, tag, block, wr, rd, st);
	}
	st->blocks_tested++;
}

static bool nand_range_args(qspi_id_t id, char **argv, uint32_t *from, uint32_t *to,
                            uint32_t *rounds)
{
	uint32_t total = bk_qspi_flash_nand_total_size(id);
	uint32_t offset = os_strtoul(argv[2], NULL, 0);
	uint32_t size = os_strtoul(argv[3], NULL, 0);

	*rounds = os_strtoul(argv[4], NULL, 0);
	if (!total || !size || !*rounds || offset > UINT32_MAX - size) {
		CLI_LOGE("range_loop: invalid offset/size/times\r\n");
		return false;
	}
	if ((offset & (NAND_BLOCK_SIZE_BYTES - 1U)) || (size & (NAND_BLOCK_SIZE_BYTES - 1U))) {
		CLI_LOGE("range_loop: offset and size must be 0x%x aligned\r\n", NAND_BLOCK_SIZE_BYTES);
		return false;
	}
	if (offset + size > total) {
		CLI_LOGE("range_loop: end 0x%x exceeds NAND size 0x%x\r\n", offset + size, total);
		return false;
	}
	*from = offset / NAND_BLOCK_SIZE_BYTES;
	*to = (offset + size) / NAND_BLOCK_SIZE_BYTES - 1U;
	return true;
}

static void cli_nand_range_loop(qspi_id_t id, int argc, char **argv)
{
	if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
	if (argc < 5) {
		CLI_LOGI("Usage: qspi_nand range_loop <offset> <size> <times> [tag] [print_cnt]\r\n");
		return;
	}

	uint32_t from, to, rounds;
	if (!nand_range_args(id, argv, &from, &to, &rounds)) {
		return;
	}
	uint32_t tag = (argc >= 6) ? os_strtoul(argv[5], NULL, 0) : 0;
	uint32_t print_cnt = (argc >= 7) ? os_strtoul(argv[6], NULL, 0) : 1;
	if (!print_cnt) {
		print_cnt = 1;
	}

	bk_err_t ret = bk_qspi_flash_nand_set_protect_none(id);
	if (ret != BK_OK) {
		CLI_LOGE("range_loop: protect_none FAIL(%d)\r\n", ret);
		return;
	}

	uint8_t *wr = (uint8_t *)os_malloc(NAND_PAGE_SIZE_BYTES);
	uint8_t *rd = (uint8_t *)os_malloc(NAND_PAGE_SIZE_BYTES);
	if (!wr || !rd) {
		CLI_LOGE("range_loop: no mem\r\n");
		if (wr) os_free(wr);
		if (rd) os_free(rd);
		return;
	}

	nand_sweep_stat_t st;
	os_memset(&st, 0, sizeof(st));
	uint32_t ecc_corr_before = 0, ecc_unc_before = 0;
	uint32_t ecc_corr_after = 0, ecc_unc_after = 0;
	bk_qspi_flash_nand_get_ecc_stat(&ecc_corr_before, &ecc_unc_before);
	CLI_LOGW("range_loop: DESTRUCTIVE - blocks %u..%u, %u round(s)\r\n", from, to, rounds);
	uint64_t t_start = bk_aon_rtc_get_us();

	for (uint32_t r = 0; r < rounds; r++) {
		for (uint32_t b = from; b <= to; b++) {
			nand_range_loop_block(id, tag + r, b, wr, rd, &st);
			nand_sweep_yield();
		}
		if (((r + 1U) % print_cnt) == 0U) {
			CLI_LOGI("range_loop: round %u/%u done\r\n", r + 1U, rounds);
		}
	}

	uint64_t elapsed = bk_aon_rtc_get_us() - t_start;
	uint32_t w_kbps, w_t, r_kbps, r_t;
	nand_sweep_kbps(st.write_bytes, st.write_us, &w_kbps, &w_t);
	nand_sweep_kbps(st.read_bytes, st.read_us, &r_kbps, &r_t);
	bk_qspi_flash_nand_get_ecc_stat(&ecc_corr_after, &ecc_unc_after);
	CLI_LOGI("=== NAND RANGE_LOOP: blocks %u..%u x%u ===\r\n", from, to, rounds);
	CLI_LOGI("  tested=%u factory_bad_skipped=%u elapsed=%llu us\r\n",
	         st.blocks_tested, st.blocks_skipped, (unsigned long long)elapsed);
	CLI_LOGI("  Write: %llu B, avg %u.%u KB/s\r\n",
	         (unsigned long long)st.write_bytes, w_kbps, w_t);
	CLI_LOGI("  Read : %llu B, avg %u.%u KB/s\r\n",
	         (unsigned long long)st.read_bytes, r_kbps, r_t);
	CLI_LOGI("  Fails: erase=%u program=%u read=%u verify=%u\r\n",
	         st.erase_fail, st.program_fail, st.read_fail, st.verify_fail);
	CLI_LOGI("  ECC delta: corrected=%u uncorrectable=%u\r\n",
	         ecc_corr_after - ecc_corr_before, ecc_unc_after - ecc_unc_before);
	if (st.any_fail) {
		CLI_LOGI("  first failing block: %u\r\n", st.first_fail_block);
	}
	CLI_LOGI("=== RANGE_LOOP RESULT: %s ===\r\n", st.any_fail ? "FAIL" : "PASS");
	os_free(wr);
	os_free(rd);
}

static void cli_nand_usage(void)
{
	CLI_LOGI("qspi_nand init\r\n");
	CLI_LOGI("qspi_nand get_id\r\n");
	CLI_LOGI("qspi_nand get_uid - GD5F05 OTP UID (vendor-specific)\r\n");
	CLI_LOGI("qspi_nand protect_none\r\n");
	CLI_LOGI("qspi_nand quad_enable - Clear WP-E to enable Quad (4-wire) mode\r\n");
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
	CLI_LOGI("qspi_nand sweep [from] [to] [rounds] - Whole-die erase/write/verify (material import)\r\n");
	CLI_LOGI("qspi_nand sweep_write [from] [to] [tag] - Fill blocks and LEAVE the data (retention test)\r\n");
	CLI_LOGI("qspi_nand sweep_verify [from] [to] [tag] - Verify only, no erase/write (after bake/power cycle)\r\n");
	CLI_LOGI("qspi_nand range_loop <offset> <size> <times> [tag] [print_cnt] - Destructive erase/read/write/read loop\r\n");
	CLI_LOGI("qspi_nand read_oob {page} - Dump 64B spare with ECC on and off\r\n");
	CLI_LOGI("qspi_nand factory_bad {block} - Check factory bad-block marker\r\n");
	CLI_LOGI("qspi_nand bbm init - Bring up the bad-block management layer\r\n");
	CLI_LOGI("qspi_nand bbm dump - Dump BBT: layout, bad blocks, remaps\r\n");
	CLI_LOGI("qspi_nand bbm inject {logical_block} - Force-retire+remap a logical block\r\n");
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
		uint8_t id_buf[FLASH_READ_ID_SIZE] = {0};
		bk_err_t ret = bk_qspi_flash_nand_get_id(qspi_id, id_buf, sizeof(id_buf));
		if (ret == BK_OK) {
			/* Compare against the descriptor that init actually probed, not a
			 * hardcoded part: the registry decides which device is supported,
			 * so a fixed expectation only produces false alarms once the
			 * board carries anything else. */
			const qspi_nand_dev_t *dev = bk_qspi_nand_dev(qspi_id);

			if (!dev) {
				CLI_LOGW("nand id: %02x %02x (no matching device descriptor)\n",
				         id_buf[0], id_buf[1]);
			} else if (os_memcmp(id_buf, dev->id, dev->id_len) != 0) {
				CLI_LOGW("nand id: %02x %02x mismatches probed %s (%02x %02x)\n",
				         id_buf[0], id_buf[1], dev->name, dev->id[0], dev->id[1]);
			} else {
				CLI_LOGI("nand id: %02x %02x (%s)\n", id_buf[0], id_buf[1], dev->name);
			}
		} else {
			CLI_LOGE("nand get id fail(%d)\n", ret);
		}
	} else if (os_strcmp(subcmd, "get_uid") == 0) {
		uint8_t uid[QSPI_NAND_UID_LEN] = {0};
		bk_err_t ret;

		if (!s_nand_initialized) {
			CLI_LOGE("QSPI not initialized! Please run 'qspi_nand init' first\r\n");
			return;
		}
		ret = bk_qspi_flash_nand_get_uid(qspi_id, uid, sizeof(uid));
		if (ret == BK_OK) {
			char line[QSPI_NAND_UID_LEN * 3];

			nand_hex_line(line, uid, QSPI_NAND_UID_LEN);
			CLI_LOGI("nand uid: %s\r\n", line);
		} else if (ret == BK_ERR_NOT_SUPPORT) {
			CLI_LOGE("get_uid not supported by probed part\r\n");
		} else {
			CLI_LOGE("nand get_uid fail(%d)\r\n", ret);
		}
	} else if (os_strcmp(subcmd, "protect_none") == 0) {
		BK_LOG_ON_ERR(bk_qspi_flash_nand_set_protect_none(qspi_id));
		CLI_LOGI("nand protect none\r\n");
	} else if (os_strcmp(subcmd, "quad_enable") == 0) {
		if (!s_nand_initialized) {
			CLI_LOGE("QSPI not initialized! Please run 'qspi_nand init' first\r\n");
			return;
		}
		BK_LOG_ON_ERR(bk_qspi_flash_quad_enable(qspi_id));
		CLI_LOGI("nand quad (4-wire) mode enabled\r\n");
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
			for (uint32_t off = 0; off < len; off += 16) {
				uint32_t n = (len - off > 16) ? 16 : (len - off);
				char line[16 * 3];

				nand_hex_line(line, buffer + off, n);
				CLI_LOGI("  %04x: %s\r\n", column + off, line);
			}
		} else {
			CLI_LOGE("page read failed: %d\r\n", ret);
		}
		os_free(buffer);
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
			for (uint32_t off = 0; off < len; off += 16) {
				uint32_t n = (len - off > 16) ? 16 : (len - off);
				char line[16 * 3];

				nand_hex_line(line, buffer + off, n);
				CLI_LOGI("  %04x: %s\r\n", column + off, line);
			}
		} else {
			CLI_LOGE("page read quad failed: %d\r\n", ret);
		}
		os_free(buffer);
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
		ret = bk_qspi_flash_nand_page_program_quad(qspi_id, page, 0, wr_buf, NAND_PAGE_SIZE_BYTES);
		if (ret != BK_OK) { CLI_LOGE("program fail\r\n"); goto pt_end; }
		os_memset(rd_buf, 0, NAND_PAGE_SIZE_BYTES);
		ret = bk_qspi_flash_nand_page_read_quad(qspi_id, page, 0, rd_buf, NAND_PAGE_SIZE_BYTES);
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
		ret = bk_qspi_flash_nand_page_program_quad(qspi_id, page, 0, wr_buf, NAND_PAGE_SIZE_BYTES);
		if (ret != BK_OK) { CLI_LOGE("program fail\r\n"); goto pt_end; }
		os_memset(rd_buf, 0, NAND_PAGE_SIZE_BYTES);
		ret = bk_qspi_flash_nand_page_read_quad(qspi_id, page, 0, rd_buf, NAND_PAGE_SIZE_BYTES);
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
			ret = bk_qspi_flash_nand_page_program_quad(qspi_id, first_page + p, 0, buf, NAND_PAGE_SIZE_BYTES);
			if (ret != BK_OK) { CLI_LOGE("program page %u fail\r\n", first_page + p); goto bt_end; }
		}
		{
			bool all_pass = true;
			for (uint32_t p = 0; p < NAND_BLOCK_PAGE_COUNT; p++) {
				os_memset(buf, 0, NAND_PAGE_SIZE_BYTES);
				ret = bk_qspi_flash_nand_page_read_quad(qspi_id, first_page + p, 0, buf, NAND_PAGE_SIZE_BYTES);
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
			if (!all_pass) {
				goto bt_end;
			}
		}
		bk_qspi_flash_nand_block_erase(qspi_id, block);
		CLI_LOGI("=== BLOCK_TEST PASS ===\r\n");
bt_end:
		os_free(buf);
	} else if (os_strcmp(subcmd, "sweep") == 0) {
		cli_nand_sweep(qspi_id, NAND_SWEEP_FULL, argc, argv);
	} else if (os_strcmp(subcmd, "sweep_write") == 0) {
		cli_nand_sweep(qspi_id, NAND_SWEEP_WRITE, argc, argv);
	} else if (os_strcmp(subcmd, "sweep_verify") == 0) {
		cli_nand_sweep(qspi_id, NAND_SWEEP_VERIFY, argc, argv);
	} else if (os_strcmp(subcmd, "range_loop") == 0) {
		cli_nand_range_loop(qspi_id, argc, argv);
	} else if (os_strcmp(subcmd, "read_oob") == 0) {
		if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
		if (argc < 3) { CLI_LOGI("Usage: qspi_nand read_oob <page>\r\n"); return; }
		uint32_t page = os_strtoul(argv[2], NULL, 0);
		uint8_t oob[NAND_SPARE_SIZE_BYTES];
		uint8_t cfg = 0;
		bk_err_t ret;

		/*
		 * Dump the same spare area twice, once per on-die ECC setting.
		 *
		 * A factory bad-block marker has to survive a data write, so it must
		 * sit in a byte the ECC engine does not own. Comparing the two views
		 * says which one the marker check should sample: with ECC enabled the
		 * part hides its parity and presents the user meta area, with ECC
		 * disabled the raw spare comes back, parity included.
		 */
		ret = bk_qspi_flash_nand_get_feature(qspi_id, NAND_FEATURE_ADDR_DRIVE, &cfg);
		if (ret != BK_OK) { CLI_LOGE("read cfg B0h fail(%d)\r\n", ret); return; }
		CLI_LOGI("=== READ_OOB page=%u (B0h=0x%02x, ECC-E=%d) ===\r\n",
		         page, cfg, (cfg & NAND_CFG_ECC_E_BIT) ? 1 : 0);

		for (uint32_t pass = 0; pass < 2; pass++) {
			bool want_ecc = (pass == 0);
			uint8_t want_cfg = want_ecc ? (uint8_t)(cfg | NAND_CFG_ECC_E_BIT)
			                            : (uint8_t)(cfg & (uint8_t)~NAND_CFG_ECC_E_BIT);

			ret = bk_qspi_flash_nand_set_feature(qspi_id, NAND_FEATURE_ADDR_DRIVE, want_cfg);
			if (ret != BK_OK) { CLI_LOGE("set cfg fail(%d)\r\n", ret); break; }

			os_memset(oob, 0, sizeof(oob));
			ret = bk_qspi_flash_nand_read_oob(qspi_id, page, 0, oob, sizeof(oob));
			if (ret != BK_OK) {
				CLI_LOGE("ECC %s: read_oob fail(%d)\r\n", want_ecc ? "on " : "off", ret);
				continue;
			}

			CLI_LOGI("ECC %s spare[0]=0x%02x:\r\n", want_ecc ? "on " : "off", oob[0]);
			for (uint32_t off = 0; off < NAND_SPARE_SIZE_BYTES; off += 16) {
				char line[16 * 3];

				nand_hex_line(line, oob + off, 16);
				CLI_LOGI("  %02x: %s\r\n", off, line);
			}
		}

		/* Always hand the part back in the state we found it in. */
		BK_LOG_ON_ERR(bk_qspi_flash_nand_set_feature(qspi_id, NAND_FEATURE_ADDR_DRIVE, cfg));
	} else if (os_strcmp(subcmd, "factory_bad") == 0) {
		if (!s_nand_initialized) { CLI_LOGE("not initialized\r\n"); return; }
		if (argc < 3) { CLI_LOGI("Usage: qspi_nand factory_bad <block>\r\n"); return; }
		uint32_t block = os_strtoul(argv[2], NULL, 0);
		bool is_bad = false;
		bk_err_t ret = bk_qspi_flash_nand_is_factory_bad(qspi_id, block, &is_bad);
		if (ret == BK_OK) {
			CLI_LOGI("block %u factory bad: %s\r\n", block, is_bad ? "YES" : "no");
		} else {
			CLI_LOGE("factory_bad check failed: %d\r\n", ret);
		}
	} else if (os_strcmp(subcmd, "bbm") == 0) {
		if (argc < 3) { CLI_LOGI("Usage: qspi_nand bbm {init|dump|inject <block>}\r\n"); return; }
		const char *bbm_sub = argv[2];
		if (os_strcmp(bbm_sub, "init") == 0) {
			if (!s_nand_initialized) { CLI_LOGE("run 'qspi_nand init' first\r\n"); return; }
			bk_err_t ret = bk_qspi_nand_bbm_init(qspi_id);
			CLI_LOGI("bbm init %s (logical size=%u bytes)\r\n",
			         (ret == BK_OK) ? "done" : "FAIL", bk_qspi_nand_bbm_logical_size(qspi_id));
		} else if (os_strcmp(bbm_sub, "dump") == 0) {
			bk_qspi_nand_bbm_dump(qspi_id);
		} else if (os_strcmp(bbm_sub, "inject") == 0) {
			if (argc < 4) { CLI_LOGI("Usage: qspi_nand bbm inject <logical_block>\r\n"); return; }
			uint32_t lb = os_strtoul(argv[3], NULL, 0);
			bk_err_t ret = bk_qspi_nand_bbm_inject_bad(qspi_id, lb);
			CLI_LOGI("bbm inject logical %u %s\r\n", lb, (ret == BK_OK) ? "done" : "FAIL");
		} else {
			CLI_LOGI("Usage: qspi_nand bbm {init|dump|inject <block>}\r\n");
		}
	} else {
		cli_nand_usage();
	}
}
#endif /* CONFIG_QSPI_NAND_FLASH */

#if CONFIG_QSPI_NAND_FLASH
#define QSPI_FLASH_CMD_CNT (sizeof(s_qspi_flash_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_qspi_flash_commands[] = {
	{"qspi_nand", "qspi_nand {init|get_id|get_uid|sweep|sweep_write|sweep_verify|...}", cli_qspi_nand_cmd},
};
#elif (CONFIG_QSPI_NOR_FLASH)
#define QSPI_FLASH_CMD_CNT (sizeof(s_qspi_flash_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_qspi_flash_commands[] = {
	{"qspi_flash", "qspi_flash {get_id|erase|read|write|...}", cli_qspi_flash_cmd},
};
#endif
