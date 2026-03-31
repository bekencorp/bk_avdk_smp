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
* \file		: mshc_main.c
* \author	: ravibabu@synopsys.com
* \date		: 15-Sep-2019
* \brief	: mshc_main.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	15-Sep-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "cee.h"
#include "ctask.h"
#include "cmsg_queue.h"
#include "clog.h"
#include "cwatch.h"
#include "clog.h"
#include "clink.h"

#include "mmc_dev.h"
#include "sdhci.h"
#include "sd_cmd.h"
#include "dwc_msdc_cfg.h"
#include "mmc_core.h"
#include "sd_card.h"
#include "mmcm_clk.h"
#include "tee.h"

#define MAX_NUM_ARGS	18	
struct arg_t {
	char opt[16];
	char sval[16];
	uint8_t type;
	void *ival;
	uint8_t defval;
};

uint8_t max_num_cmdargs;

/* globals */
/* \brief: mmc_dev0 
 * 	mmc_dev context structure
 */
struct mmc_dev_t *mmc_dev0; 

/* \brief: sd_card0 
 * 	sd_card_t context structure for mmcsd/eMMC card
 */
struct sd_card_t *sd_card0;

/* selected os */
char os_cfg[16];

/** debug log file */
char log_file[16];

/* \brief: sdhci_0
 * 	sdhci_0 driver instance
 */
extern int clink_terminal_server_module_init(void);
extern int mmc_dev_set_usr_state(struct sd_card_t *sd_card, uint8_t state);

/* module specific parameters */
extern uint8_t tx_phase;
extern uint8_t rx_phase;
/* below params are passed from API, below module param are not used,TBD */
extern uint8_t speed_mode;
extern uint8_t bus_width;
extern uint8_t xfer_mode;
extern uint8_t emmc_vdd;
extern uint8_t mmcm_clock;
extern uint8_t is_emmc_dev;
extern uint32_t g_block_addr;
extern uint32_t g_block_cnt;

uint8_t debug_level = 2;
uint8_t is_usr_intf = 1;

char tc_cfg_file[16];

/* application input argument table */
struct arg_t mshc_args[MAX_NUM_ARGS];


/* application input argument table */
struct arg_t mshc_args[MAX_NUM_ARGS] = {
	{"-d", 		"", XUINT8, &debug_level, 2},
	{"--speed_mode","", XUINT8, &speed_mode, 0},
	{"--xfer_mode", "", XUINT8, &xfer_mode, 1},
	{"--bus_width", "", XUINT8, &bus_width, 4},
	{"--vdd", 	"", XUINT8, &emmc_vdd, 1},
	{"--mmcm_clk", 	"", XUINT8, &mmcm_clock, 100},
	{"--tx_phase", 	"", XUINT8, &tx_phase, 0x3f},
	{"--rx_phase", 	"", XUINT8, &rx_phase, 0},
};

int mmc_set_exit_state(void)
{
        return mmc_dev_set_usr_state(sd_card0, XSTATE_FINISH);
}

int mmc_set_rd_block_state(void)
{
        return mmc_dev_set_usr_state(sd_card0, XSTATE_READ_ONE_BLOCK);
}

int mmc_set_wr_block_state(void)
{
        return mmc_dev_set_usr_state(sd_card0, XSTATE_WRITE_ONE_BLOCK);
}

int mshc_read_block(uint32_t block_addr, uint32_t block_cnt)
{
        g_block_addr = block_addr;
        g_block_cnt = block_cnt;
        mmc_set_rd_block_state();

        return 0;
}

int mshc_write_block(uint32_t block_addr, uint32_t block_cnt)
{
        g_block_addr = block_addr;
        g_block_cnt = block_cnt;
        mmc_set_wr_block_state();

        return 0;
}

int mshc_exit_enumerate(void)
{
        mmc_set_exit_state();

        return 0;
}

/**
 * \brief mmc_app_task
 *	Thsi task enumerate the sdcard or eMMC card
 * \param task_id: task id
 * \param args: task private data 
 * \returns : 
 */
int mmc_app_task(uint8_t task_id, void *args)
{
	int state = ctask_get_fnstate(task_id);
	int nxt_state = state;
    int ret;

	switch(state) {
	case XSTATE0:
		nxt_state = XSTATE1;

		/* if there test-input configuration is given
		 * then test cases executed from TEE 
		 */
		if(strlen(tc_cfg_file) > 0)
			break;

		if (is_emmc_dev){
			ret = emmc_dev_enumerate(sd_card0, speed_mode, bus_width,
				xfer_mode, emmc_vdd, mmcm_clock, is_usr_intf);
        }
		else{
			ret = mmc_dev_enumerate(sd_card0, speed_mode, bus_width,
				xfer_mode, emmc_vdd, mmcm_clock, is_usr_intf);
			if(1 != ret){
				return 0;
			}
		}
 
		nxt_state = XSTATE0;
		break;

	case XSTATE1:
		break;

	default:
		break;
	}

	ctask_set_fnstate(task_id, nxt_state);

	return 1; /* return 1 alwyas */
}

