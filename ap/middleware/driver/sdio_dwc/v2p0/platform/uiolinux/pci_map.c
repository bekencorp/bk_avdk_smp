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
* \file		: pci_map.c
* \author	: ravibabu@synopsys.com
* \date		: 15-Aug-2019
* \brief	: pci_map.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	15-Aug-2019	ravibabui@synopsys.com  001		dev in progress
*/


#define CONFIG_PCI_MAP

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef CONFIG_PCI_MAP
#include <pci/pci.h>
#endif
#include <sys/io.h>
#include <sys/mman.h>
#include <pthread.h>
#include "common.h"
#include "cee.h"
#include "clog.h"

#include "mmc_dev.h"
#include "sdhci.h"
#include "pci_map.h"

#define PCI_MEM_LEN (1<<16)

addr_t pci_mem_addr;
uint32_t offset;

struct uio_pci_t uio_pci[MAX_UIO_DEVS];
#define PCI_BASE_ADDR_MEM_MASK 0xffffffff

#ifdef CONFIG_PCI_MAP
/** \brief: get_pci_mem_mmap
 * 	map the PCI memory region (BAR0) of vendor/device-id using mmap()
 *	on /dev/mem and returns the mmap addres
 * \param vendor_id : pci vendor-id
 * \param device_id : pci device-id
 * \returns mmap address of pci-memory address space
 */
char *get_pci_mem_mmap(struct pci_mmap_t *pci_mmap, uint16_t vendor_id, uint16_t device_id)
{
	struct pci_access *pci_dev_list;
	struct pci_dev *pci_devs;

	/* scan all pci devices */
	pci_dev_list = pci_alloc();
	pci_init(pci_dev_list);
	pci_scan_bus(pci_dev_list);

	for(pci_devs = pci_dev_list->devices; pci_devs; pci_devs = pci_devs->next) {
		clog_print(CLOG_LEVEL5, "%s: vendor_id(%0x) device_id(%0x)\n", __func__,
			pci_devs->vendor_id, pci_devs->device_id);
		if ((pci_devs->vendor_id == vendor_id) && (pci_devs->device_id== device_id))
			break;
	}

	if (pci_devs == NULL) {
		clog_print(CLOG_LEVEL5, "%s: vendor_id(%0x) device_id(%0x): pci dev not availabe\n", __func__,
			vendor_id, device_id);
		return 0;
	}

	pci_mem_addr = pci_read_long(pci_devs, 0x10) & PCI_BASE_ADDR_MEM_MASK;
	pci_mmap->vendor_id = vendor_id;
	pci_mmap->device_id = device_id;
	pci_mmap->pci_mem = (int8_t *)pci_mem_addr;

	pci_mmap->fd_mmap = open ("/dev/mem", O_RDWR);
	if (pci_mmap->fd_mmap == 0) {
		clog_print(CLOG_LEVEL5, "%s: unable to open dev-mem\n", __func__);
		return 0;
	}

	clog_print(CLOG_LEVEL5, "pci_mem_addr (%p)\n", pci_mem_addr);
	pci_mmap->io_mem = (char *) mmap(NULL, PCI_MEM_LEN, PROT_READ|PROT_WRITE,
				MAP_SHARED, pci_mmap->fd_mmap,(off_t)pci_mem_addr);
	clog_print(CLOG_LEVEL5, "pci_mmap.iomem (%x)\n", pci_mmap->io_mem);
	clog_print(CLOG_LEVEL5, "pci_mmap.iomem ival = (%x)\n", *(int *)pci_mmap->io_mem);

	pci_mmap->is_valid = 1;
	return pci_mmap->io_mem;
}

/** \brief: pci_mem_unmap
 * 	unmap the previously mapped pci-mem region
 * \param pci_mem_mmap : pci mmap address
 * \returns none
 */
