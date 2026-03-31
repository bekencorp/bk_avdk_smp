#include <os/os.h>
#include "os/mem.h"
#include "os/str.h"
#include <components/bk_frame_buffer.h>
#include <components/log.h>
#include "h264_decoder_api.h"
#include "h264_decode_stress.h"
#include "h264_decode_stress_stream.h"
#include "psram_dma_stress.h"

#define TAG "h264_dec_stress"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#ifndef H264_DECODE_STRESS_WIDTH
#define H264_DECODE_STRESS_WIDTH 1920
#endif

#ifndef H264_DECODE_STRESS_HEIGHT
#define H264_DECODE_STRESS_HEIGHT 1080
#endif

/*
 * Decoder work buffer size (YUV420 semi-planar style layout used by legacy callback sizing).
 * Override if platform decoder needs a different output layout for the configured resolution.
 */
#ifndef H264_DECODE_STRESS_OUT_BUF_SIZE
#define H264_DECODE_STRESS_OUT_BUF_SIZE \
    ((uint32_t)(H264_DECODE_STRESS_WIDTH) * (uint32_t)(H264_DECODE_STRESS_HEIGHT) * 3U / 2U)
#endif

#ifndef H264_DECODE_STRESS_LOOP_DELAY_MS
#define H264_DECODE_STRESS_LOOP_DELAY_MS 1U
#endif

#ifndef H264_DECODE_STRESS_VERIFY_OUTPUT_SIZE
#define H264_DECODE_STRESS_VERIFY_OUTPUT_SIZE 0
#endif

#ifndef H264_DECODE_STRESS_DMA_COPY_SIZE
#define H264_DECODE_STRESS_DMA_COPY_SIZE (512 * 1024)
#endif

#ifndef H264_DECODE_STRESS_ENABLE_DMA_PRESSURE
#define H264_DECODE_STRESS_ENABLE_DMA_PRESSURE 1
#endif

#ifndef H264_DECODE_STRESS_USE_PSRAM
#define H264_DECODE_STRESS_USE_PSRAM 1
#endif

#define H264_DECODE_STRESS_THREAD_STACK_SIZE (16 * 1024)

static void *s_h264_dec_ctx = NULL;
static beken_thread_t s_h264_dec_stress_thread = NULL;
static volatile uint8_t s_h264_dec_stress_stop = 0;

static uint8_t *s_stream_psram = NULL;
static frame_buffer_t *s_stream_fb = NULL;
static uint8_t *s_out_buf = NULL;
static frame_buffer_t *s_out_fb = NULL;
#if H264_DECODE_STRESS_ENABLE_DMA_PRESSURE
static uint8_t *s_h264d_dma_src_buf = NULL;
static uint8_t *s_h264d_dma_dst_buf = NULL;
static frame_buffer_t *s_h264d_dma_src_fb = NULL;
static frame_buffer_t *s_h264d_dma_dst_fb = NULL;
static psram_dma_stress_handle_t s_h264d_dma_handle = NULL;
/* Optional override from CLI: start [dma_copy_size_kb]. */
static uint32_t s_h264d_dma_copy_size = H264_DECODE_STRESS_DMA_COPY_SIZE;
#endif

#if H264_DECODE_STRESS_VERIFY_OUTPUT_SIZE
static volatile uint8_t s_baseline_set = 0;
static uint32_t s_expected_w = 0;
static uint32_t s_expected_h = 0;
#endif

static void h264_decode_stress_out_cb(uint8_t *y, uint8_t *cb, uint8_t *cr,
                                      uint32_t width, uint32_t height, uint32_t type)
{
    (void)y;
    (void)cb;
    (void)cr;

    LOGI("h264_decode_stress_out_cb, width=%u height=%u type=%u\n", width, height, type);

#if H264_DECODE_STRESS_VERIFY_OUTPUT_SIZE
    if (type == VCDEC_OUT_INFO) {
        if (!s_baseline_set) {
            s_expected_w = width;
            s_expected_h = height;
            s_baseline_set = 1;
            LOGI("decode info baseline w=%u h=%u\n", width, height);
        } else if (width != s_expected_w || height != s_expected_h) {
            LOGE("output size changed, expect %ux%u got %ux%u type=%u\n",
                 s_expected_w, s_expected_h, width, height, type);
            s_h264_dec_stress_stop = 1;
        }
    }
#else
    (void)width;
    (void)height;
    (void)type;
#endif
}

static void h264_decode_stress_dma_pressure_close(void);

