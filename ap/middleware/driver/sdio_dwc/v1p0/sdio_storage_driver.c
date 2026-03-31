// Copyright 2020-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <driver/io_matrix.h>
#include <driver/hal/hal_gpio_types.h>
#include "sdio_storage_driver.h"
#include "sys_a35_ll.h"
#include "sys_ana_ll.h"
#include <driver/gicv2.h>
#include <driver/int_types.h>
#include <driver/int.h>

#define INIT_400K     1

uint32 adma3_wr_descriptor_addr[42];
uint32 adma3_rd_descriptor_addr[42];
volatile uint8_t CMD_COMPLETE_STATE =0;
volatile uint8_t XFER_COMPLETE_STATE =0;
volatile uint8_t BGAP_EVENT_STATE =0;
volatile uint8_t DMA_INTERRUPT_STATE =0;
volatile uint8_t BUF_WR_READY_STATE =0;
volatile uint8_t BUF_RD_READY_STATE =0;
volatile uint8_t CARD_INSERTION_STATE =0;
volatile uint8_t CARD_REMOVAL_STATE =0;
volatile uint8_t CARD_INTERRUPT_STATE =0;
volatile uint8_t INT_A_STATE =0;
volatile uint8_t INT_B_STATE =0;
volatile uint8_t INT_C_STATE =0;
volatile uint8_t RE_TUNE_EVENT_STATE =0;
volatile uint8_t FX_EVENT_STATE =0;
volatile uint8_t CQE_EVENT_STATE =0;
volatile uint8_t ERROR_INTERRUPT_STATE=0;
volatile uint8_t CMD_TOUT_ERR_STATE =0;
volatile uint8_t CMD_CRC_ERR_STATE =0;
volatile uint8_t CMD_END_BIT_ERR_STATE =0;
volatile uint8_t CMD_IDX_ERR_STATE =0;
volatile uint8_t DATA_TOUT_ERR_STATE =0;
volatile uint8_t DATA_CRC_ERR_STATE =0;
volatile uint8_t DATA_END_BIT_ERR_STATE =0;
volatile uint8_t CUR_LMT_ERR_STATE =0;
volatile uint8_t AUTO_CMD_ERR_STATE =0;
volatile uint8_t ADMA_ERR_STATE =0;
volatile uint8_t TUNING_ERR_STATE =0;
volatile uint8_t RESP_ERR_STATE =0;
volatile uint8_t BOOT_ACK_ERR_STATE =0;
volatile uint8_t VENDOR_ERR1_STATE =0;
volatile uint8_t VENDOR_ERR2_STATE =0;
volatile uint8_t VENDOR_ERR3_STATE =0;
uint32_t sdio_rca = 0xAAAA0000;

void spe_delay( volatile unsigned int times)
{
        while(times--){
                ;
        }
}

#define  GPIO_CFG(port)    *((volatile unsigned int *) (0x44000400+port*4))

void sdio_gpio_init(uint8_t io_pos)
{
    if(io_pos == 0)
    {
        GPIO_CFG(2) = 0x0378;
        GPIO_CFG(3) = 0x0378;
        GPIO_CFG(4) = 0x0378;
        GPIO_CFG(5) = 0x0378;
        GPIO_CFG(10) = 0x0378;
        GPIO_CFG(11) = 0x0378;

        SDIOD_LOGI("GPIO 2~11\r\n");
        bk_iomx_set_gpio_func(GPIO_2, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_3, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_4, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_5, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_10, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_11, FUNC_CODE_129);

        bk_iomx_set_gpio_func(GPIO_6, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_7, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_8, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_9, FUNC_CODE_129);
    }
    else
    {
        SDIOD_LOGI("GPIO 14~23\r\n");
        bk_iomx_set_gpio_func(GPIO_14, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_15, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_16, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_17, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_18, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_19, FUNC_CODE_129);

        bk_iomx_set_gpio_func(GPIO_20, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_21, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_22, FUNC_CODE_129);
        bk_iomx_set_gpio_func(GPIO_23, FUNC_CODE_129);
    }
}

void detect_card(uintptr_t addr)
{
    uint16 card_inserted;

    NORMAL_INT_STAT_EN_R(addr)= CARD_INSERTION_SIGNAL_EN | CARD_REMOVAL_SIGNAL_EN;
    NORMAL_INT_SIGNAL_EN_R(addr)= CARD_INSERTION_SIGNAL_EN | CARD_REMOVAL_SIGNAL_EN;

    while((CARD_INSERTION_STATE==0) && (CARD_REMOVAL_STATE==0));
    card_inserted=((PSTATE_REG_R(addr)&(1<<16)) && CARD_INSERTION_STATE);
    if(card_inserted)
    {
        CARD_INSERTION_STATE=0;
    }
    else
    {
        SDIOD_LOGI("Card is inserted error\r\n");
        CARD_REMOVAL_STATE=0;
    }
}

void host_ctrl_set(uintptr_t addr,uint8 SD_BUS_VOL_VDD1,uint8 TOUT_CNT,uint8 CARD_IS_EMMC,uint8 DAT_XFER_WIDTH)
{
    uint16_t vers;
    uint8 data_width;

    PWR_CTRL_R(addr)= SD_BUS_VOL_VDD1;//PWR_CTRL_R.SD_BUS_VOL_VDD1=3.3v: 0x0e
    TOUT_CTRL_R(addr)= TOUT_CNT;      //TOUT_CTRL_R.TOUT_CNT=TMCLK x 2^13

    SNPS_EMMC_CTRL_R(addr) = (SNPS_EMMC_CTRL_R(addr) & 0xfffe) | CARD_IS_EMMC;
    vers = HOST_CNTRL_VERS_R(addr);

    switch(DAT_XFER_WIDTH)
    {
        case 1:
            data_width = 0<<1;
            break;
        case 4:
            data_width = 1<<1;
            break;
        case 8:
            data_width = 1<<5;
            break;
        default :
            data_width = 0<<1;
    }

    if(vers >=3)
    {
        SDIOD_LOGI("VERS>=3\r\n");
        CLK_CTRL_R(addr)= 0x00;//base clk
        HOST_CTRL2_R(addr)=0x1000;//HOST_CTRL2_R.HOST_VER4_ENABLE=1,HOST_CTRL2_R.ADDRESSING=0(32bit),HOST_CTRL2_R.ASYNC_INT_ENABLE=0;EXEC_TUNING=0
        HOST_CTRL1_R(addr)= data_width;

    }
    else
        CLK_CTRL_R(addr)=0x00;
}

void card_clk_supply(uintptr_t addr)
{
    CLK_CTRL_R(addr) =  CLK_CTRL_R(addr) | SD_CLK_EN;
}

void card_clk_stop(uintptr_t addr)
{
    CLK_CTRL_R(addr) =  CLK_CTRL_R(addr) & 0xfffffffb;
}

void sd_clk_change(uintptr_t addr,uint16 SD_FREQ_SEL)
{
    uint16 internal_clk_stable;
    card_clk_stop(addr);

    CLK_CTRL_R(addr)= CLK_CTRL_R(addr) & 0xfffffff7;//Set CLK_CTRL_R.PLL_ENABLE to 0
    CLK_CTRL_R(addr)= (CLK_CTRL_R(addr)& 0x003f) | ((SD_FREQ_SEL&0xff)<<8) | ((SD_FREQ_SEL & 0x0300)>>2);
    CLK_CTRL_R(addr) =  CLK_CTRL_R(addr) & 0xffffffdf;
    CLK_CTRL_R(addr) =  CLK_CTRL_R(addr) | INTERNAL_CLK_EN | PLL_ENABLE;//Set INTERNAL_CLK_EN

    internal_clk_stable = CLK_CTRL_R(addr) & 0x0002;
    while(internal_clk_stable ==0);//wait internal clk stable

    card_clk_supply(addr);
}

void sd_card_interface_set(uintptr_t addr,uint8 UHS_MODE_SEL)
{
    HOST_CTRL2_R(addr) = (HOST_CTRL2_R(addr) & 0xfff8) | UHS_MODE_SEL;
    PWR_CTRL_R(addr) = PWR_CTRL_R(addr) | SD_BUS_PWR_VDD1;
}

void emmc_card_interface_set(uintptr_t addr,uint8 UHS_MODE_SEL)
{
    HOST_CTRL2_R(addr) = (HOST_CTRL2_R(addr) & 0xfffc)| UHS_MODE_SEL;//HOST_CTRL2_R.UHS2_IF_ENABLE=0,HOST_CTRL2.R.UHS_MODE_SEL=0
    PWR_CTRL_R(addr) = PWR_CTRL_R(addr) | SD_BUS_PWR_VDD1;
}

int send_cmd(uintptr_t addr, uint8 CMD_INDEX, uint8 RESP_TYPE, uint32 ARGUMENT)
{
    uint32 pstate;
    uint32 resp01, resp23,resp45,resp67;

    pstate = PSTATE_REG_R(addr);
    while(pstate & 0x00000001)
    {
        pstate = PSTATE_REG_R(addr);
    }
    SDIOD_LOGI("send cmd[%d], pstate:0x%x\r\n", CMD_INDEX, pstate);

    NORMAL_INT_STAT_EN_R(addr)   = NORMAL_INT_STAT_EN_R(addr) | CMD_COMPLETE_STAT_EN;
    NORMAL_INT_SIGNAL_EN_R(addr) = NORMAL_INT_SIGNAL_EN_R(addr) | CMD_COMPLETE_SIGNAL_EN;
    ERROR_INT_STAT_EN_R(addr)    = ERROR_INT_STAT_EN_R(addr) | 0x80f;
    ERROR_INT_SIGNAL_EN_R(addr)  = ERROR_INT_SIGNAL_EN_R(addr) | 0x80f;

    ARGUMENT_R(addr) = ARGUMENT;
    CMD_R(addr) = (CMD_INDEX<<8) | RESP_TYPE;
    XFER_MODE_R(addr) = XFER_MODE_R(addr) & 0xfffffeff;

    while(CMD_COMPLETE_STATE == 0){
        ;
    }
    CMD_COMPLETE_STATE = 0;

    resp01 = RESP01_R(addr);
    resp23 = RESP23_R(addr);
    resp45 = RESP45_R(addr);
    resp67 = RESP67_R(addr);
    SDIOD_LOGI("resp01[0x%x]\r\n", resp01);

    (void)resp23;
    (void)resp45;
    (void)resp67;

    return resp01;
}

