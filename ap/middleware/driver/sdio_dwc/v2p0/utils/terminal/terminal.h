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
* \file		: terminal.h
* \author	: ravibabu@synopsys.com
* \date		: 28-Sep-2010
* \brief	: terminal.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	28-Sep-2010	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include "common.h"
#include "dwc_type.h"

/****************************** terminal-core configuration *********************/
#define MAX_TOTAL_CMDS		32	
#define MAX_NUM_TERMINALS	2
#define MAX_CMDLINE_LEN		8
	
/**
 * @brief terminal_ops_t 
 *	terminal operation relies of registered communiation driver (like UART)
 *	for transmit/receive/init functions
 * init: initialize the ocmmunication link driver
 * getch: get the character from register link driver
 * putch: get the character from register link driver
 * uninit: unitialize the link driver
 */
struct terminal_ops_t {
	int (*init)(void *priv_obj);
	int (*getch)(void *priv_obj, char *ch);
	int (*putch)(void *priv_obj, char ch);
	int (*uninit)(void *priv_obj);
};

/**
 * @brief terminal_t 
 * registered : whether terminal object is registered or not
 * priv_data : privdate data of communication link driver (like uart) 
 * inbuf : user input command line sring
 * idx : buffer index
 * prompt: User defined prompt string (like "#", "TEST >" etc)
 * ops : terminal_ops_t - communication link driver operations
 */
struct terminal_t {
	uint8_t registered;
	void *priv_data;
	char inbuf[MAX_CMDLINE_LEN];
	uint8_t idx;
	char prompt[20];
	uint8_t state, skip_state;
	struct terminal_ops_t ops;
};

/**
 * @brief terminal cmd type
 * cmd_str: Name of the command
 * cmd_func: function gets executed for this user input command
 */
typedef struct type_cmd_t {
	char cmd_str[20];
	void (*cmd_func)(void *terminal, token_t *token, int max_token);
} cmd_t;

/**
 * @brief  terminal_core_init()
 *	   - called only once by main init application
 * 	   - initializes the terminal core data structure
 *	   - creates the terminal core thread to handle regiserted terminals
 *	   - also registers the terminal common commands like help, version, etc.
 * @param  : none
 * @return 0 by default
 */
int terminal_core_init(void);

/**
 * @brief  terminal_create()
 * 	creates terminal object and regisster terminals ops & parameters.	
 * @param  private_object of communication link driver which creates terminal.
 * @param  terminal_ops_t: pointers to terminal operation
 * @param  prompt string to be appeared on screen.
 * @return integer terminal id
 */
int terminal_create(void *priv_obj, struct terminal_ops_t *ops, char *prompt);

/**
 * @brief  terminal_register_cmd
 * 	- the application or other driver moduls can register its
 *	supported command
 * @param  cmdstr: Command string
 * @param  cmd_func: this function gets executed if command(cmd_str)received
 * @return 0 for success
 */
int terminal_register_cmd(char *cmdstr, void (*cmd_func)(void *terminal, token_t *token, int max_token));

/**
 * @brief  terminal_puts()
 * 		Put string to the terminal
 * @param  t - pointer to terminal_t object
 * @param  str  - ASCII string to put to terminal
 * @return 1 on success
 */
int terminal_puts(struct terminal_t *t, char *str);
#endif /* _TERMINAL_H_ */
