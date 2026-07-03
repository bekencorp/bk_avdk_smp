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

#include "bk_cli.h"
#include "driver/flash_partition.h"
#include "driver/flash.h"
#include "flash/flash_driver.h"

#include "openthread/platform/flash.h"

#define BK_OT_FLASH_TST_LOG_EN 1
#if BK_OT_FLASH_TST_LOG_EN
#define bk_ot_flash_log os_printf
#else
#define bk_ot_flash_log
#endif
static void cli_ot_argflash_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if(argc <2)
    {
        bk_ot_flash_log("[Error] please enter more than 2 parameters\r\n");
    }
    for(uint8_t i=0;i<argc;i++)
    {
        bk_ot_flash_log("argv[%d]= %s\r\n",i,argv[i]);
    }
    if(strncmp(argv[1],"erase",sizeof("erase"))==0)
    {
        uint8_t uSwapIdx=strtoul(argv[2],NULL,0);
        otPlatFlashErase(NULL,uSwapIdx);
    }
    else if(strncmp(argv[1],"read",sizeof("read"))==0)
    {
        uint8_t uSwapIdx=strtoul(argv[2],NULL,0);
        uint32_t uOffset=strtoul(argv[3],NULL,0);
        uint32_t uSize = strtoul(argv[4],NULL,0);
        uint8_t *uBuffer=psram_malloc(uSize);

        otPlatFlashRead(NULL,uSwapIdx,uOffset,uBuffer,uSize);
        for(uint8_t i=0;i<uSize;i++)
        {
            bk_ot_flash_log("[%d]=0x%02x ",i,uBuffer[i]);
        }
        bk_ot_flash_log("\r\n");
        psram_free(uBuffer);

    }
    else if(strncmp(argv[1],"write",sizeof("write"))==0)
    {
        uint8_t uSwapIdx=strtoul(argv[2],NULL,0);
        uint32_t uOffset=strtoul(argv[3],NULL,0);
        uint32_t uSize = strtoul(argv[4],NULL,0);
        uint8_t *uBuffer=psram_malloc(uSize);
        for(uint32_t i=0;i<uSize;i++)
        {
            uBuffer[i]=i;
        }
        otPlatFlashWrite(NULL,uSwapIdx,uOffset,uBuffer,uSize);
        psram_free(uBuffer);
    }
}
void otTestFlash(void)
{
    bk_ot_flash_log("Start Openthread selftest\r\n");

    uint8_t readBuffer[256]={0};
    uint8_t writeBuffer[256]={0};
    uint32_t mSwapSize=0;
    uint32_t mSwapUsed=0;
    uint8_t mSwapIndex=0;
    uint32_t sSwapActive=0;
    uint32_t swapMarker=0;

    for(uint32_t i=0;i<sizeof(writeBuffer);i++)
    {
        writeBuffer[i]=i&0xFF;
        // bk_ot_flash_log("[%d]= 0x%02x\n",i,writeBuffer[i]);
    }
    //init flash
    otPlatFlashInit(NULL);

    mSwapSize = otPlatFlashGetSwapSize(NULL);
    bk_ot_flash_log("mSwapSize:%d\r\n",mSwapSize);
    otPlatFlashErase(NULL,0);

    for(mSwapIndex=0;;mSwapIndex++)
    {
        if(mSwapIndex>=2)
        {
            otPlatFlashWrite(NULL,0,0,&sSwapActive,sizeof(sSwapActive));
            mSwapIndex=0;
            mSwapUsed = sizeof(sSwapActive);
            bk_ot_flash_log("[Warning] No Swap Index used init 0!!\r\n");
        }
        otPlatFlashRead(NULL,mSwapIndex,0,&swapMarker,sizeof(swapMarker));
        if(swapMarker == sSwapActive)
        {
            bk_ot_flash_log("swapMarker:%d sSwapActive:%d\r\n",swapMarker,sSwapActive);
            break;
        }
    }
    memset(readBuffer,0,sizeof(readBuffer));
    mSwapUsed=4;
    for(uint16_t key=0;key<16;key++)
    {
        uint16_t length=key;
        otPlatFlashWrite(NULL,mSwapIndex,mSwapUsed,writeBuffer,length);
        otPlatFlashRead(NULL,mSwapIndex,mSwapUsed,readBuffer,length);
        for(uint16_t kk=0;kk<length;kk++)
        {
            bk_ot_flash_log("[%d]=0x%x 0x%x =%d",kk,writeBuffer[kk],readBuffer[kk],(writeBuffer[kk]==readBuffer[kk]));
        }
        bk_ot_flash_log("\r\n");
        if(memcmp(readBuffer,writeBuffer,length)==0)
            bk_ot_flash_log("[Success] %d ln:%d\r\n",key,__LINE__);
        else
        {
            bk_ot_flash_log("[Error]%d ln:%d\r\n",key,__LINE__);
            return;
        }
        mSwapUsed+=length;
    }
//======================swap 1====================================//
    otPlatFlashErase(NULL,1);
    memset(readBuffer,0,sizeof(readBuffer));
    mSwapUsed=4;
    mSwapIndex =1;
    for(uint16_t key=0;key<16;key++)
    {
        uint16_t length=key;
        otPlatFlashWrite(NULL,mSwapIndex,mSwapUsed,writeBuffer,length);
        otPlatFlashRead(NULL,mSwapIndex,mSwapUsed,readBuffer,length);
        for(uint16_t kk=0;kk<length;kk++)
        {
            bk_ot_flash_log("[%d]=0x%x 0x%x =%d",kk,writeBuffer[kk],readBuffer[kk],(writeBuffer[kk]==readBuffer[kk]));
        }
        bk_ot_flash_log("\r\n");
        if(memcmp(readBuffer,writeBuffer,length)==0)
            bk_ot_flash_log("[Success] %d ln:%d\r\n",key,__LINE__);
        else
        {
            bk_ot_flash_log("[Eror]%d ln:%d\r\n",key,__LINE__);
            return;
        }

        mSwapUsed+=length;
    }

    bk_ot_flash_log("ot flash interface test ok!!!\r\n");
}
static void cli_ot_flash_selftest_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    otTestFlash();
}

#define OT_FLASH_CMD_CNT (sizeof(s_ot_flash_commands)/sizeof(struct cli_command))
static const struct cli_command s_ot_flash_commands[] = {
    {"otFlash", "otFlash {read | write | erase} {indx} {offset} {size}", cli_ot_argflash_cmd},
    {"otFlashSelfTest", "openthread flash interface selftest", cli_ot_flash_selftest_cmd},
};
int cli_ot_flash_test_init(void)
{
    return cli_register_commands(s_ot_flash_commands, OT_FLASH_CMD_CNT);
}