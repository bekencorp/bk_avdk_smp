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
* \file		: rtl_svdpi.c
* \author	: ravibabu@synopsys.com
* \date		: 15-Oct-2019
* \brief	: rtl_svdpi.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	15-Oct-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "clink.h"
#include "clog.h"
#include "cee.h"

#include "sd_cmd.h"
#include "mmc_dev.h"
#include "sdhci.h"

#ifdef CONFIG_PLATFORM_RTLSIM
#include "acc_user.h"
#include "vcs_acc_user.h"
#include "svdpi.h"

extern void sv_write_val(uint32_t addr, uint32_t data);
extern void sv_read_val(uint32_t addr, uint32_t *data);
#else
void sv_write_val(void *addr, uint32_t data) {}
void sv_read_val(void *addr, uint32_t *data) { *data = 0; }
#endif

/**
 * \brief rtl_sim_write
 *	write 32bit value to register address
 * \param addr: register address
 */
uint32_t rtl_sim_write(void *pdata, void *addr, uint32_t val, uint8_t grain)
{
	struct sdhci_t *sdhci = (struct sdhci_t *)pdata;
	uint16_t reg_offs = (addr_t)addr - (addr_t)sdhci->reg;
	uint32_t data;

	data = 0;
	switch (grain) {
	case 1:
		sv_read_val(addr, &data);
		data &= ~0xFF;
		break;
	case 2:
		sv_read_val(addr, &data);
		data &= ~0xFFFF;
		break;
	case 4:
		break;
	default:
		break;
	}
	data |= val;
	sv_write_val(addr, data);

	clog_print(CLOG_INFO, "%s: writing value (val-%0x) to addr[%x] reg_offs=%x\n", __func__,
			val, (addr_t)addr, reg_offs);

	return 0;
}
/**
 * \brief rtl_sim_read
 *	read 32bit register data
 * \param addr: register address
 */
uint32_t rtl_sim_read(void *pdata, void *addr, uint8_t grain)
{
	struct sdhci_t *sdhci = (struct sdhci_t *)pdata;
	uint16_t reg_offs = (addr_t)addr - (addr_t)sdhci->reg;
	uint32_t data;

	sv_read_val(addr, &data);

	switch (grain) {
	case 1:
		data = data & 0xFF;
		break;
	case 2:
		data = data & 0xFFFF;
		break;
	case 4:
	default:
		break;
	}

	clog_print(CLOG_LEVEL9, "%s: read value (%0x) from addr[%x] reg_offs=%x\n",__func__,
			data, (addr_t)addr, reg_offs);

	return data;
}
