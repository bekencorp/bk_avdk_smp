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
* \file		: cmsg_queue.c
* \author	: ravibabu@synopsys.com
* \date		: 23-May-2019
* \brief	: cmsg_queue.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "error.h"
#include "common.h"
#include "cmsg_queue.h"
#include "clog.h"

/*
 *
 * The message queue is implemented over shared memory or static internal
 * memory, with static configurable number of queues & dynamic allocation of size
 * or depth for each queues.
 * The queue memory used to store queue data structure and q_data, multiple task/process
 * or subsytem or application (say task1 and task2 or app1/app2) can send/receive
 * the messages through queue intefaces as shown in figure below.
 * 	No protection required while accessing queues until unless
 * queuse are used as pipes or one direction queues.
 *
 *
 *	     shared_mem base -> 0000 +-----------------+
 *                                   |                 |
 *                                   |                 |
 *  +----+                           | Queue-0 to      |                   +-----------+
 *  |App1|<---msgQ send ------------>| Queue-N         |<-----MsgQ send--->| App2/Task2|
 *  +----+       or  recieve-->      | data structures | <--- or receieve->+-----------+
 *                                   |                 |
 *                              Qdata+-----------------+
 *                                   |                 |
 *                                   | queue data      |
 *                                   |  memory         |
 *                                   |                 |
 *                                   |                 |
 *                                   |                 |
 *                                   |                 |
 *                                   |                 |
 *                                   |                 |
 *                    Max_q_mem_addr +-----------------+
 *
 *
 */
#include "cmsg_queue_cfg.h"

/* Qtyes */
#define MAX_NUM_QTYPES		2
#define QTYPE_INTERNAL_SRAM	0	/* static allocated interna internal queue*/
#define QTYPE_SHARED_MEM	1	/* shared memory queues */

/* queue internal data structure */
typedef struct {
    uint8_t		num;		/* message queue number */
    uint16_t	size;		/* size of message queue */
    uint16_t	read_ptr;	/* queue read pointer */
    uint16_t    	write_ptr;	/* queue write pointer */
    uint8_t		*p_data;	/* pointer to data memory */
    uint32_t	d_offs;		/* data offset within data memory */
    char name[20];
} Queue;

struct cmsg_q_t {
    uint8_t	  valid;		/* 0- queue is invalid, 1-queue is valid */
    uint8_t	  q_type;		/* qtype 1-internal queue, 2-shared memory queue */
    sint8_t	  n_queues;		/* total number of queues */
    uint32_t  memsize;		/* queue data memory size */
    Queue	 queue[MAX_NUM_QUEUES]; /* queue data structure */
    uint8_t q_data[QUEUE_MEM_SIZE]; /* queue data memory */
};

/* globals */
static uint8_t gMsgQInitDone[MAX_NUM_QTYPES] = {0, 0};	/* MsgQueue Init is done or not */
static struct cmsg_q_t gMsgQueue[MAX_NUM_QTYPES];	/* static msg queue structure */
static struct cmsg_q_t *gpMsgQueue[MAX_NUM_QTYPES] = {NULL, NULL}; /* pointer to MsgQueues */
static uint8_t gMsgQInitFlags[MAX_NUM_QTYPES];		/* flags passed to MsgQ init modules */

/*
 * \brief: cmsg_q_init
 *	MsgQueue_module init must be called once during application init
 * \param qt: queue type
 * \param flag: MsgQueue flags
 *	pass flag QUEUE_CTRLF_RESET_ALL to reset the q-memory
 * \param q_mem_addr: address of queue memory
 * \param q_mem_size: queue memory size
 * \returns : ERROR_OPER_FAIL on fail to init
 *	0 on success
 */
static int cmsgq_init(uint8_t qt, uint32_t flags, void *q_mem_addr, uint32_t q_mem_size)
{
    int i;

    /* check MsgQ module already initialized */
    if (gMsgQInitDone[qt])
        return 0;

    if (q_mem_addr == NULL)
        return ERROR_OPER_FAIL;

    /* assign allocated queue memory */
    gpMsgQueue[qt] = q_mem_addr;

    /* initialize the Queue memory only once, incase of
     * shared memory multiple application can use MsgQueue
     * using the same MsgQueue library, so queue data structure
     * must be intialized only once
     */
    cpu_int_enable();
    if (flags & QUEUE_CTRLF_RESET_ALL) {
        for (i = 0; i < q_mem_size; ++i)
            *(unsigned char *)((addr_t)gpMsgQueue[qt] + i) = 0;

        gpMsgQueue[qt]->n_queues = 0;
        gpMsgQueue[qt]->memsize = q_mem_size - (sizeof(Queue) - 16);
        gpMsgQueue[qt]->valid = 1;
        gpMsgQueue[qt]->q_type = qt;
    }
    cpu_int_disable();

    gMsgQInitFlags[qt] = flags;
    gMsgQInitDone[qt] = 1;
    return 0;
}

