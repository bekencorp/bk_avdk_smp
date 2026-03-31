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
* \file		: tee.c
* \author	: ravibabu@synopsys.com
* \date		: 20-Feb-2020
* \brief	: tee.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	20-Feb-2020	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "dwc_type.h"
#include "ctask.h"
#include "cmsg_queue.h"
#include "clog.h"
#include "cwatch.h"
#include "sd_cmds.h"
#include "tee.h"
#include "mshc_regs.h"
#include "mmc_core.h"
#include "sd_cmd.h"
#include "parser.h"

/* globals */
struct tee_t g_tee[MAX_NUM_TEE];

/* externs */
extern struct key_val_t dict[32];
extern int dict_len;
extern uint32_t ascii2hex(char *str, int *err_flag);
extern struct cmd_param_t *alloc_cmd_req(uint8_t flags);
extern void cmd_req_reinit(struct cmd_param_t *mmc_cmd); 

#define MAX_MSHC_SPEED_MODE	9
struct str_map_t mshc_speed_mode[MAX_MSHC_SPEED_MODE] = {
	{"SDR12",	SDR12_DS_SPEED_MODE },
	{"SDR25",	SDR25_HS_SPEED_MODE },
	{"SDR50",	SDR50_SPEED_MODE },
	{"SDR104",	SDR104_SPEED_MODE },
	{"DDR50",	DDR50_SPEED_MODE },
	{"EMMC_DS",	XEMMC_DS_SPEED_MODE },
	{"EMMC_HS",	XEMMC_HS_SPEED_MODE },
	{"EMMC_HS200",	XEMMC_HS200_SPEED_MODE },
	{"EMMC_HS400",	XEMMC_HS400_SPEED_MODE },
};

#define is_yes(x) ((x == 'y') || (x == 'Y'))
int tee_load_testcases(struct tee_t *tee, char *tc_cfg_file);

uint32_t stoi(char *str, int *errflag) 
{
	uint32_t val = 0;
	int i;

	*errflag = 0;
	if (str[0] == '0' && ((str[1] == 'x') || (str[1] == 'X')))
		val = ascii2hex(str, errflag);
	else {
		for (i = 0; i < strlen(str); ++i)
			if (!(str[i] >= '0' && str[i] <= '9')) {
				*errflag = -1;
				return 0;
			}
		val = atoi(str);
	}

	return val;
}

/* \brief : tee_init
 * 	initialize tee module
 * \param none 
 * returns : returns 0 on success
 */
int tee_init(void)
{
	memset(&g_tee[0], 0, sizeof(g_tee));
	return 0;
}

/* \brief : alloc_tee
 * 	allocate tee object
 * \param name: name of TEE
 * returns : returns pointer to tee;
 */
struct tee_t *alloc_tee(char *name)
{
	int i;

	for (i = 0; i < MAX_NUM_TEE; ++i) {
		if (g_tee[i].allocated == 0)
			break;
	}

	if (i >= MAX_NUM_TEE)
		return NULL;

	if (name)
		strncpy(g_tee[i].name, name, 40);
	g_tee[i].allocated = 1;

	return &g_tee[i];
}

/* \brief : free_tee
 * 	free the tee object
 * \param tee: pointer to tee object
 * returns : returns none;
 */
void free_tee(struct tee_t *tee)
{
	if (tee)
		tee->allocated = 0;
}

/* \brief : register_tee
 * 	rgister with tee
 * \param name: name of TEE
 * \param ops : TEE opeations
 * \param tc_cfg_file: test case cfg file
 * \returns : returns pointer to tee object
 */
struct tee_t *register_tee(char *name, struct tee_ops_t *ops, char *tc_cfg_file)
{

	struct tee_t *tee = NULL;

	if (ops == NULL)
		return NULL;

	tee = alloc_tee(name);
	if (tee == NULL)
		return NULL;

	if (name)
		strncpy(tee->name, name, 40);
	tee->priv_data = ops->priv_data;

	if (tc_cfg_file)
		strncpy(tee->tc_cfg_file, tc_cfg_file, 40);

