#include <os/mem.h>
#include <os/os.h>
#include <components/log.h>
#include <components/bk_audio/audio_pipeline/audio_types.h>

#include "network_transfer.h"
#include "network_type.h"
#include "network_transfer_internal.h"
#include "video_drop.h"
#if CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE
#include "video_fps.h"
#endif
#include "bk_network_service/bk_ntwk_socket/ntwk_socket.h"
#include "ntwk_pack.h"
#include "ntwk_fragmentation.h"
#if CONFIG_NTWK_CTRL_CHAN_JSON
#include "ntwk_json.h"
#endif

#define TAG "ntwk-trans"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)


static ntwk_trans_ctxt_t *s_ntwk_trans_ctxt = NULL;
static volatile uint8_t s_ntwk_trans_chan_abort[NTWK_TRANS_CHAN_MAX] = {0};
static volatile uint8_t s_ntwk_trans_chan_send_bypass[NTWK_TRANS_CHAN_MAX] = {0};
typedef struct
{
    ntwk_trans_session_recv_cb_t recv_cb;
    beken_mutex_t ctrl_session_lock;
    uint8_t ctrl_session_lock_inited;
    ntwk_session_id_t rx_session_id;
    ntwk_session_id_t tx_session_id;
} ntwk_trans_session_ctx_t;

static ntwk_trans_session_ctx_t s_session_ctx =
{
    .rx_session_id = NTWK_INVALID_SESSION_ID,
    .tx_session_id = NTWK_INVALID_SESSION_ID,
};
#if CONFIG_NTWK_CLIENT_SERVICE_ENABLE
static ntwk_server_net_info_t s_ntwk_server_net_info = {0};
#endif

static int ntwk_trans_ctrl_send_to_transport(ntwk_session_id_t sid, uint8_t *data, uint32_t length)
{
    ntwk_trans_ctrl_chan_t *ctrl =
        (s_ntwk_trans_ctxt != NULL) ? s_ntwk_trans_ctxt->cntrl_chan : NULL;

    if (ctrl == NULL)
    {
        return BK_FAIL;
    }

    if (ctrl->send_to != NULL)
    {
        return (ctrl->send_to)(sid, data, length);
    }

    if (ctrl->send != NULL)
    {
        return (ctrl->send)(data, length);
    }

    return BK_FAIL;
}

static int ntwk_trans_chan_abort_check(uint32_t chan_type)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX)
    {
        return 1;
    }

    if (s_ntwk_trans_chan_send_bypass[chan_type])
    {
        return 0;
    }

    return s_ntwk_trans_chan_abort[chan_type] != 0;
}

ntwk_trans_ctxt_t *ntwk_trans_get_ctxt(void)
{
	return s_ntwk_trans_ctxt;
}

const char *ntwk_trans_get_service_name(void)
{
	if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
	{
		LOGE("%s, context not initialized\n", __func__);
		return NULL;
	}

	return s_ntwk_trans_ctxt->service_name;
}

bk_err_t ntwk_trans_register_msg_event_cb(ntwk_trans_msg_event_cb_t cb)
{
    return ntwk_msg_register_event_cb(cb);
}

