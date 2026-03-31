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
        uint32_t clk3ddis                         :  1; /**<bit[0 : 0] */
        uint32_t clk2ddis                         :  1; /**<bit[1 : 1] */
        uint32_t fscale_val                       :  7; /**<bit[2 : 8] */
        uint32_t fscale_cmd_load                  :  1; /**<bit[9 : 9] */
        uint32_t dis_ram_clk_gating               :  1; /**<bit[10 : 10] */
        uint32_t dis_dbg_register                 :  1; /**<bit[11 : 11] */
        uint32_t soft_rst                         :  1; /**<bit[12 : 12] */
        uint32_t reserved_13_15                   :  3; /**<bit[13 : 15] */
        uint32_t idle3_d                          :  1; /**<bit[16 : 16] */
        uint32_t idle2_d                          :  1; /**<bit[17 : 17] */
        uint32_t idle_vg                          :  1; /**<bit[18 : 18] */
        uint32_t isolategpu                       :  1; /**<bit[19 : 19] */
        uint32_t reserved_bit_20_31               : 12; /**<bit[20 : 31] */
    };
    uint32_t v;
} gpu_aqhiclkctrl_t;


typedef volatile union
{
    struct
    {
        uint32_t idle_fe                          :  1; /**<bit[0 : 0] */
        uint32_t idle_de                          :  1; /**<bit[1 : 1] */
        uint32_t idle_pe                          :  1; /**<bit[2 : 2] */
        uint32_t idle_sh                          :  1; /**<bit[3 : 3] */
        uint32_t idle_pa                          :  1; /**<bit[4 : 4] */
        uint32_t idle_se                          :  1; /**<bit[5 : 5] */
        uint32_t idle_ra                          :  1; /**<bit[6 : 6] */
        uint32_t idle_tx                          :  1; /**<bit[7 : 7] */
        uint32_t idle_vg                          :  1; /**<bit[8 : 8] */
        uint32_t idle_im                          :  1; /**<bit[9 : 9] */
        uint32_t idle_fp                          :  1; /**<bit[10 : 10] */
        uint32_t idle_ts                          :  1; /**<bit[11 : 11] */
        uint32_t idle_blt                         :  1; /**<bit[12 : 12] */
        uint32_t reserved_13_30                   : 18; /**<bit[13 : 30] */
        uint32_t axi_lp                           :  1; /**<bit[31 : 31] */
    };
    uint32_t v;
} gpu_aqhiidlereg_t;


typedef volatile union
{
    struct
    {
        uint32_t reserved_0_7                     :  8; /**<bit[0 : 7] */
        uint32_t awcache                          :  4; /**<bit[8 : 11] */
        uint32_t arcache                          :  4; /**<bit[12 : 15] */
        uint32_t axdomain_shared                  :  2; /**<bit[16 : 17] */
        uint32_t axdomain_noshared                :  2; /**<bit[18 : 19] */
        uint32_t axcache_override_shared          :  4; /**<bit[20 : 23] */
        uint32_t reserved_bit_24_31               :  8; /**<bit[24 : 31] */
    };
    uint32_t v;
} gpu_aqaxiconfig_t;


typedef volatile union
{
    struct
    {
        uint32_t wr_err_id                        :  4; /**<bit[0 : 3] */
        uint32_t rd_err_id                        :  4; /**<bit[4 : 7] */
        uint32_t det_wr_err                       :  1; /**<bit[8 : 8] */
        uint32_t det_rd_err                       :  1; /**<bit[9 : 9] */
        uint32_t reserved_bit_10_31               : 22; /**<bit[10 : 31] */
    };
    uint32_t v;
} gpu__t;


