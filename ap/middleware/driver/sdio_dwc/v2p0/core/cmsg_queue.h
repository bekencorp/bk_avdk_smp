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
* \file		: cmsg_queue.h
* \author	: ravibabu@synopsys.com
* \date		: 23-May-2019
* \brief	: cmsg_queue.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/
#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_

#include "dwc_type.h"

/* undef if message queue support not required */
#define QUEUE_CTRLF_RESET_ALL	BIT(0)

/* MsgQ queue error values */
#define MSG_Q_ALLOC_FAILD		-1
#define MSG_Q_INVALID_QNUM		-2

/*
 * There are two types of Message Queues available for user
 *	1) Internal or normal queues implemented in internal ram memory
 *	2) shared-memory Message Queues implemented over shared memory
 *		for inter-task communication or between two application
 *
 * 1) Internal Message Queues API
 *	cmsg_q_module_init(..) 	- message queue module init called only once
 *	cmsg_q_create(..) 	- create a Message queue
 *	cmsg_q_send(..) 	- send a message on specified Message queue
 *	cmsg_q_receive(..) 	- receive a message on specified Message queue
 *	cmsg_q_is_empty(..) 	- check whether Message queue is empty
 *	get_cmsg_q_num(..) 	- get the Message queues based on name of MsgQ.
 *
 * 2) Shared-memory(sm) Message Queues API are
 *	sm_cmsg_q_module_init(..) - message queue module init called only once
 *	sm_cmsg_q_create(..)	  - create a Message queue
 *	sm_cmsg_q_send(..)	  - send a message on specified Message queue
 *	sm_cmsg_q_receive(..) 	  - receive a message on specified Message queue
 *	sm_cmsg_q_is_empty(..) 	  - check whether Message queue is empty
 *	sm_get_cmsg_q_num(..) 	  - get the Message queues based on name of MsgQ.
 *
 * 3) cmsq_q_print - to print the status of both internal or shared-memory MsgQ
 */

/*
 * \brief: cmsg_q_module_init
 *	must be called once during application init
 * param flag: MsgQueue flags
 *	pass flag QUEUE_CTRLF_RESET_ALL to reset the q-memory else pass 0
 * returns : ERROR_OPER_FAIL on fail to init
 *	0 on success
 */
int cmsg_q_module_init(uint32_t flags);


/*
 * \brief: cmsg_q_create
 *	create of Message Queue of name with length q_size(in bytes)
 *	in internal/ram memory
 * param q_size: length of queue in bytes or number of elements in queue
 * pram name: name of the queue
 * returns
 *		MSG_Q_INVALID_QNUM
 *		MSG_Q_INVALID_ARG
 *		+ve queue number on success
 */
int cmsg_q_create(int q_size, char *name);

/*
 * \brief : cmsg_q_send
 *	send the message(buffer) of len bytes on specified q-num
 * param msg_q_num : queue number
 * param buffer: pointer to buffer
 * param len  : number of bytes to be sent
 * returns number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 *		MSG_Q_INVALID_ARG
 */
int cmsg_q_send(uint8_t msg_q_num, void *buffer, uint16_t len);

/*
 * \brief : cmsg_q_receive
 *	recieves atmost len bytes of data into buffer from specified q-num
 * param msg_q_num : queue number
 * param buffer: pointer to buffer
 * param len  : number of bytes to be sent
 * param return number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 *		MSG_Q_INVALID_ARG
 */
int cmsg_q_receive(uint8_t msg_q_num, void *buffer, uint16_t len);

/*
 * \brief cmsg_q_is_empty : check queue is empty
 * param msg_q_num : queue number
 * return true if queue is empty else false
 */
int cmsg_q_is_empty(uint8_t msg_q_num);

/*
 * \brief : get_cmsg_q_num
 *	get message queue by name
 * param name : queue name
 * return queue-num if found else -ve
 */
int get_cmsg_q_num(char *name);

/*
 * \brief: sm_cmsg_q_module_init
 *	m_cmsg_q_module init must be called once during application init
 * param flag: pass flag QUEUE_CTRLF_RESET_ALL to reset the q-memory else pass 0
 * param sm_qmem_addr: shared queue-memory address
 * param qmem_size : szie of shared queue memory
 * returns : ERROR_OPER_FAIL on fail to init
 *	0 on success
 */
int sm_cmsg_q_module_init(uint32_t flags, void *sm_qmem_addr, uint32_t qmem_size);

/*
 * \brief: sm_cmsg_q_create
 *	create of msg queue of length q_size bytes in shared memory
 * param q_size: length of queue or number of elements in queue
 * pram size: size of the queue
 * returns
 *		MSG_Q_INVALID_QNUM
 *		MSG_Q_INVALID_ARG
 *		+ve queue number on success
 */
int sm_cmsg_q_create(int q_size, char *name);

/*
 * \brief : sm_cmsg_q_send
 *	send the message(buffer) of len bytes on specified q-num in
 *	shared memory
 * param msg_q_num : queue number
 * param buffer: pointer to buffer
 * param len  : number of bytes to be sent
 * returns number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 *		MSG_Q_INVALID_ARG
 */
int sm_cmsg_q_send(uint8_t msg_q_num, void *buffer, uint16_t len);

/*
 * \brief : sm_cmsg_q_receive
 *	recieves atmost len bytes of data into buffer from specified q-num
 *	in shared memory
 * param msg_q_num : queue number
 * param buffer: pointer to buffer
 * param len  : number of bytes to be sent
 * param return number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 *		MSG_Q_INVALID_ARG
 */
int sm_cmsg_q_receive(uint8_t msg_q_num, void *buffer, uint16_t len);


/*
 * \brief sm_cmsg_q_is_empty : check shared memory queue is empty
 * param msg_q_num : queue number
 * return true if queue is empty else false
 */
int sm_cmsg_q_is_empty(uint8_t msg_q_num);

/*
 * \brief : get_sm_cmsg_q_num
 *	get message queue by name
 * param name : queue name
 * return queue-num if found else -ve
 */
int get_sm_cmsg_q_num(char *name);

/*
 * \brief : cmsgq_print
	print debug status of all MsgQueues
 * param name : queue name
 * return queue-num if found else -ve
 */
void cmsg_q_print(void *terminal, token_t *token, int max_token);

#endif

