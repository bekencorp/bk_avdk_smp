#pragma once

#ifdef __cplusplus
extern "C" {
#endif



//====[INT BIT]============================================================================================
#define mipi_int1_te_err                      1<<20
#define mipi_int1_dpi_bpl_udflw               1<<19
#define mipi_int1_dbi_err0                    1<<17
#define mipi_int1_dbi_err1                    1<<16
#define mipi_int1_dbi_err2                    1<<15
#define mipi_int1_dbi_err3                    1<<14
#define mipi_int1_dbi_err4                    1<<13
#define mipi_int1_gen_pld_rd_full             1<<12
#define mipi_int1_gen_dcs_rd_empy             1<<11
#define mipi_int1_gen_pld_empy                1<<10
#define mipi_int1_gen_pld_wr_full             1<<9
#define mipi_int1_gen_cmd_wr_full             1<<8
#define mipi_int1_dpi_pld_full                1<<7
#define mipi_int1_eotp_rece_err               1<<6
#define mipi_int1_pkt_rece_err                1<<5
#define mipi_int1_crc_rece_err                1<<4
#define mipi_int1_ecc_rece_err0               1<<3
#define mipi_int1_ecc_rece_err1               1<<2
#define mipi_int1_lp_rece_tmout               1<<1
#define mipi_int1_hs_tran_tmour               1<<0

typedef enum  {
    DPHY_BR_100M  = 100,          // dsi byteclk 12.5Mhz
    DPHY_BR_200M  = 200,          // dsi byteclk 25Mhz
    DPHY_BR_300M  = 250,          // dsi byteclk 37.5Mhz
    DPHY_BR_400M  = 400,          // dsi byteclk 50Mhz
    DPHY_BR_440M  = 440,          // dsi byteclk 55Mhz
    DPHY_BR_500M  = 500,          // dsi byteclk 62.5Mhz
    DPHY_BR_600M  = 600,          // dsi byteclk 75Mhz
    DPHY_BR_700M  = 700,          // dsi byteclk 87.5Mhz
    DPHY_BR_800M  = 800,          // dsi byteclk 100Mhz
    DPHY_BR_1000M = 1000,         // dsi byteclk 125Mhz
    DPHY_BR_1200M = 1200,         // dsi byteclk 150Mhz
    DPHY_BR_1400M = 1400,         // dsi byteclk 175Mhz
    DPHY_BR_1500M = 1500,         // dsi byteclk 187.5Mhz
    DPHY_BR_1600M = 1600,         // dsi byteclk 200Mhz
}nn_dphy_bitrate;

/**
 * @brief Supported clock sources for modules (CPU, peripherals, RTC, etc.)
 *
 * @note enum starts from 1, to save 0 for special purpose
 */
typedef enum {
    // For digital domain: peripherals
    SOC_MOD_CLK_PLL_F20M,                      /*!< PLL_F20M_CLK is derived from SPLL (clock gating + "fixed" divider of 24), it has a fixed frequency of 20MHz */
    SOC_MOD_CLK_PLL_F25M,                      /*!< PLL_F25M_CLK is derived from MPLL (clock gating + configurable divider), it will have a frequency of 25MHz */
    SOC_MOD_CLK_RC_FAST,                       /*!< RC_FAST_CLK comes from the internal 20MHz rc oscillator, passing a clock gating to the peripherals */
    SOC_MOD_CLK_INVALID,                       /*!< Indication of the end of the available module clock sources */
    SOC_MOD_CLK_XTAL,                          /*!< XTAL_CLK comes from the external 40MHz crystal */
    SOC_MOD_CLK_PLL_F160M,                     /*!< PLL_F160M_CLK is derived from PLL (clock gating + fixed divider of 3), it has a fixed frequency of 160MHz */
    SOC_MOD_CLK_PLL_F240M,                     /*!< PLL_F240M_CLK is derived from PLL (clock gating + fixed divider of 2), it has a fixed frequency of 240MHz */

} soc_module_clk_t;


typedef enum
{
    DSI_ACTIVE_LANES_1 = 0,
    DSI_ACTIVE_LANES_2 = 1,
    DSI_ACTIVE_LANES_3 = 2,
    DSI_ACTIVE_LANES_4 = 3
} dsi_active_lanes_t;


/**
 * @brief Type of MIPI DSI PHY clock source
 */
typedef enum {
    MIPI_DSI_PHY_CLK_SRC_RC_FAST = SOC_MOD_CLK_RC_FAST,    /*!< Select RC_FAST as MIPI DSI PHY source clock */
    MIPI_DSI_PHY_CLK_SRC_PLL_F25M = SOC_MOD_CLK_PLL_F25M,  /*!< Select PLL_F25M as MIPI DSI PHY source clock */
    MIPI_DSI_PHY_CLK_SRC_PLL_F20M = SOC_MOD_CLK_PLL_F20M,  /*!< Select PLL_F20M as MIPI DSI PHY source clock */
    MIPI_DSI_PHY_CLK_SRC_DEFAULT = SOC_MOD_CLK_PLL_F20M,   /*!< Select PLL_F20M as default clock */
} soc_periph_mipi_dsi_phy_clk_src_t;
    
/**
 * @brief Type of MIPI DSI DPI clock source
 */
typedef enum {
    MIPI_DSI_DPI_CLK_SRC_XTAL = SOC_MOD_CLK_XTAL,            /*!< Select XTAL as MIPI DSI DPI source clock */
    MIPI_DSI_DPI_CLK_SRC_PLL_F160M = SOC_MOD_CLK_PLL_F160M,  /*!< Select PLL_F160M as MIPI DSI DPI source clock */
    MIPI_DSI_DPI_CLK_SRC_PLL_F240M = SOC_MOD_CLK_PLL_F240M,  /*!< Select PLL_F240M as MIPI DSI DPI source clock */
    MIPI_DSI_DPI_CLK_SRC_DEFAULT = SOC_MOD_CLK_PLL_F240M,    /*!< Select PLL_F240M as default clock */
} soc_periph_mipi_dsi_dpi_clk_src_t;


/**
 * @brief MIPI DSI PHY clock source
 */
typedef soc_periph_mipi_dsi_phy_clk_src_t mipi_dsi_phy_clock_source_t;

/**
 * @brief MIPI DSI DPI clock source
 */
typedef soc_periph_mipi_dsi_dpi_clk_src_t mipi_dsi_dpi_clock_source_t;

#ifdef __cplusplus
}
#endif

