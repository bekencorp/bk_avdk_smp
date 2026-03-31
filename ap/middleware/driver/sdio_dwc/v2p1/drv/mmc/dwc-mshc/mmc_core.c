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
* \file		: mmc_core.c
* \author	: ravibabu@synopsys.com
* \date		: 21-June-2019
* \brief	: mmc_core.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "cee.h"
#include "ctask.h"
#include "cmsg_queue.h"
#include "clog.h"
#include "cwatch.h"
#include "clog.h"
#include "mmc_dev.h"

/** \brief: mmc_cancel_request
 *	cancel the mmc request
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \returns 0
 */
int mmc_cancel_request(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req)
{
    io_req->issued = 0;
    io_req->abort = 1;
    mmc_dev->cmd_inprogress = 0;
    return 0;
}

/** \brief: mmc_core_submit_request
	submit request to mmc request queue to execute
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \returns 0 on success, -ve error on failure
 */
int mmc_core_submit_request(struct mmc_dev_t *mmc_dev, struct io_req_t *io_req)
{
    addr_t data;
    if (mmc_dev == NULL || io_req == NULL)
        return ERROR_INVARG;

    clog_print(CLOG_LEVEL5, "[mmc_core_submit_request]submitting req:%p to mmc%d cip(%d)\n", io_req, mmc_dev->id,
               mmc_dev->cmd_inprogress);

    data = (addr_t)io_req;
    if (cmsg_q_send(mmc_dev->req_queue, &data, sizeof(addr_t)) == sizeof(addr_t))
        return 0;
    else
        return ERROR_OPER_FAIL;
}

/** \brief: mmc_core_request_complete
	comple the mmc-io request
 * \param io_req: pointer to io_request
 * \returns none
 */
void mmc_core_request_complete(struct io_req_t *io_req)
{
    struct mmc_req_t *req = io_req->req;

    req->mmc_dev->cmd_inprogress = 0;
    memcpy(io_req->resp, req->resp, 16);
    clog_print(CLOG_LEVEL5, "mmc_core req:%08x completed cip(%d)\n",
               io_req, req->mmc_dev->cmd_inprogress);
    ctask_resume(io_req->task_id);
}

/** \brief: mmc_core_request_handler
 *	mmc-core request handler thread,
 *	execute the mmc-cmd, only one request executed each time
 * \param task_id: task id
 * \param args: pointer to mmc-dev structure
 * \returns 1 to schedule this task always by ctask-schedular
 */
int mmc_core_request_handler(uint8_t task_id, void *args)
{
    int state = ctask_get_fnstate(task_id);
    int nxt_state = state;
    struct mmc_dev_t *mmc_dev = (struct mmc_dev_t *)args;
    struct io_req_t *io_req;

    switch(state) {
        case XSTATE0:
            if (mmc_dev->cmd_inprogress)
                break;

            if (cmsg_q_is_empty(mmc_dev->req_queue)) {
                clog_print(CLOG_LEVEL9, "%s: no request queued\n",__func__);
                break;
            }

            io_req = NULL;
            if (cmsg_q_receive(mmc_dev->req_queue, &io_req, sizeof(addr_t)) == sizeof(addr_t)) {
                clog_print(CLOG_LEVEL5, "mmc_core: executing req:%08x\n", io_req);

                /* retrive the io_request */
                clog_print(CLOG_LEVEL9, "retrive the io_request\n");
                if (io_req->abort) {
                    /* application aborted the request
                       early giveback the request */
                    io_req->status = MMC_CMD_STATUS_ABORT;
                    ctask_resume(io_req->task_id);
                    break;
                }
                mmc_dev->cmd_inprogress = 1;
                mmc_dev->host->ops.send_cmd(mmc_dev->host->drv_data, io_req);
            }
            break;
        case XSTATE1:
            break;
    }

    ctask_set_fnstate(task_id, nxt_state);

    return 1;
}

/** \brief: mmc_host_io_ctrl
 * 	host driver io control interface
 * \param mmc_dev: pointer to mmc_dev obj
 * \param io_req: pointer to io_request
 * \param cmd: cmd
 * \param args: cmd argument
 * @returns -ve on error 0 on success
 */
