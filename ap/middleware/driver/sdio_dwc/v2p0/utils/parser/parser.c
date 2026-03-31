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
* \file		: parser.c
* \author	: ravibabu@synopsys.com
* \date		: 24-Sep-2010
* \brief	: parser.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	24-Sep-2010	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "clog.h"
#include "parser.h"

/* parser configuraiton */
#define MAX_DICT_ENTRIES	32
#define CONFIG_FILE	"config.txt"

char parse_buf[128];
/* global declarations */
struct key_val_t dict[MAX_DICT_ENTRIES];
int dict_len;

struct parser_t {
	uint16_t cond_flag;
	uint8_t cur_state;
	uint8_t nxt_state_true;
	uint8_t nxt_state_false;
	uint16_t action_true;
	uint16_t action_false;
#define	ACT_NONE		(0)
#define	ACT_EXEC_TRUE		(1)
#define	ACT_EXEC_FALSE		(2)
#define ACT_STAY_CONTINUE	(3)
#define ACT_ACCEPT_CONTINUE	(4)
#define ACT_ACCEPT_WORD		(5)
#define ACT_ACCEPT_CMT		(6)
#define ACT_REJECT		(7)
};

/* parser state definition */
#define S0		XSTATE0
#define S1		XSTATE1
#define S2		XSTATE2
#define S3		XSTATE3
#define S4		XSTATE4
#define S5		XSTATE5
#define S6		XSTATE6
#define S7		XSTATE7
#define S8		XSTATE8
#define S9		XSTATE9
#define S10		10
#define S11		11
#define SE		100
#define SR		101

#define IS_SPC		(0)	
#define IS_ALPHA	(1)
#define IS_NUM		(2)
#define IS_CON		(3)
#define IS_EQ		(4)
#define IS_CMT		(5)
#define IS_DELMTR	(6)
#define IS_QUOTE	(7)
#define IS_NOT_QUOTE	(8)

#define IS_PKT_HDR	(9)
#define IS_RCVD_EQ_2B	(10)
#define IS_RCVD_LT_NB	(11)
#define IS_RCVD_EQ_NB	(12)

char *act_str[8] = {
	"ACT_NONE",
	"ACT_EXEC_TRUE",
	"ACT_EXEC_FALSE",
	"ACT_STAY_CONTINUE",
	"ACT_ACCEPT_CONTINUE",
	"ACT_ACCEPT_WORD",
	"ACT_ACCEPT_CMT",
	"ACT_REJECT",
};

/* CFG Grammar
var_LHS <== []*[a-zA-Z][0-9]*[_|-]*[a-zA-Z]*
var_RHS <== []*[a-zA-Z]*[0-9]*[_|-]*[a-zA-Z]*[; ]*
comment <== #[any-symbol]*
asgn-stmt <== var_LHS [=] var_RHS
[parser_LW] ->ACCEPT_WORD->parser_EQ->ACCEPT_WORD->parser_EQ->ACCEPT_WORD->add to dictionary
*/
struct parser_t var_LHS[7] = {
/*    +------------+---------+-------------+----------+--------------+--------------+ */	
/*    | condition  |cur_state| nxt_st_true | nxt_st_f | action_true  | action false | */
/*    +------------+---------+-------------+----------+--------------+--------------+ */	
	{ IS_CMT,	S0,	SE,		S1,	ACT_ACCEPT_CMT, 	ACT_EXEC_FALSE},
	{ IS_SPC,	S1,	S1,		S2,	ACT_STAY_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_SPC,	S2,	S2,		S3,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_ALPHA,	S3,	S3,		S4,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_NUM,	S4,	S4,		S5,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_ALPHA,	S5,	S3,		S6,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_SPC,	S6,	SE,		SR,	ACT_ACCEPT_WORD, 	ACT_REJECT},
};

struct parser_t var_RHS[20] = {
/*    +------------+---------+-------------+----------+--------------+--------------+ */	
/*    | condition  |cur_state| nxt_st_true | nxt_st_f | action_true  | action false | */
/*    +------------+---------+-------------+----------+--------------+--------------+ */	
	{ IS_SPC,	S0,	S0,		S1,	ACT_STAY_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_SPC,	S1,	S1,		S6,	ACT_STAY_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_ALPHA,	S2,	S2,		S3,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_NUM,	S3,	S3,		S4,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_ALPHA,	S4,	S3,		S5,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_DELMTR,	S5,	SE,		SR,	ACT_ACCEPT_WORD, 	ACT_REJECT},

