#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include "sys_driver.h"
#include "mipi_dsi_host_reg.h"
#include <mipi_dsi_types.h>
#include <components/bk_lcd_types.h>
#include <components/bk_display_types.h>
#include <mipi_dsi_hal.h>
#include <driver/mipi_dsi.h>
#include <avdk_check.h>
#include "dpu_driver.h"

#define TAG "dsi_core"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct mipi_dsi_io_t mipi_dsi_io_t;

struct mipi_dsi_io_t {
    bk_lcd_bus_io_t base;         // Base class of generic lcd panel

    uint8_t virtual_channel;      // Virtual channel ID, index from 0
    int lcd_cmd_bits;             // Bit-width of LCD command
    int lcd_param_bits;           // Bit-width of LCD parameter
    void *user_ctx; // User context for the callback
};

static bk_err_t mipi_dsi_bus_io_del(bk_lcd_bus_io_t *io);
static bk_err_t mipi_dsi_bus_io_tx_param(bk_lcd_bus_io_t *io, int lcd_cmd, const void *param, uint16_t param_size);
static bk_err_t mipi_dsi_bus_io_rx_param(bk_lcd_bus_io_t *io, int lcd_cmd, void *param, uint16_t param_size);


/**
 * Send a DCS READ command to peripheral
 * function sets the packet data type automatically
 * @param vc destination virtual channel
 * @param cmd DCS code
 * @param bytes_to_read no of bytes to read (expected to arrive at buffer)
 * @param read_buffer pointer to 8-bit array to hold the read buffer words
 * return status
 * @note this function will enable BTA
 */
uint16_t mipi_dsi_dcs_read(uint8_t cmd, uint8_t bytes_to_read, uint8_t *read_buffer)
{
    return hal_dsi_gen_read_pkt(0, 0x06, 0x0, cmd, bytes_to_read, read_buffer);  /* COMMAND_TYPE 0x06 - DCS Read no params refer to DSI spec p.47 */
}

/**
 * Send Generic READ command to peripheral
 * - function sets the packet data type automatically
 * @param vc destination virtual channel
 * @param params byte array of command parameters
 * @param param_len length of the above array
 * @param bytes_to_read no of bytes to read (expected to arrive at buffer)
 * @param read_buffer pointer to 8-bit array to hold the read buffer words
 * return status
 * @note this function will enable BTA
 */
uint16_t mipi_dsi_gen_read(uint8_t *params, uint8_t param_len, uint8_t bytes_to_read, uint8_t *read_buffer)
{
    uint16_t status = false;

    if (param_len == 0)        // short read with 0 parameters
        status = hal_dsi_gen_read_pkt(0, 0x04, 0x00, 0x00, bytes_to_read, read_buffer);
    else if (param_len == 1)   // short read with 1 parameters
        status = hal_dsi_gen_read_pkt(0, 0x14, 0x00, params[0], bytes_to_read, read_buffer);
    else if (param_len == 2)   // short read with 2 parameters
        status = hal_dsi_gen_read_pkt(0, 0x24, params[1], params[0], bytes_to_read, read_buffer);
    else
        LOGI("%s param_len error:%d\n", __func__, param_len);

    return status;
}


uint16_t mipi_dsi_dcs_write(uint8_t data_len, const uint8_t *data)
{
    uint16_t status = false;

    if (data_len == 1)        // short write with 0 parameters
        status = hal_dsi_gen_write_pkt(0, 0x05, 0x00, data[0], data_len, data);
    else if (data_len == 2)   // short write with 1 parameters
        status = hal_dsi_gen_write_pkt(0, 0x15, data[1], data[0], data_len, data);
    else if (data_len == 3)   // short write with 2 parameters
        LOGI("%s param_len error:%d\n", __func__, data_len);
    else
        status = hal_dsi_gen_write_pkt(0, 0x39, (data_len >> 8) & 0xFF, data_len & 0xFF, data_len, data);

    return status;
}

uint16_t mipi_dsi_gen_write(uint16_t data_len, const uint8_t *data)
{
    uint16_t status = false;

    if (data_len == 0)        // short write with 0 parameters
        status = hal_dsi_gen_write_pkt(0, 0x03, 0x00, 0x00, data_len, data);
    else if (data_len == 1)   // short write with 1 parameters
        status = hal_dsi_gen_write_pkt(0, 0x13, 0x00, data[0], data_len, data);
    else if (data_len == 2)   // short write with 2 parameters
        status = hal_dsi_gen_write_pkt(0, 0x23, data[1], data[0], data_len, data);
    else
        status = hal_dsi_gen_write_pkt(0, 0x29, (data_len >> 8) & 0xFF, data_len & 0xFF, data_len, data);

    return status;
}

