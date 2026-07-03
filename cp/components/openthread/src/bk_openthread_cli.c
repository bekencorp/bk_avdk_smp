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

#include "openthread-core-config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "sys_rtos.h"
#include  "os/os.h"
#include "os/mem.h"

#include <openthread-system.h>
#include <openthread/cli.h>
#include <openthread/logging.h>

#include "cli/cli_config.h"
#include "common/code_utils.hpp"
#include "common/debug.hpp"
#include "utils/uart.h"

#include "bk_openthread.h"
#if OPENTHREAD_POSIX
#include <signal.h>
#include <sys/types.h>
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
 *
 * The size of CLI UART RX buffer in bytes.
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
#if OPENTHREAD_CONFIG_REFERENCE_DEVICE_ENABLE
#define OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE 640
#else
#define OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE 512
#endif
#endif

/**
 * @def OPENTHREAD_CONFIG_CLI_TX_BUFFER_SIZE
 *
 * The size of CLI message buffer in bytes.
 */
#ifndef OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE
#define OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE 1024
#endif

#if OPENTHREAD_CONFIG_DIAG_ENABLE
#if OPENTHREAD_CONFIG_DIAG_OUTPUT_BUFFER_SIZE > OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE
#error "diag output buffer should be smaller than CLI UART tx buffer"
#endif
#if OPENTHREAD_CONFIG_DIAG_CMD_LINE_BUFFER_SIZE > OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
#error "diag command line should be smaller than CLI UART rx buffer"
#endif
#endif

#if OPENTHREAD_CONFIG_CLI_MAX_LINE_LENGTH > OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE
#error "command line should be should be smaller than CLI rx buffer"
#endif

enum
{
    kRxBufferSize = OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE,
    kTxBufferSize = OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE,
};

char     sRxBuffer[kRxBufferSize];
uint16_t sRxLength;

char     sTxBuffer[kTxBufferSize];
uint16_t sTxHead;
uint16_t sTxLength;

uint16_t sSendLength;

#ifdef OT_CLI_UART_LOCK_HDR_FILE

#include OT_CLI_UART_LOCK_HDR_FILE

#else

/**
 * Macro to acquire an exclusive lock of uart cli output
 * Default implementation does nothing
 */
#ifndef OT_CLI_UART_OUTPUT_LOCK
#define OT_CLI_UART_OUTPUT_LOCK() \
    do                            \
    {                             \
    } while (0)
#endif

/**
 * Macro to release the exclusive lock of uart cli output
 * Default implementation does nothing
 */
#ifndef OT_CLI_UART_OUTPUT_UNLOCK
#define OT_CLI_UART_OUTPUT_UNLOCK() \
    do                              \
    {                               \
    } while (0)
#endif

#endif // OT_CLI_UART_LOCK_HDR_FILE

static int     Output(const char *aBuf, uint16_t aBufLength);
static otError ProcessCommand(void);

static void ReceiveTask(const uint8_t *aBuf, uint16_t aBufLength)
{
    static const char sEraseString[] = {'\b', ' ', '\b'};
    static const char CRNL[]         = {'\r', '\n'};
    static uint8_t    sLastChar      = '\0';
    const uint8_t    *end;

    end = aBuf + aBufLength;

    for (; aBuf < end; aBuf++)
    {
        switch (*aBuf)
        {
        case '\n':
            if (sLastChar == '\r')
            {
                break;
            }

            OT_FALL_THROUGH;

        case '\r':
            Output(CRNL, sizeof(CRNL));
            sRxBuffer[sRxLength] = '\0';
            IgnoreError(ProcessCommand());
            break;

#if OPENTHREAD_POSIX && !defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION)
        case 0x03: // ASCII for Ctrl-C
            kill(0, SIGINT);
            break;

        case 0x04: // ASCII for Ctrl-D
            exit(EXIT_SUCCESS);
            break;
#endif

        case '\b':
        case 127:
            if (sRxLength > 0)
            {
                Output(sEraseString, sizeof(sEraseString));
                sRxBuffer[--sRxLength] = '\0';
            }

            break;

        default:
            if (sRxLength < kRxBufferSize - 1)
            {
                Output((const char *)(aBuf), 1);
                sRxBuffer[sRxLength++] = (char)(*aBuf);
            }

            break;
        }

        sLastChar = *aBuf;
    }
}