	if (ops->prepare_test_case)
		tee->ops.prepare_test_case = ops->prepare_test_case;
	if (ops->run_test_case)
		tee->ops.run_test_case = ops->run_test_case;
	if (ops->get_status)
		tee->ops.get_status = ops->get_status;
	if (ops->release_test_case)
		tee->ops.release_test_case = ops->release_test_case;
	if (ops->update_tc_param)
		tee->ops.update_tc_param = ops->update_tc_param;
	if (ops->get_hal_obj)
		tee->ops.get_hal_obj = ops->get_hal_obj;
	if (ops->select_device)
		tee->ops.select_device = ops->select_device;

	return tee;
}

/** \brief: tee_execute_test_case
 * 	execute testcase by TEE
 * \param tc: pointer to test-case object
 * \returns test-case status success/failure
 */
int tee_execute_test_case(struct test_case_t *tc)
{
#define XSTATE_TEE_INIT_TEST		XSTATE0
#define XSTATE_TEE_EXEC_TEST		XSTATE1
#define XSTATE_TEE_TEST_CMPLTD		XSTATE2
	uint8_t state;
	struct tee_t *tee;
	int i, retval = 1;

	if (tc->tee == NULL)
		return ERROR_INVARG;

	if (tc->tee->priv_data == NULL)
		return ERROR_INVARG;

	tee = tc->tee;
	state = tc->fn_state;

	switch (state) {
	case XSTATE_TEE_INIT_TEST:
		if (1 /*tc->initial_condition(tc)*/) {
			if (tee->cmd_req == NULL) {
				clog_print(CLOG_INFO, "==============CARD DETECTED (%d) ============\n",
					sd_card_is_inserted());
				tee->cmd_req = alloc_cmd_req(0);
			}
			if (tee->cmd_req) {
				state = XSTATE_TEE_EXEC_TEST;
				tee->cmd_req->state = XSTATE_CMD_INIT;
			}
		} else {
			/* no card present end the test-case */
			tc->status = TEST_STATUS_ABORT;
			state = XSTATE_TEE_INIT_TEST;
			break;
		}
		clog_print(CLOG_INFO, "%s: Executing test %s\n", __func__,
				tc->tc_id_name);
		for (i = 0; i < tc->param_len; ++i)
			clog_print(CLOG_INFO, "param[%d] = 0x%x\n", i, tc->param[i]);
		tc->status = TEST_STATUS_IN_PROGRESS;

		//break; /* avoid break purposefully, fix for coverity */

	case XSTATE_TEE_EXEC_TEST:
		retval = tee->ops.run_test_case(&tee->tc_list[tee->cur_tc]);
		if (tee->cmd_req->state == XSTATE_CMD_DONE) {
			tc->status = TEST_STATUS_COMPLETED;
			if (tee->cmd_req->cur_cmd.status == IO_STATUS_SUCCESS)
				tc->err_status = TEST_STATUS_SUCCESS;
			else
				tc->err_status = TEST_STATUS_FAILED;
			state = XSTATE_TEE_TEST_CMPLTD;
			cmd_req_reinit(tee->cmd_req);
		}
		break;
	default :
		break;
	}

	if (state == XSTATE_TEE_TEST_CMPLTD) {
		state = XSTATE_TEE_INIT_TEST;
		clog_print(CLOG_INFO, "%s: Test completed, status = %d\n", __func__,
				tc->status);
	}

	if (retval)
		clog_print(CLOG_LEVEL10, "%s: retval=%d\n", __func__, retval);

	tc->fn_state = state;
	return tc->status;
}

/** \brief: tee_core_task
 * 	TEE core task, execute execute each test-case
 * \param task_id: task id
 * \param args : task args pointer to driver data
 * \returns a status success/failure
 */
