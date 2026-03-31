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
* \file		: sdhci.h
* \author	: ravibabu@synopsys.com
* \date		: 01-June-2019
* \brief	: sdhci.h header
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	01-June-2019	ravibabui@synopsys.com  001		dev in progress
*/

#ifndef _SDHCI_H_
#define _SDHCI_H_
#include <soc/soc.h>

#define SNPS_VENDOR_ID		0x16c3
#define SNPS_DEVICE_ID		0xc202

#define CONFIG_MSHC0_BASE_ADDR   (SOC_SDIO0_REG_BASE - 0x700)
#define CONFIG_MSHC1_BASE_ADDR   (SOC_SDIO1_REG_BASE - 0x700)

#define MMCM_REG_OFFS		(0x8 << 2)

#define SDCMD_TIMEOUT_MS	250 /* time out in ms */
#define SDCMD_IN_PROGESS	1
#define SDCMD_TIME_OUT		2
#define SDCMD_COMPLETE		0

union xfer_mode_t {
    uint16_t word;
    struct {
        uint8_t dma_en:1;
        uint8_t blkcnt_en:1;
        uint8_t auto_cmd_en:2;
        uint8_t data_dir:1;
        uint8_t multi_blk_sel:1;
        uint8_t resp_type:1;
        uint8_t resp_err_check_en:1;
        uint8_t resp_intr_disble:1;
        uint8_t res:7;
    } bit;
};

/**
 * @brief sdhci_reg_t
 *	sdhci register structure definition
 */
struct sdhci_reg_t {
    union {						 /* offs : 0 blkcnt or sdma system address */
        struct u32_low_high_t blk_cnt;
        struct u32_low_high_t sdma_sys_adr;
    } reg0;

    uint16_t block_size; 				/* offs: 4 block size */
#define SDHCI_SDMA_BUF_BOUNDARY(x)	(x << 12)
#define SDHCI_XFER_BLKSIZE(x)		((x & 0xFFF))
    uint16_t block_cnt_16b; 			/* offs: 6 block cnt */
    struct u32_low_high_t arg;			/* offs: 8 argument */
    union xfer_mode_t xfer_mode;			/* offs: 0x0c transfer mode */
#define SDHCI_XFER_DMA_EN(x)			(x << 0)
#define SDHCI_XFER_BLKCNT_EN(x)			(x << 1)
#define SDHCI_XFER_AUTOCMD_EN(x)		((x & 3) << 2)
#define SDHCI_XFER_DATADIR(x)			(x << 4)
#define SDHCI_XFER_MULTBLK(x)			(x << 5)
#define SDHCI_XFER_RESPTYPE(x)			(x << 6)
#define SDHCI_XFER_RESP_ERRCHK_EN(x)		(x << 7)
#define SDHCI_XFER_RESP_INTR_DIS(x)		(x << 8)
    union reg16_t cmd;				/* offs: 0x0e command  */

    union reg16_t resp0;				/* offs: 0x10 command  */
    union reg16_t resp1;				/* offs: 0x12 command  */
    union reg16_t resp2;				/* offs: 0x14 command  */
    union reg16_t resp3;				/* offs: 0x16 command  */
    union reg16_t resp4;				/* offs: 0x18 command  */
    union reg16_t resp5;				/* offs: 0x1a command  */
    union reg16_t resp6;				/* offs: 0x1c command  */
    union reg16_t resp7;				/* offs: 0x1e command  */

    uint16_t buf_data_port0;			/* offs: 0x20 buffer data port0 */
    uint16_t buf_data_port1;			/* offs: 0x22 buffer data port1 */

