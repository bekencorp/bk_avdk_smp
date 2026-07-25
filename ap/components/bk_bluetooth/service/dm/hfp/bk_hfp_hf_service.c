#include <components/system.h>
#include <os/mem.h>
#include <os/os.h>
#include <os/str.h>

#include "bk_hfp_hf_service.h"
#include "components/bluetooth/bk_dm_hfp.h"
#include "components/log.h"
#include "bt_manager.h"

#define TAG "bk_hfp_srv"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct
{
    uint8_t inited;
    uint8_t connected;            /* service level (SLC) connected */
    uint8_t bt_manager_index;
    uint8_t profile_peer_addr[6]; /* service level (SLC) peer */
    uint8_t audio_peer_addr[6];   /* SCO audio peer */
    uint8_t codec;
    beken_semaphore_t disconnect_sema;
    bk_hfp_hf_event_cb_t event_cb;
    void *event_user_data;
} hfp_hf_service_ctx_t;

static hfp_hf_service_ctx_t s_hfp = { .bt_manager_index = 0xFF };

static void hfp_service_emit(bk_hfp_hf_evt_t evt, void *arg)
{
    if (s_hfp.event_cb)
    {
        s_hfp.event_cb(evt, arg, s_hfp.event_user_data);
    }
}

static void hfp_service_cb(bk_hf_client_cb_event_t event, bk_hf_client_cb_param_t *param)
{
    if (!param)
    {
        LOGE("%s null param\n", __func__);
        return;
    }

    LOGI("%s event: %d, addr:%02x:%02x:%02x:%02x:%02x:%02x\r\n", __func__, event,
         param->remote_bda[0], param->remote_bda[1], param->remote_bda[2],
         param->remote_bda[3], param->remote_bda[4], param->remote_bda[5]);

    switch (event)
    {
        case BK_HF_CLIENT_AUDIO_STATE_EVT:
        {
            LOGI("HFP client audio state: %d\r\n", param->audio_state.state);

            if (BK_HF_CLIENT_AUDIO_STATE_DISCONNECTED == param->audio_state.state)
            {
                hfp_service_emit(BK_HFP_HF_EVT_AUDIO_DISCONNECTED, NULL);
            }
            else if (BK_HF_CLIENT_AUDIO_STATE_CONNECTED == param->audio_state.state)
            {
                bk_hfp_hf_audio_info_t info = {0};

                s_hfp.codec = param->audio_state.codec_type;
                os_memcpy(s_hfp.audio_peer_addr, param->remote_bda, 6);
                os_memcpy(info.remote_bda, param->remote_bda, 6);
                info.codec = param->audio_state.codec_type;
                LOGI("sco connected to %02x:%02x:%02x:%02x:%02x:%02x, codec type %d\n",
                     info.remote_bda[5], info.remote_bda[4], info.remote_bda[3],
                     info.remote_bda[2], info.remote_bda[1], info.remote_bda[0], info.codec);

                hfp_service_emit(BK_HFP_HF_EVT_AUDIO_CONNECTED, &info);
            }
        }
        break;

        case BK_HF_CLIENT_CONNECTION_STATE_EVT:
        {
            if (param->conn_state.state == BK_HF_CLIENT_CONNECTION_STATE_SLC_CONNECTED)
            {
                bk_hfp_hf_conn_info_t conn = {0};

                LOGI("HFP service level connected, ag_feature:0x%x, ag_chld_feature:0x%x \n",
                     param->conn_state.peer_feat, param->conn_state.chld_feat);
                LOGI("HFP client connect to peer address: %02x:%02x:%02x:%02x:%02x:%02x \n",
                     param->remote_bda[0], param->remote_bda[1], param->remote_bda[2],
                     param->remote_bda[3], param->remote_bda[4], param->remote_bda[5]);
                os_memcpy(s_hfp.profile_peer_addr, param->remote_bda, sizeof(s_hfp.profile_peer_addr));
                s_hfp.connected = 1;

                os_memcpy(conn.remote_bda, param->remote_bda, sizeof(conn.remote_bda));
                conn.peer_feat = param->conn_state.peer_feat;
                conn.chld_feat = param->conn_state.chld_feat;
                hfp_service_emit(BK_HFP_HF_EVT_CONNECTED, &conn);
            }
            else if (param->conn_state.state == BK_HF_CLIENT_CONNECTION_STATE_DISCONNECTED)
            {
                bk_hfp_hf_conn_info_t conn = {0};

                LOGI("HFP disconnected \n");
                LOGI("HFP disconnect peer address: %02x:%02x:%02x:%02x:%02x:%02x \n",
                     param->remote_bda[0], param->remote_bda[1], param->remote_bda[2],
                     param->remote_bda[3], param->remote_bda[4], param->remote_bda[5]);
                os_memcpy(conn.remote_bda, param->remote_bda, sizeof(conn.remote_bda));
                os_memset(s_hfp.profile_peer_addr, 0, sizeof(s_hfp.profile_peer_addr));
                s_hfp.connected = 0;
                hfp_service_emit(BK_HFP_HF_EVT_DISCONNECTED, &conn);
                if (s_hfp.disconnect_sema)
                {
                    rtos_set_semaphore(&s_hfp.disconnect_sema);
                }
            }
        }
        break;

        case BK_HF_CLIENT_BVRA_EVT:
            LOGI("+BRVA: HPF voice recognition activation status: %d \n", param->bvra.value);
            break;
        case BK_HF_CLIENT_CIND_CALL_EVT:
        {
            bk_hfp_hf_call_info_t call = {0};

            call.status = param->call.status;
            LOGI("+CIND: HFP call staus:%d \n", param->call.status);
            hfp_service_emit(BK_HFP_HF_EVT_CALL_IND, &call);
        }
        break;
        case BK_HF_CLIENT_CIND_CALL_SETUP_EVT:
        {
            bk_hfp_hf_call_setup_info_t setup = {0};

            setup.status = param->call_setup.status;
            LOGI("+CIND: HFP call_setup status:%d \n", param->call_setup.status);
            hfp_service_emit(BK_HFP_HF_EVT_CALL_SETUP_IND, &setup);
        }
        break;
        case BK_HF_CLIENT_CIND_CALL_HELD_EVT:
            LOGI("+CIND: HFP call_hold status:%d \n", param->call_held.status);
            break;
        case BK_HF_CLIENT_CIND_SERVICE_AVAILABILITY_EVT:
            LOGI("+CIND: HFP service availability ind: %d\n", param->service_availability.status);
            break;
        case BK_HF_CLIENT_CIND_SIGNAL_STRENGTH_EVT:
            LOGI("+CIND: HFP signal strength ind: %d\n", param->signal_strength.value);
            break;
        case BK_HF_CLIENT_CIND_ROAMING_STATUS_EVT:
            LOGI("+CIND: HFP roming status:%d \n", param->roaming.status);
            break;
        case BK_HF_CLIENT_CIND_BATTERY_LEVEL_EVT:
            LOGI("+CIND: HFP battery ind:%d \n", param->battery_level.value);
            break;
        case BK_HF_CLIENT_COPS_CURRENT_OPERATOR_EVT:
            LOGI("+COPS: HFP network operator name:%s \n", param->cops.name);
            break;
        case BK_HF_CLIENT_BTRH_EVT:
        {
            bk_hfp_hf_btrh_info_t btrh = {0};

            btrh.status = param->btrh.status;
            LOGI("+BTRH: HFP Hold status: %d \n", param->btrh.status);
            hfp_service_emit(BK_HFP_HF_EVT_BTRH, &btrh);
        }
        break;
        case BK_HF_CLIENT_CLIP_EVT:
        {
            bk_hfp_hf_clip_info_t clip = {0};

            clip.number = (const char *)param->clip.number;
            clip.name = (const char *)param->clip.name;
            LOGI("+CLIP: HFP calling line number: %s, name:%s \n", param->clip.number, param->clip.name);
            hfp_service_emit(BK_HFP_HF_EVT_CLIP, &clip);
        }
        break;
        case BK_HF_CLIENT_CCWA_EVT:
            LOGI("+CCWA: HFP calling waiting number:%s, name: %s\n", param->ccwa.number, param->ccwa.name);
            break;
        case BK_HF_CLIENT_CLCC_EVT:
        {
            bk_hfp_hf_clcc_info_t clcc = {0};

            LOGI("+CLCC: HFP calls result dir:%d, idx:%d, mpty:%d, number:%s, status:%d \n",
                 param->clcc.dir, param->clcc.idx, param->clcc.mpty, param->clcc.number, param->clcc.status);

            clcc.idx = param->clcc.idx;
            clcc.dir = (int)param->clcc.dir;
            clcc.status = (int)param->clcc.status;
            clcc.mpty = (int)param->clcc.mpty;
            clcc.number = (const char *)param->clcc.number;
            hfp_service_emit(BK_HFP_HF_EVT_CLCC, &clcc);
        }
        break;

        case BK_HF_CLIENT_VOLUME_CONTROL_EVT:
        {
            bk_hfp_hf_volume_info_t vol = {0};

            vol.type = param->volume_control.type;
            vol.volume = param->volume_control.volume;
            if (param->volume_control.type == BK_HF_VOLUME_CONTROL_TARGET_SPK)
            {
                LOGI("+VGS: HPF Speaker gain: %d \n", param->volume_control.volume);
            }
            else if (param->volume_control.type == BK_HF_VOLUME_CONTROL_TARGET_MIC)
            {
                LOGI("+VGM: HPF Microphone gain: %d \n", param->volume_control.volume);
            }
            hfp_service_emit(BK_HFP_HF_EVT_VOLUME_CHANGED, &vol);
        }
        break;

        case BK_HF_CLIENT_AT_RESPONSE_EVT:
        {
            bk_hfp_hf_at_response_info_t at = {0};

            at.code = param->at_response.code;
            at.asso_cmd = param->at_response.asso_cmd;
            at.cme = param->at_response.cme;

            if (param->at_response.code == BK_HF_AT_RESPONSE_CODE_OK)
            {
                LOGI("AT_RESPONSE ok, asso_cmd %d\n", param->at_response.asso_cmd);
            }
            else if (param->at_response.code == BK_HF_AT_RESPONSE_CODE_CME)
            {
                LOGI("AT_RESPONSE cme err, cme code 0x%x, asso_cmd %d\n", param->at_response.cme, param->at_response.asso_cmd);
            }
            else
            {
                LOGI("AT_RESPONSE normal err 0x%x, asso_cmd %d\n", param->at_response.code, param->at_response.asso_cmd);
            }
            hfp_service_emit(BK_HFP_HF_EVT_AT_RESPONSE, &at);
        }
        break;

        case BK_HF_CLIENT_CNUM_EVT:
            LOGI("+CNUM: HFP subscriber number info, type:%d, number:%s \n", param->cnum.type, param->cnum.number);
            break;
        case BK_HF_CLIENT_BSIR_EVT:
            LOGI("+BSIR: HFP In-band Ring tone staus: %d\n", param->bsir.state);
            break;
        case BK_HF_CLIENT_BINP_EVT:
            LOGI("+BINP: HFP last voice tag record: %s \n", param->binp.number);
            break;
        case BK_HF_CLIENT_RING_IND_EVT:
            LOGI("RING HPF incoming call ind evt\n");
            hfp_service_emit(BK_HFP_HF_EVT_RING, NULL);
            break;

        case BK_HF_CLIENT_UNKNOWN_DATA_IND_EVT:
        {
            bk_hfp_hf_unknown_data_info_t unknown = {0};

            unknown.data = (const char *)param->unknown_data.data;
            unknown.len = param->unknown_data.data_len;
            LOGI("unknown data received (len %d)\n", param->unknown_data.data_len);
            hfp_service_emit(BK_HFP_HF_EVT_UNKNOWN_DATA, &unknown);
        }
        break;

        default:
            LOGW("Invalid HFP client event: %d\r\n", event);
            break;
    }
}

