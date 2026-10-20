#include <stdint.h>
#include "os/os.h"
#include "os/mem.h"
#include <components/log.h>
#include <components/bk_frame_buffer.h>
#include <components/bk_encode/bk_h264_encode_ctlr.h>
#include "bk_flexa_bond_types.h"
#include "h264_encode_test.h"
#include "../common/h264_encode_stream_256x128.h"

#define TAG "vcenc_h264_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define VCENC_H264_TEST_WIDTH          (256U)
#define VCENC_H264_TEST_HEIGHT         (128U)
#define VCENC_H264_TEST_FLEXA_LINES    (16U)
#define VCENC_H264_TEST_FLEXA_CNT      (VCENC_H264_TEST_HEIGHT / VCENC_H264_TEST_FLEXA_LINES)
#define VCENC_H264_TEST_TIMEOUT_MS     (3000U)
#define VCENC_H264_TEST_GOP            (15U)
#define VCENC_H264_TEST_FRAME_CNT      (30U)

static beken_thread_t s_vcenc_h264_boot_demo_thread = NULL;
static volatile uint8_t s_vcenc_h264_boot_demo_running = 0;

#define VCENC_H264_BOOT_DEMO_TASK_PRIORITY   (BEKEN_DEFAULT_WORKER_PRIORITY)
#define VCENC_H264_BOOT_DEMO_TASK_STACK_SIZE (1024 * 16)

typedef struct
{
    beken_semaphore_t done_sem;
    bk_h264_encode_ctlr_handle_t handle;
    uint32_t result;
    uint32_t frame_size;
    uint32_t frame_type;
    uint32_t flexa_wr_blocks;
} vcenc_h264_test_ctx_t;

static void vcenc_h264_log_test_result(const char *case_name, uint8_t pass,
                                       const char *stage, int ret,
                                       uint32_t done_frames,
                                       uint32_t frame_size, uint32_t frame_type)
{
    if (pass) {
        LOGI("[RESULT][PASS] %s success, frames=%u/%u encoded_size=%u frame_type=%u\r\n",
             case_name, done_frames, VCENC_H264_TEST_FRAME_CNT, frame_size, frame_type);
    } else {
        LOGE("[RESULT][FAIL] %s failed at %s, ret=%d, frames=%u/%u encoded_size=%u frame_type=%u\r\n",
             case_name, stage, ret, done_frames, VCENC_H264_TEST_FRAME_CNT, frame_size, frame_type);
    }
}

static void *vcenc_h264_test_outbuf_malloc(uint32_t size, void *args)
{
    (void)args;

    uint32_t frame_size = ((sizeof(frame_buffer_t) + size + 63) >> 6) << 6;
    frame_buffer_t *frame = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED,
                                                                    frame_size);
    if (frame == NULL) {
        LOGE("output frame malloc failed, size=%u\r\n", size);
        return NULL;
    }

    frame->frame = ((uint8_t *)frame) + (((sizeof(frame_buffer_t) + 63) >> 6) << 6);
    frame->size = size;
    frame->length = 0;
    frame->width = VCENC_H264_TEST_WIDTH;
    frame->height = VCENC_H264_TEST_HEIGHT;
    // LOGI("output buffer ready, frame=%p payload=%p size=%u\r\n", frame, frame->frame, size);
    return frame->frame;
}

static uint32_t vcenc_h264_test_outbuf_complete(bk_h264_encode_outbuf_info_t *info)
{
    if (info == NULL) {
        return BK_FAIL;
    }

    vcenc_h264_test_ctx_t *ctx = (vcenc_h264_test_ctx_t *)info->args;

    if (ctx != NULL) {
        ctx->result = info->status;
        ctx->frame_size = info->length;
        ctx->frame_type = info->type;
        if (ctx->done_sem != NULL) {
            rtos_set_semaphore(&ctx->done_sem);
        }
    }
    if (info->status == BK_OK) {
        LOGI("encode callback success, size=%u type=%u\r\n",
             info->length, info->type);
    } else {
        LOGE("encode callback failed, status=%u size=%u type=%u\r\n",
             info->status, info->length, info->type);
    }

    if (info->outbuf == NULL) {
        LOGE("output buffer is NULL\r\n");
        return BK_FAIL;
    }
    uint32_t frame_size = ((sizeof(frame_buffer_t) + 63) >> 6) << 6;
    frame_buffer_t *frame = (frame_buffer_t *)((uint8_t *)info->outbuf - frame_size);
    bk_frame_buffer_free(frame);
    return BK_OK;
}

static uint8_t *vcenc_h264_test_alloc_input(const uint8_t *src, uint32_t input_size)
{
    uint8_t *input = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
        input_size + 32U);
    if (input == NULL) {
        LOGE("input frame malloc failed, size=%u\r\n", input_size);
        return NULL;
    }

    os_memcpy(input, src, input_size);
    LOGI("input frame ready, addr=%p size=%u width=%u height=%u format=NV12\r\n",
         input, input_size, VCENC_H264_TEST_WIDTH, VCENC_H264_TEST_HEIGHT);
    return input;
}

static int vcenc_h264_wait_done(vcenc_h264_test_ctx_t *ctx, const char *name)
{
    bk_err_t ret;

    if (ctx == NULL || ctx->done_sem == NULL) {
        return BK_FAIL;
    }

    ret = rtos_get_semaphore(&ctx->done_sem, VCENC_H264_TEST_TIMEOUT_MS);
    if (ret != BK_OK) {
        LOGE("%s wait encode done timeout, ret=%d\r\n", name, ret);
        return BK_FAIL;
    }
    if (ctx->result != BK_OK) {
        LOGE("%s encode failed, result=%u\r\n", name, ctx->result);
        return BK_FAIL;
    }

    LOGI("%s encode done, size=%u type=%u\r\n", name, ctx->frame_size, ctx->frame_type);
    return BK_OK;
}

