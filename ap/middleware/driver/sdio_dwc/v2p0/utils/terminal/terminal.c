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
* \file		: terminal.c
* \author	: ravibabu@synopsys.com
* \date		: 28-Sep-2010
* \brief	: terminal.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	28-Sep-2010	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "dwc_type.h"
#include "terminal.h"
#include "ctask.h"
#include "clog.h"

#if CONFIG_CTASK_NO_OS
#include "ctask_no_os.h"
#endif

#define MAX_CMDS	MAX_TOTAL_CMDS

//#define is_digit(ch) (((ch) >= '0') && ((ch) <= '9'))
#define is_alphabet(ch) ((((ch) >= 'a') && ((ch) <= 'z')) || (((ch) >= 'A') && ((ch) <= 'Z')))
#define is_space(ch) (ch == 0x20)
//#define is_backspace(ch) (ch == 8)
#define is_backspace(ch) ((ch == 127) || (ch == 8))
#define is_hypen(ch) (ch == 45)
#define is_cr(ch) ((ch == 0x0D) || (ch == 0x0a))
#define is_underscore(ch) (ch == '_')

/* terminal command table */
static cmd_t	cmd_tbl[MAX_CMDS];

/* maximum number of terminal command supported */
static int 	max_num_cmds;

/* list of terminal object */
static struct terminal_t terminal_objs[MAX_NUM_TERMINALS];

/* number of terminal registered */
static int max_num_terminals;

/* extern function definition */
extern int terminal_std_cmd_init(void);

/**
 * \brief  terminal_register()
 * 	creates terminal object and regisster terminals ops & parameters.	
 * \param  private_object of communication link driver which creates terminal.
 * \param  terminal_ops_t: pointers to terminal operation
 * \param  prompt string to be appeared on screen.
 * \return integer terminal id
 */
static int terminal_register(void *priv_obj, struct terminal_ops_t *ops, char *prompt)
{
	int i;

	/* allocate ther terminal object */
	for (i = 0; i < MAX_NUM_TERMINALS; ++i)
		if (terminal_objs[i].registered == 0) 
			break;

	if (i >= MAX_NUM_TERMINALS)
		return -1;
	
	/* register the ops and other parameters */
	terminal_objs[i].ops.init = ops->init;
	terminal_objs[i].ops.uninit = ops->uninit;
	terminal_objs[i].ops.getch = ops->getch;
	terminal_objs[i].ops.putch = ops->putch;
	terminal_objs[i].priv_data = priv_obj;

	if (prompt)
		strcpy(terminal_objs[i].prompt, prompt);
	else
		strcpy(terminal_objs[i].prompt, "# ");

	terminal_objs[i].registered = 1;
	max_num_terminals++;

	if (ops->init)
		ops->init(priv_obj);

	/* returns terminal id */
	return i;
}

/**
 * \brief  terminal_create()
 * 	creates terminal object and regisster terminals ops & parameters.	
 * \param  private_object of communication link driver which creates terminal.
 * \param  terminal_ops_t: pointers to terminal operation
 * \param  prompt string to be appeared on screen.
 * \return integer terminal id
 */
int terminal_create(void *priv_obj, struct terminal_ops_t *ops, char *prompt)
{

	/* allocate ther terminal object */
	return terminal_register(priv_obj, ops, prompt);
}	
/**
 * \brief  terminal_unregister
 * 		unregister the registered terminal 
 * \param  terminal id to be unregistered
 * \return none
 */
void terminal_unregister(int id)
{
	if (terminal_objs[id].registered) {
		terminal_objs[id].registered = 0;
		if (max_num_terminals)
			max_num_terminals--;
	}
}

/**
 * \brief  terminal_register_cmd()
 * 	- the application or other driver moduls can register its
 *	supported command
 * \param  cmdstr: Command string
 * \param  cmd_func: this function gets executed if command(cmd_str)received
 * \return 0 for success
 */
