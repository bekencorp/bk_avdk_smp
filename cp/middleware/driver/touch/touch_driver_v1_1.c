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


#include <common/bk_include.h>
#include "sys_driver.h"
#include "gpio_map.h"
#include "gpio_driver.h"
#include "touch_driver.h"
#include "aon_pmu_driver.h"
#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/timer.h>
#include <driver/touch_types.h>
#include <os/os.h>

#include <modules/pm.h>
#include <driver/touch.h>

extern void delay(int num);

typedef struct {
	touch_isr_t callback;
	void *param;
} touch_callback_t;


#define TOUCH_RETURN_ON_INVALID_ID(touch_id) do {\
		if ((touch_id) >= SOC_TOUCH_ID_NUM) {\
			return BK_ERR_TOUCH_ID;\
		}\
	} while(0)

uint32_t s_touch_channel = 0;
uint32_t s_touch_wakeup_channel = 0;

static touch_callback_t s_touch_isr[SOC_TOUCH_ID_NUM] = {NULL};

uint8_t digital_led_gpio_map[9] = {GPIO_26, GPIO_8, GPIO_9, GPIO_0, GPIO_1, GPIO_24, GPIO_45, GPIO_44, GPIO_39};

uint16_t disp_value_table[10] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90};


static uint32_t bk_touch_channel_transfer(touch_channel_t touch_id)
{
	uint32_t touch_channel = 0;
	for (touch_channel = 0; touch_channel < 16; touch_channel++)
	{
		touch_id = touch_id / 2;
		if (touch_id == 0 ) {
			break;
		}
	}
	return touch_channel;
}

bk_err_t bk_touch_gpio_init(touch_channel_t touch_id)
{
#if CONFIG_USR_GPIO_CFG_EN
	uint32_t touch_select = 0;
	touch_select = bk_touch_channel_transfer(touch_id);
	TOUCH_RETURN_ON_INVALID_ID(touch_select);
	switch(touch_id)
	{
		case BK_TOUCH_0:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH0);
			break;
		case BK_TOUCH_1:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH1);
			break;
		case BK_TOUCH_2:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH2);
			break;
		case BK_TOUCH_3:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH3);
			break;
		case BK_TOUCH_4:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH4);
			break;
		case BK_TOUCH_5:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH5);
			break;
		case BK_TOUCH_6:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH6);
			break;
		case BK_TOUCH_7:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH7);
			break;
		case BK_TOUCH_8:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH8);
			break;
		case BK_TOUCH_9:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH9);
			break;
		case BK_TOUCH_10:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH10);
			break;
		case BK_TOUCH_11:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH11);
			break;
		case BK_TOUCH_12:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH12);
			break;
		case BK_TOUCH_13:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH13);
			break;
		case BK_TOUCH_14:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH14);
			break;
		case BK_TOUCH_15:
			gpio_dev_map_by_func(GPIO_DEV_TOUCH15);
			break;
		default:
			TOUCH_LOGD("unsupported touch id\r\n");
			break;
	}
#else
	(void)touch_id;
#endif
	return BK_OK;
}

bk_err_t bk_touch_enable(touch_channel_t touch_id)
{
	uint32_t touch_select = 0;
	touch_select = bk_touch_channel_transfer(touch_id);
	TOUCH_RETURN_ON_INVALID_ID(touch_select);
	bk_int_isr_register(INT_SRC_TOUCHED, touch_isr, NULL);

	sys_drv_touch_serial_cap_enable();
	sys_drv_touch_serial_cap_sel(0);
	sys_drv_touch_power_down(0);
	sys_drv_touch_spi_unlock();

	sys_drv_touch_scan_mode_chann_set(touch_id);
	sys_drv_touch_scan_mode_chann_sel(touch_select);

#if CONFIG_CLI && CONFIG_TOUCH_TEST
        int bk_touch_register_cli_test_feature(void);
        bk_touch_register_cli_test_feature();
#endif

	return BK_OK;
}

bk_err_t bk_touch_disable(void)
{
	sys_drv_touch_power_down(1);

	return BK_OK;
}