void sd_card_init(uintptr_t addr)
{
    uint32_t resp[4] = {0};

    send_cmd(addr, CMD0, 0, 0); //send CMD0
    send_cmd(addr, CMD8, 2, 0x1aa); //send CMD8

    send_cmd(addr, CMD55, 2, 0); //send ACMD41
    resp[0] = send_cmd(addr, CMD41, 2, (0xff8000|0x40000000)); //send_cmd(addr, CMD41, 2, (0xff80001|0x40000000)); //send CMD41
    while(!((0xFFFFFFFF != resp[0]) && ((resp[0] >> 31) & 0x01)))
    {
        spe_delay(40000);
        spe_delay(40000);
        spe_delay(40000);
        send_cmd(addr, CMD55, 2, 0); //send ACMD41
        resp[0] = send_cmd(addr, CMD41, 2, (0x200000|0x40000000)); //send_cmd(addr, CMD41, 2, (0xff80001|0x40000000)); //send CMD41
    }

    resp[0] = send_cmd(addr, CMD2, 1, 0); //send CMD2
    while((0xFFFFFFFF == resp[0]) || (1 == CMD_TOUT_ERR_STATE)){
        CMD_TOUT_ERR_STATE = 0;
        resp[0] = send_cmd(addr, CMD2, 1, 0);
    }
    spe_delay(40000);
    spe_delay(40000);
    resp[0] = send_cmd(addr, CMD3, 2, 0); //send CMD3
    while((0xFFFFFFFF == resp[0]) || (1 == CMD_TOUT_ERR_STATE)){
        CMD_TOUT_ERR_STATE = 0;
        resp[0] = send_cmd(addr, CMD3, 2, 0);
    }
    sdio_rca = resp[0] & 0xFFFF0000;
    spe_delay(40000);
    spe_delay(40000);

    resp[0] = send_cmd(addr, CMD9, 1, sdio_rca);    //send CMD9
    while((0xFFFFFFFF == resp[0]) || (1 == CMD_TOUT_ERR_STATE)){
        CMD_TOUT_ERR_STATE = 0;
        resp[0] = send_cmd(addr, CMD9, 1, sdio_rca);    //send CMD9
    }
    spe_delay(40000);
    spe_delay(40000);
    resp[0] = send_cmd(addr, CMD7, 3, sdio_rca);    //send CMD7
    while((0xFFFFFFFF == resp[0]) || (1 == CMD_TOUT_ERR_STATE)){
        CMD_TOUT_ERR_STATE = 0;
        resp[0] = send_cmd(addr, CMD7, 3, sdio_rca);
    }
}

void emmc_card_init(uintptr_t addr,uint8 ddr_mode)
{
    uint32_t resp[4] = {0};

    send_cmd(addr, CMD0, 0, 0); //send CMD0

    resp[0] = send_cmd(addr, CMD1, 2, 0xC0000080);  //send CMD1
    while(!((0xFFFFFFFF != resp[0]) && ((resp[0] >> 31) & 0x01)))
    {
        spe_delay(40000);

        resp[0] = send_cmd(addr, CMD1, 2, 0xC0000080);  //send CMD1
    }

    send_cmd(addr, CMD2, 1, 0); //send CMD2
    send_cmd(addr, CMD3, 2, 0x00010000);    //send CMD3
    send_cmd(addr, CMD9, 1, 0x00010000);    //send CMD9
    send_cmd(addr, CMD7, 2, 0x00010000);    //send CMD7
    send_cmd(addr, CMD8, 2, 0x00000000);    //send CMD8
}

bk_err_t mshc_host_init(uintptr_t addr,uint16 sysclk_div,uint16 sdclk_div,uint8 tmclk_div,uint8 cqetmclk_div,uint8 CARD_IS_EMMC,uint8 UHS_MODE_SEL,uint8 DAT_XFER_WIDTH)
{
    uint8 ddr_mode;
    sdio_reg2(addr)= 3<<0;
    sdio_reg4(addr)= (tmclk_div<<8)|(cqetmclk_div);
    if(addr == sdio_mshc_0_base)
    {
        sys_a35_ll_set_clk_ctrl0_ckdiv_sdio0(sysclk_div);
        sys_a35_ll_set_clk_en_sdio0_cken(1);
    }
    if(addr == sdio_mshc_1_base)
    {
        sys_a35_ll_set_clk_ctrl0_ckdiv_sdio1(sysclk_div);
        sys_a35_ll_set_clk_en_sdio1_cken(1);
    }

    host_ctrl_set(addr,SD_BUS_PWR_VDD1,0x0e,CARD_IS_EMMC,DAT_XFER_WIDTH);//addr,SD_BUS_VOL_VDD1,TOUT_CNT,DAT_XFER_WIDTH
    sd_clk_change(addr,sdclk_div);

    if(CARD_IS_EMMC == 1)  //EMMC CARD INIT
    {
        emmc_card_interface_set(addr,UHS_MODE_SEL);
        if(UHS_MODE_SEL == UHS_MODE_EMMC_HSDDR)
        {
            SDIOD_LOGI("EMMC Init DDR\r\n");
            ddr_mode =1;
            emmc_card_init(addr,ddr_mode);
        }
        else
        {
            SDIOD_LOGI("EMMC Init SDR\r\n");
            ddr_mode =0;
            emmc_card_init(addr,ddr_mode);
        }
    }
    else
    {
        sd_card_interface_set(addr,UHS_MODE_SEL);
        sd_card_init(addr);
    }

    return BK_OK;
}

bk_err_t send_single_data(uintptr_t addr,uint16 BLOCK_SIZE,uint16 CMD,uint32 ARGUMENT)
{
    uint32 resp01;
    uint32 num=0;

    NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | BUF_WR_READY_STAT_EN;
    ERROR_INT_STAT_EN_R(addr) = 0x870;
    NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | BUF_WR_READY_SIGNAL_EN;
    ERROR_INT_SIGNAL_EN_R(addr) = 0x870;

    BLOCKSIZE_R(addr) = BLOCK_SIZE;
    ARGUMENT_R(addr)  = ARGUMENT;
    XFER_MODE_R(addr) = 0x80;//single blocks;resp_err_check_enable
    CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | 0x2;//NO CHECK CMD INDEX ,NO CHECK CMD CRC;RESP TYPE: 0X2,

    while(CMD_COMPLETE_STATE==0);
    CMD_COMPLETE_STATE=0;

    resp01 = RESP01_R(addr);

    while (BUF_WR_READY_STATE == 0);
    BUF_WR_READY_STATE = 0;

    while(num<128)
    {
        BUF_DATA_R(addr) = num;
        num++;
    }

    while(XFER_COMPLETE_STATE==0);
    XFER_COMPLETE_STATE=0;

    ARGUMENT_R(addr) = 0x0;
    CMD_R(addr) = 12<<8;    //CMD12:Card stop transmission

    while(CMD_COMPLETE_STATE==0);
    CMD_COMPLETE_STATE=0;

    SDIOD_LOGI("**Singal Data Buf Write End**\r\n");
    (void)resp01;

    return BK_OK;
}

bk_err_t receive_single_data(uintptr_t addr,uint16 BLOCK_SIZE,uint16 CMD,uint32 ARGUMENT)
{
    uint32 resp01;
    uint32 read_data;
    uint32 num=0;
    uint32 err_num =0;

    NORMAL_INT_STAT_EN_R(addr) = CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | BUF_RD_READY_STAT_EN;
    ERROR_INT_STAT_EN_R(addr) = 0x870;
    NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | BUF_RD_READY_STAT_EN;
    ERROR_INT_SIGNAL_EN_R(addr) = 0x870;

    BLOCKSIZE_R(addr) = BLOCK_SIZE;
    ARGUMENT_R(addr)  = ARGUMENT;

    XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_DATA_READ | XFR_MODE_AUTOCMD12_EN;
    CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | 0x2;

    while(CMD_COMPLETE_STATE==0);
    CMD_COMPLETE_STATE=0;

    resp01 = RESP01_R(addr);

    while(BUF_RD_READY_STATE==0);
    BUF_RD_READY_STATE=0;

    while(num<128)
    {
        read_data=BUF_DATA_R(addr);
        if(read_data != num)
        {
            err_num = err_num + 1;
            SDIOD_LOGI("Receive DATA Error: BUF_DATA = %x, num =%x \r\n",read_data,num);
        }
        num++;
    }

    while(XFER_COMPLETE_STATE==0);
    XFER_COMPLETE_STATE=0;

    SDIOD_LOGI("**Singnal Data Buf Read End**\r\n");

    if(err_num !=0)
    {
        SDIOD_LOGI("err_num is : %x\r\n",err_num);
        SDIOD_LOGI("Sgl Data Trx Error\r\n");
    }
    else
    {
        SDIOD_LOGI("Sgl Data Trx Right\r\n");
    }
    (void)resp01;

    return err_num;
}

bk_err_t send_mult_data(uintptr_t addr,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
    uint32 resp01;
    uint32 num=0;
    uint16 block_num =0;

    NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | BUF_WR_READY_STAT_EN;
    ERROR_INT_STAT_EN_R(addr) = 0x870;
    NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | BUF_WR_READY_SIGNAL_EN;
    ERROR_INT_SIGNAL_EN_R(addr) = 0x870;

    BLOCKSIZE_R(addr) = BLOCK_SIZE;
    BLOCKCOUNT_R(addr)= BLOCK_CNT;
    ARGUMENT_R(addr)  = ARGUMENT;
    XFER_MODE_R(addr) = 0xa2;////RESP TYPE: 0X2,NO CHECK CMD INDEX ,NO CHECK CMD CRC;multi blocks;block counter enable;resp_err_check_enable
    CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | 0x2;

    while(CMD_COMPLETE_STATE==0);
    CMD_COMPLETE_STATE=0;

    resp01 = RESP01_R(addr);

    for (block_num = 0; block_num < BLOCK_CNT; block_num = block_num+1)
    {
        while (BUF_WR_READY_STATE == 0);
        BUF_WR_READY_STATE = 0;

        while(num<(128*(block_num+1)))
        {
            BUF_DATA_R(addr) = num;
            num++;
        }
    }

    while(XFER_COMPLETE_STATE==0);
    XFER_COMPLETE_STATE=0;

    ARGUMENT_R(addr) = 0x0;
    CMD_R(addr) = 12<<8;    //CMD12:Card stop transmission

    while(CMD_COMPLETE_STATE==0);
    CMD_COMPLETE_STATE=0;

    SDIOD_LOGI("**Mul Data Buf Write End**\r\n");
    (void)resp01;

    return BK_OK;
}

