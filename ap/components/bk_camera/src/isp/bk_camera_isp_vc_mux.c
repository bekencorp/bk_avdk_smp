#include <os/os.h>
#include <os/mem.h>

#include <components/bk_camera_isp_ctlr.h>
#include <driver/isp.h>
#include <driver/mipi_csi.h>
#include <avdk_check.h>
#include <modules/veri_isp/vsios_error.h>
#include "isp/isp_camera_vc_mux_priv.h"

#define TAG "bk_cam_isp"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define ISP_CAMERA_VC_MUX_POP_TIMEOUT_MS 1000
#define ISP_CAMERA_VC_MUX_SWITCH_DISCARD_FRAMES 2
#define ISP_CAMERA_VC_MUX_IDLE_POP_TIMEOUT_MS 100
#define ISP_CAMERA_VC_MUX_TIMEOUT_SWITCH_COUNT 3
#define ISP_CAMERA_VC_MUX_MIPI_DATA_TYPE_YUV422 0x1E
#define ISP_CAMERA_VC_MUX_MAX_HELD_BUFS 4

static bk_camera_isp_vc_mux_t *isp_vc_mux_control(bk_isp_camera_vc_mux_handle_t handle)
{
    if (handle == NULL)
    {
        return NULL;
    }

    return __containerof(handle, bk_camera_isp_vc_mux_t, ops);
}

static bk_camera_isp_ctlr_t *isp_vc_mux_camera(bk_camera_isp_vc_mux_t *mux)
{
    if (mux == NULL || mux->camera == NULL)
    {
        return NULL;
    }

    return __containerof(mux->camera, bk_camera_isp_ctlr_t, ops);
}

static uint8_t *isp_camera_vc_mux_buf_addr(VIDEO_BUF_S *buf)
{
    if (buf->numPlanes == 0)
    {
        return NULL;
    }

    if (buf->planes[0].pUserAddr != NULL)
    {
        return (uint8_t *)buf->planes[0].pUserAddr;
    }

    return (uint8_t *)(uintptr_t)buf->planes[0].dmaPhyAddr;
}

static uint32_t isp_camera_vc_mux_buf_size(VIDEO_BUF_S *buf)
{
    uint32_t size = 0;
    uint8_t plane_cnt = buf->numPlanes ? buf->numPlanes : 1;

    if (plane_cnt > VIDEO_MAX_PLANES)
    {
        plane_cnt = VIDEO_MAX_PLANES;
    }

    for (uint8_t i = 0; i < plane_cnt; i++)
    {
        size += buf->planes[i].size;
    }

    return size;
}

static void isp_camera_vc_mux_init_nodes(bk_camera_isp_vc_mux_t *mux)
{
    mux->free_head = 0;
    for (uint8_t i = 0; i < ISP_CAMERA_VC_MUX_NODE_MAX; i++)
    {
        os_memset(&mux->node[i], 0, sizeof(mux->node[i]));
        mux->node[i].next = (i + 1 < ISP_CAMERA_VC_MUX_NODE_MAX)
                            ? (i + 1)
                            : ISP_CAMERA_VC_MUX_NODE_INVALID;
    }

    for (uint8_t vc = 0; vc < 2; vc++)
    {
        mux->queue_head[vc] = ISP_CAMERA_VC_MUX_NODE_INVALID;
        mux->queue_tail[vc] = ISP_CAMERA_VC_MUX_NODE_INVALID;
        mux->queue_count[vc] = 0;
        mux->sequence[vc] = 0;
    }
}

static uint8_t isp_camera_vc_mux_pop_free_node(bk_camera_isp_vc_mux_t *mux)
{
    uint8_t node = mux->free_head;

    if (node == ISP_CAMERA_VC_MUX_NODE_INVALID)
    {
        return ISP_CAMERA_VC_MUX_NODE_INVALID;
    }

    mux->free_head = mux->node[node].next;
    mux->node[node].next = ISP_CAMERA_VC_MUX_NODE_INVALID;
    return node;
}

static void isp_camera_vc_mux_push_free_node(bk_camera_isp_vc_mux_t *mux, uint8_t node)
{
    mux->node[node].next = mux->free_head;
    mux->free_head = node;
}