static void vcenc_h264_test_flexa_done(uint32_t rd_blocks, void *arg)
{
    vcenc_h264_test_ctx_t *ctx = (vcenc_h264_test_ctx_t *)arg;
    uint32_t next_wr_blocks;
    avdk_err_t ret;

    if (ctx == NULL || ctx->handle == NULL) {
        LOGE("flexa done invalid ctx, rd_blocks=%u\r\n", rd_blocks);
        return;
    }

    /*
     * SW flexa starts with a small lead window. After HW consumes one block,
     * advance WR pointer to keep feeding the remaining 16-line blocks.
     */
    next_wr_blocks = rd_blocks + 1U;
    if (next_wr_blocks > VCENC_H264_TEST_FLEXA_CNT) {
        next_wr_blocks = VCENC_H264_TEST_FLEXA_CNT;
    }

    if (next_wr_blocks > ctx->flexa_wr_blocks) {
        ret = bk_h264_encode_ioctl(ctx->handle,
                                   BK_H264_ENCODE_IOCTL_SET_FLEXA_LINES_READY,
                                   (void *)next_wr_blocks);
        if (ret != AVDK_ERR_OK) {
            LOGE("advance flexa wr failed, rd=%u wr=%u ret=%d\r\n",
                 rd_blocks, next_wr_blocks, ret);
            return;
        }
        ctx->flexa_wr_blocks = next_wr_blocks;
        // LOGI("advance flexa wr, rd=%u wr=%u/%u\r\n",
        //      rd_blocks, next_wr_blocks, VCENC_H264_TEST_FLEXA_CNT);
    } else {
        // LOGD("flexa wr unchanged, rd=%u wr=%u/%u\r\n",
        //      rd_blocks, ctx->flexa_wr_blocks, VCENC_H264_TEST_FLEXA_CNT);
    }
}

int vcenc_h264_frame_test(void)
{
    int ret = BK_FAIL;
    avdk_err_t avdk_ret;
    uint8_t *input = NULL;
    bk_h264_encode_ctlr_handle_t handle = NULL;
    vcenc_h264_test_ctx_t ctx;
    bk_h264_encode_frame_config_t config;
    const char *fail_stage = "start";
    uint8_t test_pass = 0U;
    uint32_t done_frames = 0U;

    LOGI("normal h264 encode demo start, gop=%u frames=%u\r\n",
         VCENC_H264_TEST_GOP, VCENC_H264_TEST_FRAME_CNT);
    os_memset(&ctx, 0, sizeof(ctx));
    os_memset(&config, 0, sizeof(config));

    if (rtos_init_semaphore(&ctx.done_sem, 1) != BK_OK) {
        fail_stage = "init_done_sem";
        LOGE("init frame done sem failed\r\n");
        vcenc_h264_log_test_result("vcenc_h264_frame_test", 0U, fail_stage, BK_FAIL, 0U, 0U, 0U);
        return BK_FAIL;
    }

    input = vcenc_h264_test_alloc_input(h264_encode_stream_256x128, h264_encode_stream_256x128_bytes);
    if (input == NULL) {
        fail_stage = "alloc_input";
        goto exit;
    }

    config.width = VCENC_H264_TEST_WIDTH;
    config.height = VCENC_H264_TEST_HEIGHT;
    config.gop_frame_count = VCENC_H264_TEST_GOP;
    config.input_format = BK_PIXEL_FORMAT_NV12;
    config.input_flexa_cnt = 1;
    config.input_buf = (uint32_t)input;
    config.input_size = h264_encode_stream_256x128_bytes;
    config.outbuf_malloc = vcenc_h264_test_outbuf_malloc;
    config.outbuf_complete = vcenc_h264_test_outbuf_complete;
    config.outbuf_complete_args = &ctx;

    avdk_ret = bk_h264_encode_frame_new(&handle, &config);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "frame_ctlr_new";
        ret = avdk_ret;
        LOGE("bk_h264_encode_frame_new failed, ret=%d\r\n", avdk_ret);
        goto exit;
    }
    LOGI("frame encoder controller created\r\n");
    avdk_ret = bk_h264_encode_init(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_init";
        ret = avdk_ret;
        LOGE("bk_h264_encode_init failed, ret=%d\r\n", avdk_ret);
        goto exit;
    }
    LOGI("frame encoder init success\r\n");
    avdk_ret = bk_h264_encode_open(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_open";
        ret = avdk_ret;
        LOGE("bk_h264_encode_open failed, ret=%d\r\n", avdk_ret);
        goto exit;
    }
    LOGI("frame encoder open success\r\n");
    for (uint32_t i = 0U; i < VCENC_H264_TEST_FRAME_CNT; i++) {
        LOGI("frame encode round %u/%u start\r\n",
             i + 1U, VCENC_H264_TEST_FRAME_CNT);
        ctx.result = BK_FAIL;
        ctx.frame_size = 0U;
        ctx.frame_type = 0U;

        avdk_ret = bk_h264_encode_start(handle);
        if (avdk_ret != AVDK_ERR_OK) {
            fail_stage = "encode_start";
            ret = avdk_ret;
            LOGE("frame encode round %u failed to start, ret=%d\r\n", i + 1U, avdk_ret);
            goto exit;
        }

        fail_stage = "wait_done";
        ret = vcenc_h264_wait_done(&ctx, "frame");
        if (ret != BK_OK) {
            LOGE("frame encode round %u failed, ret=%d\r\n", i + 1U, ret);
            goto exit;
        }
        done_frames++;
        LOGI("frame encode round %u/%u done\r\n",
             i + 1U, VCENC_H264_TEST_FRAME_CNT);
    }

    test_pass = 1U;
    LOGI("normal h264 encode demo complete\r\n");

exit:
    if (handle != NULL) {
        bk_h264_encode_close(handle);
        bk_h264_encode_deinit(handle);
        bk_h264_encode_delete(handle);
        LOGI("frame encoder destroyed\r\n");
    }
    if (input != NULL) {
        bk_frame_buffer_free(input);
        LOGI("free input frame\r\n");
    }
    if (ctx.done_sem != NULL) {
        rtos_deinit_semaphore(&ctx.done_sem);
    }
    vcenc_h264_log_test_result("vcenc_h264_frame_test", test_pass, fail_stage, ret, done_frames,
                               ctx.frame_size, ctx.frame_type);
    return ret;
}