	{ IS_QUOTE,	S6,	S7,		S2,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_NOT_QUOTE,	S7,	S7,		S8,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_QUOTE,	S8,	S8,		S9,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_DELMTR,	S9,	SE,		SR,	ACT_ACCEPT_WORD, 	ACT_REJECT},
};

struct parser_t var_EQ[3] = {
/*    +------------+---------+-------------+----------+--------------+--------------+ */	
/*    | condition  |cur_state| nxt_st_true | nxt_st_f | action_true  | action false | */
/*    +------------+---------+-------------+----------+--------------+--------------+ */	
	{ IS_SPC,	S0,	S0,		S1,	ACT_STAY_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_SPC,	S1,	S1,		S2,	ACT_STAY_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_EQ,	S2,	SE,		SR,	ACT_ACCEPT_WORD, 	ACT_REJECT},
};

struct parser_t *reg_expr_asgn_stmt[4] = {
	var_LHS,
	var_EQ,
	var_RHS,
	NULL,
};

/* parser macros for grammer */
#define _is_ascii(x) ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z') || (x == '_') || (x == '.'))
#define _is_num(x) ((x >= '0' && x <= '9'))
#define _is_con_sym(x) ((x == '_') || (x == '-'))
#define _is_space(x) ((x == ' ') || (x == 9))
#define _is_line(x) ((x == 0x0a) || (x == 0x0d))
#define _is_equal_sym(x) (x == '=')
#define _is_comment_sym(x) (x == '#')
#define _is_delimiter(x) ((x == ';') || (x == ' ') || (x == 0x0a) || (x == 0x0d))
#define _is_quote(x) (x == '"')
#define _is_not_quote(x) (x != '"')

#ifdef CONFIG_PKT_PARSER
#define _is_pkt_hdr(x) (((x & 0xA0) == 0xA0) && ((x != 0xA0))) 
int is_pkt_hdr(char ch, struct msg_desc_t *w) {
	return _is_pkt_hdr(ch);
}
int is_rcvd_eq_2b(char ch, struct msg_desc_t *w) {
	return (w->xferlen == 3);
}
int is_rcvd_lt_nb(char ch, struct msg_desc_t *w) {
	uint8_t *ptr = w->buffer;
	uint16_t nb = *(uint16_t *)&ptr[1]; 
	return ((w->xferlen - 2) < nb);
}
uint8_t compute_checksum(uint8_t *buf, int len)
{
	int i;
	uint8_t chksum = 0;
	for (i = 0; i < len; ++i) {
		chksum += buf[i];
	}
	chksum = ~chksum + 1;
	return chksum;
}
int is_rcvd_eq_nb(char ch, struct msg_desc_t *w) {
	uint8_t *ptr = w->buffer, flag;
	uint16_t nb = *(uint16_t *)&ptr[1]; 

	flag = ((w->xferlen - 2) == nb);
	if (flag) {
		ptr[nb + 2] = ch;
		flag = (compute_checksum(&ptr[3], nb) == 0);
	}
	return flag;
}
#endif

int is_alpha(char ch, struct msg_desc_t *w) {
	return _is_ascii(ch);
}
int is_num(char ch, struct msg_desc_t *w) {
	return _is_num(ch);
}
int is_space(char ch, struct msg_desc_t *w) {
	return _is_space(ch);
}
int is_con(char ch, struct msg_desc_t *w) {
	return _is_con_sym(ch);
}
int is_eq(char ch, struct msg_desc_t *w) {
	return _is_equal_sym(ch);
}
int is_comment(char ch, struct msg_desc_t *w) {
	return _is_comment_sym(ch);
}
int is_delimiter(char ch, struct msg_desc_t *w) {
	return _is_delimiter(ch);
}
int is_quote(char ch, struct msg_desc_t *w) {
	return _is_quote(ch);
}
int is_not_quote(char ch, struct msg_desc_t *w) {
	return _is_not_quote(ch);
}

int (*is_condx[12])(char ch, struct msg_desc_t *w) = {
	is_space,
	is_alpha,
	is_num,
	is_con,
	is_eq,
	is_comment,
	is_delimiter,
	is_quote,
	is_not_quote,
#ifdef CONFIG_PKT_PARSER
	is_pkt_hdr,
	is_rcvd_eq_2b,
	is_rcvd_lt_nb,
	is_rcvd_eq_nb,
#endif
};

