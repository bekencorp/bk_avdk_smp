// Copyright 2020-2025 Beken
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

#include "sys_types.h"
#include <driver/hal/hal_uart_types.h>
#include <driver/hal/hal_pwm_types.h>
#include <driver/hal/hal_timer_types.h>
#include <driver/hal/hal_spi_types.h>
#include <driver/hal/hal_i2c_types.h>
#include <driver/sys_pm_types.h>
#include <modules/pm.h>

#if CONFIG_GPIO_CLOCK_PIN_SUPPORT
#include <driver/hal/hal_clock_types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**  Platform Start **/

//Platform
typedef struct {
	sys_hw_t *hw;
} sys_hal_t;

/** Platform Misc Start **/
bk_err_t sys_hal_init(void);
void sys_hal_enable_swd(void);
/** Platform Misc End **/

/** Platform USB Start **/
void sys_hal_usb_enable_clk(bool en);
void sys_hal_usb_analog_phy_en(bool en);
void sys_hal_usb_analog_speed_en(bool en);
void sys_hal_usb_analog_ckmcu_en(bool en);
void sys_hal_usb_analog_deepsleep_en(bool en);
void sys_hal_set_usb_analog_dp_capability(uint8_t capability);
void sys_hal_set_usb_analog_dn_capability(uint8_t capability);
void sys_hal_usb_enable_charge(bool en);
void sys_hal_usb_charge_vlcf_cal();
void sys_hal_usb_charge_icp_cal();
void sys_hal_usb_charge_vcv_cal();
void sys_hal_usb_charge_get_cal();

/** Platform USB End **/

/** Platform PWM Start **/
void sys_hal_pwm_set_clock(uint32_t mode, uint32_t param);
/** Platform PWM End **/

/** Platform flash Start **/
void sys_drv_flash_select_clock(flash_clk_src_t src, flash_clk_div_t div);

void sys_hal_flash_set_clk(uint32_t value);

void sys_hal_flash_set_clk_div(uint32_t value);

uint32_t sys_hal_flash_get_clk_sel(void);

uint32_t sys_hal_flash_get_clk_div(void);

/** Platform flash End **/

/** Platform qspi Start **/
void sys_hal_set_qspi_vddram_voltage(uint32_t param);

void sys_hal_set_qspi_io_voltage(uint32_t param);

void sys_hal_qspi_clk_sel(uint32_t id, uint32_t param);

void sys_hal_qspi_set_src_clk_div(uint32_t id, uint32_t value);

/** Platform qspi End **/

/** Platform SDIO Start **/
void sys_hal_set_sdio_clk_en(uint32_t value);
void sys_hal_set_cpu0_sdio_int_en(uint32_t value);
void sys_hal_set_cpu1_sdio_int_en(uint32_t value);
void sys_hal_set_cpu2_sdio_int_en(uint32_t value);
void sys_hal_set_sdio_clk_div(uint32_t value);
uint32_t sys_hal_get_sdio_clk_div();
void sys_hal_set_sdio_clk_sel(uint32_t value);
uint32_t sys_hal_get_sdio_clk_sel();
/** Platform SDIO End **/

/*low power feature start*/
void sys_hal_enter_deep_sleep(void *param);
void sys_hal_enter_normal_sleep();
void sys_hal_enter_normal_wakeup();
void sys_hal_enter_low_voltage(void);
void sys_hal_enter_cpu_wfi(void);
void sys_hal_module_power_ctrl(power_module_name_t module,power_module_state_t power_state);
void sys_hal_wakeup_interrupt_clear(wakeup_source_t interrupt_source);
void sys_hal_module_power_ctrl(power_module_name_t module,power_module_state_t power_state);
void sys_hal_module_RF_power_ctrl (module_name_t module,power_module_state_t power_state);
void sys_hal_cpu0_main_int_ctrl(dev_clk_pwr_ctrl_t clock_state);
void sys_hal_cpu1_main_int_ctrl(dev_clk_pwr_ctrl_t clock_state);
void sys_hal_set_cpu1_boot_address_offset(uint32_t address_offset);
void sys_hal_set_cpu1_reset(uint32_t reset_value);
void sys_hal_set_cpu1_pwr_dw(uint32_t is_pwr_down);
void sys_hal_set_cpu2_boot_address_offset(uint32_t address_offset);
void sys_hal_set_cpu2_reset(uint32_t reset_value);
void sys_hal_set_cpu2_pwr_dw(uint32_t is_pwr_down);
void sys_hal_enable_mac_wakeup_source();
void sys_hal_disable_mac_wakeup_source();
void sys_hal_enable_bt_wakeup_source();
void sys_hal_disable_bt_wakeup_source();

uint32_t sys_hal_all_modules_clk_div_get(clk_div_reg_e reg);
void sys_hal_all_modules_clk_div_set(clk_div_reg_e reg, uint32_t value);
void sys_hal_usb_wakeup_enable(uint8_t index);
void sys_hal_touch_wakeup_enable(uint8_t index);
void sys_hal_rtc_wakeup_enable(uint32_t value);
void sys_hal_rtc_ana_wakeup_enable(uint32_t period);
void sys_hal_gpio_ana_wakeup_enable(uint32_t count, uint32_t index, uint32_t type);
#if !CONFIG_AON_PMU_REG0_REFACTOR_DEV
void sys_hal_gpio_state_switch(bool lock);
#endif
void sys_hal_cpu_clk_div_set(uint32_t core_index, uint32_t value);
uint32_t sys_hal_cpu_clk_div_get(uint32_t core_index);
void sys_hal_low_power_hardware_init();
void sys_hal_set_iobyapssen(uint32_t enable);
void sys_hal_set_violdosel(uint32_t flag);
void sys_hal_lp_anabuf_set(uint32_t value);
int32 sys_hal_lp_vol_set(uint32_t value);
uint32_t sys_hal_lp_vol_get();
int32 sys_hal_rf_tx_vol_set(uint32_t value);
uint32_t sys_hal_rf_tx_vol_get();
int32 sys_hal_rf_rx_vol_set(uint32_t value);
uint32_t sys_hal_rf_rx_vol_get();
int32 sys_hal_module_power_state_get(power_module_name_t module);
int32 sys_hal_rosc_calibration(uint32_t rosc_cali_mode, uint32_t cali_interval);
int sys_hal_rosc_test_mode(bool enabled);
int32 sys_hal_bandgap_cali_set(uint32_t value);//increase or decrease the dvdddig voltage
uint32_t sys_hal_bandgap_cali_get();
bk_err_t sys_hal_switch_cpu_bus_freq(pm_cpu_freq_e cpu_bus_freq);
bk_err_t sys_hal_core_bus_clock_ctrl(uint32_t cksel_core, uint32_t ckdiv_core,uint32_t ckdiv_bus, uint32_t ckdiv_cpu0,uint32_t ckdiv_cpu1);
bk_err_t sys_hal_cpu_freq_dump();
void sys_hal_set_cpu0_rxevt_sel(uint32_t param);
void sys_hal_set_cpu1_rxevt_sel(uint32_t param);
void sys_hal_set_cpu2_rxevt_sel(uint32_t param);

