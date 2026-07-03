#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include <components/bk_display.h>

#include "video_player_common.h"
#include "video_play_callbacks.h"
#include "video_play_gpu_postprocess.h"
#include "audio_player_device.h"
#include <components/bk_frame_buffer.h>
#include <components/bk_video_player/bk_video_player_types.h>
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
#include <components/bk_video_player/video_decoder/bk_video_player_hw_h264_decoder.h>
#endif

#define TAG "video_play_callbacks"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

#define VIDEO_PACKET_BUFFER_SAFETY_PAD_BYTES   (2048U)
#define VIDEO_FRAME_BUFFER_SAFETY_PAD_BYTES    (128U)

/* mem_slab debug guards require user_size 64 B-aligned (see bk_mem_slab.c). */
#define VIDEO_PLAY_MEM_SLAB_ALIGN_BYTES        (64U)

static video_play_lcd_video_fmt_t s_runtime_lcd_fmt = VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW;
static bool s_runtime_lcd_fmt_valid = false;
static video_play_rotate_mode_t s_video_rotate_mode = VIDEO_PLAY_ROTATE_NONE;

static void video_play_lcd_sync_format_for_output_frame(bk_display_ctlr_handle_t handle,
                                                        uint32_t display_pixel_fmt)
{
    if (handle == NULL)
    {
        return;
    }

    video_play_lcd_video_fmt_t need = VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW;
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    if (display_pixel_fmt == PIXEL_FMT_ARGB8888)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_COMPRESSED;
    }
    else if (display_pixel_fmt == PIXEL_FMT_RGB565)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_RGB565_RAW;
    }
#else
    if (display_pixel_fmt == PIXEL_FMT_RGB565)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_RGB565_RAW;
    }
    else
    {
        (void)display_pixel_fmt;
    }
#endif

    if (s_runtime_lcd_fmt_valid && need == s_runtime_lcd_fmt)
    {
        return;
    }

    if (video_play_lcd_apply_format(handle, need) == AVDK_ERR_OK)
    {
        s_runtime_lcd_fmt = need;
        s_runtime_lcd_fmt_valid = true;
    }
}

void video_play_lcd_runtime_format_reset(void)
{
    s_runtime_lcd_fmt_valid = false;
    video_play_gpu_postprocess_deinit();
}

void video_play_lcd_runtime_format_mark(video_play_lcd_video_fmt_t fmt)
{
    s_runtime_lcd_fmt = fmt;
    s_runtime_lcd_fmt_valid = true;
}

void video_play_video_set_rotate_mode(video_play_rotate_mode_t mode)
{
    s_video_rotate_mode = mode;
    video_play_lcd_runtime_format_reset();
}

video_play_rotate_mode_t video_play_video_get_rotate_mode(void)
{
    return s_video_rotate_mode;
}

static uint32_t video_play_rotate_mode_to_degree(video_play_rotate_mode_t mode)
{
    if (mode == VIDEO_PLAY_ROTATE_90)
    {
        return 90U;
    }
    if (mode == VIDEO_PLAY_ROTATE_270)
    {
        return 270U;
    }
    return 0U;
}

uint32_t video_play_video_get_rotate_degree(void)
{
    return video_play_rotate_mode_to_degree(s_video_rotate_mode);
}

static video_play_rotate_mode_t video_play_video_effective_rotate_mode(const video_player_video_frame_meta_t *meta)
{
    (void)meta;
    return s_video_rotate_mode;
}

static inline uint32_t video_play_slab_alloc_size(uint32_t payload_plus_pad)
{
    return (payload_plus_pad + VIDEO_PLAY_MEM_SLAB_ALIGN_BYTES - 1U) &
           ~(VIDEO_PLAY_MEM_SLAB_ALIGN_BYTES - 1U);
}