/**
 * \brief set_value_int
 *	assign value of specified type to variable
 * \param addr: address of variable
 * \param val_str: value of data in string
 * \param type: type of data
 * \returns 0 on success -ve on error
 */
int set_value_int(void *addr, uint32_t val, uint8_t type)
{
	int retval = 0;
	if (addr == NULL)
		return ERR_INVARG;

	switch(type) {
	case XUINT8:
		*(uint8_t *)addr = val;
		break;
	case XUINT16:
		*(uint16_t *)addr = val;
		break;
	case XUINT32:
		*(uint32_t *)addr = val;
		break;
	default:
		retval = ERR_INVARG;
		break;
	}
	return retval;
}

/**
 * \brief set_val
 *	assign value of specified type to variable
 * \param addr: address of variable
 * \param val_str: value of data in string
 * \param type: type of data
 * \returns 0 on success -ve on error
 */
int set_value_str(void *addr, char *val_str, uint8_t type)
{
	int err = 0;
	if (addr == NULL || val_str == NULL)
		return ERR_INVARG;

	switch(type) {
	case XUINT8:
		*(uint8_t *)addr = stoi(val_str, &err);
		break;
	case XUINT16:
		*(uint16_t *)addr = stoi(val_str, &err);
		break;
	case XUINT32:
		*(uint32_t *)addr = stoi(val_str, &err);
		break;
	case XSTRING:
                /* FIXME:unsafe*/
		strcpy(addr, val_str);
		break;
	default:
		err = ERR_INVARG;
		break;
	}
	return err;
}

/**
 * \brief app_cmdarg_add_option
 *	add cmdline options
 * \param none
 * \returns 0 on success -ve on error
 */
int app_cmdarg_add_option(char *opt_str, uint8_t type, void *addr, uint32_t val)
{
	int i = max_num_cmdargs;

	if (i >= MAX_NUM_ARGS)
		return ERROR_OPER_FAIL;

        /* FIXME:unsafe*/
	strcpy(mshc_args[i].opt, opt_str);
	mshc_args[i].type = type;
	mshc_args[i].ival = addr;
	mshc_args[i].defval = val;
	set_value_int(addr, val, type);
	max_num_cmdargs = i + 1;

	return 0;
}

/**
 * \brief mmc_app_task
 *	Thsi task enumerate the sdcard or eMMC card
 * \param task_id: task id
 * \param args: task private data
 * \returns 0 on success -ve on error
 */
int read_input_args(int argc, char *argv[], struct arg_t *args, int max_args)
{
	int i, j;

	for (i = 1; i < argc; ++i) {
		for (j = 0; j < max_args; ++j) {
			if (strcmp(argv[i], args[j].opt) == 0) {
				i++;
				if (i > argc)
					return -1;
				set_value_str(args[j].ival, argv[i],
					args[j].type);
				break;
			}
		} /* for */
		if (j >= max_args)
			return -1;
	} /* for */
	return 0;
}

/**
 * \brief app_cmdarg_init
 *	initialze cmdarg module
 * \param none
 * \returns 0 on success
 */
int app_cmdarg_init(struct arg_t *args, uint8_t max_args)
{
	max_num_cmdargs = 0;
	memset(args, 0, sizeof(struct arg_t) * max_args);
	return 0;
}

/**
 * \brief mshc_app_cmdarg_init
 *	initialze cmdarg module
 * \param none
 * \returns 0 on success
 */
int mshc_app_cmdarg_init(void)
{
	int retval;

	retval = app_cmdarg_init(mshc_args, MAX_NUM_ARGS);

	app_cmdarg_add_option("-d", 		XUINT8, &debug_level, 2);
	app_cmdarg_add_option("-t", 		XSTRING,tc_cfg_file, 0);
	app_cmdarg_add_option("--speed_mode",	XUINT8, &speed_mode, speed_mode);
	app_cmdarg_add_option("--xfer_mode", 	XUINT8, &xfer_mode, xfer_mode);
	app_cmdarg_add_option("--bus_width", 	XUINT8, &bus_width, bus_width);
	app_cmdarg_add_option("--vdd", 		XUINT8, &emmc_vdd, emmc_vdd);
	app_cmdarg_add_option("--mmcm_clk", 	XUINT8, &mmcm_clock, mmcm_clock);
	app_cmdarg_add_option("--tx_phase", 	XUINT8, &tx_phase, tx_phase);
	app_cmdarg_add_option("--rx_phase", 	XUINT8, &rx_phase, rx_phase);
	app_cmdarg_add_option("--emmc", 	XUINT8, &is_emmc_dev, is_emmc_dev);
	app_cmdarg_add_option("--l", 		XSTRING, &log_file, 0);

	return retval;
}

