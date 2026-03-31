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

/**  Touch Start **/
uint32_t sys_drv_touch_power_down(uint32_t enable)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_power_down(enable);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_sensitivity_level_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_sensitivity_level_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_scan_mode_enable(uint32_t enable)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_scan_mode_enable(enable);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_detect_threshold_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_detect_threshold_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_detect_range_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_detect_range_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_calib_enable(uint32_t enable)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_calib_enable(enable);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_cal_ctrl_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_touch_cal_ctrl_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_cal_ctrl_get(void)
{
	uint32_t int_level = sys_drv_enter_critical();
	uint32_t ret = sys_hal_touch_cal_ctrl_get();
	sys_drv_exit_critical(int_level);
	return ret;
}

uint32_t sys_drv_touch_cal_vth_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_touch_cal_vth_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_cal_vth_get(void)
{
	uint32_t int_level = sys_drv_enter_critical();
	uint32_t ret = sys_hal_touch_cal_vth_get();
	sys_drv_exit_critical(int_level);
	return ret;
}

uint32_t sys_drv_touch_rstb_dig_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_touch_rstb_dig_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_rstb_dig_get(void)
{
	uint32_t int_level = sys_drv_enter_critical();
	uint32_t ret = sys_hal_touch_rstb_dig_get();
	sys_drv_exit_critical(int_level);
	return ret;
}

uint32_t sys_drv_touch_ldoen_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_touch_ldoen_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_ldoen_get(void)
{
	uint32_t int_level = sys_drv_enter_critical();
	uint32_t ret = sys_hal_touch_ldoen_get();
	sys_drv_exit_critical(int_level);
	return ret;
}

uint32_t sys_drv_touch_cal_auto_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_touch_cal_auto_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_cal_done_clr(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();
	sys_hal_touch_cal_done_clr(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_manul_mode_calib_value_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_manul_mode_calib_value_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_calib_value_get(void)
{
	uint32_t int_level = sys_drv_enter_critical();
	uint32_t ret = sys_hal_touch_calib_value_get();
	sys_drv_exit_critical(int_level);
	return ret;
}

uint32_t sys_drv_touch_manul_mode_enable(uint32_t enable)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_manul_mode_enable(enable);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}


uint32_t sys_drv_touch_scan_mode_chann_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_scan_mode_chann_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_scan_mode_chann_sel(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_scan_mode_chann_sel(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_serial_cap_enable(void)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_serial_cap_enable();
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_serial_cap_disable(void)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_serial_cap_disable();
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_serial_cap_sel(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_serial_cap_sel(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_spi_lock(void)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_spi_lock();
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_spi_unlock(void)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_spi_unlock();
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_test_period_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_test_period_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_test_number_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_test_number_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_calib_period(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_calib_period_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_calib_period_get(void)
{
	uint32_t int_level = sys_drv_enter_critical();
	uint32_t ret = sys_hal_touch_calib_period_get();

	sys_drv_exit_critical(int_level);
	return ret;
}

uint32_t sys_drv_touch_calib_number(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_calib_number_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_calib_number_get(void)
{
	uint32_t int_level = sys_drv_enter_critical();
	uint32_t ret = sys_hal_touch_calib_number_get();

	sys_drv_exit_critical(int_level);
	return ret;
}

uint32_t sys_drv_touch_modsel_spi_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_modsel_spi_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_modsel_spi_get(void)
{
	uint32_t int_level = sys_drv_enter_critical();
	uint32_t ret = sys_hal_touch_modsel_spi_get();

	sys_drv_exit_critical(int_level);
	return ret;
}

uint32_t sys_drv_touch_int_set(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_int_set(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_int_clear(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_int_clear(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

uint32_t sys_drv_touch_int_enable(uint32_t value)
{
	uint32_t int_level = sys_drv_enter_critical();

	sys_hal_touch_int_enable(value);
	sys_drv_exit_critical(int_level);
	return SYS_DRV_SUCCESS;
}

/**  Touch End **/

