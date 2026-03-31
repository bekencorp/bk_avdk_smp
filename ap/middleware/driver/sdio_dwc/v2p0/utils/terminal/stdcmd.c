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
* \file		: stdcmd.c
* \author	: ravibabu@synopsys.com
* \date		: 28-Sep-2010
* \brief	: stdcmd.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	28-Sep-2010	ravibabui@synopsys.com  001		dev in progress
*/


#include <stdio.h>
#include <stdlib.h>
#include <dwc_type.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "dwc_type.h"
#include "terminal.h"
#include "clog.h"
#include "hal.h"

/* globals */
void *mem_hal_obj;
char 	tmnl_prnbuf[512] = "";
uint8_t bit_wise_diplay_en = 0;

/* extern */
uint32_t ascii2hex(char *str, int *err_flag);
extern void ctest_shut_down(void);
extern void print_new_line(struct terminal_t *t);
extern void print_reg_desc(addr_t offs, uint32_t value, uint8_t print_flag);
extern void print_reg_desc(uint32_t offs, uint32_t value, uint8_t print_log);

/**
 * \brief  terminal_print_line()
 *	print line on terminal
 * \param  terminal - pointer to terminal object
 */
void terminal_print_line(void *terminal, int n_char)
{
	memset(tmnl_prnbuf, '-', n_char);
	terminal_puts(terminal, tmnl_prnbuf);
	sprintf(tmnl_prnbuf, "\n");
	terminal_puts(terminal, tmnl_prnbuf);
}
/**
 * \brief  cmd_version()
 * 		Prints the terminal versions
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void cmd_version(void *terminal, token_t *token, int max_token)
{
	struct terminal_t *t = terminal;

	terminal_puts(t, "Terminal Version: 0.1\n");
}

/**
 * \brief  cmd_exit()
 * 		Exit the terminal
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void cmd_exit(void *terminal, token_t *token, int max_token)
{
	struct terminal_t *t = terminal;

	terminal_puts(t, "cmd_exit\n");
	exit(0);
}

/**
 * \brief  cmd_help()
 * 		Help implementation for the terminal
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void cmd_help(void *terminal, token_t *token, int max_token)
{
	struct terminal_t *t = terminal;

	terminal_puts(t, "Terminal Version: 0.1\n");
	terminal_puts(t, "	version: display version\n");
	terminal_puts(t, "	mr <addr> <len> or -b : memory read dword, -b bitwise display\n");
	terminal_puts(t, "	mw <addr> <data> : memory write dword\n");
	terminal_puts(t, "	help: help\n");
	terminal_puts(t, "	exit: exit console\n");
	return;
}

/**
 * \brief  ascii2hex()
 * 		Utils function - ascii to hex
 * \param  str - ASCII string
 * \return hex value
 */
uint32_t ascii2hex(char *str, int *err_flag)
{
	int i, val = 0, limit = 0, ii , len = strlen(str), p16 =1;

	if (str[0] == '0' && ((str[1] == 'x') || (str[1] == 'X')))
	limit = 2;

	if (len - 2 > 8) {
		*err_flag |= 1;
		return 0;
	}

	len = len-1;
	for (i = len; i >= limit; --i) {
		if ((str[i] >= 'a' && str[i] <= 'f'))
		ii = str[i] - 'a' + 10;
		else if ((str[i] >= 'A' && str[i] <= 'F'))
		ii = str[i] - 'A' + 10;
		else if ((str[i] >= '0' && str[i] <= '9'))
		ii = str[i] - '0';
		else {
			*err_flag |= 2;
			return 0;
		}
		val += ii * (p16);
		p16 = p16 * 16;
	}
	return val;
}

#ifdef CONFIG_TERMINAL_CMD_MEM
void hal_rd_val(void *addr, uint32_t *val, int grain)
{
	if (mem_hal_obj)
		*val = hal_read(mem_hal_obj, addr);
	else
		*val = 0xDEADBEEF;
}
void hal_wr_val(void *addr, uint32_t val, int grain)
{
	clog_print(CLOG_LEVEL5, "addr:%x, val:%x\n",addr, val);

	if (mem_hal_obj)
		hal_write(mem_hal_obj, (void *)addr, val);
}

