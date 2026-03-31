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

#include "hal_config.h"
#include "aud_hw.h"
#include "audio_fifo_hw.h"

#include <driver/aud_adc_types.h>
#include <driver/aud_dtmf_types.h>
#include <driver/aud_dmic_types.h>
#include <driver/aud_dac_types.h>

#ifdef __cplusplus
extern "C" {
#endif


/* AUDIO_REG */
//reg device_id:
#define audio_reg_hal_set_device_id_value(v)                                audio_reg_ll_set_device_id_value(v)
#define audio_reg_hal_get_device_id_value()                                 audio_reg_ll_get_device_id_value()

#define audio_reg_hal_get_device_id_device_id()                             audio_reg_ll_get_device_id_device_id()

//reg version_id:
#define audio_reg_hal_set_version_id_value(v)                               audio_reg_ll_set_version_id_value(v)

#define audio_reg_hal_get_version_id_value()                                audio_reg_ll_get_version_id_value()
#define audio_reg_hal_get_version_id_version_id()                           audio_reg_ll_get_version_id_version_id()

//reg reserved0:
#define audio_reg_hal_set_reserved0_value(v)                                audio_reg_ll_set_reserved0_value(v)

#define audio_reg_hal_get_reserved0_value()                                 audio_reg_ll_get_reserved0_value()
#define audio_reg_hal_get_reserved0_reserved0()                             audio_reg_ll_get_reserved0_reserved0()

//reg reserved1:
#define audio_reg_hal_set_reserved1_value(v)                                audio_reg_ll_set_reserved1_value(v)

#define audio_reg_hal_get_reserved1_value()                                 audio_reg_ll_get_reserved1_value()
#define audio_reg_hal_get_reserved1_reserved1()                             audio_reg_ll_get_reserved1_reserved1()

//reg sys_cfg:
#define audio_reg_hal_set_sys_cfg_value(v)                                  audio_reg_ll_set_sys_cfg_value(v)
#define audio_reg_hal_get_sys_cfg_value()                                   audio_reg_ll_get_sys_cfg_value()

#define audio_reg_hal_set_sys_cfg_digmic_en(v)                              audio_reg_ll_set_sys_cfg_digmic_en(v)
#define audio_reg_hal_get_sys_cfg_digmic_en()                               audio_reg_ll_get_sys_cfg_digmic_en()

#define audio_reg_hal_set_sys_cfg_dmic_sel(v)                               audio_reg_ll_set_sys_cfg_dmic_sel(v)
#define audio_reg_hal_get_sys_cfg_dmic_sel()                                audio_reg_ll_get_sys_cfg_dmic_sel()

#define audio_reg_hal_set_sys_cfg_spl_sel_adc(v)                            audio_reg_ll_set_sys_cfg_spl_sel_adc(v)
#define audio_reg_hal_get_sys_cfg_spl_sel_adc()                             audio_reg_ll_get_sys_cfg_spl_sel_adc()

#define audio_reg_hal_set_sys_cfg_mem_dac_iir1x_sw_init(v)                  audio_reg_ll_set_sys_cfg_mem_dac_iir1x_sw_init(v)
#define audio_reg_hal_get_sys_cfg_mem_dac_iir1x_sw_init()                   audio_reg_ll_get_sys_cfg_mem_dac_iir1x_sw_init()

#define audio_reg_hal_set_sys_cfg_mem_auto_init_trig(v)                     audio_reg_ll_set_sys_cfg_mem_auto_init_trig(v)

#define audio_reg_hal_set_sys_cfg_mem_dac_sw_init(v)                        audio_reg_ll_set_sys_cfg_mem_dac_sw_init(v)
#define audio_reg_hal_get_sys_cfg_mem_dac_sw_init()                         audio_reg_ll_get_sys_cfg_mem_dac_sw_init()

#define audio_reg_hal_set_sys_cfg_mem_mic_sw_init(v)                        audio_reg_ll_set_sys_cfg_mem_mic_sw_init(v)
#define audio_reg_hal_get_sys_cfg_mem_mic_sw_init()                         audio_reg_ll_get_sys_cfg_mem_mic_sw_init()

#define audio_reg_hal_set_sys_cfg_mem_anc_sw_init(v)                        audio_reg_ll_set_sys_cfg_mem_anc_sw_init(v)
#define audio_reg_hal_get_sys_cfg_mem_anc_sw_init()                         audio_reg_ll_get_sys_cfg_mem_anc_sw_init()

#define audio_reg_hal_set_sys_cfg_clk_mic_sel(v)                            audio_reg_ll_set_sys_cfg_clk_mic_sel(v)
#define audio_reg_hal_get_sys_cfg_clk_mic_sel()                             audio_reg_ll_get_sys_cfg_clk_mic_sel()

#define audio_reg_hal_set_sys_cfg_rx_sp_sel(v)                              audio_reg_ll_set_sys_cfg_rx_sp_sel(v)
#define audio_reg_hal_get_sys_cfg_rx_sp_sel()                               audio_reg_ll_get_sys_cfg_rx_sp_sel()

#define audio_reg_hal_set_sys_cfg_mem_dac_eq_sw_init(v)                     audio_reg_ll_set_sys_cfg_mem_dac_eq_sw_init(v)
#define audio_reg_hal_get_sys_cfg_mem_dac_eq_sw_init()                      audio_reg_ll_get_sys_cfg_mem_dac_eq_sw_init()

#define audio_reg_hal_set_sys_cfg_mem_dac_comp_sw_init(v)                   audio_reg_ll_set_sys_cfg_mem_dac_comp_sw_init(v)
#define audio_reg_hal_get_sys_cfg_mem_dac_comp_sw_init()                    audio_reg_ll_get_sys_cfg_mem_dac_comp_sw_init()

#define audio_reg_hal_set_sys_cfg_apb_clk_en_dis(v)                         audio_reg_ll_set_sys_cfg_apb_clk_en_dis(v)
#define audio_reg_hal_get_sys_cfg_apb_clk_en_dis()                          audio_reg_ll_get_sys_cfg_apb_clk_en_dis()

//reg a2dp_comp:
#define audio_reg_hal_set_a2dp_comp_value(v)                                audio_reg_ll_set_a2dp_comp_value(v)
#define audio_reg_hal_get_a2dp_comp_value()                                 audio_reg_ll_get_a2dp_comp_value()

#define audio_reg_hal_set_a2dp_comp_anc_comp_st_val(v)                      audio_reg_ll_set_a2dp_comp_anc_comp_st_val(v)
#define audio_reg_hal_get_a2dp_comp_anc_comp_st_val()                       audio_reg_ll_get_a2dp_comp_anc_comp_st_val()

#define audio_reg_hal_set_a2dp_comp_anc_comp_en_frc(v)                      audio_reg_ll_set_a2dp_comp_anc_comp_en_frc(v)
#define audio_reg_hal_get_a2dp_comp_anc_comp_en_frc()                       audio_reg_ll_get_a2dp_comp_anc_comp_en_frc()

//reg adc_cfg:
#define audio_reg_hal_set_adc_cfg_value(v)                                  audio_reg_ll_set_adc_cfg_value(v)
#define audio_reg_hal_get_adc_cfg_value()                                   audio_reg_ll_get_adc_cfg_value()

#define audio_reg_hal_set_adc_cfg_aec_en(v)                                 audio_reg_ll_set_adc_cfg_aec_en(v)
#define audio_reg_hal_get_adc_cfg_aec_en()                                  audio_reg_ll_get_adc_cfg_aec_en()

#define audio_reg_hal_set_adc_cfg_adc_16b_sel(v)                            audio_reg_ll_set_adc_cfg_adc_16b_sel(v)
#define audio_reg_hal_get_adc_cfg_adc_16b_sel()                             audio_reg_ll_get_adc_cfg_adc_16b_sel()

#define audio_reg_hal_set_adc_cfg_aec_16b_sel(v)                            audio_reg_ll_set_adc_cfg_aec_16b_sel(v)
#define audio_reg_hal_get_adc_cfg_aec_16b_sel()                             audio_reg_ll_get_adc_cfg_aec_16b_sel()

#define audio_reg_hal_set_adc_cfg_adc_en(v)                                 audio_reg_ll_set_adc_cfg_adc_en(v)
#define audio_reg_hal_get_adc_cfg_adc_en()                                  audio_reg_ll_get_adc_cfg_adc_en()

#define audio_reg_hal_set_adc_cfg_adc_lpf_bps1(v)                           audio_reg_ll_set_adc_cfg_adc_lpf_bps1(v)
#define audio_reg_hal_get_adc_cfg_adc_lpf_bps1()                            audio_reg_ll_get_adc_cfg_adc_lpf_bps1()

#define audio_reg_hal_set_adc_cfg_adc_lpf_bps2(v)                           audio_reg_ll_set_adc_cfg_adc_lpf_bps2(v)
#define audio_reg_hal_get_adc_cfg_adc_lpf_bps2()                            audio_reg_ll_get_adc_cfg_adc_lpf_bps2()

#define audio_reg_hal_set_adc_cfg_adc_lpf_bps3(v)                           audio_reg_ll_set_adc_cfg_adc_lpf_bps3(v)
#define audio_reg_hal_get_adc_cfg_adc_lpf_bps3()                            audio_reg_ll_get_adc_cfg_adc_lpf_bps3()

#define audio_reg_hal_set_adc_cfg_adc_hpf_bps(v)                            audio_reg_ll_set_adc_cfg_adc_hpf_bps(v)
#define audio_reg_hal_get_adc_cfg_adc_hpf_bps()                             audio_reg_ll_get_adc_cfg_adc_hpf_bps()

#define audio_reg_hal_set_adc_cfg_clk_adc_inv(v)                            audio_reg_ll_set_adc_cfg_clk_adc_inv(v)
#define audio_reg_hal_get_adc_cfg_clk_adc_inv()                             audio_reg_ll_get_adc_cfg_clk_adc_inv()

//reg anc_cfg:
#define audio_reg_hal_set_anc_cfg_value(v)                                  audio_reg_ll_set_anc_cfg_value(v)
#define audio_reg_hal_get_anc_cfg_value()                                   audio_reg_ll_get_anc_cfg_value()

#define audio_reg_hal_set_anc_cfg_anc_frc_on(v)                             audio_reg_ll_set_anc_cfg_anc_frc_on(v)
#define audio_reg_hal_get_anc_cfg_anc_frc_on()                              audio_reg_ll_get_anc_cfg_anc_frc_on()

#define audio_reg_hal_set_anc_cfg_anc0_ramp_bps(v)                          audio_reg_ll_set_anc_cfg_anc0_ramp_bps(v)
#define audio_reg_hal_get_anc_cfg_anc0_ramp_bps()                           audio_reg_ll_get_anc_cfg_anc0_ramp_bps()

#define audio_reg_hal_set_anc_cfg_anc1_ramp_bps(v)                          audio_reg_ll_set_anc_cfg_anc1_ramp_bps(v)
#define audio_reg_hal_get_anc_cfg_anc1_ramp_bps()                           audio_reg_ll_get_anc_cfg_anc1_ramp_bps()

#define audio_reg_hal_set_anc_cfg_anc0_ramp_down_trig(v)                    audio_reg_ll_set_anc_cfg_anc0_ramp_down_trig(v)
#define audio_reg_hal_get_anc_cfg_anc0_ramp_down_trig()                     audio_reg_ll_get_anc_cfg_anc0_ramp_down_trig()

#define audio_reg_hal_set_anc_cfg_anc1_ramp_down_trig(v)                    audio_reg_ll_set_anc_cfg_anc1_ramp_down_trig(v)
#define audio_reg_hal_get_anc_cfg_anc1_ramp_down_trig()                     audio_reg_ll_get_anc_cfg_anc1_ramp_down_trig()

#define audio_reg_hal_set_anc_cfg_anc_ramp_cfg(v)                           audio_reg_ll_set_anc_cfg_anc_ramp_cfg(v)
#define audio_reg_hal_get_anc_cfg_anc_ramp_cfg()                            audio_reg_ll_get_anc_cfg_anc_ramp_cfg()

#define audio_reg_hal_set_anc_cfg_anc_cic_setp_3(v)                         audio_reg_ll_set_anc_cfg_anc_cic_setp_3(v)
#define audio_reg_hal_get_anc_cfg_anc_cic_setp_3()                          audio_reg_ll_get_anc_cfg_anc_cic_setp_3()

#define audio_reg_hal_set_anc_cfg_dac_cic_step_2(v)                         audio_reg_ll_set_anc_cfg_dac_cic_step_2(v)
#define audio_reg_hal_get_anc_cfg_dac_cic_step_2()                          audio_reg_ll_get_anc_cfg_dac_cic_step_2()

#define audio_reg_hal_set_anc_cfg_anc0_ramp_up_trig(v)                      audio_reg_ll_set_anc_cfg_anc0_ramp_up_trig(v)
#define audio_reg_hal_get_anc_cfg_anc0_ramp_up_trig()                       audio_reg_ll_get_anc_cfg_anc0_ramp_up_trig()

#define audio_reg_hal_set_anc_cfg_anc1_ramp_up_trig(v)                      audio_reg_ll_set_anc_cfg_anc1_ramp_up_trig(v)
#define audio_reg_hal_get_anc_cfg_anc1_ramp_up_trig()                       audio_reg_ll_get_anc_cfg_anc1_ramp_up_trig()

#define audio_reg_hal_set_anc_cfg_anc_en0(v)                                audio_reg_ll_set_anc_cfg_anc_en0(v)
#define audio_reg_hal_get_anc_cfg_anc_en0()                                 audio_reg_ll_get_anc_cfg_anc_en0()

#define audio_reg_hal_set_anc_cfg_anc_en1(v)                                audio_reg_ll_set_anc_cfg_anc_en1(v)
#define audio_reg_hal_get_anc_cfg_anc_en1()                                 audio_reg_ll_get_anc_cfg_anc_en1()

//reg dac_cfg:
#define audio_reg_hal_set_dac_cfg_value(v)                                  audio_reg_ll_set_dac_cfg_value(v)
#define audio_reg_hal_get_dac_cfg_value()                                   audio_reg_ll_get_dac_cfg_value()

#define audio_reg_hal_get_dac_cfg_reserved_0_0()                            audio_reg_ll_get_dac_cfg_reserved_0_0()

#define audio_reg_hal_set_dac_cfg_dac_enable_l(v)                           audio_reg_ll_set_dac_cfg_dac_enable_l(v)
#define audio_reg_hal_get_dac_cfg_dac_enable_l()                            audio_reg_ll_get_dac_cfg_dac_enable_l()

#define audio_reg_hal_set_dac_cfg_dac_enable_r(v)                           audio_reg_ll_set_dac_cfg_dac_enable_r(v)
#define audio_reg_hal_get_dac_cfg_dac_enable_r()                            audio_reg_ll_get_dac_cfg_dac_enable_r()

#define audio_reg_hal_set_dac_cfg_dac_iir_bps(v)                            audio_reg_ll_set_dac_cfg_dac_iir_bps(v)
#define audio_reg_hal_get_dac_cfg_dac_iir_bps()                             audio_reg_ll_get_dac_cfg_dac_iir_bps()

#define audio_reg_hal_set_dac_cfg_dac_lpf_bps1(v)                           audio_reg_ll_set_dac_cfg_dac_lpf_bps1(v)
#define audio_reg_hal_get_dac_cfg_dac_lpf_bps1()                            audio_reg_ll_get_dac_cfg_dac_lpf_bps1()

#define audio_reg_hal_set_dac_cfg_dac_lpf_bps2(v)                           audio_reg_ll_set_dac_cfg_dac_lpf_bps2(v)
#define audio_reg_hal_get_dac_cfg_dac_lpf_bps2()                            audio_reg_ll_get_dac_cfg_dac_lpf_bps2()

#define audio_reg_hal_set_dac_cfg_dac_lpf_bps3(v)                           audio_reg_ll_set_dac_cfg_dac_lpf_bps3(v)
#define audio_reg_hal_get_dac_cfg_dac_lpf_bps3()                            audio_reg_ll_get_dac_cfg_dac_lpf_bps3()

#define audio_reg_hal_set_dac_cfg_dac_tx_anc_d2(v)                          audio_reg_ll_set_dac_cfg_dac_tx_anc_d2(v)
#define audio_reg_hal_get_dac_cfg_dac_tx_anc_d2()                           audio_reg_ll_get_dac_cfg_dac_tx_anc_d2()

#define audio_reg_hal_set_dac_cfg_dac_16b_sel(v)                            audio_reg_ll_set_dac_cfg_dac_16b_sel(v)
#define audio_reg_hal_get_dac_cfg_dac_16b_sel()                             audio_reg_ll_get_dac_cfg_dac_16b_sel()

#define audio_reg_hal_set_dac_cfg_dac_spl_sel(v)                            audio_reg_ll_set_dac_cfg_dac_spl_sel(v)
#define audio_reg_hal_get_dac_cfg_dac_spl_sel()                             audio_reg_ll_get_dac_cfg_dac_spl_sel()

#define audio_reg_hal_set_dac_cfg_dac_hpf_bps(v)                            audio_reg_ll_set_dac_cfg_dac_hpf_bps(v)
#define audio_reg_hal_get_dac_cfg_dac_hpf_bps()                             audio_reg_ll_get_dac_cfg_dac_hpf_bps()

#define audio_reg_hal_set_dac_cfg_stereo_en(v)                              audio_reg_ll_set_dac_cfg_stereo_en(v)
#define audio_reg_hal_get_dac_cfg_stereo_en()                               audio_reg_ll_get_dac_cfg_stereo_en()

#define audio_reg_hal_set_dac_cfg_drc_bypass(v)                              audio_reg_ll_set_dac_cfg_drc_bypass(v)
#define audio_reg_hal_get_dac_cfg_drc_bypass()                               audio_reg_ll_get_dac_cfg_drc_bypass()

#define audio_reg_hal_set_dac_cfg_spk2mic_tst(v)                            audio_reg_ll_set_dac_cfg_spk2mic_tst(v)
#define audio_reg_hal_get_dac_cfg_spk2mic_tst()                             audio_reg_ll_get_dac_cfg_spk2mic_tst()

#define audio_reg_hal_set_dac_cfg_mono_sel(v)                               audio_reg_ll_set_dac_cfg_mono_sel(v)
#define audio_reg_hal_get_dac_cfg_mono_sel()                                audio_reg_ll_get_dac_cfg_mono_sel()

#define audio_reg_hal_set_dac_cfg_hint_spl_sel(v)                           audio_reg_ll_set_dac_cfg_hint_spl_sel(v)
#define audio_reg_hal_get_dac_cfg_hint_spl_sel()                            audio_reg_ll_get_dac_cfg_hint_spl_sel()

#define audio_reg_hal_set_dac_cfg_call_spl_sel(v)                           audio_reg_ll_set_dac_cfg_call_spl_sel(v)
#define audio_reg_hal_get_dac_cfg_call_spl_sel()                            audio_reg_ll_get_dac_cfg_call_spl_sel()

#define audio_reg_hal_set_dac_cfg_dith_en(v)                                audio_reg_ll_set_dac_cfg_dith_en(v)
#define audio_reg_hal_get_dac_cfg_dith_en()                                 audio_reg_ll_get_dac_cfg_dith_en()

#define audio_reg_hal_set_dac_cfg_clk_dac_inv(v)                            audio_reg_ll_set_dac_cfg_clk_dac_inv(v)
#define audio_reg_hal_get_dac_cfg_clk_dac_inv()                             audio_reg_ll_get_dac_cfg_clk_dac_inv()

//reg adc_cic_coef0:
#define audio_reg_hal_set_adc_cic_coef0_value(v)                            audio_reg_ll_set_adc_cic_coef0_value(v)
#define audio_reg_hal_get_adc_cic_coef0_value()                             audio_reg_ll_get_adc_cic_coef0_value()

#define audio_reg_hal_set_adc_cic_coef0_reg_cic_coef0(v)                    audio_reg_ll_set_adc_cic_coef0_reg_cic_coef0(v)
#define audio_reg_hal_get_adc_cic_coef0_reg_cic_coef0()                     audio_reg_ll_get_adc_cic_coef0_reg_cic_coef0()

//reg adc_cic_coef1:
#define audio_reg_hal_set_adc_cic_coef1_value(v)                            audio_reg_ll_set_adc_cic_coef1_value(v)
#define audio_reg_hal_get_adc_cic_coef1_value()                             audio_reg_ll_get_adc_cic_coef1_value()

#define audio_reg_hal_set_adc_cic_coef1_reg_cic_coef1(v)                    audio_reg_ll_set_adc_cic_coef1_reg_cic_coef1(v)
#define audio_reg_hal_get_adc_cic_coef1_reg_cic_coef1()                     audio_reg_ll_get_adc_cic_coef1_reg_cic_coef1()

//reg adc_cic_coef2:
#define audio_reg_hal_set_adc_cic_coef2_value(v)                            audio_reg_ll_set_adc_cic_coef2_value(v)
#define audio_reg_hal_get_adc_cic_coef2_value()                             audio_reg_ll_get_adc_cic_coef2_value()

#define audio_reg_hal_set_adc_cic_coef2_reg_cic_coef2(v)                    audio_reg_ll_set_adc_cic_coef2_reg_cic_coef2(v)
#define audio_reg_hal_get_adc_cic_coef2_reg_cic_coef2()                     audio_reg_ll_get_adc_cic_coef2_reg_cic_coef2()

//reg adc_cic_coef3:
#define audio_reg_hal_set_adc_cic_coef3_value(v)                            audio_reg_ll_set_adc_cic_coef3_value(v)
#define audio_reg_hal_get_adc_cic_coef3_value()                             audio_reg_ll_get_adc_cic_coef3_value()

#define audio_reg_hal_set_adc_cic_coef3_reg_cic_coef3(v)                    audio_reg_ll_set_adc_cic_coef3_reg_cic_coef3(v)
#define audio_reg_hal_get_adc_cic_coef3_reg_cic_coef3()                     audio_reg_ll_get_adc_cic_coef3_reg_cic_coef3()

//reg adc_cic_coef4:
#define audio_reg_hal_set_adc_cic_coef4_value(v)                            audio_reg_ll_set_adc_cic_coef4_value(v)
#define audio_reg_hal_get_adc_cic_coef4_value()                             audio_reg_ll_get_adc_cic_coef4_value()

#define audio_reg_hal_set_adc_cic_coef4_reg_cic_coef4(v)                    audio_reg_ll_set_adc_cic_coef4_reg_cic_coef4(v)
#define audio_reg_hal_get_adc_cic_coef4_reg_cic_coef4()                     audio_reg_ll_get_adc_cic_coef4_reg_cic_coef4()

//reg adc_cic_coef5:
#define audio_reg_hal_set_adc_cic_coef5_value(v)                            audio_reg_ll_set_adc_cic_coef5_value(v)
#define audio_reg_hal_get_adc_cic_coef5_value()                             audio_reg_ll_get_adc_cic_coef5_value()

#define audio_reg_hal_set_adc_cic_coef5_reg_cic_coef5(v)                    audio_reg_ll_set_adc_cic_coef5_reg_cic_coef5(v)
#define audio_reg_hal_get_adc_cic_coef5_reg_cic_coef5()                     audio_reg_ll_get_adc_cic_coef5_reg_cic_coef5()

//reg adc_cic_coef6:
#define audio_reg_hal_set_adc_cic_coef6_value(v)                            audio_reg_ll_set_adc_cic_coef6_value(v)
#define audio_reg_hal_get_adc_cic_coef6_value()                             audio_reg_ll_get_adc_cic_coef6_value()

#define audio_reg_hal_set_adc_cic_coef6_reg_cic_coef6(v)                    audio_reg_ll_set_adc_cic_coef6_reg_cic_coef6(v)
#define audio_reg_hal_get_adc_cic_coef6_reg_cic_coef6()                     audio_reg_ll_get_adc_cic_coef6_reg_cic_coef6()

//reg adc_cic_coef7:
#define audio_reg_hal_set_adc_cic_coef7_value(v)                            audio_reg_ll_set_adc_cic_coef7_value(v)
#define audio_reg_hal_get_adc_cic_coef7_value()                             audio_reg_ll_get_adc_cic_coef7_value()

#define audio_reg_hal_set_adc_cic_coef7_reg_cic_coef7(v)                    audio_reg_ll_set_adc_cic_coef7_reg_cic_coef7(v)
#define audio_reg_hal_get_adc_cic_coef7_reg_cic_coef7()                     audio_reg_ll_get_adc_cic_coef7_reg_cic_coef7()

//reg mic1_dbg_ctrl:
#define audio_reg_hal_set_mic1_dbg_ctrl_value(v)                            audio_reg_ll_set_mic1_dbg_ctrl_value(v)
#define audio_reg_hal_get_mic1_dbg_ctrl_value()                             audio_reg_ll_get_mic1_dbg_ctrl_value()

#define audio_reg_hal_set_mic1_dbg_ctrl_spk2mic_dbg_en(v)                   audio_reg_ll_set_mic1_dbg_ctrl_spk2mic_dbg_en(v)
#define audio_reg_hal_get_mic1_dbg_ctrl_spk2mic_dbg_en()                    audio_reg_ll_get_mic1_dbg_ctrl_spk2mic_dbg_en()

#define audio_reg_hal_set_mic1_dbg_ctrl_dac_cfg_anc_add(v)                  audio_reg_ll_set_mic1_dbg_ctrl_dac_cfg_anc_add(v)
#define audio_reg_hal_get_mic1_dbg_ctrl_dac_cfg_anc_add()                   audio_reg_ll_get_mic1_dbg_ctrl_dac_cfg_anc_add()

#define audio_reg_hal_set_mic1_dbg_ctrl_dac_cfg_dac_add(v)                  audio_reg_ll_set_mic1_dbg_ctrl_dac_cfg_dac_add(v)
#define audio_reg_hal_get_mic1_dbg_ctrl_dac_cfg_dac_add()                   audio_reg_ll_get_mic1_dbg_ctrl_dac_cfg_dac_add()

//reg buf_ctrl:
#define audio_reg_hal_set_buf_ctrl_value(v)                                 audio_reg_ll_set_buf_ctrl_value(v)
#define audio_reg_hal_get_buf_ctrl_value()                                  audio_reg_ll_get_buf_ctrl_value()

#define audio_reg_hal_set_buf_ctrl_en_spk0(v)                               audio_reg_ll_set_buf_ctrl_en_spk0(v)
#define audio_reg_hal_get_buf_ctrl_en_spk0()                                audio_reg_ll_get_buf_ctrl_en_spk0()

#define audio_reg_hal_set_buf_ctrl_en_spk1(v)                               audio_reg_ll_set_buf_ctrl_en_spk1(v)
#define audio_reg_hal_get_buf_ctrl_en_spk1()                                audio_reg_ll_get_buf_ctrl_en_spk1()

#define audio_reg_hal_set_buf_ctrl_en_mic(v)                                audio_reg_ll_set_buf_ctrl_en_mic(v)
#define audio_reg_hal_get_buf_ctrl_en_mic()                                 audio_reg_ll_get_buf_ctrl_en_mic()

#define audio_reg_hal_set_buf_ctrl_dma_mask_spk0(v)                         audio_reg_ll_set_buf_ctrl_dma_mask_spk0(v)
#define audio_reg_hal_get_buf_ctrl_dma_mask_spk0()                          audio_reg_ll_get_buf_ctrl_dma_mask_spk0()

#define audio_reg_hal_set_buf_ctrl_dma_mask_spk1(v)                         audio_reg_ll_set_buf_ctrl_dma_mask_spk1(v)
#define audio_reg_hal_get_buf_ctrl_dma_mask_spk1()                          audio_reg_ll_get_buf_ctrl_dma_mask_spk1()

#define audio_reg_hal_set_buf_ctrl_dma_mask_mic(v)                          audio_reg_ll_set_buf_ctrl_dma_mask_mic(v)
#define audio_reg_hal_get_buf_ctrl_dma_mask_mic()                           audio_reg_ll_get_buf_ctrl_dma_mask_mic()

//reg mic_fifo_cfg:
#define audio_reg_hal_set_mic_fifo_cfg_value(v)                             audio_reg_ll_set_mic_fifo_cfg_value(v)
#define audio_reg_hal_get_mic_fifo_cfg_value()                              audio_reg_ll_get_mic_fifo_cfg_value()

#define audio_reg_hal_set_mic_fifo_cfg_mic0_wr_thrd(v)                      audio_reg_ll_set_mic_fifo_cfg_mic0_wr_thrd(v)
#define audio_reg_hal_get_mic_fifo_cfg_mic0_wr_thrd()                       audio_reg_ll_get_mic_fifo_cfg_mic0_wr_thrd()

#define audio_reg_hal_set_mic_fifo_cfg_mic0_rd_thrd(v)                      audio_reg_ll_set_mic_fifo_cfg_mic0_rd_thrd(v)
#define audio_reg_hal_get_mic_fifo_cfg_mic0_rd_thrd()                       audio_reg_ll_get_mic_fifo_cfg_mic0_rd_thrd()

#define audio_reg_hal_set_mic_fifo_cfg_mic1_wr_thrd(v)                      audio_reg_ll_set_mic_fifo_cfg_mic1_wr_thrd(v)
#define audio_reg_hal_get_mic_fifo_cfg_mic1_wr_thrd()                       audio_reg_ll_get_mic_fifo_cfg_mic1_wr_thrd()

#define audio_reg_hal_set_mic_fifo_cfg_mic1_rd_thrd(v)                      audio_reg_ll_set_mic_fifo_cfg_mic1_rd_thrd(v)
#define audio_reg_hal_get_mic_fifo_cfg_mic1_rd_thrd()                       audio_reg_ll_get_mic_fifo_cfg_mic1_rd_thrd()

//reg spk0_fifo_cfg:
#define audio_reg_hal_set_spk0_fifo_cfg_value(v)                            audio_reg_ll_set_spk0_fifo_cfg_value(v)
#define audio_reg_hal_get_spk0_fifo_cfg_value()                             audio_reg_ll_get_spk0_fifo_cfg_value()

#define audio_reg_hal_set_spk0_fifo_cfg_spk0_hint_wr_thrd(v)                audio_reg_ll_set_spk0_fifo_cfg_spk0_hint_wr_thrd(v)
#define audio_reg_hal_get_spk0_fifo_cfg_spk0_hint_wr_thrd()                 audio_reg_ll_get_spk0_fifo_cfg_spk0_hint_wr_thrd()

#define audio_reg_hal_set_spk0_fifo_cfg_spk0_hint_rd_thrd(v)                audio_reg_ll_set_spk0_fifo_cfg_spk0_hint_rd_thrd(v)
#define audio_reg_hal_get_spk0_fifo_cfg_spk0_hint_rd_thrd()                 audio_reg_ll_get_spk0_fifo_cfg_spk0_hint_rd_thrd()

#define audio_reg_hal_set_spk0_fifo_cfg_spk0_call_wr_thrd(v)                audio_reg_ll_set_spk0_fifo_cfg_spk0_call_wr_thrd(v)
#define audio_reg_hal_get_spk0_fifo_cfg_spk0_call_wr_thrd()                 audio_reg_ll_get_spk0_fifo_cfg_spk0_call_wr_thrd()

#define audio_reg_hal_set_spk0_fifo_cfg_spk0_call_rd_thrd(v)                audio_reg_ll_set_spk0_fifo_cfg_spk0_call_rd_thrd(v)
#define audio_reg_hal_get_spk0_fifo_cfg_spk0_call_rd_thrd()                 audio_reg_ll_get_spk0_fifo_cfg_spk0_call_rd_thrd()

#define audio_reg_hal_set_spk0_fifo_cfg_spk0_a2dp_wr_thrd(v)                audio_reg_ll_set_spk0_fifo_cfg_spk0_a2dp_wr_thrd(v)
#define audio_reg_hal_get_spk0_fifo_cfg_spk0_a2dp_wr_thrd()                 audio_reg_ll_get_spk0_fifo_cfg_spk0_a2dp_wr_thrd()

#define audio_reg_hal_set_spk0_fifo_cfg_spk0_a2dp_rd_thrd(v)                audio_reg_ll_set_spk0_fifo_cfg_spk0_a2dp_rd_thrd(v)
#define audio_reg_hal_get_spk0_fifo_cfg_spk0_a2dp_rd_thrd()                 audio_reg_ll_get_spk0_fifo_cfg_spk0_a2dp_rd_thrd()

//reg spk1_fifo_cfg:
#define audio_reg_hal_set_spk1_fifo_cfg_value(v)                            audio_reg_ll_set_spk1_fifo_cfg_value(v)
#define audio_reg_hal_get_spk1_fifo_cfg_value()                             audio_reg_ll_get_spk1_fifo_cfg_value()

#define audio_reg_hal_set_spk1_fifo_cfg_spk1_hint_wr_thrd(v)                audio_reg_ll_set_spk1_fifo_cfg_spk1_hint_wr_thrd(v)
#define audio_reg_hal_get_spk1_fifo_cfg_spk1_hint_wr_thrd()                 audio_reg_ll_get_spk1_fifo_cfg_spk1_hint_wr_thrd()

#define audio_reg_hal_set_spk1_fifo_cfg_spk1_hint_rd_thrd(v)                audio_reg_ll_set_spk1_fifo_cfg_spk1_hint_rd_thrd(v)
#define audio_reg_hal_get_spk1_fifo_cfg_spk1_hint_rd_thrd()                 audio_reg_ll_get_spk1_fifo_cfg_spk1_hint_rd_thrd()

#define audio_reg_hal_set_spk1_fifo_cfg_spk1_call_wr_thrd(v)                audio_reg_ll_set_spk1_fifo_cfg_spk1_call_wr_thrd(v)
#define audio_reg_hal_get_spk1_fifo_cfg_spk1_call_wr_thrd()                 audio_reg_ll_get_spk1_fifo_cfg_spk1_call_wr_thrd()

#define audio_reg_hal_set_spk1_fifo_cfg_spk1_call_rd_thrd(v)                audio_reg_ll_set_spk1_fifo_cfg_spk1_call_rd_thrd(v)
#define audio_reg_hal_get_spk1_fifo_cfg_spk1_call_rd_thrd()                 audio_reg_ll_get_spk1_fifo_cfg_spk1_call_rd_thrd()

#define audio_reg_hal_set_spk1_fifo_cfg_spk1_a2dp_wr_thrd(v)                audio_reg_ll_set_spk1_fifo_cfg_spk1_a2dp_wr_thrd(v)
#define audio_reg_hal_get_spk1_fifo_cfg_spk1_a2dp_wr_thrd()                 audio_reg_ll_get_spk1_fifo_cfg_spk1_a2dp_wr_thrd()

#define audio_reg_hal_set_spk1_fifo_cfg_spk1_a2dp_rd_thrd(v)                audio_reg_ll_set_spk1_fifo_cfg_spk1_a2dp_rd_thrd(v)
#define audio_reg_hal_get_spk1_fifo_cfg_spk1_a2dp_rd_thrd()                 audio_reg_ll_get_spk1_fifo_cfg_spk1_a2dp_rd_thrd()

//reg adc_cut_cfg:
#define audio_reg_hal_set_adc_cut_cfg_value(v)                              audio_reg_ll_set_adc_cut_cfg_value(v)
#define audio_reg_hal_get_adc_cut_cfg_value()                               audio_reg_ll_get_adc_cut_cfg_value()

#define audio_reg_hal_set_adc_cut_cfg_adc_cut0(v)                           audio_reg_ll_set_adc_cut_cfg_adc_cut0(v)
#define audio_reg_hal_get_adc_cut_cfg_adc_cut0()                            audio_reg_ll_get_adc_cut_cfg_adc_cut0()

#define audio_reg_hal_set_adc_cut_cfg_adc_cut1(v)                           audio_reg_ll_set_adc_cut_cfg_adc_cut1(v)
#define audio_reg_hal_get_adc_cut_cfg_adc_cut1()                            audio_reg_ll_get_adc_cut_cfg_adc_cut1()

#define audio_reg_hal_set_adc_cut_cfg_adc_cut2(v)                           audio_reg_ll_set_adc_cut_cfg_adc_cut2(v)
#define audio_reg_hal_get_adc_cut_cfg_adc_cut2()                            audio_reg_ll_get_adc_cut_cfg_adc_cut2()

#define audio_reg_hal_set_adc_cut_cfg_adc_cut3(v)                           audio_reg_ll_set_adc_cut_cfg_adc_cut3(v)
#define audio_reg_hal_get_adc_cut_cfg_adc_cut3()                            audio_reg_ll_get_adc_cut_cfg_adc_cut3()

#define audio_reg_hal_set_adc_cut_cfg_adc_cut4(v)                           audio_reg_ll_set_adc_cut_cfg_adc_cut4(v)
#define audio_reg_hal_get_adc_cut_cfg_adc_cut4()                            audio_reg_ll_get_adc_cut_cfg_adc_cut4()

//reg iir_sft_cfg:
#define audio_reg_hal_set_iir_sft_cfg_value(v)                              audio_reg_ll_set_iir_sft_cfg_value(v)
#define audio_reg_hal_get_iir_sft_cfg_value()                               audio_reg_ll_get_iir_sft_cfg_value()

#define audio_reg_hal_set_iir_sft_cfg_dac_eq_sft_r_sel_0(v)                 audio_reg_ll_set_iir_sft_cfg_dac_eq_sft_r_sel_0(v)
#define audio_reg_hal_get_iir_sft_cfg_dac_eq_sft_r_sel_0()                  audio_reg_ll_get_iir_sft_cfg_dac_eq_sft_r_sel_0()

#define audio_reg_hal_set_iir_sft_cfg_dac_eq_sft_r_sel_1(v)                 audio_reg_ll_set_iir_sft_cfg_dac_eq_sft_r_sel_1(v)
#define audio_reg_hal_get_iir_sft_cfg_dac_eq_sft_r_sel_1()                  audio_reg_ll_get_iir_sft_cfg_dac_eq_sft_r_sel_1()

#define audio_reg_hal_set_iir_sft_cfg_dac_eq_sft_l_sel_0(v)                 audio_reg_ll_set_iir_sft_cfg_dac_eq_sft_l_sel_0(v)
#define audio_reg_hal_get_iir_sft_cfg_dac_eq_sft_l_sel_0()                  audio_reg_ll_get_iir_sft_cfg_dac_eq_sft_l_sel_0()

#define audio_reg_hal_set_iir_sft_cfg_dac_eq_sft_l_sel_1(v)                 audio_reg_ll_set_iir_sft_cfg_dac_eq_sft_l_sel_1(v)
#define audio_reg_hal_get_iir_sft_cfg_dac_eq_sft_l_sel_1()                  audio_reg_ll_get_iir_sft_cfg_dac_eq_sft_l_sel_1()

#define audio_reg_hal_set_iir_sft_cfg_comp_eq_sft_r_sel_0(v)                audio_reg_ll_set_iir_sft_cfg_comp_eq_sft_r_sel_0(v)
#define audio_reg_hal_get_iir_sft_cfg_comp_eq_sft_r_sel_0()                 audio_reg_ll_get_iir_sft_cfg_comp_eq_sft_r_sel_0()

#define audio_reg_hal_set_iir_sft_cfg_comp_eq_sft_r_sel_1(v)                audio_reg_ll_set_iir_sft_cfg_comp_eq_sft_r_sel_1(v)
#define audio_reg_hal_get_iir_sft_cfg_comp_eq_sft_r_sel_1()                 audio_reg_ll_get_iir_sft_cfg_comp_eq_sft_r_sel_1()

#define audio_reg_hal_set_iir_sft_cfg_comp_eq_sft_l_sel_0(v)                audio_reg_ll_set_iir_sft_cfg_comp_eq_sft_l_sel_0(v)
#define audio_reg_hal_get_iir_sft_cfg_comp_eq_sft_l_sel_0()                 audio_reg_ll_get_iir_sft_cfg_comp_eq_sft_l_sel_0()

#define audio_reg_hal_set_iir_sft_cfg_comp_eq_sft_l_sel_1(v)                audio_reg_ll_set_iir_sft_cfg_comp_eq_sft_l_sel_1(v)
#define audio_reg_hal_get_iir_sft_cfg_comp_eq_sft_l_sel_1()                 audio_reg_ll_get_iir_sft_cfg_comp_eq_sft_l_sel_1()

//reg dac_cfg1:
#define audio_reg_hal_set_dac_cfg1_value(v)                                 audio_reg_ll_set_dac_cfg1_value(v)
#define audio_reg_hal_get_dac_cfg1_value()                                  audio_reg_ll_get_dac_cfg1_value()

#define audio_reg_hal_set_dac_cfg1_dac_pn_conf(v)                           audio_reg_ll_set_dac_cfg1_dac_pn_conf(v)
#define audio_reg_hal_get_dac_cfg1_dac_pn_conf()                            audio_reg_ll_get_dac_cfg1_dac_pn_conf()

#define audio_reg_hal_set_dac_cfg1_notchen(v)                               audio_reg_ll_set_dac_cfg1_notchen(v)
#define audio_reg_hal_get_dac_cfg1_notchen()                                audio_reg_ll_get_dac_cfg1_notchen()

#define audio_reg_hal_set_dac_cfg1_sw_board(v)                              audio_reg_ll_set_dac_cfg1_sw_board(v)
#define audio_reg_hal_get_dac_cfg1_sw_board()                               audio_reg_ll_get_dac_cfg1_sw_board()

#define audio_reg_hal_set_dac_cfg1_cfg_dac_wait_cnt(v)                      audio_reg_ll_set_dac_cfg1_cfg_dac_wait_cnt(v)
#define audio_reg_hal_get_dac_cfg1_cfg_dac_wait_cnt()                       audio_reg_ll_get_dac_cfg1_cfg_dac_wait_cnt()

#define audio_reg_hal_set_dac_cfg1_dac_eq_bps(v)                            audio_reg_ll_set_dac_cfg1_dac_eq_bps(v)
#define audio_reg_hal_get_dac_cfg1_dac_eq_bps()                             audio_reg_ll_get_dac_cfg1_dac_eq_bps()

#define audio_reg_hal_set_dac_cfg1_rsp_bps(v)                               audio_reg_ll_set_dac_cfg1_rsp_bps(v)
#define audio_reg_hal_get_dac_cfg1_rsp_bps()                                audio_reg_ll_get_dac_cfg1_rsp_bps()

#define audio_reg_hal_set_dac_cfg1_dac_frc_o(v)                             audio_reg_ll_set_dac_cfg1_dac_frc_o(v)
#define audio_reg_hal_get_dac_cfg1_dac_frc_o()                              audio_reg_ll_get_dac_cfg1_dac_frc_o()

#define audio_reg_hal_set_dac_cfg1_dac_frc_hw(v)                            audio_reg_ll_set_dac_cfg1_dac_frc_hw(v)
#define audio_reg_hal_get_dac_cfg1_dac_frc_hw()                             audio_reg_ll_get_dac_cfg1_dac_frc_hw()

#define audio_reg_hal_set_dac_cfg1_dac_frc_hw_mask(v)                       audio_reg_ll_set_dac_cfg1_dac_frc_hw_mask(v)
#define audio_reg_hal_get_dac_cfg1_dac_frc_hw_mask()                        audio_reg_ll_get_dac_cfg1_dac_frc_hw_mask()

//reg anc_cfg2:
#define audio_reg_hal_set_anc_cfg2_value(v)                                 audio_reg_ll_set_anc_cfg2_value(v)
#define audio_reg_hal_get_anc_cfg2_value()                                  audio_reg_ll_get_anc_cfg2_value()

#define audio_reg_hal_set_anc_cfg2_anc1_sft_sel_1(v)                        audio_reg_ll_set_anc_cfg2_anc1_sft_sel_1(v)
#define audio_reg_hal_get_anc_cfg2_anc1_sft_sel_1()                         audio_reg_ll_get_anc_cfg2_anc1_sft_sel_1()

#define audio_reg_hal_set_anc_cfg2_anc1_sft_sel_2(v)                        audio_reg_ll_set_anc_cfg2_anc1_sft_sel_2(v)
#define audio_reg_hal_get_anc_cfg2_anc1_sft_sel_2()                         audio_reg_ll_get_anc_cfg2_anc1_sft_sel_2()

#define audio_reg_hal_set_anc_cfg2_anc1_rshift0(v)                          audio_reg_ll_set_anc_cfg2_anc1_rshift0(v)
#define audio_reg_hal_get_anc_cfg2_anc1_rshift0()                           audio_reg_ll_get_anc_cfg2_anc1_rshift0()

#define audio_reg_hal_set_anc_cfg2_anc1_rshift1(v)                          audio_reg_ll_set_anc_cfg2_anc1_rshift1(v)
#define audio_reg_hal_get_anc_cfg2_anc1_rshift1()                           audio_reg_ll_get_anc_cfg2_anc1_rshift1()

#define audio_reg_hal_set_anc_cfg2_up_spl_sel(v)                            audio_reg_ll_set_anc_cfg2_up_spl_sel(v)
#define audio_reg_hal_get_anc_cfg2_up_spl_sel()                             audio_reg_ll_get_anc_cfg2_up_spl_sel()

#define audio_reg_hal_set_anc_cfg2_anc2_sft_sel_1(v)                        audio_reg_ll_set_anc_cfg2_anc2_sft_sel_1(v)
#define audio_reg_hal_get_anc_cfg2_anc2_sft_sel_1()                         audio_reg_ll_get_anc_cfg2_anc2_sft_sel_1()

#define audio_reg_hal_set_anc_cfg2_anc2_sft_sel_2(v)                        audio_reg_ll_set_anc_cfg2_anc2_sft_sel_2(v)
#define audio_reg_hal_get_anc_cfg2_anc2_sft_sel_2()                         audio_reg_ll_get_anc_cfg2_anc2_sft_sel_2()

#define audio_reg_hal_set_anc_cfg2_anc2_rshift0(v)                          audio_reg_ll_set_anc_cfg2_anc2_rshift0(v)
#define audio_reg_hal_get_anc_cfg2_anc2_rshift0()                           audio_reg_ll_get_anc_cfg2_anc2_rshift0()

#define audio_reg_hal_set_anc_cfg2_anc2_rshift1(v)                          audio_reg_ll_set_anc_cfg2_anc2_rshift1(v)
#define audio_reg_hal_get_anc_cfg2_anc2_rshift1()                           audio_reg_ll_get_anc_cfg2_anc2_rshift1()

#define audio_reg_hal_set_anc_cfg2_anc_spl_sel(v)                           audio_reg_ll_set_anc_cfg2_anc_spl_sel(v)
#define audio_reg_hal_get_anc_cfg2_anc_spl_sel()                            audio_reg_ll_get_anc_cfg2_anc_spl_sel()

#define audio_reg_hal_set_anc_cfg2_dac_sdm_dis(v)                           audio_reg_ll_set_anc_cfg2_dac_sdm_dis(v)
#define audio_reg_hal_get_anc_cfg2_dac_sdm_dis()                            audio_reg_ll_get_anc_cfg2_dac_sdm_dis()

//reg ramp_up_cfg:
#define audio_reg_hal_set_ramp_up_cfg_value(v)                              audio_reg_ll_set_ramp_up_cfg_value(v)
#define audio_reg_hal_get_ramp_up_cfg_value()                               audio_reg_ll_get_ramp_up_cfg_value()

#define audio_reg_hal_set_ramp_up_cfg_anc_comp_en(v)                        audio_reg_ll_set_ramp_up_cfg_anc_comp_en(v)
#define audio_reg_hal_get_ramp_up_cfg_anc_comp_en()                         audio_reg_ll_get_ramp_up_cfg_anc_comp_en()

#define audio_reg_hal_set_ramp_up_cfg_comp_ramp_down_trig(v)                audio_reg_ll_set_ramp_up_cfg_comp_ramp_down_trig(v)
#define audio_reg_hal_get_ramp_up_cfg_comp_ramp_down_trig()                 audio_reg_ll_get_ramp_up_cfg_comp_ramp_down_trig()

#define audio_reg_hal_set_ramp_up_cfg_comp_ramp_cfg(v)                      audio_reg_ll_set_ramp_up_cfg_comp_ramp_cfg(v)
#define audio_reg_hal_get_ramp_up_cfg_comp_ramp_cfg()                       audio_reg_ll_get_ramp_up_cfg_comp_ramp_cfg()

#define audio_reg_hal_set_ramp_up_cfg_comp_ramp_bps(v)                      audio_reg_ll_set_ramp_up_cfg_comp_ramp_bps(v)
#define audio_reg_hal_get_ramp_up_cfg_comp_ramp_bps()                       audio_reg_ll_get_ramp_up_cfg_comp_ramp_bps()

#define audio_reg_hal_set_ramp_up_cfg_eq_ramp_down_trig(v)                  audio_reg_ll_set_ramp_up_cfg_eq_ramp_down_trig(v)
#define audio_reg_hal_get_ramp_up_cfg_eq_ramp_down_trig()                   audio_reg_ll_get_ramp_up_cfg_eq_ramp_down_trig()

#define audio_reg_hal_set_ramp_up_cfg_eq_ramp_cfg(v)                        audio_reg_ll_set_ramp_up_cfg_eq_ramp_cfg(v)
#define audio_reg_hal_get_ramp_up_cfg_eq_ramp_cfg()                         audio_reg_ll_get_ramp_up_cfg_eq_ramp_cfg()

#define audio_reg_hal_set_ramp_up_cfg_eq_ramp_bps(v)                        audio_reg_ll_set_ramp_up_cfg_eq_ramp_bps(v)
#define audio_reg_hal_get_ramp_up_cfg_eq_ramp_bps()                         audio_reg_ll_get_ramp_up_cfg_eq_ramp_bps()

#define audio_reg_hal_set_ramp_up_cfg_eq_ramp_up_trig(v)                    audio_reg_ll_set_ramp_up_cfg_eq_ramp_up_trig(v)
#define audio_reg_hal_get_ramp_up_cfg_eq_ramp_up_trig()                     audio_reg_ll_get_ramp_up_cfg_eq_ramp_up_trig()

#define audio_reg_hal_set_ramp_up_cfg_comp_ramp_up_trig(v)                  audio_reg_ll_set_ramp_up_cfg_comp_ramp_up_trig(v)
#define audio_reg_hal_get_ramp_up_cfg_comp_ramp_up_trig()                   audio_reg_ll_get_ramp_up_cfg_comp_ramp_up_trig()

//reg anc1_gain_cfg1:
#define audio_reg_hal_set_anc1_gain_cfg1_value(v)                           audio_reg_ll_set_anc1_gain_cfg1_value(v)
#define audio_reg_hal_get_anc1_gain_cfg1_value()                            audio_reg_ll_get_anc1_gain_cfg1_value()

#define audio_reg_hal_set_anc1_gain_cfg1_anc1_gain1_1(v)                    audio_reg_ll_set_anc1_gain_cfg1_anc1_gain1_1(v)
#define audio_reg_hal_get_anc1_gain_cfg1_anc1_gain1_1()                     audio_reg_ll_get_anc1_gain_cfg1_anc1_gain1_1()

//reg anc1_gain_cfg2:
#define audio_reg_hal_set_anc1_gain_cfg2_value(v)                           audio_reg_ll_set_anc1_gain_cfg2_value(v)
#define audio_reg_hal_get_anc1_gain_cfg2_value()                            audio_reg_ll_get_anc1_gain_cfg2_value()

#define audio_reg_hal_set_anc1_gain_cfg2_anc1_gain_comp(v)                  audio_reg_ll_set_anc1_gain_cfg2_anc1_gain_comp(v)
#define audio_reg_hal_get_anc1_gain_cfg2_anc1_gain_comp()                   audio_reg_ll_get_anc1_gain_cfg2_anc1_gain_comp()

//reg anc1_gain_cfg3:
#define audio_reg_hal_set_anc1_gain_cfg3_value(v)                           audio_reg_ll_set_anc1_gain_cfg3_value(v)
#define audio_reg_hal_get_anc1_gain_cfg3_value()                            audio_reg_ll_get_anc1_gain_cfg3_value()

#define audio_reg_hal_set_anc1_gain_cfg3_anc1_gain0_1(v)                    audio_reg_ll_set_anc1_gain_cfg3_anc1_gain0_1(v)
#define audio_reg_hal_get_anc1_gain_cfg3_anc1_gain0_1()                     audio_reg_ll_get_anc1_gain_cfg3_anc1_gain0_1()

//reg anc1_gain_cfg4:
#define audio_reg_hal_set_anc1_gain_cfg4_value(v)                           audio_reg_ll_set_anc1_gain_cfg4_value(v)
#define audio_reg_hal_get_anc1_gain_cfg4_value()                            audio_reg_ll_get_anc1_gain_cfg4_value()

#define audio_reg_hal_set_anc1_gain_cfg4_anc1_gain0_0(v)                    audio_reg_ll_set_anc1_gain_cfg4_anc1_gain0_0(v)
#define audio_reg_hal_get_anc1_gain_cfg4_anc1_gain0_0()                     audio_reg_ll_get_anc1_gain_cfg4_anc1_gain0_0()

//reg sync_ctrl1:
#define audio_reg_hal_set_sync_ctrl1_value(v)                               audio_reg_ll_set_sync_ctrl1_value(v)
#define audio_reg_hal_get_sync_ctrl1_value()                                audio_reg_ll_get_sync_ctrl1_value()

#define audio_reg_hal_set_sync_ctrl1_sync_a2dp_cnt(v)                       audio_reg_ll_set_sync_ctrl1_sync_a2dp_cnt(v)
#define audio_reg_hal_get_sync_ctrl1_sync_a2dp_cnt()                        audio_reg_ll_get_sync_ctrl1_sync_a2dp_cnt()

#define audio_reg_hal_set_sync_ctrl1_start_a2dp_cnt(v)                      audio_reg_ll_set_sync_ctrl1_start_a2dp_cnt(v)
#define audio_reg_hal_get_sync_ctrl1_start_a2dp_cnt()                       audio_reg_ll_get_sync_ctrl1_start_a2dp_cnt()

#define audio_reg_hal_set_sync_ctrl1_sync_call_cnt(v)                       audio_reg_ll_set_sync_ctrl1_sync_call_cnt(v)
#define audio_reg_hal_get_sync_ctrl1_sync_call_cnt()                        audio_reg_ll_get_sync_ctrl1_sync_call_cnt()

#define audio_reg_hal_set_sync_ctrl1_start_call_cnt(v)                      audio_reg_ll_set_sync_ctrl1_start_call_cnt(v)
#define audio_reg_hal_get_sync_ctrl1_start_call_cnt()                       audio_reg_ll_get_sync_ctrl1_start_call_cnt()

//reg sync_ctrl2:
#define audio_reg_hal_set_sync_ctrl2_value(v)                               audio_reg_ll_set_sync_ctrl2_value(v)
#define audio_reg_hal_get_sync_ctrl2_value()                                audio_reg_ll_get_sync_ctrl2_value()

#define audio_reg_hal_set_sync_ctrl2_sync_hint_cnt(v)                       audio_reg_ll_set_sync_ctrl2_sync_hint_cnt(v)
#define audio_reg_hal_get_sync_ctrl2_sync_hint_cnt()                        audio_reg_ll_get_sync_ctrl2_sync_hint_cnt()

#define audio_reg_hal_set_sync_ctrl2_start_hint_cnt(v)                      audio_reg_ll_set_sync_ctrl2_start_hint_cnt(v)
#define audio_reg_hal_get_sync_ctrl2_start_hint_cnt()                       audio_reg_ll_get_sync_ctrl2_start_hint_cnt()

//reg dac_ro_sts:
#define audio_reg_hal_set_dac_ro_sts_value(v)                               audio_reg_ll_set_dac_ro_sts_value(v)
#define audio_reg_hal_get_dac_ro_sts_value()                                audio_reg_ll_get_dac_ro_sts_value()

#define audio_reg_hal_get_dac_ro_sts_dac_fifo_status()                      audio_reg_ll_get_dac_ro_sts_dac_fifo_status()

#define audio_reg_hal_get_dac_ro_sts_mem_auto_init_done()                   audio_reg_ll_get_dac_ro_sts_mem_auto_init_done()

//reg adc_ro_sts:
#define audio_reg_hal_set_adc_ro_sts_value(v)                               audio_reg_ll_set_adc_ro_sts_value(v)
#define audio_reg_hal_get_adc_ro_sts_value()                                audio_reg_ll_get_adc_ro_sts_value()

#define audio_reg_hal_get_adc_ro_sts_adc_fifo_status()                      audio_reg_ll_get_adc_ro_sts_adc_fifo_status()

//reg anc_sts:
#define audio_reg_hal_set_anc_sts_value(v)                                  audio_reg_ll_set_anc_sts_value(v)
#define audio_reg_hal_get_anc_sts_value()                                   audio_reg_ll_get_anc_sts_value()

#define audio_reg_hal_get_anc_sts_anc_status()                              audio_reg_ll_get_anc_sts_anc_status()

//reg ramp_intr_ctrl:
#define audio_reg_hal_set_ramp_intr_ctrl_value(v)                           audio_reg_ll_set_ramp_intr_ctrl_value(v)
#define audio_reg_hal_get_ramp_intr_ctrl_value()                            audio_reg_ll_get_ramp_intr_ctrl_value()

#define audio_reg_hal_set_ramp_intr_ctrl_ramp_interrupt_mask(v)             audio_reg_ll_set_ramp_intr_ctrl_ramp_interrupt_mask(v)
#define audio_reg_hal_get_ramp_intr_ctrl_ramp_interrupt_mask()              audio_reg_ll_get_ramp_intr_ctrl_ramp_interrupt_mask()

#define audio_reg_hal_set_ramp_intr_ctrl_iir_ovl_int_mask(v)                audio_reg_ll_set_ramp_intr_ctrl_iir_ovl_int_mask(v)
#define audio_reg_hal_get_ramp_intr_ctrl_iir_ovl_int_mask()                 audio_reg_ll_get_ramp_intr_ctrl_iir_ovl_int_mask()

#define audio_reg_hal_set_ramp_intr_ctrl_ramp_interrupt_clr(v)              audio_reg_ll_set_ramp_intr_ctrl_ramp_interrupt_clr(v)
#define audio_reg_hal_get_ramp_intr_ctrl_ramp_interrupt_clr()               audio_reg_ll_get_ramp_intr_ctrl_ramp_interrupt_clr()

#define audio_reg_hal_set_ramp_intr_ctrl_iir_ovl_int_clr(v)                 audio_reg_ll_set_ramp_intr_ctrl_iir_ovl_int_clr(v)
#define audio_reg_hal_get_ramp_intr_ctrl_iir_ovl_int_clr()                  audio_reg_ll_get_ramp_intr_ctrl_iir_ovl_int_clr()

//reg aud_int_ctrl:
#define audio_reg_hal_set_aud_int_ctrl_value(v)                             audio_reg_ll_set_aud_int_ctrl_value(v)
#define audio_reg_hal_get_aud_int_ctrl_value()                              audio_reg_ll_get_aud_int_ctrl_value()

#define audio_reg_hal_set_aud_int_ctrl_aud_interrupt_mask(v)                audio_reg_ll_set_aud_int_ctrl_aud_interrupt_mask(v)
#define audio_reg_hal_get_aud_int_ctrl_aud_interrupt_mask()                 audio_reg_ll_get_aud_int_ctrl_aud_interrupt_mask()

#define audio_reg_hal_set_aud_int_ctrl_aud_interrupt_clr(v)                 audio_reg_ll_set_aud_int_ctrl_aud_interrupt_clr(v)
#define audio_reg_hal_get_aud_int_ctrl_aud_interrupt_clr()                  audio_reg_ll_get_aud_int_ctrl_aud_interrupt_clr()

//reg aud_int_sts:
#define audio_reg_hal_set_aud_int_sts_value(v)                              audio_reg_ll_set_aud_int_sts_value(v)
#define audio_reg_hal_get_aud_int_sts_value()                               audio_reg_ll_get_aud_int_sts_value()

#define audio_reg_hal_get_aud_int_sts_aud_interrupt_status()                audio_reg_ll_get_aud_int_sts_aud_interrupt_status()

//reg anc2_limit_cfg1:
#define audio_reg_hal_set_anc2_limit_cfg1_value(v)                          audio_reg_ll_set_anc2_limit_cfg1_value(v)
#define audio_reg_hal_get_anc2_limit_cfg1_value()                           audio_reg_ll_get_anc2_limit_cfg1_value()

#define audio_reg_hal_set_anc2_limit_cfg1_anc2_limit_val1(v)                audio_reg_ll_set_anc2_limit_cfg1_anc2_limit_val1(v)
#define audio_reg_hal_get_anc2_limit_cfg1_anc2_limit_val1()                 audio_reg_ll_get_anc2_limit_cfg1_anc2_limit_val1()

#define audio_reg_hal_set_anc2_limit_cfg1_anc2_limit_bps1(v)                audio_reg_ll_set_anc2_limit_cfg1_anc2_limit_bps1(v)
#define audio_reg_hal_get_anc2_limit_cfg1_anc2_limit_bps1()                 audio_reg_ll_get_anc2_limit_cfg1_anc2_limit_bps1()

//reg anc2_limit_cfg2:
#define audio_reg_hal_set_anc2_limit_cfg2_value(v)                          audio_reg_ll_set_anc2_limit_cfg2_value(v)
#define audio_reg_hal_get_anc2_limit_cfg2_value()                           audio_reg_ll_get_anc2_limit_cfg2_value()

#define audio_reg_hal_set_anc2_limit_cfg2_anc2_limit_val2(v)                audio_reg_ll_set_anc2_limit_cfg2_anc2_limit_val2(v)
#define audio_reg_hal_get_anc2_limit_cfg2_anc2_limit_val2()                 audio_reg_ll_get_anc2_limit_cfg2_anc2_limit_val2()

#define audio_reg_hal_set_anc2_limit_cfg2_anc2_limit_bps2(v)                audio_reg_ll_set_anc2_limit_cfg2_anc2_limit_bps2(v)
#define audio_reg_hal_get_anc2_limit_cfg2_anc2_limit_bps2()                 audio_reg_ll_get_anc2_limit_cfg2_anc2_limit_bps2()

//reg anc1_limit_cfg1:
#define audio_reg_hal_set_anc1_limit_cfg1_value(v)                          audio_reg_ll_set_anc1_limit_cfg1_value(v)
#define audio_reg_hal_get_anc1_limit_cfg1_value()                           audio_reg_ll_get_anc1_limit_cfg1_value()

#define audio_reg_hal_set_anc1_limit_cfg1_anc1_limit_val1(v)                audio_reg_ll_set_anc1_limit_cfg1_anc1_limit_val1(v)
#define audio_reg_hal_get_anc1_limit_cfg1_anc1_limit_val1()                 audio_reg_ll_get_anc1_limit_cfg1_anc1_limit_val1()

#define audio_reg_hal_set_anc1_limit_cfg1_anc1_limit_bps1(v)                audio_reg_ll_set_anc1_limit_cfg1_anc1_limit_bps1(v)
#define audio_reg_hal_get_anc1_limit_cfg1_anc1_limit_bps1()                 audio_reg_ll_get_anc1_limit_cfg1_anc1_limit_bps1()

//reg anc2_limit_cfg2:
#define audio_reg_hal_set_anc2_limit_cfg2_value(v)                          audio_reg_ll_set_anc2_limit_cfg2_value(v)
#define audio_reg_hal_get_anc2_limit_cfg2_value()                           audio_reg_ll_get_anc2_limit_cfg2_value()

#define audio_reg_hal_set_anc2_limit_cfg2_anc1_limit_val2(v)                audio_reg_ll_set_anc2_limit_cfg2_anc1_limit_val2(v)
#define audio_reg_hal_get_anc2_limit_cfg2_anc1_limit_val2()                 audio_reg_ll_get_anc2_limit_cfg2_anc1_limit_val2()

#define audio_reg_hal_set_anc2_limit_cfg2_anc1_limit_bps2(v)                audio_reg_ll_set_anc2_limit_cfg2_anc1_limit_bps2(v)
#define audio_reg_hal_get_anc2_limit_cfg2_anc1_limit_bps2()                 audio_reg_ll_get_anc2_limit_cfg2_anc1_limit_bps2()

//reg anc2_gain_cfg1:
#define audio_reg_hal_set_anc2_gain_cfg1_value(v)                           audio_reg_ll_set_anc2_gain_cfg1_value(v)
#define audio_reg_hal_get_anc2_gain_cfg1_value()                            audio_reg_ll_get_anc2_gain_cfg1_value()

#define audio_reg_hal_set_anc2_gain_cfg1_anc2_gain1_1(v)                    audio_reg_ll_set_anc2_gain_cfg1_anc2_gain1_1(v)
#define audio_reg_hal_get_anc2_gain_cfg1_anc2_gain1_1()                     audio_reg_ll_get_anc2_gain_cfg1_anc2_gain1_1()

//reg anc2_gain_cfg2:
#define audio_reg_hal_set_anc2_gain_cfg2_value(v)                           audio_reg_ll_set_anc2_gain_cfg2_value(v)
#define audio_reg_hal_get_anc2_gain_cfg2_value()                            audio_reg_ll_get_anc2_gain_cfg2_value()

#define audio_reg_hal_set_anc2_gain_cfg2_anc2_gain_comp(v)                  audio_reg_ll_set_anc2_gain_cfg2_anc2_gain_comp(v)
#define audio_reg_hal_get_anc2_gain_cfg2_anc2_gain_comp()                   audio_reg_ll_get_anc2_gain_cfg2_anc2_gain_comp()

//reg anc2_gain_cfg3:
#define audio_reg_hal_set_anc2_gain_cfg3_value(v)                           audio_reg_ll_set_anc2_gain_cfg3_value(v)
#define audio_reg_hal_get_anc2_gain_cfg3_value()                            audio_reg_ll_get_anc2_gain_cfg3_value()

#define audio_reg_hal_set_anc2_gain_cfg3_anc2_gain0_1(v)                    audio_reg_ll_set_anc2_gain_cfg3_anc2_gain0_1(v)
#define audio_reg_hal_get_anc2_gain_cfg3_anc2_gain0_1()                     audio_reg_ll_get_anc2_gain_cfg3_anc2_gain0_1()

//reg anc2_gain_cfg4:
#define audio_reg_hal_set_anc2_gain_cfg4_value(v)                           audio_reg_ll_set_anc2_gain_cfg4_value(v)
#define audio_reg_hal_get_anc2_gain_cfg4_value()                            audio_reg_ll_get_anc2_gain_cfg4_value()

#define audio_reg_hal_set_anc2_gain_cfg4_anc2_gain0_0(v)                    audio_reg_ll_set_anc2_gain_cfg4_anc2_gain0_0(v)
#define audio_reg_hal_get_anc2_gain_cfg4_anc2_gain0_0()                     audio_reg_ll_get_anc2_gain_cfg4_anc2_gain0_0()

//reg anc_comp_cfg:
#define audio_reg_hal_set_anc_comp_cfg_value(v)                             audio_reg_ll_set_anc_comp_cfg_value(v)
#define audio_reg_hal_get_anc_comp_cfg_value()                              audio_reg_ll_get_anc_comp_cfg_value()

#define audio_reg_hal_set_anc_comp_cfg_comp_iir_bps(v)                      audio_reg_ll_set_anc_comp_cfg_comp_iir_bps(v)
#define audio_reg_hal_get_anc_comp_cfg_comp_iir_bps()                       audio_reg_ll_get_anc_comp_cfg_comp_iir_bps()

#define audio_reg_hal_set_anc_comp_cfg_dac_comp_spl(v)                      audio_reg_ll_set_anc_comp_cfg_dac_comp_spl(v)
#define audio_reg_hal_get_anc_comp_cfg_dac_comp_spl()                       audio_reg_ll_get_anc_comp_cfg_dac_comp_spl()

#define audio_reg_hal_set_anc_comp_cfg_anc_cfg_start_val(v)                 audio_reg_ll_set_anc_comp_cfg_anc_cfg_start_val(v)
#define audio_reg_hal_get_anc_comp_cfg_anc_cfg_start_val()                  audio_reg_ll_get_anc_comp_cfg_anc_cfg_start_val()

//reg dac_gain_cfg0:
#define audio_reg_hal_set_dac_gain_cfg0_value(v)                            audio_reg_ll_set_dac_gain_cfg0_value(v)
#define audio_reg_hal_get_dac_gain_cfg0_value()                             audio_reg_ll_get_dac_gain_cfg0_value()

#define audio_reg_hal_set_dac_gain_cfg0_spk0_a2dp_gain(v)                   audio_reg_ll_set_dac_gain_cfg0_spk0_a2dp_gain(v)
#define audio_reg_hal_get_dac_gain_cfg0_spk0_a2dp_gain()                    audio_reg_ll_get_dac_gain_cfg0_spk0_a2dp_gain()

//reg dac_gain_cfg1:
#define audio_reg_hal_set_dac_gain_cfg1_value(v)                            audio_reg_ll_set_dac_gain_cfg1_value(v)
#define audio_reg_hal_get_dac_gain_cfg1_value()                             audio_reg_ll_get_dac_gain_cfg1_value()

#define audio_reg_hal_set_dac_gain_cfg1_spk0_call_gain(v)                   audio_reg_ll_set_dac_gain_cfg1_spk0_call_gain(v)
#define audio_reg_hal_get_dac_gain_cfg1_spk0_call_gain()                    audio_reg_ll_get_dac_gain_cfg1_spk0_call_gain()

//reg dac_gain_cfg2:
#define audio_reg_hal_set_dac_gain_cfg2_value(v)                            audio_reg_ll_set_dac_gain_cfg2_value(v)
#define audio_reg_hal_get_dac_gain_cfg2_value()                             audio_reg_ll_get_dac_gain_cfg2_value()

#define audio_reg_hal_set_dac_gain_cfg2_spk0_hint_gain(v)                   audio_reg_ll_set_dac_gain_cfg2_spk0_hint_gain(v)
#define audio_reg_hal_get_dac_gain_cfg2_spk0_hint_gain()                    audio_reg_ll_get_dac_gain_cfg2_spk0_hint_gain()

//reg dac_gain_cfg3:
#define audio_reg_hal_set_dac_gain_cfg3_value(v)                            audio_reg_ll_set_dac_gain_cfg3_value(v)
#define audio_reg_hal_get_dac_gain_cfg3_value()                             audio_reg_ll_get_dac_gain_cfg3_value()

#define audio_reg_hal_set_dac_gain_cfg3_spk1_a2dp_gain(v)                   audio_reg_ll_set_dac_gain_cfg3_spk1_a2dp_gain(v)
#define audio_reg_hal_get_dac_gain_cfg3_spk1_a2dp_gain()                    audio_reg_ll_get_dac_gain_cfg3_spk1_a2dp_gain()

//reg dac_gain_cfg4:
#define audio_reg_hal_set_dac_gain_cfg4_value(v)                            audio_reg_ll_set_dac_gain_cfg4_value(v)
#define audio_reg_hal_get_dac_gain_cfg4_value()                             audio_reg_ll_get_dac_gain_cfg4_value()

#define audio_reg_hal_set_dac_gain_cfg4_spk1_call_gain(v)                   audio_reg_ll_set_dac_gain_cfg4_spk1_call_gain(v)
#define audio_reg_hal_get_dac_gain_cfg4_spk1_call_gain()                    audio_reg_ll_get_dac_gain_cfg4_spk1_call_gain()

//reg dac_gain_cfg5:
#define audio_reg_hal_set_dac_gain_cfg5_value(v)                            audio_reg_ll_set_dac_gain_cfg5_value(v)
#define audio_reg_hal_get_dac_gain_cfg5_value()                             audio_reg_ll_get_dac_gain_cfg5_value()

#define audio_reg_hal_set_dac_gain_cfg5_spk1_hint_gain(v)                   audio_reg_ll_set_dac_gain_cfg5_spk1_hint_gain(v)
#define audio_reg_hal_get_dac_gain_cfg5_spk1_hint_gain()                    audio_reg_ll_get_dac_gain_cfg5_spk1_hint_gain()

//reg adc_gain_cfg1:
#define audio_reg_hal_set_adc_gain_cfg1_value(v)                            audio_reg_ll_set_adc_gain_cfg1_value(v)
#define audio_reg_hal_get_adc_gain_cfg1_value()                             audio_reg_ll_get_adc_gain_cfg1_value()

#define audio_reg_hal_set_adc_gain_cfg1_adc_chn1_gain(v)                    audio_reg_ll_set_adc_gain_cfg1_adc_chn1_gain(v)
#define audio_reg_hal_get_adc_gain_cfg1_adc_chn1_gain()                     audio_reg_ll_get_adc_gain_cfg1_adc_chn1_gain()

//reg dac_l_gain_mix:
#define audio_reg_hal_set_dac_l_gain_mix_value(v)                           audio_reg_ll_set_dac_l_gain_mix_value(v)
#define audio_reg_hal_get_dac_l_gain_mix_value()                            audio_reg_ll_get_dac_l_gain_mix_value()

#define audio_reg_hal_set_dac_l_gain_mix_dac_l_gain(v)                      audio_reg_ll_set_dac_l_gain_mix_dac_l_gain(v)
#define audio_reg_hal_get_dac_l_gain_mix_dac_l_gain()                       audio_reg_ll_get_dac_l_gain_mix_dac_l_gain()

//reg dac_r_gain_mix:
#define audio_reg_hal_set_dac_r_gain_mix_value(v)                           audio_reg_ll_set_dac_r_gain_mix_value(v)
#define audio_reg_hal_get_dac_r_gain_mix_value()                            audio_reg_ll_get_dac_r_gain_mix_value()

#define audio_reg_hal_set_dac_r_gain_mix_dac_r_gain(v)                      audio_reg_ll_set_dac_r_gain_mix_dac_r_gain(v)
#define audio_reg_hal_get_dac_r_gain_mix_dac_r_gain()                       audio_reg_ll_get_dac_r_gain_mix_dac_r_gain()

//reg adc_gain_cfg2:
#define audio_reg_hal_set_adc_gain_cfg2_value(v)                            audio_reg_ll_set_adc_gain_cfg2_value(v)
#define audio_reg_hal_get_adc_gain_cfg2_value()                             audio_reg_ll_get_adc_gain_cfg2_value()

#define audio_reg_hal_set_adc_gain_cfg2_adc_chn0_gain(v)                    audio_reg_ll_set_adc_gain_cfg2_adc_chn0_gain(v)
#define audio_reg_hal_get_adc_gain_cfg2_adc_chn0_gain()                     audio_reg_ll_get_adc_gain_cfg2_adc_chn0_gain()

//reg adc_gain_cfg3:
#define audio_reg_hal_set_adc_gain_cfg3_value(v)                            audio_reg_ll_set_adc_gain_cfg3_value(v)
#define audio_reg_hal_get_adc_gain_cfg3_value()                             audio_reg_ll_get_adc_gain_cfg3_value()

#define audio_reg_hal_set_adc_gain_cfg3_adc_chn4_gain(v)                    audio_reg_ll_set_adc_gain_cfg3_adc_chn4_gain(v)
#define audio_reg_hal_get_adc_gain_cfg3_adc_chn4_gain()                     audio_reg_ll_get_adc_gain_cfg3_adc_chn4_gain()

//reg adc_gain_cfg4:
#define audio_reg_hal_set_adc_gain_cfg4_value(v)                            audio_reg_ll_set_adc_gain_cfg4_value(v)
#define audio_reg_hal_get_adc_gain_cfg4_value()                             audio_reg_ll_get_adc_gain_cfg4_value()

#define audio_reg_hal_set_adc_gain_cfg4_adc_chn3_gain(v)                    audio_reg_ll_set_adc_gain_cfg4_adc_chn3_gain(v)
#define audio_reg_hal_get_adc_gain_cfg4_adc_chn3_gain()                     audio_reg_ll_get_adc_gain_cfg4_adc_chn3_gain()

//reg adc_gain_cfg5:
#define audio_reg_hal_set_adc_gain_cfg5_value(v)                            audio_reg_ll_set_adc_gain_cfg5_value(v)
#define audio_reg_hal_get_adc_gain_cfg5_value()                             audio_reg_ll_get_adc_gain_cfg5_value()

#define audio_reg_hal_set_adc_gain_cfg5_adc_chn2_gain(v)                    audio_reg_ll_set_adc_gain_cfg5_adc_chn2_gain(v)
#define audio_reg_hal_get_adc_gain_cfg5_adc_chn2_gain()                     audio_reg_ll_get_adc_gain_cfg5_adc_chn2_gain()

//reg anc_sft_l_para:
#define audio_reg_hal_set_anc_sft_l_para_value(v)                           audio_reg_ll_set_anc_sft_l_para_value(v)
#define audio_reg_hal_get_anc_sft_l_para_value()                            audio_reg_ll_get_anc_sft_l_para_value()

#define audio_reg_hal_set_anc_sft_l_para_anc_if_sft_l_0(v)                  audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_0(v)
#define audio_reg_hal_get_anc_sft_l_para_anc_if_sft_l_0()                   audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_0()

#define audio_reg_hal_set_anc_sft_l_para_anc_if_sft_l_1(v)                  audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_1(v)
#define audio_reg_hal_get_anc_sft_l_para_anc_if_sft_l_1()                   audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_1()

#define audio_reg_hal_set_anc_sft_l_para_anc_if_sft_l_2(v)                  audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_2(v)
#define audio_reg_hal_get_anc_sft_l_para_anc_if_sft_l_2()                   audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_2()

#define audio_reg_hal_set_anc_sft_l_para_anc_if_sft_l_3(v)                  audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_3(v)
#define audio_reg_hal_get_anc_sft_l_para_anc_if_sft_l_3()                   audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_3()

#define audio_reg_hal_set_anc_sft_l_para_anc_if_sft_l_4(v)                  audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_4(v)
#define audio_reg_hal_get_anc_sft_l_para_anc_if_sft_l_4()                   audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_4()

#define audio_reg_hal_set_anc_sft_l_para_anc_if_sft_l_5(v)                  audio_reg_ll_set_anc_sft_l_para_anc_if_sft_l_5(v)
#define audio_reg_hal_get_anc_sft_l_para_anc_if_sft_l_5()                   audio_reg_ll_get_anc_sft_l_para_anc_if_sft_l_5()

//reg anc_sft_r_para:
#define audio_reg_hal_set_anc_sft_r_para_value(v)                           audio_reg_ll_set_anc_sft_r_para_value(v)
#define audio_reg_hal_get_anc_sft_r_para_value()                            audio_reg_ll_get_anc_sft_r_para_value()

#define audio_reg_hal_set_anc_sft_r_para_anc_if_sft_r_0(v)                  audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_0(v)
#define audio_reg_hal_get_anc_sft_r_para_anc_if_sft_r_0()                   audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_0()

#define audio_reg_hal_set_anc_sft_r_para_anc_if_sft_r_1(v)                  audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_1(v)
#define audio_reg_hal_get_anc_sft_r_para_anc_if_sft_r_1()                   audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_1()

#define audio_reg_hal_set_anc_sft_r_para_anc_if_sft_r_2(v)                  audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_2(v)
#define audio_reg_hal_get_anc_sft_r_para_anc_if_sft_r_2()                   audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_2()

#define audio_reg_hal_set_anc_sft_r_para_anc_if_sft_r_3(v)                  audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_3(v)
#define audio_reg_hal_get_anc_sft_r_para_anc_if_sft_r_3()                   audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_3()

#define audio_reg_hal_set_anc_sft_r_para_anc_if_sft_r_4(v)                  audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_4(v)
#define audio_reg_hal_get_anc_sft_r_para_anc_if_sft_r_4()                   audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_4()

#define audio_reg_hal_set_anc_sft_r_para_anc_if_sft_r_5(v)                  audio_reg_ll_set_anc_sft_r_para_anc_if_sft_r_5(v)
#define audio_reg_hal_get_anc_sft_r_para_anc_if_sft_r_5()                   audio_reg_ll_get_anc_sft_r_para_anc_if_sft_r_5()

//reg k_val_0:
#define audio_reg_hal_set_k_val_0_value(v)                                  audio_reg_ll_set_k_val_0_value(v)
#define audio_reg_hal_get_k_val_0_value()                                   audio_reg_ll_get_k_val_0_value()

#define audio_reg_hal_set_k_val_0_k_val_0(v)                                audio_reg_ll_set_k_val_0_k_val_0(v)
#define audio_reg_hal_get_k_val_0_k_val_0()                                 audio_reg_ll_get_k_val_0_k_val_0()

//reg k_val_1:
#define audio_reg_hal_set_k_val_1_value(v)                                  audio_reg_ll_set_k_val_1_value(v)
#define audio_reg_hal_get_k_val_1_value()                                   audio_reg_ll_get_k_val_1_value()

#define audio_reg_hal_set_k_val_1_k_val_1(v)                                audio_reg_ll_set_k_val_1_k_val_1(v)
#define audio_reg_hal_get_k_val_1_k_val_1()                                 audio_reg_ll_get_k_val_1_k_val_1()

//reg k_val_2:
#define audio_reg_hal_set_k_val_2_value(v)                                  audio_reg_ll_set_k_val_2_value(v)
#define audio_reg_hal_get_k_val_2_value()                                   audio_reg_ll_get_k_val_2_value()

#define audio_reg_hal_set_k_val_2_k_val_2(v)                                audio_reg_ll_set_k_val_2_k_val_2(v)
#define audio_reg_hal_get_k_val_2_k_val_2()                                 audio_reg_ll_get_k_val_2_k_val_2()

//reg k_val_3:
#define audio_reg_hal_set_k_val_3_value(v)                                  audio_reg_ll_set_k_val_3_value(v)
#define audio_reg_hal_get_k_val_3_value()                                   audio_reg_ll_get_k_val_3_value()

#define audio_reg_hal_set_k_val_3_k_val_3(v)                                audio_reg_ll_set_k_val_3_k_val_3(v)
#define audio_reg_hal_get_k_val_3_k_val_3()                                 audio_reg_ll_get_k_val_3_k_val_3()

//reg k_val_4:
#define audio_reg_hal_set_k_val_4_value(v)                                  audio_reg_ll_set_k_val_4_value(v)
#define audio_reg_hal_get_k_val_4_value()                                   audio_reg_ll_get_k_val_4_value()

#define audio_reg_hal_set_k_val_4_k_val_4(v)                                audio_reg_ll_set_k_val_4_k_val_4(v)
#define audio_reg_hal_get_k_val_4_k_val_4()                                 audio_reg_ll_get_k_val_4_k_val_4()

//reg k_val_5:
#define audio_reg_hal_set_k_val_5_value(v)                                  audio_reg_ll_set_k_val_5_value(v)
#define audio_reg_hal_get_k_val_5_value()                                   audio_reg_ll_get_k_val_5_value()

#define audio_reg_hal_set_k_val_5_k_val_5(v)                                audio_reg_ll_set_k_val_5_k_val_5(v)
#define audio_reg_hal_get_k_val_5_k_val_5()                                 audio_reg_ll_get_k_val_5_k_val_5()

//reg k_val_6:
#define audio_reg_hal_set_k_val_6_value(v)                                  audio_reg_ll_set_k_val_6_value(v)
#define audio_reg_hal_get_k_val_6_value()                                   audio_reg_ll_get_k_val_6_value()

#define audio_reg_hal_set_k_val_6_k_val_6(v)                                audio_reg_ll_set_k_val_6_k_val_6(v)
#define audio_reg_hal_get_k_val_6_k_val_6()                                 audio_reg_ll_get_k_val_6_k_val_6()

//reg k_val_7:
#define audio_reg_hal_set_k_val_7_value(v)                                  audio_reg_ll_set_k_val_7_value(v)
#define audio_reg_hal_get_k_val_7_value()                                   audio_reg_ll_get_k_val_7_value()

#define audio_reg_hal_set_k_val_7_k_val_7(v)                                audio_reg_ll_set_k_val_7_k_val_7(v)
#define audio_reg_hal_get_k_val_7_k_val_7()                                 audio_reg_ll_get_k_val_7_k_val_7()

//reg st_val_0:
#define audio_reg_hal_set_st_val_0_value(v)                                 audio_reg_ll_set_st_val_0_value(v)
#define audio_reg_hal_get_st_val_0_value()                                  audio_reg_ll_get_st_val_0_value()

#define audio_reg_hal_set_st_val_0_st_val_0(v)                              audio_reg_ll_set_st_val_0_st_val_0(v)
#define audio_reg_hal_get_st_val_0_st_val_0()                               audio_reg_ll_get_st_val_0_st_val_0()

//reg st_val_1:
#define audio_reg_hal_set_st_val_1_value(v)                                 audio_reg_ll_set_st_val_1_value(v)
#define audio_reg_hal_get_st_val_1_value()                                  audio_reg_ll_get_st_val_1_value()

#define audio_reg_hal_set_st_val_1_st_val_1(v)                              audio_reg_ll_set_st_val_1_st_val_1(v)
#define audio_reg_hal_get_st_val_1_st_val_1()                               audio_reg_ll_get_st_val_1_st_val_1()

//reg st_val_2:
#define audio_reg_hal_set_st_val_2_value(v)                                 audio_reg_ll_set_st_val_2_value(v)
#define audio_reg_hal_get_st_val_2_value()                                  audio_reg_ll_get_st_val_2_value()

#define audio_reg_hal_set_st_val_2_st_val_2(v)                              audio_reg_ll_set_st_val_2_st_val_2(v)
#define audio_reg_hal_get_st_val_2_st_val_2()                               audio_reg_ll_get_st_val_2_st_val_2()

//reg st_val_3:
#define audio_reg_hal_set_st_val_3_value(v)                                 audio_reg_ll_set_st_val_3_value(v)
#define audio_reg_hal_get_st_val_3_value()                                  audio_reg_ll_get_st_val_3_value()

#define audio_reg_hal_set_st_val_3_st_val_3(v)                              audio_reg_ll_set_st_val_3_st_val_3(v)
#define audio_reg_hal_get_st_val_3_st_val_3()                               audio_reg_ll_get_st_val_3_st_val_3()

//reg st_val_4:
#define audio_reg_hal_set_st_val_4_value(v)                                 audio_reg_ll_set_st_val_4_value(v)
#define audio_reg_hal_get_st_val_4_value()                                  audio_reg_ll_get_st_val_4_value()

#define audio_reg_hal_set_st_val_4_st_val_4(v)                              audio_reg_ll_set_st_val_4_st_val_4(v)
#define audio_reg_hal_get_st_val_4_st_val_4()                               audio_reg_ll_get_st_val_4_st_val_4()

//reg st_val_5:
#define audio_reg_hal_set_st_val_5_value(v)                                 audio_reg_ll_set_st_val_5_value(v)
#define audio_reg_hal_get_st_val_5_value()                                  audio_reg_ll_get_st_val_5_value()

#define audio_reg_hal_set_st_val_5_st_val_5(v)                              audio_reg_ll_set_st_val_5_st_val_5(v)
#define audio_reg_hal_get_st_val_5_st_val_5()                               audio_reg_ll_get_st_val_5_st_val_5()

//reg st_val_6:
#define audio_reg_hal_set_st_val_6_value(v)                                 audio_reg_ll_set_st_val_6_value(v)
#define audio_reg_hal_get_st_val_6_value()                                  audio_reg_ll_get_st_val_6_value()

#define audio_reg_hal_set_st_val_6_st_val_6(v)                              audio_reg_ll_set_st_val_6_st_val_6(v)
#define audio_reg_hal_get_st_val_6_st_val_6()                               audio_reg_ll_get_st_val_6_st_val_6()

//reg st_val_7:
#define audio_reg_hal_set_st_val_7_value(v)                                 audio_reg_ll_set_st_val_7_value(v)
#define audio_reg_hal_get_st_val_7_value()                                  audio_reg_ll_get_st_val_7_value()

#define audio_reg_hal_set_st_val_7_st_val_7(v)                              audio_reg_ll_set_st_val_7_st_val_7(v)
#define audio_reg_hal_get_st_val_7_st_val_7()                               audio_reg_ll_get_st_val_7_st_val_7()

//reg p_val_0:
#define audio_reg_hal_set_p_val_0_value(v)                                  audio_reg_ll_set_p_val_0_value(v)
#define audio_reg_hal_get_p_val_0_value()                                   audio_reg_ll_get_p_val_0_value()

#define audio_reg_hal_set_p_val_0_p_val_0(v)                                audio_reg_ll_set_p_val_0_p_val_0(v)
#define audio_reg_hal_get_p_val_0_p_val_0()                                 audio_reg_ll_get_p_val_0_p_val_0()

//reg p_val_1:
#define audio_reg_hal_set_p_val_1_value(v)                                  audio_reg_ll_set_p_val_1_value(v)
#define audio_reg_hal_get_p_val_1_value()                                   audio_reg_ll_get_p_val_1_value()

#define audio_reg_hal_set_p_val_1_p_val_1(v)                                audio_reg_ll_set_p_val_1_p_val_1(v)
#define audio_reg_hal_get_p_val_1_p_val_1()                                 audio_reg_ll_get_p_val_1_p_val_1()

//reg p_val_2:
#define audio_reg_hal_set_p_val_2_value(v)                                  audio_reg_ll_set_p_val_2_value(v)
#define audio_reg_hal_get_p_val_2_value()                                   audio_reg_ll_get_p_val_2_value()

#define audio_reg_hal_set_p_val_2_p_val_2(v)                                audio_reg_ll_set_p_val_2_p_val_2(v)
#define audio_reg_hal_get_p_val_2_p_val_2()                                 audio_reg_ll_get_p_val_2_p_val_2()

//reg p_val_3:
#define audio_reg_hal_set_p_val_3_value(v)                                  audio_reg_ll_set_p_val_3_value(v)
#define audio_reg_hal_get_p_val_3_value()                                   audio_reg_ll_get_p_val_3_value()

#define audio_reg_hal_set_p_val_3_p_val_3(v)                                audio_reg_ll_set_p_val_3_p_val_3(v)
#define audio_reg_hal_get_p_val_3_p_val_3()                                 audio_reg_ll_get_p_val_3_p_val_3()

//reg p_val_4:
#define audio_reg_hal_set_p_val_4_value(v)                                  audio_reg_ll_set_p_val_4_value(v)
#define audio_reg_hal_get_p_val_4_value()                                   audio_reg_ll_get_p_val_4_value()

#define audio_reg_hal_set_p_val_4_p_val_4(v)                                audio_reg_ll_set_p_val_4_p_val_4(v)
#define audio_reg_hal_get_p_val_4_p_val_4()                                 audio_reg_ll_get_p_val_4_p_val_4()

//reg p_val_5:
#define audio_reg_hal_set_p_val_5_value(v)                                  audio_reg_ll_set_p_val_5_value(v)
#define audio_reg_hal_get_p_val_5_value()                                   audio_reg_ll_get_p_val_5_value()

#define audio_reg_hal_set_p_val_5_p_val_5(v)                                audio_reg_ll_set_p_val_5_p_val_5(v)
#define audio_reg_hal_get_p_val_5_p_val_5()                                 audio_reg_ll_get_p_val_5_p_val_5()

//reg p_val_6:
#define audio_reg_hal_set_p_val_6_value(v)                                  audio_reg_ll_set_p_val_6_value(v)
#define audio_reg_hal_get_p_val_6_value()                                   audio_reg_ll_get_p_val_6_value()

#define audio_reg_hal_set_p_val_6_p_val_6(v)                                audio_reg_ll_set_p_val_6_p_val_6(v)
#define audio_reg_hal_get_p_val_6_p_val_6()                                 audio_reg_ll_get_p_val_6_p_val_6()

//reg anc_iir_bps:
#define audio_reg_hal_set_anc_iir_bps_value(v)                              audio_reg_ll_set_anc_iir_bps_value(v)
#define audio_reg_hal_get_anc_iir_bps_value()                               audio_reg_ll_get_anc_iir_bps_value()

#define audio_reg_hal_set_anc_iir_bps_anc_iir_bps0(v)                       audio_reg_ll_set_anc_iir_bps_anc_iir_bps0(v)
#define audio_reg_hal_get_anc_iir_bps_anc_iir_bps0()                        audio_reg_ll_get_anc_iir_bps_anc_iir_bps0()

#define audio_reg_hal_set_anc_iir_bps_anc_iir_bps1(v)                       audio_reg_ll_set_anc_iir_bps_anc_iir_bps1(v)
#define audio_reg_hal_get_anc_iir_bps_anc_iir_bps1()                        audio_reg_ll_get_anc_iir_bps_anc_iir_bps1()

#define audio_reg_hal_set_anc_iir_bps_anc_iir_bps2(v)                       audio_reg_ll_set_anc_iir_bps_anc_iir_bps2(v)
#define audio_reg_hal_get_anc_iir_bps_anc_iir_bps2()                        audio_reg_ll_get_anc_iir_bps_anc_iir_bps2()

//reg interface_matrix:
#define audio_reg_hal_set_interface_matrix_value(v)                         audio_reg_ll_set_interface_matrix_value(v)
#define audio_reg_hal_get_interface_matrix_value()                          audio_reg_ll_get_interface_matrix_value()

#define audio_reg_hal_set_interface_matrix_adc_chn0_sel(v)                  audio_reg_ll_set_interface_matrix_adc_chn0_sel(v)
#define audio_reg_hal_get_interface_matrix_adc_chn0_sel()                   audio_reg_ll_get_interface_matrix_adc_chn0_sel()

#define audio_reg_hal_set_interface_matrix_adc_chn1_sel(v)                  audio_reg_ll_set_interface_matrix_adc_chn1_sel(v)
#define audio_reg_hal_get_interface_matrix_adc_chn1_sel()                   audio_reg_ll_get_interface_matrix_adc_chn1_sel()

#define audio_reg_hal_set_interface_matrix_adc_chn2_sel(v)                  audio_reg_ll_set_interface_matrix_adc_chn2_sel(v)
#define audio_reg_hal_get_interface_matrix_adc_chn2_sel()                   audio_reg_ll_get_interface_matrix_adc_chn2_sel()

#define audio_reg_hal_set_interface_matrix_adc_chn3_sel(v)                  audio_reg_ll_set_interface_matrix_adc_chn3_sel(v)
#define audio_reg_hal_get_interface_matrix_adc_chn3_sel()                   audio_reg_ll_get_interface_matrix_adc_chn3_sel()

#define audio_reg_hal_set_interface_matrix_adc_chn4_sel(v)                  audio_reg_ll_set_interface_matrix_adc_chn4_sel(v)
#define audio_reg_hal_get_interface_matrix_adc_chn4_sel()                   audio_reg_ll_get_interface_matrix_adc_chn4_sel()

#define audio_reg_hal_set_interface_matrix_dac_r_chn_sel(v)                 audio_reg_ll_set_interface_matrix_dac_r_chn_sel(v)
#define audio_reg_hal_get_interface_matrix_dac_r_chn_sel()                  audio_reg_ll_get_interface_matrix_dac_r_chn_sel()

#define audio_reg_hal_set_interface_matrix_dac_l_chn_sel(v)                 audio_reg_ll_set_interface_matrix_dac_l_chn_sel(v)
#define audio_reg_hal_get_interface_matrix_dac_l_chn_sel()                  audio_reg_ll_get_interface_matrix_dac_l_chn_sel()

#define audio_reg_hal_set_interface_matrix_clk_frc_on(v)                    audio_reg_ll_set_interface_matrix_clk_frc_on(v)
#define audio_reg_hal_get_interface_matrix_clk_frc_on()                     audio_reg_ll_get_interface_matrix_clk_frc_on()


/* AUDIO_FIFO */
//reg spk0_a2dp_port:
#define audio_fifo_hal_set_spk0_a2dp_port_value(v)                          audio_fifo_ll_set_spk0_a2dp_port_value(v)
#define audio_fifo_hal_get_spk0_a2dp_port_value()                           audio_fifo_ll_get_spk0_a2dp_port_value()

#define audio_fifo_hal_set_spk0_a2dp_port_spk0_a2dp(v)                      audio_fifo_ll_set_spk0_a2dp_port_spk0_a2dp(v)
#define audio_fifo_hal_get_spk0_a2dp_port_spk0_a2dp()                       audio_fifo_ll_get_spk0_a2dp_port_spk0_a2dp()

//reg spk0_call_port:
#define audio_fifo_hal_set_spk0_call_port_value(v)                          audio_fifo_ll_set_spk0_call_port_value(v)
#define audio_fifo_hal_get_spk0_call_port_value()                           audio_fifo_ll_get_spk0_call_port_value()

#define audio_fifo_hal_set_spk0_call_port_spk0_call(v)                      audio_fifo_ll_set_spk0_call_port_spk0_call(v)
#define audio_fifo_hal_get_spk0_call_port_spk0_call()                       audio_fifo_ll_get_spk0_call_port_spk0_call()

//reg spk0_hint_port:
#define audio_fifo_hal_set_spk0_hint_port_value(v)                          audio_fifo_ll_set_spk0_hint_port_value(v)
#define audio_fifo_hal_get_spk0_hint_port_value()                           audio_fifo_ll_get_spk0_hint_port_value()

#define audio_fifo_hal_set_spk0_hint_port_spk0_hint(v)                      audio_fifo_ll_set_spk0_hint_port_spk0_hint(v)
#define audio_fifo_hal_get_spk0_hint_port_spk0_hint()                       audio_fifo_ll_get_spk0_hint_port_spk0_hint()

//reg spk1_a2dp_port:
#define audio_fifo_hal_set_spk1_a2dp_port_value(v)                          audio_fifo_ll_set_spk1_a2dp_port_value(v)
#define audio_fifo_hal_get_spk1_a2dp_port_value()                           audio_fifo_ll_get_spk1_a2dp_port_value()

#define audio_fifo_hal_set_spk1_a2dp_port_spk1_a2dp(v)                      audio_fifo_ll_set_spk1_a2dp_port_spk1_a2dp(v)
#define audio_fifo_hal_get_spk1_a2dp_port_spk1_a2dp()                       audio_fifo_ll_get_spk1_a2dp_port_spk1_a2dp()

//reg spk1_call_port:
#define audio_fifo_hal_set_spk1_call_port_value(v)                          audio_fifo_ll_set_spk1_call_port_value(v)
#define audio_fifo_hal_get_spk1_call_port_value()                           audio_fifo_ll_get_spk1_call_port_value()

#define audio_fifo_hal_set_spk1_call_port_spk1_call(v)                      audio_fifo_ll_set_spk1_call_port_spk1_call(v)
#define audio_fifo_hal_get_spk1_call_port_spk1_call()                       audio_fifo_ll_get_spk1_call_port_spk1_call()

//reg spk1_hint_port:
#define audio_fifo_hal_set_spk1_hint_port_value(v)                          audio_fifo_ll_set_spk1_hint_port_value(v)
#define audio_fifo_hal_get_spk1_hint_port_value()                           audio_fifo_ll_get_spk1_hint_port_value()

#define audio_fifo_hal_set_spk1_hint_port_spk1_hint(v)                      audio_fifo_ll_set_spk1_hint_port_spk1_hint(v)
#define audio_fifo_hal_get_spk1_hint_port_spk1_hint()                       audio_fifo_ll_get_spk1_hint_port_spk1_hint()

//reg mic0_data_bus:
#define audio_fifo_hal_set_mic0_data_bus_value(v)                           audio_fifo_ll_set_mic0_data_bus_value(v)
#define audio_fifo_hal_get_mic0_data_bus_value()                            audio_fifo_ll_get_mic0_data_bus_value()

#define audio_fifo_hal_get_mic0_data_bus_mic0_data_bus()                    audio_fifo_ll_get_mic0_data_bus_mic0_data_bus()

//reg mic1_data_bus:
#define audio_fifo_hal_set_mic1_data_bus_value(v)                           audio_fifo_ll_set_mic1_data_bus_value(v)
#define audio_fifo_hal_get_mic1_data_bus_value()                            audio_fifo_ll_get_mic1_data_bus_value()

#define audio_fifo_hal_get_mic1_data_bus_mic1_data_bus()                    audio_fifo_ll_get_mic1_data_bus_mic1_data_bus()


bk_err_t aud_hal_adc_hpf_config(aud_adc_hpf_config_t *config);
bk_err_t aud_hal_adc_agc_config(aud_adc_agc_config_t *config);
bk_err_t aud_hal_dac_hpf_config(aud_dac_hpf_config_t *config);
bk_err_t aud_hal_dac_filt_config(aud_dac_eq_config_t *config);
bk_err_t aud_hal_dtmf_config(aud_dtmf_config_t *config);

/* get adc fifo port address */
bk_err_t aud_hal_adc_get_mic0_data_bus_fifo_addr(uint32_t *fifo_addr);
bk_err_t aud_hal_adc_get_mic1_data_bus_fifo_addr(uint32_t *fifo_addr);

/* get dac fifo port address */
bk_err_t aud_hal_dac_spk0_get_a2dp_fifo_addr(uint32_t *fifo_addr);
bk_err_t aud_hal_dac_spk0_get_call_fifo_addr(uint32_t *fifo_addr);
bk_err_t aud_hal_dac_spk0_get_hint_fifo_addr(uint32_t *fifo_addr);
bk_err_t aud_hal_dac_spk1_get_a2dp_fifo_addr(uint32_t *fifo_addr);
bk_err_t aud_hal_dac_spk1_get_call_fifo_addr(uint32_t *fifo_addr);
bk_err_t aud_hal_dac_spk1_get_hint_fifo_addr(uint32_t *fifo_addr);

/* get dtmf fifo port address */
bk_err_t aud_hal_dtmf_get_fifo_addr(uint32_t *dtmf_fifo_addr);

/* get dmic fifo port address */
bk_err_t aud_hal_dmic_get_fifo_addr(uint32_t *dmic_fifo_addr);


#if CFG_HAL_DEBUG_AUD
void aud_struct_dump(void);
#else
#define aud_struct_dump()
#endif


#ifdef __cplusplus
}
#endif
