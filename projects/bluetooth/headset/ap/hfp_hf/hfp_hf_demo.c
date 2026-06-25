#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "hfp_hf_demo.h"

#include "components/bluetooth/bk_dm_hfp.h"

#include "hfp_hf_audio.h"
#include "bk_hfp_hf_service.h"

#define TAG "hfp_client"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define BT_AUDIO_HF_DEMO_MSG_COUNT       (30)

#define HF_AT_TEST 0
#define HF_AT_ENABLE_CMEE 1

enum
{
    BT_AUDIO_MSG_NULL = 0,
    BT_AUDIO_VOICE_START_MSG = 1,
    BT_AUDIO_VOICE_STOP_MSG  = 2,
    BT_AUDIO_VOICE_IND_MSG   = 3,
    BT_AUDIO_EXIT_MSG        = 4,
};

typedef struct
{
    uint8_t type;
    uint16_t len;
    char *data;
} bt_audio_hf_demo_msg_t;

enum
{
    HFP_STATUS_IDLE,
    HFP_STATUS_WAIT_QUERY_CALL,
    HFP_STATUS_WAIT_VGS,
    HFP_STATUS_WAIT_VGM,

    HFP_STATUS_WAIT_CUSTOM,
    HFP_STATUS_WAIT_QUERY_CURRENT_OP,
    HFP_STATUS_WAIT_RETRIEVE_SUB_INFO,
    HFP_STATUS_WAIT_SEND_VTS,
    HFP_STATUS_WAIT_REQ_LAST_TAG_NUM,
    HFP_STATUS_WAIT_NREC,
    HFP_STATUS_WAIT_VR_ENABLE,
    HFP_STATUS_WAIT_VR_DISBLE,
    HFP_STATUS_WAIT_DIAL,
    HFP_STATUS_WAIT_DIAL_HUP,
    HFP_STATUS_WAIT_DIAL_MEM,
    HFP_STATUS_WAIT_DIAL_MEM_HUP,
    HFP_STATUS_WAIT_REDAIL,
    HFP_STATUS_WAIT_REDAIL_HUP,
    HFP_STATUS_WAIT_DONE,
};

static uint8_t bt_audio_hfp_hf_codec = CODEC_VOICE_CVSD;
static uint8_t hfp_peer_addr [ 6 ] = {0};

static uint8_t s_hfp_status_mach = HFP_STATUS_IDLE;

static beken_queue_t bt_audio_hf_demo_msg_que = NULL;
static beken_thread_t bt_audio_hf_demo_thread_handle = NULL;

int bt_audio_hf_demo_task_init(void);

static void hf_post_simple_msg(uint8_t type)
{
    bt_audio_hf_demo_msg_t demo_msg;
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(bt_audio_hf_demo_msg_t));
    if (bt_audio_hf_demo_msg_que == NULL)
        return;

    demo_msg.type = type;
    demo_msg.len = 0;

    rc = rtos_push_to_queue(&bt_audio_hf_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);
    if (kNoErr != rc)
    {
        LOGI("%s, send queue failed\r\n", __func__);
    }
}

static void hf_post_voice_data(const uint8_t *data, uint16_t data_len)
{
    bt_audio_hf_demo_msg_t demo_msg;
    int rc = -1;

    os_memset(&demo_msg, 0x0, sizeof(bt_audio_hf_demo_msg_t));
    if (bt_audio_hf_demo_msg_que == NULL)
        return;

    demo_msg.data = (char *) os_malloc(data_len);
    if (demo_msg.data == NULL)
    {
        LOGI("%s, malloc failed\r\n", __func__);
        return;
    }

    os_memcpy(demo_msg.data, data, data_len);
    demo_msg.type = BT_AUDIO_VOICE_IND_MSG;
    demo_msg.len = data_len;

    rc = rtos_push_to_queue(&bt_audio_hf_demo_msg_que, &demo_msg, BEKEN_NO_WAIT);
    if (kNoErr != rc)
    {
        LOGI("%s, send queue failed\r\n", __func__);
        if (demo_msg.data)
        {
            os_free(demo_msg.data);
        }
    }
}

