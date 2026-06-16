#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/bk_frame_buffer.h>
#include <components/log.h>
#include <common/avdk_pixel_types.h>
#include <common/bk_err.h>
#include <driver/isp.h>
#include <driver/isp_base.h>

extern void *app_isp_handle_get(void);
#include "network_transfer.h"
#include "network_type.h"
#include "common/bk_ntwk_pack/ntwk_fragmentation.h"
#include "isp_frame_capture_config.h"
#include "isp_frame_priv.h"

#define TAG "isp_frame_stream"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)

#define ISP_FRAME_CAPTURE_FRAME_COUNT_MAX  64
#define ISP_FRAME_PREVIEW_READ_TIMEOUT_MS  200
#define ISP_FRAME_PREVIEW_READ_TIMEOUT_CAP_MS  1000
#define ISP_FRAME_PREVIEW_READ_RETRY_MAX   25
#define ISP_FRAME_CAPTURE_ABORT_WAIT_MS    500

static uint16_t s_isp_frame_width = 1920;
static uint16_t s_isp_frame_height = 1080;
static isp_frame_read_cb_t s_isp_frame_read = NULL;
static frame_buffer_t s_isp_frame_desc;
static uint32_t s_isp_frame_sequence;
static uint8_t s_isp_frame_stream_active;
static beken_thread_t s_isp_frame_capture_thread;
static volatile uint8_t s_isp_frame_capture_busy;
static volatile uint8_t s_isp_frame_capture_cancel;

typedef struct
{
    uint32_t frame_count;
} isp_frame_capture_task_arg_t;

static isp_frame_capture_task_arg_t *s_isp_frame_capture_task_arg;

static int isp_frame_fragment_abort_cb(void)
{
    return s_isp_frame_capture_cancel ? 1 : 0;
}

static void isp_frame_capture_abort_register(void)
{
    (void)ntwk_fragment_register_abort_cb(NTWK_TRANS_CHAN_VIDEO, isp_frame_fragment_abort_cb);
}

static void isp_frame_capture_abort_unregister(void)
{
    (void)ntwk_fragment_register_abort_cb(NTWK_TRANS_CHAN_VIDEO, NULL);
}

void isp_frame_stream_cancel(void)
{
    s_isp_frame_capture_cancel = 1;
}

static void isp_frame_capture_abort_inflight(void)
{
    uint32_t waited_ms = 0;

    s_isp_frame_capture_cancel = 1;
    isp_frame_capture_abort_register();

    while (s_isp_frame_capture_busy && waited_ms < ISP_FRAME_CAPTURE_ABORT_WAIT_MS)
    {
        rtos_delay_milliseconds(10);
        waited_ms += 10;
    }

    if (s_isp_frame_capture_busy)
    {
        LOGW("capture task still busy after %ums\n", ISP_FRAME_CAPTURE_ABORT_WAIT_MS);
    }
    else
    {
        s_isp_frame_capture_cancel = 0;
    }
}

void isp_frame_capture_wait_idle(uint32_t timeout_ms)
{
    uint32_t waited_ms = 0;

    while (s_isp_frame_capture_busy && waited_ms < timeout_ms)
    {
        rtos_delay_milliseconds(10);
        waited_ms += 10;
    }
}

uint8_t isp_frame_stream_is_busy(void)
{
    return s_isp_frame_capture_busy;
}

void isp_frame_stream_register_read_cb(isp_frame_read_cb_t cb)
{
    s_isp_frame_read = cb;
}

void isp_frame_stream_set_profile(uint16_t width, uint16_t height)
{
    if (width > 0)
    {
        s_isp_frame_width = width;
    }
    if (height > 0)
    {
        s_isp_frame_height = height;
    }
}

static uint32_t isp_frame_size_get(void)
{
    const isp_frame_capture_config_t *cap = isp_frame_session_get_capture_config();
    bk_pixel_format_t fmt = BK_PIXEL_FORMAT_NV12;

    if (cap != NULL)
    {
        fmt = isp_frame_format_to_bk_pixel(cap->format);
    }

    return bk_image_size_get(s_isp_frame_width, s_isp_frame_height, fmt);
}

