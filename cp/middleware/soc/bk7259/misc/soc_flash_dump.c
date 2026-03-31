// Copyright 2020-2021 Beken
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

#include "soc_flash_dump.h"
#include <driver/flash_partition.h>

bk_partition_t dump_partition_id = BK_PARTITION_OTA;
DUMP_MAP_T dump_map[] = DUMP_MODULE_MAP;
uint32_t g_dump_position = 0;

bk_err_t soc_fdump_buf(uint8_t *buf, uint32_t len)
{
    bk_err_t ret;

    ret = bk_flash_partition_write(dump_partition_id, (uint8_t *)buf, g_dump_position, len);
    if(BK_OK != ret){
        return ret;
    }

    g_dump_position += len; 

    return ret;
}

uint32_t soc_fdump_get_dump_space_size(void)
{
    DUMP_MAP_T *dump_item_ptr;
    uint32_t space_size = 0;
    uint32_t i, save_cnt;

    for(i = 0; i < sizeof(dump_map)/sizeof(dump_map[0]); i ++){
        dump_item_ptr = &dump_map[i];
        save_cnt = MAX(dump_item_ptr->len, dump_item_ptr->end - dump_item_ptr->start);
        if(0 == save_cnt){
            break;
        }

        space_size += save_cnt;
    }

    space_size = (space_size + DUMP_PAGE_SIZE - 1) / DUMP_PAGE_SIZE * DUMP_PAGE_SIZE;

    return space_size;
}

bk_err_t soc_fdump_init_partition(void)
{
    bk_err_t ret = BK_OK;
    static uint32_t s_init_dump_partition_flag = 0;

    if(0 == s_init_dump_partition_flag){
#if CONFIG_DUMP_ERASE_PARTITION_ALL
        bk_flash_partition_erase_all(dump_partition_id);
#else
        uint32_t erase_len;
        erase_len = soc_fdump_get_dump_space_size();
        bk_flash_partition_erase(dump_partition_id, 0, erase_len);
#endif

        ret = soc_fdump_buf((uint8_t *)dump_map, sizeof(dump_map));
        if(BK_OK != ret){
            return ret;
        }

        s_init_dump_partition_flag = 1;
    }

    return ret;
}

bk_err_t soc_fdump_cpu_registers(uint32_t mcause, SAVED_CONTEXT *context)
{
    bk_err_t ret;

    soc_fdump_init_partition();

    ret = soc_fdump_buf((uint8_t *)context, sizeof(*context));
    if(BK_OK != ret){
        return ret;
    }

    return ret; 
}

bk_err_t soc_fdump_save(void)
{
    bk_err_t ret = BK_OK;
    uint32_t i, save_cnt;
    SECTION_HDR_T item_hdr = {SECTION_HDR_MAGIC_WORD};
    DUMP_MAP_T *dump_item_ptr;

    soc_fdump_init_partition();

    for(i = 1; i < sizeof(dump_map)/sizeof(dump_map[0]); i ++){
        dump_item_ptr = &dump_map[i];
        save_cnt = MAX(dump_item_ptr->len, dump_item_ptr->end - dump_item_ptr->start);
        if(0 == save_cnt){
            break;
        }

        item_hdr.len = save_cnt;
        item_hdr.start = dump_item_ptr->start;
        item_hdr.end = dump_item_ptr->end;
        ret = soc_fdump_buf((uint8_t *)&item_hdr, sizeof(item_hdr));
        if(BK_OK != ret){
            break;
        }

        ret = soc_fdump_buf((uint8_t *)dump_item_ptr->start, save_cnt);
        if(BK_OK != ret){
            break;
        }
    }

    return ret;
}
//eof

