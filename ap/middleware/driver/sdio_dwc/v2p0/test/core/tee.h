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
* \file		: tee.h
* \author	: ravibabu@synopsys.com
* \date		: 20-Feb-2020
* \brief	: tee.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	20-Feb-2020	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _TEE_H_
#define _TEE_H_

#include "dwc_type.h"
#include "parser.h"

#define MAX_NUM_TEE		2
#define MAX_NUM_TESTCASES	16
#define MAX_TC_PARAMS		8

#define TEST_STATUS_IN_PROGRESS	1
#define TEST_STATUS_SUCCESS	2
#define TEST_STATUS_FAILED	3
#define TEST_STATUS_ABORT	4
#define TEST_STATUS_COMPLETED	5

struct tee_t;
struct test_case_t;

struct test_case_t {
	struct tee_t *tee;

	/* test case description */
	char desc[80];
	char tc_id_name[40];
	uint8_t fn_state;

	/* test case number */	
	uint16_t tc_num;

	/* log file */
	char log_file[40];

	/* test category type */
	uint8_t category;

	/* input parameters to test case */
	char cmd_str[40];
	uint32_t param[16];
	uint32_t param_len;
	int (*test_fn)(void *tc);

	/*  execution status of test case */
	uint8_t status;
	uint8_t err_status;
	uint32_t freq_cnt;

	/* next test csse to be executed */
	uint16_t rep_cnt;
	uint16_t ntimes;
	uint8_t is_execute;
	uint16_t next_tc;
};

struct tee_ops_t {
	void *priv_data;
	struct test_case_t *drv_tc;
	struct cmd_param_t *(*alloc_cmd_req)(struct test_case_t *tc);
	int (*prepare_test_case)(struct test_case_t *tc);
	int (*run_test_case)(struct test_case_t *tc);
	int (*get_status)(struct test_case_t *tc);
	int (*release_test_case)(struct test_case_t *tc);
	int (*update_tc_param)(struct test_case_t *tc, struct key_val_t *param);
	struct hal_obj_t* (*get_hal_obj)(void *drv_data, addr_t *io_mem);
	int (*select_device)(void *drv_data, uint8_t is_emmc);
};

struct tee_t {
	/* name of driver register with tee, which support test-drv supporting list of testcases
	 * to be executed by tee */
	char name[40];
	char tc_cfg_file[40];
	void *priv_data;
	struct cmd_param_t *cmd_req;

	/* log file */
	char log_file[40];

	/* tee ops */
	struct tee_ops_t ops;

	/* maximum number of test cases */
	uint32_t max_num_tc;
	struct test_case_t tc_list[MAX_NUM_TESTCASES];

	/* current test case under execution */
	uint16_t cur_tc;

	/* version info */
	char ip_ver[40];
	char sw_ver[40];
	char board_info[80];

	uint8_t allocated;
	uint8_t task_init_done;
	uint8_t task_id;
};

struct tc_param_t {
	char name[40];
	uint8_t type;
};

struct tc_desc_t {
	char tc_id_name[40];
	uint8_t tc_max_param;
	struct tc_param_t param[MAX_TC_PARAMS];
	int (*test_fn)(void *tc);
};

/* \brief : tee_init
 * 	initialize tee module
 * @param :
 * returns : returns 0 on success
 */
int tee_init(void);

/* \brief : alloc_tee
 * 	allocate tee object
 * \param name: name of TEE
 * returns : returns pointer to tee;
 */
struct tee_t *alloc_tee(char *name);

/* \brief : free_tee
 * 	free the tee object
 * \param tee: pointer to tee object
 * returns : returns none;
 */
void free_tee(struct tee_t *tee);

/** \brief: tee_execute_test_case
 * 	execute testcase by TEE
 * \param tc: pointer to test-case object
 * \returns test-case status success/failure
 */
int tee_load_testcases(struct tee_t *tee, char *tc_cfg_file);

/* \brief : register_tee
 * 	rgister with tee
 * \param name: name of TEE
 * \param ops : TEE opeations
 * \param tc_cfg_file: test case cfg file
 * \returns : returns pointer to tee object
 */
struct tee_t *register_tee(char *name, struct tee_ops_t *ops, char *tc_cfg_file);

/* \brief : start_tee
 * 	enable tee core task
 * \param tee: pointner to tee object
 * returns : returns none;
 */
int start_tee(struct tee_t *tee);

/* \brief : stop_tee
 * 	suspend the tee-core task	
 * \param tee: pointner to tee object
 * returns : returns none;
 */
int start_stop(struct tee_t *tee);

/** \brief: mmc_update_tc_param
 *	update the test-parameter to test-case parameter fields
 * \param tc: pointer to testcase
 * \param param: parameter key/value pair.
 * \return 0 on success, -ve error on failure
 */
int mmc_update_tc_param(struct test_case_t *tc, struct key_val_t *param);

/** \brief: mmc_test_tee_init
 *	initialize mmc-test module and register to TEE
 * \param sd_card: pointer to sd_card
 * \param tc_cfg_file: testcase configuration input file
 * \return 0 on success, -ve error on failure
 */
int mmc_test_tee_init(struct sd_card_t *sd_card, char *tc_cfg_file);
uint32_t stoi(char *str, int *errflag); 
#endif