bk_err_t bk_touch_config(const touch_config_t *touch_config)
{
	sys_drv_touch_sensitivity_level_set(touch_config->sensitivity_level);
	sys_drv_touch_detect_threshold_set(touch_config->detect_threshold);
	sys_drv_touch_detect_range_set(touch_config->detect_range);

	return BK_OK;
}

bk_err_t bk_touch_calib_enable(uint32_t enable)
{
	if(enable) {
		sys_drv_touch_calib_enable(1);
	} else {
		sys_drv_touch_calib_enable(0);
	}

	return BK_OK;
}

bk_err_t bk_touch_calibration_start(void)
{
	uint32_t timeout = 1000;

	/* Trigger calibration pulse: 0 -> 1 -> 0, as required by hardware spec. */
	bk_touch_calib_enable(0);
	delay(100);
	bk_touch_calib_enable(1);

	/* Poll cal_done_mode1 to confirm calibration is complete before reading result.
	 * A fixed delay is not reliable across different CPU frequencies. */
	while (!aon_pmu_drv_get_td_caldone_mode1() && timeout--) {
		delay(10);
	}

	/* Reset en_cal_force to 0 to complete the pulse, allowing next trigger. */
	bk_touch_calib_enable(0);

	return BK_OK;
}

bk_err_t bk_touch_scan_mode_enable(uint32_t enable)
{
	sys_drv_touch_scan_mode_enable(enable);
	return BK_OK;
}

bk_err_t bk_touch_power_down(uint32_t enable)
{
	sys_drv_touch_power_down(enable);
	return BK_OK;
}

bk_err_t bk_touch_detect_range_set(uint32_t crg)
{
	sys_drv_touch_detect_range_set(crg);
	return BK_OK;
}

bk_err_t bk_touch_gain_s_set(uint32_t value)
{
	sys_drv_touch_sensitivity_level_set(value);
	return BK_OK;
}

bk_err_t bk_touch_vrefs_set(uint32_t value)
{
	sys_drv_touch_detect_threshold_set(value);
	return BK_OK;
}

bk_err_t bk_touch_cal_vth_set(uint32_t value)
{
	sys_drv_touch_cal_vth_set(value);
	return BK_OK;
}

bk_err_t bk_touch_cal_auto_set(uint32_t enable)
{
	sys_drv_touch_cal_auto_set(enable);
	return BK_OK;
}

bk_err_t bk_touch_cal_done_clr(uint32_t value)
{
	sys_drv_touch_cal_done_clr(value);
	return BK_OK;
}

bk_err_t bk_touch_rstb_dig_set(uint32_t value)
{
	sys_drv_touch_rstb_dig_set(value);
	return BK_OK;
}

uint32_t bk_touch_rstb_dig_get(void)
{
	return sys_drv_touch_rstb_dig_get();
}

bk_err_t bk_touch_ldoen_set(uint32_t value)
{
	sys_drv_touch_ldoen_set(value);
	return BK_OK;
}

uint32_t bk_touch_ldoen_get(void)
{
	return sys_drv_touch_ldoen_get();
}

bk_err_t bk_touch_manul_mode_enable(uint32_t calib_value)
{
	sys_drv_touch_manul_mode_calib_value_set(calib_value);
	sys_drv_touch_manul_mode_enable(1);
	return BK_OK;
}

bk_err_t bk_touch_manul_mode_disable(void)
{
	sys_drv_touch_manul_mode_enable(0);
	return BK_OK;
}

bk_err_t bk_touch_scan_mode_multi_channl_set(touch_channel_t touch_id)
{
	sys_drv_touch_scan_mode_chann_set(touch_id);
	return BK_OK;
}

bk_err_t bk_touch_mode_select_set(uint32_t mode)
{
	sys_drv_touch_modsel_spi_set(mode);
	return BK_OK;
}

uint32_t bk_touch_mode_select_get(void)
{
	return sys_drv_touch_modsel_spi_get();
}

bk_err_t bk_touch_int_enable(touch_channel_t touch_id, uint32_t enable)
{
	if(enable) {
		sys_drv_touch_int_enable(1);
		sys_drv_touch_int_set(touch_id);
	} else {
		sys_drv_touch_int_set(0);
		sys_drv_touch_int_enable(0);
	}

	return BK_OK;
}

