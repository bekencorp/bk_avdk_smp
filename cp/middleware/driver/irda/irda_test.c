// Copyright 2023-2024 Beken
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
#include <driver/irda.h>

static uint16_t send_data[]={9020, 4410, 1670 ,570, 1670, 570, 1670, 570,580 ,1670, 570, 1670 ,570 ,570, 580 ,570 ,1670, 570, 1670, 580 ,1650 ,580, 570 ,570, 570, 580 ,570 ,580, 1670, 570 ,1670 ,570, 570, 580, 10938};

static void cli_irda_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "tx") == 0) {
		irda_tx_init_config_t tx_config = {0};
		tx_config.clk_freq_input = 26;
		tx_config.carrier_period_cycle = 26;
		tx_config.carrier_duty_cycle = 10;
		bk_irda_init_tx(&tx_config);

		bk_irda_write_words(send_data, sizeof(send_data) / sizeof(send_data[0]));

		CLI_LOGD("irda tx test\r\n");
	} else if (os_strcmp(argv[1], "rx_init") == 0) {
		irda_rx_init_config_t rx_config = {0};
		/* default is 0 ，When connecting an infrared receiver, set this to 1*/
		// rx_config.rx_initial_level = 1;
		rx_config.clk_freq_input = 26;
		rx_config.rx_timeout_us = 8000;
		rx_config.rx_start_threshold_us = 1000;
		bk_irda_init_rx(&rx_config);
		CLI_LOGD("irda rx init done\r\n");
	} else if (os_strcmp(argv[1], "rx_read") == 0) {
		const uint32_t recv_cap = 512;
		uint16_t *recv_buf = (uint16_t *)os_malloc(recv_cap * sizeof(uint16_t));
		uint32_t recv_total = 0;

		if (!recv_buf) {
			BK_LOGE(NULL, "irda rx malloc failed, cap:%u\r\n", recv_cap);
			return;
		}

		while (recv_total < recv_cap) {
			uint32_t read_timeout = (recv_total == 0) ? BEKEN_WAIT_FOREVER : 60;
			int recv_num = bk_irda_read_words(recv_buf + recv_total, recv_cap - recv_total, read_timeout);
			if (recv_num > 0) {
				recv_total += (uint32_t)recv_num;
				continue;
			}

			if (recv_num == BK_ERR_TIMEOUT) {
				break;
			}

			BK_LOGE(NULL, "irda rx failed, ret:%d\r\n", recv_num);
			break;
		}

		BK_LOGD(NULL, "irda rx total:%u\r\n", recv_total);
		for (uint32_t i = 0; i < recv_total; i++) {
			BK_LOGD(NULL, "recv_buf[%d]:%d\r\n", i, recv_buf[i]);
		}

		os_free(recv_buf);
		CLI_LOGD("irda rx read done\r\n");
	} else {
		CLI_LOGD("usage: irda {tx|rx_init|rx_read}\r\n");
	}
}

#define IRDA_CMD_CNT (sizeof(s_irda_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_irda_commands[] = {
	{"irda", "irda {tx|rx_init|rx_read}", cli_irda_cmd}
};

int bk_irda_register_cli_test_feature(void)
{
	return cli_register_module_test_feature(s_irda_commands, IRDA_CMD_CNT);
}
// eof

