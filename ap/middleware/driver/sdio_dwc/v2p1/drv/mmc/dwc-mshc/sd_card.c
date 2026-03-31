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
#include "error.h"
#include "clog.h"
#include "sdhci.h"
#include "hal.h"
#include "mmc_dev.h"
#include "sd_cmd.h"
#include "mmc_core.h"
#include "mshc_regs.h"
#include "sd_cmds.h"
#include "sd_card.h"
#include "mmcm_clk.h"
#include "os/mem.h"

static struct sd_card_t *s_cards[CONFIG_MAX_MMC_CARDS] = {
    NULL,
};

int sd_card_register(uint32_t instance_id, struct sd_card_t *sdcard)
{
    if(instance_id >= CONFIG_MAX_MMC_CARDS){
        return ERROR_INVARG;
    }

    s_cards[instance_id] = sdcard;

    return RET_SUCCESS;
}

int sd_card_unregister(uint32_t instance_id)
{
    if(instance_id >= CONFIG_MAX_MMC_CARDS){
        return ERROR_INVARG;
    }

    s_cards[instance_id] = NULL;

    return RET_SUCCESS;
}

int sd_card_is_enumerate_ok(uint32_t instance_id)
{
    int ret = 0;
    struct sd_card_t *sdcard_ptr;

    if(instance_id >= CONFIG_MAX_MMC_CARDS){
        goto check_exit;
    }

    sdcard_ptr = s_cards[instance_id];
    if((NULL != sdcard_ptr) && (sdcard_ptr->is_enumerate_success)){
        ret = 1;
    }

check_exit:
    return ret;
}

struct sd_card_t *sd_card_get_object(uint32_t instance_id)
{
    if(instance_id >= CONFIG_MAX_MMC_CARDS){
        return NULL;
    }

    return s_cards[instance_id];
}

void *sd_card_get_private_data(uint32_t instance_id)
{
    void *data;
    struct mmc_dev_t *mmc_dev;
    struct sd_card_t *sd_card;

    sd_card = sd_card_get_object(instance_id);
    if(NULL == sd_card){
        goto get_exit;
    }

    mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;
    data = mmc_dev->sdhci;

get_exit:
    return data;
}

/** \brief: sd_card_alloc
 * 	allocate mmc cards from free pool
 * \param flags : memory allocation flags
 * \returns none
 */
struct sd_card_t *sd_card_alloc(uint8_t flags)
{
    struct sd_card_t *card;

    card = (struct sd_card_t *)os_zalloc(sizeof(card[0]));
    if(NULL == card){
        goto alloc_exit;
    }
    card->in_use = 1;

alloc_exit:
    return card;
}

/** \brief: sd_card_free
 * 	free the allocated mmc card to pool
 * \param card: pointer to sd_card object
 * \returns none
 */
void sd_card_free(struct sd_card_t *card)
{
    memset(card, 0xFF, sizeof(struct sd_card_t));
    card->in_use = 0;

    os_free(card);
}
/**
 * \brief sd_card_is_inserted
 *	check whether card is inserted or not
 * \param none
 * \returns true if card is inserted, 0 otherwise
 */
int sd_card_is_inserted(struct sd_card_t *sd_card)
{
    uint32_t pstate_reg;
    struct sdhci_t *sdhci;
    struct mmc_dev_t *mmc_dev;

    if((NULL == sd_card) || (NULL == sd_card->mmc_dev)){
        return 0;
    }

    mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;
    sdhci = mmc_dev->sdhci;

    /* PState Register always shows (bit16) card is inserted
     * irrespective of card is removed or inserted */
    pstate_reg = sdhci_get_pstate(sdhci);
    sdhci->card_present = ((pstate_reg & 0x10000) == 0x10000);
    printf("%s: pstate-reg=%x, card_present=%d\n", __func__, pstate_reg, sdhci->card_present);
    if (sdhci)
        return sdhci->card_present;
    else
        return 0;
}


/**
 * \brief app_main
 *	application main, 
 *		- initialize the ctest core modules,
 *		- iniitalize mmc and sdhci driver modules
 *		- initialize the msdc modules
 *		- initialize platform csim-link
 *		- run the application and schedular in loop
 * \param : 
 * \returns : 
 */
int sd_card_initialize(uint32_t instance_id, struct sd_card_t **sdcard_obj)
{
    struct sdhci_t *sdhci = NULL;
    struct mmc_dev_t *mmc_dev_ptr; 
    struct sd_card_t *sd_card_ptr;
    bk_err_t ret;

    /* ctest core modules init, first function to be called */
    ctest_core_init();

	/* initialize mmc modules */
	mmc_module_init();

	/* initialize sdhci driver init */
	ret = sdhci_module_init(instance_id, &sdhci);
	if (BK_OK != ret) {
		return ERROR_OPER_FAIL;
	}

	/* allocate mmc device instance */
	mmc_dev_ptr = mmc_dev_new(0);
	if (mmc_dev_ptr == NULL) {
		clog_print(CLOG_LEVEL5, "unable to allocated mmc device\n");
		return ERROR_NOMEM;
	}

	ret = rtos_init_semaphore(&mmc_dev_ptr->cmd_sync, 1);
	if (BK_OK != ret) {
		return ERROR_OPER_FAIL;
	}

	/* mmc card module initialization */
	sd_card_ptr = sd_card_alloc(0);
	if (sd_card_ptr == NULL) {
		clog_print(CLOG_LEVEL5, "unable to allocated mmc card\n");
		return ERROR_NOMEM;
	}
	sd_card_ptr->mmc_dev = mmc_dev_ptr;
	mmc_dev_ptr->card = sd_card_ptr;

	mmc_core_init(mmc_dev_ptr, &sdhci->host);
    mmc_dev_ptr->sdhci = sdhci;
    sdhci->private_data = mmc_dev_ptr;

    *sdcard_obj = sd_card_ptr;

    return RET_SUCCESS;
}

