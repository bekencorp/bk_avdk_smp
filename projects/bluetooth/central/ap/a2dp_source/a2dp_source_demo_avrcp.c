#include <stdio.h>
#include <stdlib.h>
#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"
#include <components/log.h>
#include "components/bluetooth/bk_dm_avrcp.h"

#include "bk_avrcp_tg_service.h"
#include "bk_avrcp_ct_service.h"

#include "a2dp_source_demo_avrcp.h"
#include "a2dp_source_demo.h"

#define TAG "avrcp_demo"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static uint8_t s_avrcp_demo_inited;

static bk_avrcp_playback_stat_t a2dp_playstatus_to_avrcp_playback(int status)
{
    switch (status)
    {
    case A2DP_PLAY_STATUS_STOPPED:
        return BK_AVRCP_PLAYBACK_STOPPED;

    case A2DP_PLAY_STATUS_PLAYING:
        return BK_AVRCP_PLAYBACK_PLAYING;

    case A2DP_PLAY_STATUS_PAUSED:
        return BK_AVRCP_PLAYBACK_PAUSED;

    case A2DP_PLAY_STATUS_FWD_SEEK:
        return BK_AVRCP_PLAYBACK_FWD_SEEK;

    case A2DP_PLAY_STATUS_REV_SEEK:
        return BK_AVRCP_PLAYBACK_REV_SEEK;

    default:
        return BK_AVRCP_PLAYBACK_ERROR;
    }
}

/* Passthrough key from the peer -> local music control policy (demo owned). */
static void avrcp_demo_handle_passthrough(uint8_t key)
{
    switch (key)
    {
    case BK_AVRCP_PT_CMD_PLAY:
        bt_a2dp_source_demo_music_play(1, NULL);
        break;

    case BK_AVRCP_PT_CMD_STOP:
        bt_a2dp_source_demo_music_stop();
        break;

    case BK_AVRCP_PT_CMD_PAUSE:
        bt_a2dp_source_demo_music_pause();
        break;

    case BK_AVRCP_PT_CMD_FORWARD:
        bt_a2dp_source_demo_music_next();
        break;

    case BK_AVRCP_PT_CMD_BACKWARD:
        bt_a2dp_source_demo_music_prev();
        break;

    case BK_AVRCP_PT_CMD_REWIND:
    case BK_AVRCP_PT_CMD_FAST_FORWARD:
    default:
        return;
    }

    bt_avrcp_demo_report_playback(a2dp_playstatus_to_avrcp_playback(bt_a2dp_source_demo_get_play_status()));
}

static void on_avrcp_tg_evt(bk_avrcp_tg_evt_t evt, void *arg, void *user_data)
{
    (void)user_data;

    switch (evt)
    {
    case BK_AVRCP_TG_EVT_CONNECTED:
        LOGI("avrcp tg connected\n");
        /* sync the peer with the current playback status */
        bt_avrcp_demo_report_playback(a2dp_playstatus_to_avrcp_playback(bt_a2dp_source_demo_get_play_status()));
        break;

    case BK_AVRCP_TG_EVT_DISCONNECTED:
        LOGI("avrcp tg disconnected\n");
        break;

    case BK_AVRCP_TG_EVT_PASSTHROUGH:
    {
        bk_avrcp_tg_passthrough_t *pt = (bk_avrcp_tg_passthrough_t *)arg;
        avrcp_demo_handle_passthrough(pt->key_code);
        break;
    }

    default:
        break;
    }
}

static void on_avrcp_ct_evt(bk_avrcp_ct_evt_t evt, void *arg, void *user_data)
{
    (void)user_data;

    switch (evt)
    {
    case BK_AVRCP_CT_EVT_REMOTE_VOLUME_CHANGED:
        LOGI("peer volume changed: %d\n", *(uint8_t *)arg);
        break;

    case BK_AVRCP_CT_EVT_REMOTE_BATTERY_CHANGED:
        LOGI("peer battery changed: %d\n", *(uint8_t *)arg);
        break;

    case BK_AVRCP_CT_EVT_SET_ABS_VOLUME_RSP:
    {
        bk_avrcp_ct_abs_vol_rsp_t *rsp = (bk_avrcp_ct_abs_vol_rsp_t *)arg;

        if (rsp->status)
        {
            LOGW("set abs vol fail, maybe remote unsupport\n");
        }
        else
        {
            LOGI("set abs vol success %d\n", rsp->volume);
        }

        break;
    }

    default:
        break;
    }
}

/*
 * Central-specific AVRCP SDP feature trimming, kept as demo policy:
 * as an A2DP source the device does not need CT category 1/3/4 or TG
 * category 2/3/4 and the various browsing/cover-art sub-features.
 */
