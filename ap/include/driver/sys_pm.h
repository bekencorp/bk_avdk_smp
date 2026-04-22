// Copyright 2020-2021 Beken
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

#pragma once

#include <driver/sys_pm_types.h>
#include <sys_types.h>

//Buck
int sys_pm_set_buck(sys_buck_type_t buck, bool ena);
int sys_pm_set_buck_pfm(sys_buck_type_t buck, bool ena);
int sys_pm_set_buck_burst(sys_buck_type_t buck, bool ena);
int sys_pm_set_buck_mpo(sys_buck_type_t buck, bool ena);

int sys_pm_set_ldo_self_lp(sys_ldo_type_t ldo, bool ena);
int sys_pm_set_ldo_current_limit(sys_ldo_type_t ldo, bool ena);
int sys_pm_set_aon_power(sys_aon_power_t power);

int sys_pm_set_aon_ldo_volt(uint32_t volt);
int sys_pm_set_io_ldo_volt(uint32_t volt);
int sys_pm_set_ana_ldo_volt(bool trsw_ena, uint32_t rx_volt, uint32_t tx_volt);
int sys_pm_set_digital_ldo_volt(bool lp_ena, uint32_t low_volt, uint32_t high_volt);
int sys_pm_set_core_ldo_volt(bool lp_ena, uint32_t low_volt, uint32_t high_volt);

int sys_pm_set_lv_ctrl_pd(bool ena);
int sys_pm_set_lv_ctrl_hf(bool ena);
int sys_pm_set_lv_ctrl_flash(bool ena);
int sys_pm_set_lv_ctrl_core(bool ena);

int sys_pm_set_lpo_src(sys_lpo_src_t src);

void sys_pm_dump_ctrl(void);

void sys_pm_set_power(power_module_name_t module, power_module_state_t state);
uint32_t sys_pm_get_power(power_module_name_t module);

/*AP: Clock Selection and Frequency Division*/
bk_err_t sys_drv_qspi0_cksel_clkdiv_set(cksel_qspi0_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_qspi1_cksel_clkdiv_set(cksel_qspi1_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_pram0_cksel_clkdiv_set(cksel_pram0_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_mbist_cksel_set(cksel_mbist_t cksel);
bk_err_t sys_drv_sdio0_clkdiv_set(uint32_t ckdiv);
bk_err_t sys_drv_sdio1_clkdiv_set(uint32_t ckdiv);
bk_err_t sys_drv_cis_mclk_cksel_clkdiv_set(cksel_cis_mclk_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_cis_auxs_clkdiv_set(uint32_t ckdiv);
bk_err_t sys_drv_cisp_cksel_clkdiv_set(cksel_cisp_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_gpu_cksel_clkdiv_set(cksel_gpu_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_h265_cksel_clkdiv_set(cksel_h265_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_dpu_cksel_clkdiv_set(cksel_dpu_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_pram1_cksel_clkdiv_set(cksel_pram1_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_trace_clkdiv_set(uint32_t ckdiv);
bk_err_t sys_drv_cis_auxs_cksel_set(cksel_cis_auxs_t cksel);

/*CP: Clock Selection and Frequency Division*/
bk_err_t sys_drv_flash_cksel_clkdiv_set(cksel_sys_flash_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_auxs_cksel_clkdiv_set(cksel_sys_auxs_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_26mo_clkdiv_set(uint32_t ckdiv);
bk_err_t sys_drv_tim0_cksel_set(cksel_sys_tim_t cksel);
bk_err_t sys_drv_tim1_cksel_set(cksel_sys_tim_t cksel);
bk_err_t sys_drv_tim2_cksel_set(cksel_sys_tim_t cksel);
bk_err_t sys_drv_tim3_cksel_set(cksel_sys_tim_t cksel);
bk_err_t sys_drv_i3c_cksel_set(cksel_sys_xtal_apll_t cksel);
bk_err_t sys_drv_sadc_cksel_set(cksel_sys_xtal_apll_t cksel);
bk_err_t sys_drv_i2s0_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_i2s1_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_i2s2_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_i2s3_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_i2s4_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_spi0_cksel_set(cksel_sys_xtal_160m_t cksel);
bk_err_t sys_drv_spi1_cksel_set(cksel_sys_xtal_160m_t cksel);
bk_err_t sys_drv_spi2_cksel_set(cksel_sys_xtal_160m_t cksel);
bk_err_t sys_drv_spi3_cksel_set(cksel_sys_xtal_160m_t cksel);
bk_err_t sys_drv_uart0_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_uart1_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_uart2_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_uart3_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_uart4_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_i2c0_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_i2c3_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_pwm0_cksel_set(cksel_sys_pwm0_t cksel);
bk_err_t sys_drv_can0_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_can1_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_scr0_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_drv_audio_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_audif0_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_audif1_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_i2so_clkdiv_set(uint32_t ckdiv);
bk_err_t sys_drv_auxs_enet_cksel_clkdiv_set(cksel_sys_dco_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_drv_trace_cksel_clkdiv_set(cksel_sys_trace_t cksel, uint32_t ckdiv);


bk_err_t sys_drv_auxldo_enable(auxldo_sel_t auxldo_sel,uint32_t value);
uint32_t sys_drv_auxldo_enable_state_get(auxldo_sel_t auxldo_sel);
bk_err_t sys_drv_auxldo_swb_set(auxldo_swb_t swb, bool bypass);
uint32_t sys_drv_auxldo_swb_get(auxldo_swb_t swb);
bk_err_t sys_drv_auxldo_out_set(auxldo_sel_t auxldo_sel, uint32_t out);
uint32_t sys_drv_auxldo_out_get(auxldo_sel_t auxldo_sel);
