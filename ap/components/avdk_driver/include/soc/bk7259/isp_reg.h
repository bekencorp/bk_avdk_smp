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


#define ISP_VI_CCL_ADDR (SOC_ISP_REG_BASE + (0x0 << 2))

#define ISP_VI_CCL_VI_CCL_DIS_STATUS_POS (0)
#define ISP_VI_CCL_VI_CCL_DIS_STATUS_MASK (0x1)

#define ISP_VI_CCL_VI_CCL_DIS_POS (1)
#define ISP_VI_CCL_VI_CCL_DIS_MASK (0x3)

#define ISP_VI_CCL_RESERVED_BIT_3_31_POS (3)
#define ISP_VI_CCL_RESERVED_BIT_3_31_MASK (0x1fffffff)

#define ISP_ISP_ID_CUSTOM_ID_ADDR (SOC_ISP_REG_BASE + (0x1 << 2))

#define ISP_ISP_ID_CUSTOM_ID_ISP_ID_CUSTOM_ID_POS (0)
#define ISP_ISP_ID_CUSTOM_ID_ISP_ID_CUSTOM_ID_MASK (0xffffffff)

#define ISP_TIMEOUT_CFG_STREAM11_ADDR (SOC_ISP_REG_BASE + (0x1075 << 2))

#define ISP_TIMEOUT_CFG_STREAM11_CONSUMER_THRESHOLD_POS (0)
#define ISP_TIMEOUT_CFG_STREAM11_CONSUMER_THRESHOLD_MASK (0xffff)

#define ISP_TIMEOUT_CFG_STREAM11_PRODUCER_THRESHOLD_POS (16)
#define ISP_TIMEOUT_CFG_STREAM11_PRODUCER_THRESHOLD_MASK (0xffff)

#define ISP_STREAM_STATUS_STREAM11_ADDR (SOC_ISP_REG_BASE + (0x1076 << 2))

#define ISP_STREAM_STATUS_STREAM11_RESERVED_SBI_POS (0)
#define ISP_STREAM_STATUS_STREAM11_RESERVED_SBI_MASK (0x3fff)

#define ISP_STREAM_STATUS_STREAM11_STREAMING_STATE_POS (14)
#define ISP_STREAM_STATUS_STREAM11_STREAMING_STATE_MASK (0x3)

#define ISP_STREAM_STATUS_STREAM11_VALID_ENTRY_COUNT_POS (16)
#define ISP_STREAM_STATUS_STREAM11_VALID_ENTRY_COUNT_MASK (0xffff)

#ifdef __cplusplus
}
#endif