/**
 * \brief mshc_cmd_help
 *	prints the mshc cmd usage
 * \param none
 * \returns 0 on success -ve on error
 */
void print_mshc_cmd_usage(int retval)
{
	if (retval)
		clog_print(CLOG_ERR, "Invalid Arguments\n");

	clog_print(CLOG_INFO, "Usage: mshc_main [<options>]\n");
	clog_print(CLOG_INFO, "\t-d <debug-level>  : set debug level 1 to 9\n");
	clog_print(CLOG_INFO, "\t-t <test-cfg.txt> : test configuration file\n");
	clog_print(CLOG_INFO, "\t--speed_mode <1..6> : speed modes\n");
	clog_print(CLOG_INFO, "\t\t	DS/SRD12(Max Clock 25Mhz)	- 0\n");
	clog_print(CLOG_INFO, "\t\t	HS/SRD25(Max Clock 50Mhz)       - 1\n");
	clog_print(CLOG_INFO, "\t\t	SRD50(Max Clock 100Mhz)	        - 2\n");
	clog_print(CLOG_INFO, "\t\t	SRD104(Max Clock 200Mhz)	- 3\n");
	clog_print(CLOG_INFO, "\t\t	DDR50(Max Clock 50Mhz)	        - 4\n");
	clog_print(CLOG_INFO, "\tfor emmc speed_mode\n");
	clog_print(CLOG_INFO, "\t\t	DefSpeed(25Mhz)	- 0\n");
	clog_print(CLOG_INFO, "\t\t	HighSpeed(50Mhz)- 1\n");
	clog_print(CLOG_INFO, "\t\t	HS200(200Mhz)	- 2\n");
	clog_print(CLOG_INFO, "\t--xfer_mode <0/1/2> : PIO/SDMA/ADMA respectively (only PIO supported)\n");
	clog_print(CLOG_INFO, "\t--bus_width <0/1/2> : 1/4/8 bit buswidth repectively\n");
	clog_print(CLOG_INFO, "\t--vdd <0/1>	   : 3.3/1.8V\n");
	clog_print(CLOG_INFO, "\t--mmcm_clk <80/160/320>: 80Mhz, 160Mhz, 320Mhz\n");
	clog_print(CLOG_INFO, "\t--tx_phase <0..127> : default 63\n");
	clog_print(CLOG_INFO, "\t--rx_phase <0..127> : default 0\n");
	clog_print(CLOG_INFO, "\t--emmc <1> : for emmc device\n");
	clog_print(CLOG_INFO, "\t--stop_at_cmd <1> : stop and ask user input to proceed to next cmd\n");
	clog_print(CLOG_INFO, "\t--l <log-file> : log the debug output specified file\n");
}

/**
 * \brief app_mshc_cmdarg_validate
 *	validate cmdline args
 * \param none
 * \returns 0 on success -ve on error
 */
int app_mshc_cmdarg_validate(void)
{
	int mmcm_clk[] = {53, 80, 160, 320};
	int i, retval = 0;

	if (debug_level >= 10) {
		clog_print(CLOG_ERR, "Invalid debug_level value\n");
		retval = ERR_INVARG;
	}

	if (speed_mode >= 5) {
		clog_print(CLOG_ERR, "Invalid speed_mode value\n");
		retval = ERR_INVARG;
	}

	if (xfer_mode != 0) {
		clog_print(CLOG_ERR, "Invalid xfer_mode value\n");
		retval = ERR_INVARG;
	}

	if (emmc_vdd > 3) {
		clog_print(CLOG_ERR, "Invalid emmc_vdd value\n");
		retval = ERR_INVARG;
	}

	for (i = 0; i < 3; ++i)
		if (mmcm_clk[i] == mmcm_clock)
			break;
	if (i >= 3) {
		clog_print(CLOG_ERR, "Invalid mmcm_clock value\n");
		retval = ERR_INVARG;
	}

	if (bus_width > 3) {
		clog_print(CLOG_ERR, "Invalid bus_width value\n");
		retval = ERR_INVARG;
	}

	if (tx_phase > 127) {
		clog_print(CLOG_ERR, "Invalid tx_phase value\n");
		retval = ERR_INVARG;
	}

	if (rx_phase > 127) {
		clog_print(CLOG_ERR, "Invalid rx_phase value\n");
		retval = ERR_INVARG;
	}

	return retval;
}


