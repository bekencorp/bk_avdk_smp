/******************************************************************************
Software Sample Terms and Conditions These Software Sample Terms and Conditions
("Terms") cover the Software you license from Synopsys, unless and until we
enter into new terms that expressly replace these Terms. If you use the Software
as an employee of or for the benefit of your company, your company will be the
licensee under these Terms. Accepting it you consent to these Terms on behalf of
yourself and the company on whose behalf you will use the Software. The
effective date of these Terms is the date that you click accepted them. If you
do not agree to these Terms or if you do not have the power and authority to
accept these Terms on behalf of your company, you may not use the Software and
Synopsys is unwilling to provide you with them.

1. The Software is not an item of Licensed Software or Licensed Product under
any end-user software license agreement with Synopsys or any supplement thereto.
Synopsys hereby grants to you a limited, personal, non-exclusive,
non-transferable, non-assignable, fully paid, royalty free, worldwide, perpetual
license to use the Software, and create modifications of the components of the
Software provided to you in source code format, solely for use with a Synopsys
DesignWare IP. All modifications of the Software are owned by Synopsys, and you
hereby irrevocably assign ownership of those modifications (and all intellectual
property rights therein) to Synopsys. However, you are under no obligation to
disclose any such modifications to Synopsys and the modifications are
automatically licensed to you as Software. The Software and all modifications
are the Confidential Information of Synopsys, and you agree not to distribute or
disclose them.

2. THE SOFTWARE IS PROVIDED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE HEREBY
DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THE SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.

3. These Terms, which can be modified only by Synopsys in writing, shall be
governed by and construed under the laws of the State of California, USA,
without regard for its conflict of laws principles. You may not transfer or
assign your license rights to any other person in any manner (by assignment,
operation of law or otherwise) unless you have obtained written consent from
Synopsys. If you attempt to transfer or assign any of your license rights
without Synopsys's consent, the transfer or assignment will be ineffective,
null, and void. For purposes of this Section, a transfer or assignment of your
license rights will be deemed to have occurred (a) if a third party (or group of
third parties acting in concert) acquires beneficial ownership of fifty percent
(50%) or more of either your assets or of the stock or other equity interests
entitled to vote for your directors or equivalent managing authority, or (b) in
the event of a merger, consolidation or other business combination between you
and one or more third parties where your stockholders immediately before that
transaction own (directly or indirectly), after that transaction, less than
fifty percent (50%) of the stock or other equity interests entitled to vote for
the directors or equivalent managing authority of the surviving entity. These
Terms constitute the entire understanding and agreement between you and Synopsys
with respect to the subject matter hereof and supersedes and replaces all prior
and contemporaneous understandings and agreements, oral or written, express or
implied, regarding the same subject matter.
******************************************************************************/
/**
* \file		: mshc_regs.h
* \author	: ravibabu@synopsys.com
* \date		: 01-July-2019
* \brief	: mshc_regs.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	01-July-2019	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _MSHC_REGS_H_
#define _MSHC_RESG_H_

/* dwc_mshc_map/DWC_mshc_block registers */
#define SDMASA_R	0x00
#define BLOCKSIZE_R	0x04
#define	SDMA_BUF_BDARY_4K	(0 << 12)
#define	SDMA_BUF_BDARY_8K	(1 << 12)
#define	SDMA_BUF_BDARY_16K	(2 << 12)
#define	SDMA_BUF_BDARY_32K	(3 << 12)
#define	SDMA_BUF_BDARY_64K	(4 << 12)
#define	SDMA_BUF_BDARY_128K	(5 << 12)
#define	SDMA_BUF_BDARY_256K	(6 << 12)
#define	SDMA_BUF_BDARY_512K	(7 << 12)
#define BLOCKCOUNT_R	0x06
#define ARGUMENT_R	0x08
#define XFER_MODE_R	0x0c
#define XFR_MODE_DMA_EN		BIT(0)
#define XFR_MODE_BLKCNT_EN	BIT(1)
#define XFR_MODE_AUTOCMD12_EN	(0x01 << 2)
#define XFR_MODE_AUTOCMD23_EN	(0x02 << 2)
#define XFR_MODE_AUTOCMD_AUTOSEL (0x03 << 2)
#define XFR_MODE_DATA_READ	BIT(4)
#define XFR_MODE_MULTBLK_SEL	BIT(5)
#define XFR_MODE_RESP_TYPE	BIT(6)
#define XFR_MODE_RESP_ERRCHK_EN	BIT(7)
#define XFR_MODE_RESP_INT_EN	BIT(8)
#define CMD_R		0x0e
#define SUB_CMD_FLAG			BIT(2)
#define CMD_CRC_CHK_ENABLE		BIT(3)
#define	CMD_IDX_CHK_ENABLE		BIT(4)
#define DATA_PRESENT_SEL		BIT(5)
#define CMD_TYPE_NORMAL			(0 << 6)
#define CMD_TYPE_SUSPEND		(1 << 6)
#define CMD_TYPE_RESUME			(2 << 6)
#define CMD_TYPE_ABORT			(3 << 6)
#define RESP01_R	0x10
#define RESP23_R	0x14
#define RESP45_R	0x18
#define RESP67_R	0x1c
#define BUF_DATA_R	0x20
#define PSTATE_REG_R	0x24
#define	CMD_INHIBIT		BIT(0)
#define CMD_INHIBIT_DAT		BIT(1)
#define DAT_LINE_ACTIVE		BIT(2)
#define RE_TUNE_REQ		BIT(3)
#define DAT_7_4_MASK		(0xF << 4)
#define WR_XFER_ACTIVE		BIT(8)
#define	RD_XFER_ACTIVE		BIT(9)
#define	BUF_WR_ENABLE		BIT(10)
#define	BUF_RD_ENABLE		BIT(11)
#define	CARD_INSERTED		BIT(16)
#define	CARD_STABLE		BIT(17)
#define HOST_CTRL1_R	0x28
#define LED_CTRL_ON		(1 << 0)
#define	DAT_XFER_WIDTH_4BIT	(1 << 1)
#define HIGH_SPEED_EN		(1 << 2)
#define DMASEL_MASK		(0x3 << 3)
#define DMASEL_SDMA		(0 << 3)
#define DMASEL_ADMA2		(2 << 3)
#define DMASEL_ADMA3		(3 << 3)
#define	EXTDAT_XFER_WIDTH8	(1 << 5)
#define PWR_CTRL_R	0x29
#define SD_BUS_PWR_VDD1		(0x1 << 0)
#define	SD_BUS_VOL_VDD1_1P8V	(0x5 << 1)
#define SD_BUS_VOL_VDD1_3P0V	(0x6 << 1)
#define SD_BUS_VOL_VDD1_3P3V	(0x7 << 1)
#define SD_BUS_PWR_VDD2		0x10
#define BGAP_CTRL_R	0x2a
#define WUP_CTRL_R	0x2b
#define CLK_CTRL_R	0x2c
#define	INTERNAL_CLK_EN		BIT(0)
#define	INTERNAL_CLK_STABLE	BIT(1)
#define	SD_CLK_EN		BIT(2)
#define	PLL_ENABLE		BIT(3)
#define	CLK_GEN_SELECT		BIT(5)
#define UPPER_FREQ_SEL_MASK	0xc0
#define	FREQ_SEL_MASK		0xff00
#define FREQ_SEL_VAL		(0xFF << 8) /* 0xff-313khz*/
#define TOUT_CTRL_R	0x2e
#define SW_RST_R	0x2f
#define	SW_RST_ALL	BIT(0)
#define SW_RST_CMD	BIT(1)
#define	SW_RST_DAT	BIT(2)
#define NORMAL_INT_STAT_R	0x30
#define ERROR_INT_STAT_R	0x32
#define NORMAL_INT_STAT_EN_R	0x34
#define ERROR_INT_STAT_EN_R	0x36
#define NORMAL_INT_SIGNAL_EN_R	0x38
#define ERROR_INT_SIGNAL_EN_R	0x3a
#define	CMD_COMPLETE_SIGNAL_EN		BIT(0) /* intr when response received */
#define XFER_COMPLETE_SIGNAL_EN		BIT(1) /* intr when data read/write xfer completed */
#define BGAP_EVENT_SIGNAL_EN		BIT(2)
#define	DMA_INTERRUPT_SIGNAL_EN		BIT(3)
#define	BUF_WR_READY_SIGNAL_EN		BIT(4)
#define BUF_RD_READY_SIGNAL_EN		BIT(5)
#define	CARD_INSERTION_SIGNAL_EN	BIT(6)
#define	CARD_REMOVAL_SIGNAL_EN		BIT(7)
#define CARD_INTERRUPT_SIGNAL_EN	BIT(8)
#define	INT_A_SIGNAL_EN			BIT(9)
#define	INT_B_SIGNAL_EN			BIT(10)
#define	INT_C_SIGNAL_EN			BIT(11)
#define	RE_TUNE_EVENT_SIGNAL_EN		BIT(12)
#define	FX_EVENT_SIGNAL_EN		BIT(13)
#define CQE_EVENT_SIGNAL_EN		BIT(14)
#define	CMD_TOUT_ERR_SIGNAL_EN		BIT(16) /* bit0 */
#define	CMD_CRC_ERR_SIGNAL_EN		BIT(17) /* bit1 */
#define	CMD_END_BIT_ERR_SIGNAL_EN	BIT(18)
#define	CMD_IDX_ERR_SIGNAL_EN		BIT(19)
#define DATA_TOUT_ERR_SIGNAL_EN		BIT(20)
#define DATA_CRC_ERR_SIGNAL_EN		BIT(21)
#define DATA_END_BIT_ERR_SIGNAL_EN	BIT(22)
#define	CUR_LMT_ERR_SIGNAL_EN		BIT(23) /* bit7 */
#define AUTO_CMD_ERR_SIGNAL_EN		BIT(24) /* bit8 */
#define	ADMA_ERR_SIGNAL_EN		BIT(25)
#define	TUNING_ERR_SIGNAL_EN		BIT(26)
#define	RESP_ERR_SIGNAL_EN		BIT(27)
#define BOOT_ACK_ERR_SIGNAL_EN		BIT(28)
#define VENDOR_ERR_SIGNAL_EN1		BIT(29)
#define	VENDOR_ERR_SIGNAL_EN2		BIT(30)
#define	VENDOR_ERR_SIGNAL_EN3		BIT(31)
#define SD_CMD_INTR_MASK	(CMD_COMPLETE_SIGNAL_EN | CMD_TOUT_ERR_SIGNAL_EN | \
				CMD_CRC_ERR_SIGNAL_EN | CMD_END_BIT_ERR_SIGNAL_EN | \
				CMD_IDX_ERR_SIGNAL_EN)
