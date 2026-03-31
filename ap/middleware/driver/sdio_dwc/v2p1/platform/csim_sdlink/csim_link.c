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
* \file		: csim_link.c
* \author	: ravibabu@synopsys.com
* \date		: 15-July-2019
* \brief	: csim_link.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	15-July-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "clink.h"
#include "clog.h"
#include "cee.h"

#include "sd_cmd.h"
#include "mmc_dev.h"
#include "sdhci.h"

#define SDSIM_CMD_PKT	0xA1
#define SDSIM_RESP_PKT	0xA2
#define SDSIM_DATA_PKT	0xA3

struct notify_event_t mshc_rx_notify, msdc_rx_notify;
struct clink_t *mshc_link, *msdc_link;
struct msg_desc_t mshc_msg, msdc_msg;
struct sd_resp_pkt_t sd_resp; 
struct sd_cmd_pkt_t sd_cmd;
uint8_t cmd_pkt[16];

#define crc7_compute(x) (x ^ 0x89) /* x^7 + x^3 + x^0 = 10001001 => 0x89 */
#define get_bit(x, pos) (!!(x & (1 << pos))) 
uint8_t crc7_t[256];

extern int mshc_rx_handler(void *priv_data);
extern int msdc_rx_handler(void *priv_data);
uint32_t get_nbits(uint8_t *pkt_bits, uint8_t st, uint8_t nbits);

void crc7_init(void)
{
	int i, j;
	for (i = 0; i < 256; ++i) {
		crc7_t[i] = i;
		if (crc7_t[i] & 0x80)
			crc7_t[i] = crc7_compute(crc7_t[i]); 
		
		for (j = 1; j < 8; ++j) {
			crc7_t[i] = crc7_t[i] << 1;
			if (crc7_t[i] & 0x80)
				crc7_t[i] = crc7_compute(crc7_t[i]); 
		}
	}
}

uint8_t crc7(uint8_t *buf, int len)
{
	int i, k;
	uint8_t crc7 = 0, crc7_temp, byte;

	for (i = len; i > 6; ) {
		k = (i < 8) ? i : 8;
		byte = get_nbits(buf, i-k, k);
		crc7_temp = crc7_t[crc7] ^ byte;
//		clog_print(CLOG_LEVEL5, "[%d] crc7[%02x] = crc7_t[%02x] ^ byte[%02x],\n", i, crc7_temp, crc7_t[crc7], byte);
		crc7 = crc7_temp;
		i = i - 8;
	}

	return crc7;
}

uint32_t get_nbits(uint8_t *pkt_bits, uint8_t st, uint8_t nbits)
{
	uint32_t val = 0;
	int i, j, k, pos;

	for (i = st, pos = 0; i < st + nbits; ++i, pos++)
	{
		k = i/8;
		j = i % 8; 	
		val |= (get_bit(pkt_bits[k], j) << pos);
#ifdef CONFIG_SDMAC_DEBUG
		clog_print(CLOG_LEVEL5, "[k:%d, j:%d, pkt_bits[%d]=%x, val:%x pos:%d\n", k, j,
			k, pkt_bits[k], val, pos);
#endif
	}
	return val;
}

int csim_clink_init(void)
{
	clink_init();
	
#ifdef CONFIG_MSHC_CLINK
	mshc_link = create_clink("HOST", 527); /* blocksize(512B) + crc(2B) + pkt-header(3B) */ 
	if (mshc_link == 0) {
		clog_print(CLOG_LEVEL5, "mshc: cannot create link\n");
		return -1;
	}


	mshc_rx_notify.priv_data = mshc_link;
	mshc_rx_notify.event_handler = mshc_rx_handler;
	clink_set_rx_notify(mshc_link, &mshc_rx_notify);

	mshc_msg.buffer = &sd_cmd;
	mshc_msg.length = sizeof(sd_cmd);
	printf("mshc_msg_adr(%p), mshc_msg.buf(%p), len(%d)\n",
		&mshc_msg, mshc_msg.buffer, mshc_msg.length);
#endif

#ifdef CONFIG_MSDC_BUILD
	mshc_link = clink_get_by_name("HOST");
	if (mshc_link == 0) {
		clog_print(CLOG_LEVEL5, "mshc: cannot create link\n");
		return -1;
	}
#endif

	msdc_link = create_clink("DEV", 0); 
	if (msdc_link == 0) {
		clog_print(CLOG_LEVEL5, "msdc: cannot create link\n");
		return -1;
	}

/*
#ifdef CONFIG_MSDC_CLINK
	msdc_rx_notify.priv_data = msdc_link;
	msdc_rx_notify.event_handler = msdc_rx_handler;
	clink_set_rx_notify(msdc_link, &msdc_rx_notify);
	
	msdc_msg.buffer = &sd_resp;
	msdc_msg.length = sizeof(sd_resp);
	printf("msdc_msg_adr(%p), msdc_msg.buf(%p), len(%d)\n",
		&msdc_msg, msdc_msg.buffer, msdc_msg.length);
#ifdef CONFIG_MSDC_BUILD
	connect_link(mshc_link, msdc_link);
#endif
#endif
*/
#ifdef CONFIG_MSHC_MSDC_CLINK
	connect_link(mshc_link, msdc_link);
#endif
	return 0;	
}

