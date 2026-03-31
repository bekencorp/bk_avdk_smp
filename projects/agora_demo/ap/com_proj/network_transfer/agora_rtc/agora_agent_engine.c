#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "agora_agent_engine.h"
#include "AgoraWebClientUtils.h"
#include "cJSON.h"
#include "driver/trng.h"
#include "agora_config.h"
// #include "video_engine.h"
#if CONFIG_BK_SMART_CONFIG
#include "bk_smart_config.h"
#endif
#if CONFIG_BK_FACTORY_CONFIG
#include "bk_factory_config.h"
#endif

#define TAG "bk_sconf_agora"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

extern char *bk_get_bk_server_url(uint8_t index);

/* Common utility functions for agent operations */
static int agora_agent_send_request(const char *uri, const char *post_data, 
                                  char *response_buffer, int *response_code)
{
    struct webclient_session *session = NULL;
    agora_post_config_t post_config = {0};
    agora_req_result_t result = {-1, NULL};
    int ret = BK_FAIL;
    
    if (!uri || !post_data || !response_buffer || !response_code) {
        LOGE("Invalid parameters\r\n");
        return BK_FAIL;
    }

    session = webclient_session_create(AGORA_AGENT_SEND_HEADER_SIZE);
    if (!session) {
        LOGE("Failed to create webclient session\r\n");
        return BK_FAIL;
    }
    
    webclient_header_fields_add(session, "Content-Length: %d\r\n", os_strlen(post_data));
    webclient_header_fields_add(session, "Content-Type: application/json\r\n");
    // #if !CONFIG_STARTUP_AGENT_FROM_BK_SERVER
    // webclient_header_fields_add(session, "Authorization: af78e30%s\r\n", CONFIG_AGORA_APP_ID);
    // #endif

    post_config.uri = uri;
    post_config.session = session;
    post_config.post_data = post_data;

    result.response = response_buffer;

    LOGI("agora_agent_send_request uri:%s, post_data:%s\r\n", uri, post_data);

    ret = agora_http_post(&post_config, &result);
    LOGI("agora_http_post ret:%d\r\n", ret);

    LOGI("agora_agent_send_request success\r\n");
    if (response_code) {
        *response_code = result.code;
    }
    
    if (result.code == 200 && result.response) {
        ret = BK_OK;
    } else {
        LOGE("HTTP request failed, code: %d\r\n", result.code);
    }
    
    if (session) {
        LOGI("close session: %p\r\n", session);
        webclient_close(session);
    }


    LOGI("agora_agent_send_request end, ret:%d\r\n", ret);
    return ret;
}


int bk_sconf_get_agent_info(agora_rtc_agent_info_t *info)
{
    agora_rtc_agent_info_t info_tmp = {0};

    if (bk_config_read("d_agent_info", (void *)&info_tmp, sizeof(agora_rtc_agent_info_t)) <= 0)
    {
        return -1;
    }
    os_memcpy(info, &info_tmp, sizeof(agora_rtc_agent_info_t));
    return 0;
}

int bk_sconf_save_agent_info(char *appid, char *channel_name)
{
    agora_rtc_agent_info_t info_tmp = {0};

    info_tmp.valid = 1;
    os_strcpy(info_tmp.appid, appid);
    os_strcpy(info_tmp.channel_name, channel_name);
    bk_config_write("d_agent_info", (const void *)&info_tmp, sizeof(bk_sconf_agent_info_t));

    return 0;
}


