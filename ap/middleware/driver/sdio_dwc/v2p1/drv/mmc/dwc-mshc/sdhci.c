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
* \file		: sdhci.c
* \author	: ravibabu@synopsys.com
* \date		: 01-June-2019
* \brief	: sdhci.c source
* Revision history:
* Ver	Date		Author       	  	Change Id	Description
* 0.1	01-June-2019	ravibabui@synopsys.com  001		dev in progress
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "cee.h"
#include "csem.h"
#include "ctask.h"
#include "cmsg_queue.h"
#include "clink.h"
#include "clog.h"
#include "error.h"
#include "mshc_regs.h"

#include "mmc_dev.h"
#include "mmc_core.h"
#include "sd_cmd.h"
#include "hal.h"
#include "dwc_msdc_cfg.h"
#include "clkcore.h"
#include "sdhci.h"
#include "delay.h"
#include "mmcm_clk.h"
#include <driver/gicv2.h>
#include <driver/int_types.h>
#include <driver/int.h>

/* module global datas */
struct sdhci_t g_sdhci[MAX_SDHCI_INSTANCES];

/* external definitions */
extern struct clink_t *mshc_link, *msdc_link;
extern struct msg_desc_t mshc_msg, msdc_msg;
extern int sdhci_hal_register(struct sdhci_t *sdhci);
extern int sdhci_hal_config_system_base(struct sdhci_t *sdhci);
extern int sdhci_hal_init(struct sdhci_t *sdhci);

/* module specific parameters */
uint8_t tx_phase = 0x3F;
uint8_t rx_phase = 0;
/* below params are passed from API, below module param are not used,TBD */
uint8_t speed_mode = SDR12_DS_SPEED_MODE;
uint8_t bus_width = SD_BUSWIDTH_4;
uint8_t xfer_mode = XFER_MODE_PIO;
uint8_t emmc_vdd = XVDD1_1P8_VOLT;
uint8_t mmcm_clock = 200;
uint8_t is_emmc_dev = 0;

int sdhci_set_voltage(struct sdhci_t *sdhci, uint8_t mmc_vdd_sel, uint8_t mmc_vdd_volt);
void *sd_card_get_private_data(uint32_t instance_id);
int sdhci_irq_register(uint32_t instance_id, uint8_t irq);

/**
 * \brief sd_card_is_inserted
 *	check whether card is inserted or not
 * \param none
 * \returns true if card is inserted, 0 otherwise
 */
uint32_t sdhci_get_pstate(struct sdhci_t *sdhci)
{
    addr_t io_mem;
    uint32_t pstate_reg;

    /* PState Register always shows (bit16) card is inserted
     * irrespective of card is removed or inserted */
    io_mem = sdhci->io_base;
    pstate_reg = hal_read(sdhci->hal, (void *)(io_mem + PSTATE_REG_R));

    return pstate_reg;
}

/**
 * \brief sdhci_get_base
 *	get base address of sdhci controller
 * \param instance: sdhci instance number
 * \returns returns the base address of sdhci
 */
addr_t sdhci_get_base(uint8_t instance)
{
    addr_t reg_base = CONFIG_MSHC0_BASE_ADDR;

    if(instance == 0)
        reg_base = CONFIG_MSHC0_BASE_ADDR;
    else if(instance == 1)
        reg_base = CONFIG_MSHC1_BASE_ADDR;

    return (addr_t)reg_base;
}

uint8_t sdhci_get_irq_index(uint8_t instance)
{
    uint8_t index = INT_SRC_SDIO0;

    if(instance == 0)
        index = INT_SRC_SDIO0;
    else if(instance == 1)
        index = INT_SRC_SDIO1;

    return index;
}

int_group_isr_t sdhci_get_irq_handler(uint32_t instance)
{
    int_group_isr_t func = sdhci_irq_instance0;

    if(instance == 0)
        func = sdhci_irq_instance0;
    else if(instance == 1)
        func = sdhci_irq_instance1;

    return func;
}

/**
 * \brief sdhci_reset
 *	do sdhci controller reset
 * \param sdhci: pointer to sdhci object
 * \param mask: reset mask value
 * \returns none
 */
void sdhci_reset(struct sdhci_t *sdhci, uint8_t mask)
{
    addr_t io_mem = sdhci->io_base;
    uint32_t timeout = SDHCI_RESET_TIMEOUT, val;

    /* reset the sdhci controller */
    clog_print(CLOG_INFO, "%s: mask = %x\n", __func__, mask);
    hal_write8(sdhci->hal, (void *)(io_mem + SW_RST_R), mask);

    /* wait for completion */
    do {
        val = hal_read8(sdhci->hal, (void *)(io_mem + SW_RST_R));
        clog_print(CLOG_LEVEL6, "%s: SW_RST_R = %x\n", __func__, val);
        if (val & mask) {
            timeout--;
        } else
            break;
    } while (timeout);

    if ((val & mask) && timeout == 0)
        clog_print(CLOG_INFO, "sdhci reset failed, regval = %x\n", val);
    else
        clog_print(CLOG_INFO, "sdhci reset success, regval = %x\n", val);

    /* give 500ms delay after reset */
    os_delay_ms(500);
}

/**
 * \brief sdhci_enable_intr
 *	enable the interrupts
 * \param sdhci: pointer to sdhci object
 * \returns none
 */
