/**
 ***************************************************************************************
 * @file     ct_sdhci.c
 * @author   ravibabu@synopsys.com
 * @version  00.00.01
 * @date     21 June 2019
 * @brief    sdhci driver
 *
 ***************************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright(C) 2017 Synopsys, Inc. All rights reserved</center></h2>
 *
 * This Synopsys software and all associated documentation are proprietary to
 * Synopsys, Inc. And may only be used pursuant to the terms and conditions of
 * a written license agreement with Synopsys, Inc. All other use, reproduction,
 * modification, or distribution of the Synopsys software or the associated
 * documentation is strictly prohibited.
 ***************************************************************************************
 @verbatim
   REVISION HISTORY:
   Version    Date         Author       	  Change Id     Description
   00.00.00   2019-06-21   ravibabu@synopsys.com    -           dev in progress
 @endverbatim
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "cee.h"
#include "ctask.h"
#include "cmsg_queue.h"
#include "clog.h"
#include "cwatch.h"
#include "mmc_dev.h"

#include "hal.h"
#include "sd_cmd.h"
#include "dwc_msdc_cfg.h"
#include "mmc_dev.h"
#include "sdhci.h"
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"
#include "sys_a35_ll.h"
#include "sys_ana_ll.h"
#include "sdio0_ll.h"
#include "sdio1_ll.h"

/* module global datas */

/* external definitions */
#ifdef CONFIG_MSHC_CSIM
extern uint32_t sdhci_sim_read(void *pdata, void *addr, uint8_t grain);
extern uint32_t sdhci_sim_write(void *pdata, void *addr, uint32_t val, uint8_t grain);
#endif
#ifdef CONFIG_PLATFORM_RTLSIM
extern uint32_t rtl_sim_write(void *pdata, void *addr, uint32_t val, uint8_t grain);
extern uint32_t rtl_sim_read(void *pdata, void *addr, uint8_t grain);
#endif

/* hal functions */
/**
 * \brief sdhci_iomem_read
 *	read 8/16/32/bit from sdhci register
 * \param pdata: pointer to sdhci
 * \param addr: register address
 * \param grain: granularity 8/16/32 bit access
 */
uint32_t sdhci_iomem_read(void *pdata, void *addr, uint8_t grain)
{
    uint32_t val = 0;
    struct sdhci_t *sdhci = (struct sdhci_t *)pdata;
    addr_t offs;

    if (((addr_t)addr & 0xFFFF0000) == 0) {
        offs = (addr_t)addr & 0xFFFF;
    } else
        offs = (addr_t)addr - (addr_t)sdhci->reg;

    switch (grain) {

        case 1: /* 8 bit read */
            val = *(volatile uint8_t *)(sdhci->io_base + (offs));
            val &= 0xFF;
            clog_print(CLOG_LEVEL9, "Write[%08x] = %02X\n", (addr_t)addr, val);
            break;

        case 2: /* 16 bit read */
            val = *(volatile uint16_t *)(sdhci->io_base + (offs));
            val &= 0xFFFF;
            clog_print(CLOG_LEVEL9, "Read[%08x] = %04X\n", (addr_t)addr, (uint16_t)val);
            break;

        default: /* 32 bit read */

            val = *(volatile uint32_t *)(sdhci->io_base + (offs));
            clog_print(CLOG_LEVEL9, "Read[%08x] = %08X\n", (addr_t)addr, val);
            break;
    }
    return val;
}

/**
 * \brief sdhci_iomem_write
 *	write 8/16/32/bit data to sdhci register
 * \param pdata: pointer to sdhci
 * \param addr: register address
 * \param grain: granularity 8/16/32 bit access
 */
