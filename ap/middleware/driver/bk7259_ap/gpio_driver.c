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
	// Handle special cases first. GPIO_DEV_NONE maps to FUNC_CODE_HIGH_Z.
	// It must be handled here because FUNC_CODE_HIGH_Z(0) is also used below
	// as the "unmapped" sentinel.
	if (dev == GPIO_DEV_NONE) {
		return FUNC_CODE_HIGH_Z;
	}
	if (dev == GPIO_DEV_INVALID) {
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

/* ===========================================================================
 * GPIO function-name resolver (consumed by gpio_dump, see gpio_driver_base.c)
 *
 * BK7259 v2px encodes the whole pad function in the 8-bit gpio_fun_sel field:
 *   - flexible IO-matrix codes 0~104 : the code alone identifies the function
 *   - fixed-map group codes 126~134  : the same code means different peripherals
 *     on different pins, so (gpio_id, code) must be resolved via
 *     MAP_FUNC_CODE_FIX_GPIO to recover the concrete device.
 * =========================================================================== */

/* Flexible IO-matrix code -> name. Index == FUNC_CODE_xxx (0~104). */
static const char *const s_funcode_name[] = {
	[FUNC_CODE_HIGH_Z]       = "HIGH_Z",
	[FUNC_CODE_INPUT]        = "GPIO_IN",
	[FUNC_CODE_OUTPUT]       = "GPIO_OUT",
	[FUNC_CODE_INPUT_OUTPUT] = "GPIO_IO",
	[FUNC_CODE_DMIC0_CLK]    = "DMIC0_CLK",
	[FUNC_CODE_DMIC0_DAT]    = "DMIC0_DAT",
	[FUNC_CODE_DMIC1_CLK]    = "DMIC1_CLK",
	[FUNC_CODE_DMIC1_DAT]    = "DMIC1_DAT",
	[FUNC_CODE_HDMI_CEC]     = "HDMI_CEC",
	[FUNC_CODE_I2S0_DIN]     = "I2S0_DIN",
	[FUNC_CODE_I2S0_DOUT]    = "I2S0_DOUT",
	[FUNC_CODE_I2S0_SCK]     = "I2S0_SCK",
	[FUNC_CODE_I2S0_SYNC]    = "I2S0_SYNC",
	[FUNC_CODE_I2S1_DIN]     = "I2S1_DIN",
	[FUNC_CODE_I2S1_DOUT]    = "I2S1_DOUT",
	[FUNC_CODE_I2S1_SCK]     = "I2S1_SCK",
	[FUNC_CODE_I2S1_SYNC]    = "I2S1_SYNC",
	[FUNC_CODE_I2S2_DIN]     = "I2S2_DIN",
	[FUNC_CODE_I2S2_DOUT]    = "I2S2_DOUT",
	[FUNC_CODE_I2S2_SCK]     = "I2S2_SCK",
	[FUNC_CODE_I2S2_SYNC]    = "I2S2_SYNC",
	[FUNC_CODE_I2S3_DIN]     = "I2S3_DIN",
	[FUNC_CODE_I2S3_DOUT]    = "I2S3_DOUT",
	[FUNC_CODE_I2S3_SCK]     = "I2S3_SCK",
	[FUNC_CODE_I2S3_SYNC]    = "I2S3_SYNC",
	[FUNC_CODE_I2S4_DIN]     = "I2S4_DIN",
	[FUNC_CODE_I2S4_DOUT]    = "I2S4_DOUT",
	[FUNC_CODE_I2S4_SCK]     = "I2S4_SCK",
	[FUNC_CODE_I2S4_SYNC]    = "I2S4_SYNC",
	[FUNC_CODE_I2S_MCLK]     = "I2S_MCLK",
	[FUNC_CODE_SWCLK]        = "SWCLK",
	[FUNC_CODE_SWDIO]        = "SWDIO",
	[FUNC_CODE_PWM0_0]       = "PWM0",
	[FUNC_CODE_PWM0_1]       = "PWM1",
	[FUNC_CODE_PWM0_2]       = "PWM2",
	[FUNC_CODE_PWM0_3]       = "PWM3",
	[FUNC_CODE_PWM0_4]       = "PWM4",
	[FUNC_CODE_PWM0_5]       = "PWM5",
	[FUNC_CODE_PWM0_6]       = "PWM6",
	[FUNC_CODE_PWM0_7]       = "PWM7",
	[FUNC_CODE_PWM0_8]       = "PWM8",
	[FUNC_CODE_PWM0_9]       = "PWM9",
	[FUNC_CODE_PWM0_10]      = "PWM10",
	[FUNC_CODE_PWM0_11]      = "PWM11",
	[FUNC_CODE_SPI0_MISO]    = "SPI0_MISO",
	[FUNC_CODE_SPI0_MOSI]    = "SPI0_MOSI",
	[FUNC_CODE_SPI0_NSS]     = "SPI0_CSN",
	[FUNC_CODE_SPI0_SCK]     = "SPI0_SCK",
	[FUNC_CODE_SPI1_MISO]    = "SPI1_MISO",
	[FUNC_CODE_SPI1_MOSI]    = "SPI1_MOSI",
	[FUNC_CODE_SPI1_NSS]     = "SPI1_CSN",
	[FUNC_CODE_SPI1_SCK]     = "SPI1_SCK",
	[FUNC_CODE_SPI2_MISO]    = "SPI2_MISO",
	[FUNC_CODE_SPI2_MOSI]    = "SPI2_MOSI",
	[FUNC_CODE_SPI2_NSS]     = "SPI2_CSN",
	[FUNC_CODE_SPI2_SCK]     = "SPI2_SCK",
	[FUNC_CODE_SPI3_MISO]    = "SPI3_MISO",
	[FUNC_CODE_SPI3_MOSI]    = "SPI3_MOSI",
	[FUNC_CODE_SPI3_NSS]     = "SPI3_CSN",
	[FUNC_CODE_SPI3_SCK]     = "SPI3_SCK",
	[FUNC_CODE_I3C_SCL]      = "I3C_SCL",
	[FUNC_CODE_I3C_SDA]      = "I3C_SDA",
	[FUNC_CODE_I3C_SDA_PURN] = "I3C_SDA_PURN",
	[FUNC_CODE_BT_ANT_0]     = "BT_ANT0",
	[FUNC_CODE_BT_ANT_1]     = "BT_ANT1",
	[FUNC_CODE_BT_ANT_2]     = "BT_ANT2",
	[FUNC_CODE_BT_ANT_3]     = "BT_ANT3",
	[FUNC_CODE_CAN0_RX]      = "CAN0_RX",
	[FUNC_CODE_CAN0_STANDBY] = "CAN0_STBY",
	[FUNC_CODE_CAN0_TX]      = "CAN0_TX",
	[FUNC_CODE_CAN1_RX]      = "CAN1_RX",
	[FUNC_CODE_CAN1_STANDBY] = "CAN1_STBY",
	[FUNC_CODE_CAN1_TX]      = "CAN1_TX",
	[FUNC_CODE_I2C0_SCL]     = "I2C0_SCL",
	[FUNC_CODE_I2C0_SDA]     = "I2C0_SDA",
	[FUNC_CODE_I2C1_SCL]     = "I2C1_SCL",
	[FUNC_CODE_I2C1_SDA]     = "I2C1_SDA",
	[FUNC_CODE_IRDA0]        = "IRDA0",
	[FUNC_CODE_IRDA1]        = "IRDA1",
	[FUNC_CODE_IRDA2]        = "IRDA2",
	[FUNC_CODE_IRDA3]        = "IRDA3",
	[FUNC_CODE_LIN0_RXD]     = "LIN0_RXD",
	[FUNC_CODE_LIN0_SLEEP]   = "LIN0_SLEEP",
	[FUNC_CODE_LIN0_TXD]     = "LIN0_TXD",
	[FUNC_CODE_SC0_CLK]      = "SC0_CLK",
	[FUNC_CODE_SC0_IO]       = "SC0_IO",
	[FUNC_CODE_SC0_RSTN]     = "SC0_RSTN",
	[FUNC_CODE_SC0_VCC]      = "SC0_VCC",
	[FUNC_CODE_SPDIF0_RX]    = "SPDIF0_RX",
	[FUNC_CODE_SPDIF0_TX]    = "SPDIF0_TX",
	[FUNC_CODE_SPDIF1_RX]    = "SPDIF1_RX",
	[FUNC_CODE_SPDIF1_TX]    = "SPDIF1_TX",
	[FUNC_CODE_UART0_CTS]    = "UART0_CTS",
	[FUNC_CODE_UART0_RTS]    = "UART0_RTS",
	[FUNC_CODE_UART0_RXD]    = "UART0_RXD",
	[FUNC_CODE_UART0_TXD]    = "UART0_TXD",
	[FUNC_CODE_UART1_RXD]    = "UART1_RXD",
	[FUNC_CODE_UART1_TXD]    = "UART1_TXD",
	[FUNC_CODE_UART2_RXD]    = "UART2_RXD",
	[FUNC_CODE_UART2_TXD]    = "UART2_TXD",
	[FUNC_CODE_UART3_RXD]    = "UART3_RXD",
	[FUNC_CODE_UART3_TXD]    = "UART3_TXD",
	[FUNC_CODE_UART4_RXD]    = "UART4_RXD",
	[FUNC_CODE_UART4_TXD]    = "UART4_TXD",
};

/* Concrete device -> name for the fixed-map peripherals (group codes 126~134).
 * Only the devices that appear in MAP_FUNC_CODE_FIX_GPIO need a name here. */
static const char *gpio_dev_name(gpio_dev_t dev)
{
	switch (dev) {
	/* FUNC_CODE_126: rf / clock / otp fixed funcs */
	case GPIO_DEV_OTP_FRE_EN:      return "OTP_FRE_EN";
	case GPIO_DEV_OTP_FRE_SEL:     return "OTP_FRE_SEL";
	case GPIO_DEV_OTP_FRE_OUT:     return "OTP_FRE_OUT";
	case GPIO_DEV_CLK13M:          return "CLK13M";
	case GPIO_DEV_CLK26M:          return "CLK26M";
	case GPIO_DEV_LPO_CLK:         return "LPO_CLK";
	case GPIO_DEV_WIFI_ACTIVE:     return "WIFI_ACTIVE";
	case GPIO_DEV_WIFI_TX_EN:      return "WIFI_TX_EN";
	case GPIO_DEV_WIFI_RX_EN:      return "WIFI_RX_EN";
	case GPIO_DEV_FEM_LNA_EN:      return "FEM_LNA_EN";
	case GPIO_DEV_CLK_AUXS:        return "CLK_AUXS";
	case GPIO_DEV_CLK_AUXS_CIS:    return "CLK_AUXS_CIS";
	case GPIO_DEV_CLK_AUXS_ENET:   return "CLK_AUXS_ENET";

	/* FUNC_CODE_127: debug bus */
	case GPIO_DEV_DEBUG0:          return "DEBUG0";
	case GPIO_DEV_DEBUG1:          return "DEBUG1";
	case GPIO_DEV_DEBUG2:          return "DEBUG2";
	case GPIO_DEV_DEBUG3:          return "DEBUG3";
	case GPIO_DEV_DEBUG4:          return "DEBUG4";
	case GPIO_DEV_DEBUG5:          return "DEBUG5";
	case GPIO_DEV_DEBUG6:          return "DEBUG6";
	case GPIO_DEV_DEBUG7:          return "DEBUG7";
	case GPIO_DEV_DEBUG8:          return "DEBUG8";
	case GPIO_DEV_DEBUG9:          return "DEBUG9";
	case GPIO_DEV_DEBUG10:         return "DEBUG10";
	case GPIO_DEV_DEBUG11:         return "DEBUG11";
	case GPIO_DEV_DEBUG12:         return "DEBUG12";
	case GPIO_DEV_DEBUG13:         return "DEBUG13";
	case GPIO_DEV_DEBUG14:         return "DEBUG14";
	case GPIO_DEV_DEBUG15:         return "DEBUG15";
	case GPIO_DEV_DEBUG16:         return "DEBUG16";
	case GPIO_DEV_DEBUG17:         return "DEBUG17";
	case GPIO_DEV_DEBUG18:         return "DEBUG18";
	case GPIO_DEV_DEBUG19:         return "DEBUG19";
	case GPIO_DEV_DEBUG20:         return "DEBUG20";
	case GPIO_DEV_DEBUG21:         return "DEBUG21";
	case GPIO_DEV_DEBUG22:         return "DEBUG22";
	case GPIO_DEV_DEBUG23:         return "DEBUG23";
	case GPIO_DEV_DEBUG24:         return "DEBUG24";
	case GPIO_DEV_DEBUG25:         return "DEBUG25";
	case GPIO_DEV_DEBUG26:         return "DEBUG26";
	case GPIO_DEV_DEBUG27:         return "DEBUG27";
	case GPIO_DEV_DEBUG28:         return "DEBUG28";
	case GPIO_DEV_DEBUG29:         return "DEBUG29";
	case GPIO_DEV_DEBUG30:         return "DEBUG30";
	case GPIO_DEV_DEBUG31:         return "DEBUG31";

	/* FUNC_CODE_129: sdio host0/1, jpeg(dvp), enet_ref */
	case GPIO_DEV_SDIO_HOST_CLK:   return "SDIO_HOST_CLK";
	case GPIO_DEV_SDIO_HOST_CMD:   return "SDIO_HOST_CMD";
	case GPIO_DEV_SDIO_HOST_DATA0: return "SDIO_HOST_DATA0";
	case GPIO_DEV_SDIO_HOST_DATA1: return "SDIO_HOST_DATA1";
	case GPIO_DEV_SDIO_HOST_DATA2: return "SDIO_HOST_DATA2";
	case GPIO_DEV_SDIO_HOST_DATA3: return "SDIO_HOST_DATA3";
	case GPIO_DEV_SDIO_HOST_DATA4: return "SDIO_HOST_DATA4";
	case GPIO_DEV_SDIO_HOST_DATA5: return "SDIO_HOST_DATA5";
	case GPIO_DEV_SDIO_HOST_DATA6: return "SDIO_HOST_DATA6";
	case GPIO_DEV_SDIO_HOST_DATA7: return "SDIO_HOST_DATA7";
	case GPIO_DEV_SDIO_HOST_STB:   return "SDIO_HOST_STB";
	case GPIO_DEV_SDIO1_HOST_CLK:  return "SDIO1_HOST_CLK";
	case GPIO_DEV_SDIO1_HOST_CMD:  return "SDIO1_HOST_CMD";
	case GPIO_DEV_SDIO1_HOST_DATA0: return "SDIO1_HOST_DATA0";
	case GPIO_DEV_SDIO1_HOST_DATA1: return "SDIO1_HOST_DATA1";
	case GPIO_DEV_SDIO1_HOST_DATA2: return "SDIO1_HOST_DATA2";
	case GPIO_DEV_SDIO1_HOST_DATA3: return "SDIO1_HOST_DATA3";
	case GPIO_DEV_SDIO1_HOST_DATA4: return "SDIO1_HOST_DATA4";
	case GPIO_DEV_SDIO1_HOST_DATA5: return "SDIO1_HOST_DATA5";
	case GPIO_DEV_SDIO1_HOST_DATA6: return "SDIO1_HOST_DATA6";
	case GPIO_DEV_SDIO1_HOST_DATA7: return "SDIO1_HOST_DATA7";
	case GPIO_DEV_SDIO1_HOST_STB:  return "SDIO1_HOST_STB";
	case GPIO_DEV_JPEG_PCLK:       return "JPEG_PCLK";
	case GPIO_DEV_JPEG_HSYNC:      return "JPEG_HSYNC";
	case GPIO_DEV_JPEG_VSYNC:      return "JPEG_VSYNC";
	case GPIO_DEV_JPEG_MCLK:       return "JPEG_MCLK";
	case GPIO_DEV_JPEG_PXDATA0:    return "JPEG_PXD0";
	case GPIO_DEV_JPEG_PXDATA1:    return "JPEG_PXD1";
	case GPIO_DEV_JPEG_PXDATA2:    return "JPEG_PXD2";
	case GPIO_DEV_JPEG_PXDATA3:    return "JPEG_PXD3";
	case GPIO_DEV_JPEG_PXDATA4:    return "JPEG_PXD4";
	case GPIO_DEV_JPEG_PXDATA5:    return "JPEG_PXD5";
	case GPIO_DEV_JPEG_PXDATA6:    return "JPEG_PXD6";
	case GPIO_DEV_JPEG_PXDATA7:    return "JPEG_PXD7";
	case GPIO_DEV_JPEG_PXDATA8:    return "JPEG_PXD8";
	case GPIO_DEV_JPEG_PXDATA9:    return "JPEG_PXD9";

	/* FUNC_CODE_130/132: qspi0/1, enet rmii/rgmii, uart5 */
	case GPIO_DEV_QSPI0_CLK:       return "QSPI0_CLK";
	case GPIO_DEV_QSPI0_CSN:       return "QSPI0_CSN";
	case GPIO_DEV_QSPI0_IO0:       return "QSPI0_IO0";
	case GPIO_DEV_QSPI0_IO1:       return "QSPI0_IO1";
	case GPIO_DEV_QSPI0_IO2:       return "QSPI0_IO2";
	case GPIO_DEV_QSPI0_IO3:       return "QSPI0_IO3";
	case GPIO_DEV_QSPI1_CLK:       return "QSPI1_CLK";
	case GPIO_DEV_QSPI1_CSN:       return "QSPI1_CSN";
	case GPIO_DEV_QSPI1_IO0:       return "QSPI1_IO0";
	case GPIO_DEV_QSPI1_IO1:       return "QSPI1_IO1";
	case GPIO_DEV_QSPI1_IO2:       return "QSPI1_IO2";
	case GPIO_DEV_QSPI1_IO3:       return "QSPI1_IO3";
	case GPIO_DEV_ENET_REF_CLK:    return "ENET_REF_CLK";
	case GPIO_DEV_ENET_GTCLK:      return "ENET_GTCLK";
	case GPIO_DEV_ENET_GRCLK:      return "ENET_GRCLK";
	case GPIO_DEV_ENET_TXD0:       return "ENET_TXD0";
	case GPIO_DEV_ENET_TXD1:       return "ENET_TXD1";
	case GPIO_DEV_ENET_TXD2:       return "ENET_TXD2";
	case GPIO_DEV_ENET_TXD3:       return "ENET_TXD3";
	case GPIO_DEV_ENET_RXD0:       return "ENET_RXD0";
	case GPIO_DEV_ENET_RXD1:       return "ENET_RXD1";
	case GPIO_DEV_ENET_RXD2:       return "ENET_RXD2";
	case GPIO_DEV_ENET_RXD3:       return "ENET_RXD3";
	case GPIO_DEV_ENET_TXEN:       return "ENET_TXEN";
	case GPIO_DEV_ENET_RXDV:       return "ENET_RXDV";
	case GPIO_DEV_ENET_MDC:        return "ENET_MDC";
	case GPIO_DEV_ENET_MDIO:       return "ENET_MDIO";
	case GPIO_DEV_ENET_PHY_INT:    return "ENET_PHY_INT";
	case GPIO_DEV_UART5_RXD:       return "UART5_RXD";
	case GPIO_DEV_UART5_TXD:       return "UART5_TXD";

	/* FUNC_CODE_131: lcd rgb/8080 */
	case GPIO_DEV_LCD_R0:          return "LCD_R0";
	case GPIO_DEV_LCD_R1:          return "LCD_R1";
	case GPIO_DEV_LCD_R2:          return "LCD_R2";
	case GPIO_DEV_LCD_R3:          return "LCD_R3";
	case GPIO_DEV_LCD_R4:          return "LCD_R4";
	case GPIO_DEV_LCD_R5:          return "LCD_R5";
	case GPIO_DEV_LCD_R6:          return "LCD_R6";
	case GPIO_DEV_LCD_R7:          return "LCD_R7";
	case GPIO_DEV_LCD_G0:          return "LCD_G0";
	case GPIO_DEV_LCD_G1:          return "LCD_G1";
	case GPIO_DEV_LCD_G2:          return "LCD_G2";
	case GPIO_DEV_LCD_G3:          return "LCD_G3";
	case GPIO_DEV_LCD_G4:          return "LCD_G4";
	case GPIO_DEV_LCD_G5:          return "LCD_G5";
	case GPIO_DEV_LCD_G6:          return "LCD_G6";
	case GPIO_DEV_LCD_G7:          return "LCD_G7";
	case GPIO_DEV_LCD_B0:          return "LCD_B0";
	case GPIO_DEV_LCD_B1:          return "LCD_B1";
	case GPIO_DEV_LCD_B2:          return "LCD_B2";
	case GPIO_DEV_LCD_B3:          return "LCD_B3";
	case GPIO_DEV_LCD_B4:          return "LCD_B4";
	case GPIO_DEV_LCD_B5:          return "LCD_B5";
	case GPIO_DEV_LCD_B6:          return "LCD_B6";
	case GPIO_DEV_LCD_B7:          return "LCD_B7";
	case GPIO_DEV_LCD_CLK:         return "LCD_CLK";
	case GPIO_DEV_LCD_DISP:        return "LCD_DISP";
	case GPIO_DEV_LCD_HSYNC:       return "LCD_HSYNC";
	case GPIO_DEV_LCD_VSYNC:       return "LCD_VSYNC";
	case GPIO_DEV_LCD_DE:          return "LCD_DE";

	default:                       return NULL;
	}
}

/* Reverse lookup the fixed map: (gpio_id, code) -> concrete device. */
static gpio_dev_t gpio_fix_dev_by_id_code(gpio_id_t id, IOMX_CODE_T code)
{
	for (int i = 0; i < ARRAY_SIZE(s_fix_gpio_map); i++) {
		if (s_fix_gpio_map[i].id == id && s_fix_gpio_map[i].code == code) {
			return s_fix_gpio_map[i].gdev;
		}
	}
	return GPIO_DEV_INVALID;
}

/* Resolve a pad's function selector value into a human readable name.
 * id is required to disambiguate the fixed-map group codes (126~134). */
const char *bk_gpio_func_name(gpio_id_t id, uint32_t fun_sel)
{
	static char buf[20];

	if (fun_sel < ARRAY_SIZE(s_funcode_name) && s_funcode_name[fun_sel]) {
		return s_funcode_name[fun_sel];
	}

	if (fun_sel >= FUNC_CODE_126 && fun_sel <= FUNC_CODE_134) {
		gpio_dev_t dev = gpio_fix_dev_by_id_code(id, (IOMX_CODE_T)fun_sel);
		if (dev != GPIO_DEV_INVALID) {
			const char *name = gpio_dev_name(dev);
			if (name) {
				return name;
			}
			snprintf(buf, sizeof(buf), "DEV#%d", dev);
			return buf;
		}
		snprintf(buf, sizeof(buf), "FIX#%lu?", (unsigned long)fun_sel);
		return buf;
	}

	snprintf(buf, sizeof(buf), "RSV(0x%02lx)", (unsigned long)fun_sel);
	return buf;
}
