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

enum
{
    DWT_MATCH_DISABLE = 0x0,
    DWT_MATCH_CYCLE_COUNTER_MATCH,
    DWT_MATCH_INSTRUCTION_ADDR,
    DWT_MATCH_INSTRUCTION_ADDR_LIMIT,
    DWT_MATCH_DATA_ADDR,
    DWT_MATCH_DATA_ADDR_WR = 0x5,
    DWT_MATCH_DATA_ADDR_RD,
    DWT_MATCH_DATA_ADDR_LIMIT,
    DWT_MATCH_DATA_VAL,
    DWT_MATCH_DATA_VAL_WR,
    DWT_MATCH_DATA_VAL_RD = 0xA,
    DWT_MATCH_DATA_VAL_LINKED,
    DWT_MATCH_DATA_ADDR_WITH_VAL,
    DWT_MATCH_DATA_ADDR_WITH_VAL_WR_ONLY,
    DWT_MATCH_DATA_ADDR_WITH_VAL_RD_ONLY
};

typedef enum
{
    ACCESS_TYPE_WHATEVER = 0,
    ACCESS_TYPE_WRITE,
    ACCESS_TYPE_READ
}ACCESS_T;

enum
{
    DWT_ACTION_TRIGGER_ONLY = 0x0,
    DWT_ACTION_DEBUG_EVENT,
    DWT_ACTION_DATA_TRACE_MATCH_PKT,
    DWT_ACTION_DATA_ADDR_PKT
};

enum
{
    COMP_ID_0 = 0,
    COMP_ID_1 = 1,
    COMP_ID_2 = 2,
    COMP_ID_3 = 3,
    COMP_ID_MAX
};

enum
{
    FUNC_ID_8 = 0x08,
    FUNC_ID_9 = 0x09,
    FUNC_ID_10 = 0x0A,
    FUNC_ID_11 = 0x0B,
    FUNC_ID_24 = 0x18,
    FUNC_ID_26 = 0x1A,
    FUNC_ID_28 = 0x1C,
    FUNC_ID_30 = 0x1E,
};

typedef struct _id_cap_
{
    uint32_t id;
    uint32_t cap;
}ID_CAP_T;

#define ID_MAX_CNT    (8)

#define ID_CAP_TABLE \
{\
    {FUNC_ID_8, 0x03},\
    {FUNC_ID_9, 0x83},\
    {FUNC_ID_10, 0x23},\
    {FUNC_ID_11, 0x93},\
    {FUNC_ID_24, 0x07},\
    {FUNC_ID_26, 0x67},\
    {FUNC_ID_28, 0x1f},\
    {FUNC_ID_30, 0x7f},\
}

#define CAP_MAX_CNT    (8)

#define STR_CAP_TABLE \
{\
    {(1 << 0), "data address"},\
    {(1 << 1), "data address with value"},\
    {(1 << 2), "data address limit"},\
    {(1 << 3), "data value"},\
    {(1 << 4), "linked data value"},\
    {(1 << 5), "instruction address"},\
    {(1 << 6), "instruction address limit"},\
    {(1 << 7), "cycle counter"},\
}

enum
{
    FUNC_DATA_SIZE_NONE = 0,
    FUNC_DATA_SIZE_BYTE = 0,
    FUNC_DATA_SIZE_HALF_WORD = 1,
    FUNC_DATA_SIZE_WORD = 2,
};

typedef struct _bit_cap_str_
{
    uint32_t bit_val;
    char *cap_desc;
}BIT_CAP_STR_T;

void dwt_enable_debug_monitor_exception(void);
void dwt_enable_debug_monitor_mode(void);
void dwt_disable_debug_monitor_mode(void);
void dwt_init_cycle_counter(void);
void dwt_reset_cycle_counter(void);
void dwt_enable_cycle_counter(void);
void dwt_disable_cycle_counter(void);
uint32_t dwt_get_cycle_counter_val(void);
void dwt_set_data_address_read(uint32_t data_address);
void dwt_set_data_address_write(uint32_t data_address);
void dwt_set_data_address_access(uint32_t data_address);
void dwt_set_instruction_address(uint32_t data_address);
void dwt_set_data_address_range(uint32_t start_addr, uint32_t end_addr, ACCESS_T type);
void dwt_set_instruction_address_range(uint32_t start_addr, uint32_t end_addr);
void dwt_conditional_data_watchpoint(uint32_t addr, uint32_t data);
void dwt_disable_watchpoint_comparator(void);

// eof

