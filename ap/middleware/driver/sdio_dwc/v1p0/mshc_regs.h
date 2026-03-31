// Copyright 2020-2024 Beken
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

//******* MSHC **********//
/* dwc_mshc_map/DWC_mshc_block registers */
#define SDMASA_R(addr)                                *((volatile unsigned int *) (addr + 0x00))
#define BLOCKSIZE_R(addr)                         *((vuint16_t *) (addr + 0x04))
#define SDMA_BUF_BDARY_4K     0 //(0 << 12)
#define SDMA_BUF_BDARY_8K     1 //(1 << 12)
#define SDMA_BUF_BDARY_16K    2 //(2 << 12)
#define SDMA_BUF_BDARY_32K    3 //(3 << 12)
#define SDMA_BUF_BDARY_64K    4 //(4 << 12)
#define SDMA_BUF_BDARY_128K   5 //(5 << 12)
#define SDMA_BUF_BDARY_256K   6 //(6 << 12)
#define SDMA_BUF_BDARY_512K   7 //(7 << 12)

#define BLOCKCOUNT_R(addr)                     *((vuint16_t *)(addr + 0x06))
#define ARGUMENT_R(addr)                       *((volatile unsigned int *)(addr + 0x08))

#define XFER_MODE_R(addr)                          *((vuint16_t *)(addr + 0x0c))
#define XFR_MODE_DMA_EN             BIT(0)
#define XFR_MODE_BLKCNT_EN        BIT(1)
#define XFR_MODE_AUTOCMD12_EN       (0x01 << 2)
#define XFR_MODE_AUTOCMD23_EN       (0x02 << 2)
#define XFR_MODE_AUTOCMD_AUTOSEL    (0x03 << 2)
#define XFR_MODE_DATA_READ        BIT(4)
#define XFR_MODE_MULTBLK_SEL          BIT(5)
#define XFR_MODE_RESP_TYPE        BIT(6)
#define XFR_MODE_RESP_ERRCHK_EN       BIT(7)
#define XFR_MODE_RESP_INT_EN          BIT(8)

