#ifndef __VIDEO_PLAYER_COMMON_H_
#define __VIDEO_PLAYER_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

#include <components/avdk_utils/avdk_error.h>
#include <components/bk_video_player/bk_video_player_types.h>
#include <driver/gpio.h>
#include <components/bk_display.h>
#include "video_player_cli.h"

// Common definitions
#define LCD_LDO_PIN          (GPIO_13)
#define GPIO_INVALID_ID      (0xFF)

#ifdef CONFIG_DVP_CTRL_POWER_GPIO_ID
#define DVP_POWER_GPIO_ID CONFIG_DVP_CTRL_POWER_GPIO_ID
#else
#define DVP_POWER_GPIO_ID GPIO_INVALID_ID
#endif

typedef enum
{
    VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW = 0,
    VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_COMPRESSED,
} video_play_lcd_video_fmt_t;

// Shared helper functions
avdk_err_t video_play_lcd_open(bk_display_ctlr_handle_t *out_handle);
avdk_err_t video_play_lcd_open_with_format(bk_display_ctlr_handle_t *out_handle,
                                           video_play_lcd_video_fmt_t fmt);
avdk_err_t video_play_lcd_apply_format(bk_display_ctlr_handle_t handle,
                                       video_play_lcd_video_fmt_t fmt);
avdk_err_t video_play_lcd_ensure_open(bk_display_ctlr_handle_t *out_handle,
                                      video_play_lcd_video_fmt_t fmt);
video_play_lcd_video_fmt_t video_play_lcd_format_for_video_codec(video_player_video_format_t format);
avdk_err_t video_play_lcd_close(void);
int sd_card_mount(void);
int sd_card_unmount(void);
bool sd_card_is_mounted(void);

/* Tear down engine/playlist/LCD/audio started by either CLI, then umount SD. */
void video_play_engine_runtime_shutdown(void);
void video_play_playlist_runtime_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* __VIDEO_PLAYER_COMMON_H_ */
