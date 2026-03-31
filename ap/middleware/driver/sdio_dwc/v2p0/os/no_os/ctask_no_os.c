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
* \file		: ctask_no_os.c
* \author	: ravibabu@synopsys.com
* \date		: 10-July-2019
* \brief	: ctask_no_os.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	10-July-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "dwc_type.h"
#include "ctask.h"
#include "ctask_no_os.h"
#include "terminal.h"
#include "clog.h"

/* static global data structures */
static struct tcb_t tcb_list[MAX_NUM_TASKS];
static int max_num_tcb;
static char *task_state[5] = {"init", "run", "sleep", "suspend", "resume"};

/**
 * \brief  no_os_ctask_create
 *	create the task and initialized to init state
 * \param fn: task function
 * \param argx: argument to function (maximum of 4 paramets to user-defined task)
 * returns unique task_id
 */
int no_os_ctask_create(char *name, int (*fn)(uint8_t task_id, void *task_pdata),
	void *task_pdata, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
	int i;
	
	/* input parameter check */
	if (fn == 0)
		return ERR_INVARG;

	/* allocate the free tcb */
	for (i = 0; i < MAX_NUM_TASKS; ++i)
		if (tcb_list[i].allocated == 0)
			break;
	
	if (i >= MAX_NUM_TASKS)
		return ERR_NORESOURCE;

	/* initialize the tcb structure */
	tcb_list[i].id = i;
	tcb_list[i].fn = fn;
	tcb_list[i].task_pdata = task_pdata;
	tcb_list[i].args[0] = arg2;
	tcb_list[i].args[1] = arg3;
	tcb_list[i].args[2] = arg4;
	tcb_list[i].allocated = 1;
	tcb_list[i].state = TASK_STATE_INIT;
	tcb_list[i].fn_state = XSTATE0;
	strncpy(tcb_list[i].name, name, 40);
	max_num_tcb++;

	/* returns the task-id */
	return tcb_list[i].id;
}

/**
 * \brief no_os_ctask_ctrl
 *	task control function to sets the state of task	
 * \param task_id: task id
 * \param ctrl_state: task's next state to changed
 * return 0 on sucess
 */
static int no_os_ctask_ctrl(uint8_t task_id, uint8_t ctrl_state)
{
	/* input parameter check */
	if ((task_id >= MAX_NUM_TASKS) || (tcb_list[task_id].id != task_id) 
		|| (tcb_list[task_id].allocated == 0))
		return ERR_INVARG;

	/* set the tasks state */
	switch (ctrl_state) {
	case TASK_STATE_INIT	:
	case TASK_STATE_RUN	:
	case TASK_STATE_SLEEP	:
	case TASK_STATE_SUSPEND	:	
	case TASK_STATE_RESUME	:
		if (tcb_list[task_id].state != ctrl_state)
			tcb_list[task_id].state = ctrl_state;
		break;
	default:
		return ERR_INVARG;
	}

	return 0;
}
/**
 * \brief no_os_ctask_enable
 *	enable the task to run state
 * \param task_id: task id
 */
int no_os_ctask_enable(uint8_t task_id)
{
	return no_os_ctask_ctrl(task_id, TASK_STATE_RUN);
}

/**
 * \brief no_os_ctask_suspend
 *	suspend the task
 * \param task_id: task id
 */
int no_os_ctask_suspend(uint8_t task_id)
{
	return no_os_ctask_ctrl(task_id, TASK_STATE_SUSPEND);
}

/**
 * \brief no_os_ctask_resume
 *	resume the task
 * \param task_id: task id
 */
int no_os_ctask_resume(uint8_t task_id)
{
	tcb_list[task_id].df = 1;
	return no_os_ctask_ctrl(task_id, TASK_STATE_RUN);
}

/**
 * \brief no_os_ctask_suspend
 *	suspend the task
 * \param task_id: task id
 */
int no_os_ctask_exit(uint8_t task_id)
{
	return no_os_ctask_ctrl(task_id, TASK_STATE_EXIT);
}

/**
 * \brief ctask_sleep
 *	puts the task to delayed/sleep state	
 * \param task_id: task id
 * \param delay_ms: delay
 * return 0 on sucess
 */
int no_os_ctask_sleep(uint8_t task_id, uint32_t delay_ms)
{
	int retval;

	tcb_list[task_id].delay_ms = delay_ms;
	retval = no_os_ctask_ctrl(task_id, TASK_STATE_SLEEP);

	return retval;
}

/**
 * \brief no_os_ctask_get_fnstate
 *	gets the state of task function of task_id		
 * \param task_id: task id
 * returns the state of task-id
 */
int no_os_ctask_get_fnstate(uint8_t task_id)
{
	if ((task_id >= MAX_NUM_TASKS) || (tcb_list[task_id].id != task_id) 
		|| (tcb_list[task_id].allocated == 0))
		return ERR_INVARG;

	return tcb_list[task_id].fn_state;
}

/**
 * \brief no_os_ctask_set_fnstate
 *	set the state of task function of task_id		
 * \param task_id: task id
 * returns the state of task-id
 */
int no_os_ctask_set_fnstate(uint8_t task_id, uint8_t nxt_state)
{
	if ((task_id >= MAX_NUM_TASKS) || (tcb_list[task_id].id != task_id) 
		|| (tcb_list[task_id].allocated == 0))
		return ERR_INVARG;

	tcb_list[task_id].fn_state = nxt_state;
	return nxt_state;
}

/**
 * \brief no_os_ctask_print_state
 *	prints the state of all task in tcblist		
 * param none
 * returns none
 */
