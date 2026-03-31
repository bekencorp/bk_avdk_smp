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
* \file		: emmc_card.c
* \author	: ravibabu@synopsys.com
* \date		: 21-Jan-2020
* \brief	: emmc_card.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/


#include <stdio.h>
#include <string.h>
#include "common.h"
#include "cee.h"
#include "csem.h"
#include "ctask.h"
#include "cmsg_queue.h"
#include "clog.h"
#include "error.h"
#include "clog.h"
#include "delay.h"

#include "mmc_dev.h"
#include "sd_cmd.h"
#include "mmc_core.h"
#include "mshc_regs.h"
#include "sd_cmds.h"
#include "sd_card.h"

/* globals */
struct cmd_param_t *cur_emmc_cmd = NULL;

/* externs */
extern uint32_t g_block_addr, g_pattern, g_block_cnt, g_data_len;
extern uint8_t	is_emmc, use_read_cmd, txphase, rx_phase;
extern uint8_t result[];
extern int get_tuning_value(uint8_t *res, uint8_t len, uint8_t *tune_value);
extern int ascii2hex(char *str, int *err_flag);

/** \brief: mmc_dev_enumerate
 * 	enumerate the device and initialize to known state
 * \param mmc_dev: pointer to mmc_dev
 * \param mmc_cmd: pointer to mmc_cmd
 * \param speed_mode: speed-mode
 * \param bus_width : sd bus width (4/8 bit)
 * \param xfer_mode : pio/sdma/adma mode (only pio supported)
 * \param emmc_vdd : vdd 1.8V
 * \param mmcm_clk : mmcm clock in Hz
 * \param usr_intf : wait for user interface cmd for read/write blocks & tuning
 * \returns
 */
int emmc_dev_enumerate(struct sd_card_t *sd_card, struct cmd_param_t *mmc_cmd, uint8_t speed_mode,
                       uint8_t bus_width, uint8_t xfer_mode, uint8_t emmc_vdd,
                       uint32_t mmcm_clock, uint8_t usr_intf)
{
#define XSTATE_CHK_MMC_CARD_DETECTED	XSTATE0
#define XSTATE_SET_CARD_IDLE		    XSTATE1
#define XSTATE_CHK_SD_INTF_COND		    XSTATE2
#define XSTATE_CHK_SD_OP_COND		    XSTATE3
#define XSTATE_SWITCH_TO_1P8V		    XSTATE4
#define XSTATE_GET_CARD_CID		        XSTATE5
#define XSTATE_GET_CARD_REL_ADR 	    XSTATE6
#define XSTATE_GET_CARD_CSD		        XSTATE7
#define XSTATE_SELECT_CARD		        XSTATE8
#define XSTATE_GET_CARD_SCR		        XSTATE9
#define XSTATE_GET_CARD_STATUS	    	XSTATE10
#define XSTATE_SWITCH_CARD_FN	    	XSTATE11
#define XSTATE_SET_BUS_WIDTH	    	XSTATE12
#define XSTATE_SET_SPEED_MODE	    	XSTATE13
#define XSTATE_READ_ONE_BLOCK	    	XSTATE14
#define XSTATE_WRITE_ONE_BLOCK		    XSTATE15
#define XSTATE_RW_USER_INTERFACE	    XSTATE16
#define XSTATE_GET_EXT_CSD		        XSTATE21
#define XSTATE_EMMC_TX_TUNING		    XSTATE22
#define XSTATE_EMMC_RX_TUNING	    	XSTATE23
#define XSTATE_OVER			            XSTATE24

    uint8_t state, done_status = 1;
    struct mmc_dev_t *mmc_dev;
    int retval = 1, i, err;
    int cmd;
    char str[40];
    uint8_t emmc_bus_width;

    mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;
    state = sd_card->fn_state;

    switch (state) {
        case XSTATE_CHK_MMC_CARD_DETECTED:
            if (sd_card_is_inserted(sd_card)) {
                clog_print(CLOG_INFO, "==============CARD DETECTED (1) ============\n");
                clog_print(CLOG_INFO, "speed_mode = %d\nbus_width = %d\nxfer_mode = %d\nemmc_vdd=%d\nmmcm_clk=%d\n",
                           speed_mode, bus_width, xfer_mode, emmc_vdd, mmcm_clock);
                if (cur_emmc_cmd == NULL)
                    cur_emmc_cmd = mmc_cmds_alloc(0);

                if (cur_emmc_cmd) {
                    state = XSTATE_SET_CARD_IDLE;
                    cur_emmc_cmd->state = XSTATE_CMD_INIT;
                }
            } else
                break;
        case XSTATE_SET_CARD_IDLE:
            retval = mmc_cmd0_card_set_idle(mmc_dev, cur_emmc_cmd, 0);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_CHK_SD_OP_COND;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS)
                    mmc_cmd_reinit(cur_emmc_cmd);
                else
                    state = XSTATE_OVER;
            }
            break;
        case XSTATE_CHK_SD_OP_COND:
