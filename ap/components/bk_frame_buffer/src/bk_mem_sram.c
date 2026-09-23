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

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>
#include <components/bk_hardware_ram.h>

#define TAG "bk_mem_sram"

#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

void *bk_sram_hsram_malloc(const char *tag, uint32_t size)
{
    const char *log_tag = (tag != NULL) ? tag : TAG;
    void *ptr = os_sram_malloc(size);

    if (ptr == NULL) {
        LOGW("%s: sram_malloc failed, use hsram_malloc, ptr=%p, size=%u\n", log_tag, ptr, size);
        ptr = hsram_malloc(size);
    }
    return ptr;
}

void *bk_get_isp_flexa_buffer(uint32_t size)
{
    return bk_sram_hsram_malloc("isp_flexa", size);
}

void *bk_get_gpu_flexa_buffer(uint32_t size)
{
    return bk_sram_hsram_malloc("gpu_flexa", size);
}

void *bk_get_gpu_output_buffer(uint32_t size)
{
    return bk_sram_hsram_malloc("gpu_output", size);
}

void *bk_get_dpu_buffer(uint32_t size)
{
    return hsram_malloc(size);
}
