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
#include "gpu_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPU_LL_REG_BASE   SOC_GPU_REG_BASE

//reg aqhiclkctrl:

static inline void gpu_ll_set_aqhiclkctrl_value(uint32_t v)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_value(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->v;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_fscale_val(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->fscale_val;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_fscale_cmd_load(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->fscale_cmd_load;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_dis_ram_clk_gating(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->dis_ram_clk_gating;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_dis_dbg_register(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->dis_dbg_register;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_soft_rst(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->soft_rst;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_reserved_13_15(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->reserved_13_15;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_idle3_d(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->idle3_d;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_idle2_d(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->idle2_d;
}

static inline void gpu_ll_set_aqhiclkctrl_isolategpu(uint32_t v)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    r->isolategpu = v;
}

static inline uint32_t gpu_ll_get_aqhiclkctrl_isolategpu(void)
{
    gpu_aqhiclkctrl_t *r = (gpu_aqhiclkctrl_t *)(SOC_GPU_REG_BASE + (0x0 << 2));
    return r->isolategpu;
}

//reg aqhiidlereg:

static inline void gpu_ll_set_aqhiidlereg_value(uint32_t v)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_value(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->v;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_idle_de(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->idle_de;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_idle_sh(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->idle_sh;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_idle_pa(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->idle_pa;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_idle_se(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->idle_se;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_idle_ra(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->idle_ra;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_idle_tx(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->idle_tx;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_idle_im(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->idle_im;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_idle_fp(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->idle_fp;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_idle_blt(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->idle_blt;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_reserved_13_30(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->reserved_13_30;
}

static inline void gpu_ll_set_aqhiidlereg_axi_lp(uint32_t v)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    r->axi_lp = v;
}

static inline uint32_t gpu_ll_get_aqhiidlereg_axi_lp(void)
{
    gpu_aqhiidlereg_t *r = (gpu_aqhiidlereg_t *)(SOC_GPU_REG_BASE + (0x1 << 2));
    return r->axi_lp;
}

//reg aqaxiconfig:

static inline void gpu_ll_set_aqaxiconfig_value(uint32_t v)
{
    gpu_aqaxiconfig_t *r = (gpu_aqaxiconfig_t *)(SOC_GPU_REG_BASE + (0x2 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_aqaxiconfig_value(void)
{
    gpu_aqaxiconfig_t *r = (gpu_aqaxiconfig_t *)(SOC_GPU_REG_BASE + (0x2 << 2));
    return r->v;
}

static inline uint32_t gpu_ll_get_aqaxiconfig_reserved_0_7(void)
{
    gpu_aqaxiconfig_t *r = (gpu_aqaxiconfig_t *)(SOC_GPU_REG_BASE + (0x2 << 2));
    return r->reserved_0_7;
}

static inline void gpu_ll_set_aqaxiconfig_awcache(uint32_t v)
{
    gpu_aqaxiconfig_t *r = (gpu_aqaxiconfig_t *)(SOC_GPU_REG_BASE + (0x2 << 2));
    r->awcache = v;
}

static inline uint32_t gpu_ll_get_aqaxiconfig_arcache(void)
{
    gpu_aqaxiconfig_t *r = (gpu_aqaxiconfig_t *)(SOC_GPU_REG_BASE + (0x2 << 2));
    return r->arcache;
}

static inline uint32_t gpu_ll_get_aqaxiconfig_axdomain_shared(void)
{
    gpu_aqaxiconfig_t *r = (gpu_aqaxiconfig_t *)(SOC_GPU_REG_BASE + (0x2 << 2));
    return r->axdomain_shared;
}

static inline uint32_t gpu_ll_get_aqaxiconfig_axdomain_noshared(void)
{
    gpu_aqaxiconfig_t *r = (gpu_aqaxiconfig_t *)(SOC_GPU_REG_BASE + (0x2 << 2));
    return r->axdomain_noshared;
}

static inline uint32_t gpu_ll_get_aqaxiconfig_axcache_override_shared(void)
{
    gpu_aqaxiconfig_t *r = (gpu_aqaxiconfig_t *)(SOC_GPU_REG_BASE + (0x2 << 2));
    return r->axcache_override_shared;
}

//reg :

static inline void gpu_ll_set__value(uint32_t v)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get__value(void)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));
    return r->v;
}

static inline void gpu_ll_set__wr_err_id(uint32_t v)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));
    r->wr_err_id = v;
}

static inline uint32_t gpu_ll_get__wr_err_id(void)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));
    return r->wr_err_id;
}

static inline void gpu_ll_set__rd_err_id(uint32_t v)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));
    r->rd_err_id = v;
}

static inline uint32_t gpu_ll_get__rd_err_id(void)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));
    return r->rd_err_id;
}

static inline void gpu_ll_set__det_wr_err(uint32_t v)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));
    r->det_wr_err = v;
}

static inline uint32_t gpu_ll_get__det_wr_err(void)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));
    return r->det_wr_err;
}

