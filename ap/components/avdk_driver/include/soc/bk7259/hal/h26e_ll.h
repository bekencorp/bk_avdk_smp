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
#include "h26e_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define H26E_LL_REG_BASE   SOC_H26E_REG_BASE

//reg sw_enc_01:

static inline void h26e_ll_set_sw_enc_01_value(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->v = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_value(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->v;
}

static inline void h26e_ll_set_sw_enc_01_irq(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->irq = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq;
}

static inline void h26e_ll_set_sw_enc_01_irq_dis(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->irq_dis = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_dis(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_dis;
}

static inline void h26e_ll_set_sw_enc_01_frame_rdy_status(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->frame_rdy_status = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_frame_rdy_status(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->frame_rdy_status;
}

static inline uint32_t h26e_ll_get_sw_enc_01_bus_error_status(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->bus_error_status;
}

static inline void h26e_ll_set_sw_enc_01_sw_reset(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->sw_reset = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_sw_reset(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->sw_reset;
}

static inline uint32_t h26e_ll_get_sw_enc_01_buffer_full(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->buffer_full;
}

static inline void h26e_ll_set_sw_enc_01_timeout(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->timeout = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_timeout(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->timeout;
}

static inline void h26e_ll_set_sw_enc_01_irq_line_buffer(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->irq_line_buffer = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_line_buffer(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_line_buffer;
}

static inline void h26e_ll_set_sw_enc_01_slice_rdy_status(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->slice_rdy_status = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_slice_rdy_status(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->slice_rdy_status;
}

static inline void h26e_ll_set_sw_enc_01_irq_fuse_error(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->irq_fuse_error = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_fuse_error(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_fuse_error;
}

static inline void h26e_ll_set_sw_enc_01_timeout_int(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->timeout_int = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_timeout_int(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->timeout_int;
}

static inline void h26e_ll_set_sw_enc_01_strm_segment_rdy_int(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->strm_segment_rdy_int = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_strm_segment_rdy_int(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->strm_segment_rdy_int;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_type_frame_rdy(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_type_frame_rdy;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_type_slice_rdy(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_type_slice_rdy;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_type_line_buffer(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_type_line_buffer;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_type_strm_segment(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_type_strm_segment;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_type_timeout(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_type_timeout;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_type_bus_error(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_type_bus_error;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_type_buffer_full(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_type_buffer_full;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_type_fuse_error(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_type_fuse_error;
}

static inline void h26e_ll_set_sw_enc_01_irq_type_sw_reset(uint32_t v)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    r->irq_type_sw_reset = v;
}

static inline uint32_t h26e_ll_get_sw_enc_01_irq_type_sw_reset(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));
    return r->irq_type_sw_reset;
}

//reg sw_enc_02:

static inline void h26e_ll_set_sw_enc_02_value(uint32_t v)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    r->v = v;
}

static inline uint32_t h26e_ll_get_sw_enc_02_value(void)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    return r->v;
}

static inline void h26e_ll_set_sw_enc_02_ctb_rc_mem_out_swap(uint32_t v)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    r->ctb_rc_mem_out_swap = v;
}

static inline uint32_t h26e_ll_get_sw_enc_02_ctb_rc_mem_out_swap(void)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    return r->ctb_rc_mem_out_swap;
}

static inline void h26e_ll_set_sw_enc_02_roi_map_qp_delta_map_swap(uint32_t v)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    r->roi_map_qp_delta_map_swap = v;
}

static inline uint32_t h26e_ll_get_sw_enc_02_roi_map_qp_delta_map_swap(void)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    return r->roi_map_qp_delta_map_swap;
}

static inline void h26e_ll_set_sw_enc_02_pic_swap(uint32_t v)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    r->pic_swap = v;
}

static inline uint32_t h26e_ll_get_sw_enc_02_pic_swap(void)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    return r->pic_swap;
}

static inline void h26e_ll_set_sw_enc_02_strm_swap(uint32_t v)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    r->strm_swap = v;
}

static inline uint32_t h26e_ll_get_sw_enc_02_strm_swap(void)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    return r->strm_swap;
}

static inline uint32_t h26e_ll_get_sw_enc_02_axi_read_id(void)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    return r->axi_read_id;
}

static inline void h26e_ll_set_sw_enc_02_axi_write_id(uint32_t v)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    r->axi_write_id = v;
}

static inline uint32_t h26e_ll_get_sw_enc_02_axi_write_id(void)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));
    return r->axi_write_id;
}

//reg sw_enc_03:

static inline void h26e_ll_set_sw_enc_03_value(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->v = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_value(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->v;
}

static inline void h26e_ll_set_sw_enc_03_strm_segment_int(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->strm_segment_int = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_strm_segment_int(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->strm_segment_int;
}

static inline void h26e_ll_set_sw_enc_03_line_buffer_int(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->line_buffer_int = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_line_buffer_int(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->line_buffer_int;
}

static inline void h26e_ll_set_sw_enc_03_slice_int(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->slice_int = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_slice_int(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->slice_int;
}

static inline void h26e_ll_set_sw_enc_03_axi_aw_qos(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->axi_aw_qos = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_axi_aw_qos(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->axi_aw_qos;
}

static inline uint32_t h26e_ll_get_sw_enc_03_axi_ar_qos(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->axi_ar_qos;
}

static inline void h26e_ll_set_sw_enc_03_sram_power_down_disable(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->sram_power_down_disable = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_sram_power_down_disable(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->sram_power_down_disable;
}

static inline void h26e_ll_set_sw_enc_03_cu_info_mem_out_swap(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->cu_info_mem_out_swap = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_cu_info_mem_out_swap(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->cu_info_mem_out_swap;
}

static inline uint32_t h26e_ll_get_sw_enc_03_axi_rd_id_e(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->axi_rd_id_e;
}

static inline void h26e_ll_set_sw_enc_03_axi_wr_id_e(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->axi_wr_id_e = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_axi_wr_id_e(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->axi_wr_id_e;
}

static inline void h26e_ll_set_sw_enc_03_clock_gate_inter_h264_e(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->clock_gate_inter_h264_e = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_clock_gate_inter_h264_e(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->clock_gate_inter_h264_e;
}

static inline void h26e_ll_set_sw_enc_03_clock_gate_inter_h265_e(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->clock_gate_inter_h265_e = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_clock_gate_inter_h265_e(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->clock_gate_inter_h265_e;
}

static inline void h26e_ll_set_sw_enc_03_clock_gate_inter_e(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->clock_gate_inter_e = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_clock_gate_inter_e(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->clock_gate_inter_e;
}

static inline void h26e_ll_set_sw_enc_03_clock_gate_encoder_h264_e(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->clock_gate_encoder_h264_e = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_clock_gate_encoder_h264_e(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->clock_gate_encoder_h264_e;
}

static inline void h26e_ll_set_sw_enc_03_clock_gate_encoder_h265_e(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->clock_gate_encoder_h265_e = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_clock_gate_encoder_h265_e(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->clock_gate_encoder_h265_e;
}

static inline void h26e_ll_set_sw_enc_03_clock_gate_encoder_e(uint32_t v)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    r->clock_gate_encoder_e = v;
}

static inline uint32_t h26e_ll_get_sw_enc_03_clock_gate_encoder_e(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));
    return r->clock_gate_encoder_e;
}

//reg sw_enc_115:

static inline void h26e_ll_set_sw_enc_115_value(uint32_t v)
{
    h26e_sw_enc_115_t *r = (h26e_sw_enc_115_t *)(SOC_H26E_REG_BASE + (0x115 << 2));
    r->v = v;
}

static inline uint32_t h26e_ll_get_sw_enc_115_value(void)
{
    h26e_sw_enc_115_t *r = (h26e_sw_enc_115_t *)(SOC_H26E_REG_BASE + (0x115 << 2));
    return r->v;
}

static inline uint32_t h26e_ll_get_sw_enc_115_prpsbi_id_0(void)
{
    h26e_sw_enc_115_t *r = (h26e_sw_enc_115_t *)(SOC_H26E_REG_BASE + (0x115 << 2));
    return r->prpsbi_id_0;
}

static inline uint32_t h26e_ll_get_sw_enc_115_syn_amount_per_loopback(void)
{
    h26e_sw_enc_115_t *r = (h26e_sw_enc_115_t *)(SOC_H26E_REG_BASE + (0x115 << 2));
    return r->syn_amount_per_loopback;
}

static inline uint32_t h26e_ll_get_sw_enc_115_log2_max_frame_num(void)
{
    h26e_sw_enc_115_t *r = (h26e_sw_enc_115_t *)(SOC_H26E_REG_BASE + (0x115 << 2));
    return r->log2_max_frame_num;
}

static inline uint32_t h26e_ll_get_sw_enc_115_log2_max_pic_order_cnt_lsb(void)
{
    h26e_sw_enc_115_t *r = (h26e_sw_enc_115_t *)(SOC_H26E_REG_BASE + (0x115 << 2));
    return r->log2_max_pic_order_cnt_lsb;
}
#ifdef __cplusplus
}
#endif
