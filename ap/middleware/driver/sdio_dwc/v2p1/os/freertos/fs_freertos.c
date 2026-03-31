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
* \file		: fs_freertos.c
* \author	: ravibabu@synopsys.com
* \date		: 15-Sep-2019
* \brief	: fs_freertos.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	15-Sep-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "common.h"
#include "dwc_type.h"
#include "error.h"
#include "fs.h"

/**
 * \brief : fs_freertos_open
 *	open a specified file	
 * \param path: file path name
 * \param flags: O_CREAT, O_RDWR, O_RDONLY, O_WRONLY
 * \retuns : returns integer file descriptor, -ve on error
 */
int fs_freertos_open(const char *path, int flags)
{
	return open(path, flags);
}
/**
 * \brief : fs_freertos_read
 *	read count number of bytes into buffer from fd	
 * \param fd: integer file descriptor
 * \param buf: buffer pointer
 * \param count: length in bytes
 * \retuns : returns number of bytes read
 */
ssize_t fs_freertos_read(int fd, void *buf, size_t count)
{
	return read(fd, buf, count);
}
/**
 * \brief : fs_freertos_write
 *	write count number of bytes from buffer into file	
 * \param fd: integer file descriptor
 * \param buf: buffer pointer
 * \param count: length in bytes
 * \retuns : returns number of bytes written
 */
ssize_t fs_freertos_write(int fd, void *buf, size_t count)
{
	return write(fd, buf, count);
}

/**
 * \brief : fs_freertos_create
 *	create a specified file	
 * \param path: file path name
 * \param flags: O_CREAT, O_RDWR, O_RDONLY, O_WRONLY
 * \retuns : returns integer file descriptor, -ve on error
 */
int fs_freertos_create(const char *path, int flags)
{
//	return create(path, flags);
	return 0;
}
/**
 * \brief : fs_freertos_lseek
 *	seek the file from the specified offset
 * \param fd: integer file descriptor
 * \param offset: offset within file
 * \param whence: 
 *      SEEK_SET
              The file offset is set to offset bytes.
       SEEK_CUR
              The file offset is set to its current location plus offset
              bytes.
       SEEK_END
              The file offset is set to the size of the file plus offset
              bytes.
 * \retuns : returns number of bytes read
 */
off_t fs_freertos_lseek(int fd, off_t offset, int whence)
{
	return lseek(fd, offset, whence);
}

/**
 * \brief : fs_close
 *	closes specified file		
 * \param fd: integer file descriptor
 * \retuns : returns 0 on scucess else -ve
 */
int fs_freertos_close(int fd)
{
	return close(fd);
}

/**
 * \brief : fs_register
 *	regsiter files sysetem API		
 * \param fs: pointer to fs_t object
 * \retuns : returns 0 on scucess else -ve
 */
int fs_freertos_init(void)
{
	struct fs_t fs_stub;

	fs_stub.open 	= fs_freertos_open;
	fs_stub.read 	= fs_freertos_read;
	fs_stub.write 	= fs_freertos_write;
	fs_stub.lseek 	= fs_freertos_lseek;
	fs_stub.create 	= fs_freertos_create;
	fs_stub.close 	= fs_freertos_close;
	
	return fs_register(&fs_stub);
}
