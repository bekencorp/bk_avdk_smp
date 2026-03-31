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
* \file		: dwc_type.h
* \author	: ravibabu@synopsys.com
* \date		: 01-May-2019
* \brief	: dwc_type.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	01-May-2019	ravibabui@synopsys.com  001		dev in progress
*/

#pragma once

#include <soc/soc.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
#ifdef CONFIG_CPU64_BIT
typedef unsigned long addr_t;
#else
typedef unsigned int addr_t;
#endif

typedef char sint8_t;
typedef short sint16_t;
typedef int sint32_t;
typedef long sint64_t;

#define XUINT8		0
#define XUINT16		1
#define XUINT32		2
#define XINT8		3
#define XINT16		4
#define XINT32		5
#define XSTRING		6
#define XFLOAT		7

#define ALIGN_4BYTE	4
#define ALIGN_8BYTE	8
#define ALIGN_16BYTE	16

union reg8_t {
    uint8_t byte;
    struct {
        uint8_t b0:1;
        uint8_t b1:1;
        uint8_t b2:1;
        uint8_t b3:1;
        uint8_t b4:1;
        uint8_t b5:1;
        uint8_t b6:1;
        uint8_t b7:1;
    } bit;
};

union reg16_t {
    uint16_t word;
    struct {
        uint8_t b0:1;
        uint8_t b1:1;
        uint8_t b2:1;
        uint8_t b3:1;
        uint8_t b4:1;
        uint8_t b5:1;
        uint8_t b6:1;
        uint8_t b7:1;
        uint8_t b8:1;
        uint8_t b9:1;
        uint8_t b10:1;
        uint8_t b11:1;
        uint8_t b12:1;
        uint8_t b13:1;
        uint8_t b14:1;
        uint8_t b15:1;
    } bit;
};

union reg32_t {
    uint16_t dword;
    struct {
        uint8_t b0:1;
        uint8_t b1:1;
        uint8_t b2:1;
        uint8_t b3:1;
        uint8_t b4:1;
        uint8_t b5:1;
        uint8_t b6:1;
        uint8_t b7:1;
        uint8_t b8:1;
        uint8_t b9:1;
        uint8_t b10:1;
        uint8_t b11:1;
        uint8_t b12:1;
        uint8_t b13:1;
        uint8_t b14:1;
        uint8_t b15:1;

        uint8_t b16:1;
        uint8_t b17:1;
        uint8_t b18:1;
        uint8_t b19:1;
        uint8_t b20:1;
        uint8_t b21:1;
        uint8_t b22:1;
        uint8_t b23:1;
        uint8_t b24:1;
        uint8_t b25:1;
        uint8_t b26:1;
        uint8_t b27:1;
        uint8_t b28:1;
        uint8_t b29:1;
        uint8_t b30:1;
        uint8_t b31:1;
    } bit;
};

struct u32_low_high_t {
    uint16_t low;
    uint16_t high;
};


#define XSTATE0		0
#define XSTATE1		1
#define XSTATE2		2
#define XSTATE3		3
#define XSTATE4		4
#define XSTATE5		5
#define XSTATE6		6
#define XSTATE7		7
#define XSTATE8		8
#define XSTATE9		9
#define XSTATE10	10
#define XSTATE11	11
#define XSTATE12	12
#define XSTATE13	13
#define XSTATE14	14
#define XSTATE15	15
#define XSTATE16	16
#define XSTATE17	17
#define XSTATE18	18
#define XSTATE19	19
#define XSTATE20	20
#define XSTATE21	21
#define XSTATE22	22
#define XSTATE23	23
#define XSTATE24	24

#define BIT_ON		1
#define BIT_OFF		0

#define ERR_INVARG	-1
#define ERR_NORESOURCE	-2

#define MAX_CMD_PARAMS		64
#define XSTATE_CMD_INIT		XSTATE0
#define XSTATE_CMD_SUBMIT	XSTATE1
#define XSTATE_WAIT_IO_COMPLETE	XSTATE2
#define XSTATE_CMD_DONE		XSTATE3

typedef void (*complete_cb)(void);
typedef void (*cmd_cb)(void *mmc_dev);

struct cmd_param_s {
    /* cmd validity bit */
    uint8_t is_valid;
    /* cmd and status */
    uint8_t cmd;
    uint8_t status;
    uint8_t completed;

    uint32_t timeout;
    void    *dev_obj;
    cmd_cb   callback;

    /* response type and response data */
    uint8_t resp_type;
    uint32_t resp[4];
    /* data buffer and datalen */
    uint8_t *databuf;
    uint32_t datalen;
    /* cmd parameter len and parameters */
    uint8_t param_len;
    uint8_t param[MAX_CMD_PARAMS];
};

struct cmd_param_t {
    uint8_t in_use;
    uint8_t state;
    struct cmd_param_s cur_cmd; /* if valid execute current command */
    struct cmd_param_s pre_cmd;	/* if valid, execute pre-cmd before executing current cmd */
    struct cmd_param_s post_cmd; /*if valid, execute post-cmd after executing current cmd */
};
// eof
