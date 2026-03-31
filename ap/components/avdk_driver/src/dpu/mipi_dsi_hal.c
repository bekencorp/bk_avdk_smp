#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include "sys_driver.h"
#include <driver/mipi_dsi_types.h>
#include <components/bk_lcd_types.h>
#include "mipi_dsi_host_reg.h"
#include "mipi_dsi_phy_reg.h"
#include "mipi_dsi_hal.h"
#include "dpu_driver.h"
#include "sys_ahbp_reg.h"

#define TAG "dsi_hal"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

void hal_dsi_wait_fpga_dphy_done(void)
{
#if(SFT_VERSION == FPGA_7259_CM55)
    while(!(reg_EXDPHY_REG0 & 0x000001))
#elif(SFT_VERSION == FPGA_7259_A35)
    while(!(reg_EXDPHY_MN_RD & 0x1000000))
#else
    while(1)
#endif
    {
        LOGI("dsi_wait_fpga_dphy...\n");
    }

    LOGI("%s\n", __func__);
}

void hal_dsi_dphy_wr_ctrl(uint8_t test_code, uint8_t test_wr)
{
    reg_PHY_TST_CTRL1 = test_code + (0x100<<8);
    reg_PHY_TST_CTRL0 = 0x2;
    reg_PHY_TST_CTRL0 = 0x0;
    reg_PHY_TST_CTRL1 = (uint32_t)test_wr;
    reg_PHY_TST_CTRL0 = 0x2;
    reg_PHY_TST_CTRL0 = 0x0;
}

void hal_dsi_wait_for_dphy_pwrup(void)
{
    reg_PHY_RSTZ = 1 + (1<<1) + (1<<2) + (1<<3);

    rtos_delay_milliseconds(1);

    while(!(reg_PHY_STATUS & 0x1)) LOGI("hal_dsi_wait_dphy_pwrup1\n"); 
    while(!(reg_PHY_STATUS & 0x4)) LOGI("hal_dsi_wait_dphy_pwrup4\n");
}

void hal_dsi_dphy_power_down(void)
{
    reg_PHY_RSTZ = 0;
}

void hal_dsi_host_power_up(void)
{
    reg_PWR_UP = 1;    // 0:reset, 1:power up
}

void hal_dsi_host_reset(void)
{
    reg_PWR_UP = 0;    // 0:reset, 1:power up
}

