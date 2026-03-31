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
* \file		: regdesc.c
* \author	: ravibabu@synopsys.com
* \date		: 10-Dec-2019
* \brief	: regdesc.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <string.h>
#include "dwc_type.h"
#include "clog.h"

#define MAX_SDHCI_REGS	64
#define END_OF_REGS	0xFFFFFFFF

struct reg_field_t {
    uint8_t st;
    uint8_t et;
    uint8_t width;
    char name[40];
    uint8_t nfval;
    char fvalstr[16][40];
};

struct reg_desc_t {
    uint32_t offs;
    char name[40];
    uint32_t regval;
    uint8_t n_field;
    struct reg_field_t field[32];
};

char prnbuf[256];
struct reg_desc_t sdhci_reg0[MAX_SDHCI_REGS] = {
    {
        0x00,		/* offset */
        "SDMASA_R System AdrReg",	/* reg_name */
        0x0,		/* regval */
        1,		/* number of fields */
        {
            /* field description */
            {   0,	/*start bit */
                31,	/* end bit */
                32,	/* width */
                "BLOCKCNT_SDMASA", /* field name */
                5, 	/* field value description */
                {
                    "32-bit Block count (SDMA adr)", /* field value0 description */
                    "0xFFFF_FFFF-4G-1Block",
                    "....",
                    "0x0000_0002 - 2 Blocks",
                    "0x0000_0001 - 1 Blocks",
                    "0x0000_0000 - Stop count",
                }
            },
        },
    },
    {
        0x0c,		/* offset */
        "XFER_MODE_R",	/* reg_name */
        0x0,	/* regval */
        4,		/* number of fields */
        {
            /* field description */
            {   0,	/*start bit */
                0,	/* end bit */
                1,	/* width */
                "DMA_Enable", /* field name */
                2, 	/* field value description */
                {
                    "1-(ENABLED) DMA Data Transfer", /* field value0 description */
                    "0-(DISABLED) No DMA Transfer", /* field value0 description */
                }
            },
            /* field description */
            {   1,	/*start bit */
                1,	/* end bit */
                1,	/* width */
                "BLOCK_COUNT_ENABLE", /* field name */
                2, 	/* field value description */
                {
                    "1-(ENABLED)", /* field value0 description */
                    "0-(DISABLED)", /* field value0 description */
                }
            },
            /* field description */
            {   2,	/*start bit */
                3,	/* end bit */
                2,	/* width */
                "AUTO_CMD_ENABLE", /* field name */
                4, 	/* field value description */
                {
                    "0x0-(AUTOCMD DISABLED)", /* field value0 description */
                    "0x1-(AUTO_CMD12_ENABLED)", /* field value0 description */
                    "0x2-(AUTO_CMD23_ENABLED)",
                    "0x3-(AUTO_CMD_AUTO_SEL)",
                }
            },
            /* field description */
            {   4,	/*start bit */
                4,	/* end bit */
                1,	/* width */
                "DATA_XFER_DIR", /* field name */
                2, 	/* field value description */
                {
                    "0x0-(READ)", /* field value0 description */
                    "0x1-(WRITE)", /* field value0 description */
                }
            },
        },
    },
    {
        0x40,		/* offset */
        "CAPABILITIES1_R",	/* reg_name */
        0x0,	/* regval */
        3,		/* number of fields */
        {
            /* field description */
            {   0,	/*start bit */
                5,	/* end bit */
                6,	/* width */
                "TOUT_CLK_FREQ", /* field name */
                1, 	/* field value description */
                {
                    "TOUT_CLK_FREQ", /* field value0 description */
                }
            },
            /* field description */
            {   6,	/*start bit */
                6,	/* end bit */
                1,	/* width */
                "RSVD_6", /* field name */
                0, 	/* field value description */
                {
                    "", /* field value0 description */
                }
            },
            /* field description */
            {   7,	/*start bit */
                7,	/* end bit */
                1,	/* width */
                "TOUT_CLK_UNIT", /* field name */
                2, 	/* field value description */
                {
                    "0x0-Khz", /* field value0 description */
                    "0x1-Mhz", /* field value0 description */
                }
            },
        },
    },
    {
        0x44,		/* offset */
        "CAPABILITIES2_R",	/* reg_name */
        0x0,	/* regval */
        2,		/* number of fields */
        {
            /* field description */
            {   0,	/*start bit */
                1,	/* end bit */
                2,	/* width */
                "mode type", /* field name */
                4, 	/* field value description */
                {
                    "00-mode-0", /* field value0 description */
                    "01-mode-1", /* field value0 description */
                    "02-mode-2", /* field value0 description */
                    "03-mode-3", /* field value0 description */
                }
            },
            /* field description */
            {   2,	/*start bit */
                2,	/* end bit */
                1,	/* width */
                "Mode-Enable", /* field name */
                2, 	/* field value description */
                {
                    "0-Enable mode", /* field value0 description */
                    "1-Disable mode", /* field value0 description */
                }
            },
        },
    },
    {
        0xFFFFFFFF,	/* offset */
        "END",	/* reg_name */
        0x0,
        1,		/* number of fields */
        {
            /* field description */
            {   0,	/*start bit */
                31,	/* end bit */
                32,	/* width */
                "dummy", /* field name */
                0, 	/* field value description */
                {
                    "", /* field value0 description */
                }
            },
        },
    }
};


