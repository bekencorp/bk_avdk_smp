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
#include <driver/timer.h>
#include "sdio_storage_driver.h"


#if 1
#include <driver/sd_card.h>
/*
sdtest I 0 --
sdtest R secnum
sdtest W secnum
*/
extern uint32_t sdcard_intf_test(void);
extern uint32_t test_sdcard_read(uint32_t blk, uint32_t blk_cnt);
extern uint32_t test_sdcard_write(uint32_t blk, uint32_t blk_cnt, uint32_t wr_val);
extern UINT32 test_sdcard_loop_test(UINT32 blk, UINT32 blk_cnt);
extern void sdcard_intf_close(void);
beken_thread_t sd_auto_read_test_handle = NULL;

static void sd_auto_read_test(void *arg) {
	uint32_t blk_num = 0;
	uint32_t blk_cnt = 8;

	while (1) {
		for(blk_num = 0; blk_num < 1000; blk_num++) {
			test_sdcard_write(blk_num, blk_cnt, 0x12345678);
			test_sdcard_loop_test(blk_num, blk_cnt);
			rtos_delay_milliseconds(5);
		}
	}
	rtos_delete_thread(&sd_auto_read_test_handle);
}


static void sd_operate(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint32_t cmd;
	uint32_t blknum = 0, blkcnt = 1, wr_val = 0x12345678;
	uint32_t task_prio;
	uint32_t ret;
	if (argc > 1) {
		cmd = argv[1][0];
		if (argc > 2)
		{
			blknum = os_strtoul(argv[2], NULL, 10);
			if (argc > 3)
			{
				blkcnt = os_strtoul(argv[3], NULL, 10);
				if (argc > 4)
				{
					wr_val = os_strtoul(argv[4], NULL, 16);
				}
			}
		}
		switch (cmd) {
		case 'I':
			ret = sdcard_intf_test();
			os_printf("init ret=%x\r\n", ret);
			break;
		case 'R':
			ret = test_sdcard_read(blknum, blkcnt);
			os_printf("read ret=%x,blknum=%d,blkcnt=%d\r\n", ret, blknum, blkcnt);
			break;
		case 'W':
			ret = test_sdcard_write(blknum, blkcnt, wr_val);
			os_printf("write ret=%x,blknum=%d,blkcnt=%d, wr_val=0x%08x\r\n", ret, blknum, blkcnt, wr_val);
			break;
		case 'C':
			sdcard_intf_close();
			os_printf("sdtest close \r\n");
			break;
		case 'S':
			//bk_sd_card_set_clock(blknum);
			break;


		case 'A':
			task_prio = os_strtoul(argv[2], NULL, 10);
			rtos_create_thread(&sd_auto_read_test_handle, task_prio,
				"sd_auto_read_test",
				(beken_thread_function_t) sd_auto_read_test,
				CONFIG_APP_MAIN_TASK_STACK_SIZE,
				(beken_thread_arg_t)0);
			break;

		case 'D':
			if (sd_auto_read_test_handle) {
				rtos_delete_thread(&sd_auto_read_test_handle);
				sd_auto_read_test_handle = NULL;
				BK_DUMP_OUT("idle_read_flash task stop\n");
			}
			break;


		default:
			break;
		}
	} else
		os_printf("cmd param error\r\n");
}





//#define SD_CMD_CNT (sizeof(s_sd_commands) / sizeof(struct cli_command))
//static const struct cli_command s_sd_commands[] = {
//#if CONFIG_SDCARD
//	{"sdtest", "sdtest <cmd>", sd_operate},
//#endif
//};



#endif


static void cli_sdio_host_help(void)
{
    CLI_LOGI("sdio_host start [timeout]\n");
}

void test_sdio(uint8_t *param);

static void cli_sdio_host_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
        uint8_t  param[5];

        if (argc < 2) {
                cli_sdio_host_help();
                return;
        }

        if (os_strcmp(argv[1], "start") == 0) {
                param[0] = os_strtoul(argv[2], NULL, 10);
                param[1] = os_strtoul(argv[3], NULL, 10);
                param[2] = os_strtoul(argv[4], NULL, 10);
                param[3] = os_strtoul(argv[5], NULL, 10);

                test_sdio(param);
        } else if (os_strcmp(argv[1], "stop") == 0) {
                CLI_LOGI("sdio_host stop\n");
        } else {
                cli_sdio_host_help();
                return;
        }
}

#define SDIO_CMD_CNT (sizeof(s_sdio_host_commands) / sizeof(struct cli_command))
static const struct cli_command s_sdio_host_commands[] = {
//    {"sdio_host", "sdio_host {start|stop|feed} [...]", cli_sdio_host_cmd},
	{"sdtest", "sdtest <cmd>", sd_operate}
};

int bk_sdio_host_register_cli_test_feature(void)
{
	CLI_LOGI("bk_sdio_host_register_cli_test_feature \r\n");
    return cli_register_commands(s_sdio_host_commands, SDIO_CMD_CNT);
}
// eof