static void hfp_service_data_cb(const uint8_t *data, uint16_t data_len)
{
    bk_hfp_hf_voice_data_t voice = {0};

    if (!data || !data_len)
    {
        return;
    }

    voice.data = data;
    voice.len = data_len;
    hfp_service_emit(BK_HFP_HF_EVT_VOICE_DATA, &voice);
}

static void hfp_service_bt_connect(uint8_t *remote_addr)
{
    if (!remote_addr)
    {
        LOGE("%s null remote addr\n", __func__);
        return;
    }

    LOGI("%s %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
         remote_addr[5], remote_addr[4], remote_addr[3],
         remote_addr[2], remote_addr[1], remote_addr[0]);
    bk_bt_hf_client_connect(remote_addr);
}

static void hfp_service_bt_disconnect(uint8_t *remote_addr)
{
    if (!remote_addr)
    {
        LOGE("%s null remote addr\n", __func__);
        return;
    }

    LOGI("%s %02x:%02x:%02x:%02x:%02x:%02x\n", __func__,
         remote_addr[5], remote_addr[4], remote_addr[3],
         remote_addr[2], remote_addr[1], remote_addr[0]);
    bk_bt_hf_client_disconnect(remote_addr);
}

int bk_hfp_hf_register_event_cb(bk_hfp_hf_event_cb_t cb, void *user_data)
{
    s_hfp.event_cb = cb;
    s_hfp.event_user_data = user_data;
    return BK_OK;
}