void sdhci_enable_intr(struct sdhci_t *sdhci)
{
    addr_t io_mem = sdhci->io_base;

    /* enable the interrupt flags */
    sdhci->intr_mask =  CMD_TOUT_ERR_SIGNAL_EN |
                        CUR_LMT_ERR_SIGNAL_EN | DATA_END_BIT_ERR_SIGNAL_EN |
                        DATA_CRC_ERR_SIGNAL_EN | DATA_TOUT_ERR_SIGNAL_EN |
                        CMD_END_BIT_ERR_SIGNAL_EN | CMD_IDX_ERR_SIGNAL_EN |
                        CMD_CRC_ERR_SIGNAL_EN | RE_TUNE_EVENT_SIGNAL_EN |
                        CARD_REMOVAL_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN |
                        CMD_COMPLETE_SIGNAL_EN | CARD_INSERTION_SIGNAL_EN |
                        BUF_RD_READY_SIGNAL_EN | BUF_WR_READY_SIGNAL_EN;

    hal_write(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_EN_R), sdhci->intr_mask);
    hal_write(sdhci->hal, (void *)(io_mem + NORMAL_INT_SIGNAL_EN_R), sdhci->intr_mask);
}

/**
 * \brief sdhci_disable_intr
 *	disable the interrupts
 * \param sdhci: pointer to sdhci object
 * \returns none
 */
void sdhci_disable_intr(struct sdhci_t *sdhci)
{
    addr_t io_mem = sdhci->io_base;

    /* enable the interrupt flags */
    sdhci->intr_mask = 0;

    hal_write(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_EN_R), sdhci->intr_mask);
    hal_write(sdhci->hal, (void *)(io_mem + NORMAL_INT_SIGNAL_EN_R), sdhci->intr_mask);
}

/**
 * \brief sdhci_io_ctrl
 *	perform various io-ctrl operation based on cmd/args
 * \param sdhci: pointer to sdhci object
 * \param cmd: cmd parameter
 * \param args : parameter
 * \returns none
 */
int sdhci_io_ctrl(struct sdhci_t *sdhci, uint32_t cmd, uint32_t args)
{
    addr_t io_mem = sdhci->io_base;
    uint16_t version;
    uint32_t val;

    switch (cmd) {
        case SDHCI_CLOCK_OFF:
            hal_write16(sdhci->hal, (void *)(io_mem + CLK_CTRL_R), 0);
            break;
        case SDHCI_CLOCK_ON:
            hal_write16(sdhci->hal, (void *)(io_mem + CLK_CTRL_R),
                        FREQ_SEL_VAL | INTERNAL_CLK_EN |
                        INTERNAL_CLK_STABLE | SD_CLK_EN | PLL_ENABLE);
            val = hal_read16(sdhci->hal, (void *)(io_mem + CLK_CTRL_R));
            clog_print(CLOG_LEVEL7, "%s: clk_ctrl_reg = %x\n", __func__, val);
            os_delay_ms(500);
            break;
        case SDHCI_POWER_OFF:
            hal_write16(sdhci->hal, (void *)(io_mem + HOST_CTRL2_R), 0);
            hal_write(sdhci->hal, (void *)(io_mem + HOST_CTRL1_R), 0);
            os_delay_ms(500);
            break;
        case SDHCI_POWER_ON:
            /* PWR_CTRL_R[0x29]: control the bus power[VDD1/VDD2] for the card. If in sd mode, sd_bus_pwr_vdd1
             * is cleared, the host controller stops the sd clock. Refer to mobile storage host controller
             * user guide: Card Setup Sequence
             */
            sdhci_set_voltage(sdhci, SD_BUS_PWR_VDD1, args);
            os_delay_ms(500);
            break;
        case SDHCI_SET_VERSION:
            version = hal_read16(sdhci->hal, (void *)(io_mem + HOST_CNTRL_VERS_R));
            if(version  >= 3) {
                /* Host Version 4 Enable*/
                hal_reg16_set_field(sdhci->hal, (void *)(io_mem + HOST_CTRL2_R),
                                    HOST_VER4_ENABLE_POS, HOST_VER4_ENABLE_MASK, 1);
            } else {
                /* Version 3.0.0 compatible mode*/
                hal_reg16_set_field(sdhci->hal, (void *)(io_mem + HOST_CTRL2_R),
                                    HOST_VER4_ENABLE_POS, HOST_VER4_ENABLE_MASK, 0);
            }
            break;
        case SDHCI_SET_DATA_TIMEOUT_COUNTER:
            hal_write8(sdhci->hal, (void *)(io_mem + TOUT_CTRL_R), args & 0xFF);
            break;
        case SDHCI_SET_MMC_DEV_TYPE:
            hal_reg16_set_field(sdhci->hal, (void *)(io_mem + sdhci->vendor1_offs + SNPS_EMMC_CTRL_R),
                                MMC_DEV_TYPE_POS, MMC_DEV_TYPE_MASK, args & 0x7F);
            break;
        case SDHCI_SET_TXPHASE:
            hal_reg32_set_field(sdhci->hal, (void *)(io_mem + 0x31c),
                                16, 8, args & 0x7F);
            break;
        case SDHCI_SET_RXPHASE:
            /* FIXME*/
            break;
        case SDHCI_SW_TUNE_CTRL:
            /* enable/disable software tuning on */
            if (args == DISABLE)
                val = 0;
            else if (args == ENABLE)
                val = 1;
            else
                return 1;

            if (cmd == SDHCI_SW_TUNE_CTRL) {
                /* FIXME*/
            }
            else if (cmd == SDHCI_CMDCFLT_CHK) {
                /* FIXME*/
            }
            break;
        default:
            break;
    }
    return 0;
}

