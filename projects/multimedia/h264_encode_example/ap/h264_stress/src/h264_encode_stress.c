#include <os/os.h>
#include <os/str.h>
#include "os/mem.h"
#include <components/bk_frame_buffer.h>
#include "h264_encoder_api.h"
#include "h264_encode_stress.h"
#include "psram_dma_stress.h"

#define TAG "h264_enc_stress"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#ifndef VCENC_INTRA_FRAME
#define VCENC_INTRA_FRAME 0u
#endif

#ifndef VCENC_PREDICTED_FRAME
#define VCENC_PREDICTED_FRAME 1u
#endif

#ifndef H264_STRESS_WIDTH
#define H264_STRESS_WIDTH 1920
#endif

#ifndef H264_STRESS_HEIGHT
#define H264_STRESS_HEIGHT 1080
#endif

#define H264_STRESS_YUV_SIZE ((uint32_t)(H264_STRESS_WIDTH) * (uint32_t)(H264_STRESS_HEIGHT) * 3U / 2U)
/* Keep same default policy as legacy wrapper internal allocation: width * height bytes. */
#define H264_STRESS_OUT_BUF_SIZE ((uint32_t)(H264_STRESS_WIDTH) * (uint32_t)(H264_STRESS_HEIGHT))
#ifndef H264_STRESS_DMA_COPY_SIZE
#define H264_STRESS_DMA_COPY_SIZE (512 * 1024)
#endif

#ifndef H264_STRESS_ENABLE_DMA_PRESSURE
#define H264_STRESS_ENABLE_DMA_PRESSURE 1
#endif

#ifndef H264_STRESS_USE_PSRAM
#define H264_STRESS_USE_PSRAM 1
#endif

#ifndef H264_STRESS_RED_Y
/*
 * NV12 pure-red (R=255,G=0,B=0) converted with BT.601 and then clamped.
 * The exact values may vary slightly by platform, but these are stable for stress testing.
 */
#define H264_STRESS_RED_Y 76u
#endif

#ifndef H264_STRESS_RED_U
#define H264_STRESS_RED_U 91u
#endif

#ifndef H264_STRESS_RED_V
#define H264_STRESS_RED_V 255u
#endif

#define H264_STRESS_THREAD_STACK_SIZE (16 * 1024)

/* stack_mem_dump is implemented in platform misc stack_base code. */
extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);

static void h264_stress_fill_nv12_pure_red(uint8_t *yuv)
{
    uint32_t i;
    uint32_t y_plane_size = (uint32_t)H264_STRESS_WIDTH * (uint32_t)H264_STRESS_HEIGHT;

    if (yuv == NULL) {
        return;
    }

    /* Fill Y plane. */
    for (i = 0; i < y_plane_size; i++) {
        yuv[i] = (uint8_t)H264_STRESS_RED_Y;
    }

    /* Fill interleaved UV plane (NV12: U,V pairs). */
    for (i = y_plane_size; i + 1U < H264_STRESS_YUV_SIZE; i += 2U) {
        yuv[i] = (uint8_t)H264_STRESS_RED_U;
        yuv[i + 1U] = (uint8_t)H264_STRESS_RED_V;
    }
}

static void *s_h264_encoder = NULL;
static beken_thread_t s_h264_stress_thread = NULL;
static volatile uint8_t s_h264_stress_stop = 0;
static uint32_t s_h264_expected_size = 0;
static uint32_t s_h264_expected_type = 0;
static uint8_t *s_h264_yuv_src = NULL;
static uint8_t *s_h264_yuv_work = NULL;
static uint8_t *s_h264_out_buf = NULL;
static frame_buffer_t *s_h264_yuv_src_fb = NULL;
static frame_buffer_t *s_h264_yuv_work_fb = NULL;
static frame_buffer_t *s_h264_out_fb = NULL;
#if H264_STRESS_ENABLE_DMA_PRESSURE
static uint8_t *s_h264_dma_src_buf = NULL;
static uint8_t *s_h264_dma_dst_buf = NULL;
static frame_buffer_t *s_h264_dma_src_fb = NULL;
static frame_buffer_t *s_h264_dma_dst_fb = NULL;
static psram_dma_stress_handle_t s_h264_dma_handle = NULL;
/* Allow user to override DMA copy size from CLI: start <dma_copy_size>. */
static uint32_t s_h264_dma_copy_size = H264_STRESS_DMA_COPY_SIZE;
#endif