int vcenc_h264_flexa_test(void)
{
    int ret = BK_FAIL;
    avdk_err_t avdk_ret;
    uint8_t *input = NULL;
    bk_h264_encode_ctlr_handle_t handle = NULL;
    vcenc_h264_test_ctx_t ctx;
    bk_h264_encode_sw_flexa_config_t config;
    const char *fail_stage = "start";
    uint8_t test_pass = 0U;
    uint32_t done_frames = 0U;

    LOGI("flexa h264 encode demo start, gop=%u frames=%u\r\n",
         VCENC_H264_TEST_GOP, VCENC_H264_TEST_FRAME_CNT);
    os_memset(&ctx, 0, sizeof(ctx));
    os_memset(&config, 0, sizeof(config));

    if (rtos_init_semaphore(&ctx.done_sem, 1) != BK_OK) {
        fail_stage = "init_done_sem";
        LOGE("init flexa done sem failed\r\n");
        vcenc_h264_log_test_result("vcenc_h264_flexa_test", 0U, fail_stage, BK_FAIL, 0U, 0U, 0U);
        return BK_FAIL;
    }

    input = vcenc_h264_test_alloc_input(h264_encode_stream_256x128, h264_encode_stream_256x128_bytes);
    if (input == NULL) {
        fail_stage = "alloc_input";
        goto exit;
    }

    config.width = VCENC_H264_TEST_WIDTH;
    config.height = VCENC_H264_TEST_HEIGHT;
    config.gop_frame_count = VCENC_H264_TEST_GOP;
    config.input_format = BK_PIXEL_FORMAT_NV12;
    config.input_flexa_cnt = VCENC_H264_TEST_FLEXA_CNT;
    config.input_buf = (uint32_t)input;
    config.input_size = h264_encode_stream_256x128_bytes;
    config.outbuf_malloc = vcenc_h264_test_outbuf_malloc;
    config.outbuf_complete = vcenc_h264_test_outbuf_complete;
    config.outbuf_complete_args = &ctx;
    config.flexa_done = vcenc_h264_test_flexa_done;
    config.flexa_done_arg = &ctx;

    avdk_ret = bk_h264_encode_sw_flexa_new(&handle, &config);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "sw_flexa_ctlr_new";
        ret = avdk_ret;
        LOGE("bk_h264_encode_sw_flexa_new failed, ret=%d\r\n", avdk_ret);
        goto exit;
    }
    ctx.handle = handle;
    LOGI("flexa encoder controller created, blocks=%u\r\n", VCENC_H264_TEST_FLEXA_CNT);
    avdk_ret = bk_h264_encode_init(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_init";
        ret = avdk_ret;
        LOGE("bk_h264_encode_init failed, ret=%d\r\n", avdk_ret);
        goto exit;
    }
    LOGI("flexa encoder init success\r\n");
    avdk_ret = bk_h264_encode_open(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_open";
        ret = avdk_ret;
        LOGE("bk_h264_encode_open failed, ret=%d\r\n", avdk_ret);
        goto exit;
    }
    LOGI("flexa encoder open success\r\n");

    for (uint32_t i = 0U; i < VCENC_H264_TEST_FRAME_CNT; i++) {
        ctx.result = BK_FAIL;
        ctx.frame_size = 0U;
        ctx.frame_type = 0U;
        ctx.flexa_wr_blocks = 2U;
        LOGI("flexa encode round %u/%u start, initial_ready_blocks=%u total_blocks=%u\r\n",
             i + 1U, VCENC_H264_TEST_FRAME_CNT, ctx.flexa_wr_blocks, VCENC_H264_TEST_FLEXA_CNT);

        avdk_ret = bk_h264_encode_ioctl(handle, BK_H264_ENCODE_IOCTL_SET_FRAME_READY, NULL);
        if (avdk_ret != AVDK_ERR_OK) {
            fail_stage = "set_frame_ready";
            ret = avdk_ret;
            LOGE("flexa encode round %u set frame ready failed, ret=%d\r\n", i + 1U, avdk_ret);
            goto exit;
        }

        fail_stage = "wait_done";
        ret = vcenc_h264_wait_done(&ctx, "flexa");
        if (ret != BK_OK) {
            LOGE("flexa encode round %u failed, ret=%d\r\n", i + 1U, ret);
            goto exit;
        }
        done_frames++;
        LOGI("flexa encode round %u/%u done\r\n",
             i + 1U, VCENC_H264_TEST_FRAME_CNT);
    }

    test_pass = 1U;
    LOGI("flexa h264 encode demo complete\r\n");

exit:
    if (handle != NULL) {
        bk_h264_encode_close(handle);
        bk_h264_encode_deinit(handle);
        bk_h264_encode_delete(handle);
        LOGI("flexa encoder destroyed\r\n");
    }
    if (input != NULL) {
        bk_frame_buffer_free(input);
        LOGI("free input frame\r\n");
    }
    if (ctx.done_sem != NULL) {
        rtos_deinit_semaphore(&ctx.done_sem);
    }
    vcenc_h264_log_test_result("vcenc_h264_flexa_test", test_pass, fail_stage, ret, done_frames,
                               ctx.frame_size, ctx.frame_type);
    return ret;
}

#define VCENC_H264_DUAL_CHANNEL_CNT       (2U)
#define VCENC_H264_DUAL_TASK_PRIORITY     (BEKEN_DEFAULT_WORKER_PRIORITY)
#define VCENC_H264_DUAL_TASK_STACK_SIZE   (1024 * 16)
#define VCENC_H264_DUAL_CH1_WIDTH         (512U)
#define VCENC_H264_DUAL_CH1_HEIGHT        (256U)

typedef struct
{
    uint32_t ch;
    uint32_t width;
    uint32_t height;
    beken_semaphore_t done_sem;
    uint32_t result;
    uint32_t frame_size;
    uint32_t frame_type;
    uint32_t done_frames;
    int ret;
} vcenc_h264_dual_chctx_t;