/**
 * \brief sdhci_setup_clock
 *	setup sdhci clock control
 * \param sdhci: pointer to sdhci object
 * \param mask: reset mask value
 * \returns none
 */
int sdhci_setup_clock(struct sdhci_t *sdhci, uint32_t clock)
{
    return 0;
}

/**
 * \brief sdhci_disable_clock
 *	disable the controller clock
 * \param sdhci: pointer to sdhci object
 * \returns none
 */
int sdhci_disable_clock(struct sdhci_t *sdhci)
{
    addr_t io_mem = sdhci->io_base;
    clog_print(CLOG_INFO, "disable sdhci clock\n");

    /* disable clock */
    hal_write(sdhci->hal, (void *)(io_mem + CLK_CTRL_R), 2);
    return 0;
}

/**
 * \brief sdhci_enable_clock
 *	configure the sdhci speed-mode, mmc-clocks
 * \param sdhci: pointer to sdhci object
 * \param speed_mode: sdhci speed-mode
 * \param mmc_vdd : mmc vdd volatage (1.8V or 3.3V)
 * \param mmcm_clock: setup mmcmc clocks
 * \returns 0 always
 */
int sdhci_enable_clock(struct sdhci_t *sdhci, uint8_t speed_mode,
                       uint8_t mmc_vdd, uint32_t mmc_clock)
{
    addr_t io_mem = sdhci->io_base;
    uint32_t freq_sel, val, speed_value;
    uint32_t mmcm_clock = mmc_clock * 1000000;
    uint8_t enable_1p8v;
    uint8_t is_emmc = sdhci->is_emmc_dev;

    clog_print(CLOG_INFO, "enable sdhci clock\n");
    /* switch to 1.8V */
    enable_1p8v = mmc_vdd;
    if (enable_1p8v)
        hal_reg16_set_field(sdhci->hal, (void *)(io_mem + HOST_CTRL2_R), 3, 1, 1);
    else
        hal_reg16_set_field(sdhci->hal, (void *)(io_mem + HOST_CTRL2_R), 3, 1, 0);

    /* set default 400Khz */
    freq_sel = FREQSEL_CLK_400KHZ;
    speed_value = DEFAULT_SPEED_VAL;

    if (is_emmc == 0) {
        switch (speed_mode) {
            case SDR12_DS_SPEED_MODE:
                speed_value = SDR12_DS_SPEED_VAL;
                break;
            case SDR25_HS_SPEED_MODE:
                speed_value = SDR25_HS_SPEED_VAL;
                break;
            case SDR50_SPEED_MODE:
                speed_value = SDR50_SPEED_VAL;
                break;
            case SDR104_SPEED_MODE:
                speed_value = SDR104_SPEED_VAL;
                break;
            case DDR50_SPEED_MODE:
                speed_value = DDR50_SPEED_VAL;
                break;
            default:
                break;
        }
        freq_sel = (mmcm_clock / speed_value) - 1;
    } else {
        /* set default 400Khz */
        freq_sel = FREQSEL_CLK_400KHZ;
        switch (speed_mode) {
            case XEMMC_DS_SPEED_MODE:
                freq_sel = FREQSEL_SDR12;
                break;
            case XEMMC_HS_SPEED_MODE:
                freq_sel = FREQSEL_SDR25;
                break;
            case XEMMC_HSDDR_SPEED_MODE:
                freq_sel = FREQSEL_DDR50;
                break;
            case XEMMC_HS200_SPEED_MODE:
            case XEMMC_HS400_SPEED_MODE:
                break;
            default:
                break;
        }
    }

    if (mmcm_clock == MMCM_CLK_400KHZ) {
        freq_sel = FREQSEL_CLK_400KHZ;
    } else {
        clock_enable(sdhci->mmcm_clk, mmcm_clock);
    }

    /* configure clkctrl to select the mmc clock */
    val = (freq_sel << 8) | INTERNAL_CLK_EN | PLL_ENABLE;

    hal_write16(sdhci->hal, (void *)(io_mem + CLK_CTRL_R), val);
    os_delay_ms(100);
    hal_write16(sdhci->hal, (void *)(io_mem + CLK_CTRL_R), val | SD_CLK_EN);
    hal_write8(sdhci->hal, (void *)(io_mem + SW_RST_R), 0x6);

    return 0;
}

/**
 * \brief sdhci_set_voltage
 *	configure sdhci VDD1 or VDD2 voltage
 * \param sdhci: pointer to sdhci object
 * \param mmc_vdd_sel: mmc vdd selection
 * \param mmc_vdd_vol: mmc vdd volatge
 * \returns 0
 */
