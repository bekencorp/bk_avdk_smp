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
* \file		: cee.c
* \author	: ravibabu@synopsys.com
* \date		: 10-Nov-2019
* \brief	: cee.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "dwc_type.h"
#include "ctask.h"
#include "cmsg_queue.h"
#include "clog.h"
#include "cwatch.h"
#include "cee.h"
#include "mshc_regs.h"
#include "sd_cmd.h"
#include "mmc_dev.h"
#include "sd_cmds.h"
#include "os/str.h"

/* globals */
int cee_log_level;
int cee_init_done = 0;
uint8_t card_init_complete = 0;
uint32_t cee_core_task_cnt;
uint8_t cee_core_task_state;
uint8_t cee_stop_at_every_cmd = 0;
struct cee_t cee_list[MAX_NUM_CEE];

/**
 * \brief cee_submit_request
 *     command execution engine (cee) submit request
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 */
uint8_t cee_submit_request(struct cee_t *cee, struct io_req_t *io_req)
{
    if (cee && cee->ops.submit_request)
        return cee->ops.submit_request(cee->ops.priv_data, io_req);
    return ERROR_OPER_FAIL;
}

/**
 * \brief cee_prepare_io_req
 *     command execution engine (cee) cmd preparation
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 * \param param: pointer to parameter list
 * \param len: paramerter length
 */
uint8_t cee_prepare_io_req(struct cee_t *cee, struct io_req_t *io_req, void *param, uint32_t len)
{
    if (cee && cee->ops.prepare_io_req) {
        return cee->ops.prepare_io_req(cee->ops.priv_data, io_req, param, len);
    }
    return ERROR_OPER_FAIL;
}

/**
 * \brief cee_prepare_io_req
 *     release io_request
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 */
uint8_t cee_release_io_req(struct cee_t *cee, struct io_req_t *io_req)
{
    if (cee && cee->ops.release_io_req)
        return cee->ops.release_io_req(cee->ops.priv_data, io_req);
    return ERROR_OPER_FAIL;
}

/**
 * \brief get_io_status
 *     command execution engine (cee) get cmd status
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 */
uint8_t cee_get_io_status(struct cee_t *cee, struct io_req_t *io_req)
{
    if (cee && cee->ops.get_io_status)
        return cee->ops.get_io_status(cee->ops.priv_data, io_req);
    return ERROR_OPER_FAIL;
}

/**
 * \brief cee_init_
 *     command execution engine (cee) module init
 * \param level: debug level
 */
uint8_t cee_init(uint8_t level)
{
    cee_log_level = level;
    cee_init_done = 0;
    memset(cee_list, 0, sizeof(cee_list));

    clog_print(cee_log_level, "cmd execution engine (cee) init done\n");

    return 0;
}


/** \brief: cee_execute_cmd
 * 	execute cmd
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 */
int cee_execute_cmd(struct cee_t *cee, struct io_req_t *io_req)
{
#define XSTATE0_INIT	XSTATE0
#define XSTATE1_SUBMIT	XSTATE1
#define XSTATE2_WAITIO	XSTATE2
#define XSTATE3_DONE	XSTATE3
    int retval;

    if (io_req == NULL)
        return ERROR_INVARG;

    clog_print(CLOG_LEVEL3, "1.%s cmd-state(%d)\n",__func__, io_req->state);

    switch (io_req->state)	{
        case XSTATE0_INIT: /* new request */
            io_req->status = IO_STATUS_IN_PROGRESS;
            io_req->state = XSTATE1_SUBMIT;
        //break; /* avoided break purposefully, fix for coverity */

        case XSTATE1: /* request submission */
            retval = cee_submit_request(cee, io_req);
            if (retval < 0) {
                clog_print(CLOG_LEVEL3, "unable to sumbit mmc-req to host-core\n");
                io_req->status = IO_STATUS_SUBMIT_FAIL;
                io_req->state = XSTATE0_INIT;
                return retval;
            }
            io_req->state = XSTATE2_WAITIO;
        //break; /* avoided break purposefully, fix for coverity */

        case XSTATE2_WAITIO: /* wait for mmc request completion */
            /* the calling task is put sleep for request timeout period
               once request completed or timeout the task is put to run state */
            io_req->state = XSTATE3_DONE;
            clog_print(CLOG_LEVEL3, "%s: sleeping....\n",__func__);
            ctask_sleep(io_req->task_id, io_req->timeout);
            break;

        case XSTATE3_DONE:
            if (io_req->status != IO_STATUS_SUCCESS) {
                io_req->status = IO_STATUS_TIMEOUT;
                io_req->state = XSTATE0_INIT;
            }
            break;
        default:
            break;
    }

    clog_print(CLOG_LEVEL7, "2.%s cmd-state(%d)\n",__func__, io_req->state);

    return io_req->state;
}

/**
 * \brief io_req_init
 * 	inititalize the request init
 * \param cee: pointer to cee structure
 * \param io_req: pointer to io_request
 */
void io_req_init(uint8_t task_id, struct io_req_t *io_req)
{
    io_req->state = XSTATE0;
    io_req->status = 0;
    io_req->abort = 0;
    io_req->timeout = 5000; //MMC_CMD_MAX_TIMEOUT;
    io_req->task_id = task_id;
    io_req->repeat_cnt = 1;
}

/**
 * \brief cee_core_task
 *	CEE core task
 * \param task_id: task-id
 * \param args: pointer to driver data
 */
