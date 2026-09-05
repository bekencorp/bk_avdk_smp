#include <os/os.h>
#include <os/mem.h>
#include <cache.h>

#include <common/avdk_pixel_types.h>
#include <components/bk_isp_camera.h>
#include <components/bk_camera_isp_ctlr.h>
#include <components/bk_frame_buffer.h>
#include <driver/isp.h>
#include <driver/i2c.h>
#include <driver/io_matrix.h>

#include <avdk_check.h>
#include <modules/veri_isp/vsios_error.h>
#include "isp_camera_ctlr.h"
#include "isp_camera_utils.h"

#define TAG "bk_cam_isp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define ISP_CAMERA_READ_IDLE_TIMEOUT_MS 200

typedef struct
{
    bk_camera_isp_ctlr_t controller;
    /* 1 = channel owned by an external zero-copy consumer; 0 = camera reader. */
    uint8_t bond_owned[ISP_CHANNEL_INSTANCE_MAX];
} isp_camera_ctlr_private_t;

static isp_camera_ctlr_private_t *isp_camera_ctlr_get_private(bk_camera_isp_ctlr_t *control)
{
    return __containerof(control, isp_camera_ctlr_private_t, controller);
}

static void isp_camera_ctlr_task_entry(void *param)
{
    int ret = BK_FAIL;
    isp_channel_read_ctx_t *read_ctx = (isp_channel_read_ctx_t *)param;
    bk_camera_isp_ctlr_t *cam_control = NULL;
    isp_control_t *isp_control = NULL;
    isp_channel_config_t *config = NULL;

    if (read_ctx == NULL || read_ctx->controller == NULL)
    {
        LOGE("%s, invalid read context\n", __func__);
        rtos_delete_thread(NULL);
        return;
    }

    cam_control = (bk_camera_isp_ctlr_t *)read_ctx->controller;
    isp_control = (isp_control_t *)cam_control->isp_handle;
    if (isp_control == NULL)
    {
        LOGE("%s, isp_control is NULL\n", __func__);
        rtos_delete_thread(NULL);
        return;
    }

    while (read_ctx->thread_enable)
    {
        uint8_t chnl_id = read_ctx->channel;
        config = &isp_control->chn[chnl_id];
        if (config == NULL || config->enable == false)
        {
            //LOGD("%s, %d config error:%p\n", __func__, __LINE__, config);
            rtos_delay_milliseconds(10);
            continue;
        }

        if (chnl_id < ISP_CHANNEL_INSTANCE_MAX &&
            isp_camera_ctlr_get_private(cam_control)->bond_owned[chnl_id])
        {
            /* The channel queue is owned by an external zero-copy consumer; it drains
             * the queue itself, so the free-running reader must not touch it. */
            rtos_delay_milliseconds(10);
            continue;
        }

        if (config->enable_flexa)
        {
            LOGE("%s, %d channel %d flexa mode not supported read frame\n", __func__, __LINE__, chnl_id);
            rtos_delay_milliseconds(1000);
            continue;
        }

        /* Free-running recycle: keep popping completed frames even when no read is
         * pending. This keeps the FIFO done-queue drained -- so the next read always
         * gets a freshly-captured frame instead of the oldest one stuck at the queue
         * head -- and keeps buffers cycling so a frame-mode channel without keep-alive
         * (e.g. MP) never stalls on buffer exhaustion. A frame is copied out only when
         * a reader is waiting; otherwise it is simply requeued. */
        VIDEO_BUF_S buf;
        ret = isp_control->pop_buf(config->channel, &buf, read_ctx->read_timeout);
        if (ret == VSI_ERR_NOT_READY)
        {
            /* StreamOff aborted DQBUF; exit cam_thread cleanly. The per-channel reader is
             * recreated on the next channel open (isp_camera_ctlr_start_channel_reader). */
            read_ctx->thread_enable = false;
            break;
        }
        if (ret != BK_OK)
        {
            LOGW("%s, channel %d pop_buf timeout %dms ret=%d\n", __func__, chnl_id, read_ctx->read_timeout, ret);
            continue;
        }

        if (read_ctx->read_enable && read_ctx->frame)
        {
            uint32_t copied = 0;
            uint8_t plane_cnt = buf.numPlanes ? buf.numPlanes : 1;

            if (plane_cnt > VIDEO_MAX_PLANES)
            {
                plane_cnt = VIDEO_MAX_PLANES;
            }

            for (uint8_t p = 0; p < plane_cnt; p++)
            {
                uint8_t *src = (uint8_t *)(uintptr_t)buf.planes[p].dmaPhyAddr;
                uint32_t plen = buf.planes[p].size;

                if (buf.planes[p].pUserAddr != NULL)
                {
                    src = (uint8_t *)buf.planes[p].pUserAddr;
                }

                if (src == NULL || plen == 0)
                {
                    continue;
                }

                if (copied + plen > read_ctx->size)
                {
                    LOGE("%s, frame size overflow, %u + %u > %u\n",
                         __func__, copied, plen, read_ctx->size);
                    copied = 0;
                    break;
                }

                arch_dcache_flush_and_invd_range(src, plen);
                os_memcpy(read_ctx->frame + copied, src, plen);
                copied += plen;
            }

            if (copied == 0)
            {
                LOGE("%s, no plane data copied\n", __func__);
                read_ctx->size = 0;
            }

            read_ctx->read_enable = false;
            rtos_set_semaphore(&read_ctx->sem);
        }

        isp_control->free_buf(config->channel, &buf);
    }

    rtos_delete_thread(NULL);
}


