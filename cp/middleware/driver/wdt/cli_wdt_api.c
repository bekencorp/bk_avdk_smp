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
#include <driver/wdt.h>
#include <driver/timer.h>
#include <bk_wdt.h>
#include "wdt_driver.h"


void bk_dump_hex(const char *prefix, const void *ptr, uint32_t buflen);

#define CONFIG_STRUCT_FIELD_CNT   5
#define ARG_ERR_REC_CNT           20

static void cli_wdt_api_driver_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *driver = (struct arg_str *)argtable[1];

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (driver->count > 0)
    {
        if (strcmp(driver->sval[0], "init") == 0) {
            BK_LOG_ON_ERR(bk_wdt_driver_init());
            CLI_LOGD("WDT driver initialized successfully\r\n");
        } else if (strcmp(driver->sval[0], "deinit") == 0) {
            BK_LOG_ON_ERR(bk_wdt_driver_deinit());
            CLI_LOGD("WDT driver deinitialized successfully\r\n");
        } else {
            CLI_LOGE("Invalid parameter for init: %s\r\n", driver->sval[0]);
        }
        return; 
    } 
 
    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argwdt_driver_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *driver = arg_str0("d", "driver", "<init/deinit>", "Init or deinit wdt driver");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, driver, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_wdt_api_driver_handler);
}

static void timer_isr_callback(timer_id_t chan)
{
	BK_LOG_ON_ERR(bk_wdt_feed());
}

static void cli_wdt_api_function_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *function = (struct arg_str *)argtable[1];
    struct arg_str *timeout = (struct arg_str *)argtable[2];

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (function->count > 0)
    {
        if (strcmp(function->sval[0], "start") == 0) {
            uint32_t timeout_ms = (uint32_t)os_strtoul(timeout->sval[0], NULL, 10);
            BK_LOG_ON_ERR(bk_wdt_start(timeout_ms));
            CLI_LOGD("WDT start successfully\r\n");
        } else if (strcmp(function->sval[0], "stop") == 0) {
            BK_LOG_ON_ERR(bk_wdt_stop());
    #if (CONFIG_TASK_WDT)
            bk_task_wdt_stop();
    #endif
            CLI_LOGD("WDT stop successfully\r\n");            
        } else if (strcmp(function->sval[0], "feed") == 0) {
            BK_LOG_ON_ERR(bk_wdt_start(6000));
            BK_LOG_ON_ERR(bk_timer_start(1, 1000, timer_isr_callback));
            CLI_LOGD("WDT feed successfully\r\n");
        }else {
            CLI_LOGE("Invalid parameter for function: %s\r\n", function->sval[0]);
        }
        return; 
    } 
 
    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argwdt_function_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *function = arg_str0("f", "function", "<start/stop/feed>", "Set wdt function");
    struct arg_str *timeout = arg_str0("t", "timeout", "<timeout_ms>", "Set wdt timeout");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, function, timeout, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_wdt_api_function_handler);
}

static void cli_wdt_api_config_handler(void **argtable)
{
    struct arg_lit *help = (struct arg_lit *)argtable[0];
    struct arg_str *config = (struct arg_str *)argtable[1];
    struct arg_str *timeout = (struct arg_str *)argtable[2];

    if (help->count > 0)
    {
        print_help(argtable[0], argtable);
        return;
    }

    else if (config->count > 0)
    {
        if (strcmp(config->sval[0], "get") == 0) {
            BK_LOG_ON_ERR(bk_wdt_get_feed_time());
            CLI_LOGD("WDT get feed time successfully\r\n");
        } else if (strcmp(config->sval[0], "set") == 0) {
            uint32_t timeout_ms = (uint32_t)os_strtoul(timeout->sval[0], NULL, 10);
            bk_wdt_set_feed_time(timeout_ms);
            CLI_LOGD("Set feed watchdog time successfully\r\n");
        } else {
            CLI_LOGE("Invalid parameter for init: %s\r\n", config->sval[0]);
        }
        return; 
    } 
 
    else
    {
        print_help(argtable[0], argtable);
    }

}

static void cli_argwdt_config_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    struct arg_lit *help = arg_lit0("h", "help", "Display this help message");
    struct arg_str *config = arg_str0("c", "config", "<get/set>", "config of feed watchdog time");
    struct arg_str *timeout = arg_str0("t", "timeout", "<timeout_ms>", "timeout of feed watchdog");
    struct arg_end *end = arg_end(ARG_ERR_REC_CNT);

    void* argtable[] = { help, config, timeout, end };
    int argtable_size = sizeof(argtable) / sizeof(argtable[0]);

    common_cmd_handler(argc, argv, argtable, argtable_size, cli_wdt_api_config_handler);
}


#define WDT_CMD_CNT (sizeof(s_wdt_api_commands) / sizeof(struct cli_command))
static const struct cli_command s_wdt_api_commands[] = {
    {"argwdt_driver", "argwdt_driver { help | driver }", cli_argwdt_driver_cmd},
    {"argwdt_function", "argwdt_function { help | function }", cli_argwdt_function_cmd},
    {"argwdt_config", "argwdt_config { help | config }", cli_argwdt_config_cmd},
};

int cli_wdt_api_register_cli_test_feature(void)
{
    BK_LOG_ON_ERR(bk_wdt_driver_init());
    return cli_register_commands(s_wdt_api_commands, WDT_CMD_CNT);
}
// eof