void no_os_ctask_print_state(void)
{
	int i;

	struct tcb_t *tcb;

	clog_print(CLOG_INFO, "task-id\t\tname\t\tstate\n");
	clog_print(CLOG_INFO, "---------------------------------------\n");

	for (i = 0; i < MAX_NUM_TASKS; ++i) {
		if (tcb_list[i].allocated == 0)
			continue;
	
		tcb = &tcb_list[i];
		//clog_print(CLOG_INFO, "task-id%d\t%s\t(%s)\n", tcb->id,
		//	tcb->name, task_state[tcb->state]);
		clog_print(CLOG_INFO, "\t%d\t%s\t(%s) delay(%d) cnt(%d) s(%d) fs(%d) wait(%d) \n", tcb->id,
			tcb->name, task_state[tcb->state], tcb->delay_ms, tcb->cnt,
			tcb->state, tcb->fn_state, tcb->max_delay_ms);
	}
	clog_print(CLOG_INFO, "\n");
}

/**
 * \brief  no_os_cmd_list_ctask()
 * 		Terminal command list all ctask
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void no_os_cmd_list_ctask(void *terminal, token_t *token, int max_token)
{
	no_os_ctask_print_state();
}
/**
 * \brief ctask_linux_schedular
 *	round robin schedular
 * \param task_id: task id
 */
void no_os_ctask_shut_down(void)
{
	int i;
	struct tcb_t *tcb;
	clog_print(CLOG_INFO, "shut down all task");

	for (i = 0; i < MAX_NUM_TASKS; ++i) {
		if (tcb_list[i].allocated == 0)
			continue;
	
		tcb = &tcb_list[i];
		no_os_ctask_ctrl(tcb->id, TASK_STATE_EXIT);
	}
	clog_print(CLOG_INFO, "\n");
}
	
/**
 * \brief ctask_schedular
 *	round robin schedular
 * \param task_id: task id
 */
void no_os_ctask_schedular(void)
{
	int i;
	struct tcb_t *tcb;

	for (i = 0; i < max_num_tcb; ++i) {

		tcb = &tcb_list[i];
		if (tcb->allocated == 0)
			continue;

		switch (tcb->state) {
		case TASK_STATE_INIT	:
			break;
		case TASK_STATE_RUN	:
			if (tcb->delay_ms > tcb->max_delay_ms)
				tcb->max_delay_ms = tcb->delay_ms;
			tcb->delay_ms = 0;
			tcb->df = 0;
			if (tcb->fn(tcb->id, tcb->task_pdata) == 0) {
			//	tcb->state = TASK_STATE_SUSPEND;
				tcb->state = TASK_STATE_INIT;
				tcb->allocated = 0;
			}
			break;
		case TASK_STATE_SLEEP	:
			if (!tcb->df && tcb->delay_ms > 0) {
				tcb->delay_ms--;
				clog_print(CLOG_LEVEL9, "%s: delay(%d)\n", __func__,
						tcb->delay_ms);
				//usleep(1000);
			} else {
				tcb->state = TASK_STATE_RUN;
				clog_print(CLOG_LEVEL9, "thread: fn-%s sleep done\n", tcb->name);
			}
			tcb->max_delay_ms = 0;
			/*if (tcb->delay_ms == 0)
				tcb->state = TASK_STATE_RUN;
			else
				tcb->delay_ms--;*/
			break;
		case TASK_STATE_SUSPEND	:	
			break;
		case TASK_STATE_RESUME	:
			tcb->state = TASK_STATE_RUN;
			break;
		case TASK_STATE_EXIT	:
			tcb->state = TASK_STATE_INIT;
			tcb->allocated = 0;
			break;
		default:
			break;
		}
	}
}


/**
 * \brief  no_os_cmd_ctask_control()
 * 		Terminal command list all ctask
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void no_os_cmd_ctask_control(void *terminal, token_t *token, int max_token)
{
	int task_id, delay_ms;

	if (max_token > 1) {
		task_id = atoi(token[1].str);
	
		if (strcmp(token[0].str, "task-suspend") == 0)
			no_os_ctask_ctrl(task_id, TASK_STATE_SUSPEND);
		else if (strcmp(token[0].str, "task-resume") == 0)
			no_os_ctask_ctrl(task_id, TASK_STATE_RESUME);
		else if (strcmp(token[0].str, "task-exit") == 0)
			no_os_ctask_ctrl(task_id, TASK_STATE_EXIT);
		else if (max_token > 2) {
			delay_ms = atoi(token[2].str);
			if (strcmp(token[0].str, "task-sleep") == 0)
				no_os_ctask_sleep(task_id, delay_ms);
		}
	}
	no_os_ctask_print_state();
}

/**
 * \brief ctask_no_os_register
 *	ctask-os registeration
 * \param task_id: task id
 */
int ctask_no_os_register(void)
{
	struct ctask_os_t ctask_os;

	ctask_os.create 	= no_os_ctask_create;
	ctask_os.enable 	= no_os_ctask_enable;
	ctask_os.suspend 	= no_os_ctask_suspend;
	ctask_os.resume 	= no_os_ctask_resume;
	ctask_os.sleep 		= no_os_ctask_sleep;
	ctask_os.exit 		= no_os_ctask_exit;
	ctask_os.schedular 	= no_os_ctask_schedular;
	ctask_os.shut_down 	= no_os_ctask_shut_down;
	ctask_os.get_fnstate 	= no_os_ctask_get_fnstate;
	ctask_os.set_fnstate 	= no_os_ctask_set_fnstate;
	ctask_os.print_state 	= no_os_ctask_print_state;

	ctask_os.semaphore_init = no_os_ctask_semaphore_init;
	ctask_os.semaphore_take = no_os_ctask_semaphore_take;
	ctask_os.semaphore_give = no_os_ctask_semaphore_give;

	/* regiser the no-os ctask to ctask module */
	return ctask_os_register(&ctask_os);
}
