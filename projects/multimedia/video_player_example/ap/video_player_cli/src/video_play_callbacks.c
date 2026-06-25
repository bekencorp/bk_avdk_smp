#include <common/bk_include.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include <components/bk_display.h>

#include "video_player_common.h"
#include "video_play_callbacks.h"
#include "audio_player_device.h"
#include <components/bk_frame_buffer.h>
#include <components/bk_video_player/bk_video_player_types.h>
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
#include <components/bk_video_player/video_decoder/bk_video_player_hw_h264_decoder.h>
#endif

#define TAG "video_play_callbacks"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define VIDEO_PACKET_BUFFER_SAFETY_PAD_BYTES   (2048U)
#define VIDEO_FRAME_BUFFER_SAFETY_PAD_BYTES    (128U)

/* mem_slab debug guards require user_size 64 B-aligned (see bk_mem_slab.c). */
#define VIDEO_PLAY_MEM_SLAB_ALIGN_BYTES        (64U)

static video_play_lcd_video_fmt_t s_runtime_lcd_fmt = VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW;
static bool s_runtime_lcd_fmt_valid = false;
static video_play_rotate_mode_t s_video_rotate_mode = VIDEO_PLAY_ROTATE_NONE;

static void video_play_lcd_sync_format_for_output_frame(bk_display_ctlr_handle_t handle,
                                                        uint32_t decoder_pixel_fmt,
                                                        bool rotated_rgb565)
{
    if (handle == NULL)
    {
        return;
    }

    video_play_lcd_video_fmt_t need = VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW;
    if (rotated_rgb565)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_RGB565_RAW;
    }
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    else if (decoder_pixel_fmt == PIXEL_FMT_ARGB8888)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_COMPRESSED;
    }
    else if (decoder_pixel_fmt == PIXEL_FMT_RGB565)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_RGB565_RAW;
    }
#else
    (void)decoder_pixel_fmt;
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

static inline uint8_t video_play_clip_u8(int value)
{
    if (value < 0)
    {
        return 0U;
    }
    if (value > 255)
    {
        return 255U;
    }

    return (uint8_t)value;
}

static inline uint16_t video_play_yuv_to_rgb565(uint8_t y, uint8_t u, uint8_t v)
{
    int c = (int)y - 16;
    int d = (int)u - 128;
    int e = (int)v - 128;

    if (c < 0)
    {
        c = 0;
    }

    uint8_t r = video_play_clip_u8((298 * c + 409 * e + 128) >> 8);
    uint8_t g = video_play_clip_u8((298 * c - 100 * d - 208 * e + 128) >> 8);
    uint8_t b = video_play_clip_u8((298 * c + 516 * d + 128) >> 8);

    return (uint16_t)((((uint16_t)r & 0xF8U) << 8) |
                      (((uint16_t)g & 0xFCU) << 3) |
                      (((uint16_t)b) >> 3));
}