typedef volatile union
{
    struct
    {
        uint32_t intr_vec                         : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_aqintrack_t;


typedef volatile union
{
    struct
    {
        uint32_t intr_enbl_vec                    : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_aqintren_t;


typedef volatile union
{
    struct
    {
        uint32_t aqident                          : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_aqidentreg_t;


typedef volatile union
{
    struct
    {
        uint32_t featureserved                    : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_gcfeatureserved_t;


typedef volatile union
{
    struct
    {
        uint32_t gcchipid                         : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_gcchipidreg_t;


typedef volatile union
{
    struct
    {
        uint32_t rev                              : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_gcchiprev_t;


typedef volatile union
{
    struct
    {
        uint32_t date                             : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_gcchipdate_t;


typedef volatile union
{
    struct
    {
        uint32_t chiptime                         : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_gcchiptime_t;


typedef volatile union
{
    struct
    {
        uint32_t chipcustomer                     : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_gcchipcustomer_t;


typedef volatile union
{
    struct
    {
        uint32_t minorfeatureserved               : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_gcminorfeatureserved_t;


typedef volatile union
{
    struct
    {
        uint32_t reservedetmem                    : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_gcreservedetmem_t;


typedef volatile union
{
    struct
    {
        uint32_t patch_rev                        : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_gcreghichippatchrev_t;


typedef volatile union
{
    struct
    {
        uint32_t grade_level                      :  4; /**<bit[0 : 3] */
        uint32_t num                              : 20; /**<bit[4 : 23] */
        uint32_t type                             :  4; /**<bit[24 : 27] */
        uint32_t reserved_bit_28_31               :  4; /**<bit[28 : 31] */
    };
    uint32_t v;
} gpu_gcproductid_t;


typedef volatile union
{
    struct
    {
        uint32_t en_module_clk_gating             :  1; /**<bit[0 : 0] */
        uint32_t dis_stall_module_clk_gating      :  1; /**<bit[1 : 1] */
        uint32_t dis_starve_module_clk_gating     :  1; /**<bit[2 : 2] */
        uint32_t reserved_bit_3_3                 :  1; /**<bit[3 : 3] */
        uint32_t turn_on_counter                  :  4; /**<bit[4 : 7] */
        uint32_t reserved_bit_8_15                :  8; /**<bit[8 : 15] */
        uint32_t turn_off_conuter                 : 16; /**<bit[16 : 31] */
    };
    uint32_t v;
} gpu_modulepowerctrl_t;


typedef volatile union
{
    struct
    {
        uint32_t dis_clk_gating_fe                :  1; /**<bit[0 : 0] */
        uint32_t reserved_bit_1_1                 :  1; /**<bit[1 : 1] */
        uint32_t dis_clk_gating_pe                :  1; /**<bit[2 : 2] */
        uint32_t reserved_bit_3_7                 :  5; /**<bit[3 : 7] */
        uint32_t dis_clk_gating_vg                :  1; /**<bit[8 : 8] */
        uint32_t dis_clk_gating_im                :  1; /**<bit[9 : 9] */
        uint32_t reserved_bit_10_10               :  1; /**<bit[10 : 10] */
        uint32_t dis_clk_gating_ts                :  1; /**<bit[11 : 11] */
        uint32_t reserved_bit_12_31               : 20; /**<bit[12 : 31] */
    };
    uint32_t v;
} gpu_powermodulectrl_t;


typedef volatile union
{
    struct
    {
        uint32_t clk_gating_fe_s                  :  1; /**<bit[0 : 0] */
        uint32_t reserved_bit_1_1                 :  1; /**<bit[1 : 1] */
        uint32_t clk_gating_pe_s                  :  1; /**<bit[2 : 2] */
        uint32_t reserved_bit_3_7                 :  5; /**<bit[3 : 7] */
        uint32_t clk_gating_vg_s                  :  1; /**<bit[8 : 8] */
        uint32_t clk_gating_im_s                  :  1; /**<bit[9 : 9] */
        uint32_t reserved_bit_10_10               :  1; /**<bit[10 : 10] */
        uint32_t clk_gating_ts_s                  :  1; /**<bit[11 : 11] */
        uint32_t clk_gating_flexa_s               :  1; /**<bit[12 : 12] */
        uint32_t reserved_bit_13_31               : 19; /**<bit[13 : 31] */
    };
    uint32_t v;
} gpu_powermodulestatus_t;


typedef volatile union
{
    struct
    {
        uint32_t max_outstanding_reads            :  8; /**<bit[0 : 7] */
        uint32_t reserved_bit_8_31                : 24; /**<bit[8 : 31] */
    };
    uint32_t v;
} gpu_aqmemorydebug_t;


typedef volatile union
{
    struct
    {
        uint32_t for_rf1p                         :  8; /**<bit[0 : 7] */
        uint32_t for_rf2p                         :  8; /**<bit[8 : 15] */
        uint32_t fast_rtc                         :  2; /**<bit[16 : 17] */
        uint32_t fast_wtc                         :  2; /**<bit[18 : 19] */
        uint32_t power_down                       :  1; /**<bit[20 : 20] */
        uint32_t reserved_bit_21_31               : 11; /**<bit[21 : 31] */
    };
    uint32_t v;
} gpu_aqregtimingctrl_t;


typedef volatile union
{
    struct
    {
        uint32_t type                             :  2; /**<bit[0 : 1] */
        uint32_t addreserveds                     : 30; /**<bit[2 : 31] */
    };
    uint32_t v;
} gpu_fetchaddreserveds_t;


typedef volatile union
{
    struct
    {
        uint32_t count                            : 21; /**<bit[0 : 20] */
        uint32_t reserved_bit_21_31               : 11; /**<bit[21 : 31] */
    };
    uint32_t v;
} gpu_fetchctrl_t;


typedef volatile union
{
    struct
    {
        uint32_t addreserveds                     : 32; /**<bit[0 : 31] */
    };
    uint32_t v;
} gpu_currentfetchaddr_t;

typedef volatile struct
{
    volatile gpu_aqhiclkctrl_t aqhiclkctrl;
    volatile gpu_aqhiidlereg_t aqhiidlereg;
    volatile gpu_aqaxiconfig_t aqaxiconfig;
    volatile gpu__t ;
    volatile gpu_aqintrack_t aqintrack;
    volatile gpu_aqintren_t aqintren;
    volatile gpu_aqidentreg_t aqidentreg;
    volatile gpu_gcfeatureserved_t gcfeatureserved;
    volatile gpu_gcchipidreg_t gcchipidreg;
    volatile gpu_gcchiprev_t gcchiprev;
    volatile gpu_gcchipdate_t gcchipdate;
    volatile gpu_gcchiptime_t gcchiptime;
    volatile gpu_gcchipcustomer_t gcchipcustomer;
    volatile gpu_gcminorfeatureserved_t gcminorfeatureserved;
    volatile uint32_t rsv_e_e[1];
    volatile gpu_gcreservedetmem_t gcreservedetmem;
    volatile uint32_t rsv_10_25[22];
    volatile gpu_gcreghichippatchrev_t gcreghichippatchrev;
    volatile uint32_t rsv_27_29[3];
    volatile gpu_gcproductid_t gcproductid;
    volatile uint32_t rsv_2b_3f[21];
    volatile gpu_modulepowerctrl_t modulepowerctrl;
    volatile gpu_powermodulectrl_t powermodulectrl;
    volatile gpu_powermodulestatus_t powermodulestatus;
    volatile uint32_t rsv_43_104[194];
    volatile gpu_aqmemorydebug_t aqmemorydebug;
    volatile uint32_t rsv_106_10a[5];
    volatile gpu_aqregtimingctrl_t aqregtimingctrl;
    volatile uint32_t rsv_10c_13f[52];
    volatile gpu_fetchaddreserveds_t fetchaddreserveds;
    volatile gpu_fetchctrl_t fetchctrl;
    volatile gpu_currentfetchaddr_t currentfetchaddr;
} gpu_hw_t;

#ifdef __cplusplus
}
#endif
