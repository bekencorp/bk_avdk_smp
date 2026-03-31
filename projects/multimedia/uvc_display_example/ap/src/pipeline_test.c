#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <avdk_error.h>
#include <components/log.h>

#include <components/bk_frame_buffer.h>
#include <components/bk_uvc_camera.h>
#include <components/bk_uvc_camera_types.h>

#include "h264_encoder_api.h"

#include "decode_private.h"
#include "test_cli.h"

#define TAG "pipeline_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define PIPELINE_ENC_QUEUE_LEN 4

/* Limit I-frame dump size to avoid flooding UART logs. */
#define PIPELINE_IFRAME_DUMP_MAX_BYTES (4096U)

/* Keep consistent with h264_encoder_api.c default GOP config. */
#define PIPELINE_H264_IDR_INTERVAL (30U)

/*
 * Reserve space at the beginning of the encoder output buffer for H.264 header (SPS/PPS).
 * This allows us to prepend header bytes to the first I-frame without moving the frame data.
 */
#define PIPELINE_H264_HEADER_MAX_BYTES (256U)
#define PIPELINE_H264_OUT_HEADROOM_BYTES (PIPELINE_H264_HEADER_MAX_BYTES)

typedef enum {
    PIPELINE_ENC_MSG_FRAME_START = 0,
    PIPELINE_ENC_MSG_EXIT,
} pipeline_enc_msg_event_t;

typedef struct {
    pipeline_enc_msg_event_t event;
} pipeline_enc_msg_t;

typedef struct {
    uint8_t running;
    uint8_t port;
    uint16_t width;
    uint16_t height;
    uint16_t aligned_height;
    uint8_t fps;

    bk_uvc_ctlr_handle_t uvc_handle;

    uint8_t *decode_flexa_buf;
    uint8_t decode_ring_cnt;
    uint32_t blocks_per_frame;

    void *h264e_handle;
    beken_thread_t enc_thread;
    beken_queue_t enc_queue;

    uint8_t *enc_out_buf;
    uint32_t enc_out_size;
    uint32_t enc_frame_cnt;

    uint8_t h264_header[PIPELINE_H264_HEADER_MAX_BYTES];
    uint32_t h264_header_size;
    uint8_t dumped_first_iframe;
    uint8_t dumped_first_pframe;

    volatile uint8_t enc_busy;
} pipeline_test_ctx_t;

static pipeline_test_ctx_t *s_pipeline = NULL;

extern bk_uvc_ctlr_handle_t uvc_camera_turn_on(bk_cam_uvc_config_t *config);
extern avdk_err_t uvc_camera_turn_off(bk_uvc_ctlr_handle_t handle);

/*
 * This register is used by the encoder low-latency line-buffer flow to report
 * how many 16-line blocks have been read by the encoder.
 *
 * Keep consistent with bk_image_flexa_action.c.
 */
#define PIPELINE_H264E_RDCNT_REG_ADDR (0x4C101310U)

