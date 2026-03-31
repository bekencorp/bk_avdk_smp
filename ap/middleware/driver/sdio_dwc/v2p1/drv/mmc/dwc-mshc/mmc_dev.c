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
* \file		: mmc_dev.c
* \author	: ravibabu@synopsys.com
* \date		: 24-June-2019
* \brief	: mmc_dev.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	24-June-2019	ravibabui@synopsys.com  001		dev in progress
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
#include "sd_cmds.h"

#include "mmc_dev.h"
#include "sd_cmd.h"
#include "mmc_core.h"
#include "mshc_regs.h"
#include "os/mem.h"

#define ALIGN_BYTE_MASK	0xF

/* globals */
struct mmc_dev_t mmc_dev_list[MAX_MMC_DEVICES]; 	/**< list of mmc-devs */
struct mmc_req_t mmc_req_pool[MAX_MMC_IO_REQUESTS]; 	/**< mmc request pool */

/* mmc cmd request prepare function table */
int (*mmc_cmd_prepare[256])  (struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                              struct cmd_param_s *param, uint32_t param_len);

extern void mmc_cmds_init(uint8_t flags);

/** \brief: mmc_alloc_buf
 * 	initialize the mmc request pool
 * 	always return static memory from g_sd_buf[0]
 * \param flags : memory allocation flags - 4/8/16 byte alignment
 * \returns aligned memory block
 */
void *mmc_alloc_buf(uint32_t size, uint8_t flags)
{
    return (void *)(os_malloc((size_t)size));
}

void mmc_free_buf(void *buf)
{
    os_free(buf);
}

/** \brief: mmc_request_pool_init
 * 	initialize the mmc request pool
 * \param flags : memory allocation flags (not used)
 * \returns none
 */
void mmc_request_pool_init(uint8_t flags)
{
    int i;
    for (i = 0; i < MAX_MMC_IO_REQUESTS; ++i)
        mmc_req_pool[i].in_use = 0;
}

/** \brief: mmc_request_alloc
 * 	allocate mmc request from free pool
 * \param flags : memory allocation flags (not used)
 * \returns pointer to mmc_request
 */
struct mmc_req_t *mmc_request_alloc(uint8_t flags)
{
    int i;
    for (i = 0; i < MAX_MMC_IO_REQUESTS; ++i) {
        if (mmc_req_pool[i].in_use == 0)
            break;
    }

    if (i >= MAX_MMC_IO_REQUESTS)
        return 0;

    memset(&mmc_req_pool[i], 0, sizeof(struct mmc_req_t));
    mmc_req_pool[i].in_use = 1;
    mmc_req_pool[i].state = XSTATE0;
    return &mmc_req_pool[i];
}

/** \brief: mmc_request_free
 * 	free the allocated mmc request from pool
 * \param pointer to mmc_request
 * \returns none
 */
void mmc_request_free(struct mmc_req_t *req)
{
    req->in_use = 0;
}

/** \brief: mmc_devs_list_init
 * 	initialize the mmc device list
 * \param none
 * \returns none
 */
void mmc_devs_list_init(void)
{
    int i;
    for (i = 0; i < MAX_MMC_DEVICES; ++i)
        mmc_dev_list[i].in_use = 0;
}

/** \brief: mmc_dev_alloc
 * 	allocate mmc device object from free pool
 * \param flags : memory allocation flags (not used)
 * \returns pointer to mmc_dev_t object
 */
struct mmc_dev_t *mmc_dev_alloc(uint8_t flags)
{
    int i;
    for (i = 0; i < MAX_MMC_DEVICES; ++i) {
        if (mmc_dev_list[i].in_use == 0)
            break;
    }

    if (i >= MAX_MMC_DEVICES)
        return 0;

    mmc_dev_list[i].in_use = 1;
    mmc_dev_list[i].id = i;
    return &mmc_dev_list[i];
}

/** \brief: mmc_dev_free
 * 	free the allocated mmc_dev_t object from pool
 * \param pointer to mmc_dev object
 * \returns none
 */
