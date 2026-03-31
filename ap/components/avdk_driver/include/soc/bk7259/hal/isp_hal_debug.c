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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#include "hal_config.h"
#include "isp_hw.h"
#include "isp_hal.h"

typedef void (*isp_dump_fn_t)(void);
typedef struct
{
    uint32_t start;
    uint32_t end;
    isp_dump_fn_t fn;
} isp_reg_fn_map_t;

static void isp_dump_vi_ccl(void)
{
    isp_vi_ccl_t *r = (isp_vi_ccl_t *)(SOC_ISP_REG_BASE + (0x0 << 2));

    SOC_LOGI("vi_ccl: %8x\r\n", REG_READ(SOC_ISP_REG_BASE + (0x0 << 2)));
    SOC_LOGI("	vi_ccl_dis_status: %8x\r\n", r->vi_ccl_dis_status);
    SOC_LOGI("	vi_ccl_dis: %8x\r\n", r->vi_ccl_dis);
    SOC_LOGI("	reserved_bit_3_31: %8x\r\n", r->reserved_bit_3_31);
}

static void isp_dump_isp_id_custom_id(void)
{
    SOC_LOGI("isp_id_custom_id: %8x\r\n", REG_READ(SOC_ISP_REG_BASE + (0x1 << 2)));
}

static void isp_dump_rsv_2_1074(void)
{
    for (uint32_t idx = 0; idx < 4211; idx++)
    {
        SOC_LOGI("rsv_2_1074: %8x\r\n", REG_READ(SOC_ISP_REG_BASE + ((0x2 + idx) << 2)));
    }
}

static void isp_dump_timeout_cfg_stream11(void)
{
    isp_timeout_cfg_stream11_t *r = (isp_timeout_cfg_stream11_t *)(SOC_ISP_REG_BASE + (0x1075 << 2));

    SOC_LOGI("timeout_cfg_stream11: %8x\r\n", REG_READ(SOC_ISP_REG_BASE + (0x1075 << 2)));
    SOC_LOGI("	consumer_threshold: %8x\r\n", r->consumer_threshold);
    SOC_LOGI("	producer_threshold: %8x\r\n", r->producer_threshold);
}

static void isp_dump_stream_status_stream11(void)
{
    isp_stream_status_stream11_t *r = (isp_stream_status_stream11_t *)(SOC_ISP_REG_BASE + (0x1076 << 2));

    SOC_LOGI("stream_status_stream11: %8x\r\n", REG_READ(SOC_ISP_REG_BASE + (0x1076 << 2)));
    SOC_LOGI("	reserved_sbi: %8x\r\n", r->reserved_sbi);
    SOC_LOGI("	streaming_state: %8x\r\n", r->streaming_state);
    SOC_LOGI("	valid_entry_count: %8x\r\n", r->valid_entry_count);
}

static isp_reg_fn_map_t s_fn[] =
{
    {0x0, 0x0, isp_dump_vi_ccl},
    {0x1, 0x1, isp_dump_isp_id_custom_id},
    {0x2, 0x1075, isp_dump_rsv_2_1074},
    {0x1075, 0x1075, isp_dump_timeout_cfg_stream11},
    {0x1076, 0x1076, isp_dump_stream_status_stream11},
    {-1, -1, 0}
};

void isp_struct_dump(uint32_t start, uint32_t end)
{
    uint32_t dump_fn_cnt = sizeof(s_fn) / sizeof(s_fn[0]) - 1;

    for (uint32_t idx = 0; idx < dump_fn_cnt; idx++)
    {
        if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end))
        {
            s_fn[idx].fn();
        }
    }
}
