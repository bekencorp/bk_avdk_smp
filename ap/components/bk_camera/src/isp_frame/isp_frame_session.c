#include <string.h>
#include <common/bk_err.h>
#include <components/log.h>
#include "common/network_transfer_common.h"
#include "network_transfer.h"
#include "network_type.h"
#include "ntwk_sdp.h"
#include "isp_frame_priv.h"

#define TAG "isp_frame"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static isp_frame_session_ctx_t s_isp_frame_ctx;

isp_frame_session_ctx_t *isp_frame_session_get_ctx(void)
{
    return &s_isp_frame_ctx;
}

static void isp_frame_network_event(ntwk_trans_event_t *event)
{
    if (event == NULL)
    {
        return;
    }
    LOGI("chan=%d event=%d param=%d\n", event->chan_type, event->code, event->param);
}

static int isp_frame_ctrl_recv(uint8_t *data, uint32_t length)
{
    return isp_frame_protocol_handle(data, length);
}

static int isp_frame_video_recv(uint8_t *data, uint32_t length)
{
    (void)data;
    (void)length;
    return 0;
}

static bk_err_t isp_frame_service_init(isp_frame_session_service_t service)
{
    if (service == ISP_FRAME_SESSION_SERVICE_UDP)
    {
        return bk_udp_trans_service_init("isp_udp_service");
    }
    return bk_tcp_trans_service_init("isp_tcp_service");
}

static bk_err_t isp_frame_service_deinit(isp_frame_session_service_t service)
{
    if (service == ISP_FRAME_SESSION_SERVICE_UDP)
    {
        return bk_udp_trans_service_deinit();
    }
    return bk_tcp_trans_service_deinit();
}

static bk_err_t isp_frame_session_configure(const isp_frame_session_config_t *config,
                                        uint8_t started)
{
    isp_frame_session_config_t default_config = {
        .service = ISP_FRAME_SESSION_SERVICE_TCP,
        .auto_force_idr = 1,
    };

    if (config != NULL)
    {
        default_config = *config;
    }

    if (default_config.service > ISP_FRAME_SESSION_SERVICE_UDP)
    {
        LOGE("invalid service %u\n", default_config.service);
        return BK_ERR_PARAM;
    }

    memset(&s_isp_frame_ctx, 0, sizeof(s_isp_frame_ctx));
    s_isp_frame_ctx.config = default_config;
    s_isp_frame_ctx.initialized = 1;
    s_isp_frame_ctx.started = started;
    return BK_OK;
}

static void isp_frame_sdp_start(isp_frame_session_service_t service)
{
    int ret;
    const char *service_name;
    uint32_t image_port;
    uint32_t audio_port;

    if (service == ISP_FRAME_SESSION_SERVICE_UDP)
    {
        service_name = "isp-udp";
        image_port = NTWK_TRANS_UDP_VIDEO_PORT;
        audio_port = NTWK_TRANS_UDP_AUDIO_PORT;
    }
    else
    {
        service_name = "isp-tcp";
        image_port = NTWK_TRANS_TCP_VIDEO_PORT;
        audio_port = NTWK_TRANS_TCP_AUDIO_PORT;
    }

    ret = ntwk_sdp_start(service_name,
                         NTWK_TRANS_CMD_PORT,
                         image_port,
                         audio_port);

    if (ret != BK_OK)
    {
        LOGW("sdp start failed: %d\n", ret);
        return;
    }

    LOGI("SDP started: service=%s scan_udp=%u ctrl=%u img=%u audio=%u\n",
         service_name, UDP_SDP_REMOTE_PORT, NTWK_TRANS_CMD_PORT, image_port, audio_port);
}

bk_err_t isp_frame_session_init(const isp_frame_session_config_t *config)
{
    bk_err_t ret;

    if (s_isp_frame_ctx.initialized)
    {
        return BK_OK;
    }

    ret = isp_frame_session_configure(config, 0);
    if (ret != BK_OK)
    {
        return ret;
    }

    ret = isp_frame_service_init(s_isp_frame_ctx.config.service);
    if (ret != BK_OK)
    {
        LOGE("network transfer service init failed: %d\n", ret);
        memset(&s_isp_frame_ctx, 0, sizeof(s_isp_frame_ctx));
        return ret;
    }

    return BK_OK;
}

