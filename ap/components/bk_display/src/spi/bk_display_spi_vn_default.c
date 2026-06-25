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
#include "display_spi_vn_ctlr.h"
#if CONFIG_LCD_SPI
#include <driver/lcd_spi.h>
#endif

#define TAG "bk_spi_disp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_LCD_SPI

typedef enum {
    LCD_SPI_DISP_REQUEST = 0,
    LCD_SPI_DISP_EXIT,
} lcd_spi_display_msg_type_t;

typedef struct {
    uint32_t event;
    void *frame;
    flush_free_cb_t cb;
} lcd_spi_display_msg_t;

static spi_vn_ctlr_t *spi_ctlr_from_handle(bk_display_ctlr_handle_t handle)
{
    return __containerof(handle, spi_vn_ctlr_t, ops);
}

static avdk_err_t spi_ctlr_lock(spi_vn_ctlr_t *control)
{
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(control->lock, AVDK_ERR_GENERIC, TAG, "controller lock is NULL");

    if (rtos_lock_mutex(&control->lock) != BK_OK) {
        LOGE("%s lock failed\n", __func__);
        return AVDK_ERR_GENERIC;
    }

    return AVDK_ERR_OK;
}

static void spi_ctlr_unlock(spi_vn_ctlr_t *control)
{
    if ((control != NULL) && (control->lock != NULL)) {
        (void)rtos_unlock_mutex(&control->lock);
    }
}

static void lcd_spi_display_complete_handler(void *frame, flush_free_cb_t frame_complete_cb)
{
    if (frame && frame_complete_cb) {
        frame_complete_cb(frame);
    }
}