static void camera_frame_complete_callback(uint32_t seqence, uint32_t line, uint8_t chnl, uint8_t ok, void *param)
{

}

static avdk_err_t isp_camera_ctlr_start_channel_reader(bk_camera_isp_ctlr_t *control, uint8_t channel)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;
    isp_channel_read_ctx_t *read_ctx = NULL;

    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(channel < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "channel out of range");

    read_ctx = &control->read_ctx[channel];
    if (read_ctx->thread_enable)
    {
        return AVDK_ERR_OK;
    }

    read_ctx->channel = channel;
    read_ctx->controller = control;
    read_ctx->read_timeout = ISP_CAMERA_READ_IDLE_TIMEOUT_MS;
    read_ctx->read_enable = false;
    read_ctx->frame = NULL;
    read_ctx->size = 0;

    ret = rtos_init_semaphore(&read_ctx->sem, 1);
    if (ret != BK_OK)
    {
        LOGE("%s, %d channel %d sem init error\n", __func__, __LINE__, channel);
        return ret;
    }

    read_ctx->thread_enable = true;
    ret = rtos_create_hsram_thread(&read_ctx->thread,
                            BEKEN_DEFAULT_WORKER_PRIORITY,
                            "cam_reader",
                            (beken_thread_function_t)isp_camera_ctlr_task_entry,
                            1024 * 2,
                            (beken_thread_arg_t)read_ctx);
    if (ret != BK_OK)
    {
        LOGE("%s, %d channel %d cam task create fail\n", __func__, __LINE__, channel);
        read_ctx->thread_enable = false;
        if (read_ctx->sem)
        {
            rtos_deinit_semaphore(&read_ctx->sem);
            read_ctx->sem = NULL;
        }
        return ret;
    }

    LOGI("%s, channel %d reader started\n", __func__, channel);
    return AVDK_ERR_OK;
}

static void isp_camera_ctlr_stop_channel_reader(bk_camera_isp_ctlr_t *control, uint8_t channel)
{
    isp_channel_read_ctx_t *read_ctx = NULL;

    if (control == NULL || channel >= ISP_CHANNEL_INSTANCE_MAX)
    {
        return;
    }

    read_ctx = &control->read_ctx[channel];
    /* Clear the run flag, then wake the reader immediately: if it is blocked in a
     * (possibly long) pop_buf/DQBUF poll, abort that DQBUF so it returns NOT_READY
     * at once and exits, instead of waiting out the poll timeout. The abort is a
     * VB-queue StreamOff only -- it neither stops the MI DMA nor frees buffers, so
     * the subsequent bk_isp_close still performs the full DisableChn + buffer free
     * (and buffers are only freed after this join confirms the reader has stopped). */
    read_ctx->thread_enable = false;
    if (read_ctx->thread)
    {
        if (control->isp_handle)
        {
            bk_isp_dqbuf_abort(&control->isp_handle, channel);
        }
        rtos_thread_join(&read_ctx->thread);
        read_ctx->thread = NULL;
    }

    if (read_ctx->sem)
    {
        rtos_deinit_semaphore(&read_ctx->sem);
        read_ctx->sem = NULL;
    }

    read_ctx->read_enable = false;
    read_ctx->frame = NULL;
    read_ctx->size = 0;
    read_ctx->read_timeout = 0;
}

static void isp_camera_ctlr_stop_all_readers(bk_camera_isp_ctlr_t *control)
{
    if (control == NULL)
    {
        return;
    }

    for (uint8_t channel = 0; channel < ISP_CHANNEL_INSTANCE_MAX; channel++)
    {
        isp_camera_ctlr_stop_channel_reader(control, channel);
    }
}

static bk_err_t isp_camera_ctlr_dev_init(bk_isp_camera_ctlr_handle_t handle)
{
    bk_camera_isp_ctlr_t *control =  __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    control->state = CAM_FSM_INIT;
    AVDK_RETURN_ON_ERROR(bk_isp_dev_init(&control->isp_handle), TAG, "exe fail");

    return AVDK_ERR_OK;
}