/*low power feature end*/
uint32 sys_hal_get_chip_id(void);
uint32 sys_hal_get_device_id(void);
int32 sys_hal_usb_power_down(void);
int32 sys_hal_usb_power_up(void);
int32_t sys_hal_set_int_en(uint32_t core_id, uint32_t int_num, uint32_t int_en);
int32 sys_hal_int_disable(uint32_t core_index,uint32 param);
int32 sys_hal_int_enable(uint32_t core_index,uint32 param);
int32 sys_hal_int_group2_disable(uint32_t core_index,uint32 param);
int32 sys_hal_int_group2_enable(uint32_t core_index,uint32 param);
int32 sys_hal_core_int_group1_disable(uint32_t core_id, uint32 param);
int32 sys_hal_core_int_group1_enable(uint32_t core_id, uint32 param);
int32 sys_hal_core_int_group2_disable(uint32_t core_id, uint32 param);
int32 sys_hal_core_int_group2_enable(uint32_t core_id, uint32 param);
int32 sys_hal_fiq_disable(uint32_t core_id,uint32 param);
int32 sys_hal_fiq_enable(uint32_t core_id,uint32 param);
int32 sys_hal_global_int_disable(uint32 param);
int32 sys_hal_global_int_enable(uint32 param);
uint32 sys_hal_get_int_status(uint32_t core_id);
uint32 sys_hal_get_int_group2_status(uint32_t core_id);
int32 sys_hal_set_int_status(uint32 param);
/* REG_0x29:cpu0_int_32_63_status->cpu0_gpio_int_st: ,R,0x29[22]*/
uint32_t sys_hal_get_cpu0_gpio_int_st(void);
uint32 sys_hal_get_fiq_reg_status(uint32_t core_id);
uint32 sys_hal_set_fiq_reg_status(uint32 param);
uint32 sys_hal_get_intr_raw_status(void);
uint32 sys_hal_set_intr_raw_status(uint32 param);
int32 sys_hal_set_jtag_mode(uint32 param);
uint32 sys_hal_get_jtag_mode(void);

/*clock power control start*/
void sys_hal_clk_pwr_ctrl(dev_clk_pwr_id_t dev, dev_clk_pwr_ctrl_t power_up);
void sys_hal_set_clk_select(dev_clk_select_id_t dev, dev_clk_select_t clk_sel);
dev_clk_select_t sys_hal_get_clk_select(dev_clk_select_id_t dev);
//DCO divider is valid for all of the peri-devices.
void sys_hal_set_dco_div(dev_clk_dco_div_t div);
//DCO divider is valid for all of the peri-devices.
dev_clk_dco_div_t sys_hal_get_dco_div(void);
__IRAM_SEC void sys_hal_adjust_dpll(void);
__IRAM_SEC void sys_hal_adjust_dco(void);
/*clock power control end*/

/* UART select clock    DIRTY **/
void sys_hal_uart_select_clock(uart_id_t id, uart_src_clk_t mode);
/* UART select clock    DIRTY **/


/* I2C select clock    DIRTY **/
void sys_hal_i2c_select_clock(i2c_id_t id, i2c_src_clk_t mode);
/* I2C select clock    DIRTY **/

void sys_hal_arm_wakeup_enable(uint32_t param);
void sys_hal_arm_wakeup_disable(uint32_t param);
uint32_t sys_hal_get_arm_wakeup(void);

void sys_hal_set_cksel_sadc(uint32_t value);
void sys_hal_set_cksel_pwm(uint32_t value);
uint32_t sys_hal_uart_select_clock_get(uart_id_t id);
uint32_t sys_hal_i2c_select_clock_get(i2c_id_t id);
void sys_hal_sadc_int_enable(void);
void sys_hal_sadc_int_disable(void);
void sys_hal_sadc_pwr_up(void);
void sys_hal_sadc_pwr_down(void);
void sys_hal_set_saradc_cali_config(void);
void sys_hal_set_saradc_config(void);
void sys_hal_set_sdmadc_config(void);

