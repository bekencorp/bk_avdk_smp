#include <os/os.h>
#include <os/mem.h>
#include <driver/lcd_spi.h>
#include <components/bk_display_types.h>
#include <components/bk_display_bus.h>
#include <avdk_check.h>
#include "display_spi_bus_vn_ctlr.h"
#include <driver/gpio.h>
#include "gpio_driver.h"


#define TAG "bk_spi_bus"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


typedef enum {
    LCD_SPI_DISP_REQUEST = 0,
    LCD_SPI_DISP_EXIT,
} lcd_spi_display_msg_type_t;

typedef struct {
    uint32_t event;
    uint32_t param0;
    uint32_t param1;
} lcd_spi_display_msg_t;

static bk_err_t lcd_spi_display_task_send_msg(private_display_spi_context_t *context, uint8_t type, uint32_t param0, uint32_t param1)
{
    bk_err_t ret = BK_OK;
    lcd_spi_display_msg_t msg;

    if (context && context->disp_task_running) {
        msg.event = type;
        msg.param0 = param0;
        msg.param1 = param1;

        ret = rtos_push_to_queue(&context->queue, &msg, BEKEN_WAIT_FOREVER);
        if (ret != BK_OK) {
            LOGE("%s push failed\n", __func__);
        }
    }

    return ret;
}

static void lcd_spi_display_complete_handler(void *frame, flush_free_cb_t frame_complete_cb)
{
    if (frame && frame_complete_cb) {
        frame_complete_cb(frame);
    }
}

static void lcd_spi_display_task_entry(beken_thread_arg_t arg)
{
    private_display_spi_context_t *context = (private_display_spi_context_t *)arg;
    context->disp_task_running = true;
    rtos_set_semaphore(&context->disp_task_sem);

    while (context->disp_task_running)
    {
        lcd_spi_display_msg_t msg;
        int ret = rtos_pop_from_queue(&context->queue, &msg, 3000);
        if (ret == BK_OK) {
            switch (msg.event) {
                case LCD_SPI_DISP_REQUEST:
                    if (context->lcd_display_flag) {
                        bk_lcd_spi_wait_display_complete(context->spi_id);
                        lcd_spi_display_complete_handler(context->display_frame, context->display_frame_cb);
                        context->lcd_display_flag = false;
                    }

                    context->display_frame = (void *)msg.param0;
                    context->display_frame_cb = (flush_free_cb_t)msg.param1;
                    if (context->device && context->device->spi) {
                        bk_lcd_spi_frame_display(context->spi_id, (uint8_t *)context->display_frame, context->device->spi->frame_len);
                        context->lcd_display_flag = true;
                    } else {
                        LOGE("%s invalid lcd panel device or spi config\n", __func__);
                        lcd_spi_display_complete_handler(context->display_frame, context->display_frame_cb);
                        context->lcd_display_flag = false;
                    }
                    break;

                case LCD_SPI_DISP_EXIT:
                    if (context->lcd_display_flag) {
                        bk_lcd_spi_wait_display_complete(context->spi_id);
                        lcd_spi_display_complete_handler(context->display_frame, context->display_frame_cb);
                        context->lcd_display_flag = false;
                    }

                    context->disp_task_running = false;
                    do {
                        ret = rtos_pop_from_queue(&context->queue, &msg, BEKEN_NO_WAIT);
                        if (ret == BK_OK) {
                            if (msg.event == LCD_SPI_DISP_REQUEST) {
                                if (msg.param1 != 0) {
                                    ((flush_free_cb_t)msg.param1)((void *)msg.param0);
                                }
                            }
                        }
                    } while (ret == BK_OK);
                    break;
            }
        } else {
            if (context->lcd_display_flag) {
                bk_lcd_spi_wait_display_complete(context->spi_id);
                lcd_spi_display_complete_handler(context->display_frame, context->display_frame_cb);
                context->lcd_display_flag = false;
            }
        }
    }

    context->disp_task = NULL;
    rtos_set_semaphore(&context->disp_task_sem);
    rtos_delete_thread(NULL);
}

