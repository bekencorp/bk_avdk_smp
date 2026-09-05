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
#include <stdbool.h>
#include <stdint.h>

#if CONFIG_TFM_FLASH_NSC
#include "tfm_flash_nsc.h"
#endif

/* CONFIG macros for periodic flash test addresses */
#ifndef CONFIG_FLASH_TEST_TASK1_ADDR
#define CONFIG_FLASH_TEST_TASK1_ADDR    0x7EF000
#endif

#ifndef CONFIG_FLASH_TEST_TASK2_ADDR
#define CONFIG_FLASH_TEST_TASK2_ADDR    0x7E0000
#endif

#ifndef CONFIG_FLASH_TEST_TASK_SIZE
#define CONFIG_FLASH_TEST_TASK_SIZE     0x100
#endif

#ifndef CONFIG_FLASH_TEST_TASK_INTERVAL_MS
#define CONFIG_FLASH_TEST_TASK_INTERVAL_MS  1000
#endif

/* Task handles for periodic flash test */
static beken_thread_t s_flash_test_task1_handle = NULL;
static beken_thread_t s_flash_test_task2_handle = NULL;
static bool s_flash_test_task1_running = false;
static bool s_flash_test_task2_running = false;

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
	CLI_LOGD("flash_test_task {start|stop} [1|2] - start/stop periodic flash test task\n");
	CLI_LOGD("flash_conc {sns|smp|peer|stop|stat|help} - S/NS & multi-core concurrent E/W/R\n");
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
					BK_LOGD(NULL, "%02x ", buf[i * 16 + j]);
				}
				BK_LOGD(NULL, "\r\n");
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

#if 1
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

int ble_callback_deal_handler(uint32_t deal_flash_time)
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

/* Periodic flash test task 1 */
static void flash_test_task1_worker(beken_thread_arg_t arg)
{
	uint32_t addr = CONFIG_FLASH_TEST_TASK1_ADDR;
	uint32_t size = CONFIG_FLASH_TEST_TASK_SIZE;
	uint8_t *write_buf = NULL;
	uint8_t *read_buf = NULL;
	uint32_t iteration = 0;

	write_buf = (uint8_t *)os_malloc(size);
	read_buf = (uint8_t *)os_malloc(size);

	if (!write_buf || !read_buf) {
		CLI_LOGE("flash_test_task1: Failed to allocate buffers\n");
		if (write_buf) os_free(write_buf);
		if (read_buf) os_free(read_buf);
		s_flash_test_task1_running = false;
		return;
	}

	CLI_LOGD("flash_test_task1: Started, addr=0x%08X, size=0x%X, interval=%dms\n",
		addr, size, CONFIG_FLASH_TEST_TASK_INTERVAL_MS);

	while (s_flash_test_task1_running) {
		iteration++;
		CLI_LOGD("flash_test_task1: Iteration %lu - Erase\n", iteration);

		/* Step 1: Erase sector */
		uint32_t sector_addr = addr & ~(FLASH_SECTOR_SIZE - 1);
		bk_err_t ret = bk_flash_erase_sector(sector_addr);
		if (ret != BK_OK) {
			CLI_LOGE("flash_test_task1: Erase failed at 0x%08X, ret=%d\n", sector_addr, ret);
		}

		/* Step 2: Write data */
		for (uint32_t i = 0; i < size; i++) {
			write_buf[i] = (uint8_t)((iteration + i) & 0xFF);
		}
		CLI_LOGD("flash_test_task1: Iteration %lu - Write\n", iteration);
		ret = bk_flash_write_bytes(addr, write_buf, size);
		if (ret != BK_OK) {
			CLI_LOGE("flash_test_task1: Write failed at 0x%08X, ret=%d\n", addr, ret);
		}

		/* Step 3: Read and verify */
		os_memset(read_buf, 0, size);
		CLI_LOGD("flash_test_task1: Iteration %lu - Read\n", iteration);
		ret = bk_flash_read_bytes(addr, read_buf, size);
		if (ret != BK_OK) {
			CLI_LOGE("flash_test_task1: Read failed at 0x%08X, ret=%d\n", addr, ret);
		} else {
			/* Verify data */
			bool verify_ok = true;
			for (uint32_t i = 0; i < size; i++) {
				if (read_buf[i] != write_buf[i]) {
					CLI_LOGE("flash_test_task1: Verify failed at offset %lu: expected 0x%02X, got 0x%02X\n",
						i, write_buf[i], read_buf[i]);
					verify_ok = false;
					break;
				}
			}
			if (verify_ok) {
				CLI_LOGD("flash_test_task1: Iteration %lu - Verify OK\n", iteration);
			}
		}

		/* Delay before next iteration */
		rtos_delay_milliseconds(CONFIG_FLASH_TEST_TASK_INTERVAL_MS);
	}

	os_free(write_buf);
	os_free(read_buf);
	CLI_LOGD("flash_test_task1: Stopped\n");
	s_flash_test_task1_handle = NULL;
}

