#ifndef __RTC_WEBCLIENT_UTILS_H__
#define __RTC_WEBCLIENT_UTILS_H__
#include "components/webclient.h"

typedef struct webclient_session rtc_session;

typedef struct {
    int code;
    char* response;
} agora_req_result_t;

typedef struct {
    const char* uri;
    struct webclient_session* session;  // key1,value1,key2,value2....keyn,valuen,NULL
    const char* post_data;
} agora_post_config_t;

int agora_http_post(agora_post_config_t* config, agora_req_result_t *req_result);

#endif // __RTC_WEBCLIENT_UTILS_H__