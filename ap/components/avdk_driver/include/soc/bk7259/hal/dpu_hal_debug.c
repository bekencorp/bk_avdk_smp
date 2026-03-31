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
#include "dpu_hw.h"
#include "dpu_hal.h"

typedef void (*dpu_dump_fn_t)(void);
typedef struct
{
    uint32_t start;
    uint32_t end;
    dpu_dump_fn_t fn;
} dpu_reg_fn_map_t;

static void dpu_dump_gcregdcproductid(void)
{
    SOC_LOGI("gcregdcproductid: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x803 << 2)));
}

static void dpu_dump_rsv_804_808(void)
{
    for (uint32_t idx = 0; idx < 5; idx++)
    {
        SOC_LOGI("rsv_804_808: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + ((0x804 + idx) << 2)));
    }
}

static void dpu_dump_gcregframebufferconfig(void)
{
    dpu_gcregframebufferconfig_t *r = (dpu_gcregframebufferconfig_t *)(SOC_DPU_REG_BASE + (0x809 << 2));

    SOC_LOGI("gcregframebufferconfig: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x809 << 2)));
    SOC_LOGI("	format: %8x\r\n", r->format);
    SOC_LOGI("	enable: %8x\r\n", r->enable);
    SOC_LOGI("	reserved_bit_4_4: %8x\r\n", r->reserved_bit_4_4);
    SOC_LOGI("	clear_en: %8x\r\n", r->clear_en);
    SOC_LOGI("	reserved_bit_6_9: %8x\r\n", r->reserved_bit_6_9);
    SOC_LOGI("	color_key_en : %8x\r\n", r->color_key_en);
    SOC_LOGI("	reserved_bit_11_16: %8x\r\n", r->reserved_bit_11_16);
    SOC_LOGI("	swizzle: %8x\r\n", r->swizzle);
    SOC_LOGI("	uv_swizzle: %8x\r\n", r->uv_swizzle);
    SOC_LOGI("	reserved_bit_20_20: %8x\r\n", r->reserved_bit_20_20);
    SOC_LOGI("	dec_mode: %8x\r\n", r->dec_mode);
    SOC_LOGI("	rot_angle  : %8x\r\n", r->rot_angle);
    SOC_LOGI("	reserved_bit_27_31: %8x\r\n", r->reserved_bit_27_31);
}

static void dpu_dump_gcregframebufferaddress(void)
{
    SOC_LOGI("gcregframebufferaddress: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x80a << 2)));
}

static void dpu_dump_gcregframebufferstride(void)
{
    dpu_gcregframebufferstride_t *r = (dpu_gcregframebufferstride_t *)(SOC_DPU_REG_BASE + (0x80b << 2));

    SOC_LOGI("gcregframebufferstride: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x80b << 2)));
    SOC_LOGI("	stride: %8x\r\n", r->stride);
    SOC_LOGI("	reserved_bit_17_31: %8x\r\n", r->reserved_bit_17_31);
}

static void dpu_dump_rsv_80c_80c(void)
{
    for (uint32_t idx = 0; idx < 1; idx++)
    {
        SOC_LOGI("rsv_80c_80c: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + ((0x80c + idx) << 2)));
    }
}

static void dpu_dump_gcregdctileincfg(void)
{
    dpu_gcregdctileincfg_t *r = (dpu_gcregdctileincfg_t *)(SOC_DPU_REG_BASE + (0x80d << 2));

    SOC_LOGI("gcregdctileincfg: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x80d << 2)));
    SOC_LOGI("	tile_format : %8x\r\n", r->tile_format);
    SOC_LOGI("	yuv_standard: %8x\r\n", r->yuv_standard);
    SOC_LOGI("	tile_format1: %8x\r\n", r->tile_format1);
    SOC_LOGI("	reserved_bit_5_31: %8x\r\n", r->reserved_bit_5_31);
}

static void dpu_dump_gcregdctileuvframebufferadr(void)
{
    SOC_LOGI("gcregdctileuvframebufferadr: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x80e << 2)));
}

static void dpu_dump_gcregdctileuvframebufferstr(void)
{
    dpu_gcregdctileuvframebufferstr_t *r = (dpu_gcregdctileuvframebufferstr_t *)(SOC_DPU_REG_BASE + (0x80f << 2));

    SOC_LOGI("gcregdctileuvframebufferstr: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x80f << 2)));
    SOC_LOGI("	stride : %8x\r\n", r->stride);
    SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void dpu_dump_rsv_810_815(void)
{
    for (uint32_t idx = 0; idx < 6; idx++)
    {
        SOC_LOGI("rsv_810_815: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + ((0x810 + idx) << 2)));
    }
}

static void dpu_dump_gcregframebufferbackground(void)
{
    dpu_gcregframebufferbackground_t *r = (dpu_gcregframebufferbackground_t *)(SOC_DPU_REG_BASE + (0x816 << 2));

    SOC_LOGI("gcregframebufferbackground: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x816 << 2)));
    SOC_LOGI("	blue: %8x\r\n", r->blue);
    SOC_LOGI("	green : %8x\r\n", r->green);
    SOC_LOGI("	red : %8x\r\n", r->red);
    SOC_LOGI("	alpha : %8x\r\n", r->alpha);
}

static void dpu_dump_gcregframebuffercolorkey(void)
{
    dpu_gcregframebuffercolorkey_t *r = (dpu_gcregframebuffercolorkey_t *)(SOC_DPU_REG_BASE + (0x817 << 2));

    SOC_LOGI("gcregframebuffercolorkey: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x817 << 2)));
    SOC_LOGI("	blue: %8x\r\n", r->blue);
    SOC_LOGI("	green : %8x\r\n", r->green);
    SOC_LOGI("	red : %8x\r\n", r->red);
    SOC_LOGI("	alpha : %8x\r\n", r->alpha);
}

static void dpu_dump_gcregframebuffercolorkeyhigh(void)
{
    dpu_gcregframebuffercolorkeyhigh_t *r = (dpu_gcregframebuffercolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x818 << 2));

    SOC_LOGI("gcregframebuffercolorkeyhigh: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x818 << 2)));
    SOC_LOGI("	blue: %8x\r\n", r->blue);
    SOC_LOGI("	green : %8x\r\n", r->green);
    SOC_LOGI("	red : %8x\r\n", r->red);
    SOC_LOGI("	alpha : %8x\r\n", r->alpha);
}

static void dpu_dump_gcregframebufferclearvalue(void)
{
    dpu_gcregframebufferclearvalue_t *r = (dpu_gcregframebufferclearvalue_t *)(SOC_DPU_REG_BASE + (0x819 << 2));

    SOC_LOGI("gcregframebufferclearvalue: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x819 << 2)));
    SOC_LOGI("	blue: %8x\r\n", r->blue);
    SOC_LOGI("	green : %8x\r\n", r->green);
    SOC_LOGI("	red : %8x\r\n", r->red);
    SOC_LOGI("	alpha : %8x\r\n", r->alpha);
}

static void dpu_dump_gcregvideotl(void)
{
    dpu_gcregvideotl_t *r = (dpu_gcregvideotl_t *)(SOC_DPU_REG_BASE + (0x81a << 2));

    SOC_LOGI("gcregvideotl: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x81a << 2)));
    SOC_LOGI("	x : %8x\r\n", r->x);
    SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
    SOC_LOGI("	y: %8x\r\n", r->y);
    SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void dpu_dump_gcregframebuffersize(void)
{
    dpu_gcregframebuffersize_t *r = (dpu_gcregframebuffersize_t *)(SOC_DPU_REG_BASE + (0x81b << 2));

    SOC_LOGI("gcregframebuffersize: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x81b << 2)));
    SOC_LOGI("	width   : %8x\r\n", r->width);
    SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
    SOC_LOGI("	height: %8x\r\n", r->height);
    SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void dpu_dump_gcregvideoglobalalpha(void)
{
    dpu_gcregvideoglobalalpha_t *r = (dpu_gcregvideoglobalalpha_t *)(SOC_DPU_REG_BASE + (0x81c << 2));

    SOC_LOGI("gcregvideoglobalalpha: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x81c << 2)));
    SOC_LOGI("	src_alpha : %8x\r\n", r->src_alpha);
    SOC_LOGI("	dst_alpha: %8x\r\n", r->dst_alpha);
    SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void dpu_dump_gcregblendstackorder(void)
{
    dpu_gcregblendstackorder_t *r = (dpu_gcregblendstackorder_t *)(SOC_DPU_REG_BASE + (0x81d << 2));

    SOC_LOGI("gcregblendstackorder: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x81d << 2)));
    SOC_LOGI("	order : %8x\r\n", r->order);
    SOC_LOGI("	reserved_bit_3_31: %8x\r\n", r->reserved_bit_3_31);
}

static void dpu_dump_gcregvideoalphablendconfig(void)
{
    dpu_gcregvideoalphablendconfig_t *r = (dpu_gcregvideoalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x81e << 2));

    SOC_LOGI("gcregvideoalphablendconfig: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x81e << 2)));
    SOC_LOGI("	alpha_blend : %8x\r\n", r->alpha_blend);
    SOC_LOGI("	src_alpha_mode: %8x\r\n", r->src_alpha_mode);
    SOC_LOGI("	reserved_bit_2_2: %8x\r\n", r->reserved_bit_2_2);
    SOC_LOGI("	src_global_alpha_mode: %8x\r\n", r->src_global_alpha_mode);
    SOC_LOGI("	reserved_bit_5_5: %8x\r\n", r->reserved_bit_5_5);
    SOC_LOGI("	src_blending_mode: %8x\r\n", r->src_blending_mode);
    SOC_LOGI("	src_alpha_factor: %8x\r\n", r->src_alpha_factor);
    SOC_LOGI("	dst_alpha_mode: %8x\r\n", r->dst_alpha_mode);
    SOC_LOGI("	dst_global_alpha_mode: %8x\r\n", r->dst_global_alpha_mode);
    SOC_LOGI("	reserved_bit_12_12: %8x\r\n", r->reserved_bit_12_12);
    SOC_LOGI("	dst_blending_mode : %8x\r\n", r->dst_blending_mode);
    SOC_LOGI("	dst_alpha_factor : %8x\r\n", r->dst_alpha_factor);
    SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void dpu_dump_gcregoverlayconfig(void)
{
    dpu_gcregoverlayconfig_t *r = (dpu_gcregoverlayconfig_t *)(SOC_DPU_REG_BASE + (0x81f << 2));

    SOC_LOGI("gcregoverlayconfig: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x81f << 2)));
    SOC_LOGI("	format: %8x\r\n", r->format);
    SOC_LOGI("	enable   : %8x\r\n", r->enable);
    SOC_LOGI("	reserved_bit_4_4: %8x\r\n", r->reserved_bit_4_4);
    SOC_LOGI("	clear_en   : %8x\r\n", r->clear_en);
    SOC_LOGI("	reserved_bit_6_16: %8x\r\n", r->reserved_bit_6_16);
    SOC_LOGI("	swizzle: %8x\r\n", r->swizzle);
    SOC_LOGI("	uv_swizzle : %8x\r\n", r->uv_swizzle);
    SOC_LOGI("	color_key_en : %8x\r\n", r->color_key_en);
    SOC_LOGI("	dec_mode  : %8x\r\n", r->dec_mode);
    SOC_LOGI("	rot_angle: %8x\r\n", r->rot_angle);
    SOC_LOGI("	reserved_bit_27_31: %8x\r\n", r->reserved_bit_27_31);
}

static void dpu_dump_gcregoverlayaddress(void)
{
    SOC_LOGI("gcregoverlayaddress: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x820 << 2)));
}

static void dpu_dump_gcregoverlaystride(void)
{
    dpu_gcregoverlaystride_t *r = (dpu_gcregoverlaystride_t *)(SOC_DPU_REG_BASE + (0x821 << 2));

    SOC_LOGI("gcregoverlaystride: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x821 << 2)));
    SOC_LOGI("	stride : %8x\r\n", r->stride);
    SOC_LOGI("	reserved_bit_17_31: %8x\r\n", r->reserved_bit_17_31);
}

static void dpu_dump_gcregdcoverlaytileincfg(void)
{
    dpu_gcregdcoverlaytileincfg_t *r = (dpu_gcregdcoverlaytileincfg_t *)(SOC_DPU_REG_BASE + (0x822 << 2));

    SOC_LOGI("gcregdcoverlaytileincfg: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x822 << 2)));
    SOC_LOGI("	tile_format : %8x\r\n", r->tile_format);
    SOC_LOGI("	yuv_standard: %8x\r\n", r->yuv_standard);
    SOC_LOGI("	tile_format1 : %8x\r\n", r->tile_format1);
    SOC_LOGI("	reserved_bit_5_31: %8x\r\n", r->reserved_bit_5_31);
}

static void dpu_dump_gcregdctileuvoverlayadr(void)
{
    SOC_LOGI("gcregdctileuvoverlayadr: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x823 << 2)));
}

static void dpu_dump_gcregdctileuvoverlaystr(void)
{
    dpu_gcregdctileuvoverlaystr_t *r = (dpu_gcregdctileuvoverlaystr_t *)(SOC_DPU_REG_BASE + (0x824 << 2));

    SOC_LOGI("gcregdctileuvoverlaystr: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x824 << 2)));
    SOC_LOGI("	stride: %8x\r\n", r->stride);
    SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void dpu_dump_gcregoverlaytl(void)
{
    dpu_gcregoverlaytl_t *r = (dpu_gcregoverlaytl_t *)(SOC_DPU_REG_BASE + (0x825 << 2));

    SOC_LOGI("gcregoverlaytl: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x825 << 2)));
    SOC_LOGI("	x  : %8x\r\n", r->x);
    SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
    SOC_LOGI("	y  : %8x\r\n", r->y);
    SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void dpu_dump_gcregoverlaysize(void)
{
    dpu_gcregoverlaysize_t *r = (dpu_gcregoverlaysize_t *)(SOC_DPU_REG_BASE + (0x826 << 2));

    SOC_LOGI("gcregoverlaysize: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x826 << 2)));
    SOC_LOGI("	width  : %8x\r\n", r->width);
    SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
    SOC_LOGI("	height : %8x\r\n", r->height);
    SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void dpu_dump_gcregoverlaycolorkey(void)
{
    dpu_gcregoverlaycolorkey_t *r = (dpu_gcregoverlaycolorkey_t *)(SOC_DPU_REG_BASE + (0x827 << 2));

    SOC_LOGI("gcregoverlaycolorkey: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x827 << 2)));
    SOC_LOGI("	blue   : %8x\r\n", r->blue);
    SOC_LOGI("	green: %8x\r\n", r->green);
    SOC_LOGI("	red: %8x\r\n", r->red);
    SOC_LOGI("	alpha  : %8x\r\n", r->alpha);
}

static void dpu_dump_gcregoverlaycolorkeyhigh(void)
{
    dpu_gcregoverlaycolorkeyhigh_t *r = (dpu_gcregoverlaycolorkeyhigh_t *)(SOC_DPU_REG_BASE + (0x828 << 2));

    SOC_LOGI("gcregoverlaycolorkeyhigh: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x828 << 2)));
    SOC_LOGI("	blue   : %8x\r\n", r->blue);
    SOC_LOGI("	green: %8x\r\n", r->green);
    SOC_LOGI("	red: %8x\r\n", r->red);
    SOC_LOGI("	alpha  : %8x\r\n", r->alpha);
}

static void dpu_dump_gcregoverlayalphablendconfig(void)
{
    dpu_gcregoverlayalphablendconfig_t *r = (dpu_gcregoverlayalphablendconfig_t *)(SOC_DPU_REG_BASE + (0x829 << 2));

    SOC_LOGI("gcregoverlayalphablendconfig: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x829 << 2)));
    SOC_LOGI("	alpha_blend : %8x\r\n", r->alpha_blend);
    SOC_LOGI("	src_alpha_mode: %8x\r\n", r->src_alpha_mode);
    SOC_LOGI("	reserved_bit_2_2: %8x\r\n", r->reserved_bit_2_2);
    SOC_LOGI("	src_global_alpha_mode: %8x\r\n", r->src_global_alpha_mode);
    SOC_LOGI("	reserved_bit_5_5: %8x\r\n", r->reserved_bit_5_5);
    SOC_LOGI("	src_blending_mode: %8x\r\n", r->src_blending_mode);
    SOC_LOGI("	src_alpha_factor: %8x\r\n", r->src_alpha_factor);
    SOC_LOGI("	dst_alpha_mode: %8x\r\n", r->dst_alpha_mode);
    SOC_LOGI("	dst_global_alpha_mode                  : %8x\r\n", r->dst_global_alpha_mode);
    SOC_LOGI("	reserved_bit_12_12: %8x\r\n", r->reserved_bit_12_12);
    SOC_LOGI("	dst_blending_mode       : %8x\r\n", r->dst_blending_mode);
    SOC_LOGI("	dst_alpha_factor: %8x\r\n", r->dst_alpha_factor);
    SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void dpu_dump_gcregoverlayglobalalpha(void)
{
    dpu_gcregoverlayglobalalpha_t *r = (dpu_gcregoverlayglobalalpha_t *)(SOC_DPU_REG_BASE + (0x82a << 2));

    SOC_LOGI("gcregoverlayglobalalpha: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x82a << 2)));
    SOC_LOGI("	src_alpha: %8x\r\n", r->src_alpha);
    SOC_LOGI("	dst_alpha: %8x\r\n", r->dst_alpha);
    SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void dpu_dump_gcregoverlayclearvalue(void)
{
    dpu_gcregoverlayclearvalue_t *r = (dpu_gcregoverlayclearvalue_t *)(SOC_DPU_REG_BASE + (0x82b << 2));

    SOC_LOGI("gcregoverlayclearvalue: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x82b << 2)));
    SOC_LOGI("	blue: %8x\r\n", r->blue);
    SOC_LOGI("	green: %8x\r\n", r->green);
    SOC_LOGI("	red : %8x\r\n", r->red);
    SOC_LOGI("	alpha: %8x\r\n", r->alpha);
}

static void dpu_dump_gcregoverlayconfig1(void)
{
    dpu_gcregoverlayconfig1_t *r = (dpu_gcregoverlayconfig1_t *)(SOC_DPU_REG_BASE + (0x82c << 2));

    SOC_LOGI("gcregoverlayconfig1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x82c << 2)));
    SOC_LOGI("	format: %8x\r\n", r->format);
    SOC_LOGI("	enable: %8x\r\n", r->enable);
    SOC_LOGI("	reserved_bit_4_4: %8x\r\n", r->reserved_bit_4_4);
    SOC_LOGI("	clear_en: %8x\r\n", r->clear_en);
    SOC_LOGI("	reserved_bit_6_16: %8x\r\n", r->reserved_bit_6_16);
    SOC_LOGI("	swizzle: %8x\r\n", r->swizzle);
    SOC_LOGI("	reserved_bit_19_19: %8x\r\n", r->reserved_bit_19_19);
    SOC_LOGI("	color_key_en: %8x\r\n", r->color_key_en);
    SOC_LOGI("	dec_mode: %8x\r\n", r->dec_mode);
    SOC_LOGI("	rot_angle: %8x\r\n", r->rot_angle);
    SOC_LOGI("	reserved_bit_27_31: %8x\r\n", r->reserved_bit_27_31);
}

static void dpu_dump_gcregoverlayaddress1(void)
{
    SOC_LOGI("gcregoverlayaddress1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x82d << 2)));
}

static void dpu_dump_gcregoverlaystride1(void)
{
    dpu_gcregoverlaystride1_t *r = (dpu_gcregoverlaystride1_t *)(SOC_DPU_REG_BASE + (0x82e << 2));

    SOC_LOGI("gcregoverlaystride1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x82e << 2)));
    SOC_LOGI("	stride: %8x\r\n", r->stride);
    SOC_LOGI("	reserved_bit_17_31: %8x\r\n", r->reserved_bit_17_31);
}

static void dpu_dump_gcregoverlaytl1(void)
{
    dpu_gcregoverlaytl1_t *r = (dpu_gcregoverlaytl1_t *)(SOC_DPU_REG_BASE + (0x82f << 2));

    SOC_LOGI("gcregoverlaytl1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x82f << 2)));
    SOC_LOGI("	x: %8x\r\n", r->x);
    SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
    SOC_LOGI("	y: %8x\r\n", r->y);
    SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void dpu_dump_gcregoverlaysize1(void)
{
    dpu_gcregoverlaysize1_t *r = (dpu_gcregoverlaysize1_t *)(SOC_DPU_REG_BASE + (0x830 << 2));

    SOC_LOGI("gcregoverlaysize1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x830 << 2)));
    SOC_LOGI("	width: %8x\r\n", r->width);
    SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
    SOC_LOGI("	height: %8x\r\n", r->height);
    SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void dpu_dump_gcregoverlaycolorkey1(void)
{
    dpu_gcregoverlaycolorkey1_t *r = (dpu_gcregoverlaycolorkey1_t *)(SOC_DPU_REG_BASE + (0x831 << 2));

    SOC_LOGI("gcregoverlaycolorkey1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x831 << 2)));
    SOC_LOGI("	blue: %8x\r\n", r->blue);
    SOC_LOGI("	green: %8x\r\n", r->green);
    SOC_LOGI("	red: %8x\r\n", r->red);
    SOC_LOGI("	alpha: %8x\r\n", r->alpha);
}

static void dpu_dump_gcregoverlaycolorkeyhigh1(void)
{
    dpu_gcregoverlaycolorkeyhigh1_t *r = (dpu_gcregoverlaycolorkeyhigh1_t *)(SOC_DPU_REG_BASE + (0x832 << 2));

    SOC_LOGI("gcregoverlaycolorkeyhigh1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x832 << 2)));
    SOC_LOGI("	blue: %8x\r\n", r->blue);
    SOC_LOGI("	green: %8x\r\n", r->green);
    SOC_LOGI("	red: %8x\r\n", r->red);
    SOC_LOGI("	alpha: %8x\r\n", r->alpha);
}

static void dpu_dump_gcregoverlayalphablendconfig1(void)
{
    dpu_gcregoverlayalphablendconfig1_t *r = (dpu_gcregoverlayalphablendconfig1_t *)(SOC_DPU_REG_BASE + (0x833 << 2));

    SOC_LOGI("gcregoverlayalphablendconfig1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x833 << 2)));
    SOC_LOGI("	alpha_blend: %8x\r\n", r->alpha_blend);
    SOC_LOGI("	src_alpha_mode: %8x\r\n", r->src_alpha_mode);
    SOC_LOGI("	reserved_bit_2_2: %8x\r\n", r->reserved_bit_2_2);
    SOC_LOGI("	src_global_alpha_mode: %8x\r\n", r->src_global_alpha_mode);
    SOC_LOGI("	reserved_bit_5_5: %8x\r\n", r->reserved_bit_5_5);
    SOC_LOGI("	src_blending_mode: %8x\r\n", r->src_blending_mode);
    SOC_LOGI("	src_alpha_factor: %8x\r\n", r->src_alpha_factor);
    SOC_LOGI("	dst_alpha_mode: %8x\r\n", r->dst_alpha_mode);
    SOC_LOGI("	dst_global_alpha_mode                  : %8x\r\n", r->dst_global_alpha_mode);
    SOC_LOGI("	reserved_bit_12_12: %8x\r\n", r->reserved_bit_12_12);
    SOC_LOGI("	dst_blending_mode: %8x\r\n", r->dst_blending_mode);
    SOC_LOGI("	dst_alpha_factor: %8x\r\n", r->dst_alpha_factor);
    SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void dpu_dump_gcregoverlayglobalalpha1(void)
{
    dpu_gcregoverlayglobalalpha1_t *r = (dpu_gcregoverlayglobalalpha1_t *)(SOC_DPU_REG_BASE + (0x834 << 2));

    SOC_LOGI("gcregoverlayglobalalpha1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x834 << 2)));
    SOC_LOGI("	src_alpha : %8x\r\n", r->src_alpha);
    SOC_LOGI("	dst_alpha: %8x\r\n", r->dst_alpha);
    SOC_LOGI("	reserved_bit_16_31: %8x\r\n", r->reserved_bit_16_31);
}

static void dpu_dump_gcregoverlayclearvalue1(void)
{
    dpu_gcregoverlayclearvalue1_t *r = (dpu_gcregoverlayclearvalue1_t *)(SOC_DPU_REG_BASE + (0x835 << 2));

    SOC_LOGI("gcregoverlayclearvalue1: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x835 << 2)));
    SOC_LOGI("	blue  : %8x\r\n", r->blue);
    SOC_LOGI("	green: %8x\r\n", r->green);
    SOC_LOGI("	red : %8x\r\n", r->red);
    SOC_LOGI("	alpha: %8x\r\n", r->alpha);
}

static void dpu_dump_rsv_836_837(void)
{
    for (uint32_t idx = 0; idx < 2; idx++)
    {
        SOC_LOGI("rsv_836_837: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + ((0x836 + idx) << 2)));
    }
}

static void dpu_dump_gcregdisplaydithertablelow(void)
{
    dpu_gcregdisplaydithertablelow_t *r = (dpu_gcregdisplaydithertablelow_t *)(SOC_DPU_REG_BASE + (0x838 << 2));

    SOC_LOGI("gcregdisplaydithertablelow: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x838 << 2)));
    SOC_LOGI("	y0_x0: %8x\r\n", r->y0_x0);
    SOC_LOGI("	y0_x1: %8x\r\n", r->y0_x1);
    SOC_LOGI("	y0_x2: %8x\r\n", r->y0_x2);
    SOC_LOGI("	y0_x3: %8x\r\n", r->y0_x3);
    SOC_LOGI("	y1_x0: %8x\r\n", r->y1_x0);
    SOC_LOGI("	y1_x1: %8x\r\n", r->y1_x1);
    SOC_LOGI("	y1_x2: %8x\r\n", r->y1_x2);
    SOC_LOGI("	y1_x3: %8x\r\n", r->y1_x3);
}

static void dpu_dump_gcregdisplaydithertablehigh(void)
{
    dpu_gcregdisplaydithertablehigh_t *r = (dpu_gcregdisplaydithertablehigh_t *)(SOC_DPU_REG_BASE + (0x839 << 2));

    SOC_LOGI("gcregdisplaydithertablehigh: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x839 << 2)));
    SOC_LOGI("	y2_x0  : %8x\r\n", r->y2_x0);
    SOC_LOGI("	y2_x1: %8x\r\n", r->y2_x1);
    SOC_LOGI("	y2_x2: %8x\r\n", r->y2_x2);
    SOC_LOGI("	y2_x3: %8x\r\n", r->y2_x3);
    SOC_LOGI("	y3_x0: %8x\r\n", r->y3_x0);
    SOC_LOGI("	y3_x1: %8x\r\n", r->y3_x1);
    SOC_LOGI("	y3_x2: %8x\r\n", r->y3_x2);
    SOC_LOGI("	y3_x3: %8x\r\n", r->y3_x3);
}

static void dpu_dump_gcregpanelconfig(void)
{
    dpu_gcregpanelconfig_t *r = (dpu_gcregpanelconfig_t *)(SOC_DPU_REG_BASE + (0x83a << 2));

    SOC_LOGI("gcregpanelconfig: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x83a << 2)));
    SOC_LOGI("	de: %8x\r\n", r->de);
    SOC_LOGI("	de_polarity: %8x\r\n", r->de_polarity);
    SOC_LOGI("	reserved_bit_2_4: %8x\r\n", r->reserved_bit_2_4);
    SOC_LOGI("	data_polarity: %8x\r\n", r->data_polarity);
    SOC_LOGI("	reserved_bit_6_7: %8x\r\n", r->reserved_bit_6_7);
    SOC_LOGI("	clock  : %8x\r\n", r->clock);
    SOC_LOGI("	clock_polarity : %8x\r\n", r->clock_polarity);
    SOC_LOGI("	reserved_bit_10_31: %8x\r\n", r->reserved_bit_10_31);
}

static void dpu_dump_gcregpanelcontrol(void)
{
    dpu_gcregpanelcontrol_t *r = (dpu_gcregpanelcontrol_t *)(SOC_DPU_REG_BASE + (0x83b << 2));

    SOC_LOGI("gcregpanelcontrol: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x83b << 2)));
    SOC_LOGI("	valid: %8x\r\n", r->valid);
    SOC_LOGI("	back_pressure_disab: %8x\r\n", r->back_pressure_disab);
    SOC_LOGI("	reserved_bit_2_31: %8x\r\n", r->reserved_bit_2_31);
}

static void dpu_dump_gcregpanelfunction(void)
{
    dpu_gcregpanelfunction_t *r = (dpu_gcregpanelfunction_t *)(SOC_DPU_REG_BASE + (0x83c << 2));

    SOC_LOGI("gcregpanelfunction: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x83c << 2)));
    SOC_LOGI("	output: %8x\r\n", r->output);
    SOC_LOGI("	gamma: %8x\r\n", r->gamma);
    SOC_LOGI("	dither: %8x\r\n", r->dither);
    SOC_LOGI("	reserved_bit_3_31: %8x\r\n", r->reserved_bit_3_31);
}

static void dpu_dump_gcregpanelworking(void)
{
    dpu_gcregpanelworking_t *r = (dpu_gcregpanelworking_t *)(SOC_DPU_REG_BASE + (0x83d << 2));

    SOC_LOGI("gcregpanelworking: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x83d << 2)));
    SOC_LOGI("	working: %8x\r\n", r->working);
    SOC_LOGI("	reserved_bit_1_31: %8x\r\n", r->reserved_bit_1_31);
}

static void dpu_dump_gcregpanelstate(void)
{
    dpu_gcregpanelstate_t *r = (dpu_gcregpanelstate_t *)(SOC_DPU_REG_BASE + (0x83e << 2));

    SOC_LOGI("gcregpanelstate: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x83e << 2)));
    SOC_LOGI("	reserved_bit_0_0: %8x\r\n", r->reserved_bit_0_0);
    SOC_LOGI("	video_under_flow: %8x\r\n", r->video_under_flow);
    SOC_LOGI("	overlay_under_flow: %8x\r\n", r->overlay_under_flow);
    SOC_LOGI("	overlay_under_flow1: %8x\r\n", r->overlay_under_flow1);
    SOC_LOGI("	reserved_bit_4_31: %8x\r\n", r->reserved_bit_4_31);
}

static void dpu_dump_rsv_83f_83f(void)
{
    for (uint32_t idx = 0; idx < 1; idx++)
    {
        SOC_LOGI("rsv_83f_83f: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + ((0x83f + idx) << 2)));
    }
}

static void dpu_dump_gcreghdisplay(void)
{
    dpu_gcreghdisplay_t *r = (dpu_gcreghdisplay_t *)(SOC_DPU_REG_BASE + (0x840 << 2));

    SOC_LOGI("gcreghdisplay: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x840 << 2)));
    SOC_LOGI("	display_end: %8x\r\n", r->display_end);
    SOC_LOGI("	reserved_bit_13_15: %8x\r\n", r->reserved_bit_13_15);
    SOC_LOGI("	total: %8x\r\n", r->total);
    SOC_LOGI("	reserved_bit_29_31: %8x\r\n", r->reserved_bit_29_31);
}

static void dpu_dump_gcreghsync(void)
{
    dpu_gcreghsync_t *r = (dpu_gcreghsync_t *)(SOC_DPU_REG_BASE + (0x841 << 2));

    SOC_LOGI("gcreghsync: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x841 << 2)));
    SOC_LOGI("	start: %8x\r\n", r->start);
    SOC_LOGI("	reserved_bit_13_15: %8x\r\n", r->reserved_bit_13_15);
    SOC_LOGI("	end: %8x\r\n", r->end);
    SOC_LOGI("	reserved_bit_29_29: %8x\r\n", r->reserved_bit_29_29);
    SOC_LOGI("	pulse: %8x\r\n", r->pulse);
    SOC_LOGI("	polarity: %8x\r\n", r->polarity);
}

static void dpu_dump_rsv_842_843(void)
{
    for (uint32_t idx = 0; idx < 2; idx++)
    {
        SOC_LOGI("rsv_842_843: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + ((0x842 + idx) << 2)));
    }
}

static void dpu_dump_gcregvdisplay(void)
{
    dpu_gcregvdisplay_t *r = (dpu_gcregvdisplay_t *)(SOC_DPU_REG_BASE + (0x844 << 2));

    SOC_LOGI("gcregvdisplay: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x844 << 2)));
    SOC_LOGI("	display_end : %8x\r\n", r->display_end);
    SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
    SOC_LOGI("	total: %8x\r\n", r->total);
    SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void dpu_dump_gcregvsync(void)
{
    dpu_gcregvsync_t *r = (dpu_gcregvsync_t *)(SOC_DPU_REG_BASE + (0x845 << 2));

    SOC_LOGI("gcregvsync: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x845 << 2)));
    SOC_LOGI("	start: %8x\r\n", r->start);
    SOC_LOGI("	reserved_bit_12_15: %8x\r\n", r->reserved_bit_12_15);
    SOC_LOGI("	end: %8x\r\n", r->end);
    SOC_LOGI("	reserved_bit_28_29: %8x\r\n", r->reserved_bit_28_29);
    SOC_LOGI("	pulse: %8x\r\n", r->pulse);
    SOC_LOGI("
             polarity: %8x\r\n", r->
             polarity);
}

static void dpu_dump_gcregdisplaycurrentlocation(void)
{
    dpu_gcregdisplaycurrentlocation_t *r = (dpu_gcregdisplaycurrentlocation_t *)(SOC_DPU_REG_BASE + (0x846 << 2));

    SOC_LOGI("gcregdisplaycurrentlocation: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x846 << 2)));
    SOC_LOGI("	x : %8x\r\n", r->x);
    SOC_LOGI("	y : %8x\r\n", r->y);
}

static void dpu_dump_gcreggammaindex(void)
{
    dpu_gcreggammaindex_t *r = (dpu_gcreggammaindex_t *)(SOC_DPU_REG_BASE + (0x847 << 2));

    SOC_LOGI("gcreggammaindex: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x847 << 2)));
    SOC_LOGI("	index  : %8x\r\n", r->index);
    SOC_LOGI("	reserved_bit_8_31: %8x\r\n", r->reserved_bit_8_31);
}

static void dpu_dump_gcreggammadata(void)
{
    dpu_gcreggammadata_t *r = (dpu_gcreggammadata_t *)(SOC_DPU_REG_BASE + (0x848 << 2));

    SOC_LOGI("gcreggammadata: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x848 << 2)));
    SOC_LOGI("	blue   : %8x\r\n", r->blue);
    SOC_LOGI("	green: %8x\r\n", r->green);
    SOC_LOGI("	red : %8x\r\n", r->red);
    SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void dpu_dump_gcregcursorconfig(void)
{
    dpu_gcregcursorconfig_t *r = (dpu_gcregcursorconfig_t *)(SOC_DPU_REG_BASE + (0x849 << 2));

    SOC_LOGI("gcregcursorconfig: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x849 << 2)));
    SOC_LOGI("	format: %8x\r\n", r->format);
    SOC_LOGI("	reserved_bit_2_7: %8x\r\n", r->reserved_bit_2_7);
    SOC_LOGI("	hot_spot_y : %8x\r\n", r->hot_spot_y);
    SOC_LOGI("	reserved_bit_13_15: %8x\r\n", r->reserved_bit_13_15);
    SOC_LOGI("	hot_spot_x: %8x\r\n", r->hot_spot_x);
    SOC_LOGI("	reserved_bit_21_31: %8x\r\n", r->reserved_bit_21_31);
}

static void dpu_dump_gcregcursoraddress(void)
{
    SOC_LOGI("gcregcursoraddress: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x84a << 2)));
}

static void dpu_dump_gcregcursorlocation(void)
{
    dpu_gcregcursorlocation_t *r = (dpu_gcregcursorlocation_t *)(SOC_DPU_REG_BASE + (0x84b << 2));

    SOC_LOGI("gcregcursorlocation: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x84b << 2)));
    SOC_LOGI("	x: %8x\r\n", r->x);
    SOC_LOGI("	reserved_bit_13_15: %8x\r\n", r->reserved_bit_13_15);
    SOC_LOGI("	y : %8x\r\n", r->y);
    SOC_LOGI("	reserved_bit_28_31: %8x\r\n", r->reserved_bit_28_31);
}

static void dpu_dump_gcregcursorbackground(void)
{
    dpu_gcregcursorbackground_t *r = (dpu_gcregcursorbackground_t *)(SOC_DPU_REG_BASE + (0x84c << 2));

    SOC_LOGI("gcregcursorbackground: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x84c << 2)));
    SOC_LOGI("	blue   : %8x\r\n", r->blue);
    SOC_LOGI("	green: %8x\r\n", r->green);
    SOC_LOGI("	red : %8x\r\n", r->red);
    SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void dpu_dump_gcregcursorforeground(void)
{
    dpu_gcregcursorforeground_t *r = (dpu_gcregcursorforeground_t *)(SOC_DPU_REG_BASE + (0x84d << 2));

    SOC_LOGI("gcregcursorforeground: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x84d << 2)));
    SOC_LOGI("	blue   : %8x\r\n", r->blue);
    SOC_LOGI("	green: %8x\r\n", r->green);
    SOC_LOGI("	red : %8x\r\n", r->red);
    SOC_LOGI("	reserved_bit_24_31: %8x\r\n", r->reserved_bit_24_31);
}

static void dpu_dump_gcregdisplayintr(void)
{
    dpu_gcregdisplayintr_t *r = (dpu_gcregdisplayintr_t *)(SOC_DPU_REG_BASE + (0x84e << 2));

    SOC_LOGI("gcregdisplayintr: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x84e << 2)));
    SOC_LOGI("	disp0: %8x\r\n", r->disp0);
    SOC_LOGI("	reserved_bit_1_11: %8x\r\n", r->reserved_bit_1_11);
    SOC_LOGI("	disp0_dbi_cfg_error: %8x\r\n", r->disp0_dbi_cfg_error);
    SOC_LOGI("	reserved_bit_13_28: %8x\r\n", r->reserved_bit_13_28);
    SOC_LOGI("	panel_underflow: %8x\r\n", r->panel_underflow);
    SOC_LOGI("	soft_reset_done: %8x\r\n", r->soft_reset_done);
    SOC_LOGI("	bus_error: %8x\r\n", r->bus_error);
}

static void dpu_dump_gcregdisplayintrenable(void)
{
    dpu_gcregdisplayintrenable_t *r = (dpu_gcregdisplayintrenable_t *)(SOC_DPU_REG_BASE + (0x84f << 2));

    SOC_LOGI("gcregdisplayintrenable: %8x\r\n", REG_READ(SOC_DPU_REG_BASE + (0x84f << 2)));
    SOC_LOGI("	disp0: %8x\r\n", r->disp0);
    SOC_LOGI("	reserved_bit_1_11: %8x\r\n", r->reserved_bit_1_11);
    SOC_LOGI("	disp0_dbi_cfg_error: %8x\r\n", r->disp0_dbi_cfg_error);
    SOC_LOGI("	reserved_bit_13_28: %8x\r\n", r->reserved_bit_13_28);
    SOC_LOGI("	panel_underflow: %8x\r\n", r->panel_underflow);
    SOC_LOGI("	soft_reset_done: %8x\r\n", r->soft_reset_done);
    SOC_LOGI("	bus_error : %8x\r\n", r->bus_error);
}

static dpu_reg_fn_map_t s_fn[] =
{
    {0x803, 0x803, dpu_dump_gcregdcproductid},
    {0x804, 0x809, dpu_dump_rsv_804_808},
    {0x809, 0x809, dpu_dump_gcregframebufferconfig},
    {0x80a, 0x80a, dpu_dump_gcregframebufferaddress},
    {0x80b, 0x80b, dpu_dump_gcregframebufferstride},
    {0x80c, 0x80d, dpu_dump_rsv_80c_80c},
    {0x80d, 0x80d, dpu_dump_gcregdctileincfg},
    {0x80e, 0x80e, dpu_dump_gcregdctileuvframebufferadr},
    {0x80f, 0x80f, dpu_dump_gcregdctileuvframebufferstr},
    {0x810, 0x816, dpu_dump_rsv_810_815},
    {0x816, 0x816, dpu_dump_gcregframebufferbackground},
    {0x817, 0x817, dpu_dump_gcregframebuffercolorkey},
    {0x818, 0x818, dpu_dump_gcregframebuffercolorkeyhigh},
    {0x819, 0x819, dpu_dump_gcregframebufferclearvalue},
    {0x81a, 0x81a, dpu_dump_gcregvideotl},
    {0x81b, 0x81b, dpu_dump_gcregframebuffersize},
    {0x81c, 0x81c, dpu_dump_gcregvideoglobalalpha},
    {0x81d, 0x81d, dpu_dump_gcregblendstackorder},
    {0x81e, 0x81e, dpu_dump_gcregvideoalphablendconfig},
    {0x81f, 0x81f, dpu_dump_gcregoverlayconfig},
    {0x820, 0x820, dpu_dump_gcregoverlayaddress},
    {0x821, 0x821, dpu_dump_gcregoverlaystride},
    {0x822, 0x822, dpu_dump_gcregdcoverlaytileincfg},
    {0x823, 0x823, dpu_dump_gcregdctileuvoverlayadr},
    {0x824, 0x824, dpu_dump_gcregdctileuvoverlaystr},
    {0x825, 0x825, dpu_dump_gcregoverlaytl},
    {0x826, 0x826, dpu_dump_gcregoverlaysize},
    {0x827, 0x827, dpu_dump_gcregoverlaycolorkey},
    {0x828, 0x828, dpu_dump_gcregoverlaycolorkeyhigh},
    {0x829, 0x829, dpu_dump_gcregoverlayalphablendconfig},
    {0x82a, 0x82a, dpu_dump_gcregoverlayglobalalpha},
    {0x82b, 0x82b, dpu_dump_gcregoverlayclearvalue},
    {0x82c, 0x82c, dpu_dump_gcregoverlayconfig1},
    {0x82d, 0x82d, dpu_dump_gcregoverlayaddress1},
    {0x82e, 0x82e, dpu_dump_gcregoverlaystride1},
    {0x82f, 0x82f, dpu_dump_gcregoverlaytl1},
    {0x830, 0x830, dpu_dump_gcregoverlaysize1},
    {0x831, 0x831, dpu_dump_gcregoverlaycolorkey1},
    {0x832, 0x832, dpu_dump_gcregoverlaycolorkeyhigh1},
    {0x833, 0x833, dpu_dump_gcregoverlayalphablendconfig1},
    {0x834, 0x834, dpu_dump_gcregoverlayglobalalpha1},
    {0x835, 0x835, dpu_dump_gcregoverlayclearvalue1},
    {0x836, 0x838, dpu_dump_rsv_836_837},
    {0x838, 0x838, dpu_dump_gcregdisplaydithertablelow},
    {0x839, 0x839, dpu_dump_gcregdisplaydithertablehigh},
    {0x83a, 0x83a, dpu_dump_gcregpanelconfig},
    {0x83b, 0x83b, dpu_dump_gcregpanelcontrol},
    {0x83c, 0x83c, dpu_dump_gcregpanelfunction},
    {0x83d, 0x83d, dpu_dump_gcregpanelworking},
    {0x83e, 0x83e, dpu_dump_gcregpanelstate},
    {0x83f, 0x840, dpu_dump_rsv_83f_83f},
    {0x840, 0x840, dpu_dump_gcreghdisplay},
    {0x841, 0x841, dpu_dump_gcreghsync},
    {0x842, 0x844, dpu_dump_rsv_842_843},
    {0x844, 0x844, dpu_dump_gcregvdisplay},
    {0x845, 0x845, dpu_dump_gcregvsync},
    {0x846, 0x846, dpu_dump_gcregdisplaycurrentlocation},
    {0x847, 0x847, dpu_dump_gcreggammaindex},
    {0x848, 0x848, dpu_dump_gcreggammadata},
    {0x849, 0x849, dpu_dump_gcregcursorconfig},
    {0x84a, 0x84a, dpu_dump_gcregcursoraddress},
    {0x84b, 0x84b, dpu_dump_gcregcursorlocation},
    {0x84c, 0x84c, dpu_dump_gcregcursorbackground},
    {0x84d, 0x84d, dpu_dump_gcregcursorforeground},
    {0x84e, 0x84e, dpu_dump_gcregdisplayintr},
    {0x84f, 0x84f, dpu_dump_gcregdisplayintrenable},
    {-1, -1, 0}
};

void dpu_struct_dump(uint32_t start, uint32_t end)
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
