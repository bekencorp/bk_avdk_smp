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
//

#pragma once

#include <common/bk_include.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (CONFIG_PSRAM_MEM_SLAB_UNCODED_SIZE) && (CONFIG_PSRAM_MEM_SLAB_UNCODED_ADDR)
#define VIDEO_MEM_SLAB_UNCODED_SIZE (CONFIG_PSRAM_MEM_SLAB_UNCODED_SIZE)
#define VIDEO_MEM_SLAB_UNCODED_ADDR (CONFIG_PSRAM_MEM_SLAB_UNCODED_ADDR)
#else
#define VIDEO_MEM_SLAB_UNCODED_SIZE (0)
#define VIDEO_MEM_SLAB_UNCODED_ADDR (0)
#endif

#if (CONFIG_PSRAM_MEM_SLAB_CODED_SIZE) && (CONFIG_PSRAM_MEM_SLAB_CODED_ADDR)
#define VIDEO_MEM_SLAB_CODED_SIZE (CONFIG_PSRAM_MEM_SLAB_CODED_SIZE)
#define VIDEO_MEM_SLAB_CODED_ADDR (CONFIG_PSRAM_MEM_SLAB_CODED_ADDR)
#else
#define VIDEO_MEM_SLAB_CODED_SIZE (0)
#define VIDEO_MEM_SLAB_CODED_ADDR (0)
#endif


#ifdef __cplusplus
}
#endif

