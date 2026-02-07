/**
 * @file lv_demo_music_main.h
 *
 */

#ifndef LV_DEMO_MUSIC_MAIN_H
#define LV_DEMO_MUSIC_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_music.h"
#if LV_USE_DEMO_MUSIC

#if CONFIG_AUDIO_PLAYER
#include <components/bk_audio_player/bk_audio_player_types.h>
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
lv_obj_t * _lv_demo_music_main_create(lv_obj_t * parent);
void _lv_demo_music_main_close(void);

void _lv_demo_music_play(uint32_t id);
void _lv_demo_music_resume(void);
void _lv_demo_music_pause(void);
void _lv_demo_music_album_next(bool next);

#if CONFIG_AUDIO_PLAYER
/* Play mode control functions */
void _lv_demo_music_set_play_mode(audio_player_mode_t mode);
audio_player_mode_t _lv_demo_music_get_play_mode(void);
void _lv_demo_music_update_progress(uint32_t second);
bool _lv_demo_music_is_playing(void);
void _lv_demo_music_request_resume(bool force);
void _lv_demo_music_on_song_start(void);
void _lv_demo_music_on_seek_complete(bool success, uint32_t second);
#endif

/**********************
 *      MACROS
 **********************/
#endif /*LV_USE_DEMO_MUSIC*/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_MUSIC_MAIN_H*/
