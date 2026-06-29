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

#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/qspi.h>
#include <driver/qspi_types.h>
#include <driver/lcd_types.h>
#include <driver/lcd_qspi.h>
#include <driver/lcd_qspi_types.h>
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "bk_misc.h"
#include <driver/hpdma.h>
 

#define LCD_QSPI_TAG "lcd_qspi_drv"

#define LCD_QSPI_LOGI(...) BK_LOGI(LCD_QSPI_TAG, ##__VA_ARGS__)
#define LCD_QSPI_LOGW(...) BK_LOGW(LCD_QSPI_TAG, ##__VA_ARGS__)
#define LCD_QSPI_LOGE(...) BK_LOGE(LCD_QSPI_TAG, ##__VA_ARGS__)
#define LCD_QSPI_LOGD(...) BK_LOGD(LCD_QSPI_TAG, ##__VA_ARGS__)

#define LCD_QSPI_DEVICE_CASET        0x2A
#define LCD_QSPI_DEVICE_RASET        0x2B

#define LCD_QSPI_FIFO_WRITE_MAX      QSPI_FIFO_LEN_MAX
#define LCD_QSPI_CMD_C_LEN_MAX       8
#define LCD_QSPI_CMD_C_DATA_LINE_4WIRE   (QSPI_4WIRE << 14)

static void lcd_qspi0_dma_finish_isr(hpdma_id_t dma_id, void *user_data);
static void lcd_qspi1_dma_finish_isr(hpdma_id_t dma_id, void *user_data);

static qspi_driver_t s_lcd_qspi[SOC_QSPI_UNIT_NUM] = {
    {
        .hal.hw = (qspi_hw_t *)(SOC_QSPI0_REG_BASE),
    },
#if (SOC_QSPI_UNIT_NUM > 1)
    {
        .hal.hw = (qspi_hw_t *)(SOC_QSPI1_REG_BASE),
    }
#endif
};

static lcd_qspi_disp_t s_qspi_disp[SOC_QSPI_UNIT_NUM] = {
    {
        .dma_finish_isr = lcd_qspi0_dma_finish_isr,
        .lcd_qspi_is_init = false,
        .qspi_data = LCD_QSPI0_DATA_ADDR,
    },
    {
        .dma_finish_isr = lcd_qspi1_dma_finish_isr,
        .lcd_qspi_is_init = false,
        .qspi_data = LCD_QSPI1_DATA_ADDR,
    },
};

static void lcd_qspi_dma_finish_handler(qspi_id_t qspi_id, hpdma_id_t dma_id)
{
    bk_err_t ret = BK_OK;

    (void)dma_id;

    if (s_qspi_disp[qspi_id].dma_sema != NULL) {
        ret = rtos_set_semaphore(&s_qspi_disp[qspi_id].dma_sema);
        if (ret != BK_OK) {
            LCD_QSPI_LOGE("lcd qspi dma semaphore set failed\r\n");
            return;
        }
    }
}

static void lcd_qspi0_dma_finish_isr(hpdma_id_t dma_id, void *user_data)
{
    (void)user_data;
    lcd_qspi_dma_finish_handler(QSPI_ID_0, dma_id);
}

static void lcd_qspi1_dma_finish_isr(hpdma_id_t dma_id, void *user_data)
{
    (void)user_data;
    lcd_qspi_dma_finish_handler(QSPI_ID_1, dma_id);
}

