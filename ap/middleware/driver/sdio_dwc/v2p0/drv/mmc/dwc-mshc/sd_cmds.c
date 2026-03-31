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
* \file		: sd_cmds.c
* \author	: ravibabu@synopsys.com
* \date		: 01-Dec-2019
* \brief	: sd_cmds.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	01-Dec-2019	ravibabui@synopsys.com  001		dev in progress
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

#include "mmc_dev.h"
#include "sd_cmd.h"
#include "mmc_core.h"
#include "mshc_regs.h"
#include "sd_cmds.h"
#include "delay.h"

#define MAX_CMD_REQS	4
#define MAX_SD_CMDS	    128

/* globals */
uint8_t cee_inp_queue;
struct cmd_param_t cmd_reqs[MAX_CMD_REQS];
char sdcmd_str[MAX_SD_CMDS][40];
extern uint8_t rx_phase;

/** \brief: get_sdcmd_str
 *	get sd cmd description stringn
 * \param cmd: sd command number
 * \returns sd-cmd description string
 */
char *get_sdcmd_str(uint8_t cmd)
{
    if (cmd >= MAX_SD_CMDS)
        return "";
    if (!sdcmd_str[cmd][0])
        return "";
    return sdcmd_str[cmd];
}

/** \brief: cmd_reqs_init
 *	initialize the mmc-req pool
 * \param flags: control flags
 * \returns none
 */
void cmd_reqs_init(uint8_t flags)
{
    int i;
    for (i = 0; i < MAX_CMD_REQS; ++i)
        memset(&cmd_reqs[i], 0, sizeof(struct cmd_param_t));
}

/** \brief: mmc_cmds_init
 * 	initialize the mmc cmd request pool
 * \param flags : memory allocation flags (not used)
 * \returns none
 */
void mmc_cmds_init(uint8_t flags)
{
    int i;

    for (i = 0; i < 64; ++i)
        sdcmd_str[i][0] = 0;

    strcpy(sdcmd_str[0], "reset card to idle");
    strcpy(sdcmd_str[2], "send card cid");
    strcpy(sdcmd_str[3], "send card relative addr");
    strcpy(sdcmd_str[4], "set driver strength register");
    strcpy(sdcmd_str[6], "set card function mode");
    strcpy(sdcmd_str[7], "select card");
    strcpy(sdcmd_str[8], "check card intf condition");
    strcpy(sdcmd_str[9], "send card csd");
    strcpy(sdcmd_str[10], "send addressed card cid");
    strcpy(sdcmd_str[11], "switch bus voltage to 1.8v");
    strcpy(sdcmd_str[12], "force card to stop tx");
    strcpy(sdcmd_str[13], "send card status register");
    strcpy(sdcmd_str[16], "set block length");
    strcpy(sdcmd_str[17], "read single block");
    strcpy(sdcmd_str[18], "read multiple block");
    strcpy(sdcmd_str[19], "send tuning block");
    strcpy(sdcmd_str[23], "set block count");
    strcpy(sdcmd_str[24], "write single block");
    strcpy(sdcmd_str[25], "write multiple block");
    strcpy(sdcmd_str[27], "program card's csd bits");
    strcpy(sdcmd_str[41], "send sd operating condition");
    strcpy(sdcmd_str[51], "send card SCR register");
    strcpy(sdcmd_str[55], "next is ACMD");

    cmd_reqs_init(flags);
}

/** \brief: cmd_req_reinit
 *	re-initialize request to init state
 * \param cmd_req: pointer to cmd_param request
 * \returns none
 */
void cmd_req_reinit(struct cmd_param_t *cmd_req)
{
    memset(cmd_req, 0, sizeof(struct cmd_param_t));
    cmd_req->state = XSTATE_CMD_INIT;
}

/** \brief: malloc_cmd_req
 * 	allocate mmc mmc_cmds from free pool
 * \param flags : memory allocation flags (not used)
 * \returns none
 */
struct cmd_param_t *alloc_cmd_req(uint8_t flags)
{
    int i;
    for (i = 0; i < MAX_CMD_REQS; ++i) {
        if (cmd_reqs[i].in_use == 0)
            break;
    }