static void pipeline_h264e_output(uint8_t *buf, uint32_t size, uint32_t type)
{
    pipeline_test_ctx_t *ctx = s_pipeline;

    if (type == VCENC_OUT_HEADER) {
        LOGI("H264E header size=%u\n", (unsigned)size);
        if (ctx) {
            if (buf == NULL || size == 0) {
                ctx->h264_header_size = 0;
            } else if (size > (uint32_t)sizeof(ctx->h264_header)) {
                os_memcpy(ctx->h264_header, buf, sizeof(ctx->h264_header));
                ctx->h264_header_size = sizeof(ctx->h264_header);
                LOGW("H264E header truncated: total=%u cached=%u\n",
                     (unsigned)size, (unsigned)ctx->h264_header_size);
            } else {
                os_memcpy(ctx->h264_header, buf, size);
                ctx->h264_header_size = size;
            }
        }
        return;
    }

    if (type < VCENC_OUT_HEADER) {
        LOGD("H264E frame type=%u size=%u\n", (unsigned)type, (unsigned)size);
    }

    /* Mark encoder as idle once a frame is produced. */
    if (ctx && type < VCENC_OUT_HEADER) {
        ctx->enc_busy = 0;
    }

    /*
     * For every encoded I-frame, prepend cached H.264 header (SPS/PPS) into headroom.
     * This is done without moving frame bytes: outBuf is offset before encoding.
     */
    if (type == VCENC_OUT_IFRAME && ctx && buf && size) {
        if (ctx->h264_header_size == 0) {
            LOGW("H264E I-frame has no cached header\n");
        } else {
            uintptr_t start = (uintptr_t)ctx->enc_out_buf;
            uintptr_t end = start + (uintptr_t)ctx->enc_out_size;
            uintptr_t b = (uintptr_t)buf;

            if (b >= start && b < end && b >= start + (uintptr_t)ctx->h264_header_size) {
                uint8_t *head_ptr = (uint8_t *)(b - (uintptr_t)ctx->h264_header_size);
                os_memcpy(head_ptr, ctx->h264_header, ctx->h264_header_size);
            } else {
                LOGW("H264E I-frame header prepend skipped: buf=%p header=%u out=[%p..%p)\n",
                     buf, (unsigned)ctx->h264_header_size, ctx->enc_out_buf, ctx->enc_out_buf + ctx->enc_out_size);
            }
        }
    }

    return;
    /* Dump the first encoded I-frame (header + frame). */
    if (type == VCENC_OUT_IFRAME && ctx && ctx->dumped_first_iframe == 0) {
        ctx->dumped_first_iframe = 1;

        if (buf == NULL || size == 0) {
            LOGW("H264E first I-frame dump skipped: buf=%p size=%u\n", buf, (unsigned)size);
            return;
        }

        uint8_t *dump_ptr = buf;
        uint32_t dump_total = size;

        if (ctx->h264_header_size > 0) {
            uintptr_t start = (uintptr_t)ctx->enc_out_buf;
            uintptr_t end = start + (uintptr_t)ctx->enc_out_size;
            uintptr_t b = (uintptr_t)buf;
            if (b >= start && b < end && b >= start + (uintptr_t)ctx->h264_header_size) {
                dump_ptr = (uint8_t *)(b - (uintptr_t)ctx->h264_header_size);
                dump_total = ctx->h264_header_size + size;
                LOGI("H264E first I-frame dump (with header): header=%u frame=%u total=%u\n",
                     (unsigned)ctx->h264_header_size, (unsigned)size, (unsigned)dump_total);
            }
        }

        uint32_t dump_len = dump_total;
        if (dump_len > PIPELINE_IFRAME_DUMP_MAX_BYTES) {
            dump_len = PIPELINE_IFRAME_DUMP_MAX_BYTES;
            LOGW("H264E first I-frame dump truncated: total=%u dump=%u\n",
                 (unsigned)dump_total, (unsigned)dump_len);
        }

        extern void stack_mem_dump(uint32_t start, uint32_t end);
        stack_mem_dump((uint32_t)(uintptr_t)dump_ptr, (uint32_t)(uintptr_t)(dump_ptr + dump_len));
    }

    /*
     * Dump the first encoded P-frame bitstream for debugging.
     * Note: do NOT prepend SPS/PPS to P-frames (not required for standard Annex-B streams).
     */
    if (type == VCENC_OUT_PFRAME && ctx && ctx->dumped_first_pframe == 0) {
        ctx->dumped_first_pframe = 1;

        if (buf == NULL || size == 0) {
            LOGW("H264E first P-frame dump skipped: buf=%p size=%u\n", buf, (unsigned)size);
            return;
        }

        uint32_t dump_len = size;
        if (dump_len > PIPELINE_IFRAME_DUMP_MAX_BYTES) {
            dump_len = PIPELINE_IFRAME_DUMP_MAX_BYTES;
            LOGW("H264E first P-frame dump truncated: total=%u dump=%u\n",
                 (unsigned)size, (unsigned)dump_len);
        } else {
            LOGI("H264E first P-frame dump: size=%u\n", (unsigned)size);
        }

        extern void stack_mem_dump(uint32_t start, uint32_t end);
        stack_mem_dump((uint32_t)(uintptr_t)buf, (uint32_t)(uintptr_t)(buf + dump_len));
    }
}