static bk_err_t lcd_qspi_driver_init(qspi_id_t qspi_id, lcd_qspi_clk_t clk)
{
    bk_err_t ret = BK_OK;
    qspi_config_t lcd_qspi_config;
    os_memset(&lcd_qspi_config, 0, sizeof(lcd_qspi_config));

    s_lcd_qspi[qspi_id].hal.id = qspi_id;
    qspi_hal_init(&s_lcd_qspi[qspi_id].hal);

    switch (clk) {
        case LCD_QSPI_80M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 2;
            break;

        case LCD_QSPI_60M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 3;
            break;

        case LCD_QSPI_53M:
            lcd_qspi_config.src_clk = QSPI_SCLK_160M;
            lcd_qspi_config.src_clk_div = 2;
            break;

        case LCD_QSPI_48M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 4;
            break;

        case LCD_QSPI_40M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 5;
            break;

        case LCD_QSPI_32M:
            lcd_qspi_config.src_clk = QSPI_SCLK_160M;
            lcd_qspi_config.src_clk_div = 4;
            break;

        case LCD_QSPI_30M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 7;
            break;

        case LCD_QSPI_24M:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 9;
            break;

        default:
            lcd_qspi_config.src_clk = QSPI_SCLK_240M;
            lcd_qspi_config.src_clk_div = 5;
            break;
    }

    ret = bk_qspi_init(qspi_id, &lcd_qspi_config);
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("bk_qspi_init failed, qspi_id=%d, ret=%d\r\n", qspi_id, ret);
        return ret;
    }

    qspi_hal_disable_soft_reset(&s_lcd_qspi[qspi_id].hal);
    bk_delay_us(10);
    qspi_hal_enable_soft_reset(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_init_common(&s_lcd_qspi[qspi_id].hal);

#if CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING
    qspi_hal_set_cmd_a_cfg2(&s_lcd_qspi[qspi_id].hal, 0x80008000);
#endif

    return BK_OK;
}

static bk_err_t lcd_qspi_hardware_reset(gpio_id_t reset_pin)
{
    gpio_id_t gpio_id = reset_pin;

    BK_LOG_ON_ERR(bk_gpio_set_output_high(gpio_id));
    rtos_delay_milliseconds(10);
    BK_LOG_ON_ERR(bk_gpio_set_output_low(gpio_id));
    rtos_delay_milliseconds(10);
    BK_LOG_ON_ERR(bk_gpio_set_output_high(gpio_id));
    rtos_delay_milliseconds(120);

    return BK_OK;
}

static bk_err_t lcd_qspi_reg_data_convert(qspi_id_t qspi_id, uint8_t *data, uint32_t data_len)
{
    uint32_t data_buffer[data_len];
    uint8_t i, j, data_tmp1;
    uint8_t data_tmp2[4];

    bk_qspi_read(qspi_id, data_buffer, data_len * 4);
    for (i = 0; i < data_len; i++) {
        for (j = 0; j < 4; j++) {
            data_tmp1 = (data_buffer[i] >> ((j * 8) + 4)) & 0x1;
            data_tmp2[j] = (data_tmp1 << 1) | ((data_buffer[i] >> (j * 8)) & 0x1);
        }

        data[i] = (data_tmp2[0] << 6) | (data_tmp2[1] << 4) | (data_tmp2[2] << 2) | (data_tmp2[3]);
    }

    return BK_OK;
}