static bk_err_t bk_lcd_spi_display_open(private_display_spi_context_t *context, bk_display_spi_bus_config_t *config)
{
    bk_err_t ret = BK_OK;

    if (context->disp_task_running) {
        LOGW("%s have opened\r\n", __func__);
        return BK_OK;
    }

    context->spi_id = config->spi_id;
    context->device = config->lcd_panel;
    context->reset_pin = config->reset_pin;
    context->dc_pin = config->dc_pin;
    bk_lcd_spi_init(context->spi_id, context->device, context->reset_pin, context->dc_pin);

    ret = rtos_init_semaphore(&context->disp_task_sem, 1);
    if (ret != BK_OK) {
        LOGE("%s disp_task_sem init failed: %d\n", __func__, ret);
        goto out;
    }

    ret = rtos_init_mutex(&context->lock);
    if (ret != BK_OK) {
        LOGE("%s lock init failed: %d\n", __func__, ret);
        goto out;
    }

    ret = rtos_init_queue(&context->queue,
                          "spi_disp_queue",
                          sizeof(lcd_spi_display_msg_t),
                          20);

    if (ret != BK_OK) {
        LOGE("%s, init display_queue failed\r\n", __func__);
        goto out;
    }

    ret = rtos_create_thread(&context->disp_task,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "spi_disp_thread",
                             (beken_thread_function_t)lcd_spi_display_task_entry,
                             1024 * 2,
                             (beken_thread_arg_t)context);

    if (BK_OK != ret) {
        LOGE("%s lcd_display_thread init failed\n", __func__);
        goto out;
    }

    ret = rtos_get_semaphore(&context->disp_task_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret) {
        LOGE("%s disp_task_sem get failed\n", __func__);
        goto out;
    }

    return ret;

out:
    bk_lcd_spi_deinit(context->spi_id, context->reset_pin, context->dc_pin);

    if (context->disp_task_sem) {
        rtos_deinit_semaphore(&context->disp_task_sem);
        context->disp_task_sem = NULL;
    }

    if (context->queue) {
        rtos_deinit_queue(&context->queue);
        context->queue = NULL;
    }

    if (context->lock) {
        rtos_deinit_mutex(&context->lock);
        context->lock = NULL;
    }

    if (context->disp_task) {
        rtos_delete_thread(&context->disp_task);
        context->disp_task = NULL;
    }

    return ret;
}

static void bk_lcd_spi_display_close(private_display_spi_context_t *context)
{
    bk_err_t ret = BK_OK;

    if (context->disp_task_running == false) {
        LOGW("%s have closed\r\n", __func__);
        return;
    }

    lcd_spi_display_task_send_msg(context, LCD_SPI_DISP_EXIT, 0, 0);

    ret = rtos_get_semaphore(&context->disp_task_sem, BEKEN_NEVER_TIMEOUT);
    if (BK_OK != ret)
    {
        LOGE("%s disp_task_sem get failed\n", __func__);
        return;
    }

    bk_lcd_spi_deinit(context->spi_id, context->reset_pin, context->dc_pin);

    if (context->queue) {
        rtos_deinit_queue(&context->queue);
        context->queue = NULL;
    }

    if (context->lock) {
        rtos_deinit_mutex(&context->lock);
        context->lock = NULL;
    }

    if (context->disp_task_sem) {
        rtos_deinit_semaphore(&context->disp_task_sem);
        context->disp_task_sem = NULL;
    }
}

static avdk_err_t bk_display_spi_bus_enable(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);

    bk_lcd_spi_display_open(&bus->spi_context, &bus->config);

    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_spi_bus_disable(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);

    bk_lcd_spi_display_close(&bus->spi_context);

    return AVDK_ERR_OK;
}

static avdk_err_t bk_display_spi_bus_delete(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, BK_ERR_NULL_PARAM, TAG, "invalid bus controller handle");
    spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);

    os_free(bus);

    return AVDK_ERR_OK;
}

avdk_err_t bk_display_spi_read(bk_display_bus_ctlr_t *controller, bk_display_bus_rw_type_t type, uint32_t cmd, void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(controller, BK_ERR_NULL_PARAM, TAG, "invalid bus controller handle");
    // spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);


    return AVDK_ERR_OK;
}

avdk_err_t bk_display_spi_write(bk_display_bus_ctlr_t *controller, bk_display_bus_rw_type_t type, uint32_t cmd, const void *param, size_t size)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    // spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);

    return AVDK_ERR_OK;
}

avdk_err_t bk_display_spi_flush(bk_display_bus_ctlr_t *controller, uint8_t *frame, flush_free_cb_t cb)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);

    lcd_spi_display_task_send_msg(&bus->spi_context, LCD_SPI_DISP_REQUEST, (uint32_t)frame, (uint32_t)cb);

    return AVDK_ERR_OK;
}

avdk_err_t bk_display_spi_bus_new(bk_display_bus_handle_t *handle, bk_display_spi_bus_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    spi_bus_vn_ctlr_t *bus = os_malloc(sizeof(spi_bus_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(bus, 0, sizeof(spi_bus_vn_ctlr_t));
    os_memcpy(&bus->config, config, sizeof(bk_display_spi_bus_config_t));

    bus->ops.enable = bk_display_spi_bus_enable;
    bus->ops.disable = bk_display_spi_bus_disable;
    bus->ops.delete = bk_display_spi_bus_delete;
    bus->ops.read = bk_display_spi_read;
    bus->ops.write = bk_display_spi_write;
    bus->ops.flush = bk_display_spi_flush;

    *handle = &(bus->ops);

    return AVDK_ERR_OK;
}