#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <avdk_error.h>

#include <components/bk_frame_buffer.h>
#include <components/log.h>
#include <common/avdk_pixel_types.h>
#include <lcd/lcd_hx8399c_mipi_1080x1920.h>

#include "h264_decoder_api.h"
#include "encode_frame_que.h"
#include "decode_private.h"

#define TAG "db-decode"

#define LOGI(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
    uint8_t task_running;
    uint8_t flexa_mode;
    uint8_t ring_buffer_cnt;
    bk_image_format_t input_format;
    bk_image_format_t output_format;
    uint16_t width;
    uint16_t height;
    uint16_t aligned_height;
    void *decode_context;
    beken_semaphore_t decode_sem;
    beken_thread_t decode_thread;
    uint8_t *decode_buffer;
    uint8_t *yuv_frame;
    struct {
        void (*cb)(uint32_t wr_cnt, void *arg);
        void *arg;
    } isr_callbacks[4];

    uint8_t port_enabled[DECODE_TEST_PORT_MAX];
    uint32_t port_rd_cnt[DECODE_TEST_PORT_MAX];
    uint8_t *table_buffer;
    uint32_t flexa_size;

    /* NV12 full-frame output (assembled from Flexa line blocks). */
    uint8_t enable_nv12_output;
    uint8_t *nv12_buffers[2];
    uint8_t nv12_in_use[2];
    uint8_t nv12_assemble_index;
    volatile uint8_t nv12_frame_ready;
    uint8_t nv12_frame_ready_index;
    decode_test_nv12_frame_cb_t nv12_frame_cb;
    void *nv12_frame_cb_arg;
} app_decoder_config_t;

static app_decoder_config_t *s_decoder_config = NULL;
static void decode_test_flexa_done_callback(uint32_t wr_cnt);

static uint32_t decode_test_min_port_rd_cnt(app_decoder_config_t *decoder_config)
{
    uint32_t min_rd = 0xFFFFFFFFU;
    uint8_t enabled = 0;

    for (uint32_t i = 0; i < (uint32_t)DECODE_TEST_PORT_MAX; i++) {
        if (decoder_config->port_enabled[i]) {
            enabled = 1;
            if (decoder_config->port_rd_cnt[i] < min_rd) {
                min_rd = decoder_config->port_rd_cnt[i];
            }
        }
    }

    if (!enabled) {
        return 0xFFFFFFFFU;
    }

    return min_rd;
}

avdk_err_t decode_test_set_port_rd_cnt(decode_test_port_t port, uint32_t rd_cnt)
{
    app_decoder_config_t *decoder_config = s_decoder_config;
    if (decoder_config == NULL) {
        LOGE("%s, %d, decode config not initialized\n", __func__, __LINE__);
        return AVDK_ERR_UNKNOWN;
    }

    if (port >= DECODE_TEST_PORT_MAX) {
        LOGE("%s, %d, invalid port:%d\n", __func__, __LINE__, (int)port);
        return AVDK_ERR_INVAL;
    }

    uint32_t min_rd = 0xFFFFFFFFU;
    uint32_t flags = rtos_enter_critical();
    decoder_config->port_enabled[port] = 1;
    decoder_config->port_rd_cnt[port] = rd_cnt;
    min_rd = decode_test_min_port_rd_cnt(decoder_config);
    rtos_exit_critical(flags);

    if (min_rd != 0xFFFFFFFFU) {
        extern void ppRbReadPointerSet(uint32_t value);
        ppRbReadPointerSet(min_rd);
    }

    return AVDK_ERR_OK;
}

/*
 * Align allocation for decoder HW/DMA requirements.
 * rtos/os allocators do not guarantee 64-byte alignment, so we wrap hsram_malloc()
 * and store the original pointer right before the aligned address for correct free.
 */
static void *hsram_aligned_malloc(uint32_t alignment, uint32_t size)
{
    if (alignment < (uint32_t)sizeof(void *)) {
        alignment = (uint32_t)sizeof(void *);
    }

    /* alignment must be power of two */
    if ((alignment & (alignment - 1U)) != 0U) {
        return NULL;
    }

    uint32_t total = size + alignment - 1U + (uint32_t)sizeof(void *);
    void *raw = hsram_malloc(total);
    if (raw == NULL) {
        return NULL;
    }

    uintptr_t start = (uintptr_t)raw + sizeof(void *);
    uintptr_t aligned = (start + (alignment - 1U)) & ~((uintptr_t)alignment - 1U);
    ((void **)aligned)[-1] = raw;

    return (void *)aligned;
}

