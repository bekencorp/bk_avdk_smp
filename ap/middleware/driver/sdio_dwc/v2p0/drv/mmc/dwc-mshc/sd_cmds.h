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
* \file		: sd_cmds.h
* \author	: ravibabu@synopsys.com
* \date		: 01-Dec-2019
* \brief	: sd_cmds.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	01-Dec-2019	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _MMC_CMDS_H_
#define _MMC_CMDS_H_

#include "dwc_type.h"
#include "error.h"
#include "mmc_dev.h"

#define CMDIDX_MASK		0x3F

/* Respone-1 : section 7.3.2.1 Format R1, part-1 phy-spec ver6.00 */
#define SD_RESP1_IN_IDLE_STATE		BIT(0)
#define SD_RESP1_ERASE_RESET		BIT(1)
#define SD_RESP1_ILLEGAL_CMD		BIT(2)
#define SD_RESP1_COMCRC_ERROR		BIT(3)
#define SD_RESP1_ERASE_SEQERR		BIT(4)
#define SD_RESP1_ADDR_ERROR		BIT(5)
#define SD_RESP1_PARAM_ERROR		BIT(6)

/* Respone-2 : section 7.3.2.3 Format R2, part-1 phy-spec ver6.00 */
#define SD_RESP2_CARD_LOCKED		BIT(0)
#define SD_RESP2_WPERASE_SKIP		BIT(1)
#define SD_RESP2_LOCK_UNLOCK_FAIL	BIT(1)
#define	SD_RESP2_ERR			BIT(2)
#define SD_RESP2_CC_ERROR		BIT(3)
#define SD_RESP2_CARD_ECC_FAILED	BIT(4)
#define SD_RESP2_WP_VIOLATION		BIT(5)
#define SD_RESP2_ERASE_PARAM		BIT(6)
#define SD_RESP2_OUT_OF_RANGE		BIT(7)
#define SD_RESP2_CSD_OVERWRITE		BIT(7)
#define SD_RESP2_IN_IDLE_STATE		BIT(8)
#define SD_RESP2_ERASE_RESET		BIT(9)
#define SD_RESP2_ILLEGAL_CMD		BIT(10)
#define SD_RESP2_COMCRC_ERROR		BIT(11)
#define SD_RESP2_ERASE_SEQERR		BIT(12)
#define SD_RESP2_ADDR_ERROR		BIT(13)
#define SD_RESP2_PARAM_ERROR		BIT(14)


/* \brief: parameter for MMC_CMD0_GO_IDLE, CMD_WITHOUT_DATA, NO_RESPONSE
 */
struct cmd0_param_t {
    uint8_t boot_mode; /* default 0, boot mode for cmd0*/
};

/* \brief: parameter for EMMC_CMD1_SEND_OP_COND,
 */
struct cmd1_param_t {
    uint32_t args;
};

/* \brief: parameter for MMC_CMD2_ALL_SEND_CID, CMD_WITHOUT_DATA, MMC_RESP1_LEN_136
 */
struct cmd2_param_t {
    uint32_t args;
};

/* \brief: parameter for MMC_CMD3_SEND_REL_ADR, CMD_WITHOUT_DATA, MMC_RESP2_LEN_48
 */
struct cmd3_param_t {
    uint16_t rca;	/* card relative address */
};

/* \brief: parameter for MMC_CMD7_CARD_SELECT, CMD_WITHOUT_DATA, MMC_RESP2_LEN_48
 */
struct cmd7_param_t {
    uint16_t rca;
};

/* \brief: parameter for MMC_CMD8_SEND_IF_COND, CMD_WITHOUT_DATA, MMC_RESP2_LEN_48
 */
struct cmd8_param_t {
    uint8_t volt_supplied;	/* set to 1 */
    uint8_t check_pattern; /* any check pattern, card returns same check pattern intf supported */
};

/* \brief: parameter for EMMC_CMD8_SEND_EXT_CSD, WITH_DATA, MMC_RESP2_LEN_48
 */
struct cmd8_emmc_param_t {
    uint32_t args;
};

/* \brief: parameter for MMC_CMD11_VOLT_SWITCH, CMD_WITHOUT_DATA, MMC_RESP2_LEN_48
 */
struct cmd11_param_t {
    uint32_t args;
};

