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
#include "h26e_hw.h"
#include "h26e_hal.h"

typedef void (*h26e_dump_fn_t)(void);
typedef struct
{
    uint32_t start;
    uint32_t end;
    h26e_dump_fn_t fn;
} h26e_reg_fn_map_t;

static void h26e_dump_sw_enc_01(void)
{
    h26e_sw_enc_01_t *r = (h26e_sw_enc_01_t *)(SOC_H26E_REG_BASE + (0x1 << 2));

    SOC_LOGI("sw_enc_01: %8x\r\n", REG_READ(SOC_H26E_REG_BASE + (0x1 << 2)));
    SOC_LOGI("	irq: %8x\r\n", r->irq);
    SOC_LOGI("	irq_dis: %8x\r\n", r->irq_dis);
    SOC_LOGI("	frame_rdy_status: %8x\r\n", r->frame_rdy_status);
    SOC_LOGI("	bus_error_status: %8x\r\n", r->bus_error_status);
    SOC_LOGI("	sw_reset: %8x\r\n", r->sw_reset);
    SOC_LOGI("	buffer_full: %8x\r\n", r->buffer_full);
    SOC_LOGI("	timeout: %8x\r\n", r->timeout);
    SOC_LOGI("	irq_line_buffer: %8x\r\n", r->irq_line_buffer);
    SOC_LOGI("	slice_rdy_status: %8x\r\n", r->slice_rdy_status);
    SOC_LOGI("	irq_fuse_error: %8x\r\n", r->irq_fuse_error);
    SOC_LOGI("	reserved_bit_10_10: %8x\r\n", r->reserved_bit_10_10);
    SOC_LOGI("	timeout_int: %8x\r\n", r->timeout_int);
    SOC_LOGI("	strm_segment_rdy_int: %8x\r\n", r->strm_segment_rdy_int);
    SOC_LOGI("	reserved_bit_13_15: %8x\r\n", r->reserved_bit_13_15);
    SOC_LOGI("	irq_type_frame_rdy: %8x\r\n", r->irq_type_frame_rdy);
    SOC_LOGI("	irq_type_slice_rdy: %8x\r\n", r->irq_type_slice_rdy);
    SOC_LOGI("	irq_type_line_buffer: %8x\r\n", r->irq_type_line_buffer);
    SOC_LOGI("	irq_type_strm_segment: %8x\r\n", r->irq_type_strm_segment);
    SOC_LOGI("	irq_type_timeout: %8x\r\n", r->irq_type_timeout);
    SOC_LOGI("	irq_type_bus_error: %8x\r\n", r->irq_type_bus_error);
    SOC_LOGI("	irq_type_buffer_full: %8x\r\n", r->irq_type_buffer_full);
    SOC_LOGI("	irq_type_fuse_error: %8x\r\n", r->irq_type_fuse_error);
    SOC_LOGI("	irq_type_sw_reset: %8x\r\n", r->irq_type_sw_reset);
    SOC_LOGI("	reserved_bit_25_31: %8x\r\n", r->reserved_bit_25_31);
}

static void h26e_dump_sw_enc_02(void)
{
    h26e_sw_enc_02_t *r = (h26e_sw_enc_02_t *)(SOC_H26E_REG_BASE + (0x2 << 2));

    SOC_LOGI("sw_enc_02: %8x\r\n", REG_READ(SOC_H26E_REG_BASE + (0x2 << 2)));
    SOC_LOGI("	ctb_rc_mem_out_swap: %8x\r\n", r->ctb_rc_mem_out_swap);
    SOC_LOGI("	roi_map_qp_delta_map_swap: %8x\r\n", r->roi_map_qp_delta_map_swap);
    SOC_LOGI("	pic_swap: %8x\r\n", r->pic_swap);
    SOC_LOGI("	strm_swap: %8x\r\n", r->strm_swap);
    SOC_LOGI("	axi_read_id: %8x\r\n", r->axi_read_id);
    SOC_LOGI("	axi_write_id: %8x\r\n", r->axi_write_id);
}