int sdhci_set_voltage(struct sdhci_t *sdhci, uint8_t mmc_vdd_sel, uint8_t mmc_vdd_volt)
{
    addr_t io_mem = sdhci->io_base;
    uint8_t val = 0, vdd = EMMC_VDD1_3P3_VOLT;
    uint8_t is_emmc = sdhci->is_emmc_dev;

    clog_print(CLOG_INFO, "set speed mode %d\n", speed_mode);

    switch (mmc_vdd_sel) {
        case XSD_BUS_PWR_VDD1:
            /* select vdd voltage based on sd or eMMC device */
            switch (mmc_vdd_volt) {
                case XVDD1_1P8_VOLT:
                    if (is_emmc)
                        vdd = EMMC_VDD1_1P8_VOLT;
                    else
                        vdd = SD_VDD1_1P8_VOLT;
                    break;
                case XVDD1_3P0_VOLT:
                    if (is_emmc)
                        return ERR_INVARG;
                    vdd = SD_VDD1_3P0_VOLT;
                    break;
                case XVDD1_1P2_VOLT:
                    if (is_emmc)
                        vdd = EMMC_VDD1_1P2_VOLT;
                    else
                        return ERR_INVARG;
                    break;
                case XVDD1_3P3_VOLT:
                    vdd = SD_VDD1_3P3_VOLT;
                    break;
                case XVDD1_POWER_OFF:
                    vdd = 0;
                    break;
                default:
                    return ERR_INVARG;
            }

            if (mmc_vdd_volt != XVDD1_POWER_OFF)
                val = (vdd << 1) | SD_BUS_PWR_VDD1;
            break;
        case XSD_BUS_PWR_VDD2:
            val = (mmc_vdd_volt << 5) | SD_BUS_PWR_VDD2;
            break;
        default:
            return ERR_INVARG;
    }

    /* set bus power */
    hal_write8(sdhci->hal, (void *)(io_mem + PWR_CTRL_R), val);
    return 0;
}

/**
 * \brief sdhci_set_speed_mode
 *	set the speed mode
 * \param sdhci: pointer to sdhci object
 * \param speed_mode: sdhci speed-mode
 * \returns none
 */
int sdhci_set_speed_mode(struct sdhci_t *sdhci, uint8_t speed_mode)
{
    addr_t io_mem = sdhci->io_base;
    uint8_t uhs_mode = 0;
    uint8_t is_emmc = sdhci->is_emmc_dev;

    clog_print(CLOG_INFO, "set speed mode %d\n", speed_mode);

    if (is_emmc == 0) {
        switch (speed_mode) {
            case SDR12_DS_SPEED_MODE:
                uhs_mode = UHS_MODE_SDR12;
                break;
            case SDR25_HS_SPEED_MODE:
                uhs_mode = UHS_MODE_SDR25;
                break;
            case SDR50_SPEED_MODE:
                uhs_mode = UHS_MODE_SDR50;
                break;
            case SDR104_SPEED_MODE:
                uhs_mode = UHS_MODE_SDR104;
                break;
            case DDR50_SPEED_MODE:
                uhs_mode = UHS_MODE_DDR50;
                break;
            default:
                uhs_mode = 0;
                break;
        }
    } else {
        switch (speed_mode) {
            case XEMMC_DS_SPEED_MODE:
                uhs_mode = UHS_MODE_EMMC_DS;
                break;
            case XEMMC_HS_SPEED_MODE:
                uhs_mode = UHS_MODE_EMMC_HS;
                break;
            case XEMMC_HSDDR_SPEED_MODE:
                uhs_mode = UHS_MODE_EMMC_HSDDR;
                break;
            case XEMMC_HS200_SPEED_MODE:
                uhs_mode = UHS_MODE_EMMC_HS200;
                break;
            case XEMMC_HS400_SPEED_MODE:
                uhs_mode = USH_MODE_EMMC_HS400;
                break;
            default:
                uhs_mode = 0;
                break;
        }
    }

    /* set speed mode */
    hal_write16(sdhci->hal, (void *)(io_mem + HOST_CTRL2_R),
                SIGNAL_1P8V_EN | uhs_mode);
    os_delay_ms(10);
    return 0;
}

/**
 * \brief sdhci_set_data_width
 *	set sd bus data width
 * \param sdhci: pointer to sdhci object
 * \param width: bus width 4 or 8bit
 * \returns none
 */
int sdhci_set_data_width(struct sdhci_t *sdhci, uint8_t width)
{
    addr_t io_mem = sdhci->io_base;
    uint32_t val;

    /* set data width */
    val = 0x00;
    if (width == SD_BUSWIDTH_8)
        val |= (HIGH_SPEED_EN | EXTDAT_XFER_WIDTH8);
    else if (width == SD_BUSWIDTH_4)
        val |= DAT_XFER_WIDTH_4BIT;

    hal_write8(sdhci->hal, (void *)(io_mem + HOST_CTRL1_R), val);
    return 0;
}

/**
 * \brief sdhci_alloc
 *	allocate sdhci instance
 * \param none:
 * \returns return pointer to sdhci obj
 */
struct sdhci_t *sdhci_alloc(void)
{
    int i;

    for (i = 0; i < MAX_SDHCI_INSTANCES; ++i)
        if (g_sdhci[i].assigned == 0)
            break;

    if (i >= MAX_SDHCI_INSTANCES)
        return NULL;

    g_sdhci[i].id = i;
    g_sdhci[i].assigned = 1;
    return &g_sdhci[i];
}

/**
 * \brief sdhci_register
 *	register sdhci controller
 * \param base_addr: base address of sdhci controller
 * \param irq: irq number
 * \returns pointer to sdhci instance
 */
struct sdhci_t *sdhci_register(uint32_t instance_id)
{
    struct sdhci_t *sdhci;
    addr_t base_addr;
    uint8_t irq;
    int retval;

    base_addr  = sdhci_get_base(instance_id);
    sdhci = sdhci_alloc();
    if (sdhci == NULL)
        return NULL;

    sdhci->io_base = base_addr;
    sdhci->reg = (struct sdhci_reg_t *)base_addr;

    irq = sdhci_get_irq_index(instance_id);
    sdhci->irq = irq;
    sdhci_irq_register(instance_id, irq);

    retval = sdhci_hal_register(sdhci);
    if (retval < 0) {
        clog_print(CLOG_LEVEL5, "failed to register hal\n");
        return NULL;
    }