/** GADC Config (ana_reg22) Start **/
void     sys_hal_set_gadc_config(uint32_t value);
uint32_t sys_hal_get_gadc_config(void);
void     sys_hal_set_gadc_enable(uint32_t value);
uint32_t sys_hal_get_gadc_enable(void);
void     sys_hal_set_gadc_inbuf_enable(uint32_t value);
uint32_t sys_hal_get_gadc_inbuf_enable(void);
void     sys_hal_set_gadc_calcap_ch(uint32_t value);
uint32_t sys_hal_get_gadc_calcap_ch(void);
void     sys_hal_set_gadc_clk_inv(uint32_t value);
uint32_t sys_hal_get_gadc_clk_inv(void);
void     sys_hal_set_gadc_clk_sel(uint32_t value);
uint32_t sys_hal_get_gadc_clk_sel(void);
void     sys_hal_set_gadc_cal_ramp_enable(uint32_t value);
uint32_t sys_hal_get_gadc_cal_ramp_enable(void);
void     sys_hal_set_gadc_clk_relatch(uint32_t value);
uint32_t sys_hal_get_gadc_clk_relatch(void);
void     sys_hal_set_gadc_vbg_sel(uint32_t value);
uint32_t sys_hal_get_gadc_vbg_sel(void);
void     sys_hal_set_gadc_comp_isel(uint32_t value);
uint32_t sys_hal_get_gadc_comp_isel(void);
void     sys_hal_set_gadc_preamp_isel(uint32_t value);
uint32_t sys_hal_get_gadc_preamp_isel(void);
void     sys_hal_set_gadc_bufamp_isel(uint32_t value);
uint32_t sys_hal_get_gadc_bufamp_isel(void);
void     sys_hal_set_gadc_vref_sel(uint32_t value);
uint32_t sys_hal_get_gadc_vref_sel(void);
void     sys_hal_set_gadc_inbuf_isel(uint32_t value);
uint32_t sys_hal_get_gadc_inbuf_isel(void);
/** GADC Config (ana_reg22) End **/

/** Analog Comparator Config (ana_reg43) Start **/
uint32_t sys_hal_get_ana_reg43_config(void);
void     sys_hal_set_ana_reg43_config(uint32_t value);
void     sys_hal_anacomp_charge_release_pulse(void);
/** Analog Comparator Config (ana_reg43) End **/

/** Audio Bias & Mic Control (ana_reg20/ana_reg27) **/
void     sys_hal_set_micbias_enable(uint32_t value);
void     sys_hal_set_mic2_enable(uint32_t value);

/** VAD Control (ana_reg23) Start **/
void     sys_hal_set_vad_config(uint32_t value);
uint32_t sys_hal_get_vad_config(void);
void     sys_hal_set_vad_enable(uint32_t value);
uint32_t sys_hal_get_vad_enable(void);
void     sys_hal_set_vad_cftrm(uint32_t value);
uint32_t sys_hal_get_vad_cftrm(void);
void     sys_hal_set_vad_cstrm(uint32_t value);
uint32_t sys_hal_get_vad_cstrm(void);
void     sys_hal_set_vad_viniset(uint32_t value);
uint32_t sys_hal_get_vad_viniset(void);
void     sys_hal_set_vad_rstn(uint32_t value);
uint32_t sys_hal_get_vad_rstn(void);
void     sys_hal_set_vad_vcm_sel(uint32_t value);
uint32_t sys_hal_get_vad_vcm_sel(void);
void     sys_hal_set_vad_clk_inv(uint32_t value);
uint32_t sys_hal_get_vad_clk_inv(void);
/** VAD Control (ana_reg23) End **/

/** ana_reg20/24 get/set **/
uint32_t sys_hal_get_ana_reg20_value(void);
void     sys_hal_set_ana_reg24_value(uint32_t value);
uint32_t sys_hal_get_ana_reg24_value(void);

/** Power/LDO Control (ana_reg10/0x4a) Start **/
void     sys_hal_set_core_ldo_low_vol(uint32_t value);
uint32_t sys_hal_get_core_ldo_low_vol(void);
void     sys_hal_set_core_ldo_high_vol(uint32_t value);
uint32_t sys_hal_get_core_ldo_high_vol(void);
void     sys_hal_set_digldo_low_vol_enable(uint32_t value);
void     sys_hal_set_coreldo_low_vol_enable(uint32_t value);
/** Power/LDO Control (ana_reg10/0x4a) End **/

/** LDO HP Mode & RTC Clock (ana_reg9/0x49) Start **/
void     sys_hal_set_aloldo_hp(uint32_t value);
void     sys_hal_set_analdo_hp(uint32_t value);
void     sys_hal_set_digldo_hp(uint32_t value);
void     sys_hal_set_coreldo_hp(uint32_t value);
void     sys_hal_set_rtc_clk_sel(uint32_t value);
void     sys_hal_set_aon_ldo_vol(uint32_t value);
/** LDO HP Mode & RTC Clock (ana_reg9/0x49) End **/

/** BuckA Control (ana_reg12/0x4c) **/
void     sys_hal_set_bucka_ea_enable(uint32_t value);

/** Analog Clock Control (ana_reg5/0x45) Start **/
void     sys_hal_set_dpll_enable(uint32_t value);
void     sys_hal_set_dco_enable(uint32_t value);
void     sys_hal_set_xtall_enable(uint32_t value);
void     sys_hal_set_cb_enable(uint32_t value);
/** Analog Clock Control (ana_reg5/0x45) End **/

void sys_hal_set_clksel_spi(uint32_t value);
void sys_hal_timer_select_clock(sys_sel_timer_t num, timer_src_clk_t mode);
uint32_t sys_hal_timer_select_clock_get(sys_sel_timer_t id);
void sys_hal_spi_select_clock(spi_id_t num, spi_src_clk_t mode);
/* PWM select clock    DIRTY **/
void sys_hal_pwm_select_clock(sys_sel_pwm_t num, pwm_src_clk_t mode);
/* PWM select clock    DIRTY **/

void sys_hal_en_tempdet(uint32_t value);

void sys_hal_trng_disckg_set(uint32_t value);
/**  Platform End **/




/**  BT Start **/
//BT
uint32_t sys_hal_mclk_mux_get(void);
uint32_t sys_hal_mclk_div_get(void);
void sys_hal_mclk_select(uint32_t value);
void sys_hal_mclk_div_set(uint32_t value);

void sys_hal_bt_power_ctrl(bool power_up);

void sys_hal_bt_clock_ctrl(bool en);
void sys_hal_xvr_clock_ctrl(bool en);

uint32_t sys_hal_interrupt_status_get(void);
void sys_hal_interrupt_status_set(uint32_t value);

