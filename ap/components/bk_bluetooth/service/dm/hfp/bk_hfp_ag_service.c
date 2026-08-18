/**
 * @file bk_hfp_ag_service.c
 *
 * HFP AG (Audio Gateway) service wrapper. Owns the AG lifecycle and SCO audio
 * engine; forwards AG events to the application policy callback.
 */

#include <components/system.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>

#include "bk_hfp_ag_service.h"
#include "hfp_ag_audio.h"
#include "components/bluetooth/bk_dm_hfp_ag.h"
#include "components/log.h"
#include "bt_manager.h"

#define TAG "bk_hfp_ag"

enum
{
    HFP_AG_SRV_DEBUG_LEVEL_ERROR,
    HFP_AG_SRV_DEBUG_LEVEL_WARNING,
    HFP_AG_SRV_DEBUG_LEVEL_INFO,
    HFP_AG_SRV_DEBUG_LEVEL_DEBUG,
    HFP_AG_SRV_DEBUG_LEVEL_VERBOSE,
};

#define HFP_AG_SRV_DEBUG_LEVEL HFP_AG_SRV_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(HFP_AG_SRV_DEBUG_LEVEL >= HFP_AG_SRV_DEBUG_LEVEL_ERROR)   BK_LOGE(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(HFP_AG_SRV_DEBUG_LEVEL >= HFP_AG_SRV_DEBUG_LEVEL_WARNING) BK_LOGW(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(HFP_AG_SRV_DEBUG_LEVEL >= HFP_AG_SRV_DEBUG_LEVEL_INFO)    BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(HFP_AG_SRV_DEBUG_LEVEL >= HFP_AG_SRV_DEBUG_LEVEL_DEBUG)   BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(HFP_AG_SRV_DEBUG_LEVEL >= HFP_AG_SRV_DEBUG_LEVEL_VERBOSE) BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

typedef struct
{
    uint8_t inited;
    uint8_t connected;             /* service level (SLC) connected */
    uint8_t bt_manager_index;
    uint8_t peer_addr[6];
    uint8_t peer_valid;
    bk_hf_codec_type_t audio_codec;
    uint16_t air_tx_packet_len;
    bk_bt_hf_ag_cb_t user_cb;
} hfp_ag_service_ctx_t;

static hfp_ag_service_ctx_t s_ag =
{
    .bt_manager_index = 0xFF,
    .audio_codec = CODEC_VOICE_CVSD,
};

static void hfp_ag_service_recv_cb(const uint8_t *buf, uint32_t len)
{
    hfp_ag_audio_service_feed_rx(buf, len);
}

