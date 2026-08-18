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
#include "argtable3.h"
#include "cli_common.h"
#include <driver/flash_partition.h>
#include <driver/flash.h>
#include "flash_driver.h"

#define FLASH_PAGE_SIZE 256 /* each page has 256 bytes */
extern void print_help(const char *progname, void **argtable);
extern void common_cmd_handler(int argc, char **argv, void **argtable, int argtable_size, void (*handler)(void **argtable));

static void cli_flash_api_cmd_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_lit *erase = (struct arg_lit *)argtable[1];
    struct arg_lit *read = (struct arg_lit *)argtable[2];
    struct arg_lit *write = (struct arg_lit *)argtable[3];
    struct arg_str *lm = (struct arg_str *)argtable[4];
    struct arg_str *arg_init = (struct arg_str *)argtable[5];
    struct arg_str *arg_address = (struct arg_str *)argtable[6];
    struct arg_int *arg_size = (struct arg_int *)argtable[7];
    struct arg_str *arg_get = (struct arg_str *)argtable[8];
    struct arg_str *arg_clock = (struct arg_str *)argtable[9];
    struct arg_str *arg_write_able = (struct arg_str *)argtable[10];
    struct arg_str *arg_protect_type = (struct arg_str *)argtable[11];
    struct arg_str *arg_protect_type_mode = (struct arg_str *)argtable[12];
    struct arg_str *arg_deep_sleep_block = (struct arg_str *)argtable[13];
    struct arg_lit *arg_read_words = (struct arg_lit *)argtable[14];
    struct arg_str *arg_register_callback = (struct arg_str *)argtable[15];
    bk_err_t err = BK_OK;

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    if (arg_init->count > 0)
    {
        if (strcmp(arg_init->sval[0], "init") == 0) {
            BK_LOG_ON_ERR(bk_flash_driver_init());
            CLI_LOGD("Flash driver initialized successfully\r\n");
        } else if (strcmp(arg_init->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_flash_driver_deinit());
            CLI_LOGD("Flash driver deinitialized successfully\r\n");
        } else {
            CLI_LOGE("Invalid parameter for init: %s\r\n", arg_init->sval[0]);
        }
        return; // Exit after processing the init/deinit command
    }

    if (arg_get->count > 0)
    {
        if (strcmp(arg_get->sval[0], "id") == 0) {
            uint32_t flash_id = bk_flash_get_id();
		    CLI_LOGD("flash_id:%x\r\n", flash_id);
        } else if (strcmp(arg_get->sval[0], "total_size") == 0) {
            uint32_t total_flash_size = bk_flash_get_current_total_size();
		    CLI_LOGD("flash_size:%x\r\n", total_flash_size);
        } else {
            CLI_LOGE("Invalid parameter for get: %s\r\n", arg_get->sval[0]);
        }
        return; // Exit after processing the get id/total_size command
    }

    if (arg_clock->count > 0)
    {
        if (strcmp(arg_clock->sval[0], "dpll") == 0) {
            // BK_LOG_ON_ERR(bk_flash_set_clk_dpll());
            CLI_LOGD("Flash clock set up dpll successfully\r\n");
        } else if (strcmp(arg_clock->sval[0], "dco") == 0) {
            // BK_LOG_ON_ERR(bk_flash_set_clk_dco());
            CLI_LOGD("Flash clock set up dco successfully\r\n");
        } else {
            CLI_LOGE("Invalid parameter for clock: %s\r\n", arg_clock->sval[0]);
        }
        return; 
    }

    if (arg_write_able->count > 0)
    {
        if (strcmp(arg_write_able->sval[0], "enable") == 0) {
            BK_LOG_ON_ERR(bk_flash_write_enable());
            CLI_LOGD("Flash write enable successfully\r\n");
        } else if (strcmp(arg_write_able->sval[0], "disable") == 0) {
            BK_LOG_ON_ERR(bk_flash_write_disable());
            CLI_LOGD("Flash write disable successfully\r\n");
        } else {
            CLI_LOGE("Invalid parameter for write: %s\r\n", arg_write_able->sval[0]);
        }
        return; 
    }

    if (arg_protect_type->count > 0)
    {
        flash_protect_type_t protect_mode;

        if (strcmp(arg_protect_type->sval[0], "get") == 0) {
            flash_protect_type_t current_protect_type = bk_flash_get_protect_type();
            CLI_LOGD("Current flash protect type: %d\r\n", current_protect_type);
        } else if (strcmp(arg_protect_type->sval[0], "set") == 0) {
            const char *protect_mode_str = arg_protect_type_mode->sval[0];

            if (strcmp(protect_mode_str, "none") == 0) {
                protect_mode = FLASH_PROTECT_NONE;
            } else if (strcmp(protect_mode_str, "all") == 0) {
                protect_mode = FLASH_PROTECT_ALL;
            } else if (strcmp(protect_mode_str, "half") == 0) {
                protect_mode = FLASH_PROTECT_HALF;
            } else if (strcmp(protect_mode_str, "unprotect") == 0) {
                protect_mode = FLASH_UNPROTECT_LAST_BLOCK;
            } else {
                CLI_LOGE("Invalid protect mode: %s\r\n", protect_mode_str);
                return;
            }

            err = bk_flash_set_protect_type(protect_mode);
            if (err == BK_OK) {
                CLI_LOGD("Flash protect type mode is %s succeeded\r\n", protect_mode_str);
            } else {
                CLI_LOGE("Flash protect type mode is %s failed, err = %x\r\n", protect_mode_str, err);
            }
        } else {
            CLI_LOGE("Invalid parameter for protect type: %s\r\n", arg_protect_type->sval[0]);
        }
        return;
    }

    uint32_t block_addr = (uint32_t)strtol(arg_address->sval[0], NULL, 0);  // Convert address from string to integer
    if (arg_deep_sleep_block->count > 0)
    {
        if (strcmp(arg_deep_sleep_block->sval[0], "erase") == 0) {
            BK_LOG_ON_ERR(bk_flash_erase_block(block_addr));
            CLI_LOGD("Flash erase block successfully\r\n");
        } else if (strcmp(arg_deep_sleep_block->sval[0], "enter") == 0) {
            BK_LOG_ON_ERR(bk_flash_enter_deep_sleep());
            CLI_LOGD("Flash enter deep sleep successfully\r\n");
        } else if (strcmp(arg_deep_sleep_block->sval[0], "exit") == 0) {
            BK_LOG_ON_ERR(bk_flash_exit_deep_sleep());
            CLI_LOGD("Flash exit deep sleep successfully\r\n");
        } else {
            CLI_LOGE("Invalid parameter for write: %s\r\n", arg_deep_sleep_block->sval[0]);
        }
        return; 
    }

    if (lm->count > 0)
    {
        const char *function_str = lm->sval[0];

        if (strcmp(function_str, "get") == 0) {
            flash_line_mode_t current_line_mode = bk_flash_get_line_mode();
            CLI_LOGD("Current flash line mode: %d\r\n", current_line_mode);
            CLI_LOGD("Get flash line mode succeeded\r\n");
        } else {
            CLI_LOGE("Invalid operation: %s\r\n", function_str);
            return;
        }

        return;
    }

    uint32_t start_addr = (uint32_t)strtol(arg_address->sval[0], NULL, 0);  // Convert address from string to integer
    uint32_t len = (uint32_t)arg_size->ival[0];

    if (erase->count > 0)
    {
        // Erase operation
        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
        for (uint32_t addr = start_addr; addr < (start_addr + len); addr += FLASH_SECTOR_SIZE) {
            err = bk_flash_erase_sector(addr);
            if (err != BK_OK) {
                CLI_LOGE("Erase failed at address 0x%x, err = %x\r\n", addr, err);
                break;
            }
        }
        bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
        CLI_LOGD(err ? "Erase flash failed, err = %x\r\n" : "Erase flash succeeded\r\n", err);
    }
    else if (read->count > 0)
    {
        // Read operation
        uint8_t buf[FLASH_PAGE_SIZE] = {0};
        for (uint32_t addr = start_addr; addr < (start_addr + 200); addr += FLASH_PAGE_SIZE) {
            os_memset(buf, 0, FLASH_PAGE_SIZE);
            err = bk_flash_read_bytes(addr, buf, FLASH_PAGE_SIZE);
            if (err != BK_OK) {
                CLI_LOGE("Read failed at address 0x%x, err = %x\r\n", addr, err);
                break;
            }
            CLI_LOGD("flash read addr: 0x%x\r\n", addr);

            CLI_LOGD("dump read flash data:\r\n");
            for (uint32_t i = 0; i < 16; i++) {
                for (uint32_t j = 0; j < 16; j++) {
                    BK_LOG_RAW("%02x ", buf[i * 16 + j]);
                }
                BK_LOG_RAW("\r\n");
            }
        }
        CLI_LOGD(err ? "Read flash failed, err = %x\r\n" : "Read flash succeeded\r\n", err);
    }
    else if (arg_read_words->count > 0)
    {
        uint32_t buf[FLASH_PAGE_SIZE] = {0};
        for (uint32_t addr = start_addr; addr < (start_addr + 200); addr += FLASH_PAGE_SIZE) {
            os_memset(buf, 0, FLASH_PAGE_SIZE);
            err = bk_flash_read_word(addr, buf, FLASH_PAGE_SIZE);
            if (err != BK_OK) {
                CLI_LOGE("Read word failed at address 0x%x, err = %x\r\n", addr, err);
                break;
            }
            CLI_LOGD("flash read word addr: 0x%x\r\n", addr);

            CLI_LOGD("dump read word flash data:\r\n");
            for (uint32_t i = 0; i < 16; i++) {
                for (uint32_t j = 0; j < 16; j++) {
                    BK_LOG_RAW("%02x ", buf[i * 16 + j]);
                }
                BK_LOG_RAW("\r\n");
            }
        }
        CLI_LOGD(err ? "Read flash word failed, err = %x\r\n" : "Read flash word succeeded\r\n", err);
    }
    else if (write->count > 0)
    {
        // Write operation
        uint8_t buf[FLASH_PAGE_SIZE] = {0};
        for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i++) {
            buf[i] = i;
        }

        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
        for (uint32_t i = 0; i < len; i += FLASH_PAGE_SIZE) {
            err = bk_flash_write_bytes(start_addr, buf, FLASH_PAGE_SIZE);
            if (err != BK_OK) {
                CLI_LOGE("Write failed at address 0x%x, err = %x\r\n", start_addr, err);
                break;
            }
        }
        bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
        CLI_LOGD(err ? "Write flash failed, err = %x\r\n" : "Write flash succeeded\r\n", err);
    }
    else if (arg_register_callback->count > 0)
    {
        flash_ps_callback_t callback = NULL;

        if (strcmp(arg_register_callback->sval[0], "suspend") == 0) {
            err = bk_flash_register_ps_suspend_callback(callback);
        } else if (strcmp(arg_register_callback->sval[0], "resume") == 0) {
            err = bk_flash_register_ps_resume_callback(callback);
        } else {
            CLI_LOGE("Invalid parameter for callback: %s\r\n", arg_register_callback->sval[0]);
        }

        if (err != BK_OK) {
            CLI_LOGE("Callback registration failed, err = %x\r\n", err);
        } else {
            CLI_LOGD("Callback registration succeeded\r\n");
        }
    }

    else
    {
        print_help(argtable[0], argtable);
    }
}

