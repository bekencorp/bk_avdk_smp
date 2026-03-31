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
* \file		: clkcore.c
* \author	: ravibabu@synopsys.com
* \date		: 10-Jan-2020
* \brief	: clkcore.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "dwc_type.h"
#include "clog.h"
#include "clkcore.h"
#include "os/str.h"

struct clk_mod_t clk_mod_list[MAX_NUM_CLK_MODULE];

/**
 * \brief alloc_clk_module
 * 	allocate clink_mod data structure and initialize
 * \param name: clink_mod name
 * returns: pointer to new clink_mod structure, NULL on fail to create.
 */
struct clk_mod_t *alloc_clk_module(char *name)
{
    int i;

    /* allocate free clink_mod descriptor */
    for (i = 0; i < MAX_NUM_CLK_MODULE; ++i)
        if (clk_mod_list[i].allocated == 0)
            break;

    if (i >= MAX_NUM_CLK_MODULE)
        return NULL;

    /* clear clink_mod data structure */
    memset(&clk_mod_list[i], 0, sizeof(clk_mod_list[i]));

    /* initializ other clink_mod members */
    if (name) {
        int src_len = os_strlen(name);
        int dest_len = sizeof(clk_mod_list[i].name);
        int cnt = MIN(src_len, dest_len);

        strncpy((void *)clk_mod_list[i].name, name, cnt - 1);
    }

    clk_mod_list[i].allocated = 1;

    return &clk_mod_list[i];
}

/**
 * \brief free_clink_mod
 * 	free clink_mod data structure
 * \param clink_mod: pointer to clink_mod to be released
 * returns: none
 */
void free_clk_mod(struct clk_mod_t *clink_mod)
{
    /* free clink_mod descriptor */
    clink_mod->allocated = 0;
}

/**
 * \brief init_clk_module
 * 	initialize clock module
 * \param none
 * returns: none
 */
void init_clk_module(void)
{
    int i;

    /* allocate free clink_mod descriptor */
    for (i = 0; i < MAX_NUM_CLK_MODULE; ++i) {
        /* clear clink_mod data structure */
        memset(&clk_mod_list[i], 0, sizeof(clk_mod_list[i]));
    }

    return ;
}

/**
 * \brief register_clk_module
 * 	register clock module
 * \param name: name of clock module
 * \param hal: pointer to hal object
 * \param io_mem: io-address of clock module
 * \param ops: clock ops
 * returns: pointer to clock_module
 */
struct clk_mod_t *register_clk_module(char *name, struct hal_obj_t *hal,
                                      void *io_mem, struct clk_ops_t *ops)
{
    struct clk_mod_t *clk_mod;

    /* allocate clock module */
    clk_mod = alloc_clk_module(name);
    if (clk_mod == NULL)
        return NULL;

    /* assing the clock ops */
    if (ops) {
        clk_mod->ops.init = ops->init;
        clk_mod->ops.enable = ops->enable;
        clk_mod->ops.disable = ops->disable;
        clk_mod->ops.deinit = ops->deinit;
    }

    /* initialize the hal object for reg i/o access */
    clk_mod->hal = hal;
    clk_mod->io_mem = (uint32_t *)io_mem;

    /* return registerd clock module */
    return clk_mod;
}

/**
 * \brief clock_init
 * 	clock init method
 * \param name: pointer to clock module
 * \param data: pointer to private data
 * returns: return 0 on success, -ve on error
 */
int clock_init (struct clk_mod_t *clk_mod, void *data)
{
    if (clk_mod && clk_mod->ops.init)
        return clk_mod->ops.init(clk_mod, data);

    return ERR_INVARG;
}

/**
 * \brief clock_deinit
 * 	clock de-init method
 * \param clink_mod: pointer to clink_mod to be released
 * returns: return 0 on success, -ve on error
 */
int clock_deinit (struct clk_mod_t *clk_mod)
{
    if (clk_mod && clk_mod->ops.deinit)
        return clk_mod->ops.deinit(clk_mod);

    return ERR_INVARG;
}

/**
 * \brief clock_enable
 * 	enable the clock
 * \param clink_mod: pointer to clink_mod to be released
 * \param clk_hz: clock in hz
 * returns: return 0 on success, -ve on error
 */
int clock_enable (struct clk_mod_t *clk_mod, uint32_t clk_hz)
{
    if (clk_mod && clk_mod->ops.enable)
        return clk_mod->ops.enable(clk_mod, clk_hz);

    return ERR_INVARG;
}

/**
 * \brief clock_disable
 * 	disable the clock
 * \param clink_mod: pointer to clink_mod to be released
 * \param clk_hz: clock in hz
 * returns: return 0 on success, -ve on error
 */
int clock_disable (struct clk_mod_t *clk_mod, uint32_t clk_hz)
{
    if (clk_mod && clk_mod->ops.disable)
        return clk_mod->ops.disable(clk_mod, clk_hz);

    return ERR_INVARG;
}
