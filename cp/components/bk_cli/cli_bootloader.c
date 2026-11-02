// Copyright 2023-2028 Beken
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

/* `bl2_version` CLI, a thin wrapper over the reader in bk_bootloader.c. */

#include <os/os.h>
#include "cli.h"
#include "bk_bootloader.h"

static void cli_bl2_version_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t ver = bk_bootloader_get_version();

	if (ver == BK_BOOTLOADER_VERSION_ABSENT) {
		BK_LOGD(NULL, "bootloader version: no stamp (legacy bootloader)\r\n");
		return;
	}
	if (ver == BK_BOOTLOADER_VERSION_CORRUPT) {
		BK_LOGD(NULL, "bootloader version: stamp corrupt\r\n");
		return;
	}

	BK_LOGD(NULL, "bootloader version: %u.%u.%u (0x%06x)\r\n",
		(unsigned)((ver >> 16) & 0xFFu), (unsigned)((ver >> 8) & 0xFFu),
		(unsigned)(ver & 0xFFu), (unsigned)ver);
}

static const struct cli_command s_bl2_commands[] = {
	{"bl2_version", "bl2_version: show the bootloader version", cli_bl2_version_cmd},
};

int cli_bootloader_init(void)
{
	return cli_register_commands(s_bl2_commands,
				     sizeof(s_bl2_commands) / sizeof(struct cli_command));
}
