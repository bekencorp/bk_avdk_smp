#include <common/bk_include.h>
#include <components/avdk_utils/avdk_error.h>
#include <components/bk_display.h>
#include <components/bk_camera_ctlr.h>
#include "audio_recorder_device.h"
#include <stdint.h>
#include <os/str.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/pwr_clk.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>
#include <components/bk_audio/audio_encoders/aac_encoder.h>
#include "cli.h"
#include "video_recorder_cli.h"
#include "video_player_common.h"
#include "module_test_cli.h"  // This includes extern declarations for voice handles
#include "frame_buffer.h"
#include "lcd_panel_devices.h"
#include <components/bk_video_recorder.h>
#include "components/bk_video_recorder_types.h"
#include "bk_video_recorder_ctlr.h"
#include "driver/h264_types.h"

#define TAG "video_recorder_cli"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

static const lcd_device_t *lcd_device = &lcd_device_st7282;

// Static handles for video recording (DVP + MIC)
static bk_video_recorder_handle_t video_recorder_handle = NULL;
static bk_camera_ctlr_handle_t video_recorder_dvp_handle = NULL;
static bk_display_ctlr_handle_t video_recorder_lcd_handle = NULL;  // LCD handle for display during recording
static audio_recorder_device_handle_t video_recorder_audio_recorder_handle = NULL;
// Frame buffer queue for recording (frames from DVP)
#define VIDEO_RECORDER_FRAME_QUEUE_SIZE 4  // Queue size for frame buffers (increased to reduce frame drops)
static beken_queue_t video_recorder_frame_queue = NULL;  // Queue to store frame_buffer_t pointers
static image_format_t video_recorder_frame_format = IMAGE_MJPEG;  // Format of frames for proper release
// Statistics for frame drops
static uint32_t video_recorder_frame_drop_count = 0;  // Count of dropped frames due to queue full
static uint32_t video_recorder_frame_total_count = 0;  // Total frames from DVP
// Audio recorder device handle is used instead of circular buffer
// Static buffer for audio data reading (avoid stack overflow)
static uint8_t video_recorder_audio_temp_buffer[4096];

// Record output file path buffer (avoid using temporary stack buffer for file extension changes).
static char video_recorder_output_path[256] = {0};

// Audio codec is configured per start command. CLI always passes raw encoded frames to recorder.

// =============================================================================
// Recording duration limit
// =============================================================================

// Maximum recording duration: 5 minutes.
// Use a one-shot timer to auto-stop recording and avoid unbounded file growth.
#define VIDEO_RECORDER_MAX_DURATION_MS   (5 * MINUTES)

static beken2_timer_t video_recorder_auto_stop_timer = {0};
static uint32_t video_recorder_session_id = 0;
static beken_thread_t video_recorder_auto_stop_thread = NULL;
static bool video_recorder_auto_stop_timer_inited = false;
static bool video_recorder_auto_stop_timer_started = false;

// Note:
// For AVI, ADTS header is prepended inside AVI write path (bk_video_recorder_avi.c).
// The CLI layer always passes raw AAC access units to the recorder.

// DVP frame malloc callback for recording
static frame_buffer_t *video_recorder_dvp_frame_malloc(image_format_t format, uint32_t size)
{
    frame_buffer_t *frame = NULL;

    // For recording, we prefer encoded formats (MJPEG/H264) to reduce file size
    if (format == IMAGE_MJPEG || format == IMAGE_H264 || format == IMAGE_H265)
    {
        frame = frame_buffer_encode_malloc(size);
    }
    else if (format == IMAGE_YUV)
    {
        frame = frame_buffer_display_malloc(size);
    }
    else
    {
        LOGE("%s: unsupported format: %d\n", __func__, format);
        return NULL;
    }

    if (frame)
    {
        frame->sequence = 0;
        frame->length = 0;
        frame->timestamp = 0;
        frame->size = size;
    }

    return frame;
}