#define CMD_R(addr)                                     *((vuint16_t *)(addr + 0x0e))
#define NO_RESP                       (0<<0)
#define RESP_LEN_136                  (1<<0)
#define RESP_LEN_48                   (2<<0)
#define RESP_LEN_48B                    (3<<0)
#define SUB_CMD_FLAG                    BIT(2)
#define CMD_CRC_CHK_ENABLE          BIT(3)
#define CMD_IDX_CHK_ENABLE          BIT(4)
#define DATA_PRESENT_SEL              BIT(5)
#define CMD_TYPE_NORMAL               (0 << 6)
#define CMD_TYPE_SUSPEND              (1 << 6)
#define CMD_TYPE_RESUME               (2 << 6)
#define CMD_TYPE_ABORT                (3 << 6)
#define RESP01_R(addr)                                *((volatile unsigned int *)(addr + 0x10))
#define RESP23_R(addr)                                *((volatile unsigned int *)(addr + 0x14))
#define RESP45_R(addr)                                *((volatile unsigned int *)(addr + 0x18))
#define RESP67_R(addr)                                *((volatile unsigned int *)(addr + 0x1c))
#define BUF_DATA_R(addr)                                *((volatile unsigned int *)(addr + 0x20))
#define PSTATE_REG_R(addr)                            *((volatile unsigned int *)(addr + 0x24))
#define CMD_INHIBIT                 BIT(0)
#define CMD_INHIBIT_DAT             BIT(1)
#define DAT_LINE_ACTIVE             BIT(2)
#define RE_TUNE_REQ                 BIT(3)
#define DAT_7_4_MASK                  (0xF << 4)
#define WR_XFER_ACTIVE              BIT(8)
#define RD_XFER_ACTIVE              BIT(9)
#define BUF_WR_ENABLE                 BIT(10)
#define BUF_RD_ENABLE                 BIT(11)
#define CARD_INSERTED                 BIT(16)
#define CARD_STABLE                 BIT(17)
#define HOST_CTRL1_R(addr)                            *((volatile unsigned char *)(addr + 0x28))
#define LED_CTRL_ON                 (1 << 0)
#define DAT_XFER_WIDTH_4BIT       (1 << 1)
#define HIGH_SPEED_EN                 (1 << 2)
#define DMASEL_MASK                 (0x3 << 3)
#define DMASEL_SDMA               (0 << 3)
#define DMASEL_ADMA2                (2 << 3)
#define DMASEL_ADMA3                (3 << 3)
#define EXTDAT_XFER_WIDTH8        (1 << 5)
#define PWR_CTRL_R(addr)                                *((volatile unsigned char *)(addr + 0x29))
#define SD_BUS_PWR_VDD1             (0x1 << 0)
#define SD_BUS_VOL_VDD1_1P8V        (0x5 << 1)
#define SD_BUS_VOL_VDD1_3P0V        (0x6 << 1)
#define SD_BUS_VOL_VDD1_3P3V        (0x7 << 1)
#define SD_BUS_PWR_VDD2             0x10
#define BGAP_CTRL_R(addr)                               *((volatile unsigned char *)(addr + 0x2a))
#define WUP_CTRL_R(addr)                                *((volatile unsigned char *)(addr + 0x2b))
#define CLK_CTRL_R(addr)                                *((vuint16_t *)(addr + 0x2c))
#define INTERNAL_CLK_EN             BIT(0)
#define INTERNAL_CLK_STABLE       BIT(1)
#define SD_CLK_EN                     BIT(2)
#define PLL_ENABLE                  BIT(3)
#define CLK_GEN_SELECT              BIT(5)
#define UPPER_FREQ_SEL_MASK       0xc0
#define FREQ_SEL_MASK                 0xff00
#define FREQ_SEL_VAL                  (0x7d << 8) /* 0x7a-400khz , 0xfa-200Khz */
#define TOUT_CTRL_R(addr)                               *((volatile unsigned char *)(addr + 0x2e))
#define SW_RST_R(addr)                                *((volatile unsigned char *)(addr + 0x2f))
#define SW_RST_ALL                BIT(0)
#define SW_RST_CMD                BIT(1)
#define SW_RST_DAT                BIT(2)
#define NORMAL_INT_STAT_R(addr)                       *((vuint16_t *)(addr + 0x30))
#define ERROR_INT_STAT_R(addr)                        *((vuint16_t *)(addr + 0x32))
#define NORMAL_INT_STAT_EN_R(addr)                    *((vuint16_t *)(addr + 0x34))
#define ERROR_INT_STAT_EN_R(addr)                     *((vuint16_t *)(addr + 0x36))
#define NORMAL_INT_SIGNAL_EN_R(addr)                  *((vuint16_t *)(addr + 0x38))
#define ERROR_INT_SIGNAL_EN_R(addr)                   *((vuint16_t *)(addr + 0x3a))
#define CMD_COMPLETE_SIGNAL_EN      BIT(0) /* intr when response received */
#define XFER_COMPLETE_SIGNAL_EN     BIT(1) /* intr when data read/write xfer completed */
#define BGAP_EVENT_SIGNAL_EN        BIT(2)
#define DMA_INTERRUPT_SIGNAL_EN     BIT(3)
#define BUF_WR_READY_SIGNAL_EN      BIT(4)
#define BUF_RD_READY_SIGNAL_EN      BIT(5)
#define CARD_INSERTION_SIGNAL_EN    BIT(6)
#define CARD_REMOVAL_SIGNAL_EN      BIT(7)
#define CARD_INTERRUPT_SIGNAL_EN    BIT(8)
#define INT_A_SIGNAL_EN         BIT(9)
#define INT_B_SIGNAL_EN         BIT(10)
#define INT_C_SIGNAL_EN         BIT(11)
#define RE_TUNE_EVENT_SIGNAL_EN       BIT(12)
#define FX_EVENT_SIGNAL_EN        BIT(13)
#define CQE_EVENT_SIGNAL_EN       BIT(14)
#define CMD_TOUT_ERR_SIGNAL_EN        BIT(0) /* bit0 */
#define CMD_CRC_ERR_SIGNAL_EN         BIT(1) /* bit1 */
#define CMD_END_BIT_ERR_SIGNAL_EN     BIT(2)
#define CMD_IDX_ERR_SIGNAL_EN         BIT(3)
#define DATA_TOUT_ERR_SIGNAL_EN       BIT(4)
#define DATA_CRC_ERR_SIGNAL_EN        BIT(5)
#define DATA_END_BIT_ERR_SIGNAL_EN    BIT(6)
#define CUR_LMT_ERR_SIGNAL_EN         BIT(7) /* bit7 */
#define AUTO_CMD_ERR_SIGNAL_EN        BIT(8) /* bit8 */
#define ADMA_ERR_SIGNAL_EN        BIT(9)
#define TUNING_ERR_SIGNAL_EN          BIT(10)
#define RESP_ERR_SIGNAL_EN        BIT(11)
#define BOOT_ACK_ERR_SIGNAL_EN        BIT(12)
#define VENDOR_ERR_SIGNAL_EN1         BIT(13)
#define VENDOR_ERR_SIGNAL_EN2         BIT(14)
#define VENDOR_ERR_SIGNAL_EN3         BIT(15)
#define SD_CMD_INTR_MASK    (CMD_COMPLETE_SIGNAL_EN | CMD_TOUT_ERR_SIGNAL_EN | \
                CMD_CRC_ERR_SIGNAL_EN | CMD_END_BIT_ERR_SIGNAL_EN | \
                CMD_IDX_ERR_SIGNAL_EN)
