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
	CLI_LOGI("sdio_host_driver {init|deinit}\r\n");
	CLI_LOGI("sdio_usr_intf {rd block_addr block_cnt} rd:read block\r\n");
	CLI_LOGI("sdio_usr_intf {wr block_addr block_cnt} wr:write block\r\n");
	CLI_LOGI("sdio_host --help\r\n");
}

static void cli_sdio_host_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        cli_sdio_host_help();
        return;
    }

    if (os_strcmp(argv[1], "init") == 0) {
        BK_LOG_ON_ERR(bk_sdio_storage_driver_init());
        CLI_LOGI("bk_sdio_storage_driver_init\n");
    } else if (os_strcmp(argv[1], "deinit") == 0) {
        BK_LOG_ON_ERR(bk_sdio_storage_driver_deinit());
        CLI_LOGI("bk_sdio_storage_driver_deinit\n");
    } else {
        cli_sdio_host_help();
        return;
    }
}

static void cli_sdio_usr_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    uint32_t blk_addr, blk_cnt;

    if((2 == argc) && (os_strcmp(argv[1], "exit") == 0)){
        CLI_LOGI("mshc_exit_enumerate\n");
        //mshc_exit_enumerate();
        return;
    }

    if (argc < 4) {
        cli_sdio_host_help();
        return;
    }else{            
            blk_addr = os_strtoul(argv[2], NULL, 10);
            blk_cnt = os_strtoul(argv[3], NULL, 10);
    }

    if (os_strcmp(argv[1], "rd") == 0) {
        CLI_LOGI("sdio_host read block:0x%x:0x%x\n", blk_addr, blk_cnt);
        //mshc_read_block(blk_addr, blk_cnt);
    } else if (os_strcmp(argv[1], "wr") == 0) {
        CLI_LOGI("sdio_host write block:0x%x:0x%x\n", blk_addr, blk_cnt);
        //mshc_write_block(blk_addr, blk_cnt);
    } else {
        cli_sdio_host_help();
        return;
    }
}

static void cli_sdio_host_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        //mshc_main(argc, argv);
}

#define WDT_CMD_CNT (sizeof(s_sdio_host_commands) / sizeof(struct cli_command))
static const struct cli_command s_sdio_host_commands[] = {
    {"sdio_host_driver", "{init|deinit}", cli_sdio_host_driver_cmd},
    {"sdio_host", "sdio_host", cli_sdio_host_cmd},
    {"sdio_usr_intf", "sdio_usr_intf", cli_sdio_usr_cmd}
};

int bk_sdio_host_register_cli_test_feature(void)
{
    return cli_register_commands(s_sdio_host_commands, WDT_CMD_CNT);
}
// eof

