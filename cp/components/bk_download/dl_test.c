// Copyright 2020-2022 Beken
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cli.h"

#define DL_QSPI_TAG "dl_qspi_test"
#define DL_QSPI_LOGI(...) BK_LOGI(DL_QSPI_TAG, ##__VA_ARGS__)
#define DL_QSPI_LOGW(...) BK_LOGW(DL_QSPI_TAG, ##__VA_ARGS__)
#define DL_QSPI_LOGE(...) BK_LOGE(DL_QSPI_TAG, ##__VA_ARGS__)
#define DL_QSPI_LOGD(...) BK_LOGD(DL_QSPI_TAG, ##__VA_ARGS__)

static void cli_dl_qspi_help()
{
	DL_QSPI_LOGE("cmd is dl_qspi_test \r\n");
}

static void cli_dl_qspi_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 1) 
	{
		cli_dl_qspi_help();
		return;
	}

	DL_QSPI_LOGI("%s enter \r\n", __FUNCTION__);
	extern void download_qspi_flash(void);
 	download_qspi_flash();
}

#define DL_QSPI_CMD_CNT    sizeof(s_dl_commands)/sizeof(s_dl_commands[0])

static const struct cli_command s_dl_commands[] ={
	{"dl_qspi_test", "dl_qspi_test", cli_dl_qspi_test}
};

int cli_dl_qspi_init(void)
{
	return cli_register_commands(s_dl_commands, DL_QSPI_CMD_CNT);
}


