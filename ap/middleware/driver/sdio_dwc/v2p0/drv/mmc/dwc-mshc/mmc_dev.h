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
* \file		: mmc_dev.h
* \author	: ravibabu@synopsys.com
* \date		: 24-June-2019
* \brief	: mmc_dev.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	24-June-2019	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _MMC_DEV_H_
#define _MMC_DEV_H_

#include "dwc_type.h"
#include "error.h"
#include "sd_card.h"
#include "cee.h"

#define MAX_MMC_DEVICES		2
#define MAX_MMC_IO_REQUESTS	32
#define MMC_REQ_QUEUE_DEPTH	16

#define MMC_CMD_STATUS_DONE		1
#define MMC_CMD_STATUS_IN_PROGRESS	2
#define MMC_CMD_STATUS_ABORT		3
#define MMC_CMD_STATUS_TIMEOUT		4
#define MMC_CMD_STATUS_FAILED		5
#define MMC_CMD_MAX_TIMEOUT	0xFFFFF /*timeout in ms , 5000 for 5second timeout*/

#define SD_BLOCK_SZ	512
#define EMMC_TUNING_BLOCK_SZ	128
#define SD_TUNING_BLOCK_SZ	64
#define MAX_NUM_BLOCKS	128

enum enum_mmc_dev_type {
    MMC_DEV	= 1,
    EMMC_DEV = 2,
};
enum enum_mmc_speed_mode {
    MMC_DEFAULT_SPEED = 1,
    MMC_HIGH_SPEED 	= 2,
    MMC_SPEED_SDR12 = 3,
    MMC_SPEED_SDR25 = 4,
    MMC_SPEED_SDR50 = 5,
    MMC_SPEED_SDR104 = 6,
    MMC_SPEED_DDR50 = 7,
    MMC_SPEED_HS200 = 8,
    MMC_SPEED_HS400 = 9,
};
enum enum_mmc_clk {
    MMC_CLK_100MHZ = 1,
    MMC_CLK_125MHZ = 2,
    MMC_CLK_143MHZ = 3,
    MMC_CLK_166MHZ = 4,
    MMC_CLK_181MHZ = 5,
    MMC_CLK_200MHZ = 6,
};
enum enum_mmc_bus_width {
    MMC_1_BIT_MODE = 1,
    MMC_4_BIT_MODE = 2,
    MMC_8_BIT_MODE = 3,
};
enum enum_mmc_xfer_mode	{
    MMC_XFER_PIO = 1,
    MMC_XFER_SDMA = 2,
    MMC_XFER_ADMA2 = 3,
    MMC_XFER_ADMA3 = 4,
};
enum enum_mmc_io_volt {
    MMC_IO_VOLT_00 = 0,
    MMC_IO_VOLT_12 = 1,
    MMC_IO_VOLT_18 = 2,
    MMC_IO_VOLT_33 = 3,
};

struct mmc_cfg_t {
    enum enum_mmc_dev_type 		dev_type;
    enum enum_mmc_speed_mode	speed;
    enum enum_mmc_clk 		clk;
    enum enum_mmc_bus_width 	bus_width;
    enum enum_mmc_xfer_mode		xfer_mode;
    enum enum_mmc_io_volt 		io_volt;
};

struct sdhci_t;
struct mmc_dev_t;

/** \brief:  mmc_req_t structure defintiions
 *	mmc request encapsulates all cmd/param & responses
 * \param mmc_dev: pointer to mmc_dev obj
 * \param cmdreg: cmdreg value
 * \param args: cmd arguments
 * \param flag: cmd request flags
 * \param resp: response from device
 * \param databuf: pointer to read/write databuf
 * \param buflen: buffer length
 * \param rx_data_len: recevied data length
 * \param tx_data_len: total transmitted datalen
 * \param nblocks: number of blocks
 * \param blocksize: block size
 * \param int_mask: interrupt mask
 * \param cfg: mmc_cfg structure
 * \param status: status of request
 */
