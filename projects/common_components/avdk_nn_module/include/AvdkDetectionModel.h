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

#include "common/avdk_pixel_types.h"


#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/testing/micro_test.h"


#define MICRO_OP_RESOLVER_SIZE 5

typedef enum {
    AVDK_NN_MEM_TYPE_FALSH,
    AVDK_NN_MEM_TYPE_HSRAM,
    AVDK_NN_MEM_TYPE_SRAM,
    AVDK_NN_MEM_TYPE_PSRAM_HEAP,
    AVDK_NN_MEM_TYPE_PSRAM_SLAB,
} avdk_nn_mem_type_t;

typedef enum {
    AVDK_NN_MODEL_TYPE_CPU,
    AVDK_NN_MODEL_TYPE_NPU,
} avdk_nn_model_type_t;

class AvdkDetectionModel {

protected:
    uint8_t* tensor_arena;          // Aligned pointer for TFLM
    void* tensor_arena_raw;          // Original pointer for freeing
    tflite::MicroInterpreter* pinterpreter;
    tflite::MicroMutableOpResolver<MICRO_OP_RESOLVER_SIZE> micro_op_resolver;


    const char *name;
    uint16_t width;
    uint16_t height;
    bk_pixel_format_t format;

    avdk_nn_model_type_t model_type;

    avdk_nn_mem_type_t model_ram_type;
    uint8_t *model_data;
    uint32_t model_data_size;
    uint8_t *model_flash_data;
    uint32_t model_flash_data_size;

    avdk_nn_mem_type_t fast_ram_type;
    uint8_t *fast_ram_data;
    uint32_t fast_ram_data_size;

    avdk_nn_mem_type_t arena_ram_type;
    uint8_t *arena_ram_data;
    uint32_t arena_data_size;

    void *allocMemory(avdk_nn_mem_type_t type, uint32_t size);
    void freeMemory(avdk_nn_mem_type_t type, void *ptr);


    int LoadModel();
    int UnloadModel();

public:
    AvdkDetectionModel();

    virtual void resolverLoad() = 0;
    virtual void resourceLoad() = 0;
    virtual void resourceUnload() = 0;
    virtual int run(uint8_t *data, uint32_t size) = 0;

    int init();
    int deinit();
    int npuStartup();
    int npuShutdown();
    int cpuStartup();
    int cpuShutdown();

    uint16_t getWidth();
    uint16_t getHeight();
    bk_pixel_format_t getFormat();

    static void LogEnable(bool enable);
};