static bk_err_t isp_camera_ctlr_port_init(bk_isp_camera_ctlr_handle_t handle, void *config)
{
    bk_camera_isp_ctlr_t *control =  __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    control->state = CAM_FSM_INIT;
    bk_isp_camera_ctlr_config_t *ctrl_cfg = (bk_isp_camera_ctlr_config_t *)config;

    if (ctrl_cfg->port_id >= ISP_PORT_CNT)
    {
        LOGE("error port id\r\n");
        return BK_FAIL;
    }

    control->attr[ctrl_cfg->port_id].snsRect.top = ctrl_cfg->input_rect.top;
    control->attr[ctrl_cfg->port_id].snsRect.left = ctrl_cfg->input_rect.left;
    control->attr[ctrl_cfg->port_id].snsRect.width = ctrl_cfg->input_rect.width;
    control->attr[ctrl_cfg->port_id].snsRect.height = ctrl_cfg->input_rect.height;

    control->attr[ctrl_cfg->port_id].inFormRect.top = ctrl_cfg->input_crop.top;
    control->attr[ctrl_cfg->port_id].inFormRect.left = ctrl_cfg->input_crop.left;
    control->attr[ctrl_cfg->port_id].inFormRect.width = ctrl_cfg->input_crop.width;
    control->attr[ctrl_cfg->port_id].inFormRect.height = ctrl_cfg->input_crop.height;
    control->attr[ctrl_cfg->port_id].outFormRect.top = ctrl_cfg->input_crop.top;
    control->attr[ctrl_cfg->port_id].outFormRect.left = ctrl_cfg->input_crop.left;
    control->attr[ctrl_cfg->port_id].outFormRect.width = ctrl_cfg->input_crop.width;
    control->attr[ctrl_cfg->port_id].outFormRect.height = ctrl_cfg->input_crop.height;
    control->attr[ctrl_cfg->port_id].iSRect.top = ctrl_cfg->input_crop.top;
    control->attr[ctrl_cfg->port_id].iSRect.left = ctrl_cfg->input_crop.left;
    control->attr[ctrl_cfg->port_id].iSRect.width = ctrl_cfg->input_crop.width;
    control->attr[ctrl_cfg->port_id].iSRect.height = ctrl_cfg->input_crop.height;
    control->attr[ctrl_cfg->port_id].snsFps = ctrl_cfg->fps * ISP_SNS_FPS_ACCU;
    control->attr[ctrl_cfg->port_id].ispInputType = ctrl_cfg->input_type;
    control->attr[ctrl_cfg->port_id].ispMode = ctrl_cfg->isp_mode;
    control->attr[ctrl_cfg->port_id].hdrMode = ctrl_cfg->hdr_mode;
    control->attr[ctrl_cfg->port_id].pixelFormat = isp_camera_format_convert(ctrl_cfg->input_pixel_fmt);
    control->attr[ctrl_cfg->port_id].pSnsObj = (void*)ctrl_cfg->sensor_object;
    control->attr[ctrl_cfg->port_id].port_id = ctrl_cfg->port_id;

    LOGI("port id %d\r\n", ctrl_cfg->port_id);
    AVDK_RETURN_ON_ERROR(bk_isp_port_init(&control->isp_handle, &(control->attr[ctrl_cfg->port_id])), TAG, "exe fail");

    return AVDK_ERR_OK;
}

static bk_err_t isp_camera_ctlr_port_change(bk_isp_camera_ctlr_handle_t handle)
{
    bk_camera_isp_ctlr_t *control =  __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    AVDK_RETURN_ON_ERROR(bk_isp_port_change(&control->isp_handle, control->chnl), TAG, "exe fail");
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_deinit(bk_isp_camera_ctlr_handle_t handle)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;

    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control != NULL, ret, TAG, "control is NULL");

    if (control->state != CAM_FSM_INIT)
    {
        LOGE("%s, %d camera not in init state\n", __func__, __LINE__);
        return ret;
    }

    /* Stop per-channel readers if still alive (may already have exited on StreamOff). */
    isp_camera_ctlr_stop_all_readers(control);

    // Deinitialize ISP core resources (threads, buffers, semaphores, etc.)
    if (control->isp_handle)
    {
        bk_err_t isp_ret = bk_isp_deinit(&control->isp_handle);
        if (isp_ret != BK_OK)
        {
            LOGW("%s, %d, failed to deinit ISP core: %d\n", __func__, __LINE__, isp_ret);
        }
        control->isp_handle = NULL;
    }

    ret = BK_OK;

    return ret;
}

static avdk_err_t isp_camera_csi_sensor_open(bk_camera_isp_ctlr_t *control, void *is_handler)
{
    if (control->state != CAM_FSM_INIT)
    {
        LOGE("%s, %d camera not init\n", __func__, __LINE__);
        return AVDK_ERR_GENERIC;
    }

    control->isp_handle = is_handler;
    control->state = CAM_FSM_ENABLE;

    LOGI("%s, %d\n", __func__, __LINE__);

    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_read(bk_isp_camera_ctlr_handle_t handle, uint16_t id, uint8_t *frame, uint32_t size, uint32_t timeout)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;

    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control != NULL, ret, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(id < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "channel out of range");
    AVDK_RETURN_ON_FALSE(isp_camera_ctlr_get_private(control)->bond_owned[id] == 0,
                         AVDK_ERR_BUSY, TAG,
                         "channel is owned by an external consumer");

    isp_channel_read_ctx_t *read_ctx = &control->read_ctx[id];

    if (control->state != CAM_FSM_ENABLE || read_ctx->thread_enable == false)
    {
        LOGE("%s, %d camera channel %d not enable\n", __func__, __LINE__, id);
        return ret;
    }

    isp_control_t *isp_control = (isp_control_t *)control->isp_handle;

    if (control->read_register == false)
    {
        // register frame end cb
        bk_isp_register_isr_callback((isp_handle_t *)&isp_control, ISP_FRAME_END_DONE,
            camera_frame_complete_callback, control);

        control->read_register = true;
    }

    if (read_ctx->read_enable)
    {
        LOGW("%s, %d channel %d state error!\n", __func__, __LINE__, id);
        return ret;
    }

    /* A previous read that timed out while the reader was mid-delivery can leave
     * one stale completion token in sem (the reader posts after read() gave up).
     * Reads are serialized by the read_enable busy-check above and read_enable is
     * false here, so the reader cannot post again during this window -> at most one
     * stale token can exist. Consume it (non-blocking, bounded) so this read only
     * wakes on its own freshly delivered frame. */
    (void)rtos_get_semaphore(&read_ctx->sem, 0);

    read_ctx->frame = frame;
    read_ctx->size = size;
    uint32_t wait_timeout = timeout ? timeout : ISP_CAMERA_READ_IDLE_TIMEOUT_MS;
    read_ctx->read_timeout = wait_timeout;
    /* Publish read_enable last so the free-running reader never observes it true
     * before frame/size are set. The reader hands us the next freshly popped
     * frame and posts sem. */
    read_ctx->read_enable = true;

    ret = rtos_get_semaphore(&read_ctx->sem, wait_timeout);
    if (ret != BK_OK)
    {
        LOGW("%s, %d channel %d read timeout %dms\n", __func__, __LINE__, id, wait_timeout);
        read_ctx->read_enable = false;
    }

    if (read_ctx->size == 0)
    {
        LOGW("%s, %d, frame size is 0\n", __func__, __LINE__);
        ret = AVDK_ERR_GENERIC;
    }

    read_ctx->size = 0;
    read_ctx->read_enable = false;
    read_ctx->frame = NULL;

    return ret;
}