static uint32_t pipeline_h264e_flexa_done_callback(uint8_t *yDst, uint8_t *uDst, uint8_t *vDst)
{
    (void)yDst;
    (void)uDst;
    (void)vDst;

    pipeline_test_ctx_t *ctx = s_pipeline;
    if (ctx == NULL || ctx->running == 0) {
        return 0;
    }

    uint32_t reg = *((volatile uint32_t *)PIPELINE_H264E_RDCNT_REG_ADDR);
    uint32_t rdcnt = (reg >> 10) & 0x3FFU;
    uint32_t rd_blocks = rdcnt ? rdcnt : ctx->blocks_per_frame;

    if (rd_blocks > ctx->blocks_per_frame) {
        rd_blocks = ctx->blocks_per_frame;
    }

    (void)decode_test_set_port_rd_cnt(DECODE_TEST_PORT_H264E, rd_blocks);
    return 0;
}

static void pipeline_decode_line_done_callback(uint32_t wr_cnt, void *arg)
{
    pipeline_test_ctx_t *ctx = (pipeline_test_ctx_t *)arg;
    if (ctx == NULL || ctx->running == 0) {
        return;
    }

    if (ctx->h264e_handle) {
        vcenc_flexa_input_linebuf_wrcnt_set(ctx->h264e_handle, wr_cnt);
    }

    if (wr_cnt == 1U) {
        /*
         * Only start a new encode when the previous frame is done.
         * Otherwise, reusing the same output buffer can corrupt streams and also
         * block decoder read pointer (GPU display artifacts).
         */
        if (ctx->enc_busy) {
            /* Encoder is not consuming this frame, do not block decoder. */
            (void)decode_test_set_port_rd_cnt(DECODE_TEST_PORT_H264E, ctx->blocks_per_frame);
            return;
        }

        ctx->enc_busy = 1;

        pipeline_enc_msg_t msg = {.event = PIPELINE_ENC_MSG_FRAME_START};
        bk_err_t qret = rtos_push_to_queue(&ctx->enc_queue, &msg, BEKEN_NO_WAIT);
        if (qret != BK_OK) {
            ctx->enc_busy = 0;
            (void)decode_test_set_port_rd_cnt(DECODE_TEST_PORT_H264E, ctx->blocks_per_frame);
        }
    }
}