int mmc_host_io_ctrl(struct mmc_dev_t *mmc_dev, uint8_t cmd, uint32_t args)
{
    if (mmc_dev->host->ops.io_ctrl)
        return mmc_dev->host->ops.io_ctrl(mmc_dev->host->drv_data,
                                          cmd, args);
    else
        return ERROR_INVARG;
}

/**
 * \brief mmc_clock_enable
 *	configure the sdhci speed-mode, mmc-clocks
 * \param sdhci: pointer to sdhci object
 * \param speed_mode: sdhci speed-mode
 * \param mmc_vdd : mmc vdd volatage (1.8V or 3.3V)
 * \param mmcm_clock: setup mmcmc clocks
 * @returns -ve on error 0 on success
 */
int mmc_clock_enable(struct mmc_dev_t *mmc_dev, uint8_t speed_mode,
                     uint8_t mmc_vdd, uint32_t mmcm_clock)
{
    if (mmc_dev->host->ops.enable_clock)
        return mmc_dev->host->ops.enable_clock(mmc_dev->host->drv_data,
                                               speed_mode, mmc_vdd, mmcm_clock);
    else
        return ERROR_INVARG;
}

/**
 * \brief mmc_clock_disable
 *	disable the controller clock
 * \param sdhci: pointer to sdhci object
 * \returns -ve on error 0 on success
 * \returns none
 */
int mmc_clock_disable(struct mmc_dev_t *mmc_dev)
{
    if (mmc_dev->host->ops.disable_clock)
        return mmc_dev->host->ops.disable_clock(mmc_dev->host->drv_data);
    else
        return ERROR_INVARG;
}

/**
 * \brief mmc_set_bus_width
 *	set sd bus data width
 * \param sdhci: pointer to sdhci object
 * \param width: bus width 4 or 8bit
 * \returns -ve on error 0 on success
 * \returns none
 */
int mmc_set_bus_width(struct mmc_dev_t *mmc_dev, uint8_t bus_width)
{
    if (mmc_dev->host->ops.set_data_width)
        return mmc_dev->host->ops.set_data_width(mmc_dev->host->drv_data, bus_width);
    else
        return ERROR_INVARG;
}

/**
 * \brief mmc_set_voltage
 *	configure sdhci VDD1 or VDD2 voltage
 * \param sdhci: pointer to sdhci object
 * \param mmc_vdd_sel: mmc vdd selection
 * \param mmc_vdd_vol: mmc vdd volatge
 * \returns -ve on error 0 on success
 */
int mmc_set_voltage(struct mmc_dev_t *mmc_dev, uint8_t mmc_vdd_sel, uint8_t mmc_vdd_volt)
{
    if (mmc_dev->host->ops.set_voltage)
        return mmc_dev->host->ops.set_voltage(mmc_dev->host->drv_data,
                                              mmc_vdd_sel, mmc_vdd_volt);
    else
        return ERROR_INVARG;
}

/**
 * \brief mmc_set_speed
 *	set the speed mode
 * \param sdhci: pointer to sdhci object
 * \param speed_mode: sdhci speed-mode
 * \returns -ve on error 0 on success
 * \returns none
 */
int mmc_set_speed_mode(struct mmc_dev_t *mmc_dev, uint8_t speed_mode)
{
    if (mmc_dev->host->ops.set_speed)
        return mmc_dev->host->ops.set_speed(mmc_dev->host->drv_data, speed_mode);
    else
        return ERROR_INVARG;
}

/** \brief: mmc_core_init
	mmc core initialiation
 * \param mmc_dev: pointer to mmc_dev object
 * \param host: pointer to mmc-host object
 * \returns -ve on error 0 on success
 */
int mmc_core_init(struct mmc_dev_t *mmc_dev, struct mmc_host_t *host)
{
    int mmc_core_tid;

    mmc_dev->host = host;

    mmc_core_tid = ctask_create("mmc-req-handler", mmc_core_request_handler,
                                mmc_dev, 0, 0, 0);
    if (mmc_core_tid < 0) {
        clog_print(CLOG_LEVEL5, "mmc_core_init failed\n");
        return ERROR_OPER_FAIL;
    }
    mmc_dev->core_thread_id = mmc_core_tid;

    ctask_enable(mmc_core_tid);

    return 0;
}
