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

#ifdef __cplusplus
extern "C" {
#endif

#include <common/bk_err.h>

/**
 * @brief GPIO defines
 * @defgroup bk_api_gpio_defs macos
 * @ingroup bk_api_gpio
 * @{
 */

/**
 * @brief default GPIO pins number
 */
typedef enum {
	GPIO_0 = 0,
	GPIO_1,
	GPIO_2,
	GPIO_3,
	GPIO_4,
	GPIO_5,
	GPIO_6,
	GPIO_7,
	GPIO_8,
	GPIO_9,
	GPIO_10,
	GPIO_11,
	GPIO_12,
	GPIO_13,
	GPIO_14,
	GPIO_15,
	GPIO_16,
	GPIO_17,
	GPIO_18,
	GPIO_19,
	GPIO_20,
	GPIO_21,
	GPIO_22,
	GPIO_23,
	GPIO_24,
	GPIO_25,
	GPIO_26,
	GPIO_27,
	GPIO_28,
	GPIO_29,
	GPIO_30,
	GPIO_31,
	GPIO_32,
	GPIO_33,
	GPIO_34,
	GPIO_35,
	GPIO_36,
	GPIO_37,
	GPIO_38,
	GPIO_39,
	GPIO_40,
	GPIO_41,
	GPIO_42,
	GPIO_43,
	GPIO_44,
	GPIO_45,
	GPIO_46,
	GPIO_47,
	GPIO_48,
	GPIO_49,
	GPIO_50,
	GPIO_51,
	GPIO_52,
	GPIO_53,
	GPIO_54,
	GPIO_55,
	GPIO_56,
	GPIO_57,
	GPIO_58,
	GPIO_59,
	GPIO_60,
	GPIO_61,
	GPIO_62,
	GPIO_63,
	GPIO_64,
	GPIO_65,
	GPIO_66,
	GPIO_67,
	GPIO_68,
	GPIO_69,
	GPIO_70,
	GPIO_71,
	GPIO_NUM, /**<The maximum GPIO number is independent of the chip. Please use SOC_GPIO_NUM for chip */
} gpio_id_t;


/**
 * @brief GPIO input/output enable
 */
typedef enum {
	GPIO_IO_DISABLE = 0,		/**<disable gpio output and input mode which is high impendence state */
	GPIO_OUTPUT_ENABLE = 1,		/**<set gpio output mode */
	GPIO_INPUT_ENABLE = 2,		/**<set gpio input mode */
	GPIO_IO_INVALID = 3,		/**<gpio invalid mode */
} gpio_io_mode_t;


/**
 * @brief GPIO pull up/down set
 */
typedef enum {
	GPIO_PULL_DISABLE = 0,		/**<disbale gpio pull mode */
	GPIO_PULL_DOWN_EN = 1,		/**<set gpio as pull down mode */
	GPIO_PULL_UP_EN = 2,		/**<set gpio as pull up mode */
	GPIO_PULL_INVALID = 3,		/**<gpio invalid pull mode */
} gpio_pull_mode_t;

/**
 * @brief GPIO second function disable/enable
 */
typedef enum {
	GPIO_SECOND_FUNC_DISABLE = 0, /**< disbale gpio second function */
	GPIO_SECOND_FUNC_ENABLE,      /**< enbale gpio second function */
} gpio_func_mode_t;

/**
 * @brief GPIO mode config
 */
typedef struct {
	gpio_io_mode_t io_mode;		/**< set gpio output or input mode */
	gpio_pull_mode_t pull_mode;	/**< set gpio pull mode */
	gpio_func_mode_t func_mode; /**< set gpio func mode */
} gpio_config_t;

/**
 * @brief GPIO intterrupt type
 */
typedef enum {
	GPIO_INT_TYPE_LOW_LEVEL = 0,		/**<set gpio as low level intterrupt type */
	GPIO_INT_TYPE_HIGH_LEVEL,		/**<set gpio as high level intterrupt type */
	GPIO_INT_TYPE_RISING_EDGE,		/**<set gpio as rising edge intterrupt type */
	GPIO_INT_TYPE_FALLING_EDGE,		/**<set gpio as falling edge intterrupt type */
	GPIO_INT_TYPE_MAX,			/**< Invalid intterrupt type mode */
} gpio_int_type_t;

/**
 * @brief GPIO Driver Capacity
 */
typedef enum {
	GPIO_DRIVER_CAPACITY_0 = 0,		/**<GPIO Driver Capacity level 0 */
	GPIO_DRIVER_CAPACITY_1 = 1,		/**<GPIO Driver Capacity level 1 */
	GPIO_DRIVER_CAPACITY_2 = 2,		/**<GPIO Driver Capacity level 2 */
	GPIO_DRIVER_CAPACITY_3 = 3,		/**<GPIO Driver Capacity level 3 */
} gpio_driver_capacity_t;

typedef enum{
	GPIO_INIT_DISABLE = 0,        /* the sdk don't operate gpio when reboot/power on*/
	GPIO_INIT_ENABLE =1,          /* the sdk operate gpio follow the default map config when reboot/power on*/
} gpio_skip_t;

typedef enum{
	GPIO_TIME_SHARING_MULTIPLEX_DISABLE = 0,   /*disable gpio time sharing multiplex*/
	GPIO_TIME_SHARING_MULTIPLEX_ENABLE =1,     /*enable gpio time sharing multiplex*/
} gpio_time_sharing_multiplex_t;

#if CONFIG_USR_GPIO_CFG_EN
typedef enum {
	GPIO_LOW_POWER_DISCARD_IO_STATUS = 0,	//low power switch to no-input,no-output,wakeup restore status
	GPIO_LOW_POWER_KEEP_INPUT_STATUS = 1,
	GPIO_LOW_POWER_KEEP_OUTPUT_STATUS = 3,
}gpio_lowpower_mode_t;

typedef enum {
	GPIO_INT_DISABLE = 0,
	GPIO_INT_ENABLE
}gpio_int_mode_t;
#endif

/**
 * @brief GPIOs device number
 *
 * The entries are grouped by peripheral type, and each group is anchored to a
 * fixed base value (with reserved headroom) so that adding a new member inside
 * a group does not shift the values of other groups. GPIO_DEV_NONE keeps 0 and
 * GPIO_DEV_INVALID keeps 0xFFF as sentinels. The numeric value is an internal
 * logical id only; the actual pad function code is resolved via the IO-matrix
 * maps (GPIO_DEV_TO_IOMX_CODE_MAP / MAP_FUNC_CODE_FIX_GPIO) using the symbol
 * name, so re-numbering here does not affect those maps.
 */
typedef enum {
	GPIO_DEV_NONE = 0, /**< The GPIO doesn't map to any device */

	/* ===== PWM ===== */
	GPIO_DEV_PWM0 = 0x01,
	GPIO_DEV_PWM1,
	GPIO_DEV_PWM2,
	GPIO_DEV_PWM3,
	GPIO_DEV_PWM4,
	GPIO_DEV_PWM5,
	GPIO_DEV_PWM6,
	GPIO_DEV_PWM7,
	GPIO_DEV_PWM8,
	GPIO_DEV_PWM9,
	GPIO_DEV_PWM10,
	GPIO_DEV_PWM11,

	/* ===== ADC ===== */
	GPIO_DEV_ADC0 = 0x0F,
	GPIO_DEV_ADC1,
	GPIO_DEV_ADC2,
	GPIO_DEV_ADC3,
	GPIO_DEV_ADC4,
	GPIO_DEV_ADC5,
	GPIO_DEV_ADC6,
	GPIO_DEV_ADC7,
	GPIO_DEV_ADC8,
	GPIO_DEV_ADC9,
	GPIO_DEV_ADC10,
	GPIO_DEV_ADC11,
	GPIO_DEV_ADC12,
	GPIO_DEV_ADC13,
	GPIO_DEV_ADC14,
	GPIO_DEV_ADC15,

	/* ===== UART ===== */
	GPIO_DEV_UART0_TXD = 0x1F,
	GPIO_DEV_UART0_RXD,
	GPIO_DEV_UART0_CTS,
	GPIO_DEV_UART0_RTS,
	GPIO_DEV_UART1_TXD,
	GPIO_DEV_UART1_RXD,
	GPIO_DEV_UART2_TXD,
	GPIO_DEV_UART2_RXD,
	GPIO_DEV_UART3_TXD,
	GPIO_DEV_UART3_RXD,
	GPIO_DEV_UART3_RTS,
	GPIO_DEV_UART3_CTS,
	GPIO_DEV_UART4_TXD,
	GPIO_DEV_UART4_RXD,
	GPIO_DEV_UART5_TXD,
	GPIO_DEV_UART5_RXD,

	/* ===== I2C ===== */
	GPIO_DEV_I2C0_SCL = 0x30,
	GPIO_DEV_I2C0_SDA,
	GPIO_DEV_I2C1_SCL,
	GPIO_DEV_I2C1_SDA,
	GPIO_DEV_I2C2_SCL,
	GPIO_DEV_I2C2_SDA,

	/* ===== SPI ===== */
	GPIO_DEV_SPI0_SCK = 0x38,
	GPIO_DEV_SPI0_CSN,
	GPIO_DEV_SPI0_MOSI,
	GPIO_DEV_SPI0_MISO,
	GPIO_DEV_SPI1_SCK,
	GPIO_DEV_SPI1_CSN,
	GPIO_DEV_SPI1_MOSI,
	GPIO_DEV_SPI1_MISO,
	GPIO_DEV_SPI2_SCK,
	GPIO_DEV_SPI2_CSN,
	GPIO_DEV_SPI2_MOSI,
	GPIO_DEV_SPI2_MISO,
	GPIO_DEV_SPI3_SCK,
	GPIO_DEV_SPI3_CSN,
	GPIO_DEV_SPI3_MOSI,
	GPIO_DEV_SPI3_MISO,

	/* ===== I2S ===== */
	GPIO_DEV_I2S0_CLK = 0x48,
	GPIO_DEV_I2S0_SYNC,
	GPIO_DEV_I2S0_DIN,
	GPIO_DEV_I2S0_DOUT,
	GPIO_DEV_I2S1_CLK,
	GPIO_DEV_I2S1_SYNC,
	GPIO_DEV_I2S1_DIN,
	GPIO_DEV_I2S1_DOUT,
	GPIO_DEV_I2S2_CLK,
	GPIO_DEV_I2S2_SYNC,
	GPIO_DEV_I2S2_DIN,
	GPIO_DEV_I2S2_DOUT,
	GPIO_DEV_I2S3_CLK,
	GPIO_DEV_I2S3_SYNC,
	GPIO_DEV_I2S3_DIN,
	GPIO_DEV_I2S3_DOUT,
	GPIO_DEV_I2S4_CLK,
	GPIO_DEV_I2S4_SYNC,
	GPIO_DEV_I2S4_DIN,
	GPIO_DEV_I2S4_DOUT,
	GPIO_DEV_I2S0_DOUT2,
	GPIO_DEV_I2S0_DOUT3,
	GPIO_DEV_I2S0_MCLK,

	/* ===== JTAG ===== */
	GPIO_DEV_SWCLK = 0x60,
	GPIO_DEV_SWDIO,

	/* ===== DMIC ===== */
	GPIO_DEV_DMIC0_CLK = 0x64,
	GPIO_DEV_DMIC0_DAT,
	GPIO_DEV_DMIC1_CLK,
	GPIO_DEV_DMIC1_DAT,

	/* ===== I3C ===== */
	GPIO_DEV_I3C_SCL = 0x68,
	GPIO_DEV_I3C_SDA,
	GPIO_DEV_I3C_SDA_PURN,

	/* ===== LIN ===== */
	GPIO_DEV_LIN_TXD = 0x6B,
	GPIO_DEV_LIN_RXD,
	GPIO_DEV_LIN_SLEEP,

	/* ===== USB ===== */
	GPIO_DEV_USB0_DP = 0x6E,
	GPIO_DEV_USB0_DN,
	GPIO_DEV_USB1_DP,
	GPIO_DEV_USB1_DN,

	/* ===== Smart card reader ===== */
	GPIO_DEV_SCR_IO = 0x72,
	GPIO_DEV_SCR_CLK,
	GPIO_DEV_SCR_RSTN,
	GPIO_DEV_SCR_VCC,

	/* ===== CAN ===== */
	GPIO_DEV_CAN_TX = 0x76,
	GPIO_DEV_CAN_RX,
	GPIO_DEV_CAN_STANDBY,
	GPIO_DEV_CAN1_TX,
	GPIO_DEV_CAN1_RX,
	GPIO_DEV_CAN1_STANDBY,

	/* ===== BT antenna ===== */
	GPIO_DEV_BT_ANT0 = 0x7C,
	GPIO_DEV_BT_ANT1,
	GPIO_DEV_BT_ANT2,
	GPIO_DEV_BT_ANT3,

	/* ===== Touch ===== */
	GPIO_DEV_TOUCH0 = 0x80,
	GPIO_DEV_TOUCH1,
	GPIO_DEV_TOUCH2,
	GPIO_DEV_TOUCH3,
	GPIO_DEV_TOUCH4,
	GPIO_DEV_TOUCH5,
	GPIO_DEV_TOUCH6,
	GPIO_DEV_TOUCH7,
	GPIO_DEV_TOUCH8,
	GPIO_DEV_TOUCH9,
	GPIO_DEV_TOUCH10,
	GPIO_DEV_TOUCH11,
	GPIO_DEV_TOUCH12,
	GPIO_DEV_TOUCH13,
	GPIO_DEV_TOUCH14,
	GPIO_DEV_TOUCH15,

	/* ===== JPEG / DVP ===== */
	GPIO_DEV_JPEG_MCLK = 0x90,
	GPIO_DEV_JPEG_PCLK,
	GPIO_DEV_JPEG_HSYNC,
	GPIO_DEV_JPEG_VSYNC,
	GPIO_DEV_JPEG_PXDATA0,
	GPIO_DEV_JPEG_PXDATA1,
	GPIO_DEV_JPEG_PXDATA2,
	GPIO_DEV_JPEG_PXDATA3,
	GPIO_DEV_JPEG_PXDATA4,
	GPIO_DEV_JPEG_PXDATA5,
	GPIO_DEV_JPEG_PXDATA6,
	GPIO_DEV_JPEG_PXDATA7,
	GPIO_DEV_JPEG_PXDATA8,
	GPIO_DEV_JPEG_PXDATA9,

	/* ===== QSPI ===== */
	GPIO_DEV_QSPI0_CLK = 0xA0,
	GPIO_DEV_QSPI0_CSN,
	GPIO_DEV_QSPI0_IO0,
	GPIO_DEV_QSPI0_IO1,
	GPIO_DEV_QSPI0_IO2,
	GPIO_DEV_QSPI0_IO3,
	GPIO_DEV_QSPI1_CLK,
	GPIO_DEV_QSPI1_CSN,
	GPIO_DEV_QSPI1_IO0,
	GPIO_DEV_QSPI1_IO1,
	GPIO_DEV_QSPI1_IO2,
	GPIO_DEV_QSPI1_IO3,

	/* ===== SPDIF ===== */
	GPIO_DEV_SPDIF1 = 0xAC,
	GPIO_DEV_SPDIF2,
	GPIO_DEV_SPDIF3,
	GPIO_DEV_SPDIF0_RX,
	GPIO_DEV_SPDIF0_TX,
	GPIO_DEV_SPDIF1_RX,
	GPIO_DEV_SPDIF1_TX,

	/* ===== IRDA ===== */
	GPIO_DEV_IRDA = 0xB3,
	GPIO_DEV_IRDA1,
	GPIO_DEV_IRDA2,
	GPIO_DEV_IRDA3,

	/* ===== OTP ===== */
	GPIO_DEV_OTP_FRE_EN = 0xB7,
	GPIO_DEV_OTP_FRE_SEL,
	GPIO_DEV_OTP_FRE_OUT,

	/* ===== LCD ===== */
	GPIO_DEV_LCD_B0 = 0xC0,
	GPIO_DEV_LCD_B1,
	GPIO_DEV_LCD_B2,
	GPIO_DEV_LCD_B3,
	GPIO_DEV_LCD_B4,
	GPIO_DEV_LCD_B5,
	GPIO_DEV_LCD_B6,
	GPIO_DEV_LCD_B7,
	GPIO_DEV_LCD_G0,
	GPIO_DEV_LCD_G1,
	GPIO_DEV_LCD_G2,
	GPIO_DEV_LCD_G3,
	GPIO_DEV_LCD_G4,
	GPIO_DEV_LCD_G5,
	GPIO_DEV_LCD_G6,
	GPIO_DEV_LCD_G7,
	GPIO_DEV_LCD_R0,
	GPIO_DEV_LCD_R1,
	GPIO_DEV_LCD_R2,
	GPIO_DEV_LCD_R3,
	GPIO_DEV_LCD_R4,
	GPIO_DEV_LCD_R5,
	GPIO_DEV_LCD_R6,
	GPIO_DEV_LCD_R7,
	GPIO_DEV_LCD_CLK,
	GPIO_DEV_LCD_DISP,
	GPIO_DEV_LCD_HSYNC,
	GPIO_DEV_LCD_VSYNC,
	GPIO_DEV_LCD_DE,

	/* ===== Debug bus ===== */
	GPIO_DEV_DEBUG0 = 0xE0,
	GPIO_DEV_DEBUG1,
	GPIO_DEV_DEBUG2,
	GPIO_DEV_DEBUG3,
	GPIO_DEV_DEBUG4,
	GPIO_DEV_DEBUG5,
	GPIO_DEV_DEBUG6,
	GPIO_DEV_DEBUG7,
	GPIO_DEV_DEBUG8,
	GPIO_DEV_DEBUG9,
	GPIO_DEV_DEBUG10,
	GPIO_DEV_DEBUG11,
	GPIO_DEV_DEBUG12,
	GPIO_DEV_DEBUG13,
	GPIO_DEV_DEBUG14,
	GPIO_DEV_DEBUG15,
	GPIO_DEV_DEBUG16,
	GPIO_DEV_DEBUG17,
	GPIO_DEV_DEBUG18,
	GPIO_DEV_DEBUG19,
	GPIO_DEV_DEBUG20,
	GPIO_DEV_DEBUG21,
	GPIO_DEV_DEBUG22,
	GPIO_DEV_DEBUG23,
	GPIO_DEV_DEBUG24,
	GPIO_DEV_DEBUG25,
	GPIO_DEV_DEBUG26,
	GPIO_DEV_DEBUG27,
	GPIO_DEV_DEBUG28,
	GPIO_DEV_DEBUG29,
	GPIO_DEV_DEBUG30,
	GPIO_DEV_DEBUG31,

	/* ===== SDIO host 0 ===== */
	GPIO_DEV_SDIO_HOST_CLK = 0x100,
	GPIO_DEV_SDIO_HOST_CMD,
	GPIO_DEV_SDIO_HOST_DATA0,
	GPIO_DEV_SDIO_HOST_DATA1,
	GPIO_DEV_SDIO_HOST_DATA2,
	GPIO_DEV_SDIO_HOST_DATA3,
	GPIO_DEV_SDIO_HOST_DATA4,
	GPIO_DEV_SDIO_HOST_DATA5,
	GPIO_DEV_SDIO_HOST_DATA6,
	GPIO_DEV_SDIO_HOST_DATA7,
	GPIO_DEV_SDIO_HOST_STB,

	/* ===== SDIO host 1 ===== */
	GPIO_DEV_SDIO1_HOST_CLK = 0x110,
	GPIO_DEV_SDIO1_HOST_CMD,
	GPIO_DEV_SDIO1_HOST_DATA0,
	GPIO_DEV_SDIO1_HOST_DATA1,
	GPIO_DEV_SDIO1_HOST_DATA2,
	GPIO_DEV_SDIO1_HOST_DATA3,
	GPIO_DEV_SDIO1_HOST_DATA4,
	GPIO_DEV_SDIO1_HOST_DATA5,
	GPIO_DEV_SDIO1_HOST_DATA6,
	GPIO_DEV_SDIO1_HOST_DATA7,
	GPIO_DEV_SDIO1_HOST_STB,

	/* ===== Ethernet ===== */
	GPIO_DEV_ENET_PHY_INT = 0x120,
	GPIO_DEV_ENET_MDC,
	GPIO_DEV_ENET_MDIO,
	GPIO_DEV_ENET_RXD0,
	GPIO_DEV_ENET_RXD1,
	GPIO_DEV_ENET_RXD2,
	GPIO_DEV_ENET_RXD3,
	GPIO_DEV_ENET_RXDV,
	GPIO_DEV_ENET_TXD0,
	GPIO_DEV_ENET_TXD1,
	GPIO_DEV_ENET_TXD2,
	GPIO_DEV_ENET_TXD3,
	GPIO_DEV_ENET_TXEN,
	GPIO_DEV_ENET_REF_CLK,
	GPIO_DEV_ENET_GTCLK,
	GPIO_DEV_ENET_GRCLK,

	/* ===== SLCD ===== */
	GPIO_DEV_SLCD_COM0 = 0x140,
	GPIO_DEV_SLCD_COM1,
	GPIO_DEV_SLCD_COM2,
	GPIO_DEV_SLCD_COM3,
	GPIO_DEV_SLCD_COM4,
	GPIO_DEV_SLCD_COM5,
	GPIO_DEV_SLCD_COM6,
	GPIO_DEV_SLCD_COM7,
	GPIO_DEV_SLCD_SEG0,
	GPIO_DEV_SLCD_SEG1,
	GPIO_DEV_SLCD_SEG2,
	GPIO_DEV_SLCD_SEG3,
	GPIO_DEV_SLCD_SEG4,
	GPIO_DEV_SLCD_SEG5,
	GPIO_DEV_SLCD_SEG6,
	GPIO_DEV_SLCD_SEG7,
	GPIO_DEV_SLCD_SEG8,
	GPIO_DEV_SLCD_SEG9,
	GPIO_DEV_SLCD_SEG10,
	GPIO_DEV_SLCD_SEG11,
	GPIO_DEV_SLCD_SEG12,
	GPIO_DEV_SLCD_SEG13,
	GPIO_DEV_SLCD_SEG14,
	GPIO_DEV_SLCD_SEG15,
	GPIO_DEV_SLCD_SEG16,
	GPIO_DEV_SLCD_SEG17,
	GPIO_DEV_SLCD_SEG18,
	GPIO_DEV_SLCD_SEG19,
	GPIO_DEV_SLCD_SEG20,
	GPIO_DEV_SLCD_SEG21,
	GPIO_DEV_SLCD_SEG22,
	GPIO_DEV_SLCD_SEG23,
	GPIO_DEV_SLCD_SEG24,
	GPIO_DEV_SLCD_SEG25,
	GPIO_DEV_SLCD_SEG26,
	GPIO_DEV_SLCD_SEG27,
	GPIO_DEV_SLCD_SEG28,
	GPIO_DEV_SLCD_SEG29,
	GPIO_DEV_SLCD_SEG30,
	GPIO_DEV_SLCD_SEG31,

	/* ===== Misc clocks / control / coexistence signals ===== */
	GPIO_DEV_CLK13M = 0x170,
	GPIO_DEV_CLK26M,
	GPIO_DEV_LPO_CLK,
	GPIO_DEV_WIFI_ACTIVE,
	GPIO_DEV_BT_ACTIVE,
	GPIO_DEV_BT_PRIORITY,
	GPIO_DEV_TXEN,
	GPIO_DEV_RXEN,
	GPIO_DEV_PGA_INP,
	GPIO_DEV_PGA_INN,
	GPIO_DEV_HDMI_CEC,
	GPIO_DEV_DIG_CLKOUT1,
	GPIO_DEV_DIG_CLKOUT2,
	GPIO_DEV_CLK_AUXS,
	GPIO_DEV_CLK_AUXS_CIS,
	GPIO_DEV_CLK_AUXS_ENET,
	GPIO_DEV_CLK_32K_XO,
	GPIO_DEV_CLK_32K_XI,
	GPIO_DEV_TAMP_RX_I,
	GPIO_DEV_TAMP_TX_O,
	GPIO_DEV_CLK_XTAL_DIV,
	GPIO_DEV_CLK_XTAL,
	GPIO_DEV_FEM_LNA_EN,
	GPIO_DEV_WIFI_TX_EN,
	GPIO_DEV_WIFI_RX_EN,
	GPIO_DEV_SW_CLK,
	GPIO_DEV_SWD_IO,
	GPIO_DEV_LEDC,

	/* ===== Pure GPIO direction pseudo devices ===== */
	GPIO_DEV_GPIO_OUTPUT,   /* pure GPIO output direction */
	GPIO_DEV_GPIO_INPUT,    /* pure GPIO input direction */

	GPIO_DEV_INVALID = 0xFFF,
} gpio_dev_t;

/**
 * @brief use the gpio ctrl module name
 *
 * Module ID allocation:
 * - SDK Reserved:    0-9   (for system modules)
 * - Application:    10-31  (for user-defined modules)
 *
 * Application layer can define custom modules like:
 *   #define MY_MODULE_AUDIO  (GPIO_CTRL_LDO_MODULE_APP_BASE + 0)  // 10
 *   #define MY_MODULE_CAMERA (GPIO_CTRL_LDO_MODULE_APP_BASE + 1)  // 11
 */
typedef enum
{
	/* SDK Reserved Area (0-9) */
	GPIO_CTRL_LDO_MODULE_SDIO = 0,  /**< SDIO module */
	GPIO_CTRL_LDO_MODULE_LCD  = 1,  /**< LCD module */
	GPIO_CTRL_LDO_MODULE_DVP  = 2,  /**< DVP module */
	GPIO_CTRL_LDO_MODULE_USB  = 3,  /**< USB module */
	/* Reserved for future SDK modules: 4-9 */

	GPIO_CTRL_LDO_MODULE_SDK_MAX = 9,    /**< SDK reserved maximum */

	/* Application Area (10-31) */
	GPIO_CTRL_LDO_MODULE_APP_BASE = 10,  /**< Application module base ID */
	GPIO_CTRL_LDO_MODULE_MAX = 32        /**< Maximum module ID */
}gpio_ctrl_ldo_module_e;
/**
 * @brief gpio output state
 */
typedef enum
{
	GPIO_OUTPUT_STATE_LOW = 0, // 0
	GPIO_OUTPUT_STATE_HIGH,    // 1

	GPIO_OUTPUT_STATE_INVALID
}gpio_output_state_e;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
