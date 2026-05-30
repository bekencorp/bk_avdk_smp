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
        uint32_t product_id                       : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} dpu_gcregdcproductid_t;


typedef volatile union
{
    struct
    {
        uint32_t format                           :  3; /**<bit[0 : 2] */
        uint32_t enable                           :  1; /**<bit[3 : 3] */
        uint32_t reserved_bit_4_4                 :  1; /**<bit[4 : 4] */
        uint32_t clear_en                         :  1; /**<bit[5 : 5] */
        uint32_t reserved_bit_6_9                 :  4; /**<bit[6 : 9] */
        uint32_t color_key_en                     :  1; /**<bit[10 : 10] */
        uint32_t reserved_bit_11_16               :  6; /**<bit[11 : 16] */
        uint32_t swizzle                          :  2; /**<bit[17 : 18] */
        uint32_t uv_swizzle                       :  1; /**<bit[19 : 19] */
        uint32_t reserved_bit_20_20               :  1; /**<bit[20 : 20] */
        uint32_t dec_mode                         :  3; /**<bit[21 : 23] */
        uint32_t rot_angle                        :  3; /**<bit[24 : 26] */
        uint32_t reserved_bit_27_31               :  5; /**<bit[27 : 31] */
    };
    uint32_t v;
} dpu_gcregframebufferconfig_t;