void pr_bit(struct terminal_t *t, unsigned int val, int st_bit, int et_bit, char z_ch)
{
	int i, bit;
	char tbuf[20];

	if (st_bit > et_bit) {
		i = st_bit;
		st_bit = et_bit;
		et_bit = i;
	}

	tmnl_prnbuf[0] = 0;
	for (i = et_bit; i >= st_bit; i--) {
		sprintf (tbuf, "b%02d%s ", i, (i%4 == 0) ? "  " : "");
		strcat(tmnl_prnbuf, tbuf);
	}
	sprintf(tbuf, "%s\n", "");
	strcat(tmnl_prnbuf, tbuf);
	terminal_puts(t, tmnl_prnbuf);

	tmnl_prnbuf[0] = 0;
	for (i = et_bit; i >= st_bit; i--)
	{
		bit = !!((val & (1 <<i)));
		if (bit)
			sprintf(tbuf, "%3d %s", bit, (i%4 == 0) ? "  " : "");
		else
			sprintf(tbuf, "  %c %s", z_ch, (i%4 == 0) ? "  " : "");
		strcat(tmnl_prnbuf, tbuf);
	}
	sprintf(tbuf, "%s\n", "");
	strcat(tmnl_prnbuf, tbuf);
	terminal_puts(t, tmnl_prnbuf);
	tmnl_prnbuf[0] = 0;
}
/**
 * \brief  mem_read()
 * 		Perform memory read operation
 * \param  addr  - Address to be read
 * \param  len   - length to be read
 * \param  grain - 1 / 2 / 4 for 8/16/32 bits
 * \return void
 */
void mem_read(struct terminal_t *t, void *addr, int len, int grain)
{
	unsigned int i, offs = 0;
	int skip;
	uint32_t val = 0, bval = 0;

	switch(grain) {
	case 1: skip = 16; break;
	case 2: skip = 8; break;
	default: skip = 4;
	}

	tmnl_prnbuf[0] = 0;
	for (i = 0; i < (len)/grain; ) {
		if ((i % skip) == 0) {
			print_new_line(t);
			sprintf(tmnl_prnbuf, "%p: ", (addr + offs));
			terminal_puts(t, tmnl_prnbuf);
		}

		switch(grain) {
		case 4:
			hal_rd_val((void *)(addr+ i * 4), &val, 0);
			sprintf(tmnl_prnbuf, "%08x ", val);
			break;
		case 2:
			//*(unsigned short *)(addr + i * 2));
			hal_rd_val((void *)(addr+ i * 2), &val, 0);
			sprintf(tmnl_prnbuf, "%04x ", val);
			break;
		case 1:
			//*(unsigned char *)(addr + i ));
			hal_rd_val((void *)(addr+ i), &val, 0);
			sprintf(tmnl_prnbuf, "%02x ", val);
			break;
		default: /*do nothing */ break;
		}

		if (i == 0)
			bval = val;
		terminal_puts(t, tmnl_prnbuf);
		offs = offs + grain;
		i++;
	}
	print_new_line(t);

	if (bit_wise_diplay_en) {
		sprintf(tmnl_prnbuf, "\nreg-addr %p\t\t\t\tvalue = %08X \n\n", addr, bval);
		terminal_puts(t, tmnl_prnbuf);
		terminal_print_line(t, 69);
		pr_bit(t, bval, 16, 31, '0');
		pr_bit(t, bval, 0, 15, '0');
		bit_wise_diplay_en = 0;
		print_reg_desc(addr, bval, 0);
	}
}

/**
 * \brief  mem_write()
 * 		Perform memory read operation
 * \param  addr  - Address to be read
 * \param  len   - length to be read
 * \param  grain - 1 / 2 / 4 for 8/16/32 bits
 * \return void
 */
