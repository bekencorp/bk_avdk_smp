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

#pragma once

#include <components/media_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_SLAB_MEM_DEBUG (1)

typedef enum
{
    MEM_SLAB_HEAP_CODED,
    MEM_SLAB_HEAP_UNCODED,
    MEM_SLAB_HEAP_MAX,
} frame_buffer_heap_type_t;

#if MEM_SLAB_MEM_DEBUG
#define bk_frame_buffer_malloc(type, size) bk_frame_buffer_malloc_debug(type, size, __FUNCTION__, __LINE__)
void *bk_frame_buffer_malloc_debug(frame_buffer_heap_type_t type, uint32_t size, const char *func, uint32_t line);
#else
void *bk_frame_buffer_malloc(frame_buffer_heap_type_t type, uint32_t size);
#endif
void bk_frame_buffer_free(void *frame);
void bk_frame_buffer_init(void);
void bk_frame_buffer_resume(void);

void bk_mem_slab_dump_heap(uint8_t type);
void bk_mem_slab_dump_all_heaps(void);
void bk_mem_slab_check_all_heaps(void);

#ifdef __cplusplus
}
#endif