#define EMMC_CMD1_ARG		0x40200000
#define EMMC_DEV_INIT_COMPLETE	(1 << 31)
            retval = mmc_cmd1_emmc_send_op_cond(mmc_dev, cur_emmc_cmd, EMMC_CMD1_ARG);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_GET_CARD_CID;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    clog_print(CLOG_INFO, "resp=%x\n", cur_emmc_cmd->cur_cmd.resp[0]);
                    if ((cur_emmc_cmd->cur_cmd.resp[0] & EMMC_DEV_INIT_COMPLETE)
                            != EMMC_DEV_INIT_COMPLETE)
                        state = XSTATE_CHK_SD_OP_COND;
                    else
                        clog_print(CLOG_INFO, "emmc device init complete\n");
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_SWITCH_TO_1P8V:
#define	EMMC_SET_1P8V	0x80
            retval = mmc_cmd11_voltage_switch(mmc_dev, cur_emmc_cmd, 0);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_GET_CARD_CID;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    /* disable and enable the mmc clocks */
                    clog_print(CLOG_INFO, "enable clock %dKhz\n",
                               mmcm_clock/1000);
                    mmc_clock_enable(sd_card->mmc_dev, speed_mode, emmc_vdd,
                                     mmcm_clock);
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_GET_CARD_CID:
            retval = mmc_cmd2_get_card_cid(mmc_dev, cur_emmc_cmd, 0);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_GET_CARD_REL_ADR;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    for (i = 0; i < 4; ++i)
                        sd_card->cid.word[i] = cur_emmc_cmd->cur_cmd.resp[i];
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_GET_CARD_REL_ADR:
            retval = mmc_cmd3_send_rel_adr(mmc_dev, cur_emmc_cmd, 0);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_GET_CARD_CSD;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    sd_card->rca = (cur_emmc_cmd->cur_cmd.resp[0] >> 16) & 0xFFFF;
                    clog_print(CLOG_INFO, "sd-card rel-addr is %d\n", sd_card->rca);
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_GET_CARD_CSD:
            retval = mmc_cmd9_get_csd(mmc_dev, cur_emmc_cmd, sd_card->rca);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_SELECT_CARD;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    for (i = 0; i < 4; ++i)
                        sd_card->csd.word[i] = cur_emmc_cmd->cur_cmd.resp[i];
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_SELECT_CARD:
            retval = mmc_cmd7_card_select(mmc_dev, cur_emmc_cmd, sd_card->rca);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_GET_EXT_CSD;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_GET_EXT_CSD:
            retval = emmc_cmd8_get_ext_csd(mmc_dev, cur_emmc_cmd, (uint8_t *)&sd_card->ext_csd,
                                           sizeof(union card_ext_csd_t), 0);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_SET_BUS_WIDTH;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    clog_print(CLOG_INFO, "card-ext-csd data\n");
                    print_data_buf(&sd_card->ext_csd, sizeof(union card_ext_csd_t));
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_GET_CARD_STATUS:
            retval = mmc_acmd13_get_status(mmc_dev, cur_emmc_cmd, &sd_card->status, 64);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_SWITCH_CARD_FN;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    for (i = 0; i < 16; ++i)
                        clog_print(CLOG_INFO, "acmd13-status.w%d %08x\n", i,
                                   sd_card->status.word[i]);
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_SET_BUS_WIDTH:
            switch (bus_width) {
                case SD_BUSWIDTH_4:
                    emmc_bus_width = EMMC_SET_BUSWIDTH_4BIT;
                    break;
                case SD_BUSWIDTH_8:
                    emmc_bus_width = EMMC_SET_BUSWIDTH_8BIT;
                    break;
                default:
                    emmc_bus_width = EMMC_SET_BUSWIDTH_1BIT;
                    break;
            }

            retval = emmc_cmd6_set_ext_csd(mmc_dev, cur_emmc_cmd, ACCESS_MODE_WRITE_BYTE,
                                           EXTCSD_IDX183_BUSWIDTH,	emmc_bus_width,	S_CMD_SET1);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_SET_SPEED_MODE;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_set_bus_width(sd_card->mmc_dev, bus_width);
                    mmc_set_voltage(sd_card->mmc_dev, XSD_BUS_PWR_VDD1, emmc_vdd);
                    clog_print(CLOG_INFO, "emmc buswidth set to 8 bit\n");
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
                os_delay_ms(200);
            }
            break;
        case XSTATE_SET_SPEED_MODE:
            retval = emmc_cmd6_set_ext_csd(mmc_dev, cur_emmc_cmd, ACCESS_MODE_WRITE_BYTE,
                                           EXTCSD_IDX184_HS_TIMING, speed_mode, S_CMD_SET1);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = (usr_intf) ? XSTATE_RW_USER_INTERFACE : XSTATE_OVER;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_set_speed_mode(mmc_dev, speed_mode);
                    clog_print(CLOG_INFO, "emmc set hs200 speed mode\n");
                    mmc_clock_enable(sd_card->mmc_dev, speed_mode, emmc_vdd, mmcm_clock);
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_READ_ONE_BLOCK:
            retval = mmc_read_block(mmc_dev, cur_emmc_cmd, g_block_addr, g_block_cnt,
                                    (uint8_t *)cur_emmc_cmd->cur_cmd.databuf, g_data_len);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_RW_USER_INTERFACE;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    print_data_buf(cur_emmc_cmd->cur_cmd.databuf, 512 * g_block_cnt);
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_WRITE_ONE_BLOCK:
            retval = mmc_write_block(mmc_dev, cur_emmc_cmd, g_block_addr, g_block_cnt,
                                     (uint8_t *)cur_emmc_cmd->cur_cmd.databuf, g_data_len);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_RW_USER_INTERFACE;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_OVER;
                }
            }
            break;
        case XSTATE_EMMC_TX_TUNING:
            retval = mmc_sd_tx_tuning(mmc_dev, cur_emmc_cmd, speed_mode, emmc_vdd,
                                      mmcm_clock, result);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                clog_print(CLOG_INFO, "Tx tuning is complete\n");
                state = XSTATE_RW_USER_INTERFACE;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_RW_USER_INTERFACE;
                }
                /* find the windows */
                get_tuning_value(result, 128, &txphase);
            }
            break;
        case XSTATE_EMMC_RX_TUNING:
            retval = mmc_sd_rx_tuning(mmc_dev, cur_emmc_cmd, speed_mode, emmc_vdd,
                                      mmcm_clock, txphase, is_emmc, bus_width,
                                      use_read_cmd, result);
            if (cur_emmc_cmd && (cur_emmc_cmd->state == XSTATE_CMD_DONE)) {
                state = XSTATE_RW_USER_INTERFACE;
                if (cur_emmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_cmd_reinit(cur_emmc_cmd);
                } else {
                    state = XSTATE_RW_USER_INTERFACE;
                }
                /* find the windows */
                get_tuning_value(result, 128, &rx_phase);
                clog_print(CLOG_INFO, "Rx tuning is complete, optimum rx_phase(%d)\n", rx_phase);
            }
            break;
        case XSTATE_RW_USER_INTERFACE:
            printf("enter option 1:read block, 2: write block, 3: Tx tuning, 4: Rx tuning, 5:exit\n");
            scanf("%d", &cmd);
            if (cmd == 1)
                state = XSTATE_READ_ONE_BLOCK;
            else if (cmd == 2)
                state = XSTATE_WRITE_ONE_BLOCK;
            else if (cmd == 3) {
                state = XSTATE_EMMC_TX_TUNING;
            } else if (cmd == 4) {
                state = XSTATE_EMMC_RX_TUNING;
            } else {
                printf("invalid input.. try again\n");
                if (cmd == 5)
                    state = XSTATE_OVER;
                break;
            }

            if (cmd == 4) {
                printf("current tx_phase = %d\n", txphase);
                printf("enter 1-sdcard, 2-emmc\n");
                scanf("%d", &cmd);
                if (cmd == 1)
                    is_emmc = 0;
                else
                    is_emmc = 1;
                printf("enter use tuning(CMD19/CMD21) cmd? 1-Yes, 0-No\n");
                scanf("%d", &cmd);
                use_read_cmd = 0;
                if (cmd == 0)
                    use_read_cmd = 1;
                break;
            }
            printf("enter block address\n");
            scanf("%4s", str);
            err = 0;
            g_block_addr = ascii2hex(str, &err);
            if (err) {
                printf("invalid input.. try again\n");
                break;
            }

            printf("enter number of blocks (block_cnt)\n");
            scanf("%u", &g_block_cnt);
            if (g_block_cnt < 1)
                g_block_cnt = 1;

            if (g_block_cnt > 128)
                g_block_cnt = 128;

            g_data_len = g_block_cnt * 512;

            if (cur_emmc_cmd) {
                cur_emmc_cmd->cur_cmd.databuf = mmc_alloc_buf(g_data_len, 0);
                memset(cur_emmc_cmd->cur_cmd.databuf, 0, g_data_len);
            }

            if (cmd == 2) {
                printf("enter 16-bit pattern to be filled\n");
                scanf("%4s", str);
                g_pattern = ascii2hex(str, &err);
                if (err) {
                    printf("invalid input.. try again\n");
                    break;
                }

                buffer_fill(&cur_emmc_cmd->cur_cmd.databuf[0],
                            g_block_cnt * 512, g_pattern, 1);
            }
            break;
        case XSTATE_OVER:
            clog_print(CLOG_INFO, "================ finish =================\n");
            state = XSTATE_OVER + 1;
            done_status = XSTATE_CMD_DONE;
            cur_emmc_cmd->state = XSTATE_CMD_DONE;
            clog_closefile();
            break;
    }

    clog_print(CLOG_LEVEL10, "%s: retval=%d\n", __func__, retval);

    sd_card->fn_state = state;
    return done_status;
}