static void hsram_aligned_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }

    void *raw = ((void **)ptr)[-1];
    os_free(raw);
}

static void SaveStream(uint8_t* y, uint8_t* cb, uint8_t* cr, uint32_t width, uint32_t height, uint32_t type)
{
    //LOGD("%s(%p, %p, %p, %d, %d, %d)\n", __func__, y, cb, cr, width, height, type);
}

avdk_err_t decode_test_register_nv12_frame_callback(decode_test_nv12_frame_cb_t cb, void *arg)
{
    app_decoder_config_t *decoder_config = s_decoder_config;
    if (decoder_config == NULL) {
        LOGE("%s, %d, decode config not initialized\n", __func__, __LINE__);
        return AVDK_ERR_UNKNOWN;
    }

    decoder_config->nv12_frame_cb = cb;
    decoder_config->nv12_frame_cb_arg = arg;
    return AVDK_ERR_OK;
}

avdk_err_t decode_test_release_nv12_frame_buffer(uint8_t buffer_index)
{
    app_decoder_config_t *decoder_config = s_decoder_config;
    if (decoder_config == NULL) {
        LOGE("%s, %d, decode config not initialized\n", __func__, __LINE__);
        return AVDK_ERR_UNKNOWN;
    }

    if (buffer_index >= 2) {
        LOGE("%s, %d, invalid buffer_index:%d\n", __func__, __LINE__, buffer_index);
        return AVDK_ERR_INVAL;
    }

    decoder_config->nv12_in_use[buffer_index] = 0;
    return AVDK_ERR_OK;
}

static void app_decode_thread_entry(void *arg)
{
    LOGD("%s, %d, decode thread entry\n", __func__, __LINE__);
    avdk_err_t ret = AVDK_ERR_OK;
    app_decoder_config_t *decoder_config = (app_decoder_config_t *)arg;
    frame_buffer_t *encode_buffer = NULL;
    uint8_t *decode_buffer = NULL;
    uint32_t decode_buffer_size = 0;
    uint32_t frame_size = 0;
    decoder_config->task_running = 1;
    rtos_set_semaphore(&decoder_config->decode_sem);
    while (decoder_config->task_running) {

        // GPIO_UP(2);
        if (encode_buffer == NULL) {
            encode_buffer = encode_ready_frame_que_pop(2000); // 2000ms timeout
            if (encode_buffer == NULL) {
                LOGE("%s, %d, get encode buffer from queue failed\n", __func__, __LINE__);
                continue;
            }
        }

        //LOGD("%s, %d, frame:%p, encode frame:%p, length:%d\n", __func__, __LINE__, encode_buffer, encode_buffer->frame, encode_buffer->length);
        // GPIO_UP(3);

        // temp code for flexa mode
        if (decoder_config->flexa_mode == false || DECODE_DUMP_FRAME_ENABLE) {
            uint32_t aligned_height = (decoder_config->height + DECODE_FLEXA_ALIGN_SIZE - 1) & ~(DECODE_FLEXA_ALIGN_SIZE - 1);
            frame_size = decoder_config->width * aligned_height * 3 / 2;
            if (decoder_config->yuv_frame == NULL) {
                decoder_config->yuv_frame = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
                if (decoder_config->yuv_frame == NULL) {
                    LOGE("%s, %d, malloc yuv frame failed\n", __func__, __LINE__);
                    continue;
                }
            }
        }

        if (decoder_config->flexa_mode == false) {
            decode_buffer_size = frame_size;
            decode_buffer = decoder_config->yuv_frame;
        }
        else {
            // flexa mode
            decode_buffer_size = decoder_config->width * DECODE_FLEXA_LINES * 3 / 2 * decoder_config->ring_buffer_cnt;
            decode_buffer = decoder_config->decode_buffer;
        }

        // GPIO_UP(4);
        if (decoder_config->input_format == BK_IMAGE_FORMAT_MJPEG) {
            ret = jpeg_decoder_decode(decoder_config->decode_context, encode_buffer->frame, encode_buffer->length, decode_buffer, decode_buffer_size);
            if (ret == 1) {
                ret = AVDK_ERR_OK;
            }
        } else if (decoder_config->input_format == BK_IMAGE_FORMAT_H264) {
            ret = h264_decoder_decode(decoder_config->decode_context, encode_buffer->frame, encode_buffer->length, decode_buffer, &decode_buffer_size);
        } else {
            ret = AVDK_ERR_INVAL;
        }
        // GPIO_DOWN(4);

#if DECODE_DUMP_FRAME_ENABLE
        if(ret == AVDK_ERR_OK) {
            extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
            stack_mem_dump((uint32_t)encode_buffer->frame, (uint32_t)encode_buffer->frame + encode_buffer->length);
            if (decoder_config->yuv_frame != NULL) {
                stack_mem_dump((uint32_t)decoder_config->yuv_frame, (uint32_t)decoder_config->yuv_frame + frame_size);
            }
        }
#endif

        if (decoder_config->flexa_mode == false) { // non-flexa mode
            if (ret == AVDK_ERR_OK) {
                LOGD("%s, %d, decode success, TODO FIX: send to display\n", __func__, __LINE__);
                bk_frame_buffer_free(decoder_config->yuv_frame);
            }
            else {
                LOGD("%s, %d, decode failed, ret: %d\n", __func__, __LINE__, ret);
                bk_frame_buffer_free(decoder_config->yuv_frame);
            }
        }

        /*
         * NV12 full-frame output: when Flexa decode finishes a frame, flexa_done_callback
         * sets nv12_frame_ready and writes into current assemble buffer.
         * Invoke user callback here (decode thread context) and rotate assemble buffer
         * only when the other buffer is free.
         */
        if (decoder_config->enable_nv12_output && decoder_config->flexa_mode && decoder_config->nv12_frame_ready) {
            uint8_t done_idx = decoder_config->nv12_frame_ready_index;
            uint8_t next_idx = 1U - done_idx;

            decoder_config->nv12_frame_ready = 0;

            if (decoder_config->nv12_in_use[next_idx]) {
                /* No free buffer for next frame: skip NV12 output for this frame. */
                LOGW("%s, %d, nv12 output drop: next buffer in use\n", __func__, __LINE__);
            } else if (decoder_config->nv12_frame_cb) {
                decoder_config->nv12_frame_cb(decoder_config->nv12_frame_cb_arg,
                    decoder_config->nv12_buffers[done_idx],
                    decoder_config->width,
                    decoder_config->aligned_height);
            }
        }

        // GPIO_DOWN(3);

        encode_free_frame_que_push(encode_buffer);
        decoder_config->yuv_frame = NULL;
        decode_buffer = NULL;
        encode_buffer = NULL;
        // GPIO_DOWN(2);
    }

    if (decoder_config->yuv_frame) {
        bk_frame_buffer_free(decoder_config->yuv_frame);
        decoder_config->yuv_frame = NULL;
    }

    if (encode_buffer) {
        encode_free_frame_que_push(encode_buffer);
        encode_buffer = NULL;
    }

    decoder_config->decode_thread = NULL;
    rtos_set_semaphore(&decoder_config->decode_sem);
    rtos_delete_thread(NULL);
}

