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

#include <driver/gpio.h>
#include <components/media_types.h>
#include <driver/lcd_types.h>
#include "lcd_disp_hal.h"
//#include "include/bk_lcd_commands.h"
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include <gpio_map.h>
#include "gpio_driver.h"


//#if CONFIG_LCD_ST7789T3

#define TAG "st7789t3"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define COUNT(A)        sizeof(A)/sizeof(A[0])


//const static uint8_t param_cmd_0x11[] = {0x00};

const static uint32_t param_cmd_0x36[] = {0x00};
const static uint32_t param_cmd_0x3A[] = {0x55};
const static uint32_t param_cmd_0xB2[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
const static uint32_t param_cmd_0xB7[] = {0x75};
const static uint32_t param_cmd_0xBB_01[] = {0x1A};
const static uint32_t param_cmd_0xC0[] = {0x2C};
const static uint32_t param_cmd_0xC2[] = {0x01};
const static uint32_t param_cmd_0xC3[] = {0x13};
const static uint32_t param_cmd_0xC4[] = {0x20};
const static uint32_t param_cmd_0xC6[] = {0x0F};
const static uint32_t param_cmd_0xD0[] = {0xA4, 0xA1};
const static uint32_t param_cmd_0xD6[] = {0xA1};
//const static uint32_t param_cmd_0xBB_02[] = {0x1F};
const static uint32_t param_cmd_0xE0[] = {0xD0, 0x0D, 0x14, 0x0D, 0x0D, 0x09, 0x38, 0x44, 0x4E, 0x3A, 0x17, 0x18, 0x2F, 0x30};
const static uint32_t param_cmd_0xE1[] = {0xD0, 0x09, 0x0F, 0x08, 0x07, 0x14, 0x37, 0x44, 0x4D, 0x38, 0x15, 0x16, 0x2C, 0x2E};


static bk_err_t lcd_st7789t3_swap_xy(const void *handle, bool swap_axes)

{
	uint8_t madctl_val = 0x48;

	if (swap_axes)
	{
		madctl_val |= (1 << 5);
	}
	else
	{
		madctl_val &= ~(1 << 5);
	}

	uint8_t madct[1] = {madctl_val};
	lcd_hal_8080_cmd_send(1, 0x36, (uint32_t *)madct);  //MV=1: 36h 0x28 or 0x68 or 0xB8

	return BK_OK;
}

static bk_err_t lcd_st7789t3_mirror(const void *handle, bool mirror_x, bool mirror_y)

{
	uint8_t madctl_val = 0x48;

	if (mirror_x)
	{
		madctl_val |= (1 << 6); // MX=1 36h 0x48
	}
	else
	{
		madctl_val &= ~(1 << 6);
	}
	if (mirror_y)
	{
		madctl_val |= (1 << 7); // MY=1 36h 0x88
	}
	else
	{
		madctl_val &= ~(1 << 7);
	}
	uint8_t madctl[1] = {madctl_val};
	lcd_hal_8080_cmd_send(1, 0x36, (uint32_t *)madctl);
	return BK_OK;
}

bk_err_t st7789t3_lcd_on(void)
{
	lcd_hal_8080_cmd_send(0, 0x29, NULL);
	return BK_OK;
}


static bk_err_t st7789t3_lcd_off(const void *handle)
{
	lcd_hal_8080_cmd_send(0, 0x28, NULL);
	return BK_OK;
}


bk_err_t st7789t3_swreset(void)
{
	lcd_hal_8080_cmd_send(0, 0x01, NULL);
	rtos_delay_milliseconds(10);
	return BK_OK;
}


void lcd_st7789t3_init(void)
{
	LOGI("%s\n", __func__);

	//rtos_delay_milliseconds(150);
	//lcd_hal_8080_cmd_send(0, 0x01, NULL);
	rtos_delay_milliseconds(120);
	lcd_hal_8080_cmd_send(0, 0x11, NULL);
	rtos_delay_milliseconds(120);

	lcd_hal_8080_cmd_send(COUNT(param_cmd_0x36), 0x36, (uint32_t *)param_cmd_0x36);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0x3A), 0x3A, (uint32_t *)param_cmd_0x3A);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xB2), 0xB2, (uint32_t *)param_cmd_0xB2);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xB7), 0xB7, (uint32_t *)param_cmd_0xB7);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xBB_01), 0xBB, (uint32_t *)param_cmd_0xBB_01);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xC0), 0xC0, (uint32_t *)param_cmd_0xC0);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xC2), 0xC2, (uint32_t *)param_cmd_0xC2);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xC3), 0xC3, (uint32_t *)param_cmd_0xC3);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xC4), 0xC4, (uint32_t *)param_cmd_0xC4);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xC6), 0xC6, (uint32_t *)param_cmd_0xC6);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xD0), 0xD0, (uint32_t *)param_cmd_0xD0);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xD6), 0xD6, (uint32_t *)param_cmd_0xD6);
	//lcd_hal_8080_cmd_send(COUNT(param_cmd_0xBB_02), 0xBB, (uint32_t *)param_cmd_0xBB_02);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xE0), 0xE0, (uint32_t *)param_cmd_0xE0);
	lcd_hal_8080_cmd_send(COUNT(param_cmd_0xE1), 0xE1, (uint32_t *)param_cmd_0xE1);

	lcd_hal_8080_cmd_send(0, 0x21, NULL);
	//rtos_delay_milliseconds(150);
	lcd_hal_8080_cmd_send(0, 0x29, NULL);
    
	//lcd_hal_8080_cmd_send(0, 0x2C, NULL);
}