int tee_core_task(uint8_t task_id, void *args)
{
#define XSTATE_INIT      XSTATE0
#define XSTATE_EVAL_TC   XSTATE1
#define XSTATE_EXECUTE   XSTATE2
#define XSTATE_TERMINATE XSTATE3
	struct tee_t *tee;
	int state = ctask_get_fnstate(task_id);
	int nxt_state = state;
	int retval = 1;

	tee = (struct tee_t *)args;
	if (tee == NULL)
		return 0;

	switch(state) {
	case XSTATE_INIT: /* load the test case */
		nxt_state = XSTATE_EVAL_TC;
		tee_load_testcases(tee, tee->tc_cfg_file);
		tee->cur_tc = 1;
		if (tee->tc_list[tee->cur_tc].rep_cnt > 0)
			tee->tc_list[tee->cur_tc].ntimes = tee->tc_list[tee->cur_tc].rep_cnt - 1;
		clog_print(CLOG_INFO, "---- test_case(%d) ntimes(%d) ---------\n",
				tee->cur_tc, tee->tc_list[tee->cur_tc].ntimes+1);
		break;
	case XSTATE_EVAL_TC: /* execute test cases */
		if (tee->tc_list[tee->cur_tc].is_execute == 0) {
			tee->cur_tc++;
			if (tee->cur_tc > tee->max_num_tc) {
				nxt_state = XSTATE_TERMINATE;
				break;
			}
			if (tee->tc_list[tee->cur_tc].rep_cnt > 0)
				tee->tc_list[tee->cur_tc].ntimes = tee->tc_list[tee->cur_tc].rep_cnt - 1;
		} else
			nxt_state = XSTATE_EXECUTE;
		break;

	case XSTATE_EXECUTE: /* execute test cases */
		if (tee->cur_tc > tee->max_num_tc)
			nxt_state = XSTATE_TERMINATE;

		retval = tee_execute_test_case(&tee->tc_list[tee->cur_tc]);
		if (tee->tc_list[tee->cur_tc].status == TEST_STATUS_COMPLETED) {
			if (tee->tc_list[tee->cur_tc].ntimes > 0) {
				tee->tc_list[tee->cur_tc].ntimes--;
			} else {
				tee->cur_tc++;
				nxt_state = XSTATE_EVAL_TC;
				/*if (tee->tc_list[tee->cur_tc].rep_cnt > 0)
					tee->tc_list[tee->cur_tc].ntimes = tee->tc_list[tee->cur_tc].rep_cnt - 1; */
			}

			if (tee->cur_tc <= tee->max_num_tc) {
				clog_print(CLOG_INFO, "---- test_case(%d) ntimes(%d) ---------\n",
					tee->cur_tc, tee->tc_list[tee->cur_tc].ntimes+1);
			}
		}
		break;
	
	case XSTATE_TERMINATE:
		clog_print(CLOG_INFO, "=========== TEST COMPLETED ============\n");
		nxt_state = XSTATE_TERMINATE + 1;
		clog_closefile();
		break;
	default:
		break;
	}

	if (retval)
		clog_print(CLOG_LEVEL10, "%s: retval=%d\n", __func__, retval);

	ctask_set_fnstate(task_id, nxt_state);

	return 1;
}

/* \brief : start_tee
 * 	enable tee core task
 * \param tee: pointner to tee object
 * returns : returns none;
 */
int start_tee(struct tee_t *tee)
{

	if (!tee->task_init_done) {
		tee->task_id = ctask_create(tee->name, tee_core_task, tee, 0, 0, 0);
		tee->task_init_done = 1;
		ctask_enable(tee->task_id);
	} else
		ctask_resume(tee->task_id);

	return 0;
}

/* \brief : stop_tee
 * 	suspend the tee-core task
 * \param tee: pointner to tee object
 * returns : returns none;
 */
int stop_tee(struct tee_t *tee)
{
	ctask_suspend(tee->task_id);
	return 0;
}

/* \brief : read_config_data
 * 	read config parameter string
 * 	returns value of configuration parameter based on type;
 * \param inp_str: input config string
 * \param str_map: list of predefine parameter string
 * \param len : length parameter string table
 * \param type: type of variable
 * \param data_val: value of variable 
 * returns : returns value of configuration parameter based on type;
 */