int cee_core_task(uint8_t task_id, void *args)
{
#define XSTATE0_CEE_INIT	XSTATE0
#define XSTATE1_CEE_PREP	XSTATE1
#define XSTATE2_CEE_EXEC	XSTATE2
#define XSTATE3_CEE_WAITIO	XSTATE3

    struct cee_t *cee;
    int retval, i, len;
    int dummy;

    int state = ctask_get_fnstate(task_id);
    int nxt_state = state;

    cee = (struct cee_t *)args;
    if (cee == NULL)
        return 0;

    clog_print(CLOG_LEVEL7, "========== %s state(%d) cmd_cnt(%d)==========\n", __func__, state,
               cee->io_stat.tot_cnt);
    cee_core_task_cnt ++;

    switch(state) {
        case XSTATE0_CEE_INIT:
            if (cmsg_q_is_empty(cee->inp_req_queue))
                return 1;

            cee->cmd_param = NULL;
            len = cmsg_q_receive(cee->inp_req_queue, &cee->cmd_param, sizeof(addr_t));
            if (cee->cmd_param == NULL)
                return 1;
            clog_print(CLOG_LEVEL3, "%s rcvd cmdno(%d), param_len = %d\n", __func__,
                       cee->cmd_param->cmd, len);
            nxt_state = XSTATE1_CEE_PREP;

        //break; /* avoided break for purposefully, fix for coverity */

        case XSTATE1_CEE_PREP:
            retval = cee_prepare_io_req(cee, &cee->io_req, cee->cmd_param,
                                        sizeof(struct cmd_param_s));
            if (retval != 0) {
                clog_print(CLOG_ERR, "%s: failed to prepare cmd-req\n", cee->name);
                return 1;
            }
            io_req_init(task_id, &cee->io_req);
            cee->io_req.resp = &cee->cmd_param->resp[0];
            nxt_state = XSTATE2_CEE_EXEC;
            clog_print(CLOG_LEVEL3, "submiting cee io_request\n");

        //break; /* avoided break for purposefully, fix for coverity */

        case XSTATE2_CEE_EXEC:
            if (cee->cmd_param->cmd != 129) {
                if (cee_stop_at_every_cmd) {
                    clog_print(CLOG_LEVEL6, "press any key to continue\n");
                    /* for debug purpose executing each cmd one by one */
                    scanf("%d", &dummy);
                }
            }

            retval = cee_execute_cmd(cee, &cee->io_req);
            if (retval < 0) {
                clog_print(CLOG_LEVEL5, "unable to execute cmd (error=%d)\n", retval);
                return 1;
            }
            nxt_state = XSTATE3_CEE_WAITIO;
            break;

        case XSTATE3_CEE_WAITIO: /* execute next step or next mmc cmd */
            if (cee->io_req.status != IO_STATUS_IN_PROGRESS) {
                cee->cmd_param->status = cee->io_req.status;
                cee->io_stat.tot_cnt++;
                if (cee->io_req.status == IO_STATUS_SUCCESS) {
                    clog_print(CLOG_LEVEL5, "io_req is sucess\n");
                    cee->io_stat.pass_cnt++;
                    for (i = 0; i < 4; ++i)
                        clog_print(CLOG_LEVEL6, "<==resp[%d] = %x\n",
                                   i, cee->io_req.resp[i]);
                } else {
                    clog_print(CLOG_ERR, "io_req failed with error:%d\n",
                               cee->io_req.status);
                    cee->io_stat.fail_cnt++;
                }

                cee_release_io_req(cee, &cee->io_req);
                io_req_init(task_id, &cee->io_req);
                nxt_state = XSTATE0_CEE_INIT;
            }
            break;
        default:
            break;
    }

    cee_core_task_state = nxt_state;
    ctask_set_fnstate(task_id, nxt_state);

    return 1;
}

/**
 * \brief cee_register
 *     command execution engine (cee) registration
 * \param name: name of CEE engine
 * \param ops: cee operations
 */
struct cee_t *cee_register(char *name, struct cee_ops_t *ops)
{
    int i;
    struct cee_t *cee;

    for (i = 0; i < MAX_NUM_CEE; ++i)
        if (cee_list[i].allocated == 0)
            break;

    if (i >= MAX_NUM_CEE)
        return NULL;

    cee = &cee_list[i];

    cee->ops.submit_request = ops->submit_request;
    cee->ops.prepare_io_req = ops->prepare_io_req;
    cee->ops.release_io_req = ops->release_io_req;
    cee->ops.get_io_status = ops->get_io_status;
    cee->ops.priv_data = ops->priv_data;

    cee->inp_req_queue = cmsg_q_create(256, "cee-inpQ");
    if (cee->inp_req_queue <= 0) {
        clog_print(CLOG_INFO, "failed to create cee-inp-req-queue\n");
    }

    if (name) {
        int src_len = os_strlen(name);
        int dest_len = sizeof(cee->name);
        int cnt = MIN(src_len, dest_len);

        strncpy((void *)cee->name, name, cnt - 1);
    }
    cee->allocated = 1;

    add_to_cwatch("cee.totcnt", &cee->io_stat.tot_cnt, XUINT32);
    add_to_cwatch("cee.passcnt", &cee->io_stat.pass_cnt, XUINT32);
    add_to_cwatch("cee.failcnt", &cee->io_stat.fail_cnt, XUINT32);
    add_to_cwatch("cee_core_task_cnt", &cee_core_task_cnt, XUINT32);
    add_to_cwatch("cee_core_task_state", &cee_core_task_state, XUINT8);

    ctask_enable(ctask_create(name, cee_core_task, &cee_list[i], 0, 0, 0));

    return &cee_list[i];
}