    clog_print(CLOG_LEVEL6, "sdhci: registered sdhci instance-%d sdhci(%x)\n", sdhci->id, sdhci);

    return sdhci;
}


/**
 * \brief sdhci_send_cmd
 *	configure sdhci to send sd command
 * \param sdhci: pointer to sdhci object
 * \param io_req: pointer to io_req_t
 * \returns none
 */
int sdhci_send_cmd(struct sdhci_t *sdhci, struct io_req_t *io_req)
{
    struct mmc_req_t *req = io_req->req;
    struct mmc_dev_t *mmc_dev = req->mmc_dev;
    addr_t io_mem = sdhci->io_base;
    uint8_t use_dat = 1;
    uint32_t pstate_reg;
    uint16_t xfer_mode;

    clog_print(CLOG_LEVEL9, "sdhci_send_cmd\n");
    if (io_req->issued) {
        clog_print(CLOG_LEVEL5, "Bug: wrong state, cmd already issued\n");
        return io_req->status;
    }

    /* check if PSTATE_REG.CMD_INHIBIT = 0 ? */
    pstate_reg = hal_read(sdhci->hal, (void *)(io_mem + PSTATE_REG_R));
    if (pstate_reg & CMD_INHIBIT) {
        clog_print(CLOG_ERR, "cannot issue cmd, cmd line busy, pst_reg=%x\n", pstate_reg);
        return io_req->status;
    }

    if (use_dat && (pstate_reg & CMD_INHIBIT_DAT)) {
        clog_print(CLOG_ERR, "cannot issue cmd, data line busy, pst_reg=%x\n", pstate_reg);
        return io_req->status;
    }

    /* if use_dat = 1 check if host driver issues abort command */
    mmc_dev->cmd_inprogress = 1;
    io_req->status = MMC_CMD_STATUS_IN_PROGRESS;
    io_req->complete = 0;
    io_req->issued = 1;
    sdhci->act_req = io_req;

    /* clear all interrupts */
    hal_write(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_R), 0xFFFFFFFF);

    xfer_mode = 0;
    clog_print(CLOG_LEVEL5, "CMD[0x%x], %s: req->flags=%x\n", req->cmdreg, __func__, req->flags);
    /* configure block size */
    if (req->flags & XFR_MODE_DATA_READ_F)
        xfer_mode |= XFR_MODE_DATA_READ;
    if (req->flags & XFR_MODE_MULTBLK_SEL_F)
        xfer_mode |= XFR_MODE_MULTBLK_SEL;
    if (req->flags & XFR_MODE_DMA_EN_F)
        xfer_mode |= XFR_MODE_DMA_EN;
    if (req->flags & XFR_MODE_AUTOCMD12_EN_F)
        xfer_mode |= XFR_MODE_AUTOCMD12_EN;
    if (req->flags & XFR_MODE_AUTOCMD23_EN_F)
        xfer_mode |= XFR_MODE_AUTOCMD23_EN;
    if (req->flags & XFR_MODE_RESP_ERRCHK_EN_F)
        xfer_mode |= XFR_MODE_RESP_ERRCHK_EN;
    if (req->flags & XFR_MODE_RESP_INT_EN_F)
        xfer_mode |= XFR_MODE_RESP_INT_EN;
    if (req->flags & XFR_MODE_BLKCNT_EN_F) {
        xfer_mode |= XFR_MODE_BLKCNT_EN;
        hal_write16(sdhci->hal, (void *)(io_mem + BLOCKSIZE_R),
                    req->blocksize & 0xFFF /*| SDMA_BUF_BDARY_512K*/);
        hal_write16(sdhci->hal, (void *)(io_mem + BLOCKCOUNT_R),
                    req->nblocks & 0xFFF);
    }
    if (req->flags & SDHCI_INTR_ENABLE)
        hal_write(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_EN_R),
                  req->int_mask);

    hal_write(sdhci->hal, (void *)(io_mem + ARGUMENT_R), req->args);

    hal_write(sdhci->hal, (void *)(io_mem + XFER_MODE_R),
              (xfer_mode | (req->cmdreg << 16)));

    return 0;
}

/**
 * \brief sdhci_wait_for_cmd_complete
 *	wait for command and do complete the io-request
 * \param sdhci: pointer to sdhci object
 * \returns status of io_request
 */
int sdhci_wait_for_cmd_complete(struct sdhci_t *sdhci)
{
    struct io_req_t *io_req = sdhci->act_req;

    uint8_t nxt_state = io_req->state;
    uint8_t cmd_end = 0;

    if (io_req->issued == 0) {
        clog_print(CLOG_LEVEL3, "warning: cmd not issued\n");
        io_req->status = MMC_CMD_STATUS_DONE;
        io_req->issued = 0;
        io_req->complete = 0;
        nxt_state = XSTATE0;
        sdhci->act_req = 0;
    } else {
        io_req->timeout--;

        if (io_req->complete) {
            clog_print(CLOG_LEVEL3, "%s: complete=%d task-id(%d)\n", __func__, io_req->complete,
                       io_req->task_id);
            if (io_req->error == 0)
                io_req->status = MMC_CMD_STATUS_DONE;
            else
                io_req->status = MMC_CMD_STATUS_FAILED;

            cmd_end = 1;
        } else if (io_req->timeout == 0) {
            clog_print(CLOG_LEVEL3, "%s: timeout=%d\n", __func__, io_req->timeout);
            io_req->status = MMC_CMD_STATUS_TIMEOUT;
            cmd_end = 1;
        }
        if (cmd_end) {
            io_req->issued = 0;
            io_req->complete = 0;
            nxt_state = XSTATE0;
            sdhci->act_req = 0;
            mmc_core_request_complete(io_req);
        }
    }

    return nxt_state;
}