#if CONFIG_STARTUP_AGENT_FROM_BK_SERVER
static int agora_parse_server_rsp(char *buffer,agora_rtc_agent_info_t *option_info)
{
    int code_update = 0;
    char *app_id_update = NULL;
    cJSON *json = cJSON_Parse(buffer);
    if (!json)
    {
        BK_LOGE(TAG, "Error before: [%s]\n", cJSON_GetErrorPtr());
        return BK_FAIL;
    }
    cJSON *code = cJSON_GetObjectItem(json, "code");
    if (code && ((code->type & 0xFF) == cJSON_Number)) {
        code_update = code->valueint;
        switch(code_update)
        {
            case HTTP_STATUS_SUCCESS:
                break;
            case HTTP_STATUS_TRIAL_LIMIT_EXCEEDED:
                BK_LOGI(TAG, "[UPDATE] the maximum number of agent expiriences has been reached, please contact armino_support@bekencorp.com for in-depth communication !\n");
            case HTTP_STATUS_PARAM_ERROR:
            case HTTP_STATUS_MAX_AGENT_UPTIME_EXCEEDED:
            case HTTP_STATUS_DEVICE_REMOVED:
            case HTTP_STATUS_AGENT_START_FAILED:
                cJSON_Delete(json);
            return BK_FAIL;
                break;
            default:
                cJSON_Delete(json);
                return BK_FAIL;
                break;
        }
    }
    else {
        BK_LOGE(TAG, "[Error] not find code msg\n");
        cJSON_Delete(json);
        return BK_FAIL;
    }

    cJSON* data = cJSON_GetObjectItem(json, "data");
    
	if (data)
	{
		cJSON *app_id = cJSON_GetObjectItem(data, "app_id");
		if (app_id && ((app_id->type & 0xFF) == cJSON_String)) {
			app_id_update = os_strdup(app_id->valuestring);
			BK_LOGI(TAG, "[UPDATE] appid:%s size:%d\n", app_id_update, strlen(app_id_update));
		}
		else {
			BK_LOGE(TAG, "[Error] not find app_id msg\n");
			cJSON_Delete(json);
			return BK_FAIL;
		}
	}
	else
	{
		BK_LOGE(TAG, "[Error] not find data msg\n");
		cJSON_Delete(json);
		return BK_FAIL;
	}


    LOGI("agora_parse_server_rsp app_id_update:%s\r\n", app_id_update);

	if (!option_info) {
		BK_LOGE(TAG, "option_info is NULL\n");
		if (app_id_update) {
			os_free(app_id_update);
		}
        
        LOGI("agora_parse_server_rsp option_info is NULL\r\n");
		return BK_FAIL;
	}


    os_strcpy(option_info->appid, app_id_update);
    LOGI("[UPDATE2] appid:%s size:%d\n", option_info->appid, strlen(option_info->appid));
    
    cJSON_Delete(json);
        
    return BK_OK;
}
int agora_start_agent_from_bk_server(agora_rtc_agent_info_t *option_info, void *device_id)
{
    char *post_data = NULL;
    char *response_buffer = NULL;
    char uri[256] = {0};
    //char *device_id = (char *)device_id;
    int url_len = 0, ret = BK_FAIL;
    uint32_t rand_flag = 0;
    cJSON *root = NULL, *agent_param = NULL;

    LOGI("agora_start_agent_from_bk_server device_id:%s\r\n", device_id);

    if (os_strlen(device_id) == 0)
    {
        LOGE("device_id is empty\r\n");
        return BK_FAIL;
    }

    url_len = os_snprintf(uri, AGORA_AGENT_MAX_URL_LEN, "%s/activate_agent/", bk_get_bk_server_url(0));
    if ((url_len < 0) || (url_len >= AGORA_AGENT_MAX_URL_LEN))
    {
        LOGE("URL len overflow\r\n");
        return BK_FAIL;
    }

    /* Generate JSON data */
    root = cJSON_CreateObject();
    agent_param = cJSON_CreateObject(); /* Will be automatically deleted when root is deleted */
    
    if (!root || !agent_param)
    {
        LOGE("no memory for cJSON objects\n");
        ret = BK_FAIL;
        goto __exit;
    }

    // Add channel
    cJSON_AddStringToObject(root, "channel", device_id);
    
    cJSON_AddNumberToObject(agent_param, "audio_duration", 20);

    #if CONFIG_AE_AUDIO_ENCODER_OPUS
    // Add audio codec
    cJSON_AddStringToObject(agent_param, "out_acodec", "OPUS");
    #elif CONFIG_AE_AUDIO_ENCODER_G722
    cJSON_AddStringToObject(agent_param, "out_acodec", "G722");
    #else
    cJSON_AddStringToObject(agent_param, "out_acodec", "PCM");
    #endif

    // Add agent_param to root
    cJSON_AddItemToObject(root, "agent_param", agent_param);

    // Add rand_flag
    rand_flag = bk_rand();
    char rand_flag_str[32] = {0};
    sprintf(rand_flag_str, "%u", rand_flag);
    cJSON_AddStringToObject(root, "rand_flag", rand_flag_str);
    
    // Generate JSON string
    post_data = cJSON_Print(root);
    if (!post_data)
    {
        LOGE("no memory for post_data buffer\n");
        ret = BK_FAIL;
        goto __exit;
    }
    
    LOGI("%s, post_data:%s\r\n", __func__, post_data);
    response_buffer = (char *) web_malloc(AGORA_AGENT_RCV_BUF_SIZE);
    if (!response_buffer) {
        LOGE("Failed to malloc response buffer\r\n");
        ret = BK_FAIL;
        goto __exit;
    }
    os_memset(response_buffer, 0, AGORA_AGENT_RCV_BUF_SIZE);
    int response_code = 0;
    /* Send HTTP request using common utility */
    ret = agora_agent_send_request(uri, post_data, response_buffer, &response_code);
    LOGI("agora_agent_send_request ret:%d\r\n", ret);


    if (ret != BK_OK || !response_buffer)
    {
        LOGE("Failed to send agent start request, ret:%d, response_code:%d\r\n", ret, response_code);
        ret = BK_FAIL;
        goto __exit;
    }

    LOGI("Response: %s\r\n", response_buffer);

    /* Parse response */
    ret = agora_parse_server_rsp(response_buffer,option_info);

    LOGI("agora_parse_server_rsp ret:%d\r\n", ret);

__exit:

    if (response_buffer)
    {
        LOGI("free response_buffer: %s\r\n", response_buffer);
        web_free(response_buffer);
    }

    if (root)
    {
        LOGI("free root: %p\r\n", root);
        cJSON_Delete(root);
    }
    /* agent_param is a child of root, no need to delete separately */

    LOGD("volc_agent_start_from_bk_server end, ret:%d\n", ret);
    return ret;
}