/*
 * Demo AT command walk-through, driven by AT response events. This is sample
 * behaviour (it dials/hangs up test numbers), so it stays in the project rather
 * than in the reusable HFP service.
 */
static void hfp_demo_run_at_state_machine(const bk_hfp_hf_at_response_info_t *at)
{
    (void)at;
#if !HF_AT_TEST
    switch(s_hfp_status_mach)
    {
    case HFP_STATUS_WAIT_QUERY_CALL:
        s_hfp_status_mach = HFP_STATUS_WAIT_VGS;
        bk_hfp_hf_volume_update(BK_HF_VOLUME_CONTROL_TARGET_SPK, 7);
        break;

    case HFP_STATUS_WAIT_VGS:
        s_hfp_status_mach = HFP_STATUS_WAIT_VGM;
        bk_hfp_hf_volume_update(BK_HF_VOLUME_CONTROL_TARGET_MIC, 7);
        break;

    case HFP_STATUS_WAIT_VGM:
        s_hfp_status_mach = HFP_STATUS_WAIT_DONE;
        LOGI("%s end op\n", __func__);
        break;

    }
#else
    switch(at->asso_cmd)
    {
    case BK_HF_AT_CMD_CLCC:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_QUERY_CALL)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_VGS;
            bk_hfp_hf_volume_update(BK_HF_VOLUME_CONTROL_TARGET_SPK, 7);
        }
        break;

    case BK_HF_AT_CMD_VGS:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_VGS)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_VGM;
            bk_hfp_hf_volume_update(BK_HF_VOLUME_CONTROL_TARGET_MIC, 7);
        }
        break;

    case BK_HF_AT_CMD_VGM:

        if(s_hfp_status_mach == HFP_STATUS_WAIT_VGM)
        {
#if HF_AT_ENABLE_CMEE
            s_hfp_status_mach = HFP_STATUS_WAIT_CUSTOM;
            bk_hfp_hf_send_custom_cmd("AT+CMEE=1");
        }
        break;

    case BK_HF_AT_CMD_CUSTOM:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_CUSTOM)
        {
#endif
            s_hfp_status_mach = HFP_STATUS_WAIT_QUERY_CURRENT_OP;
            bk_hfp_hf_query_current_operator_name();
        }
        break;

    case BK_HF_AT_CMD_COPS:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_QUERY_CURRENT_OP)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_RETRIEVE_SUB_INFO;
            bk_hfp_hf_retrieve_subscriber_info();
        }
        break;

    case BK_HF_AT_CMD_CNUM:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_RETRIEVE_SUB_INFO)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_SEND_VTS;
            bk_hfp_hf_send_dtmf("1");
        }
        break;

    case BK_HF_AT_CMD_VTS:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_SEND_VTS)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_REQ_LAST_TAG_NUM;
            bk_hfp_hf_request_last_voice_tag_number();
        }
        break;

    case BK_HF_AT_CMD_BINP:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_REQ_LAST_TAG_NUM)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_NREC;
            bk_hfp_hf_send_nrec();
        }
        break;

    case BK_HF_AT_CMD_NREC:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_NREC)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_VR_ENABLE;
            bk_hfp_hf_start_voice_recognition();
        }
        break;

    case BK_HF_AT_CMD_BVRA:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_VR_ENABLE)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_VR_DISBLE;
            bk_hfp_hf_stop_voice_recognition();
        }
        else if(s_hfp_status_mach == HFP_STATUS_WAIT_VR_DISBLE)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_DIAL;
            bk_hfp_hf_dial("112");
        }
        break;

    case BK_HF_AT_CMD_OUTCALL:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_DIAL)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_DIAL_HUP;
            bk_hfp_hf_reject_call();
        }
        else if(s_hfp_status_mach == HFP_STATUS_WAIT_DIAL_MEM)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_DIAL_MEM_HUP;
            bk_hfp_hf_reject_call();
        }
        else if(s_hfp_status_mach == HFP_STATUS_WAIT_REDAIL)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_REDAIL_HUP;
            bk_hfp_hf_reject_call();
        }
        break;

    case BK_HF_AT_CMD_CHUP:
        if(s_hfp_status_mach == HFP_STATUS_WAIT_DIAL_HUP)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_DIAL_MEM;
            bk_hfp_hf_dial_memory(1);
        }
        else if(s_hfp_status_mach == HFP_STATUS_WAIT_DIAL_MEM_HUP)
        {
            s_hfp_status_mach = HFP_STATUS_WAIT_REDAIL;
            bk_hfp_hf_redial();
        }
        else if(s_hfp_status_mach == HFP_STATUS_WAIT_REDAIL_HUP)
        {
            LOGI("%s end op\n", __func__);
            s_hfp_status_mach = HFP_STATUS_WAIT_DONE;
        }
        break;

    default:
        break;
    }