/**
 * \brief sdhci_complete_request
 *	complete the io-req and giveback
 * \param sdhci: pointer to sdhci object
 * \returns none
 */
void sdhci_complete_request(struct sdhci_t *sdhci)
{
    struct io_req_t *io_req = sdhci->act_req;

    io_req->complete = 1;
    sdhci_wait_for_cmd_complete(sdhci);

    if(NULL != io_req->callback){
        clog_print(CLOG_LEVEL9, "io_req->callback() executed\n");
        io_req->callback();
    }
}

/**
 * \brief sdhci_handle_cmd_intr
 *	handle the command interrupt events
 * \param sdhci: pointer to sdhci object
 * \param mask: interrupt mask
 * \returns none
 */
int sdhci_handle_cmd_intr(struct sdhci_t *sdhci, uint32_t isr_status)
{
    struct io_req_t *io_req = sdhci->act_req;
    addr_t io_mem = sdhci->io_base;
    struct mmc_req_t *mmc_req;
    uint8_t req_complete = 0, i;

    mmc_req = io_req->req;

    /* handle command with/without data transfer */
    /*check cmd complete interrupt rcvd */
    if (isr_status & CMD_COMPLETE_SIGNAL_EN) {
        /* read response register */
        sdhci->intr_mask &= ~CMD_COMPLETE_SIGNAL_EN;

        for (i = 0; i < 4; ++i) {
            mmc_req->resp[i] = hal_read(sdhci->hal, (void *)(io_mem	+ RESP01_R + i * 4));
            clog_print(CLOG_LEVEL9, "%s: reg(%p) resp[%d]=%x reqflag=%x\n",__func__,
                       (io_mem + RESP01_R + i * 4), i, mmc_req->resp[i], mmc_req->flags);
        }

        if (mmc_req->flags & CMD_WITHOUT_DATA) {
            req_complete = 1;
        }
    }

	hal_write(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_R), isr_status);
    if (sdhci->intr_mask & SD_CMD_INTR_MASK) {
        clog_print(CLOG_LEVEL9, "unhandled interrupts %x\n", sdhci->intr_mask & SD_CMD_INTR_MASK);
    }

    io_req->error = isr_status >> 16;
    /* give back and complete the request */

    return req_complete;
}

/**
 * \brief sdhci_handle_data_intr
 *	handle the data interrupt events
 * \param sdhci: pointer to sdhci object
 * \param mask: interrupt mask
 * \returns none
 */
int sdhci_handle_data_intr(struct sdhci_t *sdhci, int32_t intr_mask)
{
    struct io_req_t *io_req = sdhci->act_req;
    addr_t io_mem = sdhci->io_base;
    struct mmc_req_t *mmc_req;
    uint8_t req_complete = 0;
    uint32_t data;
    int idx, i;

    mmc_req = io_req->req;
    if (mmc_req == NULL)
        return req_complete;

    /* handle command with/without data transfer */
    /*check cmd complete interrupt rcvd */
    if (intr_mask & BUF_RD_READY_SIGNAL_EN) {
        clog_print(CLOG_LEVEL3, "%s: buffer data Ready signal \n",__func__);
        /* read response register */
        sdhci->intr_mask &= ~BUF_RD_READY_SIGNAL_EN;

        idx = mmc_req->rx_data_len;
        clog_print(CLOG_LEVEL4, "%s:mmc_req->blocksize(%d), idx(%d) mmc_req->buflen(%d)\n", __func__,
                   mmc_req->blocksize, idx, mmc_req->buflen);
        clog_print(CLOG_LEVEL4, "%s: reading args/block-adr(%x)\n", __func__, mmc_req->args);
        for (i = 0; (i < mmc_req->blocksize/4) && (idx < mmc_req->buflen); ++i) {
            data = hal_read(sdhci->hal, (void *)(io_mem+ BUF_DATA_R));
            if (mmc_req->databuf && idx < mmc_req->buflen)
                *(uint32_t *)&mmc_req->databuf[idx] = data;
            else {
                clog_print(CLOG_ERR, "Error: DataLoss: insufficient read buffer\n");
                clog_print(CLOG_ERR, "databuf(%x), buf_offs(%d), buflen(%d)\n",
                           mmc_req->databuf, idx, mmc_req->buflen);
            }
            idx += 4;
        }
        mmc_req->rx_data_len = idx;
        clog_print(CLOG_LEVEL4, "\n");
    }

    if (intr_mask & BUF_WR_READY_SIGNAL_EN) {
        clog_print(CLOG_LEVEL3, "%s: buffer data Write ready signal len(%d)\n",__func__,
                   mmc_req->buflen);

        sdhci->intr_mask &= ~BUF_WR_READY_SIGNAL_EN;
        idx = mmc_req->tx_data_len;

        clog_print(CLOG_LEVEL4, "%s:mmc_req->blocksize(%d), idx(%d) mmc_req->buflen(%d)\n",
                   __func__, mmc_req->blocksize, idx, mmc_req->buflen);
        if (mmc_req->databuf && (idx < mmc_req->buflen)) {
            /* write data to write buffer */
            for (i = 0; (i < mmc_req->blocksize/4) && (idx < mmc_req->buflen); ++i) {
                hal_write(sdhci->hal, (void *)(io_mem+ BUF_DATA_R),
                          *(uint32_t *)&mmc_req->databuf[idx]);
                idx += 4;
            }
            mmc_req->tx_data_len = idx;
        }
    }

    return req_complete;
}

