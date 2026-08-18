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

#include <stdint.h>
#include "hal_hw_fih.h"
#include "fih.h"
#include "bk_tfm_log.h"
#include "partitions_gen.h"

bool s_is_crypto_inited = false;

#define TAG "FIH"
#if CONFIG_HW_FIH

#if CONFIG_BL2_HW_FIH
#define FIH_CMP_REG_START	PRRO_REG1B_CMP1_ADDR_START
#define FIH_CMP_REG_END		PRRO_REG1C_CMP1_ADDR_END
#define FIH_CMP_DATA_SRC	PRRO_REG1D_CMP1_DATA_SRC
#define FIH_CMP_DATA_DST	PRRO_REG1E_CMP1_DATA_DST

#define FIH_CMP_ADDR_SRC	(SOC_FLASH_DATA_BASE + CONFIG_PRIMARY_TFM_S_VIRTUAL_CODE_START)
#define FIH_CMP_ADDR_DST	(FIH_CMP_ADDR_SRC + CONFIG_PRIMARY_TFM_S_VIRTUAL_CODE_SIZE)

#elif CONFIG_TFM_HW_FIH

#define FIH_CMP_REG_START	PRRO_REG1F_CMP2_ADDR_START
#define FIH_CMP_REG_END		PRRO_REG20_CMP2_ADDR_END
#define FIH_CMP_DATA_SRC	PRRO_REG21_CMP2_DATA_SRC
#define FIH_CMP_DATA_DST	PRRO_REG22_CMP2_DATA_DST

#define FIH_CMP_ADDR_SRC	(SOC_FLASH_DATA_BASE + CONFIG_PRIMARY_TFM_NS_VIRTUAL_CODE_START)
#define FIH_CMP_ADDR_DST	(FIH_CMP_ADDR_SRC + CONFIG_PRIMARY_TFM_NS_VIRTUAL_CODE_SIZE)
#else
#error "CONFIG_BL2_HW_FIH or CONFIG_TFM_HW_FIH is not defined!!!"
#endif /* CONFIG_BL2_HW_FIH and  CONFIG_TFM_HW_FIH */

static inline void hw_fih_delay(void)
{
	if (s_is_crypto_inited == true) {
		fih_delay();
	} else {
		BK_LOGD(TAG, "hw_fih_delay: not delay\r\n");
	}
}

void bk_fih_set_src(uint32_t id, uint32_t data)
{
	hw_fih_delay();
	switch (id) {
	case FIH_DATA_PUBLIC_KEY_HASH:
		REG_SET_FIELD(FIH_CMP_DATA_SRC, FIH_DATA_PUBLIC_KEY_HASH, data);
		break;
	case FIH_DATA_SIG:
		REG_SET_FIELD(FIH_CMP_DATA_SRC, FIH_DATA_SIG, data);
		break;
	case FIH_DATA_IMG_HASH:
		REG_SET_FIELD(FIH_CMP_DATA_SRC, FIH_DATA_IMG_HASH, data);
		break;
	case FIH_DATA_MSP_PC:
		REG_SET_FIELD(FIH_CMP_DATA_SRC, FIH_DATA_MSP_PC, data);
		break;
	case FIH_DATA_BOOT_TYPE:
		REG_SET_FIELD(FIH_CMP_DATA_SRC, FIH_DATA_BOOT_TYPE, data);
		break;
	case FIH_DATA_BOOT_FLAG:
		REG_SET_FIELD(FIH_CMP_DATA_SRC, FIH_DATA_BOOT_FLAG, data);
		break;
	case FIH_DATA_LCS:
		REG_SET_FIELD(FIH_CMP_DATA_SRC, FIH_DATA_LCS, data);
		break;
	case FIH_DATA_EFUSE:
		REG_SET_FIELD(FIH_CMP_DATA_SRC, FIH_DATA_EFUSE, data);
		break;
	default:
		break;
	}

	// BK_LOGI(TAG, "set src, id=%d data=%x src=%x\n", id, data, REG_READ(FIH_CMP_DATA_SRC));
}

