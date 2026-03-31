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
* \file		: cwatch.c
* \author	: ravibabu@synopsys.com
* \date		: 01-sep-2019
* \brief	: cwatch.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "dwc_type.h"
#include "clog.h"
#include "cwatch.h"
#include "os/str.h"

#ifdef CONFIG_TERMINAL
#include "terminal.h"
#endif

int max_cwatch_entries;
char *type_str[8] = {
    "uint8", "uint16", "uint32",
    "int8", "int16", "int32",
    "string", "float"
};
struct cwatch_t cwatch[MAX_CWATCH_ENTRIES];

/**
 * \brief  add_to_cwatch
 *     add symbol/variable to cwatch
 * \param name: name of the symbol to be watched
 * \param addr: address of symbol
 * \param type: type of symbol
 * \returns : 0 on sucessfully symbol added to cwatch, -ve on error
 */
int add_to_cwatch(char *name, void *addr, uint8_t type)
{
    int i;

    for (i = 0; i < MAX_CWATCH_ENTRIES; ++i) {
        if (cwatch[i].is_valid)
            continue;
        if (name) {
            int src_len = os_strlen(name);
            int dest_len = sizeof(cwatch[i].name);
            int cnt = MIN(src_len, dest_len);

            strncpy((void *)cwatch[i].name, name, cnt - 1);
        }
        cwatch[i].addr = addr;
        cwatch[i].type = type;
        cwatch[i].is_valid = 1;
        max_cwatch_entries++;

        if (i > max_cwatch_entries)
            max_cwatch_entries = i;
        return 0;
    }

    return ERROR_OPER_FAIL;
}

/**
 * \brief  del_from_cwatch
 *     delete symbol/variable from cwatch
 * \param name: name of the symbol to be deleted
 * \returns : 0 on sucessfully symbol deleted, -ve on error
 */
int del_from_cwatch(char *name)
{
    int i;

    if (name == NULL)
        return ERROR_INVARG;

    for (i = 0; i < MAX_CWATCH_ENTRIES; ++i) {
        if (cwatch[i].is_valid == 0)
            continue;
        if (strcmp(cwatch[i].name, name) == 0) {
            cwatch[i].is_valid = 0;
            max_cwatch_entries--;
        }
        return 0;
    }
    return 0;
}

/**
 * \brief print_cwatch_data
 *     print cwatch data element
 * \param watch:  cwatch object
 * \retuns :  none
 */
void print_cwatch_data(struct cwatch_t *watch)
{
    uint32_t val;
    int32_t sval;
    float fval;
    char *str;

    switch (watch->type) {
        case XUINT8:
            val = *(uint8_t *)watch->addr;
            clog_print(CLOG_INFO, "%s\t%s\t%08x\t(0x%02X, %d)\n",
                       watch->name, type_str[watch->type], (addr_t)watch->addr,
                       val, val);
            break;
        case XUINT16:
            val = *(uint16_t *)watch->addr;
            clog_print(CLOG_INFO, "%s\t%s\t%08x\t(0x%04X, %d)\n",
                       watch->name, type_str[watch->type], (addr_t)watch->addr,
                       val, val);
            break;
        case XUINT32:
            val = *(uint32_t *)watch->addr;
            clog_print(CLOG_INFO, "%s\t%s\t%08x\t(0x%08X, %d)\n",
                       watch->name, type_str[watch->type], (addr_t)watch->addr,
                       val, val);
            break;
        case XINT8:
            sval = *(int8_t *)watch->addr;
            clog_print(CLOG_INFO, "%s\t%s\t%08x\t(0x%02X, %d)\n",
                       watch->name, type_str[watch->type], (addr_t)watch->addr,
                       sval, sval);
            break;
        case XINT16:
            sval = *(int16_t *)watch->addr;
            clog_print(CLOG_INFO, "%s\t%s\t%08x\t(0x%04X, %d)\n",
                       watch->name, type_str[watch->type], (addr_t)watch->addr,
                       sval, sval);
            break;
        case XINT32:
            sval = *(int32_t *)watch->addr;
            clog_print(CLOG_INFO, "%s\t%s\t%08x\t(0x%08X, %d)\n",
                       watch->name, type_str[watch->type], (addr_t)watch->addr,
                       sval, sval);
            break;
        case XSTRING:
            str = (char *)watch->addr;
            clog_print(CLOG_INFO, "%s\t%s\t%08x\t(%s)\n",
                       watch->name, type_str[watch->type], (addr_t)watch->addr,
                       str);
            break;
        case XFLOAT:
            fval = *(float *)watch->addr;
            clog_print(CLOG_INFO, "%s\t%s\t%08x\t%f\n",
                       watch->name, type_str[watch->type], (addr_t)watch->addr,
                       fval, fval);
            break;
        default:
            break;
    }
}