#define SD_DAT_INTR_MASK    (BUF_RD_READY_SIGNAL_EN | BUF_WR_READY_SIGNAL_EN | \
                XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN | \
                DATA_TOUT_ERR_SIGNAL_EN | DATA_CRC_ERR_SIGNAL_EN | \
                DATA_END_BIT_ERR_SIGNAL_EN | BGAP_EVENT_SIGNAL_EN)
#define SD_BUS_PWR_INTR_MASK    (CUR_LMT_ERR_SIGNAL_EN)
#define SD_CARD_INTR_CHANGE (CARD_INSERTION_SIGNAL_EN | CARD_REMOVAL_SIGNAL_EN)
#define SD_ERROR_MASK       (0xFFFF8000)

#define AUTO_CMD_STAT_R(addr)                         *((vuint16_t *)(addr + 0x3c))
#define HOST_CTRL2_R(addr)                            *((vuint16_t *)(addr + 0x3e))
#define USH_MODE_MASK     0x3
#define UHS_MODE_SDR12    (0)
#define UHS_MODE_SDR25    (1)
#define UHS_MODE_SDR50    (2)
#define UHS_MODE_SDR104   (3)
#define UHS_MODE_DDR50    (4)
#define UHS_MODE_UHS2     (7)
#define SIGNAL_1P8V_EN  (1 << 3)

/* eMMC speed mode */
#define UHS_MODE_EMMC_DS      0
#define UHS_MODE_EMMC_HS      1 /* 25Mhz */
#define UHS_MODE_EMMC_HS200   3 /* 200Mhz */
#define UHS_MODE_EMMC_HSDDR   4 /* DDR50 */
#define USH_MODE_EMMC_HS400   7