void sys_hal_btdm_interrupt_ctrl(bool en);
void sys_hal_ble_interrupt_ctrl(bool en);
void sys_hal_bt_interrupt_ctrl(bool en);

void sys_hal_bt_rf_ctrl(bool en);
uint32_t sys_hal_bt_rf_status_get(void);
void sys_hal_bt_sleep_exit_ctrl(bool en);

void sys_hal_rf_ctrl(uint8_t type);
uint8_t sys_hal_rf_ctrl_type_get(void);

/**  BT End **/

/** MAC802.15.4 start **/
#if CONFIG_MAC802154_ENABLE
void sys_hal_thread_interrupt_ctrl(bool en);
#endif
/**  MAC802.15.4 End **/


/**  Audio Start **/
//Audio
/**  Audio End **/



/**  Video Start **/


/**  Video End **/




/**  WIFI Start **/
//WIFI
void sys_hal_cali_dpll_spi_trig_disable(void);
void sys_hal_cali_dpll_spi_trig_enable(void);
void sys_hal_cali_dpll_spi_detect_disable(void);
void sys_hal_cali_dpll_spi_detect_enable(void);
uint32_t sys_hal_bias_reg_clean(uint32_t param);
uint32_t sys_hal_bias_reg_set(uint32_t param);
uint32_t sys_hal_bias_reg_read(void);
uint32_t sys_hal_bias_reg_write(uint32_t param);
uint32_t sys_hal_analog_reg1_get(void);
uint32_t sys_hal_analog_reg2_get(void);
uint32_t sys_hal_analog_reg4_get(void);
uint32_t sys_hal_analog_reg6_get(void);
uint32_t sys_hal_analog_reg7_get(void);
uint32_t sys_hal_analog_reg2_set(uint32_t param);
uint32_t sys_hal_analog_reg4_set(uint32_t param);
void sys_hal_set_xtalh_ctune(uint32_t value);
void sys_hal_set_ana_reg1_value(uint32_t value);
void sys_hal_set_ana_reg2_value(uint32_t value);
uint32_t sys_hal_get_ana_reg2_value(void);
void sys_hal_set_ana_reg3_value(uint32_t value);
void sys_hal_set_ana_reg4_value(uint32_t value);
void sys_hal_set_ana_reg12_value(uint32_t value);
void sys_hal_set_ana_reg13_value(uint32_t value);
void sys_hal_set_ana_reg14_value(uint32_t value);
void sys_hal_set_ana_reg15_value(uint32_t value);
void sys_hal_set_ana_reg16_value(uint32_t value);
void sys_hal_set_ana_reg17_value(uint32_t value);
void sys_hal_set_ana_reg18_value(uint32_t value);
void sys_hal_set_ana_reg19_value(uint32_t value);
void sys_hal_set_ana_reg20_value(uint32_t value);
void sys_hal_set_ana_reg21_value(uint32_t value);
void sys_hal_set_ana_reg27_value(uint32_t value);
uint32_t sys_hal_get_ana_reg27_value(void);
void sys_hal_set_ana_reg28_value(uint32_t value);
uint32_t sys_hal_get_ana_reg28_value(void);
void sys_hal_set_ana_reg29_value(uint32_t value);
void sys_hal_set_ana_reg30_value(uint32_t value);
void sys_hal_set_ana_reg25_value(uint32_t value);

void sys_hal_analog_reg4_bits_or(uint32_t value);
void sys_hal_set_ana_reg6_value(uint32_t value);
void sys_hal_set_ana_reg7_value(uint32_t value);
uint32_t sys_hal_get_xtalh_ctune(void);
uint32_t sys_hal_cali_dpll(uint32_t first_time);
uint32_t sys_hal_cali_bgcalm(void);
uint32_t sys_hal_get_bgcalm(void);
void sys_hal_set_bgcalm(uint32_t value);
void sys_hal_set_audioen(uint32_t value);
void sys_hal_set_dpll_div_cksel(uint32_t value);
void sys_hal_set_dpll_reset(uint32_t value);
void sys_hal_set_gadc_ten(uint32_t value);
void sys_hal_analog_set(analog_reg_t reg, uint32_t value);
uint32_t sys_hal_analog_get(analog_reg_t reg);
//Yantao Add Start
void sys_hal_modem_core_reset(void);
void sys_hal_mpif_invert(void);
void sys_hal_modem_subsys_reset(void);
void sys_hal_mac_subsys_reset(void);
void sys_hal_usb_subsys_reset(void);
void sys_hal_dsp_subsys_reset(void);
void sys_hal_mac_power_ctrl(bool power_up);
void sys_hal_modem_power_ctrl(bool power_up);
void sys_hal_pta_ctrl(bool pta_en);
void sys_hal_modem_bus_clk_ctrl(bool clk_en);
void sys_hal_modem_clk_ctrl(bool clk_en);
void sys_hal_mac_bus_clk_ctrl(bool clk_en);
void sys_hal_mac_clk_ctrl(bool clk_en);
uint32_t sys_hal_clk_pwr_status_get(dev_clk_pwr_id_t dev);
uint32_t sys_hal_clk_pwr_is_enabled(dev_clk_pwr_id_t dev);
void sys_hal_set_cpu_power_sleep_wakeup_pwd_ofdm(uint32_t v);
uint32_t sys_hal_get_cpu_power_sleep_wakeup_pwd_ofdm(void);
uint32_t sys_hal_wifi_wlss_clk_is_enabled(void);
uint32_t sys_hal_wifi_mac_clk_is_enabled(void);
uint32_t sys_hal_wifi_phy_clk_is_enabled(void);
void sys_hal_wifi_reg_access_status_get(uint32_t *clk_status, uint32_t *power_status);
void sys_hal_set_vdd_value(uint32_t param);
uint32_t sys_hal_get_vdd_value(void);
void sys_hal_block_en_mux_set(uint32_t param);
void sys_hal_enable_mac_gen_int(void);
void sys_hal_enable_mac_prot_int(void);
void sys_hal_enable_mac_tx_trigger_int(void);
void sys_hal_enable_mac_rx_trigger_int(void);
void sys_hal_enable_mac_txrx_misc_int(void);
void sys_hal_enable_mac_txrx_timer_int(void);
void sys_hal_enable_modem_int(void);
void sys_hal_enable_modem_rc_int(void);
void sys_hal_enable_hsu_int(void);
void sys_hal_set_debug_mux(int type);
void sys_hal_diag_debug_mac(void);
void sys_hal_diag_debug_phy(void);
void dbg_enable_debug_gpio(void);
void sys_hal_rf_clk_ctrl(bool clk_en);
//Yantao Add End