#endif
}

static void hfp_demo_event_cb(bk_hfp_hf_evt_t evt, void *arg, void *user_data)
{
    (void)user_data;

    switch (evt)
    {
        case BK_HFP_HF_EVT_CONNECTED:
            s_hfp_status_mach = HFP_STATUS_WAIT_QUERY_CALL;
            bk_hfp_hf_query_current_calls();
            break;

        case BK_HFP_HF_EVT_DISCONNECTED:
            s_hfp_status_mach = HFP_STATUS_IDLE;
            break;

        case BK_HFP_HF_EVT_AUDIO_CONNECTED:
        {
            bk_hfp_hf_audio_info_t *info = (bk_hfp_hf_audio_info_t *)arg;

            bt_audio_hfp_hf_codec = info->codec;
            os_memcpy(hfp_peer_addr, info->remote_bda, sizeof(hfp_peer_addr));
            hf_post_simple_msg(BT_AUDIO_VOICE_START_MSG);
            break;
        }

        case BK_HFP_HF_EVT_AUDIO_DISCONNECTED:
            hf_post_simple_msg(BT_AUDIO_VOICE_STOP_MSG);
            break;

        case BK_HFP_HF_EVT_VOICE_DATA:
        {
            bk_hfp_hf_voice_data_t *voice = (bk_hfp_hf_voice_data_t *)arg;

            hf_post_voice_data(voice->data, voice->len);
            break;
        }

        case BK_HFP_HF_EVT_VOLUME_CHANGED:
        {
            bk_hfp_hf_volume_info_t *vol = (bk_hfp_hf_volume_info_t *)arg;

            if (vol->type == BK_HF_VOLUME_CONTROL_TARGET_SPK)
            {
                hfp_hf_audio_set_gain(vol->volume);
            }
            break;
        }

        case BK_HFP_HF_EVT_AT_RESPONSE:
            hfp_demo_run_at_state_machine((const bk_hfp_hf_at_response_info_t *)arg);
            break;

        case BK_HFP_HF_EVT_RING:
            break;

        default:
            break;
    }
}

void hfp_demo_vr(uint8_t enable)
{
    if(enable)
    {
        bk_hfp_hf_start_voice_recognition();
    }
    else
    {
        bk_hfp_hf_stop_voice_recognition();
    }
}

void hfp_demo_dial(uint8_t enable, uint8_t *num)
{
    if(enable)
    {
        bk_hfp_hf_dial((const char *)num);
    }
    else
    {
        bk_hfp_hf_reject_call();
    }
}

void hfp_demo_answer(uint8_t accept)
{
    if(accept)
    {
        bk_hfp_hf_answer_call();
    }
    else
    {
        bk_hfp_hf_reject_call();
    }
}

void hfp_demo_cust_cmd(uint8_t *cmd)
{
    LOGI("%s len %d\n", __func__, strlen((char *)cmd));
    bk_hfp_hf_send_custom_cmd((const char *)cmd);
}