void pci_mem_unmap(struct uio_pci_t *uio_pci, void *pci_mem_mmap)
{
	if ((uint64_t)uio_pci->map.io_mem == (uint64_t)pci_mem_mmap) {
		munmap(pci_mem_mmap, PCI_MEM_LEN);
		close(uio_pci->map.fd_mmap);
		uio_pci->map.io_mem = 0;
		uio_pci->map.is_valid = 0;
	}
}

/**
 * \brief pci_config_read
 * 	read pci register from pci-config space
 * \param uio_dev: uio_dev number
 * \param reg_offs: register offset
 * \param rd_data: register read value 
 * \returns number of bytes read
 */
int pci_config_read(int uio_dev, uint32_t reg_offs, uint32_t *rd_data)
{
	char uiodev_path[64];
	int fd, len;

	clog_print(CLOG_LEVEL5, uiodev_path, sizeof(uiodev_path),
		"/sys/class/uio/uio%d/device/config", uio_dev);
	if ((fd = open(uiodev_path, O_RDONLY)) < 0)
		return -1;

	*rd_data = 0xffffffff;
	len = pread(fd, rd_data, sizeof(*rd_data), reg_offs);
	close(fd);
	return len;
}

/**
 * \brief pci_config_write
 * 	write to pci register in pci-config space
 * \param uio_dev: uio_dev number
 * \param reg_offs: register offset
 * \param wr_data: value to be written
 * \returns number of bytes written
 */
int pci_config_write(int uio_dev, uint32_t reg_offs, uint32_t wr_data)
{
	char uiodev_path[64];
	int fd, len;

	clog_print(CLOG_LEVEL5, uiodev_path, sizeof(uiodev_path),
		"/sys/class/uio/uio%d/device/config", uio_dev);
	if ((fd = open(uiodev_path, O_WRONLY)) < 0)
		return -1;

	len = pwrite(fd, (void *)&wr_data, sizeof(wr_data), reg_offs);
	close(fd);
	return len;
}

int get_uio_devnum(struct pci_mmap_t *pci_map)
{
	char uiodev_path[64];
	uint64_t pci_addr;
	FILE *fp;
	int i, uio_devnum = -1;

	for (i = 0; i < MAX_UIO_DEVS; ++i) {
		//clog_print(CLOG_LEVEL5, uiodev_path, sizeof(uiodev_path),
		//	"/sys/class/uio/uio%d/device/resource", i);
		sprintf(uiodev_path, "/sys/class/uio/uio%d/device/resource", i);
		clog_print(CLOG_LEVEL7, "opening file %s\n", uiodev_path);
		if ((fp = fopen(uiodev_path, "r")) == NULL)
			continue;
		clog_print(CLOG_LEVEL7, "%s: opened successfully\n", uiodev_path);
		while(fscanf(fp, "%lx %*x %*x\n", &pci_addr)>0) {
			clog_print(CLOG_LEVEL5, "%s: pci_addr(%x), io_mem(%x)\n", __func__, pci_addr, pci_map->io_mem);
			if (pci_addr == (uint64_t)pci_map->pci_mem) {
				uio_devnum = i;
				break;
			}
		}
		fclose(fp);
		if (uio_devnum >= 0)
			break;
	}
	return uio_devnum;
}
#endif

/**
 * \brief uio_pci_alloc
 * 	allocate uio_pci instance
 * \param none:
 *	returns pointer to uio_pci instance.
 */
struct uio_pci_t *uio_pci_alloc(void)
{
	int i;

	for (i = 0; i < MAX_UIO_DEVS; ++i)
		if (uio_pci[i].assigned == 0)
			break;

	if (i >= MAX_UIO_DEVS)
		return NULL;

	uio_pci[i].assigned = 1;
	return &uio_pci[i];
}

/**
 * \brief uio_pci_free
 * 	release uio_pci instance
 * \param none:
 */
void uio_pci_free(struct uio_pci_t *uio_pci)
{
	if (uio_pci->assigned == 1)
		uio_pci->assigned = 0;
}