#define SD_DAT_INTR_MASK	(BUF_RD_READY_SIGNAL_EN | BUF_WR_READY_SIGNAL_EN | \
				XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN | \
				DATA_TOUT_ERR_SIGNAL_EN | DATA_CRC_ERR_SIGNAL_EN | \
				DATA_END_BIT_ERR_SIGNAL_EN | BGAP_EVENT_SIGNAL_EN)
#define SD_BUS_PWR_INTR_MASK	(CUR_LMT_ERR_SIGNAL_EN)
#define SD_CARD_INTR_CHANGE	(CARD_INSERTION_SIGNAL_EN | CARD_REMOVAL_SIGNAL_EN)
#define SD_ERROR_MASK		(0xFFFF8000)

#define AUTO_CMD_STAT_R		0x3c
#define HOST_CTRL2_R		0x3e
#define USH_MODE_MASK	0x3
#define UHS_MODE_SDR12	(0)
#define UHS_MODE_SDR25	(1)
#define UHS_MODE_SDR50	(2)
#define UHS_MODE_SDR104	(3)
#define UHS_MODE_DDR50	(4)
#define UHS_MODE_UHS2	(7)
#define SIGNAL_1P8V_EN	(1 << 3)

#define HOST_VER4_ENABLE_POS     (12)
#define HOST_VER4_ENABLE_MASK    (0x01)