static void pipeline_enc_thread_entry(void *arg)
{
    pipeline_test_ctx_t *ctx = (pipeline_test_ctx_t *)arg;
    pipeline_enc_msg_t msg;

    while (1) {
        bk_err_t qret = rtos_pop_from_queue(&ctx->enc_queue, &msg, BEKEN_WAIT_FOREVER);
        if (qret != BK_OK) {
            continue;
        }

        if (msg.event == PIPELINE_ENC_MSG_EXIT) {
            break;
        }

        if (msg.event != PIPELINE_ENC_MSG_FRAME_START) {
            continue;
        }

        if (ctx->h264e_handle == NULL || ctx->decode_flexa_buf == NULL) {
            continue;
        }

        /*
         * codingType follows VCEncPictureCodingType:
         * 0 = Intra frame, 1 = Predicted frame.
         *
         * Do NOT include heavy encoder public headers here, because they may require
         * additional OS abstraction types that are only available inside the encoder component.
         */
        uint32_t coding_type = (ctx->enc_frame_cnt == 0) ? 0 : 1;
        uint8_t *out_payload = NULL;
        uint32_t out_payload_size = 0;
        uint8_t need_headroom = 0;
        if (PIPELINE_H264_IDR_INTERVAL != 0U && ((ctx->enc_frame_cnt % PIPELINE_H264_IDR_INTERVAL) == 0U)) {
            need_headroom = 1;
        }

        if (need_headroom) {
            out_payload = ctx->enc_out_buf + PIPELINE_H264_OUT_HEADROOM_BYTES;
            out_payload_size = (ctx->enc_out_size > PIPELINE_H264_OUT_HEADROOM_BYTES) ? (ctx->enc_out_size - PIPELINE_H264_OUT_HEADROOM_BYTES) : 0;
            if (out_payload_size == 0) {
                LOGE("H264E invalid I-frame out buffer: total=%u headroom=%u\n",
                     (unsigned)ctx->enc_out_size, (unsigned)PIPELINE_H264_OUT_HEADROOM_BYTES);
                continue;
            }
        } else {
            /* P-frames: do not offset output pointer. */
            out_payload = ctx->enc_out_buf;
            out_payload_size = ctx->enc_out_size;
        }
        int32_t enc_ret = h264_encoder_encode(ctx->h264e_handle,
                                             (uint32_t)(uintptr_t)ctx->decode_flexa_buf,
                                             0,
                                             coding_type,
                                             (uint32_t)(uintptr_t)out_payload,
                                             out_payload_size);

        if (enc_ret == -8) {
            uint32_t new_size = ctx->enc_out_size + ctx->enc_out_size / 2;
            uint8_t *new_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, new_size);
            if (new_buf) {
                bk_frame_buffer_free(ctx->enc_out_buf);
                ctx->enc_out_buf = new_buf;
                ctx->enc_out_size = new_size;
                if (need_headroom) {
                    out_payload = ctx->enc_out_buf + PIPELINE_H264_OUT_HEADROOM_BYTES;
                    out_payload_size = (ctx->enc_out_size > PIPELINE_H264_OUT_HEADROOM_BYTES) ? (ctx->enc_out_size - PIPELINE_H264_OUT_HEADROOM_BYTES) : 0;
                } else {
                    out_payload = ctx->enc_out_buf;
                    out_payload_size = ctx->enc_out_size;
                }
                enc_ret = h264_encoder_encode(ctx->h264e_handle,
                                             (uint32_t)(uintptr_t)ctx->decode_flexa_buf,
                                             0,
                                             coding_type,
                                             (uint32_t)(uintptr_t)out_payload,
                                             out_payload_size);
            } else {
                LOGE("H264E realloc output buffer failed, new_size=%u\n", (unsigned)new_size);
            }
        }

        if (enc_ret < 0) {
            LOGW("H264E encode failed, ret=%d\n", (int)enc_ret);
            ctx->enc_busy = 0;
            (void)decode_test_set_port_rd_cnt(DECODE_TEST_PORT_H264E, ctx->blocks_per_frame);
        } else {
            ctx->enc_frame_cnt++;
        }
    }

    ctx->enc_thread = NULL;
    rtos_delete_thread(NULL);
}