int read_config_data(char *inp_str, struct str_map_t *str_map, uint8_t len,
	uint8_t type, void *data_val)
{
	char str[40];
	int i, retval, errflag = 0;
	uint32_t val;

	retval = get_config_data(inp_str, str, 40);
	if (retval != 0)
		return retval;

	if (str_map && len) {
		for (i = 0; i < len; ++i) {
			if (strcmp(str_map[i].str_val, str) == 0) {
				val = str_map[i].ival;
				printf("%s: %s = val=%u\n", __func__, str, val);
				break;
			}
		}
		if (i >= len) {
			clog_print(CLOG_ERR, "%s: %s Not Found\n", __func__, str);
			retval = -1;
		}
	} else {
		val = stoi(str, &errflag);
		if (errflag != 0)
			retval = -1;
	}

	if (retval == 0) {
		switch (type) {
		case XUINT8 : *(uint8_t *)data_val = val; break;
		case XUINT16 : *(uint16_t *)data_val = val; break;
		case XUINT32 : *(uint32_t *)data_val = val; break;
		default :
			retval = -1;
			break;
		}
	}

	if (retval != 0)
		clog_print(CLOG_ERR, "failed to read %s\n", inp_str);

	return retval;
}

/* \brief : load_testcases from test case file
 * 	free the tee object
 * @param :
 * returns : returns none;
 */
int tee_load_testcases(struct tee_t *tee, char *tc_cfg_file)
{
	int i, retval = 0, tc_i;
	int errflag;

	if (tee == NULL || tc_cfg_file == NULL)
		return ERR_INVARG;

	clog_print(CLOG_INFO, "reading %s file\n", tc_cfg_file);
	retval = read_config_file(tc_cfg_file);
	if (retval < 0) {
		clog_print(CLOG_ERR, "reading %s failed, error: %d\n", tc_cfg_file, retval);
	}

	/* print the contents of test-case cfg file */
	//print_dictionary(1);

	tee->max_num_tc = 0;
	memset(&tee->tc_list[0], 0, sizeof(struct test_case_t) * MAX_NUM_TESTCASES);
	for (tc_i = i = 0; i < dict_len; ++i) {
		if (strcmp(dict[i].w1.name, "TC_SEQ_NUM") == 0) {
			if (tc_i)
				tee->ops.prepare_test_case(&tee->tc_list[tc_i]);
			tc_i++;
			tee->tc_list[tc_i].tc_num = stoi(dict[i].w2.name, &errflag);
			tee->tc_list[tc_i].tee = tee;
		} else if (strcmp(dict[i].w1.name, "TC_DESC") == 0)
			strncpy(tee->tc_list[tc_i].desc, dict[i].w2.name, 80);
		else if (strcmp(dict[i].w1.name, "TC_ID") == 0)
			strcpy(tee->tc_list[tc_i].tc_id_name, dict[i].w2.name);
		else if (strncmp(dict[i].w1.name, "TC_PARAM", 8) == 0)
			tee->ops.update_tc_param(&tee->tc_list[tc_i], &dict[i]);
		else if (strcmp(dict[i].w1.name, "TC_EXEC") == 0) {
			tee->tc_list[tc_i].is_execute = is_yes((dict[i].w2.name[0])) ? 1 : 0;
			clog_print(CLOG_INFO, "tc(%d) is_excute=%d\n", tc_i, tee->tc_list[tc_i].is_execute);
		} else if (strcmp(dict[i].w1.name, "TC_REPCNT") == 0)
			tee->tc_list[tc_i].rep_cnt = stoi(dict[i].w2.name, &errflag);
		else if (strcmp(dict[i].w1.name, "TC_EXEC_NXT") == 0)
			tee->tc_list[tc_i].next_tc = stoi(dict[i].w2.name, &errflag);
	}
	if (tc_i)
		tee->ops.prepare_test_case(&tee->tc_list[tc_i]);

	tee->max_num_tc = tc_i;

	for (i = 1; i <= tee->max_num_tc; ++i)
		printf("[%d], test_fn(%p)\n", i, tee->tc_list[i].test_fn);
	return retval;
}