void bt_audio_hf_demo_main(void *arg)
{
    int running = 1;

    while (running)
    {
        bk_err_t err;
        bt_audio_hf_demo_msg_t msg;

        err = rtos_pop_from_queue(&bt_audio_hf_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);
        if (kNoErr == err)
        {
            switch (msg.type)
            {
                case BT_AUDIO_VOICE_START_MSG:
                {
                    hfp_hf_audio_start(bt_audio_hfp_hf_codec, hfp_peer_addr);
                }
                break;

                case BT_AUDIO_VOICE_STOP_MSG:
                {
                    hfp_hf_audio_stop();
                }
                break;

                case BT_AUDIO_VOICE_IND_MSG:
                {
                    hfp_hf_audio_handle_data((const uint8_t *)msg.data, msg.len);
                    os_free(msg.data);
                }
                break;

                case BT_AUDIO_EXIT_MSG:
                {
                    running = 0;
                }
                break;

                default:
                    break;
            }
        }
    }

    rtos_delete_thread(NULL);
}

int bt_audio_hf_demo_task_init(void)
{
    bk_err_t ret = BK_OK;
    if ((!bt_audio_hf_demo_thread_handle) && (!bt_audio_hf_demo_msg_que))
    {
        ret = rtos_init_queue(&bt_audio_hf_demo_msg_que,
                              "bt_audio_hf_demo_msg_que",
                              sizeof(bt_audio_hf_demo_msg_t),
                              BT_AUDIO_HF_DEMO_MSG_COUNT);
        if (ret != kNoErr)
        {
            LOGI("bt_audio hf demo msg queue failed \r\n");
            return BK_FAIL;
        }

        ret = rtos_create_thread(&bt_audio_hf_demo_thread_handle,
                                 BEKEN_DEFAULT_WORKER_PRIORITY,
                                 "bt_audio_hf_demo",
                                 (beken_thread_function_t)bt_audio_hf_demo_main,
                                 4096,
                                 (beken_thread_arg_t)0);
        if (ret != kNoErr)
        {
            LOGI("bt_audio hf demo task fail \r\n");
            rtos_deinit_queue(&bt_audio_hf_demo_msg_que);
            bt_audio_hf_demo_msg_que = NULL;
            bt_audio_hf_demo_thread_handle = NULL;
        }

        return kNoErr;
    }
    else
    {
        return kInProgressErr;
    }
}

static int bt_audio_hf_demo_task_deinit(void)
{
    if (bt_audio_hf_demo_thread_handle)
    {
        LOGI("%s wait demo task end\n", __func__);
        hf_post_simple_msg(BT_AUDIO_EXIT_MSG);
        rtos_thread_join(&bt_audio_hf_demo_thread_handle);
        bt_audio_hf_demo_thread_handle = NULL;
        LOGI("%s demo task end !!!\n", __func__);
    }

    if (bt_audio_hf_demo_msg_que)
    {
        bt_audio_hf_demo_msg_t msg = {0};

        while (rtos_pop_from_queue(&bt_audio_hf_demo_msg_que, &msg, 0) == kNoErr)
        {
            if (msg.type == BT_AUDIO_VOICE_IND_MSG && msg.data)
            {
                os_free(msg.data);
                msg.data = NULL;
            }
            os_memset(&msg, 0, sizeof(msg));
        }

        rtos_deinit_queue(&bt_audio_hf_demo_msg_que);
        bt_audio_hf_demo_msg_que = NULL;
    }

    return 0;
}

int hfp_hf_demo_init(uint8_t msbc_supported)
{
    LOGI("%s\r\n", __func__);

    bt_audio_hf_demo_task_init();

    bk_hfp_hf_register_event_cb(hfp_demo_event_cb, NULL);

    return bk_hfp_hf_service_init(msbc_supported);
}

int hfp_hf_demo_deinit(void)
{
    LOGI("%s\r\n", __func__);

    bk_hfp_hf_service_deinit();

    bt_audio_hf_demo_task_deinit();

    return 0;
}

#ifdef CONFIG_AUDIO
/* Strong override of the weak hook in the bk_bluetooth a2dp_sink_audio component:
 * before the A2DP player (re)opens it waits here for the HFP speaker/mic tasks to
 * exit, so the two audio paths never drive the DAC at the same time. */
int32_t wait_hfp_speaker_mic_task_end(void)
{
    return hfp_hf_audio_wait_player_end();
}
#endif