static otError ProcessCommand(void)
{
    otError error = OT_ERROR_NONE;

    while (sRxLength > 0 && (sRxBuffer[sRxLength - 1] == '\n' || sRxBuffer[sRxLength - 1] == '\r'))
    {
        sRxBuffer[--sRxLength] = '\0';
    }

    otCliInputLine(sRxBuffer);
    sRxLength = 0;

    return error;
}

static void Send(void)
{
    VerifyOrExit(sSendLength == 0);

    if (sTxLength > kTxBufferSize - sTxHead)
    {
        sSendLength = kTxBufferSize - sTxHead;
    }
    else
    {
        sSendLength = sTxLength;
    }

    if (sSendLength > 0)
    {
#if OPENTHREAD_CONFIG_ENABLE_DEBUG_UART
        /* duplicate the output to the debug uart */
        otSysDebugUart_write_bytes((uint8_t *)(sTxBuffer + sTxHead), sSendLength);
#endif
        IgnoreError(otPlatUartSend((uint8_t *)(sTxBuffer + sTxHead), sSendLength));
    }

exit:
    return;
}

static void SendDoneTask(void)
{
    sTxHead = (sTxHead + sSendLength) % kTxBufferSize;
    sTxLength -= sSendLength;
    sSendLength = 0;

    Send();
}

static int Output(const char *aBuf, uint16_t aBufLength)
{
    OT_CLI_UART_OUTPUT_LOCK();
    uint16_t sent = 0;

    while (aBufLength > 0)
    {
        uint16_t remaining = kTxBufferSize - sTxLength;
        uint16_t tail;
        uint16_t sendLength = aBufLength;

        if (sendLength > remaining)
        {
            sendLength = remaining;
        }

        for (uint16_t i = 0; i < sendLength; i++)
        {
            tail            = (sTxHead + sTxLength) % kTxBufferSize;
            sTxBuffer[tail] = *aBuf++;
            aBufLength--;
            sTxLength++;
        }

        Send();

        sent += sendLength;

        if (aBufLength > 0)
        {
            // More to send, so flush what's waiting now
            otError err = otPlatUartFlush();

            if (err == OT_ERROR_NONE)
            {
                // Flush successful, reset the pointers
                SendDoneTask();
            }
            else
            {
                // Flush did not succeed, so abort here.
                break;
            }
        }
    }

    OT_CLI_UART_OUTPUT_UNLOCK();

    return sent;
}

static int CliUartOutput(void *aContext, const char *aFormat, va_list aArguments)
{
    OT_UNUSED_VARIABLE(aContext);

    int rval;

    if (sTxLength == 0)
    {
        rval = vsnprintf(sTxBuffer, kTxBufferSize, aFormat, aArguments);
        VerifyOrExit(rval >= 0 && rval < kTxBufferSize, otLogWarnPlat("Failed to format CLI output `%s`", aFormat));
        sTxHead     = 0;
        sTxLength   = (uint16_t)(rval);
        sSendLength = 0;
    }
    else
    {
        va_list  retryArguments;
        uint16_t tail      = (sTxHead + sTxLength) % kTxBufferSize;
        uint16_t remaining = (sTxHead > tail ? (sTxHead - tail) : (kTxBufferSize - tail));

        va_copy(retryArguments, aArguments);

        rval = vsnprintf(&sTxBuffer[tail], remaining, aFormat, aArguments);

        if (rval < 0)
        {
            otLogWarnPlat("Failed to format CLI output `%s`", aFormat);
        }
        else if (rval < remaining)
        {
            sTxLength += rval;
        }
        else if (rval < kTxBufferSize)
        {
            while (sTxLength != 0)
            {
                otError error;

                Send();

                error = otPlatUartFlush();

                if (error == OT_ERROR_NONE)
                {
                    // Flush successful, reset the pointers
                    SendDoneTask();
                }
                else
                {
                    // Flush did not succeed, so abandon buffered output.
                    otLogWarnPlat("Failed to output CLI: %s", otThreadErrorToString(error));
                    break;
                }
            }
            rval = vsnprintf(sTxBuffer, kTxBufferSize, aFormat, retryArguments);
            OT_ASSERT(rval > 0);
            sTxLength   = (uint16_t)(rval);
            sTxHead     = 0;
            sSendLength = 0;
        }
        else
        {
            otLogWarnPlat("CLI output `%s` truncated", aFormat);
        }

        va_end(retryArguments);
    }

    Send();

exit:
    return rval;
}

