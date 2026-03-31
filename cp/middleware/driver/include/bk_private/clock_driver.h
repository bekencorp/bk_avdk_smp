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

#include <components/log.h>
#include "clock_hal.h"

#include "sys_hal.h"

#define clk_set_uart_clk_26m(id)    sys_hal_uart_select_clock(id,UART_SCLK_XTAL_26M)
#define clk_set_spi_clk_26m(id)     //sys_hal_set_clksel_spi(SPI_CLK_SRC_XTAL)
#define clk_set_spi_clk_dco(id)     //sys_hal_set_clksel_spi(SPI_CLK_SRC_UNKNOW)//spi don't support DCO clock source

#define clk_enable_saradc_audio_pll()   //do nothing
#define clk_disable_saradc_audio_pll()  //do nothing
#define clk_set_saradc_clk_26m()	sys_hal_set_cksel_sadc(0)
#define clk_set_saradc_clk_dco()	sys_hal_set_cksel_sadc(1)

#define clk_set_pwms_clk_26m()		sys_hal_set_cksel_pwm(1)
#define clk_set_pwms_clk_dco()		//pwm don't support DCO clock source
#define clk_enable_pwm_clk_lpo(chan)	//pwm don't support lpo clock source
#define clk_disable_pwm_clk_lpo(chan)	//pwm don't support lpo clock source

#define clk_get_uart_clk(id)        sys_hal_uart_select_clock_get(id)
// eof