uint16_t mipi_dsi_gen_write_dcs_command(int lcd_cmd, const void *param, uint8_t param_size)
{
    uint16_t status = false;
    uint8_t cmd_data[param_size + 1];

    cmd_data[0] = lcd_cmd;

    if(param && param_size)
        os_memcpy(cmd_data + 1, param, param_size);

    if (param_size == 0)   // short write with 1 parameters
        status = hal_dsi_gen_write_pkt(0, 0x13, 0x00, lcd_cmd, 1, cmd_data);
    else if (param_size == 1)   // short write with 2 parameters
        status = hal_dsi_gen_write_pkt(0, 0x23, ((uint8_t *)param)[0], lcd_cmd, 2, cmd_data);
    else
        status = hal_dsi_gen_write_pkt(0, 0x29, ((param_size + 1) >> 8) & 0xFF, (param_size + 1) & 0xFF, param_size + 1, cmd_data);

    return status;
}

static void mipi_dsi_isr()
{
    uint32_t status0 = reg_INT_ST0;
    uint32_t status1 = reg_INT_ST1;

    if(status0)                           LOGI("DPHY ERR:%x\n", status0);

    if(status1 & mipi_int1_te_err         )  LOGI("DSI ERR te_err\n");
    if(status1 & mipi_int1_dpi_bpl_udflw  )  LOGI("DSI ERR dpi_bpl_udflw\n");
    if(status1 & mipi_int1_dbi_err0       )  LOGI("DSI ERR dbi err0\n");
    if(status1 & mipi_int1_dbi_err1       )  LOGI("DSI ERR dbi err1\n.");
    if(status1 & mipi_int1_dbi_err2       )  LOGI("DSI ERR dbi err2\n");
    if(status1 & mipi_int1_dbi_err3       )  LOGI("DSI ERR dbi err3\n");
    if(status1 & mipi_int1_dbi_err4       )  LOGI("DSI ERR dbi err4\n");
    if(status1 & mipi_int1_gen_pld_rd_full)  LOGI("DSI ERR gen_pld_rd_full\n");
    if(status1 & mipi_int1_gen_dcs_rd_empy)  LOGI("DSI ERR gen_dcs_rd_empy\n");
    if(status1 & mipi_int1_gen_pld_empy   )  LOGI("DSI ERR gen_pld_empy\n");
    if(status1 & mipi_int1_gen_pld_wr_full)  LOGI("DSI ERR gen_pld_wr_full\n");
    if(status1 & mipi_int1_gen_cmd_wr_full)  LOGI("DSI ERR gen_cmd_wr_full\n");
    if(status1 & mipi_int1_dpi_pld_full   )  LOGI("DSI ERR dpi_pld_full\n");
    if(status1 & mipi_int1_eotp_rece_err  )  LOGI("DSI ERR eotp_rece_err\n");
    if(status1 & mipi_int1_pkt_rece_err   )  LOGI("DSI ERR pkt_rece_err\n");
    if(status1 & mipi_int1_crc_rece_err   )  LOGI("DSI ERR crc_rece_err\n");
    if(status1 & mipi_int1_ecc_rece_err0  )  LOGI("DSI ERR ecc_rece_err0\n");
    if(status1 & mipi_int1_ecc_rece_err1  )  LOGI("DSI ERR ecc_rece_err1\n");
    if(status1 & mipi_int1_lp_rece_tmout  )  LOGI("DSI ERR lp_rece_tmout\n");
    if(status1 & mipi_int1_hs_tran_tmour  )  LOGI("DSI ERR lp_rece_tmout\n");
}

void mipi_dsi_interrupt_init(void)
{
    bk_int_isr_register(INT_SRC_DSI, (int_group_isr_t)mipi_dsi_isr, NULL);
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_DSI, 1);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_DSI, 1);
#endif
}

void mipi_dsi_sys_init(void)
{
#if((SFT_VERSION == FPGA_7259_CM55) || (SFT_VERSION == FPGA_7259_A35))
    reg_PHY_RSTZ = 0xf;
#endif
    // mipi_dsi interrupt init
    mipi_dsi_interrupt_init();
    hal_dsi_sys_clk_switch(1);
}



