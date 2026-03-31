/*************************************************************
 *
 * This is a part of the Agora Media Framework Library.
 * Copyright (C) 2025 Agora IO
 * All rights reserved.
 *
 *************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/shell_task.h>
#include <components/event.h>
#include <components/netif_types.h>
#include "bk_rtos_debug.h"
#include "agora_config.h"
#include "bk_agora_api.h"
#include "agora_agent_engine.h"
#include <modules/wifi.h>
#include "modules/wifi_types.h"
#include "components/bk_uid.h"
#include <driver/h264.h>
#include <driver/aon_rtc.h>
#include <modules/vcenc/vcenc_common.h>
#if CONFIG_APP_EVT
#include "app_event.h"
#endif
#include "cli.h"

#define TAG "agora_main"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

// agora_rtc_room_info_t *agora_room_info = NULL;

bool g_connected_flag = false;
bool g_agent_offline = true;
// static char agora_appid[33] = {0};
static char channel_name[128] = {0};
static char token[256] = CONFIG_AGORA_TOKEN;
static uint32_t uid = CONFIG_AGORA_UID;

static beken_thread_t  agora_thread_hdl = NULL;
static beken_semaphore_t agora_sem = NULL;
bool agora_runing = false;
static agora_rtc_config_t agora_rtc_config = DEFAULT_AGORA_RTC_CONFIG();
static agora_rtc_option_t agora_rtc_option = DEFAULT_AGORA_RTC_OPTION();

agora_rtc_agent_info_t *agora_rtc_agent_info;

static bool bk_agora_is_h264_key_frame(uint32_t h264_type)
{
    if (h264_type == VCENC_OUT_IFRAME) {
        return true;
    }

    if (h264_type & (1U << H264_NAL_I_FRAME)) {
        return true;
    }

    return false;
}

static uint8_t bk_agora_audio_codec_type_mapping(audio_enc_type_t codec_type)
{
    uint8_t aud_data_type = AUDIO_DATA_TYPE_GENERIC;
    
    switch(codec_type)
    {
        case AUDIO_ENC_TYPE_G711A:
            aud_data_type = AUDIO_DATA_TYPE_PCMA;
            break;
        case AUDIO_ENC_TYPE_G711U:
            aud_data_type = AUDIO_DATA_TYPE_PCMU;
            break;
        case AUDIO_ENC_TYPE_PCM:
            aud_data_type = AUDIO_DATA_TYPE_PCM;
            break;
        case AUDIO_ENC_TYPE_G722:
            aud_data_type = AUDIO_DATA_TYPE_G722;
            break;
        case AUDIO_ENC_TYPE_OPUS:
            aud_data_type = AUDIO_DATA_TYPE_OPUS;
            break;
        default:
            LOGE("Unknown codec type:%d\n", codec_type);
            break;
    }
    
    return aud_data_type;
}

/* ============================= Message Handler ============================= */

static void bk_agora_user_notify_msg_handle(agora_rtc_msg_t *p_msg)
{
    switch (p_msg->code)
    {
        case AGORA_RTC_MSG_JOIN_CHANNEL_SUCCESS:
            g_connected_flag = true;
            LOGI("Join channel success\n");
            break;
            
        case AGORA_RTC_MSG_REJOIN_CHANNEL_SUCCESS:
            g_connected_flag = true;
            LOGI("Rejoin channel success\n");
            #if CONFIG_APP_EVT
            app_event_send_msg(APP_EVT_RTC_REJOIN_SUCCESS, 0);
            #endif
            break;
            
        case AGORA_RTC_MSG_USER_JOINED:
            LOGI("User joined\n");
            g_agent_offline = false;
            #if CONFIG_APP_EVT
            app_event_send_msg(APP_EVT_AGENT_JOINED, 0);
            #endif
            break;
            
        case AGORA_RTC_MSG_USER_OFFLINE:
            LOGI("User offline\n");
            g_agent_offline = true;
            #if CONFIG_APP_EVT
            app_event_send_msg(APP_EVT_AGENT_OFFLINE, 0);
            #endif
            break;
            
        case AGORA_RTC_MSG_CONNECTION_LOST:
            LOGE("Connection lost. Please check WiFi status\n");
            g_connected_flag = false;
            #if CONFIG_APP_EVT
            app_event_send_msg(APP_EVT_RTC_CONNECTION_LOST, 0);
            #endif
            break;
            
        case AGORA_RTC_MSG_INVALID_APP_ID:
            LOGE("Invalid App ID. Please double check\n");
            break;
            
        case AGORA_RTC_MSG_INVALID_CHANNEL_NAME:
            LOGE("Invalid channel name. Please double check\n");
            break;
            
        case AGORA_RTC_MSG_INVALID_TOKEN:
        case AGORA_RTC_MSG_TOKEN_EXPIRED:
            LOGE("Invalid or expired token. Please double check\n");
            break;
            
        case AGORA_RTC_MSG_BWE_TARGET_BITRATE_UPDATE:
            // Handle bitrate update if needed
            // g_target_bps = p_msg->data.bwe.target_bitrate;
            break;
            
        case AGORA_RTC_MSG_KEY_FRAME_REQUEST:
            // Handle key frame request if needed
            break;
            
        default:
            break;
    }
}

