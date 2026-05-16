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

    frame_buffer_t *frame = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED,
                                                                     size + sizeof(frame_buffer_t) + 32U);
    if (frame == NULL) {
        LOGE("output frame malloc failed, size=%u\r\n", size);
        return NULL;
    }

    frame->frame = (uint8_t *)((((uint32_t)(frame + 1) + 31U) >> 5) << 5);
    frame->size = size;
    frame->length = 0;
    frame->width = VCENC_H264_TEST_WIDTH;
    frame->height = VCENC_H264_TEST_HEIGHT;
    // LOGI("output buffer ready, frame=%p payload=%p size=%u\r\n", frame, frame->frame, size);
    return frame;
}

static uint32_t vcenc_h264_test_outbuf_complete(void *buffer, uint32_t status, void *args)
{
    vcenc_h264_test_ctx_t *ctx = (vcenc_h264_test_ctx_t *)args;
    frame_buffer_t *frame = (frame_buffer_t *)buffer;

    if (ctx != NULL) {
        ctx->result = status;
        ctx->frame_size = frame ? frame->length : 0;
        ctx->frame_type = frame ? frame->h264_type : 0;
        if (ctx->done_sem != NULL) {
            rtos_set_semaphore(&ctx->done_sem);
        }
    }
    if (status == BK_OK) {
        LOGI("encode callback success, size=%u type=%u\r\n",
             frame ? frame->length : 0, frame ? frame->h264_type : 0);
    } else {
        LOGE("encode callback failed, status=%u size=%u type=%u\r\n",
             status, frame ? frame->length : 0, frame ? frame->h264_type : 0);
    }

    if (frame != NULL) {
        bk_frame_buffer_free(frame);
        // LOGI("free output buffer\r\n");
    }

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
    next_wr_blocks = rd_blocks + 2U;
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
    config.pframe_number = VCENC_H264_TEST_GOP;
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
    config.pframe_number = VCENC_H264_TEST_GOP;
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