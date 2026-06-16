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

#include "sys_hal.h"
#include "sys_driver.h"
#include "sys_driver_common.h"

/**  Platform Start **/
//Platform

/** Platform Misc Start **/
void sys_drv_init()
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_amp_res_init();
	sys_hal_init();

	sys_drv_exit_critical(int_level);
}

/** Platform Misc End **/

uint32 sys_drv_get_chip_id(void)
{
	uint32 reg = 0;
	uint32_t int_level = sys_drv_enter_critical();

	reg = sys_hal_get_chip_id();

	sys_drv_exit_critical(int_level);

	return reg;
}

// Replace sddev_control(DD_DEV_TYPE_SCTRL,CMD_GET_DEVICE_ID, NULL)
uint32 sys_drv_get_device_id(void)
{
	uint32 reg = 0;
	uint32_t int_level = sys_drv_enter_critical();

	reg = sys_hal_get_device_id();

	sys_drv_exit_critical(int_level);

	return reg;
}

int32 sys_drv_set_jtag_mode(uint32 param)
{
	int32 ret = 0;
	uint32_t int_level = sys_drv_enter_critical();

	ret = sys_hal_set_jtag_mode(param);
	sys_drv_exit_critical(int_level);

	return ret;
}

uint32 sys_drv_get_jtag_mode(void)
{
	int32 ret = 0;
	uint32_t int_level = sys_drv_enter_critical();

	ret = sys_hal_get_jtag_mode();
	sys_drv_exit_critical(int_level);

	return ret;
}

void sys_drv_en_tempdet(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_en_tempdet(value);
    sys_drv_exit_critical(int_level);
}
/**  Platform End **/

uint32_t sys_drv_get_cpu_storage_connect_op_select_flash_sel(void)
{
	return 	sys_hal_get_cpu_storage_connect_op_select_flash_sel();
}

void sys_drv_set_cpu_storage_connect_op_select_flash_sel(uint32_t value)
{
	sys_hal_set_cpu_storage_connect_op_select_flash_sel(value);
}

/**  Misc Start **/
//Misc
/**  Misc End **/

#if 1
void system_driver_set_bts_wakeup_platform_en(bool value)
{
	uint32_t ret = SYS_DRV_FAILURE;
	ret = sys_amp_res_acquire();

	if(value)
		sys_hal_set_bts_wakeup_platform_en(1);
	else
		sys_hal_set_bts_wakeup_platform_en(0);

	if(!ret)
		ret = sys_amp_res_release();

}
uint32_t system_driver_get_bts_wakeup_platform_en()
{
	return sys_hal_get_bts_wakeup_platform_en();
}

void system_driver_set_bts_sleep_exit_req(bool value)
{
	if(value)
		sys_hal_set_bts_sleep_exit_req(1);
	else
		sys_hal_set_bts_sleep_exit_req(0);
}
uint32_t system_driver_get_bts_sleep_exit_req()
{
	return sys_hal_get_bts_sleep_exit_req();
}
#endif

void sys_drv_set_ana_trxt_tst_enable(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_trxt_tst_enable(value);
    sys_drv_exit_critical(int_level);
}
void sys_drv_set_ana_scal_en(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_scal_en(value);
    sys_drv_exit_critical(int_level);
}
void sys_drv_set_ana_gadc_buf_ictrl(uint32_t value)
{
   uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_gadc_buf_ictrl(value);
    sys_drv_exit_critical(int_level);
}
void sys_drv_set_ana_gadc_cmp_ictrl(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_gadc_cmp_ictrl(value);
    sys_drv_exit_critical(int_level);
}

void sys_drv_set_ana_pwd_gadc_buf(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_pwd_gadc_buf(value);
    sys_drv_exit_critical(int_level);
}

void sys_drv_set_ana_hres_sel0v9(void)
{
}

void sys_drv_set_ana_vref_sel(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_vref_sel(value);
    sys_drv_exit_critical(int_level);
}
void sys_drv_set_ana_cb_cal_manu(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_cb_cal_manu(value);
    sys_drv_exit_critical(int_level);
}

void sys_drv_set_ana_adc_div(uint32_t value)
{
}


void sys_drv_set_ana_cb_cal_trig(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_cb_cal_trig(value);
    sys_drv_exit_critical(int_level);
}

UINT32 sys_drv_get_ana_cb_cal_manu_val(void)
{
    uint32_t cal_manu_val;
    uint32_t int_level = sys_drv_enter_critical();

    cal_manu_val = sys_hal_get_ana_cb_cal_manu_val();
    sys_drv_exit_critical(int_level);
    return cal_manu_val;
}

void sys_drv_set_ana_cb_cal_manu_val(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_cb_cal_manu_val(value);
    sys_drv_exit_critical(int_level);
}