/* \brief: parameter for MMC_CMD9_SEND_CSD, CMD_WITHOUT_DATA, MMC_RESP1_LEN_136
 */
struct cmd9_param_t {
    uint16_t rca;
};

/* \brief: parameter for MMC_CMD13_SEND_STATUS, CMD_WITH_DATA(64bytes), MMC_RESP2_LEN_48
 */
struct cmd13_param_t {
    uint16_t rca;
};

/* \brief: parameter for MMC_CMD6_SWITCH_FN, CMD_WITH_DATA (64 bytes), MMC_RESP2_LEN_48
 */
struct cmd6_param_t {
    uint8_t is_acmd;
    uint8_t speed_mode;
    uint8_t bus_width;
};

/* \brief: parameter for MMC_ACMD6_SWITCH_FN, CMD_WITHOUT_DATA, MMC_RESP2_LEN_48
 */
struct cmd6_emmc_param_t {
    uint8_t access_mode;
    uint8_t index;
    uint8_t value;
    uint8_t cmd_set;
};

/* \brief: parameter for MMC_ACMD6_SWITCH_FN, CMD_WITHOUT_DATA, MMC_RESP2_LEN_48
 */
struct acmd6_param_t {
    uint8_t is_acmd;
    uint8_t bus_width;
};


/* \brief: parameter for MMC_CMD17_READ_ONE_BLOCK, CMD_WITH_DATA(512byes),MMC_RESP2_LEN_48
 */
struct cmd17_param_t {
    uint32_t block_addr;	/* start block address */
};

/* \brief: parameter for MMC_CMD18_READ_MULTI_BLOCK, CMD_WITH_DATA(N*512),MMC_RESP2_LEN_48
 */
struct cmd18_param_t {
    uint32_t block_addr;	/* start block address */
    uint32_t nblocks;	/* number of blocks */
};

/* \brief: parameter for MMC_CMD21_READ_TUNING_BLOCK,MMC_RESP2_LEN_48
 */
struct cmd21_param_t {
    uint8_t bus_width;	/* eMMC bus-width 4 or 8 */
};

/* \brief: parameter for MMC_CMD23_SET_BLOCKCNT, CMD_WITHOUT_DATA, MMC_RESP1_LEN_136
 */
struct cmd23_param_t {
    uint32_t block_cnt;
};

/* \brief: parameter for MMC_CMD24_WRITE_ONE_BLOCK, CMD_WITH_DATA(N*512),MMC_RESP2_LEN_48
 */
struct cmd24_param_t {
    uint32_t block_addr;	/* start block address */
};

/* \brief: parameter for MMC_CMD25_WRITE_MULTIPLE BLOCK, CMD_WITH_DATA(N*512),MMC_RESP2_LEN_48
 */
struct cmd25_param_t {
    uint32_t block_addr;	/* start block address */
    uint32_t nblocks;
};

/* \brief: parameter for MMC_ACMD41_SD_SEND_OP_COND, CMD_WITHOUT_DATA, MMC_RESP2_LEN_48
 */
struct cmd41_param_t {
    uint8_t is_hcs_sdxc_support;
    uint8_t is_max_perf;
    uint8_t s18r_use_18v;
    uint8_t is_ocr;
};

/* \brief: parameter for MMC_CMD51_SEND_SCR, CMD_WITH_DATA(8bytes), MMC_RESP2_LEN_48
 */
struct cmd51_param_t {
    uint8_t is_acmd;
};

/* \brief: parameter for MMC_CMD55_NXT_IS_ACMD, CMD_WITHOUT_DATA, MMC_RESP2_LEN_48
 */
struct cmd55_param_t {
    uint16_t rca;
};