int receive_mult_data(uintptr_t addr,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
    uint32 resp01;
    uint32 read_data;
    uint32 num=0;
    uint16 block_num =0;
    uint32 err_num =0;

    NORMAL_INT_STAT_EN_R(addr) = CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | BUF_RD_READY_STAT_EN;
    ERROR_INT_STAT_EN_R(addr) = 0x870;
    NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | BUF_RD_READY_STAT_EN;
    ERROR_INT_SIGNAL_EN_R(addr) = 0x870;

    BLOCKSIZE_R(addr) = BLOCK_SIZE;
    BLOCKCOUNT_R(addr)= BLOCK_CNT;
    ARGUMENT_R(addr)  = ARGUMENT;

    XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL | XFR_MODE_DATA_READ | XFR_MODE_AUTOCMD12_EN | XFR_MODE_BLKCNT_EN;//0xb2;
    CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | 0x2;

    while(CMD_COMPLETE_STATE==0);
    CMD_COMPLETE_STATE=0;

    resp01 = RESP01_R(addr);

    for (block_num=0; block_num<BLOCK_CNT; block_num++)
    {
        while(BUF_RD_READY_STATE==0);
        BUF_RD_READY_STATE=0;

        SDIOD_LOGI("Receive Block %d:\r\n",block_num);

        while(num<128*(block_num+1))
        {
            read_data=BUF_DATA_R(addr);
            if(read_data != num)
            {
                err_num = err_num + 1;
                SDIOD_LOGI("Receive DATA Error: BUF_DATA = %x, num =%x \r\n",read_data,num);
            }
            num = num+1;
        }
    }

    while(XFER_COMPLETE_STATE==0);
    XFER_COMPLETE_STATE=0;

    SDIOD_LOGI("**Data Buf Mul Read End**\r\n");

    if(err_num !=0)
    {
        SDIOD_LOGI("err_num is : %x\r\n",err_num);
        SDIOD_LOGI("Mul Data Trx Error\r\n");
    }
    else
    {
        SDIOD_LOGI("Mul Data Trx Right\r\n");
    }
    (void)resp01;

    return err_num;
}

bk_err_t sdma_send_data(uintptr_t addr,uint8 HOST_VER4_EN,uint32 SYS_ADDR,uint16 SDMA_BUF_BDARY,uint16 SDMA_BUF_BDARY_NUM,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
    uint16 num;
    uint32 i;
    uint32 resp01;
    uint32 *mem_addr;

    mem_addr = (uint32 *)((uintptr_t)SYS_ADDR);//smem1
    for(i=0; i<1280; i++) //store 1280 words datas in smem5(5120 bytes:10 blocks)
    {
        *mem_addr = i;
        mem_addr = mem_addr+1;
    }

    HOST_CTRL1_R(addr)= (HOST_CTRL1_R(addr) & 0xe7) | DMASEL_SDMA;
    HOST_CTRL2_R(addr)= (HOST_CTRL2_R(addr) & 0xefff) | HOST_VER4_EN<<12;

    if(HOST_VER4_EN)
    {
        ADMA_SA_LOW_R(addr)= SYS_ADDR;
    }
    else
    {
        SDMASA_R(addr)= SYS_ADDR;
    }

    NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | DMA_INTERRUPT_STAT_EN;
    ERROR_INT_STAT_EN_R(addr)= 0xb7f;

    NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN;
    ERROR_INT_SIGNAL_EN_R(addr)= 0xb7f;

    BLOCKCOUNT_R(addr)= BLOCK_CNT;
    BLOCKSIZE_R(addr)=  (SDMA_BUF_BDARY << 12) | BLOCK_SIZE;

    ARGUMENT_R(addr) = ARGUMENT;

//**RESP TYPE: 0X2,NO CHECK CMD INDEX ,NO CHECK CMD CRC;multi blocks;resp_err_check_enable
//**DMA ENABLE; block counter enable;AUTO CMD12 DISABLE; transfer :wirte
    XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL | XFR_MODE_BLKCNT_EN | XFR_MODE_DMA_EN;
    CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | RESP_LEN_48;

    while(CMD_COMPLETE_STATE == 0);
    CMD_COMPLETE_STATE = 0;

    resp01 = RESP01_R(addr);

    num = 0;
    while(num < SDMA_BUF_BDARY_NUM)
    {
        while(DMA_INTERRUPT_STATE == 0);
        DMA_INTERRUPT_STATE = 0;
        num++;

        if(HOST_VER4_EN)
        {
            if(SDMA_BUF_BDARY== 0x0) //4k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(4096*num);
            }
            if(SDMA_BUF_BDARY== 0x1) //8k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(8192*num);
            }
            if(SDMA_BUF_BDARY== 0x2) //16k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(16384*num);
            }
            if(SDMA_BUF_BDARY== 0x3) //32k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(32768*num);
            }
            if(SDMA_BUF_BDARY== 0x4) //64k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(65536*num);
            }
            if(SDMA_BUF_BDARY== 0x5) //128k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(131072*num);
            }
            if(SDMA_BUF_BDARY== 0x6) //256k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(262144*num);
            }
            if(SDMA_BUF_BDARY== 0x7) //512k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(524288*num);
            }
        }
        else
        {
            if(SDMA_BUF_BDARY== 0x0) //4k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(4096*num);
            }
            if(SDMA_BUF_BDARY== 0x1) //8k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(8192*num);
            }
            if(SDMA_BUF_BDARY== 0x2) //16k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(16384*num);
            }
            if(SDMA_BUF_BDARY== 0x3) //32k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(32768*num);
            }
            if(SDMA_BUF_BDARY== 0x4) //64k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(65536*num);
            }
            if(SDMA_BUF_BDARY== 0x5) //128k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(131072*num);
            }
            if(SDMA_BUF_BDARY== 0x6) //256k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(262144*num);
            }
            if(SDMA_BUF_BDARY== 0x7) //512k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(524288*num);
            }
        }

    }

    while(XFER_COMPLETE_STATE==0);
    XFER_COMPLETE_STATE=0;

    ARGUMENT_R(addr) = 0x0;
    CMD_R(addr) = 12<<8;    //CMD12:Card stop transmission

    while(CMD_COMPLETE_STATE==0);
    CMD_COMPLETE_STATE=0;
    SDIOD_LOGI("*****Data(SDMA) Write End*****\r\n");
    (void)resp01;

    return BK_OK;
}

bk_err_t sdma_receive_data (uintptr_t addr,uint8 HOST_VER4_EN,uint32 SYS_ADDR,uint16 SDMA_BUF_BDARY,uint16 SDMA_BUF_BDARY_NUM,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
    uint16 num;
    uint32 i;
    uint32 resp01;
    uint32 err_num;
    uint32 data;
    uint32 *mem_addr;

    mem_addr = (uint32 *)((uintptr_t)SYS_ADDR);//smem1

    HOST_CTRL1_R(addr)= (HOST_CTRL1_R(addr) & 0xe7) | DMASEL_SDMA;
    HOST_CTRL2_R(addr)= (HOST_CTRL2_R(addr) & 0xefff) | HOST_VER4_EN<<12;

    if(HOST_VER4_EN)
    {
        ADMA_SA_LOW_R(addr)= SYS_ADDR;
    }
    else
    {
        SDMASA_R(addr)= SYS_ADDR;
    }

    NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | DMA_INTERRUPT_STAT_EN;
    ERROR_INT_STAT_EN_R(addr)= 0xb7f;

    NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN;
    ERROR_INT_SIGNAL_EN_R(addr)= 0xb7f;

    BLOCKCOUNT_R(addr)= BLOCK_CNT;
    BLOCKSIZE_R(addr)=  (SDMA_BUF_BDARY << 12) | BLOCK_SIZE;

    ARGUMENT_R(addr) = ARGUMENT;

//**RESP TYPE: 0X2,NO CHECK CMD INDEX ,NO CHECK CMD CRC;multi blocks;resp_err_check_enable
//**DMA ENABLE; block counter enable;AUTO CMD12 ENABLE; transfer :wirte
    XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL | XFR_MODE_DATA_READ | XFR_MODE_AUTOCMD12_EN | XFR_MODE_BLKCNT_EN | XFR_MODE_DMA_EN;
    CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | RESP_LEN_48;

    while(CMD_COMPLETE_STATE == 0);
    CMD_COMPLETE_STATE = 0;

    resp01 = RESP01_R(addr);

    num = 0;
    while(num < SDMA_BUF_BDARY_NUM)
    {
        while(DMA_INTERRUPT_STATE == 0);
        DMA_INTERRUPT_STATE = 0;
        num++;

        if(HOST_VER4_EN)
        {
            if(SDMA_BUF_BDARY== 0x0) //4k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(4096*num);
            }
            if(SDMA_BUF_BDARY== 0x1) //8k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(8192*num);
            }
            if(SDMA_BUF_BDARY== 0x2) //16k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(16384*num);
            }
            if(SDMA_BUF_BDARY== 0x3) //32k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(32768*num);
            }
            if(SDMA_BUF_BDARY== 0x4) //64k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(65536*num);
            }
            if(SDMA_BUF_BDARY== 0x5) //128k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(131072*num);
            }
            if(SDMA_BUF_BDARY== 0x6) //256k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(262144*num);
            }
            if(SDMA_BUF_BDARY== 0x7) //512k boundary
            {
                ADMA_SA_LOW_R(addr) = SYS_ADDR+(524288*num);
            }
        }
        else
        {
            if(SDMA_BUF_BDARY== 0x0) //4k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(4096*num);
            }
            if(SDMA_BUF_BDARY== 0x1) //8k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(8192*num);
            }
            if(SDMA_BUF_BDARY== 0x2) //16k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(16384*num);
            }
            if(SDMA_BUF_BDARY== 0x3) //32k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(32768*num);
            }
            if(SDMA_BUF_BDARY== 0x4) //64k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(65536*num);
            }
            if(SDMA_BUF_BDARY== 0x5) //128k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(131072*num);
            }
            if(SDMA_BUF_BDARY== 0x6) //256k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(262144*num);
            }
            if(SDMA_BUF_BDARY== 0x7) //512k boundary
            {
                SDMASA_R(addr) = SYS_ADDR+(524288*num);
            }
        }
    }

    while(XFER_COMPLETE_STATE==0);
    XFER_COMPLETE_STATE=0;

    SDIOD_LOGI("*****Data(SDMA) Read End*****\r\n");
    SDIOD_LOGI("***** Check SDMA Data: ******\r\n");
    err_num = 0;
    for(i=0; i<1280; i++) //check 1280 words datas(5120 bytes:10 blocks)
    {
        data=*mem_addr;
        if(data != i)
        {
            err_num = err_num+1;
            SDIOD_LOGI("Data Error: Mem_data = %d, i= %d\r\n",data,i);
        }
        else
        {
        }
        mem_addr = mem_addr+1;
    }
    if(err_num != 0)
    {
        SDIOD_LOGI("err_num = %d\r\n",err_num);
        SDIOD_LOGI("SDMA Test FAILfail\r\n");
    }
    else
    {
        SDIOD_LOGI("SDMA Test PASSpass\r\n");
    }
    (void)resp01;

    return BK_OK;
}