#define CAPABILITIES1_R(addr)                         *((volatile unsigned int *)(addr + 0x40))
#define CAPABILITIES2_R(addr)                         *((volatile unsigned int *)(addr + 0x44))
#define CURR_CAPABILITIES1_R(addr)                    *((volatile unsigned int *)(addr + 0x48))
#define CURR_CAPABILITIES2_R(addr)                    *((volatile unsigned int *)(addr + 0x4c))
#define FORCE_AUTO_CMD_STAT_R(addr)                   *((vuint16_t *)(addr + 0x50))
#define FORCE_ERROR_INT_STAT_R(addr)                  *((vuint16_t *)(addr + 0x52))
#define ADMA_ERR_STAT_R(addr)                         *((volatile unsigned int *)(addr + 0x54))
#define ADMA_SA_LOW_R(addr)                           *((volatile unsigned int *)(addr + 0x58))
#define ADMA_SA_HIGH_R(addr)                          *((volatile unsigned int *)(addr + 0x5c))
#define PRESET_INT_R(addr)                            *((volatile unsigned int *)(addr + 0x60))
#define PRESET_DS_R(addr)                             *((vuint16_t *)(addr + 0x62))
#define PRESET_HS_R(addr)                             *((vuint16_t *)(addr + 0x64))
#define PRESET_SD12_R(addr)                           *((vuint16_t *)(addr + 0x66))
#define PRESET_SD25_R(addr)                           *((vuint16_t *)(addr + 0x68))
#define PRESET_SDR50_R(addr)                          *((vuint16_t *)(addr + 0x6a))
#define PRESET_SDR104_R(addr)                         *((vuint16_t *)(addr + 0x6c))
#define PRESET_DDR50_R(addr)                          *((vuint16_t *)(addr + 0x6e))
#define PRESET_UHS2_R(addr)                           *((volatile unsigned int *)(addr + 0x74))
#define ADMA_ID_LOW_R(addr)                           *((volatile unsigned int *)(addr + 0x78))
#define ADMA_ID_HIGH_R(addr)                          *((volatile unsigned int *)(addr + 0x7c))
#define UHS2_BLOCK_SIZE_R(addr)                       *((volatile unsigned long *)(addr + 0x80))
#define UHS2_BLOCK_COUNT_R(addr)                        *((volatile unsigned long *)(addr + 0x84))
#define UHS2_CMD_PKT_0_3_R(addr)                        *((volatile unsigned long *)(addr + 0x88))
#define UHS2_CMD_PKT_4_7_R(addr)                        *((volatile unsigned long *)(addr + 0x8c))
#define UHS2_CMD_PKT_8_11_R(addr)                       *((volatile unsigned long *)(addr + 0x90))
#define UHS2_CMD_PKT_12_15_R(addr)                    *((volatile unsigned long *)(addr + 0x94))
#define UHS2_CMD_PKT_16_19_R(addr)                    *((volatile unsigned long *)(addr+0X98))
#define UHS2_XFER_MODE_R(addr)                        *((volatile unsigned long *)(addr + 0x9c))
#define UHS2_CMD_R(addr)                                  *((volatile unsigned long *)(addr + 0x9e))
#define UHS2_RESP_0_3_R(addr)                             *((volatile unsigned long *)(addr + 0xa0))
#define UHS2_RESP_4_7_R(addr)                             *((volatile unsigned long *)(addr + 0xa4))
#define UHS2_RESP_8_11_R(addr)                        *((volatile unsigned long *)(addr + 0xa8))
#define UHS2_RESP_12_15_R(addr)                       *((volatile unsigned long *)(addr + 0xac))
#define UHS2_RESP_16_19_R(addr)                       *((volatile unsigned long *)(addr + 0xb0))
#define UHS2_MSG_SEL_R(addr)                              *((volatile unsigned long *)(addr + 0xb4))
#define UHS2_MSG_R(addr)                                  *((volatile unsigned long *)(addr + 0xb8))
#define UHS2_DEV_INTR_STATUS_R(addr)                    *((volatile unsigned long *)(addr + 0xbc))
#define UHS2_DEV_SEL_R(addr)                              *((volatile unsigned long *)(addr + 0xbe))
#define UHS2_DEV_INR_CODE_R(addr)                       *((volatile unsigned long *)(addr + 0xbf))
#define UHS2_SOFT_RESET_R(addr)                       *((volatile unsigned long *)(addr + 0xc0))
#define UHS2_TIMER_CNTRL_R(addr)                        *((volatile unsigned long *)(addr + 0xc2))
#define UHS2_ERR_INTR_STATUS_R(addr)                    *((volatile unsigned long *)(addr + 0xc4))
#define UHS2_ERR_INTR_STATUS_EN_R(addr)               *((volatile unsigned long *)(addr + 0xc8))
#define UHS2_ERR_INTR_SIGNAL_EN_R(addr)               *((volatile unsigned long *)(addr + 0xcc))
#define P_UHS2_SETTINGS_R(addr)                         *((volatile unsigned long *)(addr + 0xe0))
#define P_UHS2_HOST_CAPAB(addr)                         *((volatile unsigned long *)(addr + 0xe2))
#define P_UHS2_TEST(addr)                                   *((volatile unsigned long *)(addr + 0xe4))
#define P_EMBEDDED_CNTRL(addr)                          *((volatile unsigned long *)(addr + 0xe6))
#define P_VENDOR_1_SPECIFIC_AREA(addr)                *((volatile unsigned long *)(addr + 0xe8))
#define P_VENDOR_2_SPECIFIC_AREA(addr)                *((volatile unsigned long *)(addr + 0xea))
#define SLOT_INTR_STATUS_R(addr)                          *((volatile unsigned long *)(addr + 0xfc))
#define HOST_CNTRL_VERS_R(addr)                           *((vuint16_t *)(addr + 0xfe))