int terminal_register_cmd(char *cmdstr, void (*cmd_func)(void *terminal, token_t *token, int max_token))
{
	int i = 0, index = 0;

	if ((cmdstr == 0) || (cmd_func == 0))
		return -1;

	for (i = 0; i < MAX_CMDS; ++i)  {
		if (!cmd_tbl[i].cmd_str[0] && !cmd_tbl[i].cmd_func) {
			index = i;
			break;
		}
	}

	if (i >= MAX_CMDS)
		return -1;

	strcpy(cmd_tbl[index].cmd_str, cmdstr);
	cmd_tbl[index].cmd_func = cmd_func;
	max_num_cmds++;
	return 0;
}

/**
 * \brief  print_new_line()
 * 		Puts new line on terminal console
 * \param  t - pointer to terminal_t object
 * \return none
 */
void print_new_line(struct terminal_t *t)
{
	while(t->ops.putch(t->priv_data, 0x0d) == 0);
	while(t->ops.putch(t->priv_data, 0x0a) == 0);
}


/**
 * \brief  terminal_puts()
 * 		Put string to the terminal
 * \param  t - pointer to terminal_t object
 * \param  com  - com number
 * \param  str  - ASCII string to put to terminal
 * \return 1 on success
 */
int terminal_puts(struct terminal_t *t, char *str)
{
	int i;
	int len	= strlen(str);

	for (i = 0; i < len; ) {
		if (t->ops.putch(t->priv_data, str[i]) == 1) {
			if (str[i+1] == 0xa) {
				print_new_line(t);
				i++;
				return 1;
			}
			i++;
		}
	}
	return 1;
}

/**
 * \brief  print_to_terminal()
 * 		unregister the terminal
 * \param  terminal id to be unregistered
 * \return none
 */
int print_to_terminal(char *str)
{
	int i;

	for (i = 0; i < MAX_NUM_TERMINALS; ++i) {
		if (terminal_objs[i].registered == 0)
			continue;

		terminal_puts(&terminal_objs[i], str);
	}
	return 0;
}

/**
 * \brief  terminal_get_token()
 * 		Get terminal token
 * \param  inbuf  - Input buffer string
 * \param  token  - tokens list
 * \param  max_token  - number of tokens
 * \returns void
 */
void terminal_get_token(char *inbuf, token_t *token, int *max_token)
{
	int i = 0, ii = 0, k = 0, len = 0;
	int state = XSTATE0;

	i = 0;
	while(inbuf[i])
	{
		switch (state)
		{
			case XSTATE0: {
				k = len = 0;
				ii = i;
				state = XSTATE1;
			}
			//break; avoid break purposefully, fix for coverity

			case XSTATE1: {
				/* remove any space */
				if (inbuf[i] != 0x20) {
					ii = i;
					state = XSTATE2;
					} else {
					i++;
					break;
				}
			}
			//break; avoid break purposefully, fix for coverity

			case XSTATE2: {
				/*  word begins */
				if (inbuf[i] == 0x20) {
					state = XSTATE3;
					len = i - ii;
					} else {
					i++;
					len++;
					break;
				}
			}
			//break; avoid break purposefully, fix for coverity

			case XSTATE3: {
				token[k].len = len;
				token[k].str = &inbuf[ii];
				inbuf[i++] = 0;
				k++;
				len = 0;
				state = XSTATE1;
				break;
			}
			//break; avoid break purposefully, fix for coverity
		};
	} // end of while

	if (state == XSTATE2) {
		token[k].len = len;
		token[k].str = &inbuf[ii];
		inbuf[i++] = 0;
		k++;
		len = 0;
		state = XSTATE0;
	}

	*max_token = k;
	return;
}

/**
 * \brief  terminal_execute_cmd
 *	- Execute valid user command if command registered
 *	by the application
 * \param  t - pointer to terminal_t object
 * \param  inbuf  - Input buffer string
 * \returns none
 */
