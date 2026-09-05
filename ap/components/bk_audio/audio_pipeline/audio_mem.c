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


#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <stdint.h>
#include <stdlib.h>
#include "string.h"
#include <components/bk_audio/audio_pipeline/audio_mem.h>

//#define ENABLE_AUDIO_MEM_TRACE

#define TAG "AUD_MEM"
#define AUDIO_HSRAM_NONCACHE_OFFSET 0x4000000U

typedef struct audio_mem_info_t {
    uint32_t aud_malloc_total_size;
    uint32_t aud_calloc_total_size;
    uint32_t aud_strdup_total_size;
    uint32_t aud_dma_calloc_total_size;
} audio_mem_info_t;

static audio_mem_info_t audio_mem_info = {
    .aud_malloc_total_size = 0,
    .aud_calloc_total_size = 0,
    .aud_strdup_total_size = 0,
    .aud_dma_calloc_total_size = 0,
};

void *audio_malloc(uint32_t size)
{
    void *data =  NULL;
#if CONFIG_ADK_USE_SRAM
    data = os_malloc(size);
#else
    data = psram_malloc(size);
#endif

    if (data)
    {
        os_memset(data, 0, size);
        audio_mem_info.aud_malloc_total_size += size;
    }

#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGD(TAG, "malloc:%p, size:%d, called:0x%08x \n", data, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    return data;
}

void audio_free(void *ptr)
{
    os_free(ptr);

#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGD(TAG, "free:%p, called:0x%08x \n", ptr, (intptr_t)__builtin_return_address(0) - 2);
#endif
}

void *audio_hsram_malloc(uint32_t size)
{
    void *data = hsram_malloc(size);
#if CONFIG_SRAM_DIRECT_ADDR
    void *ret_data = data;
#else
    uint32_t raw_addr = (uint32_t)(uintptr_t)data;
    void *ret_data = data ? (void *)(uintptr_t)(raw_addr + AUDIO_HSRAM_NONCACHE_OFFSET) : NULL;
#endif
#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGD(TAG, "hsram malloc raw:%p, ret:%p, size:%d, called:0x%08x \r\n",
        data, ret_data, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    return ret_data;
}

void audio_hsram_free(void *ptr)
{
    if (!ptr)
    {
        BK_LOGE(TAG, "hsram free ptr is NULL\r\n");
        return;
    }
#if CONFIG_SRAM_DIRECT_ADDR
    void *raw_ptr = ptr;
#else
    uint32_t ret_addr = (uint32_t)(uintptr_t)ptr;
    void *raw_ptr = ptr ? (void *)(uintptr_t)(ret_addr - AUDIO_HSRAM_NONCACHE_OFFSET) : NULL;
#endif
    hsram_free(raw_ptr);

#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGD(TAG, "hsram free ret:%p, raw:%p, called:0x%08x \r\n",
        ptr, raw_ptr, (intptr_t)__builtin_return_address(0) - 2);
#endif
}

void *audio_calloc(uint32_t nmemb, uint32_t size)
{
    void *data =  NULL;
    //data = calloc(nmemb, size);
#if CONFIG_ADK_USE_SRAM
    data = os_malloc(nmemb * size);
#else
    data = psram_malloc(nmemb * size);
#endif
    if (data)
    {
        os_memset(data, 0x00, nmemb * size);
        audio_mem_info.aud_calloc_total_size += nmemb * size;
    }

#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGD(TAG, "calloc:%p, size:%d, called:0x%08x \n", data, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    return data;
}

char *audio_strdup(const char *str)
{
#if CONFIG_ADK_USE_SRAM
    char *copy = os_malloc(strlen(str) + 1);
#else
    char *copy = psram_malloc(strlen(str) + 1);
#endif
    if (copy)
    {
        strcpy(copy, str);
        audio_mem_info.aud_strdup_total_size += strlen(str);
    }
#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGD(TAG, "strdup:%p, size:%d, called:0x%08x \n", copy, strlen(copy), (intptr_t)__builtin_return_address(0) - 2);
#endif
    return copy;
}

void audio_mem_print(char *tag, int line, const char *func)
{
    BK_LOGD(TAG, "Func:%s, Line:%d, MEM Total:%d Bytes\r\n", func, line, rtos_get_free_heap_size());
    BK_LOGD(TAG, "aud_malloc_total_size:%d, aud_calloc_total_size:%d, aud_strdup_total_size:%d, aud_dma_calloc_total_size:%d\r\n\n", 
        audio_mem_info.aud_malloc_total_size, 
        audio_mem_info.aud_calloc_total_size, 
        audio_mem_info.aud_strdup_total_size, 
        audio_mem_info.aud_dma_calloc_total_size);
    os_memset(&audio_mem_info, 0x00, sizeof(audio_mem_info_t));
}

void audio_dma_mem_free(void *ptr)
{
    os_free(ptr);

#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGD(TAG, "dma mem free:%p, called:0x%08x \n", ptr, (intptr_t)__builtin_return_address(0) - 2);
#endif
}

void *audio_dma_mem_calloc(uint32_t nmemb, uint32_t size)
{
    void *data =  NULL;

#if CONFIG_ADK_USE_SRAM
    data = os_malloc(nmemb * size);
#else
    data = psram_malloc(nmemb * size);
#endif
    if (data)
    {
        os_memset(data, 0x00, nmemb * size);
        audio_mem_info.aud_dma_calloc_total_size += nmemb * size;
    }

#ifdef ENABLE_AUDIO_MEM_TRACE
    BK_LOGD(TAG, "dma mem calloc:%p, size:%d, called:0x%08x \n", data, size, (intptr_t)__builtin_return_address(0) - 2);
#endif
    return data;
}