/*
 * \brief: cmsg_q_init
 *	MsgQueue_module init must be called once during application init
 * \param flag: MsgQueue flags
 *	pass flag QUEUE_CTRLF_RESET_ALL to reset the q-memory
 * \returns : ERROR_OPER_FAIL on fail to init
 *	0 on success
 */
int cmsg_q_module_init(uint32_t flags)
{
    return cmsgq_init(QTYPE_INTERNAL_SRAM, flags, &gMsgQueue[QTYPE_INTERNAL_SRAM],
                      QUEUE_MEM_SIZE);
}

/*
 * \brief: sm_cmsg_q_init
 *	MsgQueue_module init must be called once during application init
 * \param flag: MsgQueue flags
 *	pass flag QUEUE_CTRLF_RESET_ALL to reset the q-memory
 * \param sm_qmem_addr: shared queue-memory address
 * \param qmem_size : szie of shared queue memory
 * \returns : ERROR_OPER_FAIL on fail to init
 *	0 on success
 */
int sm_cmsg_q_module_init(uint32_t flags, void *sm_qmem_addr, uint32_t qmem_size)
{
    /* allocate shared q-mem if NULL is passed*/
    if (sm_qmem_addr && (qmem_size <= sizeof(Queue))) {
        clog_print(CLOG_ERR, "sm queue-mem not allocated or not sufficient\n");
        return MSG_Q_ALLOC_FAILD;
    }

    /* allocate sm_message queue in internal memory if sm_qmem_addr is NULL */
    if (sm_qmem_addr == NULL) {
        sm_qmem_addr = &gMsgQueue[QTYPE_SHARED_MEM];
        qmem_size = QUEUE_MEM_SIZE;
    }

    return cmsgq_init(QTYPE_SHARED_MEM, flags, sm_qmem_addr, qmem_size);
}

/*
 * \brief: cmsg_q_deinit
 *	De-initialize the MsgQueue module
 */
int cmsg_q_deinit(void)
{
    /* TODO : not implemented */
    return 0;
}

/*
 * \brief: sm_cmsg_q_deinit
 *	De-initialize the shared memory MsgQueue module
 */
int sm_cmsg_q_deinit(void)
{
    /* TODO : not implemented */
    return 0;
}

/*
 * \brief: cmsgq_create
 *	MsgQueue_create of length q_size
 * \param qt : Type of queue
 * \param q_size: length of queue or number of elements in queue
 *		MSG_Q_INVALID_QNUM
 *		0 on success
 */
static int cmsgq_create(struct cmsg_q_t *ptr_msg_q, int q_size, char *name)
{
    int q_num;
    int n_queues;
    uint16_t size;
    uint32_t d_offs, memsize;

    /* input parameter check */
    if (ptr_msg_q == NULL) {
        clog_print(CLOG_ERR, "MsgQueue_init not done\n");
        return ERROR_OPER_FAIL;
    }

    if (q_size <= 0)
        return MSG_Q_INVALID_QNUM;

    q_num = ptr_msg_q->n_queues;

    if (q_num >= MAX_NUM_QUEUES)
        return MSG_Q_ALLOC_FAILD;

    q_size++;

    /* allocate queue data offset */
    memsize = ptr_msg_q->memsize;
    if (q_num > 0) {
        size = ptr_msg_q->queue[q_num-1].size;
        d_offs = (uint32_t)(ptr_msg_q->queue[q_num - 1].d_offs);
        d_offs = d_offs + (size + 1);
    } else
        d_offs = 0;

    /* q_size shall not exceed max capacity */
    if (d_offs + q_size > memsize)
        return MSG_Q_ALLOC_FAILD;

    /* initialize the queue data structures */
    ptr_msg_q->queue[q_num].d_offs = d_offs; /* queue data offset */

    /* initialize q_size */
    *(uint16_t *)&ptr_msg_q->queue[q_num].size = q_size;

    /* reset read/write pointers */
    ptr_msg_q->queue[q_num].read_ptr = 0;
    ptr_msg_q->queue[q_num].write_ptr = 0;

    /* assign message queue number */
    n_queues = ptr_msg_q->n_queues + 1;
    ptr_msg_q->n_queues = n_queues;
    ptr_msg_q->queue[q_num].num = q_num;

    /* copy the queue name */
    if (name)
        strncpy(ptr_msg_q->queue[q_num].name, name, 20);

    /* return queue number, for user valid queues are always 1 to N */
    return q_num + 1;
}

