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


#define XDAC0_REG0_ADDR (SOC_XDAC0_REG_BASE + (0x0 << 2))

#define XDAC0_REG0_DEVICEID_POS (0)
#define XDAC0_REG0_DEVICEID_MASK (0xffffffff)

#define XDAC0_REG1_ADDR (SOC_XDAC0_REG_BASE + (0x1 << 2))

#define XDAC0_REG1_VERSIONID_POS (0)
#define XDAC0_REG1_VERSIONID_MASK (0xffffffff)

#define XDAC0_REG2_ADDR (SOC_XDAC0_REG_BASE + (0x2 << 2))

#define XDAC0_REG2_SOFT_RESET_POS (0)
#define XDAC0_REG2_SOFT_RESET_MASK (0x1)

#define XDAC0_REG2_CLKG_BYPASS_POS (1)
#define XDAC0_REG2_CLKG_BYPASS_MASK (0x1)

#define XDAC0_REG2_RESERVED_BIT_2_31_POS (2)
#define XDAC0_REG2_RESERVED_BIT_2_31_MASK (0x3fffffff)

#define XDAC0_REG3_ADDR (SOC_XDAC0_REG_BASE + (0x3 << 2))

#define XDAC0_REG3_DEVSTATUS_POS (0)
#define XDAC0_REG3_DEVSTATUS_MASK (0xffffffff)

#define XDAC0_REG4_ADDR (SOC_XDAC0_REG_BASE + (0x4 << 2))

#define XDAC0_REG4_DAC_ENABLE_POS (0)
#define XDAC0_REG4_DAC_ENABLE_MASK (0x1)

#define XDAC0_REG4_DAC_CLK_EN_POS (1)
#define XDAC0_REG4_DAC_CLK_EN_MASK (0x1)

#define XDAC0_REG4_DAC_MODE_POS (2)
#define XDAC0_REG4_DAC_MODE_MASK (0x1)

#define XDAC0_REG4_FIFO_ENABLE_POS (3)
#define XDAC0_REG4_FIFO_ENABLE_MASK (0x1)

#define XDAC0_REG4_RESERVED_4_15_POS (4)
#define XDAC0_REG4_RESERVED_4_15_MASK (0xfff)

#define XDAC0_REG4_DAC_CLK_DIV_POS (16)
#define XDAC0_REG4_DAC_CLK_DIV_MASK (0xffff)

#define XDAC0_REG5_ADDR (SOC_XDAC0_REG_BASE + (0x5 << 2))

#define XDAC0_REG5_FIFO_EMPTY_INT_POS (0)
#define XDAC0_REG5_FIFO_EMPTY_INT_MASK (0x1)

#define XDAC0_REG5_FIFO_FULL_INT_POS (1)
#define XDAC0_REG5_FIFO_FULL_INT_MASK (0x1)

#define XDAC0_REG5_FIFO_NEAR_FULL_INT_POS (2)
#define XDAC0_REG5_FIFO_NEAR_FULL_INT_MASK (0x1)

#define XDAC0_REG5_FIFO_NEAR_EMPTY_INT_POS (3)
#define XDAC0_REG5_FIFO_NEAR_EMPTY_INT_MASK (0x1)

#define XDAC0_REG5_RESERVED_4_31_POS (4)
#define XDAC0_REG5_RESERVED_4_31_MASK (0xfffffff)

#define XDAC0_REG6_ADDR (SOC_XDAC0_REG_BASE + (0x6 << 2))

#define XDAC0_REG6_FIFO_EMPTY_INT_EN_POS (0)
#define XDAC0_REG6_FIFO_EMPTY_INT_EN_MASK (0x1)

#define XDAC0_REG6_FIFO_FULL_INT_EN_POS (1)
#define XDAC0_REG6_FIFO_FULL_INT_EN_MASK (0x1)

#define XDAC0_REG6_FIFO_NEAR_FULL_INT_EN_POS (2)
#define XDAC0_REG6_FIFO_NEAR_FULL_INT_EN_MASK (0x1)

#define XDAC0_REG6_FIFO_NEAR_EMPTY_INT_EN_POS (3)
#define XDAC0_REG6_FIFO_NEAR_EMPTY_INT_EN_MASK (0x1)

#define XDAC0_REG6_RESERVED_4_31_POS (4)
#define XDAC0_REG6_RESERVED_4_31_MASK (0xfffffff)

#define XDAC0_REG7_ADDR (SOC_XDAC0_REG_BASE + (0x7 << 2))

#define XDAC0_REG7_DAC_RTHRD_POS (0)
#define XDAC0_REG7_DAC_RTHRD_MASK (0x1f)

#define XDAC0_REG7_DAC_WTHRD_POS (5)
#define XDAC0_REG7_DAC_WTHRD_MASK (0x1f)

#define XDAC0_REG7_RESERVED_10_31_POS (10)
#define XDAC0_REG7_RESERVED_10_31_MASK (0x3fffff)

#define XDAC0_REG8_ADDR (SOC_XDAC0_REG_BASE + (0x8 << 2))

#define XDAC0_REG8_TX_FIFO_WR_DATA_POS (0)
#define XDAC0_REG8_TX_FIFO_WR_DATA_MASK (0xfff)

#define XDAC0_REG8_RESERVED_12_31_POS (12)
#define XDAC0_REG8_RESERVED_12_31_MASK (0xfffff)

#ifdef __cplusplus
}
#endif
