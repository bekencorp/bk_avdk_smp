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

typedef enum {
	INT_SRC_CP_DMA0_NSEC        = 0,
	INT_SRC_CP_ENC_SEC          = 1,
	INT_SRC_CP_ENC_NSEC         = 2,
	INT_SRC_CP_TIMER            = 3,
	INT_SRC_CP_UART0            = 4,
	INT_SRC_CP_PWM              = 5,
	INT_SRC_CP_I2C0             = 6,
	INT_SRC_CP_SPI0             = 7,
	INT_SRC_CP_SARADC           = 8,
	INT_SRC_CP_IRDA             = 9,
	INT_SRC_CP_GDMA             = 11,
	INT_SRC_CP_LA               = 12,
	INT_SRC_CP_ACOMP0           = 13,
	INT_SRC_CP_ACOMP1           = 14,
	INT_SRC_CP_UART1            = 15,
	INT_SRC_CP_CPU0_FPU         = 16,
	INT_SRC_CP_CPU1_FPU         = 17,
	INT_SRC_CP_CAN              = 18,
	INT_SRC_CP_L2CACHE_ERR      = 19,
	INT_SRC_CP_VID_DISP0        = 20,
	INT_SRC_CP_CKMN             = 21,
	INT_SRC_CP_VID_DISP1        = 22,
	INT_SRC_CP_AUDIO            = 23,
	INT_SRC_CP_I2S0             = 24,
	INT_SRC_CP_I2S1             = 25,
	INT_SRC_CP_VID_DISP2        = 26,
	INT_SRC_CP_IPC_CHKSUM       = 27,
	INT_SRC_CP_THREAD           = 28,

	/* wifi */
	INT_SRC_CP_MODEM            = 29,
	INT_SRC_CP_MODEM_RC         = 30,
	INT_SRC_CP_MAC_TXRX_TIMER   = 31,
	INT_SRC_CP_MAC_TXRX_MISC    = 32,
	INT_SRC_CP_MAC_RX_TRIGGER   = 33,
	INT_SRC_CP_MAC_TX_TRIGGER   = 34,
	INT_SRC_CP_MAC_PROT_TRIGGER = 35,
	INT_SRC_CP_MAC_GENERAL      = 36,
	INT_SRC_CP_GPIO_NS          = 37,
	INT_SRC_CP_MAC_WAKEUP       = 38,

	/* btdm */
	INT_SRC_CP_BTDM             = 39,
	INT_SRC_CP_BLE              = 40,
	INT_SRC_CP_BT               = 41,
	INT_SRC_CP_BTDM_WAKE_UP     = 42,

	INT_SRC_CP_TOUCHED          = 43,
	INT_SRC_CP_I2S2             = 44,
	INT_SRC_CP_I2S3             = 45,
	INT_SRC_CP_SPDIF0           = 46,
	INT_SRC_CP_CEC              = 47,
	INT_SRC_CP_XDAC0            = 48,
	INT_SRC_CP_XDAC1            = 49,
	INT_SRC_CP_OTP              = 50,
	INT_SRC_CP_PLL_UNLOCK       = 51,
	INT_SRC_CP_DCO_UNLOCK       = 52,
	INT_SRC_CP_USBPLUG          = 53,
	INT_SRC_CP_RTC              = 54,
	INT_SRC_CP_GPIO             = 55, // GPIO_S
	INT_SRC_CP_UART2            = 56,
	INT_SRC_CP_SPI1             = 57,
	INT_SRC_CP_TIMER1           = 58,
	INT_SRC_CP_SPI3             = 59,
	INT_SRC_CP_SCR              = 60,
	INT_SRC_CP_LIN              = 61,
	INT_SRC_CP_CAN1             = 62,
	INT_SRC_CP_TIMER2           = 63,
	INT_SRC_CP_TIMER3           = 64,
	INT_SRC_CP_UART3            = 65,
	INT_SRC_CP_SPI2             = 66,
	INT_SRC_CP_UART4            = 67,
	INT_SRC_CP_I2C1             = 68,//address mapping indicates I2C3
	INT_SRC_CP_HSPL             = 69,
	INT_SRC_CP_BK24             = 70,
	INT_SRC_CP_IRDA1            = 71,
	INT_SRC_CP_IRDA2            = 72,
	INT_SRC_CP_IRDA3            = 73,
	INT_SRC_CP_I3C              = 74,
	INT_SRC_CP_I2S4             = 75,
	INT_SRC_CP_SPDIF1           = 76,
	INT_SRC_CP_MAILBOX          = 78,
	INT_SRC_CP_IPI              = 79,
	INT_SRC_CP_VID_DISP3        = 80,
	INT_SRC_CP_VAD              = 81,
	INT_SRC_CP_MAX_NUM          = 82,
	INT_SRC_CP_NONE,
} icu_int_m52s_src_t;

typedef enum {
	INT_SRC_M52S             = 0,
	INT_SRC_HPDMA            = 1,
	INT_SRC_MAILBOX          = 2,
	INT_SRC_IPI              = 3,
	INT_SRC_GDMA0            = 4,
	INT_SRC_CPU0_FPU         = 5,
	INT_SRC_NPU              = 6,
	INT_SRC_USB_FS           = 7,
	INT_SRC_USB_HS           = 8,
	INT_SRC_USB_PLUG         = 9,
	INT_SRC_UART5            = 10,
	INT_SRC_WWDT             = 11,
	INT_SRC_SDIO0            = 12,
	INT_SRC_SDIO1            = 13,
	INT_SRC_INET0            = 14,

	INT_SRC_QSPI0            = 16,
	INT_SRC_QSPI1            = 17,
	INT_SRC_HSPL             = 18,
	INT_SRC_ISP_MI           = 19,
	INT_SRC_ISP_FE           = 20,
	INT_SRC_ISP_ISP          = 21,
	INT_SRC_CSI              = 22,
	INT_SRC_H26E             = 23,
	INT_SRC_GPU              = 24,
	INT_SRC_H264D            = 25,
	INT_SRC_DPU              = 26,
	INT_SRC_DSI              = 27,
	INT_SRC_H264D_PP         = 28,
	INT_SRC_PSRAM0_ERR       = 29,
	INT_SRC_PSRAM1_ERR       = 30,
	INT_SRC_MPC              = 31,
	INT_SRC_TIMER4           = 32,
	INT_SRC_TIMER5           = 33,
	INT_SRC_GPIO_NS          = 34,
	INT_SRC_GPIO             = 35,
	INT_SRC_AUDIO            = 36,
	INT_SRC_I2S0             = 37,
	INT_SRC_I2S1             = 38,
	INT_SRC_I2S2             = 39,
	INT_SRC_I2S3             = 40,
	INT_SRC_I2S4             = 41,
	INT_SRC_SPDIF0           = 42,
	INT_SRC_SPDIF1           = 43,
	INT_SRC_CEC              = 44,
	INT_SRC_I2C0             = 45,
	INT_SRC_I2C1             = 46,//address mapping indicates I2C3
	INT_SRC_I3C              = 47,
	INT_SRC_UART0            = 48,
	INT_SRC_UART1            = 49,
	INT_SRC_UART2            = 50,
	INT_SRC_UART3            = 51,
	INT_SRC_UART4            = 52,
	INT_SRC_L2CACHE_ERR      = 53,

	INT_SRC_NONE = 54,
} icu_int_src_t;

#ifdef __cplusplus
}
#endif