int bk_hfp_hf_service_init(uint8_t msbc_supported)
{
    int ret;

    LOGI("%s\r\n", __func__);

    if (s_hfp.inited)
    {
        LOGE("%s already init\n", __func__);
        return BK_OK;
    }

    ret = bk_bt_hf_client_register_callback(hfp_service_cb);
    if (ret)
    {
        LOGE("%s bk_bt_hf_client_register_callback err %d\n", __func__, ret);
        return -1;
    }

    ret = bk_bt_hf_client_init(msbc_supported);
    if (ret)
    {
        LOGE("%s bk_bt_hf_client_init err %d\n", __func__, ret);
        return -1;
    }

    ret = bk_bt_hf_client_register_data_callback(hfp_service_data_cb);
    if (ret)
    {
        LOGE("%s bk_bt_hf_client_register_data_callback err %d\n", __func__, ret);
        return -1;
    }

    {
        btm_callback_s btm_cb = {
            .start_connect_cb = hfp_service_bt_connect,
            .start_disconnect_cb = hfp_service_bt_disconnect,
        };
        s_hfp.bt_manager_index = bt_manager_register_callback(&btm_cb);
    }

    s_hfp.inited = 1;
    return ret;
}

int bk_hfp_hf_service_deinit(void)
{
    LOGI("%s\r\n", __func__);

    if (!s_hfp.inited)
    {
        LOGE("%s already deinit\n", __func__);
        return BK_OK;
    }

    if (s_hfp.connected)
    {
        if (!s_hfp.disconnect_sema)
        {
            if (rtos_init_semaphore(&s_hfp.disconnect_sema, 1) != BK_OK)
            {
                LOGE("%s disconnect sema init failed\n", __func__);
            }
        }

        bk_bt_hf_client_disconnect(s_hfp.profile_peer_addr);
        if (s_hfp.disconnect_sema)
        {
            rtos_get_semaphore(&s_hfp.disconnect_sema, 5000);
        }
    }

    if (s_hfp.bt_manager_index != 0xFF)
    {
        bt_manager_unregister_callback(s_hfp.bt_manager_index);
        s_hfp.bt_manager_index = 0xFF;
    }

    bk_bt_hf_client_register_data_callback(NULL);
    bk_bt_hf_client_register_callback(NULL);
    bk_bt_hf_client_deinit();

    if (s_hfp.disconnect_sema)
    {
        rtos_deinit_semaphore(&s_hfp.disconnect_sema);
        s_hfp.disconnect_sema = NULL;
    }

    os_memset(&s_hfp, 0, sizeof(s_hfp));
    s_hfp.bt_manager_index = 0xFF;
    return BK_OK;
}