/*
 * \brief: cmsg_q_create
 *	create of msg queue of length q_size in internal memory
 * \param q_size: length of queue or number of elements in queue
 * \param name: name of queue
 * returns
 *		MSG_Q_INVALID_QNUM
 *		+ve queue number on success
 */
int cmsg_q_create(int q_size, char *name)
{
    /* create msesage queue of internal type */
    return cmsgq_create(gpMsgQueue[QTYPE_INTERNAL_SRAM], q_size, name);
}

/*
 * \brief: sm_cmsg_q_create
 *	create of msg queue of length q_size in shared memory
 * \param q_size: length of queue or number of elements in queue
 * \param size: size of the queue
 * \returns
 *		MSG_Q_INVALID_QNUM
 *		+ve queue number on success
 */
int sm_cmsg_q_create(int q_size, char *name)
{
    /* create msesage queue of shared memory type */
    return cmsgq_create(gpMsgQueue[QTYPE_SHARED_MEM], q_size, name);
}

/*
 * \brief : cmsgq_send
 *	send the message(buffer) of len bytes on specified q-num
 * \param ptr_msg_q : pointer to msg_q data structure
 * \param msg_q_num : queue number
 * \param buffer: pointer to buffer
 * \param len  : number of bytes to be sent
 * \returns number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 */
static int cmsgq_send(struct cmsg_q_t *ptr_msg_q, uint8_t msg_q_num, void *buffer, uint16_t len)
{
    unsigned int temp;
    uint16_t rd_ptr, wr_ptr, size;
    uint32_t d_offs;
    int i;
    uint8_t *buf = buffer;
    uint8_t q_num = msg_q_num;

    /* parameter check */
    if (q_num <= 0)
        return MSG_Q_INVALID_QNUM;

    if (ptr_msg_q == NULL) {
        clog_print(CLOG_ERR, "MsgQueue_init not done\n");
        return ERROR_OPER_FAIL;
    }

    if (q_num > ptr_msg_q->n_queues)
        return MSG_Q_INVALID_QNUM;

    /* internal queue number starts from 0 to N-1 */
    q_num--;

    cpu_int_disable();

    /* get read/write pointers */
    rd_ptr = ptr_msg_q->queue[q_num].read_ptr;
    wr_ptr = ptr_msg_q->queue[q_num].write_ptr;
    size =  ptr_msg_q->queue[q_num].size;
    d_offs = (uint32_t)(ptr_msg_q->queue[q_num].d_offs);

    /* push data elements to queue */
    for (i = 0; i < len; ++i) {
        temp = (wr_ptr + 1) % (size);
        if (temp != rd_ptr) {
            ptr_msg_q->q_data[d_offs + wr_ptr] = buf[i];
            wr_ptr = temp;
        } else
            break;
    }

    /* return number of elements pushed */
    if (i > 0) {
        ptr_msg_q->queue[q_num].write_ptr = temp;
        cpu_int_enable();
        return i;
    }

    cpu_int_enable();
    return 0;
}

/*
 * \brief : cmsg_q_send
 *	send the message(buffer) of len bytes on specified q-num
 * \param msg_q_num : queue number
 * \param buffer: pointer to buffer
 * \param len  : number of bytes to be sent
 * \returns number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 */
int cmsg_q_send(uint8_t msg_q_num, void *buffer, uint16_t len)
{
    return cmsgq_send(gpMsgQueue[QTYPE_INTERNAL_SRAM], msg_q_num, buffer, len);
}

