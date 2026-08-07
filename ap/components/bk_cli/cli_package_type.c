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

#include "cli.h"
#include <modules/chip_support.h>

static const char *const s_package_type_names[] = {
	[BK_PACKAGE_TYPE_UNKNOWN] = "UNKNOWN",
	[BK_PACKAGE_TYPE_A_OLD_128A_S_MIC] = "A_OLD_128A_S_MIC",
	[BK_PACKAGE_TYPE_A_NEW_128A_S_MIC] = "A_NEW_128A_S_MIC",
	[BK_PACKAGE_TYPE_B_OLD_128A_S_MIC] = "B_OLD_128A_S_MIC",
	[BK_PACKAGE_TYPE_B_NEW_128A_S_OR_128B_D_MIC] = "B_NEW_128A_S_OR_128B_D_MIC",
};

static void cli_package_type_cmd(char *pcWriteBuffer, int xWriteBufferLen,
	int argc, char **argv)
{
	bk_package_type_t package_type;
	bk_err_t ret;

	(void)pcWriteBuffer;
	(void)xWriteBufferLen;
	(void)argc;
	(void)argv;

	ret = bk_get_package_type(&package_type);
	if (ret != BK_OK) {
		CLI_LOGE("PACKAGE_TYPE:READ_ERROR ret=%d\r\n", ret);
		return;
	}

	if ((uint32_t)package_type >= ARRAY_SIZE(s_package_type_names)) {
		package_type = BK_PACKAGE_TYPE_UNKNOWN;
	}

	CLI_LOGI("PACKAGE_TYPE:%s enum=%d\r\n",
		s_package_type_names[package_type], package_type);
}

static const struct cli_command s_package_type_commands[] = {
	{"package_type", "read package type from OTP2", cli_package_type_cmd},
};

int cli_package_type_init(void)
{
	return cli_register_commands(s_package_type_commands,
		ARRAY_SIZE(s_package_type_commands));
}
