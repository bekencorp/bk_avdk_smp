#pragma once
#include <stdint.h>
#include <common/bk_err.h>
#include "isp_frame_capture_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ISP_FRAME_SESSION_SERVICE_TCP = 0,
    ISP_FRAME_SESSION_SERVICE_UDP,
} isp_frame_session_service_t;

typedef struct
{
    isp_frame_session_service_t service;
    uint8_t auto_force_idr;
} isp_frame_session_config_t;

typedef bk_err_t (*isp_frame_session_media_start_cb_t)(void *user_data);
typedef bk_err_t (*isp_frame_session_media_stop_cb_t)(void *user_data);
typedef int (*isp_frame_read_cb_t)(uint8_t *frame, uint32_t size, uint32_t timeout_ms);

bk_err_t isp_frame_session_init(const isp_frame_session_config_t *config);
bk_err_t isp_frame_session_init_local(const isp_frame_session_config_t *config);
bk_err_t isp_frame_session_deinit(void);
bk_err_t isp_frame_session_deinit_local(void);
bk_err_t isp_frame_session_start(void);
bk_err_t isp_frame_session_stop(void);
bk_err_t isp_frame_session_register_media_ops(isp_frame_session_media_start_cb_t start,
                                          isp_frame_session_media_stop_cb_t stop,
                                          void *user_data);

int isp_frame_session_send_image(uint8_t *data, uint32_t length);

const char *isp_frame_session_get_form_json(void);
const char *isp_frame_session_get_rate_ctrl_schema_json(void);
const isp_frame_capture_config_t *isp_frame_session_get_capture_config(void);

#ifdef __cplusplus
}
#endif