static bk_err_t lcd_qspi_refresh_by_line_lcd_head_config(qspi_id_t qspi_id, const bk_display_qspi_panel_t *device)
{
    uint8_t *cmd = NULL;
    uint32_t head_cmd[4];
    uint8_t i;

    cmd = device->qspi->pixel_write_config.cmd;
    for (i = 0; i < device->qspi->pixel_write_config.cmd_len; i++) {
        if (0 == cmd[i]) {
            head_cmd[i] = 0x0;
            continue;
        } else {
            uint8_t cmd_temp = cmd[i];
            cmd_temp = ((cmd_temp >> 4) & 0x0F) | ((cmd_temp << 4) & 0xF0);
            cmd_temp = ((cmd_temp >> 2) & 0x33) | ((cmd_temp << 2) & 0xCC);
            cmd_temp = ((cmd_temp >> 1) & 0x55) | ((cmd_temp << 1) & 0xAA);

            head_cmd[i] = ((cmd_temp << 21) & 0x10000000) | ((cmd_temp << 18) & 0x01000000) |
                        ((cmd_temp << 15) & 0x00100000) | ((cmd_temp << 12) & 0x00010000) |
                        ((cmd_temp << 9) & 0x00001000) | ((cmd_temp << 6) & 0x00000100) |
                        ((cmd_temp << 3) & 0x00000010) | (cmd_temp & 0x00000001);
        }
    }

    qspi_hal_set_lcd_head_cmd0(&s_lcd_qspi[qspi_id].hal, head_cmd[0]);
    qspi_hal_set_lcd_head_cmd1(&s_lcd_qspi[qspi_id].hal, head_cmd[1]);
    qspi_hal_set_lcd_head_cmd2(&s_lcd_qspi[qspi_id].hal, head_cmd[2]);
    qspi_hal_set_lcd_head_cmd3(&s_lcd_qspi[qspi_id].hal, head_cmd[3]);
    qspi_hal_set_lcd_head_resolution(&s_lcd_qspi[qspi_id].hal, device->qspi->refresh_config.line_len * 2, device->height);

    qspi_hal_enable_lcd_head_selection_without_ram(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_set_lcd_head_len(&s_lcd_qspi[qspi_id].hal, 0x20);
    qspi_hal_set_lcd_head_dly(&s_lcd_qspi[qspi_id].hal, (device->qspi->clk >> 1) + 6);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_read_data(qspi_id_t qspi_id, uint8_t *data, const bk_display_qspi_panel_t *device, uint8_t regist_addr, uint8_t data_len)
{
    qspi_hal_set_cmd_d_h(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_d_cfg1(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_d_cfg2(&s_lcd_qspi[qspi_id].hal, 0);

    qspi_hal_set_cmd_d_h(&s_lcd_qspi[qspi_id].hal, (regist_addr << 16 | device->qspi->reg_read_cmd) & 0xFF00FF);
    qspi_hal_set_cmd_d_cfg1(&s_lcd_qspi[qspi_id].hal, 0x300);

    qspi_hal_set_cmd_d_data_line(&s_lcd_qspi[qspi_id].hal, 2);
    qspi_hal_set_cmd_d_data_length(&s_lcd_qspi[qspi_id].hal, data_len * 4);
    qspi_hal_set_cmd_d_dummy_clock(&s_lcd_qspi[qspi_id].hal, device->qspi->reg_read_config.dummy_clk);
    qspi_hal_set_cmd_d_dummy_mode(&s_lcd_qspi[qspi_id].hal, device->qspi->reg_read_config.dummy_mode);

    qspi_hal_cmd_d_start(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_qspi[qspi_id].hal);

    lcd_qspi_reg_data_convert(qspi_id, data, data_len);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_send_cmd(qspi_id_t qspi_id, uint8_t write_cmd, uint8_t cmd, const uint8_t *data, uint8_t data_len)
{
    qspi_hal_set_cmd_c_l(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_lcd_qspi[qspi_id].hal, 0);

    if (data_len == 0) {
        qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, (cmd << 16 | write_cmd) & 0xFF00FF);
    } else if (data_len > 0 && data_len <= 4) {
        uint32_t value = 0;
        for (uint8_t i = 0; i < data_len; i++) {
            value = value | (data[i] << (i * 8));
        }
        qspi_hal_set_cmd_c_l(&s_lcd_qspi[qspi_id].hal, value);
        qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, (cmd << 16 | write_cmd) & 0xFF00FF);
    } else if (data_len > 4 && data_len != 0xFF) {
        qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, (cmd << 16 | write_cmd) & 0xFF00FF);
        qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0x300);
        qspi_hal_set_cmd_c_cfg2(&s_lcd_qspi[qspi_id].hal, data_len << 2);
        bk_qspi_write(qspi_id, data, data_len);
        qspi_hal_cmd_c_start(&s_lcd_qspi[qspi_id].hal);
        qspi_hal_wait_cmd_done(&s_lcd_qspi[qspi_id].hal);
        qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0);
        qspi_hal_set_cmd_c_cfg2(&s_lcd_qspi[qspi_id].hal, 0);
        return BK_OK;
    }

    qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0x3 << ((data_len + 4) * 2));
    qspi_hal_cmd_c_start(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_qspi[qspi_id].hal);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_quad_write_start(qspi_id_t qspi_id, lcd_qspi_write_config_t reg_config, bool addr_is_4wire)
{
    uint32_t cmd_c_h = 0;

    qspi_hal_force_spi_cs_low_enable(&s_lcd_qspi[qspi_id].hal);

    qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, 0);

    for (uint8_t i = 0; i < reg_config.cmd_len; i++) {
        cmd_c_h = qspi_hal_get_cmd_c_h(&s_lcd_qspi[qspi_id].hal);
        cmd_c_h |= ((reg_config.cmd[i]) << i * 8);
        qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, cmd_c_h);
    }

    if (addr_is_4wire) {
        qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0x3A8);
    } else {
        qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0x300);
    }
    qspi_hal_cmd_c_start(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_qspi[qspi_id].hal);

    qspi_hal_io_cpu_mem_select(&s_lcd_qspi[qspi_id].hal, 1);
    qspi_hal_disable_cmd_sck_enable(&s_lcd_qspi[qspi_id].hal);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_quad_write_stop(qspi_id_t qspi_id)
{
    qspi_hal_disable_cmd_sck_disable(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_force_spi_cs_low_disable(&s_lcd_qspi[qspi_id].hal);

    qspi_hal_io_cpu_mem_select(&s_lcd_qspi[qspi_id].hal, 0);

    return BK_OK;
}

static void lcd_qspi_disp_area_config(qspi_id_t qspi_id, const bk_display_qspi_panel_t *device, lcd_display_area_t *area)
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

    bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, LCD_QSPI_DEVICE_CASET, column_value, 4);
    bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, LCD_QSPI_DEVICE_RASET, row_value, 4);
}