uint32_t sdhci_iomem_write(void *pdata, void *addr, uint32_t val, uint8_t grain)
{
    struct sdhci_t *sdhci = (struct sdhci_t *)pdata;
    addr_t offs;

    if (((addr_t)addr & 0xFFFF0000) == 0) {
        offs = (addr_t)addr & 0xFFFF;
    } else
        offs = (addr_t)addr - (addr_t)sdhci->reg;

    switch (grain) {
        case 1: /* 8 bit write */
            val &= 0xFF;
            *(volatile uint8_t *)(sdhci->io_base + offs) = val;
            clog_print(CLOG_LEVEL9, "Write[%08x] = %02X\n", (addr_t)addr, val);
            break;
        case 2: /* 16 bit write */
            val &= 0xFFFF;
            *(volatile uint16_t *)(sdhci->io_base + offs) = val;
            clog_print(CLOG_LEVEL9, "Write[%08x] = %04X\n", (addr_t)addr, val);
            break;
        default : /* 32 bit write */
            *(volatile uint32_t *)(sdhci->io_base + offs) = val;
            clog_print(CLOG_LEVEL9, "Write[%08x] = %08X\n", (addr_t)addr, val);
            break;
    }
    return 0;
}


/**
 * \brief sdhci_read
 *	wrapper API to read 8/16/32/bit data from sdhci register
 * \param pdata: pointer to sdhci
 * \param addr: register address
 * \param grain: granularity 8/16/32 bit access
 */
uint32_t sdhci_read(void *pdata, void *addr, uint8_t grain)
{
    uint32_t val = 0;
    struct sdhci_t *sdhci = (struct sdhci_t *)pdata;
    uint32_t offs = (addr_t)addr - (addr_t)sdhci->reg;

    if (sdhci->pal)
        val = sdhci->pal->read(sdhci, (void *)(sdhci->io_base + (offs)), grain);

    return val;
}

/**
 * \brief sdhci_write
 *	Wrapper API to write 8/16/32/bit data to sdhci register
 * \param pdata: pointer to sdhci
 * \param addr: register address
 * \param grain: granularity 8/16/32 bit access
 */
uint32_t sdhci_write(void *pdata, void *addr, uint32_t val, uint8_t grain)
{
    struct sdhci_t *sdhci = (struct sdhci_t *)pdata;
    uint32_t offs = (addr_t)addr - (addr_t)sdhci->reg;

    if (sdhci->pal && sdhci->pal->write)
        return sdhci->pal->write(sdhci, (void *)(sdhci->io_base + (offs)), val, grain);

    return ERROR_INVARG;
}

/* FIXME:wangzhilei*/
void sdhci_gpio_init(gpio_id_t id, uint32_t drive_level)
{
    bk_gpio_set_capacity(id, drive_level);
    bk_gpio_set_output_high(id);
    bk_gpio_pull_up(id);
}

