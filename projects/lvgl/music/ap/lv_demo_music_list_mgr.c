/**
 * @file lv_demo_music_list_mgr.c
 * Music list manager implementation
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_demo_music_list_mgr.h"

#if LV_USE_DEMO_MUSIC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <components/log.h>
#include <components/bk_audio_player/bk_audio_player.h>
#include <os/mem.h>
#include <os/str.h>
#include "bk_vfs.h"
#include "bk_posix.h"

/*********************
 *      DEFINES
 *********************/
#define TAG  "MUSIC_LIST_MGR"

/* Default values for music metadata */
#define DEFAULT_GENRE       "Metal - 2015"
#define DEFAULT_TIME        180
#define UNKNOWN_ARTIST      "Unknown artist"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool is_music_file(const char *filename);
static char *trim_string(char *str);
static bk_err_t scan_directory_recursive(lv_demo_music_list_t *music_list, const char *directory, uint32_t *track_id, void *player_handle);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bk_err_t lv_demo_music_list_init(lv_demo_music_list_t *music_list)
{
    /* Check input parameter */
    if (music_list == NULL)
    {
        BK_LOGE(TAG, "music_list is NULL\n");
        return BK_FAIL;
    }

    /* Initialize the list */
    STAILQ_INIT(music_list);

    return BK_OK;
}

bk_err_t lv_demo_music_list_add(lv_demo_music_list_t *music_list, lv_demo_music_info_t *music_info)
{
    /* Check input parameters */
    if (music_list == NULL || music_info == NULL)
    {
        BK_LOGE(TAG, "music_list or music_info is NULL\n");
        return BK_FAIL;
    }

    /* Check if strings are valid */
    if (music_info->title == NULL || music_info->artist == NULL ||
        music_info->genre == NULL || music_info->path == NULL)
    {
        BK_LOGE(TAG, "music_info contains NULL string pointers\n");
        return BK_FAIL;
    }

    /* Allocate memory for new item */
    lv_demo_music_list_item_t *new_item = (lv_demo_music_list_item_t *)os_malloc(sizeof(lv_demo_music_list_item_t));
    if (new_item == NULL)
    {
        BK_LOGE(TAG, "Failed to allocate memory for music list item\n");
        return BK_FAIL;
    }

    /* Initialize the new item */
    os_memset(new_item, 0, sizeof(lv_demo_music_list_item_t));

    /* Dynamically allocate and copy title */
    new_item->music_info.title = (char *)os_malloc(strlen(music_info->title) + 1);
    if (new_item->music_info.title == NULL)
    {
        BK_LOGE(TAG, "Failed to allocate memory for title\n");
        os_free(new_item);
        return BK_FAIL;
    }
    strcpy(new_item->music_info.title, music_info->title);

    /* Dynamically allocate and copy artist */
    new_item->music_info.artist = (char *)os_malloc(strlen(music_info->artist) + 1);
    if (new_item->music_info.artist == NULL)
    {
        BK_LOGE(TAG, "Failed to allocate memory for artist\n");
        os_free(new_item->music_info.title);
        os_free(new_item);
        return BK_FAIL;
    }
    strcpy(new_item->music_info.artist, music_info->artist);

    /* Dynamically allocate and copy genre */
    new_item->music_info.genre = (char *)os_malloc(strlen(music_info->genre) + 1);
    if (new_item->music_info.genre == NULL)
    {
        BK_LOGE(TAG, "Failed to allocate memory for genre\n");
        os_free(new_item->music_info.artist);
        os_free(new_item->music_info.title);
        os_free(new_item);
        return BK_FAIL;
    }
    strcpy(new_item->music_info.genre, music_info->genre);

    /* Dynamically allocate and copy path */
    new_item->music_info.path = (char *)os_malloc(strlen(music_info->path) + 1);
    if (new_item->music_info.path == NULL)
    {
        BK_LOGE(TAG, "Failed to allocate memory for path\n");
        os_free(new_item->music_info.genre);
        os_free(new_item->music_info.artist);
        os_free(new_item->music_info.title);
        os_free(new_item);
        return BK_FAIL;
    }
    strcpy(new_item->music_info.path, music_info->path);

    /* Copy other fields */
    new_item->music_info.time = music_info->time;
    new_item->music_info.track_id = music_info->track_id;

    /* Insert new item at the end of list */
    STAILQ_INSERT_TAIL(music_list, new_item, next);

    BK_LOGI(TAG, "Added music: id=%d, title=%s, artist=%s\n",
            music_info->track_id, music_info->title, music_info->artist);

    return BK_OK;
}