    if (i >= MAX_CMD_REQS)
        return NULL;

    memset(&cmd_reqs[i], 0, sizeof(struct cmd_param_t));
    cmd_reqs[i].in_use = 1;
    return &cmd_reqs[i];
}

/** \brief: mmc_cmd_reinit
 *	re-init mmc-cmd to init state
 * \param mmc_cmd: pointer to mmc_cmd-param
 * \returns none
 */
void mmc_cmd_reinit(struct cmd_param_t *mmc_cmd)
{
    cmd_req_reinit(mmc_cmd);
}

/** \brief: mmc_cmds_alloc
 * 	allocate mmc mmc_cmds from free pool
 * \param flags : memory allocation flags (not used)
 * \returns none
 */
struct cmd_param_t *mmc_cmds_alloc(uint8_t flags)
{
    return alloc_cmd_req(flags);
}

/** \brief: mmc_cmds_free
 * 	free the allocated mmc request from pool
 * \param pointer to mmc_request
 * \returns none
 */
void mmc_cmds_free(struct cmd_param_t *cmd)
{
    cmd->in_use = 0;
}

/** \brief: mmc_cmd_submit_wait_for_completion
 *	submit mmc-cmd and wait for io-completion
 * \param mmc_dev: pointer to mmc_dev obj
 * \param mmc_cmds: pointer to mmc-cmd-param
 * \returns success/failure on cmd completion
 */
