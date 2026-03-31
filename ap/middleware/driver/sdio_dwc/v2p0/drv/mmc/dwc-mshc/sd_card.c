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
* \file		: sd_card.c
* \author	: ravibabu@synopsys.com
* \date		: 10-Dec-2019
* \brief	: sd_card.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	10-Dec-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "cee.h"
#include "csem.h"
#include "ctask.h"
#include "cmsg_queue.h"
#include "clog.h"
#include "error.h"
#include "clog.h"

#include "hal.h"
#include "mmc_dev.h"
#include "sd_cmd.h"
#include "mmc_core.h"
#include "mshc_regs.h"
#include "sd_cmds.h"
#include "sd_card.h"
#include "mmcm_clk.h"
#include "os/mem.h"

#define MAX_MMC_CARDS	1

/* globals */
struct sd_card_t sd_cards[MAX_MMC_CARDS];

uint8_t result[128];
uint8_t txphase = 0x3f;
uint8_t	is_emmc, use_read_cmd;
uint32_t g_block_addr, g_pattern = 0x5a5a0000, g_block_cnt = 1, g_data_len;
extern uint8_t rx_phase;
char g_logbuf[180];
uint32_t g_block_trx_flag = 0;

/* extern */
extern int ascii2hex(char *str, int *err_flag);

/** \brief: sd_card_alloc
 * 	allocate mmc cards from free pool
 * \param flags : memory allocation flags
 * \returns none
 */
struct sd_card_t *sd_card_alloc(uint8_t flags)
{
    int i;
    for (i = 0; i < MAX_MMC_CARDS; ++i) {
        if (sd_cards[i].in_use == 0)
            break;
    }

    if (i >= MAX_MMC_CARDS)
        return NULL;

    memset(&sd_cards[i], 0, sizeof(struct sd_card_t));
    sd_cards[i].in_use = 1;
    return &sd_cards[i];
}

/** \brief: sd_card_free
 * 	free the allocated mmc card to pool
 * \param card: pointer to sd_card object
 * \returns none
 */
void sd_card_free(struct sd_card_t *card)
{
    card->in_use = 0;
}

/** \brief: buffer_fill
 * 	fill buffer with required pattern type
 * \param buffer: buffer pointer
 * \param len : buffer length
 * \param pattern : pattern to be filled
 * \param type : 0-user defined pattern, 1-random pattern
 * \returns none
 */
void buffer_fill(void *databuf, uint32_t len, uint32_t pattern, uint8_t type)
{
    int i, j, k;
    uint32_t *pbuf = NULL;
    uint32_t count;

    pbuf = (uint32_t *)databuf;
    count = len / sizeof(pbuf[0]);
    for (j = k = 0; j < count; ++k) {
        for (i = 0; i < 128; i ++) {
            pbuf[j+i] = i + 0xffff0000;
        }
        j += 128;
    }
}

/** \brief: sd_card_module_init
 * 	initialize the mmc-card module
 * \param none
 * \returns none
 */
void sd_card_module_init(void)
{
    int i;

    for (i = 0; i < MAX_MMC_CARDS; ++i) {
        memset(&sd_cards[i], 0, sizeof(struct sd_card_t));
    }
}


/** \brief: print_data_buf
 * 	print data buffer
 * \param buf: buffer pointer
 * \param len : buffer length
 * \returns none
 */
void print_data_buf(void *buf, uint32_t len)
{
    uint8_t *pbuf = buf;
    int i;
    uint32_t chksum = 0, val;
    char tempbuf[20];

    clog_print(CLOG_INFO, "============ data buf, len(%d)========\n", len);
    strcpy(g_logbuf, "");
    for (i = 0; i < len; i+=4) {
        if (i % 512 == 0) {
            if (i > 0) {
                clog_print(CLOG_INFO, "%s\n", g_logbuf);
                strcpy(g_logbuf, "");
                clog_print(CLOG_INFO, "\n-------------block_offs (%d) blksum(%d)---------------\n",
                           i - 512, chksum);
            }
            chksum = 0;
        }
        if (i % 32 == 0) {
            if (i)
                clog_print(CLOG_INFO, "%s\n", g_logbuf);
            sprintf(g_logbuf, "%04x :", i);
        }
        val = *(uint32_t *)&pbuf[i];
        chksum += val;
        sprintf(tempbuf, " %08X", val);
        strcat(g_logbuf, tempbuf);
    }
    if (i > 0) {
        clog_print(CLOG_INFO, "%s\n", g_logbuf);
        strcpy(g_logbuf, "");
        clog_print(CLOG_INFO, "\n-------------block_offs (%d) blksum(%d)---------------\n",
                   i - 512, chksum);
    }
}

