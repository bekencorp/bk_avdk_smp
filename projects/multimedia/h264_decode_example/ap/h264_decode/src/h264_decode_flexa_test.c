#include <stdint.h>
#include <os/os.h>
#include "os/mem.h"
#include "os/str.h"
#include <components/bk_frame_buffer.h>
#include <components/log.h>
#include <common/avdk_pixel_types.h>
#include "h264_decoder_api.h"
#include "h264_decode_flexa_test.h"
#include "h264_decode_stream_1280x720.h"

#define TAG "h264d_flexa"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define CLI_CMD_RSP_SUCCEED "CMDRSP:OK\r\n"
#define CLI_CMD_RSP_ERROR   "CMDRSP:ERROR\r\n"

#ifndef H264_DECODE_FLEXA_OUT_BUF_SIZE
#define H264_DECODE_FLEXA_OUT_BUF_SIZE (1280U * 720U * 3U / 2U)
#endif

#ifndef H264_DECODE_FLEXA_DUMP_SECOND_I_FRAME
#define H264_DECODE_FLEXA_DUMP_SECOND_I_FRAME 0     //dump nv12 image decoded from second i frame 
#endif

#define H264_DECODE_FLEXA_THREAD_STACK_SIZE (16 * 1024)

static void *s_h264d_flexa_ctx = NULL;
static beken_thread_t s_h264d_flexa_thread = NULL;
static frame_buffer_t *s_h264d_flexa_stream_fb = NULL;
static frame_buffer_t *s_h264d_flexa_out_fb = NULL;
static uint8_t *s_h264d_flexa_stream = NULL;
static uint8_t *s_h264d_flexa_out = NULL;
static volatile uint32_t s_h264d_flexa_wr_cb_count = 0;
static volatile uint32_t s_h264d_flexa_info_count = 0;
static volatile uint32_t s_h264d_flexa_frame_count = 0;
static volatile uint32_t s_h264d_flexa_i_frame_count = 0;
static volatile uint8_t s_h264d_flexa_second_i_dumped = 0;

extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);

static void h264d_flexa_cli_write_rsp(char *pcWriteBuffer, int xWriteBufferLen, const char *msg)
{
    int len;

    if (pcWriteBuffer == NULL || xWriteBufferLen <= 0 || msg == NULL) {
        return;
    }

    len = (int)os_strlen(msg);
    if (len >= xWriteBufferLen) {
        len = xWriteBufferLen - 1;
    }

    os_memcpy(pcWriteBuffer, msg, (size_t)len);
    pcWriteBuffer[len] = '\0';
}

static void h264d_flexa_done_callback(uint32_t wrCnt)
{
    bk_printf("[W]: %d\n", wrCnt);

    vcdec_flexa_input_linebuf_rdcnt_set(0, wrCnt);
}

static void h264d_flexa_dump_nv12_frame(const char *tag, uint8_t *y, uint32_t width, uint32_t height)
{
#if H264_DECODE_FLEXA_DUMP_SECOND_I_FRAME
    uint32_t dump_size;
    uint32_t dump_aligned;
    uint32_t irq_flags;

    if (y == NULL || width == 0U || height == 0U) {
        return;
    }

    dump_size = bk_image_size_get((uint16_t)width, (uint16_t)height, BK_PIXEL_FORMAT_NV12);
    dump_aligned = (dump_size + 3U) & ~3U;

    irq_flags = rtos_enter_critical();
    LOGI("nv12_dump tag=%s size=%u y=%p aligned=%u\n",
         tag,
         (unsigned)dump_size,
         y,
         (unsigned)dump_aligned);
    stack_mem_dump((uint32_t)(uintptr_t)y,
                   (uint32_t)(uintptr_t)(y + dump_aligned));
    rtos_exit_critical(irq_flags);
#else
    (void)tag;
    (void)y;
    (void)width;
    (void)height;
#endif
}

