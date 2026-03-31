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
* \file		: clink.c
* \author	: ravibabu@synopsys.com
* \date		: 12-Aug-2019
* \brief	: clink.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include "common.h"
#include "dwc_type.h"
#include "ctask.h"
#include "error.h"
#include "cmsg_queue.h"
#include <string.h>
#include "clink.h"
#include "clog.h"
#include "os/str.h"

/* global static data structure */
static struct clink_t clink_list[MAX_MSG_SOCKETS];
static struct clink_cnx_t link_cnx_list[MAX_MSG_SOCKETS_CNX];
static uint8_t clink_init_done = 0;
static int clink_tid = -1, max_num_links_cnx = 0;
static uint8_t clink_dbg_level = 0;

/**
 * \brief clink_send
 *	send the message through clink
 * \param clink: pointer to clink
 * \param msg: pointer to msg_desc structure
 * returns the number of bytes sent over the link
 */
int clink_send_msg(struct clink_t *clink, struct msg_desc_t *msg)
{
    int retval;

    /* check for valid clink & message descriptor */
    if (!(clink && msg && msg->buffer && msg->length))
        return ERROR_INVARG;

    /* send the message over the clink */
    retval = sm_cmsg_q_send(clink->tx_msg_q, msg->buffer, msg->length);
    clog_print(clink_dbg_level, "%ssent msg(%x) to clink%d.%s\n",
               retval ? "" : "failed to ", msg, clink->id, clink->name);

    msg->status = retval;

    return retval;
}

/**
 * \brief clink_get
 *	get the msg from link if there any new msg
 * \param clink: pointer to clink
 * \param msg: pointer to msg_desc_t structure
 * returns: 1 if msg is available, 0 otherwise
 */
int clink_get_msg(struct clink_t *clink, struct msg_desc_t *msg)
{
    int retval;

    /* check for valid clink & message descriptor */
    if (!(clink && msg && msg->buffer && msg->length))
        return ERROR_INVARG;

    /* receieve the message from clink */
    retval = sm_cmsg_q_receive(clink->rx_msg_q, msg->buffer, msg->length);
    clog_print(clink_dbg_level, "%smsg(%x) received from clink%d.%s\n",
               retval ? "" : "no ", msg, clink->id, clink->name);

    msg->status = retval;

    return retval;
}

/**
 * \brief alloc_clink
 * 	allocate clink data structure and initialize
 * \param name: clink name
 * returns: pointer to new clink structure, NULL on fail to create.
 */
struct clink_t *alloc_clink(char *name)
{
    int i;

    /* allocate free clink descriptor */
    for (i = 0; i < MAX_MSG_SOCKETS; ++i)
        if (clink_list[i].allocated == 0)
            break;

    if (i >= MAX_MSG_SOCKETS)
        return NULL;

    /* clear clink data structure */
    memset(&clink_list[i], 0, sizeof(clink_list[i]));

    /* initializ other clink members */
    if (name) {
        int src_len = os_strlen(name);
        int dest_len = sizeof(clink_list[i].name);
        int cnt = MIN(src_len, dest_len);

        strncpy((void *)clink_list[i].name, name, cnt - 1);
    }
    clink_list[i].id = i;
    clink_list[i].put_msg = clink_send_msg;
    clink_list[i].get_msg = clink_get_msg;

    return &clink_list[i];
}

/**
 * \brief free_clink
 * 	free clink data structure
 * \param clink: pointer to clink to be released
 * returns: none
 */
void free_clink(struct clink_t *clink)
{
    /* free clink descriptor */
    clink->allocated = 0;
}

/**
 * \brief create_clink
 *	create a clink interface, with tx/rx queue length as specified
 *	by queue_depth. return pointer to new clink created.
 * \param name: name of link to be created
 * \param queeu_depth: depth of rx/tx queue of link
 * returns: pointer to new clink structure, NULL on fail to create.
 */
struct clink_t *create_clink(char *name, uint16_t queue_depth)
{
    struct clink_t *clink;
    char qname[40];

    /* allocate clink from free pool */
    clink = alloc_clink(name);
    if (clink == NULL)
        return NULL;

    /* create tx/rx queue for bi-direction transfer over clink */
    if (queue_depth) {
        sprintf(qname, "%s.txQ", name);
        clink->tx_msg_q = sm_cmsg_q_create(queue_depth, qname);
        if (clink->tx_msg_q < 0)
            return 0;

        sprintf(qname, "%s.rxQ", name);
        clink_list->rx_msg_q = sm_cmsg_q_create(queue_depth, qname);
        if (clink_list->rx_msg_q < 0)
            return 0;
    }

    /* initializ other clink members */
    clink->allocated = 1;