/* DWC_mshc_map/DWC_mshc_UHS2_setting block register */
#define UHS2_GEN_SET_R(addr)                              *((volatile unsigned long *)(addr + 0x00))
#define UHS2_PHY_SET_R(addr)                              *((volatile unsigned long *)(addr + 0x04))
#define USH2_LNK_TRAN_SET_1_R(addr)                   *((volatile unsigned long *)(addr + 0x08))
#define UHS2_LNK_TRAN_SET_2_R(addr)                   *((volatile unsigned long *)(addr + 0x0c))

/* DWC_mshc_map/DWC_mshc_UHS2_capability register */
#define UHS2_GEN_CAP_R(addr)                              *((volatile unsigned long *)(addr + 0x00))
#define UHS2_PHY_CAP_R(addr)                              *((volatile unsigned long *)(addr + 0x04))
#define UHS2_LNK_TRAN_CAP_1_R(addr)                   *((volatile unsigned long *)(addr + 0x08))
#define UHS2_LNK_TRAN_CAP_2_R(addr)                   *((volatile unsigned long *)(addr + 0x0c))

/* DWC_mshc_map/DWC_mshc_UHS2_test block egister */
#define FORCE_UHS2_ERR_INTR_STATUS_R(addr)            *((volatile unsigned long *)(addr + 0x00))

/* DWC_mshc_map/DWC_mshc_UHS2_ embedded control block egister */
#define EMBEDDED_CTRL_R(addr)                             *((volatile unsigned int *)(addr + 0xf6c))