/*
 * Start optional PSRAM DMA copy stress in parallel with decode (same pattern as h264_encode_stress).
 * DMA buffers are allocated from MEM_SLAB_HEAP_CODED when H264_DECODE_STRESS_USE_PSRAM is 1.
 */
static bk_err_t h264_decode_stress_dma_pressure_open(void)
{
#if !H264_DECODE_STRESS_ENABLE_DMA_PRESSURE
    return BK_OK;
#else
    bk_err_t ret = BK_OK;

    if (s_h264d_dma_handle != NULL) {
        return BK_OK;
    }

    if (H264_DECODE_STRESS_USE_PSRAM) {
        uint32_t dma_fb_size = s_h264d_dma_copy_size + sizeof(frame_buffer_t) + 32U;

        s_h264d_dma_src_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, dma_fb_size);
        if (s_h264d_dma_src_fb == NULL) {
            LOGE("failed to allocate DMA source buffer, size=%u\n", dma_fb_size);
            goto fail;
        }
        os_memset(s_h264d_dma_src_fb, 0, sizeof(frame_buffer_t));
        s_h264d_dma_src_fb->frame = (uint8_t *)((((uint32_t)(s_h264d_dma_src_fb + 1U) >> 5U) + 1U) << 5U);
        s_h264d_dma_src_fb->size = s_h264d_dma_copy_size;
        s_h264d_dma_src_buf = s_h264d_dma_src_fb->frame;

        s_h264d_dma_dst_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, dma_fb_size);
        if (s_h264d_dma_dst_fb == NULL) {
            LOGE("failed to allocate DMA destination buffer, size=%u\n", dma_fb_size);
            goto fail;
        }
        os_memset(s_h264d_dma_dst_fb, 0, sizeof(frame_buffer_t));
        s_h264d_dma_dst_fb->frame = (uint8_t *)((((uint32_t)(s_h264d_dma_dst_fb + 1U) >> 5U) + 1U) << 5U);
        s_h264d_dma_dst_fb->size = s_h264d_dma_copy_size;
        s_h264d_dma_dst_buf = s_h264d_dma_dst_fb->frame;
    } else {
        s_h264d_dma_src_buf = (uint8_t *)os_sram_malloc(s_h264d_dma_copy_size);
        if (s_h264d_dma_src_buf == NULL) {
            LOGE("failed to allocate DMA source buffer, size=%u\n", (unsigned)s_h264d_dma_copy_size);
            goto fail;
        }
        s_h264d_dma_dst_buf = (uint8_t *)os_sram_malloc(s_h264d_dma_copy_size);
        if (s_h264d_dma_dst_buf == NULL) {
            LOGE("failed to allocate DMA destination buffer, size=%u\n", (unsigned)s_h264d_dma_copy_size);
            goto fail;
        }
    }

    ret = psram_dma_stress_create(&s_h264d_dma_handle);
    if (ret != BK_OK || s_h264d_dma_handle == NULL) {
        LOGE("psram_dma_stress_create failed, ret=%d handle=%p\n", (int)ret, s_h264d_dma_handle);
        goto fail;
    }

    ret = psram_dma_stress_start(s_h264d_dma_handle,
                                 s_h264d_dma_src_buf,
                                 s_h264d_dma_dst_buf,
                                 s_h264d_dma_copy_size);
    if (ret != BK_OK) {
        LOGE("psram_dma_stress_start failed, ret=%d\n", (int)ret);
        goto fail;
    }

    return BK_OK;

fail:
    h264_decode_stress_dma_pressure_close();
    return BK_FAIL;
#endif
}

static void h264_decode_stress_dma_pressure_close(void)
{
#if !H264_DECODE_STRESS_ENABLE_DMA_PRESSURE
    return;
#else
    if (s_h264d_dma_handle != NULL) {
        psram_dma_stress_stop(s_h264d_dma_handle);
        psram_dma_stress_deinit(s_h264d_dma_handle);
        s_h264d_dma_handle = NULL;
    }

    if (H264_DECODE_STRESS_USE_PSRAM) {
        if (s_h264d_dma_dst_fb != NULL) {
            bk_frame_buffer_free(s_h264d_dma_dst_fb);
            s_h264d_dma_dst_fb = NULL;
            s_h264d_dma_dst_buf = NULL;
        }
        if (s_h264d_dma_src_fb != NULL) {
            bk_frame_buffer_free(s_h264d_dma_src_fb);
            s_h264d_dma_src_fb = NULL;
            s_h264d_dma_src_buf = NULL;
        }
    } else {
        if (s_h264d_dma_dst_buf != NULL) {
            os_free(s_h264d_dma_dst_buf);
            s_h264d_dma_dst_buf = NULL;
        }
        if (s_h264d_dma_src_buf != NULL) {
            os_free(s_h264d_dma_src_buf);
            s_h264d_dma_src_buf = NULL;
        }
    }
#endif
}

