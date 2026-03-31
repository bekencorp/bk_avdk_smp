// Copyright 2022-2023 Beken
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

// This is a generated file, if you need to modify it, use the script to
// generate and modify all the struct.h, ll.h, reg.h, debug_dump.c files!

#pragma once


#ifdef __cplusplus
extern "C" {
#endif


#define WWDT_SMB_DEVID_ADDR (SOC_WWDT_REG_BASE + (0x0 << 2))

#define WWDT_SMB_DEVID_DEVICE_ID_POS (0)
#define WWDT_SMB_DEVID_DEVICE_ID_MASK (0xffffffff)

#define WWDT_SMB_VERID_ADDR (SOC_WWDT_REG_BASE + (0x1 << 2))

#define WWDT_SMB_VERID_VERSION_ID_POS (0)
#define WWDT_SMB_VERID_VERSION_ID_MASK (0xffffffff)

#define WWDT_SMB_CLKRST_ADDR (SOC_WWDT_REG_BASE + (0x2 << 2))

#define WWDT_SMB_CLKRST_SOFT_RESET_POS (0)
#define WWDT_SMB_CLKRST_SOFT_RESET_MASK (0x1)

#define WWDT_SMB_CLKRST_CLKG_BYPASS_POS (1)
#define WWDT_SMB_CLKRST_CLKG_BYPASS_MASK (0x1)

#define WWDT_SMB_CLKRST_RESV_POS (2)
#define WWDT_SMB_CLKRST_RESV_MASK (0x3fffffff)

#define WWDT_SMB_STATE_ADDR (SOC_WWDT_REG_BASE + (0x3 << 2))

#define WWDT_SMB_STATE_DEV_STATUS_POS (0)
#define WWDT_SMB_STATE_DEV_STATUS_MASK (0xffffffff)

#define WWDT_WDT_CONFIG_ADDR (SOC_WWDT_REG_BASE + (0x4 << 2))

#define WWDT_WDT_CONFIG_PERIOD_POS (0)
#define WWDT_WDT_CONFIG_PERIOD_MASK (0xffff)
#define WWDT_F_PERIOD_V             (0xffff)
#define WWDT_F_PERIOD_MIN_V         (0xA)
#define WWDT_F_PERIOD_MAX_V         (0xfff0)
#define WWDT_V_KEY_1ST               (0x5A)
#define WWDT_V_KEY_2ND               (0xA5)

#define WWDT_WDT_CONFIG_KEY_POS (16)
#define WWDT_WDT_CONFIG_KEY_MASK (0xff)

#define WWDT_WDT_CONFIG_RESERVED_BIT_24_31_POS (24)
#define WWDT_WDT_CONFIG_RESERVED_BIT_24_31_MASK (0xff)

#define WWDT_WDT_CNT_ADDR (SOC_WWDT_REG_BASE + (0x5 << 2))

#define WWDT_WDT_CNT_COUNT_POS (0)
#define WWDT_WDT_CNT_COUNT_MASK (0xffff)

#define WWDT_WDT_CNT_RESERVED_BIT_16_31_POS (16)
#define WWDT_WDT_CNT_RESERVED_BIT_16_31_MASK (0xffff)

#define WWDT_WDT_WIN_SET_ADDR (SOC_WWDT_REG_BASE + (0x6 << 2))

#define WWDT_WDT_WIN_SET_WIN_VAL_POS (0)
#define WWDT_WDT_WIN_SET_WIN_VAL_MASK (0xffff)

#define WWDT_WDT_WIN_SET_WIN_KEY_POS (16)
#define WWDT_WDT_WIN_SET_WIN_KEY_MASK (0xff)

#define WWDT_WDT_WIN_SET_WIN_EN_POS (24)
#define WWDT_WDT_WIN_SET_WIN_EN_MASK (0x1)

#define WWDT_WDT_WIN_SET_RESERVED_BIT_25_31_POS (25)
#define WWDT_WDT_WIN_SET_RESERVED_BIT_25_31_MASK (0x7f)

#define WWDT_CPUID_ADDR (SOC_WWDT_REG_BASE + (0x7 << 2))

#define WWDT_CPUID_CPU_ID_POS (0)
#define WWDT_CPUID_CPU_ID_MASK (0xf)

#define WWDT_CPUID_MAGIC_WORD_POS (4)
#define WWDT_CPUID_MAGIC_WORD_MASK (0xf)
#define WWDT_CPUID_VALID_MAGIC_WORD (0xc)

#define WWDT_CPUID_RESERVED_BIT_8_31_POS (8)
#define WWDT_CPUID_RESERVED_BIT_8_31_MASK (0xffffff)
#define WWDT_V_PERIOD_DEFAULT_VALUE       (0x100)

#ifdef __cplusplus
}
#endif
