/**
 ***************************************************************************************
 * @file     svdpi_main.c
 * @author   ravibabu@synopsys.com
 * @version  00.00.01
 * @date     12 Aug 2019
 * @brief    svdpi_main source
*
 ***************************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright(C) 2019 Synopsys, Inc. All rights reserved</center></h2>
 *
 * This Synopsys software and all associated documentation are proprietary to
 * Synopsys, Inc. And may only be used pursuant to the terms and conditions of
 * a written license agreement with Synopsys, Inc. All other use, reproduction,
 * modification, or distribution of the Synopsys software or the associated
 * documentation is strictly prohibited.
 ***************************************************************************************
 @verbatim
   REVISION HISTORY:
   Version    Date         Author       	  Change Id     Description
   00.00.00   2019-08-12   ravibabu@synopsys.com    -           dev in progress
 @endverbatim
 */

#include <stdio.h>
#include "dwc_type.h"
#include "clink.h"
#include "ctask.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#define PKT_HDR_CMD	0xA1
#define PKT_HDR_RESP	0xA2
#define PKT_HDR_DATA	0xA3

#define CMD_REG_READ	1
#define CMD_REG_WRITE	2

/*
 * \brief: message descriptor structure
 * buffer : pointer to buffer
 * length : length of buffer in bytes
 * xferlen : transferred or received length of buffer in bytes
 * status : user dependent field
 */
struct msg_desc_t {
	void *buffer;
	int length;
	int xferlen;
	int status;
};

/*#include "acc_user.h"
#include "vcs_acc_user.h"
#include "svdpi.h"
*/
void *sm_queue_addr;
uint32_t sm_qmem_size;
int shmid;

uint8_t svdpi_init_done = 0;
struct notify_event_t mshc_rx_notify, msdc_rx_notify;
struct clink_t *clink_mshc, *clink_msdc;
uint8_t mshc_packet[16], prn_buf[32];
uint8_t msdc_packet[16];
struct msg_desc_t msgh, msgd;
int msgh_id = 0, msgd_id;

/**
 * @brief send_packet
 * 	send the packet with specific header type, with payload buffer & length
 *	packet format: 
 *	-----------------------------------------------------------
 *	offset	len	description/data
 *	-----------------------------------------------------------
 *	0	1	PKT_HDR_CMD(0xA1), PKT_DATA(0XA2), PKT_RESP(0XA3)
 *	1	2       length of packet 
 *	3	Nbytes	Nbytes data or payload
 *	------------------------------------------------------------
 * @param clink: pointer to clink
 * @param pkt_type: PKT_HDR_CMD(0xA1), PKT_DATA(0XA2), PKT_RESP(0XA3)
 * @param buffer: payload data
 * @param xfer_len: transfer payload length
 * returns the number of bytes sent over the link
 */
int send_packet(struct clink_t *link, uint8_t pkt_type, uint8_t *buffer, uint16_t xfer_len, uint8_t *pkt)
{
	struct msg_desc_t msg;

	/* form the packet */
	pkt[0] = pkt_type;
	*(uint16_t *)&pkt[1] = xfer_len;
	memcpy(&pkt[3], buffer, xfer_len);

	msg.length = xfer_len + 3;
        msg.buffer = &pkt[0];
	return link->put_msg(link, &msg);
}
/*
 * \brief: sm_alloc
 *	shared memory allocate allocate queue memory
 * param flags: MsgQueue flags
 *	 use QUEUE_ALLOC_SM flag for shared memory
 * param size_bytes: queue memory size in bytes
 * returns : address to MsgQueue Memory or NULL
 */
void *sm_alloc(uint16_t sm_id, uint32_t size_bytes, int *shm_id)
{
	int shmid;

	shmid = shmget((key_t)sm_id, size_bytes, 0666 | IPC_CREAT);
	if (shmid == -1) {
		clog_print(CLOG_ERR, "shmget failed to allocate %d bytes\n", size_bytes);
		return NULL;
	}

	*shm_id = shmid;
	return shmat(shmid, (void *)0, 0);
}

void sv_write_val(void *addr, uint32_t val)
{
}

void sv_read_val(void *addr, uint32_t *val)
{
}

/**
 * @brief mshc_rx_handler
 *	mshc rx handler when any packet received from device
 * @param priv_data:  handler object
 * returns 0 
 */
int mshc_rx_handler(void *priv_data)
{
	struct clink_t *link = (struct clink_t *)priv_data;
	uint8_t *ptr;
	int i, rxlen, txlen;
	uint32_t cmd, addr, data;

        msgh.buffer = mshc_packet;
        msgh.length = sizeof(mshc_packet);
	msgh.status = 0;
        if ((rxlen = link->get_msg(link, &msgh)) == 0)
                return 0;

        ptr = msgh.buffer;
	sprintf(prn_buf, "recived pkt[%x] len(%d)",
			ptr[0], *(uint16_t *)&ptr[1]);

	cmd = ptr[3];
	addr = *(uint32_t *)&ptr[4];
	data = 0;

        clog_print(CLOG_LEVEL3, "cmd:%s addr=%x", (cmd == CMD_REG_WRITE) ? "write" : "read", addr); 
	if (cmd == CMD_REG_WRITE) {
		data = *(uint32_t *)&ptr[8];
		clog_print(CLOG_LEVEL3, "data=%x", data);
	}
	clog_print(CLOG_LEVEL3,"\n");

	if (cmd == CMD_REG_WRITE)
		sv_write_val(addr, data);
	else
		sv_read_val(addr, &data);	

	prn_buf[0] = cmd | 0x80;
	*(uint32_t *)&prb_buf[1] = data;

	send_packet(link, PKT_HDR_RESP, prn_buf, 5, mshc_packet);
	msgh_id++;
}

