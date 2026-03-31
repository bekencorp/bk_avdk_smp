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
* \file		: sd_cmd.h
* \author	: ravibabu@synopsys.com
* \date		: 01-June-2019
* \brief	: sd_cmd.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	01-June-2019	ravibabui@synopsys.com  001		dev in progress
*/

/* sd commands, bc-broadcast, bcr-broad cast cmd with response */
/* refer SD Phylaer spec section 4.7.4 table 4-23 */
/* responses refer 4.9 
 * Response R1 48 bit length
		(Start(0):1b:[47], TxBit(0):1b:[46], CmdIndx(0x3F):6b:[45:40]
		CardStatus(0xFFFFFFFF):32b:[39:8], CRC7(0x7F):7b:[7:1] EndBit(1):1b[0] 
   Response R2 -136 bit length for CID/CSD commands
   Response R3 -48 bit length for OCR register
   Response R6 -48 bit length for Published RCA register, CMD3
   Response R7 -48 bit length for Card interface condition 
 */
#ifndef _MMC_CMD_H
#define _MMC_CMD_H

/* SD BUSWIDTH select */
#define SD_BUSWIDTH_1		0
#define SD_BUSWIDTH_4		1
#define SD_BUSWIDTH_8		2

/* SDHCI xfer mode */
#define XFER_MODE_PIO		0
#define XFER_MODE_ADMA2		1
#define XFER_MODE_ADMA3		2

/* definitions for APIs */
#define XVDD1_3P3_VOLT		0
#define XVDD1_1P8_VOLT		1
#define XVDD1_3P0_VOLT		2
#define XVDD1_1P2_VOLT		3
#define XVDD1_POWER_OFF		4

/* MMCM clock select */
/*#define MMCM_CLK_200MHZ		200000000
#define MMCM_CLK_125MHZ		125000000
#define MMCM_CLK_143MHZ		143000000
#define MMCM_CLK_166MHZ		166000000
#define MMCM_CLK_181MHZ		181000000*/

/* SD/MMC/eMMC voltage select */
#define XSD_BUS_PWR_VDD1		1
#define XSD_BUS_PWR_VDD2		2

/* defintions for sdhci register */
#define SD_VDD_POWER_OFF	0
#define SD_VDD1_3P3_VOLT	7
#define SD_VDD1_3P0_VOLT	6
#define SD_VDD1_1P8_VOLT	5
#define EMMC_VDD1_3P3_VOLT	7
#define EMMC_VDD1_1P8_VOLT	6
#define EMMC_VDD1_1P2_VOLT	5

#define SD_ACMD			1
#define EMMC_CMD		(1 << 7)

/* class-0 basic commands */
#define MMC_CMD0_GO_IDLE		0 /* bc:reset all cards to idle state */
	/* CMD0 (GO_PRE_IDLE_STATE) command with arg 0xf0f0f0f0 is software
	 * reset command and puts the device into pre-idle state */
	#define CMD0_ARG_SETEMMC_PRE_IDLE	0xF0F0F0F0
	/* In boot mode operation, the master (e*MMC host) can read boot data
	 * from slave (eMMC device) by keeping CMD line low(74clk) or sending CMD0
	 * with argument + 0xFFFFFFFA befor issuing CMD1. The data can be read
	 * from either boot/user area*/
	#define CMD0_ARG_BOOT_FRM_EMMC		0xFFFFFFFA
#define EMMC_CMD1_SEND_OP_COND	(1 | EMMC_CMD)/* CMD1 is first cmd issued to eMMC device after idle,
					refer emmc spec */
#define MMC_CMD2_ALL_SEND_CID	2 /* bcr2:Req card to send CID on CMD line */
#define MMC_CMD3_SEND_REL_ADR 	3 /* bcr6:Ask card to publish new Relative adr (RCA) */
#define MMC_CMD4_SET_DSR		4 /* bc: Program the DSR of all cards */
#define MMC_CMD5			5 /* reserved for SDIO cards */
#define MMC_CMD6_SWITCH_FN	6 /*R1-checks switchable function (mode0), switches card fn(mode1)*/
	#define	SDR12_DS_SPEED_MODE	0
	#define	SDR25_HS_SPEED_MODE	1
	#define	SDR50_SPEED_MODE	2
	#define	SDR104_SPEED_MODE	3
	#define DDR50_SPEED_MODE	4
	/* eMMC speed modes */
	#define XEMMC_DS_SPEED_MODE	0
	#define XEMMC_HS_SPEED_MODE	1
	#define XEMMC_HS200_SPEED_MODE	2
	#define XEMMC_HS400_SPEED_MODE	3
	#define XEMMC_HSDDR_SPEED_MODE	4

enum {
        DEFAULT_SPEED_VAL = 400000,
        SDR12_DS_SPEED_VAL = 12000000,
        SDR25_HS_SPEED_VAL = 25000000,
        SDR50_SPEED_VAL = 50000000,
        SDR104_SPEED_VAL = 104000000,
        DDR50_SPEED_VAL = 50000000
};

#define EMMC_CMD6_SET_EXT_CSD	(6 | EMMC_CMD) /*R1-checks switchable function (mode0), switches card fn(mode1)*/
	#define	ACCESS_MODE_CMDSET	0
	#define ACCESS_MODE_SETBIT	1
	#define ACCESS_MODE_CLRBIT	2
	#define ACCESS_MODE_WRITE_BYTE	3
#define MMC_CMD7_CARD_SELECT	7

#define MMC_CMD8_SEND_IF_COND	8 /* bcr:R1 sends SD memory card interface conditon */
#define EMMC_CMD8_SEND_EXT_CSD	(8 | EMMC_CMD) /* adtc: R1 the device sends its EXT_CSD
						register as block of data*/