bk_err_t bk_touch_get_int_status(void)
{
	uint32_t  int_status = 0;
	int_status = aon_pmu_drv_get_touch_int_status();

	return int_status;
}

bk_err_t bk_touch_clear_int(touch_channel_t touch_id)
{
	sys_drv_touch_int_clear(touch_id);

	return BK_OK;
}

uint32_t bk_touch_get_calib_value(void)
{
	uint32_t calib_value = 0;
	calib_value = aon_pmu_drv_get_cap_cal();

	return calib_value;
}

uint32_t bk_touch_get_touch_status(void)
{
	uint32_t touch_status = 0;
	touch_status = aon_pmu_drv_get_touch_state();

	return touch_status;
}

bk_err_t bk_touch_set_test_mode(uint8_t period, uint8_t number)
{
	sys_drv_touch_test_period_set(period);
	sys_drv_touch_test_number_set(number);

	return BK_OK;
}

bk_err_t bk_touch_set_calib_mode(uint8_t period, uint8_t number)
{
	sys_drv_touch_calib_period(period);
	sys_drv_touch_calib_number(number);

	return BK_OK;
}

bk_err_t bk_touch_digital_tube_init(void)
{
	uint8_t i = 0;
	for (i = 0; i < 9; i++) {
		bk_gpio_enable_output(digital_led_gpio_map[i]);
		bk_gpio_set_output_high(digital_led_gpio_map[i]);
	}

	return BK_OK;
}

void bk_touch_digital_tube_display(uint8_t disp_value)
{
	if (0 <= disp_value && disp_value <= 9) {
		bk_gpio_set_output_high(digital_led_gpio_map[8]);
		bk_gpio_set_output_low(digital_led_gpio_map[7]);

		for (uint8_t j = 0; j < 7; j++) {
			if (((disp_value_table[0] >> j) & 0x01) == 0) {
				bk_gpio_set_output_low(digital_led_gpio_map[j]);
			} else {
				bk_gpio_set_output_high(digital_led_gpio_map[j]);
			}
		}
		rtos_delay_milliseconds(10);

		bk_gpio_set_output_high(digital_led_gpio_map[7]);
		bk_gpio_set_output_low(digital_led_gpio_map[8]);
		for (uint8_t k = 0; k < 7; k++) {
			if (((disp_value_table[disp_value] >> k) & 0x01) == 0) {
				bk_gpio_set_output_low(digital_led_gpio_map[k]);
			} else {
				bk_gpio_set_output_high(digital_led_gpio_map[k]);
			}
		}
		rtos_delay_milliseconds(10);

	} else if (10 <= disp_value && disp_value < 16) {
		bk_gpio_set_output_high(digital_led_gpio_map[8]);
		bk_gpio_set_output_low(digital_led_gpio_map[7]);

		for (uint8_t i = 0; i < 7; i++) {
			if (((disp_value_table[1] >> i) & 0x01) == 0) {
				bk_gpio_set_output_low(digital_led_gpio_map[i]);
			} else {
				bk_gpio_set_output_high(digital_led_gpio_map[i]);
			}
		}
		rtos_delay_milliseconds(10);

		bk_gpio_set_output_high(digital_led_gpio_map[7]);
		bk_gpio_set_output_low(digital_led_gpio_map[8]);

		for (uint8_t j = 0; j < 7; j++) {
			if (((disp_value_table[disp_value % 10] >> j) & 0x01) == 0) {
				bk_gpio_set_output_low(digital_led_gpio_map[j]);
			} else {
				bk_gpio_set_output_high(digital_led_gpio_map[j]);
			}
		}
		rtos_delay_milliseconds(10);

	} else {
		TOUCH_LOGD("Invalid touch channel!\r\n");
		return;
	}

}

bk_err_t bk_touch_register_touch_isr(touch_channel_t touch_id, touch_isr_t isr, void *param)
{
	uint32_t touch_channel = 0;
	touch_channel = bk_touch_channel_transfer(touch_id);
	TOUCH_RETURN_ON_INVALID_ID(touch_channel);
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	s_touch_isr[touch_channel].callback = isr;
	s_touch_isr[touch_channel].param = param;
	GLOBAL_INT_RESTORE();

	return BK_OK;
}