/* DWC_mshc_map/DWC_mshc_phy_block register */
//#define DWC_MSHC_PTR_PHY_R       0x300
//#define PHY_CNFG_R        (DWC_MSHC_PTR_PHY_R + 0x00)
//#define PHY_CMDPAD_CNFG_R (DWC_MSHC_PTR_PHY_R + 0x04)
//#define PHY_DATAPAD_CNFG_R    (DWC_MSHC_PTR_PHY_R + 0x06)
//#define PHY_CLKPAD_CNFG_R (DWC_MSHC_PTR_PHY_R + 0x08)
//#define PHY_STBPAD_CNFG_R (DWC_MSHC_PTR_PHY_R + 0x0a)
//#define PHY_RSTNPAD_CNFG_R    (DWC_MSHC_PTR_PHY_R + 0x0c)
//#define PHY_PADTEST_CNFG_R    (DWC_MSHC_PTR_PHY_R + 0x0e)
//#define PHY_PADTEST_OUT_R (DWC_MSHC_PTR_PHY_R + 0x10)
//#define PHY_PADTEST_IN_R  (DWC_MSHC_PTR_PHY_R + 0x12)
//#define PHY_PRBS_CNFG_R       (DWC_MSHC_PTR_PHY_R + 0x18)
//#define PHY_PHYLBK_CNFG_R (DWC_MSHC_PTR_PHY_R + 0x1a)
//#define PHY_COMMDL_CNFG_R (DWC_MSHC_PTR_PHY_R + 0x1c)
//#define PHY_SDCLKDL_CNFG_R    (DWC_MSHC_PTR_PHY_R + 0x1d)
//#define PHY_SDCLKDL_DC_R  (DWC_MSHC_PTR_PHY_R + 0x1e)
//#define PHY_SMPLDL_CNFG_R (DWC_MSHC_PTR_PHY_R + 0x20)
//#define PHY_ATDL_CNFG_R       (DWC_MSHC_PTR_PHY_R + 0x21)
//#define PHY_DLL_CTRL_R        (DWC_MSHC_PTR_PHY_R + 0x24)
//#define PHY_DLL_CNFG1_R       (DWC_MSHC_PTR_PHY_R + 0x25)
//#define PHY_DLLDL_CNFG_R  (DWC_MSHC_PTR_PHY_R + 0x28)
//#define PHY_DLL_OFFST_R       (DWC_MSHC_PTR_PHY_R + 0x29)
//#define PHY_DLLMST_TSTDC_R    (DWC_MSHC_PTR_PHY_R + 0x2a)
//#define PHY_DLLBT_CNFG_R  (DWC_MSHC_PTR_PHY_R + 0x2c)
//#define PHY_DLL_STATUS_R  (DWC_MSHC_PTR_PHY_R + 0x2e)
//#define PHY_DLLDBG_MLKDC_R    (DWC_MSHC_PTR_PHY_R + 0x30)
//#define PHY_DLLDBG_SLKDC_R    (DWC_MSHC_PTR_PHY_R + 0x32)

/* DWC_mshc_map/DWC_mshc_UHS2 mhsc vendor1_block register */
//#define MHSC_CTRL_R       0x8
//  #define CMD_CNFLT_CHK_STBIT 0
//#define AT_CTRL_R     0x40
//  #define SW_TUNE_EN_STBIT    4
//#define AT_STAT_R     0x44

/* DWC_mshc_map/DWC_mshc_UHS2 mhsc vendor2_block CQE register */
#define CQVER_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x00))
#define CQCAP_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x04))
#define CQCFG_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x08))
#define CQCTL_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x0c))
#define CQIS_R(addr)                                          *((volatile unsigned int *)(addr + 0x180+0x10))
#define CQISE_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x14))
#define CQISGE_R(addr)                                    *((volatile unsigned int *)(addr + 0x180+0x18))
#define CQIC_R(addr)                                          *((volatile unsigned int *)(addr + 0x180+0x1c))
#define CQTDLBA_R(addr)                                   *((volatile unsigned int *)(addr + 0x180+0x20))
#define CQTDLBAU_R(addr)                                    *((volatile unsigned int *)(addr + 0x180+0x24))
#define CQTDBR_R(addr)                                    *((volatile unsigned int *)(addr + 0x180+0x28))
#define CQTCN_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x2c))
#define CQDQS_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x30))
#define CQDPT_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x34))
#define CQTCLR_R(addr)                                    *((volatile unsigned int *)(addr + 0x180+0x38))
#define CQSSC1_R(addr)                                    *((volatile unsigned int *)(addr + 0x180+0x40))
#define CQSSC2_R(addr)                                    *((volatile unsigned int *)(addr + 0x180+0x44))
#define CQCRDCT_R(addr)                                   *((volatile unsigned int *)(addr + 0x180+0x48))
#define CQRMEM_R(addr)                                    *((volatile unsigned int *)(addr + 0x180+0x50))
#define CQTERRI_R(addr)                                   *((volatile unsigned int *)(addr + 0x180+0x54))
#define CQCRI_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x58))
#define CQCRA_R(addr)                                         *((volatile unsigned int *)(addr + 0x180+0x5c))