// DVP frame complete callback for recording
// - YUV frames: display to LCD
// - Encoded frames (MJPEG/H264): push to recording queue
static void video_recorder_dvp_frame_complete(image_format_t format, frame_buffer_t *frame, int result)
{
    if (frame == NULL)
    {
        return;
    }

    // Only process valid frames
    if (result == AVDK_ERR_OK && frame->frame != NULL && frame->length > 0)
    {
        if (format == IMAGE_YUV)
        {
            // Display YUV frame to LCD
            if (video_recorder_lcd_handle != NULL)
            {
                // Set pixel format for display
                frame->fmt = PIXEL_FMT_YUYV;

                // Flush frame to LCD display
                avdk_err_t ret = bk_display_flush(video_recorder_lcd_handle, frame, display_frame_free_cb);
                if (ret != AVDK_ERR_OK)
                {
                    LOGE("%s: bk_display_flush failed, ret:%d\n", __func__, ret);
                    // Free frame on error
                    frame_buffer_display_free(frame);
                }
                // If successful, frame will be freed by display_frame_free_cb callback
            }
            else
            {
                // LCD not opened, free frame directly
                frame_buffer_display_free(frame);
            }
        }
        else if (format == IMAGE_MJPEG || format == IMAGE_H264 || format == IMAGE_H265)
        {
            // Save format for later use
            video_recorder_frame_format = format;

            // Push encoded frame to recording queue (non-blocking, must return quickly from DVP callback)
            if (video_recorder_frame_queue != NULL)
            {
                video_recorder_frame_total_count++;
                // Non-blocking push (timeout=0) to avoid delaying DVP callback
                // If queue is full, drop frame to prevent blocking
                bk_err_t ret = rtos_push_to_queue(&video_recorder_frame_queue, &frame, 0);
                if (ret != kNoErr)
                {
                    // Queue is full, free this frame to avoid memory leak
                    video_recorder_frame_drop_count++;
                    if (video_recorder_frame_drop_count % 30 == 0)  // Log every 30 dropped frames to avoid spam
                    {
                        LOGW("%s: Frame queue full, dropped %u frames (total: %u, drop rate: %.1f%%)\n",
                             __func__, video_recorder_frame_drop_count, video_recorder_frame_total_count,
                             (video_recorder_frame_drop_count * 100.0f) / video_recorder_frame_total_count);
                    }
                    frame_buffer_encode_free(frame);
                }
            }
            else
            {
                // Queue not initialized, free frame
                frame_buffer_encode_free(frame);
            }
        }
        else
        {
            // Unsupported format, free frame
            if (format == IMAGE_MJPEG || format == IMAGE_H264 || format == IMAGE_H265)
            {
                frame_buffer_encode_free(frame);
            }
            else if (format == IMAGE_YUV)
            {
                frame_buffer_display_free(frame);
            }
        }
    }
    else
    {
        // Free frame on error
        if (format == IMAGE_MJPEG || format == IMAGE_H264 || format == IMAGE_H265)
        {
            frame_buffer_encode_free(frame);
        }
        else if (format == IMAGE_YUV)
        {
            frame_buffer_display_free(frame);
        }
    }
}

static const bk_dvp_callback_t video_recorder_dvp_cbs = {
    .malloc = video_recorder_dvp_frame_malloc,
    .complete = video_recorder_dvp_frame_complete,
};


// Video record callback: get frame data from queue
static int video_recorder_get_frame_cb(void *user_data, video_recorder_frame_data_t *frame_data)
{
    if (frame_data == NULL || video_recorder_frame_queue == NULL)
    {
        return -1;
    }

    // Pop frame from queue (non-blocking, return immediately if no frame available)
    frame_buffer_t *frame = NULL;
    bk_err_t ret = rtos_pop_from_queue(&video_recorder_frame_queue, &frame, 0);
    if (ret != kNoErr || frame == NULL)
    {
        // No frame available, recording thread will retry in next loop
        return -1;
    }

    // Verify frame is valid
    if (frame->frame == NULL || frame->length == 0)
    {
        // Invalid frame, free it and return error
        if (video_recorder_frame_format == IMAGE_MJPEG ||
            video_recorder_frame_format == IMAGE_H264 ||
            video_recorder_frame_format == IMAGE_H265)
        {
            frame_buffer_encode_free(frame);
        }
        else if (video_recorder_frame_format == IMAGE_YUV)
        {
            frame_buffer_display_free(frame);
        }
        return -1;
    }

    // Fill frame data structure
    frame_data->data = frame->frame;
    frame_data->length = frame->length;
    frame_data->width = frame->width;
    frame_data->height = frame->height;
    frame_data->frame_buffer = frame;  // Store frame buffer pointer for later release
    // Use driver-provided H264 type flag to mark key frames for recording start.
    if (video_recorder_frame_format == IMAGE_H264)
    {
        frame_data->is_key_frame = (frame->h264_type & (1U << H264_NAL_I_FRAME)) ||
                                   (frame->h264_type & (1U << H264_NAL_IDR_SLICE));
    }
    else
    {
        frame_data->is_key_frame = 1;
    }

    return 0;  // Success
}

// Video record callback: get audio data from MIC
// Note: This callback is called by recording thread to get audio data
// The audio_data structure should be filled with a pointer to audio buffer and its length
// The buffer will be freed in release_audio_cb
static int video_recorder_get_audio_cb(void *user_data, video_recorder_audio_data_t *audio_data)
{
    if (audio_data == NULL || video_recorder_audio_recorder_handle == NULL)
    {
        return -1;
    }

    // Read audio data from audio recorder device
    uint32_t data_len = 0;
    avdk_err_t ret = audio_recorder_device_read(video_recorder_audio_recorder_handle, video_recorder_audio_temp_buffer, sizeof(video_recorder_audio_temp_buffer), &data_len);

    if (ret == AVDK_ERR_OK && data_len > 0)
    {
        // Always pass raw audio frames to recorder:
        // - PCM: raw PCM
        // - AAC: raw AAC access unit (no ADTS); AVI write path will prepend ADTS if needed
        uint8_t *audio_buffer = psram_malloc(data_len);
        if (audio_buffer == NULL)
        {
            LOGE("%s: Failed to allocate audio buffer\n", __func__);
            return -1;
        }
        os_memcpy(audio_buffer, video_recorder_audio_temp_buffer, data_len);
        
        audio_data->data = audio_buffer;
        audio_data->length = data_len;
        return 0;  // Success
    }
    else if (ret == AVDK_ERR_TIMEOUT || ret == AVDK_ERR_EOF)
    {
        // No data available (timeout or end of stream), return -1 to indicate no data
        return -1;
    }
    else
    {
        // Error reading audio data
        LOGE("%s: audio_recorder_device_read failed, ret=%d\n", __func__, ret);
        return -1;
    }
}

