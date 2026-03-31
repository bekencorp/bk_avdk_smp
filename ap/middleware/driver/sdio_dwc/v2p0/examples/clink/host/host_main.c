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
* \file		: host_main.c
* \author	: ravibabu@synopsys.com
* \date		: 24-Aug-2019
* \brief	: host_main.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	24-Aug-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "dwc_type.h"
#include "ctask.h"
#include "clink.h"
#include "terminal.h"
#include "hal.h"
#include "clog.h"
#include "cwatch.h"
#include "cmsg_queue.h"

#define PKT_HDR_CMD	0xA1
#define PKT_HDR_RESP	0xA2
#define PKT_HDR_DATA	0xA3

/* globals */
uint32_t loop_cnt = 0;
struct notify_event_t host_rx_notify;
struct clink_t *clink_host, *clink_device;
uint8_t host_packet[256];
char prn_buf[512];
int msg_id = 0;
struct msg_desc_t msg;

/* extern */
extern void *os_sm_alloc(uint16_t sm_id, uint32_t size_bytes, int *shm_id);

/**
 * \brief send_packet
 * 	send the packet with specific header type, with payload buffer & length
 *	packet format: 
 *	-----------------------------------------------------------
 *	offset	len	description/data
 *	-----------------------------------------------------------
 *	0	1	PKT_HDR_CMD(0xA1), PKT_DATA(0XA2), PKT_RESP(0XA3)
 *	1	2       length of packet 
 *	3	Nbytes	Nbytes data or payload
 *	------------------------------------------------------------
 * \param clink: pointer to clink
 * \param pkt_type: PKT_HDR_CMD(0xA1), PKT_DATA(0XA2), PKT_RESP(0XA3)
 * \param buffer: payload data
 * \param xfer_len: transfer payload length
 * returns the number of bytes sent over the link
 */
int send_packet(struct clink_t *link, uint8_t pkt_type, uint8_t *buffer, uint16_t xfer_len)
{
	struct msg_desc_t msg;

	/* form the packet */
	host_packet[0] = pkt_type;
	*(uint16_t *)&host_packet[1] = xfer_len;
	memcpy(&host_packet[3], buffer, xfer_len);

	msg.length = xfer_len + 3;
        msg.buffer = &host_packet[0];
	return link->put_msg(link, &msg);
}


/**
 * \brief host_rx_handler
 *	host rx handler when any packet received from device
 * \param priv_data:  handler object
 * returns 0 
 */
int host_rx_handler(void *priv_data)
{
	struct clink_t *link = (struct clink_t *)priv_data;
	uint8_t *ptr;
	int i, rxlen, txlen;

        msg.buffer = host_packet;
        msg.length = sizeof(host_packet);
	msg.status = 0;
        if ((rxlen = link->get_msg(link, &msg)) == 0)
                return 0;

        ptr = msg.buffer;
	sprintf(prn_buf, "recived pkt[%x] len(%d)",
			ptr[0], *(uint16_t *)&ptr[1]);

        clog_print(CLOG_LEVEL3, "%s", prn_buf); 
	ptr[rxlen] = 0;
	clog_print(CLOG_LEVEL3, "%s\n", &ptr[3]);

	sprintf(prn_buf, "message from host msgid(%d).", msg_id);
	send_packet(link, PKT_HDR_CMD, prn_buf, strlen(prn_buf));
	msg_id++;
	loop_cnt++;

	return 0;
}

/* 
 * \brief main
 * 	sample example to demonstare clink usage
 *	Description: This example program demonstrates the how to
 *	send/receive message between host/device application or
 *	client/server application model.
 *	This is host example implemenation, does the follwoing
 *		- initialize the clog level and output to registerd
 *			terminals
 *		- initialize the os
 *		- initialize the terminal-core module (one time)
 *		- create stdio based terminal (refer stdio_term.c
 *		- initialze the cwatch to view run time varibles
 *
 *		- creates host clink and attach rx-hander to handle
 *		the packets from device, also sends the initial 
 *		message packet to device.
 */ 
int main(int argc, char *argv[])
{
	int i, level = 2, txlen;
	void *sm_queue_addr = NULL;
	uint32_t sm_qmem_size, flags;
	int shmid;

	if (argc > 1)
		level = atoi(argv[1]);

	set_clog_level(level);
	set_clog_output(CLOG_TERMINAL);
	clog_print(CLOG_INFO, "standard-io terminal\n\n");

	flags = QUEUE_CTRLF_RESET_ALL;
	sm_qmem_size = 5 * 1024;
	sm_queue_addr = os_sm_alloc(QUEUE_SM_ID, sm_qmem_size, &shmid);
	if ((void *)sm_queue_addr == (void *)-1) {
		clog_print(CLOG_ERR, "shared memory allocation failed\n");
		return NULL;
	}

	/* cmsg_q module init */
	cmsg_q_module_init(flags);

	/* shared msg_q module init */
	sm_cmsg_q_module_init(flags, sm_queue_addr, sm_qmem_size);

	/* initialize ctask module */
#ifdef CONFIG_CTASK_LINUX
	ctask_linux_os_register();
#else
	/* register ctask function based schedular */
	ctask_no_os_register();
#endif
	/* initialize the terminal core driver once */
	terminal_core_init();

	/* create the sdio based terminal */
	stdio_terminal_init();

	/*************************** CLINK example begin ****************************/
	/* clink module init */
	clink_init();

	/* The host application will create clink by name "host" 
	 * and device application will get/search the host link by
	 *  its name "host" and get attached to its device clink.
	 *	[host-side-init] 
	 *		clink_host = create_clink("host", 540);
	 *	[device-side-init]
	 *		clink_host = clink_get_by_name("host");
	 *		clink_device = create_clink("dev", 0);
	 *		connect_link(clink_host, clink_device);
	 */
	clink_host = create_clink("host", 1200);
	if (clink_host == NULL) {
		printf("unable to create clink host\n");
		exit(0);
	}

	host_rx_notify.priv_data = clink_host;
	host_rx_notify.event_handler = host_rx_handler;
	clink_set_rx_notify(clink_host, &host_rx_notify);
	
	/********************** CLINK example end **********************/

	/* cwatch module init */
	cwatch_init();
	terminal_register_cmd("ql", cmsg_q_print);
	terminal_register_cmd("cl", print_clink_cnx);

	add_to_cwatch("loop_cnt", &loop_cnt, XUINT32);

	sprintf(prn_buf, "sending msgid(%d).", msg_id);
	send_packet(clink_host, PKT_HDR_CMD, prn_buf, strlen(prn_buf));
	msg_id++;

	/* run the schedular in loop */
	while (1) {
		ctask_schedular();
		usleep(100);
	}
	clog_print(CLOG_INFO, "\n");
}
