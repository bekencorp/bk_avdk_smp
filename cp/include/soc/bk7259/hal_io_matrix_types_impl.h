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

#ifdef __cplusplus
extern "C" {
#endif

enum io_matrix_code {
	FUNC_CODE_HIGH_Z       = 0,
	FUNC_CODE_INPUT        = 1,
	FUNC_CODE_OUTPUT       = 2,
	FUNC_CODE_INPUT_OUTPUT = 3,

	FUNC_CODE_DMIC0_CLK    = 4,
	FUNC_CODE_DMIC0_DAT    = 5,
	FUNC_CODE_DMIC1_CLK    = 6,
	FUNC_CODE_DMIC1_DAT    = 7,

	FUNC_CODE_HDMI_CEC     = 8,

	FUNC_CODE_I2S0_DIN     = 9,
	FUNC_CODE_I2S0_DOUT    = 10,
	FUNC_CODE_I2S0_SCK     = 11,
	FUNC_CODE_I2S0_SYNC    = 12,
	FUNC_CODE_I2S1_DIN     = 13,
	FUNC_CODE_I2S1_DOUT    = 14,
	FUNC_CODE_I2S1_SCK     = 15,
	FUNC_CODE_I2S1_SYNC    = 16,
	FUNC_CODE_I2S2_DIN     = 17,
	FUNC_CODE_I2S2_DOUT    = 18,
	FUNC_CODE_I2S2_SCK     = 19,
	FUNC_CODE_I2S2_SYNC    = 20,
	FUNC_CODE_I2S3_DIN     = 21,
	FUNC_CODE_I2S3_DOUT    = 22,
	FUNC_CODE_I2S3_SCK     = 23,
	FUNC_CODE_I2S3_SYNC    = 24,
	FUNC_CODE_I2S4_DIN     = 25,
	FUNC_CODE_I2S4_DOUT    = 26,
	FUNC_CODE_I2S4_SCK     = 27,
	FUNC_CODE_I2S4_SYNC    = 28,
	FUNC_CODE_I2S_MCLK     = 29,

	FUNC_CODE_SWCLK        = 30,
	FUNC_CODE_SWDIO        = 31,

	FUNC_CODE_PWM0_0       = 32,
	FUNC_CODE_PWM0_1       = 33,
	FUNC_CODE_PWM0_2       = 34,
	FUNC_CODE_PWM0_3       = 35,
	FUNC_CODE_PWM0_4       = 36,
	FUNC_CODE_PWM0_5       = 37,
	FUNC_CODE_PWM0_6       = 38,
	FUNC_CODE_PWM0_7       = 39,
	FUNC_CODE_PWM0_8       = 40,
	FUNC_CODE_PWM0_9       = 41,
	FUNC_CODE_PWM0_10      = 42,
	FUNC_CODE_PWM0_11      = 43,

	FUNC_CODE_SPI0_MISO    = 44,
	FUNC_CODE_SPI0_MOSI    = 45,
	FUNC_CODE_SPI0_NSS     = 46,
	FUNC_CODE_SPI0_SCK     = 47,
	FUNC_CODE_SPI1_MISO    = 48,
	FUNC_CODE_SPI1_MOSI    = 49,
	FUNC_CODE_SPI1_NSS     = 50,
	FUNC_CODE_SPI1_SCK     = 51,
	FUNC_CODE_SPI2_MISO    = 52,
	FUNC_CODE_SPI2_MOSI    = 53,
	FUNC_CODE_SPI2_NSS     = 54,
	FUNC_CODE_SPI2_SCK     = 55,
	FUNC_CODE_SPI3_MISO    = 56,
	FUNC_CODE_SPI3_MOSI    = 57,
	FUNC_CODE_SPI3_NSS     = 58,
	FUNC_CODE_SPI3_SCK     = 59,

	FUNC_CODE_I3C_SCL      = 60,
	FUNC_CODE_I3C_SDA      = 61,
	FUNC_CODE_I3C_SDA_PURN = 62,

	FUNC_CODE_BT_ANT_0     = 64,
	FUNC_CODE_BT_ANT_1     = 65,
	FUNC_CODE_BT_ANT_2     = 66,
	FUNC_CODE_BT_ANT_3     = 67,

	FUNC_CODE_CAN0_RX      = 68,
	FUNC_CODE_CAN0_STANDBY = 69,
	FUNC_CODE_CAN0_TX      = 70,
	FUNC_CODE_CAN1_RX      = 71,
	FUNC_CODE_CAN1_STANDBY = 72,
	FUNC_CODE_CAN1_TX      = 73,

	FUNC_CODE_I2C0_SCL     = 74,
	FUNC_CODE_I2C0_SDA     = 75,
	FUNC_CODE_I2C1_SCL     = 76,
	FUNC_CODE_I2C1_SDA     = 77,

	FUNC_CODE_IRDA0        = 78,
	FUNC_CODE_IRDA1        = 79,
	FUNC_CODE_IRDA2        = 80,
	FUNC_CODE_IRDA3        = 81,

	FUNC_CODE_LIN0_RXD     = 82,
	FUNC_CODE_LIN0_SLEEP   = 83,
	FUNC_CODE_LIN0_TXD     = 84,

	FUNC_CODE_SC0_CLK      = 85,
	FUNC_CODE_SC0_IO       = 86,
	FUNC_CODE_SC0_RSTN     = 87,
	FUNC_CODE_SC0_VCC      = 88,

	FUNC_CODE_SPDIF0_RX    = 89,
	FUNC_CODE_SPDIF0_TX    = 90,
	FUNC_CODE_SPDIF1_RX    = 91,
	FUNC_CODE_SPDIF1_TX    = 92,

	FUNC_CODE_UART0_CTS    = 93,
	FUNC_CODE_UART0_RTS    = 94,
	FUNC_CODE_UART0_RXD    = 95,
	FUNC_CODE_UART0_TXD    = 96,
	FUNC_CODE_UART1_RXD    = 97,
	FUNC_CODE_UART1_TXD    = 98,
	FUNC_CODE_UART2_RXD    = 99,
	FUNC_CODE_UART2_TXD    = 100,
	FUNC_CODE_UART3_RXD    = 101,
	FUNC_CODE_UART3_TXD    = 102,
	FUNC_CODE_UART4_RXD    = 103,
	FUNC_CODE_UART4_TXD    = 104,

	FUNC_CODE_126 = 126,
	FUNC_CODE_127 = 127,
	FUNC_CODE_128 = 128,
	FUNC_CODE_129 = 129,
	FUNC_CODE_130 = 130,
	FUNC_CODE_131 = 131,
	FUNC_CODE_132 = 132,
	FUNC_CODE_133 = 133,
	FUNC_CODE_134 = 134,
	FUNC_CODE_INVALID = 0xff,
};

#ifdef __cplusplus
}
#endif