static void decode_test_flexa_done_callback(uint32_t wr_cnt)
{
    app_decoder_config_t *decoder_config = s_decoder_config;
    if (decoder_config == NULL) {
        LOGE("%s, %d, decoder config not initialized\n", __func__, __LINE__);
        return;
    }
    // GPIO_UP(30);

    /*
     * Decoder Flexa write counter is per-frame and starts from 1.
     * Reset per-frame port read counters at frame start to avoid carrying
     * stale rd_cnt from the previous frame.
     */
    if (decoder_config->flexa_mode && wr_cnt == 1U) {
        uint32_t flags = rtos_enter_critical();
        for (uint32_t i = 0; i < (uint32_t)DECODE_TEST_PORT_MAX; i++) {
            if (decoder_config->port_enabled[i]) {
                decoder_config->port_rd_cnt[i] = 0;
            }
        }
        rtos_exit_critical(flags);

        extern void ppRbReadPointerSet(uint32_t value);
        ppRbReadPointerSet(0);
    }

    /*
     * Assemble NV12 full frame from Flexa ring buffers.
     * Copy per 16-line block: Y (width*16) + UV (width*8).
     *
     * NOTE: This runs in decoder callback context. Keep the logic simple.
     */
    if (decoder_config->enable_nv12_output && decoder_config->flexa_mode && decoder_config->nv12_buffers[decoder_config->nv12_assemble_index]) {
        uint32_t y_flexa_length = (uint32_t)decoder_config->width * DECODE_FLEXA_LINES;
        uint32_t uv_flexa_length = (uint32_t)decoder_config->width * DECODE_FLEXA_LINES / 2;
        uint8_t ring_cnt = decoder_config->ring_buffer_cnt ? decoder_config->ring_buffer_cnt : DECODE_BUFFER_CNT;

        uint32_t tail = (wr_cnt - 1U) % ring_cnt;
        uint8_t *y_src = decoder_config->decode_buffer + tail * y_flexa_length;
        uint8_t *uv_src = decoder_config->decode_buffer + y_flexa_length * ring_cnt + tail * uv_flexa_length;

        uint8_t *dst = decoder_config->nv12_buffers[decoder_config->nv12_assemble_index];
        uint8_t *y_dst = dst + (wr_cnt - 1U) * y_flexa_length;
        uint8_t *uv_dst = dst + (uint32_t)decoder_config->width * decoder_config->aligned_height + (wr_cnt - 1U) * uv_flexa_length;

        os_memcpy(y_dst, y_src, y_flexa_length);
        os_memcpy(uv_dst, uv_src, uv_flexa_length);

        uint32_t blocks_per_frame = decoder_config->aligned_height / DECODE_FLEXA_LINES;
        if (wr_cnt == blocks_per_frame) {
            decoder_config->nv12_frame_ready_index = decoder_config->nv12_assemble_index;
            decoder_config->nv12_frame_ready = 1;
        }
    }

    uint8_t called = 0;
    uint32_t max = (uint32_t)(sizeof(decoder_config->isr_callbacks) / sizeof(decoder_config->isr_callbacks[0]));
    for (uint32_t i = 0; i < max; i++) {
        if (decoder_config->isr_callbacks[i].cb) {
            called = 1;
            decoder_config->isr_callbacks[i].cb(wr_cnt, decoder_config->isr_callbacks[i].arg);
        }
    }

    if (!called) {
        extern void ppRbReadPointerSet(uint32_t value);
        ppRbReadPointerSet(wr_cnt);
    }
    // GPIO_DOWN(30);
}