static void *vcenc_h264_dual_outbuf_malloc(uint32_t size, void *args)
{
    vcenc_h264_dual_chctx_t *ctx = (vcenc_h264_dual_chctx_t *)args;

    uint32_t frame_size = ((sizeof(frame_buffer_t) + size + 63) >> 6) << 6;
    frame_buffer_t *frame = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED,
                                                                    frame_size);
    if (frame == NULL) {
        LOGE("ch%u output frame malloc failed, size=%u\r\n", ctx ? ctx->ch : 0U, size);
        return NULL;
    }

    frame->frame = ((uint8_t *)frame) + (((sizeof(frame_buffer_t) + 63) >> 6) << 6);
    frame->size = size;
    frame->length = 0;
    frame->width = (ctx != NULL) ? ctx->width : VCENC_H264_TEST_WIDTH;
    frame->height = (ctx != NULL) ? ctx->height : VCENC_H264_TEST_HEIGHT;
    return frame->frame;
}

static uint8_t *vcenc_h264_test_alloc_pattern_input(uint32_t width, uint32_t height)
{
    uint32_t y_size = width * height;
    uint32_t uv_size = y_size / 2U;
    uint32_t input_size = y_size + uv_size;

    uint8_t *input = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED,
                                                       input_size + 32U);
    if (input == NULL) {
        LOGE("pattern input malloc failed, width=%u height=%u size=%u\r\n",
             width, height, input_size);
        return NULL;
    }

    /*
     * Tile the built-in 256x128 NV12 frame into the target (here 512x256 => 2x2 tiles).
     * Y plane  = 2x2 tiles of 256x128, UV plane = 2x2 tiles of 256x64.
     */
    const uint8_t *src = h264_encode_stream_256x128;
    uint32_t src_w = VCENC_H264_TEST_WIDTH;
    uint32_t src_h = VCENC_H264_TEST_HEIGHT;
    uint32_t src_y_size = src_w * src_h;
    const uint8_t *src_uv = src + src_y_size;
    uint32_t tile_cols = width / src_w;
    uint32_t tile_rows = height / src_h;

    for (uint32_t r = 0U; r < tile_rows; r++) {
        for (uint32_t c = 0U; c < tile_cols; c++) {
            uint32_t dst_y_off = (r * src_h) * width + c * src_w;
            for (uint32_t j = 0U; j < src_h; j++) {
                os_memcpy(input + dst_y_off + j * width, src + j * src_w, src_w);
            }
            uint32_t dst_uv_off = y_size + (r * (src_h / 2U)) * width + c * src_w;
            for (uint32_t j = 0U; j < src_h / 2U; j++) {
                os_memcpy(input + dst_uv_off + j * width, src_uv + j * src_w, src_w);
            }
        }
    }

    LOGI("pattern input ready, addr=%p size=%u width=%u height=%u tiles=%ux%u format=NV12\r\n",
         input, input_size, width, height, tile_cols, tile_rows);
    return input;
}

static uint32_t vcenc_h264_dual_outbuf_complete(bk_h264_encode_outbuf_info_t *info)
{
    if (info == NULL) {
        return BK_FAIL;
    }

    vcenc_h264_dual_chctx_t *ctx = (vcenc_h264_dual_chctx_t *)info->args;

    if (ctx != NULL) {
        ctx->result = info->status;
        ctx->frame_size = info->length;
        ctx->frame_type = info->type;
        if (ctx->done_sem != NULL) {
            rtos_set_semaphore(&ctx->done_sem);
        }
    }
    if (info->status == BK_OK) {
        LOGI("ch%u encode callback success, size=%u type=%u\r\n",
             ctx ? ctx->ch : 0U, info->length, info->type);
    } else {
        LOGE("ch%u encode callback failed, status=%u size=%u type=%u\r\n",
             ctx ? ctx->ch : 0U, info->status, info->length, info->type);
    }

    if (info->outbuf == NULL) {
        LOGE("ch%u output buffer is NULL\r\n", ctx ? ctx->ch : 0U);
        return BK_FAIL;
    }
    uint32_t frame_size = ((sizeof(frame_buffer_t) + 63) >> 6) << 6;
    frame_buffer_t *frame = (frame_buffer_t *)((uint8_t *)info->outbuf - frame_size);
    bk_frame_buffer_free(frame);
    return BK_OK;
}

static int vcenc_h264_dual_wait_done(vcenc_h264_dual_chctx_t *ctx)
{
    bk_err_t ret;

    if (ctx == NULL || ctx->done_sem == NULL) {
        return BK_FAIL;
    }

    ret = rtos_get_semaphore(&ctx->done_sem, VCENC_H264_TEST_TIMEOUT_MS);
    if (ret != BK_OK) {
        LOGE("ch%u wait encode done timeout, ret=%d\r\n", ctx->ch, ret);
        return BK_FAIL;
    }
    if (ctx->result != BK_OK) {
        LOGE("ch%u encode failed, result=%u\r\n", ctx->ch, ctx->result);
        return BK_FAIL;
    }

    LOGI("ch%u encode done, size=%u type=%u\r\n", ctx->ch, ctx->frame_size, ctx->frame_type);
    return BK_OK;
}

static void vcenc_h264_dual_log_test_result(uint8_t pass, const char *stage, int ret,
                                            const vcenc_h264_dual_chctx_t *chctx)
{
    uint32_t expect_frames = VCENC_H264_DUAL_CHANNEL_CNT * VCENC_H264_TEST_FRAME_CNT;
    uint32_t done_frames = chctx[0].done_frames + chctx[1].done_frames;

    if (pass) {
        LOGI("[RESULT][PASS] vcenc_h264_dual_test success, channels=%u/%u frames=%u/%u "
             "ch0=%ux%u size=%u ch1=%ux%u size=%u\r\n",
             VCENC_H264_DUAL_CHANNEL_CNT, VCENC_H264_DUAL_CHANNEL_CNT,
             done_frames, expect_frames,
             chctx[0].width, chctx[0].height, chctx[0].frame_size,
             chctx[1].width, chctx[1].height, chctx[1].frame_size);
    } else {
        LOGE("[RESULT][FAIL] vcenc_h264_dual_test failed at %s, ret=%d, channels=%u/%u "
             "frames=%u/%u\r\n",
             stage, ret, VCENC_H264_DUAL_CHANNEL_CNT, VCENC_H264_DUAL_CHANNEL_CNT,
             done_frames, expect_frames);
    }
}

static void vcenc_h264_dual_channel_entry(void *arg)
{
    vcenc_h264_dual_chctx_t *ctx = (vcenc_h264_dual_chctx_t *)arg;
    avdk_err_t avdk_ret;
    uint32_t width;
    uint32_t height;
    uint32_t input_size;
    uint8_t *input = NULL;
    bk_h264_encode_ctlr_handle_t handle = NULL;
    bk_h264_encode_frame_config_t config;
    const char *fail_stage = "start";
    int ret = BK_FAIL;

    if (ctx == NULL) {
        LOGE("dual channel invalid ctx\r\n");
        return;
    }

    LOGI("ch%u dual encode task start\r\n", ctx->ch);
    os_memset(&config, 0, sizeof(config));

    if (ctx->ch == 0U) {
        width = VCENC_H264_TEST_WIDTH;
        height = VCENC_H264_TEST_HEIGHT;
        input_size = h264_encode_stream_256x128_bytes;
        input = vcenc_h264_test_alloc_input(h264_encode_stream_256x128, input_size);
    } else {
        width = VCENC_H264_DUAL_CH1_WIDTH;
        height = VCENC_H264_DUAL_CH1_HEIGHT;
        input_size = width * height * 3U / 2U;
        input = vcenc_h264_test_alloc_pattern_input(width, height);
    }
    if (input == NULL) {
        fail_stage = "alloc_input";
        goto exit;
    }
    ctx->width = width;
    ctx->height = height;

    config.width = width;
    config.height = height;
    config.gop_frame_count = VCENC_H264_TEST_GOP;
    config.input_format = BK_PIXEL_FORMAT_NV12;
    config.input_flexa_cnt = 1;
    config.input_buf = (uint32_t)input;
    config.input_size = input_size;
    config.outbuf_malloc = vcenc_h264_dual_outbuf_malloc;
    config.outbuf_malloc_args = ctx;
    config.outbuf_complete = vcenc_h264_dual_outbuf_complete;
    config.outbuf_complete_args = ctx;

    avdk_ret = bk_h264_encode_frame_new(&handle, &config);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "frame_ctlr_new";
        ret = avdk_ret;
        LOGE("ch%u bk_h264_encode_frame_new failed, ret=%d\r\n", ctx->ch, avdk_ret);
        goto exit;
    }

    avdk_ret = bk_h264_encode_init(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_init";
        ret = avdk_ret;
        LOGE("ch%u bk_h264_encode_init failed, ret=%d\r\n", ctx->ch, avdk_ret);
        goto exit;
    }
    avdk_ret = bk_h264_encode_open(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_open";
        ret = avdk_ret;
        LOGE("ch%u bk_h264_encode_open failed, ret=%d\r\n", ctx->ch, avdk_ret);
        goto exit;
    }
    LOGI("ch%u encoder open success\r\n", ctx->ch);

    for (uint32_t i = 0U; i < VCENC_H264_TEST_FRAME_CNT; i++) {
        ctx->result = BK_FAIL;
        ctx->frame_size = 0U;
        ctx->frame_type = 0U;

        avdk_ret = bk_h264_encode_start(handle);
        if (avdk_ret != AVDK_ERR_OK) {
            fail_stage = "encode_start";
            ret = avdk_ret;
            LOGE("ch%u encode round %u failed to start, ret=%d\r\n", ctx->ch, i + 1U, avdk_ret);
            goto exit;
        }

        fail_stage = "wait_done";
        ret = vcenc_h264_dual_wait_done(ctx);
        if (ret != BK_OK) {
            LOGE("ch%u encode round %u failed, ret=%d\r\n", ctx->ch, i + 1U, ret);
            goto exit;
        }
        ctx->done_frames++;
    }

    LOGI("ch%u dual encode done, frames=%u\r\n", ctx->ch, ctx->done_frames);
    ret = BK_OK;

exit:
    if (handle != NULL) {
        bk_h264_encode_close(handle);
        bk_h264_encode_deinit(handle);
        bk_h264_encode_delete(handle);
    }
    if (input != NULL) {
        bk_frame_buffer_free(input);
    }
    if (ret != BK_OK) {
        LOGE("ch%u dual encode failed at %s, ret=%d\r\n", ctx->ch, fail_stage, ret);
    }
    ctx->ret = ret;
    rtos_delete_thread(NULL);
}

int vcenc_h264_dual_test(void)
{
    vcenc_h264_dual_chctx_t chctx[VCENC_H264_DUAL_CHANNEL_CNT];
    beken_thread_t threads[VCENC_H264_DUAL_CHANNEL_CNT] = { NULL, NULL };
    uint8_t created[VCENC_H264_DUAL_CHANNEL_CNT] = { 0U, 0U };
    uint8_t sem_inited[VCENC_H264_DUAL_CHANNEL_CNT] = { 0U, 0U };
    int ret = BK_FAIL;
    const char *fail_stage = "start";
    uint8_t test_pass = 0U;
    bk_err_t bk_ret;

    LOGI("dual h264 encode demo start, channels=%u gop=%u frames=%u/ch ch0=%ux%u ch1=%ux%u\r\n",
         VCENC_H264_DUAL_CHANNEL_CNT, VCENC_H264_TEST_GOP, VCENC_H264_TEST_FRAME_CNT,
         VCENC_H264_TEST_WIDTH, VCENC_H264_TEST_HEIGHT,
         VCENC_H264_DUAL_CH1_WIDTH, VCENC_H264_DUAL_CH1_HEIGHT);
    os_memset(chctx, 0, sizeof(chctx));
    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        chctx[ch].ch = ch;
    }

    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        if (rtos_init_semaphore(&chctx[ch].done_sem, 1) != BK_OK) {
            fail_stage = "init_done_sem";
            LOGE("init ch%u done sem failed\r\n", ch);
            goto exit;
        }
        sem_inited[ch] = 1U;
    }

    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        const char *name = (ch == 0U) ? "vcenc_h264_dual0" : "vcenc_h264_dual1";
        bk_ret = rtos_create_thread(&threads[ch],
                                    VCENC_H264_DUAL_TASK_PRIORITY,
                                    name,
                                    (beken_thread_function_t)vcenc_h264_dual_channel_entry,
                                    VCENC_H264_DUAL_TASK_STACK_SIZE,
                                    (beken_thread_arg_t)&chctx[ch]);
        if (bk_ret != BK_OK) {
            fail_stage = "create_thread";
            LOGE("create ch%u dual task failed, ret=%d\r\n", ch, bk_ret);
            goto exit;
        }
        created[ch] = 1U;
        LOGI("ch%u dual task created\r\n", ch);
    }

    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        if (!created[ch]) {
            continue;
        }
        bk_ret = rtos_thread_join(&threads[ch]);
        if (bk_ret != BK_OK) {
            fail_stage = "join_thread";
            LOGE("join ch%u dual task failed, ret=%d\r\n", ch, bk_ret);
            goto exit;
        }
    }

    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        if (chctx[ch].ret != BK_OK) {
            fail_stage = "channel_result";
            LOGE("ch%u dual encode failed, ret=%d\r\n", ch, chctx[ch].ret);
            ret = chctx[ch].ret;
            goto exit;
        }
    }

    test_pass = 1U;
    ret = BK_OK;
    LOGI("dual h264 encode demo complete\r\n");

