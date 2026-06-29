#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <avdk_error.h>

#include <components/bk_frame_buffer.h>
#include <common/avdk_pixel_types.h>
#include <components/bk_decode/bk_jpeg_decode_ctlr.h>
#include <components/bk_decode/bk_jpeg_decode_types.h>

#include "encode_frame_que.h"
#include "decode_test.h"

#define TAG "decode_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
    uint8_t task_running;
    uint8_t flexa_mode;
    uint8_t ring_buffer_cnt;
    bk_image_format_t input_format;
    bk_pixel_format_t output_format;
    uint16_t width;
    uint16_t height;
    uint16_t aligned_height;
    bk_jpeg_decode_ctlr_handle_t decode_handle;
    beken_semaphore_t decode_sem;
    beken_thread_t decode_thread;
    uint8_t *decode_buffer;
} decode_test_ctx_t;

static decode_test_ctx_t *s_decode_ctx = NULL;

static void *hsram_aligned_malloc(uint32_t alignment, uint32_t size)
{
    if (alignment < (uint32_t)sizeof(void *)) {
        alignment = (uint32_t)sizeof(void *);
    }
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
    os_free(((void **)ptr)[-1]);
}

static void decode_jpeg_frame_done_cb(int status, void *args)
{
    (void)args;
    if (status != BK_OK) {
        LOGE("%s decode failed, status=%d\n", __func__, status);
    }
}

static void decode_flexa_done_cb(uint32_t wr_cnt, void *args)
{
    (void)wr_cnt;
    (void)args;
}

static void decode_thread_entry(void *arg)
{
    decode_test_ctx_t *ctx = (decode_test_ctx_t *)arg;
    frame_buffer_t *encode_buffer = NULL;

    ctx->task_running = 1;
    rtos_set_semaphore(&ctx->decode_sem);

    while (ctx->task_running) {
        if (encode_buffer == NULL) {
            encode_buffer = encode_ready_frame_que_pop(2000);
            if (encode_buffer == NULL) {
                continue;
            }
        }

        uint32_t decode_buffer_size;
        uint8_t *decode_buffer;

        if (ctx->flexa_mode) {
            decode_buffer_size = bk_image_size_get(ctx->width,
                                                   DECODE_FLEXA_LINES * ctx->ring_buffer_cnt,
                                                   ctx->output_format);
            decode_buffer = ctx->decode_buffer;
        } else {
            decode_buffer_size = bk_image_size_get(ctx->width, ctx->aligned_height, ctx->output_format);
            decode_buffer = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, decode_buffer_size);
            if (decode_buffer == NULL) {
                LOGE("%s malloc decode buffer failed\n", __func__);
                encode_free_frame_que_push(encode_buffer);
                encode_buffer = NULL;
                continue;
            }
        }

        avdk_err_t ret = AVDK_ERR_INVAL;
        if (ctx->input_format == BK_IMAGE_FORMAT_MJPEG) {
            bk_jpeg_decode_input_t in = {0};
            in.stream = encode_buffer->frame;
            in.stream_len = encode_buffer->length;
            in.out_buffer = decode_buffer;
            in.out_buffer_size = decode_buffer_size;
            ret = bk_jpeg_decode_frame(ctx->decode_handle, &in);
        }

        if (!ctx->flexa_mode && decode_buffer != NULL) {
            bk_frame_buffer_free(decode_buffer);
        }
        if (ret != AVDK_ERR_OK) {
            LOGD("%s decode failed, ret=%d\n", __func__, ret);
        }

        encode_free_frame_que_push(encode_buffer);
        encode_buffer = NULL;
    }

    if (encode_buffer != NULL) {
        encode_free_frame_que_push(encode_buffer);
    }

    ctx->decode_thread = NULL;
    rtos_set_semaphore(&ctx->decode_sem);
    rtos_delete_thread(NULL);
}

static void decode_test_destroy_ctx(decode_test_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->decode_handle != NULL) {
        (void)bk_jpeg_decode_close(ctx->decode_handle);
        (void)bk_jpeg_decode_deinit(ctx->decode_handle);
        (void)bk_jpeg_decode_delete(ctx->decode_handle);
        ctx->decode_handle = NULL;
    }

    if (ctx->decode_sem != NULL) {
        rtos_deinit_semaphore(&ctx->decode_sem);
        ctx->decode_sem = NULL;
    }

    if (ctx->flexa_mode && ctx->decode_buffer != NULL) {
        hsram_aligned_free(ctx->decode_buffer);
        ctx->decode_buffer = NULL;
    }

    os_free(ctx);
}

