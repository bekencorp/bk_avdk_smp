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

#include <string.h>
#include <stdlib.h>
#include <os/os.h>
#include <os/mem.h>

#include "bk_cli.h"

#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/time.h>
#include <openthread/instance.h>

#include "bk_ot_alarm.h"

#define BK_OT_ALARM_TST_LOG_EN 1
#if BK_OT_ALARM_TST_LOG_EN
#define bk_ot_alarm_log os_printf
#else
#define bk_ot_alarm_log
#endif
static uint8_t uRiskVal=0;
static void alarm_main_thread(uint32_t data)
{
    bk_ot_alarm_log("%s uRiskVal:%d\r\n",__func__,uRiskVal);
    while(uRiskVal)
    {
        bk_ot_alarm_process(NULL);
        rtos_delay_milliseconds(1);
    }
    rtos_delete_thread(NULL);
}

static void cli_ot_alarm_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if(argc < 3)
    {
        bk_ot_alarm_log("[Error] Please enter more than 2 parameters\r\n");
        return;
    }
    if(strncmp(argv[1],"get",sizeof("get"))==0)
    {
        uint32_t uCurrentTime=0;
        if(strncmp(argv[2],"ms",sizeof("ms"))==0)
            uCurrentTime = otPlatAlarmMilliGetNow();
        else if(strncmp(argv[2],"us",sizeof("us"))==0)
            uCurrentTime = otPlatAlarmMicroGetNow();
        bk_ot_alarm_log("%s get current time:%lu(%s)\r\n",__func__,uCurrentTime,argv[2]);
    }
    else if(strncmp(argv[1],"stop",sizeof("stop"))==0)
    {
        if(strncmp(argv[2],"ms",sizeof("ms"))==0)
            otPlatAlarmMilliStop(NULL);
        else if(strncmp(argv[2],"us",sizeof("us"))==0)
            otPlatAlarmMicroStop(NULL);
        uRiskVal=0;
        bk_ot_alarm_log("%s stop (%s)\r\n",__func__,argv[2]);
    }
    else if(strncmp(argv[1],"start",sizeof("start"))==0)
    {
        if(argc<4)
        {
            bk_ot_alarm_log("[Error] Please enter duration\r\n");
            return;
        }
        uint32_t uDuration = strtoul(argv[3],NULL,0);
        if(strncmp(argv[2],"ms",sizeof("ms"))==0)
            otPlatAlarmMilliStartAt(NULL,otPlatAlarmMilliGetNow(),uDuration);
        else if(strncmp(argv[2],"us",sizeof("us"))==0)
            otPlatAlarmMicroStartAt(NULL,otPlatAlarmMicroGetNow(),uDuration);
        if(uRiskVal==0)
        {
            uRiskVal=1;
            bk_ot_alarm_log("create test thread:%d\r\n",rtos_create_thread(NULL,8,"alarm_proc",(beken_thread_function_t)alarm_main_thread,1024,0));
        }

        bk_ot_alarm_log("%s start (%s) duration:%d\r\n",__func__,argv[2],uDuration);
    }
}

#define OT_ALARM_CMD_CNT (sizeof(s_ot_alarm_commands)/sizeof(struct cli_command))
static const struct cli_command s_ot_alarm_commands[] = {
    {"otAlarm", "otAlarm {get | start | stop}", cli_ot_alarm_cmd},
};
int cli_ot_alarm_test_init(void)
{
    return cli_register_commands(s_ot_alarm_commands, OT_ALARM_CMD_CNT);
}
