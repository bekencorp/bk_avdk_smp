// Copyright 2020-2021 Beken
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

#include <os/os.h>
#include <os/mem.h>
#include <stdint.h>
#include <common/bk_err.h>
#include "dpu_core.h"
#include <components/bk_display_dpu_ctlr.h>
#include "display_dpu_vn_ctlr.h"
#include "avdk_monitor.h"
#include "driver/sys_pm.h"
#include "sys_types.h"
#include "sys_driver.h"
#define TAG "bk_dpu"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static dpu_vn_ctlr_t *dpu_ctlr_from_handle(bk_display_ctlr_handle_t handle)
{
    return __containerof(handle, dpu_vn_ctlr_t, ops);
}

static void dpu_ctlr_build_core_config(const bk_display_dpu_config_t *config, dpu_config_t *dpu_config)
{
    os_memset(dpu_config, 0, sizeof(*dpu_config));

    dpu_config->dpu_clk_src = config->clk_src;
    dpu_config->dpi_clock_freq_mhz = config->timing.clk;
    dpu_config->video_timing = config->timing;
    dpu_config->video = config->video;
    dpu_config->graphic.enable = false;
}

static avdk_err_t dpu_ctlr_init(bk_display_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    dpu_vn_ctlr_t *control = dpu_ctlr_from_handle(handle);
    dpu_config_t dpu_config = {0};

    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    if (control->state == DISP_STATE_READY)
        return AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(control->state == DISP_STATE_DEINIT, AVDK_ERR_GENERIC, TAG, "display state invalid");

    dpu_ctlr_build_core_config(&control->config, &dpu_config);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_DPU, PM_POWER_MODULE_STATE_ON);
    sys_drv_set_psram_dpu_qos(3);

    ret = dpu_core_init(&dpu_config, &control->dpu_handle);
    AVDK_GOTO_ON_ERROR(ret, err, TAG, "dpu core init err");

    ret = dpu_core_layer_config(&dpu_config, &control->dpu_handle);
    AVDK_GOTO_ON_ERROR(ret, init_deinit, TAG, "dpu core layer config err");

    AVDK_MONITOR_DPU_ENABLE();
    control->state = DISP_STATE_READY;
    LOGI("%s complete\n", __func__);
    return AVDK_ERR_OK;

init_deinit:
    (void)dpu_core_deinit(&control->dpu_handle);
err:
    control->dpu_handle = NULL;
    control->state = DISP_STATE_DEINIT;
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_DPU, PM_POWER_MODULE_STATE_OFF);
    return ret;
}

static avdk_err_t dpu_ctlr_deinit(bk_display_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    dpu_vn_ctlr_t *controller = dpu_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL\n");

    if (controller->state == DISP_STATE_DEINIT)
        return AVDK_ERR_OK;

    ret = dpu_core_deinit(&controller->dpu_handle);
    AVDK_GOTO_ON_ERROR(ret, err, (char *)TAG, "dpu core deinit err\n");

    controller->dpu_handle = NULL;
    controller->state = DISP_STATE_DEINIT;
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_DPU, PM_POWER_MODULE_STATE_OFF);
    LOGI("%s complete\n", __func__);

    return BK_OK;
err:
    return ret;
}

static avdk_err_t dpu_ctlr_open(bk_display_ctlr_handle_t handle)
{
    dpu_vn_ctlr_t *controller = dpu_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    return AVDK_ERR_OK;
}

static avdk_err_t dpu_ctlr_close(bk_display_ctlr_handle_t handle)
{
    dpu_vn_ctlr_t *controller = dpu_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    return AVDK_ERR_OK;
}

static avdk_err_t dpu_ctlr_del(bk_display_ctlr_handle_t handle)
{
    dpu_vn_ctlr_t *control = dpu_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->state != DISP_STATE_DEINIT)
        (void)dpu_ctlr_deinit(handle);

    os_free(control);
    return AVDK_ERR_OK;
}

avdk_err_t dpu_ctlr_flush(bk_display_ctlr_handle_t handle, uint8_t *frame, flush_free_cb_t cb)
{
    dpu_vn_ctlr_t *control = dpu_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(control->state == DISP_STATE_READY, AVDK_ERR_GENERIC, TAG, "display is not ready");
    return dpu_core_flush(&control->dpu_handle, DPU_LAYER_VIDEO, frame, cb);
}



static avdk_err_t dpu_ctlr_ioctl(bk_display_ctlr_handle_t handle, bk_display_ioctl_cmd_t cmd, void *arg)
{
    dpu_vn_ctlr_t *control = dpu_ctlr_from_handle(handle);
    bk_display_pixel_format_config_t *runtime_config = (bk_display_pixel_format_config_t *)arg;
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(control->state == DISP_STATE_READY, AVDK_ERR_GENERIC, TAG, "display is not ready");

    switch (cmd)
    {
        case BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT:
            AVDK_RETURN_ON_FALSE(runtime_config, AVDK_ERR_INVAL, TAG, "pixel format arg is NULL");
            ret = dpu_core_runtime_switch(&control->dpu_handle, runtime_config);
            if (ret == AVDK_ERR_OK)
            {
                control->config.video.format = runtime_config->format;
                control->config.video.decompress = runtime_config->decompress;
                LOGI("DPU runtime switch format=%d decompress=%d\n",
                     runtime_config->format, runtime_config->decompress);
            }
        break;
        default:
            LOGE("unsupported ioctl cmd: %d\n", (int)cmd);
            ret = AVDK_ERR_UNSUPPORTED;
        break;
    }
    return ret;
}

avdk_err_t bk_display_dpu_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_dpu_config_t *config)
{
    AVDK_RETURN_ON_FALSE(config && handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    dpu_vn_ctlr_t *controller = os_malloc(sizeof(dpu_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(dpu_vn_ctlr_t));
    controller->state = DISP_STATE_DEINIT;

    os_memcpy(&controller->config, config, sizeof(bk_display_dpu_config_t));

    controller->ops.init = dpu_ctlr_init;
    controller->ops.open = dpu_ctlr_open;
    controller->ops.close = dpu_ctlr_close;
    controller->ops.flush = dpu_ctlr_flush;
    controller->ops.deinit = dpu_ctlr_deinit;
    controller->ops.del = dpu_ctlr_del;
    controller->ops.ioctl = dpu_ctlr_ioctl;
    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}

