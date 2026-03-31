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

#include "stack_base.h"
#include <soc/soc.h>

/*  dump information:
    section 0.0:dump_map
    section 0.1: cpu context
    section 1: section_hdr + section
    section 2: section_hdr + section
    .......
 */
enum{
    MODULE_ID_CPU = 0xBEEF0000,
    MODULE_ID_PPB_SYS_CTRL,
    MODULE_ID_PPB_MPU,
    MODULE_ID_SRAM,
    MODULE_ID_UART0,
    MODULE_ID_FLASH
};

typedef struct _dump_map_{
    uint32_t id;
    uint32_t start;
    uint32_t end;
    uint32_t len;
}DUMP_MAP_T;

#define SECTION_HDR_MAGIC_WORD  (0xA5A5AA55)
typedef struct section_hdr{
    uint32_t magic;
    uint32_t start;
    uint32_t end;
    uint32_t len;
}SECTION_HDR_T;

/* module_id, start_addr, end_addr, save_count*/
#define DUMP_MODULE_MAP  \
{\
    {MODULE_ID_CPU, 0, 0, 24 * 4},\
    {MODULE_ID_PPB_SYS_CTRL, 0xE000ED00, 0xE000ED64 + 4},\
    {MODULE_ID_PPB_MPU, 0xE000ED90, 0xE000EDC4 + 4},\
    {MODULE_ID_SRAM, SOC_SRAM0_DATA_BASE, SOC_SRAM0_DATA_BASE + 0x09ffff},\
    {MODULE_ID_UART0, SOC_UART0_REG_BASE, SOC_UART0_REG_BASE + 0x0C * 4 + 4},\
    {MODULE_ID_FLASH, SOC_FLASH_REG_BASE, SOC_FLASH_REG_BASE + 4}, \
    {0xFFFFFFFF, 0, 0, 0}, \
}

#define DUMP_PAGE_SIZE     (4096)

bk_err_t soc_fdump_cpu_registers(uint32_t mcause, SAVED_CONTEXT *context);
bk_err_t soc_fdump_save(void);

// eof