/**
 * \brief run_parser
 *	parse the input char and move the state as per parser table
 * \param parser: parser table
 * \param state: state of parse table
 * \param ch : input character
 * \param w : pointer to word_t structure
 */
int run_parser(struct parser_t *parser, uint8_t *state, char ch, struct msg_desc_t *w)
{
	int i, nxt_state, j, stop;
	uint16_t action;
	int k;

	do {
		i = nxt_state = *state;
		j = parser[i].cond_flag;
		stop = 1;
		if (is_condx[j](ch, w)) {
			nxt_state = parser[i].nxt_state_true;
			action = parser[i].action_true;
		} else {
			nxt_state = parser[i].nxt_state_false;
			action = parser[i].action_false;
		}

		clog_print(CLOG_LEVEL10, "%s: state(%d) nxt_state(%d) action(%d)\n", __func__, *state, nxt_state, action);

		switch (action) {
		case ACT_NONE:
		case ACT_STAY_CONTINUE:
		case ACT_ACCEPT_CMT:
			break;
		case ACT_ACCEPT_CONTINUE:
			*(char *)(w->buffer + w->xferlen) = ch;
			w->xferlen++;
			break;
		case ACT_EXEC_TRUE:
		case ACT_EXEC_FALSE:
			*state = nxt_state;
			stop = 0;
			break;
		case ACT_ACCEPT_WORD:
			clog_print(CLOG_LEVEL10, "%s: %s\n", __func__, act_str[action]);
			break;
		default:
			clog_print(CLOG_WARN, "error: no action(%d) found\n", action);
			break;
		}

		clog_print(CLOG_LEVEL10, "%s: %s\n", __func__, act_str[action]);

		*(char *)(w->buffer + w->xferlen) = 0;
		for (k = 0; k < w->xferlen; ++k)
			clog_print(CLOG_LEVEL10, "%s: buf[%d] = %c, \n", __func__,
					k, *(char *)(w->buffer + k));
    	} while (!stop);

	*state = nxt_state;
	return action;
}

/**
 * \brief print_dictionary
 *	print the dictionary table
 * \param level : log_level
 */
void print_dictionary(int level)
{
	int i;
	//char out_str[40];
	
	clog_print(level, "\n=== dictionary : entries(%d) ====\n", dict_len);
	for (i = 0; i < dict_len; ++i) {
		/*if (get_config_data(dict[i].w1.name, out_str, 40) == 0)
			clog_print(level, "[%d] %s = %s\n", i, dict[i].w1.name,
					out_str);
		*/
		clog_print(level, "[%d] %s = %s\n", i, dict[i].w1.name,
					dict[i].w2.name);
	}
	clog_print(level, "=================================\n");
}

/**
 * \brief : read_config_file
 *	read the configuration file
 * \param config_file : configuration text file
		includes key-value paris (config_data = data)
 */
