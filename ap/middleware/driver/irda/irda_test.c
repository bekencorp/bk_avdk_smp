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

static uint16_t send_data[]={9020, 4410, 570, 1670 ,570, 1670, 570, 1670, 570,580 ,1670, 570, 1670 ,570 ,570, 580 ,570 ,1670, 570, 1670, 580 ,1650 ,580, 570 ,570, 570, 580 ,570 ,580, 1670, 570 ,1670 ,570, 570, 580, 10938};

static void cli_irda_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "tx") == 0) {
		irda_tx_init_config_t tx_config = {0};
		tx_config.clk_freq_input = 26;
		tx_config.carrier_period_cycle = 26;
		tx_config.carrier_duty_cycle = 10;
		bk_irda_init_tx(&tx_config);

		bk_irda_write_bytes(send_data, sizeof(send_data));

		CLI_LOGD("irda tx test\r\n");
	} else if (os_strcmp(argv[1], "rx") == 0) {
		irda_rx_init_config_t rx_config = {0};
		rx_config.clk_freq_input = 26;
		rx_config.rx_timeout_us = 8000;
		rx_config.rx_start_threshold_us = 1000;
		bk_irda_init_rx(&rx_config);

		uint16_t recv_buf[50];

		int recv_num = bk_irda_read_bytes(recv_buf, sizeof(recv_buf));

		for (int i = 0; i < recv_num; i++) {
			BK_LOGD(NULL, "recv_buf[%d]:%d\r\n", i, recv_buf[i]);
		}

		CLI_LOGD("irda rx test\r\n");
	} else {

	}
}

#define IRDA_CMD_CNT (sizeof(s_irda_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_irda_commands[] = {
	{"irda", "irda {start|stop|feed} [...]", cli_irda_cmd}
};

int bk_irda_register_cli_test_feature(void)
{
	return cli_register_module_test_feature(s_irda_commands, IRDA_CMD_CNT);
}
// eof

