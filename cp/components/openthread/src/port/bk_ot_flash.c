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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openthread/platform/flash.h>
#include <openthread/platform/toolchain.h>
#include "sdkconfig.h"


#include "driver/flash.h"
#include <driver/flash_partition.h>
#include <vnd_flash_partition.h>

#include "flash/flash_driver.h"
#include "os/mem.h"

#if CONFIG_OPENTHREAD
#define  BK_CHECK_POINTER_NULL(pointer) \
do{\
    if(NULL == pointer)\
    {\
        os_printf("[%s][%d] pointer is null \r\n",__func__,__LINE__);\
    }\
}while(0)

#define CFG_OPENTHREAD_SWAP_SIZE    8192//4096 //same as the bk7236n_parititons.cs
/**
 * Initializes the flash driver.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 */
void otPlatFlashInit(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
}

/**
 * Gets the size of the swap space.
 *
 * @param[in] aInstance  The OpenThread instance structure.
 *
 * @returns The size of the swap space in bytes.
 */
uint32_t otPlatFlashGetSwapSize(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return CFG_OPENTHREAD_SWAP_SIZE;
}

/**
 * Erases the swap space indicated by @p aSwapIndex.
 *
 * @param[in] aInstance   The OpenThread instance structure.
 * @param[in] aSwapIndex  A value in [0, 1] that indicates the swap space.
 */
void otPlatFlashErase(otInstance *aInstance, uint8_t aSwapIndex)
{
    OT_UNUSED_VARIABLE(aInstance);
    if(aSwapIndex>1)
        os_printf("[Error] ot flash swap index error\r\n");

    uint32_t operation_addr = 0;
    bk_logic_partition_t *partition_info = NULL;
    uint8_t uSectorSize = CFG_OPENTHREAD_SWAP_SIZE/FLASH_SECTOR_SIZE;

    partition_info = bk_flash_partition_get_info(BK_PARTITION_OT_SETTING_USER);
    BK_CHECK_POINTER_NULL(partition_info);

    operation_addr = partition_info->partition_start_addr;

    if(aSwapIndex == 0)
    {
        operation_addr += 0;
    }
    else
    {
        operation_addr += CFG_OPENTHREAD_SWAP_SIZE;
    }

    for(uint8_t i=0;i<uSectorSize;i++)
    {
        bk_flash_erase_sector(operation_addr+i*FLASH_SECTOR_SIZE);
    }
}

/**
 * Reads @p aSize bytes into @p aData.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aSwapIndex  A value in [0, 1] that indicates the swap space.
 * @param[in]  aOffset     A byte offset within the swap space.
 * @param[out] aData       A pointer to the data buffer for reading.
 * @param[in]  aSize       Number of bytes to read.
 */
void otPlatFlashRead(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);
    BK_CHECK_POINTER_NULL(aData);
    uint32_t data_offset_addr=0;
    if(aSwapIndex == 0)
        data_offset_addr = aOffset;
    else
        data_offset_addr = CFG_OPENTHREAD_SWAP_SIZE + aOffset;

    bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_OT_SETTING_USER);
    BK_CHECK_POINTER_NULL(pt);
    bk_flash_read_bytes((pt->partition_start_addr + data_offset_addr), (uint8_t *)aData, aSize);
}

/**
 * Writes @p aSize bytes from @p aData.
 *
 * @param[in]  aInstance   The OpenThread instance structure.
 * @param[in]  aSwapIndex  A value in [0, 1] that indicates the swap space.
 * @param[in]  aOffset     A byte offset within the swap space.
 * @param[out] aData       A pointer to the data to write.
 * @param[in]  aSize       Number of bytes to write.
 */
void otPlatFlashWrite(otInstance *aInstance, uint8_t aSwapIndex, uint32_t aOffset, const void *aData, uint32_t aSize)
{
    OT_UNUSED_VARIABLE(aInstance);
    BK_CHECK_POINTER_NULL(aData);
    uint32_t data_offset_addr=0;

    bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_OT_SETTING_USER);
    BK_CHECK_POINTER_NULL(pt);
    if(aSwapIndex == 0)
    {
        data_offset_addr = aOffset;
    }
    else
    {
        data_offset_addr = CFG_OPENTHREAD_SWAP_SIZE + aOffset;
    }
    bk_flash_write_bytes((pt->partition_start_addr + data_offset_addr), (uint8_t *)aData, aSize);
}
#endif