static void lcd_st7789t3_set_display_mem_area(const void *handle, uint16 xs, uint16 xe, uint16 ys, uint16 ye)
{
	uint16 xs_l, xs_h, xe_l, xe_h;
	uint16 ys_l, ys_h, ye_l, ye_h;

	xs_h = xs >> 8;
	xs_l = xs & 0xff;

	xe_h = xe >> 8;
	xe_l = xe & 0xff;

	ys_h = ys >> 8;
	ys_l = ys & 0xff;

	ye_h = ye >> 8;
	ye_l = ye & 0xff;

	uint32_t param_clumn[4] = {xs_h, xs_l, xe_h, xe_l};
	uint32_t param_row[4] = {ys_h, ys_l, ye_h, ye_l};

	lcd_hal_8080_cmd_send(4, 0x2a, param_clumn);
	lcd_hal_8080_cmd_send(4, 0x2b, param_row);
}


static void lcd_st7789t3_start_transfer(const void *handle)
{
	//lcd_hal_8080_cmd_send(0, 0x2c, NULL);
}


static void lcd_st7789t3_continue_transfer(const void *handle)
{
	//lcd_hal_8080_cmd_send(0, 0x3c, NULL);
}


static const lcd_mcu_t lcd_mcu =
{
	.clk = LCD_26M,
	.set_xy_swap = lcd_st7789t3_swap_xy,
	.set_mirror = lcd_st7789t3_mirror,
	.set_display_area = lcd_st7789t3_set_display_mem_area,
	.start_transfer = lcd_st7789t3_start_transfer,
	.continue_transfer = lcd_st7789t3_continue_transfer,
};


const lcd_device_t lcd_device_st7789t3 =
{
	.id = LCD_DEVICE_ST7789T3,
	.name = "st7789t3",
	.type = LCD_TYPE_MCU8080,
	.width = 240,
	.height = 320,
	.mcu = &lcd_mcu,
	.init = lcd_st7789t3_init,
	.lcd_off = st7789t3_lcd_off,
	.src_fmt = PIXEL_FMT_RGB565_LE,
	.out_fmt = PIXEL_FMT_RGB565_LE,
};
