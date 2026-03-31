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

#include <stdlib.h>
#include "cli.h"
#include <components/bk_uid.h>

static void cli_uid_help(void)
{
	CLI_LOGD("uid [init/get]\r\n");
}

static void cli_uid_ops_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2)
	{
		cli_uid_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		BK_LOG_ON_ERR(bk_uid_driver_init());
	} else if (os_strcmp(argv[1], "get") == 0) {
		unsigned char data[32] = {0};
		BK_LOG_ON_ERR(bk_uid_get_data(data));
		for (int j = 0; j < 32; j++)
		{
			CLI_LOGD("%02x index:%d\r\n", data[j], j);
		}
	} else {
		cli_uid_help();
		return;
	}
}

#define UID_CMD_CNT (sizeof(s_uid_commands) / sizeof(struct cli_command))
COMPONENTS_CLI_CMD_EXPORT static const struct cli_command s_uid_commands[] = {
	{"uid", "uid [init/get]", cli_uid_ops_cmd},
};

int cli_uid_init(void)
{
	return cli_register_commands(s_uid_commands, UID_CMD_CNT);
}