/* DWC_mshc_map/DWC_mshc_UHS2 mhsc vendor1_block CQE register *
 * P_VENDOR_1_SPECIFIC_AREA
 */
#define SNPS_MSHC_VER_ID_R(addr)                      *((volatile unsigned int *)(addr + 0x500+0x00))
#define SNPS_MSHC_VER_TYPE_R(addr)                    *((volatile unsigned int *)(addr + 0x500+0x04))
#define SNPS_MSHC_CTRL_R(addr)                            *((volatile unsigned char *)(addr + 0x500+0x08))
#define SNPS_MBIU_CTRL_R(addr)                            *((volatile unsigned char *)(addr + 0x500+0x10))
#define SNPS_EMMC_CTRL_R(addr)                            *((vuint16_t *)(addr + 0x500+0x2c))
#define SNPS_BOOT_CTRL_R(addr)                            *((vuint16_t *)(addr + 0x500+0x2e))
#define SNPS_GP_IN_R(addr)                        *((volatile unsigned int *)(addr + 0x500+0x30))
#define SNPS_GP_OUT_R(addr)                           *((volatile unsigned int *)(addr + 0x500+0x34))
#define SNPS_AT_CTRL_R(addr)                              *((volatile unsigned int *)(addr + 0x500+0x40))
#define AUTO_TUNE_EN          0
#define CI_SEL                1
#define SWIN_TH_EN_BITP     2
#define SWIN_TH_EN_BITW     1
#define RPT_TUNE_ERR          3
#define SW_TUNE_EN          4
#define WIN_EDGE_SEL          8
#define SNPS_AT_STAT_R(addr)                                *((volatile unsigned int *)(addr + 0x500+0x44))

#define OFF             0
#define ON              1
#define DISABLE         0
#define ENABLE          1

#define SDHCI_CLOCK_OFF     1
#define SDHCI_CLOCK_ON      2
#define SDHCI_POWER_OFF     3
#define SDHCI_POWER_ON      4
#define SDHCI_SET_TXPHASE   5
#define SDHCI_SET_RXPHASE   6
#define SDHCI_SW_TUNE_CTRL  7
#define SW_TUNE_DISABLE     0
#define SW_TUNE_ENABLE      1
#define SDHCI_CMDCFLT_CHK      8

#define FREQSEL_CLK_400KHZ  0xff
//#define FREQSEL_SDR12         0x02 /* 25Mhz */
//#define FREQSEL_SDR25         0x01 /* 50Mhz */
//#define FREQSEL_SDR50         0x00 /* 100Mhz */
//#define FREQSEL_SDR104          0x00
//#define FREQSEL_DDR50         0x01

/* MMCM module reset through mshc gpio controls */
#define MMCM_RESET_REG_OFFS 0x534
#define MMCM_RESET_ON   (1 << 0)

//*******  interrupt state EN*********//
#define CMD_COMPLETE_STAT_EN          BIT(0) /* intr when response received */
#define XFER_COMPLETE_STAT_EN         BIT(1) /* intr when data read/write xfer completed */
#define BGAP_EVENT_STAT_EN        BIT(2)
#define DMA_INTERRUPT_STAT_EN         BIT(3)
#define BUF_WR_READY_STAT_EN          BIT(4)
#define BUF_RD_READY_STAT_EN          BIT(5)
#define CARD_INSERTION_STAT_EN            BIT(6)
#define CARD_REMOVAL_STAT_EN          BIT(7)
#define CARD_INTERRUPT_STAT_EN            BIT(8)
#define INT_A_STAT_EN                 BIT(9)
#define INT_B_STAT_EN             BIT(10)
#define INT_C_STAT_EN             BIT(11)
#define RE_TUNE_EVENT_STAT_EN         BIT(12)
#define FX_EVENT_STAT_EN          BIT(13)
#define CQE_EVENT_STAT_EN         BIT(14)
#define ERROR_INTERRUPT_STAT_EN       BIT(15)