/* eMMC speed mode */
#define UHS_MODE_EMMC_DS	0
#define UHS_MODE_EMMC_HS 	1 /* 25Mhz */
#define UHS_MODE_EMMC_HS200	3 /* 200Mhz */
#define UHS_MODE_EMMC_HSDDR	4 /* DDR50 */
#define USH_MODE_EMMC_HS400	7

#define CAPABILITIES1_R		0x40
#define CAPABILITIES2_R		0x44
#define CURR_CAPABILITIES1_R	0x48
#define CURR_CAPABILITIES2_R	0x4c
#define FORCE_AUTO_CMD_STAT_R	0x50
#define FORCE_ERROR_INT_STAT_R	0x52
#define ADMA_ERR_STAT_R		0x54
#define ADMA_SA_LOW_R		0x58
#define ADMA_SA_HIGH_R		0x5c
#define PRESET_INT_R		0x60
#define PRESET_DS_R		0x62
#define PRESET_HS_R		0x64
#define PRESET_SD12_R		0x66
#define PRESET_SD25_R		0x68
#define PRESET_SDR50_R		0x6a
#define PRESET_SDR104_R		0x6c
#define PRESET_DDR50_R		0x6e
#define PRESET_UHS2_R		0x74
#define ADMA_ID_LOW_R		0x78
#define ADMA_ID_HIGH_R		0x7c
#define UHS2_BLOCK_SIZE_R	0x80
#define UHS2_BLOCK_COUNT_R	0x84
#define UHS2_CMD_PKT_0_3_R	0x88
#define UHS2_CMD_PKT_4_7_R	0x8c
#define UHS2_CMD_PKT_8_11_R	0x90
#define UHS2_CMD_PKT_12_15_R	0x94
#define UHS2_CMD_PKT_16_19_R	0X98
#define UHS2_XFER_MODE_R	0x9c
#define UHS2_CMD_R		0x9e
#define UHS2_RESP_0_3_R		0xa0
#define UHS2_RESP_4_7_R		0xa4
#define UHS2_RESP_8_11_R	0xa8
#define UHS2_RESP_12_15_R	0xac
#define UHS2_RESP_16_19_R	0xb0
#define UHS2_MSG_SEL_R		0xb4
#define UHS2_MSG_R		0xb8
#define UHS2_DEV_INTR_STATUS_R	0xbc
#define UHS2_DEV_SEL_R		0xbe
#define UHS2_DEV_INR_CODE_R	0xbf
#define UHS2_SOFT_RESET_R	0xc0
#define UHS2_TIMER_CNTRL_R	0xc2
#define UHS2_ERR_INTR_STATUS_R	0xc4
#define UHS2_ERR_INTR_STATUS_EN_R	0xc8
#define UHS2_ERR_INTR_SIGNAL_EN_R	0xcc
#define P_UHS2_SETTINGS_R		0xe0
#define P_UHS2_HOST_CAPAB		0xe2
#define P_UHS2_TEST			0xe4
#define P_EMBEDDED_CNTRL		0xe6
#define P_VENDOR_1_SPECIFIC_AREA	0xe8
#define P_VENDOR_2_SPECIFIC_AREA	0xea
#define SLOT_INTR_STATUS_R		0xfc
#define HOST_CNTRL_VERS_R		0xfe

