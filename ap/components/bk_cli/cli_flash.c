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

#include <driver/flash.h>
#include <driver/flash_partition.h>
#include "cli.h"
#include "flash_driver.h"

#if CONFIG_TFM_FLASH_NSC
#include "tfm_flash_nsc.h"
#endif

static void cli_flash_help(void)
{
	CLI_LOGD("flash driver init\n");
	CLI_LOGD("flash_driver deinit\n");
	CLI_LOGD("flash {erase|write|read} [start_addr] [len]\n");
	CLI_LOGD("flash_partition show\n");
	CLI_LOGD("flash_erase_test ble\n");
}

static void cli_flash_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	if (argc < 2) {
		cli_flash_help();
		return;
	}

	uint32_t start_addr = os_strtoul(argv[2], NULL, 16);
	uint32_t len = os_strtoul(argv[3], NULL, 16);

	if (os_strcmp(argv[1], "erase") == 0) {
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_SECTOR_SIZE) {
			bk_flash_erase_sector(addr);
		}
		bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "read") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			os_memset(buf, 0, FLASH_PAGE_SIZE);
			bk_flash_read_bytes(addr, buf, FLASH_PAGE_SIZE);
			CLI_LOGD("flash read addr:%x\r\n", addr);

			CLI_LOGD("dump read flash data:\r\n");
			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_LOGD(NULL, "%02x ", buf[i * 16 + j]);
				}
				BK_LOGD(NULL, "\r\n");
			}
		}
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "write") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
			buf[i] = i;
		}
		bk_flash_set_protect_type(FLASH_PROTECT_NONE);
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			bk_flash_write_bytes(addr, buf, FLASH_PAGE_SIZE);
		}
		bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "get_id") == 0) {
		uint32_t flash_id = bk_flash_get_id();
		CLI_LOGD("flash_id:%x\r\n", flash_id);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "mutex_test") == 0) {
	#if CONFIG_FLASH_TEST
		extern void flash_svr_test_task(void * param);
		int task_pri = os_strtoul(argv[2], NULL, 16);
		rtos_create_thread(NULL, task_pri, "flash_test", flash_svr_test_task, 2048, NULL);
	#endif
		msg = CLI_CMD_RSP_SUCCEED;
	} else {
		cli_flash_help();
		msg = CLI_CMD_RSP_ERROR;
	}

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#if CONFIG_TFM_FLASH_NSC
static void cli_flash_cmd_s(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	if (argc < 2) {
		cli_flash_help();
		return;
	}

	uint32_t start_addr = os_strtoul(argv[2], NULL, 16);
	uint32_t len = os_strtoul(argv[3], NULL, 16);

	if (os_strcmp(argv[1], "erase") == 0) {
		psa_flash_set_protect_type(FLASH_PROTECT_NONE);
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_SECTOR_SIZE) {
			psa_flash_erase_sector(addr);
		}
		psa_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "read") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			os_memset(buf, 0, FLASH_PAGE_SIZE);
			psa_flash_read_bytes(addr, buf, FLASH_PAGE_SIZE);
			CLI_LOGD("flash read addr:%x\r\n", addr);

			CLI_LOGD("dump read flash data:\r\n");
			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_LOGD(NULL, "%02x ", buf[i * 16 + j]);
				}
				BK_LOGD(NULL, "\r\n");
			}
		}
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "write") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
			buf[i] = i;
		}
		int level = rtos_enter_critical();
		psa_flash_set_protect_type(FLASH_PROTECT_NONE);
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			psa_flash_write_bytes(addr, buf, FLASH_PAGE_SIZE);
		}
		psa_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		rtos_exit_critical(level);
		msg = CLI_CMD_RSP_SUCCEED;
	} else {
		cli_flash_help();
		msg = CLI_CMD_RSP_ERROR;
	}

	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
#endif

static void cli_flash_partition_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_logic_partition_t *partition;

	if (os_strcmp(argv[1], "show") == 0) {
		for (bk_partition_t par= BK_PARTITION_BOOTLOADER; par < BK_PARTITIONS_TABLE_SIZE; par++) {
			partition = bk_flash_partition_get_info(par);
			if (partition == NULL)
				continue;

			CLI_LOGD("%4d | %11s |  Dev:%d  | 0x%08lx | 0x%08lx |\r\n", par,
					partition->partition_description, partition->partition_owner,
					partition->partition_start_addr, partition->partition_length);
		}
	} else {
		cli_flash_help();
	}
}

void flash_erase_with_ble_sleep(uint32_t erase_addr)
{
    uint32_t  anchor_time = 0;
    uint32_t  temp_time = 0;
    uint8_t   flash_erase_ready = 0;

    anchor_time = rtos_get_time();
    while(1)
    {
        flash_erase_ready = ble_callback_deal_handler(ERASE_FLASH_TIMEOUT);
        temp_time = rtos_get_time();
        if(temp_time >= anchor_time)
        {
            temp_time -= anchor_time;
        }
        else
        {
            temp_time += (0xFFFFFFFF - anchor_time);
        }
        if(temp_time >= ERASE_TOUCH_TIMEOUT)
            flash_erase_ready = 1;
        if(flash_erase_ready == 1)
        {
            bk_flash_erase_sector(erase_addr);
            flash_erase_ready = 0;
            break;
        }
        else
        {
            rtos_delay_milliseconds(2);
        }
    }
}

static void cli_flash_erase_test_with_ble(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	uint32_t start_addr = 0x260000;
	uint32_t erase_len = 0x180000;

	if (os_strcmp(argv[1], "ble") == 0) {

		for (uint32_t erase_addr = start_addr; erase_addr <= (start_addr + erase_len);) {
			flash_erase_with_ble_sleep(erase_addr);
			erase_addr += FLASH_SECTOR_SIZE;
			CLI_LOGD("erase_addr:%x\r\n", erase_addr);
		}
		CLI_LOGD("cli_flash_erase_test_with_ble finish.\r\n");
		msg = CLI_CMD_RSP_SUCCEED;
	} else {
		cli_flash_help();
		msg = CLI_CMD_RSP_ERROR;
	}
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define FLASH_CMD_CNT (sizeof(s_flash_commands) / sizeof(struct cli_command))
static const struct cli_command s_flash_commands[] = {
	{"flash", "flash {erase|read|write} [start_addr] [len]", cli_flash_cmd},
#if CONFIG_TFM_FLASH_NSC
	{"flash_s", "flash {erase|read|write} [start_addr] [len]", cli_flash_cmd_s},
#endif
	{"flash_partition", "flash_partition {show}", cli_flash_partition_cmd},
	{"flash_erase_test", "cli_flash_erase_test with ble connecting", cli_flash_erase_test_with_ble},
};

int cli_flash_init(void)
{
	BK_LOG_ON_ERR(bk_flash_driver_init());
	return cli_register_commands(s_flash_commands, FLASH_CMD_CNT);
}