    clog_print(clink_dbg_level, "clink%d %s created\n", clink->id, name);
    return clink;
}

/**
 * \brief create_clink by queue
 *	create clink based on existing tx and rx queue names
 *	This is useful, if clink is already created by host/master/server
 *	application, the device/slave/client application can just hook to
 *	existing clink by get providing the tx and rx queue names.
 * \param name: name of link to be created
 * \param queeu_depth: depth of rx/tx queue of link
 */
struct clink_t *create_clink_by_queue(char *clink_name, char *tx_qname, char *rx_qname)
{
    struct clink_t *clink;
    int rx_q, tx_q;

    if (tx_qname == NULL || rx_qname == NULL)
        return NULL;

    /* get existing tx queue */
    tx_q = get_sm_cmsg_q_num(tx_qname);
    if (tx_q <= 0)
        return NULL;

    /* get existing tx queue */
    rx_q = get_sm_cmsg_q_num(rx_qname);
    if (rx_q <= 0)
        return NULL;

    /* allocate clink from free pool */
    clink = alloc_clink(clink_name);
    if (clink == NULL)
        return NULL;

    /* initialize the clink members */
    clink->tx_msg_q = tx_q;
    clink->rx_msg_q = rx_q;
    clink->allocated = 1;

    clog_print(clink_dbg_level, "clink%d %s created\n", clink->id, clink_name);
    clog_print(clink_dbg_level, "clink%d connected to %s Host.txQ(%d) \n", clink->id,
               tx_qname, clink->tx_msg_q);
    clog_print(clink_dbg_level, "clink%d connected to %s Host.rxQ(%d) \n", clink->id,
               rx_qname, clink->rx_msg_q);

    return clink;
}

/**
 * \brief set_rx_notify_handler
 *	set rx notification handler to clink
 * \param clink: pointer to clink
 * \param rx_msg_notify: pointer to rx notification
 * returns 0 on success, -ve on failure
 */
int clink_set_rx_notify(struct clink_t * clink, struct notify_event_t *rx_msg_notify)
{
    if (clink == 0)
        return ERROR_INVARG;

    if (clink->allocated == 0)
        return ERROR_INVALID_SOCKET;

    clink->notify_handler = rx_msg_notify;

    return 0;
}

/**
 * \brief connect_link
 *	etablishes the connection between two soruce & destination links
 * \param clink: src link
 * \param clink: dst link
 * returns 0 on success, -ve on failure
 */
int connect_link(struct clink_t *src, struct clink_t *dst)
{
    int i;

    /* check for valid src/dst clink */
    if (src->allocated && src->connected_to)
        return ERROR_INVALID_SOCKET;
    if (dst->allocated && dst->connected_to)
        return ERROR_INVALID_SOCKET;

    /* allocate clink connection object from free pool */
    for (i = 0; i < MAX_MSG_SOCKETS_CNX; ++i)
        if (link_cnx_list[i].valid == 0)
            break;

    if (i >= MAX_MSG_SOCKETS_CNX)
        return ERR_NORESOURCE;

    /* initialize the clink connection members */
    if (src->tx_msg_q && dst->rx_msg_q == 0)
        dst->rx_msg_q = src->tx_msg_q;
    if (src->rx_msg_q && dst->tx_msg_q == 0)
        dst->tx_msg_q = src->rx_msg_q;

    link_cnx_list[i].src = src;
    link_cnx_list[i].dst = dst;
    src->connected_to = dst;
    dst->connected_to = src;

    link_cnx_list[i].valid = 1;

    max_num_links_cnx++;
    clog_print(clink_dbg_level, "established link bw clink%d.%s clink%d.%s\n",
               src->id, src->name, dst->id, dst->name);

    return 0;
}
/**
 * \brief clink_get_by_name
 *	get the clink by name
 * \param clink_name: name of clink
 * return pointer to clink if found else NULL
 */
struct clink_t *clink_get_by_name(char *clink_name)
{
    int i;
    char rxq_name[40], txq_name[40];

    if (!clink_name)
        return NULL;

    for (i = 0; i < MAX_MSG_SOCKETS; ++i) {
        if (clink_list[i].allocated == 0)
            continue;
        if (strcmp((void *)clink_list[i].name, clink_name) == 0)
            return &clink_list[i];
    }

    sprintf(txq_name, "%s.txQ", clink_name);
    sprintf(rxq_name, "%s.rxQ", clink_name);

    return create_clink_by_queue(clink_name, txq_name, rxq_name);
}

/**
 * \brief connect_to_host
 *	etablishes the connection between two host/src & device/destination links
 *	by name	and returns the handles to src/dst clinks
 * \param clink: host link name
 * \param clink: dev link name
 * \param clink: host link
 * \param clink: dev link
 * returns 0 on success after connect_link else error
 */