/** \brief: mmc_cmds_go_idle
 *	The command GO_IDLE_STATE (CMD0) is the software reset command and sets each
 * card into Idle State regardless of the current card state. Cards in Inactive State
 * are not affected by this command. After power-on by the host, all cards are in Idle
 * State, including the cards that have been in Inactive State before.
 *	After power-on or CMD0, all cards’ CMD lines are in input mode, waiting for
 * start bit of the next command. The cards are initialized with a default relative card
 * address (RCA=0x0000) and with a default driver stage register setting (lowest speed,
 * highest driving current capability).
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param boot-mode: cmd0 boot-mode argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd0_card_set_idle(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint8_t boot_mode);

/** \brief: mmc_cmd1_emmc_send_op_cond
 *	prepare for cmd1 emmc send_operating condition
 *	CMD1 is special synchronization command used to negotiate the operation
 * volatage range and to poll the device until it is out of its power up sequence.
 * In addition to operation voltage profile of the emmc device, the response to CMD1
 * contains busy flag indicating that the device is still working on it spower up
 * procedure and is not ready for identification. This bit informs the host that the
 * device is not ready, and the host must wait until this bit is cleared.
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param args: cmd specific argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd1_emmc_send_op_cond(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint32_t args);

/** \brief: mmc_cmd3_send_rel_adr
 *	prepare for cmd3 send relative address
 *	ask the card to publish the new relativ address
 *	device respond with response type r6 has following format
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param boot-mode: cmd0 boot-mode argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd3_send_rel_adr(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint8_t rca);

/** \brief: mmc_cmd11_voltage_switch
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param args: cmd specific argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd11_voltage_switch(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint32_t args);

/** \brief: mmc_cmd2_get_cid
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param args: cmd specific argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd2_get_card_cid(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint32_t args);

/** \brief: mmc_cmd7_card_select
 *	prepare for cmd7 for card select/deselect
 *	host resend the CMD3 to change its RCA number
 *	other than 0 and then use CMD7 with RCA=0 for
 *	card de-selection
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param args: cmd specific argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd7_card_select(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint16_t rca);

/** \brief: mmc_cmd9_get_csd
 *	prepare for cmd9 for send card's csd data
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param args: cmd specific argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd9_get_csd(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint16_t rca);

/** \brief: mmc_cmd13_send_status
 *	cmd13, request card to send 64-byte(512-bits) status
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param card_status_buf: status buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd13_send_status(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                          void *card_status_buf, uint32_t buflen);

/** \brief: mmc_acmd13_get_status
 * 	The application specific command ACMD13 to get card status
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_acmd13_get_status(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                          void *databuf, uint32_t buflen);

/** \brief: mmc_cmd6_switch_function
 *		prepare for cmd6, switch function
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param speed_mode: mmc/sd speed_mode
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd6_switch_function(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                             uint8_t speed_mode, void *databuf, uint32_t buflen);

/** \brief: mmc_acmd6_switch_function
 *		prepare for cmd6, switch function
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param bus_width: mmc/sd bus-width
 * \returns succes/fail on cmd command completion
 */
int mmc_acmd6_switch_function(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                              uint8_t bus_width);

/** \brief: mmc_cmd6_set_ext_csd
 *		prepare for cmd6, set_ext_csd
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param access_mode: cmd access-mode
 * \param index: index to csd data byte
 * \param value: value need to set
 * \param cmd_set: cmd operation
 * \returns succes/fail on cmd command completion
 */
int emmc_cmd6_set_ext_csd(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                          uint8_t access_mode, uint8_t index, uint8_t value, uint8_t cmd_set);

/** \brief: mmc_acmd6_set_buswidth
 * 	The application specific command ACMD6 to set bus-width
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param bus_width: 4 or 8 bit bus-width
 * \returns succes/fail on cmd command completion
 */
int mmc_acmd6_set_buswidth(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                           uint8_t bus_width);

/** \brief: mmc_read_block
 * 	 API to read blocks from sd-card
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param block_addr: sd block address
 * \param block_cnt: nubmer of blocks to read
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_read_block(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                   uint32_t block_addr, uint32_t block_cnt, uint8_t  *databuf, uint32_t buflen);

/** \brief: mmc_set_block_cnt
 *		cmd23, set bus block_cnt
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param block_cnt: number of blocks
 * \returns succes/fail on cmd command completion
 */
int mmc_set_block_cnt(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                      uint32_t block_cnt);