static void h264_stress_dma_pressure_close(void);

/* One-round dump mode: dump 1 header + 1 I + 29 P frames via stack_mem_dump. */
static volatile uint8_t s_h264_one_round_dump_mode = 0;
static uint32_t s_h264_one_round_header_cnt = 0;
static uint32_t s_h264_one_round_i_cnt = 0;
static uint32_t s_h264_one_round_p_cnt = 0;

static void h264_stress_dump_bitstream_with_marker(const char *tag,
                                                     uint32_t p_index,
                                                     uint8_t *buffer,
                                                     uint32_t size,
                                                     uint32_t type)
{
    uint32_t dump_aligned = (size + 3U) & ~3U;
    uint32_t irq_flags;

    irq_flags = rtos_enter_critical();
    LOGI("h264_frame_dump tag=%s p_index=%u type=%u size=%u buf=%p aligned=%u\n",
         tag, p_index, type, size, buffer, dump_aligned);

    stack_mem_dump((uint32_t)(uintptr_t)buffer,
                    (uint32_t)(uintptr_t)(buffer + dump_aligned));
    rtos_exit_critical(irq_flags);
}

static void h264_stress_fill_source_pattern(void)
{
    uint32_t i;
    uint32_t y_plane_size = (uint32_t)H264_STRESS_WIDTH * (uint32_t)H264_STRESS_HEIGHT;
    if (!s_h264_yuv_src) {
        return;
    }

    /* Fill Y plane. */
    for (i = 0; i < y_plane_size; i++) {
        s_h264_yuv_src[i] = (uint8_t)H264_STRESS_RED_Y;
    }

    /* Fill interleaved UV plane (NV12: U,V pairs). */
    for (i = y_plane_size; i + 1U < H264_STRESS_YUV_SIZE; i += 2U) {
        s_h264_yuv_src[i] = (uint8_t)H264_STRESS_RED_U;
        s_h264_yuv_src[i + 1U] = (uint8_t)H264_STRESS_RED_V;
    }
}

static void h264_stress_out_callback(uint8_t *buffer, uint32_t size, uint32_t type)
{
    LOGI("h264_stress_out_callback buffer=%p size=%u type=%u\n", buffer, size, type);

    if (s_h264_one_round_dump_mode && !s_h264_stress_stop) {
        /* Only dump when running one-round capture; stop flag controls exit from thread. */
        if (buffer != NULL && size > 0) {
            if (type == VCENC_OUT_HEADER) {
                if (s_h264_one_round_header_cnt == 0U) {
                    h264_stress_dump_bitstream_with_marker("h264_h", 0U, buffer, size, type);
                    s_h264_one_round_header_cnt = 1U;
                }
            } else if (type == VCENC_OUT_IFRAME) {
                if (s_h264_one_round_i_cnt == 0U) {
                    h264_stress_dump_bitstream_with_marker("h264_i", 0U, buffer, size, type);
                    s_h264_one_round_i_cnt = 1U;
                }
            } else if (type == VCENC_OUT_PFRAME) {
                if (s_h264_one_round_p_cnt < 29U) {
                    uint32_t idx = s_h264_one_round_p_cnt + 1U;
                    h264_stress_dump_bitstream_with_marker("h264_p", idx, buffer, size, type);
                    s_h264_one_round_p_cnt = idx;
                }
            }
        }

        if (s_h264_one_round_header_cnt == 1U &&
            s_h264_one_round_i_cnt == 1U &&
            s_h264_one_round_p_cnt == 29U) {
            s_h264_one_round_dump_mode = 0U;
            s_h264_stress_stop = 1U;
        }
    }
#if 0
    if (buffer == NULL || size == 0) {
         LOGE("%s, invalid output buffer or size, size=%u, type=%u\n", __func__, size, type);
         s_h264_stress_stop = 1;
         return;
    }

    if (type == VCENC_OUT_HEADER || type == VCENC_OUT_ENDING) {
        return;
    }

    if (s_h264_expected_size == 0) {
        s_h264_expected_size = size;
        s_h264_expected_type = type;
        LOGI("stress baseline set, size=%u, type=%u\n", size, type);
        return;
    }

    if (size != s_h264_expected_size || type != s_h264_expected_type) {
        LOGE("stress mismatch, expect size=%u type=%u, got size=%u type=%u\n",
            s_h264_expected_size, s_h264_expected_type, size, type);
        s_h264_stress_stop = 1;
    }
#endif
}

