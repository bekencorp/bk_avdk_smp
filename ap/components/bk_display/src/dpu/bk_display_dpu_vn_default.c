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

// Default DPU virtual node controller (::bk_display_ctlr_t backend).
//
// Owns the lifecycle state machine:
//
//   DEINIT --init--> INITED --open--> ACTIVE --flush--> ACTIVE
//      ^               |  ^             |
//      |               |  +---close-----+
//      +-deinit--------+
//
// open() is idempotent and may be triggered implicitly by flush()
// (lazy promotion via flush_lazy_promoted) so callers that just want
// to push frames need not pair init/open manually.

#include <os/os.h>
#include <os/mem.h>
#include <stdint.h>
#include <common/bk_err.h>
#include <avdk_check.h>
#include "dpu_core.h"
#include <components/bk_display.h>
#include "display_dpu_vn_ctlr.h"
#include "bk_lcd_panel_priv.h"   /* bk_avdk_lcd_panel_t layout (bus, timing, pixel_clock_hz, clk_src) */
#include "avdk_monitor.h"
#include "driver/sys_pm.h"
#include "sys_types.h"
#include "sys_driver.h"
#define TAG "bk_dpu"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static dpu_vn_ctlr_t *dpu_ctlr_from_handle(bk_display_ctlr_handle_t handle)
{
    return __containerof(handle, dpu_vn_ctlr_t, ops);
}

static avdk_err_t dpu_ctlr_lock(dpu_vn_ctlr_t *control)
{
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(control->lock, AVDK_ERR_GENERIC, TAG, "controller lock is NULL");

    bk_err_t ret = rtos_lock_mutex(&control->lock);
    if (ret != BK_OK)
    {
        LOGE("%s lock failed: %d\n", __func__, ret);
        return AVDK_ERR_GENERIC;
    }

    return AVDK_ERR_OK;
}

static void dpu_ctlr_unlock(dpu_vn_ctlr_t *control)
{
    if ((control != NULL) && (control->lock != NULL))
    {
        (void)rtos_unlock_mutex(&control->lock);
    }
}

static void dpu_ctlr_flush_exit(dpu_vn_ctlr_t *control)
{
    if (control == NULL)
    {
        return;
    }

    if (control->inflight_flush > 0)
    {
        control->inflight_flush--;
    }

    if ((control->state == DISP_STATE_DEINITING) && (control->inflight_flush == 0) && (control->flush_idle_sem != NULL))
    {
        (void)rtos_set_semaphore(&control->flush_idle_sem);
    }
}

static avdk_err_t dpu_ctlr_wait_flush_idle(dpu_vn_ctlr_t *control)
{
    avdk_err_t ret = AVDK_ERR_OK;

    while ((control != NULL) && (control->inflight_flush > 0))
    {
        dpu_ctlr_unlock(control);
        if (rtos_get_semaphore(&control->flush_idle_sem, BEKEN_WAIT_FOREVER) != BK_OK)
        {
            LOGE("%s wait flush idle failed\n", __func__);
            return AVDK_ERR_GENERIC;
        }

        ret = dpu_ctlr_lock(control);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s relock failed: %d\n", __func__, ret);
            return ret;
        }
    }

    return ret;
}

static void dpu_ctlr_build_core_config(const dpu_vn_ctlr_t *control, dpu_config_t *dpu_config)
{
    os_memset(dpu_config, 0, sizeof(*dpu_config));

    dpu_config->video          = control->config.video;
    dpu_config->graphic.enable = false;
    dpu_config->dpu_clk_src    = control->panel->clk_src;
    dpu_config->video_timing   = control->panel->timing;
    dpu_config->pixel_clock_hz = control->panel->pixel_clock_hz;
}

static avdk_err_t dpu_ctlr_init(bk_display_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    dpu_vn_ctlr_t *control = dpu_ctlr_from_handle(handle);
    dpu_config_t dpu_config = {0};

    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_GOTO_ON_ERROR(dpu_ctlr_lock(control), lock_err, TAG, "controller lock failed");
    if ((control->state == DISP_STATE_INITED) ||
        (control->state == DISP_STATE_ACTIVE) ||
        (control->state == DISP_STATE_CLOSED))
    {
        dpu_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if (control->state != DISP_STATE_DEINIT)
    {
        dpu_ctlr_unlock(control);
        LOGE("%s invalid display state: %d\n", __func__, control->state);
        return AVDK_ERR_GENERIC;
    }
    dpu_ctlr_unlock(control);

    dpu_ctlr_build_core_config(control, &dpu_config);

    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_DPU, PM_POWER_MODULE_STATE_ON);

    sys_drv_set_psram_dpu_qos(BK_DISPLAY_DPU_QOS_DEFAULT);

    ret = dpu_core_init(&dpu_config, &control->dpu_handle);
    AVDK_GOTO_ON_ERROR(ret, err, TAG, "dpu core init err");

    ret = dpu_core_layer_config(&dpu_config, &control->dpu_handle);
    AVDK_GOTO_ON_ERROR(ret, init_deinit, TAG, "dpu core layer config err");

    (void)dpu_ctlr_lock(control);
    control->state = DISP_STATE_INITED;
    dpu_ctlr_unlock(control);
    LOGI("%s complete (INITED, awaiting open())\n", __func__);
    return AVDK_ERR_OK;

init_deinit:
    (void)dpu_core_deinit(&control->dpu_handle);
err:
    (void)dpu_ctlr_lock(control);
    control->dpu_handle = NULL;
    control->state = DISP_STATE_DEINIT;
    dpu_ctlr_unlock(control);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_DPU, PM_POWER_MODULE_STATE_OFF);
    return ret;