/*
 * \brief : sm_cmsg_q_send
 *	send the message(buffer) of len bytes on specified q-num in
 *	shared memory
 * \param msg_q_num : queue number
 * \param buffer: pointer to buffer
 * \param len  : number of bytes to be sent
 * \returns number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 */
int sm_cmsg_q_send(uint8_t msg_q_num, void *buffer, uint16_t len)
{
    return cmsgq_send(gpMsgQueue[QTYPE_SHARED_MEM], msg_q_num, buffer, len);
}

/*
 * \brief : cmsgq_receive
 *	recieves atmost len bytes of data into buffer from specified q-num
 * \param ptr_msg_q : pointer to msg queue data structure
 * \param msg_q_num : queue number
 * \param buffer: pointer to buffer
 * \param len  : number of bytes to be sent
 * \param return number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 */
static int cmsgq_receive(struct cmsg_q_t *ptr_msg_q, unsigned char msg_q_num, void *buffer, int len)
{
    uint16_t rd_ptr, wr_ptr, size;
    uint32_t data, d_offs;
    int i;
    uint8_t q_num = msg_q_num;
    uint8_t *buf = buffer;

    /* input parameter check */
    if (q_num <= 0)
        return MSG_Q_INVALID_QNUM;

    if (ptr_msg_q == NULL) {
        clog_print(CLOG_ERR, "MsgQueue_init not done\n");
        return ERROR_OPER_FAIL;
    }

    if (q_num > ptr_msg_q->n_queues)
        return MSG_Q_INVALID_QNUM;

    q_num--;

    /* get read/write pointers */
    cpu_int_disable();
    rd_ptr = ptr_msg_q->queue[q_num].read_ptr;
    wr_ptr = ptr_msg_q->queue[q_num].write_ptr;
    size =  ptr_msg_q->queue[q_num].size;
    d_offs = (uint32_t)(ptr_msg_q->queue[q_num].d_offs);

    /* read the data elements from queue to buffer */
    for (i = 0; i < len; ++i) {
        if (rd_ptr != wr_ptr) {
            data = ptr_msg_q->q_data[d_offs + rd_ptr];
            buf[i] = data;
            rd_ptr = (rd_ptr + 1) % (size);
            clog_print(CLOG_LEVEL10, "%s: q_num(%d) qname(%s) data(%x)\n", __func__,
                       q_num, ptr_msg_q->queue[q_num].name,
                       data);
        } else
            break;
    }

    /* return number of bytes read */
    if (i > 0)
        ptr_msg_q->queue[q_num].read_ptr = rd_ptr;

    cpu_int_enable();
    return i;
}

/*
 * \brief : cmsg_q_receive
 *	recieves atmost len bytes of data into buffer from specified q-num
 * \param msg_q_num : queue number
 * \param buffer: pointer to buffer
 * \param len  : number of bytes to be sent
 * \param return number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 */
int cmsg_q_receive(uint8_t msg_q_num, void *buffer, uint16_t len)
{
    return cmsgq_receive(gpMsgQueue[QTYPE_INTERNAL_SRAM], msg_q_num, buffer, len);
}

/*
 * \brief : sm_cmsg_q_receive
 *	recieves atmost len bytes of data into buffer from specified q-num
 *	in shared memory
 * \param msg_q_num : queue number
 * \param buffer: pointer to buffer
 * \param len  : number of bytes to be sent
 * \param return number of bytes send upon sucess, else return -ve error
 *		MSG_Q_INVALID_QNUM
 */
int sm_cmsg_q_receive(uint8_t msg_q_num, void *buffer, uint16_t len)
{
    return cmsgq_receive(gpMsgQueue[QTYPE_SHARED_MEM], msg_q_num, buffer, len);
}

/*
 * \brief cmsgq_is_empty : check queue is empty
 * \param ptr_msg_q : pointer to msg_q
 * \param msg_q_num : queue number
 * \return true if queue is empty else false
 */
static int cmsgq_is_empty(struct cmsg_q_t *ptr_msg_q, uint8_t msg_q_num)
{
    uint16_t rd_ptr, wr_ptr;
    uint8_t q_num = msg_q_num;

    /* validate input parameters */
    if (q_num <= 0)
        return MSG_Q_INVALID_QNUM;

    if (ptr_msg_q == NULL) {
        clog_print(CLOG_ERR, "MsgQueue_init not done\n");
        return ERROR_OPER_FAIL;
    }

    if (q_num > ptr_msg_q->n_queues)
        return MSG_Q_INVALID_QNUM;

    if (q_num)
        q_num--;

    /* check message queue empty condition */
    rd_ptr = ptr_msg_q->queue[q_num].read_ptr;
    wr_ptr = ptr_msg_q->queue[q_num].write_ptr;
    return (rd_ptr == wr_ptr);
}

