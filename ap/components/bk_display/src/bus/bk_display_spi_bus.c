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

// SPI bus backend (HW + SW modes).
//
// Implements ::bk_display_bus_ctlr_t for both:
//   * BK_DISPLAY_SPI_BUS_MODE_HW :: avdk_driver/lcd_spi + DMA + frame
//                                   task, used by SPI panels (frame
//                                   data flows through the bus). The
//                                   command channel is the same DMA
//                                   stream, so tx_param / rx_param ops
//                                   are not implemented.
//   * BK_DISPLAY_SPI_BUS_MODE_SW :: GPIO bit-bang command channel for
//                                   RGB panel SPI register init. Pixel
//                                   data does NOT go through this bus
//                                   in SW mode - the DPU drives the
//                                   parallel RGB lanes directly.
//
// SW mode wires bus->ops.tx_param to a bit-bang implementation that
// honours the panel-descriptor wire format (8-bit 9-bit / 16-bit
// packed) selected by ::bk_display_spi_bus_config_t::cmd_width.

#include <os/os.h>
#include <os/mem.h>
#include <avdk_check.h>
#include <components/log.h>
#include <components/bk_display_bus.h>
#include <components/bk_lcd_panel.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#if CONFIG_LCD_SPI
#include <driver/lcd_spi.h>
#endif

#include "display_spi_bus_vn_ctlr.h"

#define TAG "bk_spi_bus"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern void delay(INT32 num);

#define LCD_SPI_BITBANG_DELAY 2

static void spi_sw_gpio_init(uint8_t sda, uint8_t clk, uint8_t csx)
{
    LOGI("%s, sda=%u clk=%u csx=%u\n", __func__, sda, clk, csx);

    (void)sda;
    (void)clk;
    (void)csx;

    /* Runtime control: drive CLK/CSX idle to high. */
    bk_gpio_set_output_high(clk);
    bk_gpio_set_output_high(csx);

    delay(200);
}

static void spi_sw_send_byte(uint8_t sda, uint8_t clk, uint8_t data)
{
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();

    for (uint8_t n = 0; n < 8; n++) {
        if (data & 0x80) {
            bk_gpio_set_output_high(sda);
        } else {
            bk_gpio_set_output_low(sda);
        }
        delay(LCD_SPI_BITBANG_DELAY);
        data <<= 1;
        bk_gpio_set_output_low(clk);
        delay(LCD_SPI_BITBANG_DELAY);
        bk_gpio_set_output_high(clk);
        delay(LCD_SPI_BITBANG_DELAY);
    }

    GLOBAL_INT_RESTORE();
}

static void spi_sw_write_cmd(uint8_t csx, uint8_t sda, uint8_t clk, uint8_t cmd)
{
    bk_gpio_set_output_low(csx);
    delay(LCD_SPI_BITBANG_DELAY);
    bk_gpio_set_output_low(sda);
    delay(LCD_SPI_BITBANG_DELAY);
    bk_gpio_set_output_low(clk);
    delay(LCD_SPI_BITBANG_DELAY);
    bk_gpio_set_output_high(clk);
    delay(LCD_SPI_BITBANG_DELAY);
    spi_sw_send_byte(sda, clk, cmd);
    bk_gpio_set_output_high(csx);
    delay(LCD_SPI_BITBANG_DELAY);
}

static void spi_sw_write_data(uint8_t csx, uint8_t sda, uint8_t clk, uint8_t data)
{
    bk_gpio_set_output_low(csx);
    delay(LCD_SPI_BITBANG_DELAY);
    bk_gpio_set_output_high(sda);
    delay(LCD_SPI_BITBANG_DELAY);
    bk_gpio_set_output_low(clk);
    delay(LCD_SPI_BITBANG_DELAY);
    bk_gpio_set_output_high(clk);
    delay(LCD_SPI_BITBANG_DELAY);
    spi_sw_send_byte(sda, clk, data);
    bk_gpio_set_output_high(csx);
    delay(LCD_SPI_BITBANG_DELAY);
}

static void spi_sw_write_hf_cmd(uint8_t csx, uint8_t sda, uint8_t clk, uint16_t cmd)
{
    bk_gpio_set_output_low(csx);
    delay(LCD_SPI_BITBANG_DELAY);
    spi_sw_send_byte(sda, clk, 0x20);
    spi_sw_send_byte(sda, clk, cmd >> 8);
    spi_sw_send_byte(sda, clk, 0x00);
    spi_sw_send_byte(sda, clk, cmd & 0xff);
    bk_gpio_set_output_high(csx);
    delay(LCD_SPI_BITBANG_DELAY);
}

static void spi_sw_write_hf_data(uint8_t csx, uint8_t sda, uint8_t clk, uint16_t data)
{
    bk_gpio_set_output_low(csx);
    delay(LCD_SPI_BITBANG_DELAY);
    spi_sw_send_byte(sda, clk, 0x40);
    spi_sw_send_byte(sda, clk, data & 0xff);
    bk_gpio_set_output_high(csx);
    delay(LCD_SPI_BITBANG_DELAY);
}