static avdk_err_t video_play_nv12_rotate_to_rgb565(const video_player_video_frame_meta_t *meta,
                                                   const uint8_t *src,
                                                   video_play_rotate_mode_t rotate,
                                                   void **out_pixel)
{
    if (meta == NULL || src == NULL || out_pixel == NULL)
    {
        return AVDK_ERR_INVAL;
    }

    if (rotate != VIDEO_PLAY_ROTATE_90 && rotate != VIDEO_PLAY_ROTATE_270)
    {
        return AVDK_ERR_UNSUPPORTED;
    }

    const uint32_t src_w = meta->video.width;
    const uint32_t src_h = meta->video.height;
    if (src_w == 0U || src_h == 0U || ((src_w | src_h) & 1U) != 0U)
    {
        LOGW("%s: invalid NV12 size %ux%u\n", __func__, (unsigned)src_w, (unsigned)src_h);
        return AVDK_ERR_INVAL;
    }

    const uint32_t src_stride = (src_w + 15U) & ~15U;
    const uint32_t src_aligned_h = (src_h + 15U) & ~15U;
    const uint8_t *src_y = src;
    const uint8_t *src_uv = src + (src_stride * src_aligned_h);
    const uint32_t dst_w = src_h;
    const uint32_t dst_h = src_w;
    const uint32_t dst_size = dst_w * dst_h * 2U;

    uint16_t *dst = (uint16_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                                       video_play_slab_alloc_size(dst_size + VIDEO_FRAME_BUFFER_SAFETY_PAD_BYTES));
    if (dst == NULL)
    {
        LOGE("%s: allocate rotated RGB565 frame failed, size=%u\n", __func__, (unsigned)dst_size);
        return AVDK_ERR_NOMEM;
    }

    for (uint32_t y = 0; y < src_h; y++)
    {
        const uint8_t *y_row = src_y + (y * src_stride);
        const uint8_t *uv_row = src_uv + ((y >> 1) * src_stride);

        for (uint32_t x = 0; x < src_w; x++)
        {
            const uint8_t yy = y_row[x];
            const uint32_t uv_x = x & ~1U;
            const uint8_t u = uv_row[uv_x];
            const uint8_t v = uv_row[uv_x + 1U];
            uint32_t dst_x;
            uint32_t dst_y;

            if (rotate == VIDEO_PLAY_ROTATE_90)
            {
                dst_x = src_h - 1U - y;
                dst_y = x;
            }
            else
            {
                dst_x = y;
                dst_y = src_w - 1U - x;
            }

            dst[(dst_y * dst_w) + dst_x] = video_play_yuv_to_rgb565(yy, u, v);
        }
    }

    *out_pixel = dst;
    return AVDK_ERR_OK;
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

static uint32_t video_play_agent_sample32(const void *frame, uint32_t size, uint32_t offset)
{
    if (frame == NULL || size < sizeof(uint32_t))
    {
        return 0U;
    }
    if (offset > (size - sizeof(uint32_t)))
    {
        offset = size - sizeof(uint32_t);
    }
    offset &= ~3U;
    return *((volatile const uint32_t *)((const uint8_t *)frame + offset));
}
 
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

void video_play_video_decode_complete_cb(void *user_data, const video_player_video_frame_meta_t *meta, video_player_buffer_t *buffer)
{
    if (buffer == NULL || buffer->data == NULL)
    {
        return;
    }

    video_play_user_ctx_t *ctx = (video_play_user_ctx_t *)user_data;
    void *pixel        = buffer->data;
    uint32_t pixel_len = buffer->length;
    const uint32_t decoder_pixel_fmt = (meta != NULL) ? (uint32_t)meta->output_format : 0U;
    const video_play_rotate_mode_t rotate_mode = video_play_video_effective_rotate_mode(meta);
    const bool h264_output_frame = (meta != NULL &&
                                    meta->video.format == VIDEO_PLAYER_VIDEO_FORMAT_H264 &&
                                    (decoder_pixel_fmt == PIXEL_FMT_ARGB8888 ||
                                     decoder_pixel_fmt == PIXEL_FMT_RGB565));
    bool rotated_rgb565 = false;

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
        meta->video.format == VIDEO_PLAYER_VIDEO_FORMAT_MJPEG)
    {
        void *rotated_pixel = NULL;
        avdk_err_t rotate_ret = video_play_nv12_rotate_to_rgb565(meta,
                                                                 (const uint8_t *)pixel,
                                                                 rotate_mode,
                                                                 &rotated_pixel);
        if (rotate_ret == AVDK_ERR_OK && rotated_pixel != NULL)
        {
            video_play_free_output_pixel(decoder_pixel_fmt, pixel);
            pixel = rotated_pixel;
            rotated_rgb565 = true;
        }
        else
        {
            LOGW("%s: rotate to RGB565 failed, ret=%d; display original frame\n", __func__, rotate_ret);
        }
    }

    video_play_lcd_sync_format_for_output_frame(ctx->lcd_handle, decoder_pixel_fmt, rotated_rgb565);

    avdk_err_t (*free_cb)(void *) = display_frame_free_cb;
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    if (h264_output_frame)
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