/* ============================= Data Send Functions ============================= */

int bk_agora_rtc_audio_data_send(uint8_t *data_ptr, size_t data_len, audio_enc_type_t audio_type)
{
    int rval = 0;
    audio_frame_info_t info = { 0 };
    agora_rtc_t *rtc = NULL;
    
    if (false == g_connected_flag)
    {
        // LOGI("Failed to send audio data, g_connected_flag:%d\n", g_connected_flag);  // Disabled: too verbose
        return BK_FAIL;
    }
    
    rtc = __get_rtc_instance();
    if (!rtc || (rtc->state != AGORA_RTC_STATE_WORKING))
    {
        return BK_FAIL;
    }
    
    info.data_type = bk_agora_audio_codec_type_mapping(audio_type);
    
    rval = agora_rtc_send_audio_data(rtc->conn_id, data_ptr, data_len, &info);
    if (rval < 0)
    {
        // LOGW("Failed to send audio data, rval:%d, err:%s\n", rval, agora_rtc_err_2_str(rval));  // Disabled: too verbose
        return BK_FAIL;
    }
    
    return data_len;
}

int bk_agora_rtc_video_data_send(frame_buffer_t *frame)
{
    int rval = 0;
    video_frame_info_t info = { 0 };
    static uint64_t before = 0, curr = 0;
    agora_rtc_t *rtc = NULL;
    
    curr = bk_aon_rtc_get_ms();
    
    if (false == g_connected_flag)
    {
        // LOGI("Failed to send video data, g_connected_flag:%d\n", g_connected_flag);  // Disabled: too verbose
        return BK_FAIL;
    }
    
    /* Validate frame parameter */
    if (frame == NULL || frame->frame == NULL || frame->length == 0)
    {
        return BK_FAIL;
    }
    
    /* Setup video frame info based on format */
    info.stream_type = VIDEO_STREAM_HIGH;
    if ((frame->fmt == PIXEL_FMT_H264) || (frame->fmt == IMAGE_H264))
    {
        /* Check if it's an I-frame (only send I-frames) */
        if (!bk_agora_is_h264_key_frame(frame->h264_type))
        {
            // LOGD("%s: skip non-I frame\n", __func__);
            return BK_OK;
        }
        
        info.data_type = VIDEO_DATA_TYPE_H264;
        info.frame_type = VIDEO_FRAME_AUTO_DETECT;
         //info.frame_rate = 1000/VIDEO_FRAME_INTERVAL;
    }
    else if ((frame->fmt == PIXEL_FMT_JPEG) || (frame->fmt == IMAGE_MJPEG))
    {
        info.data_type = VIDEO_DATA_TYPE_GENERIC_JPEG;
        info.frame_type = VIDEO_FRAME_KEY;
    }
    else if ((frame->fmt == PIXEL_FMT_H265) || (frame->fmt == IMAGE_H265))
    {
        info.data_type = VIDEO_DATA_TYPE_H265;
        info.frame_type = VIDEO_FRAME_AUTO_DETECT;
    }
    else
    {
        LOGE("%s: Unsupported format: %d\n", __func__, frame->fmt);
        return BK_FAIL;
    }
    
    /* Frame rate control - send every VIDEO_FRAME_INTERVAL ms */
    if (curr <= before || curr - before < VIDEO_FRAME_INTERVAL)
    {
        return BK_OK;
    }
    
    rtc = __get_rtc_instance();
    if (!rtc || (rtc->state != AGORA_RTC_STATE_WORKING))
    {
        // LOGI("Failed to send video data, rtc->state:%d\n", rtc->state);  // Disabled: too verbose
        return BK_FAIL;
    }
    
    /* Send video data to agora_rtc */
    rval = agora_rtc_send_video_data(rtc->conn_id, (uint8_t *)frame->frame, (size_t)frame->length, &info);
    //LOGI("send video data to agora_rtc, rval:%d, rtc->conn_id:%d, frame->length:%d, frame->fmt:%d\n", rval,rtc->conn_id,frame->length,frame->fmt,info.data_type,info.frame_type);
    if (rval < 0)
    {
         LOGW("%s: send video data failed, rval=%d data_type=%d len=%d frame_type=%d fmt=%d\n",
             __func__, rval, info.data_type, (int)frame->length, info.frame_type,frame->fmt);  // Disabled: too verbose
         return BK_FAIL;
    }
    else
    {
         LOGD("%s: send video data successfully, len=%d\n", __func__, (int)frame->length);
    }
    
    /* Update timestamp */
    before = curr;
    
    return (int)frame->length;
}

