// Copyright     2023-2028 Beken
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

/*
 * BK7259 CP NVIC / ICU interrupt numbers.
 *
 * Aligned with cp/include/soc/bk7259/int_types_impl.h (icu_int_src_t).
 */

typedef enum _IRQn_Type {
	NonMaskableInt_IRQn         = -14,
	HardFault_IRQn              = -13,
	MemoryManagement_IRQn       = -12,
	BusFault_IRQn               = -11,
	UsageFault_IRQn             = -10,
	SecureFault_IRQn            = -9,
	SVCall_IRQn                 = -5,
	DebugMonitor_IRQn           = -4,
	PendSV_IRQn                 = -2,
	SysTick_IRQn                = -1,

	/* External interrupts (same numeric values as INT_SRC_* on BK7259 CP). */
	DMA0_NSEC_IRQn              =   0,
	ENCP_S_IRQn                   =   1,
	ENCP_NS_IRQn                  =   2,
	TIMER0_IRQn                   =   3,    /* HW Timer0 module (TIMER_ID0~2) */
	UART0_IRQn                    =   4,
	PWM0_IRQn                     =   5,
	I2C0_IRQn                     =   6,
	SPI0_IRQn                     =   7,
	SARADC_IRQn                   =   8,
	IRDA_IRQn                     =   9,
	GDMA_IRQn                     =  11,
	LA_IRQn                       =  12,
	ACOMP0_IRQn                   =  13,
	ACOMP1_IRQn                   =  14,
	UART1_IRQn                    =  15,
	CPU0_FPU_IRQn                 =  16,
	CPU1_FPU_IRQn                 =  17,
	CAN_IRQn                      =  18,
	L2CACHE_ERR_IRQn              =  19,
	VID_DISP0_IRQn                =  20,
	CKMN_IRQn                     =  21,
	VID_DISP1_IRQn                =  22,
	AUDIO_IRQn                    =  23,
	I2S0_IRQn                     =  24,
	I2S1_IRQn                     =  25,
	VID_DISP2_IRQn                =  26,
	IPC_CHKSUM_IRQn               =  27,
	THREAD_IRQn                   =  28,
	MODEM_IRQn                    =  29,
	MODEM_RC_IRQn                 =  30,
	MAC_TXRX_TIMER_IRQn           =  31,
	MAC_TXRX_MISC_IRQn            =  32,
	MAC_RX_TRIGGER_IRQn           =  33,
	MAC_TX_TRIGGER_IRQn           =  34,
	MAC_PROT_TRIGGER_IRQn         =  35,
	MAC_GENERAL_IRQn              =  36,
	GPIO_NS_IRQn                  =  37,
	MAC_WAKEUP_IRQn               =  38,
	BTDM_IRQn                     =  39,
	BLE_IRQn                      =  40,
	BT_IRQn                       =  41,
	BTDM_WAKE_UP_IRQn             =  42,
	TOUCHED_IRQn                  =  43,
	I2S2_IRQn                     =  44,
	I2S3_IRQn                     =  45,
	SPDIF0_IRQn                   =  46,
	CEC_IRQn                      =  47,
	XDAC0_IRQn                    =  48,
	XDAC1_IRQn                    =  49,
	OTP_IRQn                      =  50,
	PLL_UNLOCK_IRQn               =  51,
	DCO_UNLOCK_IRQn               =  52,
	USB_PLUG_IRQn                 =  53,
	RTC_IRQn                      =  54,
	GPIO_IRQn                     =  55,
	UART2_IRQn                    =  56,
	SPI1_IRQn                     =  57,
	TIMER1_IRQn                   =  58,    /* HW Timer1 module (TIMER_ID3~5) */
	SPI3_IRQn                     =  59,
	SCR_IRQn                      =  60,
	LIN_IRQn                      =  61,
	CAN1_IRQn                     =  62,
	TIMER2_IRQn                   =  63,    /* HW Timer2 module (TIMER_ID6~8) */
	TIMER3_IRQn                   =  64,    /* HW Timer3 module (TIMER_ID9~11) */
	UART3_IRQn                    =  65,
	SPI2_IRQn                     =  66,
	UART4_IRQn                    =  67,
	I2C3_IRQn                     =  68,
	HSPL_IRQn                     =  69,
	BK24_IRQn                     =  70,
	IRDA1_IRQn                    =  71,
	IRDA2_IRQn                    =  72,
	IRDA3_IRQn                    =  73,
	I3C_IRQn                      =  74,
	I2S4_IRQn                     =  75,
	SPDIF1_IRQn                   =  76,
	INT_M55SUB_IRQn               =  77,
	MBOX_IRQn                     =  78,
	IPI_IRQn                      =  79,
	VID_DISP3_IRQn                =  80,
	VAD_IRQn                      =  81,

	InterruptMAX_IRQn
} IRQn_Type;
