/**
 * @file lv_demo_music_list_mgr.h
 * Music list manager based on BSD queue
 */

#ifndef LV_DEMO_MUSIC_LIST_MGR_H
#define LV_DEMO_MUSIC_LIST_MGR_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

#if LV_USE_DEMO_MUSIC

#include <components/bk_audio/audio_pipeline/bsd_queue.h>
#include <common/bk_err.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief Music information structure
 *
 * Note: title, artist, genre, path are dynamically allocated to save memory.
 * Memory is allocated when adding to list and freed when clearing list.
 */
typedef struct lv_demo_music_info
{
    char        *title;         /*!< Song title (dynamically allocated) */
    char        *artist;        /*!< Artist name (dynamically allocated) */
    char        *genre;         /*!< Music genre (dynamically allocated) */
    uint32_t    time;           /*!< Song duration in seconds */
    char        *path;          /*!< Full file path (dynamically allocated) */
    uint32_t    track_id;       /*!< Track ID */
} lv_demo_music_info_t;

/**
 * @brief Music list item
 */
typedef struct lv_demo_music_list_item
{
    STAILQ_ENTRY(lv_demo_music_list_item)   next;          /*!< Next item in list */
    lv_demo_music_info_t                    music_info;    /*!< Music information */
} lv_demo_music_list_item_t;

/**
 * @brief Music list type
 */
typedef STAILQ_HEAD(lv_demo_music_list, lv_demo_music_list_item) lv_demo_music_list_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Initialize the music list
 *
 * @param[in] music_list  Pointer to the music list
 *
 * @return
 *     - BK_OK: Success
 *     - BK_FAIL: Failed
 */
bk_err_t lv_demo_music_list_init(lv_demo_music_list_t *music_list);

/**
 * @brief Add music to list
 *
 * Note: This function will dynamically allocate memory for title, artist, genre, and path.
 * The allocated memory will be freed when lv_demo_music_list_clear() is called.
 *
 * @param[in] music_list  Pointer to the music list
 * @param[in] music_info  Pointer to the music info to add (strings must be valid pointers)
 *
 * @return
 *     - BK_OK: Success
 *     - BK_FAIL: Failed (including memory allocation failure)
 */
bk_err_t lv_demo_music_list_add(lv_demo_music_list_t *music_list, lv_demo_music_info_t *music_info);

/**
 * @brief Get music info by track id
 *
 * @param[in] music_list  Pointer to the music list
 * @param[in] track_id    Track ID to search for
 *
 * @return
 *     - Not NULL: Pointer to the music info
 *     - NULL: Music not found
 */
lv_demo_music_info_t *lv_demo_music_list_get_by_id(lv_demo_music_list_t *music_list, uint32_t track_id);

/**
 * @brief Get total number of music tracks
 *
 * @param[in] music_list  Pointer to the music list
 *
 * @return Total number of tracks
 */
uint32_t lv_demo_music_list_get_count(lv_demo_music_list_t *music_list);

/**
 * @brief Clear and free all nodes in the music list
 *
 * @param[in] music_list  Pointer to the music list
 *
 * @return
 *     - BK_OK: Success
 *     - BK_FAIL: Failed
 */
bk_err_t lv_demo_music_list_clear(lv_demo_music_list_t *music_list);

/**
 * @brief Scan SD card directory and add music files to list
 *
 * This function scans the specified directory for music files (e.g., .mp3, .wav)
 * and automatically adds them to the music list.
 * File name format: "title(artist).ext" or "title.ext"
 * If artist is not provided, uses "Unknown artist"
 * If player_handle is non-NULL, metadata is extracted via bk_audio_player_get_metadata_from_file.
 *
 * @param[in] music_list     Pointer to the music list
 * @param[in] directory     Directory path to scan
 * @param[in] player_handle Audio player handle for metadata (NULL to use filename fallback only)
 *
 * @return
 *     - BK_OK: Success
 *     - BK_FAIL: Failed
 */
bk_err_t lv_demo_music_list_scan_directory(lv_demo_music_list_t *music_list, const char *directory, void *player_handle);

/**
 * @brief Parse filename to extract title and artist (dynamically allocated)
 *
 * Filename format: "title(artist).ext" or "title.ext"
 * Examples:
 *   - "abc(de).mp3" -> title: "abc", artist: "de"
 *   - "abc.mp3" -> title: "abc", artist: "Unknown artist"
 *
 * Note: This function dynamically allocates memory for title and artist.
 * Caller is responsible for freeing the memory.
 *
 * @param[in]  filename   Input filename
 * @param[out] title      Output title pointer (will be allocated)
 * @param[out] artist     Output artist pointer (will be allocated)
 *
 * @return
 *     - BK_OK: Success
 *     - BK_FAIL: Failed (including memory allocation failure)
 */
bk_err_t lv_demo_music_parse_filename(const char *filename, char **title, char **artist);

/**
 * @brief Debug print music list
 *
 * @param[in] music_list  Pointer to the music list
 * @param[in] func        Function name for debug
 * @param[in] line        Line number for debug
 *
 * @return None
 */
void lv_demo_music_list_debug_print(lv_demo_music_list_t *music_list, const char *func, int line);

#endif /*LV_USE_DEMO_MUSIC*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_MUSIC_LIST_MGR_H*/