static avdk_err_t isp_camera_ctlr_delete(bk_isp_camera_ctlr_handle_t handle)
{
    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");

    os_free(isp_camera_ctlr_get_private(control));

    return AVDK_ERR_OK;
}


/* Validate a live SP stream before open returns, and re-arm a HW metastable "dead-arm" in
 * place, so a plain channel_open() hands back an already-streaming channel and the caller
 * needs no probe/retry of its own.
 *
 * A frame-mode (non-flexa) channel such as SP can arm into a metastable state when brought up
 * while the MP flexa path is free-running: it emits the FIRST frame, then stalls mid-frame
 * (no further frame-end). A single-frame check would pass such a channel, yet the caller then
 * reads 0 frames for the rest of the session. So read ISP_CAM_ARM_PROBE_FRAMES consecutive
 * frames to confirm the stream; if that fails, close+reopen (which pulses the SP-only SRSZ
 * soft-reset) and retry. Everything runs in the caller's task context -- never the ISR, never
 * touching frame-shared units -- so the concurrent MP flexa display is left undisturbed. */
#define ISP_CAM_ARM_PROBE_FRAMES  3
#define ISP_CAM_ARM_RETRY         24
#define ISP_CAM_ARM_BACKOFF_MS    20
#define ISP_CAM_ARM_PROBE_TMO_MS  1000

static avdk_err_t isp_camera_ctlr_arm_probe_sustain(bk_camera_isp_ctlr_t *control, uint8_t channel,
                                                    isp_config_ext_t *isp_config,
                                                    bk_isp_camera_channel_config_t *cfg)
{
    uint32_t fsize;

    if (cfg->format == BK_PIXEL_FORMAT_RGB888 || cfg->format == BK_PIXEL_FORMAT_BGR888)
    {
        /* ISP outputs RGB888/BGR888 as RGBX (4 bytes/pixel). */
        fsize = (uint32_t)cfg->width * cfg->height * 4U;
    }
    else
    {
        fsize = bk_image_size_get(cfg->width, cfg->height, (bk_pixel_format_t)cfg->format);
    }
    if (fsize == 0U)
    {
        return AVDK_ERR_INVAL;
    }

    uint8_t *probe = (uint8_t *)bk_frame_buffer_malloc(MEM_SLAB_HEAP_UNCODED, fsize);
    if (probe == NULL)
    {
        LOGE("%s, %d probe malloc %u failed\n", __func__, __LINE__, fsize);
        return AVDK_ERR_NO_RESOURCE;
    }

    avdk_err_t ret = AVDK_ERR_GENERIC;
    for (int att = 1; att <= ISP_CAM_ARM_RETRY; att++)
    {
        int got = 0;
        for (; got < ISP_CAM_ARM_PROBE_FRAMES; got++)
        {
            ret = isp_camera_ctlr_read(&control->ops, channel, probe, fsize, ISP_CAM_ARM_PROBE_TMO_MS);
            if (ret != AVDK_ERR_OK)
            {
                break;
            }
        }
        if (got >= ISP_CAM_ARM_PROBE_FRAMES)
        {
            if (att > 1)
            {
                LOGW("%s, ch%d arm ok after att=%d\n", __func__, channel, att);
            }
            bk_frame_buffer_free(probe);
            return AVDK_ERR_OK;
        }

        /* got<N: the stream stalled. A full stream off/on drives the SDK SRSZ soft-reset that
         * re-arms the SP self-path, then reopen (task context, so MP flexa is undisturbed).
         * Under the per-channel reader model, stream-off makes the reader thread self-exit and
         * bk_isp_open does not recreate it, so stop it explicitly first and restart it after
         * reopen -- mirroring the channel_open/channel_close pairing -- so the next probe read
         * has a live consumer. */
        LOGW("%s, ch%d arm att=%d probe got=%d/%d, reopen\n",
             __func__, channel, att, got, ISP_CAM_ARM_PROBE_FRAMES);
        isp_camera_ctlr_stop_channel_reader(control, channel);
        bk_isp_close(&control->isp_handle, channel);
        rtos_delay_milliseconds(ISP_CAM_ARM_BACKOFF_MS + (att & 0x7));
        if (bk_isp_open(&control->isp_handle, isp_config) != BK_OK)
        {
            LOGW("%s, ch%d reopen failed att=%d\n", __func__, channel, att);
            continue;
        }
        if (isp_camera_ctlr_start_channel_reader(control, channel) != AVDK_ERR_OK)
        {
            LOGW("%s, ch%d reader restart failed att=%d\n", __func__, channel, att);
            continue;
        }
    }

    bk_frame_buffer_free(probe);
    return ret;
}

