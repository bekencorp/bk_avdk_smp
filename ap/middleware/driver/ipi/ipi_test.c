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
#include "cli_section.h"

#define IPI_TEST_TAG "ipi_test"
#define IPI_TEST_LOGI(...) BK_LOGI(IPI_TEST_TAG, ##__VA_ARGS__)
#define IPI_TEST_LOGD(...) BK_LOGD(IPI_TEST_TAG, ##__VA_ARGS__)

#define IPI_TEST_VALUE_TO_EVENT(value)   (((value) >> IPI_VALUE_EVENT_POS) & 0xFF)
#define IPI_TEST_VALUE_TO_PAYLOAD(value) ((value) & IPI_VALUE_PAYLOAD_MASK)

/* TEST-domain callback for IPI interrupt */
static void ipi_test_callback(ipi_core_id_t core_id, uint32_t value,
	uint8_t src_cpu, uint8_t event, uint16_t payload, void *param)
{
	IPI_TEST_LOGI("IPI callback: core_id=%d, value=0x%08X, src=%u, event=0x%02X, payload=0x%04X, param=0x%p\r\n",
		core_id, value, src_cpu, event, payload, param);
}

static void cli_ipi_help(void)
{
	CLI_LOGD("ipi_driver init [local|all] (default: local) - Register test callbacks and enable channels\r\n");
	CLI_LOGD("ipi_driver deinit - Unregister callbacks and disable channels\r\n");
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
		/* IPI driver is initialized in driver_init() by default */

		bool init_all = false;
		if (argc >= 3 && os_strcmp(argv[2], "all") == 0) {
			init_all = true;
		}

		/*
		 * Default: only enable local AP channels (2/3) to avoid interfering with CP/DSP.
		 * Use `ipi_driver init all` if you want to enable all channels.
		 */
		BK_LOG_ON_ERR(bk_ipi_register_domain_callback(IPI_DOMAIN_TEST, ipi_test_callback, NULL));
		for (core_id = 0; core_id < IPI_CORE_MAX; core_id++) {
			if (!init_all && (core_id != IPI_AP_CORE0) && (core_id != IPI_AP_CORE1)) {
				continue;
			}
			BK_LOG_ON_ERR(bk_ipi_enable(core_id));
		}

		CLI_LOGD("IPI TEST-domain callback registered successfully\r\n");


	} else if (os_strcmp(argv[1], "deinit") == 0) {
		/* Only disable channels/unregister callbacks; keep driver initialized */
		for (core_id = 0; core_id < IPI_CORE_MAX; core_id++) {
			BK_LOG_ON_ERR(bk_ipi_disable(core_id));
		}
		BK_LOG_ON_ERR(bk_ipi_unregister_domain_callback(IPI_DOMAIN_TEST));
		CLI_LOGD("IPI TEST-domain callback unregistered successfully\r\n");
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
		uint32_t raw_value = os_strtoul(argv[3], NULL, 16);
		uint8_t event = IPI_TEST_VALUE_TO_EVENT(raw_value);
		uint16_t payload = IPI_TEST_VALUE_TO_PAYLOAD(raw_value);
		bk_err_t ret = bk_ipi_send_domain(core_id, IPI_DOMAIN_TEST, event, payload);
		if (ret == BK_OK) {
			CLI_LOGD("Sent TEST-domain IPI: from_cpu=%u -> core %d, raw=0x%08X, event=0x%02X, payload=0x%04X\r\n",
			         (unsigned)rtos_get_core_id(), core_id, raw_value, event, payload);
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
#if CONFIG_IPI_DUMP
		bk_ipi_dump_info();
#else
		CLI_LOGW("IPI dump disabled (enable CONFIG_IPI_DUMP)\r\n");
#endif
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