/** \brief: get_tuning_value
 * 	get tuning values
 * \param res: result buffer array
 * \param len : result buffer length
 * \returns 0 on success, -ve error/fail
 * \param tune_value : returns optmial tuning value
 */
int get_tuning_value(uint8_t *res, uint8_t len, uint8_t *tune_value)
{
    int state = XSTATE_INIT;
    int win = 1, si, i, ms = 0, tune_val, wi;

    for (i = 0; i < len; ++i) {
        clog_print(CLOG_INFO, "[%02d] = (%3d) %s\n", i, result[i],
                   (result[i] == 1) ? "Pass" : "Fail");

        switch (state) {
            case XSTATE_INIT:
                si = i;
                tune_val = 0;
                if (result[i] == XPASS)
                    state = XSTATE_PASS;
                else if (result[i] == XFAIL)
                    state = XSTATE_FAIL;
                else {
                    clog_print(CLOG_ERR, "%s: invalid result\n", __func__);
                    return -1;
                }
                break;

            case XSTATE_PASS:
                if (result[i] == XFAIL) {
                    clog_print(CLOG_INFO, "==> Pass Window%d (%d, %d) - max_sample(%d),  midvalue(%d)\n",
                               win++, si, i, i-si, (si + i)/2);
                    if ( (i-si) >= ms) {
                        ms = (i - si);
                        wi = win;
                        tune_val = (si + i)/2;
                        clog_print(CLOG_INFO, "wi=%d, ms=%d val=%d\n", wi, ms, tune_val);
                    }
                    si = i;
                    state = XSTATE_FAIL;
                }
                break;
            case XSTATE_FAIL:
                if (result[i] == XPASS) {
                    clog_print(CLOG_INFO, "==> Fail Window%d (%d, %d) - max_sample(%d),  midvalue(%d)\n",
                               win++, si, i, i-si, (si + i)/2);
                    si = i;
                    state = XSTATE_PASS;
                }
                break;
            default:
                clog_print(CLOG_ERR, "%s: Illegal state\n", __func__);
                return -1;
        }
    }
    clog_print(CLOG_INFO, "==> %s Window%d (%d, %d) - max_sample(%d),  midvalue(%d)\n",
               (result[si] == XPASS) ? "Pass" : "Fail", win, si, i, i-si, (si + i)/2);

    if ( (i-si) >= ms) {
        ms = (i - si);
        wi = win;
        tune_val = (si + i)/2;
        clog_print(CLOG_INFO, "wi=%d, ms=%d val=%d\n", wi, ms, tune_val);
    }

    clog_print(CLOG_INFO, "Window%d Best tuning value = %d\n", wi, tune_val);
    if (tune_value)
        *tune_value = tune_val;
    return 0;
}