/* DWC_mshc_map/DWC_mshc_UHS2_setting block register */
#define UHS2_GEN_SET_R		0x00
#define UHS2_PHY_SET_R		0x04
#define USH2_LNK_TRAN_SET_1_R	0x08
#define UHS2_LNK_TRAN_SET_2_R	0x0c

/* DWC_mshc_map/DWC_mshc_UHS2_capability register */
#define UHS2_GEN_CAP_R		0x00
#define UHS2_PHY_CAP_R		0x04
#define UHS2_LNK_TRAN_CAP_1_R	0x08
#define UHS2_LNK_TRAN_CAP_2_R	0x0c

/* DWC_mshc_map/DWC_mshc_UHS2_test block egister */
#define FORCE_UHS2_ERR_INTR_STATUS_R	0x00

/* DWC_mshc_map/DWC_mshc_UHS2_ embedded control block egister */
#define EMBEDDED_CTRL_R		0x00

/* DWC_mshc_map/DWC_mshc_phy_block register */
#define DWC_MSHC_PTR_PHY_R	0x300
#define PHY_CNFG_R		(DWC_MSHC_PTR_PHY_R + 0x00)
#define PHY_CMDPAD_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x04)
#define PHY_DATAPAD_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x06)
#define PHY_CLKPAD_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x08)
#define PHY_STBPAD_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x0a)
#define PHY_RSTNPAD_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x0c)
#define PHY_PADTEST_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x0e)
#define PHY_PADTEST_OUT_R	(DWC_MSHC_PTR_PHY_R + 0x10)
#define PHY_PADTEST_IN_R	(DWC_MSHC_PTR_PHY_R + 0x12)
#define PHY_PRBS_CNFG_R		(DWC_MSHC_PTR_PHY_R + 0x18)
#define PHY_PHYLBK_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x1a)
#define PHY_COMMDL_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x1c)
#define PHY_SDCLKDL_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x1d)
#define PHY_SDCLKDL_DC_R	(DWC_MSHC_PTR_PHY_R + 0x1e)
#define PHY_SMPLDL_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x20)
#define PHY_ATDL_CNFG_R		(DWC_MSHC_PTR_PHY_R + 0x21)
#define PHY_DLL_CTRL_R		(DWC_MSHC_PTR_PHY_R + 0x24)
#define PHY_DLL_CNFG1_R		(DWC_MSHC_PTR_PHY_R + 0x25)
#define PHY_DLLDL_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x28)
#define PHY_DLL_OFFST_R		(DWC_MSHC_PTR_PHY_R + 0x29)
#define PHY_DLLMST_TSTDC_R	(DWC_MSHC_PTR_PHY_R + 0x2a)
#define PHY_DLLBT_CNFG_R	(DWC_MSHC_PTR_PHY_R + 0x2c)
#define PHY_DLL_STATUS_R	(DWC_MSHC_PTR_PHY_R + 0x2e)
#define PHY_DLLDBG_MLKDC_R	(DWC_MSHC_PTR_PHY_R + 0x30)
#define PHY_DLLDBG_SLKDC_R	(DWC_MSHC_PTR_PHY_R + 0x32)

