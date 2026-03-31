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
* \file		: mshc_ctest_main.c
* \author	: ravibabu@synopsys.com
* \date		: 01-Dec-2019
* \brief	: mshc_ctest_main.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	01-Dec-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdarg.h>
#include "common.h"
#include "dwc_type.h"
#include "clog.h"

#include <string.h>
#include <stdlib.h>

#ifdef CONFIG_PLATFORM_RTLSIM
/* VCS */
#include "acc_user.h"
#include "vcs_acc_user.h"
#include "svdpi.h"
#endif

typedef unsigned char           byte;
typedef signed int              int32_t;
typedef unsigned int            uint32_t;

extern void sv_write_val(uint32_t, uint32_t);
extern void sv_read_val(uint32_t, uint32_t *reg_content);
extern int mshc_main(int argc, char *argv[]);

 /* function declaration */
void test_dpi_c(void)
{
   uint32_t reg_content;

  /* mshc driver initialization */	
   sv_write_val(0x0000002c , 0x00000000   );
   sv_write_val(0x00000028 , 0x00000000   );
   sv_write_val(0x0000002f , 0x0000000f   );
   sv_write_val(0x00000028 , 0x00000f01   );
   sv_write_val(0x0000002c , 0x00007d07   );
   sv_read_val (0x0000002c , &reg_content );
   printf      (" Read reg   , 0x2c = %x\n" ,  reg_content);
   sv_write_val(0x00000508 , 0x000f0000   );
   sv_write_val(0x00000034 , 0xffffffff   );
   sv_write_val(0x0000000c , 0x00000000   );
   sv_read_val (0x00000030 , &reg_content );
   printf      (" Read reg   , 0x30 = %x\n" ,  reg_content);

   printf ("========= mshc_main ========\n");
   mshc_main(1, NULL);

   return;
}
// eof

