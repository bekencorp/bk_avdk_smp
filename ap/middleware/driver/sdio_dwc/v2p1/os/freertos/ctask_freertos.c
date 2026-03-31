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
* \file		: ctask_freertos.c
* \author	: ravibabu@synopsys.com
* \date		: 15-Sep-2019
* \brief	: ctask_freertos.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	15-Sep-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include "common.h"
#include "dwc_type.h"
#include "ctask.h"
#include "clog.h"
#include "FreeRTOS_POSIX.h"
#include "posix/pthread.h"
#include "posix/unistd.h"
#include "ctask_freertos.h"
#include "os/str.h"

struct tcb_t tcb_list[MAX_NUM_TASKS];
int max_num_tcb;
char *freertos_task_state[6] = {"init", "run", "sleep", "suspend", "resume", "exit"};

void *pthread_func(void *arg)
{
	struct tcb_t *tcb = (struct tcb_t *)arg;

	while (1) {
		tcb->cnt++;
		switch (tcb->state) {
		case TASK_STATE_INIT	:
			break;
		case TASK_STATE_RUN	:
			if (tcb->delay_ms > tcb->max_delay_ms)
				tcb->max_delay_ms = tcb->delay_ms;
			tcb->delay_ms = 0;
			tcb->df = 0;
			if (tcb->fn(tcb->id, tcb->task_pdata) == 0) {
				tcb->allocated = 0;
				pthread_exit(tcb->name);
			}
			break;
		case TASK_STATE_SLEEP	:
			if (!tcb->df && tcb->delay_ms > 0) {
				tcb->delay_ms--;
				usleep(1000);
			} else {
				tcb->state = TASK_STATE_RUN;
				clog_print(CLOG_LEVEL3, "thread: fn-%s sleep done\n", tcb->name);
			}
			tcb->max_delay_ms = 0;
			break;
		case TASK_STATE_SUSPEND	:
			usleep(1000);
			break;
		case TASK_STATE_RESUME	:
			clog_print(1, "%s resuming task- %s\n", __func__, tcb->name);
			tcb->state = TASK_STATE_RUN;
			break;
		case TASK_STATE_EXIT:
			tcb->allocated = 0;
			clog_print(1, "%s shutdown task- %s\n", __func__, tcb->name);
			pthread_exit(NULL);
			break;
		default:
			break;
		}
	}
}

/**
 * \brief ctask_freertos_create
 *	create the task and initialized to init state
 * \param fn: task function
 * \param arg1: argument to function
 */
int ctask_freertos_create(char *name, int (*fn)(uint8_t task_id, void *task_pdata),
	void *task_pdata, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
	int i, retval;
	
	if (fn == 0)
		return ERR_INVARG;

	for (i = 0; i < MAX_NUM_TASKS; ++i)
		if (tcb_list[i].allocated == 0)
			break;
	
	if (i >= MAX_NUM_TASKS)
		return ERR_NORESOURCE;

	tcb_list[i].id = i;
	tcb_list[i].fn = fn;
	tcb_list[i].task_pdata = task_pdata;
	tcb_list[i].args[0] = arg2;
	tcb_list[i].args[1] = arg3;
	tcb_list[i].args[2] = arg4;
	tcb_list[i].allocated = 1;
	tcb_list[i].state = TASK_STATE_INIT;
	tcb_list[i].fn_state = XSTATE0;
	tcb_list[i].cnt = 0;
	tcb_list[i].df = 0;
    if (name){
        int src_len = os_strlen(name);
        int dest_len = sizeof(tcb_list[i].name);
        int cnt = MIN(src_len, dest_len - 1);

        strncpy((void *)tcb_list[i].name, name, cnt);
    }

	max_num_tcb ++;

	retval = pthread_create(&tcb_list[i].thread, NULL, pthread_func, &tcb_list[i]);
	if (retval != 0)
		clog_print(CLOG_LEVEL7, "failed to create pthread for task %s\n", name);
	else
		clog_print(CLOG_LEVEL7, "created pthread for task %s\n", name);

	return tcb_list[i].id;
}

/**
 * \brief ctask_freertos_enable
 *	enable the task to run state
 * \param task_id: task id
 */