void hal_dsi_config(uint8_t n_lanes, 
                    uint16_t width, 
                    uint16_t height, 
                    uint16_t hsa_time, 
                    uint16_t hbp_time, 
                    uint16_t hline_time, 
                    uint16_t vsa_line, 
                    uint16_t vbp_line, 
                    uint16_t vfp_line
                )
{
    //configure Display Pixel Interface
    //~dpi_vc_id
    reg_DPI_VCID              = 0;
    //~dpi_color_mode\ loosely packed variant to 18bit config.
    reg_DPI_COLOR_CODING      = 5 + (0<<8);
    //~dpi datten active low\ vsync low\ hsync low\ shut_down pin low\ color mode pin low
    reg_DPI_CFG_POL           = 0 + (1<<1) + (1<<2) + (0<<3) + (0<<4);
    //~div tx esc clk(lanebyteclk)\ div time out clk.
    reg_CLKMGR_CFG            = 16 + (1<<8); //16is clk div
    // reg_CLKMGR_CFG            = 16 + (1<<8); //16is clk div

    //configure Display Bus Interface
    //~ena gen short or long packet with diff pararm.
    //~ena dsc short or long packet with diff param\ cfg max rd packet size. all packet in lp mode, dis te & ack.
    reg_CMD_MODE_CFG          = 0 + (0<<1) + (1<<8) + (1<<9) + (1<<10) + (1<<11) + (1<<12) + (1<<13) + (1<<14)
                                  + (1<<16) + (1<<17) + (1<<18) + (1<<19) + (1<<24);

    //Configure Packet Handler
    //~eotp tx ena\ eotp rx ena\ bta req ena\ ena ecc rx\ ena crc rx.
    reg_PCKHDL_CFG            = 0 + (0<<1) + (0<<2) + (1<<3) + (1<<4);
    //~generic vc\ generic vc for te req.
    reg_GEN_VCID              = 0 + (3<<8);  //Read responses Virtual Channel ID
    //~vid cmd mode sel.
    reg_MODE_CFG              = 1;//please add in cmd mode, before open dpu.
    //~video tran mode\ ena lp vsa\ ena lp vbp\ ena lp vfp\ ena lp vact\ ena lp hbp\ ena lp hfp\ ena lp cmd transfer.
    //~ena vpg\ pattern gen mode\ pattern orientaton.
    reg_VID_MODE_CFG          = 2 + (1<<8) + (1<<9) + (1<<10) + (1<<11) + (1<<12) + (1<<13) + (1<<15)
                                  + (0<<16) + (0<<20) + (0<<24);

    /*cfg for dpu 120mhz, dsi in 100mhz*/
    //~pixel per packet.
    reg_VID_PKT_SIZE          = width;
    //~number of chunks
    reg_VID_NUM_CHUNKS        = 0;
    //~numm packet size.
    reg_VID_NULL_SIZE         = 0;
    //~hsa period cfg.
    reg_VID_HSA_TIME          = hsa_time;
    //~hbp period cfg.
    reg_VID_HBP_TIME          = hbp_time;
    //~hline period cfg.
    reg_VID_HLINE_TIME        = hline_time;
    //~vsa line cfg.
    reg_VID_VSA_LINES         = vsa_line;
    //~vbp line cfg.
    reg_VID_VBP_LINES         = vbp_line;
    //~vfp line cfg.
    reg_VID_VFP_LINES         = vfp_line;
    //~size of most packet can fit in line during vact\ size of most packet can fit in line during vsa, vbp, vfp
    reg_DPI_LP_CMD_TIM        = 0 + (0<<16);
    //~vact line cfg.
    reg_VID_VACTIVE_LINES     = height;

    //define SFR2GENERIC ADDRESS REGIS
    //~max time go lp to hs trans in lane byte clk\ max time go hs to lp trans in lane byte clk.
    reg_PHY_TMR_CFG           = 0x60 + (0x26<<16);
    //~max size edpi wr mem cmd.
    reg_EDPI_CMD_SIZE         = 0x0;
    //~ena te by hdware\ change te by hdw priorities\ cfg dcs packet type by host\ cfg param that te out line.
    reg_EDPI_TE_HW_CFG        = 0 + (0<<1) + (0<<4) + (0<<8);
    //~req hs clock trans.\ auto stop provide clk
    reg_LPCLK_CTRL            = 1 + (1<<1);

    //Configure core's timeouts
    //~timeout counter hs trans\ timeout counter lp rece.
    reg_TO_CNT_CFG            = 0 + (0<<16);
    //~time out after hs read
    reg_HS_RD_TO_CNT          = 0;
    //~time out after lp read
    reg_LP_RD_TO_CNT          = 0;
    //~peri resp timeout after hs wr\ time out mode.
    reg_HS_WR_TO_CNT          = 0 + (0<<24);
    //~peri resp timeout after lp wr.
    reg_LP_WR_TO_CNT          = 0;
    //~peri resp timeout after bta.
    reg_BTA_TO_CNT            = 0;

    //Configure core's phy parameters
    //~maxtime goto lp to hs trans in lane byteclk\ maxtime goto hs to lp trans in lane byte clk.
    reg_PHY_TMR_LPCLK_CFG     = 0x7d + (0x38<<16);
    //~maxtime req to perform a read cmd in lane byte clk.
    reg_PHY_TMR_RD_CFG        = 0;
    //~number of active lane, phy wait time to hstx.
    reg_PHY_IF_CFG            = (n_lanes&0x3) + (0x28<<8);  // lane num, 0:1 lanes, 1:2lanes, 2:3lanes, 3:4lanes
    //~ulps mode req on clk lane\ ulps mode exit on clk lane\ ulps mode req on data lane\ ulps mde exit on data lanes.
    reg_PHY_ULPS_CTRL         = 0 + (0<<1) + (0<<2) + (0<<3);
    //~trigger transmission
    reg_PHY_TX_TRIGGERS       = 0x0;
    //~shadow ctrl.
    reg_VID_SHADOW_CTRL       = 0x0 + (0<<8) + (0<<16);

    //ifdef edpi interf
    //~cfg delay in lanebyteclk before in ulps
    reg_AUTO_ULPS_ENTRY_DELAY = 0;
    //~twakeup clock div. min is 1\ counter of twakeup in pclk. min is 1
    reg_AUTO_ULPS_WAKEUP_TIME = 0 + (0<<16);
    //~min req between .. and .. in clock and data lane.
    reg_AUTO_ULPS_MIN_TIME    = 0;

    //~ena all int
    reg_INT_MSK0              = 0xffffffff;
    reg_INT_MSK1              = 0xffffffff;

    //~power on or reset contrller.
    reg_PWR_UP                = 0xf;

    hal_dsi_wait_for_dphy_pwrup();

    //~ena auto enter & exit ULPS.\ turn off DPHY during ULPS.\ while ena, allow to turnoff pll before req enter in ULPS.
    reg_AUTO_ULPS_MODE        = 0 + (0<<16) + (0<<17);

    LOGI("%s\n", __func__);
}



/*
 * 0x0 (VIDMODE): video mode
 * 0x1 (CMDMODE): command mode
 */
void hal_dsi_operation_mode_set(uint32_t mode)
{
    if((reg_MODE_CFG & 0x1) != (mode & 0x1))
    {
        reg_MODE_CFG = mode & 0x1;
        // LOGI("%s: %d\n", __func__, mode);
    }
}

uint32_t hal_dsi_operation_mode_get(void)
{
    return reg_MODE_CFG & 0x1;
}

