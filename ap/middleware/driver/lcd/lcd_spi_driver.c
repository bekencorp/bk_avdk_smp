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
#include <driver/hpdma.h>
#include <driver/gpio.h>
#include "gpio_driver.h"
#include <driver/lcd_types.h>
#include <driver/lcd_qspi.h>
#include <driver/lcd_qspi_types.h>
#include <driver/qspi.h>
#include <driver/qspi_types.h>
#include <driver/spi.h>
#include <driver/dma.h>
#include "bk_general_dma.h"
#include "bk_misc.h"


#define LCD_SPI_TAG "lcd_spi_drv"

#define LCD_SPI_LOGI(...) BK_LOGI(LCD_SPI_TAG, ##__VA_ARGS__)
#define LCD_SPI_LOGW(...) BK_LOGW(LCD_SPI_TAG, ##__VA_ARGS__)
#define LCD_SPI_LOGE(...) BK_LOGE(LCD_SPI_TAG, ##__VA_ARGS__)
#define LCD_SPI_LOGD(...) BK_LOGD(LCD_SPI_TAG, ##__VA_ARGS__)

#define LCD_SPI_DEVICE_CASET        0x2A
#define LCD_SPI_DEVICE_RASET        0x2B
#define LCD_SPI_DEVICE_RAMWR        0x2C

typedef struct {
    uint8_t dma_id;
    void *dma_list_table;
    beken_semaphore_t dma_sema;
    hpdma_isr_t dma_finish_isr;
    bool lcd_qspi_is_init;
    uint32_t qspi_data;
    uint8_t dc_pin;
    uint16_t width;
    uint16_t height;
} lcd_spi_disp_t;

static void lcd_spi0_dma_finish_isr(hpdma_id_t dma_id, void *user_data);
static void lcd_spi1_dma_finish_isr(hpdma_id_t dma_id, void *user_data);

static lcd_spi_disp_t s_spi_disp[SOC_QSPI_UNIT_NUM] = {
    {
        .dma_finish_isr = lcd_spi0_dma_finish_isr,
        .lcd_qspi_is_init = false,
        .qspi_data = LCD_QSPI0_DATA_ADDR,
    },
    {
        .dma_finish_isr = lcd_spi1_dma_finish_isr,
        .lcd_qspi_is_init = false,
        .qspi_data = LCD_QSPI1_DATA_ADDR,
    },
};

static void lcd_spi_dma_finish_handler(qspi_id_t qspi_id, dma_id_t dma_id)
{
    bk_err_t ret = BK_OK;

    if (s_spi_disp[qspi_id].dma_sema != NULL) {
        ret = rtos_set_semaphore(&s_spi_disp[qspi_id].dma_sema);
        if (ret != BK_OK) {
            LCD_SPI_LOGE("lcd qspi dma semaphore set failed\r\n");
            return;
        }
    }

}

static void lcd_spi0_dma_finish_isr(hpdma_id_t dma_id, void *user_data)
{
    lcd_spi_dma_finish_handler(QSPI_ID_0, dma_id);
}

static void lcd_spi1_dma_finish_isr(hpdma_id_t dma_id, void *user_data)
{
    lcd_spi_dma_finish_handler(QSPI_ID_1, dma_id);
}

static void lcd_spi_device_gpio_init(uint8_t reset_pin, uint8_t dc_pin)
{
    BK_LOG_ON_ERR(bk_gpio_driver_init());

    /* The following set_output_high/low forms the LCD reset pulse sequence
     * and the DC idle level. These are runtime control and MUST always be
     * executed. */
    BK_LOG_ON_ERR(bk_gpio_set_output_high(reset_pin));
    BK_LOG_ON_ERR(bk_gpio_set_output_high(dc_pin));

    BK_LOG_ON_ERR(bk_gpio_set_output_low(reset_pin));
    rtos_delay_milliseconds(100);
    BK_LOG_ON_ERR(bk_gpio_set_output_high(reset_pin));
    rtos_delay_milliseconds(120);
}

static void lcd_spi_device_gpio_deinit(uint8_t reset_pin, uint8_t dc_pin)
{
    BK_LOG_ON_ERR(gpio_dev_unmap(reset_pin));
    BK_LOG_ON_ERR(gpio_dev_unmap(dc_pin));
}

static qspi_driver_t s_lcd_spi[SOC_QSPI_UNIT_NUM] =
{
    {
        .hal.hw = (qspi_hw_t *)(SOC_QSPI0_REG_BASE),
    },
#if (SOC_QSPI_UNIT_NUM > 1)
    {
        .hal.hw = (qspi_hw_t *)(SOC_QSPI1_REG_BASE),
    }
#endif
};

static void lcd_spi_dma_init(qspi_id_t qspi_id)
{
    bk_err_t ret = BK_OK;

    ret = rtos_init_semaphore(&s_spi_disp[qspi_id].dma_sema, 1);
    if (ret != kNoErr) {
        LCD_SPI_LOGE("lcd qspi dma semaphore init failed.\r\n");
        return;
    }

    s_spi_disp[qspi_id].dma_list_table = bk_hpdma_link_init(1);
    if (s_spi_disp[qspi_id].dma_list_table == NULL) {
        LCD_SPI_LOGE("%s dma list table malloc failed!\r\n", __func__);
        return;
    }

    s_spi_disp[qspi_id].dma_id = bk_hpdma_alloc(DMA_DEV_DTCM);
    if ((s_spi_disp[qspi_id].dma_id < HPDMA_ID_0) || (s_spi_disp[qspi_id].dma_id >= HPDMA_ID_MAX)) {
        LCD_SPI_LOGE("%s dma id malloc failed!\r\n", __func__);
        return;
    }

#if (CONFIG_SPE)
    bk_hpdma_set_src_sec_attr(s_spi_disp[qspi_id].dma_id, DMA_ATTR_SEC);
    bk_hpdma_set_dest_sec_attr(s_spi_disp[qspi_id].dma_id, DMA_ATTR_SEC);
    // bk_hpdma_set_dest_burst_len(s_spi_disp[qspi_id].dma_id, HPDMA_BURST_LEN_INC8);
    // bk_hpdma_set_dest_burst_len(s_spi_disp[qspi_id].dma_id, HPDMA_BURST_LEN_INC8);
#endif
}

static void lcd_spi_dma_deinit(qspi_id_t qspi_id)
{
    bk_err_t ret = BK_OK;

    bk_hpdma_stop(s_spi_disp[qspi_id].dma_id);

    bk_hpdma_disable_finish_interrupt(s_spi_disp[qspi_id].dma_id);

    bk_hpdma_link_deinit(s_spi_disp[qspi_id].dma_list_table);
    ret = bk_hpdma_free(DMA_DEV_DTCM, s_spi_disp[qspi_id].dma_id);
    if (ret != BK_OK) {
        LCD_SPI_LOGE("%s dma id free failed.\r\n", __func__);
        return;
    }

    ret = rtos_deinit_semaphore(&s_spi_disp[qspi_id].dma_sema);
    if (ret != kNoErr) {
        LCD_SPI_LOGE("%s dma semaphore deinit failed.\r\n", __func__);
        return;
    }
}

static bk_err_t lcd_spi_quad_write_start(qspi_id_t qspi_id)
{
    qspi_hal_force_spi_cs_low_enable(&s_lcd_spi[qspi_id].hal);

    qspi_hal_io_cpu_mem_select(&s_lcd_spi[qspi_id].hal, 1);

    qspi_hal_disable_cmd_sck_enable(&s_lcd_spi[qspi_id].hal);

    return BK_OK;
}

