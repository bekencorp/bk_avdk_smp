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

#include <common/bk_include.h>
#include <driver/touch_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief TOUCH API
 * @defgroup touch.h TOUCH API group
 * @{
 */

/**
 * @brief     Init the touch gpio pin
 *
 * This API init the GPIO pin corresponding to touch channel:
 *   - Unmap gpio pin function
 *   - Change gpio pin function to touch
 *   - Configure gpio to be high impedance
 *
 * This API should be called before any other TOUCH APIs.
 *
 * @param
 *    - touch_id: touch channel, channel 0 ~ channel 15
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_touch_gpio_init(touch_channel_t touch_id);

/**
 * @brief     enable touch function
 *
 * This API enable the touch function:
 *   - Select the touch channel
 *   - Power on the touch module
 *
 *
 * @param
 *    - touch_id: touch channel, channel 0 ~ channel 15
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_touch_enable(touch_channel_t touch_id);

/**
 * @brief     disable touch function
 *
 * This API disable the touch function:
 *   - Power down the touch module
 *
 *
 * @param
 *    - None
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_disable(void);

/**
 * @brief     config touch function
 *
 * This API config the touch function:
 *   - Set touch sensitivity level, the smaller the value, the more sensitive. The range of level value from 0 to 3
 *   - Set touch detect threshold, the range of value from 0 to 7.
 *   - Set touch detect range, 8pF/12pF/19pF/27pF
 *
 *
 * @param
 *    - touch_config: the config of touch work mode
 *
 * @return
 *    - BK_OK: succeed
 *
 *
 * Usage example:
 *
 *		touch_config_t touch_config;
 *		touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_3;
 *		touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
 *		touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
 *		bk_touch_config(&touch_config);
 */
bk_err_t bk_touch_config(const touch_config_t *touch_config);

/**
 * @brief     start calibrating touch channel
 *
 * This API start calibrating the touch channel that selected.
 *
 *
 * @param
 *    - None
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_calibration_start(void);

/**
 * @brief     enable/disable scan mode of touch channel
 *
 * This API enable or disable the scan mode of touch channel.
 *
 *
 * @param
 *    - enable: enable —— 1; disable —— 0;
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_scan_mode_enable(uint32_t enable);

/**
 * @brief     set touch calibration interface mode (SPI or digital)
 *
 * This API selects the calibration readback interface mode via ana_reg34.modsel_spi.
 * Set to 0 for digital mode (default); set to 1 for SPI mode.
 *
 * @param
 *    - mode: 0 — digital mode; 1 — SPI mode
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_mode_select_set(uint32_t mode);

/**
 * @brief     set touch detection test period and test number
 *
 * This API configures the touch detection test parameters via ana_reg33.
 * test_period controls the detection window length; test_number controls
 * how many consecutive detections are required to confirm a touch event.
 *
 * @param
 *    - period: test period value (ana_reg33.test_period, bit[7:4])
 *    - number: test number value (ana_reg33.test_number, bit[3:0])
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_set_test_mode(uint8_t period, uint8_t number);

/**
 * @brief     set touch calibration charge length period and calibration number
 *
 * This API configures the calibration timing parameters via ana_reg34.
 * cl_period sets the charge length period; cal_number sets the number of
 * calibration cycles averaged to produce the final calibration result.
 *
 * @param
 *    - period: charge length period (ana_reg34.cl_period, bit[31:23])
 *    - number: calibration cycle count (ana_reg34.cal_number, bit[22:19])
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_set_calib_mode(uint8_t period, uint8_t number);

/**
 * @brief     power down or power on the touch analog front-end
 *
 * This API controls the pwd_td bit (sys.reg60 bit31) to power on or power
 * off the touch analog circuit. Must be set to 0 (power on) before
 * initiating any calibration or detection operation.
 *
 * @param
 *    - enable: 1 — power down (disable touch analog); 0 — power on
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_power_down(uint32_t enable);

/**
 * @brief     set the touch sensor capacitance detect range (crg)
 *
 * This API sets the detect range field crg (sys.reg60 bit[21:20]) which
 * selects the reference capacitor range for calibration:
 *   0 — 8 pF, 1 — 12 pF, 2 — 19 pF, 3 — 27 pF.
 * During multi-range calibration, crg is incremented until cap_out < 0x1F0.
 *
 * @param
 *    - crg: detect range index (0–3); see touch_detect_range_t for named values
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_detect_range_set(uint32_t crg);

/**
 * @brief     set the touch sensor analog gain (gain_s)
 *
 * This API sets the gain_s field (sys.reg60 bit[29:26]) which controls
 * the amplifier gain of the touch analog front-end.
 * Recommended optimal value: 1.
 *
 * @param
 *    - value: gain value (0–15)
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_gain_s_set(uint32_t value);

/**
 * @brief     set the touch sensor reference voltage (vrefs)
 *
 * This API sets the vrefs field (sys.reg60 bit[25:22]) which selects
 * the internal reference voltage level used for touch detection comparison.
 * Recommended optimal value: 2.
 *
 * @param
 *    - value: reference voltage selection (0–15)
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_vrefs_set(uint32_t value);

/**
 * @brief     set the touch calibration voltage threshold (cal_vth)
 *
 * This API sets the cal_vth field (sys.reg60 bit[19:17]) which controls
 * the voltage threshold used during hardware auto-calibration.
 * Recommended optimal value: 2.
 *
 * @param
 *    - value: calibration voltage threshold (0–7)
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_cal_vth_set(uint32_t value);

/**
 * @brief     enable or disable touch automatic calibration mode
 *
 * This API controls the en_cal_auto bit (sys.reg61 bit30). When enabled,
 * the hardware triggers calibration automatically at a fixed interval.
 * Must be set to 0 before using force calibration (en_cal_force).
 *
 * @param
 *    - enable: 1 — enable auto calibration; 0 — disable
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_cal_auto_set(uint32_t enable);

/**
 * @brief     clear the touch calibration done flag
 *
 * This API writes to cal_done_clr (sys.reg61 bit28) to reset the hardware
 * calibration completion flag before starting a new calibration sequence.
 * Should be set to 1 prior to triggering en_cal_force.
 *
 * @param
 *    - value: write 1 to clear the calibration done status
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_cal_done_clr(uint32_t value);

/**
 * @brief     set the touch digital reset signal (rstb_dig)
 *
 * This API controls the rstb_dig bit (sys.reg60 bit8) which holds the
 * touch digital logic in reset when set to 1. Must be cleared to 0
 * before initiating calibration or detection.
 *
 * @param
 *    - value: 1 — assert reset; 0 — release reset
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_rstb_dig_set(uint32_t value);

/**
 * @brief     get the current value of the touch digital reset signal (rstb_dig)
 *
 * This API reads the rstb_dig bit (sys.reg60 bit8).
 *
 * @param
 *    - None
 *
 * @return
 *    - 0: reset not asserted (normal operation)
 *    - 1: reset asserted
 */
uint32_t bk_touch_rstb_dig_get(void);

/**
 * @brief     set the touch analog LDO enable signal (ldoen)
 *
 * This API controls the ldoen bit (sys.reg60 bit5) which enables the
 * internal LDO powering the touch analog front-end. Must be cleared to 0
 * before calibration to ensure the LDO is in the correct initial state.
 *
 * @param
 *    - value: 1 — enable LDO; 0 — disable LDO
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_ldoen_set(uint32_t value);

/**
 * @brief     get the current value of the touch analog LDO enable signal (ldoen)
 *
 * This API reads the ldoen bit (sys.reg60 bit5).
 *
 * @param
 *    - None
 *
 * @return
 *    - 0: LDO disabled
 *    - 1: LDO enabled
 */
uint32_t bk_touch_ldoen_get(void);

/**
 * @brief     enable/disable adc mode of touch channel
 *
 * This API enable or disable the adc mode of touch channel.
 *
 *
 * @param
 *    - enable: enable —— 1; disable —— 0;
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_adc_mode_enable(uint32_t enable);

/**
 * @brief     enable manul calibration mode of touch channel
 *
 * This API enable the manul calibration mode of touch channel:
 *    - set the calibration value of manul mode;
 *    - enable the manul calibretion mode;
 *
 *
 * @param
 *    - calib_value: calibration value of manul mode, the max value is 0x1FF;
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_manul_mode_enable(uint32_t calib_value);

/**
 * @brief     disable manul calibration mode of touch channel
 *
 * This API disable the manul calibration mode of touch channel
 *
 * @param
 *    - None
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_manul_mode_disable(void);

/**
 * @brief     set multi channels that need to scan
 *
 * This API set multi channels that need to scan
 *
 *
 * @param
 *    - touch_id: the multi channels that need to scan. See the enum of touch_channel_t.
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_scan_mode_multi_channl_set(touch_channel_t touch_id);

/**
 * @brief     enable/disable touch channel interrupt
 *
 * This API enable or disable the touch channel interrupt:
 *    - register the interrupt service handle;
 *    - enable or disable the touch interrupt;
 *
 *
 * @param
 *    - touch_id: touch channel, channel 0 ~ channel 15;
 *    - enable: enable —— 1; disable —— 0;
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_int_enable(touch_channel_t touch_id, uint32_t enable);

/**
 * @brief     get the calibration value of touch channel
 *
 * This API get the calibration value of touch channel.
 *
 *
 * @param
 *    - None
 *
 * @return
 *    - calib_value: the max value is 0x1FF.
 */
uint32_t bk_touch_get_calib_value(void);

/**
 * @brief     get the status of touch channel
 *
 * This API get the status of touch channel:
 *
 *
 * @param
 *    - None
 *
 * @return
 *    - touch_status: the touch status of every channel. One bit corresponding to one channel. 1 —— the channel is touched; 0 —— the channel is idle.
 */
uint32_t bk_touch_get_touch_status(void);

/**
 * @brief     init the digital tube of touch
 *
 * This API init the digital tube:
 *
 *
 * @param
 *    - None
 *
 * @return
 *    - BK_OK: succeed
 */
bk_err_t bk_touch_digital_tube_init(void);

/**
 * @brief     display the value in the digital tube
 *
 * This API display the value in the digital tube
 *
 *
 * @param
 *    - disp_value: the value need to display;
 *
 *
 * @return
 *    - None
 */
void bk_touch_digital_tube_display(uint8_t disp_value);

/**
 * @brief     get the touch channel for low voltage and deepsleep wake up
 *
 * This API get the touch channel for low voltage and deepsleep wake up
 *
 *
 * @param
 *    - channel: touch channel, channel 0 ~ channel 15. See the enum of touch_channel_t.
 *
 *
 * @return
 *    - None
 */
void bk_touch_wakeup_channel_set(touch_channel_t channel);

/**
 * @brief     set the touch channel for low voltage and deepsleep wake up
 *
 * This API set the touch channel for low voltage and deepsleep wake up
 *
 *
 * @param
 *    - None
 *
 *
 * @return
 *    - s_touch_wakeup_channel: the channel number of touch
 */
uint32_t bk_touch_wakeup_channel_get(void);

/**
 * @brief     Register touch low voltage and deepsleep config
 *
 * This API register touch low voltage and deepsleep config
 *
 *
 * @param
 *    - None
 *
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_touch_pm_init(void);

/**
 * @brief     Register touch isr
 *
 * This API register touch isr
 *
 *
 * @param
 *    - touch_id: touch channel, channel 0 ~ channel 15;
 *    - isr: touch isr callback;
 *    - param: touch isr callback parameter;
 *
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_touch_register_touch_isr(touch_channel_t touch_id, touch_isr_t isr, void *param);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif


