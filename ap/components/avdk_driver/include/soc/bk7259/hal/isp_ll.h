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

#include <soc/soc.h>
#include "hal_port.h"
#include "isp_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISP_LL_REG_BASE   SOC_ISP_REG_BASE

//reg vi_ccl:

static inline void isp_ll_set_vi_ccl_value(uint32_t v)
{
    isp_vi_ccl_t *r = (isp_vi_ccl_t *)(SOC_ISP_REG_BASE + (0x0 << 2));
    r->v = v;
}

static inline uint32_t isp_ll_get_vi_ccl_value(void)
{
    isp_vi_ccl_t *r = (isp_vi_ccl_t *)(SOC_ISP_REG_BASE + (0x0 << 2));
    return r->v;
}

static inline void isp_ll_set_vi_ccl_vi_ccl_dis_status(uint32_t v)
{
    isp_vi_ccl_t *r = (isp_vi_ccl_t *)(SOC_ISP_REG_BASE + (0x0 << 2));
    r->vi_ccl_dis_status = v;
}

static inline uint32_t isp_ll_get_vi_ccl_vi_ccl_dis_status(void)
{
    isp_vi_ccl_t *r = (isp_vi_ccl_t *)(SOC_ISP_REG_BASE + (0x0 << 2));
    return r->vi_ccl_dis_status;
}

static inline void isp_ll_set_vi_ccl_vi_ccl_dis(uint32_t v)
{
    isp_vi_ccl_t *r = (isp_vi_ccl_t *)(SOC_ISP_REG_BASE + (0x0 << 2));
    r->vi_ccl_dis = v;
}

static inline uint32_t isp_ll_get_vi_ccl_vi_ccl_dis(void)
{
    isp_vi_ccl_t *r = (isp_vi_ccl_t *)(SOC_ISP_REG_BASE + (0x0 << 2));
    return r->vi_ccl_dis;
}

//reg isp_id_custom_id:

static inline void isp_ll_set_isp_id_custom_id_value(uint32_t v)
{
    isp_isp_id_custom_id_t *r = (isp_isp_id_custom_id_t *)(SOC_ISP_REG_BASE + (0x1 << 2));
    r->v = v;
}

static inline uint32_t isp_ll_get_isp_id_custom_id_value(void)
{
    isp_isp_id_custom_id_t *r = (isp_isp_id_custom_id_t *)(SOC_ISP_REG_BASE + (0x1 << 2));
    return r->v;
}

static inline void isp_ll_set_isp_id_custom_id_isp_id_custom_id(uint32_t v)
{
    isp_isp_id_custom_id_t *r = (isp_isp_id_custom_id_t *)(SOC_ISP_REG_BASE + (0x1 << 2));
    r->isp_id_custom_id = v;
}

static inline uint32_t isp_ll_get_isp_id_custom_id_isp_id_custom_id(void)
{
    isp_isp_id_custom_id_t *r = (isp_isp_id_custom_id_t *)(SOC_ISP_REG_BASE + (0x1 << 2));
    return r->isp_id_custom_id;
}

//reg timeout_cfg_stream11:

static inline void isp_ll_set_timeout_cfg_stream11_value(uint32_t v)
{
    isp_timeout_cfg_stream11_t *r = (isp_timeout_cfg_stream11_t *)(SOC_ISP_REG_BASE + (0x1075 << 2));
    r->v = v;
}

static inline uint32_t isp_ll_get_timeout_cfg_stream11_value(void)
{
    isp_timeout_cfg_stream11_t *r = (isp_timeout_cfg_stream11_t *)(SOC_ISP_REG_BASE + (0x1075 << 2));
    return r->v;
}

static inline void isp_ll_set_timeout_cfg_stream11_consumer_threshold(uint32_t v)
{
    isp_timeout_cfg_stream11_t *r = (isp_timeout_cfg_stream11_t *)(SOC_ISP_REG_BASE + (0x1075 << 2));
    r->consumer_threshold = v;
}

static inline uint32_t isp_ll_get_timeout_cfg_stream11_consumer_threshold(void)
{
    isp_timeout_cfg_stream11_t *r = (isp_timeout_cfg_stream11_t *)(SOC_ISP_REG_BASE + (0x1075 << 2));
    return r->consumer_threshold;
}

static inline void isp_ll_set_timeout_cfg_stream11_producer_threshold(uint32_t v)
{
    isp_timeout_cfg_stream11_t *r = (isp_timeout_cfg_stream11_t *)(SOC_ISP_REG_BASE + (0x1075 << 2));
    r->producer_threshold = v;
}

static inline uint32_t isp_ll_get_timeout_cfg_stream11_producer_threshold(void)
{
    isp_timeout_cfg_stream11_t *r = (isp_timeout_cfg_stream11_t *)(SOC_ISP_REG_BASE + (0x1075 << 2));
    return r->producer_threshold;
}

//reg stream_status_stream11:

static inline void isp_ll_set_stream_status_stream11_value(uint32_t v)
{
    isp_stream_status_stream11_t *r = (isp_stream_status_stream11_t *)(SOC_ISP_REG_BASE + (0x1076 << 2));
    r->v = v;
}

static inline uint32_t isp_ll_get_stream_status_stream11_value(void)
{
    isp_stream_status_stream11_t *r = (isp_stream_status_stream11_t *)(SOC_ISP_REG_BASE + (0x1076 << 2));
    return r->v;
}

static inline void isp_ll_set_stream_status_stream11_reserved_sbi(uint32_t v)
{
    isp_stream_status_stream11_t *r = (isp_stream_status_stream11_t *)(SOC_ISP_REG_BASE + (0x1076 << 2));
    r->reserved_sbi = v;
}

static inline uint32_t isp_ll_get_stream_status_stream11_reserved_sbi(void)
{
    isp_stream_status_stream11_t *r = (isp_stream_status_stream11_t *)(SOC_ISP_REG_BASE + (0x1076 << 2));
    return r->reserved_sbi;
}

static inline void isp_ll_set_stream_status_stream11_streaming_state(uint32_t v)
{
    isp_stream_status_stream11_t *r = (isp_stream_status_stream11_t *)(SOC_ISP_REG_BASE + (0x1076 << 2));
    r->streaming_state = v;
}

static inline uint32_t isp_ll_get_stream_status_stream11_streaming_state(void)
{
    isp_stream_status_stream11_t *r = (isp_stream_status_stream11_t *)(SOC_ISP_REG_BASE + (0x1076 << 2));
    return r->streaming_state;
}

static inline void isp_ll_set_stream_status_stream11_valid_entry_count(uint32_t v)
{
    isp_stream_status_stream11_t *r = (isp_stream_status_stream11_t *)(SOC_ISP_REG_BASE + (0x1076 << 2));
    r->valid_entry_count = v;
}

static inline uint32_t isp_ll_get_stream_status_stream11_valid_entry_count(void)
{
    isp_stream_status_stream11_t *r = (isp_stream_status_stream11_t *)(SOC_ISP_REG_BASE + (0x1076 << 2));
    return r->valid_entry_count;
}
#ifdef __cplusplus
}
#endif