static bk_err_t lcd_spi_quad_write_stop(qspi_id_t qspi_id)
{
    qspi_hal_disable_cmd_sck_disable(&s_lcd_spi[qspi_id].hal);
    qspi_hal_force_spi_cs_low_disable(&s_lcd_spi[qspi_id].hal);

    qspi_hal_io_cpu_mem_select(&s_lcd_spi[qspi_id].hal, 0);

    return BK_OK;
}

static void lcd_spi_dma_start(qspi_id_t qspi_id, void *data, uint16_t xsize, uint16_t ysize)
{
    hpdma_link_config_t config = {0};

    config.src_addr = (uint32_t)data;
    config.dst_addr = (uint32_t)s_spi_disp[qspi_id].qspi_data;
    config.src_xsize = xsize * CONFIG_LCD_SPI_COLOR_DEPTH_BYTE;
    config.src_ysize = ysize;
    config.dst_xsize = xsize * CONFIG_LCD_SPI_COLOR_DEPTH_BYTE;
    config.dst_ysize = ysize;
    config.src_step = 0;
    config.dst_step = 0;
    config.finish_int_en = 1;
    config.half_finish_int_en = 0;

    bk_err_t ret = bk_hpdma_link_set_desc(s_spi_disp[qspi_id].dma_list_table, 0, &config);
    if (ret != BK_OK) {
        LCD_SPI_LOGE("%s bk_hpdma_link_set_desc failed, ret=%d\n", __func__, ret);
        return;
    }

    bk_hpdma_register_isr(s_spi_disp[qspi_id].dma_id, NULL, NULL, s_spi_disp[qspi_id].dma_finish_isr, NULL);
    bk_hpdma_enable_finish_interrupt(s_spi_disp[qspi_id].dma_id);

    ret = bk_hpdma_link_transfer(s_spi_disp[qspi_id].dma_id, s_spi_disp[qspi_id].dma_list_table);
    if (ret != BK_OK) {
        LCD_SPI_LOGE("%s bk_hpdma_link_transfer failed, ret=%d\n", __func__, ret);
        return;
    }
}

static void lcd_spi_send_data_with_qspi_mapping_mode(qspi_id_t qspi_id, uint8_t *data, uint32_t data_len)
{
    if (s_spi_disp[qspi_id].width * s_spi_disp[qspi_id].height * CONFIG_LCD_SPI_COLOR_DEPTH_BYTE != data_len) {
        LCD_SPI_LOGE("data length is not equal to the width * height\r\n");
        return;
    }

    lcd_spi_quad_write_start(qspi_id);
    lcd_spi_dma_start(qspi_id, data, s_spi_disp[qspi_id].width, s_spi_disp[qspi_id].height);
}

static void lcd_spi_driver_init_with_qspi(qspi_id_t qspi_id, lcd_qspi_clk_t clk)
{
    BK_LOG_ON_ERR(bk_qspi_driver_init());

    qspi_config_t lcd_qspi_config;
    os_memset(&lcd_qspi_config, 0, sizeof(lcd_qspi_config));

    s_lcd_spi[qspi_id].hal.id = qspi_id;
    qspi_hal_init(&s_lcd_spi[qspi_id].hal);

    switch (clk) {
        case LCD_QSPI_80M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 2;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_60M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 3;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_53M:
            lcd_qspi_config.src_clk = QSPI_SCLK_160M;
            lcd_qspi_config.src_clk_div = 2;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_48M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 4;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_40M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 5;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_32M:
            lcd_qspi_config.src_clk = QSPI_SCLK_160M;
            lcd_qspi_config.src_clk_div = 4;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_30M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 7;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        case LCD_QSPI_24M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 9;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;

        default:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 5;
            BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &lcd_qspi_config));
            break;
    }
    qspi_hal_disable_soft_reset(&s_lcd_spi[qspi_id].hal);
    bk_delay_us(10);
    qspi_hal_enable_soft_reset(&s_lcd_spi[qspi_id].hal);

    qspi_hal_set_cmd_a_cfg2(&s_lcd_spi[qspi_id].hal, 0x80000000);
}