exit:
    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        if (sem_inited[ch]) {
            rtos_deinit_semaphore(&chctx[ch].done_sem);
        }
    }
    vcenc_h264_dual_log_test_result(test_pass, fail_stage, ret, chctx);
    return ret;
}

typedef struct
{
    uint32_t ch;
    uint32_t width;
    uint32_t height;
    uint32_t flexa_cnt;
    beken_semaphore_t done_sem;
    bk_h264_encode_ctlr_handle_t handle;
    uint32_t result;
    uint32_t frame_size;
    uint32_t frame_type;
    uint32_t flexa_wr_blocks;
    uint32_t done_frames;
    int ret;
} vcenc_h264_flexa_dual_ctx_t;

static void *vcenc_h264_flexa_dual_outbuf_malloc(uint32_t size, void *args)
{
    vcenc_h264_flexa_dual_ctx_t *ctx = (vcenc_h264_flexa_dual_ctx_t *)args;

    uint32_t frame_size = ((sizeof(frame_buffer_t) + size + 63) >> 6) << 6;
    frame_buffer_t *frame = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED,
                                                                    frame_size);
    if (frame == NULL) {
        LOGE("ch%u flexa output frame malloc failed, size=%u\r\n", ctx ? ctx->ch : 0U, size);
        return NULL;
    }

    frame->frame = ((uint8_t *)frame) + (((sizeof(frame_buffer_t) + 63) >> 6) << 6);
    frame->size = size;
    frame->length = 0;
    frame->width = (ctx != NULL) ? ctx->width : VCENC_H264_TEST_WIDTH;
    frame->height = (ctx != NULL) ? ctx->height : VCENC_H264_TEST_HEIGHT;
    return frame->frame;
}

static uint32_t vcenc_h264_flexa_dual_outbuf_complete(bk_h264_encode_outbuf_info_t *info)
{
    if (info == NULL) {
        return BK_FAIL;
    }

    vcenc_h264_flexa_dual_ctx_t *ctx = (vcenc_h264_flexa_dual_ctx_t *)info->args;

    if (ctx != NULL) {
        ctx->result = info->status;
        ctx->frame_size = info->length;
        ctx->frame_type = info->type;
        if (ctx->done_sem != NULL) {
            rtos_set_semaphore(&ctx->done_sem);
        }
    }
    if (info->status == BK_OK) {
        LOGI("ch%u flexa encode callback success, size=%u type=%u\r\n",
             ctx ? ctx->ch : 0U, info->length, info->type);
    } else {
        LOGE("ch%u flexa encode callback failed, status=%u size=%u type=%u\r\n",
             ctx ? ctx->ch : 0U, info->status, info->length, info->type);
    }

    if (info->outbuf == NULL) {
        LOGE("ch%u output buffer is NULL\r\n", ctx ? ctx->ch : 0U);
        return BK_FAIL;
    }
    uint32_t frame_size = ((sizeof(frame_buffer_t) + 63) >> 6) << 6;
    frame_buffer_t *frame = (frame_buffer_t *)((uint8_t *)info->outbuf - frame_size);
    bk_frame_buffer_free(frame);
    return BK_OK;
}

static void vcenc_h264_flexa_dual_done(uint32_t rd_blocks, void *arg)
{
    vcenc_h264_flexa_dual_ctx_t *ctx = (vcenc_h264_flexa_dual_ctx_t *)arg;
    uint32_t next_wr_blocks;
    avdk_err_t ret;

    if (ctx == NULL || ctx->handle == NULL) {
        LOGE("flexa dual done invalid ctx, rd_blocks=%u\r\n", rd_blocks);
        return;
    }

    /*
     * SW flexa starts with a small lead window. After HW consumes one block,
     * advance WR pointer to keep feeding the remaining 16-line blocks.
     */
    next_wr_blocks = rd_blocks + 1U;
    if (next_wr_blocks > ctx->flexa_cnt) {
        next_wr_blocks = ctx->flexa_cnt;
    }

    if (next_wr_blocks > ctx->flexa_wr_blocks) {
        ret = bk_h264_encode_ioctl(ctx->handle,
                                   BK_H264_ENCODE_IOCTL_SET_FLEXA_LINES_READY,
                                   (void *)next_wr_blocks);
        if (ret != AVDK_ERR_OK) {
            LOGE("ch%u advance flexa wr failed, rd=%u wr=%u ret=%d\r\n",
                 ctx->ch, rd_blocks, next_wr_blocks, ret);
            return;
        }
        ctx->flexa_wr_blocks = next_wr_blocks;
    }
}

static int vcenc_h264_flexa_dual_wait_done(vcenc_h264_flexa_dual_ctx_t *ctx)
{
    bk_err_t ret;

    if (ctx == NULL || ctx->done_sem == NULL) {
        return BK_FAIL;
    }

    ret = rtos_get_semaphore(&ctx->done_sem, VCENC_H264_TEST_TIMEOUT_MS);
    if (ret != BK_OK) {
        LOGE("ch%u wait flexa encode done timeout, ret=%d\r\n", ctx->ch, ret);
        return BK_FAIL;
    }
    if (ctx->result != BK_OK) {
        LOGE("ch%u flexa encode failed, result=%u\r\n", ctx->ch, ctx->result);
        return BK_FAIL;
    }

    LOGI("ch%u flexa encode done, size=%u type=%u\r\n", ctx->ch, ctx->frame_size, ctx->frame_type);
    return BK_OK;
}