static void lcd_qspi_disp_full_area_config(qspi_id_t qspi_id, const bk_display_qspi_panel_t *device)
{
    lcd_display_area_t disp_area = {0};

    disp_area.x_start = 0;
    disp_area.y_start = 0;
    disp_area.x_end = device->width - 1;
    disp_area.y_end = device->height - 1;
    lcd_qspi_disp_area_config(qspi_id, device, &disp_area);
}

static void lcd_qspi_dma_init(qspi_id_t qspi_id)
{
    bk_err_t ret = BK_OK;

    ret = bk_hpdma_driver_init();
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("hpdma driver init failed!\r\n");
        return;
    }

    ret = rtos_init_semaphore(&s_qspi_disp[qspi_id].dma_sema, 1);
    if (ret != kNoErr) {
        LCD_QSPI_LOGE("lcd qspi dma semaphore init failed.\r\n");
        return;
    }

    s_qspi_disp[qspi_id].dma_list_table = bk_hpdma_link_init(1);
    if (s_qspi_disp[qspi_id].dma_list_table == NULL) {
        LCD_QSPI_LOGE("%s dma list table malloc failed!\r\n", __func__);
        return;
    }

    s_qspi_disp[qspi_id].dma_id = bk_hpdma_alloc(DMA_DEV_DTCM);
    if ((s_qspi_disp[qspi_id].dma_id < HPDMA_ID_0) || (s_qspi_disp[qspi_id].dma_id >= HPDMA_ID_MAX)) {
        LCD_QSPI_LOGE("%s dma id malloc failed!\r\n", __func__);
        return;
    }

#if (CONFIG_SPE)
    bk_hpdma_set_src_sec_attr(s_qspi_disp[qspi_id].dma_id, DMA_ATTR_SEC);
    bk_hpdma_set_dest_sec_attr(s_qspi_disp[qspi_id].dma_id, DMA_ATTR_SEC);
#endif
}

