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
* \file		: mmc_test.c
* \author	: ravibabu@synopsys.com
* \date		: 23-Dec-2019
* \brief	: mmc_test.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	23-Dec-2019	ravibabui@synopsys.com  001		dev in progress
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
#include "parser.h"
#include "tee.h"
#include "mmc_test.h"
#include "hal.h"
#include "sdhci.h"

struct tee_t *g_mmc_tee;
uint32_t tc_block_adr, tc_block_cnt, tc_datalen;
void *tc_databuf;

/** \brief: tc_ mmc_cmd0_card_idle
 *	test-case to set mmc card to idle state
 * \param tc_data: pointer to test-case
 * \returns command success or fail
 */
int tc_mmc_cmd0_test_set_card_idle(void *tc_data)
{
    struct test_case_t *tc = (struct test_case_t *)tc_data;
    struct tee_t *tee = tc->tee;
    struct cmd_param_t *mmc_cmd = tee->cmd_req;

    struct sd_card_t *sd_card = (struct sd_card_t *)tee->priv_data;
    struct mmc_dev_t *mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;

    return mmc_cmd0_card_set_idle(mmc_dev, mmc_cmd, 0);
}

/** \brief: tc_mmc_cmd8_send_if_cond
 *	test-case (cmd8) to get interface condition of card
 * \param tc_data: pointer to test-case
 * \returns command success or fail
 */
int tc_mmc_cmd8_send_if_cond(void *tc_data)
{
    int retval;
    struct test_case_t *tc = (struct test_case_t *)tc_data;
    struct tee_t *tee = tc->tee;
    struct cmd_param_t *mmc_cmd = tee->cmd_req;

    struct sd_card_t *sd_card = (struct sd_card_t *)tee->priv_data;
    struct mmc_dev_t *mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;

    retval = mmc_cmd8_check_card_intf_condition(mmc_dev, mmc_cmd,
             tc->param[0], tc->param[1] & 0xff);
    if (mmc_cmd->state == XSTATE_CMD_DONE) {
        tc->status = TEST_STATUS_COMPLETED;
        if (mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
            tc->err_status = TEST_STATUS_SUCCESS;
            clog_print(CLOG_INFO, "volt=%d, pattern=%x\n",
                       (mmc_cmd->cur_cmd.resp[0] >> 8) & 0xff,
                       (mmc_cmd->cur_cmd.resp[0] & 0xff));
        } else
            tc->err_status = TEST_STATUS_FAILED;
    }

    return retval;
}

/** \brief: tc_mmc_acmd41_send_op_cond
 * *	acmd41 test case to get operating condition of sd-bus
 * \param tc_data: pointer to test-case
 * \returns command success or fail
 */
int tc_mmc_acmd41_send_op_cond(void *tc_data)
{
    int retval;
    struct test_case_t *tc = (struct test_case_t *)tc_data;
    struct tee_t *tee = tc->tee;
    struct cmd_param_t *mmc_cmd = tee->cmd_req;

    struct sd_card_t *sd_card = (struct sd_card_t *)tee->priv_data;
    struct mmc_dev_t *mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;

    retval = mmc_acmd41_check_card_op_condition(mmc_dev, mmc_cmd,
             tc->param[0], tc->param[1], tc->param[2], tc->param[3]);
    if (mmc_cmd->state == XSTATE_CMD_DONE) {
        tc->status = TEST_STATUS_COMPLETED;
        if (mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS) {
            tc->err_status = TEST_STATUS_SUCCESS;
            clog_print(CLOG_INFO, "sd-card supports %s, %s %s\n",
                       sd_card->is_sdxc_card ? "sdxc" : "sdhc",
                       sd_card->is_ush2_card ? "uhs2," : "",
                       sd_card->s18a_volt_switch_ready ? "ready to switch 1.8v":"");
        } else
            tc->err_status = TEST_STATUS_FAILED;
    }

    return retval;
}

/** \brief: tc_mmc_dev_enumerate
 * 	test-case to enumerate the device and initialize to known state
 * 	issues sequence of enumeration sd commands and configure devie
 * 	to known state with selected speed-mode, bus-width & vdd levels.
 * \param tc_data: pointer to test-case
 * \returns command success or fail
 */
