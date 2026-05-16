// MIPI-DSI bus backend.
//
// Implements ::bk_display_bus_ctlr_t on top of the driver-layer
// mipi_dsi primitives. bk_display_dsi_bus_new() performs the full
// bring-up: MIPI sub-domain power vote, SoC D-PHY analog rail vote,
// mipi_dsi_init(). The DCS generic-write / read primitives are wired
// directly onto bus->ops.tx_param / rx_param - no separate panel-IO
// object is needed because the DSI host owns a single command channel.

#include <os/os.h>
#include <os/mem.h>
#include <avdk_check.h>
#include <components/log.h>
#include <components/bk_display.h>
#include <driver/mipi_dsi.h>
#include <modules/pm.h>

#include "display_dsi_bus_vn_ctlr.h"

#define TAG "bk_dis_bus"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

/*
 * BK7259 MIPI D-PHY analog domain power.
 *
 * The D-PHY analog block on this SoC is fed by two PMU AUXLDO rails
 * (2.8V + 3V) routed internally; they are chip-fixed, board-invariant
 * and never exposed to applications. Board-level rails such as panel
 * VDDIO and backlight supplies belong to the application/board layer
 * (e.g. doorbell's app_display.c), not to this SoC-internal helper.
 *
 * Kept as file-static helpers (rather than inlined four times) so the
 * delete() / error rollback paths read the same vote symmetrically.
 */
static void dsi_phy_analog_power_on(void)
{
    pm_auxldo_ctrl_cfg_t cfg;

    cfg = (pm_auxldo_ctrl_cfg_t){0};
    cfg.ldo   = AUXLDOS_SEL_2P8V;
    cfg.out   = PM_AUXLDO_2P8V_OUT_2P8V;
    cfg.user  = PM_AUXLDO_USER_DISPLAY;
    cfg.state = PM_AUXLDO_ENABLE;
    if (bk_pm_auxldo_ctrl_vote(&cfg) != BK_OK) {
        LOGE("dsi 2p8v ldo on err");
    }

    cfg = (pm_auxldo_ctrl_cfg_t){0};
    cfg.ldo   = AUXLDOS_SEL_3V;
    cfg.out   = PM_AUXLDO_3V_OUT_2P8V;
    cfg.user  = PM_AUXLDO_USER_DISPLAY;
    cfg.state = PM_AUXLDO_ENABLE;
    if (bk_pm_auxldo_ctrl_vote(&cfg) != BK_OK) {
        LOGE("dsi 3v ldo on err");
    }
}

static void dsi_phy_analog_power_off(void)
{
    pm_auxldo_ctrl_cfg_t cfg;

    cfg = (pm_auxldo_ctrl_cfg_t){0};
    cfg.ldo   = AUXLDOS_SEL_2P8V;
    cfg.out   = PM_AUXLDO_2P8V_OUT_2P8V;
    cfg.user  = PM_AUXLDO_USER_DISPLAY;
    cfg.state = PM_AUXLDO_DISABLE;
    if (bk_pm_auxldo_ctrl_vote(&cfg) != BK_OK) {
        LOGW("dsi 2p8v ldo off err");
    }

    cfg = (pm_auxldo_ctrl_cfg_t){0};
    cfg.ldo   = AUXLDOS_SEL_3V;
    cfg.out   = PM_AUXLDO_3V_OUT_2P8V;
    cfg.user  = PM_AUXLDO_USER_DISPLAY;
    cfg.state = PM_AUXLDO_DISABLE;
    if (bk_pm_auxldo_ctrl_vote(&cfg) != BK_OK) {
        LOGW("dsi 3v ldo off err");
    }
}

/*
 * DCS command channel directly on the bus controller.
 *
 * The DSI host has a single hardware command channel, so there is no
 * need for per-instance command-channel state - the ops are stateless
 * thin wrappers around the driver-layer mipi_dsi_* primitives.
 * mipi_dsi_init/deinit() handles the real bring-up / tear-down.
 */
static bk_err_t dsi_tx_param(bk_display_bus_ctlr_t *controller, int lcd_cmd,
                             const void *param, uint16_t param_size)
{
    (void)controller;
    mipi_dsi_gen_write_dcs_command(lcd_cmd, param, (uint8_t)param_size);
    return BK_OK;
}

static bk_err_t dsi_rx_param(bk_display_bus_ctlr_t *controller, int lcd_cmd,
                             void *param, uint16_t param_size)
{
    (void)controller;
    mipi_dsi_dcs_read((uint8_t)lcd_cmd, (uint8_t)param_size, param);
    return BK_OK;
}

static avdk_err_t bk_display_dsi_set_clock(bk_display_bus_ctlr_t *controller, bk_panel_clock_config_t *clock)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(clock, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    dsi_bus_vn_ctlr_t *bus = __containerof(controller, dsi_bus_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, "bus is NULL");

    bk_panel_clock_config_t dsi =
    {
        .clk = clock->clk,
        .n_lanes = clock->n_lanes,
        .fps = clock->fps,
        .clk_src = bus->dsi_clk_src,
        .timing = clock->timing,
    };

    AVDK_RETURN_ON_ERROR(mipi_dsi_clock_set(&dsi), (char *)TAG, "mipi dsi clock set err");

    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_dsi_set_clock_src(bk_display_bus_ctlr_t *controller, dpu_clk_src_t clk_src)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    dsi_bus_vn_ctlr_t *bus = __containerof(controller, dsi_bus_vn_ctlr_t, ops);
    bus->dsi_clk_src = clk_src;
    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_dsi_bus_delete(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    dsi_bus_vn_ctlr_t *bus = __containerof(controller, dsi_bus_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, "bus is NULL");

    dsi_phy_analog_power_off();
    AVDK_RETURN_ON_ERROR(mipi_dsi_deinit(), (char *)TAG, "mipi dsi deinit err");
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_MIPI_DSI, PM_POWER_MODULE_STATE_OFF);

    os_free(bus);
    return AVDK_ERR_OK;
}

avdk_err_t bk_display_dsi_bus_new(bk_display_bus_handle_t *handle, bk_display_dsi_bus_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    dsi_bus_vn_ctlr_t *bus = os_malloc(sizeof(dsi_bus_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(bus, 0, sizeof(dsi_bus_vn_ctlr_t));
    (void)config;  /* reserved for future per-board options */
    /* dsi_clk_src starts as DPU_CLK_SRC_UNKNOWN (=0); the DPU pushes the
     * real source through set_clock_src() before the first set_clock(). */

    bus->ops.set_clock = bk_display_dsi_set_clock;
    bus->ops.set_clock_src = bk_display_dsi_set_clock_src;
    bus->ops.delete = bk_display_dsi_bus_delete;
    bus->ops.tx_param = dsi_tx_param;
    bus->ops.rx_param = dsi_rx_param;

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_MIPI_DSI, PM_POWER_MODULE_STATE_ON);
    dsi_phy_analog_power_on();

    if (mipi_dsi_init() != BK_OK) {
        LOGE("mipi dsi init failed\n");
        dsi_phy_analog_power_off();
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_MIPI_DSI, PM_POWER_MODULE_STATE_OFF);
        os_free(bus);
        return AVDK_ERR_GENERIC;
    }

    *handle = &(bus->ops);
    return AVDK_ERR_OK;
}