/* DWC_mshc_map/DWC_mshc_UHS2 mhsc vendor1_block register */
#define MHSC_CTRL_R		0x8
#define CMD_CNFLT_CHK_STBIT	0
#define AT_CTRL_R		0x40
#define SW_TUNE_EN_STBIT	4
#define AT_STAT_R		0x44

/* DWC_mshc_map/DWC_mshc_UHS2 mhsc vendor2_block CQE register */
#define CQVER_R		0x00
#define CQCAP_R		0x04
#define CQCFG_R		0x08
#define CQCTL_R		0x0c
#define CQIS_R		0x10
#define CQISE_R		0x14
#define CQISGE_R	0x18
#define CQIC_R		0x1c
#define CQTDLBA_R	0x20
#define CQTDLBAU_R	0x24
#define CQTDBR_R	0x28
#define CQTCN_R		0x2c
#define CQDQS_R		0x30
#define CQDPT_R		0x34
#define CQTCLR_R	0x38
#define CQSSC1_R	0x40
#define CQSSC2_R	0x44
#define CQCRDCT_R	0x48
#define CQRMEM_R	0x50
#define CQTERRI_R	0x54
#define CQCRI_R		0x58
#define CQCRA_R		0x5c


/* DWC_mshc_map/DWC_mshc_UHS2 mhsc vendor1_block CQE register *
 * P_VENDOR_1_SPECIFIC_AREA
 */