int tc_mmc_device_enumerate(void *tc_data)
{
    struct test_case_t *tc = (struct test_case_t *)tc_data;
    struct tee_t *tee = tc->tee;
    struct cmd_param_t *mmc_cmd = tee->cmd_req;

    struct sd_card_t *sd_card = (struct sd_card_t *)tee->priv_data;

    tee->ops.select_device(tee->priv_data, 0);

    return mmc_dev_enumerate(sd_card, mmc_cmd,
                             tc->param[0], /* speed_mode */
                             tc->param[1], /* bus_width */
                             tc->param[2], /* xfer_mode */
                             tc->param[3], /* emmc_vdd */
                             tc->param[4], /* mmcm_clock */
                             tc->param[5]); /* usr_intf */
}

/** \brief: tc_emmc_dev_enumerate
 * 	test-case to enumerate the eMMC device and initialize to known state
 * 	issues sequence of enumeration sd commands and configure devie
 * 	to known state with selected speed-mode, bus-width & vdd levels.
 * \param tc_data: pointer to test-case
 * \returns command success or fail
 */
int tc_emmc_device_enumerate(void *tc_data)
{
    struct test_case_t *tc = (struct test_case_t *)tc_data;
    struct tee_t *tee = tc->tee;
    struct cmd_param_t *mmc_cmd = tee->cmd_req;

    struct sd_card_t *sd_card = (struct sd_card_t *)tee->priv_data;

    tee->ops.select_device(tee->priv_data, 1);

    return emmc_dev_enumerate(sd_card, mmc_cmd,
                              tc->param[0], /* speed_mode */
                              tc->param[1], /* bus_width */
                              tc->param[2], /* xfer_mode */
                              tc->param[3], /* emmc_vdd */
                              tc->param[4], /* mmcm_clock */
                              tc->param[5]); /* usr_intf */
}

/** \brief: tc_mmc_read_block
 * 	test case to read sd blocks
 * \param tc_data: pointer to test-case
 * \returns command success or fail
 */
int tc_mmc_read_block(void *tc_data)
{
#define XSTATE_INIT	XSTATE0
#define XSTATE_RDBLK	XSTATE1
    int retval = 1;
    struct test_case_t *tc = (struct test_case_t *)tc_data;
    struct tee_t *tee = tc->tee;
    struct cmd_param_t *mmc_cmd = tee->cmd_req;
    static uint8_t state = XSTATE_INIT;

    struct sd_card_t *sd_card = (struct sd_card_t *)tee->priv_data;
    struct mmc_dev_t *mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;

    switch (state) {
        case XSTATE_INIT:
            tc_block_adr = tc->param[0];
            if (tc->param[1] > MAX_NUM_BLOCKS)
                tc->param[1] = MAX_NUM_BLOCKS;
            tc_block_cnt = tc->param[1];
            tc_databuf = mmc_alloc_buf(0, ALIGN_4BYTE);
            tc_datalen = tc_block_cnt * SD_BLOCK_SZ;
            memset(tc_databuf, 0, tc_datalen);
            state = XSTATE_RDBLK;

        case XSTATE_RDBLK:
            retval = mmc_read_block(mmc_dev, mmc_cmd,
                                    tc_block_adr, /* block address */
                                    tc_block_cnt, /* block count */
                                    tc_databuf, /* data buffer */
                                    tc_datalen); /* data length */
            if (mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_INIT;
                if (mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS)
                    print_data_buf(tc_databuf, tc_datalen);
            }
            break;
    }

    return retval;
}

/** \brief: tc_mmc_write_block
 * 	test case to write sd blocks
 * \param tc_data: pointer to test-case
 * \returns command success or fail
 */
int tc_mmc_write_block(void *tc_data)
{
#define XSTATE_INIT	XSTATE0
#define XSTATE_RDBLK	XSTATE1
    int retval = 1;
    struct test_case_t *tc = (struct test_case_t *)tc_data;
    struct tee_t *tee = tc->tee;
    struct cmd_param_t *mmc_cmd = tee->cmd_req;
    static uint8_t state = XSTATE_INIT;

    struct sd_card_t *sd_card = (struct sd_card_t *)tee->priv_data;
    struct mmc_dev_t *mmc_dev = (struct mmc_dev_t *)sd_card->mmc_dev;

    switch (state) {
        case XSTATE_INIT:
            tc_block_adr = tc->param[0];
            if (tc->param[1] > MAX_NUM_BLOCKS)
                tc->param[1] = MAX_NUM_BLOCKS;
            tc_block_cnt = tc->param[1];
            tc_databuf = mmc_alloc_buf(0, ALIGN_4BYTE);
            tc_datalen = tc_block_cnt * SD_BLOCK_SZ;
            memset(tc_databuf, 0, tc_datalen);
            buffer_fill(tc_databuf, tc_datalen, tc->param[2], tc->param[3]);
            state = XSTATE_RDBLK;

        case XSTATE_RDBLK:
            retval = mmc_write_block(mmc_dev, mmc_cmd,
                                     tc_block_adr, /* block address */
                                     tc_block_cnt, /* block count */
                                     (uint8_t *)tc_databuf, /* data buffer */
                                     tc_datalen); /* data length */
            if (mmc_cmd->state == XSTATE_CMD_DONE) {
                state = XSTATE_INIT;
                if (mmc_cmd->cur_cmd.status == IO_STATUS_SUCCESS)
                    print_data_buf(tc_databuf, tc_datalen);
            }
            break;
    }

    return retval;
}

