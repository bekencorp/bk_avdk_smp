// Copyright 2020-2024 Beken
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
#include <driver/sdio_storage.h>
#include <driver/timer.h>
#include "sdio_storage_driver.h"

static void cli_sdio_host_help(void)
{
    CLI_LOGI("sdio_host_driver init\n");
    CLI_LOGI("sdio_host_driver deinit\n");
    CLI_LOGI("sdio_host start [timeout]\n");
}

static void cli_sdio_host_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        cli_sdio_host_help();
        return;
    }

    if (os_strcmp(argv[1], "init") == 0) {
        BK_LOG_ON_ERR(bk_sdio_storage_driver_init());
        CLI_LOGI("sdio_host driver init\n");
    } else if (os_strcmp(argv[1], "deinit") == 0) {
        BK_LOG_ON_ERR(bk_sdio_storage_driver_deinit());
        CLI_LOGI("sdio_host driver deinit\n");
    } else {
        cli_sdio_host_help();
        return;
    }
}

void test_sdio(uint8_t *param);

static void cli_sdio_host_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        uint8_t  param[2];

        if (argc < 2) {
                cli_sdio_host_help();
                return;
        }

        if (os_strcmp(argv[1], "start") == 0) {
                param[0] = 1;
                param[1] = 6;
                test_sdio(param);
        } else if (os_strcmp(argv[1], "stop") == 0) {
                CLI_LOGI("sdio_host stop\n");
        } else {
                cli_sdio_host_help();
                return;
        }
}

#define WDT_CMD_CNT (sizeof(s_sdio_host_commands) / sizeof(struct cli_command))
static const struct cli_command s_sdio_host_commands[] = {
    {"sdio_host_driver", "{init|deinit}", cli_sdio_host_driver_cmd},
    {"sdio_host", "sdio_host {start|stop|feed} [...]", cli_sdio_host_cmd}
};

int bk_sdio_host_register_cli_test_feature(void)
{
    return cli_register_commands(s_sdio_host_commands, WDT_CMD_CNT);
}
// eof