/*
 * \brief cmsg_q_is_empty : check queue is empty
 * \param q_num : queue number
 * \return true if queue is empty else false
 */
int cmsg_q_is_empty(uint8_t msg_q_num)
{
    return cmsgq_is_empty(gpMsgQueue[QTYPE_INTERNAL_SRAM], msg_q_num);
}

/*
 * \brief sm_cmsg_q_is_empty : check shared memory queue is empty
 * \param msg_q_num : queue number
 * \return true if queue is empty else false
 */
int sm_cmsg_q_is_empty(uint8_t msg_q_num)
{
    return cmsgq_is_empty(gpMsgQueue[QTYPE_SHARED_MEM], msg_q_num);
}

/*
 * \brief : get_cmsg_q_num
 *	get message queue by name
 * \param qt : queue type (0-internal, 1-shared)
 * \param name : queue name
 * \return queue-num if found else -ve
 */
static int get_cmsgq_num(struct cmsg_q_t *ptr_msg_q, char *name)
{
    int i, q_num;

    /* input parameter check */
    if (!name)
        return ERROR_INVARG;

    if (ptr_msg_q == NULL) {
        clog_print(CLOG_ERR, "MsgQueue_init not done\n");
        return ERROR_OPER_FAIL;
    }

    /* search for queue names and return queue number */
    for (i = 0; i < ptr_msg_q->n_queues; ++i) {
        q_num = ptr_msg_q->queue[i].num + 1;
        if ((strcmp(ptr_msg_q->queue[i].name, name) == 0))
            return (q_num);
    }
    return ERROR_OPER_FAIL;
}

/*
 * \brief : get_cmsg_q_num
 *	get message queue by name
 * \param name : queue name
 * \return queue-num if found else -ve
 */
int get_cmsg_q_num(char *name)
{
    return get_cmsgq_num(gpMsgQueue[QTYPE_INTERNAL_SRAM], name);
}

/*
 * \brief : get_sm_cmsg_q_num
 *	get message queue by name
 * \param name : queue name
 * \return queue-num if found else -ve
 */
int get_sm_cmsg_q_num(char *name)
{
    return get_cmsgq_num(gpMsgQueue[QTYPE_SHARED_MEM], name);
}

/*
 * \brief : cmsgq_print
	print debug status of all MsgQueues
 * \param qt : queue type (0-internal, 1-shared)
 * \param name : queue name
 * \return queue-num if found else -ve
 */
static void cmsgq_print(struct cmsg_q_t *ptr_msg_q)
{
    int i, q_num;

    /* input parameter check */
    if (ptr_msg_q == NULL)
        return ;

    if (ptr_msg_q->n_queues == 0)
        return ;

    /* print status of queues */
    clog_print(CLOG_INFO, "%s\n",
               (ptr_msg_q->q_type == 0) ? "<Internal Queues>" : "<Shared Memory Queues>");
    for (i = 0; i < ptr_msg_q->n_queues; ++i) {
        q_num = ptr_msg_q->queue[i].num + 1;
        clog_print(CLOG_INFO, "q_num(%d) q_size(%d) rd(%d) wr(%d) d_offs(%d) (%s)\n",
                   q_num, //ptr_msg_q->queue[i].num,
                   ptr_msg_q->queue[i].size,
                   ptr_msg_q->queue[i].read_ptr,
                   ptr_msg_q->queue[i].write_ptr,
                   ptr_msg_q->queue[i].d_offs,
                   ptr_msg_q->queue[i].name);
    }
}

/*
 * \brief : cmsgq_print
	print debug status of all MsgQueues
 * \param name : queue name
 * \return queue-num if found else -ve
 */
void cmsg_q_print(void *terminal, token_t *token, int max_token)
{
    /* print both internal/shared memory queue status */
    cmsgq_print(gpMsgQueue[QTYPE_INTERNAL_SRAM]);
    cmsgq_print(gpMsgQueue[QTYPE_SHARED_MEM]);
}