static void lcd_qspi_dma_deinit(qspi_id_t qspi_id)
{
    bk_err_t ret = BK_OK;

    bk_hpdma_stop(s_qspi_disp[qspi_id].dma_id);

    bk_hpdma_disable_finish_interrupt(s_qspi_disp[qspi_id].dma_id);

    bk_hpdma_link_deinit(s_qspi_disp[qspi_id].dma_list_table);
    ret = bk_hpdma_free(DMA_DEV_DTCM, s_qspi_disp[qspi_id].dma_id);
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("%s dma id free failed.\r\n", __func__);
        return;
    }

    ret = rtos_deinit_semaphore(&s_qspi_disp[qspi_id].dma_sema);
    if (ret != kNoErr) {
        LCD_QSPI_LOGE("%s dma semaphore deinit failed.\r\n", __func__);
        return;
    }
}

static bk_err_t lcd_qspi_dma_start(qspi_id_t qspi_id,
                                   const bk_display_qspi_panel_t *device,
                                   uint8_t *data,
                                   uint32_t data_len)
{
    hpdma_link_config_t config = {0};
    uint32_t line_bytes = device->width * CONFIG_LCD_QSPI_COLOR_DEPTH_BYTE;

    if ((data == NULL) || (data_len == 0)) {
        return BK_ERR_PARAM;
    }

    config.src_addr = (uint32_t)data;
    config.dst_addr = s_qspi_disp[qspi_id].qspi_data;

    if ((line_bytes > 0) && ((data_len % line_bytes) == 0)) {
        config.src_xsize = line_bytes;
        config.dst_xsize = line_bytes;
        config.src_ysize = data_len / line_bytes;
        config.dst_ysize = data_len / line_bytes;
    } else if (data_len <= 0xFFFF) {
        config.src_xsize = data_len;
        config.dst_xsize = data_len;
        config.src_ysize = 1;
        config.dst_ysize = 1;
    } else {
        LCD_QSPI_LOGE("%s invalid hpdma size, data_len=%lu, line_bytes=%lu\r\n",
                      __func__, data_len, line_bytes);
        return BK_ERR_PARAM;
    }

    config.src_step = 0;
    config.dst_step = 0;
    config.finish_int_en = 1;
    config.half_finish_int_en = 0;

    bk_err_t ret = bk_hpdma_link_set_desc(s_qspi_disp[qspi_id].dma_list_table, 0, &config);
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("%s bk_hpdma_link_set_desc failed, ret=%d\r\n", __func__, ret);
        return ret;
    }

    bk_hpdma_register_isr(s_qspi_disp[qspi_id].dma_id,
                          NULL,
                          NULL,
                          s_qspi_disp[qspi_id].dma_finish_isr,
                          NULL);
    bk_hpdma_enable_finish_interrupt(s_qspi_disp[qspi_id].dma_id);

    ret = bk_hpdma_link_transfer(s_qspi_disp[qspi_id].dma_id, s_qspi_disp[qspi_id].dma_list_table);
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("%s bk_hpdma_link_transfer failed, ret=%d\r\n", __func__, ret);
    }

    return ret;
}

bk_err_t bk_lcd_qspi_mapping_display(qspi_id_t qspi_id, const bk_display_qspi_panel_t *device, uint32_t *data, uint32_t data_len)
{
#if CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING
    bk_err_t ret = BK_OK;

    bk_lcd_qspi_quad_write_start(qspi_id, device->qspi->pixel_write_config, 0);
    ret = lcd_qspi_dma_start(qspi_id, device, (uint8_t *)data, data_len);
    if (ret != BK_OK) {
        bk_lcd_qspi_quad_write_stop(qspi_id);
        return ret;
    }

    return BK_OK;
#else
    (void)qspi_id;
    (void)device;
    (void)data;
    (void)data_len;
    LCD_QSPI_LOGE("%s mapping mode is disabled\r\n", __func__);
    return BK_FAIL;
#endif
}

static void lcd_qspi_set_cmd_c_byte(uint32_t *cmd_c_h, uint32_t *cmd_c_l, uint8_t index, uint8_t value)
{
    if (index < 4) {
        *cmd_c_h |= ((uint32_t)value << (index * 8));
    } else {
        *cmd_c_l |= ((uint32_t)value << ((index - 4) * 8));
    }
}