struct mmc_req_t {
    struct mmc_dev_t *mmc_dev;
    /* cmd & args */
    uint16_t cmdreg;
#define sd_cmd(x)	((x >> 8) & 0x3F)
    uint32_t args;
    uint32_t flags;
#define MMC_RESP0_NONE			0
#define MMC_RESP1_LEN_136		1
#define MMC_RESP2_LEN_48		2
#define MMC_RESP3_LEN_48B		3
#define CMD_NO_RESPONSE		BIT(8)
#define CMD_RESP1_CARDSTS	BIT(9)
#define CMD_RESP1B_CARDSTS	BIT(10)
#define CMD_RESP2_CIDCSD	BIT(11)
#define CMD_RESP3_OCR		BIT(12)
#define CMD_WITHOUT_DATA	BIT(13)
#define CMD_WITH_DATA		BIT(14)

#define XFR_MODE_BLKCNT_EN_F	BIT(16)
#define XFR_MODE_DATA_READ_F	BIT(17)
#define XFR_MODE_MULTBLK_SEL_F	BIT(18)
#define XFR_MODE_DMA_EN_F	BIT(19)
#define XFR_MODE_AUTOCMD12_EN_F	BIT(20)
#define XFR_MODE_AUTOCMD23_EN_F	BIT(21)
#define XFR_MODE_AUTOCMD_AUTO_SEL_F	BIT(22)
#define XFR_MODE_RESP_ERRCHK_EN_F	BIT(23)
#define XFR_MODE_RESP_INT_EN_F	BIT(24)
#define SDHCI_INTR_ENABLE	BIT(31)

    /* response type */
    uint32_t resp[4];

    /* data buffers */
    uint8_t *databuf;		/* pointer to data buf */
    uint32_t buflen;		/* buffer length */
    uint32_t rx_data_len;		/* received data length */
    uint32_t tx_data_len;
    uint32_t nblocks;
    uint16_t blocksize;
    uint32_t int_mask;

    struct mmc_cfg_t *cfg;
    uint8_t status; 		/* status of request */
    uint8_t in_use;
    uint8_t state;
};

/** \brief: mmc_host_ops_t
 *	mmc host operation structure, its members are
 * \param send_cmd: to send sd-cmd
 * \param io_ctrl: configure controller settings
 * \param get_card_detect: check card is present
 * \param get_card_write_protect: check card is write protected
 * \param start_io_tuning: start io tuning
 * \param enable_clock: enable mmc clocks
 * \param disable_clock: disable mmc clcoks
 * \param set_speed: set the speed-mode
 * \param set_data_width: set data width
 * \param set_voltage: set voltage level
 */
struct mmc_host_ops_t {
    /* send mmc commands */
    int (*send_cmd)(struct sdhci_t *sdhci, struct io_req_t *req);
    int (*io_ctrl)(struct sdhci_t *sdhci, uint32_t cmd, uint32_t args);
    int (*get_card_detect)(struct mmc_dev_t *mmc_dev);
    int (*get_card_write_protect)(struct mmc_dev_t *mmc_dev);
    int (*start_io_tuning)(struct mmc_dev_t *mmc_dev, struct io_req_t *req);
    int (*enable_clock)(struct sdhci_t *sdhci, uint8_t speed_mode,
                        uint8_t mmc_vdd, uint32_t mmcm_clock);
    int (*disable_clock)(struct sdhci_t *sdhci);
    int (*set_speed)(struct sdhci_t *sdhci, uint8_t speed_mode);
    int (*set_data_width)(struct sdhci_t *sdhci, uint8_t width);
    int (*set_voltage)(struct sdhci_t *sdhci, uint8_t mmc_vdd_sel, uint8_t mmc_vdd_volt);
};

/** \brief: mmc_host_t
 *	mmc host controller structure, its members are
 * \param drv_data: pointer to sdhci controller object
 * \param ops: pointer to mmc host operations
 */
struct mmc_host_t {
    void *drv_data;
    struct mmc_host_ops_t ops;
};

/** \brief: mmc_dev_t
 *	mmc-device structure, its members are
 * \param in_use: is device is allocated/free
 * \param id: device id
 * \param req_queue: io_request queue number to
 * 	service io-request by mmc-core layer
 * \param cmd_inprogess: cmd inprogress or not
 * \param host: pointer to mmc-host
 * \param card: pointer to mmc-card
 */
struct mmc_dev_t {
    uint8_t in_use, id;
    uint8_t req_queue;
    uint8_t cmd_inprogress;
    struct cmd_param_t *cur_mmc_cmd;

    struct mmc_cfg_t cfg;
    struct mmc_host_t *host;
    struct sd_card_t *card;
};

/** \brief: mmc_request_alloc
 * 	allocate mmc request from free pool
 * \param flags : memory allocation flags (not used)
 * \returns pointer to mmc_request
 */
void mmc_module_init(void);

/** \brief: mmc_dev_new
 * 	allocate new mmc-dev object
 * \param flags: control flags
 * \returns pointer to mmc-dev object
 */
struct mmc_dev_t *mmc_dev_new(uint32_t flags);

/** \brief: mmc_alloc_buf
 * 	initialize the mmc request pool
 * 	always return static memory from g_sd_buf[0]
 * \param flags : memory allocation flags - 4/8/16 byte alignment
 * \returns aligned memory block
 */
void *mmc_alloc_buf(uint32_t size, uint8_t flags);
void mmc_free_buf(void *buf);
void print_data_buf(void *buf, uint32_t len);
#endif