void sys_hal_set_ana_vtempsel(uint32_t value);
/**  WIFI End **/

/**  MISC Start  */

void sys_hal_set_ioldo_lp(uint32_t value);

/**  MISC End  */

/**  Audio Start  **/

void sys_hal_aud_select_clock(uint32_t value);
void sys_hal_aud_set_ckdiv_audio(uint32_t value);
void sys_hal_aud_clock_en(uint32_t value);
void sys_hal_aud_vdd1v_en(uint32_t value);
void sys_hal_aud_vdd1v5_en(uint32_t value);
void sys_hal_aud_mic1_en(uint32_t value);
void sys_hal_aud_mic2_en(uint32_t value);
void sys_hal_aud_audpll_en(uint32_t value);
void sys_hal_aud_aud_en(uint32_t value);
void sys_hal_aud_dac_drv_en(uint32_t value);
void sys_hal_aud_bias_en(uint32_t value);
void sys_hal_aud_dacr_en(uint32_t value);
void sys_hal_aud_dacl_en(uint32_t value);
//void sys_hal_aud_diffen_en(uint32_t value); ////????
void sys_hal_aud_dac_diffen_en(uint32_t value); 
void sys_hal_aud_rvcmd_en(uint32_t value);
void sys_hal_aud_lvcmd_en(uint32_t value);
void sys_hal_aud_micbias1v_en(uint32_t value);
void sys_hal_aud_micbias_trim_set(uint32_t value);

void sys_hal_aud_mic_rst_set(uint32_t value);
void sys_hal_aud_mic1_rst_set(uint32_t value);

void sys_hal_aud_mic1_gain_set(uint32_t value);
void sys_hal_aud_mic2_gain_set(uint32_t value);
void sys_hal_aud_mic1_single_en(uint32_t value);
void sys_hal_aud_mic2_single_en(uint32_t value);
void sys_hal_aud_dacg_set(uint32_t value);
uint32_t sys_hal_aud_dacg_get(void);
void sys_hal_aud_dcoc_en(uint32_t value);
void sys_hal_aud_lmdcin_set(uint32_t value);
void sys_hal_aud_audbias_en(uint32_t value);
void sys_hal_aud_adcbias_en(uint32_t value);
void sys_hal_aud_micbias_en(uint32_t value);
void sys_hal_aud_dac_bias_en(uint32_t value);
//void sys_hal_aud_idac_en(uint32_t value); ////????
void sys_hal_aud_idacl_en(uint32_t value);
void sys_hal_aud_idacr_en(uint32_t value);
void sys_hal_aud_int_en(uint32_t value);
void sys_hal_sbc_int_en(uint32_t value);
void sys_hal_aud_power_en(uint32_t value);
void sys_hal_aud_dac_dcoc_en(uint32_t value);
void sys_hal_aud_dac_idac_en(uint32_t value);
void sys_hal_aud_dac_bypass_dwa_en(uint32_t value);
void sys_hal_aud_dac_dacmute_en(uint32_t value);

void sys_hal_aud_mic1_en(uint32_t value);
void sys_hal_aud_mic1_rst_set(uint32_t value);
void sys_hal_aud_mic1_gain_set(uint32_t value);
void sys_hal_aud_mic1_single_en(uint32_t value);
void sys_hal_aud_mic2_en(uint32_t value);
void sys_hal_aud_mic2_rst_set(uint32_t value);
void sys_hal_aud_mic2_gain_set(uint32_t value);
void sys_hal_aud_mic2_single_en(uint32_t value);
void sys_hal_aud_mic3_en(uint32_t value);
void sys_hal_aud_mic3_rst_set(uint32_t value);
void sys_hal_aud_mic3_gain_set(uint32_t value);
void sys_hal_aud_mic3_single_en(uint32_t value);
/**  Audio End  **/

void sys_hal_set_sys2flsh_2wire(uint32_t value);

/**  FFT Start  **/

void sys_hal_fft_disckg_set(uint32_t value);
void sys_hal_cpu_fft_int_en(uint32_t value);

/**  FFT End  **/

/**  I2S Start  **/
void sys_hal_i2s_select_clock(uint32_t value);
void sys_hal_i2s_clock_en(uint32_t value);
void sys_hal_i2s1_clock_en(uint32_t value);
void sys_hal_i2s2_clock_en(uint32_t value);
void sys_hal_i2s3_clock_en(uint32_t value);
void sys_hal_i2s4_clock_en(uint32_t value);

void sys_hal_i2s_disckg_set(uint32_t value);
void sys_hal_i2s_int_en(uint32_t value);
void sys_hal_i2s1_int_en(uint32_t value);
void sys_hal_i2s2_int_en(uint32_t value);
void sys_hal_i2s3_int_en(uint32_t value);
void sys_hal_i2s4_int_en(uint32_t value);

void sys_hal_apll_en(uint32_t value);
uint32_t sys_hal_apll_en_get(void);
void sys_hal_cb_manu_val_set(uint32_t value);
void sys_hal_ana_reg11_vsel_set(uint32_t value);
void sys_hal_apll_cal_val_set(uint32_t value);
void sys_hal_apll_spi_trigger_set(uint32_t value);
void sys_hal_i2s0_ckdiv_set(uint32_t value);
void sys_hal_apll_config_set(uint32_t value);
void sys_hal_dmic_clk_div_set(uint32_t value);
/**  I2S End  **/