#if CONFIG_LCD_SPI

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

    while (context->disp_task_running) {
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
    if (BK_OK != ret) {
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

#endif /* CONFIG_LCD_SPI */

static bk_err_t spi_sw_tx_param(bk_display_bus_ctlr_t *controller, int lcd_cmd,
                                const void *param, uint16_t param_size)
{
    AVDK_RETURN_ON_FALSE(controller, BK_ERR_NULL_PARAM, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);

    const uint8_t csx = bus->config.csx_pin;
    const uint8_t sda = bus->config.sda_pin;
    const uint8_t clk = bus->config.clk_pin;
    const uint8_t *bytes = (const uint8_t *)param;

    if (bus->config.cmd_width == 16) {
        spi_sw_write_hf_cmd(csx, sda, clk, (uint16_t)lcd_cmd);
        for (uint16_t i = 0; i < param_size; i++) {
            spi_sw_write_hf_data(csx, sda, clk, (uint16_t)bytes[i]);
        }
    } else {
        spi_sw_write_cmd(csx, sda, clk, (uint8_t)lcd_cmd);
        for (uint16_t i = 0; i < param_size; i++) {
            spi_sw_write_data(csx, sda, clk, bytes[i]);
        }
    }
    return BK_OK;
}

static avdk_err_t spi_sw_delete(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);
    os_free(bus);
    return AVDK_ERR_OK;
}

#if CONFIG_LCD_SPI

static avdk_err_t spi_hw_flush(bk_display_bus_ctlr_t *controller, uint8_t *frame, flush_free_cb_t cb)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);
    lcd_spi_display_task_send_msg(&bus->spi_context, LCD_SPI_DISP_REQUEST, (uint32_t)frame, (uint32_t)cb);
    return AVDK_ERR_OK;
}

static avdk_err_t spi_hw_delete(bk_display_bus_ctlr_t *controller)
{
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    spi_bus_vn_ctlr_t *bus = __containerof(controller, spi_bus_vn_ctlr_t, ops);
    bk_lcd_spi_display_close(&bus->spi_context);
    os_free(bus);
    return AVDK_ERR_OK;
}

#endif /* CONFIG_LCD_SPI */

avdk_err_t bk_display_spi_bus_new(bk_display_bus_handle_t *handle, bk_display_spi_bus_config_t *config)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);

    spi_bus_vn_ctlr_t *bus = os_malloc(sizeof(spi_bus_vn_ctlr_t));
    AVDK_RETURN_ON_FALSE(bus, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);

    os_memset(bus, 0, sizeof(spi_bus_vn_ctlr_t));
    os_memcpy(&bus->config, config, sizeof(bk_display_spi_bus_config_t));

    switch (config->mode) {

    case BK_DISPLAY_SPI_BUS_MODE_SW:
        if (config->clk_pin == 0 || config->csx_pin == 0 || config->sda_pin == 0) {
            LOGE("SW mode requires clk/csx/sda pins (got %u/%u/%u)\n",
                 config->clk_pin, config->csx_pin, config->sda_pin);
            os_free(bus);
            return AVDK_ERR_INVAL;
        }
        if (config->cmd_width != 8 && config->cmd_width != 16) {
            LOGE("SW mode requires cmd_width == 8 or 16 (got %u)\n", config->cmd_width);
            os_free(bus);
            return AVDK_ERR_INVAL;
        }
        spi_sw_gpio_init(config->sda_pin, config->clk_pin, config->csx_pin);
        bus->ops.tx_param = spi_sw_tx_param;
        bus->ops.rx_param = NULL;            /* SW SPI is write-only */
        bus->ops.delete   = spi_sw_delete;
        break;

    case BK_DISPLAY_SPI_BUS_MODE_HW:
#if CONFIG_LCD_SPI
        if (config->lcd_panel == NULL) {
            LOGE("HW mode requires config->lcd_panel\n");
            os_free(bus);
            return AVDK_ERR_INVAL;
        }
        if (bk_lcd_spi_display_open(&bus->spi_context, &bus->config) != BK_OK) {
            LOGE("HW mode bring-up failed\n");
            os_free(bus);
            return AVDK_ERR_GENERIC;
        }
        bus->ops.flush  = spi_hw_flush;
        bus->ops.delete = spi_hw_delete;
        break;
#else
        LOGE("HW mode requested but CONFIG_LCD_SPI is not enabled\n");
        os_free(bus);
        return AVDK_ERR_UNSUPPORTED;
#endif

    default:
        LOGE("invalid SPI bus mode %d\n", (int)config->mode);
        os_free(bus);
        return AVDK_ERR_INVAL;
    }

    *handle = &(bus->ops);
    return AVDK_ERR_OK;
}