void mmc_dev_free(struct mmc_dev_t *mmc_dev)
{
    mmc_dev->in_use = 0;
}


/** \brief: mmc_init
	mmc module initialization
 * \param pointer to mmc_dev object
 * \returns none
 */
void mmc_module_init(void)
{
    mmc_devs_list_init();
    mmc_request_pool_init(0);
}

/** \brief:  mmc_cmd_fill
 * 	fill mmc cmd io_request
 * \param req: pointer to mmc_request
 * \param cmd: mmcsd command
 * \param args: cmd argument
 * \param flag: cmd flags
 * \returns none
 */
void mmc_cmd_fill(struct mmc_req_t *req, uint8_t cmd, uint32_t args, uint32_t flags)
{
    req->cmdreg = ((cmd & CMDIDX_MASK) << 8) | (flags & 3);
    req->args = args;
    req->flags = flags;
    clog_print(CLOG_LEVEL3, "--------------------------------------------------------\n");
}

/** \brief: prepare_mmc_cmd_not_supported
	mmc cmd not supported
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns error CMD NOT SUPPORTED
 */
int prepare_mmc_cmd_not_supported(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                  struct cmd_param_s *cmd_param, uint32_t param_len)
{
    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);
    return ERROR_IO_NOT_SUPPORTED;
}