avdk_err_t video_play_audio_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL || buffer->length == 0)
    {
        return AVDK_ERR_INVAL;
    }

    const uint32_t requested = buffer->length;
    buffer->data = (uint8_t *)os_malloc(requested + VIDEO_PACKET_BUFFER_SAFETY_PAD_BYTES);
    if (buffer->data == NULL)
    {
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    buffer->frame_buffer = NULL;
    buffer->length = requested;
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

static avdk_err_t display_frame_free_cb(void *frame)
{
    bk_frame_buffer_free(frame);
    return AVDK_ERR_OK;
}

static void video_play_free_output_pixel(uint32_t decoder_pixel_fmt, void *pixel)
{
    if (pixel == NULL)
    {
        return;
    }

#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    if (decoder_pixel_fmt == PIXEL_FMT_ARGB8888)
    {
        (void)bk_video_player_hw_h264_decoder_free_output_frame(pixel);
        return;
    }
#else
    (void)decoder_pixel_fmt;
#endif

    bk_frame_buffer_free(pixel);
}

#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
 /* LCD-flush completion callback for HW H.264 GPU output frames. The frame
  * may live in HSRAM (zero-copy fast path) or in PSRAM (fallback), so we
  * must use the allocator-aware free instead of the plain
  * bk_frame_buffer_free used by display_frame_free_cb above. */
 static avdk_err_t display_h264_output_frame_free_cb(void *frame)
 {
     return bk_video_player_hw_h264_decoder_free_output_frame(frame);
 }
 #endif
 
avdk_err_t video_play_video_buffer_alloc_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL || buffer->length == 0)
    {
        return AVDK_ERR_INVAL;
    }

    const uint32_t requested = buffer->length;
    const uint32_t alloc_size = video_play_slab_alloc_size(
        requested + VIDEO_PACKET_BUFFER_SAFETY_PAD_BYTES);

    void *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, alloc_size);
    if (frame == NULL)
    {
        buffer->data = NULL;
        buffer->frame_buffer = NULL;
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    buffer->data = frame;
    buffer->frame_buffer = frame;
    buffer->length = requested;
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
        bk_frame_buffer_free(buffer->frame_buffer);
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

    const uint32_t requested = buffer->length;
    const uint32_t alloc_size = video_play_slab_alloc_size(
        requested + VIDEO_FRAME_BUFFER_SAFETY_PAD_BYTES);

    void *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, alloc_size);
    if (frame == NULL)
    {
        buffer->data = NULL;
        buffer->frame_buffer = NULL;
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    buffer->data         = frame;
    buffer->frame_buffer = NULL;
    buffer->length       = requested;
    buffer->user_data    = NULL;
    return AVDK_ERR_OK;
}


void video_play_video_buffer_free_yuv_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL)
    {
        return;
    }

    if (buffer->data != NULL)
    {
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
        (void)bk_video_player_hw_h264_decoder_free_output_frame(buffer->data);
#else
        bk_frame_buffer_free(buffer->data);
#endif
    }

    buffer->data         = NULL;
    buffer->frame_buffer = NULL;
    buffer->length       = 0;
    buffer->pts          = 0;
    buffer->user_data    = NULL;
}

avdk_err_t video_play_video_buffer_alloc_yuv_coded_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL || buffer->length == 0)
    {
        return AVDK_ERR_INVAL;
    }

    const uint32_t requested = buffer->length;
    const uint32_t alloc_size = video_play_slab_alloc_size(
        requested + VIDEO_FRAME_BUFFER_SAFETY_PAD_BYTES);

    /* PSRAM1 (CODED slab): keeps the frame-zerocopy decode pool's PSRAM0 budget
     * free. Both PSRAM windows are non-cacheable, so the DPU reads coherent. */
    void *frame = bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, alloc_size);
    if (frame == NULL)
    {
        buffer->data = NULL;
        buffer->frame_buffer = NULL;
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    buffer->data         = frame;
    buffer->frame_buffer = NULL;
    buffer->length       = requested;
    buffer->user_data    = NULL;
    return AVDK_ERR_OK;
}

void video_play_video_buffer_free_yuv_coded_cb(void *user_data, video_player_buffer_t *buffer)
{
    (void)user_data;

    if (buffer == NULL)
    {
        return;
    }

    if (buffer->data != NULL)
    {
        bk_frame_buffer_free(buffer->data);
    }

    buffer->data         = NULL;
    buffer->frame_buffer = NULL;
    buffer->length       = 0;
    buffer->pts          = 0;
    buffer->user_data    = NULL;
}

