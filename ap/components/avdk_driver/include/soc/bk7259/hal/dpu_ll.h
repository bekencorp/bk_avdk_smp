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
#include "dpu_hw.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DPU_LL_REG_BASE   SOC_DPU_REG_BASE

//reg gcregdcproductid:

static inline void dpu_ll_set_gcregdcproductid_value(uint32_t v)
{
    dpu_gcregdcproductid_t *r = (dpu_gcregdcproductid_t *)(SOC_DPU_REG_BASE + (0x803 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdcproductid_value(void)
{
    dpu_gcregdcproductid_t *r = (dpu_gcregdcproductid_t *)(SOC_DPU_REG_BASE + (0x803 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdcproductid_product_id(void)
{
    dpu_gcregdcproductid_t *r = (dpu_gcregdcproductid_t *)(SOC_DPU_REG_BASE + (0x803 << 2));
    return r->product_id ;
}

//reg gcregframebufferconfig:

static inline void dpu_ll_set_gcregframebufferconfig_value(uint32_t v)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregframebufferconfig_value(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregframebufferconfig_format(uint32_t v)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    r->format = v;
}

static inline uint32_t dpu_ll_get_gcregframebufferconfig_format(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    return r->format;
}

static inline uint32_t dpu_ll_get_gcregframebufferconfig_enable(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    return r->enable;
}

static inline uint32_t dpu_ll_get_gcregframebufferconfig_clear_en(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    return r->clear_en;
}

static inline uint32_t dpu_ll_get_gcregframebufferconfig_color_key_en(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    return r->color_key_en ;
}

static inline uint32_t dpu_ll_get_gcregframebufferconfig_swizzle(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    return r->swizzle;
}

static inline uint32_t dpu_ll_get_gcregframebufferconfig_uv_swizzle(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    return r->uv_swizzle;
}

static inline uint32_t dpu_ll_get_gcregframebufferconfig_dec_mode(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    return r->dec_mode;
}

static inline uint32_t dpu_ll_get_gcregframebufferconfig_rot_angle(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));
    return r->rot_angle  ;
}

//reg gcregframebufferaddress:

static inline void dpu_ll_set_gcregframebufferaddress_value(uint32_t v)
{
    dpu_gcregframebufferaddress_t *r = (dpu_gcregframebufferaddress_t *)(SOC_DPU_REG_BASE + (0x80a << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregframebufferaddress_value(void)
{
    dpu_gcregframebufferaddress_t *r = (dpu_gcregframebufferaddress_t *)(SOC_DPU_REG_BASE + (0x80a << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregframebufferaddress_address(void)
{
    dpu_gcregframebufferaddress_t *r = (dpu_gcregframebufferaddress_t *)(SOC_DPU_REG_BASE + (0x80a << 2));
    return r->address;
}

//reg gcregframebufferstride:

static inline void dpu_ll_set_gcregframebufferstride_value(uint32_t v)
{
    dpu_gcregframebufferstride_t *r = (dpu_gcregframebufferstride_t *)(SOC_DPU_REG_BASE + (0x80b << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregframebufferstride_value(void)
{
    dpu_gcregframebufferstride_t *r = (dpu_gcregframebufferstride_t *)(SOC_DPU_REG_BASE + (0x80b << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregframebufferstride_stride(void)
{
    dpu_gcregframebufferstride_t *r = (dpu_gcregframebufferstride_t *)(SOC_DPU_REG_BASE + (0x80b << 2));
    return r->stride;
}

//reg gcregdctileincfg:

static inline void dpu_ll_set_gcregdctileincfg_value(uint32_t v)
{
    dpu_gcregdctileincfg_t *r = (dpu_gcregdctileincfg_t *)(SOC_DPU_REG_BASE + (0x80d << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdctileincfg_value(void)
{
    dpu_gcregdctileincfg_t *r = (dpu_gcregdctileincfg_t *)(SOC_DPU_REG_BASE + (0x80d << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdctileincfg_tile_format(void)
{
    dpu_gcregdctileincfg_t *r = (dpu_gcregdctileincfg_t *)(SOC_DPU_REG_BASE + (0x80d << 2));
    return r->tile_format ;
}

static inline uint32_t dpu_ll_get_gcregdctileincfg_yuv_standard(void)
{
    dpu_gcregdctileincfg_t *r = (dpu_gcregdctileincfg_t *)(SOC_DPU_REG_BASE + (0x80d << 2));
    return r->yuv_standard;
}

static inline void dpu_ll_set_gcregdctileincfg_tile_format1(uint32_t v)
{
    dpu_gcregdctileincfg_t *r = (dpu_gcregdctileincfg_t *)(SOC_DPU_REG_BASE + (0x80d << 2));
    r->tile_format1 = v;
}

static inline uint32_t dpu_ll_get_gcregdctileincfg_tile_format1(void)
{
    dpu_gcregdctileincfg_t *r = (dpu_gcregdctileincfg_t *)(SOC_DPU_REG_BASE + (0x80d << 2));
    return r->tile_format1;
}

//reg gcregdctileuvframebufferadr:

static inline void dpu_ll_set_gcregdctileuvframebufferadr_value(uint32_t v)
{
    dpu_gcregdctileuvframebufferadr_t *r = (dpu_gcregdctileuvframebufferadr_t *)(SOC_DPU_REG_BASE + (0x80e << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdctileuvframebufferadr_value(void)
{
    dpu_gcregdctileuvframebufferadr_t *r = (dpu_gcregdctileuvframebufferadr_t *)(SOC_DPU_REG_BASE + (0x80e << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregdctileuvframebufferadr_address(uint32_t v)
{
    dpu_gcregdctileuvframebufferadr_t *r = (dpu_gcregdctileuvframebufferadr_t *)(SOC_DPU_REG_BASE + (0x80e << 2));
    r->address = v;
}

static inline uint32_t dpu_ll_get_gcregdctileuvframebufferadr_address(void)
{
    dpu_gcregdctileuvframebufferadr_t *r = (dpu_gcregdctileuvframebufferadr_t *)(SOC_DPU_REG_BASE + (0x80e << 2));
    return r->address;
}

//reg gcregdctileuvframebufferstr:

static inline void dpu_ll_set_gcregdctileuvframebufferstr_value(uint32_t v)
{
    dpu_gcregdctileuvframebufferstr_t *r = (dpu_gcregdctileuvframebufferstr_t *)(SOC_DPU_REG_BASE + (0x80f << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdctileuvframebufferstr_value(void)
{
    dpu_gcregdctileuvframebufferstr_t *r = (dpu_gcregdctileuvframebufferstr_t *)(SOC_DPU_REG_BASE + (0x80f << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregdctileuvframebufferstr_stride(uint32_t v)
{
    dpu_gcregdctileuvframebufferstr_t *r = (dpu_gcregdctileuvframebufferstr_t *)(SOC_DPU_REG_BASE + (0x80f << 2));
    r->stride  = v;
}

static inline uint32_t dpu_ll_get_gcregdctileuvframebufferstr_stride(void)
{
    dpu_gcregdctileuvframebufferstr_t *r = (dpu_gcregdctileuvframebufferstr_t *)(SOC_DPU_REG_BASE + (0x80f << 2));
    return r->stride ;
}

//reg gcregframebufferbackground:

static inline void dpu_ll_set_gcregframebufferbackground_value(uint32_t v)
{
    dpu_gcregframebufferbackground_t *r = (dpu_gcregframebufferbackground_t *)(SOC_DPU_REG_BASE + (0x816 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregframebufferbackground_value(void)
{
    dpu_gcregframebufferbackground_t *r = (dpu_gcregframebufferbackground_t *)(SOC_DPU_REG_BASE + (0x816 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregframebufferbackground_blue(void)
{
    dpu_gcregframebufferbackground_t *r = (dpu_gcregframebufferbackground_t *)(SOC_DPU_REG_BASE + (0x816 << 2));
    return r->blue;
}

static inline uint32_t dpu_ll_get_gcregframebufferbackground_green(void)
{
    dpu_gcregframebufferbackground_t *r = (dpu_gcregframebufferbackground_t *)(SOC_DPU_REG_BASE + (0x816 << 2));
    return r->green ;
}

static inline uint32_t dpu_ll_get_gcregframebufferbackground_red(void)
{
    dpu_gcregframebufferbackground_t *r = (dpu_gcregframebufferbackground_t *)(SOC_DPU_REG_BASE + (0x816 << 2));
    return r->red ;
}

static inline uint32_t dpu_ll_get_gcregframebufferbackground_alpha(void)
{
    dpu_gcregframebufferbackground_t *r = (dpu_gcregframebufferbackground_t *)(SOC_DPU_REG_BASE + (0x816 << 2));
    return r->alpha ;
}

//reg gcregframebuffercolorkey:

static inline void dpu_ll_set_gcregframebuffercolorkey_value(uint32_t v)
{
    dpu_gcregframebuffercolorkey_t *r = (dpu_gcregframebuffercolorkey_t *)(SOC_DPU_REG_BASE + (0x817 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkey_value(void)
{
    dpu_gcregframebuffercolorkey_t *r = (dpu_gcregframebuffercolorkey_t *)(SOC_DPU_REG_BASE + (0x817 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkey_blue(void)
{
    dpu_gcregframebuffercolorkey_t *r = (dpu_gcregframebuffercolorkey_t *)(SOC_DPU_REG_BASE + (0x817 << 2));
    return r->blue;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkey_green(void)
{
    dpu_gcregframebuffercolorkey_t *r = (dpu_gcregframebuffercolorkey_t *)(SOC_DPU_REG_BASE + (0x817 << 2));
    return r->green ;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkey_red(void)
{
    dpu_gcregframebuffercolorkey_t *r = (dpu_gcregframebuffercolorkey_t *)(SOC_DPU_REG_BASE + (0x817 << 2));
    return r->red ;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkey_alpha(void)
{
    dpu_gcregframebuffercolorkey_t *r = (dpu_gcregframebuffercolorkey_t *)(SOC_DPU_REG_BASE + (0x817 << 2));
    return r->alpha ;
}

//reg gcregframebuffercolorkeyhigh:

static inline void dpu_ll_set_gcregframebuffercolorkeyhigh_value(uint32_t v)
{
    dpu_gcregframebuffercolorkeyhigh_t *r = (dpu_gcregframebuffercolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x818 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkeyhigh_value(void)
{
    dpu_gcregframebuffercolorkeyhigh_t *r = (dpu_gcregframebuffercolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x818 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkeyhigh_blue(void)
{
    dpu_gcregframebuffercolorkeyhigh_t *r = (dpu_gcregframebuffercolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x818 << 2));
    return r->blue;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkeyhigh_green(void)
{
    dpu_gcregframebuffercolorkeyhigh_t *r = (dpu_gcregframebuffercolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x818 << 2));
    return r->green ;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkeyhigh_red(void)
{
    dpu_gcregframebuffercolorkeyhigh_t *r = (dpu_gcregframebuffercolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x818 << 2));
    return r->red ;
}

static inline uint32_t dpu_ll_get_gcregframebuffercolorkeyhigh_alpha(void)
{
    dpu_gcregframebuffercolorkeyhigh_t *r = (dpu_gcregframebuffercolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x818 << 2));
    return r->alpha ;
}

//reg gcregframebufferclearvalue:

static inline void dpu_ll_set_gcregframebufferclearvalue_value(uint32_t v)
{
    dpu_gcregframebufferclearvalue_t *r = (dpu_gcregframebufferclearvalue_t *)(SOC_DPU_REG_BASE + (0x819 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregframebufferclearvalue_value(void)
{
    dpu_gcregframebufferclearvalue_t *r = (dpu_gcregframebufferclearvalue_t *)(SOC_DPU_REG_BASE + (0x819 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregframebufferclearvalue_blue(void)
{
    dpu_gcregframebufferclearvalue_t *r = (dpu_gcregframebufferclearvalue_t *)(SOC_DPU_REG_BASE + (0x819 << 2));
    return r->blue;
}

static inline uint32_t dpu_ll_get_gcregframebufferclearvalue_green(void)
{
    dpu_gcregframebufferclearvalue_t *r = (dpu_gcregframebufferclearvalue_t *)(SOC_DPU_REG_BASE + (0x819 << 2));
    return r->green ;
}

static inline uint32_t dpu_ll_get_gcregframebufferclearvalue_red(void)
{
    dpu_gcregframebufferclearvalue_t *r = (dpu_gcregframebufferclearvalue_t *)(SOC_DPU_REG_BASE + (0x819 << 2));
    return r->red ;
}

static inline uint32_t dpu_ll_get_gcregframebufferclearvalue_alpha(void)
{
    dpu_gcregframebufferclearvalue_t *r = (dpu_gcregframebufferclearvalue_t *)(SOC_DPU_REG_BASE + (0x819 << 2));
    return r->alpha ;
}

//reg gcregvideotl:

static inline void dpu_ll_set_gcregvideotl_value(uint32_t v)
{
    dpu_gcregvideotl_t *r = (dpu_gcregvideotl_t *)(SOC_DPU_REG_BASE + (0x81a << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregvideotl_value(void)
{
    dpu_gcregvideotl_t *r = (dpu_gcregvideotl_t *)(SOC_DPU_REG_BASE + (0x81a << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregvideotl_x(uint32_t v)
{
    dpu_gcregvideotl_t *r = (dpu_gcregvideotl_t *)(SOC_DPU_REG_BASE + (0x81a << 2));
    r->x  = v;
}

static inline uint32_t dpu_ll_get_gcregvideotl_x(void)
{
    dpu_gcregvideotl_t *r = (dpu_gcregvideotl_t *)(SOC_DPU_REG_BASE + (0x81a << 2));
    return r->x ;
}

static inline void dpu_ll_set_gcregvideotl_y(uint32_t v)
{
    dpu_gcregvideotl_t *r = (dpu_gcregvideotl_t *)(SOC_DPU_REG_BASE + (0x81a << 2));
    r->y = v;
}

static inline uint32_t dpu_ll_get_gcregvideotl_y(void)
{
    dpu_gcregvideotl_t *r = (dpu_gcregvideotl_t *)(SOC_DPU_REG_BASE + (0x81a << 2));
    return r->y;
}

//reg gcregframebuffersize:

static inline void dpu_ll_set_gcregframebuffersize_value(uint32_t v)
{
    dpu_gcregframebuffersize_t *r = (dpu_gcregframebuffersize_t *)(SOC_DPU_REG_BASE + (0x81b << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregframebuffersize_value(void)
{
    dpu_gcregframebuffersize_t *r = (dpu_gcregframebuffersize_t *)(SOC_DPU_REG_BASE + (0x81b << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregframebuffersize_width(uint32_t v)
{
    dpu_gcregframebuffersize_t *r = (dpu_gcregframebuffersize_t *)(SOC_DPU_REG_BASE + (0x81b << 2));
    r->width    = v;
}

static inline uint32_t dpu_ll_get_gcregframebuffersize_width(void)
{
    dpu_gcregframebuffersize_t *r = (dpu_gcregframebuffersize_t *)(SOC_DPU_REG_BASE + (0x81b << 2));
    return r->width   ;
}

static inline uint32_t dpu_ll_get_gcregframebuffersize_height(void)
{
    dpu_gcregframebuffersize_t *r = (dpu_gcregframebuffersize_t *)(SOC_DPU_REG_BASE + (0x81b << 2));
    return r->height;
}

//reg gcregvideoglobalalpha:

static inline void dpu_ll_set_gcregvideoglobalalpha_value(uint32_t v)
{
    dpu_gcregvideoglobalalpha_t *r = (dpu_gcregvideoglobalalpha_t *)(SOC_DPU_REG_BASE + (0x81c << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregvideoglobalalpha_value(void)
{
    dpu_gcregvideoglobalalpha_t *r = (dpu_gcregvideoglobalalpha_t *)(SOC_DPU_REG_BASE + (0x81c << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregvideoglobalalpha_src_alpha(void)
{
    dpu_gcregvideoglobalalpha_t *r = (dpu_gcregvideoglobalalpha_t *)(SOC_DPU_REG_BASE + (0x81c << 2));
    return r->src_alpha ;
}

static inline uint32_t dpu_ll_get_gcregvideoglobalalpha_dst_alpha(void)
{
    dpu_gcregvideoglobalalpha_t *r = (dpu_gcregvideoglobalalpha_t *)(SOC_DPU_REG_BASE + (0x81c << 2));
    return r->dst_alpha;
}

//reg gcregblendstackorder:

static inline void dpu_ll_set_gcregblendstackorder_value(uint32_t v)
{
    dpu_gcregblendstackorder_t *r = (dpu_gcregblendstackorder_t *)(SOC_DPU_REG_BASE + (0x81d << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregblendstackorder_value(void)
{
    dpu_gcregblendstackorder_t *r = (dpu_gcregblendstackorder_t *)(SOC_DPU_REG_BASE + (0x81d << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregblendstackorder_order(void)
{
    dpu_gcregblendstackorder_t *r = (dpu_gcregblendstackorder_t *)(SOC_DPU_REG_BASE + (0x81d << 2));
    return r->order ;
}

//reg gcregvideoalphablendconfig:

static inline void dpu_ll_set_gcregvideoalphablendconfig_value(uint32_t v)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_value(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_alpha_blend(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->alpha_blend ;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_src_alpha_mode(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->src_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_src_global_alpha_mode(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->src_global_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_src_blending_mode(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->src_blending_mode;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_src_alpha_factor(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->src_alpha_factor;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_dst_alpha_mode(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->dst_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_dst_global_alpha_mode(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->dst_global_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_dst_blending_mode(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->dst_blending_mode ;
}

static inline uint32_t dpu_ll_get_gcregvideoalphablendconfig_dst_alpha_factor(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));
    return r->dst_alpha_factor ;
}

//reg gcregoverlayconfig:

static inline void dpu_ll_set_gcregoverlayconfig_value(uint32_t v)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig_value(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig_format(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    return r->format;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig_enable(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    return r->enable   ;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig_clear_en(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    return r->clear_en   ;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig_swizzle(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    return r->swizzle;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig_uv_swizzle(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    return r->uv_swizzle ;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig_color_key_en(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    return r->color_key_en ;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig_dec_mode(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    return r->dec_mode  ;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig_rot_angle(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));
    return r->rot_angle;
}

//reg gcregoverlayaddress:

static inline void dpu_ll_set_gcregoverlayaddress_value(uint32_t v)
{
    dpu_gcregoverlayaddress_t *r = (dpu_gcregoverlayaddress_t *)(SOC_DPU_REG_BASE + (0x820 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayaddress_value(void)
{
    dpu_gcregoverlayaddress_t *r = (dpu_gcregoverlayaddress_t *)(SOC_DPU_REG_BASE + (0x820 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayaddress_address(void)
{
    dpu_gcregoverlayaddress_t *r = (dpu_gcregoverlayaddress_t *)(SOC_DPU_REG_BASE + (0x820 << 2));
    return r->address  ;
}

//reg gcregoverlaystride:

static inline void dpu_ll_set_gcregoverlaystride_value(uint32_t v)
{
    dpu_gcregoverlaystride_t *r = (dpu_gcregoverlaystride_t *)(SOC_DPU_REG_BASE + (0x821 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaystride_value(void)
{
    dpu_gcregoverlaystride_t *r = (dpu_gcregoverlaystride_t *)(SOC_DPU_REG_BASE + (0x821 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlaystride_stride(void)
{
    dpu_gcregoverlaystride_t *r = (dpu_gcregoverlaystride_t *)(SOC_DPU_REG_BASE + (0x821 << 2));
    return r->stride ;
}

//reg gcregdcoverlaytileincfg:

static inline void dpu_ll_set_gcregdcoverlaytileincfg_value(uint32_t v)
{
    dpu_gcregdcoverlaytileincfg_t *r = (dpu_gcregdcoverlaytileincfg_t *)(SOC_DPU_REG_BASE + (0x822 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdcoverlaytileincfg_value(void)
{
    dpu_gcregdcoverlaytileincfg_t *r = (dpu_gcregdcoverlaytileincfg_t *)(SOC_DPU_REG_BASE + (0x822 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdcoverlaytileincfg_tile_format(void)
{
    dpu_gcregdcoverlaytileincfg_t *r = (dpu_gcregdcoverlaytileincfg_t *)(SOC_DPU_REG_BASE + (0x822 << 2));
    return r->tile_format ;
}

static inline uint32_t dpu_ll_get_gcregdcoverlaytileincfg_yuv_standard(void)
{
    dpu_gcregdcoverlaytileincfg_t *r = (dpu_gcregdcoverlaytileincfg_t *)(SOC_DPU_REG_BASE + (0x822 << 2));
    return r->yuv_standard;
}

static inline void dpu_ll_set_gcregdcoverlaytileincfg_tile_format1(uint32_t v)
{
    dpu_gcregdcoverlaytileincfg_t *r = (dpu_gcregdcoverlaytileincfg_t *)(SOC_DPU_REG_BASE + (0x822 << 2));
    r->tile_format1  = v;
}

static inline uint32_t dpu_ll_get_gcregdcoverlaytileincfg_tile_format1(void)
{
    dpu_gcregdcoverlaytileincfg_t *r = (dpu_gcregdcoverlaytileincfg_t *)(SOC_DPU_REG_BASE + (0x822 << 2));
    return r->tile_format1 ;
}

//reg gcregdctileuvoverlayadr:

static inline void dpu_ll_set_gcregdctileuvoverlayadr_value(uint32_t v)
{
    dpu_gcregdctileuvoverlayadr_t *r = (dpu_gcregdctileuvoverlayadr_t *)(SOC_DPU_REG_BASE + (0x823 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdctileuvoverlayadr_value(void)
{
    dpu_gcregdctileuvoverlayadr_t *r = (dpu_gcregdctileuvoverlayadr_t *)(SOC_DPU_REG_BASE + (0x823 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdctileuvoverlayadr_address(void)
{
    dpu_gcregdctileuvoverlayadr_t *r = (dpu_gcregdctileuvoverlayadr_t *)(SOC_DPU_REG_BASE + (0x823 << 2));
    return r->address;
}

//reg gcregdctileuvoverlaystr:

static inline void dpu_ll_set_gcregdctileuvoverlaystr_value(uint32_t v)
{
    dpu_gcregdctileuvoverlaystr_t *r = (dpu_gcregdctileuvoverlaystr_t *)(SOC_DPU_REG_BASE + (0x824 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdctileuvoverlaystr_value(void)
{
    dpu_gcregdctileuvoverlaystr_t *r = (dpu_gcregdctileuvoverlaystr_t *)(SOC_DPU_REG_BASE + (0x824 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdctileuvoverlaystr_stride(void)
{
    dpu_gcregdctileuvoverlaystr_t *r = (dpu_gcregdctileuvoverlaystr_t *)(SOC_DPU_REG_BASE + (0x824 << 2));
    return r->stride;
}

//reg gcregoverlaytl:

static inline void dpu_ll_set_gcregoverlaytl_value(uint32_t v)
{
    dpu_gcregoverlaytl_t *r = (dpu_gcregoverlaytl_t *)(SOC_DPU_REG_BASE + (0x825 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaytl_value(void)
{
    dpu_gcregoverlaytl_t *r = (dpu_gcregoverlaytl_t *)(SOC_DPU_REG_BASE + (0x825 << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregoverlaytl_x(uint32_t v)
{
    dpu_gcregoverlaytl_t *r = (dpu_gcregoverlaytl_t *)(SOC_DPU_REG_BASE + (0x825 << 2));
    r->x   = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaytl_x(void)
{
    dpu_gcregoverlaytl_t *r = (dpu_gcregoverlaytl_t *)(SOC_DPU_REG_BASE + (0x825 << 2));
    return r->x  ;
}

static inline void dpu_ll_set_gcregoverlaytl_y(uint32_t v)
{
    dpu_gcregoverlaytl_t *r = (dpu_gcregoverlaytl_t *)(SOC_DPU_REG_BASE + (0x825 << 2));
    r->y   = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaytl_y(void)
{
    dpu_gcregoverlaytl_t *r = (dpu_gcregoverlaytl_t *)(SOC_DPU_REG_BASE + (0x825 << 2));
    return r->y  ;
}

//reg gcregoverlaysize:

static inline void dpu_ll_set_gcregoverlaysize_value(uint32_t v)
{
    dpu_gcregoverlaysize_t *r = (dpu_gcregoverlaysize_t *)(SOC_DPU_REG_BASE + (0x826 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaysize_value(void)
{
    dpu_gcregoverlaysize_t *r = (dpu_gcregoverlaysize_t *)(SOC_DPU_REG_BASE + (0x826 << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregoverlaysize_width(uint32_t v)
{
    dpu_gcregoverlaysize_t *r = (dpu_gcregoverlaysize_t *)(SOC_DPU_REG_BASE + (0x826 << 2));
    r->width   = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaysize_width(void)
{
    dpu_gcregoverlaysize_t *r = (dpu_gcregoverlaysize_t *)(SOC_DPU_REG_BASE + (0x826 << 2));
    return r->width  ;
}

static inline uint32_t dpu_ll_get_gcregoverlaysize_height(void)
{
    dpu_gcregoverlaysize_t *r = (dpu_gcregoverlaysize_t *)(SOC_DPU_REG_BASE + (0x826 << 2));
    return r->height ;
}

//reg gcregoverlaycolorkey:

static inline void dpu_ll_set_gcregoverlaycolorkey_value(uint32_t v)
{
    dpu_gcregoverlaycolorkey_t *r = (dpu_gcregoverlaycolorkey_t *)(SOC_DPU_REG_BASE + (0x827 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkey_value(void)
{
    dpu_gcregoverlaycolorkey_t *r = (dpu_gcregoverlaycolorkey_t *)(SOC_DPU_REG_BASE + (0x827 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkey_blue(void)
{
    dpu_gcregoverlaycolorkey_t *r = (dpu_gcregoverlaycolorkey_t *)(SOC_DPU_REG_BASE + (0x827 << 2));
    return r->blue   ;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkey_green(void)
{
    dpu_gcregoverlaycolorkey_t *r = (dpu_gcregoverlaycolorkey_t *)(SOC_DPU_REG_BASE + (0x827 << 2));
    return r->green;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkey_red(void)
{
    dpu_gcregoverlaycolorkey_t *r = (dpu_gcregoverlaycolorkey_t *)(SOC_DPU_REG_BASE + (0x827 << 2));
    return r->red;
}

//reg gcregoverlaycolorkeyhigh:

static inline void dpu_ll_set_gcregoverlaycolorkeyhigh_value(uint32_t v)
{
    dpu_gcregoverlaycolorkeyhigh_t *r = (dpu_gcregoverlaycolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x828 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkeyhigh_value(void)
{
    dpu_gcregoverlaycolorkeyhigh_t *r = (dpu_gcregoverlaycolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x828 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkeyhigh_blue(void)
{
    dpu_gcregoverlaycolorkeyhigh_t *r = (dpu_gcregoverlaycolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x828 << 2));
    return r->blue   ;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkeyhigh_green(void)
{
    dpu_gcregoverlaycolorkeyhigh_t *r = (dpu_gcregoverlaycolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x828 << 2));
    return r->green;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkeyhigh_red(void)
{
    dpu_gcregoverlaycolorkeyhigh_t *r = (dpu_gcregoverlaycolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x828 << 2));
    return r->red;
}

//reg gcregoverlayalphablendconfig:

static inline void dpu_ll_set_gcregoverlayalphablendconfig_value(uint32_t v)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_value(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_alpha_blend(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->alpha_blend ;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_src_alpha_mode(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->src_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_src_global_alpha_mode(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->src_global_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_src_blending_mode(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->src_blending_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_src_alpha_factor(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->src_alpha_factor;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_dst_alpha_mode(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->dst_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_dst_global_alpha_mode(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->dst_global_alpha_mode                  ;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_dst_blending_mode(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->dst_blending_mode       ;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig_dst_alpha_factor(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));
    return r->dst_alpha_factor;
}

//reg gcregoverlayglobalalpha:

static inline void dpu_ll_set_gcregoverlayglobalalpha_value(uint32_t v)
{
    dpu_gcregoverlayglobalalpha_t *r = (dpu_gcregoverlayglobalalpha_t *)(SOC_DPU_REG_BASE + (0x82a << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayglobalalpha_value(void)
{
    dpu_gcregoverlayglobalalpha_t *r = (dpu_gcregoverlayglobalalpha_t *)(SOC_DPU_REG_BASE + (0x82a << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayglobalalpha_src_alpha(void)
{
    dpu_gcregoverlayglobalalpha_t *r = (dpu_gcregoverlayglobalalpha_t *)(SOC_DPU_REG_BASE + (0x82a << 2));
    return r->src_alpha;
}

static inline uint32_t dpu_ll_get_gcregoverlayglobalalpha_dst_alpha(void)
{
    dpu_gcregoverlayglobalalpha_t *r = (dpu_gcregoverlayglobalalpha_t *)(SOC_DPU_REG_BASE + (0x82a << 2));
    return r->dst_alpha;
}

//reg gcregoverlayclearvalue:

static inline void dpu_ll_set_gcregoverlayclearvalue_value(uint32_t v)
{
    dpu_gcregoverlayclearvalue_t *r = (dpu_gcregoverlayclearvalue_t *)(SOC_DPU_REG_BASE + (0x82b << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayclearvalue_value(void)
{
    dpu_gcregoverlayclearvalue_t *r = (dpu_gcregoverlayclearvalue_t *)(SOC_DPU_REG_BASE + (0x82b << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayclearvalue_blue(void)
{
    dpu_gcregoverlayclearvalue_t *r = (dpu_gcregoverlayclearvalue_t *)(SOC_DPU_REG_BASE + (0x82b << 2));
    return r->blue;
}

static inline uint32_t dpu_ll_get_gcregoverlayclearvalue_green(void)
{
    dpu_gcregoverlayclearvalue_t *r = (dpu_gcregoverlayclearvalue_t *)(SOC_DPU_REG_BASE + (0x82b << 2));
    return r->green;
}

static inline uint32_t dpu_ll_get_gcregoverlayclearvalue_red(void)
{
    dpu_gcregoverlayclearvalue_t *r = (dpu_gcregoverlayclearvalue_t *)(SOC_DPU_REG_BASE + (0x82b << 2));
    return r->red ;
}

//reg gcregoverlayconfig1:

static inline void dpu_ll_set_gcregoverlayconfig1_value(uint32_t v)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig1_value(void)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig1_format(void)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));
    return r->format;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig1_enable(void)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));
    return r->enable;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig1_clear_en(void)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));
    return r->clear_en;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig1_swizzle(void)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));
    return r->swizzle;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig1_color_key_en(void)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));
    return r->color_key_en;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig1_dec_mode(void)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));
    return r->dec_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayconfig1_rot_angle(void)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));
    return r->rot_angle;
}

//reg gcregoverlayaddress1:

static inline void dpu_ll_set_gcregoverlayaddress1_value(uint32_t v)
{
    dpu_gcregoverlayaddress1_t *r = (dpu_gcregoverlayaddress1_t *)(SOC_DPU_REG_BASE + (0x82d << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayaddress1_value(void)
{
    dpu_gcregoverlayaddress1_t *r = (dpu_gcregoverlayaddress1_t *)(SOC_DPU_REG_BASE + (0x82d << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayaddress1_address(void)
{
    dpu_gcregoverlayaddress1_t *r = (dpu_gcregoverlayaddress1_t *)(SOC_DPU_REG_BASE + (0x82d << 2));
    return r->address;
}

//reg gcregoverlaystride1:

static inline void dpu_ll_set_gcregoverlaystride1_value(uint32_t v)
{
    dpu_gcregoverlaystride1_t *r = (dpu_gcregoverlaystride1_t *)(SOC_DPU_REG_BASE + (0x82e << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaystride1_value(void)
{
    dpu_gcregoverlaystride1_t *r = (dpu_gcregoverlaystride1_t *)(SOC_DPU_REG_BASE + (0x82e << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlaystride1_stride(void)
{
    dpu_gcregoverlaystride1_t *r = (dpu_gcregoverlaystride1_t *)(SOC_DPU_REG_BASE + (0x82e << 2));
    return r->stride;
}

//reg gcregoverlaytl1:

static inline void dpu_ll_set_gcregoverlaytl1_value(uint32_t v)
{
    dpu_gcregoverlaytl1_t *r = (dpu_gcregoverlaytl1_t *)(SOC_DPU_REG_BASE + (0x82f << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaytl1_value(void)
{
    dpu_gcregoverlaytl1_t *r = (dpu_gcregoverlaytl1_t *)(SOC_DPU_REG_BASE + (0x82f << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregoverlaytl1_x(uint32_t v)
{
    dpu_gcregoverlaytl1_t *r = (dpu_gcregoverlaytl1_t *)(SOC_DPU_REG_BASE + (0x82f << 2));
    r->x = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaytl1_x(void)
{
    dpu_gcregoverlaytl1_t *r = (dpu_gcregoverlaytl1_t *)(SOC_DPU_REG_BASE + (0x82f << 2));
    return r->x;
}

static inline void dpu_ll_set_gcregoverlaytl1_y(uint32_t v)
{
    dpu_gcregoverlaytl1_t *r = (dpu_gcregoverlaytl1_t *)(SOC_DPU_REG_BASE + (0x82f << 2));
    r->y = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaytl1_y(void)
{
    dpu_gcregoverlaytl1_t *r = (dpu_gcregoverlaytl1_t *)(SOC_DPU_REG_BASE + (0x82f << 2));
    return r->y;
}

//reg gcregoverlaysize1:

static inline void dpu_ll_set_gcregoverlaysize1_value(uint32_t v)
{
    dpu_gcregoverlaysize1_t *r = (dpu_gcregoverlaysize1_t *)(SOC_DPU_REG_BASE + (0x830 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaysize1_value(void)
{
    dpu_gcregoverlaysize1_t *r = (dpu_gcregoverlaysize1_t *)(SOC_DPU_REG_BASE + (0x830 << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregoverlaysize1_width(uint32_t v)
{
    dpu_gcregoverlaysize1_t *r = (dpu_gcregoverlaysize1_t *)(SOC_DPU_REG_BASE + (0x830 << 2));
    r->width = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaysize1_width(void)
{
    dpu_gcregoverlaysize1_t *r = (dpu_gcregoverlaysize1_t *)(SOC_DPU_REG_BASE + (0x830 << 2));
    return r->width;
}

static inline uint32_t dpu_ll_get_gcregoverlaysize1_height(void)
{
    dpu_gcregoverlaysize1_t *r = (dpu_gcregoverlaysize1_t *)(SOC_DPU_REG_BASE + (0x830 << 2));
    return r->height;
}

//reg gcregoverlaycolorkey1:

static inline void dpu_ll_set_gcregoverlaycolorkey1_value(uint32_t v)
{
    dpu_gcregoverlaycolorkey1_t *r = (dpu_gcregoverlaycolorkey1_t *)(SOC_DPU_REG_BASE + (0x831 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkey1_value(void)
{
    dpu_gcregoverlaycolorkey1_t *r = (dpu_gcregoverlaycolorkey1_t *)(SOC_DPU_REG_BASE + (0x831 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkey1_blue(void)
{
    dpu_gcregoverlaycolorkey1_t *r = (dpu_gcregoverlaycolorkey1_t *)(SOC_DPU_REG_BASE + (0x831 << 2));
    return r->blue;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkey1_green(void)
{
    dpu_gcregoverlaycolorkey1_t *r = (dpu_gcregoverlaycolorkey1_t *)(SOC_DPU_REG_BASE + (0x831 << 2));
    return r->green;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkey1_red(void)
{
    dpu_gcregoverlaycolorkey1_t *r = (dpu_gcregoverlaycolorkey1_t *)(SOC_DPU_REG_BASE + (0x831 << 2));
    return r->red;
}

//reg gcregoverlaycolorkeyhigh1:

static inline void dpu_ll_set_gcregoverlaycolorkeyhigh1_value(uint32_t v)
{
    dpu_gcregoverlaycolorkeyhigh1_t *r = (dpu_gcregoverlaycolorkeyhigh1_t *)(SOC_DPU_REG_BASE + (0x832 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkeyhigh1_value(void)
{
    dpu_gcregoverlaycolorkeyhigh1_t *r = (dpu_gcregoverlaycolorkeyhigh1_t *)(SOC_DPU_REG_BASE + (0x832 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkeyhigh1_blue(void)
{
    dpu_gcregoverlaycolorkeyhigh1_t *r = (dpu_gcregoverlaycolorkeyhigh1_t *)(SOC_DPU_REG_BASE + (0x832 << 2));
    return r->blue;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkeyhigh1_green(void)
{
    dpu_gcregoverlaycolorkeyhigh1_t *r = (dpu_gcregoverlaycolorkeyhigh1_t *)(SOC_DPU_REG_BASE + (0x832 << 2));
    return r->green;
}

static inline uint32_t dpu_ll_get_gcregoverlaycolorkeyhigh1_red(void)
{
    dpu_gcregoverlaycolorkeyhigh1_t *r = (dpu_gcregoverlaycolorkeyhigh1_t *)(SOC_DPU_REG_BASE + (0x832 << 2));
    return r->red;
}

//reg gcregoverlayalphablendconfig1:

static inline void dpu_ll_set_gcregoverlayalphablendconfig1_value(uint32_t v)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_value(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_alpha_blend(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->alpha_blend;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_src_alpha_mode(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->src_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_src_global_alpha_mode(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->src_global_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_src_blending_mode(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->src_blending_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_src_alpha_factor(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->src_alpha_factor;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_dst_alpha_mode(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->dst_alpha_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_dst_global_alpha_mode(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->dst_global_alpha_mode                  ;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_dst_blending_mode(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->dst_blending_mode;
}

static inline uint32_t dpu_ll_get_gcregoverlayalphablendconfig1_dst_alpha_factor(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));
    return r->dst_alpha_factor;
}

//reg gcregoverlayglobalalpha1:

static inline void dpu_ll_set_gcregoverlayglobalalpha1_value(uint32_t v)
{
    dpu_gcregoverlayglobalalpha1_t *r = (dpu_gcregoverlayglobalalpha1_t *)(SOC_DPU_REG_BASE + (0x834 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayglobalalpha1_value(void)
{
    dpu_gcregoverlayglobalalpha1_t *r = (dpu_gcregoverlayglobalalpha1_t *)(SOC_DPU_REG_BASE + (0x834 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayglobalalpha1_src_alpha(void)
{
    dpu_gcregoverlayglobalalpha1_t *r = (dpu_gcregoverlayglobalalpha1_t *)(SOC_DPU_REG_BASE + (0x834 << 2));
    return r->src_alpha ;
}

static inline uint32_t dpu_ll_get_gcregoverlayglobalalpha1_dst_alpha(void)
{
    dpu_gcregoverlayglobalalpha1_t *r = (dpu_gcregoverlayglobalalpha1_t *)(SOC_DPU_REG_BASE + (0x834 << 2));
    return r->dst_alpha;
}

//reg gcregoverlayclearvalue1:

static inline void dpu_ll_set_gcregoverlayclearvalue1_value(uint32_t v)
{
    dpu_gcregoverlayclearvalue1_t *r = (dpu_gcregoverlayclearvalue1_t *)(SOC_DPU_REG_BASE + (0x835 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregoverlayclearvalue1_value(void)
{
    dpu_gcregoverlayclearvalue1_t *r = (dpu_gcregoverlayclearvalue1_t *)(SOC_DPU_REG_BASE + (0x835 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregoverlayclearvalue1_blue(void)
{
    dpu_gcregoverlayclearvalue1_t *r = (dpu_gcregoverlayclearvalue1_t *)(SOC_DPU_REG_BASE + (0x835 << 2));
    return r->blue  ;
}

static inline uint32_t dpu_ll_get_gcregoverlayclearvalue1_green(void)
{
    dpu_gcregoverlayclearvalue1_t *r = (dpu_gcregoverlayclearvalue1_t *)(SOC_DPU_REG_BASE + (0x835 << 2));
    return r->green;
}

static inline uint32_t dpu_ll_get_gcregoverlayclearvalue1_red(void)
{
    dpu_gcregoverlayclearvalue1_t *r = (dpu_gcregoverlayclearvalue1_t *)(SOC_DPU_REG_BASE + (0x835 << 2));
    return r->red ;
}

//reg gcregdisplaydithertablelow:

static inline void dpu_ll_set_gcregdisplaydithertablelow_value(uint32_t v)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablelow_value(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablelow_y0_x0(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    return r->y0_x0;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablelow_y0_x1(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    return r->y0_x1;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablelow_y0_x2(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    return r->y0_x2;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablelow_y0_x3(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    return r->y0_x3;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablelow_y1_x0(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    return r->y1_x0;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablelow_y1_x1(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    return r->y1_x1;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablelow_y1_x2(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    return r->y1_x2;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablelow_y1_x3(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));
    return r->y1_x3;
}

//reg gcregdisplaydithertablehigh:

static inline void dpu_ll_set_gcregdisplaydithertablehigh_value(uint32_t v)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablehigh_value(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablehigh_y2_x0(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    return r->y2_x0  ;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablehigh_y2_x1(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    return r->y2_x1;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablehigh_y2_x2(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    return r->y2_x2;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablehigh_y2_x3(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    return r->y2_x3;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablehigh_y3_x0(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    return r->y3_x0;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablehigh_y3_x1(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    return r->y3_x1;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablehigh_y3_x2(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    return r->y3_x2;
}

static inline uint32_t dpu_ll_get_gcregdisplaydithertablehigh_y3_x3(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));
    return r->y3_x3;
}

//reg gcregpanelconfig:

static inline void dpu_ll_set_gcregpanelconfig_value(uint32_t v)
{
    dpu_gcregpanelconfig_t *r = (dpu_gcregpanelconfig_t *)(SOC_DPU_REG_BASE + (0x83a << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregpanelconfig_value(void)
{
    dpu_gcregpanelconfig_t *r = (dpu_gcregpanelconfig_t *)(SOC_DPU_REG_BASE + (0x83a << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregpanelconfig_de_polarity(void)
{
    dpu_gcregpanelconfig_t *r = (dpu_gcregpanelconfig_t *)(SOC_DPU_REG_BASE + (0x83a << 2));
    return r->de_polarity;
}

static inline uint32_t dpu_ll_get_gcregpanelconfig_data_polarity(void)
{
    dpu_gcregpanelconfig_t *r = (dpu_gcregpanelconfig_t *)(SOC_DPU_REG_BASE + (0x83a << 2));
    return r->data_polarity;
}

static inline uint32_t dpu_ll_get_gcregpanelconfig_clock_polarity(void)
{
    dpu_gcregpanelconfig_t *r = (dpu_gcregpanelconfig_t *)(SOC_DPU_REG_BASE + (0x83a << 2));
    return r->clock_polarity ;
}

//reg gcregpanelcontrol:

static inline void dpu_ll_set_gcregpanelcontrol_value(uint32_t v)
{
    dpu_gcregpanelcontrol_t *r = (dpu_gcregpanelcontrol_t *)(SOC_DPU_REG_BASE + (0x83b << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregpanelcontrol_value(void)
{
    dpu_gcregpanelcontrol_t *r = (dpu_gcregpanelcontrol_t *)(SOC_DPU_REG_BASE + (0x83b << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregpanelcontrol_valid(uint32_t v)
{
    dpu_gcregpanelcontrol_t *r = (dpu_gcregpanelcontrol_t *)(SOC_DPU_REG_BASE + (0x83b << 2));
    r->valid = v;
}

static inline uint32_t dpu_ll_get_gcregpanelcontrol_valid(void)
{
    dpu_gcregpanelcontrol_t *r = (dpu_gcregpanelcontrol_t *)(SOC_DPU_REG_BASE + (0x83b << 2));
    return r->valid;
}

static inline uint32_t dpu_ll_get_gcregpanelcontrol_back_pressure_disab(void)
{
    dpu_gcregpanelcontrol_t *r = (dpu_gcregpanelcontrol_t *)(SOC_DPU_REG_BASE + (0x83b << 2));
    return r->back_pressure_disab;
}

//reg gcregpanelfunction:

static inline void dpu_ll_set_gcregpanelfunction_value(uint32_t v)
{
    dpu_gcregpanelfunction_t *r = (dpu_gcregpanelfunction_t *)(SOC_DPU_REG_BASE + (0x83c << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregpanelfunction_value(void)
{
    dpu_gcregpanelfunction_t *r = (dpu_gcregpanelfunction_t *)(SOC_DPU_REG_BASE + (0x83c << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregpanelfunction_output(uint32_t v)
{
    dpu_gcregpanelfunction_t *r = (dpu_gcregpanelfunction_t *)(SOC_DPU_REG_BASE + (0x83c << 2));
    r->output = v;
}

static inline uint32_t dpu_ll_get_gcregpanelfunction_output(void)
{
    dpu_gcregpanelfunction_t *r = (dpu_gcregpanelfunction_t *)(SOC_DPU_REG_BASE + (0x83c << 2));
    return r->output;
}

static inline void dpu_ll_set_gcregpanelfunction_gamma(uint32_t v)
{
    dpu_gcregpanelfunction_t *r = (dpu_gcregpanelfunction_t *)(SOC_DPU_REG_BASE + (0x83c << 2));
    r->gamma = v;
}

static inline uint32_t dpu_ll_get_gcregpanelfunction_gamma(void)
{
    dpu_gcregpanelfunction_t *r = (dpu_gcregpanelfunction_t *)(SOC_DPU_REG_BASE + (0x83c << 2));
    return r->gamma;
}

static inline void dpu_ll_set_gcregpanelfunction_dither(uint32_t v)
{
    dpu_gcregpanelfunction_t *r = (dpu_gcregpanelfunction_t *)(SOC_DPU_REG_BASE + (0x83c << 2));
    r->dither = v;
}

static inline uint32_t dpu_ll_get_gcregpanelfunction_dither(void)
{
    dpu_gcregpanelfunction_t *r = (dpu_gcregpanelfunction_t *)(SOC_DPU_REG_BASE + (0x83c << 2));
    return r->dither;
}

//reg gcregpanelworking:

static inline void dpu_ll_set_gcregpanelworking_value(uint32_t v)
{
    dpu_gcregpanelworking_t *r = (dpu_gcregpanelworking_t *)(SOC_DPU_REG_BASE + (0x83d << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregpanelworking_value(void)
{
    dpu_gcregpanelworking_t *r = (dpu_gcregpanelworking_t *)(SOC_DPU_REG_BASE + (0x83d << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregpanelworking_working(uint32_t v)
{
    dpu_gcregpanelworking_t *r = (dpu_gcregpanelworking_t *)(SOC_DPU_REG_BASE + (0x83d << 2));
    r->working = v;
}

static inline uint32_t dpu_ll_get_gcregpanelworking_working(void)
{
    dpu_gcregpanelworking_t *r = (dpu_gcregpanelworking_t *)(SOC_DPU_REG_BASE + (0x83d << 2));
    return r->working;
}

//reg gcregpanelstate:

static inline void dpu_ll_set_gcregpanelstate_value(uint32_t v)
{
    dpu_gcregpanelstate_t *r = (dpu_gcregpanelstate_t *)(SOC_DPU_REG_BASE + (0x83e << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregpanelstate_value(void)
{
    dpu_gcregpanelstate_t *r = (dpu_gcregpanelstate_t *)(SOC_DPU_REG_BASE + (0x83e << 2));
    return r->v;
}

static inline void dpu_ll_set_gcregpanelstate_video_under_flow(uint32_t v)
{
    dpu_gcregpanelstate_t *r = (dpu_gcregpanelstate_t *)(SOC_DPU_REG_BASE + (0x83e << 2));
    r->video_under_flow = v;
}

static inline uint32_t dpu_ll_get_gcregpanelstate_video_under_flow(void)
{
    dpu_gcregpanelstate_t *r = (dpu_gcregpanelstate_t *)(SOC_DPU_REG_BASE + (0x83e << 2));
    return r->video_under_flow;
}

static inline void dpu_ll_set_gcregpanelstate_overlay_under_flow(uint32_t v)
{
    dpu_gcregpanelstate_t *r = (dpu_gcregpanelstate_t *)(SOC_DPU_REG_BASE + (0x83e << 2));
    r->overlay_under_flow = v;
}

static inline uint32_t dpu_ll_get_gcregpanelstate_overlay_under_flow(void)
{
    dpu_gcregpanelstate_t *r = (dpu_gcregpanelstate_t *)(SOC_DPU_REG_BASE + (0x83e << 2));
    return r->overlay_under_flow;
}

static inline void dpu_ll_set_gcregpanelstate_overlay_under_flow1(uint32_t v)
{
    dpu_gcregpanelstate_t *r = (dpu_gcregpanelstate_t *)(SOC_DPU_REG_BASE + (0x83e << 2));
    r->overlay_under_flow1 = v;
}

static inline uint32_t dpu_ll_get_gcregpanelstate_overlay_under_flow1(void)
{
    dpu_gcregpanelstate_t *r = (dpu_gcregpanelstate_t *)(SOC_DPU_REG_BASE + (0x83e << 2));
    return r->overlay_under_flow1;
}

//reg gcreghdisplay:

static inline void dpu_ll_set_gcreghdisplay_value(uint32_t v)
{
    dpu_gcreghdisplay_t *r = (dpu_gcreghdisplay_t *)(SOC_DPU_REG_BASE + (0x840 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcreghdisplay_value(void)
{
    dpu_gcreghdisplay_t *r = (dpu_gcreghdisplay_t *)(SOC_DPU_REG_BASE + (0x840 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcreghdisplay_display_end(void)
{
    dpu_gcreghdisplay_t *r = (dpu_gcreghdisplay_t *)(SOC_DPU_REG_BASE + (0x840 << 2));
    return r->display_end;
}

static inline uint32_t dpu_ll_get_gcreghdisplay_total(void)
{
    dpu_gcreghdisplay_t *r = (dpu_gcreghdisplay_t *)(SOC_DPU_REG_BASE + (0x840 << 2));
    return r->total;
}

//reg gcreghsync:

static inline void dpu_ll_set_gcreghsync_value(uint32_t v)
{
    dpu_gcreghsync_t *r = (dpu_gcreghsync_t *)(SOC_DPU_REG_BASE + (0x841 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcreghsync_value(void)
{
    dpu_gcreghsync_t *r = (dpu_gcreghsync_t *)(SOC_DPU_REG_BASE + (0x841 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcreghsync_start(void)
{
    dpu_gcreghsync_t *r = (dpu_gcreghsync_t *)(SOC_DPU_REG_BASE + (0x841 << 2));
    return r->start;
}

static inline uint32_t dpu_ll_get_gcreghsync_end(void)
{
    dpu_gcreghsync_t *r = (dpu_gcreghsync_t *)(SOC_DPU_REG_BASE + (0x841 << 2));
    return r->end;
}

static inline uint32_t dpu_ll_get_gcreghsync_pulse(void)
{
    dpu_gcreghsync_t *r = (dpu_gcreghsync_t *)(SOC_DPU_REG_BASE + (0x841 << 2));
    return r->pulse;
}

static inline uint32_t dpu_ll_get_gcreghsync_polarity(void)
{
    dpu_gcreghsync_t *r = (dpu_gcreghsync_t *)(SOC_DPU_REG_BASE + (0x841 << 2));
    return r->polarity;
}

//reg gcregvdisplay:

static inline void dpu_ll_set_gcregvdisplay_value(uint32_t v)
{
    dpu_gcregvdisplay_t *r = (dpu_gcregvdisplay_t *)(SOC_DPU_REG_BASE + (0x844 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregvdisplay_value(void)
{
    dpu_gcregvdisplay_t *r = (dpu_gcregvdisplay_t *)(SOC_DPU_REG_BASE + (0x844 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregvdisplay_display_end(void)
{
    dpu_gcregvdisplay_t *r = (dpu_gcregvdisplay_t *)(SOC_DPU_REG_BASE + (0x844 << 2));
    return r->display_end ;
}

static inline uint32_t dpu_ll_get_gcregvdisplay_total(void)
{
    dpu_gcregvdisplay_t *r = (dpu_gcregvdisplay_t *)(SOC_DPU_REG_BASE + (0x844 << 2));
    return r->total;
}

//reg gcregvsync:

static inline void dpu_ll_set_gcregvsync_value(uint32_t v)
{
    dpu_gcregvsync_t *r = (dpu_gcregvsync_t *)(SOC_DPU_REG_BASE + (0x845 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregvsync_value(void)
{
    dpu_gcregvsync_t *r = (dpu_gcregvsync_t *)(SOC_DPU_REG_BASE + (0x845 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregvsync_start(void)
{
    dpu_gcregvsync_t *r = (dpu_gcregvsync_t *)(SOC_DPU_REG_BASE + (0x845 << 2));
    return r->start;
}

static inline uint32_t dpu_ll_get_gcregvsync_end(void)
{
    dpu_gcregvsync_t *r = (dpu_gcregvsync_t *)(SOC_DPU_REG_BASE + (0x845 << 2));
    return r->end;
}

static inline uint32_t dpu_ll_get_gcregvsync_pulse(void)
{
    dpu_gcregvsync_t *r = (dpu_gcregvsync_t *)(SOC_DPU_REG_BASE + (0x845 << 2));
    return r->pulse;
}

static inline uint32_t dpu_ll_get_gcregvsync_
polarity(void)
{
    dpu_gcregvsync_t *r = (dpu_gcregvsync_t *)(SOC_DPU_REG_BASE + (0x845 << 2));
    return r->
           polarity;
}

//reg gcregdisplaycurrentlocation:

static inline void dpu_ll_set_gcregdisplaycurrentlocation_value(uint32_t v)
{
    dpu_gcregdisplaycurrentlocation_t *r = (dpu_gcregdisplaycurrentlocation_t *)(SOC_DPU_REG_BASE + (0x846 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdisplaycurrentlocation_value(void)
{
    dpu_gcregdisplaycurrentlocation_t *r = (dpu_gcregdisplaycurrentlocation_t *)(SOC_DPU_REG_BASE + (0x846 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdisplaycurrentlocation_x(void)
{
    dpu_gcregdisplaycurrentlocation_t *r = (dpu_gcregdisplaycurrentlocation_t *)(SOC_DPU_REG_BASE + (0x846 << 2));
    return r->x ;
}

static inline uint32_t dpu_ll_get_gcregdisplaycurrentlocation_y(void)
{
    dpu_gcregdisplaycurrentlocation_t *r = (dpu_gcregdisplaycurrentlocation_t *)(SOC_DPU_REG_BASE + (0x846 << 2));
    return r->y ;
}

//reg gcreggammaindex:

static inline void dpu_ll_set_gcreggammaindex_value(uint32_t v)
{
    dpu_gcreggammaindex_t *r = (dpu_gcreggammaindex_t *)(SOC_DPU_REG_BASE + (0x847 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcreggammaindex_value(void)
{
    dpu_gcreggammaindex_t *r = (dpu_gcreggammaindex_t *)(SOC_DPU_REG_BASE + (0x847 << 2));
    return r->v;
}

//reg gcreggammadata:

static inline void dpu_ll_set_gcreggammadata_value(uint32_t v)
{
    dpu_gcreggammadata_t *r = (dpu_gcreggammadata_t *)(SOC_DPU_REG_BASE + (0x848 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcreggammadata_value(void)
{
    dpu_gcreggammadata_t *r = (dpu_gcreggammadata_t *)(SOC_DPU_REG_BASE + (0x848 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcreggammadata_blue(void)
{
    dpu_gcreggammadata_t *r = (dpu_gcreggammadata_t *)(SOC_DPU_REG_BASE + (0x848 << 2));
    return r->blue   ;
}

static inline uint32_t dpu_ll_get_gcreggammadata_green(void)
{
    dpu_gcreggammadata_t *r = (dpu_gcreggammadata_t *)(SOC_DPU_REG_BASE + (0x848 << 2));
    return r->green;
}

static inline uint32_t dpu_ll_get_gcreggammadata_red(void)
{
    dpu_gcreggammadata_t *r = (dpu_gcreggammadata_t *)(SOC_DPU_REG_BASE + (0x848 << 2));
    return r->red ;
}

//reg gcregcursorconfig:

static inline void dpu_ll_set_gcregcursorconfig_value(uint32_t v)
{
    dpu_gcregcursorconfig_t *r = (dpu_gcregcursorconfig_t *)(SOC_DPU_REG_BASE + (0x849 << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregcursorconfig_value(void)
{
    dpu_gcregcursorconfig_t *r = (dpu_gcregcursorconfig_t *)(SOC_DPU_REG_BASE + (0x849 << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregcursorconfig_format(void)
{
    dpu_gcregcursorconfig_t *r = (dpu_gcregcursorconfig_t *)(SOC_DPU_REG_BASE + (0x849 << 2));
    return r->format;
}

static inline uint32_t dpu_ll_get_gcregcursorconfig_hot_spot_y(void)
{
    dpu_gcregcursorconfig_t *r = (dpu_gcregcursorconfig_t *)(SOC_DPU_REG_BASE + (0x849 << 2));
    return r->hot_spot_y ;
}

static inline uint32_t dpu_ll_get_gcregcursorconfig_hot_spot_x(void)
{
    dpu_gcregcursorconfig_t *r = (dpu_gcregcursorconfig_t *)(SOC_DPU_REG_BASE + (0x849 << 2));
    return r->hot_spot_x;
}

//reg gcregcursoraddress:

static inline void dpu_ll_set_gcregcursoraddress_value(uint32_t v)
{
    dpu_gcregcursoraddress_t *r = (dpu_gcregcursoraddress_t *)(SOC_DPU_REG_BASE + (0x84a << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregcursoraddress_value(void)
{
    dpu_gcregcursoraddress_t *r = (dpu_gcregcursoraddress_t *)(SOC_DPU_REG_BASE + (0x84a << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregcursoraddress_address(void)
{
    dpu_gcregcursoraddress_t *r = (dpu_gcregcursoraddress_t *)(SOC_DPU_REG_BASE + (0x84a << 2));
    return r->address;
}

//reg gcregcursorlocation:

static inline void dpu_ll_set_gcregcursorlocation_value(uint32_t v)
{
    dpu_gcregcursorlocation_t *r = (dpu_gcregcursorlocation_t *)(SOC_DPU_REG_BASE + (0x84b << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregcursorlocation_value(void)
{
    dpu_gcregcursorlocation_t *r = (dpu_gcregcursorlocation_t *)(SOC_DPU_REG_BASE + (0x84b << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregcursorlocation_x(void)
{
    dpu_gcregcursorlocation_t *r = (dpu_gcregcursorlocation_t *)(SOC_DPU_REG_BASE + (0x84b << 2));
    return r->x;
}

static inline uint32_t dpu_ll_get_gcregcursorlocation_y(void)
{
    dpu_gcregcursorlocation_t *r = (dpu_gcregcursorlocation_t *)(SOC_DPU_REG_BASE + (0x84b << 2));
    return r->y ;
}

//reg gcregcursorbackground:

static inline void dpu_ll_set_gcregcursorbackground_value(uint32_t v)
{
    dpu_gcregcursorbackground_t *r = (dpu_gcregcursorbackground_t *)(SOC_DPU_REG_BASE + (0x84c << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregcursorbackground_value(void)
{
    dpu_gcregcursorbackground_t *r = (dpu_gcregcursorbackground_t *)(SOC_DPU_REG_BASE + (0x84c << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregcursorbackground_blue(void)
{
    dpu_gcregcursorbackground_t *r = (dpu_gcregcursorbackground_t *)(SOC_DPU_REG_BASE + (0x84c << 2));
    return r->blue   ;
}

static inline uint32_t dpu_ll_get_gcregcursorbackground_green(void)
{
    dpu_gcregcursorbackground_t *r = (dpu_gcregcursorbackground_t *)(SOC_DPU_REG_BASE + (0x84c << 2));
    return r->green;
}

static inline uint32_t dpu_ll_get_gcregcursorbackground_red(void)
{
    dpu_gcregcursorbackground_t *r = (dpu_gcregcursorbackground_t *)(SOC_DPU_REG_BASE + (0x84c << 2));
    return r->red ;
}

//reg gcregcursorforeground:

static inline void dpu_ll_set_gcregcursorforeground_value(uint32_t v)
{
    dpu_gcregcursorforeground_t *r = (dpu_gcregcursorforeground_t *)(SOC_DPU_REG_BASE + (0x84d << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregcursorforeground_value(void)
{
    dpu_gcregcursorforeground_t *r = (dpu_gcregcursorforeground_t *)(SOC_DPU_REG_BASE + (0x84d << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregcursorforeground_blue(void)
{
    dpu_gcregcursorforeground_t *r = (dpu_gcregcursorforeground_t *)(SOC_DPU_REG_BASE + (0x84d << 2));
    return r->blue   ;
}

static inline uint32_t dpu_ll_get_gcregcursorforeground_green(void)
{
    dpu_gcregcursorforeground_t *r = (dpu_gcregcursorforeground_t *)(SOC_DPU_REG_BASE + (0x84d << 2));
    return r->green;
}

static inline uint32_t dpu_ll_get_gcregcursorforeground_red(void)
{
    dpu_gcregcursorforeground_t *r = (dpu_gcregcursorforeground_t *)(SOC_DPU_REG_BASE + (0x84d << 2));
    return r->red ;
}

//reg gcregdisplayintr:

static inline void dpu_ll_set_gcregdisplayintr_value(uint32_t v)
{
    dpu_gcregdisplayintr_t *r = (dpu_gcregdisplayintr_t *)(SOC_DPU_REG_BASE + (0x84e << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdisplayintr_value(void)
{
    dpu_gcregdisplayintr_t *r = (dpu_gcregdisplayintr_t *)(SOC_DPU_REG_BASE + (0x84e << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdisplayintr_disp0(void)
{
    dpu_gcregdisplayintr_t *r = (dpu_gcregdisplayintr_t *)(SOC_DPU_REG_BASE + (0x84e << 2));
    return r->disp0;
}

static inline uint32_t dpu_ll_get_gcregdisplayintr_disp0_dbi_cfg_error(void)
{
    dpu_gcregdisplayintr_t *r = (dpu_gcregdisplayintr_t *)(SOC_DPU_REG_BASE + (0x84e << 2));
    return r->disp0_dbi_cfg_error;
}

static inline void dpu_ll_set_gcregdisplayintr_panel_underflow(uint32_t v)
{
    dpu_gcregdisplayintr_t *r = (dpu_gcregdisplayintr_t *)(SOC_DPU_REG_BASE + (0x84e << 2));
    r->panel_underflow = v;
}

static inline uint32_t dpu_ll_get_gcregdisplayintr_panel_underflow(void)
{
    dpu_gcregdisplayintr_t *r = (dpu_gcregdisplayintr_t *)(SOC_DPU_REG_BASE + (0x84e << 2));
    return r->panel_underflow;
}

static inline uint32_t dpu_ll_get_gcregdisplayintr_soft_reset_done(void)
{
    dpu_gcregdisplayintr_t *r = (dpu_gcregdisplayintr_t *)(SOC_DPU_REG_BASE + (0x84e << 2));
    return r->soft_reset_done;
}

static inline uint32_t dpu_ll_get_gcregdisplayintr_bus_error(void)
{
    dpu_gcregdisplayintr_t *r = (dpu_gcregdisplayintr_t *)(SOC_DPU_REG_BASE + (0x84e << 2));
    return r->bus_error;
}

//reg gcregdisplayintrenable:

static inline void dpu_ll_set_gcregdisplayintrenable_value(uint32_t v)
{
    dpu_gcregdisplayintrenable_t *r = (dpu_gcregdisplayintrenable_t *)(SOC_DPU_REG_BASE + (0x84f << 2));
    r->v = v;
}

static inline uint32_t dpu_ll_get_gcregdisplayintrenable_value(void)
{
    dpu_gcregdisplayintrenable_t *r = (dpu_gcregdisplayintrenable_t *)(SOC_DPU_REG_BASE + (0x84f << 2));
    return r->v;
}

static inline uint32_t dpu_ll_get_gcregdisplayintrenable_disp0(void)
{
    dpu_gcregdisplayintrenable_t *r = (dpu_gcregdisplayintrenable_t *)(SOC_DPU_REG_BASE + (0x84f << 2));
    return r->disp0;
}

static inline uint32_t dpu_ll_get_gcregdisplayintrenable_disp0_dbi_cfg_error(void)
{
    dpu_gcregdisplayintrenable_t *r = (dpu_gcregdisplayintrenable_t *)(SOC_DPU_REG_BASE + (0x84f << 2));
    return r->disp0_dbi_cfg_error;
}

static inline void dpu_ll_set_gcregdisplayintrenable_panel_underflow(uint32_t v)
{
    dpu_gcregdisplayintrenable_t *r = (dpu_gcregdisplayintrenable_t *)(SOC_DPU_REG_BASE + (0x84f << 2));
    r->panel_underflow = v;
}

static inline uint32_t dpu_ll_get_gcregdisplayintrenable_panel_underflow(void)
{
    dpu_gcregdisplayintrenable_t *r = (dpu_gcregdisplayintrenable_t *)(SOC_DPU_REG_BASE + (0x84f << 2));
    return r->panel_underflow;
}

static inline uint32_t dpu_ll_get_gcregdisplayintrenable_soft_reset_done(void)
{
    dpu_gcregdisplayintrenable_t *r = (dpu_gcregdisplayintrenable_t *)(SOC_DPU_REG_BASE + (0x84f << 2));
    return r->soft_reset_done;
}

static inline uint32_t dpu_ll_get_gcregdisplayintrenable_bus_error(void)
{
    dpu_gcregdisplayintrenable_t *r = (dpu_gcregdisplayintrenable_t *)(SOC_DPU_REG_BASE + (0x84f << 2));
    return r->bus_error ;
}
#ifdef __cplusplus
}
#endif
