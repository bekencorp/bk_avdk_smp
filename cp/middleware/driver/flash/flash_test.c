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
#include <os/os.h>

#if CONFIG_TFM_FLASH_NSC
#include "tfm_flash_nsc.h"
#endif

/* CONFIG macros for periodic flash test address */
#ifndef CONFIG_FLASH_TEST_TASK_ADDR
#define CONFIG_FLASH_TEST_TASK_ADDR     0x7F0000
#endif

#ifndef CONFIG_FLASH_TEST_TASK_SIZE
#define CONFIG_FLASH_TEST_TASK_SIZE     0x100
#endif

#ifndef CONFIG_FLASH_TEST_TASK_INTERVAL_MS
#define CONFIG_FLASH_TEST_TASK_INTERVAL_MS  1000
#endif

/* Task handle for periodic flash test */
static beken_thread_t s_flash_test_task_handle = NULL;
static bool s_flash_test_task_running = false;

static inline void flash_test_task_wdt_feed(void)
{
#if CONFIG_TASK_WDT
extern void bk_task_wdt_feed(void);
	bk_task_wdt_feed();
#endif
}

static void cli_flash_help(void)
{
	CLI_LOGD("flash driver init\n");
	CLI_LOGD("flash_driver deinit\n");
	CLI_LOGD("flash {erase|write|read} [start_addr] [len]\n");
	CLI_LOGD("flash_partition show\n");
	CLI_LOGD("flash_erase_test ble\n");
	CLI_LOGD("flash_test_task {start|stop} - start/stop periodic flash test task\n");
}