static uint8_t isp_camera_vc_mux_held_count(bk_camera_isp_vc_mux_t *mux)
{
    uint8_t count = 0;

    for (uint8_t i = 0; i < ISP_CAMERA_VC_MUX_NODE_MAX; i++)
    {
        if (mux->node[i].valid)
        {
            count++;
        }
    }

    return count;
}

static void isp_camera_vc_mux_unlink_node(bk_camera_isp_vc_mux_t *mux, uint8_t vc, uint8_t node, uint8_t prev)
{
    uint8_t next = mux->node[node].next;

    if (prev == ISP_CAMERA_VC_MUX_NODE_INVALID)
    {
        mux->queue_head[vc] = next;
    }
    else
    {
        mux->node[prev].next = next;
    }

    if (mux->queue_tail[vc] == node)
    {
        mux->queue_tail[vc] = prev;
    }

    mux->node[node].queued = 0;
    mux->node[node].next = ISP_CAMERA_VC_MUX_NODE_INVALID;
    if (mux->queue_count[vc] > 0)
    {
        mux->queue_count[vc]--;
    }
}

static void isp_camera_vc_mux_release_node_to_isp(bk_camera_isp_vc_mux_t *mux, uint8_t node)
{
    bk_camera_isp_ctlr_t *cam = isp_vc_mux_camera(mux);
    isp_control_t *isp_control = cam ? (isp_control_t *)cam->isp_handle : NULL;

    if (mux->node[node].valid && isp_control != NULL && isp_control->free_buf != NULL)
    {
        isp_control->free_buf(isp_control->chn[mux->channel].channel, &mux->node[node].buf);
    }

    os_memset(&mux->node[node], 0, sizeof(mux->node[node]));
    mux->node[node].next = ISP_CAMERA_VC_MUX_NODE_INVALID;
    isp_camera_vc_mux_push_free_node(mux, node);
}

static uint8_t isp_camera_vc_mux_release_oldest_releasable(bk_camera_isp_vc_mux_t *mux, uint8_t vc)
{
    uint8_t prev = ISP_CAMERA_VC_MUX_NODE_INVALID;
    uint8_t node = mux->queue_head[vc];

    while (node != ISP_CAMERA_VC_MUX_NODE_INVALID)
    {
        uint8_t next = mux->node[node].next;
        if (!mux->node[node].in_use)
        {
            isp_camera_vc_mux_unlink_node(mux, vc, node, prev);
            isp_camera_vc_mux_release_node_to_isp(mux, node);
            return 1;
        }
        prev = node;
        node = next;
    }

    return 0;
}

static void isp_camera_vc_mux_release_pressure(bk_camera_isp_vc_mux_t *mux, uint8_t vc)
{
    while (isp_camera_vc_mux_held_count(mux) > ISP_CAMERA_VC_MUX_MAX_HELD_BUFS)
    {
        uint8_t other_vc = vc ? 0 : 1;
        if (isp_camera_vc_mux_release_oldest_releasable(mux, vc))
        {
            continue;
        }
        if (isp_camera_vc_mux_release_oldest_releasable(mux, other_vc))
        {
            continue;
        }
        break;
    }
}

static void isp_camera_vc_mux_enqueue_node(bk_camera_isp_vc_mux_t *mux, uint8_t vc, uint8_t node)
{
    mux->node[node].queued = 1;
    mux->node[node].next = ISP_CAMERA_VC_MUX_NODE_INVALID;

    if (mux->queue_tail[vc] == ISP_CAMERA_VC_MUX_NODE_INVALID)
    {
        mux->queue_head[vc] = node;
        mux->queue_tail[vc] = node;
    }
    else
    {
        mux->node[mux->queue_tail[vc]].next = node;
        mux->queue_tail[vc] = node;
    }

    mux->queue_count[vc]++;
    while (mux->queue_count[vc] > ISP_CAMERA_VC_MUX_QUEUE_MAX)
    {
        if (!isp_camera_vc_mux_release_oldest_releasable(mux, vc))
        {
            break;
        }
    }

    isp_camera_vc_mux_release_pressure(mux, vc);
}

static void isp_camera_vc_mux_free_frames(bk_camera_isp_vc_mux_t *mux)
{
    for (uint8_t i = 0; i < ISP_CAMERA_VC_MUX_NODE_MAX; i++)
    {
        if (mux->node[i].valid)
        {
            isp_camera_vc_mux_release_node_to_isp(mux, i);
        }
    }
    isp_camera_vc_mux_init_nodes(mux);
}