int agora_upate_agent_from_bk_server(agora_rtc_agent_info_t *option_info, void *device_id, void *update_info)
{
    char *post_data = NULL;
    char *response_buffer = NULL;
    char uri[256] = {0};
    int url_len = 0, ret = BK_FAIL;
    cJSON *root = NULL;

    if (!option_info)
    {
        LOGE("option_info is null\r\n");
        return BK_FAIL;
    }

    if (os_strlen(device_id) == 0)
    {
        LOGE("device_id is empty\r\n");
        return BK_FAIL;
    }

    LOGI("volc_upate_agent_from_bk_server device_id:%s\r\n", device_id);

    url_len = os_snprintf(uri, AGORA_AGENT_MAX_URL_LEN, "%s/switch_model_type/", bk_get_bk_server_url(0));
    if ((url_len < 0) || (url_len >= AGORA_AGENT_MAX_URL_LEN))
    {
        BK_LOGE(TAG, "URL len overflow\r\n");
        return BK_FAIL;
    }

    /* Generate JSON data */
    root = cJSON_CreateObject();
    
    if (!root)
    {
        LOGE("no memory for cJSON objects\n");
        ret = BK_FAIL;
        goto __exit;
    }

    // Add channel
    cJSON_AddStringToObject(root, "channel", device_id);


    if (update_info && os_strcmp((char *)update_info, "vision") == 0)
    {
        cJSON_AddStringToObject(root, "model_type", "text_and_image");
        
    }
    else
    {
        cJSON_AddStringToObject(root, "model_type", "text");
    }

      
    // Generate JSON string
    post_data = cJSON_PrintUnformatted(root);
    if (!post_data)
    {
        LOGE("no memory for post_data buffer\n");
        ret = BK_FAIL;
        goto __exit;
    }
    
    LOGI("%s, %s\r\n", __func__, post_data);

    response_buffer =(char *) web_malloc(AGORA_AGENT_RCV_BUF_SIZE);
    if (!response_buffer) {
        LOGE("Failed to malloc response buffer\r\n");
        ret = BK_FAIL;
        goto __exit;
    }
    os_memset(response_buffer, 0, AGORA_AGENT_RCV_BUF_SIZE);
    int response_code = 0;
    /* Send HTTP request using common utility */
    ret = agora_agent_send_request(uri, post_data, response_buffer, &response_code);
    if (ret != BK_OK || !response_buffer)
    {
        LOGE("Failed to send agent update request, ret:%d, response_code:%d\r\n", ret, response_code);
        ret = BK_FAIL;
        goto __exit;
    }

    LOGI("Response: %s\r\n", response_buffer);

    /* Parse response */
    ret = agora_parse_server_rsp(response_buffer,option_info);

__exit:
    if (response_buffer)
    {
        web_free(response_buffer);
    }
    
    if (root)
    {
        cJSON_Delete(root);
    }

    return ret;
}
int agora_stop_agent_from_bk_server(agora_rtc_agent_info_t *option_info, void *device_id)
{
    return BK_OK;
}

#endif

int agora_agent_update(agora_rtc_agent_info_t *option_info, void *device_id, void *update_info)
{
    #if CONFIG_STARTUP_AGENT_FROM_BK_SERVER
    return agora_upate_agent_from_bk_server(option_info, device_id, update_info);
    #else
    return BK_FAIL;
    #endif
}
int agora_agent_start(agora_rtc_agent_info_t *option_info, void *device_id)
{
    #if CONFIG_STARTUP_AGENT_FROM_BK_SERVER
    return agora_start_agent_from_bk_server(option_info, device_id);
    #else
    return BK_FAIL;
    #endif
}
int agora_agent_stop(agora_rtc_agent_info_t *option_info, void *device_id)
{
    #if CONFIG_STARTUP_AGENT_FROM_BK_SERVER
    return agora_stop_agent_from_bk_server(option_info, device_id);
    #else
    return BK_FAIL;
    #endif
}