/**  Touch Start **/
void sys_hal_touch_power_down(uint32_t value);
void sys_hal_touch_sensitivity_level_set(uint32_t value);
void sys_hal_touch_scan_mode_enable(uint32_t value);
void sys_hal_touch_detect_threshold_set(uint32_t value);
void sys_hal_touch_detect_range_set(uint32_t value);
void sys_hal_touch_calib_enable(uint32_t value);
void     sys_hal_touch_cal_ctrl_set(uint32_t value);
uint32_t sys_hal_touch_cal_ctrl_get(void);
void     sys_hal_touch_cal_vth_set(uint32_t value);
uint32_t sys_hal_touch_cal_vth_get(void);
void     sys_hal_touch_rstb_dig_set(uint32_t value);
uint32_t sys_hal_touch_rstb_dig_get(void);
void     sys_hal_touch_ldoen_set(uint32_t value);
uint32_t sys_hal_touch_ldoen_get(void);
void     sys_hal_touch_cal_auto_set(uint32_t value);
void     sys_hal_touch_cal_done_clr(uint32_t value);
void     sys_hal_touch_manul_mode_calib_value_set(uint32_t value);
uint32_t sys_hal_touch_calib_value_get(void);
void sys_hal_touch_manul_mode_enable(uint32_t value);
void sys_hal_touch_scan_mode_chann_set(uint32_t value);
void sys_hal_touch_scan_mode_chann_sel(uint32_t value);
void sys_hal_touch_serial_cap_enable(void);
void sys_hal_touch_serial_cap_disable(void);
void sys_hal_touch_serial_cap_sel(uint32_t value);
void sys_hal_touch_spi_lock(void);
void sys_hal_touch_spi_unlock(void);
void sys_hal_touch_test_period_set(uint32_t value);
void sys_hal_touch_test_number_set(uint32_t value);
void     sys_hal_touch_calib_period_set(uint32_t value);
uint32_t sys_hal_touch_calib_period_get(void);
void     sys_hal_touch_calib_number_set(uint32_t value);
uint32_t sys_hal_touch_calib_number_get(void);
void     sys_hal_touch_modsel_spi_set(uint32_t value);
uint32_t sys_hal_touch_modsel_spi_get(void);
void sys_hal_touch_int_set(uint32_t value);
void sys_hal_touch_int_clear(uint32_t value);
void sys_hal_touch_int_enable(uint32_t value);

/**  Touch End **/


/** jpeg start **/
void sys_hal_mclk_mux_set(uint32_t value);
void sys_hal_set_jpeg_clk_sel(uint32_t value);
void sys_hal_set_clk_div_mode1_clkdiv_jpeg(uint32_t value);
void sys_hal_set_jpeg_disckg(uint32_t value);
void sys_hal_set_cpu_clk_div_mode1_clkdiv_bus(uint32_t value);
void sys_hal_video_power_en(uint32_t value);
void sys_hal_set_auxs_cis_clk_sel(uint32_t value);
void sys_hal_set_auxs_cis_clk_div(uint32_t value);
void sys_hal_set_jpeg_clk_en(uint32_t value);
void sys_hal_set_cis_auxs_clk_en(uint32_t value);

/** jpeg end **/

/** h264 Start **/
void sys_hal_set_h264_clk_en(uint32_t value);

/** h264 End **/

/**  psram Start **/
void sys_hal_psram_volstage_sel(uint32_t enable);
void sys_hal_psram_xtall_osc_enable(uint32_t enable);
void sys_hal_psram_doc_enable(uint32_t enable);
void sys_hal_psram_dpll_enable(uint32_t enable);
void sys_hal_psram_ldo_enable(uint32_t enable);
uint32_t sys_hal_psram_ldo_status();
void sys_hal_psram_clk_sel_with_id(uint32_t id, uint32_t value);
void sys_hal_psram_set_clkdiv_with_id(uint32_t id, uint32_t value);
void sys_hal_psram_psldo_vsel(uint32_t value);
void sys_hal_psram_psldo_vset(uint32_t output_voltage, uint32_t is_add_200mv);
void sys_hal_psram_psram0_disckg(uint32_t value);
void sys_hal_psram_psram1_disckg(uint32_t value);
/* PSRAM bus-clock enable (rega.pramX_cken) routed by id. */
void sys_hal_psram_disckg_with_id(uint32_t id, uint32_t value);

/**  psram End **/

uint32_t sys_hal_get_cpu_storage_connect_op_select_flash_sel(void);
void sys_hal_set_cpu_storage_connect_op_select_flash_sel(uint32_t value);

/** Temp reserve heare  */
void sys_hal_set_btdm_clk_en(bool value);
uint32 sys_hal_get_btdm_clk_en();
void sys_hal_set_xvr_clk_en(bool value);
uint32 sys_hal_get_xvr_clk_en();
void sys_hal_set_power_on_btsp(bool value);
uint32 sys_hal_get_power_on_btsp();
void sys_hal_set_bts_wakeup_platform_en(bool value);
uint32 sys_hal_get_bts_wakeup_platform_en();
void sys_hal_set_bts_sleep_exit_req(bool value);
uint32 sys_hal_get_bts_sleep_exit_req();

