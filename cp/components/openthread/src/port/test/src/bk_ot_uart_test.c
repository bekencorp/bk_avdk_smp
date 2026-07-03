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
#include <os/os.h>
#include <os/mem.h>
#include <openthread/error.h>

#include "bk_cli.h"

#include <utils/uart.h>


#define BK_OT_UART_TST_LOG_EN 1
#if BK_OT_UART_TST_LOG_EN
#define bk_ot_uart_test_log os_printf
#else
#define bk_ot_uart_test_log
#endif

static void cli_ot_uart_commands(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if(argc < 2)
    {
        bk_ot_uart_test_log("[Error] need more than 2 parameters\r\n");
        return;
    }
    for(uint8_t i=0;i<argc;i++)
    {
        bk_printf("arg[%d]:%s\r\n",i,argv[i]);
    }
    if(strncmp(argv[1],"enable",sizeof("enable"))==0)
    {
        otPlatUartEnable();
    }
    else if(strncmp(argv[1],"disable",sizeof("disable"))==0)
    {
        otPlatUartDisable();
    }
    else if(strncmp(argv[1],"send",sizeof("send"))==0)
    {
        char sSend[]="hello";
        otPlatUartSend((uint8_t*)sSend,sizeof(sSend));
    }
    else if(strncmp(argv[1],"flush",sizeof("flush"))==0)
    {
        otPlatUartFlush();
    }
}


#define OT_UART_CMD_CNT (sizeof(s_ot_uart_commands)/sizeof(struct cli_command))
static const struct cli_command s_ot_uart_commands[] = {
    {"otUart", "otUart {enable| disable| send |flush}", cli_ot_uart_commands},
};
int cli_ot_uart_test_init(void)
{
    return cli_register_commands(s_ot_uart_commands, OT_UART_CMD_CNT);
}