void terminal_execute_cmd(struct terminal_t *t, char *inbuf)
{
	int max_token, i;
	token_t token[10];

	/* saperate the input string into tokens */
	terminal_get_token(inbuf, token, &max_token);
	if (max_token > 0) {
		
		/* execute if supported valid command found in cmd table */
		for (i = 0; i < MAX_CMDS; ++i)  {
			if (strcmp(cmd_tbl[i].cmd_str, token[0].str) == 0) {
				cmd_tbl[i].cmd_func(t, token, max_token);
				break;
			}
		}

		/* user input command not found */
		if (i >= MAX_CMDS) {
			terminal_puts(t, "invalid command\n");
		}
	}
	return;
}

/**
 * \brief  terminal_skip_char()
 * 		skip control/special ascii char set and allow valid char
 * \param  t - pointer to terminal_t object
 * \param  ch  - char
 * \return 0 if chracter is valid, 1 when character to be skipped
 */
int terminal_skip_char(struct terminal_t *t, char ch)
{
	int is_valid_ch = 0;
	uint8_t state = t->skip_state, nxt_state;

	is_valid_ch = (is_digit(ch) || is_alphabet(ch) || is_space(ch) || is_backspace(ch) ||
			is_hypen(ch) || is_cr(ch) || is_underscore(ch));

	/* TODO: This logic added as bugfix for terminal on uart-interface on Arc board,
	 *	need to be revisited for right fix
	 */
	nxt_state = state;
	switch (state)
	{
	case XSTATE0 :
		if (is_valid_ch == 1)
			return 0;
		nxt_state = XSTATE1;
		break;

	case XSTATE1:
		if (is_valid_ch == 1)
			nxt_state =XSTATE0;
		break;
	}
	t->skip_state = nxt_state;

	return 1;
}

/**
 * \brief  terminal_clear_console()
 * 		Clear and print on string on terminal
 * \param  t - pointer to terminal_t object
 * \param  str  - ASCII string to put to terminal
 * \param  len  - length of string
 * \return 0 on sucess
 */
int terminal_clear_console(struct terminal_t *t, char *str, int len)
{
	int i;

	for (i = 0; i < len+strlen(t->prompt); ++i)
		while(t->ops.putch(t->priv_data, 0x20) == 0);

	while(t->ops.putch(t->priv_data, 0x0d) == 0);
	terminal_puts(t, t->prompt);

	for (i = 0; i < len-1; ++i)
		while(t->ops.putch(t->priv_data, str[i]) == 0);

	return 0;
}

		
/**
 * \brief  terminal_get_line()
 *	get the user input command from terminal input
 * \param  t - pointer to terminal_t object
 * \param  buf  - ASCII string to put to terminal
 * \param  len  - length of string
 * \return 1 when user input is received, 0 otherwise
 */
int terminal_get_line(struct terminal_t *t, char *buf, int len)
{
#define S0_INIT			XSTATE0
#define S1_GET_NEXT_CHAR	XSTATE1

	int state, i, nxt_state;
	char ch;

	state = t->state;
	nxt_state = state;
	i = t->idx;
	switch (state)
	{
		case S0_INIT:
			/* clear buffer index to 0 */
			t->idx = 0;
			i = 0;
			nxt_state = S1_GET_NEXT_CHAR;
			//break; avoid break purposefully, fix for coverity

		case S1_GET_NEXT_CHAR:
			/* accumulate the next char received till
		   	 * enter key pressed 
			 */
			if (t->ops.getch(t->priv_data, &ch) == 1)  {

				/* skip all special keyboard keys allow
				 * only the ascii and numbers & enter key
				 */
				if (terminal_skip_char(t, ch))
					break;
				/* if enter key is pressed then accept the
				 * the user input command
				 */
				if ((ch == 0x0a) ||(ch == 0x0d)) {
					while(t->ops.putch(t->priv_data, 0x0d) == 0);
					while(t->ops.putch(t->priv_data, 0x0a) == 0);
					buf[i] = 0;
					i = 0;
					t->idx = 0;
					nxt_state = S0_INIT;
					/* return the valid user input is received */
					return 1;
				} else {
					/* accept the next character form terminal input */
					if ( i >= len-1)
					{
						buf[i] = 0;
						i = 0;
						t->idx = 0;
						nxt_state = S0_INIT;
						return 1;
					} else {
						/* handle the backspace */
						if (i && is_backspace(ch))
							i--;
						else
							buf[i++] = ch;
						buf[i] = 0;
						
						/* echo back the input character on terminal outptu */
						while(t->ops.putch(t->priv_data, 0x0d) == 0);
						terminal_clear_console(t, buf, i+1);
					}
				}
			}
			break;

		default:
			/* invalid state clear input buffer */
			buf[i] = 0;
			i = 0;
			t->idx = 0;
			nxt_state = S0_INIT;
			return 1;
	};

	t->state = nxt_state;
	t->idx = i;
	return 0;
}

