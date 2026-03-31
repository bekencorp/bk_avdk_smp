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
* \file		: main.c
* \author	: ravibabu@synopsys.com
* \date		: 25-Aug-2019
* \brief	: main.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	25-Aug-2019	ravibabui@synopsys.com  001		dev in progress
*/
#include <stdio.h>
#include "common.h"
#include "dwc_type.h"
#include "ctask.h"
#include "clog.h"

struct semaphore_t odd_lock, even_lock;

/** \brief print_odd
 * 	print the odd numbers only
 * \param task_id: task id
 * \param args_list: args
 * returns 0 once done
 */
int print_odd(uint8_t task_id, void *arg_list)
{
	static int num = 1;

	int state = ctask_get_fnstate(task_id);
	int nxt_state = state;

	clog_print(CLOG_LEVEL7, "%s: state:%d\n", __func__, state);
	do {
		switch (state)
		{
		case XSTATE0:
			/* begin : task functinality */
			clog_print(CLOG_INFO, "%s: %d\n", __func__, num);
			num += 2;
			semaphore_give(&odd_lock, task_id);
			nxt_state = XSTATE1;
			break;
		
		case XSTATE1:
			semaphore_take(&even_lock, task_id);
			nxt_state = XSTATE0;
		default:
			break;
		}
		/* end : task functinality */
	} while (LOOP_FOREVER);

	ctask_set_fnstate(task_id, nxt_state);

	return (num > 100) ? 0 : 1;
}

/** \brief print_even
 * 	print the even numbers only
 * \param task_id: task id
 * \param args_list: args
 * returns 0 once done
 */
int print_even(uint8_t task_id, void *arg_list)
{
	static int num = 2, cnt = 0;

	int state = ctask_get_fnstate(task_id);
	int nxt_state = state;

	clog_print(CLOG_LEVEL7, "%s: state:%d\n", __func__, state);
	do {
		switch (state)
		{
		case XSTATE0:
			semaphore_take(&odd_lock, task_id);
			nxt_state = XSTATE2;
			cnt = 0;
			break;
		case XSTATE1:
			/* begin : task functinality */
			clog_print(CLOG_INFO, "\t\t%s: %d\n", __func__, num);
			num += 2;
			semaphore_give(&even_lock, task_id);
			nxt_state = XSTATE0;
			break;
		case XSTATE2:
			cnt++;
			if (cnt > 5)
				nxt_state = XSTATE1;
		default:
			break;
		}
		/* end : task functinality */
	} while (LOOP_FOREVER);

	ctask_set_fnstate(task_id, nxt_state);

	return (num > 100) ? 0 : 1;
}

/** \brief main
 * 	application main
 */
void main(void)
{
	int task1_id, task2_id, i;

	set_clog_level(CLOG_INFO);

	ctask_no_os_register();
	semaphore_init(&odd_lock, SEMAPHORE_NOT_AVAIL);
	semaphore_init(&even_lock, SEMAPHORE_NOT_AVAIL);
	task1_id = ctask_create("t1", print_odd, 0, 0, 0, 0);
	task2_id = ctask_create("t2", print_even, 0, 0, 0, 0);

	clog_print(CLOG_INFO, "task1-id(%d), task2-id(%d)\n", task1_id, task2_id);
	ctask_enable(task1_id);
	ctask_enable(task2_id);


	for (i = 0; i < 500; ++i)
		ctask_schedular();	
}