avdk_err_t decode_test_open(uint16_t width,
                            uint16_t height,
                            bk_image_format_t format,
                            uint8_t flexa_mode,
                            uint8_t enable_nv12_output)
{
    avdk_err_t ret = AVDK_ERR_OK;

    app_decoder_config_t *decoder_config = s_decoder_config;

    if (decoder_config != NULL) {
        LOGE("%s, %d, decoder config already initialized\n", __func__, __LINE__);
        return AVDK_ERR_OK;
    }

    ret = encode_frame_que_init();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init encode frame queue failed\n", __func__, __LINE__);
        return ret;
    }

    decoder_config = (app_decoder_config_t *)os_malloc(sizeof(app_decoder_config_t));
    if (decoder_config == NULL) {
        LOGE("%s, %d, malloc decoder config failed\n", __func__, __LINE__);
        return AVDK_ERR_NOMEM;
    }

    os_memset(decoder_config, 0, sizeof(app_decoder_config_t));

    decoder_config->flexa_mode = flexa_mode;
    decoder_config->input_format = format;
    decoder_config->output_format = BK_PIXEL_FORMAT_NV12;
    decoder_config->width = width;
    decoder_config->height = height;
    decoder_config->aligned_height = (height + DECODE_FLEXA_ALIGN_SIZE - 1) & ~(DECODE_FLEXA_ALIGN_SIZE - 1);
    decoder_config->decode_sem = NULL;
    decoder_config->decode_thread = NULL;

    for (uint32_t i = 0; i < (uint32_t)DECODE_TEST_PORT_MAX; i++) {
        decoder_config->port_enabled[i] = 0;
        decoder_config->port_rd_cnt[i] = 0;
    }

    for (uint32_t i = 0; i < (uint32_t)(sizeof(decoder_config->isr_callbacks) / sizeof(decoder_config->isr_callbacks[0])); i++) {
        decoder_config->isr_callbacks[i].cb = NULL;
        decoder_config->isr_callbacks[i].arg = NULL;
    }

    decoder_config->enable_nv12_output = enable_nv12_output ? 1 : 0;
    decoder_config->nv12_frame_cb = NULL;
    decoder_config->nv12_frame_cb_arg = NULL;
    decoder_config->nv12_assemble_index = 0;
    decoder_config->nv12_frame_ready = 0;
    decoder_config->nv12_frame_ready_index = 0;
    decoder_config->nv12_in_use[0] = 0;
    decoder_config->nv12_in_use[1] = 0;
    decoder_config->nv12_buffers[0] = NULL;
    decoder_config->nv12_buffers[1] = NULL;

    if (flexa_mode) { // flexa mode
        decoder_config->ring_buffer_cnt = DECODE_BUFFER_CNT;
        decoder_config->flexa_size = width * DECODE_FLEXA_LINES * 3 / 2 * DECODE_BUFFER_CNT;
        decoder_config->decode_buffer = (uint8_t *)hsram_aligned_malloc(64, decoder_config->flexa_size);
        if (decoder_config->decode_buffer == NULL) {
            LOGE("%s, %d, malloc 64-byte aligned decode buffer:%dbytes failed\n", __func__, __LINE__, decoder_config->flexa_size);
            return AVDK_ERR_NOMEM;
        }

        LOGD("%s, %d, decode buffer:%p, size:%d\n", __func__, __LINE__, decoder_config->decode_buffer, decoder_config->flexa_size);
    }

    if (decoder_config->enable_nv12_output && flexa_mode) {
        uint32_t frame_size = (uint32_t)decoder_config->width * decoder_config->aligned_height * 3 / 2;
        decoder_config->nv12_buffers[0] = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
        decoder_config->nv12_buffers[1] = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
        if (decoder_config->nv12_buffers[0] == NULL || decoder_config->nv12_buffers[1] == NULL) {
            LOGE("%s, %d, malloc nv12 buffers failed, size:%d\n", __func__, __LINE__, frame_size);
            ret = AVDK_ERR_NOMEM;
            goto out;
        }
        os_memset(decoder_config->nv12_buffers[0], 0, frame_size);
        os_memset(decoder_config->nv12_buffers[1], 0, frame_size);
    }

    if (format == BK_IMAGE_FORMAT_MJPEG) {
        ret = jpeg_decoder_init(&decoder_config->decode_context, flexa_mode, decode_test_flexa_done_callback, NULL, &SaveStream);
    } else if (format == BK_IMAGE_FORMAT_H264) {
        ret = h264_decoder_init(&decoder_config->decode_context, flexa_mode, decode_test_flexa_done_callback, NULL, &SaveStream);
    } else {
        ret = AVDK_ERR_INVAL;
    }

    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init decoder context failed\n", __func__, __LINE__);
        goto out;
    }

    ret = rtos_init_semaphore(&decoder_config->decode_sem, 1);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, init decode sem failed\n", __func__, __LINE__);
        goto out;
    }

    ret = rtos_create_thread(&decoder_config->decode_thread,
                            BEKEN_DEFAULT_WORKER_PRIORITY,
                            "decode_thread",
                            (beken_thread_function_t)app_decode_thread_entry,
                            1024 * 4, // 4KB
                            decoder_config);
    if (ret != AVDK_ERR_OK) {
        LOGE("%s, %d, create decode thread failed\n", __func__, __LINE__);
        goto out;
    }

    rtos_get_semaphore(&decoder_config->decode_sem, BEKEN_WAIT_FOREVER);

    s_decoder_config = decoder_config;

    LOGD("%s, %d, decode turn on complete\n", __func__, __LINE__);

    return ret;