static uint8_t isp_camera_vc_mux_vc_enabled(bk_camera_isp_vc_mux_t *mux, uint8_t vc)
{
    return (mux != NULL) && ((mux->vc_enable_mask & (1u << (vc & 0x01))) != 0);
}

static uint8_t isp_camera_vc_mux_enabled_count(bk_camera_isp_vc_mux_t *mux)
{
    uint8_t count = 0;

    if (mux == NULL)
    {
        return 0;
    }

    for (uint8_t vc = 0; vc < 2; vc++)
    {
        if (isp_camera_vc_mux_vc_enabled(mux, vc))
        {
            count++;
        }
    }

    return count;
}

static uint8_t isp_camera_vc_mux_first_enabled_vc(bk_camera_isp_vc_mux_t *mux)
{
    for (uint8_t vc = 0; vc < 2; vc++)
    {
        if (isp_camera_vc_mux_vc_enabled(mux, vc))
        {
            return vc;
        }
    }

    return 0;
}

static uint8_t isp_camera_vc_mux_next_enabled_vc(bk_camera_isp_vc_mux_t *mux, uint8_t after_vc)
{
    for (uint8_t step = 1; step <= 2; step++)
    {
        uint8_t vc = (uint8_t)((after_vc + step) & 0x01);
        if (isp_camera_vc_mux_vc_enabled(mux, vc))
        {
            return vc;
        }
    }

    return after_vc & 0x01;
}

static void isp_camera_vc_mux_schedule_next_vc(bk_camera_isp_vc_mux_t *mux, uint8_t after_vc)
{
    uint8_t next_vc;

    if (mux == NULL || isp_camera_vc_mux_enabled_count(mux) == 0)
    {
        return;
    }

    if (isp_camera_vc_mux_enabled_count(mux) == 1)
    {
        next_vc = isp_camera_vc_mux_first_enabled_vc(mux);
    }
    else
    {
        next_vc = isp_camera_vc_mux_next_enabled_vc(mux, after_vc);
    }

    mux->current_vc = next_vc;
    mux->discard_remaining[next_vc] = mux->vc_discard_frames[next_vc];
    mux->switch_pending = 1;
}

static void isp_camera_vc_mux_flush_vc_frames(bk_camera_isp_vc_mux_t *mux, uint8_t vc)
{
    if (mux == NULL)
    {
        return;
    }

    while (isp_camera_vc_mux_release_oldest_releasable(mux, vc))
    {
    }
}

static void isp_camera_vc_mux_apply_vc(bk_camera_isp_vc_mux_t *mux)
{
    if (!mux->enable || mux->vc_enable_mask == 0)
    {
        return;
    }

    if (mux->applied_vc != mux->current_vc)
    {
        if (mux->runtime_vc_switch)
        {
            bk_mipi_csi_set_runtime_vc(mux->current_vc);
        }
        else
        {
            bk_mipi_csi_controller_init_vc(mux->width, mux->height,
                                           ISP_CAMERA_VC_MUX_MIPI_DATA_TYPE_YUV422,
                                           mux->current_vc);
        }
        mux->applied_vc = mux->current_vc;
        mux->completed_valid = 0;
    }
}

static void isp_camera_vc_mux_request_vc_switch(bk_camera_isp_vc_mux_t *mux, uint8_t vc, uint8_t discard_frames)
{
    mux->current_vc = vc;
    mux->discard_remaining[vc] = discard_frames;
    mux->drain_stale_remaining = ISP_CAMERA_VC_MUX_SWITCH_DISCARD_FRAMES;
    mux->switch_pending = 1;
}

static uint8_t isp_camera_vc_mux_active_vc(bk_camera_isp_vc_mux_t *mux)
{
    if (mux->applied_vc != ISP_CAMERA_VC_MUX_APPLIED_INVALID)
    {
        return mux->applied_vc & 0x01;
    }

    return mux->current_vc & 0x01;
}

