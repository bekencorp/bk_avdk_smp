// Copyright 2022-2025 Beken
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

#include <stdint.h>

#define CONFIG_PSRAM_HEAP_BASE CONFIG_CP_PSRAM_HEAP_ADDR
#define CONFIG_PSRAM_HEAP_SIZE CONFIG_CP_PSRAM_HEAP_SIZE

#define BK_DUMP_PRINT_UART_PORT CONFIG_UART_PRINT_PORT
#define BK_DUMP_TASK_WD_TIMER_INTERRUPT   (3)   // TODO: Set appropriate interrupt number

typedef struct bk_dump_mem_info {
    const char *name;
    uint32_t start_addr;
    uint32_t size;
} bk_dump_mem_info_t;

const bk_dump_mem_info_t* bk_get_peri_reg_info_list(void);
uint32_t bk_get_peri_reg_info_count(void);
const bk_dump_mem_info_t* bk_get_sram_info_list(void);
uint32_t bk_get_sram_info_count(void);

void bk_get_psram_heap_info(bk_dump_mem_info_t *info);
void bk_get_psram_bss_info(bk_dump_mem_info_t *info);
void bk_get_psram_data_info(bk_dump_mem_info_t *info);
void bk_get_ap_psram_heap_info(bk_dump_mem_info_t *info);
   
bool bk_check_addr_in_code_section(uint32_t addr);
bool bk_check_addr_in_ram(uint32_t addr);
uint32_t bk_get_msp_bottom(void);
uint32_t bk_get_current_stack_bottom(void);