void hal_dsi_dphy_init(uint32_t br)
{
    //clane_param0, time cunt, lptx after rset
    if (br < DPHY_BR_200M)      reg_NN_PHY_R00 = 0x28;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R00 = 0x50;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R00 = 0xC8;
    else                        reg_NN_PHY_R00 = 0xFA;

    //clane_param1, lp11 hold during initial.
    if (br < DPHY_BR_200M)      reg_NN_PHY_R04 = 0xFA0;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R04 = 0x1F40;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R04 = 0x4E20;
    else                        reg_NN_PHY_R04 = 0x61A8;

    //clane_param2
    if (br < DPHY_BR_200M)      reg_NN_PHY_R08 = (0x0<<16) + (0x07<<8) + 0x2;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R08 = (0x1<<16) + (0x0D<<8) + 0x2;
    else if(br < DPHY_BR_800M)  reg_NN_PHY_R08 = (0x3<<16) + (0x1A<<8) + 0x2;
    else if(br < DPHY_BR_1000M) reg_NN_PHY_R08 = (0x5<<16) + (0x20<<8) + 0x2;
    else if(br < DPHY_BR_1200M) reg_NN_PHY_R08 = (0x7<<16) + (0x28<<8) + 0x2;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R08 = (0x8<<16) + (0x2D<<8) + 0x2;
    else                        reg_NN_PHY_R08 = (0x9<<16) + (0x33<<8) + 0x2;

    //clane_param3
    if (br < DPHY_BR_200M)      reg_NN_PHY_R0c = (0x08<<16) + (0x03<<8) + 0x03;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R0c = (0x0A<<16) + (0x03<<8) + 0x06;
    else if(br < DPHY_BR_800M)  reg_NN_PHY_R0c = (0x0D<<16) + (0x07<<8) + 0x0B;
    else if(br < DPHY_BR_1000M) reg_NN_PHY_R0c = (0x0F<<16) + (0x08<<8) + 0x0D;
    else if(br < DPHY_BR_1200M) reg_NN_PHY_R0c = (0x10<<16) + (0x0A<<8) + 0x10;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R0c = (0x12<<16) + (0x0C<<8) + 0x12;
    else                        reg_NN_PHY_R0c = (0x14<<16) + (0x0D<<8) + 0x15;

    //dlane0_param0
    if (br < DPHY_BR_200M)      reg_NN_PHY_R10 = 0x28;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R10 = 0x50;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R10 = 0xc8;
    else                        reg_NN_PHY_R10 = 0xFA;

    //dlane0_param1
    if (br < DPHY_BR_200M)      reg_NN_PHY_R14 = 0xFA0;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R14 = 0x1F40;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R14 = 0x4E20;
    else                        reg_NN_PHY_R14 = 0x61A8;

    //dlane0_param2, hs-prepare, hs-zero, hs-trail, hs-exit hold time in byteclk.
    if (br < DPHY_BR_200M)      reg_NN_PHY_R18 = (0x0<<24) + (0x03<<16) + (0x3<<8) + 0x03;  // hs-zero must set 0x64 ???
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R18 = (0x2<<24) + (0x05<<16) + (0x4<<8) + 0x06;
    else if(br < DPHY_BR_800M)  reg_NN_PHY_R18 = (0x4<<24) + (0x0B<<16) + (0x7<<8) + 0x0B;
    else if(br < DPHY_BR_1000M) reg_NN_PHY_R18 = (0x6<<24) + (0x0C<<16) + (0x8<<8) + 0x0D;
    else if(br < DPHY_BR_1200M) reg_NN_PHY_R18 = (0x8<<24) + (0x0E<<16) + (0xA<<8) + 0x10;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R18 = (0xA<<24) + (0x0F<<16) + (0xC<<8) + 0x12;
    else                        reg_NN_PHY_R18 = (0xA<<24) + (0x14<<16) + (0xE<<8) + 0x15;

    //dlane0_param3
    if (br < DPHY_BR_200M)      reg_NN_PHY_R1c = 0x9C40;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R1c = 0x13880;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R1c = 0x30D40;
    else                        reg_NN_PHY_R1c = 0x3D090;

    //dlane0_param4
    if (br < DPHY_BR_200M)      reg_NN_PHY_R20 = (0x0B<<16) + (0x04<<8) + 0x0E;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R20 = (0x0F<<16) + (0x04<<8) + 0x13;
    else if(br < DPHY_BR_800M)  reg_NN_PHY_R20 = (0x0F<<16) + (0x04<<8) + 0x13;//(0x23<<16) + (0x0A<<8) + 0x2C;
    else if(br < DPHY_BR_1000M) reg_NN_PHY_R20 = (0x2B<<16) + (0x0C<<8) + 0x36;
    else if(br < DPHY_BR_1200M) reg_NN_PHY_R20 = (0x33<<16) + (0x0E<<8) + 0x40;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R20 = (0x3F<<16) + (0x12<<8) + 0x4F;
    else                        reg_NN_PHY_R20 = (0x53<<16) + (0x16<<8) + 0x68;

    //dlane1_param0
    if (br < DPHY_BR_200M)      reg_NN_PHY_R24 = 0x28;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R24 = 0x50;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R24 = 0xC8;
    else                        reg_NN_PHY_R24 = 0xFA;

    //dlane1_param1
    if (br < DPHY_BR_200M)      reg_NN_PHY_R28 = 0xFA0;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R28 = 0x1F40;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R28 = 0x4E20;
    else                        reg_NN_PHY_R28 = 0x61A8;

    //dlane1_param2, hs-prepare, hs-zero, hs-rail, hs-exit hold time in byteclk.
    if (br < DPHY_BR_200M)      reg_NN_PHY_R2c = (0x0<<24) + (0x03<<16) + (0x3<<8) + 0x03; // hs-zero must set 0x64 ???
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R2c = (0x2<<24) + (0x05<<16) + (0x4<<8) + 0x06;
    else if(br < DPHY_BR_800M)  reg_NN_PHY_R2c = (0x4<<24) + (0x0B<<16) + (0x7<<8) + 0x0B;
    else if(br < DPHY_BR_1000M) reg_NN_PHY_R2c = (0x6<<24) + (0x0C<<16) + (0x8<<8) + 0x0D;
    else if(br < DPHY_BR_1200M) reg_NN_PHY_R2c = (0x8<<24) + (0x0E<<16) + (0xA<<8) + 0x10;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R2c = (0xA<<24) + (0x0F<<16) + (0xC<<8) + 0x12;
    else                        reg_NN_PHY_R2c = (0xA<<24) + (0x14<<16) + (0xE<<8) + 0x15;

    //dlane1_pram3
    if (br < DPHY_BR_200M)      reg_NN_PHY_R30 = 0x9C40;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R30 = 0x13880;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R30 = 0x30D40;
    else                        reg_NN_PHY_R30 = 0x3D090;

    //dlane2_param0
    if (br < DPHY_BR_200M)      reg_NN_PHY_R34 = 0x28;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R34 = 0x50;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R34 = 0xC8;
    else                        reg_NN_PHY_R34 = 0xFA;

    //dlane2_param1
    if (br < DPHY_BR_200M)      reg_NN_PHY_R38 = 0xFA0;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R38 = 0x1F40;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R38 = 0x4E20;
    else                        reg_NN_PHY_R38 = 0x61A8;

    //dlane2_param2, hs-prepare, hs-zero, hs-rail, hs-exit hold time in byteclk.
    if (br < DPHY_BR_200M)      reg_NN_PHY_R3c = (0x0<<24) + (0x03<<16) + (0x3<<8) + 0x03; // hs-zero must set 0x64 ???
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R3c = (0x2<<24) + (0x05<<16) + (0x4<<8) + 0x06;
    else if(br < DPHY_BR_800M)  reg_NN_PHY_R3c = (0x4<<24) + (0x0B<<16) + (0x7<<8) + 0x0B;
    else if(br < DPHY_BR_1000M) reg_NN_PHY_R3c = (0x6<<24) + (0x0C<<16) + (0x8<<8) + 0x0D;
    else if(br < DPHY_BR_1200M) reg_NN_PHY_R3c = (0x8<<24) + (0x0E<<16) + (0xA<<8) + 0x10;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R3c = (0xA<<24) + (0x0F<<16) + (0xC<<8) + 0x12;
    else                        reg_NN_PHY_R3c = (0xA<<24) + (0x14<<16) + (0xE<<8) + 0x15;

    //dlane2_param3
    if (br < DPHY_BR_200M)      reg_NN_PHY_R40 = 0x9C40;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R40 = 0x13880;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R40 = 0x30D40;
    else                        reg_NN_PHY_R40 = 0x3D090;

    //dlane3_param0
    if (br < DPHY_BR_200M)      reg_NN_PHY_R44 = 0x28;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R44 = 0x50;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R44 = 0xC8;
    else                        reg_NN_PHY_R44 = 0xFA;

    //dlane3_param1
    if (br < DPHY_BR_200M)      reg_NN_PHY_R48 = 0xFA0;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R48 = 0x1F40;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R48 = 0x4E20;
    else                        reg_NN_PHY_R48 = 0x61A8;

    //dlane3_param2, hs-prepare, hs-zero, hs-rail, hs-exit hold time in byteclk.
    if (br < DPHY_BR_200M)      reg_NN_PHY_R4c = (0x0<<24) + (0x03<<16) + (0x3<<8) + 0x03; // hs-zero must set 0x64 ???
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R4c = (0x2<<24) + (0x05<<16) + (0x4<<8) + 0x06;
    else if(br < DPHY_BR_800M)  reg_NN_PHY_R4c = (0x4<<24) + (0x0B<<16) + (0x7<<8) + 0x0B;
    else if(br < DPHY_BR_1000M) reg_NN_PHY_R4c = (0x6<<24) + (0x0C<<16) + (0x8<<8) + 0x0D;
    else if(br < DPHY_BR_1200M) reg_NN_PHY_R4c = (0x8<<24) + (0x0E<<16) + (0xA<<8) + 0x10;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R4c = (0xA<<24) + (0x0F<<16) + (0xC<<8) + 0x12;
    else                        reg_NN_PHY_R4c = (0xA<<24) + (0x14<<16) + (0xE<<8) + 0x15;

    //dlane3_param3
    if (br < DPHY_BR_200M)      reg_NN_PHY_R50 = 0x9C40;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R50 = 0x13880;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R50 = 0x30D40;
    else                        reg_NN_PHY_R50 = 0x3D090;

    //com_param0, the number of byteclk cycles of transmitted length of lp state period
    reg_NN_PHY_R54 = 0x0A; //0x15;

    //ctrl_param0
    reg_NN_PHY_R58 = 0x38;

    /*[26:24]:rate [2:0], Date Rate=FVCO/(1<< rate), rate: 0:(1600Mbps, 3200Mbps];b001: (800Mbps, 1600Mbps]; b010: (400Mbsp, 800Mbps]; 011: (200Mbps, 400Mbps]; b100: (100Mbps, 200Mbps];101~111: (50Mbps, 100Mbps];
    * [22:19]：NREF_DIV:refclk_div[3:0],  refclk_div=0x0 表示不进行分频
    * [18:14]:NI_POST_DIV: 6~14
    * [13:4]:NF_POST_DIV ：0：0/1024， 1：1/1024， 2：2/1024， 等， 1023：1023/1024
    * [3:0]:dsi_pixelclk_div: 0表示2分频, 1:3分频， 2:4分频， 3:5分频， 4:6分频， 5:7分频， 6:8分频， 7:9分频， 8:10分频， dsi_pixelclk_div+2=实际分频数
    * FREFCLK=26M；NLOOP_DIV = 8 * ( NI_POST_DIV + NF_POST_DIV ), FVCO = FREFCLK * NLOOP_DIV / NREF_DIV;
    * Pixel Clock：Date Rate/dsi_pixelclk_div
    * 以DPHY_BR_950M为例：
    * 1. 计算fvco：fvco = FREFCLK * NLOOP_DIV / NREF_DIV = 26M*8*(8+0x2c3/0x400) = 1.9g
    * 2. 计算data_rate：data_rate = fvco/2 = 0.95g
    * 3. 计算byte_clk(输出)：byte_clk = data_rate/8 = 95Mhz
    */
    //pll_ctrl_param0
    /* 注释与上文 pll 说明一致：FREFCLK=26M；FVCO=26m*8*(NI+NF/0x400)/NREF(NREF=0 不分频)；
     * data_rate=FVCO/(1<<[26:24])；byte_clk=data_rate/8；DPI pixel_clk=data_rate/(dsi_pixelclk_div+2)，dsi 为 [3:0]。 */
    if(br == DPHY_BR_100M)       reg_NN_PHY_R5c = (0x4<<24) + (0x0<<19) + (0x8<<14) + (0x155<<4) + 0x6;  //fvco=26m*8*(8+0x155/0x400)≈1.733g, data_rate=fvco/(1<<4)≈0.108g, byte_clk=data_rate/8≈13.5MHz, dpi=data_rate/(6+2)≈13.5MHz
    else if(br == DPHY_BR_200M)  reg_NN_PHY_R5c = (0x3<<24) + (0x0<<19) + (0x8<<14) + (0x155<<4) + 0x6;  //fvco=26m*8*(8+0x155/0x400)≈1.733g, data_rate=fvco/(1<<3)≈0.217g, byte_clk=data_rate/8≈27.1MHz, dpi=data_rate/(6+2)≈27.1MHz
    else if(br == DPHY_BR_300M)  reg_NN_PHY_R5c = (0x3<<24) + (0x0<<19) + (0xC<<14) + (0x200<<4) + 0x6;  //fvco=26m*8*(0xC+0x200/0x400)≈2.600g, data_rate=fvco/(1<<3)≈0.325g, byte_clk=data_rate/8≈40.6MHz, dpi=data_rate/(6+2)≈40.6MHz
    else if(br == DPHY_BR_400M)  reg_NN_PHY_R5c = (0x2<<24) + (0x0<<19) + (0x8<<14) + (0x155<<4) + 0x6;  //fvco=26m*8*(8+0x155/0x400)≈1.733g, data_rate=fvco/(1<<2)≈0.433g, byte_clk=data_rate/8≈54.2MHz, dpi=data_rate/(6+2)≈54.2MHz
    else if(br == DPHY_BR_440M)  reg_NN_PHY_R5c = (0x2<<24) + (0x0<<19) + (0x9<<14) + (0x0AB<<4) + 0x9;  //fvco=26m*8*(9+0xAB/0x400)≈1.907g, data_rate=fvco/(1<<2)≈0.477g, byte_clk=data_rate/8≈59.6MHz, dpi=data_rate/(9+2)≈43.3MHz
    else if(br == DPHY_BR_500M)  reg_NN_PHY_R5c = (0x2<<24) + (0x0<<19) + (0xA<<14) + (0x1AB<<4) + 0x6;  //fvco=26m*8*(0xA+0x1AB/0x400)≈2.167g, data_rate=fvco/(1<<2)≈0.542g, byte_clk=data_rate/8≈67.7MHz, dpi=data_rate/(6+2)≈67.7MHz
    else if(br == DPHY_BR_600M)  reg_NN_PHY_R5c = (0x2<<24) + (0x0<<19) + (0xC<<14) + (0x200<<4) + 0x4;  //fvco=26m*8*(0xC+0x200/0x400)≈2.600g, data_rate=fvco/(1<<2)≈0.650g, byte_clk=data_rate/8≈81.2MHz, dpi=data_rate/(4+2)≈60.8MHz
    else if(br == DPHY_BR_700M)  reg_NN_PHY_R5c = (0x2<<24) + (0x0<<19) + (0xC<<14) + (0x200<<4) + 0x4;  //fvco=26m*8*(0xC+0x200/0x400)≈2.600g, data_rate=fvco/(1<<2)≈0.650g, dpi=data_rate/(4+2)=650/6≈108.3MHz
    else if(br == DPHY_BR_750M)  reg_NN_PHY_R5c = (0x2<<24) + (0x0<<19) + (0xA<<14) + (0x1AB<<4) + 0x4;  //fvco=26m*8*(0xA+0x1AB/0x400)≈2.167g, data_rate=fvco/(1<<2)≈0.542g, byte_clk=data_rate/8≈67.7MHz, dpi=data_rate/(4+2)≈90.3MHz
    else if(br == DPHY_BR_800M)  reg_NN_PHY_R5c = (0x1<<24) + (0x0<<19) + (0x8<<14) + (0x100<<4) + 0x7;  //fvco=26m*8*(8+0x100/0x400)≈1.716g, data_rate=fvco/(1<<1)≈0.858g, byte_clk=data_rate/8≈107.2MHz, dpi=data_rate/(7+2)≈95.3MHz
    else if(br == DPHY_BR_900M)  reg_NN_PHY_R5c = (0x1<<24) + (0x0<<19) + (0x8<<14) + (0x2c3<<4) + 0x7;  //fvco=26m*8*(8+0x2c3/0x400)≈1.808g, data_rate=fvco/(1<<1)≈0.904g, byte_clk=data_rate/8≈113.0MHz, dpi=data_rate/(7+2)≈100.4MHz
    else if(br == DPHY_BR_950M)  reg_NN_PHY_R5c = (0x1<<24) + (0x0<<19) + (0x8<<14) + (0x255<<4) + 0x7;  //fvco=26m*8*(8+0x255/0x400)≈1.858g, data_rate=fvco/(1<<1)≈0.929g, byte_clk=data_rate/8≈120.0MHz, dpi=data_rate/(7+2)≈102.1MHz
    else if(br == DPHY_BR_1000M) reg_NN_PHY_R5c = (0x1<<24) + (0x0<<19) + (0xA<<14) + (0x000<<4) + 0x8;  //fvco=26m*8*(0xA+0/0x400)≈2.080g, data_rate=fvco/(1<<1)≈1.040g, byte_clk=data_rate/8≈130.0MHz, dpi=data_rate/(8+2)≈104.0MHz
    else if(br == DPHY_BR_1200M) reg_NN_PHY_R5c = (0x1<<24) + (0x0<<19) + (0xC<<14) + (0x200<<4) + 0x6;  //fvco=26m*8*(0xC+0x200/0x400)≈2.600g, data_rate=fvco/(1<<1)≈1.300g, byte_clk=data_rate/8≈162.5MHz, dpi=data_rate/(6+2)≈162.5MHz
    else if(br == DPHY_BR_1400M) reg_NN_PHY_R5c = (0x1<<24) + (0x0<<19) + (0xE<<14) + (0x255<<4) + 0x6;  //fvco=26m*8*(0xE+0x255/0x400)≈3.033g, data_rate=fvco/(1<<1)≈1.517g, byte_clk=data_rate/8≈189.6MHz, dpi=data_rate/(6+2)≈189.6MHz
    else if(br == DPHY_BR_1500M) reg_NN_PHY_R5c = (0x1<<24) + (0x0<<19) + (0xF<<14) + (0x280<<4) + 0x6;  //fvco=26m*8*(0xF+0x280/0x400)≈3.250g, data_rate=fvco/(1<<1)≈1.625g, byte_clk=data_rate/8≈203.1MHz, dpi=data_rate/(6+2)≈203.1MHz
    else if(br == DPHY_BR_1600M) reg_NN_PHY_R5c = (0x0<<24) + (0x0<<19) + (0x8<<14) + (0x155<<4) + 0x6;  //fvco=26m*8*(8+0x155/0x400)≈1.733g, data_rate=fvco/(1<<0)≈1.733g, byte_clk=data_rate/8≈216.7MHz, dpi=data_rate/(6+2)≈216.7MHz
    else                         reg_NN_PHY_R5c = (0x2<<24) + (0x0<<19) + (0xC<<14) + (0x200<<4) + 0x6;  //默认：同 DPHY_BR_600M，fvco=26m*8*(0xC+0x200/0x400)≈2.600g, data_rate=fvco/(1<<2)≈0.650g, byte_clk=data_rate/8≈81.2MHz, dpi=data_rate/(6+2)≈81.2MHz
    //pll_ctrl_param1 //!do not cfg temp
    //reg_NN_PHY_R60 = 0x0;
    //rcal_ctrl //!do not cfg temp
    //reg_NN_PHY_R64 = 0x2e00;
    //trim_param //!do not cfg temp
    //reg_NN_PHY_R68 = 0x3322;
    //test_pram0
    reg_NN_PHY_R6c = 0x0;
    //test_param1
    reg_NN_PHY_R70 = 0x9c40c0;
    //misc_param
    reg_NN_PHY_R74 = 0x7f;

    //clane_param4
    if (br < DPHY_BR_200M)      reg_NN_PHY_R78 = 0x9C40;
    else if(br < DPHY_BR_400M)  reg_NN_PHY_R78 = 0x13880;
    else if(br < DPHY_BR_1400M) reg_NN_PHY_R78 = 0x30D40;
    else                        reg_NN_PHY_R78 = 0x3D090;

    //interf_param
    reg_NN_PHY_R7c = (0x10<<8) + 0x01;

    //pcs_resev_pin_param
    reg_NN_PHY_R80 = 0x0;
    //pma_resev_pin_param0
    reg_NN_PHY_R84 = 0x21d9b36;
    //pam_resev_pin_param1
    reg_NN_PHY_R88 = 0x0;
    //clane_data_param
    reg_NN_PHY_R8c = 0x0aa;
    //pam_lane_sel_param
    reg_NN_PHY_R90 = 0x0d;
    //dphytx_pma_dbg //!do not cfg temp
    //reg_NN_PHY_R94 = 0x0;
    //dphytx_pcs_dbg0 //!do not cfg temp
    //reg_NN_PHY_R98 = 0x0;
    //dphytx_pcs_dbg1 //!do not cfg temp
    //reg_NN_PHY_R9c = 0x0;
    //dphytx_pcs_dbg2 //!do not cfg temp
    //reg_NN_PHY_Ra0 = 0x0;
    //dphytx_pcs_dbg3 //!do not cfg temp
    //reg_NN_PHY_Ra4 = 0x0;
    //dphytx_pcs_dbg4 //!do not cfg temp
    //reg_NN_PHY_Ra8 = 0x0;
    //dphytx_pcs_dbg5 //!do not cfg temp
    //reg_NN_PHY_Rac = 0x0;
    //dphytx_pcs_dbg6 //!do not cfg temp
    //reg_NN_PHY_Rb0 = 0x0;
    //dphytx_pcs_dbg7 //!do not cfg temp
    //reg_NN_PHY_Rb4 = 0x0;
    //dphytx_pcs_dbg8 //!do not cfg temp
    //reg_NN_PHY_Rb8 = 0x0;
    //dphytx_pcs_dbg9 //!do not cfg temp
    //reg_NN_PHY_Rbc = 0x0;

    // dcreg_DPU_Beken_01 = 0x00000002 + 1;
}