// void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength) { ReceiveTask(aBuf, aBufLength); }
void otPlatUartReceived(const uint8_t *aBuf, uint16_t aBufLength)
{
    ReceiveTask(aBuf, aBufLength);

}

void otPlatUartSendDone(void) { SendDoneTask(); }

void otAppCliInit(otInstance *aInstance)
{
    sRxLength   = 0;
    sTxHead     = 0;
    sTxLength   = 0;
    sSendLength = 0;

#if !CONFIG_CLI
    IgnoreError(otPlatUartEnable());
#endif

    otCliInit(aInstance, CliUartOutput, aInstance);
}

#if CONFIG_CLI
#include "os/str.h"
#include "bk_cli.h"
#include "cmsis_gcc.h"
// #include "bk_cli/cli.h"
typedef struct {
    const uint8_t* ptrCmd;
    uint16_t uCmdLen;
}sOtCmd;
bool bCliIsBusy = 0;
sOtCmd ptrOtCmd;
bool isCliInOtProcess(void)
{
    return bCliIsBusy;
}
void setCliInOtProcessFlag(bool bFlag)
{
    bCliIsBusy = bFlag;

}
void postCliCmdToOtProcess(uint8_t* pCmd, uint16_t cmdLen)
{
    ptrOtCmd.ptrCmd = pCmd;
    ptrOtCmd.uCmdLen = cmdLen;
    setCliInOtProcessFlag(1);

}
void processOtCliCmd(void)
{
    otPlatUartReceived((uint8_t*)ptrOtCmd.ptrCmd, ptrOtCmd.uCmdLen);
    if(ptrOtCmd.ptrCmd != NULL)
        psram_free((char*)ptrOtCmd.ptrCmd);
    ptrOtCmd.ptrCmd = NULL;
    ptrOtCmd.uCmdLen = 0;
    setCliInOtProcessFlag(0);
}

void cli_ot_command(char* pcWriteBuffer, int xWriteBufferLen, int argc, char** argv)
{
    char* ptrCmd = NULL;
    char *msg = NULL;
    if(argc < 2)
    {
        goto error;
    }
    if(isCliInOtProcess())
    {
        os_printf("[Error]some cmd is running\n");
        goto error;
    }
    if(bk_ot_get_single_instance() == NULL)
        goto error;
    if(os_strncmp(argv[0], "ot", os_strlen("ot")) != 0)
        goto error;

    ptrCmd = (char*)psram_zalloc(1024);
    if(ptrCmd == NULL)
    {
        os_printf("[Error]%s:%d failed to malloc\n", __func__, __LINE__);
        goto error;
    }
    uint16_t ptrIdx = 0;
    for(uint8_t i = 1; i < argc; i++)
    {
        os_strncpy(&ptrCmd[ptrIdx], argv[i], os_strlen(argv[i]));
        ptrIdx += os_strlen(argv[i]);
        ptrCmd[ptrIdx++] = ' ';
    }
    ptrCmd[ptrIdx--] = '\n';
    ptrCmd[ptrIdx] = '\r';
    postCliCmdToOtProcess((uint8_t*)ptrCmd, os_strlen(ptrCmd));
    goto exit;
error:
    msg = "CMDRSP:ERROR\r\n";
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
    if(ptrCmd != NULL)
        psram_free(ptrCmd);
exit:
    return;
}


void cli_open_thread_command(char* pcWriteBuffer, int xWriteBufferLen, int argc, char** argv)
{
    char *msg = NULL;
    static uint8_t uThreadOpened = 0;
    if(argc < 2)
        goto error;
    if(os_strncmp(argv[0], "openthread", os_strlen("openthread")) != 0)
        goto error;

    if(os_strncmp(argv[1], "open", os_strlen("open")) == 0)
    {
        if(uThreadOpened == 0)
        {
            uThreadOpened = 1;
            bk_openthread_init();
        }
        else
        {
            os_printf("[Warning]openthread already openned\r\n");
        }
    }
    goto exit;
error:
    msg = "CMDRSP:ERROR\r\n";
    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
exit:
    return;

}
#define OT_CMD_CNT (sizeof(s_openthread_commands) / sizeof(struct cli_command))
static const struct cli_command s_openthread_commands[] = {
    {"ot", "ot command", cli_ot_command},
    {"openthread", "start openthread", cli_open_thread_command},
};
int cli_openthread_init(void)
{
    return cli_register_commands(s_openthread_commands, OT_CMD_CNT);
}

#endif