typedef volatile union
{
    struct
    {
        uint32_t address                          : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} dpu_gcregframebufferaddress_t;


typedef volatile union
{
    struct
    {
        uint32_t stride                           : 17; /**<bit[0 : 16] */
        uint32_t reserved_bit_17_31               : 15; /**<bit[17 : 31] */
    };
    uint32_t v;
} dpu_gcregframebufferstride_t;


typedef volatile union
{
    struct
    {
        uint32_t tile_format                      :  2; /**<bit[0 : 1] */
        uint32_t yuv_standard                     :  1; /**<bit[2 : 2] */
        uint32_t tile_format1                     :  2; /**<bit[3 : 4] */
        uint32_t reserved_bit_5_31                : 27; /**<bit[5 : 31] */
    };
    uint32_t v;
} dpu_gcregdctileincfg_t;


typedef volatile union
{
    struct
    {
        uint32_t address                          : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} dpu_gcregdctileuvframebufferadr_t;


typedef volatile union
{
    struct
    {
        uint32_t stride                           : 16; /**<bit[0 : 15] */
        uint32_t reserved_bit_16_31               : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} dpu_gcregdctileuvframebufferstr_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregframebufferbackground_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregframebuffercolorkey_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregframebuffercolorkeyhigh_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregframebufferclearvalue_t;


typedef volatile union
{
    struct
    {
        uint32_t x                                : 12; /**<bit[0 : 11] */
        uint32_t reserved_bit_12_15               :  4; /**<bit[12 : 15] */
        uint32_t y                                : 12; /**<bit[16 : 27] */
        uint32_t reserved_bit_28_31               :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregvideotl_t;


typedef volatile union
{
    struct
    {
        uint32_t width                            : 12; /**<bit[0 : 11] */
        uint32_t reserved_bit_12_15               :  4; /**<bit[12 : 15] */
        uint32_t height                           : 12; /**<bit[16 : 27] */
        uint32_t reserved_bit_28_31               :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregframebuffersize_t;


typedef volatile union
{
    struct
    {
        uint32_t src_alpha                        :  8; /**<bit[0 : 7] */
        uint32_t dst_alpha                        :  8; /**<bit[8 : 15] */
        uint32_t reserved_bit_16_31               : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} dpu_gcregvideoglobalalpha_t;


typedef volatile union
{
    struct
    {
        uint32_t order                            :  3; /**<bit[0 : 2] */
        uint32_t reserved_bit_3_31                : 29; /**<bit[3 : 31] */
    };
    uint32_t v;
} dpu_gcregblendstackorder_t;


typedef volatile union
{
    struct
    {
        uint32_t alpha_blend                      :  1; /**<bit[0 : 0] */
        uint32_t src_alpha_mode                   :  1; /**<bit[1 : 1] */
        uint32_t reserved_bit_2_2                 :  1; /**<bit[2 : 2] */
        uint32_t src_global_alpha_mode            :  2; /**<bit[3 : 4] */
        uint32_t reserved_bit_5_5                 :  1; /**<bit[5 : 5] */
        uint32_t src_blending_mode                :  2; /**<bit[6 : 7] */
        uint32_t src_alpha_factor                 :  1; /**<bit[8 : 8] */
        uint32_t dst_alpha_mode                   :  1; /**<bit[9 : 9] */
        uint32_t dst_global_alpha_mode            :  2; /**<bit[10 : 11] */
        uint32_t reserved_bit_12_12               :  1; /**<bit[12 : 12] */
        uint32_t dst_blending_mode                :  2; /**<bit[13 : 14] */
        uint32_t dst_alpha_factor                 :  1; /**<bit[15 : 15] */
        uint32_t reserved_bit_16_31               : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} dpu_gcregvideoalphablendconfig_t;


typedef volatile union
{
    struct
    {
        uint32_t format                           :  3; /**<bit[0 : 2] */
        uint32_t enable                           :  1; /**<bit[3 : 3] */
        uint32_t reserved_bit_4_4                 :  1; /**<bit[4 : 4] */
        uint32_t clear_en                         :  1; /**<bit[5 : 5] */
        uint32_t reserved_bit_6_16                : 11; /**<bit[6 : 16] */
        uint32_t swizzle                          :  2; /**<bit[17 : 18] */
        uint32_t uv_swizzle                       :  1; /**<bit[19 : 19] */
        uint32_t color_key_en                     :  1; /**<bit[20 : 20] */
        uint32_t dec_mode                         :  3; /**<bit[21 : 23] */
        uint32_t rot_angle                        :  3; /**<bit[24 : 26] */
        uint32_t reserved_bit_27_31               :  5; /**<bit[27 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayconfig_t;


typedef volatile union
{
    struct
    {
        uint32_t address                          : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayaddress_t;


typedef volatile union
{
    struct
    {
        uint32_t stride                           : 17; /**<bit[0 : 16] */
        uint32_t reserved_bit_17_31               : 15; /**<bit[17 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaystride_t;


typedef volatile union
{
    struct
    {
        uint32_t tile_format                      :  2; /**<bit[0 : 1] */
        uint32_t yuv_standard                     :  1; /**<bit[2 : 2] */
        uint32_t tile_format1                     :  2; /**<bit[3 : 4] */
        uint32_t reserved_bit_5_31                : 27; /**<bit[5 : 31] */
    };
    uint32_t v;
} dpu_gcregdcoverlaytileincfg_t;


typedef volatile union
{
    struct
    {
        uint32_t address                          : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} dpu_gcregdctileuvoverlayadr_t;


typedef volatile union
{
    struct
    {
        uint32_t stride                           : 16; /**<bit[0 : 15] */
        uint32_t reserved_bit_16_31               : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} dpu_gcregdctileuvoverlaystr_t;


typedef volatile union
{
    struct
    {
        uint32_t x                                : 12; /**<bit[0 : 11] */
        uint32_t reserved_bit_12_15               :  4; /**<bit[12 : 15] */
        uint32_t y                                : 12; /**<bit[16 : 27] */
        uint32_t reserved_bit_28_31               :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaytl_t;


typedef volatile union
{
    struct
    {
        uint32_t width                            : 12; /**<bit[0 : 11] */
        uint32_t reserved_bit_12_15               :  4; /**<bit[12 : 15] */
        uint32_t height                           : 12; /**<bit[16 : 27] */
        uint32_t reserved_bit_28_31               :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaysize_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaycolorkey_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaycolorkeyhigh_t;


typedef volatile union
{
    struct
    {
        uint32_t alpha_blend                      :  1; /**<bit[0 : 0] */
        uint32_t src_alpha_mode                   :  1; /**<bit[1 : 1] */
        uint32_t reserved_bit_2_2                 :  1; /**<bit[2 : 2] */
        uint32_t src_global_alpha_mode            :  2; /**<bit[3 : 4] */
        uint32_t reserved_bit_5_5                 :  1; /**<bit[5 : 5] */
        uint32_t src_blending_mode                :  2; /**<bit[6 : 7] */
        uint32_t src_alpha_factor                 :  1; /**<bit[8 : 8] */
        uint32_t dst_alpha_mode                   :  1; /**<bit[9 : 9] */
        uint32_t dst_global_alpha_mode                   :  2; /**<bit[10 : 11] */
        uint32_t reserved_bit_12_12               :  1; /**<bit[12 : 12] */
        uint32_t dst_blending_mode                :  2; /**<bit[13 : 14] */
        uint32_t dst_alpha_factor                 :  1; /**<bit[15 : 15] */
        uint32_t reserved_bit_16_31               : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayalphablendconfig_t;


typedef volatile union
{
    struct
    {
        uint32_t src_alpha                        :  8; /**<bit[0 : 7] */
        uint32_t dst_alpha                        :  8; /**<bit[8 : 15] */
        uint32_t reserved_bit_16_31               : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayglobalalpha_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayclearvalue_t;


typedef volatile union
{
    struct
    {
        uint32_t format                           :  3; /**<bit[0 : 2] */
        uint32_t enable                           :  1; /**<bit[3 : 3] */
        uint32_t reserved_bit_4_4                 :  1; /**<bit[4 : 4] */
        uint32_t clear_en                         :  1; /**<bit[5 : 5] */
        uint32_t reserved_bit_6_16                : 11; /**<bit[6 : 16] */
        uint32_t swizzle                          :  2; /**<bit[17 : 18] */
        uint32_t reserved_bit_19_19               :  1; /**<bit[19 : 19] */
        uint32_t color_key_en                     :  1; /**<bit[20 : 20] */
        uint32_t dec_mode                         :  3; /**<bit[21 : 23] */
        uint32_t rot_angle                        :  3; /**<bit[24 : 26] */
        uint32_t reserved_bit_27_31               :  5; /**<bit[27 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayconfig1_t;


typedef volatile union
{
    struct
    {
        uint32_t address                          : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayaddress1_t;


typedef volatile union
{
    struct
    {
        uint32_t stride                           : 17; /**<bit[0 : 16] */
        uint32_t reserved_bit_17_31               : 15; /**<bit[17 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaystride1_t;


typedef volatile union
{
    struct
    {
        uint32_t x                                : 12; /**<bit[0 : 11] */
        uint32_t reserved_bit_12_15               :  4; /**<bit[12 : 15] */
        uint32_t y                                : 12; /**<bit[16 : 27] */
        uint32_t reserved_bit_28_31               :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaytl1_t;


typedef volatile union
{
    struct
    {
        uint32_t width                            : 12; /**<bit[0 : 11] */
        uint32_t reserved_bit_12_15               :  4; /**<bit[12 : 15] */
        uint32_t height                           : 12; /**<bit[16 : 27] */
        uint32_t reserved_bit_28_31               :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaysize1_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaycolorkey1_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlaycolorkeyhigh1_t;


typedef volatile union
{
    struct
    {
        uint32_t alpha_blend                      :  1; /**<bit[0 : 0] */
        uint32_t src_alpha_mode                   :  1; /**<bit[1 : 1] */
        uint32_t reserved_bit_2_2                 :  1; /**<bit[2 : 2] */
        uint32_t src_global_alpha_mode            :  2; /**<bit[3 : 4] */
        uint32_t reserved_bit_5_5                 :  1; /**<bit[5 : 5] */
        uint32_t src_blending_mode                :  2; /**<bit[6 : 7] */
        uint32_t src_alpha_factor                 :  1; /**<bit[8 : 8] */
        uint32_t dst_alpha_mode                   :  1; /**<bit[9 : 9] */
        uint32_t dst_global_alpha_mode                   :  2; /**<bit[10 : 11] */
        uint32_t reserved_bit_12_12               :  1; /**<bit[12 : 12] */
        uint32_t dst_blending_mode                :  2; /**<bit[13 : 14] */
        uint32_t dst_alpha_factor                 :  1; /**<bit[15 : 15] */
        uint32_t reserved_bit_16_31               : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayalphablendconfig1_t;


typedef volatile union
{
    struct
    {
        uint32_t src_alpha                        :  8; /**<bit[0 : 7] */
        uint32_t dst_alpha                        :  8; /**<bit[8 : 15] */
        uint32_t reserved_bit_16_31               : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayglobalalpha1_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t alpha                            :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregoverlayclearvalue1_t;


typedef volatile union
{
    struct
    {
        uint32_t y0_x0                            :  4; /**<bit[0 : 3] */
        uint32_t y0_x1                            :  4; /**<bit[4 : 7] */
        uint32_t y0_x2                            :  4; /**<bit[8 : 11] */
        uint32_t y0_x3                            :  4; /**<bit[12 : 15] */
        uint32_t y1_x0                            :  4; /**<bit[16 : 19] */
        uint32_t y1_x1                            :  4; /**<bit[20 : 23] */
        uint32_t y1_x2                            :  4; /**<bit[24 : 27] */
        uint32_t y1_x3                            :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregdisplaydithertablelow_t;


typedef volatile union
{
    struct
    {
        uint32_t y2_x0                            :  4; /**<bit[0 : 3] */
        uint32_t y2_x1                            :  4; /**<bit[4 : 7] */
        uint32_t y2_x2                            :  4; /**<bit[8 : 11] */
        uint32_t y2_x3                            :  4; /**<bit[12 : 15] */
        uint32_t y3_x0                            :  4; /**<bit[16 : 19] */
        uint32_t y3_x1                            :  4; /**<bit[20 : 23] */
        uint32_t y3_x2                            :  4; /**<bit[24 : 27] */
        uint32_t y3_x3                            :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregdisplaydithertablehigh_t;


typedef volatile union
{
    struct
    {
        uint32_t de                               :  1; /**<bit[0 : 0] */
        uint32_t de_polarity                      :  1; /**<bit[1 : 1] */
        uint32_t reserved_bit_2_4                 :  3; /**<bit[2 : 4] */
        uint32_t data_polarity                    :  1; /**<bit[5 : 5] */
        uint32_t reserved_bit_6_7                 :  2; /**<bit[6 : 7] */
        uint32_t clock                            :  1; /**<bit[8 : 8] */
        uint32_t clock_polarity                   :  1; /**<bit[9 : 9] */
        uint32_t reserved_bit_10_31               : 22; /**<bit[10 : 31] */
    };
    uint32_t v;
} dpu_gcregpanelconfig_t;


typedef volatile union
{
    struct
    {
        uint32_t valid                            :  1; /**<bit[0 : 0] */
        uint32_t back_pressure_disab              :  1; /**<bit[1 : 1] */
        uint32_t reserved_bit_2_31                : 30; /**<bit[2 : 31] */
    };
    uint32_t v;
} dpu_gcregpanelcontrol_t;


typedef volatile union
{
    struct
    {
        uint32_t output                           :  1; /**<bit[0 : 0] */
        uint32_t gamma                            :  1; /**<bit[1 : 1] */
        uint32_t dither                           :  1; /**<bit[2 : 2] */
        uint32_t reserved_bit_3_31                : 29; /**<bit[3 : 31] */
    };
    uint32_t v;
} dpu_gcregpanelfunction_t;


typedef volatile union
{
    struct
    {
        uint32_t working                          :  1; /**<bit[0 : 0] */
        uint32_t reserved_bit_1_31                : 31; /**<bit[1 : 31] */
    };
    uint32_t v;
} dpu_gcregpanelworking_t;


typedef volatile union
{
    struct
    {
        uint32_t reserved_bit_0_0                 :  1; /**<bit[0 : 0] */
        uint32_t video_under_flow                 :  1; /**<bit[1 : 1] */
        uint32_t overlay_under_flow               :  1; /**<bit[2 : 2] */
        uint32_t overlay_under_flow1              :  1; /**<bit[3 : 3] */
        uint32_t reserved_bit_4_31                : 28; /**<bit[4 : 31] */
    };
    uint32_t v;
} dpu_gcregpanelstate_t;


typedef volatile union
{
    struct
    {
        uint32_t display_end                      : 13; /**<bit[0 : 12] */
        uint32_t reserved_bit_13_15               :  3; /**<bit[13 : 15] */
        uint32_t total                            : 13; /**<bit[16 : 28] */
        uint32_t reserved_bit_29_31               :  3; /**<bit[29 : 31] */
    };
    uint32_t v;
} dpu_gcreghdisplay_t;


typedef volatile union
{
    struct
    {
        uint32_t start                            : 13; /**<bit[0 : 12] */
        uint32_t reserved_bit_13_15               :  3; /**<bit[13 : 15] */
        uint32_t end                              : 13; /**<bit[16 : 28] */
        uint32_t reserved_bit_29_29               :  1; /**<bit[29 : 29] */
        uint32_t pulse                            :  1; /**<bit[30 : 30] */
        uint32_t polarity                         :  1; /**<bit[31 : 31] */
    };
    uint32_t v;
} dpu_gcreghsync_t;


typedef volatile union
{
    struct
    {
        uint32_t display_end                      : 12; /**<bit[0 : 11] */
        uint32_t reserved_bit_12_15               :  4; /**<bit[12 : 15] */
        uint32_t total                            : 12; /**<bit[16 : 27] */
        uint32_t reserved_bit_28_31               :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregvdisplay_t;


typedef volatile union
{
    struct
    {
        uint32_t start                            : 12; /**<bit[0 : 11] */
        uint32_t reserved_bit_12_15               :  4; /**<bit[12 : 15] */
        uint32_t end                              : 12; /**<bit[16 : 27] */
        uint32_t reserved_bit_28_29               :  2; /**<bit[28 : 29] */
        uint32_t pulse                            :  1; /**<bit[30 : 30] */
        uint32_t
        polarity                        :  1; /**<bit[31 : 31] */
    };
    uint32_t v;
} dpu_gcregvsync_t;


typedef volatile union
{
    struct
    {
        uint32_t x                                : 16; /**<bit[0 : 15] */
        uint32_t y                                : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} dpu_gcregdisplaycurrentlocation_t;


typedef volatile union
{
    struct
    {
        uint32_t index                            :  8; /**<bit[0 : 7] */
        uint32_t reserved_bit_8_31                : 24; /**<bit[8 : 31] */
    };
    uint32_t v;
} dpu_gcreggammaindex_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t reserved_bit_24_31               :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcreggammadata_t;


typedef volatile union
{
    struct
    {
        uint32_t format                           :  2; /**<bit[0 : 1] */
        uint32_t reserved_bit_2_7                 :  6; /**<bit[2 : 7] */
        uint32_t hot_spot_y                       :  5; /**<bit[8 : 12] */
        uint32_t reserved_bit_13_15               :  3; /**<bit[13 : 15] */
        uint32_t hot_spot_x                       :  5; /**<bit[16 : 20] */
        uint32_t reserved_bit_21_31               : 11; /**<bit[21 : 31] */
    };
    uint32_t v;
} dpu_gcregcursorconfig_t;


typedef volatile union
{
    struct
    {
        uint32_t address                          : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} dpu_gcregcursoraddress_t;


typedef volatile union
{
    struct
    {
        uint32_t x                                : 13; /**<bit[0 : 12] */
        uint32_t reserved_bit_13_15               :  3; /**<bit[13 : 15] */
        uint32_t y                                : 12; /**<bit[16 : 27] */
        uint32_t reserved_bit_28_31               :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} dpu_gcregcursorlocation_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t reserved_bit_24_31               :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregcursorbackground_t;


typedef volatile union
{
    struct
    {
        uint32_t blue                             :  8; /**<bit[0 : 7] */
        uint32_t green                            :  8; /**<bit[8 : 15] */
        uint32_t red                              :  8; /**<bit[16 : 23] */
        uint32_t reserved_bit_24_31               :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} dpu_gcregcursorforeground_t;


typedef volatile union
{
    struct
    {
        uint32_t disp0                            :  1; /**<bit[0 : 0] */
        uint32_t reserved_bit_1_11                : 11; /**<bit[1 : 11] */
        uint32_t disp0_dbi_cfg_error              :  1; /**<bit[12 : 12] */
        uint32_t reserved_bit_13_28               : 16; /**<bit[13 : 28] */
        uint32_t panel_underflow                  :  1; /**<bit[29 : 29] */
        uint32_t soft_reset_done                  :  1; /**<bit[30 : 30] */
        uint32_t bus_error                        :  1; /**<bit[31 : 31] */
    };
    uint32_t v;
} dpu_gcregdisplayintr_t;


typedef volatile union
{
    struct
    {
        uint32_t disp0                            :  1; /**<bit[0 : 0] */
        uint32_t reserved_bit_1_11                : 11; /**<bit[1 : 11] */
        uint32_t disp0_dbi_cfg_error              :  1; /**<bit[12 : 12] */
        uint32_t reserved_bit_13_28               : 16; /**<bit[13 : 28] */
        uint32_t panel_underflow                  :  1; /**<bit[29 : 29] */
        uint32_t soft_reset_done                  :  1; /**<bit[30 : 30] */
        uint32_t bus_error                        :  1; /**<bit[31 : 31] */
    };
    uint32_t v;
} dpu_gcregdisplayintrenable_t;

typedef volatile struct
{
    volatile dpu_gcregdcproductid_t gcregdcproductid;
    volatile uint32_t rsv_804_808[5];
    volatile dpu_gcregframebufferconfig_t gcregframebufferconfig;
    volatile dpu_gcregframebufferaddress_t gcregframebufferaddress;
    volatile dpu_gcregframebufferstride_t gcregframebufferstride;
    volatile uint32_t rsv_80c_80c[1];
    volatile dpu_gcregdctileincfg_t gcregdctileincfg;
    volatile dpu_gcregdctileuvframebufferadr_t gcregdctileuvframebufferadr;
    volatile dpu_gcregdctileuvframebufferstr_t gcregdctileuvframebufferstr;
    volatile uint32_t rsv_810_815[6];
    volatile dpu_gcregframebufferbackground_t gcregframebufferbackground;
    volatile dpu_gcregframebuffercolorkey_t gcregframebuffercolorkey;
    volatile dpu_gcregframebuffercolorkeyhigh_t gcregframebuffercolorkeyhigh;
    volatile dpu_gcregframebufferclearvalue_t gcregframebufferclearvalue;
    volatile dpu_gcregvideotl_t gcregvideotl;
    volatile dpu_gcregframebuffersize_t gcregframebuffersize;
    volatile dpu_gcregvideoglobalalpha_t gcregvideoglobalalpha;
    volatile dpu_gcregblendstackorder_t gcregblendstackorder;
    volatile dpu_gcregvideoalphablendconfig_t gcregvideoalphablendconfig;
    volatile dpu_gcregoverlayconfig_t gcregoverlayconfig;
    volatile dpu_gcregoverlayaddress_t gcregoverlayaddress;
    volatile dpu_gcregoverlaystride_t gcregoverlaystride;
    volatile dpu_gcregdcoverlaytileincfg_t gcregdcoverlaytileincfg;
    volatile dpu_gcregdctileuvoverlayadr_t gcregdctileuvoverlayadr;
    volatile dpu_gcregdctileuvoverlaystr_t gcregdctileuvoverlaystr;
    volatile dpu_gcregoverlaytl_t gcregoverlaytl;
    volatile dpu_gcregoverlaysize_t gcregoverlaysize;
    volatile dpu_gcregoverlaycolorkey_t gcregoverlaycolorkey;
    volatile dpu_gcregoverlaycolorkeyhigh_t gcregoverlaycolorkeyhigh;
    volatile dpu_gcregoverlayalphablendconfig_t gcregoverlayalphablendconfig;
    volatile dpu_gcregoverlayglobalalpha_t gcregoverlayglobalalpha;
    volatile dpu_gcregoverlayclearvalue_t gcregoverlayclearvalue;
    volatile dpu_gcregoverlayconfig1_t gcregoverlayconfig1;
    volatile dpu_gcregoverlayaddress1_t gcregoverlayaddress1;
    volatile dpu_gcregoverlaystride1_t gcregoverlaystride1;
    volatile dpu_gcregoverlaytl1_t gcregoverlaytl1;
    volatile dpu_gcregoverlaysize1_t gcregoverlaysize1;
    volatile dpu_gcregoverlaycolorkey1_t gcregoverlaycolorkey1;
    volatile dpu_gcregoverlaycolorkeyhigh1_t gcregoverlaycolorkeyhigh1;
    volatile dpu_gcregoverlayalphablendconfig1_t gcregoverlayalphablendconfig1;
    volatile dpu_gcregoverlayglobalalpha1_t gcregoverlayglobalalpha1;
    volatile dpu_gcregoverlayclearvalue1_t gcregoverlayclearvalue1;
    volatile uint32_t rsv_836_837[2];
    volatile dpu_gcregdisplaydithertablelow_t gcregdisplaydithertablelow;
    volatile dpu_gcregdisplaydithertablehigh_t gcregdisplaydithertablehigh;
    volatile dpu_gcregpanelconfig_t gcregpanelconfig;
    volatile dpu_gcregpanelcontrol_t gcregpanelcontrol;
    volatile dpu_gcregpanelfunction_t gcregpanelfunction;
    volatile dpu_gcregpanelworking_t gcregpanelworking;
    volatile dpu_gcregpanelstate_t gcregpanelstate;
    volatile uint32_t rsv_83f_83f[1];
    volatile dpu_gcreghdisplay_t gcreghdisplay;
    volatile dpu_gcreghsync_t gcreghsync;
    volatile uint32_t rsv_842_843[2];
    volatile dpu_gcregvdisplay_t gcregvdisplay;
    volatile dpu_gcregvsync_t gcregvsync;
    volatile dpu_gcregdisplaycurrentlocation_t gcregdisplaycurrentlocation;
    volatile dpu_gcreggammaindex_t gcreggammaindex;
    volatile dpu_gcreggammadata_t gcreggammadata;
    volatile dpu_gcregcursorconfig_t gcregcursorconfig;
    volatile dpu_gcregcursoraddress_t gcregcursoraddress;
    volatile dpu_gcregcursorlocation_t gcregcursorlocation;
    volatile dpu_gcregcursorbackground_t gcregcursorbackground;
    volatile dpu_gcregcursorforeground_t gcregcursorforeground;
    volatile dpu_gcregdisplayintr_t gcregdisplayintr;
    volatile dpu_gcregdisplayintrenable_t gcregdisplayintrenable;
} dpu_hw_t;

#ifdef __cplusplus
}
#endif