/* PHY supported bitrate table (Mbps), ascending, used to select the minimum bitrate satisfying the bandwidth constraint */
static const uint32_t s_dphy_supported_bitrates[] = {
    DPHY_BR_100M, DPHY_BR_200M, DPHY_BR_300M, DPHY_BR_400M, DPHY_BR_440M,
    DPHY_BR_500M, DPHY_BR_600M, DPHY_BR_700M,DPHY_BR_750M, DPHY_BR_800M, DPHY_BR_900M,DPHY_BR_950M,DPHY_BR_1000M,
    DPHY_BR_1200M, DPHY_BR_1400M, DPHY_BR_1500M, DPHY_BR_1600M,
};
#define DPHY_BITRATE_TABLE_SIZE  (sizeof(s_dphy_supported_bitrates) / sizeof(s_dphy_supported_bitrates[0]))

/**
 * according to DPU clock and lane count, calculate the minimum DPHY bitrate, with 1.3 margin.
 * constraint1: dpu_clk(MHz)*3*8 <  1.3 *bitrate*(n_lanes+1)  =>  bitrate > dpu_clk*24/lane_count  =>  min_bitrate
 * constraint2: dpu_clk(MHz)*3*8 > 1.3 * (1/3)*bitrate*(n_lanes+1)  =>  bitrate < dpu_clk*72/lane_count  =>  max_bitrate
 */