bk_err_t isp_frame_session_init_local(const isp_frame_session_config_t *config)
{
    if (s_isp_frame_ctx.initialized)
    {
        s_isp_frame_ctx.started = 1;
        return BK_OK;
    }
    return isp_frame_session_configure(config, 1);
}

bk_err_t isp_frame_session_deinit(void)
{
    isp_frame_session_service_t service = s_isp_frame_ctx.config.service;

    if (!s_isp_frame_ctx.initialized)
    {
        return BK_OK;
    }

    isp_frame_session_stop();
    isp_frame_service_deinit(service);
    memset(&s_isp_frame_ctx, 0, sizeof(s_isp_frame_ctx));
    return BK_OK;
}

bk_err_t isp_frame_session_deinit_local(void)
{
    if (!s_isp_frame_ctx.initialized)
    {
        return BK_OK;
    }
    isp_frame_session_stop();
    memset(&s_isp_frame_ctx, 0, sizeof(s_isp_frame_ctx));
    return BK_OK;
}

bk_err_t isp_frame_session_register_media_ops(isp_frame_session_media_start_cb_t start,
                                          isp_frame_session_media_stop_cb_t stop,
                                          void *user_data)
{
    s_isp_frame_ctx.media_start = start;
    s_isp_frame_ctx.media_stop = stop;
    s_isp_frame_ctx.media_user_data = user_data;
    return BK_OK;
}

bk_err_t isp_frame_session_start(void)
{
    bk_err_t ret;
    ntwk_trans_ctxt_t *ctxt;

    if (!s_isp_frame_ctx.initialized)
    {
        LOGE("demo is not initialized\n");
        return BK_ERR_STATE;
    }

    if (s_isp_frame_ctx.started)
    {
        return BK_OK;
    }

    ctxt = ntwk_trans_get_ctxt();
    if (ctxt != NULL && ctxt->video_chan != NULL)
    {
        ctxt->video_chan->vid_type = IMAGE_YUV;
    }

    ntwk_trans_register_msg_event_cb(isp_frame_network_event);
    ntwk_trans_register_ctrl_recv_cb(isp_frame_ctrl_recv);
    ntwk_trans_register_video_recv_cb(isp_frame_video_recv);

    ret = ntwk_trans_chan_start(NTWK_TRANS_CHAN_CTRL, NULL);
    if (ret != BK_OK)
    {
        LOGE("ctrl channel start failed: %d\n", ret);
        return ret;
    }

    ret = ntwk_trans_chan_start(NTWK_TRANS_CHAN_VIDEO, NULL);
    if (ret != BK_OK)
    {
        LOGE("video channel start failed: %d\n", ret);
        ntwk_trans_chan_stop(NTWK_TRANS_CHAN_CTRL);
        return ret;
    }

    isp_frame_sdp_start(s_isp_frame_ctx.config.service);
    s_isp_frame_ctx.started = 1;
    s_isp_frame_ctx.network_started = 1;
    return BK_OK;
}

bk_err_t isp_frame_session_stop(void)
{
    if (!s_isp_frame_ctx.started)
    {
        return BK_OK;
    }

    if (s_isp_frame_ctx.media_ready && s_isp_frame_ctx.media_stop != NULL)
    {
        (void)s_isp_frame_ctx.media_stop(s_isp_frame_ctx.media_user_data);
    }
    (void)isp_frame_stream_stop();
    if (s_isp_frame_ctx.network_started)
    {
        ntwk_sdp_stop();
        ntwk_trans_chan_stop(NTWK_TRANS_CHAN_VIDEO);
        ntwk_trans_chan_stop(NTWK_TRANS_CHAN_CTRL);
    }
    s_isp_frame_ctx.started = 0;
    s_isp_frame_ctx.network_started = 0;
    s_isp_frame_ctx.media_ready = 0;
    return BK_OK;
}
