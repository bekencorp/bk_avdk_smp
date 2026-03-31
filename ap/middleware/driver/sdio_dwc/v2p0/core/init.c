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
* \file		: init.c
* \author	: ravibabu@synopsys.com
* \date		: 10-July-2019
* \brief	: init.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "dwc_type.h"
#include "ctask.h"
#include "error.h"
#include "fs.h"
#include "clog.h"
#include "terminal.h"
#include "cmsg_queue.h"
#include "cwatch.h"
#include "clink.h"
#include "cee.h"
#include "clkcore.h"
#include <os/mem.h>

int ctest_core_init_done = 0;
extern void stdio_terminal_init(void);

void *sm_queue_addr;
uint32_t sm_qmem_size;
int shmid;

/*
 * \brief: os_sm_alloc linux-os-dependent shared mem alloc
 *	shared memory allocate allocate queue memory
 * param flags: MsgQueue flags
 *	 use QUEUE_ALLOC_SM flag for shared memory
 * param size_bytes: queue memory size in bytes
 * returns : address to MsgQueue Memory or NULL
 */
void *os_sm_alloc(uint16_t sm_id, uint32_t size_bytes, int *shm_id)
{
    void *addr = os_malloc(size_bytes);

    *shm_id = 0xdead0001;
    return addr;
}

/**
 * \brief ctest_core_init
 *	initialize all core modules
 * \param : none
 */
void ctest_core_init(void)
{
    uint32_t flags;

    if (ctest_core_init_done)
        return ;

    set_clog_output(CLOG_STDIO);

    set_clog_level(CLOG_WARN);

    /* initialize the queue module */
    flags = 0;
    #ifdef CONFIG_MSHC_BUILD
    flags = QUEUE_CTRLF_RESET_ALL;
    #endif
    sm_qmem_size = 5 * 1024;
    sm_queue_addr = os_sm_alloc(QUEUE_SM_ID, sm_qmem_size, &shmid);
    if ((void *)sm_queue_addr == (void *)-1) {
        clog_print(CLOG_ERR, "shared memory allocation failed\n");
        return ;
    }
    /* cmsg_q module init */
    cmsg_q_module_init(flags);

    /* shared msg_q module init */
    sm_cmsg_q_module_init(flags, sm_queue_addr, sm_qmem_size);

    /* initialize cmd-execution engine */
    cee_init(CLOG_WARN);

    /* initialize ctask module */
    #ifdef CONFIG_CTASK_LINUX
    ctask_linux_os_register();

    /* file system support init */
    fs_linux_init();
    #elif CONFIG_CTASK_FREERTOS
    ctask_freertos_os_register();
    #else
    /* register ctask function based schedular */
    ctask_no_os_register();
    #endif

    #ifdef CONFIG_CLINK
    /* initialize the clink module */
    clink_init();
    #endif
    cwatch_init();

    #ifdef CONFIG_TERMINAL
    terminal_core_init();
    terminal_register_cmd("ql", cmsg_q_print);
    terminal_register_cmd("cl", print_clink_cnx);
    #endif
    /* initialize the clock module */
    init_clk_module();

    ctest_core_init_done = 1;

    clog_print(CLOG_INFO, "ctest core initialization done\n");
}

/**
 * \brief ctest_core_init
 *	initialize all core modules
 * \param : none
 */
void ctest_shut_down(void)
{
    ctask_shut_down();
    exit(0);
}