static void vcenc_h264_flexa_dual_log_test_result(uint8_t pass, const char *stage, int ret,
                                                  const vcenc_h264_flexa_dual_ctx_t *chctx)
{
    uint32_t expect_frames = VCENC_H264_DUAL_CHANNEL_CNT * VCENC_H264_TEST_FRAME_CNT;
    uint32_t done_frames = chctx[0].done_frames + chctx[1].done_frames;

    if (pass) {
        LOGI("[RESULT][PASS] vcenc_h264_flexa_dual_test success, channels=%u/%u frames=%u/%u "
             "ch0=%ux%u size=%u ch1=%ux%u size=%u\r\n",
             VCENC_H264_DUAL_CHANNEL_CNT, VCENC_H264_DUAL_CHANNEL_CNT,
             done_frames, expect_frames,
             chctx[0].width, chctx[0].height, chctx[0].frame_size,
             chctx[1].width, chctx[1].height, chctx[1].frame_size);
    } else {
        LOGE("[RESULT][FAIL] vcenc_h264_flexa_dual_test failed at %s, ret=%d, channels=%u/%u "
             "frames=%u/%u\r\n",
             stage, ret, VCENC_H264_DUAL_CHANNEL_CNT, VCENC_H264_DUAL_CHANNEL_CNT,
             done_frames, expect_frames);
    }
}

static void vcenc_h264_flexa_dual_channel_entry(void *arg)
{
    vcenc_h264_flexa_dual_ctx_t *ctx = (vcenc_h264_flexa_dual_ctx_t *)arg;
    avdk_err_t avdk_ret;
    uint32_t width;
    uint32_t height;
    uint32_t flexa_cnt;
    uint32_t input_size;
    uint8_t *input = NULL;
    bk_h264_encode_ctlr_handle_t handle = NULL;
    bk_h264_encode_sw_flexa_config_t config;
    const char *fail_stage = "start";
    int ret = BK_FAIL;

    if (ctx == NULL) {
        LOGE("flexa dual channel invalid ctx\r\n");
        return;
    }

    LOGI("ch%u flexa dual encode task start\r\n", ctx->ch);
    os_memset(&config, 0, sizeof(config));

    if (ctx->ch == 0U) {
        width = VCENC_H264_TEST_WIDTH;
        height = VCENC_H264_TEST_HEIGHT;
        flexa_cnt = VCENC_H264_TEST_FLEXA_CNT;
        input_size = h264_encode_stream_256x128_bytes;
        input = vcenc_h264_test_alloc_input(h264_encode_stream_256x128, input_size);
    } else {
        width = VCENC_H264_DUAL_CH1_WIDTH;
        height = VCENC_H264_DUAL_CH1_HEIGHT;
        flexa_cnt = height / VCENC_H264_TEST_FLEXA_LINES;
        input_size = width * height * 3U / 2U;
        input = vcenc_h264_test_alloc_pattern_input(width, height);
    }
    if (input == NULL) {
        fail_stage = "alloc_input";
        goto exit;
    }
    ctx->width = width;
    ctx->height = height;
    ctx->flexa_cnt = flexa_cnt;

    config.width = width;
    config.height = height;
    config.gop_frame_count = VCENC_H264_TEST_GOP;
    config.input_format = BK_PIXEL_FORMAT_NV12;
    config.input_flexa_cnt = flexa_cnt;
    config.input_buf = (uint32_t)input;
    config.input_size = input_size;
    config.outbuf_malloc = vcenc_h264_flexa_dual_outbuf_malloc;
    config.outbuf_malloc_args = ctx;
    config.outbuf_complete = vcenc_h264_flexa_dual_outbuf_complete;
    config.outbuf_complete_args = ctx;
    config.flexa_done = vcenc_h264_flexa_dual_done;
    config.flexa_done_arg = ctx;

    avdk_ret = bk_h264_encode_sw_flexa_new(&handle, &config);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "sw_flexa_ctlr_new";
        ret = avdk_ret;
        LOGE("ch%u bk_h264_encode_sw_flexa_new failed, ret=%d\r\n", ctx->ch, avdk_ret);
        goto exit;
    }
    ctx->handle = handle;
    LOGI("ch%u flexa encoder controller created, blocks=%u\r\n",
         ctx->ch, flexa_cnt);

    avdk_ret = bk_h264_encode_init(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_init";
        ret = avdk_ret;
        LOGE("ch%u bk_h264_encode_init failed, ret=%d\r\n", ctx->ch, avdk_ret);
        goto exit;
    }
    avdk_ret = bk_h264_encode_open(handle);
    if (avdk_ret != AVDK_ERR_OK) {
        fail_stage = "encoder_open";
        ret = avdk_ret;
        LOGE("ch%u bk_h264_encode_open failed, ret=%d\r\n", ctx->ch, avdk_ret);
        goto exit;
    }
    LOGI("ch%u flexa encoder open success\r\n", ctx->ch);

    for (uint32_t i = 0U; i < VCENC_H264_TEST_FRAME_CNT; i++) {
        ctx->result = BK_FAIL;
        ctx->frame_size = 0U;
        ctx->frame_type = 0U;
        ctx->flexa_wr_blocks = 2U;

        avdk_ret = bk_h264_encode_ioctl(handle, BK_H264_ENCODE_IOCTL_SET_FRAME_READY, NULL);
        if (avdk_ret != AVDK_ERR_OK) {
            fail_stage = "set_frame_ready";
            ret = avdk_ret;
            LOGE("ch%u flexa encode round %u set frame ready failed, ret=%d\r\n",
                 ctx->ch, i + 1U, avdk_ret);
            goto exit;
        }

        fail_stage = "wait_done";
        ret = vcenc_h264_flexa_dual_wait_done(ctx);
        if (ret != BK_OK) {
            LOGE("ch%u flexa encode round %u failed, ret=%d\r\n", ctx->ch, i + 1U, ret);
            goto exit;
        }
        ctx->done_frames++;
    }

    LOGI("ch%u flexa dual encode done, frames=%u\r\n", ctx->ch, ctx->done_frames);
    ret = BK_OK;

