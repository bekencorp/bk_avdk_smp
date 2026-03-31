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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif


typedef volatile union
{
    struct
    {
        uint32_t vi_ccl_dis_status        :  1; /**<bit[0 : 0] */
        uint32_t vi_ccl_dis               :  2; /**<bit[1 : 2] */
        uint32_t reserved_bit_3_31        : 29; /**<bit[3 : 31] */
    };
    uint32_t v;
} isp_vi_ccl_t;


typedef volatile union
{
    struct
    {
        uint32_t isp_id_custom_id         : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} isp_isp_id_custom_id_t;


typedef volatile union
{
    struct
    {
        uint32_t consumer_threshold       : 16; /**<bit[0 : 15] */
        uint32_t producer_threshold       : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} isp_timeout_cfg_stream11_t;


typedef volatile union
{
    struct
    {
        uint32_t reserved_sbi             : 14; /**<bit[0 : 13] */
        uint32_t streaming_state          :  2; /**<bit[14 : 15] */
        uint32_t valid_entry_count        : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} isp_stream_status_stream11_t;

typedef volatile struct
{
    volatile isp_vi_ccl_t vi_ccl;
    volatile isp_isp_id_custom_id_t isp_id_custom_id;
    volatile uint32_t rsv_2_1074[4211];
    volatile isp_timeout_cfg_stream11_t timeout_cfg_stream11;
    volatile isp_stream_status_stream11_t stream_status_stream11;
} isp_hw_t;

#ifdef __cplusplus
}
#endif