static void hfp_ag_service_cb(bk_hf_ag_cb_event_t event, bk_hf_ag_cb_param_t *param)
{
    if (!param)
    {
        LOGE("null param");
        return;
    }

    switch (event)
    {
        case BK_HF_AG_CONNECTION_STATE_EVT:
        {
            os_memcpy(s_ag.peer_addr, param->remote_addr, sizeof(s_ag.peer_addr));
            s_ag.peer_valid = 1;

            LOGI("conn state=%d peer_feat=0x%x chld=0x%x",
                 param->conn_stat.state, param->conn_stat.peer_feat, param->conn_stat.chld_feat);

            if (param->conn_stat.state == BK_HF_AG_CONNECTION_STATE_SLC_CONNECTED)
            {
                s_ag.connected = 1;
                bt_manager_set_connect_state(BT_STATE_PROFILE_CONNECTED);
            }
            else if (param->conn_stat.state == BK_HF_AG_CONNECTION_STATE_DISCONNECTED)
            {
                s_ag.connected = 0;
                hfp_ag_audio_service_stop();
            }
        }
        break;

        case BK_HF_AG_AUDIO_STATE_EVT:
        {
            uint8_t connected = (param->audio_stat.state == BK_HF_AG_AUDIO_STATE_CONNECTED) ||
                                (param->audio_stat.state == BK_HF_AG_AUDIO_STATE_CONNECTED_MSBC);

            LOGI("audio state=%d codec=%d interval=%d tx=%d rx=%d ptype=%d",
                 param->audio_stat.state, param->audio_stat.codec, param->audio_stat.interval,
                 param->audio_stat.tx_packet_len, param->audio_stat.rx_packet_len, param->audio_stat.packet_type);

            s_ag.air_tx_packet_len = param->audio_stat.tx_packet_len;

            if (connected)
            {
                bk_hf_codec_type_t codec = (param->audio_stat.state == BK_HF_AG_AUDIO_STATE_CONNECTED_MSBC)
                                           ? CODEC_VOICE_MSBC : param->audio_stat.codec;
                s_ag.audio_codec = codec;
                hfp_ag_audio_service_start(codec, s_ag.peer_addr, s_ag.air_tx_packet_len);
            }
            else if (param->audio_stat.state == BK_HF_AG_AUDIO_STATE_DISCONNECTED)
            {
                hfp_ag_audio_service_stop();
            }
        }
        break;

        case BK_HF_AG_VOLUME_CONTROL_EVT:
        {
            if (param->volume_control.type == BK_HF_VOLUME_CONTROL_TARGET_SPK)
            {
                hfp_ag_audio_service_set_spk_gain((uint8_t)param->volume_control.volume);
            }
            else
            {
                hfp_ag_audio_service_set_mic_gain((uint8_t)param->volume_control.volume);
            }
        }
        break;

        case BK_HF_AG_BCS_RESPONSE_EVT:
            s_ag.audio_codec = param->bcs_rep.codec;
            break;

        default:
            break;
    }

    if (s_ag.user_cb)
    {
        s_ag.user_cb(event, param);
    }
}

static void hfp_ag_service_bt_connect(uint8_t *remote_addr)
{
    if (remote_addr)
    {
        bk_bt_hf_ag_slc_connect(remote_addr);
    }
}

static void hfp_ag_service_bt_disconnect(uint8_t *remote_addr)
{
    uint8_t *dev = bt_manager_get_connected_device();
    bk_bt_hf_ag_slc_disconnect(dev ? dev : (s_ag.peer_valid ? s_ag.peer_addr : remote_addr));
}

int bk_hfp_ag_service_register_cb(bk_bt_hf_ag_cb_t user_cb)
{
    s_ag.user_cb = user_cb;
    return BK_OK;
}

int bk_hfp_ag_service_init(void)
{
    int ret;

    LOGI("enter");

    if (s_ag.inited)
    {
        LOGE("already init");
        return BK_OK;
    }

    ret = bk_bt_hf_ag_register_callback(hfp_ag_service_cb);
    if (ret)
    {
        LOGE("register_callback err %d", ret);
        return -1;
    }

    ret = bk_bt_hf_ag_init();
    if (ret)
    {
        LOGE("init err %d", ret);
        return -1;
    }

    /* Configure AG feature bitmaps after init and before SLC setup. SDP
     * SupportedFeatures (BK_HF_AG_SDP_FEAT_*) and +BRSF (BK_HF_AG_FEAT_*) are
     * different bitfields; query the allowed set, then enable a sane subset. */
    {
        uint16_t sdp_feat  = 0;
        uint32_t brsf_feat = 0;
        uint32_t chld_feat = 0;

        bk_bt_hf_ag_sdp_feature_operation(BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED, &sdp_feat);
        bk_bt_hf_ag_brsf_feature_operation(BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED, &brsf_feat);
        bk_bt_hf_ag_chld_feature_operation(BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED, &chld_feat);

        sdp_feat  &= (BK_HF_AG_SDP_FEAT_3WAY | BK_HF_AG_SDP_FEAT_WBS);
        brsf_feat &= (BK_HF_AG_FEAT_3WAY | BK_HF_AG_FEAT_REJECT | BK_HF_AG_FEAT_ECS |
                      BK_HF_AG_FEAT_ECC  | BK_HF_AG_FEAT_EXTERR | BK_HF_AG_FEAT_CODEC);
        chld_feat &= (BK_HF_CHLD_FEAT_REL | BK_HF_CHLD_FEAT_REL_ACC | BK_HF_CHLD_FEAT_HOLD_ACC |
                      BK_HF_CHLD_FEAT_MERGE | BK_HF_CHLD_FEAT_MERGE_DETACH);

        bk_bt_hf_ag_sdp_feature_operation(BK_HF_AG_FEATURE_API_METHOD_SET, &sdp_feat);
        bk_bt_hf_ag_brsf_feature_operation(BK_HF_AG_FEATURE_API_METHOD_SET, &brsf_feat);
        bk_bt_hf_ag_chld_feature_operation(BK_HF_AG_FEATURE_API_METHOD_SET, &chld_feat);

        LOGI("AG features set: sdp=0x%x brsf=0x%x chld=0x%x",
             sdp_feat, (unsigned)brsf_feat, (unsigned)chld_feat);
    }

    {
        btm_callback_s btm_cb =
        {
            .start_connect_cb = hfp_ag_service_bt_connect,
            .start_disconnect_cb = hfp_ag_service_bt_disconnect,
        };
        s_ag.bt_manager_index = bt_manager_register_callback(&btm_cb);
    }

    /* incoming SCO PCM (HF mic -> AG speaker) for the two-way voice path */
    bk_bt_hf_ag_register_data_callback(hfp_ag_service_recv_cb, NULL);

    s_ag.inited = 1;
    return BK_OK;
}