/** \brief: tc_mmc_mshc_read_version
 * 	test-case to read mshc version and display version details
 * \param tc_data: pointer to test-case
 * \returns command success or fail
 */
int tc_mmc_mshc_read_version(void *tc_data)
{
    struct test_case_t *tc = (struct test_case_t *)tc_data;
    struct tee_t *tee = tc->tee;
    struct cmd_param_t *mmc_cmd = tee->cmd_req;
    struct hal_obj_t *hal;
    uint16_t vptr;
    addr_t io_mem;
    uint32_t mshc_ver_id, mshc_ver_type;

    hal = tee->ops.get_hal_obj(tee->priv_data, &io_mem);
    mmc_cmd->state = XSTATE_CMD_DONE;
    if (hal) {
        vptr = hal_read16(hal, (void *)(io_mem + P_VENDOR_1_SPECIFIC_AREA));/* 0x500*/
        mshc_ver_id = hal_read(hal, (void *)(io_mem + vptr + SNPS_MSHC_VER_ID_R));
        mshc_ver_type = hal_read(hal, (void *)(io_mem + vptr + SNPS_MSHC_VER_TYPE_R));
        clog_print(CLOG_INFO, "MSHC_VER_ID = %08x => %c.%c%c-%x\n", mshc_ver_id,
                   (mshc_ver_id >> 24), (mshc_ver_id >> 16) & 0xFF,
                   (mshc_ver_id >> 8) & 0xFF, (mshc_ver_id & 0xff));
        clog_print(CLOG_INFO, "MSHC_VER_TYPE = %08x => %c%c%c%c\n", mshc_ver_type,
                   (mshc_ver_type >> 24), (mshc_ver_type >> 16) & 0xFF,
                   (mshc_ver_type >> 8) & 0xFF, (mshc_ver_type & 0xff));
        mmc_cmd->cur_cmd.status = IO_STATUS_SUCCESS;
    } else {
        clog_print(CLOG_ERR, "unable to get hal_obj\n");
        mmc_cmd->cur_cmd.status = TEST_STATUS_FAILED;
    }
    return mmc_cmd->state;
}

/** \brief: mmc_prepare_test_case
 *	prepare mmc test case
 * \param tc: pointer to testcase
 * \return 0 on success, -ve error on failure
 */
int mmc_prepare_test_case(struct test_case_t *tc)
{
    int i, found = 0;
    for (i = 0; i < MAX_MMC_TESTCASE; ++i) {
        printf("%s: (%s, %s)\n", __func__, mmc_tc_desc[i].tc_id_name, tc->tc_id_name);
        if (strcmp(mmc_tc_desc[i].tc_id_name, tc->tc_id_name) == 0) {
            printf("assigning test_fn(%p) to tc_id(%d)\n", mmc_tc_desc[i].test_fn, i);
            tc->test_fn = mmc_tc_desc[i].test_fn;
            found = 1;
            break;
        }
    }

    if (!found) {
        clog_print(CLOG_ERR, "%s: test case is NOT supported\n",
                   tc->tc_id_name);
        return ERROR_OPER_FAIL;
    }

    return 0;
}

/** \brief: mmc_run_test_case
 *	excute test case by CEE
 * \param tc: pointer to testcase
 * \return 0 on success, -ve error on failure
 */
int mmc_run_test_case(struct test_case_t *tc)
{
    if (tc->test_fn)
        return tc->test_fn(tc);
    return 0;
}

/** \brief: mmc_select_device
 *	select device SD or eMMC
 * \param tc: pointer to testcase
 * \return 0 on success, -ve error on failure
 */