lv_demo_music_info_t *lv_demo_music_list_get_by_id(lv_demo_music_list_t *music_list, uint32_t track_id)
{
    lv_demo_music_list_item_t *item = NULL;

    /* Check input parameter */
    if (music_list == NULL)
    {
        BK_LOGE(TAG, "music_list is NULL\n");
        return NULL;
    }

    /* Check if list is initialized: stqh_last should not be NULL after STAILQ_INIT */
    if (music_list->stqh_last == NULL)
    {
        /* List not initialized yet */
        return NULL;
    }

    /* Search for the music by track id */
    STAILQ_FOREACH(item, music_list, next)
    {
        if (item && item->music_info.track_id == track_id)
        {
            return &item->music_info;
        }
    }

    return NULL;
}

uint32_t lv_demo_music_list_get_count(lv_demo_music_list_t *music_list)
{
    uint32_t count = 0;
    lv_demo_music_list_item_t *item = NULL;

    /* Check input parameter */
    if (music_list == NULL)
    {
        BK_LOGE(TAG, "music_list is NULL\n");
        return 0;
    }

    /* Check if list is initialized */
    if (music_list->stqh_last == NULL)
    {
        /* List not initialized yet */
        return 0;
    }

    /* Count all items in the list */
    STAILQ_FOREACH(item, music_list, next)
    {
        if (item)
        {
            count++;
        }
    }

    return count;
}

bk_err_t lv_demo_music_list_clear(lv_demo_music_list_t *music_list)
{
    lv_demo_music_list_item_t *item = NULL;
    lv_demo_music_list_item_t *tmp_item = NULL;

    /* Check input parameter */
    if (music_list == NULL)
    {
        BK_LOGE(TAG, "music_list is NULL\n");
        return BK_FAIL;
    }

    /* Traverse and free all nodes in the list */
    STAILQ_FOREACH_SAFE(item, music_list, next, tmp_item)
    {
        if (item)
        {
            STAILQ_REMOVE(music_list, item, lv_demo_music_list_item, next);

            /* Free dynamically allocated strings */
            if (item->music_info.title != NULL)
            {
                os_free(item->music_info.title);
            }
            if (item->music_info.artist != NULL)
            {
                os_free(item->music_info.artist);
            }
            if (item->music_info.genre != NULL)
            {
                os_free(item->music_info.genre);
            }
            if (item->music_info.path != NULL)
            {
                os_free(item->music_info.path);
            }

            /* Free the item itself */
            os_free(item);
        }
    }

    BK_LOGI(TAG, "Music list cleared\n");

    return BK_OK;
}

bk_err_t lv_demo_music_parse_filename(const char *filename, char **title, char **artist)
{
    char temp_filename[256];
    char *left_paren = NULL;
    char *right_paren = NULL;
    char *dot = NULL;
    char *title_str = NULL;
    char *artist_str = NULL;

    /* Check input parameters */
    if (filename == NULL || title == NULL || artist == NULL)
    {
        BK_LOGE(TAG, "Invalid parameters for music_parse_filename\n");
        return BK_FAIL;
    }

    /* Initialize output pointers */
    *title = NULL;
    *artist = NULL;

    /* Copy filename to temporary buffer for parsing */
    os_memset(temp_filename, 0, sizeof(temp_filename));
    strncpy(temp_filename, filename, sizeof(temp_filename) - 1);

    /* Find the last dot (file extension) */
    dot = strrchr(temp_filename, '.');
    if (dot != NULL)
    {
        *dot = '\0';  /* Remove file extension */
    }

    /* Find parentheses for artist */
    left_paren = strchr(temp_filename, '(');
    right_paren = strchr(temp_filename, ')');

    if (left_paren != NULL && right_paren != NULL && right_paren > left_paren)
    {
        /* Artist found in parentheses */
        *left_paren = '\0';
        *right_paren = '\0';

        title_str = temp_filename;
        artist_str = left_paren + 1;

        /* Trim whitespace */
        title_str = trim_string(title_str);
        artist_str = trim_string(artist_str);

        /* Allocate and copy title */
        *title = (char *)os_malloc(strlen(title_str) + 1);
        if (*title == NULL)
        {
            BK_LOGE(TAG, "Failed to allocate memory for title\n");
            return BK_FAIL;
        }
        strcpy(*title, title_str);

        /* Allocate and copy artist */
        if (strlen(artist_str) > 0)
        {
            *artist = (char *)os_malloc(strlen(artist_str) + 1);
            if (*artist == NULL)
            {
                BK_LOGE(TAG, "Failed to allocate memory for artist\n");
                os_free(*title);
                *title = NULL;
                return BK_FAIL;
            }
            strcpy(*artist, artist_str);
        }
        else
        {
            /* Empty artist, use default */
            *artist = (char *)os_malloc(strlen(UNKNOWN_ARTIST) + 1);
            if (*artist == NULL)
            {
                BK_LOGE(TAG, "Failed to allocate memory for artist\n");
                os_free(*title);
                *title = NULL;
                return BK_FAIL;
            }
            strcpy(*artist, UNKNOWN_ARTIST);
        }
    }
    else
    {
        /* No artist in filename */
        title_str = temp_filename;
        title_str = trim_string(title_str);

        /* Allocate and copy title */
        *title = (char *)os_malloc(strlen(title_str) + 1);
        if (*title == NULL)
        {
            BK_LOGE(TAG, "Failed to allocate memory for title\n");
            return BK_FAIL;
        }
        strcpy(*title, title_str);

        /* Use default artist */
        *artist = (char *)os_malloc(strlen(UNKNOWN_ARTIST) + 1);
        if (*artist == NULL)
        {
            BK_LOGE(TAG, "Failed to allocate memory for artist\n");
            os_free(*title);
            *title = NULL;
            return BK_FAIL;
        }
        strcpy(*artist, UNKNOWN_ARTIST);
    }

    return BK_OK;
}