/* ============================= Audio RX Handler ============================= */

static int bk_agora_user_audio_rx_data_handle(unsigned char *data, unsigned int size, const audio_frame_info_t *info_ptr)
{
    #if CONFIG_BK_NETWORK_TRANSFER
    return ntwk_trans_recv_audio(data, size);
    #else
    LOGE("BK Network transfer not enabled\n");
    return BK_FAIL;
    #endif
}

/* ============================= Main RTC Thread ============================= */

void bk_agora_rtc_main(void)
{
    bk_err_t ret = BK_OK;

    // Allocate and copy app_id
    agora_rtc_config.p_appid = (char *)psram_malloc(strlen(agora_rtc_agent_info->appid) + 1);
    os_strcpy((char *)agora_rtc_config.p_appid, agora_rtc_agent_info->appid);

    LOGI("agora_main use record appid:%s %s\r\n", agora_rtc_config.p_appid,agora_rtc_agent_info->appid);
    
    agora_rtc_config.log_disable = true;
    agora_rtc_config.bwe_param_max_bps = BANDWIDTH_ESTIMATE_MAX_BITRATE;
    
    // Create Agora RTC instance
    ret = __agora_rtc_create(&agora_rtc_config, (agora_rtc_msg_notify_cb)bk_agora_user_notify_msg_handle);
    if (ret != BK_OK)
    {
        LOGE("__agora_rtc_create failed\n");
        goto exit;
    }
    
    agora_rtc_option.p_channel_name = (char *)psram_malloc(os_strlen(channel_name) + 1);

    os_strcpy((char *)agora_rtc_option.p_channel_name, channel_name);
    agora_rtc_option.audio_config.audio_data_type = CONFIG_AUDIO_CODEC_TYPE;

    if (strlen(token))
    {
        LOGI("agora_main use record token:%s \r\n", token);
        agora_rtc_option.p_token = token;
    }
    else
    {
        LOGI("agora_main use default token:%s \r\n", CONFIG_AGORA_TOKEN);
        agora_rtc_option.p_token = CONFIG_AGORA_TOKEN;
    }

    if (uid != CONFIG_AGORA_UID)
    {
        LOGI("agora_main use record uid:%d \r\n", uid);
        agora_rtc_option.uid = uid;
    }
    else
    {
        LOGI("agora_main use default uid:%d \r\n", CONFIG_AGORA_UID);
        agora_rtc_option.uid = CONFIG_AGORA_UID;
    }

    // return;
    // Start Agora RTC
    ret = __agora_rtc_start(&agora_rtc_option);
    if (ret != BK_OK)
    {
        LOGE("__agora_rtc_start failed, ret:%d\n", ret);
        goto exit;
    }
    
   

    agora_runing = true;
    rtos_set_semaphore(&agora_sem);
    
    /* Wait until we join channel successfully */
    while (!g_connected_flag)
    {
        if (!agora_runing)
        {
            goto exit;
        }
        rtos_delay_milliseconds(100);
    }
    
    LOGI("-----Agora RTC join channel success-----\n");
    /* Main loop */

    ret = __agora_rtc_register_audio_rx_handle((agora_rtc_audio_rx_data_handle)bk_agora_user_audio_rx_data_handle);
    if (ret != BK_OK)
    {
       LOGE("Failed to register audio RX handle, ret:%d\n", ret);
    }    

     while (agora_runing)
     {
         rtos_delay_milliseconds(100);
     }


exit:
    __agora_rtc_register_audio_rx_handle(NULL);

    /* Stop Agora RTC */
    __agora_rtc_stop();
    
    /* Destroy Agora RTC */
    __agora_rtc_destroy();
    
    if (agora_rtc_config.p_appid)
    {
        psram_free((char *)agora_rtc_config.p_appid);
        agora_rtc_config.p_appid = NULL;
    }
    
    if (agora_rtc_option.p_channel_name)
    {
        psram_free((char *)agora_rtc_option.p_channel_name);
        agora_rtc_option.p_channel_name = NULL;
    }
    
    g_connected_flag = false;
    agora_thread_hdl = NULL;
    agora_runing = false;
    
    rtos_set_semaphore(&agora_sem);
    rtos_delete_thread(NULL);
}