static void h264_decode_stress_thread_entry(void *arg)
{
    int32_t dec_ret;
    uint32_t frame_round = 0;
    uint32_t used_size = 0;

    (void)arg;

    s_h264_dec_stress_stop = 0;
#if H264_DECODE_STRESS_VERIFY_OUTPUT_SIZE
    s_baseline_set = 0;
    s_expected_w = 0;
    s_expected_h = 0;
#endif

    if (h264_decode_stress_stream_bytes == 0U) {
        LOGE("embedded stream size is zero\n");
        goto exit_thread;
    }

    {
        uint32_t fb_size = h264_decode_stress_stream_bytes + (uint32_t)sizeof(frame_buffer_t) + 32U;

        /* Use coded heap for all bk_frame_buffer_malloc allocations. */
        s_stream_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, fb_size);
        if (s_stream_fb == NULL) {
            LOGE("failed to allocate PSRAM stream buffer, size=%u\n", fb_size);
            goto exit_thread;
        }
        os_memset(s_stream_fb, 0, sizeof(frame_buffer_t));
        s_stream_fb->frame = (uint8_t *)((((uint32_t)(s_stream_fb + 1U) >> 5U) + 1U) << 5U);
        s_stream_fb->size = h264_decode_stress_stream_bytes;
        s_stream_psram = s_stream_fb->frame;
        os_memcpy(s_stream_psram, h264_decode_stress_stream_global, h264_decode_stress_stream_bytes);
    }

    {
        uint32_t out_fb_size = H264_DECODE_STRESS_OUT_BUF_SIZE + (uint32_t)sizeof(frame_buffer_t) + 32U;

        /* Use coded heap for all bk_frame_buffer_malloc allocations. */
        s_out_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, out_fb_size);
        if (s_out_fb == NULL) {
            LOGE("failed to allocate decoder output buffer, size=%u\n", out_fb_size);
            goto exit_thread;
        }
        os_memset(s_out_fb, 0, sizeof(frame_buffer_t));
        s_out_fb->frame = (uint8_t *)((((uint32_t)(s_out_fb + 1U) >> 5U) + 1U) << 5U);
        s_out_fb->size = H264_DECODE_STRESS_OUT_BUF_SIZE;
        s_out_buf = s_out_fb->frame;
    }

    dec_ret = h264_decoder_init(&s_h264_dec_ctx, VCDEC_FLEXA_MODE_NONE, NULL, NULL, h264_decode_stress_out_cb);
    if (dec_ret != 0 || s_h264_dec_ctx == NULL) {
        LOGE("h264_decoder_init failed, ret=%d\n", (int)dec_ret);
        goto exit_thread;
    }

    if (h264_decode_stress_dma_pressure_open() != BK_OK) {
        goto exit_thread;
    }

    LOGI("h264 decode stress thread started, target resolution %ux%u (macros), stream_bytes=%u\n",
         (unsigned)H264_DECODE_STRESS_WIDTH, (unsigned)H264_DECODE_STRESS_HEIGHT,
         (unsigned)h264_decode_stress_stream_bytes);

    while (s_h264_dec_stress_stop == 0) {
        uint8_t *in_ptr = s_stream_psram;
        uint32_t remain = h264_decode_stress_stream_bytes;

        while (remain > 0U && s_h264_dec_stress_stop == 0) {
            used_size = 0;
            dec_ret = h264_decoder_decode(s_h264_dec_ctx, in_ptr, remain, s_out_buf, &used_size);
            if (dec_ret < 0) {
                LOGE("h264_decoder_decode error, ret=%d round=%u remain=%u\n",
                     (int)dec_ret, (unsigned)frame_round, (unsigned)remain);
                s_h264_dec_stress_stop = 1;
                break;
            }
            if (used_size == 0U) {
                LOGE("h264_decoder_decode consumed zero bytes, round=%u remain=%u ret=%d\n",
                     (unsigned)frame_round, (unsigned)remain, (int)dec_ret);
                s_h264_dec_stress_stop = 1;
                break;
            }
            if (used_size > remain) {
                LOGE("invalid usedSize=%u remain=%u\n", (unsigned)used_size, (unsigned)remain);
                s_h264_dec_stress_stop = 1;
                break;
            }
            in_ptr += used_size;
            remain -= used_size;
        }

        if (s_h264_dec_stress_stop != 0) {
            break;
        }

        frame_round++;
        if (H264_DECODE_STRESS_LOOP_DELAY_MS > 0U) {
            rtos_delay_milliseconds((uint32_t)H264_DECODE_STRESS_LOOP_DELAY_MS);
        }
    }