static void h264d_flexa_out_cb(uint8_t *y, uint8_t *cb, uint8_t *cr,
                               uint32_t width, uint32_t height, uint32_t type)
{
    if (type == VCDEC_OUT_INFO) {
        s_h264d_flexa_info_count++;
        LOGI("out(info#%u, %p, %p, %p, %u, %u, %u)\n",
             (unsigned)s_h264d_flexa_info_count,
             y,
             cb,
             cr,
             (unsigned)width,
             (unsigned)height,
             (unsigned)type);
    } else {
        s_h264d_flexa_frame_count++;
        LOGI("out(frame#%u, %p, %p, %p, %u, %u, %u)\n",
             (unsigned)s_h264d_flexa_frame_count,
             y,
             cb,
             cr,
             (unsigned)width,
             (unsigned)height,
             (unsigned)type);

        if (type == VCDEC_OUT_IFRAME) {
            s_h264d_flexa_i_frame_count++;
            if ((s_h264d_flexa_i_frame_count == 2U) && (s_h264d_flexa_second_i_dumped == 0U)) {
                s_h264d_flexa_second_i_dumped = 1U;
                h264d_flexa_dump_nv12_frame("second_i", y, width, height);
            }
        }
    }
}

static void h264d_flexa_release_buffers(void)
{
    if (s_h264d_flexa_out_fb != NULL) {
        bk_frame_buffer_free(s_h264d_flexa_out_fb);
        s_h264d_flexa_out_fb = NULL;
        s_h264d_flexa_out = NULL;
    }

    if (s_h264d_flexa_stream_fb != NULL) {
        bk_frame_buffer_free(s_h264d_flexa_stream_fb);
        s_h264d_flexa_stream_fb = NULL;
        s_h264d_flexa_stream = NULL;
    }
}

static void h264d_flexa_thread_entry(void *arg)
{
    int32_t ret = BK_FAIL;
    uint32_t used_size = 0;
    uint32_t total_used = 0;
    uint32_t decode_round = 0;
    uint32_t remain = h264_decode_stream_1280x720_bytes;
    uint8_t *in_ptr = NULL;

    (void)arg;

    s_h264d_flexa_wr_cb_count = 0;
    s_h264d_flexa_info_count = 0;
    s_h264d_flexa_frame_count = 0;
    s_h264d_flexa_i_frame_count = 0;
    s_h264d_flexa_second_i_dumped = 0;

    LOGI("enter h264_decode_flexa_test, stream_bytes=%u out_buf=%u\n",
         (unsigned)h264_decode_stream_1280x720_bytes,
         (unsigned)H264_DECODE_FLEXA_OUT_BUF_SIZE);

    if (h264_decode_stream_1280x720_bytes == 0U) {
        LOGE("embedded stream size is zero\n");
        goto exit_thread;
    }

    {
        uint32_t fb_size = h264_decode_stream_1280x720_bytes + (uint32_t)sizeof(frame_buffer_t) + 32U;

        s_h264d_flexa_stream_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, fb_size);
        if (s_h264d_flexa_stream_fb == NULL) {
            LOGE("failed to allocate stream buffer, size=%u\n", (unsigned)fb_size);
            goto exit_thread;
        }

        os_memset(s_h264d_flexa_stream_fb, 0, sizeof(frame_buffer_t));
        s_h264d_flexa_stream_fb->frame = (uint8_t *)((((uint32_t)(s_h264d_flexa_stream_fb + 1U) >> 5U) + 1U) << 5U);
        s_h264d_flexa_stream_fb->size = h264_decode_stream_1280x720_bytes;
        s_h264d_flexa_stream = s_h264d_flexa_stream_fb->frame;
        os_memcpy(s_h264d_flexa_stream, h264_decode_stream_1280x720, h264_decode_stream_1280x720_bytes);
    }

    {
        uint32_t out_fb_size = H264_DECODE_FLEXA_OUT_BUF_SIZE + (uint32_t)sizeof(frame_buffer_t) + 32U;

        s_h264d_flexa_out_fb = (frame_buffer_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, out_fb_size);
        if (s_h264d_flexa_out_fb == NULL) {
            LOGE("failed to allocate output buffer, size=%u\n", (unsigned)out_fb_size);
            goto exit_thread;
        }

        os_memset(s_h264d_flexa_out_fb, 0, sizeof(frame_buffer_t));
        s_h264d_flexa_out_fb->frame = (uint8_t *)((((uint32_t)(s_h264d_flexa_out_fb + 1U) >> 5U) + 1U) << 5U);
        s_h264d_flexa_out_fb->size = H264_DECODE_FLEXA_OUT_BUF_SIZE;
        s_h264d_flexa_out = s_h264d_flexa_out_fb->frame;
    }

    ret = h264_decoder_init(&s_h264d_flexa_ctx,
                            VCDEC_FLEXA_MODE_FLEXA,
                            h264d_flexa_done_callback,
                            NULL,
                            h264d_flexa_out_cb);
    LOGI("h264_decoder_init(ret=%d, ctx=%p)\n", (int)ret, s_h264d_flexa_ctx);
    if (ret != 0 || s_h264d_flexa_ctx == NULL) {
        LOGE("h264_decoder_init failed, ret=%d ctx=%p\n", (int)ret, s_h264d_flexa_ctx);
        goto exit_thread;
    }

    in_ptr = s_h264d_flexa_stream;
    while (remain > 0U) {
        uint32_t remain_before = remain;

        used_size = 0;
        decode_round++;
        ret = h264_decoder_decode(s_h264d_flexa_ctx, in_ptr, remain, s_h264d_flexa_out, &used_size);
        LOGI("decode_round=%u ret=%d usedSize=%u remain_before=%u remain_after=%u\n",
             (unsigned)decode_round,
             (int)ret,
             (unsigned)used_size,
             (unsigned)remain_before,
             (unsigned)(remain_before - used_size));

        if (ret < 0) {
            LOGE("h264_decoder_decode failed at round=%u ret=%d\n",
                 (unsigned)decode_round,
                 (int)ret);
            break;
        }

        if (used_size == 0U) {
            LOGE("h264_decoder_decode consumed zero bytes at round=%u\n",
                 (unsigned)decode_round);
            break;
        }

        if (used_size > remain) {
            LOGE("invalid usedSize=%u remain=%u\n",
                 (unsigned)used_size,
                 (unsigned)remain);
            break;
        }

        total_used += used_size;
        in_ptr += used_size;
        remain -= used_size;
    }