__IRAM_SEC void sys_drv_set_ana_reg11_apfms(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_reg11_apfms(value);
    sys_drv_exit_critical(int_level);
}

__IRAM_SEC void sys_drv_set_ana_reg12_dpfms(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();

    sys_hal_set_ana_reg12_dpfms(value);
    sys_drv_exit_critical(int_level);
}

void sys_drv_set_ana_vlsel_ldodig(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();
    sys_hal_set_ana_vlsel_ldodig(value);
    sys_drv_exit_critical(int_level);
}
void sys_drv_set_ana_vhsel_ldodig(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();
    sys_hal_set_ana_vhsel_ldodig(value);
    sys_drv_exit_critical(int_level);
}

void sys_drv_set_ana_vctrl_sysldo(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();
    sys_hal_set_ana_vctrl_sysldo(value);
    sys_drv_exit_critical(int_level);
}

void sys_drv_set_ana_vtempsel(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();
    sys_hal_set_ana_vtempsel(value);
    sys_drv_exit_critical(int_level);
}

void sys_drv_set_ana_ioldo_lp(uint32_t value)
{
    uint32_t int_level = sys_drv_enter_critical();
    sys_hal_set_ioldo_lp(value);
    sys_drv_exit_critical(int_level);
}

void sys_drv_early_init(void)
{
	sys_hal_early_init();
}

__IRAM_SEC void sys_drv_set_sys2flsh_2wire(uint32_t value)
{
    sys_hal_set_sys2flsh_2wire(value);
}


void sys_drv_set_cpu0_rxevt_sel(uint32 value) {
    sys_hal_set_cpu0_rxevt_sel(value);
}

void sys_drv_set_cpu1_rxevt_sel(uint32 value) {
    sys_hal_set_cpu1_rxevt_sel(value);
}

void sys_drv_set_cpu2_rxevt_sel(uint32 value) {
    sys_hal_set_cpu2_rxevt_sel(value);
}

void sys_drv_set_otp_clk_enable(uint32_t value) 
{
    sys_hal_set_cpu_device_clk_enable_otp_cken(value);
}

/** reg20 - AHB Peripheral Bus Master QoS Control **/

void sys_drv_set_psram_qos_value(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_qos_value(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_qos_value(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_qos_value();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_cpu0_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_cpu0_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_cpu0_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_cpu0_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_dma1_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_dma1_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_dma1_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_dma1_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_sdio0_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_sdio0_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_sdio0_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_sdio0_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_sdio1_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_sdio1_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_sdio1_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_sdio1_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

/* NOTE: enet0 QoS is marked as INVALID in hardware spec. Do NOT use. */
void sys_drv_set_psram_enet0_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_enet0_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_enet0_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_enet0_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

/* NOTE: enet1 QoS is marked as INVALID in hardware spec. Do NOT use. */
void sys_drv_set_psram_enet1_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_enet1_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_enet1_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_enet1_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_usb_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_usb_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_usb_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_usb_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_h26e_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_h26e_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_h26e_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_h26e_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_isp_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_isp_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_isp_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_isp_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_videopost_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_videopost_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_videopost_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_videopost_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_dpu_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_dpu_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_dpu_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_dpu_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_cbus_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_cbus_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_cbus_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_cbus_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_npu0_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_npu0_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_npu0_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_npu0_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_npu1_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_npu1_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_npu1_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_npu1_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_psram_cpu1_qos(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_psram_cpu1_qos(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_psram_cpu1_qos(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_psram_cpu1_qos();
	sys_drv_exit_critical(int_level);
	return reg;
}

/** GADC Config (ana_reg22) Start **/

void sys_drv_set_gadc_config(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_gadc_config(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_gadc_config(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_gadc_config();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_gadc_enable(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_gadc_enable(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_gadc_enable(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_gadc_enable();
	sys_drv_exit_critical(int_level);
	return reg;
}

/** GADC Config (ana_reg22) End **/

/** VAD Control (ana_reg23) Start **/

void sys_drv_set_vad_config(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_vad_config(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_vad_config(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_vad_config();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_vad_enable(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_vad_enable(v);
	sys_drv_exit_critical(int_level);
}

uint32_t sys_drv_get_vad_enable(void)
{
	uint32_t reg;
	uint32_t int_level = sys_drv_enter_critical();
	reg = sys_hal_get_vad_enable();
	sys_drv_exit_critical(int_level);
	return reg;
}

void sys_drv_set_vad_viniset(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_vad_viniset(v);
	sys_drv_exit_critical(int_level);
}

void sys_drv_set_vad_rstn(uint32_t v)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_set_vad_rstn(v);
	sys_drv_exit_critical(int_level);
}

/** VAD Control (ana_reg23) End **/