static bk_err_t h264_stress_dma_pressure_open(void)
{
#if !H264_STRESS_ENABLE_DMA_PRESSURE
    return BK_OK;
#else
    bk_err_t ret = BK_OK;

    if (s_h264_dma_handle != NULL) {
        return BK_OK;
    }

    if (H264_STRESS_USE_PSRAM) {
        uint32_t dma_fb_size = s_h264_dma_copy_size + sizeof(frame_buffer_t) + 32U;

        s_h264_dma_src_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, dma_fb_size);
        if (!s_h264_dma_src_fb) {
            LOGE("failed to allocate DMA source buffer, size=%u\n", dma_fb_size);
            goto fail;
        }
        os_memset(s_h264_dma_src_fb, 0, sizeof(frame_buffer_t));
        s_h264_dma_src_fb->frame = (uint8_t *)((((uint32_t)(s_h264_dma_src_fb + 1U) >> 5U) + 1U) << 5U);
        s_h264_dma_src_fb->size = s_h264_dma_copy_size;
        s_h264_dma_src_buf = s_h264_dma_src_fb->frame;

        s_h264_dma_dst_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, dma_fb_size);
        if (!s_h264_dma_dst_fb) {
            LOGE("failed to allocate DMA destination buffer, size=%u\n", dma_fb_size);
            goto fail;
        }
        os_memset(s_h264_dma_dst_fb, 0, sizeof(frame_buffer_t));
        s_h264_dma_dst_fb->frame = (uint8_t *)((((uint32_t)(s_h264_dma_dst_fb + 1U) >> 5U) + 1U) << 5U);
        s_h264_dma_dst_fb->size = s_h264_dma_copy_size;
        s_h264_dma_dst_buf = s_h264_dma_dst_fb->frame;
    } else {
        s_h264_dma_src_buf = (uint8_t *)os_sram_malloc(s_h264_dma_copy_size);
        if (!s_h264_dma_src_buf) {
            LOGE("failed to allocate DMA source buffer, size=%u\n", (unsigned)s_h264_dma_copy_size);
            goto fail;
        }

        s_h264_dma_dst_buf = (uint8_t *)os_sram_malloc(s_h264_dma_copy_size);
        if (!s_h264_dma_dst_buf) {
            LOGE("failed to allocate DMA destination buffer, size=%u\n", (unsigned)s_h264_dma_copy_size);
            goto fail;
        }
    }

    ret = psram_dma_stress_create(&s_h264_dma_handle);
    if (ret != BK_OK || s_h264_dma_handle == NULL) {
        LOGE("psram_dma_stress_create failed, ret=%d handle=%p\n", (int)ret, s_h264_dma_handle);
        goto fail;
    }

    ret = psram_dma_stress_start(s_h264_dma_handle,
                                 s_h264_dma_src_buf,
                                 s_h264_dma_dst_buf,
                                 s_h264_dma_copy_size);
    if (ret != BK_OK) {
        LOGE("psram_dma_stress_start failed, ret=%d\n", (int)ret);
        goto fail;
    }

    return BK_OK;