exit:
    if (handle != NULL) {
        bk_h264_encode_close(handle);
        bk_h264_encode_deinit(handle);
        bk_h264_encode_delete(handle);
    }
    if (input != NULL) {
        bk_frame_buffer_free(input);
    }
    if (ret != BK_OK) {
        LOGE("ch%u flexa dual encode failed at %s, ret=%d\r\n", ctx->ch, fail_stage, ret);
    }
    ctx->ret = ret;
    rtos_delete_thread(NULL);
}

int vcenc_h264_flexa_dual_test(void)
{
    vcenc_h264_flexa_dual_ctx_t chctx[VCENC_H264_DUAL_CHANNEL_CNT];
    beken_thread_t threads[VCENC_H264_DUAL_CHANNEL_CNT] = { NULL, NULL };
    uint8_t created[VCENC_H264_DUAL_CHANNEL_CNT] = { 0U, 0U };
    uint8_t sem_inited[VCENC_H264_DUAL_CHANNEL_CNT] = { 0U, 0U };
    int ret = BK_FAIL;
    const char *fail_stage = "start";
    uint8_t test_pass = 0U;
    bk_err_t bk_ret;

    LOGI("flexa dual h264 encode demo start, channels=%u gop=%u frames=%u/ch ch0=%ux%u ch1=%ux%u\r\n",
         VCENC_H264_DUAL_CHANNEL_CNT, VCENC_H264_TEST_GOP, VCENC_H264_TEST_FRAME_CNT,
         VCENC_H264_TEST_WIDTH, VCENC_H264_TEST_HEIGHT,
         VCENC_H264_DUAL_CH1_WIDTH, VCENC_H264_DUAL_CH1_HEIGHT);
    os_memset(chctx, 0, sizeof(chctx));
    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        chctx[ch].ch = ch;
    }

    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        if (rtos_init_semaphore(&chctx[ch].done_sem, 1) != BK_OK) {
            fail_stage = "init_done_sem";
            LOGE("init ch%u flexa done sem failed\r\n", ch);
            goto exit;
        }
        sem_inited[ch] = 1U;
    }

    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        const char *name = (ch == 0U) ? "vcenc_h264_flexa_dual0" : "vcenc_h264_flexa_dual1";
        bk_ret = rtos_create_thread(&threads[ch],
                                    VCENC_H264_DUAL_TASK_PRIORITY,
                                    name,
                                    (beken_thread_function_t)vcenc_h264_flexa_dual_channel_entry,
                                    VCENC_H264_DUAL_TASK_STACK_SIZE,
                                    (beken_thread_arg_t)&chctx[ch]);
        if (bk_ret != BK_OK) {
            fail_stage = "create_thread";
            LOGE("create ch%u flexa dual task failed, ret=%d\r\n", ch, bk_ret);
            goto exit;
        }
        created[ch] = 1U;
        LOGI("ch%u flexa dual task created\r\n", ch);
    }

    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        if (!created[ch]) {
            continue;
        }
        bk_ret = rtos_thread_join(&threads[ch]);
        if (bk_ret != BK_OK) {
            fail_stage = "join_thread";
            LOGE("join ch%u flexa dual task failed, ret=%d\r\n", ch, bk_ret);
            goto exit;
        }
    }

    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        if (chctx[ch].ret != BK_OK) {
            fail_stage = "channel_result";
            LOGE("ch%u flexa dual encode failed, ret=%d\r\n", ch, chctx[ch].ret);
            ret = chctx[ch].ret;
            goto exit;
        }
    }

    test_pass = 1U;
    ret = BK_OK;
    LOGI("flexa dual h264 encode demo complete\r\n");

exit:
    for (uint32_t ch = 0U; ch < VCENC_H264_DUAL_CHANNEL_CNT; ch++) {
        if (sem_inited[ch]) {
            rtos_deinit_semaphore(&chctx[ch].done_sem);
        }
    }
    vcenc_h264_flexa_dual_log_test_result(test_pass, fail_stage, ret, chctx);
    return ret;
}

static void vcenc_h264_boot_demo_task_entry(void *arg)
{
    int test_ret;

    (void)arg;

    rtos_delay_milliseconds(1000);
    LOGI("vcenc_h264_flexa_test start\r\n");
    test_ret = vcenc_h264_flexa_test();
    if (test_ret != 0) {
        LOGE("vcenc_h264_flexa_test failed, ret=%d\r\n", test_ret);
    } else {
        LOGI("vcenc_h264_flexa_test done\r\n");
    }

    rtos_delay_milliseconds(1000);
    LOGI("vcenc_h264_frame_test start\r\n");
    test_ret = vcenc_h264_frame_test();
    if (test_ret != 0) {
        LOGE("vcenc_h264_frame_test failed, ret=%d\r\n", test_ret);
    } else {
        LOGI("vcenc_h264_frame_test done\r\n");
    }

    s_vcenc_h264_boot_demo_running = 0;
    s_vcenc_h264_boot_demo_thread = NULL;
    rtos_delete_thread(NULL);
}

void vcenc_h264_run_boot_demo(void)
{
    bk_err_t ret;

    if (s_vcenc_h264_boot_demo_running != 0U) {
        LOGE("h264 encode boot demo task is already running\r\n");
        return;
    }

    s_vcenc_h264_boot_demo_running = 1U;
    ret = rtos_create_thread(&s_vcenc_h264_boot_demo_thread,
                             VCENC_H264_BOOT_DEMO_TASK_PRIORITY,
                             "vcenc_h264_boot_demo",
                             (beken_thread_function_t)vcenc_h264_boot_demo_task_entry,
                             VCENC_H264_BOOT_DEMO_TASK_STACK_SIZE,
                             NULL);
    if (ret != BK_OK) {
        LOGE("create h264 encode boot demo task failed, ret=%d\r\n", ret);
        s_vcenc_h264_boot_demo_running = 0U;
        s_vcenc_h264_boot_demo_thread = NULL;
        return;
    }

    LOGI("h264 encode boot demo task created\r\n");
}