    union reg32_t present_state;			/* offs: 0x24 Present state */
    union reg8_t host_ctrl_1;			/* offs: 0x28 host control -1 */
    union reg8_t power_ctrl;			/* offs: 0x29 power control */
    union reg8_t blk_gap_ctrl;			/* offs: 0x2a block gap control */
    union reg8_t wakeup_ctrl;			/* offs: 0x2b wakeup control */
    union reg16_t clk_ctrl ;			/* offs: 0x2c clock control  */
    union reg8_t timeout_ctrl;			/* offs: 0x2e timeout contrl */
    union reg8_t sw_reset;				/* offs: 0x2f software reset */

    union reg16_t intr_status;			/* offs: 0x30 normal interrupt status  */
    union reg16_t intr_err_status;			/* offs: 0x32 error interrupt status  */
    union reg16_t intr_status_en;			/* offs: 0x34 normal interrupt status enable */
    union reg16_t intr_err_status_en;		/* offs: 0x36 error interrupt status enable */
    union reg16_t intr_status_sigen;		/* offs: 0x38 normal interrupt status signal enable */
    union reg16_t intr_err_status_sigen;		/* offs: 0x3a error interrupt status signal enable */

    union reg16_t auto_cmd_err;			/* offs: 0x3c auto CMD Error status */
    union reg16_t host_ctrl_2;			/* offs: 0x3e host control -2 */

    union reg32_t cap;				/* offs: 0x40: capabilities 1 */
    union reg32_t cap2;				/* offs: 0x44: capabilities 2 */
    union reg32_t max_cur_cap;			/* offs: 0x48: capabilities 2 */
    union reg32_t max_cur_cap2_rsvd;		/* offs: 0x4c: capabilities 2 */

    union reg16_t autocmd_err;			/* offs: 0x50 forced auto CMD Error status */
    union reg16_t autocmd_intr_err;			/* offs: 0x52 forced auto CMD Error status */

    union reg8_t adma_err;				/* offs: 0x54 ADMA error status */
    union reg8_t res1;				/* offs: 0x55 reserved1 */
    union reg8_t res2;				/* offs: 0x56 reserved2 */
    union reg8_t res3;				/* offs: 0x57 reserved3 */

    uint32_t adma_sys_adr_low;			/* offs: 0x58 ADMA system address low */
    uint32_t adma_sys_adr_high;			/* offs: 0x5c ADMA system address high */

    uint32_t preset_val1;				/* offs: 0x60 preset value - 1 */
    uint32_t preset_val2;				/* offs: 0x64 preset value - 2 */
    uint32_t preset_val3;				/* offs: 0x68 preset value - 3 */
    uint32_t preset_val4;				/* offs: 0x6c preset value - 4 */
    uint32_t res4;					/* offs: 0x70 reserved - 4 */
    uint16_t preset_val_uhs2;			/* offs: 0x74 preset value for UHS-II */
    uint16_t res5;					/* offs: 0x76 reserved - 5 */

    uint32_t adma3_sys_adr_low;			/* offs: 0x78 ADMA system address low */
    uint32_t adma3_sys_adr_high;			/* offs: 0x7c ADMA system address high */