static avdk_err_t pipeline_open(uint8_t port, uint16_t width, uint16_t height, uint8_t fps)
{
    avdk_err_t ret = AVDK_ERR_OK;

    if (s_pipeline != NULL) {
        LOGW("pipeline already running\n");
        return AVDK_ERR_OK;
    }

    pipeline_test_ctx_t *ctx = (pipeline_test_ctx_t *)os_malloc(sizeof(pipeline_test_ctx_t));
    if (ctx == NULL) {
        LOGE("malloc pipeline ctx failed\n");
        return AVDK_ERR_NOMEM;
    }
    os_memset(ctx, 0, sizeof(*ctx));

    ctx->running = 1;
    ctx->port = port;
    ctx->width = width;
    ctx->height = height;
    ctx->aligned_height = (uint16_t)((height + 15U) & ~15U);
    ctx->fps = fps;
    ctx->decode_flexa_buf = NULL;
    ctx->decode_ring_cnt = 0;
    ctx->blocks_per_frame = (uint32_t)ctx->aligned_height / DECODE_FLEXA_LINES;
    ctx->h264_header_size = 0;
    ctx->dumped_first_iframe = 0;
    ctx->dumped_first_pframe = 0;
    ctx->enc_busy = 0;

    /*
     * Set global context early so encoder callbacks (header/frame output) can
     * cache header bytes even before UVC streaming is started.
     */
    s_pipeline = ctx;

    /* 1) Start decoder first: GPU pipeline depends on decode flexa buffer. */
    ret = decode_test_open(width, height, BK_IMAGE_FORMAT_MJPEG, 1, 0);
    if (ret != AVDK_ERR_OK) {
        LOGE("decode_test_open failed, ret=%d\n", ret);
        goto fail;
    }

    /* 2) Open display and start GPU. */
    ret = display_test_open_with_gpu();
    if (ret != AVDK_ERR_OK) {
        LOGE("display_test_open_with_gpu failed, ret=%d\n", ret);
        goto fail;
    }

    /* 3) Get decoder Flexa ring-buffer context for low-latency encoding. */
    {
        uint8_t *decode_buf = NULL;
        uint8_t ring_cnt = 0;
        ret = decode_test_get_decode_context(&decode_buf, &ring_cnt);
        if (ret != AVDK_ERR_OK || decode_buf == NULL) {
            LOGE("decode_test_get_decode_context failed, ret=%d\n", ret);
            ret = AVDK_ERR_GENERIC;
            goto fail;
        }
        ctx->decode_flexa_buf = decode_buf;
        ctx->decode_ring_cnt = ring_cnt;
    }

    /* 4) Create encoder queue/thread and init H264 encoder (Flexa mode). */
    ret = rtos_init_queue(&ctx->enc_queue, "pipeline_h264e_q", sizeof(pipeline_enc_msg_t), PIPELINE_ENC_QUEUE_LEN);
    if (ret != BK_OK) {
        LOGE("init enc queue failed, ret=%d\n", ret);
        ret = AVDK_ERR_GENERIC;
        goto fail;
    }

    ret = rtos_create_thread(&ctx->enc_thread,
                             BEKEN_DEFAULT_WORKER_PRIORITY,
                             "pipeline_h264e",
                             (beken_thread_function_t)pipeline_enc_thread_entry,
                             1024 * 6,
                             ctx);
    if (ret != BK_OK) {
        LOGE("create enc thread failed, ret=%d\n", ret);
        ret = AVDK_ERR_GENERIC;
        goto fail;
    }

    ctx->enc_out_size = 512 * 1024;
    ctx->enc_out_buf = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_CODED, ctx->enc_out_size);
    if (ctx->enc_out_buf == NULL) {
        LOGE("malloc h264e out buffer failed, size=%u\n", (unsigned)ctx->enc_out_size);
        ret = AVDK_ERR_NOMEM;
        goto fail;
    }

    int32_t hret = h264_encoder_init(&ctx->h264e_handle,
                                     width,
                                     ctx->aligned_height,
                                     (uint32_t)VCENC_FLEXA_MODE_SOFTWARE,
                                     pipeline_h264e_flexa_done_callback,
                                     pipeline_h264e_output);
    if (hret != 0) {
        LOGE("h264_encoder_init failed, ret=%d\n", (int)hret);
        ret = AVDK_ERR_GENERIC;
        goto fail;
    }

    ret = decode_test_register_isr_callback(pipeline_decode_line_done_callback, ctx);
    if (ret != AVDK_ERR_OK) {
        LOGE("register flexa line callback failed, ret=%d\n", ret);
        ret = AVDK_ERR_GENERIC;
        goto fail;
    }

    /* 5) Open UVC and start streaming MJPEG frames into the shared queue. */
    bk_cam_uvc_config_t uvc_cfg = MEDIA_UVC_MJPEG_864X480_30FPS_CONFIG();
    uvc_cfg.port = port;
    uvc_cfg.width = width;
    uvc_cfg.height = height;
    uvc_cfg.fps = fps;
    uvc_cfg.format = BK_IMAGE_FORMAT_MJPEG;

    ctx->uvc_handle = uvc_camera_turn_on(&uvc_cfg);
    if (ctx->uvc_handle == NULL) {
        LOGE("uvc_camera_turn_on failed\n");
        ret = AVDK_ERR_GENERIC;
        goto fail;
    }

    LOGI("pipeline open ok: port=%u %ux%u fps=%u\n",
         (unsigned)port, (unsigned)width, (unsigned)height, (unsigned)fps);
    return AVDK_ERR_OK;