static uint32_t isp_frame_read_timeout_ms(void)
{
    const isp_frame_capture_config_t *cap = isp_frame_session_get_capture_config();
    uint32_t timeout_ms = ISP_FRAME_PREVIEW_READ_TIMEOUT_MS;

    if (cap != NULL && cap->rx_timeout > 0)
    {
        timeout_ms = isp_frame_rx_timeout_ms_for_read(cap->rx_timeout, ISP_FRAME_PREVIEW_READ_TIMEOUT_MS);
    }

    if (s_isp_frame_capture_cancel && timeout_ms > ISP_FRAME_PREVIEW_READ_TIMEOUT_MS)
    {
        timeout_ms = ISP_FRAME_PREVIEW_READ_TIMEOUT_MS;
    }

    if (timeout_ms > ISP_FRAME_PREVIEW_READ_TIMEOUT_CAP_MS)
    {
        timeout_ms = ISP_FRAME_PREVIEW_READ_TIMEOUT_CAP_MS;
    }

    return timeout_ms;
}

static void isp_frame_preview_prepare_mp_read(void)
{
    void *isp = app_isp_handle_get();

    if (isp == NULL)
    {
        return;
    }

    /*
     * H264E flexa bond routes MP through SBI; channel_read needs ring-buffer mode.
     * Disable SBI before on-demand preview (stop H264E encode if preview fails after bond).
     */
    {
        isp_handle_t isp_h = (isp_handle_t)isp;
        (void)bk_isp_flexa_sbi_config(&isp_h, ISP_MP_CHN_ID, 0);
    }
}

int isp_frame_session_send_image(uint8_t *data, uint32_t length)
{
    isp_frame_session_ctx_t *ctx = isp_frame_session_get_ctx();
    int ret;

    if (data == NULL || length == 0)
    {
        return -1;
    }

    if (!ctx->initialized || !ctx->started || !ctx->media_ready || !ctx->stream_started ||
        s_isp_frame_capture_cancel)
    {
        return -1;
    }

    os_memset(&s_isp_frame_desc, 0, sizeof(s_isp_frame_desc));
    s_isp_frame_desc.fmt = IMAGE_YUV;
    s_isp_frame_desc.frame = data;
    s_isp_frame_desc.length = length;
    s_isp_frame_desc.size = length;
    s_isp_frame_desc.width = s_isp_frame_width;
    s_isp_frame_desc.height = s_isp_frame_height;
    s_isp_frame_desc.sequence = s_isp_frame_sequence++;

    ret = ntwk_trans_video_send((uint8_t *)&s_isp_frame_desc, length, IMAGE_YUV);
    return (ret < 0) ? -1 : ret;
}

bk_err_t isp_frame_send_one_frame(void)
{
    uint32_t frame_size;
    uint8_t *frame = NULL;
    int read_ret;
    int send_ret;
    bk_err_t ret = BK_FAIL;

    if (s_isp_frame_read == NULL)
    {
        LOGE("frame read callback not registered\n");
        return BK_ERR_STATE;
    }

    if (s_isp_frame_capture_cancel)
    {
        return BK_ERR_STATE;
    }

    frame_size = isp_frame_size_get();
    frame = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, frame_size);
    if (frame == NULL)
    {
        LOGE("frame malloc failed, size=%u\n", frame_size);
        return BK_ERR_NO_MEM;
    }

    isp_frame_preview_prepare_mp_read();

    {
        uint32_t per_try_ms = isp_frame_read_timeout_ms();
        uint32_t attempt;

        read_ret = -1;
        for (attempt = 0; attempt < ISP_FRAME_PREVIEW_READ_RETRY_MAX; attempt++)
        {
            if (s_isp_frame_capture_cancel)
            {
                goto exit;
            }
            read_ret = s_isp_frame_read(frame, frame_size, per_try_ms);
            if (read_ret == 0)
            {
                break;
            }
            rtos_delay_milliseconds(40);
        }
    }

    if (read_ret != 0)
    {
        LOGW("preview read failed after %u tries, timeout=%ums each\n",
             ISP_FRAME_PREVIEW_READ_RETRY_MAX, isp_frame_read_timeout_ms());
        goto exit;
    }

    {
        uint32_t send_start_ms = rtos_get_time();

        if (s_isp_frame_capture_cancel)
        {
            goto exit;
        }

        send_ret = isp_frame_session_send_image(frame, frame_size);
        if (send_ret < 0)
        {
            if (s_isp_frame_capture_cancel)
            {
                uint8_t frame_id = (uint8_t)((s_isp_frame_sequence - 1U) & 0xFFU);

                (void)ntwk_fragment_discard_frame(NTWK_TRANS_CHAN_VIDEO, frame_id);
                LOGI("preview send aborted, discard frame_id=%u\n", frame_id);
            }
            else
            {
                LOGW("preview send failed (video TCP %u connected?)\n", NTWK_TRANS_TCP_VIDEO_PORT);
            }
            goto exit;
        }

        /* Fragment count must be <= 255 (uint8 in ntwk_fragm_head_t). */
        LOGI("preview sent seq=%u len=%u send_ms=%u (~%u frags max 255)\n",
             s_isp_frame_sequence - 1, frame_size,
             rtos_get_time() - send_start_ms,
             (frame_size + 20460 - 1) / 20460);
    }
    ret = BK_OK;