/**
 * \brief uio_pci_init
 * 	uio pci initialization
 * \param vendor_id: pci vendor-id
 * \param device_id: pci device-id
 * \param irq_handler: irq handler
 * \param irq_data: irq private data
 * \returns pointer to uio_pci_t structure
 */
struct uio_pci_t *uio_pci_init(uint16_t vendor_id, uint16_t device_id,
		int (*irq_handler)(void *), void *irq_data)
{
	struct uio_pci_t *uio_pci;
	struct pci_mmap_t *pci_map;

	clog_print(CLOG_LEVEL5, "1.%s\n",__func__);
	uio_pci = uio_pci_alloc();
	if (uio_pci == 0)
		return 0;

	uio_pci->map.io_mem = 0;
	//return uio_pci;

#ifndef CONFIG_MSHC_CSIM
#ifndef CONFIG_PLATFORM_RTLSIM
	clog_print(CLOG_LEVEL5, "2.%s\n",__func__);
	pci_map = &uio_pci->map;
	get_pci_mem_mmap(pci_map, vendor_id, device_id);
	if (pci_map->io_mem == 0)
		return 0;

	clog_print(CLOG_LEVEL5, "3.%s pci->io_mem(%x)\n",__func__, pci_map->io_mem);
	uio_pci->uio_devnum = get_uio_devnum(pci_map);
	clog_print(CLOG_LEVEL5, "4.%s uio%d\n",__func__, uio_pci->uio_devnum);
#endif
#endif

	return uio_pci;
}

#ifdef CONFIG_PCI_MAP
/**
 * \brief uio_pci_irqthread
 * 	irq thread
 * \param irq_data: irq private data
 * \param none:
 */
static void *uio_pci_irqthread(void *irq_data)
{
	struct uio_pci_t *uio_pci = irq_data;
	int retval, uio_devnum = uio_pci->uio_devnum;
	uint32_t val;

	clog_print(CLOG_LEVEL5, "===== %s (running)=====\n", __func__);
	do {
		pci_config_read(uio_devnum, 0x04, &val);
		if (val & 0x400) {
			val &= ~0x400;
			pci_config_write(uio_devnum, 0x04, val);
		}

		retval = read(uio_pci->fd_read, &val, sizeof(val));
		if (retval == -1) {
			clog_print(CLOG_ERR, "no data from uio%d \n", uio_pci->uio_devnum);
		} else if (retval > 0) {
			uio_pci->irq_handler(uio_pci->irq_data);
		} else
			clog_print(CLOG_LEVEL5, "invalid interrupt!!\n");
	} while(1);
	clog_print(CLOG_LEVEL5, "========= %s exiting ==========\n", __func__);
	return 0;
}

/**
 * \brief uio_set_irq
 * 	attach irq handler
 * \param uio_pci: pointer to uio_pci_t
 * \param irq_handler: irq handler
 * \param irq_data: irq private data
 * \param none:
 */
int uio_set_irq(struct uio_pci_t *uio_pci, int (*irq_handler)(void *), void *irq_data)
{
	pthread_t pthread;
	char uiodev_path[64];

	if (!uio_pci  || !irq_handler)
		return -1;

	clog_print(CLOG_LEVEL5, uiodev_path, sizeof(uiodev_path), "/dev/uio%d",
			uio_pci->uio_devnum);
	uio_pci->fd_read = open(uiodev_path, O_RDWR);
	if (uio_pci->fd_read < 0) {
		clog_print(CLOG_ERR, "file %s open error", uiodev_path);
		return -1;
	}

	uio_pci->irq_handler = irq_handler;
	uio_pci->irq_data = irq_data;

	if (pthread_create(&pthread, NULL, uio_pci_irqthread, uio_pci) != 0) {
		close(uio_pci->fd_read);
		return -1;
	}

	return 0;
}
#endif