out:
    if (decoder_config) {
        if (decoder_config->decode_context) {
            if (decoder_config->input_format == BK_IMAGE_FORMAT_MJPEG) {
                jpeg_decoder_deinit(decoder_config->decode_context);
            } else if (decoder_config->input_format == BK_IMAGE_FORMAT_H264) {
                h264_decoder_deinit(decoder_config->decode_context);
            }
            decoder_config->decode_context = NULL;
        }

        if (decoder_config->decode_sem) {
            rtos_deinit_semaphore(&decoder_config->decode_sem);
        }

        if (decoder_config->nv12_buffers[0]) {
            bk_frame_buffer_free(decoder_config->nv12_buffers[0]);
            decoder_config->nv12_buffers[0] = NULL;
        }
        if (decoder_config->nv12_buffers[1]) {
            bk_frame_buffer_free(decoder_config->nv12_buffers[1]);
            decoder_config->nv12_buffers[1] = NULL;
        }

        if (decoder_config->flexa_mode && decoder_config->decode_buffer) {
            hsram_aligned_free(decoder_config->decode_buffer);
            decoder_config->decode_buffer = NULL;
        }

        os_free(decoder_config);
        s_decoder_config = NULL;
    }

    return ret;
}

avdk_err_t decode_test_close(void)
{
    app_decoder_config_t *decoder_config = s_decoder_config;

    if (decoder_config == NULL) {
        LOGE("%s, %d, decode context not initialized\n", __func__, __LINE__);
        return AVDK_ERR_OK;
    }

    LOGD("%s, %d, decode turn off start\n", __func__, decoder_config->task_running);

    decoder_config->task_running = 0;
    rtos_get_semaphore(&decoder_config->decode_sem, BEKEN_WAIT_FOREVER);

    if (decoder_config->input_format == BK_IMAGE_FORMAT_MJPEG) {
        jpeg_decoder_deinit(decoder_config->decode_context);
    } else if (decoder_config->input_format == BK_IMAGE_FORMAT_H264) {
        h264_decoder_deinit(decoder_config->decode_context);
    }
    decoder_config->decode_context = NULL;

    if (decoder_config->decode_sem) {
        rtos_deinit_semaphore(&decoder_config->decode_sem);
    }

    if (decoder_config->nv12_buffers[0]) {
        bk_frame_buffer_free(decoder_config->nv12_buffers[0]);
        decoder_config->nv12_buffers[0] = NULL;
    }
    if (decoder_config->nv12_buffers[1]) {
        bk_frame_buffer_free(decoder_config->nv12_buffers[1]);
        decoder_config->nv12_buffers[1] = NULL;
    }

    if (decoder_config->flexa_mode && decoder_config->decode_buffer) {
        hsram_aligned_free(decoder_config->decode_buffer);
        decoder_config->decode_buffer = NULL;
    }

    os_free(decoder_config);
    s_decoder_config = NULL;

    LOGD("%s, %d, decode turn off complete\n", __func__, __LINE__);
    return AVDK_ERR_OK;
}

