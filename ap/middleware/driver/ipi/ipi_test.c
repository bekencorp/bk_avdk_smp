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

#include <os/os.h>
#include "cli.h"
#include "ipi_driver.h"
#include <components/log.h>

#define IPI_TEST_TAG "ipi_test"
#define IPI_TEST_LOGI(...) BK_LOGI(IPI_TEST_TAG, ##__VA_ARGS__)
#define IPI_TEST_LOGD(...) BK_LOGD(IPI_TEST_TAG, ##__VA_ARGS__)

/* Test callback for IPI interrupt */
static void ipi_test_callback(ipi_core_id_t core_id, uint32_t value, void *param)
{
	IPI_TEST_LOGI("IPI callback: core_id=%d, value=0x%08X, param=0x%p\r\n", core_id, value, param);
}

static void cli_ipi_help(void)
{
	CLI_LOGD("ipi_driver {init|deinit}\r\n");
	CLI_LOGD("ipi send {core_id} {value} - Send IPI interrupt to core (0-4)\r\n");
	CLI_LOGD("ipi status {core_id|all} - Get IPI interrupt status\r\n");
	CLI_LOGD("ipi enable {core_id} - Enable IPI interrupt for core (0-4)\r\n");
	CLI_LOGD("ipi disable {core_id} - Disable IPI interrupt for core (0-4)\r\n");
	CLI_LOGD("ipi clear {core_id} - Clear IPI interrupt for core (0-4)\r\n");
	CLI_LOGD("ipi dump - Dump IPI driver information\r\n");
}

static void cli_ipi_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    ipi_core_id_t core_id;

	if (argc < 2) {
		cli_ipi_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {

        /* Register test callback for all cores by default */
        for (core_id = 0; core_id < IPI_CORE_MAX; core_id++) {
            BK_LOG_ON_ERR(bk_ipi_register_callback(core_id, ipi_test_callback, (void *)(unsigned long)core_id));
            BK_LOG_ON_ERR(bk_ipi_enable(core_id));
        }

		CLI_LOGD("IPI driver initialized successfully\r\n");


	} else if (os_strcmp(argv[1], "deinit") == 0) {
		bk_err_t ret = bk_ipi_driver_deinit();
		if (ret == BK_OK) {
			CLI_LOGD("IPI driver deinitialized successfully\r\n");
		} else {
			CLI_LOGE("IPI driver deinitialization failed: %d\r\n", ret);
		}
	} else {
		cli_ipi_help();
		return;
	}
}

static void cli_ipi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_ipi_help();
		return;
	}

	if (os_strcmp(argv[1], "send") == 0) {
		if (argc < 4) {
			CLI_LOGE("Usage: ipi send {core_id} {value}\r\n");
			return;
		}
		ipi_core_id_t core_id = (ipi_core_id_t)os_strtoul(argv[2], NULL, 10);
		uint32_t value = os_strtoul(argv[3], NULL, 16);
		bk_err_t ret = bk_ipi_send(core_id, value);
		if (ret == BK_OK) {
			CLI_LOGD("Sent IPI to core %d with value 0x%08X\r\n", core_id, value);
		} else {
			CLI_LOGE("Failed to send IPI: %d\r\n", ret);
		}
	} else if (os_strcmp(argv[1], "status") == 0) {
		if (argc < 3) {
			CLI_LOGE("Usage: ipi status {core_id|all}\r\n");
			return;
		}
		if (os_strcmp(argv[2], "all") == 0) {
			uint32_t status = bk_ipi_get_all_status();
			CLI_LOGD("IPI status (all cores): 0x%02X\r\n", status);
			CLI_LOGD("  Core 0 (CP_CORE0): %s\r\n", (status & 0x01) ? "pending" : "clear");
			CLI_LOGD("  Core 1 (CP_CORE1): %s\r\n", (status & 0x02) ? "pending" : "clear");
			CLI_LOGD("  Core 2 (AP_CORE0): %s\r\n", (status & 0x04) ? "pending" : "clear");
			CLI_LOGD("  Core 3 (AP_CORE1): %s\r\n", (status & 0x08) ? "pending" : "clear");
			CLI_LOGD("  Core 4 (DSP_CORE): %s\r\n", (status & 0x10) ? "pending" : "clear");
		} else {
			ipi_core_id_t core_id = (ipi_core_id_t)os_strtoul(argv[2], NULL, 10);
			uint32_t status = bk_ipi_get_status(core_id);
			CLI_LOGD("IPI status for core %d: %s\r\n", core_id, status ? "pending" : "clear");
		}
	} else if (os_strcmp(argv[1], "enable") == 0) {
		if (argc < 3) {
			CLI_LOGE("Usage: ipi enable {core_id}\r\n");
			return;
		}
		ipi_core_id_t core_id = (ipi_core_id_t)os_strtoul(argv[2], NULL, 10);
		bk_err_t ret = bk_ipi_enable(core_id);
		if (ret == BK_OK) {
			CLI_LOGD("Enabled IPI interrupt for core %d\r\n", core_id);
		} else {
			CLI_LOGE("Failed to enable IPI interrupt: %d\r\n", ret);
		}
	} else if (os_strcmp(argv[1], "disable") == 0) {
		if (argc < 3) {
			CLI_LOGE("Usage: ipi disable {core_id}\r\n");
			return;
		}
		ipi_core_id_t core_id = (ipi_core_id_t)os_strtoul(argv[2], NULL, 10);
		bk_err_t ret = bk_ipi_disable(core_id);
		if (ret == BK_OK) {
			CLI_LOGD("Disabled IPI interrupt for core %d\r\n", core_id);
		} else {
			CLI_LOGE("Failed to disable IPI interrupt: %d\r\n", ret);
		}
	} else if (os_strcmp(argv[1], "clear") == 0) {
		if (argc < 3) {
			CLI_LOGE("Usage: ipi clear {core_id}\r\n");
			return;
		}
		ipi_core_id_t core_id = (ipi_core_id_t)os_strtoul(argv[2], NULL, 10);
		bk_err_t ret = bk_ipi_clear(core_id);
		if (ret == BK_OK) {
			CLI_LOGD("Cleared IPI interrupt for core %d\r\n", core_id);
		} else {
			CLI_LOGE("Failed to clear IPI interrupt: %d\r\n", ret);
		}
	} else if (os_strcmp(argv[1], "dump") == 0) {
		bk_ipi_dump_info();
	} else {
		cli_ipi_help();
		return;
	}
}

#define IPI_CMD_CNT (sizeof(s_ipi_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_ipi_commands[] = {
	{"ipi_driver", "{init|deinit}", cli_ipi_driver_cmd},
	{"ipi", "ipi {send|status|enable|disable|clear|dump} [...]", cli_ipi_cmd}
};

int bk_ipi_register_cli_test_feature(void)
{

	return cli_register_module_test_feature(s_ipi_commands, IPI_CMD_CNT);
}