static bk_err_t spi_ctlr_frame_display(spi_vn_ctlr_t *control, uint8_t *frame)
{
    AVDK_RETURN_ON_FALSE(control, BK_ERR_NULL_PARAM, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(frame, BK_ERR_NULL_PARAM, TAG, "frame is NULL");

    if (control->config.lcd_panel == NULL || control->config.lcd_panel->spi == NULL) {
        LOGE("%s invalid lcd panel device or spi config\n", __func__);
        return BK_ERR_NOT_SUPPORT;
    }

    return bk_lcd_spi_frame_display(control->config.spi_id,
                                    frame,
                                    control->config.lcd_panel->spi->frame_len);
}

static bk_err_t spi_ctlr_wait_display_complete(spi_vn_ctlr_t *control)
{
    AVDK_RETURN_ON_FALSE(control, BK_ERR_NULL_PARAM, TAG, "control is NULL");
    return bk_lcd_spi_wait_display_complete(control->config.spi_id);
}

static bk_err_t lcd_spi_display_task_send_msg(spi_vn_ctlr_t *control,
                                              uint8_t type,
                                              void *frame,
                                              flush_free_cb_t cb)
{
    lcd_spi_display_msg_t msg = {
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

static void lcd_spi_display_finish_inflight(spi_vn_ctlr_t *control)
{
    if ((control != NULL) && control->lcd_display_flag) {
        (void)spi_ctlr_wait_display_complete(control);
        lcd_spi_display_complete_handler(control->display_frame, control->display_frame_cb);
        control->display_frame = NULL;
        control->display_frame_cb = NULL;
        control->lcd_display_flag = false;
    }
}

static void lcd_spi_display_task_entry(beken_thread_arg_t arg)
{
    spi_vn_ctlr_t *control = (spi_vn_ctlr_t *)arg;
    control->disp_task_running = true;
    rtos_set_semaphore(&control->disp_task_sem);

    while (control->disp_task_running) {
        lcd_spi_display_msg_t msg;
        bk_err_t ret = rtos_pop_from_queue(&control->queue, &msg, 3000);
        if (ret == BK_OK) {
            switch (msg.event) {
                case LCD_SPI_DISP_REQUEST:
                    lcd_spi_display_finish_inflight(control);
                    control->display_frame = msg.frame;
                    control->display_frame_cb = msg.cb;
                    if (spi_ctlr_frame_display(control, (uint8_t *)msg.frame) == BK_OK) {
                        control->lcd_display_flag = true;
                    } else {
                        LOGE("%s frame display failed\n", __func__);
                        lcd_spi_display_complete_handler(control->display_frame, control->display_frame_cb);
                        control->display_frame = NULL;
                        control->display_frame_cb = NULL;
                        control->lcd_display_flag = false;
                    }
                    break;

                case LCD_SPI_DISP_EXIT:
                    lcd_spi_display_finish_inflight(control);
                    control->disp_task_running = false;
                    do {
                        ret = rtos_pop_from_queue(&control->queue, &msg, BEKEN_NO_WAIT);
                        if ((ret == BK_OK) && (msg.event == LCD_SPI_DISP_REQUEST)) {
                            lcd_spi_display_complete_handler(msg.frame, msg.cb);
                        }
                    } while (ret == BK_OK);
                    break;
            }
        } else {
            lcd_spi_display_finish_inflight(control);
        }
    }

    control->disp_task = NULL;
    rtos_set_semaphore(&control->disp_task_sem);
    rtos_delete_thread(NULL);
}

static void spi_ctlr_open_cleanup(spi_vn_ctlr_t *control)
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

static avdk_err_t spi_ctlr_init(bk_display_ctlr_handle_t handle)
{
    spi_vn_ctlr_t *control = spi_ctlr_from_handle(handle);
    bk_display_bus_handle_t bus_handle = NULL;
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (spi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if ((control->state == SPI_DISP_STATE_INITED) ||
        (control->state == SPI_DISP_STATE_ACTIVE) ||
        (control->state == SPI_DISP_STATE_CLOSED)) {
        spi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if (control->state != SPI_DISP_STATE_DEINITED) {
        spi_ctlr_unlock(control);
        LOGE("%s invalid display state: %d\n", __func__, control->state);
        return AVDK_ERR_GENERIC;
    }
    control->state = SPI_DISP_STATE_INITING;
    spi_ctlr_unlock(control);

    avdk_err_t ret = bk_display_spi_bus_new(&bus_handle, &control->config);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s bus new failed: %d\n", __func__, ret);
        if (spi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = SPI_DISP_STATE_DEINITED;
            spi_ctlr_unlock(control);
        }
        return ret;
    }

    if (spi_ctlr_lock(control) != AVDK_ERR_OK) {
        (void)bk_display_bus_delete(bus_handle);
        return AVDK_ERR_GENERIC;
    }
    if (control->state != SPI_DISP_STATE_INITING) {
        spi_display_state_t state = control->state;
        spi_ctlr_unlock(control);
        (void)bk_display_bus_delete(bus_handle);
        LOGE("%s invalid display state after bus new: %d\n", __func__, state);
        return AVDK_ERR_GENERIC;
    }
    control->bus_handle = bus_handle;
    control->state = SPI_DISP_STATE_INITED;
    spi_ctlr_unlock(control);
    LOGI("%s complete (INITED)\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t spi_ctlr_open(bk_display_ctlr_handle_t handle)
{
    spi_vn_ctlr_t *control = spi_ctlr_from_handle(handle);
    spi_display_state_t previous_state;
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (spi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if (control->state == SPI_DISP_STATE_ACTIVE) {
        spi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if ((control->state != SPI_DISP_STATE_INITED) &&
        (control->state != SPI_DISP_STATE_CLOSED)) {
        LOGE("%s invalid display state: %d\n", __func__, control->state);
        spi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    previous_state = control->state;
    control->state = SPI_DISP_STATE_OPENING;
    spi_ctlr_unlock(control);

    bk_err_t ret = rtos_init_semaphore(&control->disp_task_sem, 1);
    if (ret != BK_OK) {
        LOGE("%s disp_task_sem init failed: %d\n", __func__, ret);
        if (spi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = previous_state;
            spi_ctlr_unlock(control);
        }
        return AVDK_ERR_GENERIC;
    }

    ret = rtos_init_queue(&control->queue,
                          "spi_disp_queue",
                          sizeof(lcd_spi_display_msg_t),
                          20);
    if (ret != BK_OK) {
        LOGE("%s queue init failed: %d\n", __func__, ret);
        spi_ctlr_open_cleanup(control);
        if (spi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = previous_state;
            spi_ctlr_unlock(control);
        }
        return AVDK_ERR_GENERIC;
    }

    ret = rtos_create_thread(&control->disp_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "spi_disp_thread",
                             (beken_thread_function_t)lcd_spi_display_task_entry,
                             1024 * 2,
                             (beken_thread_arg_t)control);
    if (ret != BK_OK) {
        LOGE("%s display thread init failed: %d\n", __func__, ret);
        spi_ctlr_open_cleanup(control);
        if (spi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = previous_state;
            spi_ctlr_unlock(control);
        }
        return AVDK_ERR_GENERIC;
    }

    rtos_get_semaphore(&control->disp_task_sem, BEKEN_NEVER_TIMEOUT);

    if (spi_ctlr_lock(control) != AVDK_ERR_OK) {
        spi_ctlr_open_cleanup(control);
        return AVDK_ERR_GENERIC;
    }
    if (control->state != SPI_DISP_STATE_OPENING) {
        spi_display_state_t state = control->state;
        spi_ctlr_unlock(control);
        spi_ctlr_open_cleanup(control);
        LOGE("%s invalid display state after thread start: %d\n", __func__, state);
        return AVDK_ERR_GENERIC;
    }
    control->state = SPI_DISP_STATE_ACTIVE;
    spi_ctlr_unlock(control);
    LOGI("%s complete (ACTIVE)\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t spi_ctlr_close(bk_display_ctlr_handle_t handle)
{
    spi_vn_ctlr_t *control = spi_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (spi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if ((control->state == SPI_DISP_STATE_DEINITED) ||
        (control->state == SPI_DISP_STATE_CLOSED)) {
        spi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if (control->state == SPI_DISP_STATE_INITED) {
        control->state = SPI_DISP_STATE_CLOSED;
        spi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if (control->state != SPI_DISP_STATE_ACTIVE) {
        LOGE("%s invalid display state: %d\n", __func__, control->state);
        spi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    control->state = SPI_DISP_STATE_CLOSING;
    spi_ctlr_unlock(control);

    if (lcd_spi_display_task_send_msg(control, LCD_SPI_DISP_EXIT, NULL, NULL) != BK_OK) {
        LOGE("%s send display exit failed\n", __func__);
        if (spi_ctlr_lock(control) == AVDK_ERR_OK) {
            control->state = SPI_DISP_STATE_ACTIVE;
            spi_ctlr_unlock(control);
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

    if (spi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if (control->state != SPI_DISP_STATE_CLOSING) {
        spi_display_state_t state = control->state;
        spi_ctlr_unlock(control);
        LOGE("%s invalid display state after thread stop: %d\n", __func__, state);
        return AVDK_ERR_GENERIC;
    }
    control->state = SPI_DISP_STATE_CLOSED;
    spi_ctlr_unlock(control);
    LOGI("%s complete (CLOSED)\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t spi_ctlr_deinit(bk_display_ctlr_handle_t handle)
{
    spi_vn_ctlr_t *control = spi_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->state == SPI_DISP_STATE_ACTIVE) {
        AVDK_RETURN_ON_ERROR(spi_ctlr_close(handle), TAG, "close failed");
    }

    if (spi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if (control->state == SPI_DISP_STATE_DEINITED) {
        spi_ctlr_unlock(control);
        return AVDK_ERR_OK;
    }
    if ((control->state != SPI_DISP_STATE_INITED) &&
        (control->state != SPI_DISP_STATE_CLOSED)) {
        LOGE("%s invalid display state: %d\n", __func__, control->state);
        spi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    control->state = SPI_DISP_STATE_DEINITING;
    bk_display_bus_handle_t bus_handle = control->bus_handle;
    control->bus_handle = NULL;
    spi_ctlr_unlock(control);

    if (bus_handle != NULL) {
        (void)bk_display_bus_delete(bus_handle);
    }

    if (spi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    control->state = SPI_DISP_STATE_DEINITED;
    spi_ctlr_unlock(control);
    LOGI("%s complete\n", __func__);
    return AVDK_ERR_OK;
}

static avdk_err_t spi_ctlr_del(bk_display_ctlr_handle_t handle)
{
    spi_vn_ctlr_t *control = spi_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    if (control->state != SPI_DISP_STATE_DEINITED) {
        AVDK_RETURN_ON_ERROR(spi_ctlr_deinit(handle), TAG, "deinit failed");
    }

    if (control->lock != NULL) {
        (void)rtos_deinit_mutex(&control->lock);
        control->lock = NULL;
    }
    os_free(control);
    return AVDK_ERR_OK;
}

static avdk_err_t spi_ctlr_flush(bk_display_ctlr_handle_t handle, uint8_t *frame, flush_free_cb_t cb)
{
    spi_vn_ctlr_t *control = spi_ctlr_from_handle(handle);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(frame, AVDK_ERR_INVAL, TAG, "frame is NULL");

    if (spi_ctlr_lock(control) != AVDK_ERR_OK) {
        return AVDK_ERR_GENERIC;
    }
    if (control->state != SPI_DISP_STATE_ACTIVE) {
        LOGE("%s display is not active, state=%d\n", __func__, control->state);
        spi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    if (lcd_spi_display_task_send_msg(control, LCD_SPI_DISP_REQUEST, frame, cb) != BK_OK) {
        LOGE("%s send display request failed\n", __func__);
        spi_ctlr_unlock(control);
        return AVDK_ERR_GENERIC;
    }
    spi_ctlr_unlock(control);

    return AVDK_ERR_OK;
}

#endif /* CONFIG_LCD_SPI */

avdk_err_t bk_display_spi_ctlr_new(bk_display_ctlr_handle_t *handle, bk_display_spi_bus_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->mode == BK_DISPLAY_SPI_BUS_MODE_HW,
                         AVDK_ERR_INVAL,
                         TAG,
                         "SPI display controller requires HW mode");
    AVDK_RETURN_ON_FALSE(config->lcd_panel, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config->lcd_panel->spi, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

#if CONFIG_LCD_SPI
    spi_vn_ctlr_t *controller = os_malloc(sizeof(spi_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(controller, 0, sizeof(spi_vn_ctlr_t));
    os_memcpy(&controller->config, config, sizeof(bk_display_spi_bus_config_t));
    controller->state = SPI_DISP_STATE_DEINITED;

    bk_err_t ret = rtos_init_mutex(&controller->lock);
    if (ret != BK_OK) {
        LOGE("%s init mutex failed: %d\n", __func__, ret);
        os_free(controller);
        return AVDK_ERR_GENERIC;
    }

    controller->ops.init = spi_ctlr_init;
    controller->ops.open = spi_ctlr_open;
    controller->ops.close = spi_ctlr_close;
    controller->ops.flush = spi_ctlr_flush;
    controller->ops.deinit = spi_ctlr_deinit;
    controller->ops.del = spi_ctlr_del;
    *handle = &(controller->ops);

    return AVDK_ERR_OK;
#else
    LOGE("SPI display controller requested but CONFIG_LCD_SPI is not enabled\n");
    return AVDK_ERR_UNSUPPORTED;
#endif
}