/** \brief: mmc_write_block
 * 	 API to write blocks from sd-card
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param block_addr: sd block address
 * \param block_cnt: nubmer of blocks to write
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_write_block(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                    uint32_t block_addr, uint32_t block_cnt, uint8_t  *databuf, uint32_t buflen);

/** \brief: mmc_cmd8_check_card_intf_condition
 * 	The command GO_send_if_cond (CMD8) to check whether card support current voltage
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param volt_supplied: volt supply
 * \param check_pattern: check pattern
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd8_check_card_intf_condition(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                                       uint8_t volt_supplied, uint8_t check_pattern);

/** \brief: emmc_cmd8_get_ext_csd
 * 	The command SEND_EXT_CSD (CMD8) issued to emmc device to get ext-csd data
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int emmc_cmd8_get_ext_csd(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                          uint8_t  *databuf, uint32_t buflen, uint32_t args);

/** \brief: mmc_cmd55_next is application cmd
 * 	The command send to inform card that next command is acmd
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param rca: relative card address
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd55_next_is_acmd(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                           uint16_t rca);

/** \brief: mmc_acmd41_send_op_cond
 * 	The command GO_send_if_cond (CMD8) to check whether card support current voltage
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param is_sdxc_support: enable card extended capacity support
 * \param iuse_1p8v: use low power 1.8V
 * \param is_max_perf : configure for card max performance
 * \param is_ocr : ocr support
 * \returns succes/fail on cmd command completion
 */
int mmc_acmd41_check_card_op_condition(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                                       uint8_t is_sdxc_support, uint8_t use_1p8v, uint8_t is_max_perf, uint8_t is_ocr);

/** \brief: mmc_acmd51_get_card_scr
 * 	The application specific command ACMD51 to get card SCR
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_acmd51_get_card_scr(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                            void *databuf, uint32_t buflen);

/** \brief: mmc_sd_tx_tuning
 *	performs the mmc/sd manual tx tuning
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param speed_mode: sd speed_mode
 * \param emmc-vdd: mmc voltage 1.8/3.3V
 * \param mmcm-clock: external mmcm-clock in HZ
 * \param result : 128 window result table pass/fail status
 * \returns succes/fail on cmd command completion
 */
int mmc_sd_tx_tuning(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                     uint8_t speed_mode, uint8_t emmc_vdd, uint32_t mmcm_clock, uint8_t *result);

/** \brief: mmc_sd_rx_tuning
 *	performs the mmc/sd manual rx tuning
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param speed_mode: sd speed_mode
 * \param emmc-vdd: mmc voltage 1.8/3.3V
 * \param mmcm-clock: external mmcm-clock in HZ
 * \param tx_phase: tx-phase value
 * \param is_emmc: device is emmc or sd-mmc
 * \param is_read_cmd: use read-cmd or tuning block cmd
 * \param result : 128 window result table pass/fail status
 * \returns succes/fail on cmd command completion
 */
int mmc_sd_rx_tuning(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                     uint8_t speed_mode, uint8_t emmc_vdd, uint32_t mmcm_clock, uint8_t tx_phase,
                     uint8_t is_emmc, uint8_t bus_width, uint8_t use_read_cmd, uint8_t *result);

/** \brief: mmc_cmd19_tuning_block
 *	cmd19 read tuning block
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd19_read_tuning_block(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                                uint8_t  *databuf, uint32_t buflen);

/** \brief: mmc_cmd21_read_tuning_block
 *	cmd21 read tuning block
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param bus_width: mmc/sd bus width
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd21_read_tuning_block(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                                uint8_t bus_width, uint8_t  *databuf, uint32_t buflen);

/** \brief: mmc_cmds_init
 * 	initialize the mmc cmd request pool
 * \param flags : memory allocation flags (not used)
 * \returns none
 */
void mmc_cmds_init(uint8_t flags);

/** \brief: mmc_cmd_reinit
 *	re-init mmc-cmd to init state
 * \param mmc_cmd: pointer to mmc_cmd-param
 * \returns none
 */
void mmc_cmd_reinit(struct cmd_param_t *mmc_cmd);

/** \brief: mmc_cmds_free
 * 	free the allocated mmc request from pool
 * \param pointer to mmc_request
 * \returns none
 */
void mmc_cmds_free(struct cmd_param_t *cmd);

/** \brief: mmc_cmds_alloc
 * 	allocate mmc mmc_cmds from free pool
 * \param flags : memory allocation flags (not used)
 * \returns none
 */
struct cmd_param_t *mmc_cmds_alloc(uint8_t flags);
#endif /* _MMC_CMDS_H_ */