fail:
    h264_stress_dma_pressure_close();
    return BK_FAIL;
#endif
}

static void h264_stress_dma_pressure_close(void)
{
#if !H264_STRESS_ENABLE_DMA_PRESSURE
    return;
#else
    if (s_h264_dma_handle) {
        psram_dma_stress_stop(s_h264_dma_handle);
        psram_dma_stress_deinit(s_h264_dma_handle);
        s_h264_dma_handle = NULL;
    }

    if (H264_STRESS_USE_PSRAM) {
        if (s_h264_dma_dst_fb) {
            bk_frame_buffer_free(s_h264_dma_dst_fb);
            s_h264_dma_dst_fb = NULL;
            s_h264_dma_dst_buf = NULL;
        }
        if (s_h264_dma_src_fb) {
            bk_frame_buffer_free(s_h264_dma_src_fb);
            s_h264_dma_src_fb = NULL;
            s_h264_dma_src_buf = NULL;
        }
    } else {
        if (s_h264_dma_dst_buf) {
            os_free(s_h264_dma_dst_buf);
            s_h264_dma_dst_buf = NULL;
        }
        if (s_h264_dma_src_buf) {
            os_free(s_h264_dma_src_buf);
            s_h264_dma_src_buf = NULL;
        }
    }
#endif
}

