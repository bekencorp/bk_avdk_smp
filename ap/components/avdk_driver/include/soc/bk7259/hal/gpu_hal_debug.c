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
#include "gpu_hw.h"
#include "gpu_hal.h"

typedef void (*gpu_dump_fn_t)(void);
typedef struct
{
    uint32_t start;
    uint32_t end;
    gpu_dump_fn_t fn;
} gpu_reg_fn_map_t;

static void gpu_dump_aqhiclkctrl(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));

    SOC_LOGI("aqhiclkctrl: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x0 << 2)));
    SOC_LOGI("	clk3ddis: %8x\r\n", r->clk3ddis);
    SOC_LOGI("	clk2ddis: %8x\r\n", r->clk2ddis);
    SOC_LOGI("	fscale_val: %8x\r\n", r->fscale_val);
    SOC_LOGI("	fscale_cmd_load: %8x\r\n", r->fscale_cmd_load);
    SOC_LOGI("	dis_ram_clk_gating: %8x\r\n", r->dis_ram_clk_gating);
    SOC_LOGI("	dis_dbg_register: %8x\r\n", r->dis_dbg_register);
    SOC_LOGI("	soft_rst: %8x\r\n", r->soft_rst);
    SOC_LOGI("	reserved_13_15: %8x\r\n", r->reserved_13_15);
    SOC_LOGI("	idle3_d: %8x\r\n", r->idle3_d);
    SOC_LOGI("	idle2_d: %8x\r\n", r->idle2_d);
    SOC_LOGI("	idle_vg: %8x\r\n", r->idle_vg);
    SOC_LOGI("	isolategpu: %8x\r\n", r->isolategpu);
    SOC_LOGI("	reserved_bit_20_31: %8x\r\n", r->reserved_bit_20_31);
}

static void gpu_dump_aqhiidlereg(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));

    SOC_LOGI("aqhiidlereg: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x1 << 2)));
    SOC_LOGI("	idle_fe: %8x\r\n", r->idle_fe);
    SOC_LOGI("	idle_de: %8x\r\n", r->idle_de);
    SOC_LOGI("	idle_pe: %8x\r\n", r->idle_pe);
    SOC_LOGI("	idle_sh: %8x\r\n", r->idle_sh);
    SOC_LOGI("	idle_pa: %8x\r\n", r->idle_pa);
    SOC_LOGI("	idle_se: %8x\r\n", r->idle_se);
    SOC_LOGI("	idle_ra: %8x\r\n", r->idle_ra);
    SOC_LOGI("	idle_tx: %8x\r\n", r->idle_tx);
    SOC_LOGI("	idle_vg: %8x\r\n", r->idle_vg);
    SOC_LOGI("	idle_im: %8x\r\n", r->idle_im);
    SOC_LOGI("	idle_fp: %8x\r\n", r->idle_fp);
    SOC_LOGI("	idle_ts: %8x\r\n", r->idle_ts);
    SOC_LOGI("	idle_blt: %8x\r\n", r->idle_blt);
    SOC_LOGI("	reserved_13_30: %8x\r\n", r->reserved_13_30);
    SOC_LOGI("	axi_lp: %8x\r\n", r->axi_lp);
}

static void gpu_dump_aqaxiconfig(void)
{
    gpu_aqaxiconfig_t *r = (gpu_aqaxiconfig_t *)(SOC_GPU_REG_BASE + (0x2 << 2));

    SOC_LOGI("aqaxiconfig: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x2 << 2)));
    SOC_LOGI("	reserved_0_7: %8x\r\n", r->reserved_0_7);
    SOC_LOGI("	awcache: %8x\r\n", r->awcache);
    SOC_LOGI("	arcache: %8x\r\n", r->arcache);
    SOC_LOGI("	axdomain_shared: %8x\r\n", r->axdomain_shared);
    SOC_LOGI("	axdomain_noshared: %8x\r\n", r->axdomain_noshared);
    SOC_LOGI("	axcache_override_shared: %8x\r\n", r->axcache_override_shared);
    SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void gpu_dump_(void)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));

    SOC_LOGI(": %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x3 << 2)));
    SOC_LOGI("	wr_err_id: %8x\r\n", r->wr_err_id);
    SOC_LOGI("	rd_err_id: %8x\r\n", r->rd_err_id);
    SOC_LOGI("	det_wr_err: %8x\r\n", r->det_wr_err);
    SOC_LOGI("	det_rd_err: %8x\r\n", r->det_rd_err);
    SOC_LOGI("	reserved_bit_10_31: %8x\r\n", r->reserved_bit_10_31);
}