int bk_hfp_hf_connect(const uint8_t bda[6])
{
    return bda ? bk_bt_hf_client_connect((uint8_t *)bda) : BK_FAIL;
}

int bk_hfp_hf_disconnect(const uint8_t bda[6])
{
    return bda ? bk_bt_hf_client_disconnect((uint8_t *)bda) : BK_FAIL;
}

int bk_hfp_hf_query_current_calls(void)
{
    return bk_bt_hf_client_query_current_calls(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_volume_update(uint8_t type, uint8_t volume)
{
    return bk_bt_hf_client_volume_update(s_hfp.profile_peer_addr, (bk_hf_volume_control_target_t)type, volume);
}

int bk_hfp_hf_send_custom_cmd(const char *cmd)
{
    return bk_bt_hf_client_send_custom_cmd(s_hfp.profile_peer_addr, cmd);
}

int bk_hfp_hf_query_current_operator_name(void)
{
    return bk_bt_hf_client_query_current_operator_name(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_retrieve_subscriber_info(void)
{
    return bk_bt_hf_client_retrieve_subscriber_info(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_send_dtmf(const char *code)
{
    return bk_bt_hf_client_send_dtmf(s_hfp.profile_peer_addr, code);
}

int bk_hfp_hf_request_last_voice_tag_number(void)
{
    return bk_bt_hf_client_request_last_voice_tag_number(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_send_nrec(void)
{
    return bk_bt_hf_client_send_nrec(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_start_voice_recognition(void)
{
    return bk_bt_hf_client_start_voice_recognition(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_stop_voice_recognition(void)
{
    return bk_bt_hf_client_stop_voice_recognition(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_dial(const char *num)
{
    return bk_bt_hf_client_dial(s_hfp.profile_peer_addr, num);
}

int bk_hfp_hf_dial_memory(int32_t location)
{
    return bk_bt_hf_client_dial_memory(s_hfp.profile_peer_addr, location);
}

int bk_hfp_hf_redial(void)
{
    return bk_bt_hf_client_redial(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_answer_call(void)
{
    return bk_bt_hf_client_answer_call(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_reject_call(void)
{
    return bk_bt_hf_client_reject_call(s_hfp.profile_peer_addr);
}

int bk_hfp_hf_send_chld_cmd(uint8_t op)
{
    return bk_bt_hf_client_send_chld_cmd(s_hfp.profile_peer_addr, (bk_hf_chld_type_t)op);
}

int bk_hfp_hf_send_btrh_cmd(uint8_t op)
{
    return bk_bt_hf_client_send_btrh_cmd(s_hfp.profile_peer_addr, (bk_hf_btrh_cmd_t)op);
}
