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

// QSPI controller-level and QSPI-PSRAM CLI test commands.
//
// The NOR/NAND flash CLI moved to bk_external_flash together with the flash/NAND
// protocol engines (see components/bk_external_flash/qspi_flash/qspi_flash_test.c),
// so middleware does not depend back on the bk_external_flash component. QSPI-PSRAM
// stays here because it relies on the middleware-private HAL direct access.

#include <soc/soc.h>
#include <driver/qspi.h>
#if CONFIG_QSPI_MST_PSRAM
#include <driver/qspi_psram.h>
#endif
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
	CLI_LOGD("  clk_div: ignored (internal divider no longer effective)\r\n");
	CLI_LOGD("  Example: qspi 0 init 0 3 0  (160M/(1+3) = 40MHz)\r\n");
	CLI_LOGD("qspi <id> set_clk <freq_hz> - Set QSPI clock by target frequency in Hz (max 80MHz)\r\n");
	CLI_LOGD("  Example: qspi 0 set_clk 80000000\r\n");
#if CONFIG_QSPI_MST_PSRAM
	CLI_LOGD("qspi enter_quad_mode\r\n");
	CLI_LOGD("qspi exit_quad_mode\r\n");
	CLI_LOGD("qspi quad_write\r\n");
	CLI_LOGD("qspi quad_read\r\n");
	CLI_LOGD("qspi compare\r\n");
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

#if CONFIG_QSPI_MST_PSRAM
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
#endif

static void cli_qspi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
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
			CLI_LOGE("  clk_div: ignored (internal divider no longer effective)\r\n");
			CLI_LOGE("  Example: qspi 0 init 0 3 0  (160M/(1+3) = 40MHz)\r\n");
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
		qspi_config_t config = {0};
		config.src_clk = (qspi_src_clk_t)src_clk;
		config.src_clk_div = src_clk_div;
		config.clk_div = clk_div;

		// Calculate and display actual frequency (clk_div is no longer effective)
		uint32_t base_clocks[] = {160, 240}; // MHz
		uint32_t base_clk = base_clocks[src_clk];
		uint32_t actual_freq = base_clk / (1 + src_clk_div);

		CLI_LOGD("Config: src_clk=%s, src_clk_div=%d, clk_div=%d\r\n",
		         (src_clk == 0) ? "160M" : "240M", src_clk_div, clk_div);
		CLI_LOGD("Calculation: %dM / (1+%d) = %dMHz\r\n",
		         base_clk, src_clk_div, actual_freq);
		if (clk_div != 0) {
			CLI_LOGW("clk_div=%d is ignored (internal divider no longer effective)\r\n", clk_div);
		}

		BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &config));
		CLI_LOGD("qspi init success\r\n");
	} else if (os_strcmp(argv[2], "set_clk") == 0) {
		if (argc < 4) {
			CLI_LOGE("Usage: qspi <id> set_clk <freq_hz>\r\n");
			CLI_LOGE("  Example: qspi 0 set_clk 80000000\r\n");
			return;
		}

		uint32_t freq_hz = os_strtoul(argv[3], NULL, 0);
		if (freq_hz == 0 || freq_hz > 80000000) {
			CLI_LOGE("Invalid freq_hz: %u (must be 1~80000000)\r\n", freq_hz);
			return;
		}

		BK_LOG_ON_ERR(bk_qspi_driver_init());
		BK_LOG_ON_ERR(bk_qspi_init_by_freq(qspi_id, freq_hz));
		CLI_LOGD("qspi set clock target to %u Hz success\r\n", freq_hz);
#if CONFIG_QSPI_MST_PSRAM
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
#endif
	} else {
		cli_qspi_help();
	}
}

#define QSPI_CMD_CNT (sizeof(s_qspi_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_qspi_commands[] = {
	{"qspi_driver", "qspi_driver {init|deinit}", cli_qspi_driver_cmd},
	{"qspi", "qspi {init|set_clk|...}", cli_qspi_cmd},
};

int bk_qspi_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_qspi_driver_init());
	return cli_register_module_test_feature(s_qspi_commands, QSPI_CMD_CNT);
}
