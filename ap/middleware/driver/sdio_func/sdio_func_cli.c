// Copyright 2020-2026 Beken
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

/*
 * CLI front-end for the SDIO peripheral (I/O card) core, used to bring up and
 * validate an external SDIO device driven by BK7259 as the host/master.
 *
 * The "sdio_func scan" sub-command runs the full card-scan flow
 * (CMD5 -> CMD3 -> CMD7 -> CCCR/CIS parse) and reports what the bus found.
 */

#include <os/os.h>
#include <os/str.h>
#include <common/bk_include.h>
#include <driver/sdio_func.h>
#include "cli.h"

#if CONFIG_SDIO_FUNC_CLI

#define SFC_TAG "sdio_func"

static void cli_sdio_func_help(void)
{
	CLI_LOGI("sdio_func scan [host_id]      - scan/enumerate the SDIO card\r\n");
	CLI_LOGI("sdio_func deinit              - release the host controller\r\n");
	CLI_LOGI("sdio_func info                - print last scan result (func count + CIS)\r\n");
	CLI_LOGI("sdio_func enable <func>       - enable an I/O function (1..7)\r\n");
	CLI_LOGI("sdio_func disable <func>      - disable an I/O function (1..7)\r\n");
	CLI_LOGI("sdio_func blksz <func> <size> - set the function block size\r\n");
	CLI_LOGI("sdio_func rb <func> <addr>    - CMD52 read byte\r\n");
	CLI_LOGI("sdio_func wb <func> <addr> <val> - CMD52 write byte\r\n");
	CLI_LOGI("sdio_func f0rb <addr>         - CMD52 read CCCR/function-0 byte\r\n");
}

static void cli_sdio_func_dump_info(void)
{
	sdio_func_cis_t cis = {0};
	uint8_t cnt = bk_sdio_func_count();

	bk_sdio_func_get_cis(&cis);
	CLI_LOGI("io functions : %d\r\n", cnt);
	CLI_LOGI("CIS manf id  : 0x%04x\r\n", cis.manf_id);
	CLI_LOGI("CIS card id  : 0x%04x\r\n", cis.card_id);
	CLI_LOGI("func0 blksz  : %d\r\n", cis.func0_blksz);
}

static void cli_sdio_func_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret;

	if (argc < 2) {
		cli_sdio_func_help();
		return;
	}

	if (os_strcmp(argv[1], "scan") == 0 || os_strcmp(argv[1], "init") == 0) {
		sdio_host_id_t host_id = SDIO_HOST_ID_MAX; /* let driver pick default */

		if (argc > 2)
			host_id = (sdio_host_id_t)os_strtoul(argv[2], NULL, 10);

		CLI_LOGI("sdio_func scan on host %d ...\r\n",
			 (host_id >= SDIO_HOST_ID_MAX) ? CONFIG_SDIO_FUNC_HOST_ID : (int)host_id);
		ret = bk_sdio_func_init(host_id);
		if (ret != BK_OK) {
			CLI_LOGE("scan failed, no SDIO card? ret=%d\r\n", ret);
			return;
		}
		CLI_LOGI("scan OK\r\n");
		cli_sdio_func_dump_info();
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		ret = bk_sdio_func_deinit();
		CLI_LOGI("deinit ret=%d\r\n", ret);
	} else if (os_strcmp(argv[1], "info") == 0) {
		cli_sdio_func_dump_info();
	} else if (os_strcmp(argv[1], "enable") == 0) {
		if (argc < 3) {
			cli_sdio_func_help();
			return;
		}
		uint8_t func = (uint8_t)os_strtoul(argv[2], NULL, 10);
		ret = bk_sdio_enable_func(func);
		CLI_LOGI("enable func%d ret=%d\r\n", func, ret);
	} else if (os_strcmp(argv[1], "disable") == 0) {
		if (argc < 3) {
			cli_sdio_func_help();
			return;
		}
		uint8_t func = (uint8_t)os_strtoul(argv[2], NULL, 10);
		ret = bk_sdio_disable_func(func);
		CLI_LOGI("disable func%d ret=%d\r\n", func, ret);
	} else if (os_strcmp(argv[1], "blksz") == 0) {
		if (argc < 4) {
			cli_sdio_func_help();
			return;
		}
		uint8_t func = (uint8_t)os_strtoul(argv[2], NULL, 10);
		uint16_t blksz = (uint16_t)os_strtoul(argv[3], NULL, 10);
		ret = bk_sdio_set_block_size(func, blksz);
		CLI_LOGI("set func%d blksz=%d ret=%d\r\n", func, blksz, ret);
	} else if (os_strcmp(argv[1], "rb") == 0) {
		if (argc < 4) {
			cli_sdio_func_help();
			return;
		}
		uint8_t func = (uint8_t)os_strtoul(argv[2], NULL, 10);
		uint32_t addr = os_strtoul(argv[3], NULL, 0);
		int err = 0;
		uint8_t val = bk_sdio_readb(func, addr, &err);
		CLI_LOGI("rb func%d addr=0x%x -> 0x%02x (err=%d)\r\n", func, (unsigned int)addr, val, err);
	} else if (os_strcmp(argv[1], "wb") == 0) {
		if (argc < 5) {
			cli_sdio_func_help();
			return;
		}
		uint8_t func = (uint8_t)os_strtoul(argv[2], NULL, 10);
		uint32_t addr = os_strtoul(argv[3], NULL, 0);
		uint8_t val = (uint8_t)os_strtoul(argv[4], NULL, 0);
		int err = 0;
		bk_sdio_writeb(func, val, addr, &err);
		CLI_LOGI("wb func%d addr=0x%x val=0x%02x (err=%d)\r\n", func, (unsigned int)addr, val, err);
	} else if (os_strcmp(argv[1], "f0rb") == 0) {
		if (argc < 3) {
			cli_sdio_func_help();
			return;
		}
		uint32_t addr = os_strtoul(argv[2], NULL, 0);
		int err = 0;
		uint8_t val = bk_sdio_f0_readb(0, addr, &err);
		CLI_LOGI("f0rb addr=0x%x -> 0x%02x (err=%d)\r\n", (unsigned int)addr, val, err);
	} else {
		cli_sdio_func_help();
	}
}

/*
 * Auto-registered at boot: DRV_CLI_CMD_EXPORT places this table in the
 * ".cli_cmdtabl" linker section, which cli_commands_init() walks during
 * bk_cli_init(). The driver static library is linked with --whole-archive,
 * so this object is always pulled in and the command appears on the AP shell
 * (including via "ap_cmd sdio_func ...") with no explicit registration call.
 */
DRV_CLI_CMD_EXPORT static const struct cli_command s_sdio_func_commands[] = {
	{"sdio_func", "sdio_func {scan|info|enable|disable|blksz|rb|wb|f0rb|deinit} [...]", cli_sdio_func_cmd},
};

#endif /* CONFIG_SDIO_FUNC_CLI */
// eof