void sys_hal_set_ana_reg_spi_latch1v(uint32_t v);
void sys_hal_set_ioldo_bypass(uint32_t v);
void sys_hal_set_ioldo_volt(uint32_t v);
void sys_hal_set_analdo_volt(uint32_t v);
void sys_hal_set_analdo_sel(uint32_t v);
void sys_hal_set_ana_trxt_tst_enable(uint32_t value);
void sys_hal_set_ana_scal_en(uint32_t value);
void sys_hal_set_ana_gadc_buf_ictrl(uint32_t value);
void sys_hal_set_ana_gadc_cmp_ictrl(uint32_t value);
void sys_hal_set_ana_pwd_gadc_buf(uint32_t value);
void sys_hal_set_ana_vref_sel(uint32_t value);
void sys_hal_set_ana_cb_cal_manu(uint32_t value);
void sys_hal_set_ana_cb_cal_trig(uint32_t value);
UINT32 sys_hal_get_ana_cb_cal_manu_val(void);
void sys_hal_set_ana_cb_cal_manu_val(uint32_t value);
void sys_hal_set_ana_vlsel_ldodig(uint32_t value);
void sys_hal_set_ana_vhsel_ldodig(uint32_t value);
void sys_hal_set_ana_vctrl_sysldo(uint32_t value);
void sys_hal_enable_eth_int(uint32_t value);
void sys_hal_set_eth_clk_en(uint32_t value);
void sys_hal_set_yuv_buf_clock_en(uint32_t value);
void sys_hal_set_h264_clock_en(uint32_t value);
void sys_hal_set_ana_reg11_apfms(uint32_t value);
void sys_hal_set_ana_reg12_dpfms(uint32_t value);
void sys_hal_set_ana_reg5_pwd_rosc_spi(uint32_t value);
#if CONFIG_ANA_RTC || CONFIG_ANA_GPIO
void sys_hal_set_ana_reg8_spi_latch1v(uint32_t value);
void sys_hal_set_ana_reg5_adc_div(uint32_t value);
void sys_hal_set_ana_reg5_rosc_disable(uint32_t value);
void sys_hal_set_ana_reg7_timer_wkrstn(uint32_t value);
void sys_hal_set_ana_reg7_clk_sel(uint32_t value);
void sys_hal_set_ana_reg8_spi_latch1v(uint32_t value);
void sys_hal_set_ana_reg8_rst_wks1v(uint32_t value);
void sys_hal_set_ana_reg8_lvsleep_wkrst(uint32_t value);
void sys_hal_set_ana_reg8_gpiowk_rstn(uint32_t value);
void sys_hal_set_ana_reg8_rtcwk_rstn(uint32_t value);
void sys_hal_set_ana_reg8_ensfsdd(uint32_t value);
void sys_hal_set_ana_reg8_vlden(uint32_t value);
void sys_hal_set_ana_reg8_pwdovp1v(uint32_t value);
void sys_hal_set_ana_reg9_spi_timerwken(uint32_t value);
void sys_hal_set_ana_reg9_spi_byp32pwd(uint32_t value);
uint32_t sys_hal_get_ana_reg11_gpiowk(void);
void sys_hal_set_ana_reg11_gpiowk(uint32_t value);
uint32_t sys_hal_get_ana_reg11_rtcsel(void);
void sys_hal_set_ana_reg11_rtcsel(uint32_t value);
uint32_t sys_hal_get_ana_reg11_timersel(void);
void sys_hal_set_ana_reg11_timersel(uint32_t value);
uint32_t sys_hal_get_ana_reg12_timersel(void);
void sys_hal_set_ana_reg12_timersel(uint32_t value);
uint32_t sys_hal_get_ana_reg13_rtcsel(void);
void sys_hal_set_ana_reg13_rtcsel(uint32_t value);
void sys_hal_enable_ana_rtc_int(void);
void sys_hal_disable_ana_rtc_int(void);
void sys_hal_enable_ana_gpio_int(void);
void sys_hal_disable_ana_gpio_int(void);
#endif

#if CONFIG_HAL_DEBUG_SYS
void sys_struct_dump(uint32_t start, uint32_t end);
#else
static inline void __sys_struct_dump(uint32_t start, uint32_t end)
{
	BK_LOGD(NULL, "start=%x, end-%x, please generate the hal_debug.c file!\r\n", start, end);
}
#define sys_struct_dump(start, end) __sys_struct_dump(start, end)
#endif

int sys_hal_set_buck(sys_buck_type_t buck, bool ena);
int sys_hal_set_buck_pfm(sys_buck_type_t buck, bool ena);
int sys_hal_set_buck_burst(sys_buck_type_t buck, bool ena);
int sys_hal_set_buck_mpo(sys_buck_type_t buck, bool ena);
int sys_hal_set_ldo_self_lp(sys_ldo_type_t ldo, bool ena);
int sys_hal_set_ldo_current_limit(sys_ldo_type_t ldo, bool ena);
int sys_hal_set_aon_power(sys_aon_power_t power);
int sys_hal_set_aon_ldo_volt(uint32_t volt);
int sys_hal_set_io_ldo_volt(uint32_t volt);
int sys_hal_set_ana_ldo_volt(bool trsw_ena, uint32_t rx_volt, uint32_t tx_volt);
int sys_hal_set_digital_ldo_volt(bool lp_ena, uint32_t low_volt, uint32_t high_volt);
int sys_hal_set_core_ldo_volt(bool lp_ena, uint32_t low_volt, uint32_t high_volt);
int sys_hal_set_lv_ctrl_pd(bool lp_ena);
int sys_hal_set_lv_ctrl_hf(bool lp_ena);
int sys_hal_set_lv_ctrl_flash(bool lp_ena);
int sys_hal_set_lv_ctrl_core(bool lp_ena);
int sys_hal_set_lpo_src(sys_lpo_src_t src);
void sys_hal_dump_ctrl(void);
void sys_hal_set_ram_sph_cfg(uint32_t value);
void sys_hal_set_ram_tph_cfg(uint32_t value);
void sys_hal_set_ram_spl_cfg(uint32_t value);
void sys_hal_set_ram_tpl_cfg(uint32_t value);
void sys_hal_set_ram_high_speed(void);
void sys_hal_set_ram_low_speed(void);
void sys_hal_early_init(void);
void sys_hal_set_rott_int_en(uint32_t core_index, uint32_t value);
void sys_hal_enter_low_analog(void);
void sys_hal_exit_low_analog(void);
void sys_hal_set_7816_int_en(uint32_t core_id, uint32_t value);
void sys_hal_set_scr_clk(uint32_t value);
uint64_t sys_hal_get_low_voltage_sleep_duration_us(void);
void sys_hal_set_low_voltage_sleep_duration_us(uint64_t sleep_duration);
uint64_t sys_hal_get_low_voltage_wakeup_time_us(void);
void sys_hal_set_low_voltage_wakeup_time_us(uint64_t wakeup_time);
void sys_hal_set_cpu_device_clk_enable_otp_cken(uint32_t value);
void sys_hal_set_cpu_power_sleep_wakeup_ticktimer_32k_enable(uint32_t value);