    uint16_t ush2_blk_size;				/* offs: 0x80 USH-II Block Size [15:0] */
    uint16_t res6;					/* offs: 0x82 reserved */
    uint32_t uhs2_blk_cnt;				/* offs: 0x84 USH-II Block count [31:0] */
    uint8_t uhs2_cmd_pkt[20];			/* offs: 0x88 to 0x9B USH-II command pkt 20 bytes */
    uint16_t uhs2_xfer_mode;			/* offs: 0x9C UHS-II Transfer mode [15:0] */
    uint16_t uhs2_cmd;				/* offs: 0x9E UHS-II command [15:0] */
    uint8_t uhs2_response[20];			/* offs: 0xA0 to 0xB3 USH-II response 20 bytes */
    uint8_t	uhs2_msg_select;			/* offs: 0xB4 UHS-II MSG Select */
    uint8_t res7;					/* offs: 0xb5 reserved */
    uint16_t res8;					/* offs: 0xb6 reserved */
    uint32_t uhs2_msg;				/* offs: 0xB8 UHS-II MSG */
    uint16_t uhs2_dev_intr_status;			/* offs: 0xBC UHS-II Device Interrput status */
    uint8_t uhs2_dev_select;			/* offs: 0xBE UHS-II Device select */
    uint8_t uhs2_dev_int_code;			/* offs: 0xBF UHS-II Device Int Code */
    uint16_t uhs2_sw_reset;				/* offs: 0xC0 UHS-II Software reset */
    uint16_t uhs2_timer_ctrl;			/* offs: 0xC2 UHS-II timer Control */
    uint32_t uhs2_err_intr_status;			/* offs: 0xC4 UHS-II Erro Interrupt status */
    uint32_t uhs2_err_intr_status_en;		/* offs: 0xC8 UHS-II Erro Interrupt Enable */
    uint32_t uhs2_err_intr_sig_en;			/* offs: 0xCC UHS-II Erro Interrupt Signal Enable */
    uint32_t res9;					/* offs: 0xD0 reserved */
    uint32_t res10;					/* offs: 0xD4 reserved */
    uint32_t res11;					/* offs: 0xD8 reserved */
    uint16_t ptr_uhs2_settings;			/* offs: 0xE0 Pointer to UHS-II settings */
    uint16_t ptr_uhs2_caps;				/* offs: 0xE2 Pointer to UHS-II caps */
    uint16_t ptr_uhs2_test;				/* offs: 0xE4 Pointer to UHS-II test */
    uint16_t ptr_uhs2_emb_ctrl;			/* offs: 0xE6 Pointer to UHS-II embedded ctrl */
    uint16_t ptr_vendor_area;			/* offs: 0xE8 Pointer to vendor specific area */
    uint16_t ptr_spec_ctrl;				/* offs: 0xEA Pointer to specific ctrl reserved */
    uint32_t res12;					/* offs: 0xEC reserved */
    uint32_t res13;					/* offs: 0xF0 reserved */
    uint32_t res14;					/* offs: 0xF4 reserved */
    uint32_t res15;					/* offs: 0xF8 reserved */
    uint16_t slot_intr_status;			/* offs: 0xFC slot interrupt status */
    uint16_t host_ctrl_version;			/* offs: 0xFE host controller version */
};


/**
 * struct sdhci_t
 * 	structure of sd host controller instance
 * reg : pointer to sdhci register structure (not used)
 * io_base : address of host controller base address
 * hal : pointer to hal_obj for hardware access
 * pal : pointer to pal_obj for platform acess
 * host : pointer to mmc_host_t object
 * mmcm_clk : pointer to external clock module to generate
 * 		 high frequency sd-clock
 * irq : irq number of sdhci controller
 * irq_handler : isr function to handle sdhci interrupt events
 * isr_task_id : task-id of isr thread in polling mode
 * act_req : current active io_request
 * assigned : sdhci object is allocated (in use) or freed.
 * intr_mask : sdhci interrupt mask
 * intr_status : current interrupt status
 * card_present : indicate card is inserted or not
 * vendor1_offs : offset to vendor-1 register of mshc/sdhci
 */
struct sdhci_t {
    struct sdhci_reg_t *reg;
    addr_t io_base;
    addr_t sys_base;
    uint32_t instance_index;

    struct hal_obj_t *hal;
    struct hal_obj_t *pal;
    struct mmc_host_t host;
    struct clk_mod_t *mmcm_clk;
    uint8_t id, irq;
    uint8_t is_emmc_dev;
    int (*irq_handler)(uint8_t task_id, void *sdhci);
    uint8_t isr_task_id;
    struct io_req_t *act_req;
    uint8_t assigned;
    uint8_t card_present;
    uint16_t vendor1_offs;
    uint32_t intr_mask;
    uint32_t intr_status;
};

int sdhci_module_init(uint32_t instance_id);
void sdhci_irq_instance0(void);
void sdhci_irq_instance1(void);
#endif /* _SDHCI_H_ */
