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
* \file		: fs.c
* \author	: ravibabu@synopsys.com
* \date		: 10-oct-2019
* \brief	: fs.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	2019-08-12	ravibabui@synopsys.com  001		dev in progress
*/

#include <unistd.h>
#include "common.h"
#include "dwc_type.h"
#include "error.h"
#include "fs.h"

struct fs_t fs_stub;

/**
 * \brief : fs_open
 *	open a specified file
 * \param path: file path name
 * \param flags: O_CREAT, O_RDWR, O_RDONLY, O_WRONLY
 * \retuns : returns integer file descriptor, -ve on error
 */
int fs_open(const char *path, int flags)
{
    if (fs_stub.open)
        return fs_stub.open(path, flags);
    return ERROR_OPER_FAIL;
}
/**
 * \brief : fs_read
 *	read count number of bytes into buffer from fd
 * \param fd: integer file descriptor
 * \param buf: buffer pointer
 * \param count: length in bytes
 * \retuns : returns number of bytes read
 */
ssize_t fs_read(int fd, void *buf, size_t count)
{
    if (fs_stub.read)
        return fs_stub.read(fd, buf, count);
    return ERROR_OPER_FAIL;
}
/**
 * \brief : fs_write
 *	write count number of bytes from buffer into file
 * \param fd: integer file descriptor
 * \param buf: buffer pointer
 * \param count: length in bytes
 * \retuns : returns number of bytes written
 */
ssize_t fs_write(int fd, void *buf, size_t count)
{
    if (fs_stub.write)
        return fs_stub.write(fd, buf, count);
    return ERROR_OPER_FAIL;
}

/**
 * \brief : fs_create
 *	create a specified file
 * \param path: file path name
 * \param flags: O_CREAT, O_RDWR, O_RDONLY, O_WRONLY
 * \retuns : returns integer file descriptor, -ve on error
 */
int fs_create(const char *path, int flags)
{
    if (fs_stub.create)
        return fs_stub.create(path, flags);
    return ERROR_OPER_FAIL;
}
/**
 * \brief : fs_lseek
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
off_t fs_lseek(int fd, off_t offset, int whence)
{
    if (fs_stub.lseek)
        return fs_stub.lseek(fd, offset, whence);
    return ERROR_OPER_FAIL;
}

/**
 * \brief : fs_close
 *	closes specified file
 * \param fd: integer file descriptor
 * \retuns : returns 0 on scucess else -ve
 */
int fs_close(int fd)
{
    if (fs_stub.close)
        return fs_stub.close(fd);
    return ERROR_OPER_FAIL;
}

/**
 * \brief : fs_register
 *	regsiter files sysetem API
 * \param fs: pointer to fs_t object
 * \retuns : returns 0 on scucess else -ve
 */
int fs_register(struct fs_t *fs)
{
    if (!fs)
        return ERROR_INVARG;

    fs_stub.open = fs->open;
    fs_stub.read = fs->read;
    fs_stub.write = fs->write;
    fs_stub.lseek = fs->lseek;
    fs_stub.create = fs->create;
    fs_stub.close = fs->close;

    return 0;
}
