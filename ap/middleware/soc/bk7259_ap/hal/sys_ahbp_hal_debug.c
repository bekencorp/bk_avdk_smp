// Copyright 2022-2023 Beken
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
#include "sys_ahbp_hw.h"
#include "sys_ahbp_hal.h"

typedef void (*sys_ahbp_dump_fn_t)(void);
typedef struct {
	uint32_t start;
	uint32_t end;
	sys_ahbp_dump_fn_t fn;
} sys_ahbp_reg_fn_map_t;

static void sys_ahbp_dump_reg0(void)
{
	SOC_LOGI("reg0: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x0 << 2)));
}

static void sys_ahbp_dump_reg1(void)
{
	SOC_LOGI("reg1: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x1 << 2)));
}

static void sys_ahbp_dump_reg2(void)
{
	sys_ahbp_reg2_t *r = (sys_ahbp_reg2_t *)(SOC_SYS_AHBP_REG_BASE + (0x2 << 2));

	SOC_LOGI("reg2: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x2 << 2)));
	SOC_LOGI("	reserved_0_0: %8x\r\n", r->reserved_0_0);
	SOC_LOGI("	clkg_bypass: %8x\r\n", r->clkg_bypass);
	SOC_LOGI("	reserved_2_31: %8x\r\n", r->reserved_2_31);
}

static void sys_ahbp_dump_reg3(void)
{
	sys_ahbp_reg3_t *r = (sys_ahbp_reg3_t *)(SOC_SYS_AHBP_REG_BASE + (0x3 << 2));

	SOC_LOGI("reg3: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x3 << 2)));
	SOC_LOGI("	cpu0_sleeping: %8x\r\n", r->cpu0_sleeping);
	SOC_LOGI("	cpu0_deepsleep: %8x\r\n", r->cpu0_deepsleep);
	SOC_LOGI("	cpu1_sleeping: %8x\r\n", r->cpu1_sleeping);
	SOC_LOGI("	cpu1_deepsleep: %8x\r\n", r->cpu1_deepsleep);
	SOC_LOGI("	reserved_bit_4_31: %8x\r\n", r->reserved_bit_4_31);
}

static void sys_ahbp_dump_reg4(void)
{
	sys_ahbp_reg4_t *r = (sys_ahbp_reg4_t *)(SOC_SYS_AHBP_REG_BASE + (0x4 << 2));

	SOC_LOGI("reg4: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x4 << 2)));
	SOC_LOGI("	cpu0_sw_rstn: %8x\r\n", r->cpu0_sw_rstn);
	SOC_LOGI("	cpu0_init_dtcm_en: %8x\r\n", r->cpu0_init_dtcm_en);
	SOC_LOGI("	cpu0_sys_nmi: %8x\r\n", r->cpu0_sys_nmi);
	SOC_LOGI("	cpu0_wfe_src: %8x\r\n", r->cpu0_wfe_src);
	SOC_LOGI("	cpu0_wfe_pulse: %8x\r\n", r->cpu0_wfe_pulse);
	SOC_LOGI("	cpu0_wait: %8x\r\n", r->cpu0_wait);
	SOC_LOGI("	cpu0_dbg_rstn_disable: %8x\r\n", r->cpu0_dbg_rstn_disable);
	SOC_LOGI("	reserved_7_7: %8x\r\n", r->reserved_7_7);
	SOC_LOGI("	cpu0_offset: %8x\r\n", r->cpu0_offset);
}

static void sys_ahbp_dump_reg5(void)
{
	sys_ahbp_reg5_t *r = (sys_ahbp_reg5_t *)(SOC_SYS_AHBP_REG_BASE + (0x5 << 2));

	SOC_LOGI("reg5: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x5 << 2)));
	SOC_LOGI("	cpu1_sw_rstn: %8x\r\n", r->cpu1_sw_rstn);
	SOC_LOGI("	cpu1_init_dtcm_en: %8x\r\n", r->cpu1_init_dtcm_en);
	SOC_LOGI("	cpu1_sys_nmi: %8x\r\n", r->cpu1_sys_nmi);
	SOC_LOGI("	cpu1_wfe_src: %8x\r\n", r->cpu1_wfe_src);
	SOC_LOGI("	cpu1_wfe_pulse: %8x\r\n", r->cpu1_wfe_pulse);
	SOC_LOGI("	cpu1_wait: %8x\r\n", r->cpu1_wait);
	SOC_LOGI("	cpu1_dbg_rstn_disable: %8x\r\n", r->cpu1_dbg_rstn_disable);
	SOC_LOGI("	reserved_7_7: %8x\r\n", r->reserved_7_7);
	SOC_LOGI("	cpu1_offset: %8x\r\n", r->cpu1_offset);
}

static void sys_ahbp_dump_reg6(void)
{
	sys_ahbp_reg6_t *r = (sys_ahbp_reg6_t *)(SOC_SYS_AHBP_REG_BASE + (0x6 << 2));

	SOC_LOGI("reg6: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x6 << 2)));
	SOC_LOGI("	npu_sw_rstn: %8x\r\n", r->npu_sw_rstn);
	SOC_LOGI("	npu_clkbps: %8x\r\n", r->npu_clkbps);
	SOC_LOGI("	axi0_awcache: %8x\r\n", r->axi0_awcache);
	SOC_LOGI("	axi0_arcache: %8x\r\n", r->axi0_arcache);
	SOC_LOGI("	axi1_awcache: %8x\r\n", r->axi1_awcache);
	SOC_LOGI("	axi1_arcache: %8x\r\n", r->axi1_arcache);
	SOC_LOGI("	cache_src: %8x\r\n", r->cache_src);
	SOC_LOGI("	reserved_19_31: %8x\r\n", r->reserved_19_31);
}

static void sys_ahbp_dump_reg7(void)
{
	sys_ahbp_reg7_t *r = (sys_ahbp_reg7_t *)(SOC_SYS_AHBP_REG_BASE + (0x7 << 2));

	SOC_LOGI("reg7: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x7 << 2)));
	SOC_LOGI("	psram_inv_config: %8x\r\n", r->psram_inv_config);
	SOC_LOGI("	videopost_m_arcache: %8x\r\n", r->videopost_m_arcache);
	SOC_LOGI("	videopost_m_awcache: %8x\r\n", r->videopost_m_awcache);
	SOC_LOGI("	reserved_10_15: %8x\r\n", r->reserved_10_15);
	SOC_LOGI("	icache_clean_mode: %8x\r\n", r->icache_clean_mode);
	SOC_LOGI("	cpu0_icache_clean_mode: %8x\r\n", r->cpu0_icache_clean_mode);
	SOC_LOGI("	cpu0_icache_clean_tag_sel: %8x\r\n", r->cpu0_icache_clean_tag_sel);
	SOC_LOGI("	cpu1_icache_clean_mode: %8x\r\n", r->cpu1_icache_clean_mode);
	SOC_LOGI("	cpu1_icache_clean_tag_sel: %8x\r\n", r->cpu1_icache_clean_tag_sel);
	SOC_LOGI("	reserved_21_23: %8x\r\n", r->reserved_21_23);
	SOC_LOGI("	icache_clean_key: %8x\r\n", r->icache_clean_key);
}

static void sys_ahbp_dump_reg8(void)
{
	sys_ahbp_reg8_t *r = (sys_ahbp_reg8_t *)(SOC_SYS_AHBP_REG_BASE + (0x8 << 2));

	SOC_LOGI("reg8: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x8 << 2)));
	SOC_LOGI("	cksel_core: %8x\r\n", r->cksel_core);
	SOC_LOGI("	ckdiv_core: %8x\r\n", r->ckdiv_core);
	SOC_LOGI("	ckdiv_bus_ls: %8x\r\n", r->ckdiv_bus_ls);
	SOC_LOGI("	reserved_5_5: %8x\r\n", r->reserved_5_5);
	SOC_LOGI("	ckdiv_uart5: %8x\r\n", r->ckdiv_uart5);
	SOC_LOGI("	cksel_qspi0: %8x\r\n", r->cksel_qspi0);
	SOC_LOGI("	ckdiv_qspi0: %8x\r\n", r->ckdiv_qspi0);
	SOC_LOGI("	cksel_qspi1: %8x\r\n", r->cksel_qspi1);
	SOC_LOGI("	ckdiv_qspi1: %8x\r\n", r->ckdiv_qspi1);
	SOC_LOGI("	cksel_pram0: %8x\r\n", r->cksel_pram0);
	SOC_LOGI("	ckdiv_pram0: %8x\r\n", r->ckdiv_pram0);
	SOC_LOGI("	cksel_mbist: %8x\r\n", r->cksel_mbist);
	SOC_LOGI("	ckdiv_sdio0: %8x\r\n", r->ckdiv_sdio0);
	SOC_LOGI("	ckdiv_sdio1: %8x\r\n", r->ckdiv_sdio1);
}

static void sys_ahbp_dump_reg9(void)
{
	sys_ahbp_reg9_t *r = (sys_ahbp_reg9_t *)(SOC_SYS_AHBP_REG_BASE + (0x9 << 2));

	SOC_LOGI("reg9: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x9 << 2)));
	SOC_LOGI("	cksel_cis_mclk: %8x\r\n", r->cksel_cis_mclk);
	SOC_LOGI("	ckdiv_cis_mclk: %8x\r\n", r->ckdiv_cis_mclk);
	SOC_LOGI("	ckdiv_cis_auxs: %8x\r\n", r->ckdiv_cis_auxs);
	SOC_LOGI("	cksel_cisp: %8x\r\n", r->cksel_cisp);
	SOC_LOGI("	ckdiv_cisp: %8x\r\n", r->ckdiv_cisp);
	SOC_LOGI("	cksel_gpu: %8x\r\n", r->cksel_gpu);
	SOC_LOGI("	ckdiv_gpu: %8x\r\n", r->ckdiv_gpu);
	SOC_LOGI("	cksel_h265: %8x\r\n", r->cksel_h265);
	SOC_LOGI("	ckdiv_h265: %8x\r\n", r->ckdiv_h265);
	SOC_LOGI("	cksel_dpu: %8x\r\n", r->cksel_dpu);
	SOC_LOGI("	ckdiv_dpu: %8x\r\n", r->ckdiv_dpu);
	SOC_LOGI("	cksel_pram1: %8x\r\n", r->cksel_pram1);
	SOC_LOGI("	ckdiv_pram1: %8x\r\n", r->ckdiv_pram1);
	SOC_LOGI("	ckdiv_trace: %8x\r\n", r->ckdiv_trace);
	SOC_LOGI("	cksel_cis_auxs: %8x\r\n", r->cksel_cis_auxs);
}

static void sys_ahbp_dump_rega(void)
{
	sys_ahbp_rega_t *r = (sys_ahbp_rega_t *)(SOC_SYS_AHBP_REG_BASE + (0xa << 2));

	SOC_LOGI("rega: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0xa << 2)));
	SOC_LOGI("	cpua_cken: %8x\r\n", r->cpua_cken);
	SOC_LOGI("	uart5_cken: %8x\r\n", r->uart5_cken);
	SOC_LOGI("	usb_hs_cken: %8x\r\n", r->usb_hs_cken);
	SOC_LOGI("	pram0_cken: %8x\r\n", r->pram0_cken);
	SOC_LOGI("	pram1_cken: %8x\r\n", r->pram1_cken);
	SOC_LOGI("	qspi0_cken: %8x\r\n", r->qspi0_cken);
	SOC_LOGI("	qspi1_cken: %8x\r\n", r->qspi1_cken);
	SOC_LOGI("	sdio0_cken: %8x\r\n", r->sdio0_cken);
	SOC_LOGI("	sdio1_cken: %8x\r\n", r->sdio1_cken);
	SOC_LOGI("	cisp_cken: %8x\r\n", r->cisp_cken);
	SOC_LOGI("	gpu_cken: %8x\r\n", r->gpu_cken);
	SOC_LOGI("	h26e_cken: %8x\r\n", r->h26e_cken);
	SOC_LOGI("	csi_cken: %8x\r\n", r->csi_cken);
	SOC_LOGI("	dsi_cken: %8x\r\n", r->dsi_cken);
	SOC_LOGI("	dpu_cken: %8x\r\n", r->dpu_cken);
	SOC_LOGI("	usb_fs_cken: %8x\r\n", r->usb_fs_cken);
	SOC_LOGI("	conf0_cken: %8x\r\n", r->conf0_cken);
	SOC_LOGI("	conf1_cken: %8x\r\n", r->conf1_cken);
	SOC_LOGI("	conf2_cken: %8x\r\n", r->conf2_cken);
	SOC_LOGI("	conf3_cken: %8x\r\n", r->conf3_cken);
	SOC_LOGI("	cpu0_cken: %8x\r\n", r->cpu0_cken);
	SOC_LOGI("	cpu1_cken: %8x\r\n", r->cpu1_cken);
	SOC_LOGI("	npu_cken: %8x\r\n", r->npu_cken);
	SOC_LOGI("	timer4_cken: %8x\r\n", r->timer4_cken);
	SOC_LOGI("	timer5_cken: %8x\r\n", r->timer5_cken);
	SOC_LOGI("	trace_cken: %8x\r\n", r->trace_cken);
	SOC_LOGI("	reserved_26_31: %8x\r\n", r->reserved_26_31);
}

static void sys_ahbp_dump_regb(void)
{
	sys_ahbp_regb_t *r = (sys_ahbp_regb_t *)(SOC_SYS_AHBP_REG_BASE + (0xb << 2));

	SOC_LOGI("regb: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0xb << 2)));
	SOC_LOGI("	phase_ck640: %8x\r\n", r->phase_ck640);
	SOC_LOGI("	reserved_8_31: %8x\r\n", r->reserved_8_31);
}

static void sys_ahbp_dump_regc(void)
{
	sys_ahbp_regc_t *r = (sys_ahbp_regc_t *)(SOC_SYS_AHBP_REG_BASE + (0xc << 2));

	SOC_LOGI("regc: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0xc << 2)));
	SOC_LOGI("	cpu0_mem_sd: %8x\r\n", r->cpu0_mem_sd);
	SOC_LOGI("	cpu1_mem_sd: %8x\r\n", r->cpu1_mem_sd);
	SOC_LOGI("	dtcm_mem_sd: %8x\r\n", r->dtcm_mem_sd);
	SOC_LOGI("	l2ch_mem_sd: %8x\r\n", r->l2ch_mem_sd);
	SOC_LOGI("	mem3_mem_sd: %8x\r\n", r->mem3_mem_sd);
	SOC_LOGI("	mem4_mem_sd: %8x\r\n", r->mem4_mem_sd);
	SOC_LOGI("	mem5_mem_sd: %8x\r\n", r->mem5_mem_sd);
	SOC_LOGI("	mem6_mem_sd: %8x\r\n", r->mem6_mem_sd);
	SOC_LOGI("	ahbp_mem_sd: %8x\r\n", r->ahbp_mem_sd);
	SOC_LOGI("	h265_mem_sd: %8x\r\n", r->h265_mem_sd);
	SOC_LOGI("	gpub_mem_sd: %8x\r\n", r->gpub_mem_sd);
	SOC_LOGI("	dpub_mem_sd: %8x\r\n", r->dpub_mem_sd);
	SOC_LOGI("	disb_mem_sd: %8x\r\n", r->disb_mem_sd);
	SOC_LOGI("	h264_mem_sd: %8x\r\n", r->h264_mem_sd);
	SOC_LOGI("	ispb_mem_sd: %8x\r\n", r->ispb_mem_sd);
	SOC_LOGI("	csib_mem_sd: %8x\r\n", r->csib_mem_sd);
	SOC_LOGI("	npub_mem_sd: %8x\r\n", r->npub_mem_sd);
	SOC_LOGI("	reserved_17_31: %8x\r\n", r->reserved_17_31);
}

static void sys_ahbp_dump_regd(void)
{
	sys_ahbp_regd_t *r = (sys_ahbp_regd_t *)(SOC_SYS_AHBP_REG_BASE + (0xd << 2));

	SOC_LOGI("regd: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0xd << 2)));
	SOC_LOGI("	cpu0_mem_ds: %8x\r\n", r->cpu0_mem_ds);
	SOC_LOGI("	cpu1_mem_ds: %8x\r\n", r->cpu1_mem_ds);
	SOC_LOGI("	dtcm_mem_ds: %8x\r\n", r->dtcm_mem_ds);
	SOC_LOGI("	l2ch_mem_ds: %8x\r\n", r->l2ch_mem_ds);
	SOC_LOGI("	mem3_mem_ds: %8x\r\n", r->mem3_mem_ds);
	SOC_LOGI("	mem4_mem_ds: %8x\r\n", r->mem4_mem_ds);
	SOC_LOGI("	mem5_mem_ds: %8x\r\n", r->mem5_mem_ds);
	SOC_LOGI("	mem6_mem_ds: %8x\r\n", r->mem6_mem_ds);
	SOC_LOGI("	ahbp_mem_ds: %8x\r\n", r->ahbp_mem_ds);
	SOC_LOGI("	h265_mem_ds: %8x\r\n", r->h265_mem_ds);
	SOC_LOGI("	gpub_mem_ds: %8x\r\n", r->gpub_mem_ds);
	SOC_LOGI("	dpub_mem_ds: %8x\r\n", r->dpub_mem_ds);
	SOC_LOGI("	disb_mem_ds: %8x\r\n", r->disb_mem_ds);
	SOC_LOGI("	h264_mem_ds: %8x\r\n", r->h264_mem_ds);
	SOC_LOGI("	ispb_mem_ds: %8x\r\n", r->ispb_mem_ds);
	SOC_LOGI("	csib_mem_ds: %8x\r\n", r->csib_mem_ds);
	SOC_LOGI("	npub_mem_ds: %8x\r\n", r->npub_mem_ds);
	SOC_LOGI("	reserved_17_31: %8x\r\n", r->reserved_17_31);
}

static void sys_ahbp_dump_rege(void)
{
	sys_ahbp_rege_t *r = (sys_ahbp_rege_t *)(SOC_SYS_AHBP_REG_BASE + (0xe << 2));

	SOC_LOGI("rege: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0xe << 2)));
	SOC_LOGI("	system_halt_en: %8x\r\n", r->system_halt_en);
	SOC_LOGI("	system_halt_high_cpu0wfi: %8x\r\n", r->system_halt_high_cpu0wfi);
	SOC_LOGI("	system_halt_high_cpu1wfi: %8x\r\n", r->system_halt_high_cpu1wfi);
	SOC_LOGI("	reserved_3_7: %8x\r\n", r->reserved_3_7);
	SOC_LOGI("	cpu_halt_en: %8x\r\n", r->cpu_halt_en);
	SOC_LOGI("	cpu_halt_high_cpu0wfi: %8x\r\n", r->cpu_halt_high_cpu0wfi);
	SOC_LOGI("	cpu_halt_high_cpu1wfi: %8x\r\n", r->cpu_halt_high_cpu1wfi);
	SOC_LOGI("	reserved_11_15: %8x\r\n", r->reserved_11_15);
	SOC_LOGI("	pwd_m55: %8x\r\n", r->pwd_m55);
	SOC_LOGI("	pwd_video_post: %8x\r\n", r->pwd_video_post);
	SOC_LOGI("	pwd_h26e: %8x\r\n", r->pwd_h26e);
	SOC_LOGI("	pwd_isp: %8x\r\n", r->pwd_isp);
	SOC_LOGI("	pwd_npu: %8x\r\n", r->pwd_npu);
	SOC_LOGI("	reserved_21_23: %8x\r\n", r->reserved_21_23);
	SOC_LOGI("	soft_rstn_isp  : %8x\r\n", r->soft_rstn_isp  );
	SOC_LOGI("	soft_rstn_h264e: %8x\r\n", r->soft_rstn_h264e);
	SOC_LOGI("	soft_rstn_h264d: %8x\r\n", r->soft_rstn_h264d);
	SOC_LOGI("	soft_rstn_gpu  : %8x\r\n", r->soft_rstn_gpu  );
	SOC_LOGI("	soft_rstn_dpu  : %8x\r\n", r->soft_rstn_dpu  );
	SOC_LOGI("	reserved_29_31: %8x\r\n", r->reserved_29_31);
}

static void sys_ahbp_dump_regf(void)
{
	sys_ahbp_regf_t *r = (sys_ahbp_regf_t *)(SOC_SYS_AHBP_REG_BASE + (0xf << 2));

	SOC_LOGI("regf: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0xf << 2)));
	SOC_LOGI("	wwdt_region_wwdt: %8x\r\n", r->wwdt_region_wwdt);
	SOC_LOGI("	wwdt_region_sys_cfg: %8x\r\n", r->wwdt_region_sys_cfg);
	SOC_LOGI("	wwdt_region_busx: %8x\r\n", r->wwdt_region_busx);
	SOC_LOGI("	wwdt_region_cpu0: %8x\r\n", r->wwdt_region_cpu0);
	SOC_LOGI("	wwdt_region_cpu1: %8x\r\n", r->wwdt_region_cpu1);
	SOC_LOGI("	wwdt_region_ahbp: %8x\r\n", r->wwdt_region_ahbp);
	SOC_LOGI("	wwdt_region_smem3: %8x\r\n", r->wwdt_region_smem3);
	SOC_LOGI("	wwdt_region_smem4: %8x\r\n", r->wwdt_region_smem4);
	SOC_LOGI("	wwdt_region_smem5: %8x\r\n", r->wwdt_region_smem5);
	SOC_LOGI("	wwdt_region_smem6: %8x\r\n", r->wwdt_region_smem6);
	SOC_LOGI("	wwdt_region_npu: %8x\r\n", r->wwdt_region_npu);
	SOC_LOGI("	wwdt_region_h26e: %8x\r\n", r->wwdt_region_h26e);
	SOC_LOGI("	wwdt_region_isp: %8x\r\n", r->wwdt_region_isp);
	SOC_LOGI("	wwdt_region_video_post: %8x\r\n", r->wwdt_region_video_post);
	SOC_LOGI("	reserved_14_31: %8x\r\n", r->reserved_14_31);
}

static void sys_ahbp_dump_reg10(void)
{
	SOC_LOGI("reg10: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x10 << 2)));
}

static void sys_ahbp_dump_reg11(void)
{
	SOC_LOGI("reg11: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x11 << 2)));
}

static void sys_ahbp_dump_reg12(void)
{
	SOC_LOGI("reg12: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x12 << 2)));
}

static void sys_ahbp_dump_reg13(void)
{
	SOC_LOGI("reg13: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x13 << 2)));
}

static void sys_ahbp_dump_reg14(void)
{
	SOC_LOGI("reg14: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x14 << 2)));
}

static void sys_ahbp_dump_reg15(void)
{
	SOC_LOGI("reg15: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x15 << 2)));
}

static void sys_ahbp_dump_reg16(void)
{
	SOC_LOGI("reg16: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x16 << 2)));
}

static void sys_ahbp_dump_reg17(void)
{
	SOC_LOGI("reg17: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x17 << 2)));
}

static void sys_ahbp_dump_reg18(void)
{
	SOC_LOGI("reg18: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x18 << 2)));
}

static void sys_ahbp_dump_reg19(void)
{
	SOC_LOGI("reg19: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x19 << 2)));
}

static void sys_ahbp_dump_reg1a(void)
{
	SOC_LOGI("reg1a: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x1a << 2)));
}

static void sys_ahbp_dump_reg1b(void)
{
	SOC_LOGI("reg1b: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x1b << 2)));
}

static void sys_ahbp_dump_reg1c(void)
{
	SOC_LOGI("reg1c: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x1c << 2)));
}

static void sys_ahbp_dump_reg1d(void)
{
	SOC_LOGI("reg1d: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x1d << 2)));
}

static void sys_ahbp_dump_reg1e(void)
{
	SOC_LOGI("reg1e: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x1e << 2)));
}

static void sys_ahbp_dump_reg1f(void)
{
	SOC_LOGI("reg1f: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x1f << 2)));
}

static void sys_ahbp_dump_reg20(void)
{
	sys_ahbp_reg20_t *r = (sys_ahbp_reg20_t *)(SOC_SYS_AHBP_REG_BASE + (0x20 << 2));

	SOC_LOGI("reg20: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x20 << 2)));
	SOC_LOGI("	cpu0_qos     : %8x\r\n", r->cpu0_qos     );
	SOC_LOGI("	dma1_qos     : %8x\r\n", r->dma1_qos     );
	SOC_LOGI("	sdio0_qos    : %8x\r\n", r->sdio0_qos    );
	SOC_LOGI("	sdio1_qos    : %8x\r\n", r->sdio1_qos    );
	SOC_LOGI("	enet0_qos    : %8x\r\n", r->enet0_qos    );
	SOC_LOGI("	enet1_qos    : %8x\r\n", r->enet1_qos    );
	SOC_LOGI("	usb_qos      : %8x\r\n", r->usb_qos      );
	SOC_LOGI("	h26e_qos     : %8x\r\n", r->h26e_qos     );
	SOC_LOGI("	isp_qos      : %8x\r\n", r->isp_qos      );
	SOC_LOGI("	videopost_qos: %8x\r\n", r->videopost_qos);
	SOC_LOGI("	dpu_qos      : %8x\r\n", r->dpu_qos      );
	SOC_LOGI("	cbus_qos: %8x\r\n", r->cbus_qos);
	SOC_LOGI("	npu0_qos: %8x\r\n", r->npu0_qos);
	SOC_LOGI("	npu1_qos: %8x\r\n", r->npu1_qos);
	SOC_LOGI("	cpu1_qos: %8x\r\n", r->cpu1_qos);
	SOC_LOGI("	reserved_30_31: %8x\r\n", r->reserved_30_31);
}

static void sys_ahbp_dump_reg21(void)
{
	sys_ahbp_reg21_t *r = (sys_ahbp_reg21_t *)(SOC_SYS_AHBP_REG_BASE + (0x21 << 2));

	SOC_LOGI("reg21: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x21 << 2)));
	SOC_LOGI("	dpu_gate_cpu_en: %8x\r\n", r->dpu_gate_cpu_en);
	SOC_LOGI("	dpu_gate_videopost_en: %8x\r\n", r->dpu_gate_videopost_en);
	SOC_LOGI("	dpu_gate_h26e_en: %8x\r\n", r->dpu_gate_h26e_en);
	SOC_LOGI("	reserved_3_7: %8x\r\n", r->reserved_3_7);
	SOC_LOGI("	reserved_8_8: %8x\r\n", r->reserved_8_8);
	SOC_LOGI("	dpu_gate_videopost_override: %8x\r\n", r->dpu_gate_videopost_override);
	SOC_LOGI("	dpu_gate_h26e_override: %8x\r\n", r->dpu_gate_h26e_override);
	SOC_LOGI("	reserved_11_15: %8x\r\n", r->reserved_11_15);
	SOC_LOGI("	cpu_bus_dis: %8x\r\n", r->cpu_bus_dis);
	SOC_LOGI("	videopost_bus_dis: %8x\r\n", r->videopost_bus_dis);
	SOC_LOGI("	h26e_bus_dis: %8x\r\n", r->h26e_bus_dis);
	SOC_LOGI("	reserved_19_23: %8x\r\n", r->reserved_19_23);
	SOC_LOGI("	dpu_gate_key: %8x\r\n", r->dpu_gate_key);
}

static void sys_ahbp_dump_reg22(void)
{
	SOC_LOGI("reg22: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x22 << 2)));
}

static void sys_ahbp_dump_reg23(void)
{
	sys_ahbp_reg23_t *r = (sys_ahbp_reg23_t *)(SOC_SYS_AHBP_REG_BASE + (0x23 << 2));

	SOC_LOGI("reg23: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x23 << 2)));
	SOC_LOGI("	dbug_mux: %8x\r\n", r->dbug_mux);
	SOC_LOGI("	dbug_config1: %8x\r\n", r->dbug_config1);
}

static void sys_ahbp_dump_reg24(void)
{
	SOC_LOGI("reg24: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x24 << 2)));
}

static void sys_ahbp_dump_reg25(void)
{
	SOC_LOGI("reg25: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x25 << 2)));
}

static void sys_ahbp_dump_reg26(void)
{
	SOC_LOGI("reg26: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x26 << 2)));
}

static void sys_ahbp_dump_reg27(void)
{
	SOC_LOGI("reg27: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x27 << 2)));
}

static void sys_ahbp_dump_reg28(void)
{
	sys_ahbp_reg28_t *r = (sys_ahbp_reg28_t *)(SOC_SYS_AHBP_REG_BASE + (0x28 << 2));

	SOC_LOGI("reg28: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x28 << 2)));
	SOC_LOGI("	gpu_buffa_enable      : %8x\r\n", r->gpu_buffa_enable      );
	SOC_LOGI("	reserved_bit_1_31: %8x\r\n", r->reserved_bit_1_31);
}

static void sys_ahbp_dump_reg29(void)
{
	SOC_LOGI("reg29: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x29 << 2)));
}

static void sys_ahbp_dump_reg2a(void)
{
	SOC_LOGI("reg2a: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x2a << 2)));
}

static void sys_ahbp_dump_reg2b(void)
{
	SOC_LOGI("reg2b: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x2b << 2)));
}

static void sys_ahbp_dump_reg2c(void)
{
	SOC_LOGI("reg2c: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x2c << 2)));
}

static void sys_ahbp_dump_reg2d(void)
{
	SOC_LOGI("reg2d: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x2d << 2)));
}

static void sys_ahbp_dump_reg2e(void)
{
	SOC_LOGI("reg2e: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x2e << 2)));
}

static void sys_ahbp_dump_reg2f(void)
{
	SOC_LOGI("reg2f: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x2f << 2)));
}

static void sys_ahbp_dump_reg30(void)
{
	sys_ahbp_reg30_t *r = (sys_ahbp_reg30_t *)(SOC_SYS_AHBP_REG_BASE + (0x30 << 2));

	SOC_LOGI("reg30: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x30 << 2)));
	SOC_LOGI("	gpu_buffb_enable      : %8x\r\n", r->gpu_buffb_enable      );
	SOC_LOGI("	reserved_bit_1_31: %8x\r\n", r->reserved_bit_1_31);
}

static void sys_ahbp_dump_reg31(void)
{
	SOC_LOGI("reg31: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x31 << 2)));
}

static void sys_ahbp_dump_reg32(void)
{
	SOC_LOGI("reg32: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x32 << 2)));
}

static void sys_ahbp_dump_reg33(void)
{
	SOC_LOGI("reg33: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x33 << 2)));
}

static void sys_ahbp_dump_reg34(void)
{
	SOC_LOGI("reg34: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x34 << 2)));
}

static void sys_ahbp_dump_reg35(void)
{
	SOC_LOGI("reg35: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x35 << 2)));
}

static void sys_ahbp_dump_reg36(void)
{
	SOC_LOGI("reg36: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x36 << 2)));
}

static void sys_ahbp_dump_reg37(void)
{
	SOC_LOGI("reg37: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x37 << 2)));
}

static void sys_ahbp_dump_reg38(void)
{
	sys_ahbp_reg38_t *r = (sys_ahbp_reg38_t *)(SOC_SYS_AHBP_REG_BASE + (0x38 << 2));

	SOC_LOGI("reg38: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x38 << 2)));
	SOC_LOGI("	h26d_buffa_enable     : %8x\r\n", r->h26d_buffa_enable     );
	SOC_LOGI("	reserved_bit_1_31: %8x\r\n", r->reserved_bit_1_31);
}

static void sys_ahbp_dump_reg39(void)
{
	SOC_LOGI("reg39: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x39 << 2)));
}

static void sys_ahbp_dump_reg3a(void)
{
	SOC_LOGI("reg3a: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x3a << 2)));
}

static void sys_ahbp_dump_reg3b(void)
{
	SOC_LOGI("reg3b: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x3b << 2)));
}

static void sys_ahbp_dump_reg3c(void)
{
	SOC_LOGI("reg3c: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x3c << 2)));
}

static void sys_ahbp_dump_reg3d(void)
{
	SOC_LOGI("reg3d: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x3d << 2)));
}

static void sys_ahbp_dump_reg3e(void)
{
	SOC_LOGI("reg3e: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x3e << 2)));
}

static void sys_ahbp_dump_reg3f(void)
{
	SOC_LOGI("reg3f: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x3f << 2)));
}

static void sys_ahbp_dump_reg40(void)
{
	sys_ahbp_reg40_t *r = (sys_ahbp_reg40_t *)(SOC_SYS_AHBP_REG_BASE + (0x40 << 2));

	SOC_LOGI("reg40: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x40 << 2)));
	SOC_LOGI("	h26d_buffb_enable     : %8x\r\n", r->h26d_buffb_enable     );
	SOC_LOGI("	reserved_bit_1_31: %8x\r\n", r->reserved_bit_1_31);
}

static void sys_ahbp_dump_reg41(void)
{
	SOC_LOGI("reg41: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x41 << 2)));
}

static void sys_ahbp_dump_reg42(void)
{
	SOC_LOGI("reg42: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x42 << 2)));
}

static void sys_ahbp_dump_reg43(void)
{
	SOC_LOGI("reg43: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x43 << 2)));
}

static void sys_ahbp_dump_reg44(void)
{
	SOC_LOGI("reg44: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x44 << 2)));
}

static void sys_ahbp_dump_reg45(void)
{
	SOC_LOGI("reg45: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x45 << 2)));
}

static void sys_ahbp_dump_reg46(void)
{
	SOC_LOGI("reg46: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x46 << 2)));
}

static void sys_ahbp_dump_reg47(void)
{
	SOC_LOGI("reg47: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x47 << 2)));
}

static void sys_ahbp_dump_reg48(void)
{
	sys_ahbp_reg48_t *r = (sys_ahbp_reg48_t *)(SOC_SYS_AHBP_REG_BASE + (0x48 << 2));

	SOC_LOGI("reg48: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x48 << 2)));
	SOC_LOGI("	h26d_buffc_enable     : %8x\r\n", r->h26d_buffc_enable     );
	SOC_LOGI("	reserved_bit_1_31: %8x\r\n", r->reserved_bit_1_31);
}

static void sys_ahbp_dump_reg49(void)
{
	SOC_LOGI("reg49: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x49 << 2)));
}

static void sys_ahbp_dump_reg4a(void)
{
	SOC_LOGI("reg4a: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x4a << 2)));
}

static void sys_ahbp_dump_reg4b(void)
{
	SOC_LOGI("reg4b: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x4b << 2)));
}

static void sys_ahbp_dump_reg4c(void)
{
	SOC_LOGI("reg4c: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x4c << 2)));
}

static void sys_ahbp_dump_reg4d(void)
{
	SOC_LOGI("reg4d: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x4d << 2)));
}

static void sys_ahbp_dump_reg4e(void)
{
	SOC_LOGI("reg4e: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x4e << 2)));
}

static void sys_ahbp_dump_reg4f(void)
{
	SOC_LOGI("reg4f: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x4f << 2)));
}

static void sys_ahbp_dump_reg50(void)
{
	sys_ahbp_reg50_t *r = (sys_ahbp_reg50_t *)(SOC_SYS_AHBP_REG_BASE + (0x50 << 2));

	SOC_LOGI("reg50: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x50 << 2)));
	SOC_LOGI("	ram_spsh_cfg: %8x\r\n", r->ram_spsh_cfg);
	SOC_LOGI("	ram_spbh_cfg: %8x\r\n", r->ram_spbh_cfg);
	SOC_LOGI("	reserved_bit_21_23: %8x\r\n", r->reserved_bit_21_23);
	SOC_LOGI("	ram_spsh_set_key: %8x\r\n", r->ram_spsh_set_key);
}

static void sys_ahbp_dump_reg51(void)
{
	sys_ahbp_reg51_t *r = (sys_ahbp_reg51_t *)(SOC_SYS_AHBP_REG_BASE + (0x51 << 2));

	SOC_LOGI("reg51: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x51 << 2)));
	SOC_LOGI("	ram_stph_cfg: %8x\r\n", r->ram_stph_cfg);
	SOC_LOGI("	reserved_bit_12_23: %8x\r\n", r->reserved_bit_12_23);
	SOC_LOGI("	ram_stph_set_key: %8x\r\n", r->ram_stph_set_key);
}

static void sys_ahbp_dump_reg52(void)
{
	sys_ahbp_reg52_t *r = (sys_ahbp_reg52_t *)(SOC_SYS_AHBP_REG_BASE + (0x52 << 2));

	SOC_LOGI("reg52: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x52 << 2)));
	SOC_LOGI("	ram_spsl_cfg: %8x\r\n", r->ram_spsl_cfg);
	SOC_LOGI("	ram_spbl_cfg: %8x\r\n", r->ram_spbl_cfg);
	SOC_LOGI("	reserved_bit_21_23: %8x\r\n", r->reserved_bit_21_23);
	SOC_LOGI("	ram_spsl_set_key: %8x\r\n", r->ram_spsl_set_key);
}

static void sys_ahbp_dump_reg53(void)
{
	sys_ahbp_reg53_t *r = (sys_ahbp_reg53_t *)(SOC_SYS_AHBP_REG_BASE + (0x53 << 2));

	SOC_LOGI("reg53: %8x\r\n", REG_READ(SOC_SYS_AHBP_REG_BASE + (0x53 << 2)));
	SOC_LOGI("	ram_stpl_cfg: %8x\r\n", r->ram_stpl_cfg);
	SOC_LOGI("	reserved_bit_12_23: %8x\r\n", r->reserved_bit_12_23);
	SOC_LOGI("	ram_stpl_set_key: %8x\r\n", r->ram_stpl_set_key);
}

static sys_ahbp_reg_fn_map_t s_fn[] =
{
	{0x0, 0x0, sys_ahbp_dump_reg0},
	{0x1, 0x1, sys_ahbp_dump_reg1},
	{0x2, 0x2, sys_ahbp_dump_reg2},
	{0x3, 0x3, sys_ahbp_dump_reg3},
	{0x4, 0x4, sys_ahbp_dump_reg4},
	{0x5, 0x5, sys_ahbp_dump_reg5},
	{0x6, 0x6, sys_ahbp_dump_reg6},
	{0x7, 0x7, sys_ahbp_dump_reg7},
	{0x8, 0x8, sys_ahbp_dump_reg8},
	{0x9, 0x9, sys_ahbp_dump_reg9},
	{0xa, 0xa, sys_ahbp_dump_rega},
	{0xb, 0xb, sys_ahbp_dump_regb},
	{0xc, 0xc, sys_ahbp_dump_regc},
	{0xd, 0xd, sys_ahbp_dump_regd},
	{0xe, 0xe, sys_ahbp_dump_rege},
	{0xf, 0xf, sys_ahbp_dump_regf},
	{0x10, 0x10, sys_ahbp_dump_reg10},
	{0x11, 0x11, sys_ahbp_dump_reg11},
	{0x12, 0x12, sys_ahbp_dump_reg12},
	{0x13, 0x13, sys_ahbp_dump_reg13},
	{0x14, 0x14, sys_ahbp_dump_reg14},
	{0x15, 0x15, sys_ahbp_dump_reg15},
	{0x16, 0x16, sys_ahbp_dump_reg16},
	{0x17, 0x17, sys_ahbp_dump_reg17},
	{0x18, 0x18, sys_ahbp_dump_reg18},
	{0x19, 0x19, sys_ahbp_dump_reg19},
	{0x1a, 0x1a, sys_ahbp_dump_reg1a},
	{0x1b, 0x1b, sys_ahbp_dump_reg1b},
	{0x1c, 0x1c, sys_ahbp_dump_reg1c},
	{0x1d, 0x1d, sys_ahbp_dump_reg1d},
	{0x1e, 0x1e, sys_ahbp_dump_reg1e},
	{0x1f, 0x1f, sys_ahbp_dump_reg1f},
	{0x20, 0x20, sys_ahbp_dump_reg20},
	{0x21, 0x21, sys_ahbp_dump_reg21},
	{0x22, 0x22, sys_ahbp_dump_reg22},
	{0x23, 0x23, sys_ahbp_dump_reg23},
	{0x24, 0x24, sys_ahbp_dump_reg24},
	{0x25, 0x25, sys_ahbp_dump_reg25},
	{0x26, 0x26, sys_ahbp_dump_reg26},
	{0x27, 0x27, sys_ahbp_dump_reg27},
	{0x28, 0x28, sys_ahbp_dump_reg28},
	{0x29, 0x29, sys_ahbp_dump_reg29},
	{0x2a, 0x2a, sys_ahbp_dump_reg2a},
	{0x2b, 0x2b, sys_ahbp_dump_reg2b},
	{0x2c, 0x2c, sys_ahbp_dump_reg2c},
	{0x2d, 0x2d, sys_ahbp_dump_reg2d},
	{0x2e, 0x2e, sys_ahbp_dump_reg2e},
	{0x2f, 0x2f, sys_ahbp_dump_reg2f},
	{0x30, 0x30, sys_ahbp_dump_reg30},
	{0x31, 0x31, sys_ahbp_dump_reg31},
	{0x32, 0x32, sys_ahbp_dump_reg32},
	{0x33, 0x33, sys_ahbp_dump_reg33},
	{0x34, 0x34, sys_ahbp_dump_reg34},
	{0x35, 0x35, sys_ahbp_dump_reg35},
	{0x36, 0x36, sys_ahbp_dump_reg36},
	{0x37, 0x37, sys_ahbp_dump_reg37},
	{0x38, 0x38, sys_ahbp_dump_reg38},
	{0x39, 0x39, sys_ahbp_dump_reg39},
	{0x3a, 0x3a, sys_ahbp_dump_reg3a},
	{0x3b, 0x3b, sys_ahbp_dump_reg3b},
	{0x3c, 0x3c, sys_ahbp_dump_reg3c},
	{0x3d, 0x3d, sys_ahbp_dump_reg3d},
	{0x3e, 0x3e, sys_ahbp_dump_reg3e},
	{0x3f, 0x3f, sys_ahbp_dump_reg3f},
	{0x40, 0x40, sys_ahbp_dump_reg40},
	{0x41, 0x41, sys_ahbp_dump_reg41},
	{0x42, 0x42, sys_ahbp_dump_reg42},
	{0x43, 0x43, sys_ahbp_dump_reg43},
	{0x44, 0x44, sys_ahbp_dump_reg44},
	{0x45, 0x45, sys_ahbp_dump_reg45},
	{0x46, 0x46, sys_ahbp_dump_reg46},
	{0x47, 0x47, sys_ahbp_dump_reg47},
	{0x48, 0x48, sys_ahbp_dump_reg48},
	{0x49, 0x49, sys_ahbp_dump_reg49},
	{0x4a, 0x4a, sys_ahbp_dump_reg4a},
	{0x4b, 0x4b, sys_ahbp_dump_reg4b},
	{0x4c, 0x4c, sys_ahbp_dump_reg4c},
	{0x4d, 0x4d, sys_ahbp_dump_reg4d},
	{0x4e, 0x4e, sys_ahbp_dump_reg4e},
	{0x4f, 0x4f, sys_ahbp_dump_reg4f},
	{0x50, 0x50, sys_ahbp_dump_reg50},
	{0x51, 0x51, sys_ahbp_dump_reg51},
	{0x52, 0x52, sys_ahbp_dump_reg52},
	{0x53, 0x53, sys_ahbp_dump_reg53},
	{-1, -1, 0}
};

void sys_ahbp_struct_dump(uint32_t start, uint32_t end)
{
	uint32_t dump_fn_cnt = sizeof(s_fn)/sizeof(s_fn[0]) - 1;

	for (uint32_t idx = 0; idx < dump_fn_cnt; idx++) {
		if ((start <= s_fn[idx].start) && (end >= s_fn[idx].end)) {
			s_fn[idx].fn();
		}
	}
}
