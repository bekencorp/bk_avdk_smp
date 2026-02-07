#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>

#include <components/bk_display.h>

#include "frame_buffer.h"
#include "video_player_common.h"
#include "video_play_callbacks.h"
#include "audio_player_device.h"

#define TAG "video_play_callbacks"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

avdk_err_t video_play_audio_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL || buffer->length == 0)
    {
        return AVDK_ERR_INVAL;
    }

    buffer->data = (uint8_t *)os_malloc(buffer->length);
    if (buffer->data == NULL)
    {
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    buffer->frame_buffer = NULL;
    buffer->user_data = NULL;
    return AVDK_ERR_OK;
}

void video_play_audio_buffer_free_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL)
    {
        return;
    }

    if (buffer->data != NULL)
    {
        os_free(buffer->data);
        buffer->data = NULL;
    }

    buffer->length = 0;
    buffer->pts = 0;
    buffer->frame_buffer = NULL;
    buffer->user_data = NULL;
}

avdk_err_t video_play_video_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL || buffer->length == 0)
    {
        return AVDK_ERR_INVAL;
    }

    // Packet buffers are used as decoder input. The decoder requires a valid frame_buffer_t pointer.
    frame_buffer_t *fb = frame_buffer_encode_malloc(buffer->length);
    if (fb == NULL)
    {
        buffer->data = NULL;
        buffer->frame_buffer = NULL;
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    fb->fmt = PIXEL_FMT_JPEG;
    fb->length = 0;

    buffer->data = fb->frame;
    buffer->frame_buffer = fb;
    buffer->user_data = NULL;
    return AVDK_ERR_OK;
}

void video_play_video_buffer_free_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL)
    {
        return;
    }

    if (buffer->frame_buffer != NULL)
    {
        frame_buffer_encode_free((frame_buffer_t *)buffer->frame_buffer);
        buffer->frame_buffer = NULL;
    }

    buffer->data = NULL;
    buffer->length = 0;
    buffer->pts = 0;
    buffer->user_data = NULL;
}

avdk_err_t video_play_video_buffer_alloc_yuv_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL || buffer->length == 0)
    {
        return AVDK_ERR_INVAL;
    }

    // Decoded output buffer is displayed on LCD. Use display frame buffer allocator.
    frame_buffer_t *fb = frame_buffer_display_malloc(buffer->length);
    if (fb == NULL)
    {
        buffer->data = NULL;
        buffer->frame_buffer = NULL;
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    fb->length = 0;
    buffer->data = fb->frame;
    buffer->frame_buffer = fb;
    buffer->user_data = NULL;
    return AVDK_ERR_OK;
}

void video_play_video_buffer_free_yuv_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL)
    {
        return;
    }

    if (buffer->frame_buffer != NULL)
    {
        frame_buffer_display_free((frame_buffer_t *)buffer->frame_buffer);
        buffer->frame_buffer = NULL;
    }

    buffer->data = NULL;
    buffer->length = 0;
    buffer->pts = 0;
    buffer->user_data = NULL;
}

void video_play_video_decode_complete_cb(void *user_data, const video_player_video_frame_meta_t *meta, video_player_buffer_t *buffer)
{
    if (buffer == NULL || buffer->frame_buffer == NULL)
    {
        return;
    }

    // Print meta information with light throttling to avoid log flooding.
    if (meta != NULL)
    {
        // const uint64_t idx = meta->frame_index;
        if (0)//idx <= 1 || (idx % 30ULL) == 0ULL)
        {
            LOGI("%s: video frame idx=%llu pts=%llu ms, w=%u h=%u fps=%u fmt=%u jpeg_ss=%u\n",
                 __func__,
                 (unsigned long long)meta->frame_index,
                 (unsigned long long)meta->pts_ms,
                 (unsigned)meta->video.width,
                 (unsigned)meta->video.height,
                 (unsigned)meta->video.fps,
                 (unsigned)meta->video.format,
                 (unsigned)meta->video.jpeg_subsampling);
        }
    }

    video_play_user_ctx_t *ctx = (video_play_user_ctx_t *)user_data;
    frame_buffer_t *frame = (frame_buffer_t *)buffer->frame_buffer;

    // If LCD is not ready, free the frame here to avoid memory leak.
    if (ctx == NULL || ctx->lcd_handle == NULL)
    {
        frame_buffer_display_free(frame);
        buffer->frame_buffer = NULL;
        buffer->data = NULL;
        buffer->length = 0;
        return;
    }

    frame->length = buffer->length;
    frame->timestamp = (uint32_t)buffer->pts;

    // Flush frame to LCD. On success, LCD driver will call display_frame_free_cb to free frame.
    avdk_err_t ret = bk_display_flush(ctx->lcd_handle, frame, display_frame_free_cb);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s: bk_display_flush failed, ret=%d\n", __func__, ret);
        frame_buffer_display_free(frame);
    }

    buffer->frame_buffer = NULL;
    buffer->data = NULL;
    buffer->length = 0;
}

void video_play_audio_decode_complete_cb(void *user_data, const video_player_audio_packet_meta_t *meta, video_player_buffer_t *buffer)
{
    if (buffer == NULL || buffer->data == NULL || buffer->length == 0)
    {
        return;
    }

    // Print meta information with light throttling to avoid log flooding.
    if (meta != NULL)
    {
        // const uint64_t idx = meta->packet_index;
        if (0)//idx <= 1 || (idx % 50ULL) == 0ULL)
        {
            LOGI("%s: audio pkt idx=%llu pts=%llu ms, ch=%u rate=%u bits=%u fmt=%u, len=%u\n",
                 __func__,
                 (unsigned long long)meta->packet_index,
                 (unsigned long long)meta->pts_ms,
                 (unsigned)meta->audio.channels,
                 (unsigned)meta->audio.sample_rate,
                 (unsigned)meta->audio.bits_per_sample,
                 (unsigned)meta->audio.format,
                 (unsigned)buffer->length);
        }
    }

    video_play_user_ctx_t *ctx = (video_play_user_ctx_t *)user_data;
    if (ctx != NULL && ctx->audio_player_handle != NULL)
    {
        /*
         * audio_player_device_write_frame_data() returns the number of bytes written (>= 0).
         * - >= 0: success (may be 0 if internal buffer is insufficient and data is dropped)
         * - < 0 : failure
         * Do NOT treat a positive return value (e.g., 2048) as an error code.
         */
        int32_t written = audio_player_device_write_frame_data(ctx->audio_player_handle, (char *)buffer->data, buffer->length);
        if (written < 0)
        {
            LOGW("%s: audio_player_device_write_frame_data failed, ret=%d\n", __func__, written);
        }
    }

    // Always free audio buffer after writing.
    os_free(buffer->data);
    buffer->data = NULL;
    buffer->length = 0;
    buffer->pts = 0;
    buffer->frame_buffer = NULL;
    buffer->user_data = NULL;
}