static void h264_stress_thread_entry(void *arg)
{
    int32_t ret;
    uint32_t frame_index = 0;

    (void)arg;

    s_h264_stress_stop = 0;
    s_h264_expected_size = 0;
    s_h264_expected_type = 0;

    if (H264_STRESS_USE_PSRAM) {
        uint32_t fb_size = H264_STRESS_YUV_SIZE + sizeof(frame_buffer_t) + 32U;
        uint32_t out_fb_size = H264_STRESS_OUT_BUF_SIZE + sizeof(frame_buffer_t) + 32U;

        /* Use coded heap for all bk_frame_buffer_malloc allocations. */
        s_h264_yuv_src_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, fb_size);
        if (!s_h264_yuv_src_fb) {
            LOGE("failed to allocate source YUV frame buffer, size=%u\n", fb_size);
            goto exit;
        }
        os_memset(s_h264_yuv_src_fb, 0, sizeof(frame_buffer_t));
        s_h264_yuv_src_fb->frame = (uint8_t *)((((uint32_t)(s_h264_yuv_src_fb + 1U) >> 5U) + 1U) << 5U);
        s_h264_yuv_src_fb->size = H264_STRESS_YUV_SIZE;
        s_h264_yuv_src = s_h264_yuv_src_fb->frame;

        /* Use coded heap for all bk_frame_buffer_malloc allocations. */
        s_h264_yuv_work_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, fb_size);
        if (!s_h264_yuv_work_fb) {
            LOGE("failed to allocate work YUV frame buffer, size=%u\n", fb_size);
            goto exit;
        }
        os_memset(s_h264_yuv_work_fb, 0, sizeof(frame_buffer_t));
        s_h264_yuv_work_fb->frame = (uint8_t *)((((uint32_t)(s_h264_yuv_work_fb + 1U) >> 5U) + 1U) << 5U);
        s_h264_yuv_work_fb->size = H264_STRESS_YUV_SIZE;
        s_h264_yuv_work = s_h264_yuv_work_fb->frame;

        /*
         * Pre-allocate stream output buffer from frame-buffer heap (PSRAM) and pass it to
         * h264_encoder_encode(), so legacy wrapper does not fallback to internal os_malloc().
         */
        s_h264_out_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, out_fb_size);
        if (!s_h264_out_fb) {
            LOGE("failed to allocate output stream frame buffer, size=%u\n", out_fb_size);
            goto exit;
        }
        os_memset(s_h264_out_fb, 0, sizeof(frame_buffer_t));
        s_h264_out_fb->frame = (uint8_t *)((((uint32_t)(s_h264_out_fb + 1U) >> 5U) + 1U) << 5U);
        s_h264_out_fb->size = H264_STRESS_OUT_BUF_SIZE;
        s_h264_out_buf = s_h264_out_fb->frame;

    } else {
        s_h264_yuv_src = (uint8_t *)os_sram_malloc(H264_STRESS_YUV_SIZE);
        if (!s_h264_yuv_src) {
            LOGE("failed to allocate source YUV buffer, size=%u\n", H264_STRESS_YUV_SIZE);
            goto exit;
        }

        s_h264_yuv_work = (uint8_t *)os_sram_malloc(H264_STRESS_YUV_SIZE);
        if (!s_h264_yuv_work) {
            LOGE("failed to allocate work YUV buffer, size=%u\n", H264_STRESS_YUV_SIZE);
            goto exit;
        }

        s_h264_out_buf = (uint8_t *)os_sram_malloc(H264_STRESS_OUT_BUF_SIZE);
        if (!s_h264_out_buf) {
            LOGE("failed to allocate output stream buffer, size=%u\n", H264_STRESS_OUT_BUF_SIZE);
            goto exit;
        }

    }

    h264_stress_fill_source_pattern();
    os_memcpy(s_h264_yuv_work, s_h264_yuv_src, H264_STRESS_YUV_SIZE);

    ret = h264_encoder_init(&s_h264_encoder,
                            H264_STRESS_WIDTH,
                            H264_STRESS_HEIGHT,
                            VCENC_FLEXA_MODE_NONE,
                            NULL,
                            h264_stress_out_callback);
    if (ret != 0 || s_h264_encoder == NULL) {
        LOGE("h264_encoder_init failed, ret=%d, handle=%p\n", (int)ret, s_h264_encoder);
        goto exit;
    }

    /* dump_one_round 测试要求：不启动 DMA 压测，避免干扰一轮 dump 结果。 */
    if (!s_h264_one_round_dump_mode) {
        ret = h264_stress_dma_pressure_open();
        if (ret != BK_OK) {
            goto exit;
        }
    }

    LOGI("h264 stress thread started, %ux%u\n", H264_STRESS_WIDTH, H264_STRESS_HEIGHT);

    while (!s_h264_stress_stop) {
        if (s_h264_one_round_dump_mode && frame_index >= 30U) {
            LOGE("one_round dump timeout, header=%u i=%u p=%u\n",
                 (unsigned)s_h264_one_round_header_cnt,
                 (unsigned)s_h264_one_round_i_cnt,
                 (unsigned)s_h264_one_round_p_cnt);
            s_h264_stress_stop = 1;
            break;
        }
        uint32_t coding_type = (frame_index == 0) ? VCENC_INTRA_FRAME : VCENC_PREDICTED_FRAME;
        ret = h264_encoder_encode(s_h264_encoder,
                                  (uint32_t)s_h264_yuv_work,
                                  0,
                                  coding_type,
                                  (uint32_t)s_h264_out_buf,
                                  H264_STRESS_OUT_BUF_SIZE);
        /* VCENC_OK(0)、VCENC_FRAME_READY(1) 等非负返回都视为正常，仅负值视为错误 */
        if (ret < 0) {
            LOGE("h264_encoder_encode error, ret=%d, frame_index=%u\n", (int)ret, frame_index);
            s_h264_stress_stop = 1;
            break;
        }
        frame_index++;
        rtos_delay_milliseconds(2);
    }

    LOGI("h264 stress thread exit, frame_index=%u, stop_flag=%u\n", frame_index, s_h264_stress_stop);

