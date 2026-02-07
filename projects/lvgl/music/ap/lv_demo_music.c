/**
 * @file lv_demo_music.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_music.h"

#if LV_USE_DEMO_MUSIC

#include "lv_demo_music_main.h"
#include "lv_demo_music_list.h"
#include "lv_demo_music_list_mgr.h"

#if CONFIG_AUDIO_PLAYER
#include <components/bk_audio_player/bk_audio_player.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_mp3_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_wav_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_m4a_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_amr_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_ts_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_aac_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_flac_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_opus_decoder.h>
#include <components/bk_audio_player/plugins/decoders/bk_audio_player_ogg_decoder.h>
#include <components/bk_audio_player/plugins/sources/bk_audio_player_file_source.h>
#include <components/bk_audio_player/plugins/sinks/bk_audio_player_onboard_speaker_sink.h>
/* Metadata parsers */
#include <components/bk_audio_player/plugins/metadata_parsers/bk_audio_player_mp3_metadata_parser.h>
#include <components/bk_audio_player/plugins/metadata_parsers/bk_audio_player_wav_metadata_parser.h>
#include <components/bk_audio_player/plugins/metadata_parsers/bk_audio_player_aac_metadata_parser.h>
#include <components/bk_audio_player/plugins/metadata_parsers/bk_audio_player_amr_metadata_parser.h>
#include <components/bk_audio_player/plugins/metadata_parsers/bk_audio_player_m4a_metadata_parser.h>
#include <components/bk_audio_player/plugins/metadata_parsers/bk_audio_player_opus_metadata_parser.h>
#include <components/bk_audio_player/plugins/metadata_parsers/bk_audio_player_ogg_metadata_parser.h>
#include <components/bk_audio_player/plugins/metadata_parsers/bk_audio_player_flac_metadata_parser.h>
#include <components/log.h>
#include "bk_vfs.h"
#include "bk_posix.h"
#endif

/*********************
 *      DEFINES
 *********************/
/* Music files scan path on SD card */
#define MUSIC_SCAN_PATH     "/sd0"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
#if LV_DEMO_MUSIC_AUTO_PLAY
    static void auto_step_cb(lv_timer_t * timer);
#endif

#if CONFIG_AUDIO_PLAYER
static void audio_player_event_handler(audio_player_event_type_t event, void *extra_info, void *args);
static void song_check_timer_cb(lv_timer_t * timer);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * ctrl;
static lv_obj_t * list;

#if CONFIG_AUDIO_PLAYER
static bk_audio_player_handle_t s_player_handle = NULL;
static volatile bool song_finish_flag = false;      /* Flag to indicate song finished */
static volatile bool song_tick_flag = false;        /* Flag to indicate progress update */
static volatile uint32_t song_tick_second = 0;      /* Latest playback position (seconds) */
static lv_timer_t * song_check_timer = NULL;        /* Timer to poll audio events in LVGL thread */
static volatile bool song_pause_flag = false;       /* Flag to indicate pipeline pause */
static volatile bool song_start_flag = false;       /* Flag to indicate pipeline start */
static volatile bool seek_complete_flag = false;    /* Flag to signal seek completion */
static audio_player_seek_result_t seek_result_cache;/* Cached seek result for LVGL thread */

void * _lv_demo_music_get_player_handle(void)
{
    return (void *)s_player_handle;
}
#endif

/* Music list based on BSD queue */
static lv_demo_music_list_t g_music_list;

#if LV_DEMO_MUSIC_AUTO_PLAY
    static lv_timer_t * auto_step_timer;
#endif

static lv_color_t original_screen_bg_color;

/**********************
 *      MACROS
 **********************/
#if CONFIG_AUDIO_PLAYER
#define MUSIC_TAG "music"
#define MUSIC_LOGI(...) BK_LOGI(MUSIC_TAG, ##__VA_ARGS__)
#define MUSIC_LOGE(...) BK_LOGE(MUSIC_TAG, ##__VA_ARGS__)
#endif

