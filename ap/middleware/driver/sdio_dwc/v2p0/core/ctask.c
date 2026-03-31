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
* \file		: ctask.c
* \author	: ravibabu@synopsys.com
* \date		: 01-July-2019
* \brief	: ctask.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include "dwc_type.h"
#include "ctask.h"
#include "error.h"

struct ctask_os_t ctask_os;

/**
 * \brief ctask_create
 *	create the task and initialized to init state
 * \param fn: task function
 * \param argx: argument to function (maximum of 4 user defined arguments to task)
 * returns the unique task_id
 */
int ctask_create(char *name, int (*fn)(uint8_t task_id, void *task_pdata),
                 void *task_pdata, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
    int retval = ERROR_OPER_FAIL;

    if (ctask_os.create)
        return ctask_os.create(name, fn, task_pdata, arg2, arg3, arg4);

    return retval;
}

/**
 * \brief ctask_enable
 *	enable the task to run state
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_enable(uint8_t task_id)
{
    if (ctask_os.enable)
        return ctask_os.enable(task_id);

    return ERROR_OPER_FAIL;
}

/**
 * \brief ctask_suspend
 *	suspend the task
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_suspend(uint8_t task_id)
{
    if (ctask_os.suspend)
        return ctask_os.suspend(task_id);

    return ERROR_OPER_FAIL;
}

/**
 * \brief ctask_resume
 *	resume the task
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_resume(uint8_t task_id)
{
    if (ctask_os.resume)
        return ctask_os.resume(task_id);

    return ERROR_OPER_FAIL;
}

/**
 * \brief ctask_sleep
 *	puts the task to sleep state
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_sleep(uint8_t task_id, uint32_t delay_ms)
{
    if (ctask_os.sleep)
        return ctask_os.sleep(task_id, delay_ms);

    return ERROR_OPER_FAIL;
}

/**
 * \brief ctask_exit
 *	exit the task from scheduling
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_exit(uint8_t task_id)
{
    if (ctask_os.exit)
        return ctask_os.exit(task_id);

    return ERROR_OPER_FAIL;
}

/**
 * \brief ctask_shut_down
 *	shuts down all tasks
 * \param task_id: task id
 */
void ctask_shut_down(void)
{
    if (ctask_os.shut_down)
        return ctask_os.shut_down();
}

/**
 * \brief ctask_get_fnstate
 *	gets the state of task function of task_id
 * \param task_id: task id
 * returns the state of task-id
 */
int ctask_get_fnstate(uint8_t task_id)
{
    if (ctask_os.get_fnstate)
        return ctask_os.get_fnstate(task_id);

    return ERROR_OPER_FAIL;
}

/**
 * \brief ctask_set_fnstate
 *	set the state of task function of task_id
 * \param task_id: task id
 * returns the state of task-id
 */
int ctask_set_fnstate(uint8_t task_id, uint8_t nxt_state)
{
    if (ctask_os.set_fnstate)
        return ctask_os.set_fnstate(task_id, nxt_state);

    return ERROR_OPER_FAIL;
}

/**
 * \brief ctask_print_state
 *	print the states of all tasks
 * returns none
 */
void ctask_print_state(void)
{
    if (ctask_os.print_state)
        ctask_os.print_state();
}

/**
 * \brief ctask_schedular
 *	schedules/invokes all ready tasks
 * \param task_id: task id
 */
void ctask_schedular(void)
{
    if (ctask_os.schedular)
        ctask_os.schedular();
}

/**
 * \brief ctask_os_register
 *	register ctask os methods
 * \param os: poiter to ctask_os APIs
 */
int ctask_os_register(struct ctask_os_t *os)
{
    if(!os)
        return -ERROR_INVARG;

    ctask_os.create = os->create;
    ctask_os.enable = os->enable;
    ctask_os.suspend = os->suspend;
    ctask_os.resume = os->resume;
    ctask_os.sleep = os->sleep;
    ctask_os.schedular = os->schedular;
    ctask_os.get_fnstate = os->get_fnstate;
    ctask_os.set_fnstate = os->set_fnstate;
    ctask_os.print_state = ctask_print_state;

    ctask_os.semaphore_init = os->semaphore_init;
    ctask_os.semaphore_take = os->semaphore_take;
    ctask_os.semaphore_give = os->semaphore_give;

    return 0;
}
