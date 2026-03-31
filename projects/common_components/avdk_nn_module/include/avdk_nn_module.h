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


#ifdef __cplusplus
extern "C" {
#endif

#include <common/avdk_pixel_types.h>

#ifdef __has_attribute
#define HAVE_ATTRIBUTE(x) __has_attribute(x)
#else
#define HAVE_ATTRIBUTE(x) 0
#endif
#if HAVE_ATTRIBUTE(aligned) || (defined(__GNUC__) && !defined(__clang__))
// Ethos-U command stream requires 16-byte aligned addresses.
#define DATA_ALIGN_ATTRIBUTE __attribute__((aligned(16)))
#else
#define DATA_ALIGN_ATTRIBUTE
#endif

typedef struct avdk_nn_module_t avdk_nn_module_t;

typedef enum {
    AVDK_NN_MEM_TYPE_FALSH,
    AVDK_NN_MEM_TYPE_HSRAM,
    AVDK_NN_MEM_TYPE_SRAM,
    AVDK_NN_MEM_TYPE_PSRAM,
} avdk_nn_mem_type_t;

typedef enum {
    AVDK_NN_MODEL_TYPE_CPU,
    AVDK_NN_MODEL_TYPE_NPU,
} avdk_nn_model_type_t;

struct avdk_nn_module_t{
    char *name;
    uint16_t width;
    uint16_t height;
    bk_pixel_format_t format;
    uint8_t *model_data;
    uint32_t model_data_size;
    avdk_nn_model_type_t model_type;
    uint32_t npu_fast_ram_size;
    avdk_nn_mem_type_t model_load_type;
    avdk_nn_mem_type_t npu_fast_ram_type;
    uint32_t arena_ram_type;
    uint32_t arena_size;


    int (*init) (const avdk_nn_module_t *module);
    int (*deinit) (const avdk_nn_module_t *module);
    int (*run) (const avdk_nn_module_t *module, void* input, uint32_t input_size, void* output, uint32_t *output_size);
};



#ifdef __cplusplus
}
#endif