int mmc_select_device(void *priv_data, uint8_t is_emmc)
{
    struct sd_card_t *sd_card;
    struct mmc_dev_t *mmc_dev;
    struct sdhci_t *sdhci;

    sd_card = (struct sd_card_t *)priv_data;
    if (sd_card == NULL)
        return ERR_INVARG;

    mmc_dev = sd_card->mmc_dev;
    if (mmc_dev && mmc_dev->host) {
        sdhci = (struct sdhci_t *)mmc_dev->host->drv_data;
        if (sdhci) {
            sdhci->is_emmc_dev = is_emmc ? 1 : 0;
            return 0;
        }
    }
    return ERROR_OPER_FAIL;
}

/** \brief: mmc_get_status
 *	get status of test-case
 * \param tc: pointer to testcase
 * \return 0 on success, -ve error on failure
 */
struct hal_obj_t *mmc_get_hal_obj(void *priv_data, addr_t *io_mem)
{
    struct sd_card_t *sd_card;
    struct mmc_dev_t *mmc_dev;
    struct sdhci_t *sdhci;

    sd_card = (struct sd_card_t *)priv_data;
    if (sd_card == NULL)
        return NULL;

    mmc_dev = sd_card->mmc_dev;
    if (mmc_dev && mmc_dev->host) {
        sdhci = (struct sdhci_t *)mmc_dev->host->drv_data;
        if (sdhci) {
            *io_mem = sdhci->io_base;
            return sdhci->hal;
        }
    }
    return NULL;
}

/** \brief: mmc_get_status
 *	get status of test-case
 * \param tc: pointer to testcase
 * \return 0 on success, -ve error on failure
 */
int mmc_get_status(struct test_case_t *tc)
{
    /* not used */
    return 0;
}


/** \brief: mmc_release_test_case
 *	abort/release testcase from execution.
 * \param tc: pointer to testcase
 * \return 0 on success, -ve error on failure
 */
int mmc_release_test_case(struct test_case_t *tc)
{
    /* not used */
    return 0;
}

/** \brief: mmc_update_tc_param
 *	update the test-parameter to test-case parameter fields
 * \param tc: pointer to testcase
 * \param param: parameter key/value pair.
 * \return 0 on success, -ve error on failure
 */
int mmc_update_tc_param(struct test_case_t *tc, struct key_val_t *param)
{
    int i, j, found = 0, err_flag = 0;

    for (i = 0; i < MAX_MMC_TESTCASE; ++i) {
        printf("[%d] %s, %s\n", i, mmc_tc_desc[i].tc_id_name, tc->tc_id_name);
        if (strcmp(mmc_tc_desc[i].tc_id_name, tc->tc_id_name) == 0) {
            for (j = 0; j < mmc_tc_desc[i].tc_max_param; ++j) {
                if (strcmp(mmc_tc_desc[i].param[j].name, &param->w1.name[11]) == 0) {
                    tc->param[tc->param_len] = stoi(param->w2.name, &err_flag);
                    if (err_flag != 0)
                        clog_print(CLOG_ERR, "%s: Invalid parameter value for %s\n",
                                   mmc_tc_desc[i].tc_id_name, param->w1.name);
                    tc->param_len++;
                    found = 1;
                    break;
                }
            }
            if (found)
                break;
        }
    }

    if (!found) {
        clog_print(CLOG_ERR, "%s: Invalid test case id\n",
                   tc->tc_id_name);
        return ERROR_OPER_FAIL;
    }

    return 0;
}

/** \brief: mmc_test_tee_init
 *	initialize mmc-test module and register to TEE
 * \param sd_card: pointer to sd_card
 * \param tc_cfg_file: testcase configuration input file
 * \return 0 on success, -ve error on failure
 */
int mmc_test_tee_init(struct sd_card_t *sd_card, char *tc_cfg_file)
{
    struct tee_ops_t ops;

    /* set mmc tee operation */
    ops.priv_data = sd_card;
    ops.prepare_test_case = mmc_prepare_test_case;
    ops.run_test_case = mmc_run_test_case;
    ops.get_status = mmc_get_status;
    ops.release_test_case = mmc_release_test_case;
    ops.update_tc_param = mmc_update_tc_param;
    ops.get_hal_obj = mmc_get_hal_obj;
    ops.select_device = mmc_select_device;

    /* register to tee */
    g_mmc_tee = NULL;
    g_mmc_tee = register_tee("mmc-tee", &ops, tc_cfg_file);
    if (g_mmc_tee == NULL) {
        clog_print(CLOG_ERR, "Unable to register mmct-test to tee\n");
        return ERROR_OPER_FAIL;
    } else
        clog_print(CLOG_INFO, "registered mmc-test to tee\n");

    printf("%s: g_mmc_tee = %p\n", __func__, g_mmc_tee);

    start_tee(g_mmc_tee);

    return 0;
}