bk_err_t mipi_dsi_panel_set_pattern(bk_lcd_bus_io_t *panel, mipi_dsi_pattern_type_t pattern)
{
    AVDK_RETURN_ON_FALSE(panel, BK_ERR_NULL_PARAM, TAG, "invalid argument");
    //mipi_dsi_io_t *dsi_panel = __containerof(panel, mipi_dsi_io_t, base);

    reg_VID_MODE_CFG |= (0x0<<20);  // vpg mode: 0:colorbar, 1:berpattern
    reg_VID_MODE_CFG |= (0x1<<16);  // vpg_en

    return BK_OK;
}

/*
 * DWC MIPI DSI Host video mode: VID_HSA_TIME, VID_HBP_TIME, VID_HLINE_TIME are in units of
 * lane byte clock (txbyteclkhs), i.e. DPI pixel intervals scaled by (F_byte_hs / F_pclk),
 * F_byte_hs = (per-lane HS bit rate) / 8.
 */
#define DSI_VID_HBLANK_OVERHEAD_PERMILLE  300u
#define DSI_VID_HLINE_OVERHEAD_PERMILLE   300u
#define DSI_LINK_BANDWIDTH_OVERHEAD_PERMILLE  DSI_VID_HBLANK_OVERHEAD_PERMILLE

static inline uint16_t dsi_time_round(float t)
{
    if (t <= 0.f) {
        return 0;
    }
    t += 0.5f;
    return (t > 65535.f) ? 65535 : (uint16_t)t;
}

static uint64_t mipi_dsi_panel_pclk_hz(const bk_panel_clock_config_t *dsi)
{
    if (dsi->fps != 0U) {
        uint32_t h_total = (uint32_t)dsi->timing.h_size + (uint32_t)dsi->timing.hsync_pulse_width
            + (uint32_t)dsi->timing.hsync_back_porch + (uint32_t)dsi->timing.hsync_front_porch;
        uint32_t v_total = (uint32_t)dsi->timing.v_size + (uint32_t)dsi->timing.vsync_pulse_width
            + (uint32_t)dsi->timing.vsync_back_porch + (uint32_t)dsi->timing.vsync_front_porch;
        LOGI("h_total:%u v_total:%u fps:%u\n", h_total, v_total, dsi->fps);
        return (uint64_t)h_total * (uint64_t)v_total * (uint64_t)dsi->fps;
    }

    return 60000000ULL;
}

static uint16_t dsi_host_vid_byte_cycles(uint32_t dpi_pixels, uint32_t lane_bitrate_mbps,
                                         uint64_t pclk_hz, uint32_t overhead_permille)
{
    if (dpi_pixels == 0U || lane_bitrate_mbps == 0U || pclk_hz == 0ULL) {
        return 0;
    }

    uint64_t num = (uint64_t)dpi_pixels * (uint64_t)lane_bitrate_mbps * 1000000ULL
        * (1000ULL + (uint64_t)overhead_permille);
    uint64_t den = 8ULL * pclk_hz * 1000ULL;
    uint64_t q = (num + den / 2ULL) / den;

    return (q > 65535ULL) ? 65535U : (uint16_t)q;
}

typedef struct {
    uint16_t hsa;
    uint16_t hbp;
    uint16_t hline;
} mipi_dsi_host_vid_hparams_t;

#if ((SFT_VERSION == FPGA_7259_CM55) || (SFT_VERSION == FPGA_7259_A35))
static void mipi_dsi_host_vid_hparams_fpga(const bk_display_timing_t *t, mipi_dsi_host_vid_hparams_t *out)
{
    const float byte_hs_per_pclk = (float)DPHY_BR_800M / (8.f * 15.f);

    out->hsa = dsi_time_round((float)t->hsync_pulse_width * byte_hs_per_pclk * 1.0f);
    out->hbp = dsi_time_round((float)t->hsync_back_porch * byte_hs_per_pclk * 1.0f);
    out->hline = dsi_time_round(((float)t->hsync_pulse_width + (float)t->hsync_back_porch
                                 + (float)t->hsync_front_porch + (float)t->h_size) * byte_hs_per_pclk * 1.1f);
}
#else
static void mipi_dsi_host_vid_hparams_asic(const bk_panel_clock_config_t *dsi, uint32_t lane_bitrate_mbps,
                                           uint64_t pclk_hz, mipi_dsi_host_vid_hparams_t *out)
{
    const bk_display_timing_t *t = &dsi->timing;

    out->hsa = dsi_host_vid_byte_cycles(t->hsync_pulse_width, lane_bitrate_mbps, pclk_hz,
                                        DSI_VID_HBLANK_OVERHEAD_PERMILLE);
    out->hbp = dsi_host_vid_byte_cycles(t->hsync_back_porch, lane_bitrate_mbps, pclk_hz,
                                        DSI_VID_HBLANK_OVERHEAD_PERMILLE);

    uint32_t h_total_px = (uint32_t)t->hsync_pulse_width + (uint32_t)t->hsync_back_porch
        + (uint32_t)t->hsync_front_porch + (uint32_t)t->h_size;
    out->hline = dsi_host_vid_byte_cycles(h_total_px, lane_bitrate_mbps, pclk_hz,
                                          DSI_VID_HLINE_OVERHEAD_PERMILLE);
}
#endif