/* Periodic flash test task 2 */
static void flash_test_task2_worker(beken_thread_arg_t arg)
{
	uint32_t addr = CONFIG_FLASH_TEST_TASK2_ADDR;
	uint32_t size = CONFIG_FLASH_TEST_TASK_SIZE;
	uint8_t *write_buf = NULL;
	uint8_t *read_buf = NULL;
	uint32_t iteration = 0;

	write_buf = (uint8_t *)os_malloc(size);
	read_buf = (uint8_t *)os_malloc(size);

	if (!write_buf || !read_buf) {
		CLI_LOGE("flash_test_task2: Failed to allocate buffers\n");
		if (write_buf) os_free(write_buf);
		if (read_buf) os_free(read_buf);
		s_flash_test_task2_running = false;
		return;
	}

	CLI_LOGD("flash_test_task2: Started, addr=0x%08X, size=0x%X, interval=%dms\n",
		addr, size, CONFIG_FLASH_TEST_TASK_INTERVAL_MS);

	while (s_flash_test_task2_running) {
		iteration++;
		CLI_LOGD("flash_test_task2: Iteration %lu - Erase\n", iteration);

		/* Step 1: Erase sector */
		uint32_t sector_addr = addr & ~(FLASH_SECTOR_SIZE - 1);
		bk_err_t ret = bk_flash_erase_sector(sector_addr);
		if (ret != BK_OK) {
			CLI_LOGE("flash_test_task2: Erase failed at 0x%08X, ret=%d\n", sector_addr, ret);
		}

		/* Step 2: Write data */
		for (uint32_t i = 0; i < size; i++) {
			write_buf[i] = (uint8_t)((iteration + i + 0x80) & 0xFF);
		}
		CLI_LOGD("flash_test_task2: Iteration %lu - Write\n", iteration);
		ret = bk_flash_write_bytes(addr, write_buf, size);
		if (ret != BK_OK) {
			CLI_LOGE("flash_test_task2: Write failed at 0x%08X, ret=%d\n", addr, ret);
		}

		/* Step 3: Read and verify */
		os_memset(read_buf, 0, size);
		CLI_LOGD("flash_test_task2: Iteration %lu - Read\n", iteration);
		ret = bk_flash_read_bytes(addr, read_buf, size);
		if (ret != BK_OK) {
			CLI_LOGE("flash_test_task2: Read failed at 0x%08X, ret=%d\n", addr, ret);
		} else {
			/* Verify data */
			bool verify_ok = true;
			for (uint32_t i = 0; i < size; i++) {
				if (read_buf[i] != write_buf[i]) {
					CLI_LOGE("flash_test_task2: Verify failed at offset %lu: expected 0x%02X, got 0x%02X\n",
						i, write_buf[i], read_buf[i]);
					verify_ok = false;
					break;
				}
			}
			if (verify_ok) {
				CLI_LOGD("flash_test_task2: Iteration %lu - Verify OK\n", iteration);
			}
		}

		/* Delay before next iteration */
		rtos_delay_milliseconds(CONFIG_FLASH_TEST_TASK_INTERVAL_MS);
	}

	os_free(write_buf);
	os_free(read_buf);
	CLI_LOGD("flash_test_task2: Stopped\n");
	s_flash_test_task2_handle = NULL;
}