int sdhci_irq_register(uint32_t instance_id, uint8_t irq)
{
    int_group_isr_t func = sdhci_get_irq_handler(instance_id);

    bk_int_isr_register(irq, func, NULL);

    return 0;
}

int sdhci_irq_unregister(uint32_t instance_id)
{
    uint8_t irq = sdhci_get_irq_index(instance_id);
    
    bk_int_isr_unregister(irq);

    return 0;
}

/**
 * \brief sdhci_irq_common
 *	interrupt service routine
 * \param task_id: task-id
 * \param data: pointer to sdhci object
 * \returns none
 */
int sdhci_irq_common(uint8_t task_id, void *data)
{
    struct sdhci_t *sdhci = (struct sdhci_t *)data;
    addr_t io_mem = sdhci->io_base;
    uint32_t pstate_reg, intr_status;
    struct io_req_t *io_req;
    uint8_t req_complete = 0;

    clog_print(CLOG_LEVEL9, "sdhci_irq_common enter\n");

    io_req = sdhci->act_req;
    intr_status = hal_read(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_R));
    if (intr_status == 0 || intr_status == 0xffffffff) {
        clog_print(CLOG_LEVEL9, "no interrupt, intr_status = %x\n", intr_status);
    }

    sdhci->intr_status = intr_status;

    if (intr_status & SD_CARD_INTR_CHANGE) {
        pstate_reg = hal_read(sdhci->hal, (void *)(io_mem + PSTATE_REG_R));
        sdhci->card_present = !!(pstate_reg & CARD_INSERTED);
        clog_print(CLOG_LEVEL9, "Isr: Card status changed, card %s pstate=%x\n", sdhci->card_present ?
                   "inserted" : "removed", pstate_reg);
        /* TODO: whether to disable the card inserted/removal interrupt*/
        if (intr_status & CARD_INSERTION_SIGNAL_EN) {
            hal_reg32_set_field(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_EN_R),
                                CARD_INSERTION_SIGNAL_EN, 1, 0);
        }
    }

    /* clear the interrupt status */
    if (intr_status) {
        clog_print(CLOG_INFO, "[ISR]: %s, intr_status = %x\n", __func__, intr_status);
        clog_print(CLOG_INFO, "Intr_status before clearing = %0x\n",
                   hal_read(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_R)));
        hal_write(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_R), intr_status);
    }

    if (intr_status & CARD_INTERRUPT_SIGNAL_EN) {
        clog_print(CLOG_LEVEL9, "Disabling card interrupt\n");
        hal_reg32_set_field(sdhci->hal, (void *)(io_mem + NORMAL_INT_STAT_EN_R),
                            8, 1, 0);
    }
    if (sdhci->act_req == NULL && intr_status) {
        clog_print(CLOG_LEVEL3, "%s: No Request queued\n", __func__);
        goto irq_exit;
    }

    if (intr_status)
        clog_print(CLOG_LEVEL9, "=========== %s intr_stat(%x)==========\n", __func__, intr_status);
    if (intr_status & SD_CMD_INTR_MASK) {
        req_complete |= sdhci_handle_cmd_intr(sdhci, intr_status);
    }

    if (intr_status & SD_DAT_INTR_MASK) {
        req_complete |= sdhci_handle_data_intr(sdhci, intr_status);
    }

    if (intr_status & SD_BUS_PWR_INTR_MASK) {
        clog_print(CLOG_ERR, "Isr: SD buspower, Current Limit interrupt, intr_status(%x)\n",
                   intr_status);
    }

    if (intr_status & XFER_COMPLETE_SIGNAL_EN) {
        req_complete = 1;
        clog_print(CLOG_LEVEL5, "XFER_COMPLETE_SIGNAL_EN\n");
    }

    if (intr_status & SD_ERROR_MASK) {
        req_complete = 1;
        io_req->error = intr_status & SD_ERROR_MASK;
        clog_print(CLOG_LEVEL5, "SD_ERROR_MASK: 0x%x:\n", io_req->error);
    }

    if (req_complete) {
        clog_print(CLOG_LEVEL5, "sdhci_complete_request\n");
        sdhci_complete_request(sdhci);
    }

irq_exit:

    return 1;
}

/**
 * \brief sdhci_init
 *	initialize the sdhci host controller
 * \param sdhci: pointer to sdhci object
 * \returns 0 on success
 */