static avdk_err_t isp_camera_ctlr_channel_open(bk_isp_camera_ctlr_handle_t handle, uint8_t channel, bk_isp_camera_channel_config_t *config)
{
    avdk_err_t ret = AVDK_ERR_OK;

    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(channel < 2, AVDK_ERR_INVAL, TAG, "channel out of range");

    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");

    LOGI("%s, %d, chnl_id: %d\n", __func__, __LINE__, channel);

    if (controller->channel_state[channel] != ISP_CHANNEL_STATE_TURN_OFF)
    {
        LOGE("%s, %d, channel %d state busy %d\n", __func__, __LINE__, channel, controller->channel_state[channel]);
        return AVDK_ERR_GENERIC;
    }

    controller->channel_state[channel] = ISP_CHANNEL_STATE_TURNING_ON;

    //return false
    AVDK_RETURN_ON_FALSE(config, AVDK_ERR_NO_RESOURCE, TAG, AVDK_ERR_NO_RESOURCE_TEXT);

    //TODO
    isp_config_ext_t isp_config = {
        .buf_cnt = config->buf_cnt,
        .chnl_id = channel,
        .port_id = config->port_id,
        .enable_flexa = config->enable_flexa,
        .work_mode = config->work_mode,
        .width = config->width,
        .height = config->height,
        .format = isp_camera_format_convert(config->format),
        .skip_frames = controller->skip_frames[channel],
    };

    LOGI("%s, buf_cnt: %d, chnl_id: %d, port_id: %d, enable_flexa: %d, work_mode: %d, width: %d, height: %d, format: %d, skip_frames: %d\n",
        __func__,
        isp_config.buf_cnt,
        isp_config.chnl_id,
        isp_config.port_id,
        isp_config.enable_flexa,
        isp_config.work_mode,
        isp_config.width,
        isp_config.height,
        isp_config.format,
        isp_config.skip_frames
    );

    AVDK_RETURN_ON_ERROR(bk_isp_open(&controller->isp_handle, &isp_config), TAG, "exe fail");
    controller->chnl = channel;

    if (isp_config.work_mode == 0)
    {
        if (controller->sensor_ctlr == 0)
        {
            ret = isp_camera_csi_sensor_open(controller, controller->isp_handle);
            if (ret != AVDK_ERR_OK)
            {
                (void)bk_isp_close(&controller->isp_handle, channel);
                controller->channel_state[channel] = ISP_CHANNEL_STATE_TURN_OFF;
                return ret;
            }
            controller->sensor_ctlr++;
        }

        ret = isp_camera_ctlr_start_channel_reader(controller, channel);
        if (ret != AVDK_ERR_OK)
        {
            (void)bk_isp_close(&controller->isp_handle, channel);
            controller->channel_state[channel] = ISP_CHANNEL_STATE_TURN_OFF;
            return ret;
        }
    }

    /* Frame-mode (non-flexa) channel (SP): confirm a live stream and re-arm a metastable
     * dead-arm internally, so the caller never sees a half-dead (0-frame) session. */
    if (isp_config.enable_flexa == 0 && isp_config.work_mode == 0 && channel != ISP_MP_CHN_ID)
    {
        if (isp_camera_ctlr_arm_probe_sustain(controller, channel, &isp_config, config) != AVDK_ERR_OK)
        {
            LOGE("%s, %d, channel %d failed to arm (dead-arm) after %d attempts\n",
                 __func__, __LINE__, channel, ISP_CAM_ARM_RETRY);
            bk_isp_close(&controller->isp_handle, channel);
            controller->channel_state[channel] = ISP_CHANNEL_STATE_TURN_OFF;
            return AVDK_ERR_GENERIC;
        }
    }

    controller->channel_state[channel] = ISP_CHANNEL_STATE_TURN_ON;

    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_channel_close(bk_isp_camera_ctlr_handle_t handle, uint8_t channel)
{
    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(channel < 2, AVDK_ERR_INVAL, TAG, "channel out of range");

    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);

    if (controller->channel_state[channel] != ISP_CHANNEL_STATE_TURN_ON)
    {
        LOGE("%s, %d, channel %d state not turn on %d\n", __func__, __LINE__, channel, controller->channel_state[channel]);
        return AVDK_ERR_GENERIC;
    }

    controller->channel_state[channel] = ISP_CHANNEL_STATE_TURNING_OFF;

    isp_camera_ctlr_stop_channel_reader(controller, channel);

    // Close ISP channel
    uint8_t chnl_id = channel;
    bk_err_t ret = bk_isp_close(&controller->isp_handle, chnl_id);
    if (ret != BK_OK)
    {
        LOGE("%s, %d, failed to close ISP channel %d\n", __func__, __LINE__, chnl_id);
        return AVDK_ERR_GENERIC;
    }

    controller->channel_state[channel] = ISP_CHANNEL_STATE_TURN_OFF;

    LOGI("%s, %d, channel %d closed\n", __func__, __LINE__, channel);


    if (controller->channel_state[ISP_MP_CHN_ID] == ISP_CHANNEL_STATE_TURN_OFF
        && controller->channel_state[ISP_SP_CHN_ID] == ISP_CHANNEL_STATE_TURN_OFF
        && controller->state == CAM_FSM_ENABLE)
    {
        controller->state = CAM_FSM_INIT;
        controller->sensor_ctlr = 0;
        LOGI("%s, %d, all channels closed, state changed to INIT\n", __func__, __LINE__);
    }

    return AVDK_ERR_OK;
}

