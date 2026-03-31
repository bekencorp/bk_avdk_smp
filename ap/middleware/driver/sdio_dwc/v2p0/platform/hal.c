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
* \file		: hal.c
* \author	: ravibabu@synopsys.com
* \date		: 10-July-2019
* \brief	: hal.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	10-July-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "error.h"
#include "hal.h"
#include "clog.h"

#define MAX_NUM_HAL	16

struct hal_obj_t hal_list[MAX_NUM_HAL];

/**
 * \brief hal_read8
 *	read 8bit from register address
 * \param addr: register address
 */
uint8_t hal_read8(struct hal_obj_t *hal, void *addr)
{
	int retval = ERROR_INVARG;

	if (hal && hal->read)
		return (uint8_t)hal->read(hal->p_data, addr, 1);

	return retval;
}

/**
 * \brief hal_write8
 *	write 8bit value to speicifed register address
 * \param addr: register address
 */
uint8_t hal_write8(struct hal_obj_t *hal, void *addr, uint8_t val)
{
	int retval = ERROR_INVARG;
	uint32_t dval = val;

	if (hal && hal->write)
		return (uint8_t)hal->write(hal->p_data, addr, dval, 1);

	return retval;
}

/**
 * \brief hal_read16
 *	read 16bit from register address
 * \param addr: register address
 */
uint16_t hal_read16(struct hal_obj_t *hal, void *addr)
{
	int retval = ERROR_INVARG;

	if (hal && hal->read)
		return (uint16_t)hal->read(hal->p_data, addr, 2);

	return retval;
}

/**
 * \brief hal_write16
 *	write 16bit value to speicifed register address
 * \param addr: register address
 */
uint16_t hal_write16(struct hal_obj_t *hal, void *addr, uint16_t val)
{
	int retval = ERROR_INVARG;
	uint32_t dval = val;

	if (hal && hal->write)
		return (uint16_t)hal->write(hal->p_data, addr, dval, 2);

	return retval;
}

/**
 * \brief hal_read32
 *	read 32bit from register address
 * \param addr: register address
 */
uint32_t hal_read(struct hal_obj_t *hal, void *addr)
{
	int retval = ERROR_INVARG;

	if (hal && hal->read)
		return (uint32_t)hal->read(hal->p_data, addr, 4);

	return retval;
}

/**
 * \brief hal_write32
 *	write 32bit value to speicifed register address
 * \param addr: register address
 */
uint32_t hal_write(struct hal_obj_t *hal, void *addr, uint32_t val)
{
	int retval = ERROR_INVARG;
	uint32_t dval = val;

	if (hal && hal->write)
		return (uint32_t)hal->write(hal->p_data, addr, dval, 4);

	return retval;
}

/**
 * \brief hal_reg8_set_field
 * \param hal : pointer to hal object
 * \param addr : address of register
 * \param st_bit: start bit position of register
 * \param nbits : width of mask in number of bits
 * \param val : value of bitfield
 * \param addr: register address
 */
uint32_t hal_reg8_set_field(struct hal_obj_t *hal, void *addr, uint8_t st_bit, uint8_t nbits, uint8_t val)
{
	int retval = ERROR_INVARG;
	uint8_t regw, mask;

	if (hal && hal->read && hal->write) {
		regw = hal->read(hal->p_data, addr, 1);
		/* clear the bits */
		mask = ((1 << nbits) - 1);
		regw &= ~(mask << st_bit);
		regw |= ((val & mask) << st_bit);

		hal->write(hal->p_data, addr, regw, 1);
	}

	return retval;
}

/**
 * \brief hal_reg16_set_field
 * \param hal : pointer to hal object
 * \param addr : address of register
 * \param st_bit: start bit position of register
 * \param nbits : width of mask in number of bits
 * \param val : value of bitfield
 * \param addr: register address
 */
uint32_t hal_reg16_set_field(struct hal_obj_t *hal, void *addr, uint8_t st_bit, uint8_t nbits, uint16_t val)
{
	int retval = ERROR_INVARG;
	uint16_t regw, mask;

	if (hal && hal->read && hal->write) {
		regw = hal->read(hal->p_data, addr, 2);
		/* clear the bits */
		mask = ((1 << nbits) - 1);
		regw &= ~(mask << st_bit);
		regw |= ((val & mask) << st_bit);

		hal->write(hal->p_data, addr, regw, 2);
	}

	return retval;
}

/**
 * \brief hal_reg32_set_field
 * \param hal : pointer to hal object
 * \param addr : address of register
 * \param st_bit: start bit position of register
 * \param nbits : width of mask in number of bits
 * \param val : value of bitfield
 * \param addr: register address
 */
uint32_t hal_reg32_set_field(struct hal_obj_t *hal, void *addr, uint8_t st_bit, uint8_t nbits, uint32_t val)
{
	int retval = ERROR_INVARG;
	uint32_t regw, mask;

	if (hal && hal->read && hal->write) {
		regw = hal->read(hal->p_data, addr, 4);
		/* clear the bits */
		mask = ((1 << nbits) - 1);
		regw &= ~(mask << st_bit);
		regw |= ((val & mask) << st_bit);

		hal->write(hal->p_data, addr, regw, 4);
	}

	return retval;
}

/**
 * \brief mem_dump
 *	dump the memory contents
 * \param adr: memory address
 * \param len: memory length in bytes
 * \param n_col: number of words per line
 */
void mem_dump(struct hal_obj_t *hal, void *adr, int len, int n_col)
{
	int i,j;
	uint32_t *addr = (uint32_t *)adr;

	if (n_col == 0)
		n_col = 4;

	clog_print(CLOG_LEVEL9, "======== mem_dump (%08x) len: %d========\n", (addr_t)addr, len);
	for (i = 0; i < len/n_col; i+=4) 
	{
		for (j = 0; j < n_col; ++j)
		{
                        clog_print(CLOG_LEVEL9, "addr:%08x, val:%08x \n", &addr[j], hal_read(hal, &addr[j]));
		}

		addr += n_col;
	}
}

/**
 * \brief hal_register
 *	register hal read/write operation
 * \param ops: hal_ops 
 * \param p-data: priviate data
 */
struct hal_obj_t *hal_register(uint32_t (*read)(void *pdata, void *addr, uint8_t grain),
	uint32_t (*write)(void *pdata, void *addr, uint32_t val, uint8_t grain), void *p_data)
{	

	int i;

	for (i = 0; i < MAX_NUM_HAL; ++i)
		if (hal_list[i].assigned == 0)
			break;
	
	if (i >= MAX_NUM_HAL)
		return NULL;

	hal_list[i].read = read;
	hal_list[i].write = write;
	hal_list[i].p_data = p_data;
	hal_list[i].assigned = 1;
	hal_list[i].id = i;

	return &hal_list[i];
}
