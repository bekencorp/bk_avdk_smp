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
        uint32_t irq                              :  1; /**<bit[0 : 0] */
        uint32_t irq_dis                          :  1; /**<bit[1 : 1] */
        uint32_t frame_rdy_status                 :  1; /**<bit[2 : 2] */
        uint32_t bus_error_status                 :  1; /**<bit[3 : 3] */
        uint32_t sw_reset                         :  1; /**<bit[4 : 4] */
        uint32_t buffer_full                      :  1; /**<bit[5 : 5] */
        uint32_t timeout                          :  1; /**<bit[6 : 6] */
        uint32_t irq_line_buffer                  :  1; /**<bit[7 : 7] */
        uint32_t slice_rdy_status                 :  1; /**<bit[8 : 8] */
        uint32_t irq_fuse_error                   :  1; /**<bit[9 : 9] */
        uint32_t reserved_bit_10_10               :  1; /**<bit[10 : 10] */
        uint32_t timeout_int                      :  1; /**<bit[11 : 11] */
        uint32_t strm_segment_rdy_int             :  1; /**<bit[12 : 12] */
        uint32_t reserved_bit_13_15               :  3; /**<bit[13 : 15] */
        uint32_t irq_type_frame_rdy               :  1; /**<bit[16 : 16] */
        uint32_t irq_type_slice_rdy               :  1; /**<bit[17 : 17] */
        uint32_t irq_type_line_buffer             :  1; /**<bit[18 : 18] */
        uint32_t irq_type_strm_segment            :  1; /**<bit[19 : 19] */
        uint32_t irq_type_timeout                 :  1; /**<bit[20 : 20] */
        uint32_t irq_type_bus_error               :  1; /**<bit[21 : 21] */
        uint32_t irq_type_buffer_full             :  1; /**<bit[22 : 22] */
        uint32_t irq_type_fuse_error              :  1; /**<bit[23 : 23] */
        uint32_t irq_type_sw_reset                :  1; /**<bit[24 : 24] */
        uint32_t reserved_bit_25_31               :  7; /**<bit[25 : 31] */
    };
    uint32_t v;
} h26e_sw_enc_01_t;


typedef volatile union
{
    struct
    {
        uint32_t ctb_rc_mem_out_swap              :  4; /**<bit[0 : 3] */
        uint32_t roi_map_qp_delta_map_swap        :  4; /**<bit[4 : 7] */
        uint32_t pic_swap                         :  4; /**<bit[8 : 11] */
        uint32_t strm_swap                        :  4; /**<bit[12 : 15] */
        uint32_t axi_read_id                      :  8; /**<bit[16 : 23] */
        uint32_t axi_write_id                     :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} h26e_sw_enc_02_t;


typedef volatile union
{
    struct
    {
        uint32_t reserved_bit_0_0                 :  1; /**<bit[0 : 0] */
        uint32_t strm_segment_int                 :  1; /**<bit[1 : 1] */
        uint32_t line_buffer_int                  :  1; /**<bit[2 : 2] */
        uint32_t slice_int                        :  1; /**<bit[3 : 3] */
        uint32_t axi_aw_qos                       :  4; /**<bit[4 : 7] */
        uint32_t axi_ar_qos                       :  4; /**<bit[8 : 11] */
        uint32_t reserved_bit_12_18               :  7; /**<bit[12 : 18] */
        uint32_t sram_power_down_disable          :  1; /**<bit[19 : 19] */
        uint32_t cu_info_mem_out_swap             :  4; /**<bit[20 : 23] */
        uint32_t axi_rd_id_e                      :  1; /**<bit[24 : 24] */
        uint32_t axi_wr_id_e                      :  1; /**<bit[25 : 25] */
        uint32_t clock_gate_inter_h264_e          :  1; /**<bit[26 : 26] */
        uint32_t clock_gate_inter_h265_e          :  1; /**<bit[27 : 27] */
        uint32_t clock_gate_inter_e               :  1; /**<bit[28 : 28] */
        uint32_t clock_gate_encoder_h264_e        :  1; /**<bit[29 : 29] */
        uint32_t clock_gate_encoder_h265_e        :  1; /**<bit[30 : 30] */
        uint32_t clock_gate_encoder_e             :  1; /**<bit[31 : 31] */
    };
    uint32_t v;
} h26e_sw_enc_03_t;


typedef volatile union
{
    struct
    {
        uint32_t prpsbi_id_0                      :  5; /**<bit[0 : 4] */
        uint32_t syn_amount_per_loopback          : 15; /**<bit[5 : 19] */
        uint32_t pic_order_cnt_type               :  2; /**<bit[20 : 21] */
        uint32_t log2_max_frame_num               :  5; /**<bit[22 : 26] */
        uint32_t log2_max_pic_order_cnt_lsb       :  5; /**<bit[27 : 31] */
    };
    uint32_t v;
} h26e_sw_enc_115_t;

typedef volatile struct
{
    volatile h26e_sw_enc_01_t sw_enc_01;
    volatile h26e_sw_enc_02_t sw_enc_02;
    volatile h26e_sw_enc_03_t sw_enc_03;
    volatile uint32_t rsv_4_114[273];
    volatile h26e_sw_enc_115_t sw_enc_115;
} h26e_hw_t;

#ifdef __cplusplus
}
#endif