static uint8_t isp_camera_vc_mux_handle_buf(bk_camera_isp_vc_mux_t *mux, VIDEO_BUF_S *buf)
{
    uint8_t vc;
    uint8_t node;
    uint8_t cached = 0;
    uint32_t frame_size;

    if (!mux->enable)
    {
        return 0;
    }

    if (mux->completed_valid)
    {
        vc = mux->completed_vc & 0x01;
        mux->completed_valid = 0;
    }
    else
    {
        if (mux->applied_vc != ISP_CAMERA_VC_MUX_APPLIED_INVALID)
        {
            vc = mux->applied_vc & 0x01;
        }
        else
        {
            vc = mux->current_vc & 0x01;
        }
    }

    if (!isp_camera_vc_mux_vc_enabled(mux, vc))
    {
        return 0;
    }

    if (mux->discard_remaining[vc])
    {
        mux->discard_remaining[vc]--;
        return 0;
    }

    frame_size = isp_camera_vc_mux_buf_size(buf);
    if (frame_size == 0 || frame_size > mux->frame_size)
    {
        return 0;
    }

    if (mux->lock_inited)
    {
        rtos_lock_mutex(&mux->lock);
    }

    node = isp_camera_vc_mux_pop_free_node(mux);
    if (node == ISP_CAMERA_VC_MUX_NODE_INVALID)
    {
        (void)isp_camera_vc_mux_release_oldest_releasable(mux, vc);
        node = isp_camera_vc_mux_pop_free_node(mux);
    }

    if (node != ISP_CAMERA_VC_MUX_NODE_INVALID)
    {
        mux->node[node].buf = *buf;
        mux->node[node].vc = vc;
        mux->node[node].valid = 1;
        mux->node[node].frame_size = frame_size;
        mux->node[node].sequence = ++mux->sequence[vc];
        mux->timeout_count = 0;
        isp_camera_vc_mux_enqueue_node(mux, vc, node);
        cached = 1;
    }

    if (mux->lock_inited)
    {
        rtos_unlock_mutex(&mux->lock);
    }

    return cached;
}

static void isp_camera_vc_mux_handle_timeout(bk_camera_isp_vc_mux_t *mux)
{
    if (!mux->enable || isp_camera_vc_mux_enabled_count(mux) <= 1)
    {
        return;
    }

    mux->timeout_count++;
    if (mux->timeout_count < ISP_CAMERA_VC_MUX_TIMEOUT_SWITCH_COUNT)
    {
        return;
    }

    mux->timeout_count = 0;
    isp_camera_vc_mux_schedule_next_vc(mux, mux->current_vc);
    if (mux->lock_inited)
    {
        rtos_lock_mutex(&mux->lock);
    }
    LOGW("vc mux timeout switch to v%u\n", mux->current_vc);
    if (mux->lock_inited)
    {
        rtos_unlock_mutex(&mux->lock);
    }
}

static void isp_camera_vc_mux_frame_complete_callback(uint32_t seqence, uint32_t line, uint8_t chnl, uint8_t ok, void *param)
{
    bk_camera_isp_vc_mux_t *mux = (bk_camera_isp_vc_mux_t *)param;

    (void)seqence;
    (void)line;

    if (mux == NULL || !ok || !mux->enable || chnl != mux->channel)
    {
        return;
    }

    mux->completed_vc = isp_camera_vc_mux_active_vc(mux);
    mux->completed_valid = 1;

    if (isp_camera_vc_mux_enabled_count(mux) > 1)
    {
        isp_camera_vc_mux_schedule_next_vc(mux, mux->applied_vc & 0x01);
    }
}

static void isp_camera_vc_mux_stop_thread(bk_camera_isp_vc_mux_t *mux)
{
    if (mux == NULL)
    {
        return;
    }

    mux->thread_enable = false;
    if (mux->thread)
    {
        rtos_thread_join(&mux->thread);
        mux->thread = NULL;
    }
}