#if CONFIG_AUDIO_PLAYER
/* Register default sources, sinks, decoders and metadata parsers used by the LVGL music demo. */
static int music_player_register_default_plugins(void)
{
    int ret;

    /* Register built-in audio sources */
    {
        ret = bk_audio_player_register_source(s_player_handle, bk_audio_player_get_file_source_ops());
        if (ret != AUDIO_PLAYER_OK)
        {
            MUSIC_LOGE("bk_audio_player_register_source(file) failed, ret=%d\n", ret);
            return ret;
        }
    }

    /* Register built-in audio sinks */
    {
        ret = bk_audio_player_register_sink(s_player_handle, bk_audio_player_get_onboard_speaker_sink_ops());
        if (ret != AUDIO_PLAYER_OK)
        {
            MUSIC_LOGE("bk_audio_player_register_sink(device) failed, ret=%d\n", ret);
            return ret;
        }
    }

    /* Register built-in audio decoders */
    ret = bk_audio_player_register_decoder(s_player_handle, bk_audio_player_get_mp3_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("bk_audio_player_register_decoder(mp3) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(s_player_handle, bk_audio_player_get_wav_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("bk_audio_player_register_decoder(wav) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(s_player_handle, bk_audio_player_get_m4a_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("bk_audio_player_register_decoder(m4a) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(s_player_handle, bk_audio_player_get_amr_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("bk_audio_player_register_decoder(amr) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(s_player_handle, bk_audio_player_get_ts_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("bk_audio_player_register_decoder(ts) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(s_player_handle, bk_audio_player_get_aac_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("bk_audio_player_register_decoder(aac) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(s_player_handle, bk_audio_player_get_flac_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("bk_audio_player_register_decoder(flac) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(s_player_handle, bk_audio_player_get_opus_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("bk_audio_player_register_decoder(opus) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_decoder(s_player_handle, bk_audio_player_get_ogg_decoder_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("bk_audio_player_register_decoder(ogg) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_metadata_parser(s_player_handle, bk_audio_player_get_mp3_metadata_parser_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("metadata_parser_register(mp3) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_metadata_parser(s_player_handle, bk_audio_player_get_wav_metadata_parser_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("metadata_parser_register(wav) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_metadata_parser(s_player_handle, bk_audio_player_get_aac_metadata_parser_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("metadata_parser_register(aac) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_metadata_parser(s_player_handle, bk_audio_player_get_amr_metadata_parser_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("metadata_parser_register(amr) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_metadata_parser(s_player_handle, bk_audio_player_get_m4a_metadata_parser_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("metadata_parser_register(m4a) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_metadata_parser(s_player_handle, bk_audio_player_get_opus_metadata_parser_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("metadata_parser_register(opus) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_metadata_parser(s_player_handle, bk_audio_player_get_ogg_metadata_parser_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("metadata_parser_register(ogg) failed, ret=%d\n", ret);
        return ret;
    }

    ret = bk_audio_player_register_metadata_parser(s_player_handle, bk_audio_player_get_flac_metadata_parser_ops());
    if (ret != AUDIO_PLAYER_OK)
    {
        MUSIC_LOGE("metadata_parser_register(flac) failed, ret=%d\n", ret);
        return ret;
    }

    return AUDIO_PLAYER_OK;
}
#endif

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

#if CONFIG_AUDIO_PLAYER
/**
 * @brief Audio player event handler
 *
 * This function handles audio player events such as song finish, pause, resume, etc.
 * NOTE: This function is called from audio player thread, NOT LVGL thread.
 *       DO NOT call LVGL functions directly here!
 *
 * @param event Event type
 * @param extra_info Extra information for the event
 * @param args User defined arguments
 */
static void audio_player_event_handler(audio_player_event_type_t event, void *extra_info, void *args)
{
    (void)extra_info;
    (void)args;

    switch (event) {
        case AUDIO_PLAYER_EVENT_SONG_FINISH:
            MUSIC_LOGI("Song finished\n");
            /* Set flag to trigger song switch in LVGL thread */
            song_finish_flag = true;
            break;

        case AUDIO_PLAYER_EVENT_SONG_START:
            MUSIC_LOGI("Song started\n");
            song_start_flag = true;
            break;

        case AUDIO_PLAYER_EVENT_SONG_PAUSE:
            MUSIC_LOGI("Song paused\n");
            song_pause_flag = true;
            break;

        case AUDIO_PLAYER_EVENT_SONG_RESUME:
            MUSIC_LOGI("Song resumed\n");
            break;

        case AUDIO_PLAYER_EVENT_SONG_FAILURE:
            MUSIC_LOGE("Song playback failed\n");
            /* Set flag to trigger song switch in LVGL thread */
            song_finish_flag = true;
            break;

        case AUDIO_PLAYER_EVENT_SONG_TICK:
            if (extra_info != NULL)
            {
                int second = *((int *)extra_info);
                song_tick_second = (second >= 0) ? (uint32_t)second : 0;
                song_tick_flag = true;
            }
            break;

        case AUDIO_PLAYER_EVENT_SEEK_COMPLETE:
            if (extra_info)
            {
                seek_result_cache = *((audio_player_seek_result_t *)extra_info);
            }
            else
            {
                seek_result_cache.second = 0;
                seek_result_cache.status = AUDIO_PLAYER_ERR;
            }
            seek_complete_flag = true;
            MUSIC_LOGI("Seek complete event received\n");
            break;

        default:
            break;
    }
}

/**
 * @brief Timer callback to check song finish flag
 *
 * This function runs in LVGL thread and checks if a song has finished.
 * If so, it triggers the next song.
 *
 * @param timer LVGL timer handle
 */
static void song_check_timer_cb(lv_timer_t * timer)
{
    (void)timer;

    if (song_tick_flag)
    {
        uint32_t second = song_tick_second;
        song_tick_flag = false;
        _lv_demo_music_update_progress(second);
    }

    /* Check if song finished flag is set */
    if (song_finish_flag) {
        song_finish_flag = false;
        MUSIC_LOGI("Song finish detected in LVGL thread, switching to next song\n");
        /* Safe to call LVGL functions here */
        _lv_demo_music_album_next(true);
    }

    if (song_pause_flag)
    {
        song_pause_flag = false;
        _lv_demo_music_request_resume(false);
    }

    if (song_start_flag)
    {
        song_start_flag = false;
        _lv_demo_music_on_song_start();
    }

    if (seek_complete_flag)
    {
        audio_player_seek_result_t result = seek_result_cache;
        seek_complete_flag = false;
        bool success = (result.status == AUDIO_PLAYER_OK);
        uint32_t second = (result.second >= 0) ? (uint32_t)result.second : 0;
        _lv_demo_music_on_seek_complete(success, second);
    }
}

/**
 * @brief Mount SD card to /sd0
 *
 * This function mounts the SD card to the /sd0 directory.
 * It uses the FATFS file system.
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
static int vfs_mount_sd0_fatfs(void)
{
    int ret = BK_OK;
    static bool is_mounted = false;

    if (!is_mounted) {
        struct bk_fatfs_partition partition;
        char *fs_name = NULL;
        fs_name = "fatfs";
        partition.part_type = FATFS_DEVICE;
        partition.part_dev.device_name = FATFS_DEV_SDCARD;
        partition.mount_path = VFS_SD_0_PATITION_0;
        ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
        if (ret == BK_OK) {
            is_mounted = true;
            MUSIC_LOGI("Mounted SD card to /sd0\n");
        } else {
            MUSIC_LOGE("Failed to mount SD card, ret=%d\n", ret);
        }
    }
    return ret;
}

/**
 * @brief Unmount SD card from /sd0
 *
 * @return BK_OK on success, error code on failure
 */
static bk_err_t vfs_unmount_sd0_fatfs(void)
{
    int ret = umount(VFS_SD_0_PATITION_0);
    if (ret == BK_OK) {
        MUSIC_LOGI("Unmounted SD card from /sd0\n");
    } else {
        MUSIC_LOGE("Failed to unmount SD card, ret=%d\n", ret);
    }
    return ret;
}

/**
 * @brief Initialize audio player and load music list
 *
 * This function initializes the audio player and scans SD card for music files.
 * Music files are automatically discovered from /sd0/music_files directory.
 *
 * @return BK_OK on success, BK_FAIL on failure
 */
static bk_err_t music_player_init(void)
{
    lv_demo_music_list_item_t *item = NULL;
    int ret;

    /* Initialize audio player with event handler */
    bk_audio_player_cfg_t cfg = DEFAULT_AUDIO_PLAYER_CONFIG();
    cfg.event_handler = audio_player_event_handler;
    cfg.args = NULL;
    ret = bk_audio_player_new(&s_player_handle, &cfg);
    if (ret != AUDIO_PLAYER_OK || s_player_handle == NULL) {
        MUSIC_LOGE("Failed to create audio player, ret=%d\n", ret);
        return BK_FAIL;
    }

    ret = music_player_register_default_plugins();
    if (ret != AUDIO_PLAYER_OK) {
        MUSIC_LOGE("Failed to register audio plugins, ret=%d\n", ret);
        return BK_FAIL;
    }

    /* Initialize music list first - always initialize so list is valid */
    ret = lv_demo_music_list_init(&g_music_list);
    if (ret != BK_OK) {
        MUSIC_LOGE("Failed to initialize music list\n");
        return BK_FAIL;
    }

    /* Mount SD card first */
    ret = vfs_mount_sd0_fatfs();
    if (ret != BK_OK) {
        MUSIC_LOGE("Failed to mount SD card, UI will work but playback disabled\n");
        /* Return FAIL but list is initialized, so UI can still work */
        return BK_FAIL;
    }

    /* Scan SD card directory for music files */
    ret = lv_demo_music_list_scan_directory(&g_music_list, MUSIC_SCAN_PATH, s_player_handle);
    if (ret != BK_OK) {
        MUSIC_LOGE("Failed to scan music directory\n");
        /* Unmount SD card before returning */
        vfs_unmount_sd0_fatfs();
        return BK_FAIL;
    }

    /* Check if any music files were found */
    if (lv_demo_music_list_get_count(&g_music_list) == 0) {
        MUSIC_LOGE("No music files found in %s\n", MUSIC_SCAN_PATH);
        /* Unmount SD card before returning */
        vfs_unmount_sd0_fatfs();
        return BK_FAIL;
    }

    /* Print music list for debugging */
    lv_demo_music_list_debug_print(&g_music_list, __func__, __LINE__);

    /* Set play mode to sequence loop */
    ret = bk_audio_player_set_play_mode(s_player_handle, AUDIO_PLAYER_MODE_SEQUENCE_LOOP);
    if (ret != AUDIO_PLAYER_OK) {
        MUSIC_LOGE("Failed to set play mode, ret=%d\n", ret);
        return BK_FAIL;
    }

    /* Set volume to 50% */
    //ret = bk_audio_player_set_volume(50);
    if (ret != AUDIO_PLAYER_OK) {
        MUSIC_LOGE("Failed to set volume, ret=%d\n", ret);
        return BK_FAIL;
    }

    /* Add all music files from list to audio player */
    STAILQ_FOREACH(item, &g_music_list, next)
    {
        if (item) {
            ret = bk_audio_player_add_music(s_player_handle, item->music_info.title, item->music_info.path);
            if (ret != AUDIO_PLAYER_OK) {
                MUSIC_LOGE("Failed to add music: %s, ret=%d\n", item->music_info.title, ret);
            }
        }
    }

    MUSIC_LOGI("Audio player initialized successfully with %d songs\n", lv_demo_music_list_get_count(&g_music_list));

    /* Create a timer to check song finish flag (runs in LVGL thread) */
    song_check_timer = lv_timer_create(song_check_timer_cb, 100, NULL);  /* Check every 100ms */
    if (song_check_timer == NULL) {
        MUSIC_LOGE("Failed to create song check timer\n");
        return BK_FAIL;
    }

    return BK_OK;
}

/**
 * @brief Deinitialize audio player and unmount SD card
 */
static void music_player_deinit(void)
{
    /* Delete song check timer */
    if (song_check_timer) {
        lv_timer_del(song_check_timer);
        song_check_timer = NULL;
    }

    /* Stop and deinitialize audio player */
    if (s_player_handle) {
        bk_audio_player_stop(s_player_handle);
        bk_audio_player_delete(s_player_handle);
        s_player_handle = NULL;
    }
    MUSIC_LOGI("Audio player deinitialized\n");

    /* Clear music list */
    lv_demo_music_list_clear(&g_music_list);

    /* Unmount SD card */
    vfs_unmount_sd0_fatfs();
}
#endif /* CONFIG_AUDIO_PLAYER */

void lv_demo_music(void)
{
    original_screen_bg_color = lv_obj_get_style_bg_color(lv_scr_act(), 0);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x343247), 0);

#if CONFIG_AUDIO_PLAYER
    /* Initialize audio player and music list first, before creating UI */
    if (music_player_init() != BK_OK) {
        MUSIC_LOGE("Failed to initialize music player\n");
        /* Continue anyway, but list will be empty */
    }
#endif

    /* Create UI after music list is initialized */
    list = _lv_demo_music_list_create(lv_scr_act());
    ctrl = _lv_demo_music_main_create(lv_scr_act());

#if LV_DEMO_MUSIC_AUTO_PLAY
    auto_step_timer = lv_timer_create(auto_step_cb, 1000, NULL);
#endif
}

void lv_demo_music_close(void)
{
    /*Delete all aniamtions*/
    lv_anim_del(NULL, NULL);

#if LV_DEMO_MUSIC_AUTO_PLAY
    lv_timer_del(auto_step_timer);
#endif

#if CONFIG_AUDIO_PLAYER
    /* Deinitialize audio player */
    music_player_deinit();
#endif

    _lv_demo_music_list_close();
    _lv_demo_music_main_close();

    lv_obj_clean(lv_scr_act());

    lv_obj_set_style_bg_color(lv_scr_act(), original_screen_bg_color, 0);
}

const char * _lv_demo_music_get_title(uint32_t track_id)
{
#if CONFIG_AUDIO_PLAYER
    lv_demo_music_info_t *info = lv_demo_music_list_get_by_id(&g_music_list, track_id);
    if (info == NULL) {
        MUSIC_LOGE("Failed to get music info for track_id=%d (info is NULL)\n", track_id);
        return "No Music";
    }
    if (info->title == NULL) {
        MUSIC_LOGE("Title is NULL for track_id=%d\n", track_id);
        return "No Music";
    }
    return info->title;
#else
    return "Demo Music";
#endif
}

const char * _lv_demo_music_get_artist(uint32_t track_id)
{
#if CONFIG_AUDIO_PLAYER
    lv_demo_music_info_t *info = lv_demo_music_list_get_by_id(&g_music_list, track_id);
    if (info == NULL) {
        MUSIC_LOGE("Failed to get music info for track_id=%d (info is NULL)\n", track_id);
        return "Unknown Artist";
    }
    if (info->artist == NULL) {
        MUSIC_LOGE("Artist is NULL for track_id=%d\n", track_id);
        return "Unknown Artist";
    }
    return info->artist;
#else
    return "Demo Artist";
#endif
}

const char * _lv_demo_music_get_genre(uint32_t track_id)
{
#if CONFIG_AUDIO_PLAYER
    lv_demo_music_info_t *info = lv_demo_music_list_get_by_id(&g_music_list, track_id);
    if (info == NULL) {
        MUSIC_LOGE("Failed to get music info for track_id=%d (info is NULL)\n", track_id);
        return "No Genre";
    }
    if (info->genre == NULL) {
        MUSIC_LOGE("Genre is NULL for track_id=%d\n", track_id);
        return "No Genre";
    }
    return info->genre;
#else
    return "Demo Genre";
#endif
}

uint32_t _lv_demo_music_get_track_length(uint32_t track_id)
{
#if CONFIG_AUDIO_PLAYER
    lv_demo_music_info_t *info = lv_demo_music_list_get_by_id(&g_music_list, track_id);
    if (info == NULL) {
        /* Return default value when no SD card or no songs */
        return 180;
    }
    return info->time;
#else
    return 180;
#endif
}

uint32_t _lv_demo_music_get_track_count(void)
{
#if CONFIG_AUDIO_PLAYER
    uint32_t count = lv_demo_music_list_get_count(&g_music_list);
    /* Return at least 1 for UI to work properly, even with no SD card */
    return (count > 0) ? count : 1;
#else
    return 1;
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#if LV_DEMO_MUSIC_AUTO_PLAY
static void auto_step_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    static uint32_t state = 0;
1
#if LV_DEMO_MUSIC_LARGE
    const lv_font_t * font_small = &lv_font_montserrat_22;
    const lv_font_t * font_large = &lv_font_montserrat_32;
#else
    const lv_font_t * font_small = &lv_font_montserrat_12;
    const lv_font_t * font_large = &lv_font_montserrat_16;
#endif

    switch(state) {
        case 5:
            _lv_demo_music_album_next(true);
            break;

        case 6:
            _lv_demo_music_album_next(true);
            break;
        case 7:
            _lv_demo_music_album_next(true);
            break;
        case 8:
            _lv_demo_music_play(0);
            break;
#if LV_DEMO_MUSIC_SQUARE || LV_DEMO_MUSIC_ROUND
        case 11:
            lv_obj_scroll_by(ctrl, 0, -LV_VER_RES, LV_ANIM_ON);
            break;
        case 13:
            lv_obj_scroll_by(ctrl, 0, -LV_VER_RES, LV_ANIM_ON);
            break;
#else
        case 12:
            lv_obj_scroll_by(ctrl, 0, -LV_VER_RES, LV_ANIM_ON);
            break;
#endif
        case 15:
            lv_obj_scroll_by(list, 0, -300, LV_ANIM_ON);
            break;
        case 16:
            lv_obj_scroll_by(list, 0, 300, LV_ANIM_ON);
            break;
        case 18:
            _lv_demo_music_play(1);
            break;
        case 19:
            lv_obj_scroll_by(ctrl, 0, LV_VER_RES, LV_ANIM_ON);
            break;
#if LV_DEMO_MUSIC_SQUARE || LV_DEMO_MUSIC_ROUND
        case 20:
            lv_obj_scroll_by(ctrl, 0, LV_VER_RES, LV_ANIM_ON);
            break;
#endif
        case 30:
            _lv_demo_music_play(2);
            break;
        case 40: {
                lv_obj_t * bg = lv_layer_top();
                lv_obj_set_style_bg_color(bg, lv_color_hex(0x6f8af6), 0);
                lv_obj_set_style_text_color(bg, lv_color_white(), 0);
                lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);
                lv_obj_fade_in(bg, 400, 0);
                lv_obj_t * dsc = lv_label_create(bg);
                lv_obj_set_style_text_font(dsc, font_small, 0);
                lv_label_set_text(dsc, "The average FPS is");
                lv_obj_align(dsc, LV_ALIGN_TOP_MID, 0, 90);

                lv_obj_t * num = lv_label_create(bg);
                lv_obj_set_style_text_font(num, font_large, 0);
#if LV_USE_PERF_MONITOR
                lv_label_set_text_fmt(num, "%d", lv_refr_get_fps_avg());
#endif
                lv_obj_align(num, LV_ALIGN_TOP_MID, 0, 120);

                lv_obj_t * attr = lv_label_create(bg);
                lv_obj_set_style_text_align(attr, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_set_style_text_font(attr, font_small, 0);
#if LV_DEMO_MUSIC_SQUARE || LV_DEMO_MUSIC_ROUND
                lv_label_set_text(attr, "Copyright 2020 LVGL Kft.\nwww.lvgl.io | lvgl@lvgl.io");
#else
                lv_label_set_text(attr, "Copyright 2020 LVGL Kft. | www.lvgl.io | lvgl@lvgl.io");
#endif
                lv_obj_align(attr, LV_ALIGN_BOTTOM_MID, 0, -10);
                break;
            }
        case 41:
            lv_scr_load(lv_obj_create(NULL));
            _lv_demo_music_pause();
            break;
    }
    state++;
}

#endif /*LV_DEMO_MUSIC_AUTO_PLAY*/

#endif /*LV_USE_DEMO_MUSIC*/