static uint32_t lcd_qspi_get_cmd_c_cfg1(uint8_t total_cmd_len, uint8_t data_start_index)
{
    uint32_t cfg1 = 0;

    for (uint8_t i = data_start_index; i < total_cmd_len; i++) {
        cfg1 |= (QSPI_4WIRE << (i * 2));
    }

    if (total_cmd_len < LCD_QSPI_CMD_C_LEN_MAX) {
        cfg1 |= (0x3 << (total_cmd_len * 2));
    }

    return cfg1;
}

static bk_err_t lcd_qspi_indirect_write_cmd_fifo(qspi_id_t qspi_id,
                                                 const uint8_t *cmd,
                                                 uint8_t cmd_len,
                                                 uint8_t data_start_index,
                                                 const uint8_t *fifo_data,
                                                 uint32_t fifo_len)
{
    bk_err_t ret = BK_OK;
    uint32_t cmd_c_l = 0;
    uint32_t cmd_c_h = 0;
    uint32_t fifo_buf[(LCD_QSPI_FIFO_WRITE_MAX + 3) / 4];

    qspi_hal_set_cmd_c_l(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal, 0);
    qspi_hal_set_cmd_c_cfg2(&s_lcd_qspi[qspi_id].hal, 0);

    for (uint8_t i = 0; i < cmd_len; i++) {
        lcd_qspi_set_cmd_c_byte(&cmd_c_h, &cmd_c_l, i, cmd[i]);
    }

    qspi_hal_set_cmd_c_l(&s_lcd_qspi[qspi_id].hal, cmd_c_l);
    qspi_hal_set_cmd_c_h(&s_lcd_qspi[qspi_id].hal, cmd_c_h);
    qspi_hal_set_cmd_c_cfg1(&s_lcd_qspi[qspi_id].hal,
                            lcd_qspi_get_cmd_c_cfg1(cmd_len, data_start_index));
    qspi_hal_set_cmd_c_cfg2(&s_lcd_qspi[qspi_id].hal,
                            (fifo_len << 2) | LCD_QSPI_CMD_C_DATA_LINE_4WIRE);

    if (fifo_len > 0) {
        os_memset(fifo_buf, 0, sizeof(fifo_buf));
        os_memcpy(fifo_buf, fifo_data, fifo_len);

        ret = bk_qspi_write(qspi_id, fifo_buf, fifo_len);
        if (ret != BK_OK) {
            LCD_QSPI_LOGE("%s qspi fifo write failed, ret=%d\r\n", __func__, ret);
            return ret;
        }
    }

    qspi_hal_cmd_c_start(&s_lcd_qspi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_lcd_qspi[qspi_id].hal);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_indirect_display(qspi_id_t qspi_id, const bk_display_qspi_panel_t *device, uint32_t *data, uint32_t data_len)
{
    bk_err_t ret = BK_OK;
    uint32_t remain_len = data_len;
    const uint8_t *data_tmp = (const uint8_t *)data;
    uint8_t cmd_buf[LCD_QSPI_CMD_C_LEN_MAX] = {0};

    if ((device == NULL) || (device->qspi == NULL) || (data == NULL)) {
        LCD_QSPI_LOGE("%s invalid param\r\n", __func__);
        return BK_ERR_PARAM;
    }

    lcd_qspi_write_config_t reg_config = device->qspi->pixel_write_config;
    if ((reg_config.cmd == NULL) || (reg_config.cmd_len == 0)) {
        LCD_QSPI_LOGE("%s invalid pixel write config\r\n", __func__);
        return BK_ERR_PARAM;
    }

    if (reg_config.cmd_len > LCD_QSPI_CMD_C_LEN_MAX) {
        LCD_QSPI_LOGE("%s invalid pixel write cmd len=%d\r\n", __func__, reg_config.cmd_len);
        return BK_ERR_PARAM;
    }

    qspi_hal_force_spi_cs_low_enable(&s_lcd_qspi[qspi_id].hal);

    uint8_t cmd_data_len = LCD_QSPI_CMD_C_LEN_MAX - reg_config.cmd_len;
    if (cmd_data_len > remain_len) {
        cmd_data_len = remain_len;
    }

    os_memset(cmd_buf, 0, sizeof(cmd_buf));
    os_memcpy(cmd_buf, reg_config.cmd, reg_config.cmd_len);
    os_memcpy(&cmd_buf[reg_config.cmd_len], data_tmp, cmd_data_len);
    data_tmp += cmd_data_len;
    remain_len -= cmd_data_len;

    uint32_t fifo_len = (remain_len > LCD_QSPI_FIFO_WRITE_MAX) ? LCD_QSPI_FIFO_WRITE_MAX : remain_len;
    ret = lcd_qspi_indirect_write_cmd_fifo(qspi_id,
                                           cmd_buf,
                                           reg_config.cmd_len + cmd_data_len,
                                           reg_config.cmd_len,
                                           data_tmp,
                                           fifo_len);
    if (ret != BK_OK) {
        qspi_hal_force_spi_cs_low_disable(&s_lcd_qspi[qspi_id].hal);
        return ret;
    }
    data_tmp += fifo_len;
    remain_len -= fifo_len;

    while (remain_len > 0) {
        cmd_data_len = (remain_len > LCD_QSPI_CMD_C_LEN_MAX) ? LCD_QSPI_CMD_C_LEN_MAX : remain_len;
        os_memset(cmd_buf, 0, sizeof(cmd_buf));
        os_memcpy(cmd_buf, data_tmp, cmd_data_len);
        data_tmp += cmd_data_len;
        remain_len -= cmd_data_len;

        fifo_len = (remain_len > LCD_QSPI_FIFO_WRITE_MAX) ? LCD_QSPI_FIFO_WRITE_MAX : remain_len;

        ret = lcd_qspi_indirect_write_cmd_fifo(qspi_id,
                                               cmd_buf,
                                               cmd_data_len,
                                               0,
                                               data_tmp,
                                               fifo_len);
        if (ret != BK_OK) {
            qspi_hal_force_spi_cs_low_disable(&s_lcd_qspi[qspi_id].hal);
            return ret;
        }

        data_tmp += fifo_len;
        remain_len -= fifo_len;
    }

    qspi_hal_force_spi_cs_low_disable(&s_lcd_qspi[qspi_id].hal);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_init(qspi_id_t qspi_id, const bk_display_qspi_panel_t *device, uint8_t reset_pin)
{
    bk_err_t ret = BK_OK;

    if (device == NULL) {
        LCD_QSPI_LOGE("lcd qspi device not found\r\n");
        return BK_FAIL;
    }

    if (s_qspi_disp[qspi_id].lcd_qspi_is_init) {
        LCD_QSPI_LOGE("lcd qspi init already complete\r\n");
        return BK_OK;
    }

#if CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING
    lcd_qspi_dma_init(qspi_id);
#endif

    lcd_qspi_hardware_reset(reset_pin);

    ret = lcd_qspi_driver_init(qspi_id, device->qspi->clk);
    if (ret != BK_OK) {
        LCD_QSPI_LOGE("lcd qspi driver init failed!\r\n");
        return ret;
    }

    if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_LINE) {
        lcd_qspi_refresh_by_line_lcd_head_config(qspi_id, device);
    }

    if (device->qspi->init_cmd != NULL) {
        const lcd_qspi_init_cmd_t *init = device->qspi->init_cmd;
        for (uint32_t i = 0; i < device->qspi->device_init_cmd_len; i++) {
            if (init->data_len == 0xff) {
                rtos_delay_milliseconds(init->data[0]);
            } else {
                bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, init->cmd, init->data, init->data_len);
            }
            init++;
        }
    } else {
        LCD_QSPI_LOGE("lcd qspi device don't init\r\n");
        return BK_FAIL;
    }

    s_qspi_disp[qspi_id].lcd_qspi_is_init = true;

    return BK_OK;
}

