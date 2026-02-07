/**
 * @file lv_demo_music_main.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_music_main.h"
#include "lv_demo_music.h"
#if LV_USE_DEMO_MUSIC

#include "lv_demo_music_list.h"
#include "assets/spectrum_1.h"
#include "assets/spectrum_2.h"
#include "assets/spectrum_3.h"

#if CONFIG_AUDIO_PLAYER
#include <components/bk_audio_player/bk_audio_player.h>
#include <components/log.h>
#endif

/*********************
 *      DEFINES
 *********************/
#define INTRO_TIME          2000
#define BAR_COLOR1          lv_color_hex(0xe9dbfc)
#define BAR_COLOR2          lv_color_hex(0x6f8af6)
#define BAR_COLOR3          lv_color_hex(0xffffff)
#if LV_DEMO_MUSIC_LARGE
    #define BAR_COLOR1_STOP     160
    #define BAR_COLOR2_STOP     200
#else
    #define BAR_COLOR1_STOP     80
    #define BAR_COLOR2_STOP     100
#endif
#define BAR_COLOR3_STOP     (2 * LV_HOR_RES / 3)
#define BAR_CNT             20
#define DEG_STEP            (180/BAR_CNT)
#define BAND_CNT            4
#define BAR_PER_BAND_CNT    (BAR_CNT / BAND_CNT)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static lv_obj_t * create_cont(lv_obj_t * parent);
static void create_wave_images(lv_obj_t * parent);
static lv_obj_t * create_title_box(lv_obj_t * parent);
static lv_obj_t * create_icon_box(lv_obj_t * parent);
static lv_obj_t * create_spectrum_obj(lv_obj_t * parent);
static lv_obj_t * create_ctrl_box(lv_obj_t * parent);
static lv_obj_t * create_handle(lv_obj_t * parent);