// Video record callback: release frame data after writing
static void video_recorder_release_frame_cb(void *user_data, video_recorder_frame_data_t *frame_data)
{
    if (frame_data == NULL)
    {
        return;
    }

    // Get frame buffer pointer from frame_data
    frame_buffer_t *frame = (frame_buffer_t *)frame_data->frame_buffer;
    if (frame != NULL)
    {
        // Free frame buffer based on format
        if (video_recorder_frame_format == IMAGE_MJPEG ||
            video_recorder_frame_format == IMAGE_H264 ||
            video_recorder_frame_format == IMAGE_H265)
        {
            frame_buffer_encode_free(frame);
        }
        else if (video_recorder_frame_format == IMAGE_YUV)
        {
            frame_buffer_display_free(frame);
        }
    }

    // Clear frame_data structure
    frame_data->data = NULL;
    frame_data->length = 0;
    frame_data->frame_buffer = NULL;
}

// Video record callback: release audio data after writing
static void video_recorder_release_audio_cb(void *user_data, video_recorder_audio_data_t *audio_data)
{
    // Free the audio buffer allocated in get_audio_cb
    if (audio_data != NULL && audio_data->data != NULL)
    {
        psram_free(audio_data->data);
        audio_data->data = NULL;
        audio_data->length = 0;
    }
}

static void video_recorder_cancel_auto_stop_timer(void)
{
    // Best-effort stop/deinit without calling rtos_is_* helpers on an uninitialized timer.
    if (video_recorder_auto_stop_timer_started)
    {
        (void)rtos_stop_oneshot_timer(&video_recorder_auto_stop_timer);
        video_recorder_auto_stop_timer_started = false;
    }
    if (video_recorder_auto_stop_timer_inited)
    {
        (void)rtos_deinit_oneshot_timer(&video_recorder_auto_stop_timer);
        video_recorder_auto_stop_timer_inited = false;
    }
}

static avdk_err_t video_recorder_stop_internal(bool from_timeout)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (video_recorder_handle == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    // Cancel timer first to prevent re-entrancy.
    video_recorder_cancel_auto_stop_timer();

    if (from_timeout)
    {
        LOGW("%s: Auto stopping recording after %u ms\n", __func__, (unsigned)VIDEO_RECORDER_MAX_DURATION_MS);
    }

    // Stop recording
    ret = bk_video_recorder_stop(video_recorder_handle);
    if (ret != AVDK_ERR_OK)
    {
        LOGE("%s: bk_video_recorder_stop failed, ret=%d\n", __func__, ret);
    }

    // Print frame drop statistics
    if (video_recorder_frame_total_count > 0)
    {
        float drop_rate = (video_recorder_frame_drop_count * 100.0f) / video_recorder_frame_total_count;
        LOGI("%s: Frame statistics - Total: %u, Dropped: %u (%.1f%%), Recorded: %u\n",
             __func__, video_recorder_frame_total_count, video_recorder_frame_drop_count,
             drop_rate, video_recorder_frame_total_count - video_recorder_frame_drop_count);
    }

    // Reset statistics
    video_recorder_frame_total_count = 0;
    video_recorder_frame_drop_count = 0;

    // Close video record
    avdk_err_t tmp = bk_video_recorder_close(video_recorder_handle);
    if (tmp != AVDK_ERR_OK)
    {
        LOGE("%s: bk_video_recorder_close failed, ret=%d\n", __func__, tmp);
        if (ret == AVDK_ERR_OK)
        {
            ret = tmp;
        }
    }

    // Delete video record
    tmp = bk_video_recorder_delete(video_recorder_handle);
    if (tmp != AVDK_ERR_OK)
    {
        LOGE("%s: bk_video_recorder_delete failed, ret=%d\n", __func__, tmp);
        if (ret == AVDK_ERR_OK)
        {
            ret = tmp;
        }
    }
    video_recorder_handle = NULL;

    // Stop and close audio recorder
    if (video_recorder_audio_recorder_handle != NULL)
    {
        audio_recorder_device_stop(video_recorder_audio_recorder_handle);
        audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
        video_recorder_audio_recorder_handle = NULL;
    }

    // Close DVP camera
    if (video_recorder_dvp_handle != NULL)
    {
        tmp = bk_camera_close(video_recorder_dvp_handle);
        if (tmp != AVDK_ERR_OK)
        {
            LOGE("%s: bk_camera_close failed, ret=%d\n", __func__, tmp);
            if (ret == AVDK_ERR_OK)
            {
                ret = tmp;
            }
        }

        tmp = bk_camera_delete(video_recorder_dvp_handle);
        if (tmp != AVDK_ERR_OK)
        {
            LOGE("%s: bk_camera_delete failed, ret=%d\n", __func__, tmp);
            if (ret == AVDK_ERR_OK)
            {
                ret = tmp;
            }
        }
        video_recorder_dvp_handle = NULL;

        // Power off DVP camera
        if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
        {
            GPIO_DOWN(DVP_POWER_GPIO_ID);
        }
    }

    // Close LCD display
    if (video_recorder_lcd_handle != NULL)
    {
        tmp = bk_display_close(video_recorder_lcd_handle);
        if (tmp != AVDK_ERR_OK)
        {
            LOGE("%s: bk_display_close failed, ret=%d\n", __func__, tmp);
            if (ret == AVDK_ERR_OK)
            {
                ret = tmp;
            }
        }

        lcd_backlight_close(GPIO_7);

        tmp = bk_display_delete(video_recorder_lcd_handle);
        if (tmp != AVDK_ERR_OK)
        {
            LOGE("%s: bk_display_delete failed, ret=%d\n", __func__, tmp);
            if (ret == AVDK_ERR_OK)
            {
                ret = tmp;
            }
        }
        video_recorder_lcd_handle = NULL;

        // Power off LCD LDO (best-effort).
        (void)bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
    }

    // Clean up frame queue - free all remaining frames
    if (video_recorder_frame_queue != NULL)
    {
        frame_buffer_t *frame = NULL;
        while (rtos_pop_from_queue(&video_recorder_frame_queue, &frame, 0) == kNoErr && frame != NULL)
        {
            if (video_recorder_frame_format == IMAGE_MJPEG ||
                video_recorder_frame_format == IMAGE_H264 ||
                video_recorder_frame_format == IMAGE_H265)
            {
                frame_buffer_encode_free(frame);
            }
            else if (video_recorder_frame_format == IMAGE_YUV)
            {
                frame_buffer_display_free(frame);
            }
        }

        rtos_deinit_queue(&video_recorder_frame_queue);
        video_recorder_frame_queue = NULL;
    }

    LOGD("%s: Video recording stopped successfully\n", __func__);
    return ret;
}