/** Naneng internal PLL path: hal_dsi_dphy_init_for_panel + byte-accurate VID timing (default for UNKNOWN / NANENG). */
static bk_err_t mipi_dsi_clock_set_internal_pll_path(bk_panel_clock_config_t *dsi, bk_lcd_bus_io_t **ret_panel)
{
    (void)ret_panel;

    uint32_t lane_bitrate_mbps;
    mipi_dsi_host_vid_hparams_t hp;
    uint64_t pclk_hz = mipi_dsi_panel_pclk_hz(dsi);

    if (pclk_hz == 0ULL) {
        LOGE("%s invalid pixel clock (clk/fps/timing)\n", __func__);
        return BK_FAIL;
    }

#if ((SFT_VERSION == FPGA_7259_CM55) || (SFT_VERSION == FPGA_7259_A35))
    lane_bitrate_mbps = DPHY_BR_800M;
    mipi_dsi_host_vid_hparams_fpga(&dsi->timing, &hp);
#else
    if (hal_dsi_dphy_init_for_panel(pclk_hz, dsi->n_lanes, 24u, DSI_LINK_BANDWIDTH_OVERHEAD_PERMILLE,
                                    &lane_bitrate_mbps) != BK_OK) {
        LOGE("%s hal_dsi_dphy_init_for_panel failed\n", __func__);
        return BK_FAIL;
    }
    mipi_dsi_host_vid_hparams_asic(dsi, lane_bitrate_mbps, pclk_hz, &hp);
#endif

    LOGI("%s (internal_pll) n_lanes:%u lane:%u Mbps pclk:%llu Hz -> VID_HSA:%u VID_HBP:%u VID_HLINE:%u\n", __func__,
         (unsigned)(dsi->n_lanes + 1U), (unsigned)lane_bitrate_mbps, (unsigned long long)pclk_hz,
         (unsigned)hp.hsa, (unsigned)hp.hbp, (unsigned)hp.hline);

#if ((SFT_VERSION == FPGA_7259_CM55) || (SFT_VERSION == FPGA_7259_A35))
    hal_dsi_wait_fpga_dphy_done();
#endif

    hal_dsi_config(dsi->n_lanes,
                   dsi->timing.h_size,
                   dsi->timing.v_size,
                   hp.hsa,
                   hp.hbp,
                   hp.hline,
                   dsi->timing.vsync_pulse_width,
                   dsi->timing.vsync_back_porch,
                   dsi->timing.vsync_front_porch);

    hal_dsi_operation_mode_set(0);
    return BK_OK;
}

/** Legacy 320M/480M root: dsi_dphy_bitrate_calc + hal_dsi_dphy_init + float-scaled VID timing. */
static bk_err_t mipi_dsi_clock_set_legacy_ext_dphy_path(bk_panel_clock_config_t *dsi, bk_lcd_bus_io_t **ret_panel)
{
    (void)ret_panel;

    float hsa_time = 0;
    float hbp_time = 0;
    float hline_time = 0;
    float scale1 = 1.3f;
    float scale2 = 1.3f;
    float clk_coefficient = 0;
    uint32_t bitrate = 0;

    bitrate = dsi_dphy_bitrate_calc(dsi->clk, dsi->n_lanes);
    clk_coefficient = ((float)bitrate) / (8.f * ((float)dsi->clk));

#if ((SFT_VERSION == FPGA_7259_CM55) || (SFT_VERSION == FPGA_7259_A35))
    clk_coefficient = ((float)DPHY_BR_800M) / (8.f * ((float)15.0f));
    scale1 = 1.0f;
    scale2 = 1.1f;
#endif

    hsa_time   = (float)dsi->timing.hsync_pulse_width * clk_coefficient * scale1;
    hbp_time   = (float)dsi->timing.hsync_back_porch * clk_coefficient * scale1;
    hline_time = ((float)dsi->timing.hsync_pulse_width + (float)dsi->timing.hsync_back_porch
                  + (float)dsi->timing.hsync_front_porch + (float)dsi->timing.h_size) * clk_coefficient * scale2;

    LOGI("%s (320m_480m) bitrate:%u clk_coefficient:%.2f n_lanes:%u hsa:%.2f hbp:%.2f hline:%.2f\n", __func__,
         (unsigned)bitrate, clk_coefficient, (unsigned)(dsi->n_lanes + 1U), hsa_time, hbp_time, hline_time);

#if ((SFT_VERSION == FPGA_7259_CM55) || (SFT_VERSION == FPGA_7259_A35))
    hal_dsi_wait_fpga_dphy_done();
#else
    hal_dsi_dphy_init(bitrate);
#endif

    hal_dsi_config(dsi->n_lanes,
                   dsi->timing.h_size,
                   dsi->timing.v_size,
                   (uint16_t)hsa_time,
                   (uint16_t)hbp_time,
                   (uint16_t)hline_time,
                   dsi->timing.vsync_pulse_width,
                   dsi->timing.vsync_back_porch,
                   dsi->timing.vsync_front_porch);

    hal_dsi_operation_mode_set(0);
    return BK_OK;
}