int mmc_dev_set_usr_state(struct sd_card_t *sd_card, uint8_t state)
{
    sd_card->fn_state = state;
    return 0;
}

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
int mmc_dev_enumerate(struct sd_card_t *sd_card, struct cmd_param_t *mmc_cmd,
                      uint8_t speed_mode, uint8_t bus_width, uint8_t xfer_mode,
                      uint8_t emmc_vdd, uint32_t mmcm_clock, uint8_t usr_intf)
{
    uint8_t state;
    int retval = 1, i;
    uint32_t status;
    uint8_t done_status = 1;
    struct mmc_dev_t *mmc_dev;
    struct cmd_param_t *cur_mmc_cmd;

    mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;
    state = sd_card->fn_state;

    cur_mmc_cmd = mmc_dev->cur_mmc_cmd;
    switch (state) {
        case XSTATE_CHK_MMC_CARD_DETECTED:
            if (is_sd_card_inserted()) {
                clog_print(CLOG_INFO, "==============CARD DETECTED (1) ============\n");
                if (mmc_cmd == NULL){
                    cur_mmc_cmd = mmc_cmds_alloc(0);
                }
                else{
                    cur_mmc_cmd = mmc_cmd;
                }
                mmc_dev->cur_mmc_cmd = cur_mmc_cmd;

                if (cur_mmc_cmd) {
                    state = XSTATE_SET_CARD_IDLE;
                    cur_mmc_cmd->state = XSTATE_CMD_INIT;
                    clog_print(CLOG_INFO, "speed_mode = %d\nxfer_mode=%d\nbus_width=%d\nemmc_vdd=%d\nmmcm_clk=%d\n",
                               speed_mode, xfer_mode, bus_width, emmc_vdd, mmcm_clock);
                }
            } else
                break;
        case XSTATE_SET_CARD_IDLE:
            retval = mmc_cmd0_card_set_idle(mmc_dev, cur_mmc_cmd, 0);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                clog_print(CLOG_INFO, "XSTATE_SET_CARD_IDLE\r\n");
                state = XSTATE_CHK_SD_INTF_COND;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS)
                    mmc_cmd_reinit(cur_mmc_cmd);
                else
                    state = XSTATE_FINISH;
            }
            break;
        case XSTATE_CHK_SD_INTF_COND:
            retval = mmc_cmd8_check_card_intf_condition(mmc_dev, cur_mmc_cmd, 1, 0xaa);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                clog_print(CLOG_INFO, "XSTATE_CHK_SD_INTF_COND\r\n");
                state = XSTATE_CHK_SD_OP_COND;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    clog_print(CLOG_INFO, "volt=%d, pattern=%x\n",
                               (cur_mmc_cmd->cur_cmd.resp[0] >> 8) & 0xff,
                               (cur_mmc_cmd->cur_cmd.resp[0] & 0xff));

                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_CHK_SD_OP_COND:
            retval = mmc_acmd41_check_card_op_condition(mmc_dev, cur_mmc_cmd, 1, emmc_vdd, 0, 1);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                clog_print(CLOG_INFO, "XSTATE_CHK_SD_OP_COND\r\n");
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    sd_card->acmd41_resp.status = cur_mmc_cmd->cur_cmd.resp[0];
                    status = sd_card->acmd41_resp.status;
                    sd_card->init_complete = !((status & ACMD41_RESP_VALID) == 0);
                    if (sd_card->init_complete) {
                        sd_card->is_sdxc_card = !!(status & ACMD41_R2_CARD_CAPACITY_STS);
                        sd_card->is_ush2_card = !!(status & ACMD41_R2_UHS2_CARD);
                        sd_card->s18a_volt_switch_ready = !!(status & ACMD41_R2_S18A_READY);
                        clog_print(CLOG_INFO, "sd-card supports %s, %s %s\n",
                                   sd_card->is_sdxc_card ? "sdxc" : "sdhc",
                                   sd_card->is_ush2_card ? "uhs2," : "",
                                   sd_card->s18a_volt_switch_ready ? "ready to switch 1.8v":"");

                        if (sd_card->s18a_volt_switch_ready && emmc_vdd)
                            state = XSTATE_SWITCH_TO_1P8V;
                        else
                            state = XSTATE_GET_CARD_CID;
                    } else
                        state = XSTATE_CHK_SD_OP_COND;
                }
                mmc_cmd_reinit(cur_mmc_cmd);
            }
            break;
        case XSTATE_SWITCH_TO_1P8V:
            retval = mmc_cmd11_voltage_switch(mmc_dev, cur_mmc_cmd, 0);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                clog_print(CLOG_INFO, "XSTATE_SWITCH_TO_1P8V\r\n");
                state = XSTATE_GET_CARD_CID;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    /* disable and enable the mmc clocks */
                    mmc_clock_enable(sd_card->mmc_dev, speed_mode, emmc_vdd, MMCM_CLK_400KHZ);
                    mmc_set_voltage(sd_card->mmc_dev, XSD_BUS_PWR_VDD1, emmc_vdd);
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_GET_CARD_CID:
            retval = mmc_cmd2_get_card_cid(mmc_dev, cur_mmc_cmd, 0);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                clog_print(CLOG_INFO, "XSTATE_GET_CARD_CID\r\n");
                state = XSTATE_GET_CARD_REL_ADR;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    for (i = 0; i < 4; ++i)
                        sd_card->cid.word[i] = cur_mmc_cmd->cur_cmd.resp[i];
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_GET_CARD_REL_ADR:
            clog_print(CLOG_INFO, "XSTATE_GET_CARD_REL_ADR\r\n");
            retval = mmc_cmd3_send_rel_adr(mmc_dev, cur_mmc_cmd, 0);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_GET_CARD_CSD;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    sd_card->rca = (cur_mmc_cmd->cur_cmd.resp[0] >> 16) & 0xFFFF;
                    clog_print(CLOG_INFO, "sd-card rel-addr is %d\n", sd_card->rca);
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_GET_CARD_CSD:
            clog_print(CLOG_INFO, "XSTATE_GET_CARD_CSD\r\n");
            retval = mmc_cmd9_get_csd(mmc_dev, cur_mmc_cmd, sd_card->rca);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_SELECT_CARD;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    for (i = 0; i < 4; ++i)
                        sd_card->csd.word[i] = cur_mmc_cmd->cur_cmd.resp[i];
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_SELECT_CARD:
            clog_print(CLOG_INFO, "XSTATE_SELECT_CARD\r\n");
            retval = mmc_cmd7_card_select(mmc_dev, cur_mmc_cmd, sd_card->rca);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_GET_CARD_SCR;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_GET_CARD_SCR:
            retval = mmc_acmd51_get_card_scr(mmc_dev, cur_mmc_cmd,
                                             &sd_card->scr.word[0], 8);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                clog_print(CLOG_INFO, "XSTATE_GET_CARD_SCR\r\n");
                state = XSTATE_GET_CARD_STATUS;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    clog_print(CLOG_INFO, "card scr %08x %08x\n",
                               sd_card->scr.word[0], sd_card->scr.word[1]);
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_GET_CARD_STATUS:
            clog_print(CLOG_INFO, "XSTATE_GET_CARD_STATUS\r\n");
            retval = mmc_acmd13_get_status(mmc_dev, cur_mmc_cmd, &sd_card->status, 64);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_SWITCH_CARD_FN;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    for (i = 0; i < 16; ++i)
                        clog_print(CLOG_INFO, "acmd13-status.w%d %08x\n", i,
                                   sd_card->status.word[i]);
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_SWITCH_CARD_FN:
            clog_print(CLOG_INFO, "XSTATE_SWITCH_CARD_FN\r\n");
            retval = mmc_cmd6_switch_function(mmc_dev, cur_mmc_cmd, 0,
                                              &sd_card->cmd6_query_status, 64);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_SET_BUS_WIDTH;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    for (i = 0; i < 16; ++i)
                        clog_print(CLOG_INFO, "cmd6-query-status.w%d %08x\n", i,
                                   sd_card->cmd6_query_status.word[i]);
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_SET_BUS_WIDTH:
            clog_print(CLOG_INFO, "XSTATE_SET_BUS_WIDTH\r\n");
            retval = mmc_acmd6_set_buswidth(mmc_dev, cur_mmc_cmd, bus_width);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_SET_SPEED_MODE;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    clog_print(CLOG_INFO, "set-buswidth resp %08x\n",
                               cur_mmc_cmd->cur_cmd.resp[0]);
                    clog_print(CLOG_INFO, "set buswidth to %d\n", bus_width);
                    mmc_set_bus_width(sd_card->mmc_dev, bus_width);
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_SET_SPEED_MODE:
            clog_print(CLOG_INFO, "XSTATE_SET_SPEED_MODE\r\n");
            retval = mmc_cmd6_switch_function(mmc_dev, cur_mmc_cmd, speed_mode,
                                              &sd_card->cmd6_query_status, 64);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = (usr_intf) ? XSTATE_TIPS : XSTATE_FINISH;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    for (i = 0; i < 16; ++i)
                        clog_print(CLOG_INFO, "cmd6-speed-mode w%d-%08x\n", i,
                                   sd_card->cmd6_query_status.word[i]);
                    /* set speed_mode */
                    mmc_set_speed_mode(mmc_dev, speed_mode);
                    mmc_clock_enable(sd_card->mmc_dev, speed_mode, emmc_vdd, mmcm_clock);
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    clog_print(CLOG_INFO, "XSTATE_SET_SPEED_MODE failed:0x%x\r\n", cur_mmc_cmd->cur_cmd.status);

                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_READ_ONE_BLOCK:
            clog_print(CLOG_INFO, "XSTATE_READ_ONE_BLOCK\r\n");
            if(0 == g_block_trx_flag) {
                uint8_t *ptr;

                g_block_trx_flag = 1;
                if (g_block_cnt < 1)
                    g_block_cnt = 1;
                if (g_block_cnt > 128)
                    g_block_cnt = 128;

                g_data_len = g_block_cnt * 512;
                ptr = mmc_alloc_buf(g_data_len, 0);
                if(NULL == ptr) {
                    clog_print(CLOG_INFO, "mmc_alloc_buf failed\r\n");
                    state = XSTATE_FINISH;
                    break;
                }
                os_memset(ptr, 0, g_data_len);
                cur_mmc_cmd->cur_cmd.databuf = ptr;
            }
            retval = mmc_read_block(mmc_dev, cur_mmc_cmd, g_block_addr, g_block_cnt,
                                    (uint8_t *)cur_mmc_cmd->cur_cmd.databuf, g_data_len);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_TIPS;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    print_data_buf(cur_mmc_cmd->cur_cmd.databuf, 512 * g_block_cnt);
                    mmc_cmd_reinit(cur_mmc_cmd);

                    g_block_trx_flag = 0;
                    mmc_free_buf(cur_mmc_cmd->cur_cmd.databuf);
                    cur_mmc_cmd->cur_cmd.databuf = NULL;
                    state = XSTATE_TIPS;
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_WRITE_ONE_BLOCK:
            if(0 == g_block_trx_flag) {
                uint8_t *ptr;

                g_block_trx_flag = 1;
                if (g_block_cnt < 1)
                    g_block_cnt = 1;
                if (g_block_cnt > 128)
                    g_block_cnt = 128;

                g_data_len = g_block_cnt * 512;
                ptr = mmc_alloc_buf(g_data_len, 0);
                if(NULL == ptr) {
                    clog_print(CLOG_INFO, "mmc_alloc_buf failed\r\n");
                    state = XSTATE_FINISH;
                    break;
                }
                cur_mmc_cmd->cur_cmd.databuf = ptr;
                buffer_fill(&cur_mmc_cmd->cur_cmd.databuf[0], g_data_len, g_pattern, 0);
            }

            retval = mmc_write_block(mmc_dev, cur_mmc_cmd, g_block_addr, g_block_cnt,
                                     (uint8_t *)cur_mmc_cmd->cur_cmd.databuf, g_data_len);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_TIPS;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_cmd_reinit(cur_mmc_cmd);

                    g_block_trx_flag = 0;
                    mmc_free_buf(cur_mmc_cmd->cur_cmd.databuf);
                    cur_mmc_cmd->cur_cmd.databuf = NULL;
                    state = XSTATE_TIPS;
                } else {
                    state = XSTATE_FINISH;
                }
            }
            break;
        case XSTATE_TX_TUNING:
            retval = mmc_sd_tx_tuning(mmc_dev, cur_mmc_cmd, speed_mode, emmc_vdd,
                                      mmcm_clock, result);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                clog_print(CLOG_INFO, "Tx tuning is complete\n");
                state = XSTATE_TIPS;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_TIPS;
                }
                /* find the windows */
                get_tuning_value(result, 128, &txphase);
            }
            break;
        case XSTATE_RX_TUNING:
            retval = mmc_sd_rx_tuning(mmc_dev, cur_mmc_cmd, speed_mode, emmc_vdd,
                                      mmcm_clock, txphase, is_emmc, bus_width,
                                      use_read_cmd, result);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_TIPS;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_TIPS;
                }
                /* find the windows */
                get_tuning_value(result, 128, &rx_phase);
                clog_print(CLOG_INFO, "Rx tuning is complete, optimum rxphase(%d)\n", rx_phase);
            }
            break;
        case XSTATE_TIPS:
            break;
        case XSTATE_RW_USER_INPUT:
            clog_print(CLOG_INFO, ".");
            rtos_delay_milliseconds(500);
            break;
        case XSTATE_FINISH:
            clog_print(CLOG_INFO, "================ finish =================\n");
            state = XSTATE_FINISH + 1;
            done_status = XSTATE_CMD_DONE;
            cur_mmc_cmd->state = XSTATE_CMD_DONE;
            clog_closefile();
            break;
        default:
            clog_print(CLOG_INFO, "unexceptional state\n");
            break;
    }

    clog_print(CLOG_LEVEL10, "%s: retval=%d\n", __func__, retval);

    if(XSTATE_RW_USER_INPUT != state) {
        sd_card->fn_state = state;
    }

    if(XSTATE_TIPS == state) {
        clog_print(CLOG_INFO, "enter option sdio_usr_intf rd/wr block_addr block_cnt, or sdio_usr_intf exit\n");
        sd_card->fn_state = XSTATE_RW_USER_INPUT;
    }

    return done_status;
}
// eof