lock_err:
    return ret;
}

static avdk_err_t dpu_ctlr_deinit(bk_display_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    dpu_vn_ctlr_t *controller = dpu_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL\n");

    if (dpu_ctlr_lock(controller) != AVDK_ERR_OK)
    {
        return AVDK_ERR_GENERIC;
    }
    if (controller->state == DISP_STATE_DEINIT)
    {
        dpu_ctlr_unlock(controller);
        return AVDK_ERR_OK;
    }

    if (controller->state == DISP_STATE_DEINITING)
    {
        dpu_ctlr_unlock(controller);
        return AVDK_ERR_BUSY;
    }

    if ((controller->state != DISP_STATE_ACTIVE) &&
        (controller->state != DISP_STATE_CLOSED) &&
        (controller->state != DISP_STATE_INITED))
    {
        LOGE("%s invalid display state: %d\n", __func__, controller->state);
        dpu_ctlr_unlock(controller);
        return AVDK_ERR_GENERIC;
    }
    bool was_active = (controller->state == DISP_STATE_ACTIVE);
    controller->state = DISP_STATE_DEINITING;

    if (was_active)
    {
        (void)dpu_core_flush_stop(&controller->dpu_handle);
    }

    ret = dpu_ctlr_wait_flush_idle(controller);
    if (ret != AVDK_ERR_OK)
    {
        controller->state = (controller->dpu_handle != NULL) ? DISP_STATE_CLOSED : DISP_STATE_DEINIT;
        dpu_ctlr_unlock(controller);
        LOGE("%s wait flush idle failed: %d\n", __func__, ret);
        return ret;
    }

    dpu_ctlr_unlock(controller);

    ret = dpu_core_deinit(&controller->dpu_handle);
    if (ret != AVDK_ERR_OK)
    {
        if (dpu_ctlr_lock(controller) == AVDK_ERR_OK)
        {
            if (controller->state == DISP_STATE_DEINITING)
            {
                controller->state = (controller->dpu_handle != NULL) ? DISP_STATE_CLOSED : DISP_STATE_DEINIT;
            }
            dpu_ctlr_unlock(controller);
        }
        LOGE("%s dpu core deinit err: %d\n", __func__, ret);
        return ret;
    }

    if (dpu_ctlr_lock(controller) != AVDK_ERR_OK)
    {
        return AVDK_ERR_GENERIC;
    }
    controller->dpu_handle = NULL;
    controller->state = DISP_STATE_DEINIT;
    dpu_ctlr_unlock(controller);
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_DPU, PM_POWER_MODULE_STATE_OFF);
    LOGI("%s complete\n", __func__);

    return BK_OK;
}