uint32_t dsi_dphy_bitrate_calc(lcd_clk_t dpu_clk, uint8_t n_lanes)
{
    uint32_t dpu_clk_mhz = (uint32_t)dpu_clk;
    uint32_t lane_count = (uint32_t)n_lanes + 1u; /* n_lanes 为 0~3 表示 1~4 lane */

    if (n_lanes > DSI_ACTIVE_LANES_4) {
        LOGE("dsi n_lanes=%d invalid\n", n_lanes);
        return 0;
    }
    if(dpu_clk_mhz > 100)
    {
        dpu_clk_mhz += 10;    
    }
    /* 1: dpu_clk*24 < 1.2 * bitrate*lane_count  =>  min_bitrate = floor(dpu_clk*24/lane_count) + 1 */
    uint32_t min_bitrate = (dpu_clk_mhz * 24u * 1.35f) / lane_count + 1u;

    /* 2: 1.2 * bitrate < dpu_clk*72/lane_count  =>  max_bitrate = floor(dpu_clk*72/lane_count) */
    uint32_t max_val = dpu_clk_mhz * 72u;
    uint32_t max_bitrate = (uint32_t)(max_val / lane_count);

    if (min_bitrate > max_bitrate) {
        LOGE("dsi clk:%u MHz, n_lanes=%d, no valid bitrate: min=%u max=%u\n",
             dpu_clk_mhz, n_lanes, min_bitrate, max_bitrate);
        return 0;
    }

    for (size_t i = 0; i < DPHY_BITRATE_TABLE_SIZE; i++) {
        if (s_dphy_supported_bitrates[i] >= min_bitrate &&
            s_dphy_supported_bitrates[i] <= max_bitrate) {
            LOGI("dsi clk:%u MHz, n_lanes=%d, min_bitrate=%u,  bitrate=%u\n",
                 dpu_clk_mhz, lane_count, min_bitrate, s_dphy_supported_bitrates[i]);
            return s_dphy_supported_bitrates[i];
        }
    }

    LOGE("dsi clk:%u MHz, n_lanes=%d, min_bitrate=%u, max_bitrate=%u, no supported bitrate in range\n",
         dpu_clk_mhz, n_lanes, min_bitrate, max_bitrate);
    return 0;
}