bk_err_t adma2_send_data(uintptr_t addr,uint32 SYS_ADDR_L,uint32 SYS_ADDR_H,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
    uint32 i;
    uint32 resp01;
    uint32 *mem_addr;
    uint32 *descriptor_addr0;
    uint32 *descriptor_addr1;
    uint32 *descriptor_addr2;
    uint32 *descriptor_addr3;

    mem_addr = (uint32 *)((uintptr_t)0x28040000);

    descriptor_addr0 = (uint32 *)((uintptr_t)0x28020000);
    descriptor_addr1 = (uint32 *)((uintptr_t)0x28020004);
    descriptor_addr2 = (uint32 *)((uintptr_t)0x28020008);
    descriptor_addr3 = (uint32 *)((uintptr_t)0x2802000c);

    *descriptor_addr0 = 0x8000021;
    *descriptor_addr1 = 0x28040000;
    *descriptor_addr2 = 0x8000027;
    *descriptor_addr3 = 0x28040800;

    for(i=0; i<1024; i++) //store 1024 words datas in smem4(4096 bytes:8 blocks)
    {
        *mem_addr = i;
        mem_addr = mem_addr+1;
    }

    HOST_CTRL1_R(addr)= (HOST_CTRL1_R(addr) & 0xe7) | DMASEL_ADMA2;
    HOST_CTRL2_R(addr)= (HOST_CTRL2_R(addr) & 0xefff) | 1<<12;    //HOST_VER4_ENABLE

    ADMA_SA_LOW_R(addr)= SYS_ADDR_L;   //Set ADMA System AddressRegister (ADMA_SA_LOW_R)
    ADMA_SA_HIGH_R(addr)= SYS_ADDR_H;    //Set ADMA System AddressRegister (ADMA_SA_HIGH_R)

    NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | DMA_INTERRUPT_STAT_EN;
    ERROR_INT_STAT_EN_R(addr)= 0xb7f;

    NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN;
    ERROR_INT_SIGNAL_EN_R(addr)= 0xb7f;

    BLOCKCOUNT_R(addr)= BLOCK_CNT;
    BLOCKSIZE_R(addr)= BLOCK_SIZE;

    ARGUMENT_R(addr) = ARGUMENT;

//**RESP TYPE: 0X2,NO CHECK CMD INDEX ,NO CHECK CMD CRC;multi blocks;resp_err_check_enable
//**DMA ENABLE; block counter enable;AUTO CMD12 DISABLE; transfer :wirte
    XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL | XFR_MODE_BLKCNT_EN | XFR_MODE_DMA_EN;
    CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | RESP_LEN_48;

    while(CMD_COMPLETE_STATE == 0);
    CMD_COMPLETE_STATE = 0;

    resp01 = RESP01_R(addr);

    while(XFER_COMPLETE_STATE == 0)
    {
        if(DMA_INTERRUPT_STATE)
        {
            DMA_INTERRUPT_STATE=0;
        }
    }
    XFER_COMPLETE_STATE = 0;

    ARGUMENT_R(addr) = 0x0;
    CMD_R(addr) = 12<<8;    //CMD12:Card stop transmission

    while(CMD_COMPLETE_STATE==0);
    CMD_COMPLETE_STATE=0;
    SDIOD_LOGI("*****Data(ADMA2) Write End*****\r\n");
    (void)resp01;

    return BK_OK;
}

bk_err_t adma2_receive_data (uintptr_t addr,uint32 SYS_ADDR_L,uint32 SYS_ADDR_H,uint16 BLOCK_SIZE,uint16 BLOCK_CNT,uint16 CMD,uint32 ARGUMENT)
{
    uint32 i;
    uint32 resp01;
    uint32 err_num;
    uint32 data;
    uint32 *mem_addr;
    uint32 *descriptor_addr0;
    uint32 *descriptor_addr1;
    uint32 *descriptor_addr2;
    uint32 *descriptor_addr3;
    mem_addr = (uint32 *)((uintptr_t)0x28050000);

    descriptor_addr0 = (uint32 *)((uintptr_t)0x28020010);
    descriptor_addr1 = (uint32 *)((uintptr_t)0x28020014);
    descriptor_addr2 = (uint32 *)((uintptr_t)0x28020018);
    descriptor_addr3 = (uint32 *)((uintptr_t)0x2802001c);

    *descriptor_addr0 = 0x8000021;
    *descriptor_addr1 = 0x28050000;
    *descriptor_addr2 = 0x8000027;
    *descriptor_addr3 = 0x28050800;

    asm("dsb sy");

    HOST_CTRL1_R(addr)= (HOST_CTRL1_R(addr) & 0xe7) | DMASEL_ADMA2;
    HOST_CTRL2_R(addr)= (HOST_CTRL2_R(addr) & 0xefff) | 1<<12;    //HOST_VER4_ENABLE

    ADMA_SA_LOW_R(addr)= SYS_ADDR_L;   //Set ADMA System AddressRegister (ADMA_SA_LOW_R)
    ADMA_SA_HIGH_R(addr)= SYS_ADDR_H;    //Set ADMA System AddressRegister (ADMA_SA_HIGH_R)

    NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | DMA_INTERRUPT_STAT_EN;
    ERROR_INT_STAT_EN_R(addr)= 0xb7f;

    NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN;
    ERROR_INT_SIGNAL_EN_R(addr)= 0xb7f;

    BLOCKCOUNT_R(addr)= BLOCK_CNT;
    BLOCKSIZE_R(addr)= BLOCK_SIZE;

    ARGUMENT_R(addr) = ARGUMENT;

//**RESP TYPE: 0X2,NO CHECK CMD INDEX ,NO CHECK CMD CRC;multi blocks;resp_err_check_enable
//**DMA ENABLE; block counter enable;AUTO CMD12 ENABLE; transfer :wirte
    XFER_MODE_R(addr) = XFR_MODE_RESP_ERRCHK_EN | XFR_MODE_MULTBLK_SEL | XFR_MODE_DATA_READ | XFR_MODE_AUTOCMD12_EN | XFR_MODE_BLKCNT_EN | XFR_MODE_DMA_EN;
    CMD_R(addr) = (CMD<<8) | DATA_PRESENT_SEL | RESP_LEN_48;

    while(CMD_COMPLETE_STATE == 0);
    CMD_COMPLETE_STATE = 0;

    resp01 = RESP01_R(addr);
    while(XFER_COMPLETE_STATE == 0)
    {
        if(DMA_INTERRUPT_STATE)
        {
            DMA_INTERRUPT_STATE=0;
        }
    }
    XFER_COMPLETE_STATE = 0;

    SDIOD_LOGI("*****Data(ADMA2) Read End*****\r\n");
    SDIOD_LOGI("*****Check ADMA2 Data: *****\r\n");
    err_num = 0;
    for(i=0; i<1024; i++)
    {
        data=*mem_addr;
        if(data != i)
        {
            err_num = err_num+1;
            SDIOD_LOGI("Data Error: Mem_data = %d, i= %d\r\n",data,i);
        }
        else
        {
        }
        mem_addr = mem_addr+1;
    }
    if(err_num != 0)
    {
        SDIOD_LOGI("err_num = %d\r\n",err_num);
        SDIOD_LOGI("ADMA2 Test FAILfail\r\n");
    }
    else
    {
        SDIOD_LOGI("ADMA2 Test PASSpass\r\n");
    }
    (void)resp01;

    return BK_OK;
}

bk_err_t adma3_send_data(uintptr_t addr,uint8 HOST_VER4_EN,uint32 ADMA_ID_L,uint32 ADMA_ID_H)
{
        uint32  i;
        uint32  *mem_addr;

        mem_addr = (uint32 *)((uintptr_t)0x28020000);

        for(i=0; i<1024; i++)
        {
                *mem_addr = i;
                mem_addr = mem_addr + 1;
        }

        //integrated descriptor pointer1
        adma3_wr_descriptor_addr[0]= 0x00000039;
        adma3_wr_descriptor_addr[1]= (uint32_t)((uintptr_t)&adma3_wr_descriptor_addr[4]);
        //integrated descriptor pointer2
        adma3_wr_descriptor_addr[2]= 0x0000003b;
        adma3_wr_descriptor_addr[3]= (uint32_t)((uintptr_t)&adma3_wr_descriptor_addr[32]);

        //descriptor pairs1 :command
        adma3_wr_descriptor_addr[4]= 0x00000009;
        adma3_wr_descriptor_addr[5]= 0x4;   //32bit block count
        adma3_wr_descriptor_addr[6]= 0x00000009;
        adma3_wr_descriptor_addr[7]= 0x200; //32bit blocksize
        adma3_wr_descriptor_addr[8]= 0x00000009;
        adma3_wr_descriptor_addr[9]= 0x0;   //32bit argument
        adma3_wr_descriptor_addr[10]= 0x00000009;
        adma3_wr_descriptor_addr[11]= CMD25<<24 | 0x22<<16 | 0x00a3;    //32bit cmd + transfer mode
        //descriptor pairs1:ADMA2
        adma3_wr_descriptor_addr[12]= 0x08000021;
        adma3_wr_descriptor_addr[13]= 0x28020000;

        //descriptor pairs2 :command
        adma3_wr_descriptor_addr[32]= 0x00000009;
        adma3_wr_descriptor_addr[33]= 0x4;  //32bit block count
        adma3_wr_descriptor_addr[34]= 0x00000009;
        adma3_wr_descriptor_addr[35]= 0x200;    //32bit blocksize
        adma3_wr_descriptor_addr[36]= 0x00000009;
        adma3_wr_descriptor_addr[37]= 0x00001000;   //32bit argument
        adma3_wr_descriptor_addr[38]= 0x00000009;
        adma3_wr_descriptor_addr[39]= CMD25<<24 | 0x22<<16 | 0x00a3;    //32bit cmd + transfer mode
        //descriptor pairs2:ADMA2
        adma3_wr_descriptor_addr[40]= 0x08000023;
        adma3_wr_descriptor_addr[41]= 0x28020800;

        NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | DMA_INTERRUPT_STAT_EN;
        ERROR_INT_STAT_EN_R(addr)= 0xb7f;

        NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN;
        ERROR_INT_SIGNAL_EN_R(addr)= 0xb7f;

        ARGUMENT_R(addr) = 0x4;//4block
        CMD_R(addr) = CMD23 <<8 | 0x2; //CMD23

        while(CMD_COMPLETE_STATE == 0);
        CMD_COMPLETE_STATE = 0;

        HOST_CTRL1_R(addr)= (HOST_CTRL1_R(addr) & 0xe7) | DMASEL_ADMA3;
        HOST_CTRL2_R(addr)= (HOST_CTRL2_R(addr) & 0xefff) | HOST_VER4_EN<<12;    //HOST_VER4_ENABLE

        ADMA_ID_LOW_R(addr)= ADMA_ID_L;
        ADMA_ID_HIGH_R(addr)= ADMA_ID_H;

        while(XFER_COMPLETE_STATE == 0)
        {
                if(DMA_INTERRUPT_STATE)
                {
                        DMA_INTERRUPT_STATE=0;
                }
        }
        XFER_COMPLETE_STATE = 0;

        ARGUMENT_R(addr) = 0x0;
        CMD_R(addr) = 12<<8;    //CMD12:Card stop transmission
        while(CMD_COMPLETE_STATE==0);
        CMD_COMPLETE_STATE=0;

        SDIOD_LOGI("*****Data(ADMA3) Write End*****\r\n");

        return BK_OK;
}