static void video_recorder_auto_stop_thread_entry(beken_thread_arg_t arg)
{
    uint32_t sid = (uint32_t)(uintptr_t)arg;

    // Only stop if this is the latest recording session and recording is still active.
    if (sid == video_recorder_session_id && video_recorder_handle != NULL)
    {
        (void)video_recorder_stop_internal(true);
    }

    video_recorder_auto_stop_thread = NULL;
    rtos_delete_thread(NULL);
}

static void video_recorder_auto_stop_timeout(void *Larg, void *Rarg)
{
    (void)Rarg;
    uint32_t sid = (uint32_t)(uintptr_t)Larg;

    // One-shot timer has fired. Mark it as not running.
    video_recorder_auto_stop_timer_started = false;

    // Avoid creating multiple stop threads.
    if (video_recorder_auto_stop_thread != NULL)
    {
        return;
    }

    // Defer heavy stop operations to a dedicated thread (do not block timer context).
    bk_err_t ret = rtos_create_thread(&video_recorder_auto_stop_thread,
                                     BEKEN_APPLICATION_PRIORITY,
                                     "vid_rec_auto_stop",
                                     video_recorder_auto_stop_thread_entry,
                                     2048,
                                     (beken_thread_arg_t)(uintptr_t)sid);
    if (ret != BK_OK)
    {
        LOGE("%s: create auto stop thread failed, ret=%d\n", __func__, ret);
        video_recorder_auto_stop_thread = NULL;
    }
}

static bk_err_t video_recorder_start_auto_stop_timer(uint32_t sid)
{
    // Re-init for each start to ensure a clean state.
    video_recorder_cancel_auto_stop_timer();

    bk_err_t ret = rtos_init_oneshot_timer(&video_recorder_auto_stop_timer,
                                          VIDEO_RECORDER_MAX_DURATION_MS,
                                          video_recorder_auto_stop_timeout,
                                          (void *)(uintptr_t)sid,
                                          NULL);
    if (ret != BK_OK)
    {
        return ret;
    }
    video_recorder_auto_stop_timer_inited = true;

    ret = rtos_start_oneshot_timer(&video_recorder_auto_stop_timer);
    if (ret != BK_OK)
    {
        (void)rtos_deinit_oneshot_timer(&video_recorder_auto_stop_timer);
        video_recorder_auto_stop_timer_inited = false;
        video_recorder_auto_stop_timer_started = false;
        return ret;
    }
    video_recorder_auto_stop_timer_started = true;
    return ret;
}