static inline uint32_t gpu_ll_get__det_rd_err(void)
{
    gpu__t *r = (gpu__t *)(SOC_GPU_REG_BASE + (0x3 << 2));
    return r->det_rd_err;
}

//reg aqintrack:

static inline void gpu_ll_set_aqintrack_value(uint32_t v)
{
    gpu_aqintrack_t *r = (gpu_aqintrack_t *)(SOC_GPU_REG_BASE + (0x4 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_aqintrack_value(void)
{
    gpu_aqintrack_t *r = (gpu_aqintrack_t *)(SOC_GPU_REG_BASE + (0x4 << 2));
    return r->v;
}

static inline void gpu_ll_set_aqintrack_intr_vec(uint32_t v)
{
    gpu_aqintrack_t *r = (gpu_aqintrack_t *)(SOC_GPU_REG_BASE + (0x4 << 2));
    r->intr_vec = v;
}

static inline uint32_t gpu_ll_get_aqintrack_intr_vec(void)
{
    gpu_aqintrack_t *r = (gpu_aqintrack_t *)(SOC_GPU_REG_BASE + (0x4 << 2));
    return r->intr_vec;
}

//reg aqintren:

static inline void gpu_ll_set_aqintren_value(uint32_t v)
{
    gpu_aqintren_t *r = (gpu_aqintren_t *)(SOC_GPU_REG_BASE + (0x5 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_aqintren_value(void)
{
    gpu_aqintren_t *r = (gpu_aqintren_t *)(SOC_GPU_REG_BASE + (0x5 << 2));
    return r->v;
}

static inline uint32_t gpu_ll_get_aqintren_intr_enbl_vec(void)
{
    gpu_aqintren_t *r = (gpu_aqintren_t *)(SOC_GPU_REG_BASE + (0x5 << 2));
    return r->intr_enbl_vec;
}

//reg aqidentreg:

static inline void gpu_ll_set_aqidentreg_value(uint32_t v)
{
    gpu_aqidentreg_t *r = (gpu_aqidentreg_t *)(SOC_GPU_REG_BASE + (0x6 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_aqidentreg_value(void)
{
    gpu_aqidentreg_t *r = (gpu_aqidentreg_t *)(SOC_GPU_REG_BASE + (0x6 << 2));
    return r->v;
}

static inline void gpu_ll_set_aqidentreg_aqident(uint32_t v)
{
    gpu_aqidentreg_t *r = (gpu_aqidentreg_t *)(SOC_GPU_REG_BASE + (0x6 << 2));
    r->aqident = v;
}

static inline uint32_t gpu_ll_get_aqidentreg_aqident(void)
{
    gpu_aqidentreg_t *r = (gpu_aqidentreg_t *)(SOC_GPU_REG_BASE + (0x6 << 2));
    return r->aqident;
}

//reg gcfeatureserved:

static inline void gpu_ll_set_gcfeatureserved_value(uint32_t v)
{
    gpu_gcfeatureserved_t *r = (gpu_gcfeatureserved_t *)(SOC_GPU_REG_BASE + (0x7 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcfeatureserved_value(void)
{
    gpu_gcfeatureserved_t *r = (gpu_gcfeatureserved_t *)(SOC_GPU_REG_BASE + (0x7 << 2));
    return r->v;
}

static inline void gpu_ll_set_gcfeatureserved_featureserved(uint32_t v)
{
    gpu_gcfeatureserved_t *r = (gpu_gcfeatureserved_t *)(SOC_GPU_REG_BASE + (0x7 << 2));
    r->featureserved = v;
}

static inline uint32_t gpu_ll_get_gcfeatureserved_featureserved(void)
{
    gpu_gcfeatureserved_t *r = (gpu_gcfeatureserved_t *)(SOC_GPU_REG_BASE + (0x7 << 2));
    return r->featureserved;
}

//reg gcchipidreg:

static inline void gpu_ll_set_gcchipidreg_value(uint32_t v)
{
    gpu_gcchipidreg_t *r = (gpu_gcchipidreg_t *)(SOC_GPU_REG_BASE + (0x8 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcchipidreg_value(void)
{
    gpu_gcchipidreg_t *r = (gpu_gcchipidreg_t *)(SOC_GPU_REG_BASE + (0x8 << 2));
    return r->v;
}

static inline void gpu_ll_set_gcchipidreg_gcchipid(uint32_t v)
{
    gpu_gcchipidreg_t *r = (gpu_gcchipidreg_t *)(SOC_GPU_REG_BASE + (0x8 << 2));
    r->gcchipid = v;
}

static inline uint32_t gpu_ll_get_gcchipidreg_gcchipid(void)
{
    gpu_gcchipidreg_t *r = (gpu_gcchipidreg_t *)(SOC_GPU_REG_BASE + (0x8 << 2));
    return r->gcchipid;
}

//reg gcchiprev:

static inline void gpu_ll_set_gcchiprev_value(uint32_t v)
{
    gpu_gcchiprev_t *r = (gpu_gcchiprev_t *)(SOC_GPU_REG_BASE + (0x9 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcchiprev_value(void)
{
    gpu_gcchiprev_t *r = (gpu_gcchiprev_t *)(SOC_GPU_REG_BASE + (0x9 << 2));
    return r->v;
}

static inline void gpu_ll_set_gcchiprev_rev(uint32_t v)
{
    gpu_gcchiprev_t *r = (gpu_gcchiprev_t *)(SOC_GPU_REG_BASE + (0x9 << 2));
    r->rev = v;
}

static inline uint32_t gpu_ll_get_gcchiprev_rev(void)
{
    gpu_gcchiprev_t *r = (gpu_gcchiprev_t *)(SOC_GPU_REG_BASE + (0x9 << 2));
    return r->rev;
}

//reg gcchipdate:

static inline void gpu_ll_set_gcchipdate_value(uint32_t v)
{
    gpu_gcchipdate_t *r = (gpu_gcchipdate_t *)(SOC_GPU_REG_BASE + (0xa << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcchipdate_value(void)
{
    gpu_gcchipdate_t *r = (gpu_gcchipdate_t *)(SOC_GPU_REG_BASE + (0xa << 2));
    return r->v;
}

static inline void gpu_ll_set_gcchipdate_date(uint32_t v)
{
    gpu_gcchipdate_t *r = (gpu_gcchipdate_t *)(SOC_GPU_REG_BASE + (0xa << 2));
    r->date = v;
}

static inline uint32_t gpu_ll_get_gcchipdate_date(void)
{
    gpu_gcchipdate_t *r = (gpu_gcchipdate_t *)(SOC_GPU_REG_BASE + (0xa << 2));
    return r->date;
}

//reg gcchiptime:

static inline void gpu_ll_set_gcchiptime_value(uint32_t v)
{
    gpu_gcchiptime_t *r = (gpu_gcchiptime_t *)(SOC_GPU_REG_BASE + (0xb << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcchiptime_value(void)
{
    gpu_gcchiptime_t *r = (gpu_gcchiptime_t *)(SOC_GPU_REG_BASE + (0xb << 2));
    return r->v;
}

static inline void gpu_ll_set_gcchiptime_chiptime(uint32_t v)
{
    gpu_gcchiptime_t *r = (gpu_gcchiptime_t *)(SOC_GPU_REG_BASE + (0xb << 2));
    r->chiptime = v;
}

static inline uint32_t gpu_ll_get_gcchiptime_chiptime(void)
{
    gpu_gcchiptime_t *r = (gpu_gcchiptime_t *)(SOC_GPU_REG_BASE + (0xb << 2));
    return r->chiptime;
}

//reg gcchipcustomer:

static inline void gpu_ll_set_gcchipcustomer_value(uint32_t v)
{
    gpu_gcchipcustomer_t *r = (gpu_gcchipcustomer_t *)(SOC_GPU_REG_BASE + (0xc << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcchipcustomer_value(void)
{
    gpu_gcchipcustomer_t *r = (gpu_gcchipcustomer_t *)(SOC_GPU_REG_BASE + (0xc << 2));
    return r->v;
}

static inline void gpu_ll_set_gcchipcustomer_chipcustomer(uint32_t v)
{
    gpu_gcchipcustomer_t *r = (gpu_gcchipcustomer_t *)(SOC_GPU_REG_BASE + (0xc << 2));
    r->chipcustomer = v;
}

static inline uint32_t gpu_ll_get_gcchipcustomer_chipcustomer(void)
{
    gpu_gcchipcustomer_t *r = (gpu_gcchipcustomer_t *)(SOC_GPU_REG_BASE + (0xc << 2));
    return r->chipcustomer;
}

//reg gcminorfeatureserved:

static inline void gpu_ll_set_gcminorfeatureserved_value(uint32_t v)
{
    gpu_gcminorfeatureserved_t *r = (gpu_gcminorfeatureserved_t *)(SOC_GPU_REG_BASE + (0xd << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcminorfeatureserved_value(void)
{
    gpu_gcminorfeatureserved_t *r = (gpu_gcminorfeatureserved_t *)(SOC_GPU_REG_BASE + (0xd << 2));
    return r->v;
}

static inline void gpu_ll_set_gcminorfeatureserved_minorfeatureserved(uint32_t v)
{
    gpu_gcminorfeatureserved_t *r = (gpu_gcminorfeatureserved_t *)(SOC_GPU_REG_BASE + (0xd << 2));
    r->minorfeatureserved = v;
}

static inline uint32_t gpu_ll_get_gcminorfeatureserved_minorfeatureserved(void)
{
    gpu_gcminorfeatureserved_t *r = (gpu_gcminorfeatureserved_t *)(SOC_GPU_REG_BASE + (0xd << 2));
    return r->minorfeatureserved;
}

//reg gcreservedetmem:

static inline void gpu_ll_set_gcreservedetmem_value(uint32_t v)
{
    gpu_gcreservedetmem_t *r = (gpu_gcreservedetmem_t *)(SOC_GPU_REG_BASE + (0xf << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcreservedetmem_value(void)
{
    gpu_gcreservedetmem_t *r = (gpu_gcreservedetmem_t *)(SOC_GPU_REG_BASE + (0xf << 2));
    return r->v;
}

//reg gcreghichippatchrev:

static inline void gpu_ll_set_gcreghichippatchrev_value(uint32_t v)
{
    gpu_gcreghichippatchrev_t *r = (gpu_gcreghichippatchrev_t *)(SOC_GPU_REG_BASE + (0x26 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcreghichippatchrev_value(void)
{
    gpu_gcreghichippatchrev_t *r = (gpu_gcreghichippatchrev_t *)(SOC_GPU_REG_BASE + (0x26 << 2));
    return r->v;
}

static inline void gpu_ll_set_gcreghichippatchrev_patch_rev(uint32_t v)
{
    gpu_gcreghichippatchrev_t *r = (gpu_gcreghichippatchrev_t *)(SOC_GPU_REG_BASE + (0x26 << 2));
    r->patch_rev = v;
}

static inline uint32_t gpu_ll_get_gcreghichippatchrev_patch_rev(void)
{
    gpu_gcreghichippatchrev_t *r = (gpu_gcreghichippatchrev_t *)(SOC_GPU_REG_BASE + (0x26 << 2));
    return r->patch_rev;
}

//reg gcproductid:

static inline void gpu_ll_set_gcproductid_value(uint32_t v)
{
    gpu_gcproductid_t *r = (gpu_gcproductid_t *)(SOC_GPU_REG_BASE + (0x2a << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_gcproductid_value(void)
{
    gpu_gcproductid_t *r = (gpu_gcproductid_t *)(SOC_GPU_REG_BASE + (0x2a << 2));
    return r->v;
}

static inline uint32_t gpu_ll_get_gcproductid_grade_level(void)
{
    gpu_gcproductid_t *r = (gpu_gcproductid_t *)(SOC_GPU_REG_BASE + (0x2a << 2));
    return r->grade_level;
}

static inline void gpu_ll_set_gcproductid_num(uint32_t v)
{
    gpu_gcproductid_t *r = (gpu_gcproductid_t *)(SOC_GPU_REG_BASE + (0x2a << 2));
    r->num = v;
}

static inline uint32_t gpu_ll_get_gcproductid_num(void)
{
    gpu_gcproductid_t *r = (gpu_gcproductid_t *)(SOC_GPU_REG_BASE + (0x2a << 2));
    return r->num;
}

static inline void gpu_ll_set_gcproductid_type(uint32_t v)
{
    gpu_gcproductid_t *r = (gpu_gcproductid_t *)(SOC_GPU_REG_BASE + (0x2a << 2));
    r->type = v;
}

static inline uint32_t gpu_ll_get_gcproductid_type(void)
{
    gpu_gcproductid_t *r = (gpu_gcproductid_t *)(SOC_GPU_REG_BASE + (0x2a << 2));
    return r->type;
}

//reg modulepowerctrl:

static inline void gpu_ll_set_modulepowerctrl_value(uint32_t v)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_modulepowerctrl_value(void)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    return r->v;
}

static inline void gpu_ll_set_modulepowerctrl_en_module_clk_gating(uint32_t v)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    r->en_module_clk_gating = v;
}

static inline uint32_t gpu_ll_get_modulepowerctrl_en_module_clk_gating(void)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    return r->en_module_clk_gating;
}

static inline void gpu_ll_set_modulepowerctrl_dis_stall_module_clk_gating(uint32_t v)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    r->dis_stall_module_clk_gating = v;
}

static inline uint32_t gpu_ll_get_modulepowerctrl_dis_stall_module_clk_gating(void)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    return r->dis_stall_module_clk_gating;
}

static inline void gpu_ll_set_modulepowerctrl_dis_starve_module_clk_gating(uint32_t v)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    r->dis_starve_module_clk_gating = v;
}

static inline uint32_t gpu_ll_get_modulepowerctrl_dis_starve_module_clk_gating(void)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    return r->dis_starve_module_clk_gating;
}

static inline void gpu_ll_set_modulepowerctrl_turn_on_counter(uint32_t v)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    r->turn_on_counter = v;
}

static inline uint32_t gpu_ll_get_modulepowerctrl_turn_on_counter(void)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    return r->turn_on_counter;
}

static inline void gpu_ll_set_modulepowerctrl_turn_off_conuter(uint32_t v)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    r->turn_off_conuter = v;
}

static inline uint32_t gpu_ll_get_modulepowerctrl_turn_off_conuter(void)
{
    gpu_modulepowerctrl_t *r = (gpu_modulepowerctrl_t *)(SOC_GPU_REG_BASE + (0x40 << 2));
    return r->turn_off_conuter;
}

//reg powermodulectrl:

static inline void gpu_ll_set_powermodulectrl_value(uint32_t v)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_powermodulectrl_value(void)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    return r->v;
}

static inline void gpu_ll_set_powermodulectrl_dis_clk_gating_fe(uint32_t v)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    r->dis_clk_gating_fe = v;
}

static inline uint32_t gpu_ll_get_powermodulectrl_dis_clk_gating_fe(void)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    return r->dis_clk_gating_fe;
}

static inline void gpu_ll_set_powermodulectrl_dis_clk_gating_pe(uint32_t v)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    r->dis_clk_gating_pe = v;
}

static inline uint32_t gpu_ll_get_powermodulectrl_dis_clk_gating_pe(void)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    return r->dis_clk_gating_pe;
}

static inline void gpu_ll_set_powermodulectrl_dis_clk_gating_vg(uint32_t v)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    r->dis_clk_gating_vg = v;
}

static inline uint32_t gpu_ll_get_powermodulectrl_dis_clk_gating_vg(void)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    return r->dis_clk_gating_vg;
}

static inline void gpu_ll_set_powermodulectrl_dis_clk_gating_im(uint32_t v)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    r->dis_clk_gating_im = v;
}

static inline uint32_t gpu_ll_get_powermodulectrl_dis_clk_gating_im(void)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    return r->dis_clk_gating_im;
}

static inline void gpu_ll_set_powermodulectrl_dis_clk_gating_ts(uint32_t v)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    r->dis_clk_gating_ts = v;
}

static inline uint32_t gpu_ll_get_powermodulectrl_dis_clk_gating_ts(void)
{
    gpu_powermodulectrl_t *r = (gpu_powermodulectrl_t *)(SOC_GPU_REG_BASE + (0x41 << 2));
    return r->dis_clk_gating_ts;
}

//reg powermodulestatus:

static inline void gpu_ll_set_powermodulestatus_value(uint32_t v)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_powermodulestatus_value(void)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    return r->v;
}

static inline void gpu_ll_set_powermodulestatus_clk_gating_fe_s(uint32_t v)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    r->clk_gating_fe_s = v;
}

static inline uint32_t gpu_ll_get_powermodulestatus_clk_gating_fe_s(void)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    return r->clk_gating_fe_s;
}

static inline void gpu_ll_set_powermodulestatus_clk_gating_pe_s(uint32_t v)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    r->clk_gating_pe_s = v;
}

static inline uint32_t gpu_ll_get_powermodulestatus_clk_gating_pe_s(void)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    return r->clk_gating_pe_s;
}

static inline void gpu_ll_set_powermodulestatus_clk_gating_vg_s(uint32_t v)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    r->clk_gating_vg_s = v;
}

static inline uint32_t gpu_ll_get_powermodulestatus_clk_gating_vg_s(void)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    return r->clk_gating_vg_s;
}

static inline void gpu_ll_set_powermodulestatus_clk_gating_im_s(uint32_t v)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    r->clk_gating_im_s = v;
}

static inline uint32_t gpu_ll_get_powermodulestatus_clk_gating_im_s(void)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    return r->clk_gating_im_s;
}

static inline void gpu_ll_set_powermodulestatus_clk_gating_ts_s(uint32_t v)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    r->clk_gating_ts_s = v;
}

static inline uint32_t gpu_ll_get_powermodulestatus_clk_gating_ts_s(void)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    return r->clk_gating_ts_s;
}

static inline void gpu_ll_set_powermodulestatus_clk_gating_flexa_s(uint32_t v)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    r->clk_gating_flexa_s = v;
}

static inline uint32_t gpu_ll_get_powermodulestatus_clk_gating_flexa_s(void)
{
    gpu_powermodulestatus_t *r = (gpu_powermodulestatus_t *)(SOC_GPU_REG_BASE + (0x42 << 2));
    return r->clk_gating_flexa_s;
}

//reg aqmemorydebug:

static inline void gpu_ll_set_aqmemorydebug_value(uint32_t v)
{
    gpu_aqmemorydebug_t *r = (gpu_aqmemorydebug_t *)(SOC_GPU_REG_BASE + (0x105 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_aqmemorydebug_value(void)
{
    gpu_aqmemorydebug_t *r = (gpu_aqmemorydebug_t *)(SOC_GPU_REG_BASE + (0x105 << 2));
    return r->v;
}

static inline void gpu_ll_set_aqmemorydebug_max_outstanding_reads(uint32_t v)
{
    gpu_aqmemorydebug_t *r = (gpu_aqmemorydebug_t *)(SOC_GPU_REG_BASE + (0x105 << 2));
    r->max_outstanding_reads = v;
}

static inline uint32_t gpu_ll_get_aqmemorydebug_max_outstanding_reads(void)
{
    gpu_aqmemorydebug_t *r = (gpu_aqmemorydebug_t *)(SOC_GPU_REG_BASE + (0x105 << 2));
    return r->max_outstanding_reads;
}

//reg aqregtimingctrl:

static inline void gpu_ll_set_aqregtimingctrl_value(uint32_t v)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_aqregtimingctrl_value(void)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));
    return r->v;
}

static inline uint32_t gpu_ll_get_aqregtimingctrl_for_rf1p(void)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));
    return r->for_rf1p;
}

static inline uint32_t gpu_ll_get_aqregtimingctrl_for_rf2p(void)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));
    return r->for_rf2p;
}

static inline uint32_t gpu_ll_get_aqregtimingctrl_fast_rtc(void)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));
    return r->fast_rtc;
}

