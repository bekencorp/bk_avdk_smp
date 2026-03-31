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
* \file		: user-vfio.h
* \author	: ravibabu@synopsys.com
* \date		: 15-Jan-2020
* \brief	: user-vfio.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	15-Jan-2020	ravibabui@synopsys.com  001		dev in progress
*/


/**
 * @file
 * @brief VFIO function support header files.
 */

#ifndef _USER_VFIO_H
#define _USER_VFIO_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

__BEGIN_DECLS
#include <linux/vfio.h>

#define INFO(fmt, arg...)     printf(fmt "\n", ##arg)
#define INFO_FN(fmt, arg...)  printf("%s " fmt "\n", __func__, ##arg)
#define ERROR(fmt, arg...)    printf("ERROR: %s " fmt "\n", __func__, ##arg)

#define VFIO_DEBUG
#ifdef VFIO_DEBUG
    #define DEBUG             INFO
    #define DEBUG_FN          INFO_FN
    #define HEX_DUMP(arg...)
#else
    #define DEBUG(arg...)
    #define DEBUG_FN(arg...)
    #define HEX_DUMP(arg...)
#endif


/// VFIO dma allocation structure
typedef struct _vfio_dma {
    void*                   buf;        ///< memory buffer
    size_t                  size;       ///< allocated size
    __u64                   addr;       ///< I/O DMA address
    struct _vfio_mem*       mem;        ///< private mem
} vfio_dma_t;

/// VFIO memory allocation entry
typedef struct _vfio_mem {
    struct _vfio_device*    dev;        ///< device owner
    int                     mmap;       ///< mmap indication flag
    vfio_dma_t              dma;        ///< dma mapped memory
    size_t                  size;       ///< size
    struct _vfio_mem*       prev;       ///< previous entry
    struct _vfio_mem*       next;       ///< next entry
} vfio_mem_t;

/// VFIO device structure
typedef struct _vfio_device {
    int                     pci;        ///< PCI device number
    int                     fd;         ///< device descriptor
    int                     groupfd;    ///< group file descriptor
    int                     contfd;     ///< container file descriptor
    int                     msixsize;   ///< max MSIX table size
    int                     msixnvec;   ///< number of enabled MSIX vectors
    int                     pagesize;   ///< system page size
    int                     ext;        ///< externally allocated flag
    __u64                   iovabase;   ///< IO virtual address base
    __u64                   iovanext;   ///< next IO virtual address to use
    __u64                   iovamask;   ///< max IO virtual address mask
    pthread_mutex_t         lock;       ///< multithreaded lock
    vfio_mem_t*             memlist;    ///< memory allocated list
} vfio_device_t;

// Export functions
void           vfio_read(vfio_device_t* dev, void* buf, size_t len, off_t off);
void           vfio_write(vfio_device_t* dev, void* buf, size_t len, off_t off);
vfio_device_t* vfio_create(vfio_device_t* dev, int pci);
void           vfio_delete(vfio_device_t* dev);
void           vfio_msix_enable(vfio_device_t* dev, int start, int nvec, __s32* efds);
void           vfio_msix_disable(vfio_device_t* dev);
int            vfio_mem_free(vfio_mem_t* mem);
vfio_dma_t*    vfio_dma_map(vfio_device_t* dev, size_t size, void* pmb);
int            vfio_dma_unmap(vfio_dma_t* dma);
vfio_dma_t*    vfio_dma_alloc(vfio_device_t* dev, size_t size);
int            vfio_dma_free(vfio_dma_t* dma);

__END_DECLS

#endif // _USER_VFIO_H