exit_thread:
    if (s_h264d_flexa_ctx != NULL) {
        (void)h264_decoder_deinit(s_h264d_flexa_ctx);
        s_h264d_flexa_ctx = NULL;
    }

    LOGI("flexa_summary(rounds=%u, total_used=%u, remain=%u, info=%u, frames=%u, wr_cb=%u)\n",
         (unsigned)decode_round,
         (unsigned)total_used,
         (unsigned)remain,
         (unsigned)s_h264d_flexa_info_count,
         (unsigned)s_h264d_flexa_frame_count,
         (unsigned)s_h264d_flexa_wr_cb_count);

    h264d_flexa_release_buffers();
    LOGI("exit h264_decode_flexa_test\n");

    s_h264d_flexa_thread = NULL;
    rtos_delete_thread(NULL);
}

void cli_h264_decode_flexa_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_err_t ret = BK_OK;

    if (argc != 2 || os_strcmp(argv[1], "start") != 0) {
        LOGE("usage: h264_decode_flexa start\n");
        h264d_flexa_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    if (s_h264d_flexa_thread != NULL) {
        LOGE("h264 decode flexa test is already running\n");
        h264d_flexa_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    ret = rtos_create_thread(&s_h264d_flexa_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "h264d_flexa",
                             (beken_thread_function_t)h264d_flexa_thread_entry,
                             H264_DECODE_FLEXA_THREAD_STACK_SIZE,
                             NULL);
    if (ret != BK_OK) {
        LOGE("create h264 decode flexa thread failed, ret=%d\n", (int)ret);
        s_h264d_flexa_thread = NULL;
        h264d_flexa_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_ERROR);
        return;
    }

    h264d_flexa_cli_write_rsp(pcWriteBuffer, xWriteBufferLen, CLI_CMD_RSP_SUCCEED);
}