int sd_card_uninitialize(uint32_t instance_id)
{
    /*TODO, FIXME*/
    return RET_SUCCESS;
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
int mmc_dev_enumerate(struct sd_card_t *sd_card, uint8_t speed_mode, 
                        uint8_t bus_width, uint8_t xfer_mode,
                        uint8_t emmc_vdd, uint32_t mmcm_clock)
{
    int retval = 1, i;
    uint32_t status;
    struct mmc_dev_t *mmc_dev;
    struct cmd_param_t *cur_mmc_cmd;
    uint8_t done_status = ERROR_ENUMERATE;
    uint8_t state = XSTATE_CHK_MMC_CARD_DETECTED;

    mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;

    cur_mmc_cmd = mmc_dev->cur_mmc_cmd;
    sd_card->is_enumerate_success = 0;

enumerate_state_machine:
    switch (state) {
        case XSTATE_CHK_MMC_CARD_DETECTED:
            if (sd_card_is_inserted(sd_card)) {
                clog_print(CLOG_INFO, "==============CARD DETECTED (1) ============\n");
                if (mmc_dev->cur_mmc_cmd == NULL){
                    cur_mmc_cmd = mmc_cmds_alloc(0);
                }
                mmc_dev->cur_mmc_cmd = cur_mmc_cmd;
                cur_mmc_cmd->cur_cmd.timeout = 0xFFFFFFFF;

                if (cur_mmc_cmd) {
                    state = XSTATE_SET_CARD_IDLE;
                    cur_mmc_cmd->state = XSTATE_CMD_INIT;
                    clog_print(CLOG_INFO, "speed_mode = %d\nxfer_mode=%d\nbus_width=%d\nemmc_vdd=%d\nmmcm_clk=%d\n",
                               speed_mode, xfer_mode, bus_width, emmc_vdd, mmcm_clock);
                }
            } else {
                break;
            }

        case XSTATE_SET_CARD_IDLE:
            retval = mmc_cmd0_card_set_idle(mmc_dev, cur_mmc_cmd, 0);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                clog_print(CLOG_INFO, "XSTATE_SET_CARD_IDLE\r\n");
                state = XSTATE_CHK_SD_INTF_COND;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS){
                    mmc_cmd_reinit(cur_mmc_cmd);
                } else {
                    state = XSTATE_FINISH;
                    break;
                }
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
                    break;
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
                    break;
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
                    break;
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
                    break;
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
                    break;
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
                    break;
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
                    break;
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
                    break;
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
                    break;
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
                    break;
                }
            }
            break;

        case XSTATE_SET_SPEED_MODE:
            clog_print(CLOG_INFO, "XSTATE_SET_SPEED_MODE\r\n");
            retval = mmc_cmd6_switch_function(mmc_dev, cur_mmc_cmd, speed_mode,
                                              &sd_card->cmd6_query_status, 64);
            if (cur_mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_FINISH;
                if (cur_mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
                    for (i = 0; i < 16; ++i)
                        clog_print(CLOG_INFO, "cmd6-speed-mode w%d-%08x\n", i,
                                   sd_card->cmd6_query_status.word[i]);
                    /* set speed_mode */
                    mmc_set_speed_mode(mmc_dev, speed_mode);
                    mmc_clock_enable(sd_card->mmc_dev, speed_mode, emmc_vdd, mmcm_clock);
                    mmc_cmd_reinit(cur_mmc_cmd);
                    done_status = MSHC_SUCCESS;
                } else {
                    clog_print(CLOG_INFO, "XSTATE_SET_SPEED_MODE failed:0x%x\r\n", cur_mmc_cmd->cur_cmd.status);

                    state = XSTATE_FINISH;
                }
            }
            break;

        case XSTATE_FINISH:
            clog_print(CLOG_INFO, "is_enumerate_success finish\n");
            state = XSTATE_FINISH + 1;
            done_status = MSHC_SUCCESS;
            cur_mmc_cmd->state = XSTATE_CMD_DONE;
            sd_card->is_enumerate_success = 1;
            break;

        default:
            clog_print(CLOG_INFO, "unexceptional state\n");
            break;
    }

    /*if mmc dev enumeration is not over, please continue to execute it*/
    if(((XSTATE_FINISH + 1) != state) && (XSTATE_FINISH != state)){
        goto enumerate_state_machine;
    }

    clog_print(CLOG_LEVEL10, "%s: retval=%d\n", __func__, retval);

    return done_status;
}
// eof

