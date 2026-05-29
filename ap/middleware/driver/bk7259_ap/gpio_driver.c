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
#include <driver/gpio.h>
#include "gpio_driver.h"
#include "gpio_driver_base.h"

#define GPIO_REG_DEFAULT_VALUE      0x0
#define GPIO_RETURN_ON_INVALID_PERIAL_MODE(mode, mode_max) do {\
	if ((mode) >= (mode_max)) {\
		return BK_ERR_GPIO_SET_INVALID_FUNC_MODE;\
	}\
} while(0)

static const IOMX_CODE_T  s_gpio_dev_map[] = GPIO_DEV_TO_IOMX_CODE_MAP;
static const struct mapping_func_code_fix_gpio s_fix_gpio_map[] = MAP_FUNC_CODE_FIX_GPIO;

bk_err_t gpio_dev_map(gpio_id_t gpio_id, gpio_dev_t dev)
{
	// Temp use for peri dev, remove later!
	gpio_dev_unprotect_map(gpio_id, dev);

	return BK_OK;
}

bk_err_t gpio_dev_unmap(gpio_id_t gpio_id)
{
	// Temp use for peri dev, remove later!
	gpio_dev_unprotect_unmap(gpio_id);

	return BK_OK;
}

// Check if gpio_dev_t is in MAP_FUNC_CODE_FIX_GPIO and verify if the given GPIO ID is allowed
// Returns: true if found in fix GPIO map and gpio_id matches one of the allowed GPIOs, false otherwise
// If found and gpio_id matches, *func_code is set
static bool get_fix_gpio_mapping(gpio_dev_t dev, gpio_id_t gpio_id, IOMX_CODE_T *func_code)
{
	for (int i = 0; i < ARRAY_SIZE(s_fix_gpio_map); i++) {
		if (s_fix_gpio_map[i].gdev == dev && s_fix_gpio_map[i].id == gpio_id) {
			// Found matching dev and GPIO ID
			if (func_code) {
				*func_code = s_fix_gpio_map[i].code;
			}
			return true;
		}
	}
	return false;
}

/* Here doesn't check the GPIO id is whether used by another CPU-CORE, but checked current CPU-CORE */
bk_err_t gpio_dev_unprotect_map(gpio_id_t gpio_id, gpio_dev_t dev)
{
	GPIO_LOGD("%s:id=%d, dev=%d\r\n", __func__, gpio_id, dev);

	IOMX_CODE_T func_code = FUNC_CODE_INVALID;

	// Step 1: Fast check - GPIO_DEV_TO_IOMX_CODE_MAP (O(1) array access, flexible mux)
	// This is optimized for high-frequency flexible map devices (UART, SPI, I2C, PWM, etc.)
	// Note: Flexible map and fixed map are mutually exclusive, so no need to check fixed map
	//       if device is found in flexible map
	func_code = convert_gpio_dev_to_iomx_code(dev);

	if (func_code != FUNC_CODE_INVALID) {
		// Found in flexible mux map, all GPIOs can be used for this dev
		// No need to check fixed GPIO map (they are mutually exclusive)
	} else {
		// Step 2: Device is not in flexible map, check fixed GPIO map (O(n))
		if (get_fix_gpio_mapping(dev, gpio_id, &func_code)) {
			// GPIO ID matches one of the allowed GPIOs, func_code is already set
		} else {
			GPIO_LOGE("GPIO device %d is not supported (not in GPIO_DEV_TO_IOMX_CODE_MAP or MAP_FUNC_CODE_FIX_GPIO)\r\n", dev);
			return BK_ERR_GPIO_SET_INVALID_FUNC_MODE;
		}
	}

	bk_gpio_set_value(gpio_id, GPIO_REG_DEFAULT_VALUE);

	return bk_gpio_set_gpio_func(gpio_id, func_code);
}

/* Here doesn't check the GPIO id is whether used by another CPU-CORE */
bk_err_t gpio_dev_unprotect_unmap(gpio_id_t gpio_id)
{
	bk_gpio_set_value(gpio_id, GPIO_REG_DEFAULT_VALUE);

	return BK_OK;
}

bk_err_t gpio_jtag_sel(gpio_jtag_map_group_t group_id)
{
	bk_err_t ret = BK_OK;
	gpio_dev_unprotect_unmap(GPIO_20);
	gpio_dev_unprotect_unmap(GPIO_21);

	#if CONFIG_SPE
	gpio_dev_unprotect_unmap(GPIO_0);
	gpio_dev_unprotect_unmap(GPIO_1);
	#endif

	if (group_id == GPIO_JTAG_MAP_GROUP0) {
		ret = gpio_dev_unprotect_map(GPIO_20, GPIO_DEV_JTAG_TCK);
		ret = gpio_dev_unprotect_map(GPIO_21, GPIO_DEV_JTAG_TMS);
	} else if (group_id == GPIO_JTAG_MAP_GROUP1) {
		ret = gpio_dev_unprotect_map(GPIO_0, GPIO_DEV_JTAG_TCK);
		ret = gpio_dev_unprotect_map(GPIO_1, GPIO_DEV_JTAG_TMS);
	} else {
		// IOMX_LOGD("Unsupported group id(%d).\r\n", group_id);
		return BK_FAIL;
	}

	return ret;
}

bk_err_t gpio_scr_sel(gpio_scr_map_group_t mode)
{
	return BK_OK;
}

IOMX_CODE_T convert_gpio_dev_to_iomx_code(gpio_dev_t dev)
{
	// Handle special cases first
	if (dev == GPIO_DEV_INVALID || dev == GPIO_DEV_NONE) {
		if (dev == GPIO_DEV_NONE) {
			return FUNC_CODE_HIGH_Z;  // GPIO_DEV_NONE maps to FUNC_CODE_HIGH_Z
		}
		return FUNC_CODE_INVALID;
	}

	if (dev >= ARRAY_SIZE(s_gpio_dev_map)) {
		return FUNC_CODE_INVALID;
	}

	IOMX_CODE_T func_code = s_gpio_dev_map[dev];

	// If the device is not in the map, the array element will be 0 (FUNC_CODE_HIGH_Z)
	// Since we already handled GPIO_DEV_NONE above, if func_code is 0 here,
	// it means the device is not mapped in GPIO_DEV_TO_IOMX_CODE_MAP
	if (func_code == FUNC_CODE_HIGH_Z) {
		return FUNC_CODE_INVALID;
	}

	return func_code;
}