int ntwk_trans_ctrl_recv_handler(uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan == NULL)
    {
        LOGE("%s, control channel not configured\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->unpack != NULL)
    {
        if(s_ntwk_trans_ctxt->cntrl_chan->unpack(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->unfragment != NULL)
    {
        if(s_ntwk_trans_ctxt->cntrl_chan->unfragment(data, length) >= 0)
        {
            return length;
        }
#if CONFIG_NTWK_CTRL_CHAN_JSON
        return BK_FAIL;
#endif
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->recive != NULL)
    {
        return s_ntwk_trans_ctxt->cntrl_chan->recive(data, length);
    }

    return BK_FAIL;
}

int ntwk_trans_ctrl_recv_handler_from_session(ntwk_session_id_t sid, uint8_t *data, uint32_t length)
{
    int ret = BK_FAIL;

    if (s_session_ctx.ctrl_session_lock_inited)
    {
        rtos_lock_mutex(&s_session_ctx.ctrl_session_lock);
    }

    s_session_ctx.rx_session_id = sid;
    ret = ntwk_trans_ctrl_recv_handler(data, length);
    s_session_ctx.rx_session_id = NTWK_INVALID_SESSION_ID;

    if (s_session_ctx.ctrl_session_lock_inited)
    {
        rtos_unlock_mutex(&s_session_ctx.ctrl_session_lock);
    }

    return ret;
}

int ntwk_trans_video_recv_handler(uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->video_chan == NULL)
    {
        LOGE("%s, video channel not configured\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->video_chan->unpack != NULL)
    {
        if(s_ntwk_trans_ctxt->video_chan->unpack(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->video_chan->unfragment != NULL)
    {
        if(s_ntwk_trans_ctxt->video_chan->unfragment(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->video_chan->recive != NULL)
    {
        return s_ntwk_trans_ctxt->video_chan->recive(data, length);
    }

    return BK_FAIL;
}

int ntwk_trans_audio_recv_handler(uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->audio_chan == NULL)
    {
        LOGE("%s, audio channel not configured\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->audio_chan->unpack != NULL)
    {
        if(s_ntwk_trans_ctxt->audio_chan->unpack(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->audio_chan->unfragment != NULL)
    {
        if(s_ntwk_trans_ctxt->audio_chan->unfragment(data, length) >= 0)
        {
            return length;
        }
    }

    if (s_ntwk_trans_ctxt->audio_chan->recive != NULL)
    {
        return s_ntwk_trans_ctxt->audio_chan->recive(data, length);
    }

    return BK_FAIL;
}

int ntwk_trans_fragment_rx_handler(chan_type_t chan, uint8_t *data, uint32_t length)
{
    uint8_t *pack_ptr = NULL;
    uint32_t pack_ptr_length = 0;

    switch (chan)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (s_ntwk_trans_ctxt->cntrl_chan && s_ntwk_trans_ctxt->cntrl_chan->send != NULL)
            {
                if (s_ntwk_trans_ctxt->cntrl_chan->pack != NULL)
                {
                  if(s_ntwk_trans_ctxt->cntrl_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
                  {
                    (s_ntwk_trans_ctxt->cntrl_chan->send)(pack_ptr, pack_ptr_length);
                    break;
                  }
                }

                (s_ntwk_trans_ctxt->cntrl_chan->send)(data, length);
            }
        }break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (s_ntwk_trans_ctxt->video_chan && s_ntwk_trans_ctxt->video_chan->send != NULL)
            {
                if (s_ntwk_trans_ctxt->video_chan->pack != NULL)
                {
                    if(s_ntwk_trans_ctxt->video_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
                    {
                        (s_ntwk_trans_ctxt->video_chan->send)(pack_ptr, pack_ptr_length, s_ntwk_trans_ctxt->video_chan->vid_type);
                        break;
                    }
                }
                (s_ntwk_trans_ctxt->video_chan->send)(data, length, s_ntwk_trans_ctxt->video_chan->vid_type);
            }
        }break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (s_ntwk_trans_ctxt->audio_chan && s_ntwk_trans_ctxt->audio_chan->send != NULL)
            {
                if (s_ntwk_trans_ctxt->audio_chan->pack != NULL)
                {
                    if(s_ntwk_trans_ctxt->audio_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
                    {
                        (s_ntwk_trans_ctxt->audio_chan->send)(pack_ptr, pack_ptr_length, s_ntwk_trans_ctxt->audio_chan->aud_type);
                        break;
                    }
                }
                (s_ntwk_trans_ctxt->audio_chan->send)(data, length, s_ntwk_trans_ctxt->audio_chan->aud_type);
            }
        }break;
        default:
            LOGE("%s, invalid channel type: %d\n", __func__, chan);
            return BK_ERR_PARAM;
    }

    return BK_OK;
}

#if CONFIG_NTWK_CTRL_CHAN_JSON
int ntwk_trans_json_tx_handler(chan_type_t chan, uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (data == NULL || length == 0)
    {
        return BK_FAIL;
    }

    switch (chan)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (s_ntwk_trans_ctxt->cntrl_chan && s_ntwk_trans_ctxt->cntrl_chan->send)
            {
                if (s_session_ctx.tx_session_id != NTWK_INVALID_SESSION_ID)
                {
                    return ntwk_trans_ctrl_send_to_transport(s_session_ctx.tx_session_id, data, length);
                }
                return (s_ntwk_trans_ctxt->cntrl_chan->send)(data, length);
            }
        } break;
        default:
            LOGE("%s, invalid json channel type: %d\n", __func__, chan);
            break;
    }

    return BK_FAIL;
}

int ntwk_trans_json_rx_handler(chan_type_t chan, uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (data == NULL || length == 0)
    {
        return BK_FAIL;
    }

    switch (chan)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (s_session_ctx.rx_session_id != NTWK_INVALID_SESSION_ID
                && s_session_ctx.recv_cb != NULL)
            {
                return s_session_ctx.recv_cb(s_session_ctx.rx_session_id,
                                                    NTWK_TRANS_CHAN_CTRL,
                                                    data,
                                                    length);
            }

            if (s_ntwk_trans_ctxt->cntrl_chan && s_ntwk_trans_ctxt->cntrl_chan->recive)
            {
                return s_ntwk_trans_ctxt->cntrl_chan->recive(data, length);
            }
        } break;
        default:
            LOGE("%s, invalid json channel type: %d\n", __func__, chan);
            break;
    }

    return BK_FAIL;
}
#endif

bk_err_t ntwk_trans_ctxt_init(ntwk_trans_ctxt_t *ctxt)
{
    LOGV("%s start\r\n", __func__);
    if (s_ntwk_trans_ctxt != NULL)
    {
        LOGW("%s, context already initialized\n", __func__);
        return BK_FAIL;
    }

    if (ctxt != NULL)
    {
        LOGE("%s, context is NULL\n", __func__);
        os_memcpy(s_ntwk_trans_ctxt, ctxt, sizeof(ntwk_trans_ctxt_t));
    }
    else
    {
        s_ntwk_trans_ctxt = ntwk_malloc(sizeof(ntwk_trans_ctxt_t));
        if (s_ntwk_trans_ctxt == NULL)
        {
            LOGE("malloc ctxt failed\n");
            return BK_FAIL;
        }
        os_memset(s_ntwk_trans_ctxt, 0, sizeof(ntwk_trans_ctxt_t));

        s_ntwk_trans_ctxt->cntrl_chan = ntwk_malloc(sizeof(ntwk_trans_ctrl_chan_t));
        if (s_ntwk_trans_ctxt->cntrl_chan == NULL)
        {
            LOGE("malloc cntrl_chan failed\n");
            return BK_FAIL;
        }
        os_memset(s_ntwk_trans_ctxt->cntrl_chan, 0, sizeof(ntwk_trans_ctrl_chan_t));
        
        s_ntwk_trans_ctxt->video_chan = ntwk_malloc(sizeof(ntwk_trans_video_chan_t));
        if (s_ntwk_trans_ctxt->video_chan == NULL)
        {
            LOGE("malloc video_chan failed\n");
            return BK_FAIL;
        }
        os_memset(s_ntwk_trans_ctxt->video_chan, 0, sizeof(ntwk_trans_video_chan_t));
        
        s_ntwk_trans_ctxt->audio_chan = ntwk_malloc(sizeof(ntwk_trans_audio_chan_t));
        if (s_ntwk_trans_ctxt->audio_chan == NULL)
        {
            LOGE("malloc audio_chan failed\n");
            return BK_FAIL;
        }
        os_memset(s_ntwk_trans_ctxt->audio_chan, 0, sizeof(ntwk_trans_audio_chan_t));
    }

    ntwk_msg_init();
    ntwk_msg_start();
    ntwk_video_drop_init();
#if CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE
    (void)ntwk_video_fps_init();
#endif

    ntwk_pack_init(NTWK_TRANS_CHAN_CTRL);
    ntwk_pack_init(NTWK_TRANS_CHAN_VIDEO);
    ntwk_pack_init(NTWK_TRANS_CHAN_AUDIO);

    ntwk_fragmentation_init(NTWK_TRANS_CHAN_CTRL);
    ntwk_fragmentation_init(NTWK_TRANS_CHAN_VIDEO);
    ntwk_fragmentation_init(NTWK_TRANS_CHAN_AUDIO);
    ntwk_trans_chan_abort(NTWK_TRANS_CHAN_CTRL, false);
    ntwk_trans_chan_abort(NTWK_TRANS_CHAN_VIDEO, false);
    ntwk_trans_chan_abort(NTWK_TRANS_CHAN_AUDIO, false);
    ntwk_fragment_register_abort_cb(NTWK_TRANS_CHAN_CTRL, ntwk_trans_chan_abort_check);
    ntwk_fragment_register_abort_cb(NTWK_TRANS_CHAN_VIDEO, ntwk_trans_chan_abort_check);
    ntwk_fragment_register_abort_cb(NTWK_TRANS_CHAN_AUDIO, ntwk_trans_chan_abort_check);
    ntwk_socket_register_abort_check_cb(ntwk_trans_chan_abort_check);
    if (!s_session_ctx.ctrl_session_lock_inited)
    {
        rtos_init_mutex(&s_session_ctx.ctrl_session_lock);
        s_session_ctx.ctrl_session_lock_inited = 1;
    }

#if CONFIG_NTWK_CTRL_CHAN_JSON
    ntwk_json_init(NTWK_TRANS_CHAN_CTRL);
#endif

    s_ntwk_trans_ctxt->initialized = true;

    LOGV("%s, service: %s\n", __func__, s_ntwk_trans_ctxt->service_name);

    return BK_OK;
}

bk_err_t ntwk_trans_ctxt_deinit(void)
{
    LOGV("%s start\r\n", __func__);

    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGW("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    ntwk_msg_stop();
    ntwk_msg_deinit();
    ntwk_video_drop_deinit();
#if CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE
    (void)ntwk_video_fps_deinit();
#endif
    ntwk_pack_deinit(NTWK_TRANS_CHAN_CTRL);
    ntwk_pack_deinit(NTWK_TRANS_CHAN_VIDEO);
    ntwk_pack_deinit(NTWK_TRANS_CHAN_AUDIO);

    ntwk_fragmentation_deinit(NTWK_TRANS_CHAN_CTRL);
    ntwk_fragmentation_deinit(NTWK_TRANS_CHAN_VIDEO);
    ntwk_fragmentation_deinit(NTWK_TRANS_CHAN_AUDIO);
    ntwk_trans_chan_abort(NTWK_TRANS_CHAN_CTRL, false);
    ntwk_trans_chan_abort(NTWK_TRANS_CHAN_VIDEO, false);
    ntwk_trans_chan_abort(NTWK_TRANS_CHAN_AUDIO, false);
    ntwk_socket_register_abort_check_cb(NULL);
    s_session_ctx.recv_cb = NULL;
    s_session_ctx.rx_session_id = NTWK_INVALID_SESSION_ID;
    s_session_ctx.tx_session_id = NTWK_INVALID_SESSION_ID;
    if (s_session_ctx.ctrl_session_lock_inited)
    {
        rtos_deinit_mutex(&s_session_ctx.ctrl_session_lock);
        s_session_ctx.ctrl_session_lock_inited = 0;
    }

#if CONFIG_NTWK_CTRL_CHAN_JSON
    ntwk_json_deinit(NTWK_TRANS_CHAN_CTRL);
#endif

    if (s_ntwk_trans_ctxt->cntrl_chan != NULL)
    {
        os_free(s_ntwk_trans_ctxt->cntrl_chan);
        s_ntwk_trans_ctxt->cntrl_chan = NULL;
    }
    if (s_ntwk_trans_ctxt->video_chan != NULL)
    {
        os_free(s_ntwk_trans_ctxt->video_chan);
        s_ntwk_trans_ctxt->video_chan = NULL;
    }
    if (s_ntwk_trans_ctxt->audio_chan != NULL)
    {
        os_free(s_ntwk_trans_ctxt->audio_chan);
        s_ntwk_trans_ctxt->audio_chan = NULL;
    }

    s_ntwk_trans_ctxt->initialized = false;

    os_free(s_ntwk_trans_ctxt);
    s_ntwk_trans_ctxt = NULL;

    LOGV("%s, completed\n", __func__);

    return BK_OK;
}

bk_err_t ntwk_trans_chan_start(chan_type_t chan_type, void *param)
{
    LOGV("%s start\r\n", __func__);

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    bk_err_t ret = BK_FAIL;

    ntwk_trans_chan_abort(chan_type, false);
    ret = ntwk_in_start(chan_type, param);

    if (ret == BK_OK)
    {
        LOGV("%s, channel %d started\n", __func__, chan_type);
    }
    else
    {
        LOGE("%s, channel %d start failed: %d\n", __func__, chan_type, ret);
    }

    return ret;
}

bk_err_t ntwk_trans_chan_stop(chan_type_t chan_type)
{
    LOGV("%s start\r\n", __func__);

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    bk_err_t ret = BK_FAIL;

    ret = ntwk_in_stop(chan_type);

    if (ret != BK_OK)
    {
        LOGE("%s, channel %d stop failed: %d\n", __func__, chan_type, ret);
    }

    return ret;
}

bk_err_t ntwk_trans_chan_stop_all(void)
{
    LOGV("%s start\r\n", __func__);

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    return ntwk_in_stop_all();
}

bk_err_t ntwk_trans_chan_abort(chan_type_t chan_type, bool abort)
{
    if (chan_type >= NTWK_TRANS_CHAN_MAX)
    {
        LOGE("%s, invalid channel type: %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    s_ntwk_trans_chan_abort[chan_type] = abort ? 1 : 0;
    return BK_OK;
}

bk_err_t ntwk_trans_chan_discard_frame(chan_type_t chan_type, uint8_t frame_id)
{
    int ret;

    if (chan_type >= NTWK_TRANS_CHAN_MAX)
    {
        LOGE("%s, invalid channel type: %d\n", __func__, chan_type);
        return BK_ERR_PARAM;
    }

    s_ntwk_trans_chan_send_bypass[chan_type] = 1;
    ret = ntwk_fragment_discard_frame(chan_type, frame_id);
    s_ntwk_trans_chan_send_bypass[chan_type] = 0;

    return (ret >= 0) ? BK_OK : BK_FAIL;
}

int ntwk_trans_ctrl_send(uint8_t *data, uint32_t length)
{
    uint8_t *pack_ptr = NULL;
    uint32_t pack_ptr_length = 0;

    if (ntwk_trans_chan_abort_check(NTWK_TRANS_CHAN_CTRL))
    {
        return BK_FAIL;
    }

    if (data == NULL || length == 0)
    {
        LOGE("%s, invalid parameters\n", __func__);
        return BK_FAIL;
    }

    if (s_session_ctx.rx_session_id != NTWK_INVALID_SESSION_ID)
    {
        return ntwk_trans_ctrl_send_to(s_session_ctx.rx_session_id, data, length);
    }

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan == NULL || s_ntwk_trans_ctxt->cntrl_chan->send == NULL)
    {
        LOGE("%s, control channel not configured\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->fragment != NULL)
    {
        if(s_ntwk_trans_ctxt->cntrl_chan->fragment(data, length) >= 0)
        {
            return BK_OK;
        }
        else
        {
            LOGE("%s, fragment failed\n", __func__);
            return BK_FAIL;
        }
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->pack != NULL)
    {
        if(s_ntwk_trans_ctxt->cntrl_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
        {
            (s_ntwk_trans_ctxt->cntrl_chan->send)(pack_ptr, pack_ptr_length);
            return BK_OK;
        } else
        {
            LOGE("%s, pack failed\n", __func__);
            return BK_FAIL;
        }
    }

    return (s_ntwk_trans_ctxt->cntrl_chan->send)(data, length);
}

int ntwk_trans_ctrl_send_to(ntwk_session_id_t sid, uint8_t *data, uint32_t length)
{
    uint8_t *pack_ptr = NULL;
    uint32_t pack_ptr_length = 0;
    int ret = BK_FAIL;

    if (sid == NTWK_INVALID_SESSION_ID)
    {
        LOGE("%s, invalid sid\n", __func__);
        return BK_FAIL;
    }

    if (ntwk_trans_chan_abort_check(NTWK_TRANS_CHAN_CTRL))
    {
        return BK_FAIL;
    }

    if (data == NULL || length == 0)
    {
        LOGE("%s, invalid parameters\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan == NULL)
    {
        LOGE("%s, control channel not configured\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->fragment != NULL)
    {
        s_session_ctx.tx_session_id = sid;
        ret = s_ntwk_trans_ctxt->cntrl_chan->fragment(data, length);
        s_session_ctx.tx_session_id = NTWK_INVALID_SESSION_ID;
        return (ret >= 0) ? BK_OK : BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan->pack != NULL)
    {
        if (s_ntwk_trans_ctxt->cntrl_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
        {
            return ntwk_trans_ctrl_send_to_transport(sid, pack_ptr, pack_ptr_length);
        }

        LOGE("%s, pack failed\n", __func__);
        return BK_FAIL;
    }

    return ntwk_trans_ctrl_send_to_transport(sid, data, length);
}

int ntwk_trans_video_send(uint8_t *data, uint32_t length, image_format_t video_type)
{
    uint8_t *pack_ptr = NULL;
    uint32_t pack_ptr_length = 0;
    int ret = BK_FAIL;

    if (ntwk_trans_chan_abort_check(NTWK_TRANS_CHAN_VIDEO))
    {
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (data == NULL || length == 0)
    {
        LOGE("%s, invalid parameters\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->video_chan == NULL || s_ntwk_trans_ctxt->video_chan->send == NULL)
    {
        LOGE("%s, video channel not configured\n", __func__);
        return BK_FAIL;
    }
    s_ntwk_trans_ctxt->video_chan->vid_type = video_type;

    if (s_ntwk_trans_ctxt->video_chan->drop_check != NULL)
    {
        if (s_ntwk_trans_ctxt->video_chan->drop_check((frame_buffer_t *)data) == true)
        {
            return BK_OK;
        }
    }

#if CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE
    ntwk_video_fps_frame_begin();
#endif

    if (s_ntwk_trans_ctxt->video_chan->fragment != NULL)
    {
        ret = s_ntwk_trans_ctxt->video_chan->fragment(data, length);
        if(ret >= 0)
        {
#if CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE
            ntwk_video_fps_frame_end(length, ret);
#endif
            return length;
        } else
        {
            LOGE("%s, fragment failed: %d\n", __func__, ret);
            return BK_FAIL;
        }

        /* Fragment failure/abort must not fall through to pack (data is frame_buffer_t*). */
        LOGE("%s, video fragment failed, ret=%d, length=%u\n", __func__, ret, length);
#if CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE
        ntwk_video_fps_frame_end(length, ret);
#endif
        return ret;
    }

    if (s_ntwk_trans_ctxt->video_chan->pack != NULL)
    {
        if(s_ntwk_trans_ctxt->video_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
        {
            ret = (s_ntwk_trans_ctxt->video_chan->send)(pack_ptr, pack_ptr_length,s_ntwk_trans_ctxt->video_chan->vid_type);
#if CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE
            ntwk_video_fps_frame_end(length, ret);
#endif
            return ret;
        } else
        {
            LOGE("%s, pack failed: %d\n", __func__, ret);
            return BK_FAIL;
        }
    }

    ret = (s_ntwk_trans_ctxt->video_chan->send)(data, length, s_ntwk_trans_ctxt->video_chan->vid_type);
#if CONFIG_NTWK_VIDEO_FPS_CALC_ENABLE
    ntwk_video_fps_frame_end(length, ret);
#endif

    return ret;
}

int ntwk_trans_audio_send(uint8_t *data, uint32_t length, audio_enc_type_t audio_type)
{
    uint8_t *pack_ptr = NULL;
    uint32_t pack_ptr_length = 0;

    if (ntwk_trans_chan_abort_check(NTWK_TRANS_CHAN_AUDIO))
    {
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (data == NULL || length == 0)
    {
        LOGE("%s, invalid parameters\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->audio_chan == NULL || s_ntwk_trans_ctxt->audio_chan->send == NULL)
    {
        LOGE("%s, audio channel not configured\n", __func__);
        return BK_FAIL;
    }

    s_ntwk_trans_ctxt->audio_chan->aud_type = audio_type;

    if (s_ntwk_trans_ctxt->audio_chan->fragment != NULL)
    {
        if(s_ntwk_trans_ctxt->audio_chan->fragment(data, length) >= 0)
        {
            return length;
        } else
        {
            LOGE("%s, fragment failed: %d\n", __func__);
            return BK_FAIL;
        }
    }

    if (s_ntwk_trans_ctxt->audio_chan->pack != NULL)
    {
        if(s_ntwk_trans_ctxt->audio_chan->pack(data, length, &pack_ptr, &pack_ptr_length) >= 0)
        {
            return (s_ntwk_trans_ctxt->audio_chan->send)(pack_ptr, pack_ptr_length, s_ntwk_trans_ctxt->audio_chan->aud_type);
        }
        else
        {
            LOGE("%s, pack failed: %d\n", __func__);
            return BK_FAIL;
        }
    }

    return (s_ntwk_trans_ctxt->audio_chan->send)(data, length, s_ntwk_trans_ctxt->audio_chan->aud_type);
}

int ntwk_trans_pack_rx_handler(chan_type_t chan_type, uint8_t *data, uint32_t length)
{
    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    switch (chan_type)
    {
        case NTWK_TRANS_CHAN_CTRL:
        {
            if (s_ntwk_trans_ctxt->cntrl_chan == NULL)
            {
                LOGE("%s, control channel not configured\n", __func__);
                return -1;
            }

            if (s_ntwk_trans_ctxt->cntrl_chan->unfragment != NULL)
            {
                if(s_ntwk_trans_ctxt->cntrl_chan->unfragment(data, length) >= 0)
                {
                    return length;
                }
#if CONFIG_NTWK_CTRL_CHAN_JSON
                return BK_FAIL;
#endif
            }

            if (s_session_ctx.rx_session_id != NTWK_INVALID_SESSION_ID
                && s_session_ctx.recv_cb != NULL)
            {
                return s_session_ctx.recv_cb(s_session_ctx.rx_session_id,
                                                    NTWK_TRANS_CHAN_CTRL,
                                                    data,
                                                    length);
            }

            if (s_ntwk_trans_ctxt->cntrl_chan->recive != NULL)
            {
                return s_ntwk_trans_ctxt->cntrl_chan->recive(data, length);
            }
        } break;
        case NTWK_TRANS_CHAN_VIDEO:
        {
            if (s_ntwk_trans_ctxt->video_chan == NULL)
            {
                LOGE("%s, video channel not configured\n", __func__);
                return BK_FAIL;
            }
            if (s_ntwk_trans_ctxt->video_chan->unfragment != NULL)
            {
                if(s_ntwk_trans_ctxt->video_chan->unfragment(data, length) >= 0)
                {
                    return length;
                }
            }
            if (s_ntwk_trans_ctxt->video_chan->recive != NULL)
            {
                return s_ntwk_trans_ctxt->video_chan->recive(data, length);
            }
        } break;
        case NTWK_TRANS_CHAN_AUDIO:
        {
            if (s_ntwk_trans_ctxt->audio_chan == NULL)
            {
                LOGE("%s, audio channel not configured\n", __func__);
                return BK_FAIL;
            }
            if (s_ntwk_trans_ctxt->audio_chan->unfragment != NULL)
            {
                if(s_ntwk_trans_ctxt->audio_chan->unfragment(data, length) >= 0)
                {
                    return length;
                }
            }
            if (s_ntwk_trans_ctxt->audio_chan->recive != NULL)
            {
                return s_ntwk_trans_ctxt->audio_chan->recive(data, length);
            }
        } break;
        default:
            LOGE("%s, invalid channel type: %d\n", __func__, chan_type);
            return BK_FAIL;
    }

    return BK_FAIL;
}

int ntwk_trans_pack_rx_handler_from_session(ntwk_session_id_t sid, chan_type_t chan_type, uint8_t *data, uint32_t length)
{
    int ret = BK_FAIL;

    if (s_session_ctx.ctrl_session_lock_inited)
    {
        rtos_lock_mutex(&s_session_ctx.ctrl_session_lock);
    }

    s_session_ctx.rx_session_id = sid;
    ret = ntwk_trans_pack_rx_handler(chan_type, data, length);
    s_session_ctx.rx_session_id = NTWK_INVALID_SESSION_ID;

    if (s_session_ctx.ctrl_session_lock_inited)
    {
        rtos_unlock_mutex(&s_session_ctx.ctrl_session_lock);
    }

    return ret;
}

bk_err_t ntwk_trans_register_ctrl_recv_cb(ntwk_trans_recv_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->cntrl_chan != NULL)
    {
        s_ntwk_trans_ctxt->cntrl_chan->recive = cb;
        return BK_OK;
    }

    return BK_FAIL;
}

bk_err_t ntwk_trans_register_session_recv_cb(ntwk_trans_session_recv_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    s_session_ctx.recv_cb = cb;
    return BK_OK;
}

bk_err_t ntwk_trans_register_video_recv_cb(ntwk_trans_recv_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->video_chan != NULL)
    {
        s_ntwk_trans_ctxt->video_chan->recive = cb;
        return BK_OK;
    }

    return BK_FAIL;
}
bk_err_t ntwk_trans_register_audio_recv_cb(ntwk_trans_recv_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    if (s_ntwk_trans_ctxt->audio_chan != NULL)
    {
        s_ntwk_trans_ctxt->audio_chan->recive = cb;
        return BK_OK;
    }

    return BK_FAIL;
}


bk_err_t ntwk_trans_register_unfragment_malloc_cb(chan_type_t chan, ntwk_trans_unfragment_malloc_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    return ntwk_unfragment_register_malloc_cb(chan, cb);
}

bk_err_t ntwk_trans_register_unfragment_send_cb(chan_type_t chan, ntwk_trans_unfragment_send_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    return ntwk_unfragment_register_send_cb(chan, cb);
}

bk_err_t ntwk_trans_register_unfragment_free_cb(chan_type_t chan, ntwk_trans_unfragment_free_cb_t cb)
{
    if (s_ntwk_trans_ctxt == NULL || !s_ntwk_trans_ctxt->initialized)
    {
        LOGE("%s, context not initialized\n", __func__);
        return BK_FAIL;
    }

    return ntwk_unfragment_register_free_cb(chan, cb);
}

#if CONFIG_NTWK_CLIENT_SERVICE_ENABLE
bk_err_t ntwk_trans_set_server_net_info(ntwk_server_net_info_t *net_info)
{
    if (!net_info)
    {
        LOGE("%s, invalid parameter\n", __func__);
        return BK_ERR_PARAM;
    }

    os_memset(&s_ntwk_server_net_info, 0, sizeof(ntwk_server_net_info_t));

    os_memcpy(&s_ntwk_server_net_info, net_info, sizeof(ntwk_server_net_info_t));

    LOGI("%s, server net info configured\n", __func__);

    return BK_OK;
}

ntwk_server_net_info_t *ntwk_trans_get_server_net_info(void)
{
    return &s_ntwk_server_net_info;
}

#endif