int csim_init(void)
{
	int retval;

	crc7_init();

	retval = csim_clink_init();
	if (retval < 0) {
		clog_print(CLOG_LEVEL5, "csim init failed, error=%d\n", retval);
		return -1;
	}
	return 0;
}

int form_sdcmd_pkt(uint8_t *cmd_pkt, uint8_t cmdidx, uint32_t args)
{
	cmd_pkt[0] = SDSIM_CMD_PKT;
	*(uint16_t *)&cmd_pkt[1] = 6;
	cmd_pkt[3] = cmdidx;
	*(uint32_t *)&cmd_pkt[4] = args;
	cmd_pkt[8] = crc7(&cmd_pkt[3], 5);	

	return cmd_pkt[1] + 3;
}

/**
 * @brief sdhci_sim_read
 *	read 32bit from sdhci register
 * @param addr: register address
 */
uint32_t sdhci_sim_read(void *pdata, void *addr, uint8_t grain)
{
	uint32_t val;
	//struct sdhci_t *sdhci = (struct sdhci_t *)pdata;

	val = *(volatile uint32_t *)addr;
	clog_print(CLOG_LEVEL9, "%s: read value (%p) from addr[%x]\n", __func__,
			val, (addr_t)addr);
	return val;
}

/**
 * @brief sdhci_sim_write
 *	write 32bit value to sdhci register
 * @param addr: register address
 */
uint32_t sdhci_sim_write(void *pdata, void *addr, uint32_t val, uint8_t grain)
{
	struct sdhci_t *sdhci = (struct sdhci_t *)pdata;
	uint16_t reg_offs = (addr_t)addr - (addr_t)sdhci->reg;

	uint8_t cmd; 
	uint32_t args;
	int tx_len;

	switch (grain) {
	case 1:  
		*(volatile uint8_t *)addr = val;
		break;
	case 2:  
		*(volatile uint16_t *)addr = val;
		break;
	case 4:  
		*(volatile uint32_t *)addr = val;
		break;
	default:
		break;
	}

	clog_print(CLOG_LEVEL9, "%s: writing value (%p) to addr[%x]\n", __func__,
			val, (addr_t)addr);

	switch (reg_offs) {
	case 0x0c: /* offs: 0x0c transfer mode */
		break;
	case 0x0e: /* offs: 0x0e command */
		cmd = val & 0x3F;
		args = *(volatile uint32_t *)&sdhci->reg->arg;
		if (cmd == 0) {
			clog_print(CLOG_LEVEL5, "[SDHCI-SIM]: Sending cmd0 (go idle command)\n");
		}
		if (cmd == 2) {
			clog_print(CLOG_LEVEL5, "[SDHCI-SIM]: Sending cmd2 (send all cid)\n");
		}

		clog_print(CLOG_LEVEL5, "mshc-link(%x)\n", mshc_link);
		if (mshc_link) {

			mshc_msg.buffer = cmd_pkt;
			mshc_msg.length = form_sdcmd_pkt(cmd_pkt, cmd, args);
			mshc_msg.status = 0;

			tx_len = mshc_link->put_msg(mshc_link, &mshc_msg); 
			clog_print(CLOG_LEVEL5, "mshc-msg(%x) buf(%x) len=%d\n", &mshc_msg, mshc_msg.buffer, tx_len);
			if (mshc_msg.status != mshc_msg.length)
				clog_print(CLOG_ERR, "unable sent msg to %s\n", mshc_link->name);
		} else
			clog_print(CLOG_ERR, "error: mshc host link not created\n");
		break;
	}
	return 0;
}

/**
 * @brief msdc_sim_read
 *	read 32bit from msdc register
 * @param addr: register address
 */
uint32_t msdc_sim_read(void *pdata, void *addr, uint8_t grain)
{
	uint32_t val;

	val = *(volatile uint8_t *)addr;
	clog_print(CLOG_LEVEL9, "%s: read value (%p) from addr[%x]\n",__func__,
			val, (addr_t)addr);
	return 0;
}

/**
 * @brief msdc_sim_write
 *	write 32bit value to msdc register
 * @param addr: register address
 */
uint32_t msdc_sim_write(void *pdata, void *addr, uint32_t val, uint8_t grain)
{
	struct sdhci_t *sdhci = (struct sdhci_t *)pdata;
	uint16_t reg_offs = (addr_t)addr - (addr_t)sdhci->reg;

	clog_print(CLOG_LEVEL9, "%s: writing value (%p) to addr[%x]\n", __func__,
			val, (addr_t)addr);
	switch (grain) {
	case 1:  
		*(volatile uint8_t *)addr = val;
		break;
	case 2:  
		*(volatile uint16_t *)addr = val;
		break;
	case 4:  
		*(volatile uint32_t *)addr = val;
		break;
	default:
		break;
	}

	switch (reg_offs) {
	case 0x0c: /* offs: 0x0c transfer mode */
		break;
	case 0x0e: /* offs: 0x0e command */
		break;
	}
	return 0;
}