bk_err_t adma3_receive_data(uintptr_t addr,uint8 HOST_VER4_EN,uint32 ADMA_ID_L,uint32 ADMA_ID_H)
{
    uint32  i, data, err_num;
    uint32  *mem_addr;

//integrated descriptor pointer1
    adma3_rd_descriptor_addr[0]= 0x00000039;
    adma3_rd_descriptor_addr[1]= (uint32_t)((uintptr_t)&adma3_rd_descriptor_addr[4]);
//integrated descriptor pointer2
    adma3_rd_descriptor_addr[2]= 0x0000003b;
    adma3_rd_descriptor_addr[3]= (uint32_t)((uintptr_t)&adma3_rd_descriptor_addr[32]);

//descriptor pairs1 :command
    adma3_rd_descriptor_addr[4]= 0x00000009;
    adma3_rd_descriptor_addr[5]= 0x4;   //32bit block count
    adma3_rd_descriptor_addr[6]= 0x00000009;
    adma3_rd_descriptor_addr[7]= 0x200; //32bit blocksize
    adma3_rd_descriptor_addr[8]= 0x00000009;
    adma3_rd_descriptor_addr[9]= 0x0;   //32bit argument
    adma3_rd_descriptor_addr[10]= 0x00000009;
    adma3_rd_descriptor_addr[11]= CMD18<<24 | 0x22<<16 | 0x00b3;    //32bit cmd + transfer mode
//descriptor pairs1:ADMA2
    adma3_rd_descriptor_addr[12]= 0x08000021;
    adma3_rd_descriptor_addr[13]= 0x28040000;

//descriptor pairs2 :command
    adma3_rd_descriptor_addr[32]= 0x00000009;
    adma3_rd_descriptor_addr[33]= 0x4;  //32bit block count
    adma3_rd_descriptor_addr[34]= 0x00000009;
    adma3_rd_descriptor_addr[35]= 0x200;    //32bit blocksize
    adma3_rd_descriptor_addr[36]= 0x00000009;
    adma3_rd_descriptor_addr[37]= 0x00001000;   //32bit argument
    adma3_rd_descriptor_addr[38]= 0x00000009;
    adma3_rd_descriptor_addr[39]= CMD18<<24 | 0x22<<16 | 0x00b7;    //32bit cmd + transfer mode
//descriptor pairs2:ADMA2
    adma3_rd_descriptor_addr[40]= 0x08000023;
    adma3_rd_descriptor_addr[41]= 0x28040800;

    asm("dsb sy");

    NORMAL_INT_STAT_EN_R(addr)= CMD_COMPLETE_STAT_EN | XFER_COMPLETE_STAT_EN | DMA_INTERRUPT_STAT_EN;
    ERROR_INT_STAT_EN_R(addr)= 0xb7f;

    NORMAL_INT_SIGNAL_EN_R(addr)= CMD_COMPLETE_SIGNAL_EN | XFER_COMPLETE_SIGNAL_EN | DMA_INTERRUPT_SIGNAL_EN;
    ERROR_INT_SIGNAL_EN_R(addr)= 0xb7f;

    ARGUMENT_R(addr) = 0x4;//4block
    CMD_R(addr) = CMD23 << 8 | 0x2; //CMD23

    while(CMD_COMPLETE_STATE == 0);
    CMD_COMPLETE_STATE = 0;

    HOST_CTRL1_R(addr)= (HOST_CTRL1_R(addr) & 0xe7) | DMASEL_ADMA3;
    HOST_CTRL2_R(addr)= (HOST_CTRL2_R(addr) & 0xefff) | HOST_VER4_EN<<12;    //HOST_VER4_ENABLE

    ADMA_ID_LOW_R(addr)= ADMA_ID_L;
    ADMA_ID_HIGH_R(addr)= ADMA_ID_H;

    while(XFER_COMPLETE_STATE == 0)
    {
        if(DMA_INTERRUPT_STATE)
        {
            DMA_INTERRUPT_STATE=0;
        }
    }
    XFER_COMPLETE_STATE = 0;

    SDIOD_LOGI("*****Data(ADMA3) Read End*****\r\n");
    SDIOD_LOGI("*****Check ADMA3 Data: *****\r\n");
    err_num = 0;

    mem_addr = (uint32 *)((uintptr_t)0x28040000);

    for(i=0; i<1024; i++)
    {
        data = *mem_addr;
        if(data != i)
        {
            err_num = err_num+1;
            SDIOD_LOGI("Data Error: Mem_data = %d, i= %d\r\n",data,i);
        }
        else
        {
        }
        mem_addr = mem_addr+1;
    }
    if(err_num != 0)
    {
        SDIOD_LOGI("err_num = %d\r\n",err_num);
        SDIOD_LOGI("ADMA3 Test FAILfail\r\n");
    }
    else
    {
        SDIOD_LOGI("ADMA3 Test PASSpass\r\n");
    }

    return BK_OK;
}

bk_err_t  sdio_reg_w_r(uintptr_t addr)
{
    int32_t   RData0,RData1,RData2,RData3,RData4,RData5;
    int32_t   WData;
    int32_t   error_num = 0;

    SDIOD_LOGI("reg w&r test start \r\n");

    RData0 = sdio_reg0(addr);
    if(RData0 == 0x5344494F)
    {
        error_num =error_num+0;
    }
    else
    {
        error_num = error_num+1;
    }

    RData1 = sdio_reg1(addr);
    if(RData1 == 0x00040000)
    {
        error_num =error_num+0;
    }
    else
    {
        error_num =error_num+1;
    }

//sdio_0 reg2
    RData2 = sdio_reg2(addr);
    if(RData2 == 0x0)
    {
        error_num =error_num+0;
        WData = 0xaaaaaaaa;
        sdio_reg2(addr)= WData;
        RData2 = sdio_reg2(addr);
        if(RData2 == 0x2)
        {
            error_num =error_num+0;
            WData = 0x55555555;
            sdio_reg2(addr)= WData;
            RData2 = sdio_reg2(addr);
            if(RData2 == 0x1)
            {
                error_num =error_num+0;
            }
            else
            {
                error_num =error_num+1;
            }
        }
        else
        {
            error_num =error_num+1;
        }
    }
    else
    {
        error_num =error_num+1;
    }

//sdio_0 reg3
    RData3 = sdio_reg3(addr);
    if(RData3 == 0x0)
    {
        error_num =error_num+0;
    }
    else
    {
        error_num =error_num+1;
    }

//sdio_0 reg4
    RData4 = sdio_reg4(addr);
    if(RData4 == 0x0)
    {
        error_num =error_num+0;
        WData = 0xaaaaaaaa;
        sdio_reg4(addr)= WData;
        RData4 = sdio_reg4(addr);
        if(RData4 == 0xaaaaaaaa)
        {
            error_num =error_num+0;
            WData = 0x55555555;
            sdio_reg4(addr)= WData;
            RData4 = sdio_reg4(addr);
            if(RData4 == 0x55555555)
            {
                error_num =error_num+0;
            }
            else
            {
                error_num =error_num+1;
            }
        }
        else
        {
            error_num =error_num+1;
        }
    }
    else
    {
        error_num =error_num+1;
    }

//sdio_0 reg5
    RData5 = sdio_reg5(addr);
    if(RData5 == 0x0)
    {
        error_num =error_num+0;
        WData = 0xaaaaaaaa;
        sdio_reg5(addr)= WData;
        RData5 = sdio_reg5(addr);
        if(RData5 == 0x2)
        {
            error_num =error_num+0;
            WData = 0x55555555;
            sdio_reg5(addr)= WData;
            RData5 = sdio_reg5(addr);
            if(RData5 == 0x1)
            {
                error_num =error_num+0;
            }
            else
            {
                error_num =error_num+1;
            }
        }
        else
        {
            error_num =error_num+1;
        }
    }
    else
    {
        error_num =error_num+1;
    }
    if(error_num !=0)
    {
        SDIOD_LOGI("Error Num: \r\n",error_num);
        SDIOD_LOGI("SDIO Reg W&R Test FAILfail\r\n");
    }
    else
    {
        SDIOD_LOGI("SDIO Reg W&R Test PASSpass\r\n");
    }

    return BK_OK;
}

void tuning_cfg(uintptr_t addr,uint8 tuning_rx_sel0,uint8 tuning_rx_sel1,uint8 sample_rx_sel0,uint8 sample_rx_sel1,uint8 tuning_tx_sel0,uint8 tuning_tx_sel1,uint8 sample_tx_sel0,uint8 sample_tx_sel1,uint8 clk_drv_inv_sel)
{
    sdio_reg5(addr) |= tuning_rx_sel0 <<5 | tuning_rx_sel1<<8 | sample_rx_sel0 <<14 | sample_rx_sel1 <<15;
    sdio_reg5(addr) |= tuning_tx_sel0 <<17 | tuning_tx_sel1<<20 | sample_tx_sel0 <<26 | sample_tx_sel1 <<27;
    sdio_reg5(addr) |= clk_drv_inv_sel << 29;
}