static void isp_camera_vc_mux_task_entry(void *param)
{
    bk_camera_isp_vc_mux_t *mux = (bk_camera_isp_vc_mux_t *)param;
    bk_camera_isp_ctlr_t *cam = NULL;
    isp_control_t *isp_control = NULL;
    isp_channel_config_t *config = NULL;

    if (mux == NULL)
    {
        LOGE("%s, invalid mux context\n", __func__);
        rtos_delete_thread(NULL);
        return;
    }

    cam = isp_vc_mux_camera(mux);
    if (cam == NULL)
    {
        LOGE("%s, camera is NULL\n", __func__);
        rtos_delete_thread(NULL);
        return;
    }

    isp_control = (isp_control_t *)cam->isp_handle;
    if (isp_control == NULL)
    {
        LOGE("%s, isp_control is NULL\n", __func__);
        rtos_delete_thread(NULL);
        return;
    }

    while (mux->thread_enable)
    {
        if (mux->switch_pending)
        {
            isp_camera_vc_mux_apply_vc(mux);
            mux->switch_pending = 0;
        }

        config = &isp_control->chn[mux->channel];
        if (config == NULL || config->enable == false)
        {
            rtos_delay_milliseconds(10);
            continue;
        }

        if (config->enable_flexa)
        {
            LOGE("%s, channel %d flexa mode not supported for vc mux\n", __func__, mux->channel);
            rtos_delay_milliseconds(1000);
            continue;
        }

        if (mux->vc_enable_mask == 0)
        {
            mux->pop_timeout_ms = ISP_CAMERA_VC_MUX_IDLE_POP_TIMEOUT_MS;
        }
        else if (isp_camera_vc_mux_enabled_count(mux) > 1)
        {
            mux->pop_timeout_ms = ISP_CAMERA_VC_MUX_POP_TIMEOUT_MS;
        }
        else
        {
            mux->pop_timeout_ms = ISP_CAMERA_VC_MUX_IDLE_POP_TIMEOUT_MS;
        }

        VIDEO_BUF_S buf;
        int ret = isp_control->pop_buf(config->channel, &buf, mux->pop_timeout_ms);
        if (ret == VSI_ERR_NOT_READY)
        {
            mux->thread_enable = false;
            break;
        }
        if (ret != BK_OK)
        {
            if (mux->vc_enable_mask != 0)
            {
                isp_camera_vc_mux_handle_timeout(mux);
            }
            continue;
        }

        if (mux->drain_stale_remaining > 0)
        {
            mux->drain_stale_remaining--;
            isp_control->free_buf(config->channel, &buf);
            continue;
        }

        if (mux->vc_enable_mask == 0)
        {
            isp_control->free_buf(config->channel, &buf);
            continue;
        }

        if (mux->lock_inited)
        {
            rtos_lock_mutex(&mux->lock);
            isp_camera_vc_mux_release_pressure(mux, isp_camera_vc_mux_active_vc(mux));
            rtos_unlock_mutex(&mux->lock);
        }

        uint8_t vc_mux_held = isp_camera_vc_mux_handle_buf(mux, &buf);
        if (!vc_mux_held)
        {
            isp_control->free_buf(config->channel, &buf);
        }
    }

    rtos_delete_thread(NULL);
}