static bk_isp_camera_channel_state_t isp_camera_ctlr_channel_state_get(bk_isp_camera_ctlr_handle_t handle, uint8_t channel)
{
    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(channel < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "channel out of range");
    return controller->channel_state[channel];
}

/**
 * @brief Register ISP interrupt service routine callback
 * @param handle Camera controller handle
 * @param type ISR type
 * @param cb ISR callback function
 * @param arg Callback argument
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_camera_ctlr_register_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, bk_camera_isr_t cb, void *arg)
{
    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->isp_handle, AVDK_ERR_INVAL, TAG, "isp_handle is NULL");
    AVDK_RETURN_ON_FALSE(cb, AVDK_ERR_INVAL, TAG, "cb is NULL");
    AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "arg is NULL");

    bk_err_t ret = bk_isp_register_isr_callback(&controller->isp_handle, (isp_isr_type_t)type, cb, arg);
    AVDK_RETURN_ON_ERROR((avdk_err_t)ret, TAG, "bk_isp_register_isr_callback failed");

    LOGI("%s: registered ISR callback type=%d\n", __func__, type);
    return AVDK_ERR_OK;
}

/**
 * @brief Deregister ISP interrupt service routine callback
 * @param handle Camera controller handle
 * @param type ISR type
 * @param arg Callback argument
 * @return AVDK_ERR_OK on success, error code otherwise
 */
static avdk_err_t isp_camera_ctlr_deregister_isr_callback(bk_isp_camera_ctlr_handle_t handle, bk_camera_isr_type_t type, void *arg)
{
    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->isp_handle, AVDK_ERR_INVAL, TAG, "isp_handle is NULL");
    AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "arg is NULL");

    bk_err_t ret = bk_isp_deregister_isr_callback(&controller->isp_handle, type, arg);
    AVDK_RETURN_ON_ERROR((avdk_err_t)ret, TAG, "bk_isp_deregister_isr_callback failed");

    LOGI("%s: deregistered ISR callback type=%d\n", __func__, type);
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_channel_acquire(bk_isp_camera_ctlr_handle_t handle, uint8_t channel);
static avdk_err_t isp_camera_ctlr_channel_release(bk_isp_camera_ctlr_handle_t handle, uint8_t channel);
static avdk_err_t isp_camera_ctlr_frame_pop(bk_isp_camera_ctlr_handle_t handle, uint8_t channel,
                                            bk_isp_camera_frame_info_t *info, uint32_t timeout);
static avdk_err_t isp_camera_ctlr_frame_qbuf(bk_isp_camera_ctlr_handle_t handle, uint8_t channel,
                                             uint8_t index);

