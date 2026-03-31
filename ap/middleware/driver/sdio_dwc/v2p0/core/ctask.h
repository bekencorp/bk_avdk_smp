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
* \file		: ctask.h
* \author	: ravibabu@synopsys.com
* \date		: 01-July-2019
* \brief	: ctask.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _CTASK_H_
#define _CTASK_H_

#include "common.h"
#include "dwc_type.h"
#include "csem.h"
#include "core_cfg.h"

#ifndef MAX_NUM_TASKS
#define MAX_NUM_TASKS	4
#endif

#define MAX_TASK_ARGS	4
#define LOOP_FOREVER	0

#define TASK_STATE_INIT		0
#define TASK_STATE_RUN		1
#define TASK_STATE_SLEEP	2
#define TASK_STATE_SUSPEND	3
#define TASK_STATE_RESUME	4
#define TASK_STATE_EXIT		5

/* ctask_os_t structure wrapper methods */
struct ctask_os_t {
    int (*create)(char *name, int (*fn)(uint8_t task_id, void *task_pdata),
                  void *task_pdata, uint32_t arg2, uint32_t arg3, uint32_t arg4);
    int (*enable)(uint8_t task_id);
    int (*suspend)(uint8_t task_id);
    int (*resume)(uint8_t task_id);
    int (*sleep)(uint8_t task_id, uint32_t delay_ms);
    void (*shut_down)(void);
    int (*exit)(uint8_t task_id);
    void (*schedular)(void);
    int (*get_fnstate)(uint8_t task_id);
    int (*set_fnstate)(uint8_t task_id, uint8_t nxt_state);
    void (*print_state)(void);

    void (*semaphore_init)(struct semaphore_t *sem, uint8_t val);
    int (*semaphore_take)(struct semaphore_t *sem, uint8_t task_id);
    int (*semaphore_give)(struct semaphore_t *sem, uint8_t task_id);
};

/* ctask APIs */
/**
 * \brief ctask_create
 *	create the task and initialized to init state
 * \param fn: task function
 * \param argx: argument to function (maximum of 4 user defined arguments to task)
 * returns the unique task_id
 */
int ctask_create(char *name, int (*fn)(uint8_t task_id, void *task_pdata),
                 void *task_pdata, uint32_t arg2, uint32_t arg3, uint32_t arg4);

/**
 * \brief ctask_enable
 *	enable the task to run state
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_enable(uint8_t task_id);

/**
 * \brief ctask_suspend
 *	suspend the task
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_suspend(uint8_t task_id);

/**
 * \brief ctask_resume
 *	resume the task
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_resume(uint8_t task_id);
/**
 * \brief ctask_sleep
 *	puts the task to sleep state
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_sleep(uint8_t task_id, uint32_t delay_ms);
/**
 * \brief ctask_exit
 *	exit the task from scheduling
 * \param task_id: task id
 * returns 0 on success or error
 */
int ctask_exit(uint8_t task_id);
/**
 * \brief ctask_schedular
 *	schedules/invokes all ready tasks
 * \param task_id: task id
 */
void ctask_schedular(void);
/**
 * \brief ctask_shut_down
 *	shuts down all tasks
 * \param task_id: task id
 */
void ctask_shut_down(void);

/* helper functions */
/**
 * \brief ctask_get_fnstate
 *	gets the state of task function of task_id
 * \param task_id: task id
 * returns the state of task-id
 */
int ctask_get_fnstate(uint8_t task_id);
/**
 * \brief ctask_set_fnstate
 *	set the state of task function of task_id
 * \param task_id: task id
 * returns the state of task-id
 */
int ctask_set_fnstate(uint8_t task_id, uint8_t nxt_state);
/**
 * \brief ctask_os_register
 *	register ctask os methods
 * \param os: poiter to ctask_os APIs
 */
int ctask_os_register(struct ctask_os_t *os);

#ifdef CONFIG_CTASK_LINUX
extern int ctask_linux_os_register(void);
#elif CONFIG_CTASK_FREERTOS
extern int ctask_freertos_os_register(void);
#else
extern int ctask_no_os_register(void);
#endif

#endif /* _CTASK_H_ */
