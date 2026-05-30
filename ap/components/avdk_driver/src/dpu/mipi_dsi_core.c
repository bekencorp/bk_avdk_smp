#include <common/bk_include.h>
#include <os/os.h>
#include <os/mem.h>
#include "sys_driver.h"
#include "mipi_dsi_host_reg.h"
#include <driver/mipi_dsi_types.h>
#include <driver/dpu_types.h>
#include <mipi_dsi_hal.h>
#include <driver/mipi_dsi.h>
#include <avdk_check.h>
#include "dpu_driver.h"

#define TAG "dsi_core"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


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

bk_err_t mipi_dsi_panel_set_pattern(mipi_dsi_pattern_type_t pattern)
{
    (void)pattern;
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

#if ((SFT_VERSION == FPGA_7259_CM55) || (SFT_VERSION == FPGA_7259_A35))
static inline uint16_t dsi_time_round(float t)
{
    if (t <= 0.f) {
        return 0;
    }
    t += 0.5f;
    return (t > 65535.f) ? 65535 : (uint16_t)t;
}
#endif

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

/**
 * Unified DSI PHY + host bring-up.
 *
 * The Naneng D-PHY register layout is the same regardless of where the
 * DPU register clock comes from; only the @c R5c value differs:
 *
 *   - First, try @c hal_dsi_dphy_init_for_panel(): build @c R5c so the
 *     PHY's internal divider emits DPI pclk == panel pclk exactly. This
 *     is the precise path used when @c dsi->clk_src is
 *     ::DPU_CLK_SRC_DPHY_DPLL (the default).
 *   - If the panel's required lane:pclk ratio exceeds the PHY's 4-bit
 *     @c pixdiv field (max = 17), the precise path returns BK_FAIL.
 *     We then transparently fall back to the SYSCLK ladder, pick a safe
 *     lane rate from the legacy lookup table, log a warning, and write
 *     ::DPU_CLK_SRC_SYSCLK back into @c dsi->clk_src so the caller's
 *     bus/panel/DPU state stay synchronised. A caller that explicitly
 *     passed ::DPU_CLK_SRC_SYSCLK takes the same SYSCLK path silently.
 *
 * VID_HSA / VID_HBP / VID_HLINE byte cycles are computed from the
 * achieved lane bitrate in both branches (no more float scaling).
 */
bk_err_t mipi_dsi_clock_set(bk_panel_clock_config_t *dsi)
{
    if (dsi == NULL) {
        return BK_ERR_NULL_PARAM;
    }

    uint64_t pclk_hz = mipi_dsi_panel_pclk_hz(dsi);
    if (pclk_hz == 0ULL) {
        LOGE("%s invalid pixel clock (fps/timing)\n", __func__);
        return BK_FAIL;
    }

    uint32_t lane_bitrate_mbps = 0u;
    mipi_dsi_host_vid_hparams_t hp;

#if ((SFT_VERSION == FPGA_7259_CM55) || (SFT_VERSION == FPGA_7259_A35))
    lane_bitrate_mbps = DPHY_BR_800M;
    mipi_dsi_host_vid_hparams_fpga(&dsi->timing, &hp);
    hal_dsi_wait_fpga_dphy_done();
#else
    /* DPHY_DPLL is the default when caller doesn't pin anything else. */
    if (dsi->clk_src != DPU_CLK_SRC_SYSCLK) {
        dsi->clk_src = DPU_CLK_SRC_DPHY_DPLL;
    }

    bool tried_dphy_dpll = (dsi->clk_src == DPU_CLK_SRC_DPHY_DPLL);
    bool dphy_dpll_ok = false;

    if (tried_dphy_dpll) {
        dphy_dpll_ok = (hal_dsi_dphy_init_for_panel(pclk_hz, dsi->n_lanes, 24u,
                                                   DSI_LINK_BANDWIDTH_OVERHEAD_PERMILLE,
                                                   &lane_bitrate_mbps) == BK_OK);
        if (!dphy_dpll_ok) {
            uint32_t lane_cnt   = (uint32_t)dsi->n_lanes + 1u;
            uint32_t need_ratio = (24u * 1300u + (lane_cnt * 1000u - 1u)) / (lane_cnt * 1000u);
            LOGW("%s PHY PLL miss for pclk=%llu Hz lanes=%u (need lane:pclk >= %u, "
                 "PHY pixdiv max = 17); auto-fallback to DPU_CLK_SRC_SYSCLK\n",
                 __func__, (unsigned long long)pclk_hz, (unsigned)lane_cnt,
                 (unsigned)need_ratio);
            dsi->clk_src = DPU_CLK_SRC_SYSCLK;
        }
    }

    if (!dphy_dpll_ok) {
        uint32_t clk_mhz = (uint32_t)((pclk_hz + 500000ULL) / 1000000ULL);
        uint32_t bitrate = dsi_dphy_bitrate_calc(clk_mhz, dsi->n_lanes);
        if (bitrate == 0u) {
            bitrate = DPHY_BR_800M;
            LOGW("%s no table entry for clk=%u MHz lanes=%u, defaulting to 800 Mbps\n",
                 __func__, (unsigned)clk_mhz, (unsigned)(dsi->n_lanes + 1U));
        }
        hal_dsi_dphy_init(bitrate);
        lane_bitrate_mbps = bitrate;
    }
    mipi_dsi_host_vid_hparams_asic(dsi, lane_bitrate_mbps, pclk_hz, &hp);
#endif

    LOGI("%s using %s n_lanes:%u lane:%u Mbps pclk:%llu Hz -> VID_HSA:%u VID_HBP:%u VID_HLINE:%u\n",
         __func__,
         (dsi->clk_src == DPU_CLK_SRC_DPHY_DPLL) ? "DPU_CLK_SRC_DPHY_DPLL" : "DPU_CLK_SRC_SYSCLK",
         (unsigned)(dsi->n_lanes + 1U),
         (unsigned)lane_bitrate_mbps, (unsigned long long)pclk_hz,
         (unsigned)hp.hsa, (unsigned)hp.hbp, (unsigned)hp.hline);

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

bk_err_t mipi_dsi_init(void)
{
    mipi_dsi_sys_init();
    return BK_OK;
}

bk_err_t mipi_dsi_deinit(void)
{
    /* dsi power down */
    hal_dsi_host_reset();
    /* dphy power down */
    hal_dsi_dphy_power_down();
    /* interrupt disable */
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_DSI, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_DSI, 0);
#endif
    bk_int_isr_unregister(INT_SRC_DSI);
    /* clk close */
    hal_dsi_sys_clk_switch(0);

    return BK_OK;
}