static avdk_err_t isp_camera_ctlr_ioctl(bk_isp_camera_ctlr_handle_t handle, bk_cam_interface_ioctl_t ioctl, void *arg)
{
    bk_camera_isp_ctlr_t *controller = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(controller, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(controller->isp_handle, AVDK_ERR_INVAL, TAG, "isp_handle is NULL");

    switch (ioctl)
    {
        case BK_CAM_IOCTL_SOFTRESET:
            bk_isp_soft_reset(&controller->isp_handle);
            break;

        case BK_CAM_IOCTL_GET_EXPOSURE_LUMINANCE:
            AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "arg is NULL");
            AVDK_RETURN_ON_ERROR(
                bk_isp_get_exposure_luminance(&controller->isp_handle, (uint32_t *)arg),
                TAG, "get exposure luminance failed");
            break;

        case BK_CAM_IOCTL_QUERY_EXPOSURE_INFO:
            AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "exposure info arg is NULL");
            AVDK_RETURN_ON_ERROR(
                bk_isp_query_exposure_info(
                    &controller->isp_handle,
                    (bk_isp_camera_exposure_info_t *)arg),
                TAG, "query exposure info failed");
            break;

        case BK_CAM_IOCTL_SET_INITIAL_EXPOSURE:
            AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "initial exposure arg is NULL");
            AVDK_RETURN_ON_ERROR(
                bk_isp_set_initial_exposure(
                    &controller->isp_handle,
                    (const bk_isp_camera_exposure_info_t *)arg),
                TAG, "set initial exposure failed");
            break;

        case BK_CAM_IOCTL_RESUME_AUTO_EXPOSURE:
            AVDK_RETURN_ON_ERROR(
                bk_isp_resume_auto_exposure(&controller->isp_handle),
                TAG, "resume auto exposure failed");
            break;

        case BK_CAM_IOCTL_SET_SKIP_FRAMES:
        {
            bk_isp_camera_skip_frames_config_t *cfg = (bk_isp_camera_skip_frames_config_t *)arg;
            AVDK_RETURN_ON_FALSE(cfg, AVDK_ERR_INVAL, TAG, "skip_frames arg is NULL");
            AVDK_RETURN_ON_FALSE(cfg->channel < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "skip_frames channel invalid");
            controller->skip_frames[cfg->channel] = cfg->count;
            break;
        }

        case BK_CAM_IOCTL_GET_CPROC:
            AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "cproc get arg is NULL");
            AVDK_RETURN_ON_ERROR(
                bk_isp_get_cproc_attr(&controller->isp_handle, arg),
                TAG, "get cproc attr failed");
            break;

        case BK_CAM_IOCTL_SET_CPROC:
            AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "cproc set arg is NULL");
            AVDK_RETURN_ON_ERROR(
                bk_isp_set_cproc_attr(&controller->isp_handle, arg),
                TAG, "set cproc attr failed");
            break;

        case BK_CAM_IOCTL_CHANNEL_ACQUIRE:
            AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "channel acquire arg is NULL");
            return isp_camera_ctlr_channel_acquire(handle, *(uint8_t *)arg);

        case BK_CAM_IOCTL_CHANNEL_RELEASE:
            AVDK_RETURN_ON_FALSE(arg, AVDK_ERR_INVAL, TAG, "channel release arg is NULL");
            return isp_camera_ctlr_channel_release(handle, *(uint8_t *)arg);

        case BK_CAM_IOCTL_FRAME_POP:
        {
            bk_isp_camera_frame_info_t *info = (bk_isp_camera_frame_info_t *)arg;
            AVDK_RETURN_ON_FALSE(info, AVDK_ERR_INVAL, TAG, "frame pop arg is NULL");
            return isp_camera_ctlr_frame_pop(handle, info->channel, info, info->timeout);
        }

        case BK_CAM_IOCTL_FRAME_QBUF:
        {
            bk_isp_camera_frame_info_t *info = (bk_isp_camera_frame_info_t *)arg;
            AVDK_RETURN_ON_FALSE(info, AVDK_ERR_INVAL, TAG, "frame qbuf arg is NULL");
            return isp_camera_ctlr_frame_qbuf(handle, info->channel, info->index);
        }

        default:
            return AVDK_ERR_INVAL;
    }
    return AVDK_ERR_OK;
}

/* Rebuild a VIDEO_BUF_S for a given frame-pool index so it can be re-queued
 * (QBUF) to the ISP. Mirrors bk_isp_complete_buffer_config's per-buffer layout:
 * plane[0] at frame_buffer[index], subsequent planes contiguous. */
static bk_err_t isp_camera_build_video_buf(isp_control_t *isp_control, uint8_t chnl,
                                           uint8_t index, VIDEO_BUF_S *buf)
{
    if (isp_control == NULL || buf == NULL || index >= ISP_FRAME_CNT_MAX ||
        isp_control->chn[chnl].frame_buffer[index] == NULL)
    {
        return BK_FAIL;
    }

    os_memset(buf, 0, sizeof(*buf));
    buf->index = index;
    buf->numPlanes = isp_control->chn[chnl].chn_attr.chnFormat.numPlanes;
    if (buf->numPlanes == 0 || buf->numPlanes > VIDEO_MAX_PLANES)
    {
        buf->numPlanes = 1;
    }

    for (vsi_u8_t p = 0; p < buf->numPlanes; p++)
    {
        buf->planes[p].size = isp_control->chn[chnl].chn_attr.chnFormat.planeFmt[p].size;
    }
    buf->planes[0].dmaPhyAddr = (vsi_dma_t)(uintptr_t)isp_control->chn[chnl].frame_buffer[index];
    for (vsi_u8_t p = 1; p < buf->numPlanes; p++)
    {
        buf->planes[p].dmaPhyAddr = buf->planes[p - 1].dmaPhyAddr + buf->planes[p - 1].size;
    }

    return BK_OK;
}