uint32_t get_val(uint32_t value, uint8_t st, uint8_t w)
{
    uint32_t mask;

    mask = ((1 << w) - 1);
    return ((value & (mask << st)) >> st);

}

/* \brief print_reg_desc
 * 	prints the register field description
 * \param offs: register offset
 */
void print_reg_fields(struct reg_desc_t *reg_desc, uint32_t n_regs, addr_t offs, uint32_t value,
                      uint8_t print_flag)
{
    struct reg_desc_t *reg;
    int i, j, k;

    for (i = 0; i < n_regs; ++i) {

        reg = &reg_desc[i];
        if (reg->offs == END_OF_REGS)
            return;

        if (reg->offs == offs) {
            memset(prnbuf, '-', 68);
            if (print_flag == 1) {
                clog_print(CLOG_LEVEL10, "%s\n", prnbuf);
                clog_print(CLOG_LEVEL10, "OFFSET %08X\t\t%s\t\t%08X\n", offs, reg->name, value);
            } else {
                printf("%s\n", prnbuf);
                printf("OFFSET %08X\t\t%s\t\t%08X\n", offs, reg->name, value);
            }

            /* print the regiser fields */
            for (j = 0; j < reg->n_field; ++j) {
                if (print_flag == 1) {
                    clog_print(CLOG_LEVEL10, "  [%d] - BIT%d:%d\t\t%s\t\t\t%0X\n", j,
                               reg->field[j].et,
                               reg->field[j].st, reg->field[j].name,
                               get_val(value, reg->field[j].st, reg->field[j].width));
                    for (k = 0; k < reg->field[j].nfval;  ++k)
                        clog_print(CLOG_LEVEL10, "\t\t\t\t-%s\n", reg->field[j].fvalstr[k]);
                } else {
                    printf("  [%d] - BIT%d:%d\t\t%s\t\t\t%0X\n", j,
                           reg->field[j].et,
                           reg->field[j].st, reg->field[j].name,
                           get_val(value, reg->field[j].st, reg->field[j].width));
                    for (k = 0; k < reg->field[j].nfval;  ++k)
                        printf("\t\t\t\t-%s\n", reg->field[j].fvalstr[k]);
                }
            }
        }
    }
}
/* \brief print_reg_desc
 * 	prints the register field description
 * \param offs: register offset
 */
void print_reg_desc(addr_t offs, uint32_t value, uint8_t print_flag)
{
    print_reg_fields(sdhci_reg0, MAX_SDHCI_REGS, offs & 0xFFFF, value, print_flag);
}