int bk_hfp_ag_service_deinit(void)
{
    LOGI("enter");

    if (!s_ag.inited)
    {
        LOGE("already deinit");
        return BK_OK;
    }

    hfp_ag_audio_service_stop();

    if (s_ag.connected && s_ag.peer_valid)
    {
        bk_bt_hf_ag_slc_disconnect(s_ag.peer_addr);
    }

    if (s_ag.bt_manager_index != 0xFF)
    {
        bt_manager_unregister_callback(s_ag.bt_manager_index);
        s_ag.bt_manager_index = 0xFF;
    }

    bk_bt_hf_ag_register_data_callback(NULL, NULL);
    bk_bt_hf_ag_register_callback(NULL);
    bk_bt_hf_ag_deinit();

    os_memset(&s_ag, 0, sizeof(s_ag));
    s_ag.bt_manager_index = 0xFF;
    s_ag.audio_codec = CODEC_VOICE_CVSD;
    return BK_OK;
}

const uint8_t *bk_hfp_ag_service_get_peer(void)
{
    return s_ag.peer_valid ? s_ag.peer_addr : NULL;
}

uint8_t bk_hfp_ag_service_is_connected(void)
{
    return s_ag.connected;
}

int bk_hfp_ag_service_connect(const uint8_t bda[6])
{
    if (!bda)
    {
        return BK_FAIL;
    }

    os_memcpy(s_ag.peer_addr, bda, sizeof(s_ag.peer_addr));
    s_ag.peer_valid = 1;
    return bk_bt_hf_ag_slc_connect((uint8_t *)bda);
}

int bk_hfp_ag_service_disconnect(void)
{
    return s_ag.peer_valid ? bk_bt_hf_ag_slc_disconnect(s_ag.peer_addr) : BK_FAIL;
}

int bk_hfp_ag_service_audio_connect(void)
{
    return s_ag.peer_valid ? bk_bt_hf_ag_audio_connect(s_ag.peer_addr) : BK_FAIL;
}

int bk_hfp_ag_service_audio_disconnect(void)
{
    hfp_ag_audio_service_stop();
    return s_ag.peer_valid ? bk_bt_hf_ag_audio_disconnect(s_ag.peer_addr) : BK_FAIL;
}

int bk_hfp_ag_service_set_codec(uint8_t msbc)
{
    return s_ag.peer_valid
           ? bk_bt_hf_ag_set_codec(s_ag.peer_addr, msbc ? CODEC_VOICE_MSBC : CODEC_VOICE_CVSD)
           : BK_FAIL;
}
