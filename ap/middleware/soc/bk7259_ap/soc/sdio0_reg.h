// Copyright 2022-2025 Beken
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


#define SDIO0_DEV_ID_ADDR (SOC_SDIO0_REG_BASE + (0x0 << 2))

#define SDIO0_DEV_ID_DEVICEID_POS (0)
#define SDIO0_DEV_ID_DEVICEID_MASK (0xffffffff)

#define SDIO0_VER_ID_ADDR (SOC_SDIO0_REG_BASE + (0x1 << 2))

#define SDIO0_VER_ID_VERSIONID_POS (0)
#define SDIO0_VER_ID_VERSIONID_MASK (0xffffffff)

#define SDIO0_CLKG_RESET_ADDR (SOC_SDIO0_REG_BASE + (0x2 << 2))

#define SDIO0_CLKG_RESET_SOFT_RESETN_POS (0)
#define SDIO0_CLKG_RESET_SOFT_RESETN_MASK (0x1)

#define SDIO0_CLKG_RESET_BPS_CLKGATE_POS (1)
#define SDIO0_CLKG_RESET_BPS_CLKGATE_MASK (0x1)

#define SDIO0_CLKG_RESET_RESERVED_BIT_2_31_POS (2)
#define SDIO0_CLKG_RESET_RESERVED_BIT_2_31_MASK (0x3fffffff)

#define SDIO0_STATUS_ADDR (SOC_SDIO0_REG_BASE + (0x3 << 2))

#define SDIO0_STATUS_GLOBALSTATUS_POS (0)
#define SDIO0_STATUS_GLOBALSTATUS_MASK (0xffffffff)

#define SDIO0_DIV_CTRL_ADDR (SOC_SDIO0_REG_BASE + (0x4 << 2))

#define SDIO0_DIV_CTRL_TMCLK_DIV_POS (0)
#define SDIO0_DIV_CTRL_TMCLK_DIV_MASK (0xff)

#define SDIO0_DIV_CTRL_CQET_MCLK_DIV_POS (8)
#define SDIO0_DIV_CTRL_CQET_MCLK_DIV_MASK (0xff)

#define SDIO0_DIV_CTRL_RESERVED_BIT_16_31_POS (16)
#define SDIO0_DIV_CTRL_RESERVED_BIT_16_31_MASK (0xffff)

#define SDIO0_SDIO_CTRL_ADDR (SOC_SDIO0_REG_BASE + (0x5 << 2))

#define SDIO0_SDIO_CTRL_CARD_WRITE_PROT_POS (0)
#define SDIO0_SDIO_CTRL_CARD_WRITE_PROT_MASK (0x1)

#define SDIO0_SDIO_CTRL_CARD_DETECT_N_POS (1)
#define SDIO0_SDIO_CTRL_CARD_DETECT_N_MASK (0x1)

#define SDIO0_SDIO_CTRL_LED_CONTROL_POS (2)
#define SDIO0_SDIO_CTRL_LED_CONTROL_MASK (0x1)

#define SDIO0_SDIO_CTRL_SD_DATXFER_WIDTH_POS (3)
#define SDIO0_SDIO_CTRL_SD_DATXFER_WIDTH_MASK (0x3)

#define SDIO0_SDIO_CTRL_TUNING_RX_SEL0_POS (5)
#define SDIO0_SDIO_CTRL_TUNING_RX_SEL0_MASK (0x7)

#define SDIO0_SDIO_CTRL_TUNING_RX_SEL1_POS (8)
#define SDIO0_SDIO_CTRL_TUNING_RX_SEL1_MASK (0x7)

#define SDIO0_SDIO_CTRL_RESERVED_11_13_POS (11)
#define SDIO0_SDIO_CTRL_RESERVED_11_13_MASK (0x7)

#define SDIO0_SDIO_CTRL_SAMPLE_RX_SEL0_POS (14)
#define SDIO0_SDIO_CTRL_SAMPLE_RX_SEL0_MASK (0x1)

#define SDIO0_SDIO_CTRL_SAMPLE_RX_SEL1_POS (15)
#define SDIO0_SDIO_CTRL_SAMPLE_RX_SEL1_MASK (0x1)

#define SDIO0_SDIO_CTRL_RESERVED_16_16_POS (16)
#define SDIO0_SDIO_CTRL_RESERVED_16_16_MASK (0x1)

#define SDIO0_SDIO_CTRL_TUNING_TX_SEL0_POS (17)
#define SDIO0_SDIO_CTRL_TUNING_TX_SEL0_MASK (0x7)

#define SDIO0_SDIO_CTRL_TUNING_TX_SEL1_POS (20)
#define SDIO0_SDIO_CTRL_TUNING_TX_SEL1_MASK (0x7)

#define SDIO0_SDIO_CTRL_RESERVED_23_25_POS (23)
#define SDIO0_SDIO_CTRL_RESERVED_23_25_MASK (0x7)

#define SDIO0_SDIO_CTRL_SAMPLE_TX_SEL0_POS (26)
#define SDIO0_SDIO_CTRL_SAMPLE_TX_SEL0_MASK (0x1)

#define SDIO0_SDIO_CTRL_SAMPLE_TX_SEL1_POS (27)
#define SDIO0_SDIO_CTRL_SAMPLE_TX_SEL1_MASK (0x1)

#define SDIO0_SDIO_CTRL_RESERVED_28_28_POS (28)
#define SDIO0_SDIO_CTRL_RESERVED_28_28_MASK (0x1)

#define SDIO0_SDIO_CTRL_CLK_DRV_NEGEDGE_SEL_POS (29)
#define SDIO0_SDIO_CTRL_CLK_DRV_NEGEDGE_SEL_MASK (0x1)

#define SDIO0_SDIO_CTRL_RESERVED_30_31_POS (30)
#define SDIO0_SDIO_CTRL_RESERVED_30_31_MASK (0x3)

#define SDIO0_PROT_CTRL_ADDR (SOC_SDIO0_REG_BASE + (0x6 << 2))

#define SDIO0_PROT_CTRL_MHPROT_POS (0)
#define SDIO0_PROT_CTRL_MHPROT_MASK (0xf)

#define SDIO0_PROT_CTRL_MHPROT_SEL_POS (4)
#define SDIO0_PROT_CTRL_MHPROT_SEL_MASK (0x1)

#define SDIO0_PROT_CTRL_RESERVED_5_31_POS (5)
#define SDIO0_PROT_CTRL_RESERVED_5_31_MASK (0x7ffffff)

#ifdef __cplusplus
}
#endif