void mem_write(struct terminal_t *t, void *addr, unsigned int data, int grain)
{
	unsigned int i, offs = 0;
	int skip;
	uint32_t val;

	switch(grain) {
	case 1: skip = 16; break;
	case 2: skip = 8; break;
	default: skip = 4;
	}

	hal_wr_val(addr, data, 1);

	i = 0;
	{
		if ((i % skip) == 0) {
			print_new_line(t);
			sprintf(tmnl_prnbuf, "%p: ", (void *)addr + offs);
			terminal_puts(t, tmnl_prnbuf);
		}

		switch(grain) {
		case 4:
			hal_rd_val((void *)(addr+ i * 4), &val, 1);
			sprintf(tmnl_prnbuf, "%08x ", val);
			break;
		case 2:
			hal_rd_val((void *)(addr+ i * 2), &val, 1);
			sprintf(tmnl_prnbuf, "%04x ", val);
			break;
		case 1:
			hal_rd_val((void *)(addr+ i), &val, 1);
			sprintf(tmnl_prnbuf, "%02x ", val);
			break;
		default: /*do nothing */ break;
		}

		terminal_puts(t, tmnl_prnbuf);
		offs = offs + grain;
		i++;
	}
	print_new_line(t);
}

/**
 * \brief  cmd_print_regbit()
 * 		print bitwise details
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void cmd_print_regbit(void *terminal, token_t *token, int max_token)
{
	uint32_t word;
	int is_err = 0;

	if (max_token > 1) {
		word = (uint32_t)ascii2hex(token[1].str, &is_err);
		if (is_err) {
			terminal_puts(terminal, "invalid args\n");
			return ;
		}
		sprintf(tmnl_prnbuf, "bitwise details of value %08x\n", word);
		terminal_print_line(terminal, 69);
		pr_bit(terminal, word, 16, 31, '0');
		pr_bit(terminal, word, 0, 15, '0');
	}
	else
		terminal_puts(terminal, "args missing\n");
}
/**
 * \brief  cmd_memdump()
 * 		Terminal command to dump the memory
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void cmd_memdump(void *terminal, token_t *token, int max_token)
{
	uint32_t len = 4;
	int i, is_err = 0;
	addr_t addr;

	bit_wise_diplay_en = 0;
	if (max_token > 1) {
		addr = (addr_t)ascii2hex(token[1].str, &is_err);
		i = max_token-1;
		if((max_token == 3) && (token[i].str[0] == '-') && (token[i].str[1] == 'b'))
			bit_wise_diplay_en = 1;
		if (bit_wise_diplay_en == 0) {
			if (max_token > 2)
				len  = ascii2hex(token[2].str, &is_err);
			if (len > 128)
				len = 128;
			if (len < 4)
				len = 4;
		}
		if (is_err) {
			terminal_puts(terminal, "invalid args\n");
			return ;
		}
		mem_read(terminal, (void *)addr, len, 4);
	}
	else
		terminal_puts(terminal, "args missing\n");
}

/**
 * \brief  cmd_memdump()
 * 		Terminal command to dump the memory
 * \param  token - Terminal token
 * \param  max_token - Maximum tokens
 * \return void
 */
void cmd_mem_write(void *terminal, token_t *token, int max_token)
{
	uint32_t data;
	void *addr;
	int is_err = 0;

	bit_wise_diplay_en = 0;
	if (max_token > 2) {
		addr = (void *)ascii2hex(token[1].str, &is_err);
		data  = ascii2hex(token[2].str, &is_err);
		if (is_err) {
			terminal_puts(terminal, "invalid args\n");
			return ;
		}
		mem_write(terminal, (void *)addr, data, 4);
	}
	else
		terminal_puts(terminal, "args missing\n");
}
#endif



/**
 * \brief  terminal_console_init()
 * 		Initializes the terminal console
 * \param  none
 * \returns 0
 */
int terminal_std_cmd_init(void)
{
	/* register terminal console basic cmds */
	terminal_register_cmd("version", cmd_version);
#ifdef CONFIG_TERMINAL_CMD_MEM
	terminal_register_cmd("mr", cmd_memdump);
	terminal_register_cmd("mw", cmd_mem_write);
#endif
        /*FIXME:wangzhilei*/
	//terminal_register_cmd("bit", cmd_print_regbit);
	terminal_register_cmd("help", cmd_help);
	terminal_register_cmd("exit", cmd_exit);

	return 0;
}