void bk_fih_set_dst(uint32_t id, uint32_t data)
{
	hw_fih_delay();
	switch (id) {
	case FIH_DATA_PUBLIC_KEY_HASH:
		REG_SET_FIELD(FIH_CMP_DATA_DST, FIH_DATA_PUBLIC_KEY_HASH, data);
		break;
	case FIH_DATA_SIG:
		REG_SET_FIELD(FIH_CMP_DATA_DST, FIH_DATA_SIG, data);
		break;
	case FIH_DATA_IMG_HASH:
		REG_SET_FIELD(FIH_CMP_DATA_DST, FIH_DATA_IMG_HASH, data);
		break;
	case FIH_DATA_MSP_PC:
		REG_SET_FIELD(FIH_CMP_DATA_DST, FIH_DATA_MSP_PC, data);
		break;
	case FIH_DATA_BOOT_TYPE:
		REG_SET_FIELD(FIH_CMP_DATA_DST, FIH_DATA_BOOT_TYPE, data);
		break;
	case FIH_DATA_BOOT_FLAG:
		REG_SET_FIELD(FIH_CMP_DATA_DST, FIH_DATA_BOOT_FLAG, data);
		break;
	case FIH_DATA_LCS:
		REG_SET_FIELD(FIH_CMP_DATA_DST, FIH_DATA_LCS, data);
		break;
	case FIH_DATA_EFUSE:
		REG_SET_FIELD(FIH_CMP_DATA_DST, FIH_DATA_EFUSE, data);
		break;
	default:
		break;
	}

	// BK_LOGI(TAG, "set dst, id=%d data=%x dst=%x\n", id, data, REG_READ(FIH_CMP_DATA_DST));
}

int bk_fih_set_addr_range(uint32_t start, uint32_t end)
{
	int ret = -1;

	REG_WRITE(FIH_CMP_REG_START, start);
	hw_fih_delay();
	REG_WRITE(FIH_CMP_REG_END, end);
	BK_LOGD(TAG, "fih: addr range, start=%x end=%x\r\n", start, end);
#if CONFIG_BL2_HW_FIH
	REG_WRITE(PRRO_REG1F_CMP2_ADDR_START, 0x02e00000);
	REG_WRITE(PRRO_REG20_CMP2_ADDR_END, 0x02F00000);
#endif
	return (ret = 0);
}

int bk_fih_init(void)
{
	int ret = -1;

	PRRO_SOFT_RESET;

	ret = bk_fih_set_addr_range(FIH_CMP_ADDR_SRC, FIH_CMP_ADDR_DST);

	BK_LOGD(TAG, "fih: init\r\n");
	return ret;
}

void bk_fih_disable(void)
{
	REG_WRITE(PRRO_REG19_CMP0_DATA_SRC, 0);
	REG_WRITE(PRRO_REG1A_CMP0_DATA_DST, 0);
	REG_WRITE(PRRO_REG1D_CMP1_DATA_SRC, 0);
	REG_WRITE(PRRO_REG1E_CMP1_DATA_DST, 0);
	REG_WRITE(PRRO_REG21_CMP2_DATA_SRC, 0);
	REG_WRITE(PRRO_REG22_CMP2_DATA_DST, 0);
}


void bk_fih_validate(void)
{
	// dump_prro_regs();
	if (REG_READ(FIH_CMP_DATA_SRC) != REG_READ(FIH_CMP_DATA_DST)) {
		BK_LOGI(TAG, "cmp0 mismatch, reboot\r\n");
#if CONFIG_SOC_BK7236N || CONFIG_SOC_BK7239N
		update_wdt(0x0A);
#else
		update_aon_wdt(0x0A);
#endif
	}
}
#endif

void dump_prro_regs(void)
{
	BK_LOGI(TAG, "prro cmp0_addr_start=%x\r\n", REG_READ(PRRO_REG17_CMP0_ADDR_START));
	BK_LOGI(TAG, "prro cmp0_addr_end=%x\r\n", REG_READ(PRRO_REG18_CMP0_ADDR_END));
	BK_LOGI(TAG, "prro cmp0_data_src=%x\r\n", REG_READ(PRRO_REG19_CMP0_DATA_SRC));
	BK_LOGI(TAG, "prro cmp0_data_dst=%x\r\n", REG_READ(PRRO_REG1A_CMP0_DATA_DST));
	BK_LOGI(TAG, "prro cmp1_addr_start=%x\r\n", REG_READ(PRRO_REG1B_CMP1_ADDR_START));
	BK_LOGI(TAG, "prro cmp1_addr_end=%x\r\n", REG_READ(PRRO_REG1C_CMP1_ADDR_END));
	BK_LOGI(TAG, "prro cmp1_data_src=%x\r\n", REG_READ(PRRO_REG1D_CMP1_DATA_SRC));
	BK_LOGI(TAG, "prro cmp1_data_dst=%x\r\n", REG_READ(PRRO_REG1E_CMP1_DATA_DST));
	BK_LOGI(TAG, "prro cmp2_addr_start=%x\r\n", REG_READ(PRRO_REG1F_CMP2_ADDR_START));
	BK_LOGI(TAG, "prro cmp2_addr_end=%x\r\n", REG_READ(PRRO_REG20_CMP2_ADDR_END));
	BK_LOGI(TAG, "prro cmp2_data_src=%x\r\n", REG_READ(PRRO_REG21_CMP2_DATA_SRC));
	BK_LOGI(TAG, "prro cmp2_data_dst=%x\r\n", REG_READ(PRRO_REG22_CMP2_DATA_DST));
	BK_LOGI(TAG, "prro mismatch int=%x\r\n", REG_READ(PRRO_REG23_CMP_INT_STATUS));
}