static avdk_err_t isp_camera_ctlr_channel_acquire(bk_isp_camera_ctlr_handle_t handle, uint8_t channel)
{
    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(channel < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "channel out of range");

    isp_camera_ctlr_get_private(control)->bond_owned[channel] = 1;
    LOGI("%s, channel %d acquired by external consumer\n", __func__, channel);
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_channel_release(bk_isp_camera_ctlr_handle_t handle, uint8_t channel)
{
    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(channel < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "channel out of range");

    isp_camera_ctlr_get_private(control)->bond_owned[channel] = 0;
    LOGI("%s, channel %d released back to cam_thread\n", __func__, channel);
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_frame_pop(bk_isp_camera_ctlr_handle_t handle, uint8_t channel,
                                            bk_isp_camera_frame_info_t *info, uint32_t timeout)
{
    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(info, AVDK_ERR_INVAL, TAG, "info is NULL");
    AVDK_RETURN_ON_FALSE(channel < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "channel out of range");

    isp_control_t *isp_control = (isp_control_t *)control->isp_handle;
    AVDK_RETURN_ON_FALSE(isp_control && isp_control->pop_buf, AVDK_ERR_INVAL, TAG, "isp pop_buf NULL");

    VIDEO_BUF_S buf;
    os_memset(&buf, 0, sizeof(buf));
    int ret = isp_control->pop_buf(isp_control->chn[channel].channel, &buf, timeout);
    if (ret != BK_OK)
    {
        return AVDK_ERR_GENERIC;
    }

    uint32_t addr = (uint32_t)(uintptr_t)buf.planes[0].dmaPhyAddr;
    if (buf.planes[0].pUserAddr != NULL)
    {
        addr = (uint32_t)(uintptr_t)buf.planes[0].pUserAddr;
    }

    uint32_t frame_size = buf.imageSize;
    if (frame_size == 0)
    {
        uint8_t plane_cnt = buf.numPlanes ? buf.numPlanes : 1;
        if (plane_cnt > VIDEO_MAX_PLANES)
        {
            plane_cnt = VIDEO_MAX_PLANES;
        }
        for (uint8_t p = 0; p < plane_cnt; p++)
        {
            frame_size += buf.planes[p].size;
        }
    }

    info->frame_addr = addr;
    info->frame_size = frame_size;
    info->channel = channel;
    info->index = (uint8_t)buf.index;
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_ctlr_frame_qbuf(bk_isp_camera_ctlr_handle_t handle, uint8_t channel, uint8_t index)
{
    bk_camera_isp_ctlr_t *control = __containerof(handle, bk_camera_isp_ctlr_t, ops);
    AVDK_RETURN_ON_FALSE(control, AVDK_ERR_INVAL, TAG, "control is NULL");
    AVDK_RETURN_ON_FALSE(channel < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "channel out of range");

    isp_control_t *isp_control = (isp_control_t *)control->isp_handle;
    AVDK_RETURN_ON_FALSE(isp_control && isp_control->free_buf, AVDK_ERR_INVAL, TAG, "isp free_buf NULL");

    VIDEO_BUF_S buf;
    if (isp_camera_build_video_buf(isp_control, channel, index, &buf) != BK_OK)
    {
        LOGE("%s, build video buf failed, chnl %d index %d\n", __func__, channel, index);
        return AVDK_ERR_INVAL;
    }

    int ret = isp_control->free_buf(isp_control->chn[channel].channel, &buf);
    return (ret == BK_OK) ? AVDK_ERR_OK : AVDK_ERR_GENERIC;
}

avdk_err_t bk_camera_isp_ctlr_new(bk_isp_camera_ctlr_handle_t *handle)
{
    isp_camera_ctlr_private_t *private = os_malloc(sizeof(isp_camera_ctlr_private_t));
    AVDK_RETURN_ON_FALSE(private, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(private, 0, sizeof(isp_camera_ctlr_private_t));
    bk_camera_isp_ctlr_t *controller = &private->controller;

    // os_memcpy(&controller->config, config, sizeof(bk_isp_camera_ctlr_config_t));
    controller->ops.dev_init = isp_camera_ctlr_dev_init;
    controller->ops.port_init = isp_camera_ctlr_port_init;
    controller->ops.port_change = isp_camera_ctlr_port_change;
    controller->ops.deinit = isp_camera_ctlr_deinit;
    controller->ops.read = isp_camera_ctlr_read;
    controller->ops.del = isp_camera_ctlr_delete;
    controller->ops.channel_open = isp_camera_ctlr_channel_open;
    controller->ops.channel_close = isp_camera_ctlr_channel_close;
    controller->ops.channel_state_get = isp_camera_ctlr_channel_state_get;
    controller->ops.register_isr_callback = isp_camera_ctlr_register_isr_callback;
    controller->ops.deregister_isr_callback = isp_camera_ctlr_deregister_isr_callback;
    controller->ops.ioctl = isp_camera_ctlr_ioctl;

    // controller->attr.snsRect.top = config->input_rect.top;
    // controller->attr.snsRect.left = config->input_rect.left;
    // controller->attr.snsRect.width = config->input_rect.width;
    // controller->attr.snsRect.height = config->input_rect.height;

    // controller->attr.inFormRect.top = config->input_crop.top;
    // controller->attr.inFormRect.left = config->input_crop.left;
    // controller->attr.inFormRect.width = config->input_crop.width;
    // controller->attr.inFormRect.height = config->input_crop.height;
    // controller->attr.outFormRect.top = config->input_crop.top;
    // controller->attr.outFormRect.left = config->input_crop.left;
    // controller->attr.outFormRect.width = config->input_crop.width;
    // controller->attr.outFormRect.height = config->input_crop.height;
    // controller->attr.iSRect.top = config->input_crop.top;
    // controller->attr.iSRect.left = config->input_crop.left;
    // controller->attr.iSRect.width = config->input_crop.width;
    // controller->attr.iSRect.height = config->input_crop.height;
    // controller->attr.snsFps = config->fps * ISP_SNS_FPS_ACCU;
    // controller->attr.ispInputType = config->input_type;
    // controller->attr.ispMode = config->isp_mode;
    // controller->attr.hdrMode = config->hdr_mode;
    // controller->attr.pixelFormat = isp_camera_format_convert(config->input_pixel_fmt);
    // controller->attr.pSnsObj = (void*)config->sensor_object;

    *handle = &(controller->ops);

    return AVDK_ERR_OK;
}