void video_play_video_decode_complete_cb(void *user_data, const video_player_video_frame_meta_t *meta, video_player_buffer_t *buffer)
{
    if (buffer == NULL || buffer->data == NULL)
    {
        return;
    }

    video_play_user_ctx_t *ctx = (video_play_user_ctx_t *)user_data;
    void *pixel = buffer->data;
    const uint32_t decoder_pixel_fmt = (meta != NULL) ? (uint32_t)meta->output_format : 0U;
    const video_play_rotate_mode_t rotate_mode = video_play_video_effective_rotate_mode(meta);
    const bool h264_output_frame = (meta != NULL &&
                                    meta->video.format == VIDEO_PLAYER_VIDEO_FORMAT_H264 &&
                                    (decoder_pixel_fmt == PIXEL_FMT_ARGB8888 ||
                                     decoder_pixel_fmt == PIXEL_FMT_RGB565));
    uint32_t display_pixel_fmt = decoder_pixel_fmt;
    bool gpu_post_frame = false;

    /* Transfer ownership out of the engine; null fields so buffer_free_yuv_cb
     * is a no-op and we never double-free. */
    buffer->data         = NULL;
    buffer->frame_buffer = NULL;
    buffer->length       = 0;

    if (ctx == NULL || ctx->lcd_handle == NULL)
    {
        video_play_free_output_pixel(decoder_pixel_fmt, pixel);
        return;
    }

    if (rotate_mode != VIDEO_PLAY_ROTATE_NONE &&
        decoder_pixel_fmt == PIXEL_FMT_NV12 &&
        meta != NULL &&
        (meta->video.format == VIDEO_PLAYER_VIDEO_FORMAT_MJPEG ||
         meta->video.format == VIDEO_PLAYER_VIDEO_FORMAT_H264))
    {
        const uint32_t src_w = meta->video.width;
        const uint32_t src_h = meta->video.height;
        const uint32_t src_stride = (meta->video.format == VIDEO_PLAYER_VIDEO_FORMAT_MJPEG)
                                    ? ((src_w + 15U) & ~15U)
                                    : src_w;
        const uint32_t src_y_height = (meta->video.format == VIDEO_PLAYER_VIDEO_FORMAT_MJPEG)
                                      ? ((src_h + 15U) & ~15U)
                                      : src_h;
        video_play_gpu_postprocess_frame_t gpu_frame;
        os_memset(&gpu_frame, 0, sizeof(gpu_frame));
        avdk_err_t rotate_ret = video_play_gpu_postprocess_nv12_rotate((const uint8_t *)pixel,
                                                                       src_w,
                                                                       src_h,
                                                                       src_stride,
                                                                       src_y_height,
                                                                       rotate_mode,
                                                                       &gpu_frame);
        if (rotate_ret == AVDK_ERR_OK && gpu_frame.data != NULL)
        {
            video_play_free_output_pixel(decoder_pixel_fmt, pixel);
            pixel = gpu_frame.data;
            display_pixel_fmt = PIXEL_FMT_ARGB8888;
            gpu_post_frame = true;
        }
        else
        {
            LOGW("%s: GPU rotate failed, ret=%d; drop rotated frame\n", __func__, rotate_ret);
            video_play_free_output_pixel(decoder_pixel_fmt, pixel);
            return;
        }
    }

    video_play_lcd_sync_format_for_output_frame(ctx->lcd_handle, display_pixel_fmt);

    avdk_err_t (*free_cb)(void *) = display_frame_free_cb;
    if (gpu_post_frame)
    {
        free_cb = video_play_gpu_postprocess_free_frame;
    }
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    else if (h264_output_frame)
    {
        free_cb = display_h264_output_frame_free_cb;
    }
#endif

    avdk_err_t ret = bk_display_flush(ctx->lcd_handle, pixel, free_cb);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s: bk_display_flush failed, ret=%d\n", __func__, ret);
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
        if (h264_output_frame)
        {
            (void)bk_video_player_hw_h264_decoder_free_output_frame(pixel);
        }
        else
#endif
        {
            (void)free_cb(pixel);
        }
    }
}

void video_play_audio_decode_complete_cb(void *user_data, const video_player_audio_packet_meta_t *meta, video_player_buffer_t *buffer)
{
    if (buffer == NULL || buffer->data == NULL || buffer->length == 0)
    {
        return;
    }

    (void)meta;

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