int sdhci_hal_init(struct sdhci_t *sdhci)
{
    bk_err_t ret = MSHC_SUCESS;

#define CONFIG_SUPPORT_NEGATIVE_EDGE_SENDING        (1)

    sys_a35_ll_set_int_en0_inten_fora35_sdio0(1);
    sys_a35_ll_set_int_en0_inten_fora35_sdio1(1);

    /* TODO: CPU CLOCK:960MHz/2*/
    sys_a35_ll_set_clk_ctrl0_cksel_cpu(3);
    sys_a35_ll_set_clk_ctrl0_ckdiv_cpu(1);

    sys_ana_ll_set_reg0_spitrig(1);
    sys_ana_ll_set_reg0_spitrig(0);
    sys_ana_ll_set_reg5_en_vout(1);

    if(sdhci->instance_index == 0)
    {
        ret = gpio_dev_map(GPIO_2, GPIO_DEV_SD0_CLK);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_2, 3);
        ret = gpio_dev_map(GPIO_3, GPIO_DEV_SD0_CMD);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_3, 3);
        ret = gpio_dev_map(GPIO_4, GPIO_DEV_SD0_DATA_0);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_4, 3);
        ret = gpio_dev_map(GPIO_5, GPIO_DEV_SD0_DATA_1);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_5, 3);
        ret = gpio_dev_map(GPIO_10, GPIO_DEV_SD0_DATA_2);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_10, 3);
        ret = gpio_dev_map(GPIO_11, GPIO_DEV_SD0_DATA_3);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_11, 3);

        ret = gpio_dev_map(GPIO_6, GPIO_DEV_SD0_DATA_4);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_6, 3);
        ret = gpio_dev_map(GPIO_7, GPIO_DEV_SD0_DATA_5);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_7, 3);
        ret = gpio_dev_map(GPIO_8, GPIO_DEV_SD0_DATA_6);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_8, 3);
        ret = gpio_dev_map(GPIO_9, GPIO_DEV_SD0_DATA_7);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_9, 3);

        sys_a35_ll_set_clk_en_sdio0_cken(1);
        sdio0_ll_set_clkg_reset_soft_resetn(1);
        sdio0_ll_set_clkg_reset_bps_clkgate(1);
        sdio0_ll_set_div_ctrl_tmclk_div(CONFIG_DEFAULT_TMCLK_DIV);
        sdio0_ll_set_div_ctrl_cqet_mclk_div(CONFIG_DEFAULT_CQET_MCLK_DIV);

        #if CONFIG_SUPPORT_NEGATIVE_EDGE_SENDING
        /* if no negative edge sending, data crc exception during writing block*/
        sdio0_ll_set_sdio_ctrl_clk_drv_negedge_sel(1);
        #endif
    }
    else
    {
        ret = gpio_dev_map(GPIO_14, GPIO_DEV_SD0_CLK);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_14, 3);
        ret = gpio_dev_map(GPIO_15, GPIO_DEV_SD0_CMD);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_15, 3);
        ret = gpio_dev_map(GPIO_16, GPIO_DEV_SD0_DATA_0);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_16, 3);
        ret = gpio_dev_map(GPIO_17, GPIO_DEV_SD0_DATA_1);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_17, 3);
        ret = gpio_dev_map(GPIO_18, GPIO_DEV_SD0_DATA_2);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_18, 3);
        ret = gpio_dev_map(GPIO_19, GPIO_DEV_SD0_DATA_3);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_19, 3);

        ret = gpio_dev_map(GPIO_20, GPIO_DEV_SD0_DATA_4);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_20, 3);
        ret = gpio_dev_map(GPIO_21, GPIO_DEV_SD0_DATA_5);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_21, 3);
        ret = gpio_dev_map(GPIO_22, GPIO_DEV_SD0_DATA_6);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_22, 3);
        ret = gpio_dev_map(GPIO_23, GPIO_DEV_SD0_DATA_7);
        if(BK_OK != ret){
            ret = ERROR_IO_NOT_SUPPORTED;
            goto hal_exit;
        }
        sdhci_gpio_init(GPIO_23, 3);

        sys_a35_ll_set_clk_en_sdio1_cken(1);
        sdio1_ll_set_clkg_reset_soft_resetn(1);
        sdio1_ll_set_clkg_reset_bps_clkgate(1);
        sdio1_ll_set_div_ctrl_tmclk_div(CONFIG_DEFAULT_TMCLK_DIV);
        sdio1_ll_set_div_ctrl_cqet_mclk_div(CONFIG_DEFAULT_CQET_MCLK_DIV);

        #if CONFIG_SUPPORT_NEGATIVE_EDGE_SENDING
        sdio1_ll_set_sdio_ctrl_clk_drv_negedge_sel(1);
        #endif
    }

hal_exit:
    return ret;
}

int sdhci_hal_config_system_base(struct sdhci_t *sdhci)
{
    sdhci->sys_base = SOC_SYSCFG_REG_BASE;

    return 0;
}

/**
 * \brief sdhci_hal_register
 *	register to sdhci driver to hal layer
 * \param sdhci: pointer to sdhci object
 * \returns 0 on success else fail -ve
 */
int sdhci_hal_register(struct sdhci_t *sdhci)
{
    sdhci->hal = hal_register(sdhci_read, sdhci_write, sdhci);
    if (sdhci->hal == NULL)
        return ERROR_OPER_FAIL;

    #if CONFIG_PLATFORM_CSIM_SDLINK
    sdhci->pal = hal_register(sdhci_sim_read, sdhci_sim_write, sdhci);
    #elif CONFIG_PLATFORM_RTLSIM
    /* add rtssim or vcs_read/vcs_write */
    sdhci->pal = hal_register(rtl_sim_read, rtl_sim_write, sdhci);
    #else
    sdhci->pal = hal_register(sdhci_iomem_read, sdhci_iomem_write, sdhci);
    #endif
    if (sdhci->hal == NULL)
        return ERROR_OPER_FAIL;

    return 0;
}