avdk_err_t decode_test_open(uint16_t width, uint16_t height, bk_image_format_t format, uint8_t flexa_mode)
{
    if (s_decode_ctx != NULL) {
        LOGW("%s already open\n", __func__);
        return AVDK_ERR_OK;
    }

    avdk_err_t ret = encode_frame_que_init();
    if (ret != AVDK_ERR_OK) {
        LOGE("%s encode_frame_que_init failed\n", __func__);
        return ret;
    }

    decode_test_ctx_t *ctx = (decode_test_ctx_t *)os_malloc(sizeof(decode_test_ctx_t));
    if (ctx == NULL) {
        return AVDK_ERR_NOMEM;
    }
    os_memset(ctx, 0, sizeof(decode_test_ctx_t));

    ctx->flexa_mode = flexa_mode;
    ctx->input_format = format;
    ctx->output_format = BK_PIXEL_FORMAT_NV12;
    ctx->width = width;
    ctx->height = height;
    ctx->aligned_height = (height + DECODE_FLEXA_ALIGN_SIZE - 1) & ~(DECODE_FLEXA_ALIGN_SIZE - 1);
    ctx->ring_buffer_cnt = DECODE_BUFFER_CNT;

    if (flexa_mode) {
        uint32_t flexa_size = bk_image_size_get(width,
                                                DECODE_FLEXA_LINES * ctx->ring_buffer_cnt,
                                                ctx->output_format);
        ctx->decode_buffer = (uint8_t *)hsram_aligned_malloc(64, flexa_size);
        if (ctx->decode_buffer == NULL) {
            LOGE("%s alloc flexa buffer %u failed\n", __func__, flexa_size);
            ret = AVDK_ERR_NOMEM;
            goto out;
        }
        LOGD("%s flexa buffer %p size %u\n", __func__, ctx->decode_buffer, flexa_size);
    }

    if (format == BK_IMAGE_FORMAT_MJPEG) {
        bk_jpeg_decode_flexa_config_t cfg = DEFAULT_JPEG_DECODE_FLEXA_CONFIG;
        cfg.frame_done_cb = decode_jpeg_frame_done_cb;
        cfg.frame_done_args = NULL;
        cfg.flexa_done_cb = decode_flexa_done_cb;
        cfg.flexa_done_args = NULL;
        cfg.out_width = width;
        cfg.out_height = height;
        cfg.segment_height = (uint16_t)(DECODE_FLEXA_LINES / 16);
        cfg.segment_number = DECODE_BUFFER_CNT;

        ret = bk_jpeg_decode_flexa_ctlr_new(&ctx->decode_handle, &cfg);
        if (ret != AVDK_ERR_OK) {
            goto out;
        }
        ret = bk_jpeg_decode_init(ctx->decode_handle);
        if (ret != AVDK_ERR_OK) {
            goto out;
        }
        ret = bk_jpeg_decode_open(ctx->decode_handle);
        if (ret != AVDK_ERR_OK) {
            goto out;
        }
    } else {
        ret = AVDK_ERR_INVAL;
        goto out;
    }

    ret = rtos_init_semaphore(&ctx->decode_sem, 1);
    if (ret != AVDK_ERR_OK) {
        goto out;
    }

    ret = rtos_create_thread(&ctx->decode_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "decode_thread",
                             (beken_thread_function_t)decode_thread_entry,
                             1024 * 4,
                             ctx);
    if (ret != AVDK_ERR_OK) {
        goto out;
    }

    rtos_get_semaphore(&ctx->decode_sem, BEKEN_WAIT_FOREVER);
    s_decode_ctx = ctx;
    LOGI("%s %ux%u flexa=%u ok\n", __func__, width, height, flexa_mode);
    return AVDK_ERR_OK;

out:
    decode_test_destroy_ctx(ctx);
    return ret;
}

avdk_err_t decode_test_close(void)
{
    decode_test_ctx_t *ctx = s_decode_ctx;
    if (ctx == NULL) {
        return AVDK_ERR_OK;
    }

    ctx->task_running = 0;
    rtos_get_semaphore(&ctx->decode_sem, BEKEN_WAIT_FOREVER);

    decode_test_destroy_ctx(ctx);
    s_decode_ctx = NULL;
    LOGI("%s done\n", __func__);
    return AVDK_ERR_OK;
}

avdk_err_t decode_test_get_flexa_context(uint8_t **decode_buffer, uint8_t *ring_buffer_cnt)
{
    if (s_decode_ctx == NULL || decode_buffer == NULL || ring_buffer_cnt == NULL) {
        return AVDK_ERR_INVAL;
    }
    *decode_buffer = s_decode_ctx->decode_buffer;
    *ring_buffer_cnt = s_decode_ctx->ring_buffer_cnt;
    return AVDK_ERR_OK;
}

avdk_err_t decode_test_get_handle(bk_jpeg_decode_ctlr_handle_t *handle)
{
    if (s_decode_ctx == NULL || handle == NULL) {
        return AVDK_ERR_INVAL;
    }
    *handle = s_decode_ctx->decode_handle;
    return AVDK_ERR_OK;
}

bool decode_test_is_open(void)
{
    return s_decode_ctx != NULL;
}

void cli_decode_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2) {
        LOGE("Usage: decode open|close\n");
        return;
    }

    if (os_strcmp(argv[1], "open") == 0) {
        if (decode_test_open(1920, 1080, BK_IMAGE_FORMAT_MJPEG, 1) != AVDK_ERR_OK) {
            LOGE("decode open failed\n");
        }
    } else if (os_strcmp(argv[1], "close") == 0) {
        if (decode_test_close() != AVDK_ERR_OK) {
            LOGE("decode close failed\n");
        }
    } else {
        LOGE("Usage: decode open|close\n");
    }
}