int read_config_file(char *config_file)
{
	FILE *fp;
	int i;
	uint8_t state;
	int ch, len;
	struct word_t word;
	int retval = 1, k;
	struct parser_t *parser;
	int line = 0;
	struct msg_desc_t msg;

	fp = fopen(config_file, "r");
	if (fp == NULL)
		return ERROR_INVARG;

	dict_len = 0;
	while(fgets(parse_buf, 80, fp) > 0) {

		if (parse_buf[0] == '\n')
			continue;

		k = 0;
		parser = reg_expr_asgn_stmt[k];
		state = parser->cur_state;;

		len = strlen(parse_buf);
		word.len = 0;
		line++;
		clog_print(CLOG_LEVEL10, "reading file ===> len=%d, %s\n", len, parse_buf);

		msg.buffer = word.name;
		msg.length = 40;
		msg.xferlen = 0;

		for (i = 0; i < len; ++i) {
			ch = parse_buf[i];

			retval = run_parser(parser, &state, ch, &msg);

			switch (retval) {
			case ACT_NONE:
			case ACT_STAY_CONTINUE:
				break;
			case ACT_ACCEPT_CONTINUE:
				break;
			case ACT_EXEC_TRUE:
			case ACT_EXEC_FALSE:
				break;
			case ACT_ACCEPT_WORD:

				switch (k) {
				case 0:	
					strcpy(dict[dict_len].w1.name, msg.buffer);
					clog_print(CLOG_LEVEL10, "dict[%d].w1 = %s\n", dict_len, dict[dict_len].w1.name);
					msg.xferlen = 0;
					break;
				case 1: break;
				case 2: 
					strcpy(dict[dict_len].w2.name, msg.buffer);
					clog_print(CLOG_LEVEL10, "dict[%d].w2 = %s\n", dict_len, dict[dict_len].w2.name);
					msg.xferlen = 0;
					dict_len++;
					//print_dictionary(1);
					break;
				default:
					clog_print(CLOG_WARN, "========== ERROR invalid grammer k=%d ======\n", k);
					break;
				}

				k++;
				parser = reg_expr_asgn_stmt[k];
				if (parser == NULL) 
					i = len + 1;	
				else
					state = parser->cur_state;

				break;
			case ACT_REJECT:
			case ACT_ACCEPT_CMT:
				if (retval == ACT_REJECT) {
					parse_buf[len-1] = 0;
					clog_print(CLOG_ERR, "%s: FILE:%s line: %d illegal statement error !!\n", 
							parse_buf, config_file, line);
					k = i;
					for (i = 0; i < k; ++i)
						clog_print(CLOG_ERR, " ");
					clog_print(CLOG_ERR, "^\n");
					retval = -1;
				}
				i = len + 1;
				k = 0;
				parser = reg_expr_asgn_stmt[k];
				break;
			default:
				clog_print(CLOG_WARN, "error: no action(%d) found\n", retval);
				break;
			}

			clog_print(CLOG_LEVEL10, "\trun_parser: state(%d) retval(%d %s, [%c]) w(%s %d)\n",
					state, retval, act_str[retval], ch, msg.buffer, msg.xferlen);
		} /* for */
		//print_dictionary(1);
	}

	fclose(fp);
	return (retval == 0) ? dict_len : retval;
}
/**
 * \brief : get_config_data
 *	returns the config data if found in configuration database
 * \param inp_data : input data string
 * \param cfg_data : copy the cfg-data if found 
 * retunrs	   : 0 on sucess, -ve on not found
 */
int get_config_data(char *inp_str, char *out_str, uint8_t out_str_len)
{
	int i;

	if (inp_str == NULL || out_str == NULL)
		return ERROR_INVARG;

	for (i = 0; i < dict_len; ++i) {
		if (strcmp(dict[i].w1.name, inp_str) == 0) {
			strncpy(out_str, dict[i].w2.name, out_str_len);
			return 0;
		}
	}

	return ERROR_OPER_FAIL;
}

#ifdef CONFIG_PKT_PARSER
/* Packet parser
 * 	In this example, there is packet need to accepted
 * which has following format/protocol fields
 *	offset	length Value	Description
 * -----------------------------------------
 *	0	1	0xA1	packet header
 * 	1	2	N-len	2bytes length
 *	3	NBytes  ...	N bytes of payload
 * --------------------------------------------
 * 	If packet is accepted if you data received
 * in above order. refer to utils/parser for
 * implementation of parser rule.
 */	
struct parser_t pkt_parser[4] = {
/*    +------------+---------+-------------+----------+--------------+--------------+ */
/*    | condition  |cur_state| nxt_st_true | nxt_st_f | action_true  | action false | */
/*    +------------+---------+-------------+----------+--------------+--------------+ */
	{ IS_PKT_HDR,	S0,	S1,		S0,	ACT_ACCEPT_CONTINUE, 	ACT_STAY_CONTINUE},
	{ IS_RCVD_EQ_2B,S1,	S2,		S1,	ACT_ACCEPT_CONTINUE, 	ACT_ACCEPT_CONTINUE},
	{ IS_RCVD_LT_NB,S2,	S2,		S3,	ACT_ACCEPT_CONTINUE, 	ACT_EXEC_FALSE},
	{ IS_RCVD_EQ_NB,S3,	SE,		S0,	ACT_ACCEPT_WORD, 	ACT_REJECT},
};

int pkt_parser_example(char *inp_str, int len)
{
	int bi;
	uint8_t state;
	char ch;
	int retval;
	struct parser_t *parser;
	struct msg_desc_t msg;


	parser = pkt_parser;
	state = parser->cur_state;;

	msg.buffer = parse_buf;
	msg.length = 256;
	msg.xferlen = 0;

	for (bi = 0; bi < len; ++bi) {

		ch = inp_str[bi];

		retval = run_parser(parser, &state, ch, &msg);
		if (retval == ACT_ACCEPT_WORD)
			clog_print(CLOG_INFO,"accepted the packet\n");
		else if (retval == ACT_REJECT)
			clog_print(CLOG_INFO,"reject the packet\n");
	}
	return 0;
}
#endif