static void lcd_spi_driver_deinit_with_qspi(qspi_id_t qspi_id)
{
    qspi_hal_set_cmd_a_cfg2(&s_lcd_spi[qspi_id].hal, 0);

    BK_LOG_ON_ERR(bk_qspi_deinit(qspi_id));
}

static void lcd_spi_send_data_with_qspi_cmd_c(qspi_id_t qspi_id, uint8_t *data, uint32_t data_len)
{
    uint32_t value = 0;

    qspi_hal_set_cmd_c_l(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_lcd_spi[qspi_id].hal, 0);

    for (uint8_t i = 0; i < data_len; i++) {
        value |= (data[i] << (i * 8));
    }

    qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, value);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0x3 << (data_len * 2));
    qspi_hal_cmd_c_start(&s_lcd_spi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_spi[qspi_id].hal);
}

static void lcd_spi_send_data_with_qspi_indirect_mode(qspi_id_t qspi_id, uint8_t *data, uint32_t data_len)
{
    uint32_t value = 0;
    uint32_t send_len = 0;
    uint32_t remain_len = data_len;
    uint8_t *data_tmp = data;

    qspi_hal_set_cmd_c_l(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_lcd_spi[qspi_id].hal, 0);

    while (remain_len > 0) {
        if (remain_len <= 4) {
            lcd_spi_send_data_with_qspi_cmd_c(qspi_id, data_tmp, remain_len);
            break;
        }

        value = (data_tmp[3] << 24) | (data_tmp[2] << 16) | (data_tmp[1] << 8) | data_tmp[0];
        remain_len -= 4;
        send_len = remain_len < 0x100 ? remain_len : 0x100;
        qspi_hal_set_cmd_c_h(&s_lcd_spi[qspi_id].hal, value);
        qspi_hal_set_cmd_c_cfg1(&s_lcd_spi[qspi_id].hal, 0x300);
        qspi_hal_set_cmd_c_cfg2(&s_lcd_spi[qspi_id].hal, send_len << 2);
        data_tmp += 4;
        bk_qspi_write(qspi_id, data_tmp, send_len);
        qspi_hal_cmd_c_start(&s_lcd_spi[qspi_id].hal);
        qspi_hal_wait_cmd_done(&s_lcd_spi[qspi_id].hal);
        remain_len -= send_len;
        data_tmp += send_len;
    }
}


#if CONFIG_LCD_SPI_REFRESH_WITH_SPI
static spi_config_t config = {0};

static void lcd_spi_driver_init(spi_id_t id)
{
    bk_spi_driver_init();

    os_memset(&config, 0, sizeof(spi_config_t));

    config.role = SPI_ROLE_MASTER;
    config.bit_width = SPI_BIT_WIDTH_8BITS;
    config.polarity = SPI_POLARITY_HIGH;
    config.phase = SPI_PHASE_2ND_EDGE;
    config.wire_mode = SPI_4WIRE_MODE;
    config.baud_rate = 40000000;
    config.bit_order = SPI_MSB_FIRST;

#if CONFIG_SPI_DMA
    config.dma_mode = SPI_DMA_MODE_ENABLE;
    config.spi_tx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0);
    config.spi_rx_dma_chan = bk_dma_alloc(DMA_DEV_GSPI0_RX);
    config.spi_tx_dma_width = DMA_DATA_WIDTH_8BITS;
    config.spi_rx_dma_width = DMA_DATA_WIDTH_8BITS;
#endif

    BK_LOG_ON_ERR(bk_spi_init(id, &config));
}

static void lcd_spi_driver_deinit(spi_id_t id)
{
    BK_LOG_ON_ERR(bk_spi_deinit(id));

#if CONFIG_SPI_DMA
    bk_dma_free(DMA_DEV_GSPI0, config.spi_tx_dma_chan);
    bk_dma_free(DMA_DEV_GSPI0_RX, config.spi_rx_dma_chan);
#endif
}
#endif

