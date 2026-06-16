// Copyright 2021-2022 Beken
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
#include "lcd_disp_ll_macro_def.h"
#include <driver/lcd_types.h>
#include "bk_misc.h"
#include "lcd_disp_hal.h"


#if CONFIG_SOC_BK7256XX
#define MINOOR_ITCM __attribute__((section(".itcm_sec_code ")))
#else
#define MINOOR_ITCM
#endif
//extern void bk_delay_us(UINT32 us);

/******************************************8080 API********************************************/
void lcd_hal_8080_cmd_send(uint8_t param_count, uint32_t command, uint32_t *param)
{
	int i = 0;

	// asic design when send data cmd param count not be 0
	if (command == 0x2c || command == 0x3c || command == 0x2c00 || command == 0x3c00)
	{
		lcd_disp_ll_set_cmd_count_i8080_cmd_para_count(1);
	}
	else
	{
		lcd_disp_ll_set_cmd_count_i8080_cmd_para_count(param_count);
	}


	lcd_disp_ll_set_i8080_cmd_fifo_value(command);

	for(i = 0; i < param_count; i++ )
	{
		lcd_disp_ll_set_i8080_dat_fifo_value ((uint32_t)(*(param + i)));
	}
	bk_delay_us(1);
	while((lcd_disp_ll_get_disp_status_value() & 0x0000800) == 0);
}

void lcd_hal_8080_data_send(uint32_t command, uint16_t *data, uint32_t len)
{
	uint16_t *pixel = (uint16_t *)data;
		lcd_disp_ll_set_dat_fifo_thrd_i8080_cmd_para_count(len*2);
	lcd_disp_ll_set_i8080_cmd_fifo_value(command);

	for(int i = 0; i < len; i++)
	{
		lcd_disp_ll_set_i8080_dat_fifo_i8080_dat_fifo((*(pixel + i)) >> 8);
		lcd_disp_ll_set_i8080_dat_fifo_i8080_dat_fifo((*(pixel + i)) & 0xff);
	}
	bk_delay_us(1);
	while((lcd_disp_ll_get_disp_status_value() & 0x0000800) == 0);
}

void lcd_hal_8080_cmd_param_count(uint32_t count)
{
	lcd_disp_ll_set_dat_fifo_thrd_i8080_cmd_para_count(count);
}

bk_err_t lcd_hal_8080_ram_write(uint32_t command)
{
		lcd_disp_ll_set_dat_fifo_thrd_i8080_cmd_para_count(1);
	lcd_disp_ll_set_i8080_cmd_fifo_value(command);
	return BK_OK;
}

void lcd_8080_reset_befor_lcd_init(void)
{
#if CONFIG_SOC_BK7256XX
	lcd_disp_ll_set_i8080_config_reset_sleep_in(1);
#endif
}

void lcd_hal_8080_display_enable(bool en)
{
	lcd_disp_ll_set_i8080_config_i8080_disp_en(en);
}

void lcd_hal_set_data_fifo_thrd(uint16_t wr_threshold_val, uint16_t rd_threshold_val)
{
#if CONFIG_SOC_BK7256XX
	lcd_disp_ll_set_dat_fifo_thrd_dat_wr_thrd(0x80);
	lcd_disp_ll_set_dat_fifo_thrd_dat_rd_thrd(0x180);
#else
	lcd_disp_ll_set_dat_fifo_thrd_dat_wr_thrd(wr_threshold_val);
	lcd_disp_ll_set_dat_fifo_thrd_dat_rd_thrd(rd_threshold_val);
#endif
}

void lcd_hal_pixel_config(uint16_t x_pixel, uint16_t y_pixel)
{
	lcd_disp_ll_set_rgb_cfg_x_pixel(x_pixel);
	lcd_disp_ll_set_rgb_cfg_y_pixel(y_pixel);
}

void lcd_hal_set_sync_low(uint8_t hsync_back_low, uint16_t vsync_back_low)
{
	lcd_disp_ll_set_rgb_sync_low_hsync_back_low(hsync_back_low);
	lcd_disp_ll_set_rgb_sync_low_vsync_back_low(vsync_back_low);
}

void lcd_hal_set_partical_display(uint8_t partial_en, uint16_t partial_clum_l, uint16_t partial_clum_r, uint16_t partial_line_l, uint16_t partial_line_r)
{
	lcd_disp_ll_set_rgb_clum_offset_value(partial_clum_l + (partial_clum_r << 16) + (partial_en << 28));
	lcd_disp_ll_set_rgb_line_offset_value(partial_line_l + (partial_line_r << 16));
}

/******************************************RGB API********************************************/

void lcd_hal_rgb_sync_config(uint16_t rgb_hsync_back_porch, uint16_t rgb_hsync_front_porch, uint16_t rgb_vsync_back_porch, uint16_t rgb_vsync_front_porch)
{
	lcd_disp_ll_set_sync_cfg_hsync_back_porch(rgb_hsync_back_porch);
	lcd_disp_ll_set_sync_cfg_hsync_front_porch(rgb_hsync_front_porch);
	lcd_disp_ll_set_sync_cfg_vsync_back_porch(rgb_vsync_back_porch);
	lcd_disp_ll_set_sync_cfg_vsync_front_porch(rgb_vsync_front_porch);
}


void lcd_hal_rgb_display_sel(bool en)
{
	lcd_disp_ll_set_rgb_cfg_lcd_display_on(en);  //Rgb module IO
	lcd_disp_ll_set_rgb_cfg_rgb_on(en);
}