/**
 * \brief app_main
 *	application main, 
 *		- initialize the ctest core modules,
 *		- iniitalize mmc and sdhci driver modules
 *		- initialize the msdc modules
 *		- initialize platform csim-link
 *		- run the application and schedular in loop
 * \param : 
 * \returns : 
 */
int app_main(int level)
{
    bk_err_t ret;
    uint32_t instance_id = 0;

	/* ctest core modules init, first function to be called */
	ctest_core_init();

#ifdef CONFIG_TERMINAL_STDIO
	stdio_terminal_init();
#else
#ifdef CONFIG_TERMINAL_VIO
	clink_terminal_server_module_init();
	set_clog_output(CLOG_TERMINAL);
#endif
#endif
	if (level >= 0)
		set_clog_level(level);

	clog_print(CLOG_INFO, "mmc sample user space application\n\n");

	/* initialize mmc modules */
	mmc_module_init();

	/* initialize sdhci driver init */
	sdhci_module_init(instance_id);

	/* allocate mmc device instance */
	mmc_dev0 = mmc_dev_new(0);
	if (mmc_dev0 == NULL) {
		clog_print(CLOG_LEVEL5, "unable to allocated mmc device\n");
		return ;
	}

	ret = rtos_init_semaphore(&mmc_dev0->cmd_sync, 1);
	if (BK_OK != ret) {
		return ERROR_OPER_FAIL;
	}

	/* mmc card module initialization */
	sd_card0 = sd_card_alloc(0);
	if (sd_card0 == NULL) {
		clog_print(CLOG_LEVEL5, "unable to allocated mmc card\n");
		return ;
	}
	sd_card0->mmc_dev = mmc_dev0;
	mmc_dev0->card = sd_card0;

	mmc_core_init(mmc_dev0, &sdhci_0->host);

#ifdef CONFIG_CTASK_LINUX
	strcpy(os_cfg, "ctask-linux");
#elif CONFIG_CTASK_FREERTOS
        /* FIXME:unsafe*/
	strcpy(os_cfg, "ctask-freertos");
#else
	strcpy(os_cfg, "ctask-fsched");
#endif
	add_to_cwatch("os_select", os_cfg, XSTRING);

	ctask_enable(ctask_create("mmc-app-task", 
                                        mmc_app_task,
                			mmc_dev0, 0, 0, 0));

	/* plugin mshc-test driver to TEE to support test automation */
	sd_card0->fn_state = 0;
	mmc_test_tee_init(sd_card0, tc_cfg_file);
}

/**
 * \brief main 
 *	entry to application main
 * \param : 
 * \returns : 
 */
int mshc_main(int argc, char *argv[])
{
	int retval;

	set_clog_output(CLOG_STDIO);
	set_clog_level(10);

	/* initialize cmdarg module */
	app_cmdarg_init(mshc_args, MAX_NUM_ARGS);
	mshc_app_cmdarg_init();

	if (argc > 1) {
		if (strcmp(argv[1], "--help") == 0) {
			print_mshc_cmd_usage(0);
			return 0;
		}
	}

	retval = read_input_args(argc, argv, mshc_args, MAX_NUM_ARGS);
	if (retval < 0) {
		print_mshc_cmd_usage(retval);
		return 0;
	}
	retval = app_mshc_cmdarg_validate();
	if (retval < 0) {
		print_mshc_cmd_usage(retval);
		return 0;
	}

	set_clog_file(log_file);

	clog_print(CLOG_INFO, "module param: tx_phase = %d\n", tx_phase);
	clog_print(CLOG_INFO, "module param: rx_phase = %d\n", rx_phase);
	clog_print(CLOG_INFO, "module param: speed_mode = %d\n", speed_mode);
	clog_print(CLOG_INFO, "module param: bus_width = %d\n", bus_width);
	clog_print(CLOG_INFO, "module param: xfer_mode = %d\n", xfer_mode);
	clog_print(CLOG_INFO, "module param: emmc_vdd = %d\n", emmc_vdd);
	clog_print(CLOG_INFO, "module param: mmcm_clock = %d\n", mmcm_clock);
	clog_print(CLOG_INFO, "module param: debug_level = %d\n", debug_level);
	clog_print(CLOG_INFO, "module param: is_emmc_dev = %d\n", is_emmc_dev);

	app_main(debug_level);

	return 0;
}
