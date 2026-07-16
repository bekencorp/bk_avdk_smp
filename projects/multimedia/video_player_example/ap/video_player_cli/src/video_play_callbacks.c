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
                                                        uint32_t display_pixel_fmt,
                                                        bool argb8888_compressed)
{
    if (handle == NULL)
    {
        return;
    }

    video_play_lcd_video_fmt_t need = VIDEO_PLAY_LCD_VIDEO_FMT_NV12_RAW;
#if CONFIG_BK_VIDEO_PLAYER_ENABLE_HW_H264_VIDEO_DECODER
    if (display_pixel_fmt == PIXEL_FMT_ARGB8888)
    {
        need = argb8888_compressed
            ? VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_COMPRESSED
            : VIDEO_PLAY_LCD_VIDEO_FMT_ARGB8888_RAW;
    }
    else if (display_pixel_fmt == PIXEL_FMT_RGB888)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_RGB888_RAW;
    }
    else if (display_pixel_fmt == PIXEL_FMT_RGB565)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_RGB565_RAW;
    }
#else
    (void)argb8888_compressed;
    if (display_pixel_fmt == PIXEL_FMT_RGB565)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_RGB565_RAW;
    }
    else if (display_pixel_fmt == PIXEL_FMT_RGB888)
    {
        need = VIDEO_PLAY_LCD_VIDEO_FMT_RGB888_RAW;
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

/* ================= Display worker (offload post-decode work) =================
 *
 * decode_complete_cb runs on the video decode thread, which has already done PTS
 * pacing. Doing the heavy GPU rotate + LCD format sync + bk_display_flush there
 * would stall decoding and couple decode throughput to display latency. Instead
 * the callback becomes a thin producer that snapshots the frame into a queue and
 * returns; this worker thread performs the actual post-decode processing.
 *
 * Threading model: single producer (decode thread) / single consumer (worker).
 * Queue is drop-oldest on full, so the newest frame always wins and memory stays
 * bounded.
 */
#define VIDEO_PLAY_DISPLAY_QUEUE_DEPTH        (1U)
#define VIDEO_PLAY_DISPLAY_WORKER_STACK_SIZE  (8 * 1024)
#define VIDEO_PLAY_DISPLAY_WORKER_PRIORITY    (BEKEN_DEFAULT_WORKER_PRIORITY)
#define VIDEO_PLAY_DISPLAY_POP_TIMEOUT_MS     (50U)

typedef struct
{
    void *pixel;                            /* owned raw decoded frame (pre-rotate) */
    uint32_t decoder_pixel_fmt;
    video_player_video_format_t video_format;
    uint16_t width;
    uint16_t height;
    video_play_rotate_mode_t rotate_mode;   /* snapshot at enqueue time */
    bk_display_ctlr_handle_t lcd_handle;
} video_play_display_node_t;

static beken_queue_t s_display_queue = NULL;
static beken_thread_t s_display_thread = NULL;
static beken_semaphore_t s_display_exit_sem = NULL;
static volatile bool s_display_worker_exit = false;
static bool s_display_worker_ready = false;

/* Perform the post-decode processing that used to run inline in the callback:
 * optional GPU rotate, LCD format sync, and the display flush. */
static void video_play_display_process_node(const video_play_display_node_t *node)
{
    if (node == NULL || node->pixel == NULL)
    {
        return;
    }

    void *pixel = node->pixel;
    const uint32_t decoder_pixel_fmt = node->decoder_pixel_fmt;
    const video_play_rotate_mode_t rotate_mode = node->rotate_mode;
    const bool h264_output_frame = (node->video_format == VIDEO_PLAYER_VIDEO_FORMAT_H264 &&
                                    (decoder_pixel_fmt == PIXEL_FMT_ARGB8888 ||
                                     decoder_pixel_fmt == PIXEL_FMT_RGB565));
    uint32_t display_pixel_fmt = decoder_pixel_fmt;
    bool gpu_post_frame = false;

    if (node->lcd_handle == NULL)
    {
        video_play_free_output_pixel(decoder_pixel_fmt, pixel);
        return;
    }

    if (rotate_mode != VIDEO_PLAY_ROTATE_NONE &&
        decoder_pixel_fmt == PIXEL_FMT_NV12 &&
        (node->video_format == VIDEO_PLAYER_VIDEO_FORMAT_MJPEG ||
         node->video_format == VIDEO_PLAYER_VIDEO_FORMAT_H264))
    {
        const uint32_t src_w = node->width;
        const uint32_t src_h = node->height;
        const uint32_t src_stride = (node->video_format == VIDEO_PLAYER_VIDEO_FORMAT_MJPEG)
                                    ? ((src_w + 15U) & ~15U)
                                    : src_w;
        const uint32_t src_y_height = (node->video_format == VIDEO_PLAYER_VIDEO_FORMAT_MJPEG)
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

    bool display_argb8888_compressed = false;
    if (display_pixel_fmt == PIXEL_FMT_ARGB8888)
    {
#if VIDEO_PLAY_H264_FLEXA_RAW_ARGB8888_ENABLE
        display_argb8888_compressed = gpu_post_frame;
#else
        display_argb8888_compressed = true;
#endif
    }

    video_play_lcd_sync_format_for_output_frame(node->lcd_handle,
                                                display_pixel_fmt,
                                                display_argb8888_compressed);

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

    avdk_err_t ret = bk_display_flush(node->lcd_handle, pixel, free_cb);
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

/* Free a queued node's frame without displaying it (drop / teardown paths).
 * Queued frames are always the raw decoded (pre-rotate) buffer, so the
 * allocator-aware free is the correct release path. */
static void video_play_display_node_discard(const video_play_display_node_t *node)
{
    if (node != NULL && node->pixel != NULL)
    {
        video_play_free_output_pixel(node->decoder_pixel_fmt, node->pixel);
    }
}

static void video_play_display_worker_thread(void *arg)
{
    (void)arg;

    LOGW("%s: display worker started\n", __func__);

    while (!s_display_worker_exit)
    {
        video_play_display_node_t node;
        if (rtos_pop_from_queue(&s_display_queue, &node, VIDEO_PLAY_DISPLAY_POP_TIMEOUT_MS) == BK_OK)
        {
            video_play_display_process_node(&node);
        }
    }

    /* Exit requested: drain any leftover frames and free them (do not display,
     * the LCD is about to be closed by the runtime teardown). */
    video_play_display_node_t node;
    while (rtos_pop_from_queue(&s_display_queue, &node, BEKEN_NO_WAIT) == BK_OK)
    {
        video_play_display_node_discard(&node);
    }

    LOGW("%s: display worker exiting\n", __func__);
    rtos_set_semaphore(&s_display_exit_sem);
    rtos_delete_thread(NULL);
}

avdk_err_t video_play_display_worker_init(void)
{
    if (s_display_worker_ready)
    {
        return AVDK_ERR_OK;
    }

    s_display_worker_exit = false;

    if (rtos_init_queue(&s_display_queue, "vp_disp_q",
                        sizeof(video_play_display_node_t),
                        VIDEO_PLAY_DISPLAY_QUEUE_DEPTH) != BK_OK)
    {
        LOGE("%s: init display queue failed\n", __func__);
        s_display_queue = NULL;
        return AVDK_ERR_NOMEM;
    }

    if (rtos_init_semaphore(&s_display_exit_sem, 1) != BK_OK)
    {
        LOGE("%s: init display exit semaphore failed\n", __func__);
        rtos_deinit_queue(&s_display_queue);
        s_display_queue = NULL;
        return AVDK_ERR_NOMEM;
    }

    if (rtos_create_thread(&s_display_thread, VIDEO_PLAY_DISPLAY_WORKER_PRIORITY, "vp_display",
                           (beken_thread_function_t)video_play_display_worker_thread,
                           VIDEO_PLAY_DISPLAY_WORKER_STACK_SIZE,
                           NULL) != BK_OK)
    {
        LOGE("%s: create display worker thread failed\n", __func__);
        rtos_deinit_semaphore(&s_display_exit_sem);
        s_display_exit_sem = NULL;
        rtos_deinit_queue(&s_display_queue);
        s_display_queue = NULL;
        return AVDK_ERR_GENERIC;
    }

    s_display_worker_ready = true;
    LOGW("%s: display worker ready\n", __func__);
    return AVDK_ERR_OK;
}

void video_play_display_worker_deinit(void)
{
    if (!s_display_worker_ready)
    {
        return;
    }

    /* Callers guarantee the decode thread is already stopped (engine stop/close
     * joined it), so no more frames are being produced here. */
    s_display_worker_exit = true;

    if (s_display_thread != NULL)
    {
        /* Worker drains and frees any remaining queued frames before exit. */
        rtos_get_semaphore(&s_display_exit_sem, BEKEN_WAIT_FOREVER);
        s_display_thread = NULL;
    }

    if (s_display_exit_sem != NULL)
    {
        rtos_deinit_semaphore(&s_display_exit_sem);
        s_display_exit_sem = NULL;
    }

    if (s_display_queue != NULL)
    {
        rtos_deinit_queue(&s_display_queue);
        s_display_queue = NULL;
    }

    s_display_worker_ready = false;
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

    /* Transfer ownership out of the engine; null fields so buffer_free_yuv_cb
     * is a no-op and we never double-free. */
    buffer->data         = NULL;
    buffer->frame_buffer = NULL;
    buffer->length       = 0;

    /* No display target, or worker not running: never do heavy processing on the
     * decode thread. Just release the frame. */
    if (ctx == NULL || ctx->lcd_handle == NULL ||
        !s_display_worker_ready || s_display_queue == NULL)
    {
        video_play_free_output_pixel(decoder_pixel_fmt, pixel);
        return;
    }

    video_play_display_node_t node;
    node.pixel            = pixel;
    node.decoder_pixel_fmt = decoder_pixel_fmt;
    node.video_format     = (meta != NULL) ? meta->video.format : VIDEO_PLAYER_VIDEO_FORMAT_UNKNOWN;
    node.width            = (meta != NULL) ? (uint16_t)meta->video.width : 0U;
    node.height           = (meta != NULL) ? (uint16_t)meta->video.height : 0U;
    node.rotate_mode      = rotate_mode;
    node.lcd_handle       = ctx->lcd_handle;

    /* Drop-oldest on full so the newest frame always wins. Single producer means
     * the pop below cannot race another producer, so the subsequent push always
     * finds a free slot. */
    if (rtos_is_queue_full(&s_display_queue))
    {
        video_play_display_node_t old_node;
        if (rtos_pop_from_queue(&s_display_queue, &old_node, BEKEN_NO_WAIT) == BK_OK)
        {
            video_play_display_node_discard(&old_node);
        }
    }

    if (rtos_push_to_queue(&s_display_queue, &node, BEKEN_NO_WAIT) != BK_OK)
    {
        LOGW("%s: enqueue display frame failed, drop\n", __func__);
        video_play_free_output_pixel(decoder_pixel_fmt, pixel);
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