bk_err_t bk_lcd_qspi_deinit(qspi_id_t qspi_id, uint8_t reset_pin)
{
    if (s_qspi_disp[qspi_id].lcd_qspi_is_init == false) {
        LCD_QSPI_LOGE("lcd qspi deinit already complete\r\n");
        return BK_OK;
    }

#if CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING
    lcd_qspi_dma_deinit(qspi_id);
#endif

    BK_LOG_ON_ERR(bk_qspi_deinit(qspi_id));

    gpio_dev_unmap(reset_pin);

    s_qspi_disp[qspi_id].lcd_qspi_is_init = false;

    return BK_OK;
}

bk_err_t bk_lcd_qspi_wait_display_complete(qspi_id_t qspi_id, const bk_display_qspi_panel_t *device)
{
    bk_err_t ret = BK_OK;

#if CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING
    if (s_qspi_disp[qspi_id].dma_sema) {
        ret = rtos_get_semaphore(&s_qspi_disp[qspi_id].dma_sema, 5000);
        if (ret != kNoErr) {
            LCD_QSPI_LOGE("ret = %d, lcd qspi get semaphore failed!\r\n", ret);
            return BK_FAIL;
        }
        bk_delay_us(10);
        bk_lcd_qspi_quad_write_stop(qspi_id);
    }
#endif

    if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_LINE) {
        for (uint16_t i = 0; i < device->qspi->refresh_config.hbp; i++) {
            bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, device->qspi->refresh_config.hsync_cmd, NULL, 0);
            bk_delay_us(40);
        }
    }

    return ret;
}