/**
 * \brief  terminal_run()
 *	- execute particular instance of registered temrinal 
 *	- Algorithm
 *		1) get the users command line
 *		2) interpret and execute the valid command
 *		3) again display terminal prompt for next 
 *		   user input
 * \param  t: pointer to terminal_t object
 * \return 0 by default
 */
int terminal_run(struct terminal_t *t)
{
	do {
		/* get the user command through terminal commmand line interface */
		if (terminal_get_line(t, t->inbuf, MAX_CMDLINE_LEN) == 1) {
			clog_print(CLOG_LEVEL9, "COMMAND RECVD : %s, len:%d\n",
					t->inbuf, strlen(t->inbuf));

			/* execute the valid command */
			if (strlen(t->inbuf) > 0) {
				terminal_execute_cmd(t, t->inbuf);
			}

			/* display prompt user for next command */
			print_new_line(t);
			terminal_puts(t, t->prompt);
		}
	} while(0);

	return 0;
}

/**
 * \brief  terminal_core_task()
 * 	   core terminal task executes all active terminals to
 *	   handle the user commands
 * \param  task id and args
 * \return 1 by default
 */
int terminal_core_task(uint8_t task_id, void *args)
{
	int i;

	/* execute ther registered multiple instance of terminals 
	 * where max_num_terminals is the number of terminals registered
	 */
	for (i = 0; i < max_num_terminals; ++i) {
		if (terminal_objs[i].registered == 0) 
			continue;
		terminal_run(&terminal_objs[i]);
	}

	/* task must always returns 1 in ctask */
	return 1;
}

/**
 * \brief  terminal_core_init()
 *	   - called only once by main init application
 * 	   - initializes the terminal core data structure
 *	   - creates the terminal core thread to handle regiserted terminals
 *	   - also registers the terminal common commands like help, version, etc.
 * \param  : none
 * \return 0 by default
 */
int terminal_core_init(void)
{
	int i;

	/* intitilaize the core teerminal data structure */
	for (i = 0; i < MAX_NUM_TERMINALS; ++i) {
		terminal_objs[i].registered = 0; 
		terminal_objs[i].state = terminal_objs[i].skip_state = 0;
	}

	/* reset number of number terminals registerd to 0 */
	max_num_terminals = 0;

	/* regsiter basic commands like help, version etc*/
	terminal_std_cmd_init();

#if CONFIG_CTASK_NO_OS
	terminal_register_cmd("tl", no_os_cmd_list_ctask);
	terminal_register_cmd("tr", no_os_cmd_ctask_control);
	terminal_register_cmd("ts", no_os_cmd_ctask_control);
	terminal_register_cmd("te", no_os_cmd_ctask_control);
	terminal_register_cmd("tz", no_os_cmd_ctask_control);
#endif

	/* creates terminal core task */
	ctask_enable(ctask_create("terminal-core", terminal_core_task, 0, 0, 0, 0));

	return 0;
}

