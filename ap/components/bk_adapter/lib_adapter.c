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

//TODO optimize it, divide this file to smaller component

#include <stdint.h>
#include <components/log.h>
#include "bk_mcu_ps.h"
#include <os/str.h>
#include "driver/flash.h"
#include <os/os.h>
#include <components/system.h>
#include "modules/wifi_types.h"

int mcu_suppress_and_sleep(uint32_t sleep_ticks)
{
#if CONFIG_MCU_PS
#if (CONFIG_FREERTOS)
        GLOBAL_INT_DECLARATION();
        GLOBAL_INT_DISABLE();
        uint32_t missed_ticks = 0;
        missed_ticks = mcu_power_save(sleep_ticks);
        bk_update_tick(missed_ticks);
        GLOBAL_INT_RESTORE();
#endif
#endif
	return BK_OK;
}

//Called by bkreg_tx()
int bkreg_tx_get_uart_port(void)
{
	int port = bk_get_printf_port();
	return port;
}

//Called by write_cal_result_to_flash() only
void write_cal_result_to_flash_secure_op1(void)
{
        bk_flash_set_protect_type(FLASH_PROTECT_NONE);
}

//Called by write_cal_result_to_flash() only
void write_cal_result_to_flash_secure_op2(void)
{
        bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);
}

void connection_fail_cb(wifi_linkstate_reason_t info)
{
}

void bk_audio_intf_dac_pause(void)
{
}

void bk_audio_intf_adc_pause(void)
{
}

void bk_audio_intf_dac_play(void)
{
}

void bk_audio_intf_adc_play(void)
{
}

void bk_audio_intf_uninit(void)
{
}

void bk_audio_intf_init(void)
{
}

void bk_audio_intf_dac_set_volume(void)
{
}

void bk_audio_intf_dac_set_sample_rate(void)
{
}

bool bk_is_audio_en(void)
{
#if CONFIG_AUDIO
	return true;
#else
	return false;
#endif
}

bool bk_is_audio_adc_en(void)
{
#if CONFIG_AUDIO_ADC
	return true;
#else
	return false;
#endif
}

bool bk_is_audio_dac_en(void)
{
#if CONFIG_AUDIO_DAC
	return true;
#else
	return false;
#endif
}