bk_err_t bk_lcd_qspi_frame_display(qspi_id_t qspi_id, const bk_display_qspi_panel_t *device, uint32_t *data, uint32_t data_len)
{
    if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_LINE) {
        for (uint16_t i = 0; i < device->qspi->refresh_config.vsw; i++) {
            bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, device->qspi->refresh_config.vsync_cmd, NULL, 0);
            bk_delay_us(40);
        }

        for (uint16_t i = 0; i < device->qspi->refresh_config.hfp; i++) {
            bk_lcd_qspi_send_cmd(qspi_id, device->qspi->reg_write_cmd, device->qspi->refresh_config.hsync_cmd, NULL, 0);
            bk_delay_us(40);
        }

        qspi_hal_clear_lcd_head(&s_lcd_qspi[qspi_id].hal, 1);
        qspi_hal_clear_lcd_head(&s_lcd_qspi[qspi_id].hal, 0);
#if CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING
        return bk_lcd_qspi_mapping_display(qspi_id, device, data, data_len);
#else
        return bk_lcd_qspi_indirect_display(qspi_id, device, data, data_len);
#endif
    } else if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_FRAME) {
        lcd_qspi_disp_full_area_config(qspi_id, device);
#if CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING
        return bk_lcd_qspi_mapping_display(qspi_id, device, data, data_len);
#else
        return bk_lcd_qspi_indirect_display(qspi_id, device, data, data_len);
#endif
    } else {
        LCD_QSPI_LOGE("invalid lcd qspi refresh method\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_lcd_qspi_partial_display(qspi_id_t qspi_id, const bk_display_qspi_panel_t *device, lcd_display_area_t *area, uint32_t *data)
{
    if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_FRAME) {
        lcd_qspi_disp_area_config(qspi_id, device, area);
#if CONFIG_LCD_QSPI_REFRESH_WITH_MAPPING
        return bk_lcd_qspi_mapping_display(qspi_id,
                                           device,
                                           data,
                                           (area->x_end - area->x_start + 1) *
                                           (area->y_end - area->y_start + 1) *
                                           CONFIG_LCD_QSPI_COLOR_DEPTH_BYTE);
#else
        return bk_lcd_qspi_indirect_display(qspi_id,
                                            device,
                                            data,
                                            (area->x_end - area->x_start + 1) *
                                            (area->y_end - area->y_start + 1) *
                                            CONFIG_LCD_QSPI_COLOR_DEPTH_BYTE);
#endif
    } else {
        LCD_QSPI_LOGE("Partial display just support qspi lcd with ram\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}