void bk_lcd_spi_send_cmd(uint8_t id, uint8_t cmd)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_low(s_spi_disp[id].dc_pin));

#if CONFIG_LCD_SPI_REFRESH_WITH_SPI
    bk_spi_write_bytes(id, &cmd, 1);
#else
    lcd_spi_send_data_with_qspi_indirect_mode(id, &cmd, 1);
#endif
}

void bk_lcd_spi_send_data(uint8_t id, uint8_t *data, uint32_t data_len)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_high(s_spi_disp[id].dc_pin));

#if CONFIG_LCD_SPI_REFRESH_WITH_SPI
#if CONFIG_SPI_DMA
    if (data_len > 32) {
        bk_spi_dma_write_bytes(id, data, data_len);
    } else {
        bk_spi_write_bytes(id, data, data_len);
    }
#else
    bk_spi_write_bytes(id, data, data_len);
#endif

#else
    lcd_spi_send_data_with_qspi_indirect_mode(id, data, data_len);
#endif
}

void bk_lcd_spi_send_data_with_qspi_mapping_mode(uint8_t id, uint8_t *data, uint32_t data_len)
{
    BK_LOG_ON_ERR(bk_gpio_set_output_high(s_spi_disp[id].dc_pin));

    lcd_spi_send_data_with_qspi_mapping_mode(id, data, data_len);
}

bk_err_t bk_lcd_spi_wait_display_complete(qspi_id_t qspi_id)
{
    bk_err_t ret = BK_OK;

#if (!CONFIG_LCD_SPI_REFRESH_WITH_SPI) && CONFIG_LCD_SPI_REFRESH_WITH_QSPI_MAPPING_MODE
    if (s_spi_disp[qspi_id].dma_sema) {
        ret = rtos_get_semaphore(&s_spi_disp[qspi_id].dma_sema, 5000);
        if (ret != kNoErr) {
            LCD_SPI_LOGE("ret = %d, lcd spi get semaphore failed!\r\n", ret);
            return BK_FAIL;
        }
        bk_delay_us(15);
        lcd_spi_quad_write_stop(qspi_id);
    }
#else
    (void)qspi_id;
#endif

    return ret;
}

static void lcd_spi_disp_area_config(uint8_t id, lcd_display_area_t *area)
{
    uint8_t column_value[4] = {0};
    uint8_t row_value[4] = {0};

    column_value[0] = (area->x_start >> 8) & 0xFF;
    column_value[1] = area->x_start & 0xFF;
    column_value[2] = (area->x_end >> 8) & 0xFF;
    column_value[3] = area->x_end & 0xFF;
    row_value[0] = (area->y_start >> 8) & 0xFF;
    row_value[1] = area->y_start & 0xFF;
    row_value[2] = (area->y_end >> 8) & 0xFF;
    row_value[3] = area->y_end & 0xFF;

    s_spi_disp[id].width = area->x_end - area->x_start + 1;
    s_spi_disp[id].height = area->y_end - area->y_start + 1;

    bk_lcd_spi_send_cmd(id, LCD_SPI_DEVICE_CASET);
    bk_lcd_spi_send_data(id, column_value, 4);
    bk_lcd_spi_send_cmd(id, LCD_SPI_DEVICE_RASET);
    bk_lcd_spi_send_data(id, row_value, 4);
}