exit:
    bk_frame_buffer_free(frame);
    return ret;
}

bk_err_t isp_frame_send_frames(uint32_t frame_count)
{
    uint32_t sent = 0;
    uint32_t i;

    if (frame_count == 0)
    {
        return BK_ERR_PARAM;
    }

    if (frame_count > ISP_FRAME_CAPTURE_FRAME_COUNT_MAX)
    {
        frame_count = ISP_FRAME_CAPTURE_FRAME_COUNT_MAX;
    }

    for (i = 0; i < frame_count; i++)
    {
        if (s_isp_frame_capture_cancel)
        {
            break;
        }
        if (isp_frame_send_one_frame() != BK_OK)
        {
            break;
        }
        sent++;
        if (i + 1 < frame_count)
        {
            rtos_delay_milliseconds(40);
        }
    }

    LOGI("captureFrames done request=%u sent=%u\n", frame_count, sent);
    return (sent > 0) ? BK_OK : BK_FAIL;
}

static void isp_frame_capture_task_entry(beken_thread_arg_t arg)
{
    isp_frame_capture_task_arg_t *task_arg = (isp_frame_capture_task_arg_t *)arg;

    isp_frame_capture_abort_register();
    if (task_arg != NULL)
    {
        (void)isp_frame_send_frames(task_arg->frame_count);
        os_free(task_arg);
        s_isp_frame_capture_task_arg = NULL;
    }

    isp_frame_capture_abort_unregister();
    s_isp_frame_capture_busy = 0;
    s_isp_frame_capture_thread = NULL;
    rtos_delete_thread(NULL);
}

bk_err_t isp_frame_send_frames_async(uint32_t frame_count)
{
    isp_frame_capture_task_arg_t *task_arg;
    bk_err_t ret;

    if (frame_count == 0)
    {
        return BK_ERR_PARAM;
    }

    if (s_isp_frame_capture_busy)
    {
        return BK_ERR_STATE;
    }

    task_arg = (isp_frame_capture_task_arg_t *)os_malloc(sizeof(*task_arg));
    if (task_arg == NULL)
    {
        return BK_ERR_NO_MEM;
    }

    task_arg->frame_count = frame_count;
    s_isp_frame_capture_task_arg = task_arg;
    s_isp_frame_capture_cancel = 0;
    s_isp_frame_capture_busy = 1;
    ret = rtos_create_thread(&s_isp_frame_capture_thread,
                             7,
                             "isp_cap",
                             isp_frame_capture_task_entry,
                             4096,
                             (beken_thread_arg_t)task_arg);
    if (ret != BK_OK)
    {
        s_isp_frame_capture_busy = 0;
        s_isp_frame_capture_task_arg = NULL;
        os_free(task_arg);
        return ret;
    }

    return BK_OK;
}

/*
 * Marks the ISP frame path ready for on-demand preview/captureFrames.
 * No background thread; camera on/off is handled by media_start/media_stop.
 */
bk_err_t isp_frame_stream_start(uint16_t width, uint16_t height, uint16_t fps)
{
    isp_frame_session_ctx_t *ctx = isp_frame_session_get_ctx();

    (void)fps;
    s_isp_frame_capture_cancel = 0;
    isp_frame_stream_set_profile(width, height);
    s_isp_frame_stream_active = 1;
    ctx->stream_started = 1;
    LOGI("stream active %ux%u (on-demand)\n", s_isp_frame_width, s_isp_frame_height);
    return BK_OK;
}

bk_err_t isp_frame_stream_stop(void)
{
    isp_frame_session_ctx_t *ctx = isp_frame_session_get_ctx();

    isp_frame_capture_abort_inflight();

    if (!s_isp_frame_stream_active && !ctx->stream_started)
    {
        return BK_OK;
    }

    s_isp_frame_stream_active = 0;
    ctx->stream_started = 0;
    s_isp_frame_capture_cancel = 0;
    os_memset(&s_isp_frame_desc, 0, sizeof(s_isp_frame_desc));
    s_isp_frame_sequence = 0;
    LOGI("stream inactive\n");
    return BK_OK;
}
