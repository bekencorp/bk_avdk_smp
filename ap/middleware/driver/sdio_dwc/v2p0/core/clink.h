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
* \file		: clink.h
* \author	: ravibabu@synopsys.com
* \date		: 12-Aug-2019
* \brief	: clink.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _MSG_SOCKET_H_
#define _MSG_SOCKET_H_

#define MAX_MSG_SOCKETS		4
#define MAX_MSG_SOCKETS_CNX	4 //(MAX_MSG_SOCKETS/2+1)

struct notify_event_t {
    void *priv_data;
    int (*event_handler)(void *priv_data);
};

struct clink_t;
struct clink_t {
    sint8_t tx_msg_q;
    sint8_t rx_msg_q;
    int (*put_msg)(struct clink_t *clink, struct msg_desc_t *msg);
    int (*get_msg)(struct clink_t *clink, struct msg_desc_t *msg);
    uint8_t allocated;
    uint8_t id;
    uint8_t name[24];
    struct notify_event_t *notify_handler;
    struct clink_t *connected_to;
};

struct clink_cnx_t {
    struct clink_t *src;
    struct clink_t *dst;
    uint8_t valid;
};


/**
 * \brief create_clink
 *	create a clink interface, with tx/rx queue length as specified
 *	by queue_depth. return pointer to new clink created.
 * \param name: name of link to be created
 * \param queeu_depth: depth of rx/tx queue of link
 * returns: pointer to new clink structure, NULL on fail to create.
 */
struct clink_t *create_clink(char *name, uint16_t queue_depth);

/**
 * \brief create_clink by queue
 *	create clink based on existing tx and rx queue names
 *	This is useful, if clink is already created by host/master/server
 *	application, the device/slave/client application can just hook to
 *	existing clink by get providing the tx and rx queue names.
 * \param name: name of link to be created
 * \param queeu_depth: depth of rx/tx queue of link
 */
struct clink_t *create_clink_by_queue(char *clink_name, char *tx_qname, char *rx_qname);

/**
 * \brief set_rx_notify_handler
 *	set rx notification handler to clink
 * \param clink: pointer to clink
 * \param rx_msg_notify: pointer to rx notification
 * returns 0 on success, -ve on failure
 */
int clink_set_rx_notify(struct clink_t *clink, struct notify_event_t *rx_msg_notify);

/**
 * \brief connect_link
 *	etablishes the connection between two soruce & destination links
 * \param clink: src link
 * \param clink: dst link
 * returns 0 on success, -ve on failure
 */
int connect_link(struct clink_t *src, struct clink_t *dst);

/**
 * \brief clink_get_by_name
 *	get the clink by name
 * \param clink_name: name of clink
 * return pointer to clink if found else NULL
 */
struct clink_t *clink_get_by_name(char *clink_name);

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
                    struct clink_t **host_link, struct clink_t **dev_link);
/**
 * \brief clink_init
 *	initialize the clink data structure and enables clink core task
 * \param: none
 */
void clink_init(void);
void print_clink_cnx(void *terminal, token_t *token, int max_token);

#endif
