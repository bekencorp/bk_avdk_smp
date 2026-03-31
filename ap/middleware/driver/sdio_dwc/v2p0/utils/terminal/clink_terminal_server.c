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
* \file		: clink_terminal_server.c
* \author	: ravibabu@synopsys.com
* \date		: 28-Sep-2010
* \brief	: clink_terminal_server.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	28-Sep-2010	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"
#include "dwc_type.h"
#include "terminal.h"
#include "clink.h"
#include "cwatch.h"
#include "clog.h"

/* terminal clink_terminal_server driver */
struct clink_terminal_server_drv {
	uint8_t state;
	char inp_str[80];
	uint8_t len;
	uint8_t index;
};

/* clink_terminal_server object */
static struct clink_terminal_server_drv clink_terminal_server0;

/* clink_terminal_server terminal operations */
static int clink_terminal_server_terminal_id;

struct clink_t *clink_host, *clink_device;
uint32_t clterm_svr_rxc, clterm_svr_txc;
uint8_t terminal_rxpkt[512], terminal_txpkt[512];

/**
 * \brief  clink_terminal_server_init
 *	 initialze the driver
 * \param priv_obj : driver prividate object
 * \return 0
 */
int clink_terminal_server_init(void *priv_obj)
{
	struct clink_terminal_server_drv *std_io = (struct clink_terminal_server_drv *)priv_obj;

	std_io->state = XSTATE0;
	std_io->len = 0;


	/* The host application will create clink by name "host"
	 * and device application will get/search the host link by
	 *  its name "host" and get attached to its device clink.
	 *	[host-side-init]
	 *		clink_host = create_clink("vio-host", 540);
	 *	[device-side-init]
	 *		clink_host = clink_get_by_name("vio-host");
	 *		clink_device = create_clink("dev", 0);
	 *		connect_link(clink_host, clink_device);
	 */
	clink_host = create_clink("vio-host", 512);
	if (clink_host == NULL) {
		printf("unable to create vio-host-clink \n");
		return 0;
	}

	clog_print(CLOG_INFO, "%s: created vio-host\n", __func__);
	/********************** CLINK example end **********************/

	/* cwatch module init */
	add_to_cwatch("clterm_svr_rxc", &clterm_svr_rxc, XUINT32);
	add_to_cwatch("clterm_svr_txc", &clterm_svr_txc, XUINT32);

	clterm_svr_rxc = clterm_svr_txc = 0;

	return 0;
}

/**
 * \brief  clink_terminal_server_uninit
 *	 uninitialze the driver
 * \param priv_obj : driver prividate object
 * \return 0
 */
int clink_terminal_server_uninit(void *priv_obj)
{
	/* add drvier specific uniinit code */
	return 0;
}

/**
 * \brief  clink_terminal_server_getch
 * 	get/read character ch from standard input console
 * \param priv_obj : driver prividate object
 * \param ch : pointer to charcter to store user input char
 * \return 1 when on key press or 0 otherwise
 */
int clink_terminal_server_getch(void *priv_obj, char *ch)
{
	struct clink_t *link = (struct clink_t *)clink_host;
	uint8_t *ptr;
	int rxlen;
	struct msg_desc_t msg;

	//printf("%s\n", __func__);
        msg.buffer = terminal_rxpkt;
        msg.length = sizeof(terminal_rxpkt);
	msg.status = 0;
        if ((rxlen = link->get_msg(link, &msg)) == 0)
                return 0;

        ptr = msg.buffer;
	*ch = ptr[0];
	clterm_svr_rxc++;
	return 1;
}

/**
 * \brief  clink_terminal_server_putch
 * 	prints character ch to standard output console
 * \param priv_obj : driver prividate object
 * \param ch : charcter to be dispalyed
 * \return 1 always
 */
int clink_terminal_server_putch(void *priv_obj, char ch)
{
	struct clink_t *link = (struct clink_t *)clink_host;

	struct msg_desc_t msg;

	/* form the packet */
	terminal_txpkt[0] = ch;

	msg.length = 1;
        msg.buffer = &terminal_txpkt[0];
	clterm_svr_txc += link->put_msg(link, &msg);
	return 1;
}

/**
 * \brief  clink_terminal_server_terminal_init
 * 	create clink_terminal_server or pc based terminal console
 * \return none
 */
int clink_terminal_server_module_init(void)
{
	struct terminal_ops_t clink_terminal_server_ops;

	/* populate clink_terminal_server terminal operations */
	clink_terminal_server_ops.init = clink_terminal_server_init;
	clink_terminal_server_ops.uninit = clink_terminal_server_uninit;
	clink_terminal_server_ops.getch = clink_terminal_server_getch;
	clink_terminal_server_ops.putch = clink_terminal_server_putch;

	clink_terminal_server_terminal_id = terminal_create(&clink_terminal_server0, &clink_terminal_server_ops, "CTEST# ");

	return 0;
}