static void avrcp_demo_trim_sdp_feature(void)
{
    uint16_t feat = 0;
    uint16_t allow = 0;

    bk_bt_avrcp_ct_sdp_feature_operation(BK_AVRCP_SDP_FEATURE_API_METHOD_GET_ALLOWED, &allow);
    bk_bt_avrcp_ct_sdp_feature_operation(BK_AVRCP_SDP_FEATURE_API_METHOD_GET_CURRENT_ENABLE, &feat);
    LOGI("current ct enable 0x%x\n", feat);
    feat &= ~(BK_AVRCP_SDP_FEATURE_CT_CAT_1 //central doesn't need cat 1 as ct
              //| BK_AVRCP_SDP_FEATURE_CT_CAT_2 //As central, if you need disable abs vol as ct, disable ct category 2 here.
              | BK_AVRCP_SDP_FEATURE_CT_CAT_3 | BK_AVRCP_SDP_FEATURE_CT_CAT_4
              | BK_AVRCP_SDP_FEATURE_CT_SUPPORT_BROWSING | BK_AVRCP_SDP_FEATURE_CT_SUPPORT_CA_GIP | BK_AVRCP_SDP_FEATURE_CT_SUPPORT_CA_GI | BK_AVRCP_SDP_FEATURE_CT_SUPPORT_CA_GLT);
    feat &= allow;
    bk_bt_avrcp_ct_sdp_feature_operation(BK_AVRCP_SDP_FEATURE_API_METHOD_SET, &feat);

    feat = 0;
    allow = 0;
    bk_bt_avrcp_tg_sdp_feature_operation(BK_AVRCP_SDP_FEATURE_API_METHOD_GET_ALLOWED, &allow);
    bk_bt_avrcp_tg_sdp_feature_operation(BK_AVRCP_SDP_FEATURE_API_METHOD_GET_CURRENT_ENABLE, &feat);
    LOGI("current tg enable 0x%x\n", feat);
    feat &= ~(BK_AVRCP_SDP_FEATURE_TG_CAT_2 //central doesn't need cat 2 as tg
              | BK_AVRCP_SDP_FEATURE_TG_CAT_3 | BK_AVRCP_SDP_FEATURE_TG_CAT_4
              | BK_AVRCP_SDP_FEATURE_TG_PLAYER_APP_SET | BK_AVRCP_SDP_FEATURE_TG_GROUP_NAV | BK_AVRCP_SDP_FEATURE_TG_SUPPORT_BROWSING | BK_AVRCP_SDP_FEATURE_TG_SUPPORT_MULT_MEDIA_PA
              | BK_AVRCP_SDP_FEATURE_TG_SUPPORT_CA);
    feat &= allow;
    bk_bt_avrcp_tg_sdp_feature_operation(BK_AVRCP_SDP_FEATURE_API_METHOD_SET, &feat);
}

int bt_avrcp_demo_report_playback(uint8_t status)
{
    return bk_avrcp_tg_notify_playback_status(status);
}

int bt_avrcp_demo_report_batt_status(uint8_t status)
{
    return bk_avrcp_tg_notify_battery_status(status);
}

int bt_avrcp_demo_report_play_pos(uint32_t pos)
{
    return bk_avrcp_tg_notify_play_pos(pos);
}

int bt_avrcp_demo_report_track_change(void)
{
    /* demo has no real playlist UID; use an incrementing id so the peer CT
     * treats every play/next/prev as a new track and re-fetches attributes. */
    static uint64_t s_track_id;
    return bk_avrcp_tg_notify_track_change(++s_track_id);
}

int bt_avrcp_demo_set_remote_abs_vol(uint32_t vol)
{
    return bk_avrcp_ct_send_absolute_volume((uint8_t)vol);
}

int bt_avrcp_is_ready(void)
{
    return s_avrcp_demo_inited && bk_avrcp_tg_is_connected();
}

int bt_avrcp_demo_init(void)
{
    bk_avrcp_tg_cfg_t tg_cfg =
    {
        .player_mode = 1,
    };
    bk_avrcp_ct_cfg_t ct_cfg =
    {
        .auto_ct_connect_after_a2dp = 0,
        .remote_volume_mode = 1,
    };

    if (s_avrcp_demo_inited)
    {
        LOGW("already init\n");
        return -1;
    }

    bk_avrcp_tg_register_event_cb(on_avrcp_tg_evt, NULL);
    bk_avrcp_ct_register_event_cb(on_avrcp_ct_evt, NULL);

    bk_avrcp_tg_service_init(&tg_cfg);
    bk_avrcp_ct_service_init(&ct_cfg);

    avrcp_demo_trim_sdp_feature();

    s_avrcp_demo_inited = 1;
    return 0;
}