int mmc_cmd_submit_wait_for_completion(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds)
{
    int len, cmd, i;
    addr_t data;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    switch (mmc_cmds->state) {
        case XSTATE_CMD_SUBMIT: /* submit request */
            clog_print(CLOG_LEVEL9, "%s. state(%d)\n", __func__, mmc_cmds->state);
            /* check input queue condition */
            if (!cee_inp_queue)
                cee_inp_queue = get_cmsg_q_num("cee-inpQ");
            if (!cee_inp_queue)
                return mmc_cmds->state;
            if (!cmsg_q_is_empty(cee_inp_queue))
                return mmc_cmds->state;

            len = sizeof(addr_t);
            data = (addr_t)&mmc_cmds->cur_cmd;
            clog_print(CLOG_LEVEL9, "%s. len=%d &cur_cmd(%p)\n", __func__, len, data);
            if (cmsg_q_send(cee_inp_queue, &data, len) != len) {
                clog_print(CLOG_ERR, "unable to sendcmd to cee-inpQ\n");
                return ERROR_OPER_FAIL;
            }
            clog_print(CLOG_LEVEL9, "send %d bytes cmd_req to cee-inpQ\n", len);
            mmc_cmds->state = XSTATE_WAIT_IO_COMPLETE;
            mmc_cmds->cur_cmd.status = IO_STATUS_IN_PROGRESS;
            break;

        case XSTATE_WAIT_IO_COMPLETE:	/* wait for completion */
            if (mmc_cmds->cur_cmd.status != IO_STATUS_IN_PROGRESS) {
                mmc_cmds->state = XSTATE_CMD_DONE;
                cmd = mmc_cmds->cur_cmd.cmd;
                if (mmc_cmds->cur_cmd.status == IO_STATUS_SUCCESS)
                    clog_print(CLOG_INFO, "cmd%d completed, %s success\n", cmd,
                               get_sdcmd_str(cmd));
                else
                    clog_print(CLOG_ERR, "cmd%d completed %s failed status = %d\n",
                               cmd, get_sdcmd_str(cmd), mmc_cmds->cur_cmd.status);
                for (i = 0; i < 4; ++i)
                    clog_print(CLOG_INFO, "resp[%d]=%x\n", i,
                               mmc_cmds->cur_cmd.resp[i]);
            }
            break;
        case XSTATE_CMD_DONE:
            break;
    }
    return mmc_cmds->state;
}

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
int mmc_cmd0_card_set_idle(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint8_t boot_mode)
{
    struct cmd0_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd0_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->boot_mode = boot_mode;

        mmc_cmds->cur_cmd.cmd = MMC_CMD0_GO_IDLE;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd0_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
int mmc_cmd1_emmc_send_op_cond(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint32_t args)
{
    struct cmd1_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd1_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->args = args;

        mmc_cmds->cur_cmd.cmd = EMMC_CMD1_SEND_OP_COND;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd1_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
int mmc_cmd3_send_rel_adr(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint8_t rca)
{
    struct cmd3_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd3_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->rca = rca;

        mmc_cmds->cur_cmd.cmd = MMC_CMD3_SEND_REL_ADR;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd3_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

/** \brief: mmc_cmd11_voltage_switch
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param args: cmd specific argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd11_voltage_switch(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint32_t args)
{
    struct cmd11_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd11_param_t *)&mmc_cmds->cur_cmd.param[0];

        param->args = args;
        mmc_cmds->cur_cmd.cmd = MMC_CMD11_VOLT_SWITCH;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd11_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

/** \brief: mmc_cmd2_get_cid
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param args: cmd specific argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd2_get_card_cid(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint32_t args)
{
    struct cmd2_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd2_param_t *)&mmc_cmds->cur_cmd.param[0];

        param->args = args;
        mmc_cmds->cur_cmd.cmd = MMC_CMD2_ALL_SEND_CID;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd2_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
int mmc_cmd7_card_select(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint16_t rca)
{
    struct cmd7_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd7_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->rca = rca;

        mmc_cmds->cur_cmd.cmd = MMC_CMD7_CARD_SELECT;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd7_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

/** \brief: mmc_cmd9_get_csd
 *	prepare for cmd9 for send card's csd data
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param args: cmd specific argument
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd9_get_csd(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds, uint16_t rca)
{
    struct cmd9_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd9_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->rca = rca;

        mmc_cmds->cur_cmd.cmd = MMC_CMD9_SEND_CSD;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd9_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
                          void *card_status_buf, uint32_t buflen)
{
    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */

        mmc_cmds->cur_cmd.databuf = card_status_buf;
        mmc_cmds->cur_cmd.datalen = buflen;

        mmc_cmds->cur_cmd.cmd = MMC_CMD13_SEND_STATUS;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd13_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
                             uint8_t speed_mode, void *databuf, uint32_t buflen)
{
    struct cmd6_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd6_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->is_acmd = 0;
        param->speed_mode = speed_mode;

        mmc_cmds->cur_cmd.databuf = databuf;
        mmc_cmds->cur_cmd.datalen = buflen;

        mmc_cmds->cur_cmd.cmd = MMC_CMD6_SWITCH_FN;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd6_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

/** \brief: mmc_acmd6_switch_function
 *		prepare for cmd6, switch function
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param bus_width: mmc/sd bus-width
 * \returns succes/fail on cmd command completion
 */
int mmc_acmd6_switch_function(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                              uint8_t bus_width)
{
    struct acmd6_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct acmd6_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->is_acmd = 1;
        param->bus_width = bus_width;

        mmc_cmds->cur_cmd.cmd = MMC_CMD6_SWITCH_FN;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd6_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
                          uint8_t access_mode, uint8_t index, uint8_t value, uint8_t cmd_set)
{
    struct cmd6_emmc_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd6_emmc_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->access_mode = access_mode;
        param->index = index;
        param->value = value;
        param->cmd_set = cmd_set;

        mmc_cmds->cur_cmd.cmd = EMMC_CMD6_SET_EXT_CSD;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd6_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

/** \brief: mmc_read_one_block
 *	cmd17 - read single block
 *	cmd18 - read multiple block
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param block_addr: device block address
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_generic_read_block(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                           uint32_t block_addr, uint32_t block_cnt, uint8_t  *databuf, uint32_t buflen)
{
    struct cmd17_param_t *cmd17_param;
    struct cmd18_param_t *cmd18_param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */

        if (buflen < SD_BLOCK_SZ)
            return ERROR_INVARG;

        if (block_cnt > 1) {
            cmd18_param = (struct cmd18_param_t *)&mmc_cmds->cur_cmd.param[0];
            cmd18_param->block_addr = block_addr;
            cmd18_param->nblocks = block_cnt;
            mmc_cmds->cur_cmd.cmd = MMC_CMD18_READ_MULTI_BLOCK;
        } else {
            cmd17_param = (struct cmd17_param_t *)&mmc_cmds->cur_cmd.param[0];
            cmd17_param->block_addr = block_addr;
            mmc_cmds->cur_cmd.cmd = MMC_CMD17_READ_ONE_BLOCK;
        }

        mmc_cmds->cur_cmd.databuf = databuf;
        mmc_cmds->cur_cmd.datalen = buflen;

        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd17_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

/** \brief: mmc_set_block_cnt
 *		cmd23, set bus block_cnt
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param block_cnt: number of blocks
 * \returns succes/fail on cmd command completion
 */
int mmc_set_block_cnt(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                      uint32_t block_cnt)
{
    struct cmd23_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        param = (struct cmd23_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->block_cnt = block_cnt;

        mmc_cmds->cur_cmd.cmd = MMC_CMD23_SET_BLOCKCNT;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd6_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

/** \brief: mmc_write_one_block
 *	cmd24 - write single block
 *	cmd25 - write multiple block
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param block_addr: device block address
 * \param block_cnt: number of blocks
 * \param databuf: data buffer
 * \param buflen : buffer length
 * \returns succes/fail on cmd command completion
 */
int mmc_generic_write_block(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                            uint32_t block_addr, uint32_t block_cnt, uint8_t  *databuf, uint32_t buflen)
{
    struct cmd24_param_t *cmd24_param;
    struct cmd25_param_t *cmd25_param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        if (buflen < SD_BLOCK_SZ)
            return ERROR_INVARG;

        if (block_cnt > 1) {
            cmd25_param = (struct cmd25_param_t *)&mmc_cmds->cur_cmd.param[0];
            cmd25_param->block_addr = block_addr;
            cmd25_param->nblocks = block_cnt;
            mmc_cmds->cur_cmd.cmd = MMC_CMD25_WRITE_MULTI_BLOCK;
        } else {
            cmd24_param = (struct cmd24_param_t *)&mmc_cmds->cur_cmd.param[0];
            cmd24_param->block_addr = block_addr;
            mmc_cmds->cur_cmd.cmd = MMC_CMD24_WRITE_ONE_BLOCK;
        }

        mmc_cmds->cur_cmd.databuf = databuf;
        mmc_cmds->cur_cmd.datalen = buflen;

        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd24_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
                                uint8_t  *databuf, uint32_t buflen)
{
    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        if (buflen < SD_TUNING_BLOCK_SZ)
            return ERROR_INVARG;

        mmc_cmds->cur_cmd.cmd = MMC_CMD19_SEND_TUNING_BLOCK;

        mmc_cmds->cur_cmd.databuf = databuf;
        mmc_cmds->cur_cmd.datalen = buflen;

        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
                                uint8_t bus_width, uint8_t  *databuf, uint32_t buflen)
{
    struct cmd21_param_t *cmd21_param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */

        if (buflen < EMMC_TUNING_BLOCK_SZ)
            return ERROR_INVARG;

        cmd21_param = (struct cmd21_param_t *)&mmc_cmds->cur_cmd.param[0];
        cmd21_param->bus_width = bus_width;
        printf("%s. bus-width = %d\n", __func__, cmd21_param->bus_width);

        mmc_cmds->cur_cmd.cmd = MMC_CMD21_SEND_TUNING_BLOCK;

        mmc_cmds->cur_cmd.databuf = databuf;
        mmc_cmds->cur_cmd.datalen = buflen;

        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd21_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
                                       uint8_t volt_supplied, uint8_t check_pattern)
{
    struct cmd8_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        mmc_cmds->cur_cmd.cmd = MMC_CMD8_SEND_IF_COND;
        param = (struct cmd8_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->volt_supplied = volt_supplied;
        param->check_pattern = check_pattern;

        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd8_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
                          uint8_t  *databuf, uint32_t buflen, uint32_t args)
{
    struct cmd8_emmc_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        mmc_cmds->cur_cmd.cmd = EMMC_CMD8_SEND_EXT_CSD;
        param = (struct cmd8_emmc_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->args = args;

        mmc_cmds->cur_cmd.databuf = databuf;
        mmc_cmds->cur_cmd.datalen = buflen;
        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd8_emmc_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

/** \brief: mmc_cmd55_next is application cmd
 * 	The command send to inform card that next command is acmd
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param rca: relative card address
 * \returns succes/fail on cmd command completion
 */
int mmc_cmd55_next_is_acmd(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                           uint16_t rca)
{
    struct cmd55_param_t *param;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    if (mmc_cmds->state == XSTATE_CMD_INIT) {	/* initialize param */
        mmc_cmds->cur_cmd.cmd = MMC_CMD55_NXT_IS_ACMD;
        param = (struct cmd55_param_t *)&mmc_cmds->cur_cmd.param[0];
        param->rca = rca;

        mmc_cmds->cur_cmd.is_valid = 1;
        mmc_cmds->cur_cmd.param_len = sizeof(struct cmd8_param_t);
        mmc_cmds->state = XSTATE_CMD_SUBMIT;
    }

    return mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
}

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
                                       uint8_t is_sdxc_support, uint8_t use_1p8v, uint8_t is_max_perf, uint8_t is_ocr)
{
    struct cmd41_param_t *param;
    int retval = 0;
    static uint8_t state = XSTATE0;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    switch(state) {
        case XSTATE0:	/* initialize param */
            retval = mmc_cmd55_next_is_acmd(mmc_dev, mmc_cmds, mmc_dev->card->rca);
            if (mmc_cmds->state == XSTATE_CMD_DONE) {
                if (mmc_cmds->cur_cmd.status == IO_STATUS_SUCCESS) {
                    /* goto next command */
                    mmc_cmd_reinit(mmc_cmds);
                    state = XSTATE1;
                } else {
                    state = XSTATE0;
                }
            }
            break;
        case XSTATE1:
            mmc_cmds->cur_cmd.cmd = MMC_ACMD41_SD_SEND_OP_COND;
            param = (struct cmd41_param_t *)&mmc_cmds->cur_cmd.param[0];
            param->is_hcs_sdxc_support = is_sdxc_support;
            param->s18r_use_18v = use_1p8v;
            param->is_max_perf = is_max_perf;
            param->is_ocr =	is_ocr;

            mmc_cmds->cur_cmd.is_valid = 1;
            mmc_cmds->cur_cmd.param_len = sizeof(struct cmd8_param_t);
            mmc_cmds->state = XSTATE_CMD_SUBMIT;

            state = XSTATE2;
        //break; avoid break purposefully, fix for coverity

        case XSTATE2:
            retval = mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
            if (mmc_cmds->state == XSTATE_CMD_DONE)
                state = XSTATE0;
            //break; avoid break purposefully, fix for coverity
    }
    return retval;
}

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
                            void *databuf, uint32_t buflen)
{
    struct cmd51_param_t *param;
    int retval = 0;
    struct sd_card_t *card;
    static uint8_t state = XSTATE0;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    card = mmc_dev->card;
    switch(state) {
        case XSTATE0:	/* initialize param */
            retval = mmc_cmd55_next_is_acmd(mmc_dev, mmc_cmds, card->rca);
            if (mmc_cmds->state == XSTATE_CMD_DONE) {
                if (mmc_cmds->cur_cmd.status == IO_STATUS_SUCCESS) {
                    /* goto next command */
                    mmc_cmd_reinit(mmc_cmds);
                    state = XSTATE1;
                } else {
                    state = XSTATE0;
                }
            }
            break;
        case XSTATE1:
            mmc_cmds->cur_cmd.cmd = MMC_CMD51_SEND_SCR;
            param = (struct cmd51_param_t *)&mmc_cmds->cur_cmd.param[0];
            param->is_acmd = 1;

            mmc_cmds->cur_cmd.databuf = databuf;
            mmc_cmds->cur_cmd.datalen = buflen;

            mmc_cmds->cur_cmd.is_valid = 1;
            mmc_cmds->cur_cmd.param_len = sizeof(struct cmd8_param_t);
            mmc_cmds->state = XSTATE_CMD_SUBMIT;

            state = XSTATE2;
        //break; avoid break purposefully, fix for coverity

        case XSTATE2:
            retval = mmc_cmd_submit_wait_for_completion(mmc_dev, mmc_cmds);
            if (mmc_cmds->state == XSTATE_CMD_DONE)
                state = XSTATE0;
            //break; avoid break purposefully, fix for coverity
    }
    return retval;
}

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
                          void *databuf, uint32_t buflen)
{
    int retval = 0;
    struct sd_card_t *card;
    static uint8_t state = XSTATE0;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    card = mmc_dev->card;
    switch(state) {
        case XSTATE0:	/* initialize param */
            retval = mmc_cmd55_next_is_acmd(mmc_dev, mmc_cmds, card->rca);
            if (mmc_cmds->state == XSTATE_CMD_DONE) {
                if (mmc_cmds->cur_cmd.status == IO_STATUS_SUCCESS) {
                    /* goto next command */
                    mmc_cmd_reinit(mmc_cmds);
                    state = XSTATE1;
                } else {
                    state = XSTATE0;
                }
            }
            break;
        case XSTATE1:
            retval = mmc_cmd13_send_status(mmc_dev, mmc_cmds, databuf, buflen);
            break;
    }
    return retval;
}

/** \brief: mmc_acmd6_set_buswidth
 * 	The application specific command ACMD6 to set bus-width
 *
 * \param mmc_dev: pointer to mmc_dev object
 * \param mmc_cmds: pointer to mmc-cmds
 * \param bus_width: 4 or 8 bit bus-width
 * \returns succes/fail on cmd command completion
 */
int mmc_acmd6_set_buswidth(struct mmc_dev_t *mmc_dev, struct cmd_param_t *mmc_cmds,
                           uint8_t bus_width)
{
    int retval = 0;
    struct sd_card_t *card;
    static uint8_t state = XSTATE0;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    card = mmc_dev->card;

    switch(state) {
        case XSTATE0:	/* initialize param */
            retval = mmc_cmd55_next_is_acmd(mmc_dev, mmc_cmds, card->rca);
            if (mmc_cmds->state == XSTATE_CMD_DONE) {
                if (mmc_cmds->cur_cmd.status == IO_STATUS_SUCCESS) {
                    /* goto next command */
                    mmc_cmd_reinit(mmc_cmds);
                    state = XSTATE1;
                } else {
                    state = XSTATE0;
                }
            }
            break;
        case XSTATE1:
            retval = mmc_acmd6_switch_function(mmc_dev, mmc_cmds, bus_width);
            break;
    }
    return retval;
}

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
                   uint32_t block_addr, uint32_t block_cnt, uint8_t  *databuf, uint32_t buflen)
{
    int retval = 0;
    static uint8_t state = XSTATE0;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    switch(state) {
        case XSTATE0:	/* initialize param */
            if (block_cnt > 1) {
                retval = mmc_set_block_cnt(mmc_dev, mmc_cmds, block_cnt);
                if (mmc_cmds->state == XSTATE_CMD_DONE) {
                    state = XSTATE0;
                    if (mmc_cmds->cur_cmd.status == IO_STATUS_SUCCESS) {
                        /* goto next command */
                        mmc_cmd_reinit(mmc_cmds);
                        state = XSTATE1;
                    }
                } else
                    break;
            } else
                state = XSTATE1;
        //break; avoid break purposefully, fix for coverity

        case XSTATE1:
            retval = mmc_generic_read_block(mmc_dev, mmc_cmds, block_addr, block_cnt,
                                            databuf, buflen);
            if (mmc_cmds->state == XSTATE_CMD_DONE)
                state = XSTATE0;
            break;
    }

    return retval;
}

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
                    uint32_t block_addr, uint32_t block_cnt, uint8_t  *databuf, uint32_t buflen)
{
    int retval = 0;
    static uint8_t state = XSTATE0;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    switch(state) {
        case XSTATE0:	/* initialize param */
            if (block_cnt > 1) {
                retval = mmc_set_block_cnt(mmc_dev, mmc_cmds, block_cnt);
                if (mmc_cmds->state == XSTATE_CMD_DONE) {
                    state = XSTATE0;
                    if (mmc_cmds->cur_cmd.status == IO_STATUS_SUCCESS) {
                        /* goto next command */
                        //os_delay_ms(500);
                        mmc_cmd_reinit(mmc_cmds);
                        state = XSTATE1;
                    }
                } else
                    break;
            } else
                state = XSTATE1;
        //break; avoid break purposefully, fix for coverity

        case XSTATE1:
            retval = mmc_generic_write_block(mmc_dev, mmc_cmds, block_addr, block_cnt,
                                             databuf, buflen);
            if (mmc_cmds->state == XSTATE_CMD_DONE)
                state = XSTATE0;
            break;
    }

    return retval;
}

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
                     uint8_t speed_mode, uint8_t emmc_vdd, uint32_t mmcm_clock, uint8_t *result)
{
#define STATE_TUNE_INIT0	XSTATE0
#define STATE_TUNE_INIT1	XSTATE1
#define STATE_TUNE_WRTBLK	XSTATE2
#define STATE_TUNE_COMPLETE	XSTATE3

    int i, retval = XSTATE_WAIT_IO_COMPLETE;
    static int state = STATE_TUNE_INIT0, tx_phase = 0;
    uint8_t *pbuf;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    switch (state) {

        case STATE_TUNE_INIT0:
            /* enable software tune */
            mmc_host_io_ctrl(mmc_dev, SDHCI_SW_TUNE_CTRL, ENABLE);
            /* enable command conflict */
            mmc_host_io_ctrl(mmc_dev, SDHCI_CMDCFLT_CHK, ENABLE);

            mmc_cmd_reinit(mmc_cmds);
            mmc_cmds->cur_cmd.databuf = mmc_alloc_buf(512, 0);
            pbuf = (uint8_t *)&mmc_cmds->cur_cmd.databuf[0];
            for (i = 0; i < 512; i+=4)
                *(uint32_t *)&pbuf[i] = 0x11000000 | (tx_phase << 16) | i;

            state = STATE_TUNE_INIT1;
            tx_phase = 0;

        //break; avoid break purposefully, fix for coverity

        case STATE_TUNE_INIT1:
            /* set the txphase value */
            mmc_host_io_ctrl(mmc_dev, SDHCI_SET_TXPHASE, tx_phase);

            /* enable the sw tune and mmcm_clock */
            mmc_clock_enable(mmc_dev, speed_mode, emmc_vdd, mmcm_clock);
            os_delay_ms(100);
            state = STATE_TUNE_WRTBLK;
        //break; avoid break purposefully, fix for coverity

        case STATE_TUNE_WRTBLK:
            retval = mmc_write_block(mmc_dev, mmc_cmds, 10, 1,
                                     mmc_cmds->cur_cmd.databuf, 512);
            if (mmc_cmds->state == XSTATE_CMD_DONE) {
                //state = STATE_TUNE_WRTBLK;
                state = STATE_TUNE_INIT1;
                retval = XSTATE_WAIT_IO_COMPLETE;
                if (mmc_cmds->cur_cmd.status == IO_STATUS_SUCCESS) {
                    result[tx_phase] = 1;
                } else
                    result[tx_phase] = 2;

                mmc_cmds->state = XSTATE_CMD_INIT;

                clog_print(CLOG_INFO, "============ tx-phase(%d)- %s (%d, %d)==========\n",
                           tx_phase, (result[tx_phase] == 1) ? "Pass" : "Fail",
                           mmc_cmds->state, mmc_cmds->cur_cmd.status);
                if (tx_phase < 128)
                    tx_phase++;
                else {
                    tx_phase = 0;
                    mmc_cmds->state = XSTATE_CMD_DONE;
                    state = STATE_TUNE_INIT0;
                    retval = XSTATE_CMD_DONE;
                    break;
                }
                pbuf = (uint8_t *)&mmc_cmds->cur_cmd.databuf[0];
                for (i = 0; i < 512; i+=4)
                    *(uint32_t *)&pbuf[i] = 0x11000000 | (tx_phase << 16) | i;
            }
            break;
        case STATE_TUNE_COMPLETE:
            tx_phase = 0;
            state = STATE_TUNE_INIT0;
            retval = XSTATE_CMD_DONE;
            break;
    }

    return retval;
}

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
                     uint8_t is_emmc, uint8_t bus_width, uint8_t use_read_cmd, uint8_t *result)
{
#define STATE_RXTUNE_INIT0	XSTATE0
#define STATE_RXTUNE_INIT1	XSTATE1
#define STATE_RXTUNE_RDBLK	XSTATE2
#define STATE_RXTUNE_COMPLETE	XSTATE3

    int retval = XSTATE_WAIT_IO_COMPLETE;
    static int state = STATE_TUNE_INIT0;
    uint8_t *pbuf;
    static uint8_t buflen;

    if (mmc_cmds == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    switch (state) {

        case STATE_RXTUNE_INIT0:
            /* enable software tune */
            mmc_host_io_ctrl(mmc_dev, SDHCI_SW_TUNE_CTRL, ENABLE);
            /* enable command conflict */
            mmc_host_io_ctrl(mmc_dev, SDHCI_CMDCFLT_CHK, ENABLE);

            mmc_cmd_reinit(mmc_cmds);
            mmc_cmds->cur_cmd.databuf = mmc_alloc_buf(512, 0);
            pbuf = (uint8_t *)&mmc_cmds->cur_cmd.databuf[0];
            memset(pbuf, 0, 512);

            /* set the txphase value */
            mmc_host_io_ctrl(mmc_dev, SDHCI_SET_TXPHASE, tx_phase);

            state = STATE_RXTUNE_INIT1;
            rx_phase = 0;
        //break; avoid break purposefully, fix for coverity

        case STATE_RXTUNE_INIT1:
            /* set the rxphase value */
            mmc_host_io_ctrl(mmc_dev, SDHCI_SET_RXPHASE, rx_phase);

            /* enable the sw tune and mmcm_clock */
            mmc_clock_enable(mmc_dev, speed_mode, emmc_vdd, mmcm_clock);
            os_delay_ms(100);
            state = STATE_RXTUNE_RDBLK;
        //break; avoid break purposefully, fix for coverity

        case STATE_RXTUNE_RDBLK:

            if (use_read_cmd) {
                retval = mmc_read_block(mmc_dev, mmc_cmds, 10, 1,
                                        (uint8_t *)mmc_cmds->cur_cmd.databuf, 512);
            } else {
                if (is_emmc) {
                    buflen = 128;
                    retval = mmc_cmd21_read_tuning_block(mmc_dev, mmc_cmds, bus_width,
                                                         (uint8_t *)mmc_cmds->cur_cmd.databuf, buflen);
                } else {
                    buflen = 64;
                    retval = mmc_cmd19_read_tuning_block(mmc_dev, mmc_cmds,
                                                         (uint8_t *)mmc_cmds->cur_cmd.databuf, buflen);
                }
            }

            if (mmc_cmds->state == XSTATE_CMD_DONE) {
                //state = STATE_TUNE_WRTBLK;
                state = STATE_RXTUNE_INIT1;
                retval = XSTATE_WAIT_IO_COMPLETE;
                if (mmc_cmds->cur_cmd.status == IO_STATUS_SUCCESS) {
                    result[rx_phase] = 1;
                } else
                    result[rx_phase] = 2;

                mmc_cmds->state = XSTATE_CMD_INIT;

                clog_print(CLOG_INFO, "=======tx_phase(%d) rx-phase(%d)- %s (%d, %d)==========\n",
                           tx_phase, rx_phase, (result[rx_phase] == 1) ? "Pass" : "Fail",
                           mmc_cmds->state, mmc_cmds->cur_cmd.status);
                if (rx_phase < 127)
                    rx_phase++;
                else {
                    mmc_cmds->state = XSTATE_CMD_DONE;
                    state = STATE_RXTUNE_INIT0;
                    retval = XSTATE_CMD_DONE;
                    break;
                }
                print_data_buf(mmc_cmds->cur_cmd.databuf, buflen);
            }
            break;
        case STATE_RXTUNE_COMPLETE:
            rx_phase = 0;
            state = STATE_RXTUNE_INIT0;
            retval = XSTATE_CMD_DONE;
            break;
    }

    return retval;
}
