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
#include <common/bk_err.h>
#include <avdk_check.h>
#include <components/log.h>
#include <components/bk_display.h>
#include "display_qspi_vn_ctlr.h"
#if CONFIG_LCD_QSPI
#include <driver/lcd_qspi.h>
#include <driver/pwr_clk.h>
#endif

#define TAG "bk_qspi_disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_LCD_QSPI

typedef enum {
    LCD_QSPI_DISP_REQUEST = 0,
    LCD_QSPI_DISP_EXIT,
} lcd_qspi_display_msg_type_t;

typedef struct {
    uint32_t event;
    void *frame;
    flush_free_cb_t cb;
} lcd_qspi_display_msg_t;

static qspi_vn_ctlr_t *qspi_ctlr_from_handle(bk_display_ctlr_handle_t handle)
{
    return __containerof(handle, qspi_vn_ctlr_t, ops);
}

static avdk_err_t qspi_ctlr_lock(qspi_vn_ctlr_t *control)
{
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(control->lock, AVDK_ERR_GENERIC, TAG, "controller lock is NULL");

    if (rtos_lock_mutex(&control->lock) != BK_OK) {
        LOGE("%s lock failed\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    return AVDK_ERR_OK;
}

static void qspi_ctlr_unlock(qspi_vn_ctlr_t *control)
{
    if ((control != NULL) && (control->lock != NULL)) {
        (void)rtos_unlock_mutex(&control->lock);
    }
}

static void lcd_qspi_display_complete_handler(void *frame, flush_free_cb_t frame_complete_cb)
{
    if (frame && frame_complete_cb) {
        frame_complete_cb(frame);
    }
}

static bk_err_t qspi_ctlr_frame_display(qspi_vn_ctlr_t *control, uint8_t *frame)
{
    AVDK_RETURN_ON_FALSE(control, BK_ERR_NULL_PARAM, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(frame, BK_ERR_NULL_PARAM, TAG, "frame is NULL");

    if (control->config.lcd_panel == NULL || control->config.lcd_panel->qspi == NULL) {
        LOGE("%s invalid lcd panel device or qspi config\n", __func__);
        return BK_ERR_NOT_SUPPORT;
    }

    return bk_lcd_qspi_frame_display(control->config.qspi_id,
                                     control->config.lcd_panel,
                                     (uint32_t *)frame,
                                     control->config.lcd_panel->qspi->frame_len);
}

static bk_err_t qspi_ctlr_wait_display_complete(qspi_vn_ctlr_t *control)
{
    AVDK_RETURN_ON_FALSE(control, BK_ERR_NULL_PARAM, TAG, "control is NULL");
    return bk_lcd_qspi_wait_display_complete(control->config.qspi_id, control->config.lcd_panel);
}

static bk_err_t lcd_qspi_display_task_send_msg(qspi_vn_ctlr_t *control,
                                               uint8_t type,
                                               void *frame,
                                               flush_free_cb_t cb)
{
    lcd_qspi_display_msg_t msg = {
        .event = type,
        .frame = frame,
        .cb = cb,
    };

    if ((control == NULL) || (control->disp_task_running == false)) {
        return BK_FAIL;
    }

    bk_err_t ret = rtos_push_to_queue(&control->queue, &msg, BEKEN_WAIT_FOREVER);
    if (ret != BK_OK) {
        LOGE("%s push failed\n", __func__);
    }

    return ret;
}

static void lcd_qspi_display_finish_inflight(qspi_vn_ctlr_t *control)
{
    if ((control != NULL) && control->lcd_display_flag) {
        (void)qspi_ctlr_wait_display_complete(control);
        lcd_qspi_display_complete_handler(control->display_frame, control->display_frame_cb);
        control->display_frame = NULL;
        control->display_frame_cb = NULL;
        control->lcd_display_flag = false;
    }
}

static void lcd_qspi_display_drain_queue(qspi_vn_ctlr_t *control)
{
    lcd_qspi_display_msg_t msg;
    bk_err_t ret;

    do {
        ret = rtos_pop_from_queue(&control->queue, &msg, BEKEN_NO_WAIT);
        if ((ret == BK_OK) && (msg.event == LCD_QSPI_DISP_REQUEST)) {
            lcd_qspi_display_complete_handler(msg.frame, msg.cb);
        }
    } while (ret == BK_OK);
}

static void lcd_qspi_display_task_entry(beken_thread_arg_t arg)
{
    qspi_vn_ctlr_t *control = (qspi_vn_ctlr_t *)arg;
    const bk_display_qspi_panel_t *device = control->config.lcd_panel;

    control->disp_task_running = true;
    rtos_set_semaphore(&control->disp_task_sem);

    while (control->disp_task_running) {
        lcd_qspi_display_msg_t msg;
        bk_err_t ret = rtos_pop_from_queue(&control->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != BK_OK) {
            lcd_qspi_display_finish_inflight(control);
            continue;
        }

        switch (msg.event) {
            case LCD_QSPI_DISP_REQUEST:
                lcd_qspi_display_finish_inflight(control);
                control->display_frame = msg.frame;
                control->display_frame_cb = msg.cb;

                if (qspi_ctlr_frame_display(control, (uint8_t *)control->display_frame) != BK_OK) {
                    LOGE("%s frame display failed\n", __func__);
                    lcd_qspi_display_complete_handler(control->display_frame, control->display_frame_cb);
                    control->display_frame = NULL;
                    control->display_frame_cb = NULL;
                    break;
                }

                if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_LINE) {
                    do {
                        ret = qspi_ctlr_wait_display_complete(control);
                        if (ret != BK_OK) {
                            LOGE("%s wait display complete failed: %d\n", __func__, ret);
                            break;
                        }

                        ret = rtos_pop_from_queue(&control->queue, &msg, 10);
                        if (ret == BK_OK) {
                            if (msg.event == LCD_QSPI_DISP_EXIT) {
                                lcd_qspi_display_complete_handler(control->display_frame,
                                                                  control->display_frame_cb);
                                control->display_frame = NULL;
                                control->display_frame_cb = NULL;
                                control->disp_task_running = false;
                                lcd_qspi_display_drain_queue(control);
                                break;
                            }

                            lcd_qspi_display_complete_handler(control->display_frame,
                                                              control->display_frame_cb);
                            control->display_frame = msg.frame;
                            control->display_frame_cb = msg.cb;
                        }

                        ret = qspi_ctlr_frame_display(control, (uint8_t *)control->display_frame);
                    } while ((ret == BK_OK) && control->disp_task_running);

                    if (control->disp_task_running == false) {
                        break;
                    }

                    lcd_qspi_display_complete_handler(control->display_frame, control->display_frame_cb);
                    control->display_frame = NULL;
                    control->display_frame_cb = NULL;
                } else {
                    control->lcd_display_flag = true;
                }
                break;

            case LCD_QSPI_DISP_EXIT:
                lcd_qspi_display_finish_inflight(control);
                control->disp_task_running = false;
                lcd_qspi_display_drain_queue(control);
                break;
        }
    }

    control->disp_task = NULL;
    rtos_set_semaphore(&control->disp_task_sem);
    rtos_delete_thread(NULL);
}

static void qspi_ctlr_open_cleanup(qspi_vn_ctlr_t *control)
{
    if (control == NULL) {
        return;
    }

    control->disp_task_running = false;
    if (control->disp_task != NULL) {
        (void)rtos_delete_thread(&control->disp_task);
        control->disp_task = NULL;
    }
    if (control->queue != NULL) {
        (void)rtos_deinit_queue(&control->queue);
        control->queue = NULL;
    }
    if (control->disp_task_sem != NULL) {
        (void)rtos_deinit_semaphore(&control->disp_task_sem);
        control->disp_task_sem = NULL;
    }
}

static avdk_err_t qspi_ctlr_init(bk_display_ctlr_handle_t handle)
{
    qspi_vn_ctlr_t *control = qspi_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (qspi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if ((control->state == QSPI_DISP_STATE_INITED) ||
        (control->state == QSPI_DISP_STATE_ACTIVE) ||
        (control->state == QSPI_DISP_STATE_CLOSED)) {
        qspi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if (control->state != QSPI_DISP_STATE_DEINITED) {
        qspi_ctlr_unlock(control);
        LOGE("%s invalid display state: %d\n", __func__, control->state);
        return AVDK_ERR_GENERIC;
    }
    control->state = QSPI_DISP_STATE_INITING;
    qspi_ctlr_unlock(control);

    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_ON);
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_480M);

    bk_err_t ret = bk_lcd_qspi_init(control->config.qspi_id,
                                    control->config.lcd_panel,
                                    control->config.reset_pin);
    if (ret != BK_OK) {
        LOGE("%s bk_lcd_qspi_init failed: %d\n", __func__, ret);
        bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);
        bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
        if (qspi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = QSPI_DISP_STATE_DEINITED;
            qspi_ctlr_unlock(control);
        }
        return AVDK_ERR_GENERIC;
    }

    if (qspi_ctlr_lock(control) != AVDK_ERR_OK) {
        (void)bk_lcd_qspi_deinit(control->config.qspi_id, control->config.reset_pin);
        bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);
        bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
        return AVDK_ERR_GENERIC;
    }
    if (control->state != QSPI_DISP_STATE_INITING) {
        qspi_display_state_t state = control->state;
        qspi_ctlr_unlock(control);
        (void)bk_lcd_qspi_deinit(control->config.qspi_id, control->config.reset_pin);
        bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);
        bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);
        LOGE("%s invalid display state after init: %d\n", __func__, state);
        return AVDK_ERR_GENERIC;
    }
    control->state = QSPI_DISP_STATE_INITED;
    qspi_ctlr_unlock(control);

    LOGI("%s complete (INITED)\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t qspi_ctlr_open(bk_display_ctlr_handle_t handle)
{
    qspi_vn_ctlr_t *control = qspi_ctlr_from_handle(handle);
    qspi_display_state_t previous_state;
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (qspi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if (control->state == QSPI_DISP_STATE_ACTIVE) {
        qspi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if ((control->state != QSPI_DISP_STATE_INITED) &&
        (control->state != QSPI_DISP_STATE_CLOSED)) {
        LOGE("%s invalid display state: %d\n", __func__, control->state);
        qspi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    previous_state = control->state;
    control->state = QSPI_DISP_STATE_OPENING;
    qspi_ctlr_unlock(control);

    bk_err_t ret = rtos_init_semaphore(&control->disp_task_sem, 1);
    if (ret != BK_OK) {
        LOGE("%s disp_task_sem init failed: %d\n", __func__, ret);
        if (qspi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = previous_state;
            qspi_ctlr_unlock(control);
        }
        return AVDK_ERR_GENERIC;
    }

    ret = rtos_init_queue(&control->queue,
                          "qspi_disp_queue",
                          sizeof(lcd_qspi_display_msg_t),
                          20);
    if (ret != BK_OK) {
        LOGE("%s queue init failed: %d\n", __func__, ret);
        qspi_ctlr_open_cleanup(control);
        if (qspi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = previous_state;
            qspi_ctlr_unlock(control);
        }
        return AVDK_ERR_GENERIC;
    }

    ret = rtos_create_thread(&control->disp_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "qspi_disp_thread",
                             (beken_thread_function_t)lcd_qspi_display_task_entry,
                             1024 * 2,
                             (beken_thread_arg_t)control);
    if (ret != BK_OK) {
        LOGE("%s display thread init failed: %d\n", __func__, ret);
        qspi_ctlr_open_cleanup(control);
        if (qspi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = previous_state;
            qspi_ctlr_unlock(control);
        }
        return AVDK_ERR_GENERIC;
    }

    rtos_get_semaphore(&control->disp_task_sem, BEKEN_NEVER_TIMEOUT);

    if (qspi_ctlr_lock(control) != AVDK_ERR_OK) {
        qspi_ctlr_open_cleanup(control);
        return AVDK_ERR_GENERIC;
    }
    if (control->state != QSPI_DISP_STATE_OPENING) {
        qspi_display_state_t state = control->state;
        qspi_ctlr_unlock(control);
        qspi_ctlr_open_cleanup(control);
        LOGE("%s invalid display state after thread start: %d\n", __func__, state);
        return AVDK_ERR_GENERIC;
    }
    control->state = QSPI_DISP_STATE_ACTIVE;
    qspi_ctlr_unlock(control);

    LOGI("%s complete (ACTIVE)\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t qspi_ctlr_close(bk_display_ctlr_handle_t handle)
{
    qspi_vn_ctlr_t *control = qspi_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (qspi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if ((control->state == QSPI_DISP_STATE_DEINITED) ||
        (control->state == QSPI_DISP_STATE_CLOSED)) {
        qspi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if (control->state == QSPI_DISP_STATE_INITED) {
        control->state = QSPI_DISP_STATE_CLOSED;
        qspi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if (control->state != QSPI_DISP_STATE_ACTIVE) {
        LOGE("%s invalid display state: %d\n", __func__, control->state);
        qspi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    control->state = QSPI_DISP_STATE_CLOSING;
    qspi_ctlr_unlock(control);

    if (lcd_qspi_display_task_send_msg(control, LCD_QSPI_DISP_EXIT, NULL, NULL) != BK_OK) {
        LOGE("%s send display exit failed\n", __func__);
        if (qspi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = QSPI_DISP_STATE_ACTIVE;
            qspi_ctlr_unlock(control);
        }
        return AVDK_ERR_GENERIC;
    }

    rtos_get_semaphore(&control->disp_task_sem, BEKEN_NEVER_TIMEOUT);

    if (control->queue) {
        rtos_deinit_queue(&control->queue);
        control->queue = NULL;
    }
    if (control->disp_task_sem) {
        rtos_deinit_semaphore(&control->disp_task_sem);
        control->disp_task_sem = NULL;
    }

    if (qspi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if (control->state != QSPI_DISP_STATE_CLOSING) {
        qspi_display_state_t state = control->state;
        qspi_ctlr_unlock(control);
        LOGE("%s invalid display state after thread stop: %d\n", __func__, state);
        return AVDK_ERR_GENERIC;
    }
    control->state = QSPI_DISP_STATE_CLOSED;
    qspi_ctlr_unlock(control);

    LOGI("%s complete (CLOSED)\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t qspi_ctlr_deinit(bk_display_ctlr_handle_t handle)
{
    qspi_vn_ctlr_t *control = qspi_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->state == QSPI_DISP_STATE_ACTIVE) {
        AVDK_RETURN_ON_ERROR(qspi_ctlr_close(handle), TAG, "close failed");
    }

    if (qspi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if (control->state == QSPI_DISP_STATE_DEINITED) {
        qspi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if ((control->state != QSPI_DISP_STATE_INITED) &&
        (control->state != QSPI_DISP_STATE_CLOSED)) {
        LOGE("%s invalid display state: %d\n", __func__, control->state);
        qspi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    control->state = QSPI_DISP_STATE_DEINITING;
    qspi_ctlr_unlock(control);

    (void)bk_lcd_qspi_deinit(control->config.qspi_id, control->config.reset_pin);
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_DISP, PM_CPU_FRQ_DEFAULT);
    bk_pm_module_vote_psram_ctrl(PM_POWER_PSRAM_MODULE_NAME_VIDP_LCD, PM_POWER_MODULE_STATE_OFF);

    if (qspi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    control->state = QSPI_DISP_STATE_DEINITED;
    qspi_ctlr_unlock(control);

    LOGI("%s complete\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t qspi_ctlr_del(bk_display_ctlr_handle_t handle)
{
    qspi_vn_ctlr_t *control = qspi_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->state != QSPI_DISP_STATE_DEINITED) {
        AVDK_RETURN_ON_ERROR(qspi_ctlr_deinit(handle), TAG, "deinit failed");
    }

    if (control->lock != NULL) {
        (void)rtos_deinit_mutex(&control->lock);
        control->lock = NULL;
    }
    os_free(control);
    return AVDK_ERR_OK;
}

static avdk_err_t qspi_ctlr_flush(bk_display_ctlr_handle_t handle, uint8_t *frame, flush_free_cb_t cb)
{
    qspi_vn_ctlr_t *control = qspi_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(frame, AVDK_ERR_INVAL, TAG, "frame is NULL");

    if (qspi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if (control->state != QSPI_DISP_STATE_ACTIVE) {
        LOGE("%s display is not active, state=%d\n", __func__, control->state);
        qspi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    if (lcd_qspi_display_task_send_msg(control,
                                       LCD_QSPI_DISP_REQUEST,
                                       frame,
                                       cb) != BK_OK) {
        LOGE("%s send display request failed\n", __func__);
        qspi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    qspi_ctlr_unlock(control);

    return AVDK_ERR_OK;
}

#endif /* CONFIG_LCD_QSPI */

avdk_err_t bk_display_qspi_ctlr_new(bk_display_ctlr_handle_t *handle,
                                    bk_display_qspi_ctlr_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->lcd_panel, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->lcd_panel->qspi, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

#if CONFIG_LCD_QSPI
    qspi_vn_ctlr_t *controller = os_malloc(sizeof(qspi_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(qspi_vn_ctlr_t));
    os_memcpy(&controller->config, config, sizeof(bk_display_qspi_ctlr_config_t));
    controller->state = QSPI_DISP_STATE_DEINITED;

    bk_err_t ret = rtos_init_mutex(&controller->lock);
    if (ret != BK_OK) {
        LOGE("%s init mutex failed: %d\n", __func__, ret);
        os_free(controller);
        return AVDK_ERR_GENERIC;
    }

    controller->ops.init = qspi_ctlr_init;
    controller->ops.open = qspi_ctlr_open;
    controller->ops.close = qspi_ctlr_close;
    controller->ops.flush = qspi_ctlr_flush;
    controller->ops.deinit = qspi_ctlr_deinit;
    controller->ops.del = qspi_ctlr_del;
    *handle = &(controller->ops);

    return AVDK_ERR_OK;
#else
    LOGE("QSPI display controller requested but CONFIG_LCD_QSPI is not enabled\n");
    return AVDK_ERR_UNSUPPORTED;
#endif
}