exit_thread:
    h264_decode_stress_dma_pressure_close();

    if (s_h264_dec_ctx != NULL) {
        h264_decoder_deinit(s_h264_dec_ctx);
        s_h264_dec_ctx = NULL;
    }

    if (s_out_fb != NULL) {
        bk_frame_buffer_free(s_out_fb);
        s_out_fb = NULL;
        s_out_buf = NULL;
    }
    if (s_stream_fb != NULL) {
        bk_frame_buffer_free(s_stream_fb);
        s_stream_fb = NULL;
        s_stream_psram = NULL;
    }

    LOGI("h264 decode stress thread exit, rounds=%u stop=%u\n",
         (unsigned)frame_round, (unsigned)s_h264_dec_stress_stop);

    s_h264_dec_stress_thread = NULL;
    rtos_delete_thread(NULL);
}

void cli_h264_decode_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;
    const char *msg = CLI_CMD_RSP_SUCCEED;

    if (argc < 2) {
        LOGE("usage: h264_decode_stress <start [dma_copy_size_kb]|stop|dma_open|dma_close>\n");
        ret = BK_FAIL;
        msg = CLI_CMD_RSP_ERROR;
        goto exit;
    }

    if (os_strcmp(argv[1], "start") == 0) {
        /* Optional arg: start <dma_copy_size_kb> (KB). */
#if H264_DECODE_STRESS_ENABLE_DMA_PRESSURE
        if (argc >= 3) {
            uint32_t user_dma_copy_size_kb = (uint32_t)os_strtoul(argv[2], NULL, 0);
            if (user_dma_copy_size_kb == 0U) {
                LOGE("invalid dma_copy_size_kb=%u\n", (unsigned)user_dma_copy_size_kb);
                ret = BK_FAIL;
                msg = CLI_CMD_RSP_ERROR;
                goto exit;
            }
            if (user_dma_copy_size_kb > (0xFFFFFFFFU / 1024U)) {
                LOGE("dma_copy_size_kb too large: %u\n", (unsigned)user_dma_copy_size_kb);
                ret = BK_FAIL;
                msg = CLI_CMD_RSP_ERROR;
                goto exit;
            }
            s_h264d_dma_copy_size = user_dma_copy_size_kb * 1024U;
        } else {
            s_h264d_dma_copy_size = H264_DECODE_STRESS_DMA_COPY_SIZE;
        }
#endif
        if (s_h264_dec_stress_thread != NULL) {
            LOGE("h264 decode stress thread already running\n");
            ret = BK_FAIL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        s_h264_dec_stress_stop = 0;
        ret = rtos_create_thread(&s_h264_dec_stress_thread,
                                 BEKEN_DEFAULT_WORKER_PRIORITY,
                                 "h264_dec_stress",
                                 (beken_thread_function_t)h264_decode_stress_thread_entry,
                                 H264_DECODE_STRESS_THREAD_STACK_SIZE,
                                 NULL);
        if (ret != BK_OK) {
            LOGE("create h264 decode stress thread failed, ret=%d\n", (int)ret);
            s_h264_dec_stress_thread = NULL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
    } else if (os_strcmp(argv[1], "stop") == 0) {
        if (s_h264_dec_stress_thread == NULL) {
            LOGE("h264 decode stress thread not running\n");
            ret = BK_FAIL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
        s_h264_dec_stress_stop = 1;
    } else if (os_strcmp(argv[1], "dma_open") == 0) {
        ret = h264_decode_stress_dma_pressure_open();
        if (ret != BK_OK) {
            LOGE("h264_decode_stress_dma_pressure_open failed, ret=%d\n", (int)ret);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
    } else if (os_strcmp(argv[1], "dma_close") == 0) {
        h264_decode_stress_dma_pressure_close();
        ret = BK_OK;
    } else {
        LOGE("usage: h264_decode_stress <start [dma_copy_size_kb]|stop|dma_open|dma_close>\n");
        ret = BK_FAIL;
        msg = CLI_CMD_RSP_ERROR;
        goto exit;
    }

exit:
    if (pcWriteBuffer != NULL && xWriteBufferLen > 0) {
        int len = os_strlen(msg);
        if (len >= xWriteBufferLen) {
            len = xWriteBufferLen - 1;
        }
        os_memcpy(pcWriteBuffer, msg, (size_t)len);
        pcWriteBuffer[len] = '\0';
    }
}