static void gpu_dump_aqintrack(void)
{
    SOC_LOGI("aqintrack: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x4 << 2)));
}

static void gpu_dump_aqintren(void)
{
    SOC_LOGI("aqintren: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x5 << 2)));
}

static void gpu_dump_aqidentreg(void)
{
    SOC_LOGI("aqidentreg: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x6 << 2)));
}

static void gpu_dump_gcfeatureserved(void)
{
    SOC_LOGI("gcfeatureserved: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x7 << 2)));
}

static void gpu_dump_gcchipidreg(void)
{
    SOC_LOGI("gcchipidreg: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x8 << 2)));
}

static void gpu_dump_gcchiprev(void)
{
    SOC_LOGI("gcchiprev: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x9 << 2)));
}

static void gpu_dump_gcchipdate(void)
{
    SOC_LOGI("gcchipdate: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0xa << 2)));
}

static void gpu_dump_gcchiptime(void)
{
    SOC_LOGI("gcchiptime: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0xb << 2)));
}

static void gpu_dump_gcchipcustomer(void)
{
    SOC_LOGI("gcchipcustomer: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0xc << 2)));
}

static void gpu_dump_gcminorfeatureserved(void)
{
    SOC_LOGI("gcminorfeatureserved: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0xd << 2)));
}

static void gpu_dump_rsv_e_e(void)
{
    for (uint32_t idx = 0; idx < 1; idx++)
    {
        SOC_LOGI("rsv_e_e: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + ((0xe + idx) << 2)));
    }
}

static void gpu_dump_gcreservedetmem(void)
{
    SOC_LOGI("gcreservedetmem: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0xf << 2)));
}

static void gpu_dump_rsv_10_25(void)
{
    for (uint32_t idx = 0; idx < 22; idx++)
    {
        SOC_LOGI("rsv_10_25: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + ((0x10 + idx) << 2)));
    }
}

static void gpu_dump_gcreghichippatchrev(void)
{
    SOC_LOGI("gcreghichippatchrev: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x26 << 2)));
}

static void gpu_dump_rsv_27_29(void)
{
    for (uint32_t idx = 0; idx < 3; idx++)
    {
        SOC_LOGI("rsv_27_29: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + ((0x27 + idx) << 2)));
    }
}

static void gpu_dump_gcproductid(void)
{
    gpu_gcproductid_t *r = (gpu_gcproductid_t *)(SOC_GPU_REG_BASE + (0x2a << 2));

    SOC_LOGI("gcproductid: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x2a << 2)));
    SOC_LOGI("	grade_level: %8x\r\n", r->grade_level);
    SOC_LOGI("	num: %8x\r\n", r->num);
    SOC_LOGI("	type: %8x\r\n", r->type);
    SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void gpu_dump_rsv_2b_3f(void)
{
    for (uint32_t idx = 0; idx < 21; idx++)
    {
        SOC_LOGI("rsv_2b_3f: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + ((0x2b + idx) << 2)));
    }
}

static void gpu_dump_modulepowerctrl(void)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));

    SOC_LOGI("modulepowerctrl: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x40 << 2)));
    SOC_LOGI("	en_module_clk_gating: %8x\r\n", r->en_module_clk_gating);
    SOC_LOGI("	dis_stall_module_clk_gating: %8x\r\n", r->dis_stall_module_clk_gating);
    SOC_LOGI("	dis_starve_module_clk_gating: %8x\r\n", r->dis_starve_module_clk_gating);
    SOC_LOGI("	reserved_bit_3_3: %8x\r\n", r->reserved_bit_3_3);
    SOC_LOGI("	turn_on_counter: %8x\r\n", r->turn_on_counter);
    SOC_LOGI("	reserved_bit_8_15: %8x\r\n", r->reserved_bit_8_15);
    SOC_LOGI("	turn_off_conuter: %8x\r\n", r->turn_off_conuter);
}

static void gpu_dump_powermodulectrl(void)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));

    SOC_LOGI("powermodulectrl: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x41 << 2)));
    SOC_LOGI("	dis_clk_gating_fe: %8x\r\n", r->dis_clk_gating_fe);
    SOC_LOGI("	reserved_bit_1_1: %8x\r\n", r->reserved_bit_1_1);
    SOC_LOGI("	dis_clk_gating_pe: %8x\r\n", r->dis_clk_gating_pe);
    SOC_LOGI("	reserved_bit_3_7: %8x\r\n", r->reserved_bit_3_7);
    SOC_LOGI("	dis_clk_gating_vg: %8x\r\n", r->dis_clk_gating_vg);
    SOC_LOGI("	dis_clk_gating_im: %8x\r\n", r->dis_clk_gating_im);
    SOC_LOGI("	reserved_bit_10_10: %8x\r\n", r->reserved_bit_10_10);
    SOC_LOGI("	dis_clk_gating_ts: %8x\r\n", r->dis_clk_gating_ts);
    SOC_LOGI("	reserved_bit_12_31: %8x\r\n", r->reserved_bit_12_31);
}

static void gpu_dump_powermodulestatus(void)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));

    SOC_LOGI("powermodulestatus: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x42 << 2)));
    SOC_LOGI("	clk_gating_fe_s: %8x\r\n", r->clk_gating_fe_s);
    SOC_LOGI("	reserved_bit_1_1: %8x\r\n", r->reserved_bit_1_1);
    SOC_LOGI("	clk_gating_pe_s: %8x\r\n", r->clk_gating_pe_s);
    SOC_LOGI("	reserved_bit_3_7: %8x\r\n", r->reserved_bit_3_7);
    SOC_LOGI("	clk_gating_vg_s: %8x\r\n", r->clk_gating_vg_s);
    SOC_LOGI("	clk_gating_im_s: %8x\r\n", r->clk_gating_im_s);
    SOC_LOGI("	reserved_bit_10_10: %8x\r\n", r->reserved_bit_10_10);
    SOC_LOGI("	clk_gating_ts_s: %8x\r\n", r->clk_gating_ts_s);
    SOC_LOGI("	clk_gating_flexa_s: %8x\r\n", r->clk_gating_flexa_s);
    SOC_LOGI("	reserved_bit_13_31: %8x\r\n", r->reserved_bit_13_31);
}

static void gpu_dump_rsv_43_104(void)
{
    for (uint32_t idx = 0; idx < 194; idx++)
    {
        SOC_LOGI("rsv_43_104: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + ((0x43 + idx) << 2)));
    }
}

static void gpu_dump_aqmemorydebug(void)
{
    gpu_aqmemorydebug_t *r = (gpu_aqmemorydebug_t *)(SOC_GPU_REG_BASE + (0x105 << 2));

    SOC_LOGI("aqmemorydebug: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x105 << 2)));
    SOC_LOGI("	max_outstanding_reads: %8x\r\n", r->max_outstanding_reads);
    SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void gpu_dump_rsv_106_10a(void)
{
    for (uint32_t idx = 0; idx < 5; idx++)
    {
        SOC_LOGI("rsv_106_10a: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + ((0x106 + idx) << 2)));
    }
}

static void gpu_dump_aqregtimingctrl(void)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));

    SOC_LOGI("aqregtimingctrl: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x10b << 2)));
    SOC_LOGI("	for_rf1p: %8x\r\n", r->for_rf1p);
    SOC_LOGI("	for_rf2p: %8x\r\n", r->for_rf2p);
    SOC_LOGI("	fast_rtc: %8x\r\n", r->fast_rtc);
    SOC_LOGI("	fast_wtc: %8x\r\n", r->fast_wtc);
    SOC_LOGI("	power_down: %8x\r\n", r->power_down);
    SOC_LOGI("	reserved_bit_21_31: %8x\r\n", r->reserved_bit_21_31);
}

static void gpu_dump_rsv_10c_13f(void)
{
    for (uint32_t idx = 0; idx < 52; idx++)
    {
        SOC_LOGI("rsv_10c_13f: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + ((0x10c + idx) << 2)));
    }
}

static void gpu_dump_fetchaddreserveds(void)
{
    gpu_fetchaddreserveds_t *r = (gpu_fetchaddreserveds_t *)(SOC_GPU_REG_BASE + (0x140 << 2));

    SOC_LOGI("fetchaddreserveds: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x140 << 2)));
    SOC_LOGI("	type: %8x\r\n", r->type);
    SOC_LOGI("	addreserveds: %8x\r\n", r->addreserveds);
}

static void gpu_dump_fetchctrl(void)
{
    gpu_fetchctrl_t *r = (gpu_fetchctrl_t *)(SOC_GPU_REG_BASE + (0x141 << 2));

    SOC_LOGI("fetchctrl: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x141 << 2)));
    SOC_LOGI("	count: %8x\r\n", r->count);
    SOC_LOGI("	reserved_bit_21_31: %8x\r\n", r->reserved_bit_21_31);
}

static void gpu_dump_currentfetchaddr(void)
{
    SOC_LOGI("currentfetchaddr: %8x\r\n", REG_READ(SOC_GPU_REG_BASE + (0x142 << 2)));
}

static gpu_reg_fn_map_t s_fn[] =
{
    {0x0, 0x0, gpu_dump_aqhiclkctrl},
    {0x1, 0x1, gpu_dump_aqhiidlereg},
    {0x2, 0x2, gpu_dump_aqaxiconfig},
    {0x3, 0x3, gpu_dump_},
    {0x4, 0x4, gpu_dump_aqintrack},
    {0x5, 0x5, gpu_dump_aqintren},
    {0x6, 0x6, gpu_dump_aqidentreg},
    {0x7, 0x7, gpu_dump_gcfeatureserved},
    {0x8, 0x8, gpu_dump_gcchipidreg},
    {0x9, 0x9, gpu_dump_gcchiprev},
    {0xa, 0xa, gpu_dump_gcchipdate},
    {0xb, 0xb, gpu_dump_gcchiptime},
    {0xc, 0xc, gpu_dump_gcchipcustomer},
    {0xd, 0xd, gpu_dump_gcminorfeatureserved},
    {0xe, 0xf, gpu_dump_rsv_e_e},
    {0xf, 0xf, gpu_dump_gcreservedetmem},
    {0x10, 0x26, gpu_dump_rsv_10_25},
    {0x26, 0x26, gpu_dump_gcreghichippatchrev},
    {0x27, 0x2a, gpu_dump_rsv_27_29},
    {0x2a, 0x2a, gpu_dump_gcproductid},
    {0x2b, 0x40, gpu_dump_rsv_2b_3f},
    {0x40, 0x40, gpu_dump_modulepowerctrl},
    {0x41, 0x41, gpu_dump_powermodulectrl},
    {0x42, 0x42, gpu_dump_powermodulestatus},
    {0x43, 0x105, gpu_dump_rsv_43_104},
    {0x105, 0x105, gpu_dump_aqmemorydebug},
    {0x106, 0x10b, gpu_dump_rsv_106_10a},
    {0x10b, 0x10b, gpu_dump_aqregtimingctrl},
    {0x10c, 0x140, gpu_dump_rsv_10c_13f},
    {0x140, 0x140, gpu_dump_fetchaddreserveds},
    {0x141, 0x141, gpu_dump_fetchctrl},
    {0x142, 0x142, gpu_dump_currentfetchaddr},
    {-1, -1, 0}
};

void gpu_struct_dump(uint32_t start, uint32_t end)
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