/**
 * Send READ packet to peripheral using the generic interface
 * This will force command mode and stop video mode (because of BTA)
 * @param vc destination virtual channel
 * @param data_type generic command type
 * @param lsb_byte first command parameter, (if DCS, it is the DCS command)
 * @param msb_byte second command parameter, (only parameter of short DCS packet)
 * @param bytes_to_read no of bytes to read (expected to arrive at buffer)
 * @param read_buffer pointer to 8-bit array to hold the read buffer words
 * return status
 * @note this function will enable BTA
 */
uint16_t hal_dsi_gen_read_pkt(  uint8_t vc, uint8_t data_type, 
                            uint8_t msb_byte, uint8_t lsb_byte, 
                            uint8_t bytes_to_read, uint8_t *read_buffer)
{
    uint32_t save_mode;

    if((vc >= 4) || (bytes_to_read < 1) || (read_buffer == NULL))
        return false;

    save_mode = hal_dsi_operation_mode_get();       // save mode
    hal_dsi_operation_mode_set(1);                  // in cmd mode
    reg_PCKHDL_CFG |= 1 << 2;                       // BTA enable

    while(reg_CMD_PKT_STATUS & ( 1 << 1))  LOGE("%s cmd full 1\n", __func__);

    reg_GEN_HDR = (bytes_to_read << 8 ) | ((vc << 6) | 0x37);                      // set maximum return packet size

    while(reg_CMD_PKT_STATUS & ( 1 << 1))  LOGE("%s cmd full 2\n", __func__);

    reg_GEN_HDR = (msb_byte <<  16) | (lsb_byte << 8 ) | ((vc << 6) | data_type);  // short read with 0, 1, 2 parameters

    rtos_delay_milliseconds(1);                     // delay for reg_GEN_HDR is done

    while(reg_CMD_PKT_STATUS & ( 1 << 6))  LOGE("%s cmd busy\n", __func__);

    for(uint8_t i = 0; i < bytes_to_read; i += 4)
    {
        uint32_t rd_data = reg_GEN_PLD_DATA;

        if((i + 4) <= bytes_to_read)
            *((uint32_t*)(read_buffer + i)) = rd_data;
        else
        {
            for(uint8_t j = 0; j < (bytes_to_read%4); j++)
                read_buffer[i + j] = (rd_data >> (8*j)) & 0xFF;
        }
    }

    reg_PCKHDL_CFG &= ~(1 << 2);                    // BTA disable
    hal_dsi_operation_mode_set(save_mode);          // save mode write back

    // LOGI("%s, cmd:%x, len:%d\n", __func__, lsb_byte, bytes_to_read);

    return true;
}



