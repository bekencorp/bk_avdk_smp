#include <os/os.h>
#include <os/mem.h>
#include <components/bk_display_types.h>
#include <components/bk_display_bus.h>
#include <avdk_check.h>
#include "display_dsi_bus_vn_ctlr.h"
#include <driver/mipi_dsi.h>
#include "sys_driver.h"
#define TAG "bk_dis_bus"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


static avdk_err_t bk_display_dsi_bus_enable(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    ///AVDK_RETURN_ON_FALSE(clock, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    dsi_bus_vn_ctlr_t *bus = __containerof(controller, dsi_bus_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, "bus is NULL");
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_MIPI_DSI, PM_POWER_MODULE_STATE_ON);

    AVDK_RETURN_ON_ERROR(mipi_dsi_bus_register(NULL, &bus->dsi_handle), (char *)TAG, "mipi dsi io enable err");

    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_dsi_bus_disable(bk_display_bus_ctlr_t *controller)
{

    return AVDK_ERR_OK;
}


avdk_err_t bk_display_dsi_read(bk_display_bus_ctlr_t *controller, bk_display_bus_rw_type_t type, uint32_t cmd, void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(controller, BK_ERR_NULL_PARAM, TAG, "invalid bus controller handle");
    dsi_bus_vn_ctlr_t *bus = __containerof(controller, dsi_bus_vn_ctlr_t, ops);
    bus->dsi_handle->rx_param(bus->dsi_handle, cmd, param, size);

    return AVDK_ERR_OK;
}

avdk_err_t bk_display_dsi_write(bk_display_bus_ctlr_t *controller, bk_display_bus_rw_type_t type, uint32_t cmd, const void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    dsi_bus_vn_ctlr_t *bus = __containerof(controller, dsi_bus_vn_ctlr_t, ops);
    bus->dsi_handle->tx_param(bus->dsi_handle, cmd, param, size);

    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_dsi_set_clock(bk_display_bus_ctlr_t *controller, bk_panel_clock_config_t *clock)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(clock, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    dsi_bus_vn_ctlr_t *bus = __containerof(controller, dsi_bus_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, "bus is NULL");

    bk_panel_clock_config_t dsi =
    {
        .n_lanes = clock->n_lanes,
        .fps = clock->fps,
        .timing = clock->timing,
    };

    AVDK_RETURN_ON_ERROR(mipi_dsi_clock_set(&dsi, &bus->dsi_handle), (char *)TAG, "mipi dsi io clock set err");

    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_dsi_bus_delete(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    dsi_bus_vn_ctlr_t *bus = __containerof(controller, dsi_bus_vn_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_INVAL, TAG, "bus is NULL");
    AVDK_RETURN_ON_ERROR(bus->dsi_handle->del(bus->dsi_handle), (char *)TAG, "mipi dsi io delete err\n");
    os_free(bus);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_MIPI_DSI, PM_POWER_MODULE_STATE_OFF);

    return AVDK_ERR_OK;
}


avdk_err_t bk_display_dsi_bus_new(bk_display_bus_handle_t *handle, bk_display_dsi_bus_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    //AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    dsi_bus_vn_ctlr_t *bus = os_malloc(sizeof(dsi_bus_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(bus, 0, sizeof(dsi_bus_vn_ctlr_t));

    bus->ops.enable = bk_display_dsi_bus_enable;
    bus->ops.disable = bk_display_dsi_bus_disable;
    bus->ops.read = bk_display_dsi_read;
    bus->ops.write = bk_display_dsi_write;
    bus->ops.set_clock = bk_display_dsi_set_clock;
    bus->ops.delete = bk_display_dsi_bus_delete;

    *handle = &(bus->ops);

    return AVDK_ERR_OK;
}