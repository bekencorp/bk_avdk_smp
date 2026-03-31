// Copyright 2026-2026 Beken
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

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *sram_malloc_impl(size_t size);
void sram_free_impl(void *ptr);
bool ptr_is_sram_heap(void *ptr);
void sram_free_debug(const char *func_name, int line, void *ptr);
void sram_free_release(void *ptr);

#ifdef CONFIG_AP_HSRAM_HEAP_ADDR
void *hsram_malloc_impl(size_t size);
void hsram_free_impl(void *ptr);
bool ptr_is_hsram_heap(void *ptr);
void hsram_free_debug(const char *func_name, int line, void *ptr);
void hsram_free_release(void *ptr);
#endif

#ifdef CONFIG_AP_PSRAM_HEAP_ADDR
void *psram_malloc_impl(size_t size);
void psram_free_impl(void *ptr);
bool ptr_is_psram_heap(void *ptr);
void psram_free_debug(const char *func_name, int line, void *ptr);
void psram_free_release(void *ptr);
#endif

#ifdef __cplusplus
}
#endif