/**
 * @brief Recursively scan directory for music files (internal helper)
 * 
 * @param[in] music_list  Pointer to the music list
 * @param[in] directory   Directory path to scan
 * @param[in,out] track_id  Current track ID counter (will be incremented)
 * 
 * @return BK_OK on success, BK_FAIL on failure
 */
static bk_err_t scan_directory_recursive(lv_demo_music_list_t *music_list, const char *directory, uint32_t *track_id, void *player_handle)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    lv_demo_music_info_t music_info;
    int ret;
    char full_path[256];

    /* Check input parameters */
    if (music_list == NULL || directory == NULL)
    {
        BK_LOGE(TAG, "music_list or directory is NULL\n");
        return BK_FAIL;
    }

    /* Open directory */
    dir = opendir(directory);
    if (dir == NULL)
    {
        BK_LOGE(TAG, "Failed to open directory: %s\n", directory);
        return BK_FAIL;
    }

    BK_LOGI(TAG, "Scanning directory: %s\n", directory);

    /* Read all entries in directory */
    while ((entry = readdir(dir)) != NULL)
    {
        /* Skip "." and ".." */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        /* Build full path for entry */
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        /* Check if it's a directory - recursively scan it */
        if (entry->d_type == DT_DIR)
        {
            BK_LOGI(TAG, "Found subdirectory: %s, scanning recursively\n", entry->d_name);
            ret = scan_directory_recursive(music_list, full_path, track_id, player_handle);
            if (ret != BK_OK)
            {
                BK_LOGW(TAG, "Failed to scan subdirectory: %s\n", full_path);
            }
            continue;
        }

        /* Check if it's a music file with supported format */
        if (!is_music_file(entry->d_name))
        {
            /* Check if file has audio extension but decoder not enabled */
            const char *ext = strrchr(entry->d_name, '.');
            if (ext != NULL)
            {
                /* Log warning for common audio formats that might not be enabled */
                if (os_strcasecmp(ext, ".mp3") == 0 || os_strcasecmp(ext, ".wav") == 0 ||
                    os_strcasecmp(ext, ".aac") == 0 || os_strcasecmp(ext, ".m4a") == 0 ||
                    os_strcasecmp(ext, ".flac") == 0 || os_strcasecmp(ext, ".ogg") == 0 ||
                    os_strcasecmp(ext, ".opus") == 0 || os_strcasecmp(ext, ".amr") == 0 ||
                    os_strcasecmp(ext, ".ts") == 0)
                {
                    BK_LOGV(TAG, "Skipping '%s': format not enabled in Kconfig\n", entry->d_name);
                }
            }
            continue;
        }

        /* Initialize music info */
        os_memset(&music_info, 0, sizeof(lv_demo_music_info_t));

        /* Extract metadata using the new audio metadata API */
        /* Allocate metadata on heap to avoid stack overflow in recursive function */
        audio_metadata_t *metadata = (audio_metadata_t *)os_malloc(sizeof(audio_metadata_t));
        if (metadata == NULL)
        {
            BK_LOGE(TAG, "Failed to allocate memory for metadata\n");
            continue;
        }

        BK_LOGI(TAG, "Extracting metadata for: %s\n", entry->d_name);
        if (player_handle != NULL)
        {
            ret = bk_audio_player_get_metadata_from_file((bk_audio_player_handle_t)player_handle, full_path, metadata);
        }
        else
        {
            ret = -1;
        }

        if (ret == 0)
        {
            /* Successfully extracted metadata */
            BK_LOGI(TAG, "Metadata extracted: title='%s', artist='%s', genre='%s', duration=%us\n",
                    metadata->title, metadata->artist, metadata->genre, (uint32_t)(metadata->duration / 1000));

            /* Use title from metadata if available, otherwise parse from filename */
            if (metadata->title[0] != '\0')
            {
                music_info.title = (char *)os_malloc(os_strlen(metadata->title) + 1);
                if (music_info.title != NULL)
                {
                    os_strcpy(music_info.title, metadata->title);
                }
            }

            /* Use artist from metadata if available, otherwise parse from filename */
            if (metadata->artist[0] != '\0')
            {
                music_info.artist = (char *)os_malloc(os_strlen(metadata->artist) + 1);
                if (music_info.artist != NULL)
                {
                    os_strcpy(music_info.artist, metadata->artist);
                }
            }

            /* Use genre from metadata if available */
            if (metadata->genre[0] != '\0')
            {
                music_info.genre = (char *)os_malloc(os_strlen(metadata->genre) + 1);
                if (music_info.genre != NULL)
                {
                    os_strcpy(music_info.genre, metadata->genre);
                }
            }

            /* Use duration from metadata if available (convert from ms to seconds) */
            uint32_t duration_ms = (uint32_t)metadata->duration;
            if (duration_ms > 0)
            {
                music_info.time = duration_ms / 1000;  /* Integer division to convert ms to seconds */
                BK_LOGI(TAG, "Duration set from metadata: %ums -> %us\n",
                        duration_ms, music_info.time);
            }
            else
            {
                BK_LOGW(TAG, "Duration from metadata is 0\n");
            }
        }
        else
        {
            BK_LOGW(TAG, "Failed to extract metadata (error %d), will use fallback\n", ret);
        }

        /* Fallback: parse filename if title/artist not set from metadata */
        if (music_info.title == NULL || music_info.artist == NULL)
        {
            char *parsed_title = NULL;
            char *parsed_artist = NULL;

            ret = lv_demo_music_parse_filename(entry->d_name, &parsed_title, &parsed_artist);
            if (ret == BK_OK)
            {
                if (music_info.title == NULL)
                {
                    music_info.title = parsed_title;
                }
                else
                {
                    os_free(parsed_title);
                }

                if (music_info.artist == NULL)
                {
                    music_info.artist = parsed_artist;
                }
                else
                {
                    os_free(parsed_artist);
                }
            }
            else
            {
                BK_LOGE(TAG, "Failed to parse filename: %s\n", entry->d_name);
                os_free(metadata);
                continue;
            }
        }

        /* Set default genre if not available from metadata */
        if (music_info.genre == NULL)
        {
            music_info.genre = (char *)os_malloc(os_strlen(DEFAULT_GENRE) + 1);
            if (music_info.genre == NULL)
            {
                BK_LOGE(TAG, "Failed to allocate memory for genre\n");
                os_free(music_info.title);
                os_free(music_info.artist);
                os_free(metadata);
                continue;
            }
            os_strcpy(music_info.genre, DEFAULT_GENRE);
        }

        /* Set default duration if not available from metadata */
        if (music_info.time == 0)
        {
            BK_LOGW(TAG, "Duration not set, using default: %us\n", DEFAULT_TIME);
            music_info.time = DEFAULT_TIME;
        }

        /* Allocate and copy file path */
        music_info.path = (char *)os_malloc(os_strlen(full_path) + 1);
        if (music_info.path == NULL)
        {
            BK_LOGE(TAG, "Failed to allocate memory for path\n");
            os_free(music_info.genre);
            os_free(music_info.title);
            os_free(music_info.artist);
            os_free(metadata);
            continue;
        }
        os_strcpy(music_info.path, full_path);

        /* Set track ID */
        music_info.track_id = *track_id;

        /* Add to music list */
        ret = lv_demo_music_list_add(music_list, &music_info);
        if (ret != BK_OK)
        {
            BK_LOGE(TAG, "Failed to add music to list: %s\n", entry->d_name);
            /* Free allocated memory on failure */
            os_free(music_info.path);
            os_free(music_info.genre);
            os_free(music_info.title);
            os_free(music_info.artist);
            os_free(metadata);
            continue;
        }

        /* Memory ownership transferred to list, increment track ID */
        (*track_id)++;

        /* Free temporary allocated strings (lv_demo_music_list_add makes its own copies) */
        os_free(music_info.path);
        os_free(music_info.genre);
        os_free(music_info.title);
        os_free(music_info.artist);
        os_free(metadata);
    }

    /* Close directory */
    closedir(dir);

    return BK_OK;
}

