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
* \file		: mmcm_clk.h
* \author	: ravibabu@synopsys.com
* \date		: 11-Jan-2020
* \brief	: mmcm_clk.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _MMCM_CLK_H_
#define _MMCM_CLK_H_

#include <stdio.h>
#include <stdarg.h>
#include "dwc_type.h"
#include "clog.h"
#include "hal.h"

#define MMCM_CLK_400KHZ		400000

/* clock source: 320MHz*/
#define MMCM_CLK_320MHZ		320000000
#define MMCM_CLK_160MHZ		160000000
#define MMCM_CLK_80MHZ		80000000
#define MMCM_CLK_53MHZ		53333333

/* clock source: 480MHz*/
#define MMCM_CLK_480MHZ		480000000
#define MMCM_CLK_240MHZ		240000000
#define MMCM_CLK_120MHZ		120000000
#define MMCM_CLK_60MHZ		60000000

/* MMCM (mixed mode clock module) */
#define MMCM_CLKOUT5_REG1	0x06
#define MMCM_CLKOUT5_REG2	0x07
#define MMCM_CLKOUT0_REG1	0x08
#define MMCM_CLKOUT0_REG2	0x09
#define MMCM_CLKOUT1_REG1	0x0A
#define MMCM_CLKOUT1_REG2	0x0B
#define MMCM_CLKOUT2_REG1	0x0C
#define MMCM_CLKOUT2_REG2	0x0D
#define MMCM_CLKOUT3_REG1	0x0E
#define MMCM_CLKOUT3_REG2	0x0F
#define MMCM_CLKOUT4_REG1	0x10
#define MMCM_CLKOUT4_REG2	0x11
#define MMCM_CLKOUT6_REG1	0x12
#define MMCM_CLKOUT6_REG2	0x13
#define CLK_LOWTIME_MASK	(0x3F << 0)
#define CLK_HIGHTIME_MASK	(0x3F << 6)
#define CLK_PHASE_MUX		(0x7 << 13)


#define MMCM_RESET_REG_OFFS	0x534
#define MMCM_RESET_ON	(1 << 0)

/* To produce 5ns clock, T = 1/200Mhz,
 * clock high time set to 2
 * clock low time set to 3
 */
#define MMCM_CLK_LEVEL_REG_OFFS	(0 << 2)
#define MMCM_CLKOUT0_BIT12_RESVD	(1 << 12)

#define CLK_LOW_TIME_200MHZ 		3	/* low time 3ns */
#define CLK_HIGH_TIME_200MHZ 		(2 << 6) /* high time 2ns */

#define CLK_LOW_TIME_100MHZ 		5	/* low time 5ns */
#define CLK_HIGH_TIME_100MHZ 		(5 << 6) /* high time 5ns */

#define CLK_LOW_TIME_125MHZ 		4	/* low time 4ns */
#define CLK_HIGH_TIME_125MHZ 		(4 << 6) /* high time 4ns */

#define CLK_LOW_TIME_143MHZ 		4	/* low time 4ns */
#define CLK_HIGH_TIME_143MHZ 		(3 << 6) /* high time 3ns */

#define CLK_LOW_TIME_166MHZ 		3	/* low time 3ns */
#define CLK_HIGH_TIME_166MHZ 		(3 << 6) /* high time 3ns */

#define CLK_LOW_TIME_181MHZ 		2	/* low time 2ns */
#define CLK_HIGH_TIME_181MHZ 		(2 << 6) /* high time 2ns */

#define MMCM_FRAC_REG_OFFS	(MMCM_CLKOUT0_REG2 << 2)
#define MMCM_FRAC_ENABLE 	(1 << 11)

struct clk_mmcm_t {
    addr_t io_mem;
};
struct clk_mod_t *mmcm_clk_module_init(struct hal_obj_t *hal, void *io_mem, char *name);
#endif /* _CLKCORE_H_ */