#define CMD_TOUT_ERR_STAT_EN          BIT(0) /* bit0 */
#define CMD_CRC_ERR_STAT_EN           BIT(1) /* bit1 */
#define CMD_END_BIT_ERR_STAT_EN           BIT(2) /* bit2 */
#define CMD_IDX_ERR_STAT_EN       BIT(3) /* bit3 */
#define DATA_TOUT_ERR_STAT_EN         BIT(4) /* bit4 */
#define DATA_CRC_ERR_STAT_EN          BIT(5) /* bit5 */
#define DATA_END_BIT_ERR_STAT_EN      BIT(6) /* bit6 */
#define CUR_LMT_ERR_STAT_EN       BIT(7) /* bit7 */
#define AUTO_CMD_ERR_STAT_EN          BIT(8) /* bit8 */
#define ADMA_ERR_STAT_EN              BIT(9) /* bit9 */
#define TUNING_ERR_STAT_EN        BIT(10) /* bit10 */
#define RESP_ERR_STAT_EN          BIT(11) /* bit11 */
#define BOOT_ACK_ERR_STAT_EN          BIT(12) /* bit12 */
#define VENDOR_ERR1_STAT_EN       BIT(13) /* bit13 */
#define VENDOR_ERR2_STAT_EN       BIT(14) /* bit14 */
#define VENDOR_ERR3_STAT_EN       BIT(15) /* bit15 */

//******* clear interrupt *********//
#define CLR_CMD_COMPLETE_STAT         BIT(0) /* intr when response received */
#define CLR_XFER_COMPLETE_STAT        BIT(1) /* intr when data read/write xfer completed */
#define CLR_BGAP_EVENT_STAT       BIT(2)
#define CLR_DMA_INTERRUPT_STAT        BIT(3)
#define CLR_BUF_WR_READY_STAT         BIT(4)
#define CLR_BUF_RD_READY_STAT         BIT(5)
#define CLR_CARD_INSERTION_STAT           BIT(6)
#define CLR_CARD_REMOVAL_STAT         BIT(7)
#define CLR_CARD_INTERRUPT_STAT           BIT(8)
#define CLR_INT_A_STAT            BIT(9)
#define CLR_INT_B_STAT            BIT(10)
#define CLR_INT_C_STAT            BIT(11)
#define CLR_RE_TUNE_EVENT_STAT        BIT(12)
#define CLR_FX_EVENT_STAT         BIT(13)
#define CLR_CQE_EVENT_STAT        BIT(14)

#define CLR_CMD_TOUT_ERR_STAT         BIT(0) /* bit0 */
#define CLR_CMD_CRC_ERR_STAT              BIT(1) /* bit1 */
#define CLR_CMD_END_BIT_ERR_STAT      BIT(2) /* bit2 */
#define CLR_CMD_IDX_ERR_STAT          BIT(3) /* bit3 */
#define CLR_DATA_TOUT_ERR_STAT        BIT(4) /* bit4 */
#define CLR_DATA_CRC_ERR_STAT         BIT(5) /* bit5 */
#define CLR_DATA_END_BIT_ERR_STAT     BIT(6) /* bit6 */
#define CLR_CUR_LMT_ERR_STAT          BIT(7) /* bit7 */
#define CLR_AUTO_CMD_ERR_STAT         BIT(8) /* bit8 */
#define CLR_ADMA_ERR_STAT             BIT(9) /* bit9 */
#define CLR_TUNING_ERR_STAT       BIT(10) /* bit10 */
#define CLR_RESP_ERR_STAT         BIT(11) /* bit11 */
#define CLR_BOOT_ACK_ERR_STAT         BIT(12) /* bit12 */
#define CLR_VENDOR_ERR1_STAT          BIT(13) /* bit13 */
#define CLR_VENDOR_ERR2_STAT          BIT(14) /* bit14 */
#define CLR_VENDOR_ERR3_STAT          BIT(15) /* bit15 */

// eof

