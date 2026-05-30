#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include <components/bk_display.h>

#include "video_player_common.h"
#include "video_play_callbacks.h"
#include "audio_player_device.h"
#include <components/bk_frame_buffer.h>
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
#include <components/bk_video_player/video_decoder/bk_video_player_hw_h264_decoder.h>
#endif
#include <common/avdk_pixel_types.h>

#define TAG "video_play_callbacks"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define VIDEO_PACKET_BUFFER_SAFETY_PAD_BYTES   (128U)
#define VIDEO_FRAME_BUFFER_SAFETY_PAD_BYTES    (128U)

/* mem_slab debug guards require user_size 64 B-aligned (see bk_mem_slab.c). */
#define VIDEO_PLAY_MEM_SLAB_ALIGN_BYTES        (64U)

static video_play_lcd_video_fmt_t s_runtime_lcd_fmt = VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW;
static bool s_runtime_lcd_fmt_valid = false;

static void video_play_lcd_sync_format_for_output_frame(bk_display_ctlr_handle_t handle,
                                                        const frame_buffer_t *fb_meta)
{
    if (handle == NULL)
    {
        return;
    }

    video_play_lcd_video_fmt_t need = VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW;
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    /* H264 flexa-GPU output is compressed ARGB8888 in GPU/PSRAM, not HSRAM.
     * is_hsram_output_frame() therefore must not drive DPU format selection. */
    if (fb_meta != NULL && fb_meta->fmt == PIXEL_FMT_ARGB8888)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_COMPRESSED;
    }
#else
    (void)fb_meta;
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
}