// Video record command handler - record DVP data and MIC data
void cli_video_record_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    char *msg = CLI_CMD_RSP_ERROR;
    avdk_err_t ret = AVDK_ERR_GENERIC;
    uint16_t output_format = IMAGE_MJPEG;
    uint32_t record_type = VIDEO_RECORDER_TYPE_AVI;
    // Note: keep function logic linear; no need for a separate "start_ok" flag.

    if (argc < 2)
    {
        LOGE("%s: insufficient arguments\n", __func__);
        goto exit;
    }

    if (os_strcmp(argv[1], "start") == 0)
    {
        // Check if already recording
        if (video_recorder_handle != NULL)
        {
            LOGE("%s: video recording already started\n", __func__);
            goto exit;
        }

        // Mount SD card before recording (required for saving video file)
        int mount_ret = sd_card_mount();
        if (mount_ret != BK_OK)
        {
            LOGE("%s: Failed to mount SD card, ret=%d. Please mount SD card first\n", __func__, mount_ret);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        LOGD("%s: SD card mounted successfully\n", __func__);

        // Parse arguments: start [file_path] [width] [height] [format] [type]
        // Default values
        char *file_path = "/sd0/record.avi";
        uint32_t width = 480;
        uint32_t height = 320;

        // Audio format selection (default: PCM)
        uint32_t audio_format = VIDEO_RECORDER_AUDIO_FORMAT_PCM;
        if (cmd_contain(argc, argv, "aac") || cmd_contain(argc, argv, "AAC"))
        {
            audio_format = VIDEO_RECORDER_AUDIO_FORMAT_AAC;
        }
        else if (cmd_contain(argc, argv, "g711u") || cmd_contain(argc, argv, "G711U") ||
                 cmd_contain(argc, argv, "ulaw") || cmd_contain(argc, argv, "ULAW"))
        {
            audio_format = VIDEO_RECORDER_AUDIO_FORMAT_MULAW;
        }
        else if (cmd_contain(argc, argv, "g711a") || cmd_contain(argc, argv, "G711A") ||
                 cmd_contain(argc, argv, "alaw") || cmd_contain(argc, argv, "ALAW") ||
                 cmd_contain(argc, argv, "g711") || cmd_contain(argc, argv, "G711"))
        {
            // Default G.711 to A-law.
            audio_format = VIDEO_RECORDER_AUDIO_FORMAT_ALAW;
        }
        else if (cmd_contain(argc, argv, "g722") || cmd_contain(argc, argv, "G722"))
        {
            audio_format = VIDEO_RECORDER_AUDIO_FORMAT_G722;
        }
        else if (cmd_contain(argc, argv, "mp3") || cmd_contain(argc, argv, "MP3"))
        {
            audio_format = VIDEO_RECORDER_AUDIO_FORMAT_MP3;
        }
        if (argc >= 3)
        {
            file_path = argv[2];
        }

        // Copy to persistent buffer (file_path may point to argv/const, but we may modify extension).
        os_memset(video_recorder_output_path, 0, sizeof(video_recorder_output_path));
        os_strncpy(video_recorder_output_path, file_path, sizeof(video_recorder_output_path) - 1);
        file_path = video_recorder_output_path;

        if (argc >= 5)
        {
            width = os_strtoul(argv[3], NULL, 10);
            height = os_strtoul(argv[4], NULL, 10);
        }
        if (cmd_contain(argc, argv, "mp4"))
        {
            record_type = VIDEO_RECORDER_TYPE_MP4;
            if (os_strstr(file_path, ".avi") != NULL)
            {
                // Replace .avi with .mp4
                char *ext = os_strstr(video_recorder_output_path, ".avi");
                if (ext != NULL)
                {
                    os_strncpy(ext, ".mp4", 4);
                }
            }
        }
        else
        {
            // Default is AVI. If user passes a .mp4 path without 'mp4' keyword, normalize extension.
            if (os_strstr(file_path, ".mp4") != NULL)
            {
                char *ext = os_strstr(video_recorder_output_path, ".mp4");
                if (ext != NULL)
                {
                    os_strncpy(ext, ".avi", 4);
                }
            }
        }

        // For AAC, CLI always provides raw AAC access units.
        // AVI mux will prepend ADTS header inside avi_write_audio_data().
        (void)record_type;
        if (cmd_contain(argc, argv, "h264") || cmd_contain(argc, argv, "H264"))
        {
            output_format = IMAGE_H264;
        }
        else if (cmd_contain(argc, argv, "mjpeg") || cmd_contain(argc, argv, "MJPEG") || cmd_contain(argc, argv, "jpeg"))
        {
            output_format = IMAGE_MJPEG;
        }

        // Initialize frame queue
        if (video_recorder_frame_queue == NULL)
        {
            ret = rtos_init_queue(&video_recorder_frame_queue, "video_recorder_frame_q",
                                  sizeof(frame_buffer_t *), VIDEO_RECORDER_FRAME_QUEUE_SIZE);
            if (ret != BK_OK)
            {
                LOGE("%s: Failed to init frame queue, ret=%d\n", __func__, ret);
                goto exit;
            }
        }

        // Audio recorder device will be initialized in Step 3
        if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_MP3)
        {
            // MP3 encoder is not available in current SDK.
            LOGE("%s: MP3 recording is not supported (no encoder)\n", __func__);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }

        // Step 1: Open LCD display for preview during recording
        if (video_recorder_lcd_handle == NULL)
        {
            // Power on LCD LDO
            ret = bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_HIGH);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_pm_module_vote_ctrl_external_ldo failed, ret:%d\n", __func__, ret);
                // Clean up resources
                if (video_recorder_frame_queue != NULL)
                {
                    rtos_deinit_queue(&video_recorder_frame_queue);
                    video_recorder_frame_queue = NULL;
                }
                goto exit;
            }

            // Configure LCD display
            bk_display_rgb_ctlr_config_t lcd_display_config = {0};
            lcd_display_config.lcd_device = lcd_device;
            lcd_display_config.clk_pin = GPIO_0;
            lcd_display_config.cs_pin = GPIO_12;
            lcd_display_config.sda_pin = GPIO_1;
            lcd_display_config.rst_pin = GPIO_6;

            // Create LCD display controller
            ret = bk_display_rgb_new(&video_recorder_lcd_handle, &lcd_display_config);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_display_rgb_new failed, ret:%d\n", __func__, ret);
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                // Clean up resources
                if (video_recorder_frame_queue != NULL)
                {
                    rtos_deinit_queue(&video_recorder_frame_queue);
                    video_recorder_frame_queue = NULL;
                }
                goto exit;
            }

            // Open backlight
            ret = lcd_backlight_open(GPIO_7);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: lcd_backlight_open failed, ret:%d\n", __func__, ret);
                bk_display_delete(video_recorder_lcd_handle);
                video_recorder_lcd_handle = NULL;
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                // Clean up resources
                if (video_recorder_frame_queue != NULL)
                {
                    rtos_deinit_queue(&video_recorder_frame_queue);
                    video_recorder_frame_queue = NULL;
                }
                goto exit;
            }

            // Open LCD display
            ret = bk_display_open(video_recorder_lcd_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_display_open failed, ret:%d\n", __func__, ret);
                lcd_backlight_close(GPIO_7);
                bk_display_delete(video_recorder_lcd_handle);
                video_recorder_lcd_handle = NULL;
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                // Clean up resources
                if (video_recorder_frame_queue != NULL)
                {
                    rtos_deinit_queue(&video_recorder_frame_queue);
                    video_recorder_frame_queue = NULL;
                }
                goto exit;
            }
            LOGD("%s: LCD display opened for recording preview\n", __func__);
        }

        // Step 2: Open DVP camera with both YUV (for display) and encoded format (for recording)
        if (video_recorder_dvp_handle == NULL)
        {
            // Power on DVP camera
            LOGD("%s: Power on DVP camera\n", __func__);
            if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
            {
                GPIO_UP(DVP_POWER_GPIO_ID);
            }

            // Configure DVP controller
            // Use combined format: IMAGE_YUV | output_format to output both YUV (for LCD display) and encoded format (for recording)
            LOGD("%s: Configure DVP controller: format=%d|IMAGE_YUV, width=%d, height=%d\n", __func__, output_format, width, height);
            bk_dvp_ctlr_config_t dvp_ctrl_config = {
                .config = BK_DVP_864X480_30FPS_MJPEG_CONFIG(),
                .cbs = &video_recorder_dvp_cbs,
            };

            dvp_ctrl_config.config.img_format = IMAGE_YUV | output_format;  // Combined format for both display and recording
            dvp_ctrl_config.config.width = width;
            dvp_ctrl_config.config.height = height;

            // Create DVP controller
            LOGD("%s: Creating DVP controller...\n", __func__);
            ret = bk_camera_dvp_ctlr_new(&video_recorder_dvp_handle, &dvp_ctrl_config);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_camera_dvp_ctlr_new failed, ret=%d\n", __func__, ret);
                if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
                {
                    GPIO_DOWN(DVP_POWER_GPIO_ID);
                }
                // Clean up LCD display
                if (video_recorder_lcd_handle != NULL)
                {
                    bk_display_close(video_recorder_lcd_handle);
                    lcd_backlight_close(GPIO_7);
                    bk_display_delete(video_recorder_lcd_handle);
                    video_recorder_lcd_handle = NULL;
                    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                }
                // Clean up queue and mutexes
                if (video_recorder_frame_queue != NULL)
                {
                    rtos_deinit_queue(&video_recorder_frame_queue);
                    video_recorder_frame_queue = NULL;
                }
                goto exit;
            }

            // Open DVP camera
            LOGD("%s: Opening DVP camera...\n", __func__);
            ret = bk_camera_open(video_recorder_dvp_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: bk_camera_open failed, ret=%d\n", __func__, ret);
                bk_camera_delete(video_recorder_dvp_handle);
                video_recorder_dvp_handle = NULL;
                if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
                {
                    GPIO_DOWN(DVP_POWER_GPIO_ID);
                }
                // Clean up LCD display
                if (video_recorder_lcd_handle != NULL)
                {
                    bk_display_close(video_recorder_lcd_handle);
                    lcd_backlight_close(GPIO_7);
                    bk_display_delete(video_recorder_lcd_handle);
                    video_recorder_lcd_handle = NULL;
                    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                }
                // Clean up queue and mutexes
                if (video_recorder_frame_queue != NULL)
                {
                    rtos_deinit_queue(&video_recorder_frame_queue);
                    video_recorder_frame_queue = NULL;
                }
                goto exit;
            }

            LOGD("%s: DVP camera opened for recording successfully\n", __func__);
        }

        // Step 3: Initialize audio recorder device
        if (video_recorder_audio_recorder_handle == NULL)
        {
            // Configure audio recorder device (mic input is always 16-bit PCM).
            audio_recorder_device_cfg_t audio_recorder_cfg = {0};
            audio_recorder_cfg.audio_channels = 1;  // Mono
            /*
             * NOTE about G722 duration:
             * Current g722 encoder element uses wideband (16kHz) mode internally. If we feed 8kHz PCM
             * while encoder is in wideband mode, the encoded stream duration becomes ~half (A/V desync).
             *
             * To keep A/V duration correct without modifying the encoder module, record G722 at 16kHz.
             */
            audio_recorder_cfg.audio_rate = (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_G722) ? 16000 : 8000;
            audio_recorder_cfg.audio_bits = 16;     // 16-bit samples (encoder input)
            audio_recorder_cfg.audio_format = audio_format;

            // Initialize audio recorder device
            LOGD("%s: Initializing audio recorder device...\n", __func__);
            ret = audio_recorder_device_init(&audio_recorder_cfg, &video_recorder_audio_recorder_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: audio_recorder_device_init failed, ret=%d\n", __func__, ret);
                // Close DVP on error
                bk_camera_close(video_recorder_dvp_handle);
                bk_camera_delete(video_recorder_dvp_handle);
                video_recorder_dvp_handle = NULL;
                if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
                {
                    GPIO_DOWN(DVP_POWER_GPIO_ID);
                }
                // Clean up LCD display
                if (video_recorder_lcd_handle != NULL)
                {
                    bk_display_close(video_recorder_lcd_handle);
                    lcd_backlight_close(GPIO_7);
                    bk_display_delete(video_recorder_lcd_handle);
                    video_recorder_lcd_handle = NULL;
                    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                }
                // Clean up queue
                if (video_recorder_frame_queue != NULL)
                {
                    rtos_deinit_queue(&video_recorder_frame_queue);
                    video_recorder_frame_queue = NULL;
                }
                goto exit;
            }

            // Start audio recorder device
            LOGD("%s: Starting audio recorder device...\n", __func__);
            ret = audio_recorder_device_start(video_recorder_audio_recorder_handle);
            if (ret != AVDK_ERR_OK)
            {
                LOGE("%s: audio_recorder_device_start failed, ret=%d\n", __func__, ret);
                audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
                video_recorder_audio_recorder_handle = NULL;
                // Close DVP on error
                bk_camera_close(video_recorder_dvp_handle);
                bk_camera_delete(video_recorder_dvp_handle);
                video_recorder_dvp_handle = NULL;
                if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
                {
                    GPIO_DOWN(DVP_POWER_GPIO_ID);
                }
                // Clean up LCD display
                if (video_recorder_lcd_handle != NULL)
                {
                    bk_display_close(video_recorder_lcd_handle);
                    lcd_backlight_close(GPIO_7);
                    bk_display_delete(video_recorder_lcd_handle);
                    video_recorder_lcd_handle = NULL;
                    bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
                }
                // Clean up queue
                if (video_recorder_frame_queue != NULL)
                {
                    rtos_deinit_queue(&video_recorder_frame_queue);
                    video_recorder_frame_queue = NULL;
                }
                goto exit;
            }
        }

        // Step 4: Create video record instance
        bk_video_recorder_config_t video_recorder_config = {0};
        video_recorder_config.record_type = record_type;
        video_recorder_config.record_format = (output_format == IMAGE_H264) ? VIDEO_RECORDER_FORMAT_H264 : VIDEO_RECORDER_FORMAT_MJPEG;
        video_recorder_config.record_quality = 0;
        video_recorder_config.record_bitrate = 0;
        video_recorder_config.record_framerate = 30;
        video_recorder_config.video_width = width;
        video_recorder_config.video_height = height;
        video_recorder_config.audio_channels = 1;  // Mono
        // Keep AVI header sample rate consistent with recorder input rate.
        video_recorder_config.audio_rate = (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_G722) ? 16000 : 8000;
        // audio_bits is a container hint (encoded bits per sample):
        // - PCM: 16
        // - AAC/MP3/G722: 0 (compressed)
        // - G711 A/U: 8
        if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_ALAW || audio_format == VIDEO_RECORDER_AUDIO_FORMAT_MULAW)
        {
            video_recorder_config.audio_bits = 8;
        }
        else if (audio_format == VIDEO_RECORDER_AUDIO_FORMAT_PCM)
        {
            video_recorder_config.audio_bits = 16;
        }
        else
        {
            video_recorder_config.audio_bits = 0;
        }
        video_recorder_config.audio_format = audio_format;
        video_recorder_config.get_frame_cb = video_recorder_get_frame_cb;
        video_recorder_config.get_audio_cb = video_recorder_get_audio_cb;
        video_recorder_config.release_frame_cb = video_recorder_release_frame_cb;
        video_recorder_config.release_audio_cb = video_recorder_release_audio_cb;
        video_recorder_config.user_data = NULL;
        LOGD("%s: Video record config prepared: type=%d, format=%d, resolution=%dx%d\n",
             __func__, record_type, video_recorder_config.record_format, width, height);

        LOGD("%s: Creating video record instance...\n", __func__);
        ret = bk_video_recorder_new(&video_recorder_handle, &video_recorder_config);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_recorder_new failed, ret=%d\n", __func__, ret);
            // Clean up DVP and MIC
            audio_recorder_device_stop(video_recorder_audio_recorder_handle);
            audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
            video_recorder_audio_recorder_handle = NULL;
            bk_camera_close(video_recorder_dvp_handle);
            bk_camera_delete(video_recorder_dvp_handle);
            video_recorder_dvp_handle = NULL;
            if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
            {
                GPIO_DOWN(DVP_POWER_GPIO_ID);
            }
            // Clean up LCD display
            if (video_recorder_lcd_handle != NULL)
            {
                bk_display_close(video_recorder_lcd_handle);
                lcd_backlight_close(GPIO_7);
                bk_display_delete(video_recorder_lcd_handle);
                video_recorder_lcd_handle = NULL;
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
            }
            goto exit;
        }

        // Step 5: Open video record
        LOGD("%s: Opening video record...\n", __func__);
        ret = bk_video_recorder_open(video_recorder_handle);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_recorder_open failed, ret=%d\n", __func__, ret);
            bk_video_recorder_delete(video_recorder_handle);
            video_recorder_handle = NULL;
            // Clean up DVP and MIC
            audio_recorder_device_stop(video_recorder_audio_recorder_handle);
            audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
            video_recorder_audio_recorder_handle = NULL;
            bk_camera_close(video_recorder_dvp_handle);
            bk_camera_delete(video_recorder_dvp_handle);
            video_recorder_dvp_handle = NULL;
            if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
            {
                GPIO_DOWN(DVP_POWER_GPIO_ID);
            }
            // Clean up LCD display
            if (video_recorder_lcd_handle != NULL)
            {
                bk_display_close(video_recorder_lcd_handle);
                lcd_backlight_close(GPIO_7);
                bk_display_delete(video_recorder_lcd_handle);
                video_recorder_lcd_handle = NULL;
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
            }
            goto exit;
        }

        // Step 6: Start recording
        LOGD("%s: Starting video recording to file: %s\n", __func__, file_path);
        ret = bk_video_recorder_start(video_recorder_handle, file_path, record_type);
        if (ret != AVDK_ERR_OK)
        {
            LOGE("%s: bk_video_recorder_start failed, ret=%d\n", __func__, ret);
            bk_video_recorder_close(video_recorder_handle);
            bk_video_recorder_delete(video_recorder_handle);
            video_recorder_handle = NULL;
            // Clean up DVP and MIC
            audio_recorder_device_stop(video_recorder_audio_recorder_handle);
            audio_recorder_device_deinit(video_recorder_audio_recorder_handle);
            video_recorder_audio_recorder_handle = NULL;
            bk_camera_close(video_recorder_dvp_handle);
            bk_camera_delete(video_recorder_dvp_handle);
            video_recorder_dvp_handle = NULL;
            if (DVP_POWER_GPIO_ID != GPIO_INVALID_ID)
            {
                GPIO_DOWN(DVP_POWER_GPIO_ID);
            }
            // Clean up LCD display
            if (video_recorder_lcd_handle != NULL)
            {
                bk_display_close(video_recorder_lcd_handle);
                lcd_backlight_close(GPIO_7);
                bk_display_delete(video_recorder_lcd_handle);
                video_recorder_lcd_handle = NULL;
                bk_pm_module_vote_ctrl_external_ldo(GPIO_CTRL_LDO_MODULE_LCD, LCD_LDO_PIN, GPIO_OUTPUT_STATE_LOW);
            }
            // Clean up queue and mutexes
            if (video_recorder_frame_queue != NULL)
            {
                rtos_deinit_queue(&video_recorder_frame_queue);
                video_recorder_frame_queue = NULL;
            }
            goto exit;
        }

        // Start max-duration auto-stop timer (5 minutes).
        // Increment session id so a previous timer callback cannot stop a new session.
        video_recorder_session_id++;
        bk_err_t t_ret = video_recorder_start_auto_stop_timer(video_recorder_session_id);
        if (t_ret != BK_OK)
        {
            LOGE("%s: Failed to start auto stop timer, ret=%d\n", __func__, t_ret);
            // Enforce time limit requirement: stop recording immediately on timer setup failure.
            (void)video_recorder_stop_internal(false);
            goto exit;
        }

        LOGD("%s: Video recording started successfully, file: %s\n", __func__, file_path);
        msg = CLI_CMD_RSP_SUCCEED;
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        // Check if recording is active
        if (video_recorder_handle == NULL)
        {
            LOGE("%s: video recording not started\n", __func__);
            goto exit;
        }

        // Invalidate any pending timer callback for previous sessions.
        video_recorder_session_id++;

        ret = video_recorder_stop_internal(false);
        if (ret != AVDK_ERR_OK)
        {
            goto exit;
        }

        msg = CLI_CMD_RSP_SUCCEED;
    }
    else
    {
        LOGE("%s: unknown command: %s\n", __func__, argv[1]);
        goto exit;
    }

exit:
    if (msg == NULL)
    {
        if (ret == AVDK_ERR_OK)
        {
            msg = CLI_CMD_RSP_SUCCEED;
        }
        else
        {
            msg = CLI_CMD_RSP_ERROR;
        }
    }

    os_memcpy(pcWriteBuffer, msg, os_strlen(msg));
}