#if CONFIG_ENABLE_HARDCODE_OFFSET_VALUE
#define VENDOR_SPECIFIC_AREA_OFFSET  (0x500)
#else
#define VENDOR_SPECIFIC_AREA_OFFSET  (0x0)
#endif

#define SNPS_MSHC_VER_ID_R	(0x00 + VENDOR_SPECIFIC_AREA_OFFSET)
#define SNPS_MSHC_VER_TYPE_R	(0x04 + VENDOR_SPECIFIC_AREA_OFFSET)
#define SNPS_MSHC_CTRL_R	(0x08 + VENDOR_SPECIFIC_AREA_OFFSET)
#define SNPS_MBIU_CTRL_R	(0x10 + VENDOR_SPECIFIC_AREA_OFFSET)
#define SNPS_EMMC_CTRL_R	(0x2c + VENDOR_SPECIFIC_AREA_OFFSET)
#define MMC_DEV_TYPE_POS     (0)
#define MMC_DEV_TYPE_MASK    (0x01)
#define SNPS_BOOT_CTRL_R	(0x2e + VENDOR_SPECIFIC_AREA_OFFSET)
#define SNPS_GP_IN_R		(0x30 + VENDOR_SPECIFIC_AREA_OFFSET)
#define SNPS_GP_OUT_R	        (0x34 + VENDOR_SPECIFIC_AREA_OFFSET)
#define SNPS_AT_CTRL_R	        (0x40 + VENDOR_SPECIFIC_AREA_OFFSET)
#define AUTO_TUNE_EN		0
#define CI_SEL			1
#define	SWIN_TH_EN_BITP		2
#define	SWIN_TH_EN_BITW		1
#define RPT_TUNE_ERR		3
#define	SW_TUNE_EN		4
#define WIN_EDGE_SEL		8
#define SNPS_AT_STAT_R	       (0x44 + VENDOR_SPECIFIC_AREA_OFFSET)

#define OFF	                0
#define ON 	                1
#define DISABLE                 0
#define ENABLE                  1

#define	SDHCI_CLOCK_OFF		1
#define	SDHCI_CLOCK_ON		2
#define	SDHCI_POWER_OFF		3
#define	SDHCI_POWER_ON		4
#define SDHCI_SET_TXPHASE	5
#define SDHCI_SET_RXPHASE	6
#define SDHCI_SW_TUNE_CTRL	7
#define SW_TUNE_DISABLE		0
#define SW_TUNE_ENABLE		1
#define SDHCI_CMDCFLT_CHK       8
#define SDHCI_SET_DATA_TIMEOUT_COUNTER  9
#define SDHCI_SET_VERSION               10
#define SDHCI_SET_MMC_DEV_TYPE          11

#define FREQSEL_CLK_400KHZ	0xFF
#define FREQSEL_SDR12		0x02 /* 25Mhz */
#define FREQSEL_SDR25		0x01 /* 50Mhz */
#define FREQSEL_SDR50		0x00 /* 100Mhz */
#define FREQSEL_SDR104		0x00
#define FREQSEL_DDR50		0x01

/* MMCM module reset through mshc gpio controls */
#define MMCM_RESET_REG_OFFS	0x534
#define MMCM_RESET_ON	(1 << 0)
#endif /* _MSHC_RESG_H_ */
