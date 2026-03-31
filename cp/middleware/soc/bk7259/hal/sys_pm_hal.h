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

#include <soc/soc.h>
#include "sys_pm_hal_debug.h"
#include "sys_pm_hal_ctrl.h"

#define ROSC_PPM_NOMINAL (300)
//TODO may need to adjust the interrupt bits
#define WS_WIFI_INT0    (BIT(29) | BIT(30) | BIT(31))
#define WS_WIFI_INT1    (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(6))
#define WS_BT_INT       (BIT(7) | BIT(8) | BIT(9))
#define WS_TOUCH_INT    BIT(20)
#define WS_USBPLUG_INT  BIT(21)
#define WS_RTC_INT      BIT(22)
#define WS_GPIO_INT     BIT(23)

#define WS_GPIO         BIT(0)
#define WS_RTC          BIT(1)
#define WS_WIFI         BIT(2)
#define WS_BT           BIT(3)
#define WS_USBPLUG      BIT(4)
#define WS_TOUCH        BIT(5)

#define WS_CPU0_INT_0_31_MASK  WS_WIFI_INT0
#define WS_CPU0_INT_32_64_MASK (WS_WIFI_INT1 | WS_BT_INT | WS_TOUCH_INT | WS_USBPLUG_INT | WS_RTC_INT | WS_GPIO_INT)

#define PD_ALL         (0xF)
#define PD_CPU1        (BIT(0))
#define PD_VEHP        (BIT(1))
#define PD_WRLS        (BIT(2))
#define PD_ROM         (BIT(3))

#define WRLP_BT        (BIT(0))
#define WRLP_WF        (BIT(1))

#define MEM_SRAM_0     (BIT(25))
#define MEM_SRAM_1     (BIT(26))
#define MEM_SRAM_2     (BIT(27))

#define PERI_SCR1      (BIT(31))
#define PERI_SCR0      (BIT(30))
#define PERI_LIN1      (BIT(29))
#define PERI_LIN0      (BIT(28))
#define PERI_I2S3      (BIT(26))
#define PERI_TIM3      (BIT(25))
#define PERI_TIM2      (BIT(24))
#define PERI_CAN1      (BIT(23))
#define PERI_CAN0      (BIT(22))
#define PERI_IRDA1     (BIT(21))
#define PERI_IRDA0     (BIT(20))
#define PERI_TIM1      (BIT(19))
#define PERI_TIM0      (BIT(18))
#define PERI_I3C       (BIT(17))
#define PERI_PWM0      (BIT(16))
#define PERI_OTP       (BIT(15))
#define PERI_SADC      (BIT(14))
#define PERI_I2S2      (BIT(13))
#define PERI_I2S1      (BIT(12))
#define PERI_I2S0      (BIT(11))
#define PERI_SPI2      (BIT(10))
#define PERI_SPI1      (BIT(9))
#define PERI_SPI0      (BIT(8))
#define PERI_UART3     (BIT(7))
#define PERI_UART2     (BIT(6))
#define PERI_UART1     (BIT(5))
#define PERI_UART0     (BIT(4))
#define PERI_I2C3      (BIT(3))
#define PERI_I2C2      (BIT(2))
#define PERI_I2C1      (BIT(1))
#define PERI_I2C0      (BIT(0))

#define PERI_THREAD    (BIT(27))
#define PERI_PHY       (BIT(26))
#define PERI_MAC       (BIT(25))
#define PERI_XVER      (BIT(24))
#define PERI_BTDM      (BIT(23))
#define PERI_WLSS      (BIT(22))
#define PERI_CEC       (BIT(10))
#define PERI_AUDIF0    (BIT(3))
#define PERI_AUDIO     (BIT(2))
#define PERI_AUDIF1    (BIT(1))

/*bakp power domain control*/
#if CONFIG_BAKP_POWER_DOMAIN_DISABLE
#define POWER_BAKP   (PD_ALL)
#else
#define POWER_BAKP  ~(PD_CPU1)
#endif

#define PD_DOWN_DOMAIN  (PD_ALL & ~(PD_WRLS) & POWER_BAKP)

#define EN_VOUT         (BIT(0))
#define EN_XTAL         (BIT(1))
#define EN_DCO          (BIT(2))
#define EN_TEMP_GSEL    (BIT(3))
#define EN_TEMP         (BIT(4))
#define EN_DPLL         (BIT(5))
#define EN_CB           (BIT(6))
//#define EN_ALL          (EN_USB | EN_XTAL | EN_DCO | EN_RAM | EN_TEMP | EN_DPLL | EN_LCD)
#define EN_ALL          (EN_XTAL | EN_DCO | EN_TEMP | EN_DPLL | EN_TEMP_GSEL)

#define ENTER_LOWVOL_WAKEUP_PROTECT_TIME 5 // ms