static void touch_timer_isr(timer_id_t chan)
{
	uint32_t touch_status = 0;
	touch_status = bk_touch_get_touch_status();
	if (!(touch_status & (1 << s_touch_channel))) {
		bk_touch_clear_int(0);
		bk_touch_int_enable(1 << s_touch_channel, 1);
		bk_timer_stop(chan);
	}
}

void __BK_IRQ touch_isr(void)
{
	int ret = 0;
	uint32_t int_status = 0;
	uint32_t touch_id = 0;
	int_status = bk_touch_get_int_status();

	for (touch_id = 0; touch_id < SOC_TOUCH_ID_NUM; touch_id++)
	{
		if (int_status & (1 << touch_id)) {
			TOUCH_LOGD("Touch[%d] has been selected!\r\n", touch_id);
			s_touch_channel = touch_id;
			bk_touch_clear_int(1 << touch_id);
			bk_touch_int_enable(1 << touch_id, 0);
			if (s_touch_isr[touch_id].callback) {
				s_touch_isr[touch_id].callback(s_touch_isr[touch_id].param);
			}

			ret = bk_timer_start(TIMER_ID1, 200, touch_timer_isr);
			if (ret != BK_OK) {
				BK_LOGD(NULL, "Timer start failed\r\n");
			}
			break;
		}
	}
}

#if (CONFIG_TOUCH_PM_SUPPORT)
void bk_touch_wakeup_channel_set(touch_channel_t channel)
{
	s_touch_wakeup_channel = channel;
}

uint32_t bk_touch_wakeup_channel_get(void)
{
	return s_touch_wakeup_channel;
}

static int touch_pm_enter_cb(uint64_t sleep_time_ms, void *args)
{
	uint32_t cap_out = 0;

	/* Touch hardware init */
	bk_touch_gpio_init(s_touch_wakeup_channel);
	bk_touch_scan_mode_enable(0);
	bk_touch_enable(s_touch_wakeup_channel);

	/* Power on touch analog */
	bk_touch_power_down(0);

	if (bk_touch_rstb_dig_get()) {
		bk_touch_rstb_dig_set(0);
	}
	if (bk_touch_ldoen_get()) {
		bk_touch_ldoen_set(0);
	}

	/* Configure touch sensitivity parameters */
	bk_touch_gain_s_set(8);
	bk_touch_vrefs_set(8);
	bk_touch_cal_vth_set(4);

	bk_touch_cal_auto_set(0);
	bk_touch_cal_done_clr(1);
	bk_touch_set_test_mode(4, 4);
	bk_touch_set_calib_mode(0, 2);
	bk_touch_mode_select_set(0);

	/* Single calibration with default detect range (19PF) */
	bk_touch_detect_range_set(TOUCH_DETECT_RANGE_19PF);
	bk_touch_calibration_start();
	cap_out = aon_pmu_drv_get_cap_cal();
	TOUCH_LOGD("touch pm calib: cap_out=0x%x\r\n", cap_out);
	if (cap_out >= 0x1F0) {
		TOUCH_LOGE("cap_out out of range, channel %d may not work\r\n",
			   s_touch_wakeup_channel);
	}

	bk_touch_int_enable(s_touch_wakeup_channel, 1);

	return BK_OK;
}

bk_err_t bk_touch_pm_init(void)
{
	bk_err_t ret = BK_OK;
	touch_wakeup_param_t touch_wakeup_param = {0};

	/* Set touch as wakeup source */
	touch_wakeup_param.touch_channel = s_touch_wakeup_channel;
	bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_TOUCHED, &touch_wakeup_param);

	/* Register PM callbacks for deepsleep and low-voltage */
	pm_cb_conf_t enter_config_touch = {touch_pm_enter_cb, NULL};
	ret = bk_pm_sleep_register_cb(PM_MODE_DEEP_SLEEP, PM_DEV_ID_TOUCH, &enter_config_touch, NULL);
	if (ret != BK_OK) {
		TOUCH_LOGE("register touch pm deep sleep cb fail!\r\n");
	}

	ret = bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_TOUCH, &enter_config_touch, NULL);
	if (ret != BK_OK) {
		TOUCH_LOGE("register touch pm low voltage cb fail!\r\n");
	}

	return ret;
}
#endif



