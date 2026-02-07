#ifndef __VIDEO_PLAYER_COMMON_H_
#define __VIDEO_PLAYER_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <components/avdk_utils/avdk_error.h>
#include <driver/gpio.h>
#include "video_player_cli.h"

// Common definitions
#define LCD_LDO_PIN          (GPIO_13)
#define GPIO_INVALID_ID      (0xFF)

#ifdef CONFIG_DVP_CTRL_POWER_GPIO_ID
#define DVP_POWER_GPIO_ID CONFIG_DVP_CTRL_POWER_GPIO_ID
#else
#define DVP_POWER_GPIO_ID GPIO_INVALID_ID
#endif

// Shared helper functions
bool cmd_contain(int argc, char **argv, char *string);
avdk_err_t lcd_backlight_open(uint8_t bl_io);
avdk_err_t lcd_backlight_close(uint8_t bl_io);
avdk_err_t display_frame_free_cb(void *frame);
int sd_card_mount(void);
int sd_card_unmount(void);
bool sd_card_is_mounted(void);

#ifdef __cplusplus
}
#endif

#endif /* __VIDEO_PLAYER_COMMON_H_ */