/**
 * @brief msdc_rx_handler
 *	mshc rx handler when any packet received from device
 * @param priv_data:  handler object
 * returns 0 
 */
int msdc_rx_handler(void *priv_data)
{
	struct clink_t *link = (struct clink_t *)priv_data;
	uint8_t *ptr;
	int i, rxlen, txlen;
	uint32_t cmd, addr, data;

        msgd.buffer = msdc_packet;
        msgd.length = sizeof(msdc_packet);
	msgd.status = 0;
        if ((rxlen = link->get_msg(link, &msgd)) == 0)
                return 0;

        ptr = msgd.buffer;
	sprintf(prn_buf, "recived pkt[%x] len(%d)",
			ptr[0], *(uint16_t *)&ptr[1]);

	cmd = ptr[3];
	addr = *(uint32_t *)&ptr[4];
	data = 0;

        clog_print(CLOG_LEVEL3, "cmd:%s addr=%x", (cmd == CMD_REG_WRITE) ? "write" : "read", addr); 
	if (cmd == CMD_REG_WRITE) {
		data = *(uint32_t *)&ptr[8];
		clog_print(CLOG_LEVEL3, "data=%x", data);
	}
	clog_print(CLOG_LEVEL3,"\n");

	if (cmd == CMD_REG_WRITE)
		sv_write_val(addr, data);
	else
		sv_read_val(addr, &data);	

	prn_buf[0] = cmd | 0x80;
	*(uint32_t *)&prb_buf[1] = data;

	send_packet(link, PKT_HDR_RESP, prn_buf, 5, msdc_packet);
	msgd_id++;
}

/**
 * @brief svdpi_main
 *	svdpi application main
 *	intitializes the task and clink to handle
 *	services from the applications
 */
void svdpi_init(void)
{
	set_clog_output(CLOG_STDIO);
	set_clog_level(CLOG_WARN);

	/* initializing the shared message queue module */
	sm_qmem_size = 5 * 1024;
	sm_queue_addr = sm_alloc(QUEUE_SM_ID, sm_qmem_size, &shmid);
	if ((void *)sm_queue_addr == (void *)-1) {
		clog_print(CLOG_ERR, "shared memory allocation failed\n");
		return ;
	}

	/* shared msg_q module init */
	sm_cmsg_q_module_init(QUEUE_CTRLF_RESET_ALL, sm_queue_addr, sm_qmem_size);

	/* initialize no-os */
	ctask_no_os_register();

	/* initialize the clink module */
	clink_init();

	/* initialize the temrinal, cwatch modules */
	terminal_core_init();
	stdio_terminal_init();
	cwatch_init();
	terminal_register_cmd("ql", cmsg_q_print);
	terminal_register_cmd("cl", print_clink_cnx);

	/* create clink for mshc module */
	/* The svdpi-application will create clink by name "mshc"
	 * and "msdc" as mshc/server to handle the mshc/msdc clients apps
	 * and client application will get/search the mshc link by
	 *  its name "mshc" and get attached to its device clink.
	 *	[server/mshc-side-init] 
	 *		clink_mshc = create_clink("mshc", 540);
	 *	[client/device-side-init]
	 *		clink_mshc = clink_get_by_name("mshc");
	 *		clink_device = create_clink("dev", 0);
	 *		connect_link(clink_mshc, clink_device);
	 */
	clink_mshc = create_clink("mshc", 64);
	if (clink_mshc == NULL) {
		printf("unable to create clink mshc\n");
		exit(0);
	}

	mshc_rx_notify.priv_data = clink_mshc;
	mshc_rx_notify.event_handler = mshc_rx_handler;
	clink_set_rx_notify(clink_mshc, &mshc_rx_notify);
	
	/* create msdc link */
	clink_msdc = create_clink("msdc", 64);
	if (clink_msdc == NULL) {
		printf("unable to create clink mshc\n");
		exit(0);
	}

	msdc_rx_notify.priv_data = clink_msdc;
	msdc_rx_notify.event_handler = msdc_rx_handler;
	clink_set_rx_notify(clink_msdc, &msdc_rx_notify);
	

	svdpi_init_done = 1;
}

/**
 * @brief svdpi_main
 *	svdpi application main
 *	intitializes the task and clink to handle
 *	services from the applications
 */
void svdpi_main(void)
{
	if (svdpi_init_done == 0) 
		svdpi_init();

	/* run the schedular in loop */
	while (1) {
		ctask_schedular();
	}
}