static void cli_flash_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	if (argc < 3) {
		cli_flash_help();
		return;
	}

	uint32_t start_addr = os_strtoul(argv[2], NULL, 16);
	uint32_t len = os_strtoul(argv[3], NULL, 16);

	if (os_strcmp(argv[1], "erase") == 0) {
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_SECTOR_SIZE) {
			flash_test_task_wdt_feed();
			bk_flash_erase_sector(addr);
		}
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "read") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			flash_test_task_wdt_feed();
			os_memset(buf, 0, FLASH_PAGE_SIZE);
			bk_flash_read_bytes(addr, buf, FLASH_PAGE_SIZE);
			BK_DUMP_OUT("flash read addr:%x\r\n", addr);

			BK_DUMP_OUT("dump read flash data:\r\n");
			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_DUMP_OUT("%02x ", buf[i * 16 + j]);
				}
				BK_DUMP_OUT("\r\n");
				flash_test_task_wdt_feed();
			}
		}
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "write") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
			buf[i] = i;
		}
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			flash_test_task_wdt_feed();
			bk_flash_write_bytes(addr, buf, FLASH_PAGE_SIZE);
		}
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "get_id") == 0) {
		uint32_t flash_id = bk_flash_get_id();
		CLI_LOGD("flash_id:%x\r\n", flash_id);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "set_line") == 0) {
		/*enable FLASH_QUAD_ENABLE first*/
		uint16_t line_mode = os_strtoul(argv[2], NULL, 16);
		if (line_mode == 2) {
			bk_flash_power_saving_enter();
			CLI_LOGI("switch to 2 line.\r\n");
		} else {
			bk_flash_power_saving_exit();
			CLI_LOGI("switch to 4 line.\r\n");
		}
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
			flash_test_task_wdt_feed();
			psa_flash_erase_sector(addr);
		}
		psa_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "read") == 0) {
		uint8_t buf[FLASH_PAGE_SIZE] = {0};
		for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_PAGE_SIZE) {
			flash_test_task_wdt_feed();
			os_memset(buf, 0, FLASH_PAGE_SIZE);
			psa_flash_read_bytes(addr, buf, FLASH_PAGE_SIZE);
			CLI_LOGD("flash read addr:%x\r\n", addr);

			CLI_LOGD("dump read flash data:\r\n");
			for (uint32_t i = 0; i < 16; i++) {
				for (uint32_t j = 0; j < 16; j++) {
					BK_LOGD(NULL,"%02x ", buf[i * 16 + j]);
				}
				BK_LOGD(NULL,"\r\n");
				flash_test_task_wdt_feed();
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
			flash_test_task_wdt_feed();
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

#if 0
/*
 * flash erase will affect ble connecting, unless flash erase while ble is sleeping
 * This test case aims to do flash erasing with ble sleeping and connecting
 */
#define S_WAKE_UP    (0)
#define S_SLEEP      (1)
#define S_POWER_OFF  (2)
#define S_NO_BT      (3)

#define ERASE_TOUCH_TIMEOUT  (3000)//ms
#define ERASE_FLASH_TIMEOUT  (56)//ms

static u32  bt_sleepend_time = -1;  //when cb return,bt is sleep ,recoeding the bt will sleep how long
static u32  bt_cb_anchor_time = 0;    //when cb return ,record current time ;
static u8   bt_sleep_state = S_NO_BT;  //record bt state;the default is 3(S_NO_BT);

static void flash_test_ble_sleep_cb(uint8_t is_sleeping, uint32_t slp_period);
typedef void (*ble_sleep_state_cb)(uint8_t is_sleeping, uint32_t slp_period);
extern void bk_ble_register_sleep_state_callback(ble_sleep_state_cb cb);

static void flash_test_ble_sleep_cb(uint8_t is_sleeping, uint32_t slp_period)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

    bt_sleep_state = is_sleeping ;
    bt_cb_anchor_time = rtos_get_time();
    if (is_sleeping == S_SLEEP)
    {
       bt_sleepend_time = bt_cb_anchor_time + slp_period/32;
    }

    GLOBAL_INT_RESTORE();
}

static int ble_callback_deal_handler(uint32_t deal_flash_time)
{
    uint32_t  cur_time =0;
    uint32_t  temp_time = 0;
    int       ret_val = 0;

    cur_time = rtos_get_time();

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();

    do
    {
        if(bt_sleep_state == S_POWER_OFF)     //poweroff
        {
            ret_val = 1;
            break;
        }
        else if(bt_sleep_state == S_WAKE_UP) //wakeup
        {
            if(cur_time >= bt_cb_anchor_time)
            {
                temp_time = (cur_time - bt_cb_anchor_time);
            }
            else
            {
                temp_time = 0xFFFFFFFF - bt_cb_anchor_time + cur_time;
            }

            if(temp_time >= ERASE_TOUCH_TIMEOUT)
            {
                bt_sleep_state = S_NO_BT;
                ret_val = 1;
                break;
            }

            ret_val = 0;
            break;
        }
        else if(bt_sleep_state == S_SLEEP) //sleep
        {
            if(bt_sleepend_time > bt_cb_anchor_time)
            {
                if(bt_sleepend_time < cur_time)
                {
                     ret_val = 1;
                     break;
                }
                else if(cur_time < bt_cb_anchor_time)
                {
                     ret_val = 1;
                     break;
                }
                else if((bt_sleepend_time - cur_time) >= deal_flash_time)
                {
                     ret_val = 1;
                     break;
                }
                else
                {
                     ret_val = 0;
                     break;
                }
            }
            else
            {
                temp_time = 0;
                if((cur_time > bt_sleepend_time)&&(bt_cb_anchor_time > cur_time))
                {
                     ret_val = 1;
                     break;
                }
                else if(bt_cb_anchor_time <= cur_time)
                {
                    temp_time = 0xFFFFFFFF - cur_time + bt_sleepend_time;
                }
                else
                {
                    temp_time = bt_sleepend_time - cur_time;
                }

                if(temp_time >= deal_flash_time )
                {
                     ret_val = 1;
                     break;
                }
                else
                {
                     ret_val = 0;
                     break;
                }
            }
        }
        else
        {
             ret_val = 1;
             break;
        }
    }while(0);

    GLOBAL_INT_RESTORE();
    return ret_val;
}
#endif
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

#if 0
static void cli_flash_erase_test_with_ble(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;
	uint32_t start_addr = 0x260000;
	uint32_t erase_len = 0x180000;

	if (os_strcmp(argv[1], "ble") == 0) {
#if (CONFIG_BLUETOOTH)
		bk_ble_register_sleep_state_callback(flash_test_ble_sleep_cb);
#endif

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
#endif

/* Periodic flash test task */
static void flash_test_task_worker(beken_thread_arg_t arg)
{
	uint32_t addr = CONFIG_FLASH_TEST_TASK_ADDR;
	uint32_t size = CONFIG_FLASH_TEST_TASK_SIZE;
	uint8_t *write_buf = NULL;
	uint8_t *read_buf = NULL;
	uint32_t iteration = 0;

	write_buf = (uint8_t *)os_malloc(size);
	read_buf = (uint8_t *)os_malloc(size);

	if (!write_buf || !read_buf) {
		CLI_LOGE("flash_test_task: Failed to allocate buffers\n");
		if (write_buf) os_free(write_buf);
		if (read_buf) os_free(read_buf);
		s_flash_test_task_running = false;
		return;
	}

	CLI_LOGD("flash_test_task: Started, addr=0x%08X, size=0x%X, interval=%dms\n",
		addr, size, CONFIG_FLASH_TEST_TASK_INTERVAL_MS);

	while (s_flash_test_task_running) {
		iteration++;
		CLI_LOGD("flash_test_task: Iteration %lu - Erase\n", iteration);

		/* Step 1: Erase sector */
		uint32_t sector_addr = addr & ~(FLASH_SECTOR_SIZE - 1);
		bk_err_t ret = bk_flash_erase_sector(sector_addr);
		if (ret != BK_OK) {
			CLI_LOGE("flash_test_task: Erase failed at 0x%08X, ret=%d\n", sector_addr, ret);
		}

		/* Step 2: Write data */
		for (uint32_t i = 0; i < size; i++) {
			write_buf[i] = (uint8_t)((iteration + i) & 0xFF);
		}
		CLI_LOGD("flash_test_task: Iteration %lu - Write\n", iteration);
		ret = bk_flash_write_bytes(addr, write_buf, size);
		if (ret != BK_OK) {
			CLI_LOGE("flash_test_task: Write failed at 0x%08X, ret=%d\n", addr, ret);
		}

		/* Step 3: Read and verify */
		os_memset(read_buf, 0, size);
		CLI_LOGD("flash_test_task: Iteration %lu - Read\n", iteration);
		ret = bk_flash_read_bytes(addr, read_buf, size);
		if (ret != BK_OK) {
			CLI_LOGE("flash_test_task: Read failed at 0x%08X, ret=%d\n", addr, ret);
		} else {
			/* Verify data */
			bool verify_ok = true;
			for (uint32_t i = 0; i < size; i++) {
				if (read_buf[i] != write_buf[i]) {
					CLI_LOGE("flash_test_task: Verify failed at offset %lu: expected 0x%02X, got 0x%02X\n",
						i, write_buf[i], read_buf[i]);
					verify_ok = false;
					break;
				}
			}
			if (verify_ok) {
				CLI_LOGD("flash_test_task: Iteration %lu - Verify OK\n", iteration);
			}
		}

		/* Delay before next iteration */
		rtos_delay_milliseconds(CONFIG_FLASH_TEST_TASK_INTERVAL_MS);
	}

	os_free(write_buf);
	os_free(read_buf);
	CLI_LOGD("flash_test_task: Stopped\n");
	s_flash_test_task_handle = NULL;
}

/* CLI command handler for flash test task */
static void cli_flash_test_task_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;

	if (argc < 2) {
		CLI_LOGD("Usage: flash_test_task {start|stop}\n");
		CLI_LOGD("  addr=0x%08X, size=0x%X, interval=%dms\n",
			CONFIG_FLASH_TEST_TASK_ADDR, CONFIG_FLASH_TEST_TASK_SIZE, CONFIG_FLASH_TEST_TASK_INTERVAL_MS);
		msg = CLI_CMD_RSP_ERROR;
		goto out;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (s_flash_test_task_running) {
			CLI_LOGE("flash_test_task: Already running\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		s_flash_test_task_running = true;
		uint32_t task_prio = 5; /* Default priority */
		rtos_create_thread(&s_flash_test_task_handle, task_prio,
			"flash_test_task",
			(beken_thread_function_t)flash_test_task_worker,
			CONFIG_APP_MAIN_TASK_STACK_SIZE,
			(beken_thread_arg_t)0);
		CLI_LOGD("flash_test_task: Started\n");
		msg = CLI_CMD_RSP_SUCCEED;
	} else if (os_strcmp(argv[1], "stop") == 0) {
		if (!s_flash_test_task_running) {
			CLI_LOGE("flash_test_task: Not running\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		s_flash_test_task_running = false;
		if (s_flash_test_task_handle) {
			rtos_delete_thread(&s_flash_test_task_handle);
			s_flash_test_task_handle = NULL;
		}
		CLI_LOGD("flash_test_task: Stopped\n");
		msg = CLI_CMD_RSP_SUCCEED;
	} else {
		CLI_LOGE("flash_test_task: Invalid command (must be 'start' or 'stop')\n");
		msg = CLI_CMD_RSP_ERROR;
	}

out:
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define FLASH_CMD_CNT (sizeof(s_flash_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_flash_commands[] = {
	{"flash", "flash {erase|read|write} [start_addr] [len]", cli_flash_cmd},
#if CONFIG_TFM_FLASH_NSC
	{"flash_s", "flash {erase|read|write} [start_addr] [len]", cli_flash_cmd_s},
#endif
	{"flash_partition", "flash_partition {show}", cli_flash_partition_cmd},
	// {"flash_erase_test", "cli_flash_erase_test with ble connecting", cli_flash_erase_test_with_ble},
	{"flash_test_task", "flash_test_task {start|stop} - periodic flash test", cli_flash_test_task_cmd},
};

int bk_flash_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_flash_driver_init());
	return cli_register_module_test_feature(s_flash_commands, FLASH_CMD_CNT);
}