int sdhci_init(struct sdhci_t *sdhci)
{
    int ret;
    uint32_t pstate_reg;
    struct sdhci_reg_t *reg;

    if (sdhci == NULL)
        return ERROR_NOMEM;

    sdhci->reg = (struct sdhci_reg_t *)sdhci->io_base;
    sdhci->irq_handler = sdhci_irq_common;
    sdhci->host.drv_data = sdhci;
    sdhci->host.ops.send_cmd = sdhci_send_cmd;
    sdhci->host.ops.enable_clock = sdhci_enable_clock;
    sdhci->host.ops.disable_clock = sdhci_disable_clock;
    sdhci->host.ops.set_data_width = sdhci_set_data_width;
    sdhci->host.ops.set_speed = sdhci_set_speed_mode;
    sdhci->host.ops.set_voltage = sdhci_set_voltage;
    sdhci->host.ops.io_ctrl = sdhci_io_ctrl;
    sdhci->is_emmc_dev = is_emmc_dev;

    sdhci_hal_config_system_base(sdhci);
    sdhci->vendor1_offs = hal_read(sdhci->hal, (void *)(sdhci->io_base + P_VENDOR_1_SPECIFIC_AREA));
    sdhci->mmcm_clk = mmcm_clk_module_init(sdhci->hal,
                                           (void *)(sdhci->sys_base + MMCM_REG_OFFS), "mmcm");
    if (sdhci->mmcm_clk == NULL)
        clog_print(CLOG_ERR, "Failed to initialize mmcm clk module\n");
    else {
        sdhci->mmcm_clk->instance_id = sdhci->instance_index;
        clock_init(sdhci->mmcm_clk, NULL);
    }

    sdhci->act_req = 0;

    /* init system setting, and io matrix*/
    ret = sdhci_hal_init(sdhci);
    if(ret){
        return ERROR_OPER_FAIL;
    }

    /* set power */
    sdhci_io_ctrl(sdhci, SDHCI_CLOCK_OFF, 0);
    sdhci_io_ctrl(sdhci, SDHCI_POWER_OFF, 0);

    /* delay for 100ms */
    os_delay_ms(100);

    /* reset sdhci controller */
    sdhci_reset(sdhci, SW_RST_ALL | SW_RST_CMD | SW_RST_DAT);
    pstate_reg = sdhci_get_pstate(sdhci);
    sdhci->card_present = ((pstate_reg & 0x10000) == 0x10000);

    /* enable the power */
    sdhci_io_ctrl(sdhci, SDHCI_POWER_ON, XVDD1_3P3_VOLT);

    /* enable the clock 400Khz */
    sdhci_io_ctrl(sdhci, SDHCI_CLOCK_ON, FREQ_SEL_VAL);
    sdhci_io_ctrl(sdhci, SDHCI_SET_TXPHASE, tx_phase);

    sdhci_io_ctrl(sdhci, SDHCI_SET_MMC_DEV_TYPE, sdhci->is_emmc_dev);
    sdhci_io_ctrl(sdhci, SDHCI_SET_VERSION, 0);
    sdhci_io_ctrl(sdhci, SDHCI_SET_DATA_TIMEOUT_COUNTER, 0x0e);

    /* enable the interrupts */
    sdhci_disable_intr(sdhci);

    /* enable the interrupts */
    sdhci_enable_intr(sdhci);

    clog_print(CLOG_LEVEL5, "%s: sdhci(%x)\n",__func__, sdhci);
    #if CONFIG_MSHC_PIO
    sdhci->isr_task_id = ctask_create("sdhci-isr", sdhci_irq_common, sdhci, 0, 0, 0);
    ctask_enable(sdhci->isr_task_id);
    #elif CONFIG_MSHC_GICV2
    #else
    uio_set_irq(sdhci->uio, sdhci_irq_common, sdhci);
    #endif

    reg = sdhci->reg;
    mem_dump(sdhci->hal, reg, sizeof(struct sdhci_reg_t), 4);

    return 0;
}

/**
 * \brief sdhci_deinit
 *	initialize the sdhci host controller
 * \param sdhci: pointer to sdhci object
 * \returns 0 on success
 */
int sdhci_deinit(struct sdhci_t *sdhci)
{
    if (sdhci == NULL)
        return ERROR_NOMEM;

    sdhci_disable_intr(sdhci);
    sdhci_io_ctrl(sdhci, SDHCI_POWER_OFF, 0);
    sdhci_io_ctrl(sdhci, SDHCI_CLOCK_OFF, 0);

    sdhci->reg = NULL;
    sdhci->irq_handler = NULL;
    sdhci->host.drv_data = NULL;
    sdhci->host.ops.send_cmd = NULL;
    sdhci->host.ops.enable_clock = NULL;
    sdhci->host.ops.disable_clock = NULL;
    sdhci->host.ops.set_data_width = NULL;
    sdhci->host.ops.set_speed = NULL;
    sdhci->host.ops.set_voltage = NULL;
    sdhci->host.ops.io_ctrl = NULL;
    sdhci->is_emmc_dev = 0;

    return 0;
}

void sdhci_irq_instance0(void)
{
    uint32_t instance = 0;
    struct sdhci_t *sdhci_0;

    sdhci_0 = sd_card_get_private_data(instance);
    sdhci_irq_common(instance, sdhci_0);
}

void sdhci_irq_instance1(void)
{
    uint32_t instance = 1;
    struct sdhci_t *sdhci_1;

    sdhci_1 = sd_card_get_private_data(instance);
    sdhci_irq_common(instance, sdhci_1);
}

/**
 * \brief sdhci_module_init
 *	initialize sdhci module
 * \param none:
 * \returns 0 on success
 */
int sdhci_module_init(uint32_t instance_id, struct sdhci_t **sdhci_obj)
{
    int ret;
    struct sdhci_t *sdhci;

    clog_print(CLOG_LEVEL5, "sdhci_module_init\n");
    sdhci = sdhci_register(instance_id);
    if (sdhci == NULL) {
        clog_print(CLOG_LEVEL5, "sdhci: unable to allocate sdhci instance-0\n");
        return ERROR_OPER_FAIL;
    }

    sdhci->instance_index = instance_id;

    clog_print(CLOG_LEVEL5, "%s: sdhci(%x)\n",__func__, sdhci);
    ret = sdhci_init(sdhci);
    if (ret < 0) {
        clog_print(CLOG_LEVEL5, "sdhci: failed to init-0\n");
        return ERROR_OPER_FAIL;
    }

    *sdhci_obj  = sdhci;

    return 0;
}
// eof