/**
 * Send a packet on the generic interface
 * @param vc destination virtual channel
 * @param data_type type of command, inserted in first byte of header
 * @param lsb_byte first command parameter, (if DCS, it is the DCS command)
 * @param msb_byte second command parameter, (only parameter of short DCS packet)
 * @param params byte array of command parameters
 * @param param_length length of the above array
 * @return error code
 * @note the controller restricts the sending of .
 * This function will not be able to send Null and Blanking packets due to
 *  controller restriction
 */
uint16_t hal_dsi_gen_write_pkt( uint8_t vc, uint8_t data_type, 
                            uint8_t msb_byte, uint8_t lsb_byte, 
                            uint16_t param_length, const uint8_t *params)
{
    uint32_t save_mode;

    if((vc >= 4) || (param_length > 200) || (params == NULL))
        return false;

    save_mode = hal_dsi_operation_mode_get();       // save mode
    hal_dsi_operation_mode_set(1);                  // in cmd mode

    while(reg_CMD_PKT_STATUS & ( 1 << 1))  LOGE("%s cmd full\n", __func__);

    if(param_length > 2)                            // long write
    {
        for(uint16_t i = 0; i < param_length/4; i++)
            reg_GEN_PLD_DATA = ((uint32_t*)params)[i];

        if(param_length%4)
            reg_GEN_PLD_DATA = ((uint32_t*)params)[param_length/4] & (0xFFFFFFFF >> (32 - (param_length%4)*8));
    }

    reg_GEN_HDR = (msb_byte <<  16) | (lsb_byte << 8 ) | ((vc << 6) | data_type);

    if(params[0] == 0x11)                           // sleep out delay
        rtos_delay_milliseconds(250);
    else if(params[0] == 0x29)                      // display on delay
        rtos_delay_milliseconds(50);
    else
        rtos_delay_milliseconds(1);
    
    hal_dsi_operation_mode_set(save_mode);          // save mode write back
    // LOGI("%s, cmd:%x, len:%d\n", __func__, lsb_byte, bytes_to_read);

    return true;
}

void hal_dsi_sys_clk_switch(uint8_t enable)
{
    uint32_t reg = REG_READ(SYS_AHBP_REGA_ADDR);

    reg &= ~(1 << 13);
    reg |= (!!enable) << 13;

    REG_WRITE(SYS_AHBP_REGA_ADDR, reg);
}

void hal_dsi_sys_ctrl_rst(void)
{
    reg_PHY_RSTZ = 0x0;
    rtos_delay_milliseconds(5);
    reg_PHY_RSTZ = 0xf;
}