/* ============================= Public API ============================= */

bk_err_t bk_agora_rtc_stop(void *device_id)
{
    if (!agora_runing)
    {
        LOGI("Agora not started\n");
        return BK_OK;
    }
    
    agora_runing = false;
    rtos_get_semaphore(&agora_sem, BEKEN_NEVER_TIMEOUT);
    
    rtos_deinit_semaphore(&agora_sem);
    agora_sem = NULL;
    
    return BK_OK;
}

bk_err_t bk_agora_rtc_start(void *device_id)
{
    bk_err_t ret = BK_OK;
    
    if (agora_runing)
    {
        LOGI("Agora already started, please close and then reopen\n");
        return BK_FAIL;
    }
    
    LOGI("bk_agora_rtc_start device_id:%s\r\n", device_id);

    ret = rtos_init_semaphore(&agora_sem, 1);
    if (ret != BK_OK)
    {
        LOGE("Failed to create semaphore\n");
        return BK_FAIL;
    }

    LOGI("rtos_create_thread success\r\n");

    ret = rtos_create_thread(&agora_thread_hdl,
                             4,
                             "agora",
                             (beken_thread_function_t)bk_agora_rtc_main,
                             8 * 1024,
                             NULL);
    if (ret != kNoErr)
    {
        LOGI("Failed to create agora app task, ret:%d\n", ret);
        agora_thread_hdl = NULL;
        goto fail;
    }
    

    rtos_get_semaphore(&agora_sem, BEKEN_NEVER_TIMEOUT);
    
    LOGI("Create agora app task complete\n");
    return BK_OK;
    
fail:
    if (agora_sem)
    {
        rtos_deinit_semaphore(&agora_sem);
        agora_sem = NULL;
    }
    
    return BK_FAIL;
}

int bk_agora_agent_start(agora_rtc_agent_info_t *option_info, void *device_id)
{
    return agora_agent_start(option_info, device_id);
}
int bk_agora_agent_stop(agora_rtc_agent_info_t *option_info, void *device_id)
{
    return agora_agent_stop(option_info, device_id);
}

int bk_agora_start(void *device_id)
{
    int ret = 0;
    LOGI("%s %d device_id:%s\r\n", __func__, __LINE__, device_id);

    if (agora_runing)
    {
        LOGI("bk_agora_start already started\n");
        return BK_FAIL;
    }

    if (!agora_rtc_agent_info)
    {
        agora_rtc_agent_info = psram_malloc(sizeof(agora_rtc_agent_info_t));
        if (!agora_rtc_agent_info) {
            LOGE("agora_rtc_agent_info malloc fail");
            return BK_FAIL;
        }
        os_memset(agora_rtc_agent_info, 0, sizeof(agora_rtc_agent_info_t));
    }

    ret = bk_agora_agent_start(agora_rtc_agent_info, device_id);
    LOGI("bk_agora_agent_start ret:%d\r\n", ret);
    if (ret < 0)
    {
        LOGE("bk_agora_agent_start fail, ret:%d \r\n", ret);
        app_event_send_msg(APP_EVT_AGENT_START_FAIL, 0);
        return ret;
    }
    os_strcpy(channel_name, device_id);


    ret = bk_agora_rtc_start(device_id);
    if (ret < 0)
    {
        LOGE("bk_agora_rtc_start fail, ret:%d \r\n", ret);
        return ret;
    }

    return ret;
}