avdk_err_t decode_test_get_decode_context(uint8_t **decode_buffer, uint8_t *ring_buffer_cnt)
{
    app_decoder_config_t *decoder_config = s_decoder_config;
    if (decoder_config == NULL) {
        LOGE("%s, %d, decode config not initialized\n", __func__, __LINE__);
        return AVDK_ERR_UNKNOWN;
    }
    *decode_buffer = decoder_config->decode_buffer;
    *ring_buffer_cnt = decoder_config->ring_buffer_cnt;
    return AVDK_ERR_OK;
}

avdk_err_t decode_test_register_isr_callback(void (*callback)(uint32_t wr_cnt, void *arg), void *arg)
{
    app_decoder_config_t *decoder_config = s_decoder_config;
    if (decoder_config == NULL) {
        LOGE("%s, %d, decode config not initialized\n", __func__, __LINE__);
        return AVDK_ERR_UNKNOWN;
    }

    if (callback == NULL) {
        LOGE("%s, %d, callback is NULL\n", __func__, __LINE__);
        return AVDK_ERR_INVAL;
    }

    uint32_t max = (uint32_t)(sizeof(decoder_config->isr_callbacks) / sizeof(decoder_config->isr_callbacks[0]));

    for (uint32_t i = 0; i < max; i++) {
        if (decoder_config->isr_callbacks[i].cb == callback && decoder_config->isr_callbacks[i].arg == arg) {
            return AVDK_ERR_OK;
        }
    }

    for (uint32_t i = 0; i < max; i++) {
        if (decoder_config->isr_callbacks[i].cb == NULL) {
            decoder_config->isr_callbacks[i].cb = callback;
            decoder_config->isr_callbacks[i].arg = arg;
            return AVDK_ERR_OK;
        }
    }

    LOGE("%s, %d, isr callback list full\n", __func__, __LINE__);
    return AVDK_ERR_BUSY;
}

avdk_err_t decode_test_set_wr_cnt(uint32_t wr_cnt)
{
    return decode_test_set_port_rd_cnt(DECODE_TEST_PORT_GPU, wr_cnt);
}

void cli_decode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2)
    {
        LOGE("Usage: decode <command>\n");
        return;
    }

    avdk_err_t ret = AVDK_ERR_OK;

    if (os_strcmp(argv[1], "open") == 0)
    {
        if(argc == 3 && os_strcmp(argv[2], "frame") == 0) {
            ret = decode_test_open(1920, 1080, BK_IMAGE_FORMAT_MJPEG, 0, 0);
        }
        else {
            ret = decode_test_open(1920, 1080, BK_IMAGE_FORMAT_MJPEG, 1, 0);
        }
    }
    else if (os_strcmp(argv[1], "close") == 0)
    {
        ret = decode_test_close();
    }
    else
    {
        LOGE("Usage: decode <command>\n");
        return;
    }

    if (ret != AVDK_ERR_OK)
    {
        LOGE("Failed to %s decode, ret=%d\n", __func__, ret);
    } else {
        LOGI("Successfully %s decode\n", __func__);
    }
}