/** \brief: mmc_cmd_go_idle
 *	The command GO_IDLE_STATE (CMD0) is the software reset command and sets each
 * card into Idle State regardless of the current card state. Cards in Inactive State
 * are not affected by this command. After power-on by the host, all cards are in Idle
 * State, including the cards that have been in Inactive State before.
 *	After power-on or CMD0, all cards’ CMD lines are in input mode, waiting for
 * start bit of the next command. The cards are initialized with a default relative card
 * address (RCA=0x0000) and with a default driver stage register setting (lowest speed,
 * highest driving current capability).
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd0_go_idle(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                             struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd0_param_t *param;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);
    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd0_param_t *)&cmd_param->param[0];
    req->mmc_dev = mmc_dev;
    req->flags = CMD_NO_RESPONSE | CMD_WITHOUT_DATA;

    /* extract cmd parameters */
    mmc_cmd_fill(req, MMC_CMD0_GO_IDLE, param->boot_mode, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd1_emmc_send_op_cond
 *	prepare for cmd1 emmc send_operating condition
 *	CMD1 is special synchronization command used to negotiate the operation
 * volatage range and to poll the device until it is out of its power up sequence.
 * In addition to operation voltage profile of the emmc device, the response to CMD1
 * contains busy flag indicating that the device is still working on it spower up
 * procedure and is not ready for identification. This bit informs the host that the
 * device is not ready, and the host must wait until this bit is cleared.
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd1_emmc_send_op_cond(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                       struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd1_param_t *param;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);
    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd1_param_t *)&cmd_param->param[0];
    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | MMC_RESP2_LEN_48;

    /* extract cmd parameters */
    mmc_cmd_fill(req, EMMC_CMD1_SEND_OP_COND, param->args, req->flags);
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd3_send_rel_adr
 *	prepare for cmd3 send relative address
 *	ask the card to publish the new relativ address
 *	device respond with response type r6 has following format
 *	as per section 4.9.5 (phy spec), code length : 48 bit
 *		6-bit[45:40] - cmd index
 *		16-bit[31:16] - new published rca
 *		16-bit[15:0] - card status bits 23,22,19,12:0
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd3_send_rel_adr(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                  struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd3_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);
    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd3_param_t *)&cmd_param->param[0];
    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48;

    /* extract cmd parameters */
    args = (param->rca << 16);
    mmc_cmd_fill(req, MMC_CMD3_SEND_REL_ADR, args, req->flags);
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd11_voltage_switch
 *	prepare for cmd11 voltage switch cmd
 *	ask the card to switch voltage from 3.6 to 1.8V.
 *	device respond with response type r2
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd11_voltage_switch(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                     struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);
    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48;

    /* extract cmd parameters */
    args = 0;
    mmc_cmd_fill(req, MMC_CMD11_VOLT_SWITCH, args, req->flags);
    req->cmdreg |= (CMD_IDX_CHK_ENABLE | CMD_CRC_CHK_ENABLE);
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd8_send_if_cond
 *		prepare for cmd8 send interface condition
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd8_send_if_cond(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                  struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd8_param_t *param;
    uint8_t volt_supplied, check_pattern;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);
    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd8_param_t *)&cmd_param->param[0];

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48;

    /* extract cmd parameters */
    volt_supplied = param->volt_supplied;
    check_pattern = param->check_pattern;
    args = (volt_supplied << 8) | check_pattern;

    mmc_cmd_fill(req, MMC_CMD8_SEND_IF_COND, args, req->flags);
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_emmc_cmd8_send_ext_csd
 *		prepare for cmd8-send_ext_csd for emmc device
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_emmc_cmd8_send_ext_csd(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                   struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd8_emmc_param_t *param;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd8_emmc_param_t *)&cmd_param->param[0];

    if (cmd_param->databuf == NULL)
        return ERROR_INVARG;
    if (cmd_param->datalen < SD_BLOCK_SZ)
        return ERROR_INVARG;

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITH_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48 |
                 XFR_MODE_BLKCNT_EN_F | XFR_MODE_DATA_READ_F;

    req->int_mask = 0xffffffff;
    req->flags |= SDHCI_INTR_ENABLE;

    /* extract cmd parameters */
    mmc_cmd_fill(req, EMMC_CMD8_SEND_EXT_CSD, param->args, req->flags);
    req->cmdreg |= (CMD_CRC_CHK_ENABLE | CMD_IDX_CHK_ENABLE | DATA_PRESENT_SEL);
    req->blocksize = SD_BLOCK_SZ;
    req->nblocks = 1;
    req->databuf = cmd_param->databuf;
    req->buflen = req->blocksize;
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_emmc_cmd6_set_ext_csd
 *		prepare for cmd6-set_ext_csd function mode of emmc device
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_emmc_cmd6_set_ext_csd(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                  struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd6_emmc_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd6_emmc_param_t *)&cmd_param->param[0];
    args = ((param->access_mode & 0x3) << 24) | (param->index << 16) |
           (param->value << 8) | (param->cmd_set & 0x3);

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | CMD_RESP1_CARDSTS | MMC_RESP3_LEN_48B;

    /* extract cmd parameters */
    mmc_cmd_fill(req, EMMC_CMD6_SET_EXT_CSD, args, req->flags);
    req->cmdreg |= (CMD_CRC_CHK_ENABLE | CMD_IDX_CHK_ENABLE);
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd2_send_cid
 *		prepare for cmd8 send interface condition
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd2_send_cid(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                              struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd2_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd2_param_t *)&cmd_param->param[0];

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | CMD_RESP2_CIDCSD | MMC_RESP1_LEN_136;

    /* extract cmd parameters */
    args = param->args;
    mmc_cmd_fill(req, MMC_CMD2_ALL_SEND_CID, args, req->flags);
    req->cmdreg |= CMD_CRC_CHK_ENABLE;
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd7_card_select
 *	prepare for cmd7 for card select/deselect
 *	host resend the CMD3 to change its RCA number
 *	other than 0 and then use CMD7 with RCA=0 for
 *	card de-selection
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd7_card_select(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                 struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd7_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd7_param_t *)&cmd_param->param[0];

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48;

    /* extract cmd parameters */
    args = (param->rca << 16);
    mmc_cmd_fill(req, MMC_CMD7_CARD_SELECT, args, req->flags);
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd9_send_csd
 *	prepare for cmd9 for send card's csd data
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd9_send_csd(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                              struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd9_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd9_param_t *)&cmd_param->param[0];

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | CMD_RESP2_CIDCSD | MMC_RESP1_LEN_136;

    /* extract cmd parameters */
    args = (param->rca << 16);
    mmc_cmd_fill(req, MMC_CMD9_SEND_CSD, args, req->flags);
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_acmd41_init_card
 *		prepare for cmd41, defines to card that next cmd
 * is application specific cmd, rather than standard command
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_acmd41_init_card(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                 struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd41_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd41_param_t *)&cmd_param->param[0];

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48;

    /* extract cmd parameters */
    args = 0;
    if (param->is_hcs_sdxc_support)
        args |= ACMD41_HCS_SDXC_SUPPORT;
    if (param->s18r_use_18v)
        args |= ACMD41_S18R_USE_18V;
    if (param->is_ocr)
        args |= ACMD41_OCR_BIT21;
    if (param->is_max_perf)
        args |= ACMD41_XPC_MAX_PERFORMANCE;
    mmc_cmd_fill(req, MMC_ACMD41_SD_SEND_OP_COND,args, req->flags);
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_acmd51_send_scr
 *		prepare for acmd51, request card to send scr
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_acmd51_send_scr(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    if (cmd_param->databuf == NULL)
        return ERROR_INVARG;
    if (cmd_param->datalen < 8)
        return ERROR_INVARG;

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITH_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48 |
                 XFR_MODE_BLKCNT_EN_F | XFR_MODE_DATA_READ_F;

    req->int_mask = 0xffffffff;
    req->flags |= SDHCI_INTR_ENABLE;

    /* extract cmd parameters */
    args = 0;
    mmc_cmd_fill(req, MMC_CMD51_SEND_SCR, args, req->flags);
    req->cmdreg |= (CMD_CRC_CHK_ENABLE | CMD_IDX_CHK_ENABLE | DATA_PRESENT_SEL);
    req->blocksize = 8;
    req->nblocks = 1;
    req->databuf = cmd_param->databuf;
    req->buflen = req->blocksize;

    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd13_send_status
 *		prepare for cmd13, request card to send 64-byte(512-bits) status
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd13_send_status(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                  struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd13_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    if (cmd_param->databuf == NULL)
        return ERROR_INVARG;
    if (cmd_param->datalen < 64)
        return ERROR_INVARG;

    param = (struct cmd13_param_t *)&cmd_param->param[0];
    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITH_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48 |
                 XFR_MODE_BLKCNT_EN_F | XFR_MODE_DATA_READ_F;

    /* extract cmd parameters */
    args = (param->rca << 16);
    mmc_cmd_fill(req, MMC_CMD13_SEND_STATUS, args, req->flags);

    req->cmdreg |= DATA_PRESENT_SEL;
    req->blocksize = 64;
    req->databuf = cmd_param->databuf;
    req->buflen = 64;
    req->nblocks = 1;

    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd6_switch_function
 *		prepare for cmd6, switch function
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd6_send_status(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                 struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd6_param_t *param;
    struct acmd6_param_t *aparam;
    uint32_t args, fn_mode, bus_width, is_acmd;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd6_param_t *)&cmd_param->param[0];
    aparam = (struct acmd6_param_t *)&cmd_param->param[0];

    req->mmc_dev = mmc_dev;

    /* extract cmd parameters */
    is_acmd = param->is_acmd;
    if (!is_acmd) {
        if (cmd_param->databuf == NULL)
            return ERROR_INVARG;
        if (cmd_param->datalen < 64)
            return ERROR_INVARG;

        fn_mode = param->speed_mode;
        if (fn_mode == 0)
            args = 0x00fffff0;
        else
            args = 0x80fffff0 | (fn_mode & 0xF);
    } else {
        switch (aparam->bus_width) {
            case SD_BUSWIDTH_1:
                bus_width = 1;
                break;
            case SD_BUSWIDTH_4:
                bus_width = 2;
                break;
            default:
                bus_width = aparam->bus_width;
                break;
        }
        args = bus_width & 0x3;
    }

    req->flags = CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48;
    if (!is_acmd)
        req->flags |= (XFR_MODE_BLKCNT_EN_F | XFR_MODE_DATA_READ_F | CMD_WITH_DATA);
    else
        req->flags |= CMD_WITHOUT_DATA;

    mmc_cmd_fill(req, MMC_CMD6_SWITCH_FN, args, req->flags);

    if (!is_acmd) {
        req->cmdreg |= DATA_PRESENT_SEL;
        req->blocksize = 64;
        req->nblocks = 1;
        req->databuf = cmd_param->databuf;
        req->buflen = 64;
    }

    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd17_readblock
 *		prepare for cmd17, read single block
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd17_read_one_block(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                     struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd17_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd17_param_t *)&cmd_param->param[0];

    if (cmd_param->databuf == NULL)
        return ERROR_INVARG;
    if (cmd_param->datalen < SD_BLOCK_SZ)
        return ERROR_INVARG;

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITH_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48 |
                 XFR_MODE_BLKCNT_EN_F | XFR_MODE_DATA_READ_F;

    /* extract cmd parameters */
    /* block address */
    args = param->block_addr;
    mmc_cmd_fill(req, MMC_CMD17_READ_ONE_BLOCK, args, req->flags);
    req->cmdreg |= DATA_PRESENT_SEL;
    req->blocksize = SD_BLOCK_SZ;
    req->nblocks = 1;
    req->databuf = cmd_param->databuf;
    req->buflen = SD_BLOCK_SZ;

    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);
    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd18_read multi block
 *		prepare for cmd18, read multi block
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd18_read_multi_block(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                       struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd18_param_t *param;
    uint32_t args, xfer_len;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd18_param_t *)&cmd_param->param[0];

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITH_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48 |
                 XFR_MODE_BLKCNT_EN_F | XFR_MODE_DATA_READ_F |
                 XFR_MODE_MULTBLK_SEL_F;

    if (cmd_param->databuf == NULL)
        return ERROR_INVARG;

    xfer_len = param->nblocks * SD_BLOCK_SZ;
    if (cmd_param->datalen < xfer_len)
        return ERROR_INVARG;

    /* extract cmd parameters */
    /* block address */
    args = param->block_addr;
    mmc_cmd_fill(req, MMC_CMD18_READ_MULTI_BLOCK, args, req->flags);
    req->cmdreg |= (DATA_PRESENT_SEL);
    req->blocksize = SD_BLOCK_SZ;
    req->nblocks = param->nblocks;
    req->databuf = cmd_param->databuf;
    req->buflen = xfer_len;
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd19_read_tuning_block
 *		prepare for cmd19, read tuning block
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd19_read_tuning_block(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                        struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    if (cmd_param->databuf == NULL)
        return ERROR_INVARG;
    if (cmd_param->datalen < SD_TUNING_BLOCK_SZ)
        return ERROR_INVARG;

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITH_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48 |
                 XFR_MODE_BLKCNT_EN_F | XFR_MODE_DATA_READ_F;

    /* extract cmd parameters */
    /* block address */
    args = 0; /* set arg to 0 */
    mmc_cmd_fill(req, MMC_CMD19_SEND_TUNING_BLOCK, args, req->flags);
    req->cmdreg |= (DATA_PRESENT_SEL | CMD_IDX_CHK_ENABLE | CMD_CRC_CHK_ENABLE);
    req->blocksize = 64;
    req->nblocks = 1;
    req->databuf = cmd_param->databuf;
    req->buflen = SD_BLOCK_SZ;

    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);
    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd21_read_tuning_block
 *		prepare for cmd21, read tuning block
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd21_read_tuning_block(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                        struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd21_param_t *param;
    uint8_t bus_width;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd21_param_t *)&cmd_param->param[0];
    bus_width = param->bus_width;
    if ((bus_width != SD_BUSWIDTH_4) && (bus_width != SD_BUSWIDTH_8))
        return ERROR_INVARG;

    if (cmd_param->databuf == NULL)
        return ERROR_INVARG;

    if (cmd_param->datalen < EMMC_TUNING_BLOCK_SZ)
        return ERROR_INVARG;

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITH_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48 |
                 XFR_MODE_BLKCNT_EN_F | XFR_MODE_DATA_READ_F;

    /* extract cmd parameters */
    /* block address */
    args = 0;
    mmc_cmd_fill(req, MMC_CMD21_SEND_TUNING_BLOCK, args, req->flags);
    req->cmdreg |= (DATA_PRESENT_SEL | CMD_IDX_CHK_ENABLE | CMD_CRC_CHK_ENABLE);
    req->blocksize = (bus_width == SD_BUSWIDTH_4) ? 64 : 128;
    req->nblocks = 1;
    req->databuf = cmd_param->databuf;
    req->buflen = SD_BLOCK_SZ;

    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);
    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd23_set_blk_count
 *		prepare for cmd8 send interface condition
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd23_set_blockcnt(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                   struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd23_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd23_param_t *)&cmd_param->param[0];

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | MMC_RESP2_LEN_48;

    /* extract cmd parameters */
    args = param->block_cnt;
    mmc_cmd_fill(req, MMC_CMD23_SET_BLOCKCNT, args, req->flags);
    req->cmdreg |= CMD_CRC_CHK_ENABLE;
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd24_writeblock
 *		prepare for cmd24, write single block
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd24_write_one_block(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                      struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd24_param_t *param;
    uint32_t args;
    int i;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    if (cmd_param->databuf == NULL)
        return ERROR_INVARG;

    if (cmd_param->datalen < SD_BLOCK_SZ)
        return ERROR_INVARG;

    param = (struct cmd24_param_t *)&cmd_param->param[0];
    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITH_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48 |
                 XFR_MODE_BLKCNT_EN_F;

    /* extract cmd parameters */
    /* block address */
    args = param->block_addr;
    mmc_cmd_fill(req, MMC_CMD24_WRITE_ONE_BLOCK, args, req->flags);
    req->cmdreg |= DATA_PRESENT_SEL;
    req->blocksize = SD_BLOCK_SZ;
    req->nblocks = 1;
    req->databuf = cmd_param->databuf;
    req->buflen = SD_BLOCK_SZ;
    for (i = 0; i < req->buflen; i+=4) {
        if (i % 32 == 0)
            clog_print(CLOG_LEVEL6, "\n%04X: ", i);
        clog_print(CLOG_LEVEL6, " %08X", *(uint32_t *)&req->databuf[i]);
    }
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd25_writeblock
 *		prepare for cmd25, write single block
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd25_write_multi_block(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                        struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd25_param_t *param;
    uint32_t args;
    int i;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    if (cmd_param->databuf == NULL)
        return ERROR_INVARG;

    if (cmd_param->datalen < SD_BLOCK_SZ)
        return ERROR_INVARG;

    param = (struct cmd25_param_t *)&cmd_param->param[0];
    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITH_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48 |
                 XFR_MODE_BLKCNT_EN_F | XFR_MODE_MULTBLK_SEL_F;

    /* extract cmd parameters */
    /* block address */
    args = param->block_addr;
    mmc_cmd_fill(req, MMC_CMD25_WRITE_MULTI_BLOCK, args, req->flags);
    req->cmdreg |= DATA_PRESENT_SEL;
    req->cmdreg |= (CMD_CRC_CHK_ENABLE | CMD_IDX_CHK_ENABLE);
    req->blocksize = SD_BLOCK_SZ;
    req->nblocks = param->nblocks;
    req->databuf = cmd_param->databuf;
    req->buflen = SD_BLOCK_SZ * param->nblocks;
    for (i = 0; i < req->buflen; i+=4) {
        if (i % 32 == 0)
            clog_print(CLOG_LEVEL6, "\n%04X: ", i);
        clog_print(CLOG_LEVEL6, " %08X", *(uint32_t *)&req->databuf[i]);
        if (i % 512 == 0)
            clog_print(CLOG_LEVEL6, "\n-------------------------------\n");
    }
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/** \brief: prepare_mmc_cmd55_nxt_is_acmd
 *	prepare for cmd55, defines to card that next cmd
 * 	is application specific cmd, rather than standard command
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int prepare_mmc_cmd55_nxt_is_acmd(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req,
                                  struct cmd_param_s *cmd_param, uint32_t param_len)
{
    struct mmc_req_t *req;
    struct cmd55_param_t *param;
    uint32_t args;

    clog_print(CLOG_LEVEL7, "Enter-Func: %s\n",__func__);

    if (mmc_dev == NULL || io_req == NULL || cmd_param == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    param = (struct cmd55_param_t *)&cmd_param->param[0];

    req->mmc_dev = mmc_dev;
    req->flags = CMD_WITHOUT_DATA | CMD_RESP1_CARDSTS | MMC_RESP2_LEN_48;

    /* extract cmd parameters */
    args = (param->rca << 16);
    mmc_cmd_fill(req, MMC_CMD55_NXT_IS_ACMD, args, req->flags);
    req->cmdreg |= (CMD_CRC_CHK_ENABLE | CMD_IDX_CHK_ENABLE);
    clog_print(CLOG_LEVEL6, "%s: cmd(%x) args(%x) flags(%x)\n", __func__,
               req->cmdreg, req->args, req->flags);

    clog_print(CLOG_LEVEL7, "Exit-Func: %s\n",__func__);
    return 0;
}

/**
 * \brief mmc_submit_request
 *     mmc command execution engine (cee) submit request
 * \param priv_data: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \returns 0 on success, -ve error on failure
 */
int mmc_submit_request(void *priv_data, struct io_req_t *io_req)
{
    struct mmc_req_t *req;
    struct mmc_dev_t *mmc_dev = priv_data;

    if (io_req == NULL || mmc_dev == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req == NULL)
        return ERROR_INVARG;

    return mmc_core_submit_request(mmc_dev, io_req);
}

/**
 * \brief cee_prepare_io_req
 *     command execution engine (cee) cmd preparation
 * \param priv_data: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int mmc_prepare_io_req(void *priv_data, struct io_req_t *io_req, void *param, uint32_t len)
{
    struct mmc_req_t *req;
    struct mmc_dev_t *mmc_dev = priv_data;
    struct cmd_param_s *cmd_param = param;
    uint8_t cmd = cmd_param->cmd;

    clog_print(CLOG_LEVEL7, "1.Enter-Func: %s\n",__func__);

    if (io_req == NULL)
        return ERROR_INVARG;

    memset(io_req, 0, sizeof(struct io_req_t));
    req = mmc_request_alloc(0);
    if (req == 0)
        return ERROR_NOMEM;

    io_req->req = req;
    io_req->timeout = MMC_CMD_MAX_TIMEOUT;
    clog_print(CLOG_LEVEL7, "2.Enter-Func: %s cmd=%d\n",__func__,cmd);
    return mmc_cmd_prepare[cmd](mmc_dev, io_req, param, len);
}

/**
 * \brief cee_prepare_io_req
 *     command execution engine (cee) cmd preparation
 * \param priv_data: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \returns 0 on success, -ve error on failure
 */
int mmc_release_io_req(void *priv_data, struct io_req_t *io_req)
{
    struct mmc_req_t *req;

    if (io_req == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    if (req)
        mmc_request_free(req);

    return 0;
}

/**
 * \brief get_io_status
 *     command execution engine (cee) get cmd status
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \returns 0 on success, -ve error on failure
 */
int mmc_get_io_status(void *priv_data, struct io_req_t *io_req)
{
    struct mmc_req_t *req;

    if (io_req == NULL)
        return ERROR_INVARG;

    req = io_req->req;
    return req->status;
}

void mmc_cmd_init(void)
{
    int i;
    for (i = 0; i < 256; ++i)
        mmc_cmd_prepare[i] = prepare_mmc_cmd_not_supported;

    /* supported command list */
    /* mmc/sd command set */
    mmc_cmd_prepare[MMC_CMD0_GO_IDLE] = prepare_mmc_cmd0_go_idle;
    mmc_cmd_prepare[MMC_CMD2_ALL_SEND_CID] = prepare_mmc_cmd2_send_cid;
    mmc_cmd_prepare[MMC_CMD3_SEND_REL_ADR] = prepare_mmc_cmd3_send_rel_adr;
    mmc_cmd_prepare[MMC_CMD7_CARD_SELECT] = prepare_mmc_cmd7_card_select;
    mmc_cmd_prepare[MMC_CMD8_SEND_IF_COND] = prepare_mmc_cmd8_send_if_cond;
    mmc_cmd_prepare[MMC_CMD13_SEND_STATUS] = prepare_mmc_cmd13_send_status;
    mmc_cmd_prepare[MMC_CMD6_SWITCH_FN] = prepare_mmc_cmd6_send_status;
    mmc_cmd_prepare[MMC_CMD11_VOLT_SWITCH] = prepare_mmc_cmd11_voltage_switch;
    mmc_cmd_prepare[MMC_CMD9_SEND_CSD] = prepare_mmc_cmd9_send_csd;
    mmc_cmd_prepare[MMC_CMD17_READ_ONE_BLOCK] = prepare_mmc_cmd17_read_one_block;
    mmc_cmd_prepare[MMC_CMD18_READ_MULTI_BLOCK] = prepare_mmc_cmd18_read_multi_block;
    mmc_cmd_prepare[MMC_CMD19_SEND_TUNING_BLOCK] = prepare_mmc_cmd19_read_tuning_block;
    mmc_cmd_prepare[MMC_CMD21_SEND_TUNING_BLOCK] = prepare_mmc_cmd21_read_tuning_block;
    mmc_cmd_prepare[MMC_CMD23_SET_BLOCKCNT] = prepare_mmc_cmd23_set_blockcnt;
    mmc_cmd_prepare[MMC_CMD24_WRITE_ONE_BLOCK] = prepare_mmc_cmd24_write_one_block;
    mmc_cmd_prepare[MMC_CMD25_WRITE_MULTI_BLOCK] = prepare_mmc_cmd25_write_multi_block;
    mmc_cmd_prepare[MMC_ACMD41_SD_SEND_OP_COND] = prepare_mmc_acmd41_init_card;
    mmc_cmd_prepare[MMC_CMD51_SEND_SCR] = prepare_mmc_acmd51_send_scr;
    mmc_cmd_prepare[MMC_CMD55_NXT_IS_ACMD] = prepare_mmc_cmd55_nxt_is_acmd;

    /* EMMC specific commands */
    mmc_cmd_prepare[EMMC_CMD1_SEND_OP_COND] = prepare_mmc_cmd1_emmc_send_op_cond;
    mmc_cmd_prepare[EMMC_CMD8_SEND_EXT_CSD] = prepare_emmc_cmd8_send_ext_csd;
    mmc_cmd_prepare[EMMC_CMD6_SET_EXT_CSD] = prepare_emmc_cmd6_set_ext_csd;
}

/** \brief: mmc_dev_init
 * 	free the allocated mmc_dev_t object from pool
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd_param: pointer to cmd parameters
 * \param param_len: parameter length in bytes
 * \returns 0 on success, -ve error on failure
 */
int mmc_dev_init(struct mmc_dev_t *mmc_dev)
{
    if (mmc_dev == NULL)
        return ERROR_INVARG;

    mmc_cmd_init();

    /* allocate request queue to device */
    mmc_dev->req_queue = cmsg_q_create(MMC_REQ_QUEUE_DEPTH, "mmcReqQueue");
    return 0;
}

/** \brief: mmc_dev_new
 * 	allocate new mmc-dev object
 * \param flags: control flags
 * \returns pointer to mmc-dev object
 */
struct mmc_dev_t *mmc_dev_new(uint32_t flags)
{
    struct mmc_dev_t *mmc_dev;
    struct cee_ops_t ops;

    mmc_dev = mmc_dev_alloc(0);
    if (mmc_dev == NULL) {
        clog_print(CLOG_LEVEL5, "unable to create mmc-dev\n");
        return 0;
    }

    ops.priv_data = mmc_dev;
    ops.submit_request = mmc_submit_request;
    ops.prepare_io_req = mmc_prepare_io_req;
    ops.release_io_req = mmc_release_io_req;
    ops.get_io_status = mmc_get_io_status;

    /* register to cmd-execution engine */
    cee_register("mmc_cee", &ops);
    mmc_dev_init(mmc_dev);
    mmc_cmds_init(0);

    return mmc_dev;
}