static void spectrum_anim_cb(void * a, int32_t v);
static void start_anim_cb(void * a, int32_t v);
static void spectrum_draw_event_cb(lv_event_t * e);
static lv_obj_t * album_img_create(lv_obj_t * parent);
static void album_gesture_event_cb(lv_event_t * e);
static void play_event_click_cb(lv_event_t * e);
static void prev_click_event_cb(lv_event_t * e);
static void next_click_event_cb(lv_event_t * e);
static void loop_mode_click_event_cb(lv_event_t * e);
static void random_mode_click_event_cb(lv_event_t * e);
static void slider_pressed_event_cb(lv_event_t * e);
static void slider_released_event_cb(lv_event_t * e);
static void timer_cb(lv_timer_t * t);
static void track_load(uint32_t id);
static void stop_start_anim_timer_cb(lv_timer_t * t);
static void spectrum_end_cb(lv_anim_t * a);
static void album_fade_anim_cb(void * var, int32_t v);
static int32_t get_cos(int32_t deg, int32_t a);
static int32_t get_sin(int32_t deg, int32_t a);
static uint32_t get_random_track_id(void);
static void update_mode_button_state(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * main_cont;
static lv_obj_t * spectrum_obj;
static lv_obj_t * title_label;
static lv_obj_t * artist_label;
static lv_obj_t * genre_label;
static lv_obj_t * album_img_obj;
static lv_obj_t * slider_obj;
static lv_obj_t * time_elapsed_label;
static lv_obj_t * time_total_label;
static uint32_t spectrum_i = 0;
static uint32_t spectrum_i_pause = 0;
static uint32_t bar_ofs = 0;
static uint32_t spectrum_lane_ofs_start = 0;
static uint32_t bar_rot = 0;
static uint32_t time_act;
static lv_timer_t  * sec_counter_timer;
static lv_timer_t * stop_start_anim_timer;
static const lv_font_t * font_small;
static const lv_font_t * font_large;
static uint32_t track_id = 0;  /* Initialize to first track */
static bool playing = false;
static bool start_anim = false;
#if CONFIG_AUDIO_PLAYER
static bool audio_player_started = false;  /* Track if audio player has ever been started */
#endif
static lv_coord_t start_anim_values[40];
static lv_obj_t * play_obj;
static lv_obj_t * loop_btn_obj;
static lv_obj_t * random_btn_obj;
static const uint16_t (* spectrum)[4];
static uint32_t spectrum_len;
static const uint16_t rnd_array[30] = {994, 285, 553, 11, 792, 707, 966, 641, 852, 827, 44, 352, 146, 581, 490, 80, 729, 58, 695, 940, 724, 561, 124, 653, 27, 292, 557, 506, 382, 199};

/* Play mode control */
/* Play mode: only support RANDOM and SEQUENCE_LOOP, default is SEQUENCE_LOOP */
static audio_player_mode_t play_mode = AUDIO_PLAYER_MODE_SEQUENCE_LOOP;
static uint32_t last_random_track_id = 0;

static bool pending_seek_active = false;
static uint32_t pending_seek_second = 0;
static uint32_t last_song_tick_second = 0;

/**********************
 *      MACROS
 **********************/
#if CONFIG_AUDIO_PLAYER
#define PLAYER_TAG "player"
#define PLAYER_LOGI(...) BK_LOGI(PLAYER_TAG, ##__VA_ARGS__)
#define PLAYER_LOGE(...) BK_LOGE(PLAYER_TAG, ##__VA_ARGS__)
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*
 * Callback adapter function to convert parameter types to avoid compile-time
 * warning.
 */
static void _img_set_zoom_anim_cb(void * obj, int32_t zoom)
{
    lv_img_set_zoom((lv_obj_t *)obj, (uint16_t)zoom);
}

/*
 * Callback adapter function to convert parameter types to avoid compile-time
 * warning.
 */
static void _obj_set_x_anim_cb(void * obj, int32_t x)
{
    lv_obj_set_x((lv_obj_t *)obj, (lv_coord_t)x);
}

lv_obj_t * _lv_demo_music_main_create(lv_obj_t * parent)
{
#if LV_DEMO_MUSIC_LARGE
    font_small = &lv_font_montserrat_22;
    font_large = &lv_font_montserrat_32;
#else
    font_small = &lv_font_montserrat_12;
    font_large = &lv_font_montserrat_16;
#endif

    /*Create the content of the music player*/
    lv_obj_t * cont = create_cont(parent);
    create_wave_images(cont);
    lv_obj_t * title_box = create_title_box(cont);
    lv_obj_t * icon_box = create_icon_box(cont);
    lv_obj_t * ctrl_box = create_ctrl_box(cont);
    spectrum_obj = create_spectrum_obj(cont);
    lv_obj_t * handle_box = create_handle(cont);

#if LV_DEMO_MUSIC_ROUND
    lv_obj_set_style_pad_hor(cont, LV_HOR_RES / 6, 0);
#endif

    /*Arrange the content into a grid*/
#if LV_DEMO_MUSIC_SQUARE || LV_DEMO_MUSIC_ROUND
    static const lv_coord_t grid_cols[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t grid_rows[] = {LV_DEMO_MUSIC_HANDLE_SIZE,     /*Spacing*/
                                     0,   /*Spectrum obj, set later*/
                                     LV_GRID_CONTENT, /*Title box*/
                                     LV_GRID_FR(3),   /*Spacer*/
                                     LV_GRID_CONTENT, /*Icon box*/
                                     LV_GRID_FR(3),   /*Spacer*/
                                     LV_GRID_CONTENT, /*Control box*/
                                     LV_GRID_FR(3),   /*Spacer*/
                                     LV_GRID_CONTENT, /*Handle box*/
                                     LV_DEMO_MUSIC_HANDLE_SIZE,     /*Spacing*/
                                     LV_GRID_TEMPLATE_LAST
                                    };

    grid_rows[1] = LV_VER_RES;

    lv_obj_set_grid_dsc_array(cont, grid_cols, grid_rows);
    lv_obj_set_style_grid_row_align(cont, LV_GRID_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_grid_cell(spectrum_obj, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    lv_obj_set_grid_cell(title_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(icon_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_ALIGN_CENTER, 4, 1);
    lv_obj_set_grid_cell(ctrl_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_ALIGN_CENTER, 6, 1);
    lv_obj_set_grid_cell(handle_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_ALIGN_CENTER, 8, 1);
#elif LV_DEMO_MUSIC_LANDSCAPE == 0
    static const lv_coord_t grid_cols[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t grid_rows[] = {LV_DEMO_MUSIC_HANDLE_SIZE,     /*Spacing*/
                                           LV_GRID_FR(1),   /*Spacer*/
                                           LV_GRID_CONTENT, /*Title box*/
                                           LV_GRID_FR(3),   /*Spacer*/
                                           LV_GRID_CONTENT, /*Icon box*/
                                           LV_GRID_FR(3),   /*Spacer*/
# if LV_DEMO_MUSIC_LARGE == 0
                                           250,    /*Spectrum obj*/
# else
                                           480,   /*Spectrum obj*/
# endif
                                           LV_GRID_FR(3),   /*Spacer*/
                                           LV_GRID_CONTENT, /*Control box*/
                                           LV_GRID_FR(3),   /*Spacer*/
                                           LV_GRID_CONTENT, /*Handle box*/
                                           LV_GRID_FR(1),   /*Spacer*/
                                           LV_DEMO_MUSIC_HANDLE_SIZE,     /*Spacing*/
                                           LV_GRID_TEMPLATE_LAST
                                          };

    lv_obj_set_grid_dsc_array(cont, grid_cols, grid_rows);
    lv_obj_set_style_grid_row_align(cont, LV_GRID_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_grid_cell(title_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(icon_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_grid_cell(spectrum_obj, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);
    lv_obj_set_grid_cell(ctrl_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 8, 1);
    lv_obj_set_grid_cell(handle_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 10, 1);
#else
    /*Arrange the content into a grid*/
    static const lv_coord_t grid_cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t grid_rows[] = {LV_DEMO_MUSIC_HANDLE_SIZE,     /*Spacing*/
                                           LV_GRID_FR(1),   /*Spacer*/
                                           LV_GRID_CONTENT, /*Title box*/
                                           LV_GRID_FR(1),   /*Spacer*/
                                           LV_GRID_CONTENT, /*Icon box*/
                                           LV_GRID_FR(3),   /*Spacer*/
                                           LV_GRID_CONTENT, /*Control box*/
                                           LV_GRID_FR(1),   /*Spacer*/
                                           LV_GRID_CONTENT, /*Handle box*/
                                           LV_GRID_FR(1),   /*Spacer*/
                                           LV_DEMO_MUSIC_HANDLE_SIZE,     /*Spacing*/
                                           LV_GRID_TEMPLATE_LAST
                                          };

    lv_obj_set_grid_dsc_array(cont, grid_cols, grid_rows);
    lv_obj_set_style_grid_row_align(cont, LV_GRID_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_grid_cell(title_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 2, 1);
    lv_obj_set_grid_cell(icon_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 4, 1);
    lv_obj_set_grid_cell(ctrl_box, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 6, 1);
    lv_obj_set_grid_cell(handle_box, LV_GRID_ALIGN_STRETCH, 0, 2, LV_GRID_ALIGN_CENTER, 8, 1);
    lv_obj_set_grid_cell(spectrum_obj, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 1, 9);
#endif

    sec_counter_timer = lv_timer_create(timer_cb, 1000, NULL);
    lv_timer_pause(sec_counter_timer);

    /*Animate in the content after the intro time*/
    lv_anim_t a;

    start_anim = true;

    stop_start_anim_timer = lv_timer_create(stop_start_anim_timer_cb, INTRO_TIME + 6000, NULL);
    lv_timer_set_repeat_count(stop_start_anim_timer, 1);

    lv_anim_init(&a);
    lv_anim_set_path_cb(&a, lv_anim_path_bounce);

    uint32_t i;
    lv_anim_set_exec_cb(&a, start_anim_cb);
    for(i = 0; i < BAR_CNT; i++) {
        lv_anim_set_values(&a, LV_HOR_RES, 5);
        lv_anim_set_delay(&a, INTRO_TIME - 200 + rnd_array[i] % 200);
        lv_anim_set_time(&a, 2500 + rnd_array[i] % 500);
        lv_anim_set_var(&a, &start_anim_values[i]);
        lv_anim_start(&a);
    }

    lv_obj_fade_in(title_box, 1000, INTRO_TIME + 1000);
    lv_obj_fade_in(icon_box, 1000, INTRO_TIME + 1000);
    lv_obj_fade_in(ctrl_box, 1000, INTRO_TIME + 1000);
    lv_obj_fade_in(handle_box, 1000, INTRO_TIME + 1000);
    lv_obj_fade_in(album_img_obj, 800, INTRO_TIME + 1000);
    lv_obj_fade_in(spectrum_obj, 0, INTRO_TIME);

    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_var(&a, album_img_obj);
    lv_anim_set_time(&a, 1000);
    lv_anim_set_delay(&a, INTRO_TIME + 1000);
    lv_anim_set_values(&a, 1, LV_IMG_ZOOM_NONE);
    lv_anim_set_exec_cb(&a, _img_set_zoom_anim_cb);
    lv_anim_set_ready_cb(&a, NULL);
    lv_anim_start(&a);

    /* Create an intro from a logo + label */
    LV_IMG_DECLARE(img_lv_demo_music_logo);
    lv_obj_t * logo = lv_img_create(lv_scr_act());
    lv_img_set_src(logo, &img_lv_demo_music_logo);
    lv_obj_move_foreground(logo);

    lv_obj_t * title = lv_label_create(lv_scr_act());
    lv_label_set_text(title, "LVGL Demo\nMusic player");
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(title, font_large, 0);
    lv_obj_set_style_text_line_space(title, 8, 0);
    lv_obj_fade_out(title, 500, INTRO_TIME);
    lv_obj_align_to(logo, spectrum_obj, LV_ALIGN_CENTER, 0, 0);
    lv_obj_align_to(title, logo, LV_ALIGN_OUT_LEFT_MID, -20, 0);

    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_var(&a, logo);
    lv_anim_set_time(&a, 400);
    lv_anim_set_delay(&a, INTRO_TIME + 800);
    lv_anim_set_values(&a, LV_IMG_ZOOM_NONE, 10);
    lv_anim_set_ready_cb(&a, lv_obj_del_anim_ready_cb);
    lv_anim_start(&a);

    lv_obj_update_layout(main_cont);

    return main_cont;
}

void _lv_demo_music_main_close(void)
{
    if(stop_start_anim_timer) lv_timer_del(stop_start_anim_timer);
    lv_timer_del(sec_counter_timer);
}

void _lv_demo_music_album_next(bool next)
{
    uint32_t id = track_id;
    uint32_t total_tracks = _lv_demo_music_get_track_count();

    /* If only placeholder track exists (no SD card), stay on it */
    const char *title = _lv_demo_music_get_title(0);
    if (title != NULL && strcmp(title, "No Music") == 0) {
        /* No actual music files, stay on placeholder */
        return;
    }

    /* Determine next track based on play mode (only RANDOM and SEQUENCE_LOOP supported) */
    if(play_mode == AUDIO_PLAYER_MODE_RANDOM) {
        /* Random mode: get a random track */
        id = get_random_track_id();
    }
    else {
        /* Sequence loop mode: move to next/previous track with wraparound */
        if(next) {
            id++;
            if(id >= total_tracks) {
                id = 0;  /* Wrap around to first track */
            }
        }
        else {
            if(id == 0) {
                id = total_tracks - 1;  /* Wrap to last track */
            }
            else {
                id--;
            }
        }
    }

    if(playing) {
        _lv_demo_music_play(id);
    }
    else {
        track_load(id);
#if CONFIG_AUDIO_PLAYER
        lv_obj_clear_state(play_obj, LV_STATE_CHECKED);
        lv_imgbtn_set_state(play_obj, LV_IMGBTN_STATE_RELEASED);
        lv_obj_invalidate(play_obj);

        /* Jump to the specified song without starting playback */
        bk_audio_player_handle_t handle = (bk_audio_player_handle_t)_lv_demo_music_get_player_handle();
        int ret = handle ? bk_audio_player_jumpto(handle, id) : AUDIO_PLAYER_NOT_INIT;
        if (ret != AUDIO_PLAYER_OK) {
            PLAYER_LOGE("Failed to jump to song %d, ret=%d\n", id, ret);
        }
#endif
    }
}

void _lv_demo_music_play(uint32_t id)
{
    track_load(id);

#if CONFIG_AUDIO_PLAYER
    /* Check if there are actual songs to play (not just the placeholder) */
    const char *title = _lv_demo_music_get_title(id);
    if (title != NULL && strcmp(title, "No Music") == 0) {
        /* No actual music files, don't try to play */
        PLAYER_LOGI("No music files available\n");
        return;
    }

    /* Jump to the specified song before resuming */
    {
        bk_audio_player_handle_t handle = (bk_audio_player_handle_t)_lv_demo_music_get_player_handle();
        int ret = handle ? bk_audio_player_jumpto(handle, id) : AUDIO_PLAYER_NOT_INIT;
        if (ret != AUDIO_PLAYER_OK) {
            PLAYER_LOGE("Failed to jump to song %d, ret=%d\n", id, ret);
        }
    }
#endif

    /* Resume will handle starting or resuming the player */
    _lv_demo_music_resume();
}

void _lv_demo_music_resume(void)
{
    playing = true;
    spectrum_i = spectrum_i_pause;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_values(&a, spectrum_i, spectrum_len - 1);
    lv_anim_set_exec_cb(&a, spectrum_anim_cb);
    lv_anim_set_var(&a, spectrum_obj);
    lv_anim_set_time(&a, ((spectrum_len - spectrum_i) * 1000) / 30);
    lv_anim_set_playback_time(&a, 0);
#if CONFIG_AUDIO_PLAYER
    /* When audio player is enabled, loop the spectrum animation instead of auto-switching songs */
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_ready_cb(&a, NULL);
#else
    /* Original behavior: auto-switch to next song when spectrum animation ends */
    lv_anim_set_ready_cb(&a, spectrum_end_cb);
#endif
    lv_anim_start(&a);

#if CONFIG_AUDIO_PLAYER
    lv_timer_pause(sec_counter_timer);
#else
    lv_timer_resume(sec_counter_timer);
#endif
    lv_slider_set_range(slider_obj, 0, _lv_demo_music_get_track_length(track_id));

    lv_obj_add_state(play_obj, LV_STATE_CHECKED);
    lv_imgbtn_set_state(play_obj, LV_IMGBTN_STATE_CHECKED_RELEASED);
    lv_obj_invalidate(play_obj);

#if CONFIG_AUDIO_PLAYER
    _lv_demo_music_update_progress(0);
    {
        bk_audio_player_handle_t handle = (bk_audio_player_handle_t)_lv_demo_music_get_player_handle();
        int ret;
        if (!handle) {
            PLAYER_LOGE("Player handle is NULL\n");
            return;
        }
        if (!audio_player_started) {
            ret = bk_audio_player_jumpto(handle, track_id);
            if (ret != AUDIO_PLAYER_OK) {
                PLAYER_LOGE("Failed to jump to song %d, ret=%d\n", track_id, ret);
                return;
            }

            ret = bk_audio_player_start(handle);
            if (ret == AUDIO_PLAYER_OK) {
                audio_player_started = true;
                PLAYER_LOGI("Started playing song %d\n", track_id);
            } else {
                PLAYER_LOGE("Failed to start audio player, ret=%d\n", ret);
            }
        } else {
            ret = bk_audio_player_resume(handle);
            if (ret != AUDIO_PLAYER_OK) {
                PLAYER_LOGE("Failed to resume audio player, ret=%d\n", ret);
            } else {
                PLAYER_LOGI("Resumed audio playback\n");
            }
        }
    }
#endif
}

void _lv_demo_music_pause(void)
{
    playing = false;
    spectrum_i_pause = spectrum_i;
    spectrum_i = 0;
    lv_anim_del(spectrum_obj, spectrum_anim_cb);
    lv_obj_invalidate(spectrum_obj);
    lv_img_set_zoom(album_img_obj, LV_IMG_ZOOM_NONE);
    lv_timer_pause(sec_counter_timer);
    lv_obj_clear_state(play_obj, LV_STATE_CHECKED);
    lv_imgbtn_set_state(play_obj, LV_IMGBTN_STATE_RELEASED);
    lv_obj_invalidate(play_obj);

#if CONFIG_AUDIO_PLAYER
    {
        bk_audio_player_handle_t handle = (bk_audio_player_handle_t)_lv_demo_music_get_player_handle();
        int ret = handle ? bk_audio_player_pause(handle) : AUDIO_PLAYER_NOT_INIT;
        if (ret != AUDIO_PLAYER_OK) {
            PLAYER_LOGE("Failed to pause audio player, ret=%d\n", ret);
        } else {
            PLAYER_LOGI("Paused audio playback\n");
        }
    }
#endif
}

#if CONFIG_AUDIO_PLAYER
bool _lv_demo_music_is_playing(void)
{
    return playing;
}

void _lv_demo_music_request_resume(bool force)
{
    if (!audio_player_started)
    {
        return;
    }

    if (!force && !playing)
    {
        return;
    }

    {
        bk_audio_player_handle_t handle = (bk_audio_player_handle_t)_lv_demo_music_get_player_handle();
        int ret = handle ? bk_audio_player_resume(handle) : AUDIO_PLAYER_NOT_INIT;
        if (ret != AUDIO_PLAYER_OK)
        {
            PLAYER_LOGE("Failed to resume audio player after seek, ret=%d\n", ret);
            return;
        }
    }

    if (!playing)
    {
        playing = true;
    }

    if (play_obj)
    {
        lv_obj_add_state(play_obj, LV_STATE_CHECKED);
        lv_imgbtn_set_state(play_obj, LV_IMGBTN_STATE_CHECKED_RELEASED);
        lv_obj_invalidate(play_obj);
    }
    PLAYER_LOGI("Resume request sent after seek\n");
}

void _lv_demo_music_on_seek_complete(bool success, uint32_t second)
{
#if CONFIG_AUDIO_PLAYER
    pending_seek_active = false;
    pending_seek_second = 0;

    uint32_t track_len = _lv_demo_music_get_track_length(track_id);
    if (track_len > 0 && second > track_len)
    {
        second = track_len;
    }

    last_song_tick_second = second;
    time_act = second;

    _lv_demo_music_update_progress(second);

    if (success)
    {
        PLAYER_LOGI("Seek completed successfully at %u s\n", second);
        _lv_demo_music_request_resume(true);
    }
    else
    {
        PLAYER_LOGE("Seek failed, status error\n");
    }
#else
    LV_UNUSED(success);
    LV_UNUSED(second);
#endif
}

void _lv_demo_music_on_song_start(void)
{
    if (!play_obj)
    {
        return;
    }

    playing = true;
    lv_obj_add_state(play_obj, LV_STATE_CHECKED);
    lv_imgbtn_set_state(play_obj, LV_IMGBTN_STATE_CHECKED_RELEASED);
    lv_obj_invalidate(play_obj);
}

void _lv_demo_music_update_progress(uint32_t second)
{
    uint32_t track_len = _lv_demo_music_get_track_length(track_id);
    if (track_len > 0 && second > track_len)
    {
        second = track_len;
    }

    if (pending_seek_active)
    {
        if (second + 1 < pending_seek_second)
        {
            return;
        }

        pending_seek_active = false;
    }

    if (!pending_seek_active && second + 2 < last_song_tick_second)
    {
        return;
    }

    last_song_tick_second = second;

    time_act = second;

    if (time_elapsed_label)
    {
        lv_label_set_text_fmt(time_elapsed_label, "%"LV_PRIu32":%02"LV_PRIu32, time_act / 60, time_act % 60);
    }

    if (time_total_label)
    {
        uint32_t total = _lv_demo_music_get_track_length(track_id);
        lv_label_set_text_fmt(time_total_label, "%"LV_PRIu32":%02"LV_PRIu32, total / 60, total % 60);
    }

    if (slider_obj)
    {
        lv_slider_set_value(slider_obj, time_act, LV_ANIM_ON);
    }
}
#endif

void _lv_demo_music_set_play_mode(audio_player_mode_t mode)
{
    /* Validate play mode: only RANDOM and SEQUENCE_LOOP are supported */
    if (mode != AUDIO_PLAYER_MODE_RANDOM && mode != AUDIO_PLAYER_MODE_SEQUENCE_LOOP) {
        return;
    }

    play_mode = mode;

#if CONFIG_AUDIO_PLAYER
    {
        bk_audio_player_handle_t handle = (bk_audio_player_handle_t)_lv_demo_music_get_player_handle();
        int ret = handle ? bk_audio_player_set_play_mode(handle, mode) : AUDIO_PLAYER_NOT_INIT;
        if (ret != AUDIO_PLAYER_OK) {
            PLAYER_LOGE("Failed to set audio player mode, ret=%d\n", ret);
        } else {
            PLAYER_LOGI("Set play mode to %d\n", mode);
        }
    }
#endif

    update_mode_button_state();
}

audio_player_mode_t _lv_demo_music_get_play_mode(void)
{
    return play_mode;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static lv_obj_t * create_cont(lv_obj_t * parent)
{
    /*A transparent container in which the player section will be scrolled*/
    main_cont = lv_obj_create(parent);
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_remove_style_all(main_cont);                            /*Make it transparent*/
    lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_scroll_snap_y(main_cont, LV_SCROLL_SNAP_CENTER);    /*Snap the children to the center*/

    /*Create a container for the player*/
    lv_obj_t * player = lv_obj_create(main_cont);
    lv_obj_set_y(player, - LV_DEMO_MUSIC_HANDLE_SIZE);
#if LV_DEMO_MUSIC_SQUARE || LV_DEMO_MUSIC_ROUND
    lv_obj_set_size(player, LV_HOR_RES, 2 * LV_VER_RES + LV_DEMO_MUSIC_HANDLE_SIZE * 2);
#else
    lv_obj_set_size(player, LV_HOR_RES, LV_VER_RES + LV_DEMO_MUSIC_HANDLE_SIZE * 2);
#endif
    lv_obj_clear_flag(player, LV_OBJ_FLAG_SNAPABLE);

    lv_obj_set_style_bg_color(player, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(player, 0, 0);
    lv_obj_set_style_pad_all(player, 0, 0);
    lv_obj_set_scroll_dir(player, LV_DIR_VER);

    /* Transparent placeholders below the player container
     * It is used only to snap it to center.*/
    lv_obj_t * placeholder1 = lv_obj_create(main_cont);
    lv_obj_remove_style_all(placeholder1);
    lv_obj_clear_flag(placeholder1, LV_OBJ_FLAG_CLICKABLE);
    //    lv_obj_set_style_bg_color(placeholder1, lv_color_hex(0xff0000), 0);
    //    lv_obj_set_style_bg_opa(placeholder1, LV_OPA_50, 0);

    lv_obj_t * placeholder2 = lv_obj_create(main_cont);
    lv_obj_remove_style_all(placeholder2);
    lv_obj_clear_flag(placeholder2, LV_OBJ_FLAG_CLICKABLE);
    //    lv_obj_set_style_bg_color(placeholder2, lv_color_hex(0x00ff00), 0);
    //    lv_obj_set_style_bg_opa(placeholder2, LV_OPA_50, 0);

#if LV_DEMO_MUSIC_SQUARE || LV_DEMO_MUSIC_ROUND
    lv_obj_t * placeholder3 = lv_obj_create(main_cont);
    lv_obj_remove_style_all(placeholder3);
    lv_obj_clear_flag(placeholder3, LV_OBJ_FLAG_CLICKABLE);
    //    lv_obj_set_style_bg_color(placeholder3, lv_color_hex(0x0000ff), 0);
    //    lv_obj_set_style_bg_opa(placeholder3, LV_OPA_20, 0);

    lv_obj_set_size(placeholder1, lv_pct(100), LV_VER_RES);
    lv_obj_set_y(placeholder1, 0);

    lv_obj_set_size(placeholder2, lv_pct(100), LV_VER_RES);
    lv_obj_set_y(placeholder2, LV_VER_RES);

    lv_obj_set_size(placeholder3, lv_pct(100),  LV_VER_RES - 2 * LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_y(placeholder3, 2 * LV_VER_RES + LV_DEMO_MUSIC_HANDLE_SIZE);
#else
    lv_obj_set_size(placeholder1, lv_pct(100), LV_VER_RES);
    lv_obj_set_y(placeholder1, 0);

    lv_obj_set_size(placeholder2, lv_pct(100),  LV_VER_RES - 2 * LV_DEMO_MUSIC_HANDLE_SIZE);
    lv_obj_set_y(placeholder2, LV_VER_RES + LV_DEMO_MUSIC_HANDLE_SIZE);
#endif

    lv_obj_update_layout(main_cont);

    return player;
}

static void create_wave_images(lv_obj_t * parent)
{
    LV_IMG_DECLARE(img_lv_demo_music_wave_top);
    LV_IMG_DECLARE(img_lv_demo_music_wave_bottom);
    lv_obj_t * wave_top = lv_img_create(parent);
    lv_img_set_src(wave_top, &img_lv_demo_music_wave_top);
    lv_obj_set_width(wave_top, LV_HOR_RES);
    lv_obj_align(wave_top, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_flag(wave_top, LV_OBJ_FLAG_IGNORE_LAYOUT);

    lv_obj_t * wave_bottom = lv_img_create(parent);
    lv_img_set_src(wave_bottom, &img_lv_demo_music_wave_bottom);
    lv_obj_set_width(wave_bottom, LV_HOR_RES);
    lv_obj_align(wave_bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(wave_bottom, LV_OBJ_FLAG_IGNORE_LAYOUT);

    LV_IMG_DECLARE(img_lv_demo_music_corner_left);
    LV_IMG_DECLARE(img_lv_demo_music_corner_right);
    lv_obj_t * wave_corner = lv_img_create(parent);
    lv_img_set_src(wave_corner, &img_lv_demo_music_corner_left);
#if LV_DEMO_MUSIC_ROUND == 0
    lv_obj_align(wave_corner, LV_ALIGN_BOTTOM_LEFT, 0, 0);
#else
    lv_obj_align(wave_corner, LV_ALIGN_BOTTOM_LEFT, -LV_HOR_RES / 6, 0);
#endif
    lv_obj_add_flag(wave_corner, LV_OBJ_FLAG_IGNORE_LAYOUT);

    wave_corner = lv_img_create(parent);
    lv_img_set_src(wave_corner, &img_lv_demo_music_corner_right);
#if LV_DEMO_MUSIC_ROUND == 0
    lv_obj_align(wave_corner, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
#else
    lv_obj_align(wave_corner, LV_ALIGN_BOTTOM_RIGHT, LV_HOR_RES / 6, 0);
#endif
    lv_obj_add_flag(wave_corner, LV_OBJ_FLAG_IGNORE_LAYOUT);
}

static lv_obj_t * create_title_box(lv_obj_t * parent)
{

    /*Create the titles*/
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    title_label = lv_label_create(cont);
    lv_obj_set_style_text_font(title_label, font_large, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x504d6d), 0);
    const char *title_text = _lv_demo_music_get_title(track_id);
    lv_label_set_text(title_label, title_text ? title_text : "No Music");
    lv_obj_set_height(title_label, lv_font_get_line_height(font_large) * 3 / 2);

    artist_label = lv_label_create(cont);
    lv_obj_set_style_text_font(artist_label, font_small, 0);
    lv_obj_set_style_text_color(artist_label, lv_color_hex(0x504d6d), 0);
    const char *artist_text = _lv_demo_music_get_artist(track_id);
    lv_label_set_text(artist_label, artist_text ? artist_text : "Unknown Artist");

    genre_label = lv_label_create(cont);
    lv_obj_set_style_text_font(genre_label, font_small, 0);
    lv_obj_set_style_text_color(genre_label, lv_color_hex(0x8a86b8), 0);
    const char *genre_text = _lv_demo_music_get_genre(track_id);
    lv_label_set_text(genre_label, genre_text ? genre_text : "No Genre");

    return cont;
}

static lv_obj_t * create_icon_box(lv_obj_t * parent)
{

    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t * icon;
    LV_IMG_DECLARE(img_lv_demo_music_icon_1);
    LV_IMG_DECLARE(img_lv_demo_music_icon_2);
    LV_IMG_DECLARE(img_lv_demo_music_icon_3);
    LV_IMG_DECLARE(img_lv_demo_music_icon_4);
    icon = lv_img_create(cont);
    lv_img_set_src(icon, &img_lv_demo_music_icon_1);
    icon = lv_img_create(cont);
    lv_img_set_src(icon, &img_lv_demo_music_icon_2);
    icon = lv_img_create(cont);
    lv_img_set_src(icon, &img_lv_demo_music_icon_3);
    icon = lv_img_create(cont);
    lv_img_set_src(icon, &img_lv_demo_music_icon_4);

    return cont;
}

static lv_obj_t * create_spectrum_obj(lv_obj_t * parent)
{
    /*Create the spectrum visualizer*/
    lv_obj_t * obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
#if LV_DEMO_MUSIC_LARGE
    lv_obj_set_height(obj, 500);
#else
    lv_obj_set_height(obj, 250);
#endif
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(obj, spectrum_draw_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_refresh_ext_draw_size(obj);
    album_img_obj = album_img_create(obj);
    return obj;
}

static lv_obj_t * create_ctrl_box(lv_obj_t * parent)
{
    /*Create the control box*/
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_height(cont, LV_SIZE_CONTENT);
#if LV_DEMO_MUSIC_LARGE
    lv_obj_set_style_pad_bottom(cont, 17, 0);
#else
    lv_obj_set_style_pad_bottom(cont, 8, 0);
#endif
    static const lv_coord_t grid_col[] = {LV_GRID_FR(2), LV_GRID_FR(3), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(5), LV_GRID_FR(3), LV_GRID_FR(2), LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t grid_row[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(cont, grid_col, grid_row);

    LV_IMG_DECLARE(img_lv_demo_music_btn_loop);
    LV_IMG_DECLARE(img_lv_demo_music_btn_rnd);
    LV_IMG_DECLARE(img_lv_demo_music_btn_next);
    LV_IMG_DECLARE(img_lv_demo_music_btn_prev);
    LV_IMG_DECLARE(img_lv_demo_music_btn_play);
    LV_IMG_DECLARE(img_lv_demo_music_btn_pause);

    lv_obj_t * icon;
    /* Random play button */
    random_btn_obj = lv_img_create(cont);
    lv_img_set_src(random_btn_obj, &img_lv_demo_music_btn_rnd);
    lv_obj_set_grid_cell(random_btn_obj, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(random_btn_obj, random_mode_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(random_btn_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_img_opa(random_btn_obj, LV_OPA_50, 0);  /* Start with inactive opacity */

    /* Loop mode button */
    loop_btn_obj = lv_img_create(cont);
    lv_img_set_src(loop_btn_obj, &img_lv_demo_music_btn_loop);
    lv_obj_set_grid_cell(loop_btn_obj, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(loop_btn_obj, loop_mode_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(loop_btn_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_img_opa(loop_btn_obj, LV_OPA_50, 0);  /* Start with inactive opacity */

    /* Initialize button states based on default play mode (SEQUENCE_LOOP) */
    update_mode_button_state();

    icon = lv_img_create(cont);
    lv_img_set_src(icon, &img_lv_demo_music_btn_prev);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 2, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, prev_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    play_obj = lv_imgbtn_create(cont);
    lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_lv_demo_music_btn_play, NULL);
    lv_imgbtn_set_src(play_obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_lv_demo_music_btn_pause, NULL);
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_grid_cell(play_obj, LV_GRID_ALIGN_CENTER, 3, 1, LV_GRID_ALIGN_CENTER, 0, 1);

    lv_obj_add_event_cb(play_obj, play_event_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(play_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(play_obj, img_lv_demo_music_btn_play.header.w);

    icon = lv_img_create(cont);
    lv_img_set_src(icon, &img_lv_demo_music_btn_next);
    lv_obj_set_grid_cell(icon, LV_GRID_ALIGN_CENTER, 4, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    lv_obj_add_event_cb(icon, next_click_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(icon, LV_OBJ_FLAG_CLICKABLE);

    LV_IMG_DECLARE(img_lv_demo_music_slider_knob);
    slider_obj = lv_slider_create(cont);
    lv_obj_set_style_anim_time(slider_obj, 100, 0);
    lv_obj_add_flag(slider_obj, LV_OBJ_FLAG_CLICKABLE); /*No input from the slider*/
    lv_obj_add_event_cb(slider_obj, slider_pressed_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(slider_obj, slider_released_event_cb, LV_EVENT_RELEASED, NULL);

#if LV_DEMO_MUSIC_LARGE == 0
    lv_obj_set_height(slider_obj, 3);
#else
    lv_obj_set_height(slider_obj, 6);
#endif
    lv_obj_set_grid_cell(slider_obj, LV_GRID_ALIGN_STRETCH, 2, 3, LV_GRID_ALIGN_CENTER, 1, 1);

    lv_obj_set_style_bg_img_src(slider_obj, &img_lv_demo_music_slider_knob, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider_obj, 20, LV_PART_KNOB);
    lv_obj_set_style_bg_grad_dir(slider_obj, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider_obj, lv_color_hex(0x569af8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(slider_obj, lv_color_hex(0xa666f1), LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(slider_obj, 0, 0);

    time_elapsed_label = lv_label_create(cont);
    lv_obj_set_style_text_font(time_elapsed_label, font_small, 0);
    lv_obj_set_style_text_color(time_elapsed_label, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(time_elapsed_label, "0:00");
    lv_obj_set_grid_cell(time_elapsed_label, LV_GRID_ALIGN_START, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    time_total_label = lv_label_create(cont);
    lv_obj_set_style_text_font(time_total_label, font_small, 0);
    lv_obj_set_style_text_color(time_total_label, lv_color_hex(0x8a86b8), 0);
    lv_label_set_text(time_total_label, "0:00");
    lv_obj_set_grid_cell(time_total_label, LV_GRID_ALIGN_END, 5, 1, LV_GRID_ALIGN_CENTER, 1, 1);

    return cont;
}

static lv_obj_t * create_handle(lv_obj_t * parent)
{
    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);

    lv_obj_set_size(cont, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 8, 0);

    /*A handle to scroll to the track list*/
    lv_obj_t * handle_label = lv_label_create(cont);
    lv_label_set_text(handle_label, "ALL TRACKS");
    lv_obj_set_style_text_font(handle_label, font_small, 0);
    lv_obj_set_style_text_color(handle_label, lv_color_hex(0x8a86b8), 0);

    lv_obj_t * handle_rect = lv_obj_create(cont);
#if LV_DEMO_MUSIC_LARGE
    lv_obj_set_size(handle_rect, 40, 3);
#else
    lv_obj_set_size(handle_rect, 20, 2);
#endif

    lv_obj_set_style_bg_color(handle_rect, lv_color_hex(0x8a86b8), 0);
    lv_obj_set_style_border_width(handle_rect, 0, 0);

    return cont;
}

static void track_load(uint32_t id)
{
    uint32_t total_tracks = _lv_demo_music_get_track_count();

    spectrum_i = 0;
    time_act = 0;
    spectrum_i_pause = 0;
    lv_slider_set_value(slider_obj, 0, LV_ANIM_OFF);
    lv_label_set_text(time_elapsed_label, "0:00");
    if (time_total_label)
    {
        uint32_t total_len = _lv_demo_music_get_track_length(id);
        lv_label_set_text_fmt(time_total_label, "%"LV_PRIu32":%02"LV_PRIu32, total_len / 60, total_len % 60);
    }

    pending_seek_active = false;
    pending_seek_second = 0;
    last_song_tick_second = 0;

    if(id == track_id) return;
    bool next = false;
    if((track_id + 1) % total_tracks == id) next = true;

    _lv_demo_music_list_btn_check(track_id, false);

    track_id = id;

    _lv_demo_music_list_btn_check(id, true);

    /* Update labels with NULL protection - check if labels are created */
    if (title_label && artist_label && genre_label) {
        const char *title_text = _lv_demo_music_get_title(track_id);
        const char *artist_text = _lv_demo_music_get_artist(track_id);
        const char *genre_text = _lv_demo_music_get_genre(track_id);

        lv_label_set_text(title_label, title_text ? title_text : "No Music");
        lv_label_set_text(artist_label, artist_text ? artist_text : "Unknown Artist");
        lv_label_set_text(genre_label, genre_text ? genre_text : "No Genre");
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, album_img_obj);
    lv_anim_set_values(&a, lv_obj_get_style_img_opa(album_img_obj, 0), LV_OPA_TRANSP);
    lv_anim_set_exec_cb(&a, album_fade_anim_cb);
    lv_anim_set_time(&a, 500);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, album_img_obj);
    lv_anim_set_time(&a, 500);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
#if LV_DEMO_MUSIC_LANDSCAPE
    if(next) {
        lv_anim_set_values(&a, 0, - LV_HOR_RES / 7);
    }
    else {
        lv_anim_set_values(&a, 0, LV_HOR_RES / 7);
    }
#else
    if(next) {
        lv_anim_set_values(&a, 0, - LV_HOR_RES / 2);
    }
    else {
        lv_anim_set_values(&a, 0, LV_HOR_RES / 2);
    }
#endif
    lv_anim_set_exec_cb(&a, _obj_set_x_anim_cb);
    lv_anim_set_ready_cb(&a, lv_obj_del_anim_ready_cb);
    lv_anim_start(&a);

    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_set_var(&a, album_img_obj);
    lv_anim_set_time(&a, 500);
    lv_anim_set_values(&a, LV_IMG_ZOOM_NONE, LV_IMG_ZOOM_NONE / 2);
    lv_anim_set_exec_cb(&a, _img_set_zoom_anim_cb);
    lv_anim_set_ready_cb(&a, NULL);
    lv_anim_start(&a);

    album_img_obj = album_img_create(spectrum_obj);

    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_set_var(&a, album_img_obj);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 100);
    lv_anim_set_values(&a, LV_IMG_ZOOM_NONE / 4, LV_IMG_ZOOM_NONE);
    lv_anim_set_exec_cb(&a, _img_set_zoom_anim_cb);
    lv_anim_set_ready_cb(&a, NULL);
    lv_anim_start(&a);

    lv_anim_init(&a);
    lv_anim_set_var(&a, album_img_obj);
    lv_anim_set_values(&a, 0, LV_OPA_COVER);
    lv_anim_set_exec_cb(&a, album_fade_anim_cb);
    lv_anim_set_time(&a, 500);
    lv_anim_set_delay(&a, 100);
    lv_anim_start(&a);
}

int32_t get_cos(int32_t deg, int32_t a)
{
    int32_t r = (lv_trigo_cos(deg) * a);

    r += LV_TRIGO_SIN_MAX / 2;
    return r >> LV_TRIGO_SHIFT;
}

int32_t get_sin(int32_t deg, int32_t a)
{
    int32_t r = lv_trigo_sin(deg) * a;

    r += LV_TRIGO_SIN_MAX / 2;
    return r >> LV_TRIGO_SHIFT;

}

static void spectrum_draw_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_REFR_EXT_DRAW_SIZE) {
#if LV_DEMO_MUSIC_LANDSCAPE
        lv_event_set_ext_draw_size(e, LV_HOR_RES);
#else
        lv_event_set_ext_draw_size(e, LV_VER_RES);
#endif
    }
    else if(code == LV_EVENT_COVER_CHECK) {
        lv_event_set_cover_res(e, LV_COVER_RES_NOT_COVER);
    }
    else if(code == LV_EVENT_DRAW_POST) {
        lv_obj_t * obj = lv_event_get_target(e);
        lv_draw_ctx_t * draw_ctx = lv_event_get_draw_ctx(e);

        lv_opa_t opa = lv_obj_get_style_opa(obj, LV_PART_MAIN);
        if(opa < LV_OPA_MIN) return;

        lv_point_t poly[4];
        lv_point_t center;
        center.x = obj->coords.x1 + lv_obj_get_width(obj) / 2;
        center.y = obj->coords.y1 + lv_obj_get_height(obj) / 2;

        lv_draw_rect_dsc_t draw_dsc;
        lv_draw_rect_dsc_init(&draw_dsc);
        draw_dsc.bg_opa = LV_OPA_COVER;

        uint16_t r[64];
        uint32_t i;

        lv_coord_t min_a = 5;
#if LV_DEMO_MUSIC_LARGE == 0
        lv_coord_t r_in = 77;
#else
        lv_coord_t r_in = 160;
#endif
        r_in = (r_in * lv_img_get_zoom(album_img_obj)) >> 8;
        for(i = 0; i < BAR_CNT; i++) r[i] = r_in + min_a;

        uint32_t s;
        for(s = 0; s < 4; s++) {
            uint32_t f;
            uint32_t band_w = 0;    /*Real number of bars in this band.*/
            switch(s) {
                case 0:
                    band_w = 20;
                    break;
                case 1:
                    band_w = 8;
                    break;
                case 2:
                    band_w = 4;
                    break;
                case 3:
                    band_w = 2;
                    break;
            }

            /* Add "side bars" with cosine characteristic.*/
            for(f = 0; f < band_w; f++) {
                uint32_t ampl_main = spectrum[spectrum_i][s];
                int32_t ampl_mod = get_cos(f * 360 / band_w + 180, 180) + 180;
                int32_t t = BAR_PER_BAND_CNT * s - band_w / 2 + f;
                if(t < 0) t = BAR_CNT + t;
                if(t >= BAR_CNT) t = t - BAR_CNT;
                r[t] += (ampl_main * ampl_mod) >> 9;
            }
        }

        uint32_t amax = 20;
        int32_t animv = spectrum_i - spectrum_lane_ofs_start;
        if(animv > amax) animv = amax;
        for(i = 0; i < BAR_CNT; i++) {
            uint32_t deg_space = 1;
            uint32_t deg = i * DEG_STEP + 90;
            uint32_t j = (i + bar_rot + rnd_array[bar_ofs % 10]) % BAR_CNT;
            uint32_t k = (i + bar_rot + rnd_array[(bar_ofs + 1) % 10]) % BAR_CNT;

            uint32_t v = (r[k] * animv + r[j] * (amax - animv)) / amax;
            if(start_anim) {
                v = r_in + start_anim_values[i];
                deg_space = v >> 7;
                if(deg_space < 1) deg_space = 1;
            }

            if(v < BAR_COLOR1_STOP) draw_dsc.bg_color = BAR_COLOR1;
            else if(v > BAR_COLOR3_STOP) draw_dsc.bg_color = BAR_COLOR3;
            else if(v > BAR_COLOR2_STOP) draw_dsc.bg_color = lv_color_mix(BAR_COLOR3, BAR_COLOR2,
                                                                              ((v - BAR_COLOR2_STOP) * 255) / (BAR_COLOR3_STOP - BAR_COLOR2_STOP));
            else draw_dsc.bg_color = lv_color_mix(BAR_COLOR2, BAR_COLOR1,
                                                      ((v - BAR_COLOR1_STOP) * 255) / (BAR_COLOR2_STOP - BAR_COLOR1_STOP));

            uint32_t di = deg + deg_space;

            int32_t x1_out = get_cos(di, v);
            poly[0].x = center.x + x1_out;
            poly[0].y = center.y + get_sin(di, v);

            int32_t x1_in = get_cos(di, r_in);
            poly[1].x = center.x + x1_in;
            poly[1].y = center.y + get_sin(di, r_in);
            di += DEG_STEP - deg_space * 2;

            int32_t x2_in = get_cos(di, r_in);
            poly[2].x = center.x + x2_in;
            poly[2].y = center.y + get_sin(di, r_in);

            int32_t x2_out = get_cos(di, v);
            poly[3].x = center.x + x2_out;
            poly[3].y = center.y + get_sin(di, v);

            lv_draw_polygon(draw_ctx, &draw_dsc, poly, 4);

            poly[0].x = center.x - x1_out;
            poly[1].x = center.x - x1_in;
            poly[2].x = center.x - x2_in;
            poly[3].x = center.x - x2_out;
            lv_draw_polygon(draw_ctx, &draw_dsc, poly, 4);
        }
    }
}

static void spectrum_anim_cb(void * a, int32_t v)
{
    lv_obj_t * obj = a;
    if(start_anim) {
        lv_obj_invalidate(obj);
        return;
    }

    spectrum_i = v;
    lv_obj_invalidate(obj);

    static uint32_t bass_cnt = 0;
    static int32_t last_bass = -1000;
    static int32_t dir = 1;
    if(spectrum[spectrum_i][0] > 12) {
        if(spectrum_i - last_bass > 5) {
            bass_cnt++;
            last_bass = spectrum_i;
            if(bass_cnt >= 2) {
                bass_cnt = 0;
                spectrum_lane_ofs_start = spectrum_i;
                bar_ofs++;
            }
        }
    }
    if(spectrum[spectrum_i][0] < 4) bar_rot += dir;

    lv_img_set_zoom(album_img_obj, LV_IMG_ZOOM_NONE + spectrum[spectrum_i][0]);
}

static void start_anim_cb(void * a, int32_t v)
{
    lv_coord_t * av = a;
    *av = v;
    lv_obj_invalidate(spectrum_obj);
}

static lv_obj_t * album_img_create(lv_obj_t * parent)
{
    LV_IMG_DECLARE(img_lv_demo_music_cover_1);
    LV_IMG_DECLARE(img_lv_demo_music_cover_2);
    LV_IMG_DECLARE(img_lv_demo_music_cover_3);

    lv_obj_t * img;
    img = lv_img_create(parent);

    /* Cycle through 3 covers for all tracks */
    uint32_t cover_id = track_id % 3;

    switch(cover_id) {
        case 2:
            lv_img_set_src(img, &img_lv_demo_music_cover_3);
            spectrum = spectrum_3;
            spectrum_len = sizeof(spectrum_3) / sizeof(spectrum_3[0]);
            break;
        case 1:
            lv_img_set_src(img, &img_lv_demo_music_cover_2);
            spectrum = spectrum_2;
            spectrum_len = sizeof(spectrum_2) / sizeof(spectrum_2[0]);
            break;
        case 0:
        default:
            lv_img_set_src(img, &img_lv_demo_music_cover_1);
            spectrum = spectrum_1;
            spectrum_len = sizeof(spectrum_1) / sizeof(spectrum_1[0]);
            break;
    }
    lv_img_set_antialias(img, false);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(img, album_gesture_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);

    return img;

}

static void album_gesture_event_cb(lv_event_t * e)
{
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if(dir == LV_DIR_LEFT) _lv_demo_music_album_next(true);
    if(dir == LV_DIR_RIGHT) _lv_demo_music_album_next(false);
}

static void play_event_click_cb(lv_event_t * e)
{
    lv_obj_t * obj = lv_event_get_target(e);
    if(lv_obj_has_state(obj, LV_STATE_CHECKED)) {
        _lv_demo_music_resume();
    }
    else {
        _lv_demo_music_pause();
    }
}

static void prev_click_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        _lv_demo_music_album_next(false);
    }
}

static void next_click_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        _lv_demo_music_album_next(true);
    }
}


static void timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    time_act++;
    lv_label_set_text_fmt(time_elapsed_label, "%"LV_PRIu32":%02"LV_PRIu32, time_act / 60, time_act % 60);
    lv_slider_set_value(slider_obj, time_act, LV_ANIM_ON);
}

static void spectrum_end_cb(lv_anim_t * a)
{
    LV_UNUSED(a);

    /* Handle track end based on play mode (only RANDOM and SEQUENCE_LOOP supported) */
    if (play_mode == AUDIO_PLAYER_MODE_RANDOM) {
        /* Random mode: play a random track */
        _lv_demo_music_album_next(true);
    } else {
        /* Sequence loop mode: play next track (loops back to first track after last) */
        _lv_demo_music_album_next(true);
    }
}


static void stop_start_anim_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    start_anim = false;
    stop_start_anim_timer = NULL;
    lv_obj_refresh_ext_draw_size(spectrum_obj);
}

static void album_fade_anim_cb(void * var, int32_t v)
{
    lv_obj_set_style_img_opa(var, v, 0);
}

/**
 * Handle loop mode button click event
 * Switches from RANDOM mode to SEQUENCE_LOOP mode (mutually exclusive with random button)
 */
static void loop_mode_click_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        /* If not already in sequence loop mode, switch to it */
        if (play_mode != AUDIO_PLAYER_MODE_SEQUENCE_LOOP) {
            _lv_demo_music_set_play_mode(AUDIO_PLAYER_MODE_SEQUENCE_LOOP);
        }
    }
}

/**
 * Handle random mode button click event
 * Switches from SEQUENCE_LOOP mode to RANDOM mode (mutually exclusive with loop button)
 */
static void random_mode_click_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        /* If not already in random mode, switch to it */
        if (play_mode != AUDIO_PLAYER_MODE_RANDOM) {
            _lv_demo_music_set_play_mode(AUDIO_PLAYER_MODE_RANDOM);
        }
    }
}

static void slider_pressed_event_cb(lv_event_t * e)
{
    (void)e;
    PLAYER_LOGI("Slider pressed\n");
}

static void slider_released_event_cb(lv_event_t * e)
{
    lv_obj_t * slider = lv_event_get_target(e);
    int32_t second = lv_slider_get_value(slider);
    if (second < 0)
    {
        second = 0;
    }

    uint32_t track_len = _lv_demo_music_get_track_length(track_id);
    if ((uint32_t)second > track_len)
    {
        second = track_len;
    }

    time_act = second;
    last_song_tick_second = (uint32_t)second;
    if (time_elapsed_label)
    {
        lv_label_set_text_fmt(time_elapsed_label, "%"LV_PRIu32":%02"LV_PRIu32, time_act / 60, time_act % 60);
    }
    if (slider_obj && slider != slider_obj)
    {
        lv_slider_set_value(slider_obj, time_act, LV_ANIM_OFF);
    }

    PLAYER_LOGI("Slider released at %ld s\n", (long)second);
#if CONFIG_AUDIO_PLAYER
    {
        bk_audio_player_handle_t handle = (bk_audio_player_handle_t)_lv_demo_music_get_player_handle();
        int ret = handle ? bk_audio_player_seek(handle, (int)second) : AUDIO_PLAYER_NOT_INIT;
        if (ret == AUDIO_PLAYER_OK)
        {
            PLAYER_LOGI("Seek request accepted\n");
            pending_seek_active = true;
            pending_seek_second = (uint32_t)second;
        }
        else
        {
            PLAYER_LOGE("Seek request failed, ret=%d\n", ret);
            pending_seek_active = false;
            pending_seek_second = 0;
            _lv_demo_music_update_progress(last_song_tick_second);
        }
    }
#endif
}

/**
 * Get a random track ID (different from the current one)
 */
static uint32_t get_random_track_id(void)
{
    uint32_t new_id;
    uint32_t attempts = 0;
    uint32_t total_tracks = _lv_demo_music_get_track_count();

    if (total_tracks == 0) {
        return 0;
    }

    /* Try to get a different track, but limit attempts to avoid infinite loop */
    do {
        new_id = rnd_array[(last_random_track_id + attempts) % 30] % total_tracks;
        attempts++;
    } while (new_id == track_id && attempts < total_tracks);

    last_random_track_id = (last_random_track_id + attempts) % 30;
    return new_id;
}

/**
 * Update the visual state of mode buttons based on current play mode
 * Only SEQUENCE_LOOP and RANDOM modes are supported (mutually exclusive)
 */
static void update_mode_button_state(void)
{
    if (loop_btn_obj == NULL || random_btn_obj == NULL) {
        return;
    }

    /* Update loop button state (active when in SEQUENCE_LOOP mode) */
    if (play_mode == AUDIO_PLAYER_MODE_SEQUENCE_LOOP) {
        lv_obj_add_state(loop_btn_obj, LV_STATE_CHECKED);
        lv_obj_set_style_img_opa(loop_btn_obj, LV_OPA_COVER, 0);
    } else {
        lv_obj_clear_state(loop_btn_obj, LV_STATE_CHECKED);
        lv_obj_set_style_img_opa(loop_btn_obj, LV_OPA_50, 0);
    }

    /* Update random button state (active when in RANDOM mode) */
    if (play_mode == AUDIO_PLAYER_MODE_RANDOM) {
        lv_obj_add_state(random_btn_obj, LV_STATE_CHECKED);
        lv_obj_set_style_img_opa(random_btn_obj, LV_OPA_COVER, 0);
    } else {
        lv_obj_clear_state(random_btn_obj, LV_STATE_CHECKED);
        lv_obj_set_style_img_opa(random_btn_obj, LV_OPA_50, 0);
    }
}

#endif /*LV_USE_DEMO_MUSIC*/

