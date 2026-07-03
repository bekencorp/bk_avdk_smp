/*
 *  Copyright (c) 2023, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * @file
 * @brief # This file provides an example on how to implement a CLI vendor extension.
 */

#include <openthread/cli.h>
#include "common/code_utils.hpp"
#include  "os/os.h"
#include "os/mem.h"
#include "bk_rtos_debug.h"

static otError helloWorldCommand(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otCliOutputFormat("Hello world!\r\n");
    return OT_ERROR_NONE;
}

static otError threadCommand(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otCliOutputFormat("Thread is great!\r\n");
    return OT_ERROR_NONE;
}

static otError memshow(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    uint32_t total_size,free_size,mini_size;
    otCliOutputFormat("================Static memory================\r\n");
    os_show_memory_config_info();

    otCliOutputFormat("================Dynamic memory================\r\n");
    otCliOutputFormat("%-5s   %-5s   %-5s   %-5s   %-5s\r\n",
    "name", "total", "free", "minimum", "peak");

    total_size = rtos_get_total_heap_size();
    free_size  = rtos_get_free_heap_size();
    mini_size  = rtos_get_minimum_free_heap_size();
    otCliOutputFormat("heap\t%d\t%d\t%d\t%d\r\n",  total_size,free_size,mini_size,total_size-mini_size);

    return OT_ERROR_NONE;
}

#if CONFIG_SUPPORT_MATTER
void matter_factory_reset(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv );
void matter_show(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv );
extern otInstance *g_otInst;

static otError matter_factoryreset(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    if(g_otInst != NULL)
    {
        otInstanceFactoryReset(g_otInst);
    }

    matter_factory_reset(0,0,0,0);
    return OT_ERROR_NONE;
}
static otError matter_show_fun(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    matter_show(0,0,0,0);
    return OT_ERROR_NONE;
}
static otError cli_tasklist(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
	rtos_dump_task_list();
    return OT_ERROR_NONE;
}

#endif
static const otCliCommand sExtensionCommands[] = {
    {"extensionhello", helloWorldCommand},
    {"extensionthread", threadCommand},
    {"memshow", memshow},
#if CONFIG_SUPPORT_MATTER
    {"matter_factoryreset", matter_factoryreset},
    {"matter_show", matter_show_fun},
    {"tasklist", cli_tasklist},
#endif
};

void otCliVendorSetUserCommands(void)
{
    IgnoreError(otCliSetUserCommands(sExtensionCommands, OT_ARRAY_LENGTH(sExtensionCommands), NULL));
}