fail:
    if (ctx) {
        ctx->running = 0;
        if (s_pipeline == ctx) {
            s_pipeline = NULL;
        }

        if (ctx->uvc_handle) {
            uvc_camera_turn_off(ctx->uvc_handle);
            ctx->uvc_handle = NULL;
        }

        if (ctx->h264e_handle) {
            h264_encoder_deinit(ctx->h264e_handle);
            ctx->h264e_handle = NULL;
        }

        if (ctx->enc_out_buf) {
            bk_frame_buffer_free(ctx->enc_out_buf);
            ctx->enc_out_buf = NULL;
        }

        if (ctx->enc_thread) {
            pipeline_enc_msg_t emsg = {.event = PIPELINE_ENC_MSG_EXIT};
            rtos_push_to_queue(&ctx->enc_queue, &emsg, BEKEN_NO_WAIT);
            rtos_delay_milliseconds(50);
        }

        if (ctx->enc_queue) {
            rtos_deinit_queue(&ctx->enc_queue);
        }

        display_test_close();
        decode_test_close();
        os_free(ctx);
    }
    return ret;
}

static avdk_err_t pipeline_close(void)
{
    pipeline_test_ctx_t *ctx = s_pipeline;
    if (ctx == NULL) {
        return AVDK_ERR_OK;
    }

    ctx->running = 0;

    if (ctx->uvc_handle) {
        uvc_camera_turn_off(ctx->uvc_handle);
        ctx->uvc_handle = NULL;
    }

    if (ctx->enc_thread) {
        pipeline_enc_msg_t msg = {.event = PIPELINE_ENC_MSG_EXIT};
        rtos_push_to_queue(&ctx->enc_queue, &msg, BEKEN_WAIT_FOREVER);
        rtos_delay_milliseconds(50);
    }

    if (ctx->h264e_handle) {
        h264_encoder_deinit(ctx->h264e_handle);
        ctx->h264e_handle = NULL;
    }

    if (ctx->enc_out_buf) {
        bk_frame_buffer_free(ctx->enc_out_buf);
        ctx->enc_out_buf = NULL;
    }

    if (ctx->enc_queue) {
        rtos_deinit_queue(&ctx->enc_queue);
    }

    display_test_close();
    decode_test_close();

    os_free(ctx);
    s_pipeline = NULL;
    return AVDK_ERR_OK;
}

void cli_pipeline_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2) {
        LOGE("Usage: pipeline open <port> <width> <height> <fps> | pipeline close\n");
        return;
    }

    avdk_err_t ret = AVDK_ERR_OK;

    if (os_strcmp(argv[1], "open") == 0) {
        uint8_t port = 1;
        uint16_t width = 1920;
        uint16_t height = 1080;
        uint8_t fps = 30;

        if (argc >= 6) {
            port = (uint8_t)os_strtoul(argv[2], NULL, 10);
            width = (uint16_t)os_strtoul(argv[3], NULL, 10);
            height = (uint16_t)os_strtoul(argv[4], NULL, 10);
            fps = (uint8_t)os_strtoul(argv[5], NULL, 10);
        }

        ret = pipeline_open(port, width, height, fps);
    } else if (os_strcmp(argv[1], "close") == 0) {
        ret = pipeline_close();
    } else {
        LOGE("Usage: pipeline open <port> <width> <height> <fps> | pipeline close\n");
        return;
    }

    if (ret != AVDK_ERR_OK) {
        LOGE("pipeline cmd failed, ret=%d\n", ret);
    } else {
        LOGI("pipeline cmd ok\n");
    }
}