static void cli_argflash_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_lit *erase = arg_lit0("e", "erase", "Erase flash");
    struct arg_lit *read = arg_lit0("r", "read", "Read flash");
    struct arg_lit *write = arg_lit0("w", "write", "Write flash");
    struct arg_str *lm = arg_str0("l", "line mode", "<get>", "Get flash line mode");
    struct arg_str *arg_init = arg_str0("i", "init", "<init/deinit>", "Init or deinit flash");
    struct arg_str *arg_address = arg_str0("a", "address", "<flash_address>", "Flash address");
    struct arg_int *arg_size = arg_int0("s", "size", "<size>", "flash size");
    struct arg_str *arg_get = arg_str0("g", "get", "<id/total_size>", "Get flash status");
    struct arg_str *arg_clock = arg_str0("c", "flash clock", "<dpll/dco>", "Set up flash ckock");
    struct arg_str *arg_write_able = arg_str0("v", "flash write", "<enable/disable>", "Flash enable or disable");
    struct arg_str *arg_protect_type = arg_str0("p", "flash protect type", "<set/get>", "Flash protect type config");
    struct arg_str *arg_protect_type_mode = arg_str0("t", "flash protect type mode", "<none/all/half/unprotect>", "Flash protect type mode config");
    struct arg_str *arg_deep_sleep_block = arg_str0("k", "flash deep sleep and block", "<erase/enter/exit>", "Flash deep sleep config and flash address block");
    struct arg_lit *arg_read_words = arg_lit0("b", "read words", "Read flash words");
    struct arg_str *arg_register_callback = arg_str0("u", "flash register", "<suspend/resume>", "Register flash power save callback");
    struct arg_end *end = arg_end(20);

    void* argtable[] = { help, erase, read, write, lm, arg_init, arg_address, arg_size, arg_get, arg_clock, arg_write_able, arg_protect_type, arg_protect_type_mode, arg_deep_sleep_block, arg_read_words, arg_register_callback, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_flash_api_cmd_handler);
}

#define FLASH_CMD_CNT (sizeof(s_flash_commands) / sizeof(struct cli_command))
static const struct cli_command s_flash_commands[] = {
    {"argflash", "argflash { init | deinit | erase | read | write | line mode <get/set> | set line mode <2/4> | get flash <id/total_size> | set flash <dpll/dco> | flash write <enable/disable> | <set/get> flash protect type | set flash protect type mode <none/all/half/unprotect> | <enter/exit> flash deep sleep or erase flash block | Read flash words | <suspend/resume> flash register }", cli_argflash_cmd},
};

int cli_flash_api_register_cli_test_feature(void)
{
    BK_LOG_ON_ERR(bk_flash_driver_init());
    return cli_register_commands(s_flash_commands, FLASH_CMD_CNT);
}
// eof