static int ctask_freertos_ctrl(uint8_t task_id, uint8_t ctrl_state)
{
	if ((task_id >= MAX_NUM_TASKS) || (tcb_list[task_id].id != task_id) 
		|| (tcb_list[task_id].allocated == 0))
		return ERR_INVARG;

	switch (ctrl_state) {
	case TASK_STATE_INIT	:
	case TASK_STATE_RUN	:
	case TASK_STATE_SLEEP	:
	case TASK_STATE_SUSPEND	:	
	case TASK_STATE_RESUME	:
	case TASK_STATE_EXIT	:
		if (tcb_list[task_id].state != ctrl_state)
			tcb_list[task_id].state = ctrl_state;
		break;
	default:
		return ERR_INVARG;
	}

	return 0;
}
/**
 * \brief ctask_freertos_enable
 *	enable the task to run state
 * \param task_id: task id
 */
int ctask_freertos_enable(uint8_t task_id)
{
	return ctask_freertos_ctrl(task_id, TASK_STATE_RUN);
}

/**
 * \brief ctask_freertos_suspend
 *	suspend the task
 * \param task_id: task id
 */
int ctask_freertos_suspend(uint8_t task_id)
{
	clog_print(CLOG_LEVEL5, "task_id%d suspended\n", task_id);
	return ctask_freertos_ctrl(task_id, TASK_STATE_SUSPEND);
}

/**
 * \brief ctask_freertos_resume
 *	resume the task
 * \param task_id: task id
 */
int ctask_freertos_resume(uint8_t task_id)
{
	clog_print(CLOG_LEVEL5, "task_id%d resume\n", task_id);
	tcb_list[task_id].df = 1;
	return ctask_freertos_ctrl(task_id, TASK_STATE_RUN);
}

/**
 * \brief ctask_freertos_resume
 *	resume the task
 * \param task_id: task id
 */
int ctask_freertos_exit(uint8_t task_id)
{
	clog_print(CLOG_LEVEL5, "task_id%d exit\n", task_id);
	return ctask_freertos_ctrl(task_id, TASK_STATE_EXIT);
}

/**
 * \brief ctask_freertos_sleep
 *	
 * \param task_id: task id
 */
int ctask_freertos_sleep(uint8_t task_id, uint32_t delay_ms)
{
	int retval = ERROR_OPER_FAIL;

//	if (tcb_list[task_id].delay_ms <= 1) 
	{
//		clog_print(1, "%s: old-delay(%d) new-delay(%d) task_id(%d)\n", __func__, 
//			tcb_list[task_id].delay_ms, delay_ms, task_id);
		tcb_list[task_id].delay_ms = delay_ms;
		retval = ctask_freertos_ctrl(task_id, TASK_STATE_SLEEP);
//	} else {
//		clog_print(1, "%s: delay(%d) task_id(%d) looping\n", __func__, 
//			tcb_list[task_id].delay_ms, task_id);
//		while(1);
	}
		

	return retval;
}

int ctask_freertos_get_fnstate(uint8_t task_id)
{
	if ((task_id >= MAX_NUM_TASKS) || (tcb_list[task_id].id != task_id) 
		|| (tcb_list[task_id].allocated == 0))
		return ERR_INVARG;

	return tcb_list[task_id].fn_state;
}

int ctask_freertos_set_fnstate(uint8_t task_id, uint8_t nxt_state)
{
	if ((task_id >= MAX_NUM_TASKS) || (tcb_list[task_id].id != task_id) 
		|| (tcb_list[task_id].allocated == 0))
		return ERR_INVARG;

	tcb_list[task_id].fn_state = nxt_state;
	return nxt_state;
}

void ctask_freertos_print_state(void)
{
	int i;
	struct tcb_t *tcb;
	clog_print(CLOG_INFO, "task-id\t\tname\t\tstate\n");
	clog_print(CLOG_INFO, "---------------------------------------\n");

	for (i = 0; i < MAX_NUM_TASKS; ++i) {
		if (tcb_list[i].allocated == 0)
			continue;
	
		tcb = &tcb_list[i];
		clog_print(CLOG_INFO, "\t%d\t%s\t(%s) dly(%d) cnt(%d) s(%d) fs(%d) wait(%d)\n", tcb->id,
			tcb->name, freertos_task_state[tcb->state], tcb->delay_ms, tcb->cnt,
			tcb->state, tcb->fn_state, tcb->max_delay_ms);
	}
	clog_print(CLOG_INFO, "\n");
}

