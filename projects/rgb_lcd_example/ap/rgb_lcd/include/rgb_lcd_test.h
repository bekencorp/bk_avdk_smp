#ifndef __RGB_LCD_TEST_H__
#define __RGB_LCD_TEST_H__

#include <os/os.h>
#include "components/bk_display.h"
#include "gpio_driver.h"
#include "frame_buffer.h"
#include "lcd_panel_devices.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_CMD_RSP_SUCCEED               "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR                 "CMDRSP:ERROR\r\n"

/* 辅助函数声明 */
avdk_err_t lcd_fill_rand_color(uint32_t len, uint8_t *addr);
avdk_err_t lcd_backlight_open(uint8_t bl_io);
avdk_err_t lcd_backlight_close(uint8_t bl_io);

avdk_err_t lcd_ldo_open(uint8_t lcd_ldo_io);
avdk_err_t lcd_ldo_close(uint8_t lcd_ldo_io);

/* CLI命令处理函数声明 */
void cli_lcd_display_api_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);
void cli_lcd_display_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

/* CLI初始化函数 */
int cli_lcd_display_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __RGB_LCD_TEST_H__ */