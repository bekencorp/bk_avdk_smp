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


#define GPIO_CFG_ADDR (SOC_AON_GPIO_REG_BASE + (0x0 << 2))

#define GPIO_CFG_GPIO_INPUT_POS (0)
#define GPIO_CFG_GPIO_INPUT_MASK (0x1)

#define GPIO_CFG_GPIO_OUTPUT_POS (1)
#define GPIO_CFG_GPIO_OUTPUT_MASK (0x1)

#define GPIO_CFG_GPIO_INPUT_ENA_POS (2)
#define GPIO_CFG_GPIO_INPUT_ENA_MASK (0x1)

#define GPIO_CFG_GPIO_OUTPUT_ENA_POS (3)
#define GPIO_CFG_GPIO_OUTPUT_ENA_MASK (0x1)

#define GPIO_CFG_GPIO_PULL_MODE_POS (4)
#define GPIO_CFG_GPIO_PULL_MODE_MASK (0x1)

#define GPIO_CFG_GPIO_PULL_ENA_POS (5)
#define GPIO_CFG_GPIO_PULL_ENA_MASK (0x1)

#define GPIO_CFG_GPIO_FUN_ENA_POS (6)
#define GPIO_CFG_GPIO_FUN_ENA_MASK (0x1)

#define GPIO_CFG_INPUT_MONITOR_POS (7)
#define GPIO_CFG_INPUT_MONITOR_MASK (0x1)

#define GPIO_CFG_GPIO_CAPACITY_POS (8)
#define GPIO_CFG_GPIO_CAPACITY_MASK (0x3)

#define GPIO_CFG_GPIO_INT_TYPE_POS (10)
#define GPIO_CFG_GPIO_INT_TYPE_MASK (0x3)

#define GPIO_CFG_GPIO_INT_ENA_POS (12)
#define GPIO_CFG_GPIO_INT_ENA_MASK (0x1)

#define GPIO_CFG_GPIO_INT_CLEAR_POS (13)
#define GPIO_CFG_GPIO_INT_CLEAR_MASK (0x1)

#define GPIO_CFG_RESERVED_14_15_POS (14)
#define GPIO_CFG_RESERVED_14_15_MASK (0x3)

#define GPIO_CFG_GPIO_STATE_POS (16)
#define GPIO_CFG_GPIO_STATE_MASK (0x1f)

#define GPIO_CFG_RESERVED_21_23_POS (21)
#define GPIO_CFG_RESERVED_21_23_MASK (0x7)

#define GPIO_CFG_GPIO_FUN_SEL_POS (24)
#define GPIO_CFG_GPIO_FUN_SEL_MASK (0xff)

#define GPIO_INTSTA0_ADDR (SOC_AON_GPIO_REG_BASE + (0x78 << 2))

#define GPIO_INTSTA0_GPIO_INTSTA0_POS (0)
#define GPIO_INTSTA0_GPIO_INTSTA0_MASK (0xffffffff)

#define GPIO_INTSTA1_ADDR (SOC_AON_GPIO_REG_BASE + (0x79 << 2))

#define GPIO_INTSTA1_GPIO_INTSTA1_POS (0)
#define GPIO_INTSTA1_GPIO_INTSTA1_MASK (0xffffffff)

#define GPIO_INTSTA2_ADDR (SOC_AON_GPIO_REG_BASE + (0x7a << 2))

#define GPIO_INTSTA2_GPIO_INTSTA2_POS (0)
#define GPIO_INTSTA2_GPIO_INTSTA2_MASK (0xff)

#define GPIO_INTSTA2_RESERVED_BIT_8_31_POS (8)
#define GPIO_INTSTA2_RESERVED_BIT_8_31_MASK (0xffffff)

#define GPIO_INT_MASK_ADDR (SOC_AON_GPIO_REG_BASE + (0x7c << 2))

#define GPIO_INT_MASK_M52_GPOUP_INT_MASK_POS (0)
#define GPIO_INT_MASK_M52_GPOUP_INT_MASK_MASK (0x1ff)

#define GPIO_INT_MASK_M52_EVEN_INT_MASK_POS (9)
#define GPIO_INT_MASK_M52_EVEN_INT_MASK_MASK (0x1)

#define GPIO_INT_MASK_M52_ODD_INT_MASK_POS (10)
#define GPIO_INT_MASK_M52_ODD_INT_MASK_MASK (0x1)

#define GPIO_INT_MASK_RESERVED_11_15_POS (11)
#define GPIO_INT_MASK_RESERVED_11_15_MASK (0x1f)

#define GPIO_INT_MASK_M55_GPOUP_INT_MASK_POS (16)
#define GPIO_INT_MASK_M55_GPOUP_INT_MASK_MASK (0x1ff)

#define GPIO_INT_MASK_M55_EVEN_INT_MASK_POS (25)
#define GPIO_INT_MASK_M55_EVEN_INT_MASK_MASK (0x1)

#define GPIO_INT_MASK_M55_ODD_INT_MASK_POS (26)
#define GPIO_INT_MASK_M55_ODD_INT_MASK_MASK (0x1)

#define GPIO_INT_MASK_RESERVED_27_31_POS (27)
#define GPIO_INT_MASK_RESERVED_27_31_MASK (0x1f)

#ifdef __cplusplus
}
#endif
