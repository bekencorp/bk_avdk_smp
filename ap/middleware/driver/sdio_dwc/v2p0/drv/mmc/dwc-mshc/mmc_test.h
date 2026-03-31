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
* \file		: mmc_test.h
* \author	: ravibabu@synopsys.com
* \date		: 23-Dec-2019
* \brief	: mmc_test.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	23-Dec-2019	ravibabui@synopsys.com  001		dev in progress
*/


#ifndef _MMC_TEST_H_
#define _MMC_TEST_H_

#include "dwc_type.h"
#include "tee.h"

#define MAX_MMC_TESTCASE	256

/* test case APIs */
extern int tc_mmc_cmd0_test_set_card_idle(void *tc_data);
extern int tc_mmc_cmd8_send_if_cond(void *tc_data);
extern int tc_mmc_acmd41_send_op_cond(void *tc_data);
extern int tc_mmc_device_enumerate(void *tc_data);
extern int tc_emmc_device_enumerate(void *tc_data);
extern int tc_mmc_read_block(void *tc_data);
extern int tc_mmc_write_block(void *tc_data);
extern int tc_mmc_mshc_read_version(void *tc_data);

/* List of tescases data structure supported by mmc-test module */
struct tc_desc_t mmc_tc_desc[MAX_MMC_TESTCASE] = {
    /* TEST-CASE-1 */
    {
        /* Test case name */
        "TC_MHSC_MMC_CMD0_SET_CARD_IDLE",
        /* number of param */
        1,
        /* parameter list */
        {
            {"BOOTMODE", XUINT8}, /* param-1 */
        },
        /* test case function */
        tc_mmc_cmd0_test_set_card_idle,
    },
    /* TEST-CASE-2 */
    {
        /* Test case name */
        "TC_MSHC_MMC_CMD8_SEND_IF_COND",
        /* number of param */
        2,
        /* parameter list */
        {
            {"CHKPAT", XUINT8}, /* param-1 */
            {"VOLT_SUPPLIED", XUINT8}, /* param-1 */
        },
        /* test case function */
        tc_mmc_cmd8_send_if_cond,
    },
    /* TEST-CASE-3 */
    {
        /* Test case name */
        "TC_MSHC_MMC_ACMD41_CHK_SD_OP_COND",
        /* number of param */
        4,
        /* parameter list */
        {
            {"SDXC_SUPPORT", XUINT8}, /* param-1 */
            {"USE_1P8V", XUINT8}, /* param-1 */
            {"IS_MAX_PERF", XUINT8}, /* param-1 */
            {"IS_OCR", XUINT8}, /* param-1 */
        },
        /* test case function */
        tc_mmc_acmd41_send_op_cond,
    },
    /* TEST-CASE-4 */
    {
        /* Test case name */
        "TC_MSHC_MMC_DEV_ENUMERATE",
        /* number of param */
        6,
        /* parameter list */
        {
            {"SPEED_MODE", XUINT8}, /* param-1 */
            {"BUS_WIDTH", XUINT8},	/* param-2 */
            {"XFER_MODE", XUINT8}, 	/* param-3 */
            {"EMMC_VDD", XUINT8}, 	/* param-4 */
            {"MMCM_CLOCK", XUINT32},/* param-5 */
            {"USR_INTF", XUINT8}, /* param-6 */
        },
        /* test case function */
        tc_mmc_device_enumerate,
    },
    /* TEST-CASE-5 */
    {
        /* Test case name */
        "TC_MSHC_EMMC_DEV_ENUMERATE",
        /* number of param */
        6,
        /* parameter list */
        {
            {"SPEED_MODE", XUINT8}, /* param-1 */
            {"BUS_WIDTH", XUINT8},	/* param-2 */
            {"XFER_MODE", XUINT8}, 	/* param-3 */
            {"EMMC_VDD", XUINT8}, 	/* param-4 */
            {"MMCM_CLOCK", XUINT32},/* param-5 */
            {"USR_INTF", XUINT8}, /* param-6 */
        },
        /* test case function */
        tc_emmc_device_enumerate,
    },
    /* TEST-CASE-6 */
    {
        /* Test case name */
        "TC_MSHC_MMC_READ_BLOCK",
        /* number of param */
        2,
        /* parameter list */
        {
            {"BLOCK_ADR", XUINT8}, /* param-1 */
            {"BLOCK_CNT", XUINT8},	/* param-2 */
        },
        /* test case function */
        tc_mmc_read_block,
    },
    /* TEST-CASE-7 */
    {
        /* Test case name */
        "TC_MSHC_MMC_WRITE_BLOCK",
        /* number of param */
        4,
        /* parameter list */
        {
            {"BLOCK_ADR", XUINT8}, /* param-1 */
            {"BLOCK_CNT", XUINT8},	/* param-2 */
            {"DATA_USER_PAT", XUINT8},/* param-3 */
            {"USE_RAND_PAT", XUINT8},/* param-3 */
        },
        /* test case function */
        tc_mmc_write_block,
    },
    /* TEST-CASE-8 */
    {
        /* Test case name */
        "TC_MSHC_VERSION_READ",
        /* number of param */
        0,
        /* parameter list */
        {
            {"", XUINT8},
        },
        /* test case function */
        tc_mmc_mshc_read_version,
    },
    /* TEST-CASE-7 */
};

#endif /* _MMC_TEST_H_ */
