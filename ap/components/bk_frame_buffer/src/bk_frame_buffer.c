// Copyright 2023-2024 Beken
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
#include <os/mem.h>

#include <components/log.h>
#include <components/bk_frame_buffer.h>

#include "bk_mem_slab.h"
#include "frame_buffer_config.h"

#define TAG "frame_buffer"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


#if MEM_SLAB_MEM_DEBUG
void *bk_frame_buffer_malloc_debug(frame_buffer_heap_type_t type, uint32_t size, const char *func, uint32_t line)
{
    return bk_mem_slab_malloc_debug(type, size, func, line);
}
#else
void *bk_frame_buffer_malloc(frame_buffer_heap_type_t type, uint32_t size)
{
    return bk_mem_slab_malloc(type, size);
}
#endif

void bk_frame_buffer_free(void *frame)
{
    bk_mem_slab_free(frame);
}

void bk_frame_buffer_resume(void)
{
    if (VIDEO_MEM_SLAB_CODED_SIZE)
    {
        bk_mem_slab_heap_resume(MEM_SLAB_HEAP_CODED, (uint8_t *)(VIDEO_MEM_SLAB_CODED_ADDR), VIDEO_MEM_SLAB_CODED_SIZE);
    }

    if (VIDEO_MEM_SLAB_UNCODED_SIZE)
    {
        bk_mem_slab_heap_resume(MEM_SLAB_HEAP_UNCODED, (uint8_t *)(VIDEO_MEM_SLAB_UNCODED_ADDR), VIDEO_MEM_SLAB_UNCODED_SIZE);
    }
}

void bk_frame_buffer_init(void)
{
    bk_mem_slab_init();

    if (VIDEO_MEM_SLAB_CODED_SIZE)
    {
        bk_mem_slab_heap_init(MEM_SLAB_HEAP_CODED, (uint8_t *)(VIDEO_MEM_SLAB_CODED_ADDR), VIDEO_MEM_SLAB_CODED_SIZE);
    }

    if (VIDEO_MEM_SLAB_UNCODED_SIZE)
    {
        bk_mem_slab_heap_init(MEM_SLAB_HEAP_UNCODED, (uint8_t *)(VIDEO_MEM_SLAB_UNCODED_ADDR), VIDEO_MEM_SLAB_UNCODED_SIZE);
    }

    LOGI("%s: [%s: %s] success\n", __func__,
        VIDEO_MEM_SLAB_CODED_SIZE ? "coded" : "N/A",
        VIDEO_MEM_SLAB_UNCODED_SIZE ? "uncoded" : "N/A");

}