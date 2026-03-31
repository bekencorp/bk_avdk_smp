// Copyright 2025-2026 Beken
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

/**
 * @file sspl_lock_test.c
 * @brief SSPL (Software Spinlock) test cases
 */

#include <common/bk_include.h>
#include <os/os.h>
#include "cli.h"
#include <components/log.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sspl_lock.h"
#include "dwt.h"

#define TAG "sspl_test"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static beken_thread_t s_sspl_test_thread = NULL;
static volatile bool s_sspl_test_running = false;

/**
 * @brief SSPL lock test task.
 *
 * Core 0/1: acquire lock, print log, release lock immediately.
 * Core 2/3: acquire lock, print log, hold for 200ms, release lock.
 * Loops until s_sspl_test_running is cleared.
 */
static void sspl_test_task(void *param)
{
    LOGI("sspl test task started\r\n");

    uint32_t count = 0;

    while (s_sspl_test_running) {
        uint32_t core_id = rtos_get_core_id();
        bk_sspl_res_lock(0);
        if (core_id <= 1) {
            LOGI("core %u: acquired lock, count: %u\r\n", core_id, count++);
        } else {
            LOGI("core %u: acquired lock, count: %u\r\n", core_id, count++);
            rtos_delay_milliseconds(1000);
        }
        LOGI("core %u: released lock\r\n", core_id);
        bk_sspl_res_unlock(0);
    }

    LOGI("sspl test task stopped\r\n");
    s_sspl_test_thread = NULL;
    rtos_delete_thread(NULL);
}

static void cli_sspl_cmd(char *pcWriteBuffer, int xWriteBufferLen,
                         int argc, char **argv)
{
    if (argc < 2) {
        LOGI("Usage: sspl start|stop\r\n");
        return;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        if (s_sspl_test_thread != NULL) {
            LOGI("sspl test already running\r\n");
            return;
        }
        s_sspl_test_running = true;
#if CONFIG_SOC_SMP
        bk_err_t ret = rtos_core0_create_thread(&s_sspl_test_thread, 5,
                                                "sspl_test", sspl_test_task,
                                                2048, NULL);
#else
        bk_err_t ret = rtos_create_thread(&s_sspl_test_thread, 5,
                                              "sspl_test", sspl_test_task,
                                              2048, NULL);
#endif
        if (ret != BK_OK) {
            LOGE("failed to create sspl test task: %d\r\n", ret);
            s_sspl_test_running = false;
        } else {
            LOGI("sspl test task created\r\n");
        }
    } else if (os_strcmp(argv[1], "stop") == 0) {
        if (!s_sspl_test_running) {
            LOGI("sspl test not running\r\n");
            return;
        }
        s_sspl_test_running = false;
        LOGI("sspl test stop requested\r\n");
    } else {
        LOGI("Usage: sspl start|stop\r\n");
    }
}

#define SSPL_TIME_TEST_COUNT 1000
static void cli_sspl_time_cmd(char *pcWriteBuffer, int xWriteBufferLen,
    int argc, char **argv)
{
    dwt_init_cycle_counter();
    dwt_enable_cycle_counter();

    uint32_t flags = rtos_disable_int();
    uint32_t t_start = dwt_get_cycle_counter_val();
    for (int32_t i = 0; i < SSPL_TIME_TEST_COUNT; i++) {
        bk_sspl_res_lock(0);
        bk_sspl_res_unlock(0);
    }
    uint32_t t_end = dwt_get_cycle_counter_val();
    rtos_enable_int(flags);

    LOGI("sspl lock+unlock x%d total cycles: %u\r\n",
         SSPL_TIME_TEST_COUNT, t_end - t_start);
}

#if CONFIG_SSPL_TEST
DRV_CLI_CMD_EXPORT static const struct cli_command s_sspl_commands[] = {
    {"sspl", "sspl start|stop", cli_sspl_cmd},
    {"sspl_time", "sspl_time", cli_sspl_time_cmd},
};
#endif