/* CLI command handler for flash test task */
static void cli_flash_test_task_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = NULL;

	if (argc < 2) {
		CLI_LOGD("Usage: flash_test_task {start|stop} [1|2]\n");
		CLI_LOGD("  Task1: addr=0x%08X, size=0x%X\n", CONFIG_FLASH_TEST_TASK1_ADDR, CONFIG_FLASH_TEST_TASK_SIZE);
		CLI_LOGD("  Task2: addr=0x%08X, size=0x%X\n", CONFIG_FLASH_TEST_TASK2_ADDR, CONFIG_FLASH_TEST_TASK_SIZE);
		msg = CLI_CMD_RSP_ERROR;
		goto out;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		if (argc < 3) {
			CLI_LOGE("flash_test_task: Please specify task number (1 or 2)\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		uint32_t task_num = os_strtoul(argv[2], NULL, 10);
		uint32_t task_prio = 5; /* Default priority */

		if (task_num == 1) {
			if (s_flash_test_task1_running) {
				CLI_LOGE("flash_test_task1: Already running\n");
				msg = CLI_CMD_RSP_ERROR;
				goto out;
			}
			s_flash_test_task1_running = true;
			rtos_create_thread(&s_flash_test_task1_handle, task_prio,
				"flash_test_task1",
				(beken_thread_function_t)flash_test_task1_worker,
				CONFIG_APP_MAIN_TASK_STACK_SIZE,
				(beken_thread_arg_t)0);
			CLI_LOGD("flash_test_task1: Started\n");
			msg = CLI_CMD_RSP_SUCCEED;
		} else if (task_num == 2) {
			if (s_flash_test_task2_running) {
				CLI_LOGE("flash_test_task2: Already running\n");
				msg = CLI_CMD_RSP_ERROR;
				goto out;
			}
			s_flash_test_task2_running = true;
			rtos_create_thread(&s_flash_test_task2_handle, task_prio,
				"flash_test_task2",
				(beken_thread_function_t)flash_test_task2_worker,
				CONFIG_APP_MAIN_TASK_STACK_SIZE,
				(beken_thread_arg_t)0);
			CLI_LOGD("flash_test_task2: Started\n");
			msg = CLI_CMD_RSP_SUCCEED;
		} else {
			CLI_LOGE("flash_test_task: Invalid task number (must be 1 or 2)\n");
			msg = CLI_CMD_RSP_ERROR;
		}
	} else if (os_strcmp(argv[1], "stop") == 0) {
		if (argc < 3) {
			CLI_LOGE("flash_test_task: Please specify task number (1 or 2)\n");
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}

		uint32_t task_num = os_strtoul(argv[2], NULL, 10);

		if (task_num == 1) {
			if (!s_flash_test_task1_running) {
				CLI_LOGE("flash_test_task1: Not running\n");
				msg = CLI_CMD_RSP_ERROR;
				goto out;
			}
			s_flash_test_task1_running = false;
			if (s_flash_test_task1_handle) {
				rtos_delete_thread(&s_flash_test_task1_handle);
				s_flash_test_task1_handle = NULL;
			}
			CLI_LOGD("flash_test_task1: Stopped\n");
			msg = CLI_CMD_RSP_SUCCEED;
		} else if (task_num == 2) {
			if (!s_flash_test_task2_running) {
				CLI_LOGE("flash_test_task2: Not running\n");
				msg = CLI_CMD_RSP_ERROR;
				goto out;
			}
			s_flash_test_task2_running = false;
			if (s_flash_test_task2_handle) {
				rtos_delete_thread(&s_flash_test_task2_handle);
				s_flash_test_task2_handle = NULL;
			}
			CLI_LOGD("flash_test_task2: Stopped\n");
			msg = CLI_CMD_RSP_SUCCEED;
		} else {
			CLI_LOGE("flash_test_task: Invalid task number (must be 1 or 2)\n");
			msg = CLI_CMD_RSP_ERROR;
		}
	} else {
		CLI_LOGE("flash_test_task: Invalid command (must be 'start' or 'stop')\n");
		msg = CLI_CMD_RSP_ERROR;
	}

out:
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}


/* ---- concurrent S/NS & multi-core flash stress (flash_conc) ---- */
#ifndef FLASH_SECTOR_SIZE
#define FLASH_SECTOR_SIZE 0x1000
#endif

/* USR_CONFIG @ 0x4E7000, size 0xF000 — keep slots non-overlapping with CP peer. */
#ifndef CONFIG_FLASH_CONC_NS_ADDR
#define CONFIG_FLASH_CONC_NS_ADDR     0x4E7000
#endif
#ifndef CONFIG_FLASH_CONC_S_ADDR
#define CONFIG_FLASH_CONC_S_ADDR      0x4E8000
#endif
#ifndef CONFIG_FLASH_CONC_SMP0_ADDR
#define CONFIG_FLASH_CONC_SMP0_ADDR   0x4E9000
#endif
#ifndef CONFIG_FLASH_CONC_SMP1_ADDR
#define CONFIG_FLASH_CONC_SMP1_ADDR   0x4EA000
#endif
#ifndef CONFIG_FLASH_CONC_PEER_ADDR
#define CONFIG_FLASH_CONC_PEER_ADDR   0x4EB000
#endif

#ifndef CONFIG_FLASH_CONC_SIZE
#define CONFIG_FLASH_CONC_SIZE        0x100
#endif
#ifndef CONFIG_FLASH_CONC_INTERVAL_MS
#define CONFIG_FLASH_CONC_INTERVAL_MS 50
#endif

#define FLASH_CONC_TASK_PRIO         5
#define FLASH_CONC_TASK_STACK        2048
#define FLASH_CONC_LOG_EVERY         20

typedef enum {
	FLASH_CONC_PATH_NS = 0,
	FLASH_CONC_PATH_S  = 1,
} flash_conc_path_t;

typedef enum {
	FLASH_CONC_CORE_ANY = 0xFF,
	FLASH_CONC_CORE_0   = 0,
	FLASH_CONC_CORE_1   = 1,
} flash_conc_core_t;

typedef struct {
	const char *name;
	uint8_t tag;
	flash_conc_path_t path;
	flash_conc_core_t core;
	uint32_t addr;
	uint32_t size;
	uint32_t interval_ms;
	volatile bool running;
	beken_thread_t handle;
	uint32_t ok_cnt;
	uint32_t fail_cnt;
	uint32_t iter;
} flash_conc_worker_t;

enum {
	FLASH_CONC_IDX_NS = 0,
	FLASH_CONC_IDX_S,
	FLASH_CONC_IDX_SMP0,
	FLASH_CONC_IDX_SMP1,
	FLASH_CONC_IDX_PEER,
	FLASH_CONC_WORKER_MAX,
};

static flash_conc_worker_t s_workers[FLASH_CONC_WORKER_MAX] = {
	[FLASH_CONC_IDX_NS] = {
		.name = "fc_ns", .tag = 0x10, .path = FLASH_CONC_PATH_NS,
		.core = FLASH_CONC_CORE_ANY, .addr = CONFIG_FLASH_CONC_NS_ADDR,
		.size = CONFIG_FLASH_CONC_SIZE, .interval_ms = CONFIG_FLASH_CONC_INTERVAL_MS,
	},
	[FLASH_CONC_IDX_S] = {
		.name = "fc_s", .tag = 0x20, .path = FLASH_CONC_PATH_S,
		.core = FLASH_CONC_CORE_ANY, .addr = CONFIG_FLASH_CONC_S_ADDR,
		.size = CONFIG_FLASH_CONC_SIZE, .interval_ms = CONFIG_FLASH_CONC_INTERVAL_MS,
	},
	[FLASH_CONC_IDX_SMP0] = {
		.name = "fc_smp0", .tag = 0x30, .path = FLASH_CONC_PATH_NS,
		.core = FLASH_CONC_CORE_0, .addr = CONFIG_FLASH_CONC_SMP0_ADDR,
		.size = CONFIG_FLASH_CONC_SIZE, .interval_ms = CONFIG_FLASH_CONC_INTERVAL_MS,
	},
	[FLASH_CONC_IDX_SMP1] = {
		.name = "fc_smp1", .tag = 0x40, .path = FLASH_CONC_PATH_NS,
		.core = FLASH_CONC_CORE_1, .addr = CONFIG_FLASH_CONC_SMP1_ADDR,
		.size = CONFIG_FLASH_CONC_SIZE, .interval_ms = CONFIG_FLASH_CONC_INTERVAL_MS,
	},
	[FLASH_CONC_IDX_PEER] = {
		.name = "fc_peer", .tag = 0x50, .path = FLASH_CONC_PATH_NS,
		.core = FLASH_CONC_CORE_ANY, .addr = CONFIG_FLASH_CONC_PEER_ADDR,
		.size = CONFIG_FLASH_CONC_SIZE, .interval_ms = CONFIG_FLASH_CONC_INTERVAL_MS,
	},
};

static inline void flash_conc_wdt_feed(void)
{
#if CONFIG_TASK_WDT
	extern void bk_task_wdt_feed(void);
	bk_task_wdt_feed();
#endif
}

static bk_err_t flash_conc_erase(flash_conc_worker_t *w, uint32_t sector_addr)
{
#if CONFIG_TFM_FLASH_NSC
	if (w->path == FLASH_CONC_PATH_S) {
		psa_flash_erase_sector(sector_addr);
		return BK_OK;
	}
#else
	(void)w;
#endif
	return bk_flash_erase_sector(sector_addr);
}

static bk_err_t flash_conc_write(flash_conc_worker_t *w, uint32_t addr,
				 const uint8_t *buf, uint32_t len)
{
#if CONFIG_TFM_FLASH_NSC
	if (w->path == FLASH_CONC_PATH_S) {
		psa_flash_write_bytes(addr, (uint8_t *)buf, len);
		return BK_OK;
	}
#else
	(void)w;
#endif
	return bk_flash_write_bytes(addr, buf, len);
}

static bk_err_t flash_conc_read(flash_conc_worker_t *w, uint32_t addr,
				uint8_t *buf, uint32_t len)
{
#if CONFIG_TFM_FLASH_NSC
	if (w->path == FLASH_CONC_PATH_S) {
		psa_flash_read_bytes(addr, buf, len);
		return BK_OK;
	}
#else
	(void)w;
#endif
	return bk_flash_read_bytes(addr, buf, len);
}

static void flash_conc_unprotect(flash_conc_worker_t *w)
{
#if CONFIG_TFM_FLASH_NSC
	if (w->path == FLASH_CONC_PATH_S) {
		psa_flash_set_protect_type(FLASH_PROTECT_NONE);
		return;
	}
#endif
	(void)w;
	test_flash_set_protect_type_none();
}

static bk_err_t flash_conc_create_thread(flash_conc_worker_t *w,
					 beken_thread_function_t fn)
{
#if CONFIG_SOC_SMP
	if (w->core == FLASH_CONC_CORE_0) {
		return rtos_core0_create_thread(&w->handle, FLASH_CONC_TASK_PRIO,
			w->name, fn, FLASH_CONC_TASK_STACK, (beken_thread_arg_t)w);
	}
	if (w->core == FLASH_CONC_CORE_1) {
		return rtos_core1_create_thread(&w->handle, FLASH_CONC_TASK_PRIO,
			w->name, fn, FLASH_CONC_TASK_STACK, (beken_thread_arg_t)w);
	}
#endif
	return rtos_create_thread(&w->handle, FLASH_CONC_TASK_PRIO,
		w->name, fn, FLASH_CONC_TASK_STACK, (beken_thread_arg_t)w);
}

static void flash_conc_worker(beken_thread_arg_t arg)
{
	flash_conc_worker_t *w = (flash_conc_worker_t *)arg;
	uint8_t *write_buf = NULL;
	uint8_t *read_buf = NULL;

	write_buf = (uint8_t *)os_malloc(w->size);
	read_buf = (uint8_t *)os_malloc(w->size);
	if (!write_buf || !read_buf) {
		CLI_LOGE("%s: oom size=0x%x\r\n", w->name, w->size);
		if (write_buf)
			os_free(write_buf);
		if (read_buf)
			os_free(read_buf);
		w->running = false;
		w->handle = NULL;
		rtos_delete_thread(NULL);
		return;
	}

	flash_conc_unprotect(w);
	CLI_LOGD("%s: start path=%s core=%u addr=0x%08x size=0x%x interval=%ums on_core=%u\r\n",
		 w->name,
		 (w->path == FLASH_CONC_PATH_S) ? "S" : "NS",
		 (unsigned)w->core, w->addr, w->size, w->interval_ms,
		 (unsigned)rtos_get_core_id());

	while (w->running) {
		bool ok = true;
		bk_err_t ret;
		uint32_t sector_addr = w->addr & ~(FLASH_SECTOR_SIZE - 1);

		w->iter++;
		flash_conc_wdt_feed();

		ret = flash_conc_erase(w, sector_addr);
		if (ret != BK_OK) {
			CLI_LOGE("%s: erase fail addr=0x%08x ret=%d\r\n", w->name, sector_addr, ret);
			ok = false;
		}

		for (uint32_t i = 0; i < w->size; i++)
			write_buf[i] = (uint8_t)((w->tag + w->iter + i) & 0xFF);

		flash_conc_wdt_feed();
		ret = flash_conc_write(w, w->addr, write_buf, w->size);
		if (ret != BK_OK) {
			CLI_LOGE("%s: write fail addr=0x%08x ret=%d\r\n", w->name, w->addr, ret);
			ok = false;
		}

		os_memset(read_buf, 0, w->size);
		flash_conc_wdt_feed();
		ret = flash_conc_read(w, w->addr, read_buf, w->size);
		if (ret != BK_OK) {
			CLI_LOGE("%s: read fail addr=0x%08x ret=%d\r\n", w->name, w->addr, ret);
			ok = false;
		} else {
			for (uint32_t i = 0; i < w->size; i++) {
				if (read_buf[i] != write_buf[i]) {
					CLI_LOGE("%s: verify fail off=%u exp=0x%02x got=0x%02x iter=%u\r\n",
						 w->name, i, write_buf[i], read_buf[i], w->iter);
					ok = false;
					break;
				}
			}
		}

		if (ok)
			w->ok_cnt++;
		else
			w->fail_cnt++;

		if (!ok || ((w->iter % FLASH_CONC_LOG_EVERY) == 0)) {
			CLI_LOGD("%s: iter=%u ok=%u fail=%u core=%u\r\n",
				 w->name, w->iter, w->ok_cnt, w->fail_cnt,
				 (unsigned)rtos_get_core_id());
		}

		rtos_delay_milliseconds(w->interval_ms);
	}

	os_free(write_buf);
	os_free(read_buf);
	CLI_LOGD("%s: stopped ok=%u fail=%u\r\n", w->name, w->ok_cnt, w->fail_cnt);
	w->handle = NULL;
	rtos_delete_thread(NULL);
}

static bool flash_conc_worker_busy(const flash_conc_worker_t *w)
{
	return w->running || (w->handle != NULL);
}

static bk_err_t flash_conc_start_one(flash_conc_worker_t *w, uint32_t addr,
				     uint32_t size, uint32_t interval_ms)
{
	bk_err_t ret;

	if (flash_conc_worker_busy(w)) {
		CLI_LOGE("%s: already running\r\n", w->name);
		return BK_FAIL;
	}

#if !CONFIG_TFM_FLASH_NSC
	if (w->path == FLASH_CONC_PATH_S) {
		CLI_LOGE("%s: CONFIG_TFM_FLASH_NSC disabled, Secure path unavailable\r\n", w->name);
		return BK_FAIL;
	}
#endif

	if (addr)
		w->addr = addr;
	if (size)
		w->size = size;
	if (interval_ms)
		w->interval_ms = interval_ms;

	w->ok_cnt = 0;
	w->fail_cnt = 0;
	w->iter = 0;
	w->running = true;

	ret = flash_conc_create_thread(w, (beken_thread_function_t)flash_conc_worker);
	if (ret != BK_OK) {
		w->running = false;
		w->handle = NULL;
		CLI_LOGE("%s: create thread fail ret=%d\r\n", w->name, ret);
		return ret;
	}
	return BK_OK;
}

static void flash_conc_stop_one(flash_conc_worker_t *w)
{
	if (!w->running && w->handle == NULL) {
		CLI_LOGD("%s: not running\r\n", w->name);
		return;
	}
	w->running = false;
	CLI_LOGD("%s: stop requested\r\n", w->name);
}

static void flash_conc_stop_all(void)
{
	for (int i = 0; i < FLASH_CONC_WORKER_MAX; i++)
		flash_conc_stop_one(&s_workers[i]);
}

static void flash_conc_print_stat(void)
{
	CLI_LOGD("flash_conc stat (AP):\r\n");
	for (int i = 0; i < FLASH_CONC_WORKER_MAX; i++) {
		flash_conc_worker_t *w = &s_workers[i];
		CLI_LOGD("  %-8s path=%-2s core=%u addr=0x%08x run=%u iter=%u ok=%u fail=%u\r\n",
			 w->name,
			 (w->path == FLASH_CONC_PATH_S) ? "S" : "NS",
			 (unsigned)w->core, w->addr,
			 w->running ? 1U : 0U, w->iter, w->ok_cnt, w->fail_cnt);
	}
}

static void cli_flash_conc_help(void)
{
	CLI_LOGD("flash_conc sns {start|stop} [ns_addr] [s_addr] [size] [interval_ms]\r\n");
	CLI_LOGD("  Secure(NSC) + Non-Secure concurrent erase/write/read/verify\r\n");
	CLI_LOGD("flash_conc smp {start|stop} [addr0] [addr1] [size] [interval_ms]\r\n");
	CLI_LOGD("  SMP core0 + core1 concurrent erase/write/read/verify\r\n");
	CLI_LOGD("flash_conc peer {start|stop} [addr] [size] [interval_ms]\r\n");
	CLI_LOGD("  This-side worker for AP<->CP; start on both CLIs with different addrs\r\n");
	CLI_LOGD("flash_conc stop | stat\r\n");
	CLI_LOGD("defaults: ns=0x%08x s=0x%08x smp0=0x%08x smp1=0x%08x peer=0x%08x size=0x%x iv=%ums\r\n",
		 CONFIG_FLASH_CONC_NS_ADDR, CONFIG_FLASH_CONC_S_ADDR,
		 CONFIG_FLASH_CONC_SMP0_ADDR, CONFIG_FLASH_CONC_SMP1_ADDR,
		 CONFIG_FLASH_CONC_PEER_ADDR, CONFIG_FLASH_CONC_SIZE,
		 CONFIG_FLASH_CONC_INTERVAL_MS);
}

static void cli_flash_conc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	char *msg = CLI_CMD_RSP_SUCCEED;
	bk_err_t ret = BK_OK;

	(void)xWriteBufferLen;

	if (argc < 2) {
		cli_flash_conc_help();
		msg = CLI_CMD_RSP_ERROR;
		goto out;
	}

	if (os_strcmp(argv[1], "help") == 0) {
		cli_flash_conc_help();
		goto out;
	}

	if (os_strcmp(argv[1], "stat") == 0) {
		flash_conc_print_stat();
		goto out;
	}

	if (os_strcmp(argv[1], "stop") == 0) {
		flash_conc_stop_all();
		goto out;
	}

	if (os_strcmp(argv[1], "sns") == 0) {
		if (argc < 3) {
			cli_flash_conc_help();
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		if (os_strcmp(argv[2], "start") == 0) {
			uint32_t ns_addr = (argc >= 4) ? os_strtoul(argv[3], NULL, 16) : 0;
			uint32_t s_addr = (argc >= 5) ? os_strtoul(argv[4], NULL, 16) : 0;
			uint32_t size = (argc >= 6) ? os_strtoul(argv[5], NULL, 16) : 0;
			uint32_t iv = (argc >= 7) ? os_strtoul(argv[6], NULL, 10) : 0;

			ret = flash_conc_start_one(&s_workers[FLASH_CONC_IDX_NS], ns_addr, size, iv);
			if (ret == BK_OK)
				ret = flash_conc_start_one(&s_workers[FLASH_CONC_IDX_S], s_addr, size, iv);
			if (ret != BK_OK) {
				flash_conc_stop_one(&s_workers[FLASH_CONC_IDX_NS]);
				flash_conc_stop_one(&s_workers[FLASH_CONC_IDX_S]);
				msg = CLI_CMD_RSP_ERROR;
			}
		} else if (os_strcmp(argv[2], "stop") == 0) {
			flash_conc_stop_one(&s_workers[FLASH_CONC_IDX_NS]);
			flash_conc_stop_one(&s_workers[FLASH_CONC_IDX_S]);
		} else {
			cli_flash_conc_help();
			msg = CLI_CMD_RSP_ERROR;
		}
		goto out;
	}

	if (os_strcmp(argv[1], "smp") == 0) {
		if (argc < 3) {
			cli_flash_conc_help();
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		if (os_strcmp(argv[2], "start") == 0) {
			uint32_t a0 = (argc >= 4) ? os_strtoul(argv[3], NULL, 16) : 0;
			uint32_t a1 = (argc >= 5) ? os_strtoul(argv[4], NULL, 16) : 0;
			uint32_t size = (argc >= 6) ? os_strtoul(argv[5], NULL, 16) : 0;
			uint32_t iv = (argc >= 7) ? os_strtoul(argv[6], NULL, 10) : 0;

			ret = flash_conc_start_one(&s_workers[FLASH_CONC_IDX_SMP0], a0, size, iv);
			if (ret == BK_OK)
				ret = flash_conc_start_one(&s_workers[FLASH_CONC_IDX_SMP1], a1, size, iv);
			if (ret != BK_OK) {
				flash_conc_stop_one(&s_workers[FLASH_CONC_IDX_SMP0]);
				flash_conc_stop_one(&s_workers[FLASH_CONC_IDX_SMP1]);
				msg = CLI_CMD_RSP_ERROR;
			}
		} else if (os_strcmp(argv[2], "stop") == 0) {
			flash_conc_stop_one(&s_workers[FLASH_CONC_IDX_SMP0]);
			flash_conc_stop_one(&s_workers[FLASH_CONC_IDX_SMP1]);
		} else {
			cli_flash_conc_help();
			msg = CLI_CMD_RSP_ERROR;
		}
		goto out;
	}

	if (os_strcmp(argv[1], "peer") == 0) {
		if (argc < 3) {
			cli_flash_conc_help();
			msg = CLI_CMD_RSP_ERROR;
			goto out;
		}
		if (os_strcmp(argv[2], "start") == 0) {
			uint32_t addr = (argc >= 4) ? os_strtoul(argv[3], NULL, 16) : 0;
			uint32_t size = (argc >= 5) ? os_strtoul(argv[4], NULL, 16) : 0;
			uint32_t iv = (argc >= 6) ? os_strtoul(argv[5], NULL, 10) : 0;

			ret = flash_conc_start_one(&s_workers[FLASH_CONC_IDX_PEER], addr, size, iv);
			if (ret != BK_OK)
				msg = CLI_CMD_RSP_ERROR;
			else
				CLI_LOGD("AP peer started; also run on CP: flash_conc peer start\r\n");
		} else if (os_strcmp(argv[2], "stop") == 0) {
			flash_conc_stop_one(&s_workers[FLASH_CONC_IDX_PEER]);
		} else {
			cli_flash_conc_help();
			msg = CLI_CMD_RSP_ERROR;
		}
		goto out;
	}

	cli_flash_conc_help();
	msg = CLI_CMD_RSP_ERROR;

out:
	os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}

#define FLASH_CMD_CNT (sizeof(s_flash_commands) / sizeof(struct cli_command))
DRV_CLI_CMD_EXPORT static const struct cli_command s_flash_commands[] = {
	{"flash", "flash {erase|read|write} [start_addr] [len]", cli_flash_cmd},
#if CONFIG_TFM_FLASH_NSC
	{"flash_s", "flash {erase|read|write} [start_addr] [len]", cli_flash_cmd_s},
#endif
	{"flash_erase_test", "cli_flash_erase_test with ble connecting", cli_flash_erase_test_with_ble},
	{"flash_conc", "flash_conc {sns|smp|peer|stop|stat|help}", cli_flash_conc_cmd},
	{"flash_test_task", "flash_test_task {start|stop} [1|2] - periodic flash test", cli_flash_test_task_cmd},
};

int bk_flash_register_cli_test_feature(void)
{
	BK_LOG_ON_ERR(bk_flash_driver_init());
	return cli_register_module_test_feature(s_flash_commands, FLASH_CMD_CNT);
}