/**
 * \brief print_cwatch
 *     print all cwatch symbol
 * \param : none
 * \returns : none
 */
void print_cwatch(char *str)
{
    int i;

    clog_print(CLOG_INFO, " Symbol\t\tType\tAddress\t\tData\t\n");
    clog_print(CLOG_INFO, "-------------------------------------------------------\n");
    for (i = 0; i < max_cwatch_entries; ++i) {
        if (cwatch[i].is_valid == 0)
            continue;
        if (str == NULL) {
            print_cwatch_data(&cwatch[i]);
        } else {
            if (strcmp(cwatch[i].name, str) == 0)
                print_cwatch_data(&cwatch[i]);
        }
    }
}
/**
 * \brief set_cwatch_data
 *    set/modify cwatch data
 * \param watch:  cwatch object
 * \retuns :  none
 */
void set_cwatch_data(struct cwatch_t *watch, uint32_t val)
{

    switch (watch->type) {
        case XUINT8:
            *(uint8_t *)watch->addr = val;
            break;
        case XUINT16:
            *(uint16_t *)watch->addr = val;
            break;
        case XUINT32:
            *(uint32_t *)watch->addr = val;
            break;
        case XINT8:
            *(int8_t *)watch->addr = val;
            break;
        case XINT16:
            *(int16_t *)watch->addr = val;
            break;
        case XINT32:
            *(int32_t *)watch->addr = val;
            break;
        case XFLOAT:
            *(float *)watch->addr = (float)val;
            break;
        default:
            break;
    }
}
/**
 * \brief print_cwatch
 *     print all cwatch symbol
 * \param : none
 * \returns : none
 */
void modify_cwatch_data(char *str, uint32_t val)
{
    int i;

    for (i = 0; i < max_cwatch_entries; ++i) {
        if (cwatch[i].is_valid == 0)
            continue;
        if (str == NULL) {
            print_cwatch_data(&cwatch[i]);
        } else {
            if (strcmp(cwatch[i].name, str) == 0)
                set_cwatch_data(&cwatch[i], val);
        }
    }
}
/**
 * \brief cmd_print_cwatch
 *     print all cwatch symbol
 * \param : none
 * \returns : none
 */
void cmd_print_cwatch(void *terminal, token_t *token, int max_token)
{
    if (max_token == 1)
        print_cwatch(NULL);
    else if (max_token == 2)
        print_cwatch(token[1].str);
    else if (max_token == 3) {
        modify_cwatch_data(token[1].str, atoi(token[2].str));
        print_cwatch(token[1].str);
    }
}

/**
 * \brief cwatch_init
 *     initialize the cwatch module
 * \param : none
 * \returns : 0
 */
int cwatch_init(void)
{
    max_cwatch_entries = 0;
    memset(cwatch, 0, sizeof(cwatch));

    #ifdef CONFIG_TERMINAL
    terminal_register_cmd("cw", cmd_print_cwatch);
    #endif
    return 0;
}