void video_play_lcd_runtime_format_mark(video_play_lcd_video_fmt_t fmt)
{
    s_runtime_lcd_fmt = fmt;
    s_runtime_lcd_fmt_valid = true;
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

 static void video_play_free_video_pixel(void *pixel)
 {
     if (pixel == NULL)
     {
         return;
     }
 
 #if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
     (void)bk_video_player_hw_h264_decoder_free_output_frame(pixel);
 #else
     bk_frame_buffer_free(pixel);
 #endif
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
 
 #ifndef VIDEO_PLAY_DUMP_FRAME_ENABLE
 #define VIDEO_PLAY_DUMP_FRAME_ENABLE     0
 #endif
 
 #ifndef VIDEO_PLAY_DUMP_FRAME_INDEX
 #define VIDEO_PLAY_DUMP_FRAME_INDEX      0U
 #endif
 
 #ifndef VIDEO_PLAY_DUMP_FRAME_MAX_BYTES
 /* 0 == dump the whole frame. */
 #define VIDEO_PLAY_DUMP_FRAME_MAX_BYTES  0U
 #endif
 
 #if VIDEO_PLAY_DUMP_FRAME_ENABLE
 extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
 
 static void video_play_dump_one_frame(const char *tag,
                                       const video_player_video_frame_meta_t *meta,
                                       const uint8_t *buf,
                                       uint32_t size)
 {
     if (buf == NULL)
     {
         return;
     }
 
     uint32_t dump_size = size;
     if (VIDEO_PLAY_DUMP_FRAME_MAX_BYTES != 0U && dump_size > VIDEO_PLAY_DUMP_FRAME_MAX_BYTES)
     {
         dump_size = VIDEO_PLAY_DUMP_FRAME_MAX_BYTES;
     }
     /* stack_mem_dump prints a multiple of 4 bytes; round UP so the trailing
      * bytes are not silently dropped (any pad is just outside the visible
      * frame and will appear as junk in the parsed file). */
     uint32_t dump_aligned = (dump_size + 3U) & ~3U;
 
     /* Disable interrupts so the buffer cannot be recycled mid-dump and so
      * no other task interleaves bytes into the stack_mem_dump output. The
      * dump is single-shot (gated by VIDEO_PLAY_DUMP_FRAME_INDEX) so even at
      * a few hundred KB the IRQ-off window is bounded and acceptable for
      * offline diagnostics. */
     uint32_t irq_flags = rtos_enter_critical();
     bk_printf("video_play_frame_dump tag=%s frame_index=%llu pts_ms=%llu "
               "w=%u h=%u src_fmt=%u jpeg_ss=%u "
               "frame_size=%u dump_size=%u buf=%p aligned=%u\n",
               tag,
               (unsigned long long)(meta != NULL ? meta->frame_index : 0ULL),
               (unsigned long long)(meta != NULL ? meta->pts_ms      : 0ULL),
               (unsigned)(meta != NULL ? meta->video.width            : 0U),
               (unsigned)(meta != NULL ? meta->video.height           : 0U),
               (unsigned)(meta != NULL ? meta->video.format           : 0U),
               (unsigned)(meta != NULL ? meta->video.jpeg_subsampling : 0U),
               (unsigned)size,
               (unsigned)dump_size,
               buf,
               (unsigned)dump_aligned);
     stack_mem_dump((uint32_t)(uintptr_t)buf,
                    (uint32_t)(uintptr_t)(buf + dump_aligned));
     rtos_exit_critical(irq_flags);
 }
 #endif /* VIDEO_PLAY_DUMP_FRAME_ENABLE */
 
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

    /* See VIDEO_PACKET_BUFFER_SAFETY_PAD_BYTES rationale above. The decoded
     * NV12/YUV output is consumed by the LCD path which may also DMA in
     * 64-bit/128-bit bursts past the nominal frame size. Pad to be safe. */
    const uint32_t requested = buffer->length;
    const uint32_t alloc_size = video_play_slab_alloc_size(
        requested + VIDEO_FRAME_BUFFER_SAFETY_PAD_BYTES);

    frame_buffer_t *fb_meta = (frame_buffer_t *)os_malloc(sizeof(frame_buffer_t));
    if (fb_meta == NULL)
    {
        buffer->data = NULL;
        buffer->frame_buffer = NULL;
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    void *pixel = bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, alloc_size);
    if (pixel == NULL)
    {
        os_free(fb_meta);
        buffer->data = NULL;
        buffer->frame_buffer = NULL;
        buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    os_memset(fb_meta, 0, sizeof(*fb_meta));
    fb_meta->frame  = (uint8_t *)pixel;
    fb_meta->size   = requested;
    fb_meta->length = requested;

    buffer->data         = pixel;
    buffer->frame_buffer = fb_meta;
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

    /* Free the pixel buffer (UNCODED PSRAM) and the small frame_buffer_t
     * metadata struct independently. Either may be NULL if the buffer was
     * already handed to the LCD layer (decode_complete_cb null'd them). */
    if (buffer->data != NULL)
    {
        video_play_free_video_pixel(buffer->data);
    }

    if (buffer->frame_buffer != NULL)
    {
        os_free(buffer->frame_buffer);
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
    void *pixel       = buffer->data;          /* decoded/compressed pixel buffer */
    void *fb_meta     = buffer->frame_buffer;  /* frame_buffer_t struct (regular heap) */
    uint32_t pixel_len = buffer->length;       /* nominal raw frame size from alloc_yuv_cb */

#if VIDEO_PLAY_DUMP_FRAME_ENABLE
    
    static uint64_t s_dump_call_counter = 0U;
    static uint8_t  s_dump_done         = 0U;
    if (s_dump_done == 0U &&
        s_dump_call_counter == (uint64_t)VIDEO_PLAY_DUMP_FRAME_INDEX)
    {
        s_dump_done = 1U;
        video_play_dump_one_frame("preflush_yuv", meta,
                                  (const uint8_t *)pixel, pixel_len);
    }
    s_dump_call_counter++;
#else
    (void)pixel_len;
#endif

    /* No matter which branch we take below, we transfer ownership of `pixel`
     * out of the engine and we free `fb_meta` ourselves. Null the buffer
     * fields up-front so the engine's buffer_free_yuv_cb is a no-op for this
     * buffer and we never double-free. */
    buffer->data         = NULL;
    buffer->frame_buffer = NULL;
    buffer->length       = 0;

    /* If LCD is not ready, free the pixel buffer here to avoid leaks. */
    if (ctx == NULL || ctx->lcd_handle == NULL)
    {
        video_play_free_video_pixel(pixel);
        if (fb_meta != NULL)
        {
            os_free(fb_meta);
        }
        return;
    }

    video_play_lcd_sync_format_for_output_frame(ctx->lcd_handle, (const frame_buffer_t *)fb_meta);

    /*
     * NOTE: bk_display_flush() ultimately forwards the second argument to
     * dpu_frame_commit(uint8_t *buff) which treats it as the raw pixel
     * source address for the DPU. We therefore MUST pass `buffer->data`
     * (the pixel buffer) and NOT the `frame_buffer_t *` metadata struct.
     * The metadata struct exists only because the H.264/JPEG decoders write
     * descriptive fields into it; the LCD path does not consume it.
     */
    avdk_err_t (*free_cb)(void *) = display_frame_free_cb;
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    if (bk_video_player_hw_h264_decoder_is_hsram_output_frame(pixel))
    {
        free_cb = display_h264_output_frame_free_cb;
    }
#endif
    avdk_err_t ret = bk_display_flush(ctx->lcd_handle, pixel, free_cb);
    if (ret != AVDK_ERR_OK)
    {
        LOGW("%s: bk_display_flush failed, ret=%d\n", __func__, ret);
        video_play_free_video_pixel(pixel);
    }

    if (fb_meta != NULL)
    {
        os_free(fb_meta);
    }
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