bk_err_t lv_demo_music_list_scan_directory(lv_demo_music_list_t *music_list, const char *directory, void *player_handle)
{
    uint32_t track_id = 0;
    bk_err_t ret;

    /* Check input parameters */
    if (music_list == NULL || directory == NULL)
    {
        BK_LOGE(TAG, "music_list or directory is NULL\n");
        return BK_FAIL;
    }

    BK_LOGI(TAG, "Starting recursive scan from: %s\n", directory);

    /* Log supported audio formats (all built-in decoders) */
    BK_LOGI(TAG, "Supported audio formats:");
    BK_LOGI(TAG, "  - MP3");
    BK_LOGI(TAG, "  - WAV");
    BK_LOGI(TAG, "  - AAC");
    BK_LOGI(TAG, "  - M4A");
    BK_LOGI(TAG, "  - FLAC");
    BK_LOGI(TAG, "  - OGG/Opus");
    BK_LOGI(TAG, "  - AMR");
    BK_LOGI(TAG, "  - TS");

    /* Call recursive scan helper */
    ret = scan_directory_recursive(music_list, directory, &track_id, player_handle);

    BK_LOGI(TAG, "Recursive scan complete, found %d music files total\n", track_id);

    return ret;
}

void lv_demo_music_list_debug_print(lv_demo_music_list_t *music_list, const char *func, int line)
{
    lv_demo_music_list_item_t *item = NULL;

    if (music_list == NULL)
    {
        BK_LOGD(TAG, "music_list is NULL\n");
        return;
    }

    BK_LOGD(TAG, "----------------- [%s] %d, music_list -----------------\n", func, line);
    STAILQ_FOREACH(item, music_list, next)
    {
        if (item)
        {
            BK_LOGD(TAG, "track_id: %d, title: %s, artist: %s, genre: %s, time: %d, path: %s\n",
                    item->music_info.track_id,
                    item->music_info.title,
                    item->music_info.artist,
                    item->music_info.genre,
                    item->music_info.time,
                    item->music_info.path);
        }
    }
    BK_LOGD(TAG, "\n");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief Check if filename is a music file based on extension
 *
 * This function checks the file extension against the audio decoder formats
 * supported by the bk_audio_player component, as configured in Kconfig.
 * Only files with extensions matching enabled decoders will be considered
 * valid music files.
 *
 * @param[in] filename  Filename to check
 *
 * @return
 *     - true: Is a music file with supported format
 *     - false: Not a music file or unsupported format
 */
static bool is_music_file(const char *filename)
{
    const char *ext = NULL;

    /* Check input parameter */
    if (filename == NULL)
    {
        return false;
    }

    /* Find file extension */
    ext = strrchr(filename, '.');
    if (ext == NULL)
    {
        return false;
    }

    /* Check if extension matches any built-in decoder format */

    /* MP3 format */
    if (os_strcasecmp(ext, ".mp3") == 0)
    {
        return true;
    }

    /* WAV format */
    if (os_strcasecmp(ext, ".wav") == 0)
    {
        return true;
    }

    /* AAC format */
    if (os_strcasecmp(ext, ".aac") == 0)
    {
        return true;
    }

    /* M4A format */
    if (os_strcasecmp(ext, ".m4a") == 0)
    {
        return true;
    }

    /* FLAC format */
    if (os_strcasecmp(ext, ".flac") == 0)
    {
        return true;
    }

    /* Opus format */
    if (os_strcasecmp(ext, ".opus") == 0)
    {
        return true;
    }

    /* OGG format */
    if (os_strcasecmp(ext, ".ogg") == 0)
    {
        return true;
    }

    /* AMR format */
    if (os_strcasecmp(ext, ".amr") == 0)
    {
        return true;
    }

    /* TS format */
    if (os_strcasecmp(ext, ".ts") == 0)
    {
        return true;
    }

    /* No matching format found or format not enabled */
    return false;
}

/**
 * @brief Trim leading and trailing whitespace from string
 *
 * @param[in] str  String to trim
 *
 * @return Pointer to trimmed string
 */
static char *trim_string(char *str)
{
    char *end = NULL;

    if (str == NULL)
    {
        return NULL;
    }

    /* Trim leading space */
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r')
    {
        str++;
    }

    if (*str == '\0')  /* All spaces */
    {
        return str;
    }

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
    {
        end--;
    }

    /* Write new null terminator */
    *(end + 1) = '\0';

    return str;
}

#endif /*LV_USE_DEMO_MUSIC*/