/* is_sof_en :1 enable,0:disable ;  is_eof_en:1 enable,0:disable   */
void lcd_hal_rgb_int_enable(bool         is_sof_en, bool is_eof_en)
{
//	lcd_disp_ll_set_display_int_de_int_en(1);
	lcd_disp_ll_set_display_int_rgb_int_en(is_sof_en | (is_eof_en << 1));
}

/******************************************8080 API********************************************/


/* is_sof_en :1 enable,0:disable ;  is_eof_en:1 enable,0:disable   */
void lcd_hal_8080_int_enable(bool is_sof_en, bool is_eof_en)
{
	lcd_disp_ll_set_display_int_i8080_int_en(is_sof_en| (is_eof_en << 1));
}


/* wr_threshold_val: 0-0xff rd_threshold_val：0-0xff */
void lcd_hal_8080_set_fifo_data_thrd(uint16_t wr_threshold_val, uint16_t rd_threshold_val)
{
	lcd_disp_ll_set_i8080_thrd_cmd_wr_thrd(wr_threshold_val);
	lcd_disp_ll_set_i8080_thrd_cmd_rd_thrd(rd_threshold_val);
}

void lcd_hal_frame_interval_config(bool en, frame_delay_unit_t delay_uint, uint16_t unit_count)
{
}


void lcd_hal_de_wait_config(bool en, uint16_t hsync_thrd)
{
}

bk_err_t lcd_hal_int_enable(lcd_int_type_t int_type)
{
	if (int_type | RGB_OUTPUT_SOF)
		lcd_disp_ll_set_display_int_rgb_int_en(1);
	if (int_type | RGB_OUTPUT_SOF)
		lcd_disp_ll_set_display_int_rgb_int_en(1 << 1);
	if (int_type | I8080_OUTPUT_SOF)
		lcd_disp_ll_set_display_int_i8080_int_en(1);
	if (int_type | I8080_OUTPUT_SOF)
		lcd_disp_ll_set_display_int_i8080_int_en(1 << 1);
	return BK_OK;
}


__attribute__((section(".itcm_sec_code")))bk_err_t lcd_hal_int_status_clear(lcd_int_type_t int_type)
{
	switch (int_type)
	{
		case RGB_OUTPUT_SOF:
			lcd_disp_ll_set_display_int_rgb_sof(1);
			break;
		case RGB_OUTPUT_EOF:
			lcd_disp_ll_set_display_int_rgb_eof(1);
			break;
		case I8080_OUTPUT_SOF:
			lcd_disp_ll_set_display_int_i8080_sof(1);
			break;
		case I8080_OUTPUT_EOF:
			lcd_disp_ll_set_display_int_i8080_eof(1);
			break;
		default:
			break;
	}
	return BK_OK;
}

void lcd_hal_soft_reset(void)
{
	lcd_disp_ll_set_module_control_soft_reset(0);
	bk_delay_us(10); 
	lcd_disp_ll_set_module_control_soft_reset(1);
}

void lcd_hal_clk_gate_disable(bool dis)
{
}


/*
 * display interface input data fmt
 * PIXEL_FMT_RGB565, PIXEL_FMT_RGB888, PIXEL_FMT_YUYV,vuyy,..yuv fmt
 */
bk_err_t lcd_hal_set_yuv_mode(pixel_format_t input_data_format)
{
	switch (input_data_format)
	{
		case PIXEL_FMT_RGB565_LE:
			lcd_disp_ll_set_sync_cfg_yuv_sel(0);
			lcd_disp_ll_set_rgb_sync_low_pfc_pixel_reve(0);
			break;
		case PIXEL_FMT_RGB565:
			lcd_disp_ll_set_sync_cfg_yuv_sel(0);
			lcd_disp_ll_set_rgb_sync_low_pfc_pixel_reve(1);
			break;
		case PIXEL_FMT_YUYV:
			lcd_disp_ll_set_sync_cfg_yuv_sel(1);
			lcd_disp_ll_set_rgb_sync_low_pfc_pixel_reve(0);
			break;
		case PIXEL_FMT_UYVY:
			lcd_disp_ll_set_sync_cfg_yuv_sel(2);
			lcd_disp_ll_set_rgb_sync_low_pfc_pixel_reve(0);
			break;
		case PIXEL_FMT_YYUV:
			lcd_disp_ll_set_sync_cfg_yuv_sel(3);
			lcd_disp_ll_set_rgb_sync_low_pfc_pixel_reve(0);
			break;
		case PIXEL_FMT_UVYY:
			lcd_disp_ll_set_sync_cfg_yuv_sel(4);
			lcd_disp_ll_set_rgb_sync_low_pfc_pixel_reve(0);
			break;
		case PIXEL_FMT_VUYY:
			lcd_disp_ll_set_sync_cfg_yuv_sel(5);
			lcd_disp_ll_set_rgb_sync_low_pfc_pixel_reve(0);
			break;
		case PIXEL_FMT_RGB888:
			break;
		default:
			break;
	}
	return BK_OK;
}


/*
 * rgb input RGB565, rgb888. yuv format
 * rgb output RGB565 rgb666, rgb888
 */
void lcd_hal_rgb_set_in_out_format(pixel_format_t in_fmt, pixel_format_t out_fmt)
{
}

 
 /*
  * mcu input RGB565, rgb888. yuv dormat
  * mcu output RGB565 rgb666, rgb888
  */
void lcd_hal_mcu_set_in_out_format(pixel_format_t in_fmt, pixel_format_t out_fmt)
{
}