int connect_to_host(char *host_link_name, char *dev_link_name,
                    struct clink_t **host_link, struct clink_t **dev_link)
{
    if (!host_link_name || !dev_link_name || !host_link || !dev_link)
        return ERROR_INVARG;

    *host_link = clink_get_by_name(host_link_name);
    if (*host_link == NULL) {
        clog_print(CLOG_ERR, "%s clink not created or found\n", host_link_name);
        return ERROR_OPER_FAIL;
    }

    *dev_link = create_clink(dev_link_name, 0);
    if (*dev_link == NULL) {
        clog_print(CLOG_ERR, "%s clink not created or found\n", dev_link_name);
        return ERROR_OPER_FAIL;
    }

    return connect_link(*host_link, *dev_link);
}
/**
 * \brief print_clink_cnx
 *	print all clink connection interfaces
 * \param none
 * returns none
 */
void print_clink_cnx(void *terminal, token_t *token, int max_token)
{
    struct clink_cnx_t *link_cnx;
    int i;

    clog_print(CLOG_INFO, "src-link\tTxQ\tRxQ\tdst-link\tTxQ\tRxQ\n");
    clog_print(CLOG_INFO, "-----------------------------------------------------------------\n");
    for (i = 0; i < max_num_links_cnx; ++i)
    {
        link_cnx = &link_cnx_list[i];

        if (link_cnx->valid == 0)
            continue;

        clog_print(CLOG_INFO, "%s\t%d\t%d\t%s\t%d\t%d",
                   link_cnx->src->name, link_cnx->src->tx_msg_q, link_cnx->src->rx_msg_q,
                   link_cnx->dst->name, link_cnx->dst->tx_msg_q, link_cnx->dst->rx_msg_q);
    }
}

/**
 * \brief clink_core_task
 *	connects the tx/rx queues in the link interface
 *	transfers the messages between rx and tx queues
 * 	of srouce and destlination link.
 * \param task_id: task id
 * \param data:	   private data
 */
int clink_core_task(uint8_t task_id, void *priv_data)
{
    struct clink_cnx_t *link_cnx;
    struct notify_event_t *rx_notify;
    struct clink_t *link;
    unsigned char msg;
    int i;

    /* iterate over number of clink connection */
    for (i = 0; i < max_num_links_cnx; ++i)
    {
        /* get active clink connection instance */
        link_cnx = &link_cnx_list[i];

        if (link_cnx->valid == 0)
            return 1;

        /* transfer msg from source to destination link */
        if (link_cnx->dst->rx_msg_q != link_cnx->src->tx_msg_q) {
            if (sm_cmsg_q_receive(link_cnx->src->tx_msg_q, &msg, 1))
                sm_cmsg_q_send(link_cnx->dst->rx_msg_q, &msg, 1);
        }

        /* transfer the message from destination to source link */
        if (link_cnx->src->rx_msg_q != link_cnx->dst->tx_msg_q) {
            if (sm_cmsg_q_receive(link_cnx->dst->tx_msg_q, &msg, 1))
                sm_cmsg_q_send(link_cnx->src->rx_msg_q, &msg, 1);
        }
    }

    /* handle all clink rx notification */
    for (i = 0; i < MAX_MSG_SOCKETS; ++i) {
        if (clink_list[i].allocated == 0)
            continue;

        link = &clink_list[i];

        /* check any rx notification to be handled for clink*/
        if (!sm_cmsg_q_is_empty(link->rx_msg_q)) {
            rx_notify = link->notify_handler;
            if (rx_notify && rx_notify->event_handler) {
                clog_print(CLOG_LEVEL6, "rx-event from clink%d.%s\n",
                           link->id, link->name);
                rx_notify->event_handler(rx_notify->priv_data);
            }
        }

    }
    /* always return 1 for task to be executed in continuous loop */
    return 1;
}

/**
 * \brief clink_init
 *	initialize the clink data structure and enables clink core task
 * \param: none
 */
void clink_init(void)
{
    int i;

    if (clink_init_done)
        return ;

    for (i = 0; i < MAX_MSG_SOCKETS; ++i)
        clink_list[i].allocated = 0;

    for (i = 0; i < MAX_MSG_SOCKETS_CNX; ++i)
        link_cnx_list[i].valid = 0;

    max_num_links_cnx = 0;
    clink_tid = ctask_create("clink-core", clink_core_task, 0, 0, 0, 0);
    ctask_enable(clink_tid);

    clink_dbg_level = CLOG_LEVEL5;
    clink_init_done = 1;

    clog_print(clink_dbg_level, "clink init done\n");
}