/* CP: SoC cpu_clk_div_mode1/2/3 clock selection and division */
bk_err_t sys_hal_flash_cksel_clkdiv_set(cksel_sys_flash_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_auxs_cksel_clkdiv_set(cksel_sys_auxs_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_26mo_clkdiv_set(uint32_t ckdiv);
bk_err_t sys_hal_tim0_cksel_set(cksel_sys_tim_t cksel);
bk_err_t sys_hal_tim1_cksel_set(cksel_sys_tim_t cksel);
bk_err_t sys_hal_tim2_cksel_set(cksel_sys_tim_t cksel);
bk_err_t sys_hal_tim3_cksel_set(cksel_sys_tim_t cksel);
bk_err_t sys_hal_i3c_cksel_set(cksel_sys_xtal_apll_t cksel);
bk_err_t sys_hal_sadc_cksel_set(cksel_sys_xtal_apll_t cksel);
bk_err_t sys_hal_i2s0_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_i2s1_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_i2s2_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_i2s3_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_i2s4_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_spi0_cksel_set(cksel_sys_xtal_160m_t cksel);
bk_err_t sys_hal_spi1_cksel_set(cksel_sys_xtal_160m_t cksel);
bk_err_t sys_hal_spi2_cksel_set(cksel_sys_xtal_160m_t cksel);
bk_err_t sys_hal_spi3_cksel_set(cksel_sys_xtal_160m_t cksel);
bk_err_t sys_hal_uart0_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_uart1_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_uart2_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_uart3_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_uart4_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_i2c0_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_i2c3_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_pwm0_cksel_set(cksel_sys_pwm0_t cksel);
bk_err_t sys_hal_can0_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_can1_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_scr0_cksel_set(cksel_sys_xtal_120m_t cksel);
bk_err_t sys_hal_audio_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_audif0_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_audif1_cksel_clkdiv_set(cksel_sys_xtal_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_i2so_clkdiv_set(uint32_t ckdiv);
bk_err_t sys_hal_auxs_enet_cksel_clkdiv_set(cksel_sys_dco_apll_t cksel, uint32_t ckdiv);
bk_err_t sys_hal_trace_cksel_clkdiv_set(cksel_sys_trace_t cksel, uint32_t ckdiv);

#if CONFIG_GPIO_CLOCK_PIN_SUPPORT
/**  System Clock Start **/
void sys_hal_clk_pin_ana_open(clk_pin_ana_src_t src, clk_pin_div_t div, bool en);
void sys_hal_clk_pin_dig_open(clk_pin_dig_src_t src, clk_pin_div_t div, bool en);
/**  System Clock End **/
#endif

/** XDAC Start **/
bk_err_t sys_hal_xdac_clk_en(uint8_t ch, uint8_t clk_en);
bk_err_t sys_hal_xdac_int_en(uint8_t cpu, uint8_t ch, uint8_t int_en);
bk_err_t sys_hal_xdac_set_enspi(uint8_t ch, uint8_t enspi);
bk_err_t sys_hal_xdac_set_endigspi_sel(uint8_t ch, uint8_t endigspi_sel);
/** XDAC end **/

/** reg20 - AHB Peripheral Bus Master QoS Control **/

void     sys_hal_set_psram_qos_value(uint32_t v);
uint32_t sys_hal_get_psram_qos_value(void);

void     sys_hal_set_psram_cpu0_qos(uint32_t v);
uint32_t sys_hal_get_psram_cpu0_qos(void);

void     sys_hal_set_psram_dma1_qos(uint32_t v);
uint32_t sys_hal_get_psram_dma1_qos(void);

void     sys_hal_set_psram_sdio0_qos(uint32_t v);
uint32_t sys_hal_get_psram_sdio0_qos(void);

void     sys_hal_set_psram_sdio1_qos(uint32_t v);
uint32_t sys_hal_get_psram_sdio1_qos(void);

/* NOTE: enet0 QoS is marked as INVALID in hardware spec. Do NOT use. */
void     sys_hal_set_psram_enet0_qos(uint32_t v);
uint32_t sys_hal_get_psram_enet0_qos(void);

/* NOTE: enet1 QoS is marked as INVALID in hardware spec. Do NOT use. */
void     sys_hal_set_psram_enet1_qos(uint32_t v);
uint32_t sys_hal_get_psram_enet1_qos(void);

void     sys_hal_set_psram_usb_qos(uint32_t v);
uint32_t sys_hal_get_psram_usb_qos(void);

void     sys_hal_set_psram_h26e_qos(uint32_t v);
uint32_t sys_hal_get_psram_h26e_qos(void);

void     sys_hal_set_psram_isp_qos(uint32_t v);
uint32_t sys_hal_get_psram_isp_qos(void);

void     sys_hal_set_psram_videopost_qos(uint32_t v);
uint32_t sys_hal_get_psram_videopost_qos(void);

void     sys_hal_set_psram_dpu_qos(uint32_t v);
uint32_t sys_hal_get_psram_dpu_qos(void);

void     sys_hal_set_psram_cbus_qos(uint32_t v);
uint32_t sys_hal_get_psram_cbus_qos(void);

void     sys_hal_set_psram_npu0_qos(uint32_t v);
uint32_t sys_hal_get_psram_npu0_qos(void);

void     sys_hal_set_psram_npu1_qos(uint32_t v);
uint32_t sys_hal_get_psram_npu1_qos(void);

void     sys_hal_set_psram_cpu1_qos(uint32_t v);
uint32_t sys_hal_get_psram_cpu1_qos(void);

#ifdef __cplusplus
}
#endif