exit:
    s_h264_one_round_dump_mode = 0U;
    s_h264_one_round_header_cnt = 0U;
    s_h264_one_round_i_cnt = 0U;
    s_h264_one_round_p_cnt = 0U;

    h264_stress_dma_pressure_close();
    if (s_h264_encoder) {
        h264_encoder_deinit(s_h264_encoder);
        s_h264_encoder = NULL;
    }
    if (H264_STRESS_USE_PSRAM) {
        if (s_h264_out_fb) {
            bk_frame_buffer_free(s_h264_out_fb);
            s_h264_out_fb = NULL;
            s_h264_out_buf = NULL;
        }
        if (s_h264_yuv_work_fb) {
            bk_frame_buffer_free(s_h264_yuv_work_fb);
            s_h264_yuv_work_fb = NULL;
            s_h264_yuv_work = NULL;
        }
        if (s_h264_yuv_src_fb) {
            bk_frame_buffer_free(s_h264_yuv_src_fb);
            s_h264_yuv_src_fb = NULL;
            s_h264_yuv_src = NULL;
        }
    } else {
        if (s_h264_out_buf) {
            os_free(s_h264_out_buf);
            s_h264_out_buf = NULL;
        }
        if (s_h264_yuv_work) {
            os_free(s_h264_yuv_work);
            s_h264_yuv_work = NULL;
        }
        if (s_h264_yuv_src) {
            os_free(s_h264_yuv_src);
            s_h264_yuv_src = NULL;
        }
    }
    s_h264_stress_thread = NULL;
    rtos_delete_thread(NULL);
}

 void cli_h264_encode_stress_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
 {
     bk_err_t ret = BK_OK;
     const char *msg = CLI_CMD_RSP_SUCCEED;

    if (argc < 2) {
        LOGE("usage: h264_encode_stress <start [dma_copy_size_kb]|stop|dma_open|dma_close|dump_nv12|dump_one_round>\n");
         ret = BK_FAIL;
         msg = CLI_CMD_RSP_ERROR;
         goto exit;
     }

     if (os_strcmp(argv[1], "start") == 0) {
         /* Optional arg: start <dma_copy_size_kb> (KB). */
#if H264_STRESS_ENABLE_DMA_PRESSURE
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
             s_h264_dma_copy_size = user_dma_copy_size_kb * 1024U;
         } else {
             s_h264_dma_copy_size = H264_STRESS_DMA_COPY_SIZE;
         }
#endif
         if (s_h264_stress_thread != NULL) {
             LOGE("h264 stress thread already running\n");
             ret = BK_FAIL;
             msg = CLI_CMD_RSP_ERROR;
             goto exit;
         }
         s_h264_stress_stop = 0;
         ret = rtos_create_thread(&s_h264_stress_thread,
                                  BEKEN_DEFAULT_WORKER_PRIORITY,
                                  "h264_stress",
                                  (beken_thread_function_t)h264_stress_thread_entry,
                                  H264_STRESS_THREAD_STACK_SIZE,
                                  NULL);
         if (ret != BK_OK) {
             LOGE("create h264 stress thread failed, ret=%d\n", (int)ret);
             s_h264_stress_thread = NULL;
             msg = CLI_CMD_RSP_ERROR;
             goto exit;
         }
    } else if (os_strcmp(argv[1], "stop") == 0) {
         if (s_h264_stress_thread == NULL) {
             LOGE("h264 stress thread not running\n");
             ret = BK_FAIL;
             msg = CLI_CMD_RSP_ERROR;
             goto exit;
         }
         s_h264_stress_stop = 1;
    } else if (os_strcmp(argv[1], "dma_open") == 0) {
        /* Test command: open DMA pressure independently from encode thread. */
        ret = h264_stress_dma_pressure_open();
        if (ret != BK_OK) {
            LOGE("h264_stress_dma_pressure_open failed, ret=%d\n", (int)ret);
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }
    } else if (os_strcmp(argv[1], "dma_close") == 0) {
        /* Test command: close DMA pressure independently from encode thread. */
        h264_stress_dma_pressure_close();
        ret = BK_OK;
    } else if (os_strcmp(argv[1], "dump_one_round") == 0) {
        if (s_h264_stress_thread != NULL) {
            LOGE("h264 stress thread already running\n");
            ret = BK_FAIL;
            msg = CLI_CMD_RSP_ERROR;
            goto exit;
        }

        s_h264_one_round_header_cnt = 0U;
        s_h264_one_round_i_cnt = 0U;
        s_h264_one_round_p_cnt = 0U;
        s_h264_one_round_dump_mode = 1U;
        s_h264_stress_stop = 0U;

        ret = rtos_create_thread(&s_h264_stress_thread,
                                 BEKEN_DEFAULT_WORKER_PRIORITY,
                                 "h264_dump_one_round",
                                 (beken_thread_function_t)h264_stress_thread_entry,
                                 H264_STRESS_THREAD_STACK_SIZE,
                                 NULL);
        if (ret != BK_OK) {
            LOGE("create h264 dump thread failed, ret=%d\n", (int)ret);
            s_h264_stress_thread = NULL;
            s_h264_one_round_dump_mode = 0U;
            msg = CLI_CMD_RSP_ERROR;
            ret = BK_FAIL;
            goto exit;
        }
    } else if (os_strcmp(argv[1], "dump_nv12") == 0) {
        uint8_t *dump_buf = NULL;
        frame_buffer_t *tmp_fb = NULL;
        uint32_t irq_flags = 0;

        /* Dump pure red NV12 before encoding. Prefer existing buffers if present. */
        dump_buf = s_h264_yuv_work ? s_h264_yuv_work : s_h264_yuv_src;
        if (dump_buf == NULL) {
            /* Allocate from frame-buffer heap (avoid SRAM allocation failure). */
            uint32_t fb_size = H264_STRESS_YUV_SIZE + (uint32_t)sizeof(frame_buffer_t) + 32U;
            /* Use coded heap for all bk_frame_buffer_malloc allocations. */
            tmp_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, fb_size);
            if (tmp_fb == NULL) {
                LOGE("dump_nv12 bk_frame_buffer_malloc failed, size=%u\n", fb_size);
                ret = BK_FAIL;
                msg = CLI_CMD_RSP_ERROR;
                goto exit;
            }
            os_memset(tmp_fb, 0, sizeof(frame_buffer_t));
            tmp_fb->frame = (uint8_t *)((((uint32_t)(tmp_fb + 1U) >> 5U) + 1U) << 5U);
            tmp_fb->size = H264_STRESS_YUV_SIZE;
            dump_buf = tmp_fb->frame;
            h264_stress_fill_nv12_pure_red(dump_buf);
        }

        /* Disable interrupts before printing/dumping for stability. */
        irq_flags = rtos_enter_critical();
        LOGI("dump_nv12: dumping buffer=%p size=%u\n", dump_buf, H264_STRESS_YUV_SIZE);
        stack_mem_dump((uint32_t)(uintptr_t)dump_buf,
                        (uint32_t)(uintptr_t)(dump_buf + H264_STRESS_YUV_SIZE));
        rtos_exit_critical(irq_flags);
        ret = BK_OK;

        if (tmp_fb != NULL) {
            bk_frame_buffer_free(tmp_fb);
        }
     }
     else {
        LOGE("usage: h264_encode_stress <start [dma_copy_size_kb]|stop|dma_open|dma_close|dump_nv12|dump_one_round>\n");
         ret = BK_FAIL;
         msg = CLI_CMD_RSP_ERROR;
         goto exit;
     }

 exit:
     if (pcWriteBuffer && xWriteBufferLen > 0) {
         int len = os_strlen(msg);
         if (len >= xWriteBufferLen) {
             len = xWriteBufferLen - 1;
         }
         os_memcpy(pcWriteBuffer, msg, len);
         pcWriteBuffer[len] = '\0';
     }
 }