#define MMC_CMD9_SEND_CSD	9 /* ac:R2 addressed card sends its card specific data on cmd line */
#define MMC_CMD10_SEND_CID	10 /*ac:R2 addressed card sends its card identifn data on cmd line */
#define MMC_CMD11_VOLT_SWITCH	11 /*ac:R1 switch to 1.8V bus signalling level */
#define MMC_CMD12_STOP_TX	12 /*ac:R1b forces the card to stop transmission */
#define MMC_CMD13_SEND_STATUS	13 /*ac:R1 card to send its status register */

/* class-2 Block oriented Read commadns */
#define MMC_CMD16_SET_BLKLEN		16 /*ac:R1 set blocklen */
#define MMC_CMD17_READ_ONE_BLOCK		17 /*ac:R1 read single blocklen */
#define MMC_CMD18_READ_MULTI_BLOCK	18 /*ac:R1 read multiple blocklen */
#define MMC_CMD19_SEND_TUNING_BLOCK	19 /*ac:R1 64B tuning pattern is sent for SDR50/104 */
#define MMC_CMD23_SET_BLOCKCNT		23 /*ac:R1 Set block count */

#define MMC_CMD21_SEND_TUNING_BLOCK	21 /*ac:R1 read tuning block from eMMC */

/* class-4 Block oriented write commadns */
#define MMC_CMD24_WRITE_ONE_BLOCK	24 /*ac:R1 write block */
#define MMC_CMD25_WRITE_MULTI_BLOCK	25 /*ac:R1 write multiple block */
#define MMC_CMD27_PROGRAM_CSD		27 /*ac:R1 programing of programmable bits of CSD */

/* class-4 Block oriented write protection commadns */
#define MMC_CMD28_SET_WRITE_PROT		28 /*ac: R1b sets write protection bits of address group*/
#define MMC_CMD29_CLR_WRITE_PROT		29 /*ac: R1b clears the write protection bit of address group */
#define MMC_CMD30_SEND_WRITE_PORT	30 /*adtc: R1b ask card send status of write protection bits */
#define MMC_CMD28_x			28 /*ac: R1b */
#define MMC_CMD28_y			28 /*ac: R1b */
#define MMC_CMD28_z			28 /*ac: R1b */

/* class-5 Erase commadns */
#define MMC_CMD32_ERASE_WR_BLK_START	32 /*ac:R1 set start adr of first write block to be erased */
#define MMC_CMD33_ERASE_WR_BLK_END	33 /*ac:R1 set start adr of last write block to be erased */
#define MMC_CMD38_ERASE			38 /*ac:R1 erases all previiously selected write blocks*/
#define MMC_CMD33_WERITE_BLOCK		25 /*ac:R1 write multiple block */
#define MMC_CMDe8_PROGRAM_CSD		27 /*ac:R1 programing of programmable bits of CSD */

#define MMC_ACMD41_SD_SEND_OP_COND	41 /*R2-initiazation cmd, param(XPC, HCS, S18R) */
/* ACMD41 arguments definitions, other bits are reserved */
	#define ACMD41_HCS_SDXC_SUPPORT		BIT(30) /* 0-SDSC only host, 1-SDHC/SDXC support */
	#define ACMD41_XPC_MAX_PERFORMANCE	BIT(28) /* card shall perform, 0-Power saving, 1-Max performance */
	#define ACMD41_S18R_USE_18V		BIT(24) /* 0-Use current signal voltage, 1-switch to 1.8V */
	#define ACMD41_OCR_BIT21		BIT(21) /* OCR[23:08]-XXXXH bit21 shall be set, but not clear why? TODO */
/* ACMD41 Response-R2 definitions, other bits are reserved */
	#define ACMD41_R2_CARD_INIT_COMPLETE 	BIT(31) /* 0- busy or card is initializing, 1-init complete */
	#define ACMD41_RESP_VALID	 	BIT(31) /* 0- bit30/29/24 is not valid, 1-bit30/29/24 is valid */
	#define ACMD41_R2_CARD_CAPACITY_STS	BIT(30) /* 0- SDSC card, 1-SDHC/SDXC card */
	#define ACMD41_R2_UHS2_CARD		BIT(29) /* 0- Non-uhs2 card, 1-uhs2 card */
	#define ACMD41_R2_S18A_READY		BIT(24)	/* 0- continue with current voltage, 1-ready for switching signal voltage */

#define MMC_CMD51_SEND_SCR		51 /*R1-reads SCR SD configuration register */
#define MMC_CMD55_NXT_IS_ACMD		55 /*R1-defines to the card next cmd is an app cmd */

struct sd_resp_pkt_t {
	uint8_t cmdidx;
	uint32_t resp[4];
	uint8_t crc7;
};

/**
 * \brief sd_cmd8_arg_t
 *	sd device structure definition
 */
union sd_cmd8_arg_t {
	uint32_t word;
	struct {
		uint32_t check_pat:8;
#define MMC_CMD8_CHECK_PAT	0xAA	/* check pattern default: 10101010b */
		uint32_t volt_supplied:4;
#define MMC_CMD8_VHS_2P7_3P6	0x1	/* voltage supplied 2.7 to 3.6V */	
		uint32_t res:20;
	}bit;
};

/**
 * \brief sd_resp7_t
 *	sd card interface condition
 */
union sd_resp7_t {
	uint32_t word;
	struct {
		uint32_t echo_pat:8;
		uint32_t volt_accepted:4;
		uint32_t res:20;
	}bit;
};

struct sd_cmd_pkt_t {
	uint8_t cmdidx;
	uint32_t args;
	uint8_t crc7;
};
#endif /* _MMC_CMD_H */
