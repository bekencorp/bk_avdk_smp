#ifndef __VOLC_AGENT_ENGINE_H__
#define __VOLC_AGENT_ENGINE_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "AgoraWebClientUtils.h"
#include "agora_rtc_engine.h"

typedef enum
{
	HTTP_STATUS_SUCCESS = 200,
	HTTP_STATUS_PARAM_ERROR = 400,
	HTTP_STATUS_MAX_AGENT_UPTIME_EXCEEDED = 403,
	HTTP_STATUS_TRIAL_LIMIT_EXCEEDED = 404,
	HTTP_STATUS_DEVICE_REMOVED = 405,
	HTTP_STATUS_AGENT_START_FAILED = 406,
}agent_status_code;

#define AGORA_AGENT_RCV_BUF_SIZE                2048
#define AGORA_AGENT_SEND_HEADER_SIZE            1024
#define AGORA_AGENT_POST_DATA_MAX_SIZE          1024*2
#define AGORA_AGENT_MAX_URL_LEN                 256

int bk_sconf_get_agent_info(agora_rtc_agent_info_t *info);
int bk_sconf_save_agent_info(char *appid, char *channel_name);

/* API */
int agora_agent_start(agora_rtc_agent_info_t *option_info, void *device_id);
int agora_agent_stop(agora_rtc_agent_info_t *option_info, void *device_id);
int agora_agent_update(agora_rtc_agent_info_t *option_info, void *device_id, void *update_info);
#if CONFIG_STARTUP_AGENT_FROM_BK_SERVER
//uint16_t volc_generate_start_agent_req_payload(char *payload, uint16_t max_len);
#endif

#ifdef __cplusplus
}
#endif
#endif /* __VOLC_AGENT_ENGINE_H__ */