void test_sdio(uint8_t *pPara)
{
        uint32          s_err_cnt = 0;
        uint32          m_err_cnt = 0;
        uint32          err_cnt = 0;
        uint8_t         host_id = pPara[0];
        uint8_t         func_id = pPara[1];

        SDIOD_LOGI("SDIO Test Start...\r\n");
        sys_a35_ll_set_int_en0_inten_fora35_sdio0(1);
        sys_a35_ll_set_int_en0_inten_fora35_sdio1(1);
        sys_a35_ll_set_clk_ctrl0_ckdiv_cpu(1);

        sys_ana_ll_set_reg0_spitrig(1);
        sys_ana_ll_set_reg0_spitrig(0);
        sys_ana_ll_set_reg5_en_vout(1);
        spe_delay(40000);

    if(host_id==0x01)//sdio0 test
    {
        sdio_gpio_init(0);
        PWR_CTRL_R(sdio_mshc_0_base) = 0x01;
        switch(func_id)
        {
            case 0 : //SDIO0 BUFFER SD CARD SDR TEST
                // tuning_cfg(sdio_mshc_0_base,0x0,0x0,0,0,0x0,0x0,0,0,1);
                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_0_base,0x4,300,0xff,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH4);     //400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_0_base,0x3,0x1,0xa,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH4);     //no 400k//addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                sd_card_interface_set(sdio_mshc_0_base,UHS_MODE_SDR50);
                sd_clk_change(sdio_mshc_0_base,0);

                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                send_cmd(sdio_mshc_0_base, CMD13, 2, 0XAAAA0000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                send_cmd(sdio_mshc_0_base, CMD55, 2, 0xAAAA0000);
                send_cmd(sdio_mshc_0_base, CMD51, 2, 0X00000000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                send_cmd(sdio_mshc_0_base, CMD55, 2, 0xaaaa0000);   //send ACMD6
                send_cmd(sdio_mshc_0_base, CMD6, 2, 2); //send CMD6 single line
                spe_delay(40000);
                send_cmd(sdio_mshc_0_base, CMD6, 2, 0x00fffff1);    //send CMD6 single line
                spe_delay(40000);

                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                send_cmd(sdio_mshc_0_base, CMD6, 2, 0x80fffff1);    //send CMD6 single line
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                s_err_cnt = 0;
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                send_mult_data(sdio_mshc_0_base,0x200,4,25,0);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);

                err_cnt = s_err_cnt | m_err_cnt;
                if(err_cnt !=0)
                {
                    SDIOD_LOGI("SD0 SDR BUF Test FAILfail\r\n");
                }
                else
                {
                    SDIOD_LOGI("SD0 SDR BUF Test PASSpass\r\n");
                }

                break;
            case 1 : //SDIO0 BUFFER EMMC CARD SDR TEST
                SNPS_EMMC_CTRL_R(sdio_mshc_1_base) |= 0x01;
                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_0_base,0x3,0xff,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH4);     //400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_0_base,0x3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //no 400k//addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);

                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                spe_delay(40000);
                send_cmd(sdio_mshc_1_base,CMD6,2,0x03b70100);//4bit bus width

                send_mult_data(sdio_mshc_0_base,0x200,4,25,0);
                m_err_cnt=receive_mult_data(sdio_mshc_0_base,0x200,4,18,0);

                err_cnt = s_err_cnt | m_err_cnt;
                if(err_cnt !=0)
                {
                    SDIOD_LOGI("EMMC0 SDR BUF Test FAILfail\r\n");
                }
                else
                {
                    SDIOD_LOGI("EMMC0 SDR BUF Test PASSpass\r\n");
                }
                break;
            case 2 : //SDIO0 BUFFER EMMC CARD DDR TEST
                tuning_cfg(sdio_mshc_0_base,0x2,0x2,1,1,0x1,0x2,1,1,0);//tuning tx and rx clk

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_0_base,0x3,0xff,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_0_base,0x3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //no 400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_0_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_0_base,CMD6,2,CMD6_DDR_8BIT);
                sd_card_interface_set(sdio_mshc_0_base,UHS_MODE_EMMC_HSDDR);
                sd_clk_change(sdio_mshc_0_base,1);

                send_mult_data(sdio_mshc_0_base,0x200,4,25,0);
                m_err_cnt=receive_mult_data(sdio_mshc_0_base,0x200,4,18,0);

                err_cnt = s_err_cnt | m_err_cnt;
                if(err_cnt !=0)
                {
                    SDIOD_LOGI("EMMC0 DDR BUF Test FAILfail\r\n");
                }
                else
                {
                    SDIOD_LOGI("EMMC0 DDR BUF Test PASSpass\r\n");
                }
                break;
            case 3 : //SDIO0 SDMA EMMC CARD DDR TEST
                tuning_cfg(sdio_mshc_0_base,0x2,0x2,1,1,0x1,0x2,1,1,0);//tuning tx and rx clk

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_0_base,0x3,0xff,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_0_base,0x3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //no 400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_0_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_0_base,CMD6,2,CMD6_DDR_8BIT);
                sd_card_interface_set(sdio_mshc_0_base,UHS_MODE_EMMC_HSDDR);
                sd_clk_change(sdio_mshc_0_base,1);

                sdma_send_data(sdio_mshc_0_base,1,0x28180000,SDMA_BUF_BDARY_4K,1,0x200,10,25,0);         //addr,HOST_VER4_EN,SYS_ADDR,SDMA_BUF_BDARY,SDMA_BUF_BDARY_NUM,BLOCK_SIZE,BLOCK_CNT,CMD,ARGUMENT
                sdma_receive_data (sdio_mshc_0_base,1,0x28190000,SDMA_BUF_BDARY_4K,1,0x200,10,18,0);      //addr,HOST_VER4_EN,SYS_ADDR,SDMA_BUF_BDARY,SDMA_BUF_BDARY_NUM,BLOCK_SIZE,BLOCK_CNT,CMD,ARGUMENT
                break;
            case 4 : //SDIO0 ADMA2 EMMC CARD DDR TEST
                tuning_cfg(sdio_mshc_0_base,0x2,0x2,1,1,0x1,0x2,1,1,0);//tuning tx and rx clk

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_0_base,0x3,0xff,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //400k//addr,sysclk_select,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_0_base,0x3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //no 400k //addr,sysclk_select,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_0_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_0_base,CMD6,2,CMD6_DDR_8BIT);
                sd_card_interface_set(sdio_mshc_0_base,UHS_MODE_EMMC_HSDDR);
                sd_clk_change(sdio_mshc_0_base,1);

                adma2_send_data(sdio_mshc_0_base,0x28020000,0x0,0x200,8,25,0);
                adma2_receive_data (sdio_mshc_0_base,0x28020010,0x0,0x200,8,18,0);
                break;
            case 5: //SDIO0 ADMA3 EMMC CARD DDR TEST
                tuning_cfg(sdio_mshc_0_base,0x2,0x2,1,1,0x1,0x2,1,1,0);//tuning tx and rx clk

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_0_base,0x3,0xff,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //400k //addr,sysclk_select,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_0_base,0x3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //no 400k //addr,sysclk_select,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_0_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_0_base,CMD6,2,CMD6_DDR_8BIT);
                sd_card_interface_set(sdio_mshc_0_base,UHS_MODE_EMMC_HSDDR);
                sd_clk_change(sdio_mshc_0_base,1);

                adma3_send_data(sdio_mshc_0_base,VER4_EN, (uint32_t)((uintptr_t)&adma3_wr_descriptor_addr[0]), 0x00000000);
                adma3_receive_data(sdio_mshc_0_base,VER4_EN, (uint32_t)((uintptr_t)&adma3_rd_descriptor_addr[0]), 0x00000000);
                break;
            case 6:
                SDIOD_LOGI("**SD0 negedge clk test start**\r\n");
                tuning_cfg(sdio_mshc_0_base,0x0,0x0,0,0,0x0,0x0,0,0,1);//negedge clk send data and cmd

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_0_base,0x3,0xff,0xa,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH4);     //400k//addr,sysclk_select,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_0_base,0x3,0x1,0xa,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH4);     //no 400k//addr,sysclk_select,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_0_base, CMD55, 2, sdio_rca);   //0x12340000 send ACMD6
                send_cmd(sdio_mshc_0_base,CMD6,2,CMD6_SDR_4BIT);
                sd_card_interface_set(sdio_mshc_0_base,UHS_MODE_SDR25);
                sd_clk_change(sdio_mshc_0_base,1);

                send_mult_data(sdio_mshc_0_base,0x200,4,25,0);
                err_cnt=receive_mult_data(sdio_mshc_0_base,0x200,4,18,0);
                if(err_cnt !=0)
                {
                    SDIOD_LOGI("SD0 SDR BUF Test FAILfail\r\n");
                }
                else
                {
                    SDIOD_LOGI("SD0 SDR BUF Test PASSpass\r\n");
                }
                SDIOD_LOGI("SD0 negedge clk test end\r\n");

                break;
            default : //REG READ AND WRITE
                sdio_reg_w_r(sdio_mshc_0_base);
                break;
        }

    }
    else if(host_id==0x02)  //sdio1 test
    {
        sdio_gpio_init(1);

        switch(func_id)
        {
            case 0 : //SDIO1 BUFFER SD CARD SDR TEST

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_1_base,3,FREQSEL_CLK_400KHZ,0xa,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH4);     //400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_1_base,3,0x1,0xa,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH4);                    //no 400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_1_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_1_base,CMD6,2,CMD6_SDR_4BIT);
                sd_card_interface_set(sdio_mshc_1_base,UHS_MODE_SDR25);
                sd_clk_change(sdio_mshc_1_base,0);

                send_single_data(sdio_mshc_1_base,0x200,24,0);
                s_err_cnt=receive_single_data(sdio_mshc_1_base,0x200,17,0);

                send_mult_data(sdio_mshc_1_base,0x200,4,25,0);
                m_err_cnt=receive_mult_data(sdio_mshc_1_base,0x200,4,18,0);

                err_cnt = s_err_cnt | m_err_cnt;
                if(err_cnt !=0)
                {
                    SDIOD_LOGI("SD1 SDR BUF Test FAILfail\r\n");
                }
                else
                {
                    SDIOD_LOGI("SD1 SDR BUF Test PASSpass\r\n");
                }
                break;

            case 1 : //SDIO1 BUFFER EMMC CARD SDR TEST
                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_1_base,3,FREQSEL_CLK_400KHZ,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //400k//addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_1_base,3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);                    //no 400k//addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_1_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_1_base,CMD6,2,CMD6_SDR_8BIT);
                sd_card_interface_set(sdio_mshc_1_base,UHS_MODE_EMMC_HS);
                sd_clk_change(sdio_mshc_1_base,0);

                send_mult_data(sdio_mshc_1_base,0x200,4,25,0);
                m_err_cnt=receive_mult_data(sdio_mshc_1_base,0x200,4,18,0);

                err_cnt= s_err_cnt|m_err_cnt;
                if(err_cnt !=0)
                {
                    SDIOD_LOGI("EMMC1 SDR BUF Test FAILfail\r\n");
                }
                else
                {
                    SDIOD_LOGI("EMMC1 SDR BUF Test PASSpass\r\n");
                }
                break;
            case 2 : //SDIO1 BUFFER EMMC CARD DDR TEST
                tuning_cfg(sdio_mshc_1_base,0x2,0x2,1,1,0x1,0x2,1,1,0);//tuning tx and rx clk

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_1_base,3,FREQSEL_CLK_400KHZ,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //400k//addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_1_base,3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);                    //no 400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_1_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_1_base,CMD6,2,CMD6_DDR_8BIT);
                sd_card_interface_set(sdio_mshc_1_base,UHS_MODE_EMMC_HSDDR);
                sd_clk_change(sdio_mshc_1_base,1);

                send_mult_data(sdio_mshc_1_base,0x200,4,25,0);
                m_err_cnt=receive_mult_data(sdio_mshc_1_base,0x200,4,18,0);

                err_cnt = s_err_cnt | m_err_cnt;
                if(err_cnt !=0)
                {
                    SDIOD_LOGI("EMMC1 DDR BUF Test FAILfail\r\n");
                }
                else
                {
                    SDIOD_LOGI("EMMC1 DDR BUF Test PASSpass\r\n");
                }
                break;
            case 3 : //SDIO1 BUFFER SDMA EMMC CARD DDR TEST
                tuning_cfg(sdio_mshc_1_base,0x2,0x2,1,1,0x1,0x2,1,1,0);//tuning tx and rx clk

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_1_base,0x3,FREQSEL_CLK_400KHZ,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_1_base,0x3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);                    //no 400k //addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_1_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_1_base,CMD6,2,CMD6_DDR_8BIT);
                sd_card_interface_set(sdio_mshc_1_base,UHS_MODE_EMMC_HSDDR);
                sd_clk_change(sdio_mshc_1_base,0);

                sdma_send_data(sdio_mshc_1_base,1,0x28020000,SDMA_BUF_BDARY_4K,1,0x200,10,25,0);         //addr,HOST_VER4_EN,SYS_ADDR,SDMA_BUF_BDARY,SDMA_BUF_BDARY_NUM,BLOCK_SIZE,BLOCK_CNT,CMD,ARGUMENT
                sdma_receive_data (sdio_mshc_1_base,1,0x28030000,SDMA_BUF_BDARY_4K,1,0x200,10,18,0);      //addr,HOST_VER4_EN,SYS_ADDR,SDMA_BUF_BDARY,SDMA_BUF_BDARY_NUM,BLOCK_SIZE,BLOCK_CNT,CMD,ARGUMENT
                break;
            case 4 : //SDIO1 ADMA2 EMMC CARD DDR TEST
                tuning_cfg(sdio_mshc_1_base,0x2,0x2,1,1,0x1,0x2,1,1,0);//tuning tx and rx clk

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_1_base,0x3,FREQSEL_CLK_400KHZ,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //400k//addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_1_base,0x3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);                    //no 400k//addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_1_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_1_base,CMD6,2,CMD6_DDR_8BIT);
                sd_card_interface_set(sdio_mshc_1_base,UHS_MODE_EMMC_HSDDR);
                sd_clk_change(sdio_mshc_1_base,0);

                adma2_send_data(sdio_mshc_1_base,0x28020000,0x0,0x200,8,25,0);
                adma2_receive_data (sdio_mshc_1_base,0x28020010,0x0,0x200,8,18,0);
                break;
            case 5: //SDIO0 ADMA3 EMMC CARD DDR TEST
                tuning_cfg(sdio_mshc_1_base,0x2,0x2,1,1,0x1,0x2,1,1,0);//tuning tx and rx clk

                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_1_base,0x3,0xff,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //400k//addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_1_base,0x3,0x1,0xa,0xa,EMMC_CARD,UHS_MODE_EMMC_DS,DATA_WIDTH8);     //no 400k//addr,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_1_base, CMD55, 2, 0x12340000);   //send ACMD6
                send_cmd(sdio_mshc_1_base,CMD6,2,CMD6_DDR_8BIT);
                sd_card_interface_set(sdio_mshc_1_base,UHS_MODE_EMMC_HSDDR);
                sd_clk_change(sdio_mshc_1_base,1);

                adma3_send_data(sdio_mshc_1_base,VER4_EN,(uint32_t)((uintptr_t)&adma3_wr_descriptor_addr[0]),0x00000000);
                adma3_receive_data(sdio_mshc_1_base,VER4_EN,(uint32_t)((uintptr_t)&adma3_rd_descriptor_addr[0]),0x00000000);
                break;
            case 6:
                tuning_cfg(sdio_mshc_1_base,0x0,0x0,0,0,0x0,0x0,0,0,1);//negedge clk send data and cmd
                #ifdef INIT_400K
                mshc_host_init(sdio_mshc_1_base,0x3,0xff,0xa,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH4);     //400K//addr,sysclk_select,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #else
                mshc_host_init(sdio_mshc_1_base,0x3,0x1,0xa,0xa,SD_CARD,UHS_MODE_SDR12,DATA_WIDTH4);     //no 400k//addr,sysclk_select,sys_div,sdclk_div,tmclk_div,cqetmclk_div,CARD_IS_EMMC,UHS_MODE_SEL,DAT_XFER_WIDTH
                #endif

                send_cmd(sdio_mshc_1_base, CMD55, 2, sdio_rca);   //0x12340000 send ACMD6
                send_cmd(sdio_mshc_1_base,CMD6,2,CMD6_SDR_4BIT);
                sd_card_interface_set(sdio_mshc_1_base,UHS_MODE_SDR25);
                sd_clk_change(sdio_mshc_1_base,1);

                send_mult_data(sdio_mshc_1_base,0x200,4,25,0);
                err_cnt=receive_mult_data(sdio_mshc_1_base,0x200,4,18,0);
                if(err_cnt !=0)
                {
                    SDIOD_LOGI("SD1 SDR BUF Test FAILfail\r\n");
                }
                else
                {
                    SDIOD_LOGI("SD1 SDR BUF Test PASSpass\r\n");
                }
                SDIOD_LOGI("SD1 negedge clk test end\r\n");
                break;
            default : //REG READ AND WRITE
                sdio_reg_w_r(sdio_mshc_1_base);
                break;
        }
    }

    SDIOD_LOGI("test end!\r\n");
}