static avdk_err_t isp_camera_vc_mux_start_thread(bk_camera_isp_vc_mux_t *mux)
{
    avdk_err_t ret = AVDK_ERR_GENERIC;

    AVDK_RETURN_ON_FALSE(mux, AVDK_ERR_INVAL, TAG, "mux is NULL");
    if (mux->thread_enable)
    {
        return AVDK_ERR_OK;
    }

    mux->thread_enable = true;
    ret = rtos_create_hsram_thread(&mux->thread,
                                   BEKEN_DEFAULT_WORKER_PRIORITY,
                                   "cam_vc_mux",
                                   (beken_thread_function_t)isp_camera_vc_mux_task_entry,
                                   1024 * 2,
                                   (beken_thread_arg_t)mux);
    if (ret != BK_OK)
    {
        LOGE("%s, vc mux thread create fail\n", __func__);
        mux->thread_enable = false;
        return ret;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_vc_mux_stop(bk_isp_camera_vc_mux_handle_t handle)
{
    bk_camera_isp_vc_mux_t *mux = isp_vc_mux_control(handle);
    bk_camera_isp_ctlr_t *cam = NULL;

    AVDK_RETURN_ON_FALSE(mux, AVDK_ERR_INVAL, TAG, "mux is NULL");

    isp_camera_vc_mux_stop_thread(mux);

    if (mux->lock_inited)
    {
        rtos_lock_mutex(&mux->lock);
    }
    mux->enable = 0;
    mux->vc_enable_mask = 0;
    if (mux->lock_inited)
    {
        rtos_unlock_mutex(&mux->lock);
    }

    cam = isp_vc_mux_camera(mux);
    if (mux->isr_registered && cam != NULL && cam->isp_handle)
    {
        (void)bk_isp_deregister_isr_callback(&cam->isp_handle, ISP_FRAME_END_DONE, mux);
        mux->isr_registered = 0;
    }

    isp_camera_vc_mux_free_frames(mux);

    if (mux->lock_inited)
    {
        rtos_deinit_mutex(&mux->lock);
        mux->lock_inited = 0;
    }

    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_vc_mux_start(bk_isp_camera_vc_mux_handle_t handle, bk_isp_camera_vc_mux_config_t *cfg)
{
    bk_camera_isp_vc_mux_t *mux = isp_vc_mux_control(handle);
    bk_camera_isp_ctlr_t *cam = NULL;
    avdk_err_t ret = AVDK_ERR_OK;

    AVDK_RETURN_ON_FALSE(mux, AVDK_ERR_INVAL, TAG, "mux is NULL");
    AVDK_RETURN_ON_FALSE(cfg, AVDK_ERR_INVAL, TAG, "vc mux config is NULL");
    AVDK_RETURN_ON_FALSE(cfg->channel < ISP_CHANNEL_INSTANCE_MAX, AVDK_ERR_INVAL, TAG, "vc mux channel invalid");
    AVDK_RETURN_ON_FALSE(cfg->frame_size > 0, AVDK_ERR_INVAL, TAG, "vc mux frame size invalid");

    cam = isp_vc_mux_camera(mux);
    AVDK_RETURN_ON_FALSE(cam, AVDK_ERR_INVAL, TAG, "camera is NULL");
    AVDK_RETURN_ON_FALSE(cam->channel_state[cfg->channel] == ISP_CHANNEL_STATE_TURN_ON,
                         AVDK_ERR_INVAL, TAG, "vc mux channel is not open");

    AVDK_RETURN_ON_ERROR(isp_camera_vc_mux_stop(handle), TAG, "vc mux stop failed");

    if (rtos_init_mutex(&mux->lock) != BK_OK)
    {
        LOGE("%s, vc mux mutex init failed\n", __func__);
        return AVDK_ERR_GENERIC;
    }
    mux->lock_inited = 1;
    isp_camera_vc_mux_init_nodes(mux);

    mux->channel = cfg->channel;
    mux->frame_size = cfg->frame_size;
    mux->width = cfg->width;
    mux->height = cfg->height;
    mux->runtime_vc_switch = cfg->runtime_vc_switch;
    mux->default_discard_frames = cfg->discard_frames;
    mux->vc_discard_frames[0] = 0;
    mux->vc_discard_frames[1] = 0;
    mux->discard_remaining[0] = 0;
    mux->discard_remaining[1] = 0;
    mux->vc_enable_mask = 0;
    mux->completed_valid = 0;
    mux->switch_pending = 0;
    mux->drain_stale_remaining = 0;
    mux->timeout_count = 0;
    mux->current_vc = 0;
    mux->applied_vc = ISP_CAMERA_VC_MUX_APPLIED_INVALID;
    mux->completed_vc = 0;
    mux->pop_timeout_ms = ISP_CAMERA_VC_MUX_IDLE_POP_TIMEOUT_MS;
    mux->enable = 1;

    if (bk_isp_register_isr_callback(&cam->isp_handle, ISP_FRAME_END_DONE,
                                     isp_camera_vc_mux_frame_complete_callback, mux) == BK_OK)
    {
        mux->isr_registered = 1;
    }

    ret = isp_camera_vc_mux_start_thread(mux);
    if (ret != AVDK_ERR_OK)
    {
        (void)isp_camera_vc_mux_stop(handle);
        return ret;
    }

    LOGI("vc mux start: channel=%u, frame_size=%u, default_discard=%u\n",
         cfg->channel, cfg->frame_size, cfg->discard_frames);
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_vc_mux_vc_enable(bk_isp_camera_vc_mux_handle_t handle, uint8_t vc, uint8_t discard_frames)
{
    bk_camera_isp_vc_mux_t *mux = isp_vc_mux_control(handle);

    AVDK_RETURN_ON_FALSE(mux, AVDK_ERR_INVAL, TAG, "mux is NULL");
    AVDK_RETURN_ON_FALSE(vc < 2, AVDK_ERR_INVAL, TAG, "vc mux vc invalid");
    AVDK_RETURN_ON_FALSE(mux->enable, AVDK_ERR_INVAL, TAG, "vc mux is not started");

    if (mux->lock_inited)
    {
        rtos_lock_mutex(&mux->lock);
    }

    if (isp_camera_vc_mux_vc_enabled(mux, vc))
    {
        mux->vc_discard_frames[vc] = discard_frames;
        if (mux->lock_inited)
        {
            rtos_unlock_mutex(&mux->lock);
        }
        return AVDK_ERR_OK;
    }

    mux->vc_enable_mask |= (1u << vc);
    mux->vc_discard_frames[vc] = discard_frames;

    if (isp_camera_vc_mux_enabled_count(mux) == 1)
    {
        isp_camera_vc_mux_request_vc_switch(mux, vc, discard_frames);
    }
    else if (mux->applied_vc == ISP_CAMERA_VC_MUX_APPLIED_INVALID
             || !isp_camera_vc_mux_vc_enabled(mux, mux->applied_vc & 0x01))
    {
        isp_camera_vc_mux_request_vc_switch(mux, vc, discard_frames);
    }

    if (mux->lock_inited)
    {
        rtos_unlock_mutex(&mux->lock);
    }

    LOGI("vc mux enable v%u, discard=%u, mask=0x%x\n", vc, discard_frames, mux->vc_enable_mask);
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_vc_mux_vc_disable(bk_isp_camera_vc_mux_handle_t handle, uint8_t vc)
{
    bk_camera_isp_vc_mux_t *mux = isp_vc_mux_control(handle);

    AVDK_RETURN_ON_FALSE(mux, AVDK_ERR_INVAL, TAG, "mux is NULL");
    AVDK_RETURN_ON_FALSE(vc < 2, AVDK_ERR_INVAL, TAG, "vc mux vc invalid");
    AVDK_RETURN_ON_FALSE(mux->enable, AVDK_ERR_INVAL, TAG, "vc mux is not started");

    if (!isp_camera_vc_mux_vc_enabled(mux, vc))
    {
        return AVDK_ERR_OK;
    }

    if (mux->lock_inited)
    {
        rtos_lock_mutex(&mux->lock);
    }

    mux->vc_enable_mask &= (uint8_t)(~(1u << vc));
    mux->vc_discard_frames[vc] = 0;
    mux->discard_remaining[vc] = 0;
    isp_camera_vc_mux_flush_vc_frames(mux, vc);

    if (isp_camera_vc_mux_enabled_count(mux) == 0)
    {
        mux->applied_vc = ISP_CAMERA_VC_MUX_APPLIED_INVALID;
        mux->completed_valid = 0;
        mux->switch_pending = 0;
        mux->drain_stale_remaining = 0;
        mux->timeout_count = 0;
    }
    else if (((mux->current_vc & 0x01) == vc) || ((mux->applied_vc & 0x01) == vc))
    {
        uint8_t next_vc = isp_camera_vc_mux_first_enabled_vc(mux);
        uint8_t switch_discard = mux->vc_discard_frames[next_vc];

        isp_camera_vc_mux_request_vc_switch(mux, next_vc, switch_discard);
    }

    if (mux->lock_inited)
    {
        rtos_unlock_mutex(&mux->lock);
    }

    LOGI("vc mux disable v%u, mask=0x%x\n", vc, mux->vc_enable_mask);
    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_vc_mux_peek(bk_isp_camera_vc_mux_handle_t handle, bk_isp_camera_vc_mux_frame_ref_t *frame)
{
    bk_camera_isp_vc_mux_t *mux = isp_vc_mux_control(handle);
    uint8_t vc;
    uint8_t node;
    uint8_t *addr;

    AVDK_RETURN_ON_FALSE(mux, AVDK_ERR_INVAL, TAG, "mux is NULL");
    AVDK_RETURN_ON_FALSE(frame, AVDK_ERR_INVAL, TAG, "vc mux frame ref is NULL");
    AVDK_RETURN_ON_FALSE(frame->vc < 2, AVDK_ERR_INVAL, TAG, "vc mux vc invalid");
    AVDK_RETURN_ON_FALSE(mux->enable, AVDK_ERR_INVAL, TAG, "vc mux is not enabled");

    vc = frame->vc & 0x01;
    frame->frame = NULL;
    frame->frame_size = 0;
    frame->sequence = 0;

    if (mux->lock_inited)
    {
        rtos_lock_mutex(&mux->lock);
    }
    node = mux->queue_tail[vc];
    if (node == ISP_CAMERA_VC_MUX_NODE_INVALID || !mux->node[node].valid)
    {
        if (mux->lock_inited)
        {
            rtos_unlock_mutex(&mux->lock);
        }
        return AVDK_ERR_TIMEOUT;
    }

    if (mux->node[node].in_use)
    {
        if (mux->lock_inited)
        {
            rtos_unlock_mutex(&mux->lock);
        }
        return AVDK_ERR_BUSY;
    }

    addr = isp_camera_vc_mux_buf_addr(&mux->node[node].buf);
    if (addr == NULL)
    {
        if (mux->lock_inited)
        {
            rtos_unlock_mutex(&mux->lock);
        }
        return AVDK_ERR_GENERIC;
    }

    mux->node[node].in_use = 1;
    frame->frame = addr;
    frame->frame_size = mux->node[node].frame_size;
    frame->sequence = mux->node[node].sequence;

    if (mux->lock_inited)
    {
        rtos_unlock_mutex(&mux->lock);
    }

    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_vc_mux_release(bk_isp_camera_vc_mux_handle_t handle, bk_isp_camera_vc_mux_frame_ref_t *frame)
{
    bk_camera_isp_vc_mux_t *mux = isp_vc_mux_control(handle);
    uint8_t vc;
    uint8_t node;

    AVDK_RETURN_ON_FALSE(mux, AVDK_ERR_INVAL, TAG, "mux is NULL");
    AVDK_RETURN_ON_FALSE(frame, AVDK_ERR_INVAL, TAG, "vc mux frame ref is NULL");
    AVDK_RETURN_ON_FALSE(frame->vc < 2, AVDK_ERR_INVAL, TAG, "vc mux vc invalid");

    vc = frame->vc & 0x01;
    if (mux->lock_inited)
    {
        rtos_lock_mutex(&mux->lock);
    }

    node = mux->queue_head[vc];
    while (node != ISP_CAMERA_VC_MUX_NODE_INVALID)
    {
        uint8_t *addr = isp_camera_vc_mux_buf_addr(&mux->node[node].buf);
        if (addr == frame->frame && mux->node[node].sequence == frame->sequence)
        {
            mux->node[node].in_use = 0;
            break;
        }
        node = mux->node[node].next;
    }

    while (mux->queue_count[vc] > ISP_CAMERA_VC_MUX_QUEUE_MAX)
    {
        if (!isp_camera_vc_mux_release_oldest_releasable(mux, vc))
        {
            break;
        }
    }

    if (mux->lock_inited)
    {
        rtos_unlock_mutex(&mux->lock);
    }

    return AVDK_ERR_OK;
}

static avdk_err_t isp_camera_vc_mux_delete(bk_isp_camera_vc_mux_handle_t handle)
{
    bk_camera_isp_vc_mux_t *mux = isp_vc_mux_control(handle);

    AVDK_RETURN_ON_FALSE(mux, AVDK_ERR_INVAL, TAG, "control is NULL");

    (void)isp_camera_vc_mux_stop(handle);
    os_free(mux);

    return AVDK_ERR_OK;
}

avdk_err_t bk_camera_isp_vc_mux_new(bk_isp_camera_vc_mux_handle_t *handle, bk_isp_camera_ctlr_handle_t camera)
{
    bk_camera_isp_vc_mux_t *mux = NULL;

    AVDK_RETURN_ON_FALSE(handle, AVDK_ERR_INVAL, TAG, AVDK_ERR_INVAL_NULL_TEXT);
    AVDK_RETURN_ON_FALSE(camera, AVDK_ERR_INVAL, TAG, "camera is NULL");

    mux = (bk_camera_isp_vc_mux_t *)os_malloc(sizeof(bk_camera_isp_vc_mux_t));
    AVDK_RETURN_ON_FALSE(mux, AVDK_ERR_NOMEM, TAG, AVDK_ERR_NOMEM_TEXT);
    os_memset(mux, 0, sizeof(bk_camera_isp_vc_mux_t));
    mux->camera = camera;
    mux->ops.start = isp_camera_vc_mux_start;
    mux->ops.stop = isp_camera_vc_mux_stop;
    mux->ops.vc_enable = isp_camera_vc_mux_vc_enable;
    mux->ops.vc_disable = isp_camera_vc_mux_vc_disable;
    mux->ops.peek = isp_camera_vc_mux_peek;
    mux->ops.release = isp_camera_vc_mux_release;
    mux->ops.del = isp_camera_vc_mux_delete;

    *handle = &(mux->ops);
    return AVDK_ERR_OK;
}