bk_err_t mipi_dsi_clock_set(bk_panel_clock_config_t *dsi, bk_lcd_bus_io_t **ret_panel)
{
    if (dsi == NULL) {
        return BK_ERR_NULL_PARAM;
    }

    if (dsi->clk_src == DPU_CLK_SRC_320M_480M) {
        return mipi_dsi_clock_set_legacy_ext_dphy_path(dsi, ret_panel);
    }

    /* DPU_CLK_SRC_UNKNOWN (default) and DPU_CLK_SRC_NANENG_DPHY_INTERNAL_DPLL */
    return mipi_dsi_clock_set_internal_pll_path(dsi, ret_panel);
}

bk_err_t mipi_dsi_bus_register(bk_panel_clock_config_t *dsi, bk_lcd_bus_io_t **ret_panel)   // dsi_init need dpu clk enable before
{
    bk_err_t ret = BK_OK;

    mipi_dsi_io_t *dsi_panel = NULL;
    dsi_panel = (mipi_dsi_io_t *)os_malloc(sizeof(mipi_dsi_io_t));
    AVDK_GOTO_ON_FALSE(dsi_panel, BK_ERR_NO_MEM, err, TAG, "no memory for DSI panel");
    os_memset(dsi_panel, 0, sizeof(mipi_dsi_io_t));

    // mipi-sys-init
    mipi_dsi_sys_init();

    dsi_panel->base.del = mipi_dsi_bus_io_del;
    dsi_panel->base.tx_param = mipi_dsi_bus_io_tx_param;
    dsi_panel->base.rx_param = mipi_dsi_bus_io_rx_param;
    *ret_panel = &dsi_panel->base;
    return ret;

err:
    LOGE("%s error\n: ", __func__);
    return BK_FAIL;
}

static bk_err_t mipi_dsi_bus_io_del(bk_lcd_bus_io_t *panel)
{
    mipi_dsi_io_t *dsi_panel = __containerof(panel, mipi_dsi_io_t, base);
	
    /* dsi power down */
    hal_dsi_host_reset();
    /* dphy power down */
    hal_dsi_dphy_power_down();
    /* interrupt disable*/
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_DSI, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_DSI, 0);
#endif
    bk_int_isr_unregister(INT_SRC_DSI);
    /* clk close */
    hal_dsi_sys_clk_switch(0);

    os_free(dsi_panel);

    return BK_OK;
}

static bk_err_t mipi_dsi_bus_io_tx_param(bk_lcd_bus_io_t *io, int lcd_cmd, const void *param, uint16_t param_size)
{
    AVDK_RETURN_ON_FALSE(io, BK_ERR_NULL_PARAM, TAG, "invalid argument");

    //mipi_dsi_io_t *dsi_io = __containerof(io, mipi_dsi_io_t, base);

    mipi_dsi_gen_write_dcs_command(lcd_cmd, param, param_size);

    return BK_OK;
}
static bk_err_t mipi_dsi_bus_io_rx_param(bk_lcd_bus_io_t *io, int lcd_cmd, void *param, uint16_t param_size)
{
    AVDK_RETURN_ON_FALSE(io, BK_ERR_NULL_PARAM, TAG, "invalid argument");

    //mipi_dsi_io_t *dsi_io = __containerof(io, mipi_dsi_io_t, base);

    mipi_dsi_dcs_read(lcd_cmd,  param_size, param);

    return BK_OK;
}