void sdio_dwc_isr0(void)
{
    volatile  uint16  normal_int;
    volatile  uint16  error_int;

    normal_int = NORMAL_INT_STAT_R(sdio_mshc_0_base);
    error_int  = ERROR_INT_STAT_R(sdio_mshc_0_base);

    if(normal_int & CMD_COMPLETE_STAT_EN)
    {
        CMD_COMPLETE_STATE = 1;
        SDIOD_LOGI("CMD_COMP\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_CMD_COMPLETE_STAT;
    }
    if(normal_int & XFER_COMPLETE_STAT_EN)
    {
        XFER_COMPLETE_STATE = 1;
        SDIOD_LOGI("XFER_COMP\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_XFER_COMPLETE_STAT;
    }
    if(normal_int & BGAP_EVENT_STAT_EN)
    {
        BGAP_EVENT_STATE = 1;
        SDIOD_LOGI("BGAP_EVENT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_BGAP_EVENT_STAT;
    }
    if(normal_int & DMA_INTERRUPT_STAT_EN)
    {
        DMA_INTERRUPT_STATE = 1;
        SDIOD_LOGI("DMA_INT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_DMA_INTERRUPT_STAT;
    }
    if(normal_int & BUF_WR_READY_STAT_EN)
    {
        BUF_WR_READY_STATE = 1;
        SDIOD_LOGI("BUF_WR_READY\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_BUF_WR_READY_STAT;
    }
    if(normal_int & BUF_RD_READY_STAT_EN)
    {
        BUF_RD_READY_STATE = 1;
        SDIOD_LOGI("BUF_RD_READY\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_BUF_RD_READY_STAT;
    }
    if(normal_int & CARD_INSERTION_STAT_EN)
    {
        CARD_INSERTION_STATE = 1;
        SDIOD_LOGI("CARD_INSERTION\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_CARD_INSERTION_STAT;//clear the card insertion bit
    }
    if(normal_int & CARD_REMOVAL_STAT_EN)
    {
        CARD_REMOVAL_STATE = 1;
        SDIOD_LOGI("CARD_REMOVAL\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_CARD_REMOVAL_STAT;
    }
    if(normal_int & CARD_INTERRUPT_STAT_EN)
    {
        CARD_INTERRUPT_STATE = 1;
        SDIOD_LOGI("CARD_INTERRUPT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_CARD_INTERRUPT_STAT;
    }
    if(normal_int & INT_A_STAT_EN)
    {
        INT_A_STATE = 1;
        SDIOD_LOGI("INT_A\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_INT_A_STAT;
    }
    if(normal_int & INT_B_STAT_EN)
    {
        INT_B_STATE = 1;
        SDIOD_LOGI("INT_B\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_INT_B_STAT;
    }
    if(normal_int & INT_C_STAT_EN)
    {
        INT_C_STATE = 1;
        SDIOD_LOGI("INT_C\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_INT_C_STAT;
    }
    if(normal_int & RE_TUNE_EVENT_STAT_EN)
    {
        RE_TUNE_EVENT_STATE = 1;
        SDIOD_LOGI("RE_TUNE_EVENT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_RE_TUNE_EVENT_STAT;
    }
    if(normal_int & FX_EVENT_STAT_EN)
    {
        FX_EVENT_STATE = 1;
        SDIOD_LOGI("FX_EVENT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_FX_EVENT_STAT;
    }
    if(normal_int & CQE_EVENT_STAT_EN)
    {
        CQE_EVENT_STATE = 1;
        SDIOD_LOGI("CQE_EVENT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_0_base)= CLR_CQE_EVENT_STAT;
    }
    if(error_int & ERROR_INTERRUPT_STAT_EN)
    {
        ERROR_INTERRUPT_STATE = 1;
        SDIOD_LOGI("ERROR_INTERRUPT\r\n");
    }

    if(error_int & CMD_TOUT_ERR_STAT_EN)
    {
        CMD_TOUT_ERR_STATE = 1;
        SDIOD_LOGI("CMD_TOUT_ERR, command timeout error\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_CMD_TOUT_ERR_STAT;
    }
    if(error_int & CMD_CRC_ERR_STAT_EN)
    {
        CMD_CRC_ERR_STATE = 1;
        SDIOD_LOGI("CMD_CRC_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_CMD_CRC_ERR_STAT;
    }
    if(error_int & CMD_END_BIT_ERR_STAT_EN)
    {
        CMD_END_BIT_ERR_STATE = 1;
        SDIOD_LOGI("CMD_END_BIT_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_CMD_END_BIT_ERR_STAT;
    }
    if(error_int & CMD_IDX_ERR_STAT_EN)
    {
        CMD_IDX_ERR_STATE = 1;
        SDIOD_LOGI("CMD_IDX_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_CMD_IDX_ERR_STAT;
    }
    if(error_int & DATA_TOUT_ERR_STAT_EN)
    {
        DATA_TOUT_ERR_STATE = 1;
        SDIOD_LOGI("DATA_TOUT_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_DATA_TOUT_ERR_STAT;
    }
    if(error_int & DATA_CRC_ERR_STAT_EN)
    {
        DATA_CRC_ERR_STATE = 1;
        SDIOD_LOGI("DATA_CRC_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_DATA_CRC_ERR_STAT;
    }
    if(error_int & DATA_END_BIT_ERR_STAT_EN)
    {
        DATA_END_BIT_ERR_STATE = 1;
        SDIOD_LOGI("DATA_END_BIT_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_DATA_END_BIT_ERR_STAT;
    }
    if(error_int & CUR_LMT_ERR_STAT_EN)
    {
        CUR_LMT_ERR_STATE = 1;
        SDIOD_LOGI("CUR_LMT_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_CUR_LMT_ERR_STAT;
    }
    if(error_int & AUTO_CMD_ERR_STAT_EN)
    {
        AUTO_CMD_ERR_STATE = 1;
        SDIOD_LOGI("AUTO_CMD_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_AUTO_CMD_ERR_STAT;
    }
    if(error_int & ADMA_ERR_STAT_EN)
    {
        ADMA_ERR_STATE = 1;
        SDIOD_LOGI("ADMA_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_ADMA_ERR_STAT;
    }
    if(error_int & TUNING_ERR_STAT_EN)
    {
        TUNING_ERR_STATE = 1;
        SDIOD_LOGI("TUNING_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_TUNING_ERR_STAT;
    }
    if(error_int & RESP_ERR_STAT_EN)
    {
        RESP_ERR_STATE = 1;
        SDIOD_LOGI("RESP_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_RESP_ERR_STAT;
    }
    if(error_int & BOOT_ACK_ERR_STAT_EN)
    {
        BOOT_ACK_ERR_STATE = 1;
        SDIOD_LOGI("BOOT_ACK_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_BOOT_ACK_ERR_STAT;
    }
    if(error_int & VENDOR_ERR1_STAT_EN)
    {
        VENDOR_ERR1_STATE = 1;
        SDIOD_LOGI("VENDOR_ERR1\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_VENDOR_ERR1_STAT;
    }
    if(error_int & VENDOR_ERR2_STAT_EN)
    {
        VENDOR_ERR2_STATE = 1;
        SDIOD_LOGI("VENDOR_ERR2\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_VENDOR_ERR2_STAT;
    }
    if(error_int & VENDOR_ERR3_STAT_EN)
    {
        VENDOR_ERR3_STATE = 1;
        SDIOD_LOGI("VENDOR_ERR3\r\n");
        ERROR_INT_STAT_R(sdio_mshc_0_base)= CLR_VENDOR_ERR3_STAT;
    }

}

void sdio_dwc_isr1(void)
{
    volatile  uint16    normal_int;
    volatile  uint16    error_int;

    normal_int = NORMAL_INT_STAT_R(sdio_mshc_1_base);
    error_int  = ERROR_INT_STAT_R(sdio_mshc_1_base);

    if(normal_int & CMD_COMPLETE_STAT_EN)
    {
        CMD_COMPLETE_STATE = 1;
        SDIOD_LOGI("CMD_COMP\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_CMD_COMPLETE_STAT;
    }
    if(normal_int & XFER_COMPLETE_STAT_EN)
    {
        XFER_COMPLETE_STATE = 1;
        SDIOD_LOGI("XFER_COMP\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_XFER_COMPLETE_STAT;
    }
    if(normal_int & BGAP_EVENT_STAT_EN)
    {
        BGAP_EVENT_STATE = 1;
        SDIOD_LOGI("BGAP_EVENT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_BGAP_EVENT_STAT;
    }
    if(normal_int & DMA_INTERRUPT_STAT_EN)
    {
        DMA_INTERRUPT_STATE = 1;
        SDIOD_LOGI("DMA_INT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_DMA_INTERRUPT_STAT;
    }
    if(normal_int & BUF_WR_READY_STAT_EN)
    {
        BUF_WR_READY_STATE = 1;
        SDIOD_LOGI("BUF_WR_READY\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_BUF_WR_READY_STAT;
    }
    if(normal_int & BUF_RD_READY_STAT_EN)
    {
        BUF_RD_READY_STATE = 1;
        SDIOD_LOGI("BUF_RD_READY\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_BUF_RD_READY_STAT;
    }
    if(normal_int & CARD_INSERTION_STAT_EN)
    {
        CARD_INSERTION_STATE = 1;
        SDIOD_LOGI("CARD_INSERTION\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_CARD_INSERTION_STAT;//clear the card insertion bit
    }
    if(normal_int & CARD_REMOVAL_STAT_EN)
    {
        CARD_REMOVAL_STATE = 1;
        SDIOD_LOGI("CARD_REMOVAL\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_CARD_REMOVAL_STAT;
    }
    if(normal_int & CARD_INTERRUPT_STAT_EN)
    {
        CARD_INTERRUPT_STATE = 1;
        SDIOD_LOGI("CARD_INTERRUPT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_CARD_INTERRUPT_STAT;
    }
    if(normal_int & INT_A_STAT_EN)
    {
        INT_A_STATE = 1;
        SDIOD_LOGI("INT_A\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_INT_A_STAT;
    }
    if(normal_int & INT_B_STAT_EN)
    {
        INT_B_STATE = 1;
        SDIOD_LOGI("INT_B\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_INT_B_STAT;
    }
    if(normal_int & INT_C_STAT_EN)
    {
        INT_C_STATE = 1;
        SDIOD_LOGI("INT_C\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_INT_C_STAT;
    }
    if(normal_int & RE_TUNE_EVENT_STAT_EN)
    {
        RE_TUNE_EVENT_STATE = 1;
        SDIOD_LOGI("RE_TUNE_EVENT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_RE_TUNE_EVENT_STAT;
    }
    if(normal_int & FX_EVENT_STAT_EN)
    {
        FX_EVENT_STATE = 1;
        SDIOD_LOGI("FX_EVENT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_FX_EVENT_STAT;
    }
    if(normal_int & CQE_EVENT_STAT_EN)
    {
        CQE_EVENT_STATE = 1;
        SDIOD_LOGI("CQE_EVENT\r\n");
        NORMAL_INT_STAT_R(sdio_mshc_1_base)= CLR_CQE_EVENT_STAT;
    }
    if(error_int & ERROR_INTERRUPT_STAT_EN)
    {
        ERROR_INTERRUPT_STATE = 1;
        SDIOD_LOGI("ERROR_INTERRUPT\r\n");
    }

    if(error_int & CMD_TOUT_ERR_STAT_EN)
    {
        CMD_TOUT_ERR_STATE = 1;
        SDIOD_LOGI("CMD_TOUT_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_CMD_TOUT_ERR_STAT;
    }
    if(error_int & CMD_CRC_ERR_STAT_EN)
    {
        CMD_CRC_ERR_STATE = 1;
        SDIOD_LOGI("CMD_CRC_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_CMD_CRC_ERR_STAT;
    }
    if(error_int & CMD_END_BIT_ERR_STAT_EN)
    {
        CMD_END_BIT_ERR_STATE = 1;
        SDIOD_LOGI("CMD_END_BIT_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_CMD_END_BIT_ERR_STAT;
    }
    if(error_int & CMD_IDX_ERR_STAT_EN)
    {
        CMD_IDX_ERR_STATE = 1;
        SDIOD_LOGI("CMD_IDX_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_CMD_IDX_ERR_STAT;
    }
    if(error_int & DATA_TOUT_ERR_STAT_EN)
    {
        DATA_TOUT_ERR_STATE = 1;
        SDIOD_LOGI("DATA_TOUT_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_DATA_TOUT_ERR_STAT;
    }
    if(error_int & DATA_CRC_ERR_STAT_EN)
    {
        DATA_CRC_ERR_STATE = 1;
        SDIOD_LOGI("DATA_CRC_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_DATA_CRC_ERR_STAT;
    }
    if(error_int & DATA_END_BIT_ERR_STAT_EN)
    {
        DATA_END_BIT_ERR_STATE = 1;
        SDIOD_LOGI("DATA_END_BIT_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_DATA_END_BIT_ERR_STAT;
    }
    if(error_int & CUR_LMT_ERR_STAT_EN)
    {
        CUR_LMT_ERR_STATE = 1;
        SDIOD_LOGI("CUR_LMT_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_CUR_LMT_ERR_STAT;
    }
    if(error_int & AUTO_CMD_ERR_STAT_EN)
    {
        AUTO_CMD_ERR_STATE = 1;
        SDIOD_LOGI("AUTO_CMD_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_AUTO_CMD_ERR_STAT;
    }
    if(error_int & ADMA_ERR_STAT_EN)
    {
        ADMA_ERR_STATE = 1;
        SDIOD_LOGI("ADMA_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_ADMA_ERR_STAT;
    }
    if(error_int & TUNING_ERR_STAT_EN)
    {
        TUNING_ERR_STATE = 1;
        SDIOD_LOGI("TUNING_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_TUNING_ERR_STAT;
    }
    if(error_int & RESP_ERR_STAT_EN)
    {
        RESP_ERR_STATE = 1;
        SDIOD_LOGI("RESP_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_RESP_ERR_STAT;
    }
    if(error_int & BOOT_ACK_ERR_STAT_EN)
    {
        BOOT_ACK_ERR_STATE = 1;
        SDIOD_LOGI("BOOT_ACK_ERR\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_BOOT_ACK_ERR_STAT;
    }
    if(error_int & VENDOR_ERR1_STAT_EN)
    {
        VENDOR_ERR1_STATE = 1;
        SDIOD_LOGI("VENDOR_ERR1\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_VENDOR_ERR1_STAT;
    }
    if(error_int & VENDOR_ERR2_STAT_EN)
    {
        VENDOR_ERR2_STATE = 1;
        SDIOD_LOGI("VENDOR_ERR2\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_VENDOR_ERR2_STAT;
    }
    if(error_int & VENDOR_ERR3_STAT_EN)
    {
        VENDOR_ERR3_STATE = 1;
        SDIOD_LOGI("VENDOR_ERR3\r\n");
        ERROR_INT_STAT_R(sdio_mshc_1_base)= CLR_VENDOR_ERR3_STAT;
    }
}

bk_err_t bk_sdio_storage_driver_init(void)
{
        /*TODO:wangzhilei*/
        bk_int_isr_register(INT_SRC_SDIO0, (int_group_isr_t)sdio_dwc_isr0, NULL);

        bk_int_isr_register(INT_SRC_SDIO1, (int_group_isr_t)sdio_dwc_isr1, NULL);

#if CONFIG_SDIO_DWC_TEST        
        int bk_sdio_host_register_cli_test_feature(void);
        bk_sdio_host_register_cli_test_feature();
#endif

        return BK_OK;
}

bk_err_t bk_sdio_storage_driver_deinit(void)
{
        /*TODO:wangzhilei*/
        bk_int_isr_unregister(INT_SRC_SDIO0);

        bk_int_isr_unregister(INT_SRC_SDIO1);

        return BK_OK;
}

// eof