int bk_agora_stop(void *device_id)
{
    int ret = 0;

    ret = bk_agora_rtc_stop(device_id);
    if (ret < 0)
    {
        LOGE("bk_agora_rtc_stop fail, ret:%d \r\n", ret);
    }

    ret = bk_agora_agent_stop(agora_rtc_agent_info, device_id);
    if (ret < 0)
    {
        LOGE("bk_agora_agent_stop fail, ret:%d \r\n", ret);
        return ret;
    }

    if (agora_rtc_agent_info)
    {
        psram_free(agora_rtc_agent_info);
        agora_rtc_agent_info = NULL;
    }

    return ret;
}

int bk_agora_update_agent(void *device_id, void *update_info)
{
    int ret = 0;
    LOGI("%s %d device_id:%s\r\n", __func__, __LINE__, device_id);

    ret =  bk_agora_rtc_stop(device_id);
    if (ret < 0)
    {
        LOGE("bk_agora_rtc_stop fail, ret:%d \r\n", ret);
    }

    ret = agora_agent_update(agora_rtc_agent_info, device_id, update_info);
    if (ret < 0)
    {
        LOGE("agora_agent_update fail, ret:%d \r\n", ret);
        return ret;
    }

    ret = bk_agora_rtc_start(device_id);
    if (ret < 0)
    {
        LOGE("bk_byte_rtc_start fail, ret:%d \r\n", ret);
        return ret;
    }
    return BK_OK;
}


#define AGORA_RTC_CMD_CNT   (sizeof(s_agora_rtc_commands) / sizeof(struct cli_command))
static void bk_agora_rtc_cli_help(void)
{
    LOGI("agora_test {start|stop}\n");
}

static void bk_agora_rtc_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    unsigned char uid[32] = {0};
    char uid_str[65] = {0};

    bk_uid_get_data(uid);
    for (int i = 0; i < 24; i++)
    {
        sprintf(uid_str + i * 2, "%02x", uid[i]);
    }

    if (argc < 2)
    {
        goto cmd_fail;
    }

    /* audio test */
    if (os_strcmp(argv[1], "start_agora") == 0)
    {
        LOGI("start rtc\r\n");
        bk_agora_rtc_start(uid_str);
    }
    else if (os_strcmp(argv[1], "stop_agora") == 0)
    {
        LOGI("stop rtc\r\n");
        bk_agora_rtc_stop(uid_str);
    }
    else if (os_strcmp(argv[1], "start_agent") == 0)
    {
        LOGI("start agent\r\n");
        bk_agora_agent_start(agora_rtc_agent_info, uid_str);
    }
    else if (os_strcmp(argv[1], "stop_agent") == 0)
    {
        LOGI("stop agent\r\n");
        bk_agora_agent_stop(agora_rtc_agent_info, uid_str);
    }
    else if (os_strcmp(argv[1], "start") == 0)
    {
        LOGI("start rtc and agent\r\n");
        // extern int audio_engine_init(void);
        // audio_engine_init();
        bk_agora_start(uid_str);
    }
    else if (os_strcmp(argv[1], "stop") == 0)
    {
        LOGI("stop rtc and agent\r\n");
        bk_agora_stop(uid_str);
    }
    else
    {
        goto cmd_fail;
    }

    return;

cmd_fail:
    bk_agora_rtc_cli_help();
}
static const struct cli_command s_agora_rtc_commands[] =
{
    {"agora_rtc", "agora_rtc ...", bk_agora_rtc_test_cmd},
};

int bk_agora_rtc_cli_init(void)
{
    return cli_register_commands(s_agora_rtc_commands, AGORA_RTC_CMD_CNT);
}
bk_err_t bk_agora_pre_config(void *device_id)
{
    bk_agora_rtc_cli_init();
    return BK_OK;
}