static avdk_err_t dpu_ctlr_open(bk_display_ctlr_handle_t handle)
{
    dpu_vn_ctlr_t *controller = dpu_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (dpu_ctlr_lock(controller) != AVDK_ERR_OK)
    {
        return AVDK_ERR_GENERIC;
    }

    if (controller->state == DISP_STATE_ACTIVE)
    {
        dpu_ctlr_unlock(controller);
        return AVDK_ERR_OK;
    }

    if ((controller->state != DISP_STATE_INITED) &&
        (controller->state != DISP_STATE_CLOSED))
    {
        LOGE("%s invalid display state: %d\n", __func__, controller->state);
        dpu_ctlr_unlock(controller);
        return AVDK_ERR_GENERIC;
    }

    AVDK_MONITOR_DPU_ENABLE();
    //(void)dpu_core_flush_restart(&controller->dpu_handle);

    controller->state = DISP_STATE_ACTIVE;
    dpu_ctlr_unlock(controller);
    LOGI("%s complete (ACTIVE)\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t dpu_ctlr_close(bk_display_ctlr_handle_t handle)
{
    dpu_vn_ctlr_t *controller = dpu_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (dpu_ctlr_lock(controller) != AVDK_ERR_OK)
    {
        return AVDK_ERR_GENERIC;
    }

    if ((controller->state == DISP_STATE_DEINIT) ||
        (controller->state == DISP_STATE_CLOSED) ||
        (controller->state == DISP_STATE_INITED))
    {
        if (controller->state == DISP_STATE_INITED)
        {
            controller->state = DISP_STATE_CLOSED;
        }
        dpu_ctlr_unlock(controller);
        return AVDK_ERR_OK;
    }

    if (controller->state != DISP_STATE_ACTIVE)
    {
        LOGE("%s invalid display state: %d\n", __func__, controller->state);
        dpu_ctlr_unlock(controller);
        return AVDK_ERR_GENERIC;
    }

    (void)dpu_core_flush_stop(&controller->dpu_handle);

    controller->state = DISP_STATE_CLOSED;
    if (dpu_ctlr_wait_flush_idle(controller) != AVDK_ERR_OK)
    {
        if (dpu_ctlr_lock(controller) == AVDK_ERR_OK)
        {
            (void)dpu_core_flush_restart(&controller->dpu_handle);
            controller->state = DISP_STATE_ACTIVE;
            dpu_ctlr_unlock(controller);
        }
        return AVDK_ERR_GENERIC;
    }

    dpu_ctlr_unlock(controller);
    LOGI("%s complete (CLOSED)\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t dpu_ctlr_del(bk_display_ctlr_handle_t handle)
{
    dpu_vn_ctlr_t *control = dpu_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->state != DISP_STATE_DEINIT)
        (void)dpu_ctlr_deinit(handle);

    if (control->flush_idle_sem != NULL)
    {
        (void)rtos_deinit_semaphore(&control->flush_idle_sem);
        control->flush_idle_sem = NULL;
    }
    if (control->lock != NULL)
    {
        (void)rtos_deinit_mutex(&control->lock);
        control->lock = NULL;
    }
    os_free(control);
    return AVDK_ERR_OK;
}

avdk_err_t dpu_ctlr_flush(bk_display_ctlr_handle_t handle, uint8_t *frame, flush_free_cb_t cb)
{
    dpu_vn_ctlr_t *control = dpu_ctlr_from_handle(handle);
    dpu_handle_t dpu_handle = NULL;
    avdk_err_t ret = AVDK_ERR_OK;

    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    if (dpu_ctlr_lock(control) != AVDK_ERR_OK)
    {
        return AVDK_ERR_GENERIC;
    }
    if (control->state == DISP_STATE_INITED)
    {
        if (!control->flush_lazy_promoted)
        {
            LOGW("%s missing bk_display_open(), auto-promoting to ACTIVE\n", __func__);
            control->flush_lazy_promoted = true;
        }
        AVDK_MONITOR_DPU_ENABLE();
        (void)dpu_core_flush_restart(&control->dpu_handle);
        control->state = DISP_STATE_ACTIVE;
    }
    if (control->state != DISP_STATE_ACTIVE)
    {
        LOGE("%s display is not active, state=%d\n", __func__, control->state);
        dpu_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    control->inflight_flush++;
    dpu_handle = control->dpu_handle;
    dpu_ctlr_unlock(control);

    ret = dpu_core_flush(&dpu_handle, DPU_LAYER_VIDEO, frame, cb);

    if (dpu_ctlr_lock(control) != AVDK_ERR_OK)
    {
        LOGE("%s controller relock failed\n", __func__);
        return AVDK_ERR_GENERIC;
    }
    dpu_ctlr_flush_exit(control);
    dpu_ctlr_unlock(control);
    return ret;
}



static avdk_err_t dpu_ctlr_ioctl(bk_display_ctlr_handle_t handle, bk_display_ioctl_cmd_t cmd, void *arg)
{
    dpu_vn_ctlr_t *control = dpu_ctlr_from_handle(handle);
    bk_display_pixel_format_config_t *runtime_config = (bk_display_pixel_format_config_t *)arg;
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    if (dpu_ctlr_lock(control) != AVDK_ERR_OK)
    {
        return AVDK_ERR_GENERIC;
    }
    if (control->state != DISP_STATE_ACTIVE)
    {
        LOGE("%s display is not active, state=%d\n", __func__, control->state);
        dpu_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }

    switch (cmd)
    {
        case BK_DISPLAY_IOCTL_DPU_PIXEL_FORMAT:
            if (runtime_config == NULL)
            {
                dpu_ctlr_unlock(control);
                return AVDK_ERR_INVAL;
            }
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
    dpu_ctlr_unlock(control);
    return ret;
}

avdk_err_t bk_display_dpu_ctlr_new(bk_display_ctlr_handle_t *handle,
                                   bk_avdk_lcd_panel_handle_t panel,
                                   const bk_display_dpu_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;
    AVDK_RETURN_ON_FALSE(config && handle && panel, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    dpu_vn_ctlr_t *controller = os_malloc(sizeof(dpu_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(dpu_vn_ctlr_t));
    controller->state = DISP_STATE_DEINIT;

    ret = rtos_init_mutex(&controller->lock);
    if (ret != BK_OK)
    {
        LOGE("%s init mutex failed: %d\n", __func__, ret);
        os_free(controller);
        return AVDK_ERR_GENERIC;
    }

    ret = rtos_init_semaphore_ex(&controller->flush_idle_sem, 1, 0);
    if (ret != BK_OK)
    {
        LOGE("%s init flush_idle_sem failed: %d\n", __func__, ret);
        (void)rtos_deinit_mutex(&controller->lock);
        controller->lock = NULL;
        os_free(controller);
        return AVDK_ERR_GENERIC;
    }

    os_memcpy(&controller->config, config, sizeof(bk_display_dpu_config_t));
    controller->panel = panel;

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

