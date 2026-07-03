// Copyright 2020-2025 Beken
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

#include <stdbool.h>
#include <string.h>
#include <sdkconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include "sys_rtos.h"
#include  "os/os.h"
#include "os/mem.h"
#include "FreeRTOS.h"
#include "bk_private/bk_wdt.h"

#include <assert.h>
#include <openthread-core-bk7239n-config.h>
#include <openthread/config.h>

#include <openthread/cli.h>
#include <openthread/diag.h>
#include <openthread/tasklet.h>
#include <openthread/platform/logging.h>
#include <openthread/platform/misc.h>

#include "openthread-system.h"
#include "cli/cli_config.h"
#include "common/code_utils.hpp"

#include "lib/platform/reset_util.h"

#include "bk_openthread.h"
#include "bk_ot_alarm.h"
#include "bk_ot_uart.h"

#include "modules/pm.h"


#if CONFIG_SUPPORT_MATTER
extern otInstance *g_otInst __attribute__((weak));
#endif
#if CONFIG_CLI
    extern "C" bool isCliInOtProcess(void);
    extern "C" void processOtCliCmd(void);
#endif
otInstance *instance = NULL;

#if CONFIG_HOMEKIT
static bool HomeKitUsesOpenThread(void)
{
#ifdef CONFIG_HOMEKIT_OVER_TRANSPORT
    return !strcmp(CONFIG_HOMEKIT_OVER_TRANSPORT, "THREAD") || !strcmp(CONFIG_HOMEKIT_OVER_TRANSPORT, "MULTI");
#else
    return false;
#endif
}
#endif

extern "C" otInstance* bk_ot_get_single_instance(void)
{
    return instance;
}

/**
 * Initializes the CLI app.
 *
 * @param[in]  aInstance  The OpenThread instance structure.
 */
#if CONFIG_HOMEKIT
extern "C" void* HAPPlatformThreadGetHandle(void);
#endif
#if CONFIG_CLI
extern "C" bool is_shell_task_event_clear(void);
#endif
static void otThreadMain(uint32_t data)
{
    os_printf("Welcome to Openthread!!!\r\n");
    bool createStandaloneInstance = true;

#if CONFIG_SUPPORT_MATTER
    //In Matter project, Matter will initialize the instance 
    //AND start CLI if CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI.
    //**Later i want the Openthread create this instance.
    instance = g_otInst;
    createStandaloneInstance = false;
#elif CONFIG_HOMEKIT
    if (HomeKitUsesOpenThread()) {
        instance = (otInstance *)HAPPlatformThreadGetHandle();
        createStandaloneInstance = false;
    }
#endif

    if (createStandaloneInstance) {
    otSysInit(0,NULL);
    instance = otInstanceInitSingle();
    assert(instance);
    }

#if !CONFIG_SUPPORT_MATTER
    otAppCliInit(instance);
#endif

    while (!otSysPseudoResetWasRequested())
    {
        while(otTaskletsArePending(instance))
        {
            otTaskletsProcess(instance);
        }
        otSysProcessDrivers(instance);
#if CONFIG_CLI
        {
            if(isCliInOtProcess() && is_shell_task_event_clear())
            {
                processOtCliCmd();
            }

        }
#endif
#if (CONFIG_TASK_WDT)
        bk_task_wdt_feed();
#endif

    }
#if CONFIG_SYS_CPU0
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_THREAD, PM_CPU_FRQ_DEFAULT);
#endif

    otInstanceFinalize(instance);

    rtos_delete_thread(NULL);
}
#define BK_OPENTHREAD_PRO       4
beken_thread_t ot_thread;

bk_err_t bk_openthread_init(void)
{
#if CONFIG_SYS_CPU0
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_THREAD, PM_CPU_FRQ_240M);
#endif

#if !CONFIG_OPENTHREAD_AON_US_TICK
    bk_ot_alarm_init();
#endif
    int ret =0;
    ret = rtos_create_thread(&ot_thread,
                            BK_OPENTHREAD_PRO,
                            "OT_thread",
                            (beken_thread_function_t)otThreadMain,
#if CONFIG_HOMEKIT
                            HomeKitUsesOpenThread() ? (8192 * 3) : 8192,
#else
                            8192,
#endif
                            0);
    if(ret !=kNoErr)
    {
        os_printf("[Error]: Failed to create openthread:%d\r\n",ret);
    }
    return BK_OK;
}