static void h26e_dump_sw_enc_03(void)
{
    h26e_sw_enc_03_t *r = (h26e_sw_enc_03_t *)(SOC_H26E_REG_BASE + (0x3 << 2));

    SOC_LOGI("sw_enc_03: %8x\r\n", REG_READ(SOC_H26E_REG_BASE + (0x3 << 2)));
    SOC_LOGI("	reserved_bit_0_0: %8x\r\n", r->reserved_bit_0_0);
    SOC_LOGI("	strm_segment_int: %8x\r\n", r->strm_segment_int);
    SOC_LOGI("	line_buffer_int: %8x\r\n", r->line_buffer_int);
    SOC_LOGI("	slice_int: %8x\r\n", r->slice_int);
    SOC_LOGI("	axi_aw_qos: %8x\r\n", r->axi_aw_qos);
    SOC_LOGI("	axi_ar_qos: %8x\r\n", r->axi_ar_qos);
    SOC_LOGI("	reserved_bit_12_18: %8x\r\n", r->reserved_bit_12_18);
    SOC_LOGI("	sram_power_down_disable: %8x\r\n", r->sram_power_down_disable);
    SOC_LOGI("	cu_info_mem_out_swap: %8x\r\n", r->cu_info_mem_out_swap);
    SOC_LOGI("	axi_rd_id_e: %8x\r\n", r->axi_rd_id_e);
    SOC_LOGI("	axi_wr_id_e: %8x\r\n", r->axi_wr_id_e);
    SOC_LOGI("	clock_gate_inter_h264_e: %8x\r\n", r->clock_gate_inter_h264_e);
    SOC_LOGI("	clock_gate_inter_h265_e: %8x\r\n", r->clock_gate_inter_h265_e);
    SOC_LOGI("	clock_gate_inter_e: %8x\r\n", r->clock_gate_inter_e);
    SOC_LOGI("	clock_gate_encoder_h264_e: %8x\r\n", r->clock_gate_encoder_h264_e);
    SOC_LOGI("	clock_gate_encoder_h265_e: %8x\r\n", r->clock_gate_encoder_h265_e);
    SOC_LOGI("	clock_gate_encoder_e: %8x\r\n", r->clock_gate_encoder_e);
}

static void h26e_dump_rsv_4_114(void)
{
    for (uint32_t idx = 0; idx < 273; idx++)
    {
        SOC_LOGI("rsv_4_114: %8x\r\n", REG_READ(SOC_H26E_REG_BASE + ((0x4 + idx) << 2)));
    }
}

static void h26e_dump_sw_enc_115(void)
{
    h26e_sw_enc_115_t *r = (h26e_sw_enc_115_t *)(SOC_H26E_REG_BASE + (0x115 << 2));

    SOC_LOGI("sw_enc_115: %8x\r\n", REG_READ(SOC_H26E_REG_BASE + (0x115 << 2)));
    SOC_LOGI("	prpsbi_id_0: %8x\r\n", r->prpsbi_id_0);
    SOC_LOGI("	syn_amount_per_loopback: %8x\r\n", r->syn_amount_per_loopback);
    SOC_LOGI("	pic_order_cnt_type: %8x\r\n", r->pic_order_cnt_type);
    SOC_LOGI("	log2_max_frame_num: %8x\r\n", r->log2_max_frame_num);
    SOC_LOGI("	log2_max_pic_order_cnt_lsb: %8x\r\n", r->log2_max_pic_order_cnt_lsb);
}

static h26e_reg_fn_map_t s_fn[] =
{
    {0x1, 0x1, h26e_dump_sw_enc_01},
    {0x2, 0x2, h26e_dump_sw_enc_02},
    {0x3, 0x3, h26e_dump_sw_enc_03},
    {0x4, 0x115, h26e_dump_rsv_4_114},
    {0x115, 0x115, h26e_dump_sw_enc_115},
    {-1, -1, 0}
};

void h26e_struct_dump(uint32_t start, uint32_t end)
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
