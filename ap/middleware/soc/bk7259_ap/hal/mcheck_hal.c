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

#include "hal_config.h"
#include "trng_hw.h"
#include "trng_hal.h"
#include "trng_ll.h"

#if (!CONFIG_ATE_TEST)
static int mcheck_section(uint32_t* reg_addr, uint32_t base_addr, int reg_points_num, int output_points_num, int addr_len, int singe_repair_points)
{
    int reg_index, output_index;
    uint32_t reg_value;

    for (output_index=0, reg_index=0; reg_index< reg_points_num; reg_index++)
    {
        // keep the low part of register
        reg_value = reg_addr[reg_index] & 0x0000FFFF;

        // confirm whether the address is valid
        if ((reg_value >> (addr_len-1)) % 2 == 1)
        {
            // if address < 16 bits, need to convert to OTP address
            if(addr_len < 16)
            {
                reg_value |= 0x00008000;              //firstly set the highest bit
                reg_value &= ~(1 << (addr_len-1));    // clear the original valid bit
                //reg_value |=  (reg_index / singe_repair_points) << (addr_len-1);    // calculate the index and set corresbonding bits
            }

            output_index += 1;
            BK_LOGD(NULL, "%08x\r\n", base_addr + reg_value * 4);
        }
    }

    return output_index;
}

int mcheck_ate(uint16_t check_res[48])
{
    uint32_t* reg_start_addr;
    int res = -1;

    reg_start_addr = (uint32_t*)SOC_MEM_CHECK_REG_BASE;

    // SMEM2, 4 points, no share
    res = mcheck_section(reg_start_addr + 8, SOC_SRAM2_DATA_BASE, 4, 4, 16, 4);

    // SMEM3, 4 points, no share
    res += mcheck_section(reg_start_addr + 12, SOC_SRAM3_DATA_BASE, 4, 4, 16, 4);

    // SMEM4, 4 points, no share
    res += mcheck_section(reg_start_addr + 16, SOC_SRAM4_DATA_BASE, 4, 4, 16, 4);

    // SMEM5, 4 points, no share
    res += mcheck_section(reg_start_addr + 20, SOC_SRAM5_DATA_BASE, 4, 4, 16, 4);

    // h265, 4 points, no share
    res += mcheck_section(reg_start_addr + 24, SOC_SRAM0_DATA_BASE, 4, 4, 16, 4);

    // SMEM0-1 overall 8 points, share 4 points
    res += mcheck_section(reg_start_addr + 28, SOC_SRAM0_DATA_BASE, 8, 4, 15, 4);

   // cpu0_0->cpu2_0 overall 12 points, share 6 points
    res += mcheck_section(reg_start_addr + 36, 0, 12, 6, 14, 4);

   // cpu0_1->cpu1_3 overall 16 points, share 8 points
    res += mcheck_section(reg_start_addr + 48, 0, 16, 8, 13, 2);

   // cpu2_3->pram overall 14 points, share 8 points
    res += mcheck_section(reg_start_addr + 64, SOC_PSRAM_DATA_BASE, 14, 8, 13, 2);

   // usb2->dmad overall 4 points, share 2 points
    // res += mcheck_section(reg_start_addr + 78, SOC_DMA2D_REG_BASE, 4, 2, 12, 2);

    return res;
}

void hal_mcheck(void)
{
	uint16_t check_res[48];
	if (mcheck_ate(check_res)) {
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "Memory Check Exception!!!Please Contact Digital Team!!!\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
		BK_LOGD(NULL, "=============================================================================================================\r\n");
	}
}
#endif
