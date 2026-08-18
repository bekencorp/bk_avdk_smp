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

#include "driver/efuse.h"
#include "bk_efuse.h"

#define EFUSE_SECUREBOOT_ENABLED_BIT        (3)
#define EFUSE_SPE_DEBUG_DISABLED_BIT        (31)
#define EFUSE_INFO_DISABLED_BIT             (8)
#define EFUSE_ERR_DISABLED_BIT              (9)
#define EFUSE_SECURE_DOWNLOAD_DISABLED_BIT  (12)
#define EFUSE_FLASH_AES_ENABLED_BIT         (27)

#if CONFIG_SOC_BK7236N || CONFIG_SOC_BK7239N
#define  EFUSE_OF_OTP_VAL                   *((volatile unsigned int *) (0x44890000 + 0x40 * 4))
#define GET_BIT_VAL(val, bit)               ((val >> bit) & 1)
#define SET_BIT_VAL(val, bit, new_val)      ((val & (~ (1 << bit)))|(new_val << bit))
#endif

#define TAG "efuse" 

static uint32_t s_efuse_data = 0;

#if CONFIG_SOC_BK7236N || CONFIG_SOC_BK7239N
int bk_efuse_init(void)
{
	if(GET_BIT_VAL(REG_READ(SOC_SYSTEM_REG_BASE + 0xC*4), 15) == 1 )          //check otp cken
	{
			if(GET_BIT_VAL(REG_READ(SOC_SYSTEM_REG_BASE + 0x10*4), 3) == 0) //check encp power on
			{
					s_efuse_data = REG_READ(SOC_OTP_REG_BASE + 0x10f*4);    //read otp_efuse val
			}
	}
	else
	{
			SET_BIT_VAL(REG_READ(SOC_SYSTEM_REG_BASE + 0xC*4), 15 , 1);
			timer_delay_us(10);
			SET_BIT_VAL(REG_READ(SOC_SYSTEM_REG_BASE + 0x10*4), 3, 0);
			timer_delay_us(10);
			s_efuse_data = REG_READ(SOC_OTP_REG_BASE + 0x10f*4);
	}

	return BK_OK;
}

bool efuse_is_secureboot_enabled(void)
{
#if CONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE
	return !!(EFUSE_OF_OTP_VAL & BIT(EFUSE_SECUREBOOT_ENABLED_BIT));
#else
	return true;
#endif
}

bool efuse_is_spe_debug_enabled(void)
{
	return !(EFUSE_OF_OTP_VAL & BIT(EFUSE_SPE_DEBUG_DISABLED_BIT));
}

bool efuse_is_info_log_enabled(void)
{
	return !(EFUSE_OF_OTP_VAL & BIT(EFUSE_INFO_DISABLED_BIT));
}

bool efuse_is_err_log_enabled(void)
{
	return !(EFUSE_OF_OTP_VAL & BIT(EFUSE_ERR_DISABLED_BIT));
}

bool efuse_is_secure_download_enabled(void)
{
	return !(EFUSE_OF_OTP_VAL & BIT(EFUSE_SECURE_DOWNLOAD_DISABLED_BIT));
}

bool efuse_is_flash_aes_enabled(void)
{
	return !!(EFUSE_OF_OTP_VAL & BIT(EFUSE_FLASH_AES_ENABLED_BIT));
}

void dump_efuse(void)
{
	BK_LOGI(TAG, "efuse=%x\r\n", EFUSE_OF_OTP_VAL);
}

uint32_t efuse_get_value(void)
{
        return EFUSE_OF_OTP_VAL;

}

#else

int bk_efuse_init(void)
{
	uint8_t *efuse_byte_p = (uint8_t*)&s_efuse_data;
	int offset;

	bk_efuse_driver_init();
	for (offset = 0; offset < 4; offset ++) {
		bk_efuse_read_byte(offset, efuse_byte_p);
		efuse_byte_p++;
	}

	return BK_OK;
}

bool efuse_is_secureboot_enabled(void)
{
#if CONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE
	return !!(s_efuse_data & BIT(EFUSE_SECUREBOOT_ENABLED_BIT));
#else
	return true;
#endif
}

bool efuse_is_spe_debug_enabled(void)
{
	return !(s_efuse_data & BIT(EFUSE_SPE_DEBUG_DISABLED_BIT));
}

bool efuse_is_info_log_enabled(void)
{
	return !(s_efuse_data & BIT(EFUSE_INFO_DISABLED_BIT));
}

bool efuse_is_err_log_enabled(void)
{
	return !(s_efuse_data & BIT(EFUSE_ERR_DISABLED_BIT));
}

bool efuse_is_secure_download_enabled(void)
{
	return !(s_efuse_data & BIT(EFUSE_SECURE_DOWNLOAD_DISABLED_BIT));
}

bool efuse_is_flash_aes_enabled(void)
{
	return !!(s_efuse_data & BIT(EFUSE_FLASH_AES_ENABLED_BIT));
}

void dump_efuse(void)
{
	printf("efuse=%x\r\n", s_efuse_data);
}

uint32_t efuse_get_value(void)
{
        return s_efuse_data;

}
#endif