/**
 * \brief ctask_freertos_schedular
 *	round robin schedular
 * \param task_id: task id
 */
void ctask_freertos_shut_down(void)
{
	int i;
	struct tcb_t *tcb;
	clog_print(CLOG_LEVEL5, "shut down all task");

	for (i = 0; i < MAX_NUM_TASKS; ++i) {
		if (tcb_list[i].allocated == 0)
			continue;
	
		tcb = &tcb_list[i];
		ctask_freertos_ctrl(tcb->id, TASK_STATE_EXIT);
	}
	clog_print(CLOG_LEVEL5, "\n");
}
	
/**
 * \brief ctask_freertos_schedular
 *	round robin schedular
 * \param task_id: task id
 */
void ctask_freertos_schedular(void)
{
#ifndef CONFIG_CTASK_FREERTOS
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
			if (tcb->fn(tcb->id, tcb->args) == 0)
				tcb->state = TASK_STATE_SUSPEND;
			break;
		case TASK_STATE_SLEEP	:
			if (tcb->delay_ms == 0)
				tcb->state = TASK_STATE_RUN;
			else
				tcb->delay_ms--;
			break;
		case TASK_STATE_SUSPEND	:	
			break;
		case TASK_STATE_RESUME	:
			tcb->state = TASK_STATE_RUN;
			break;
		default:
			break;
		}
	}
#endif
}

/**
 * \brief  cmd_list_ctask()
 * 		Terminal command list all ctask
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void cmd_list_ctask(void *terminal, token_t *token, int max_token)
{
	ctask_freertos_print_state();
}
	
/**
 * \brief  cmd_ctask_control()
 * 		Terminal command list all ctask
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void cmd_ctask_control(void *terminal, token_t *token, int max_token)
{
	int task_id, delay_ms;

	if (max_token > 1) {
		task_id = atoi(token[1].str);
	
		if (strcmp(token[0].str, "ts") == 0)
			ctask_freertos_ctrl(task_id, TASK_STATE_SUSPEND);
		else if (strcmp(token[0].str, "tr") == 0)
			ctask_freertos_ctrl(task_id, TASK_STATE_RESUME);
		else if (strcmp(token[0].str, "te") == 0)
			ctask_freertos_ctrl(task_id, TASK_STATE_EXIT);
		else if (max_token > 2) {
			delay_ms = atoi(token[2].str);
			if (strcmp(token[0].str, "tz") == 0)
				ctask_freertos_sleep(task_id, delay_ms);
		}
	}
	ctask_freertos_print_state();
}

/**
 * \brief ctask_freertos_fos_register
 *	ctask-os registeration
 * \param task_id: task id
 */
int ctask_freertos_os_register(void)
{
	struct ctask_os_t ctask_freertos_os;

	ctask_freertos_os.create = ctask_freertos_create;
	ctask_freertos_os.enable = ctask_freertos_enable;
	ctask_freertos_os.suspend = ctask_freertos_suspend;
	ctask_freertos_os.resume = ctask_freertos_resume;
	ctask_freertos_os.sleep = ctask_freertos_sleep;
	ctask_freertos_os.exit = ctask_freertos_exit;
	ctask_freertos_os.schedular = ctask_freertos_schedular;
	ctask_freertos_os.shut_down = ctask_freertos_shut_down;
	ctask_freertos_os.get_fnstate = ctask_freertos_get_fnstate;
	ctask_freertos_os.set_fnstate = ctask_freertos_set_fnstate;
	ctask_freertos_os.print_state = ctask_freertos_print_state;

	ctask_freertos_os.semaphore_init = ctask_freertos_semaphore_init;
	ctask_freertos_os.semaphore_take = ctask_freertos_semaphore_take;
	ctask_freertos_os.semaphore_give = ctask_freertos_semaphore_give;

	return ctask_os_register(&ctask_freertos_os);
}
// eof