static inline void gpu_ll_set_aqregtimingctrl_fast_wtc(uint32_t v)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));
    r->fast_wtc = v;
}

static inline uint32_t gpu_ll_get_aqregtimingctrl_fast_wtc(void)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));
    return r->fast_wtc;
}

static inline void gpu_ll_set_aqregtimingctrl_power_down(uint32_t v)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));
    r->power_down = v;
}

static inline uint32_t gpu_ll_get_aqregtimingctrl_power_down(void)
{
    gpu_aqregtimingctrl_t *r = (gpu_aqregtimingctrl_t *)(SOC_GPU_REG_BASE + (0x10b << 2));
    return r->power_down;
}

//reg fetchaddreserveds:

static inline void gpu_ll_set_fetchaddreserveds_value(uint32_t v)
{
    gpu_fetchaddreserveds_t *r = (gpu_fetchaddreserveds_t *)(SOC_GPU_REG_BASE + (0x140 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_fetchaddreserveds_value(void)
{
    gpu_fetchaddreserveds_t *r = (gpu_fetchaddreserveds_t *)(SOC_GPU_REG_BASE + (0x140 << 2));
    return r->v;
}

static inline void gpu_ll_set_fetchaddreserveds_type(uint32_t v)
{
    gpu_fetchaddreserveds_t *r = (gpu_fetchaddreserveds_t *)(SOC_GPU_REG_BASE + (0x140 << 2));
    r->type = v;
}

static inline uint32_t gpu_ll_get_fetchaddreserveds_type(void)
{
    gpu_fetchaddreserveds_t *r = (gpu_fetchaddreserveds_t *)(SOC_GPU_REG_BASE + (0x140 << 2));
    return r->type;
}

static inline void gpu_ll_set_fetchaddreserveds_addreserveds(uint32_t v)
{
    gpu_fetchaddreserveds_t *r = (gpu_fetchaddreserveds_t *)(SOC_GPU_REG_BASE + (0x140 << 2));
    r->addreserveds = v;
}

static inline uint32_t gpu_ll_get_fetchaddreserveds_addreserveds(void)
{
    gpu_fetchaddreserveds_t *r = (gpu_fetchaddreserveds_t *)(SOC_GPU_REG_BASE + (0x140 << 2));
    return r->addreserveds;
}

//reg fetchctrl:

static inline void gpu_ll_set_fetchctrl_value(uint32_t v)
{
    gpu_fetchctrl_t *r = (gpu_fetchctrl_t *)(SOC_GPU_REG_BASE + (0x141 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_fetchctrl_value(void)
{
    gpu_fetchctrl_t *r = (gpu_fetchctrl_t *)(SOC_GPU_REG_BASE + (0x141 << 2));
    return r->v;
}

static inline void gpu_ll_set_fetchctrl_count(uint32_t v)
{
    gpu_fetchctrl_t *r = (gpu_fetchctrl_t *)(SOC_GPU_REG_BASE + (0x141 << 2));
    r->count = v;
}

static inline uint32_t gpu_ll_get_fetchctrl_count(void)
{
    gpu_fetchctrl_t *r = (gpu_fetchctrl_t *)(SOC_GPU_REG_BASE + (0x141 << 2));
    return r->count;
}

//reg currentfetchaddr:

static inline void gpu_ll_set_currentfetchaddr_value(uint32_t v)
{
    gpu_currentfetchaddr_t *r = (gpu_currentfetchaddr_t *)(SOC_GPU_REG_BASE + (0x142 << 2));
    r->v = v;
}

static inline uint32_t gpu_ll_get_currentfetchaddr_value(void)
{
    gpu_currentfetchaddr_t *r = (gpu_currentfetchaddr_t *)(SOC_GPU_REG_BASE + (0x142 << 2));
    return r->v;
}

static inline uint32_t gpu_ll_get_currentfetchaddr_addreserveds(void)
{
    gpu_currentfetchaddr_t *r = (gpu_currentfetchaddr_t *)(SOC_GPU_REG_BASE + (0x142 << 2));
    return r->addreserveds;
}
#ifdef __cplusplus
}
#endif