void bk_lcd_spi_init(uint8_t id, const bk_display_spi_panel_t *device, uint8_t reset_pin, uint8_t dc_pin)
{
    if (device == NULL) {
        LCD_SPI_LOGE("lcd spi device not found\r\n");
        return;
    }

    if (s_spi_disp[id].lcd_qspi_is_init) {
        LCD_SPI_LOGE("lcd qspi init already complete\r\n");
        return;
    }

#if CONFIG_LCD_SPI_REFRESH_WITH_SPI
    lcd_spi_driver_init(id);
#elif CONFIG_LCD_SPI_REFRESH_WITH_QSPI_MAPPING_MODE
    lcd_spi_dma_init(id);
    lcd_spi_driver_init_with_qspi(id, device->spi->clk);
#else
    lcd_spi_driver_init_with_qspi(id, device->spi->clk);
#endif

    lcd_spi_device_gpio_init(reset_pin, dc_pin);
    s_spi_disp[id].dc_pin = dc_pin;

    if (device->spi->init_cmd != NULL) {
        const lcd_qspi_init_cmd_t *init = device->spi->init_cmd;
        for (uint32_t i = 0; i < device->spi->device_init_cmd_len; i++) {
            if (init->data_len == 0xFF) {
                rtos_delay_milliseconds(init->data[0]);
            } else {
                bk_lcd_spi_send_cmd(id, init->cmd);
                if (init->data_len != 0) {
                    bk_lcd_spi_send_data(id, (uint8_t *)init->data, init->data_len);
                }
            }
            init++;
        }
    } else {
        LCD_SPI_LOGE("lcd spi device init cmd is null\r\n");
        return;
    }

    s_spi_disp[id].width = device->width;
    s_spi_disp[id].height = device->height;

    lcd_display_area_t disp_area = {0};
    disp_area.x_start = 0;
    disp_area.y_start = 0;
    disp_area.x_end = device->width - 1;
    disp_area.y_end = device->height - 1;
    lcd_spi_disp_area_config(id, &disp_area);

    s_spi_disp[id].lcd_qspi_is_init = true;

    LCD_SPI_LOGI("%s[%d] is complete\r\n", __func__, id);
}

void bk_lcd_spi_deinit(uint8_t id, uint8_t reset_pin, uint8_t dc_pin)
{
    lcd_spi_device_gpio_deinit(reset_pin, dc_pin);

#if CONFIG_LCD_SPI_REFRESH_WITH_SPI
    lcd_spi_driver_deinit(id);
#elif CONFIG_LCD_SPI_REFRESH_WITH_QSPI_MAPPING_MODE
    lcd_spi_dma_deinit(id);
    lcd_spi_driver_deinit_with_qspi(id);
#else
    lcd_spi_driver_deinit_with_qspi(id);
#endif

    s_spi_disp[id].lcd_qspi_is_init = false;

    LCD_SPI_LOGI("%s is complete\r\n", __func__);
}

bk_err_t bk_lcd_spi_frame_display(uint8_t id, uint8_t *data, uint32_t data_len)
{
    bk_lcd_spi_send_cmd(id, LCD_SPI_DEVICE_RAMWR);

#if CONFIG_LCD_SPI_REFRESH_WITH_SPI
    bk_lcd_spi_send_data(id, data, data_len);
#elif CONFIG_LCD_SPI_REFRESH_WITH_QSPI_MAPPING_MODE
    bk_lcd_spi_send_data_with_qspi_mapping_mode(id, data, data_len);
#else
    bk_lcd_spi_send_data(id, data, data_len);
#endif

    return BK_OK;
}

bk_err_t bk_lcd_spi_partial_display(uint8_t id, lcd_display_area_t *area, uint8_t *data)
{
    lcd_spi_disp_area_config(id, area);

    bk_lcd_spi_send_cmd(id, LCD_SPI_DEVICE_RAMWR);

#if CONFIG_LCD_SPI_REFRESH_WITH_QSPI_MAPPING_MODE
    bk_lcd_spi_send_data_with_qspi_mapping_mode(id, data, (area->x_end - area->x_start + 1) * (area->y_end - area->y_start + 1) * CONFIG_LCD_SPI_COLOR_DEPTH_BYTE);
#else
    bk_lcd_spi_send_data(id, data, (area->x_end - area->x_start + 1) * (area->y_end - area->y_start + 1) * CONFIG_LCD_SPI_COLOR_DEPTH_BYTE);
#endif

    return BK_OK;
}
