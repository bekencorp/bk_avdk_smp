#pragma once

#include <stdint.h>
#include "isp_frame_session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    ISP_FRAME_STATUS_OK = 0,
    ISP_FRAME_STATUS_JSON_ERROR = 1,
    ISP_FRAME_STATUS_UNKNOWN_CMD = 2,
    ISP_FRAME_STATUS_ENCODER_UNBOUND = 3,
    ISP_FRAME_STATUS_INVALID_PARAM = 4,
    ISP_FRAME_STATUS_SDK_ERROR = 5,
    ISP_FRAME_STATUS_INVALID_STATE = 6,
} isp_frame_status_t;

typedef struct
{
    isp_frame_session_config_t config;
    isp_frame_session_media_start_cb_t media_start;
    isp_frame_session_media_stop_cb_t media_stop;
    void *media_user_data;
    uint8_t initialized;
    uint8_t started;
    uint8_t network_started;
    uint8_t media_ready;
    uint8_t stream_started;
} isp_frame_session_ctx_t;

isp_frame_session_ctx_t *isp_frame_session_get_ctx(void);
int isp_frame_protocol_handle(uint8_t *data, uint32_t length);
int isp_frame_protocol_send_status(const char *cmd, uint32_t seq, isp_frame_status_t status, const char *message);

void isp_frame_stream_register_read_cb(isp_frame_read_cb_t cb);
void isp_frame_stream_set_profile(uint16_t width, uint16_t height);
int isp_frame_session_send_image(uint8_t *data, uint32_t length);
bk_err_t isp_frame_send_one_frame(void);
bk_err_t isp_frame_send_frames(uint32_t frame_count);
bk_err_t isp_frame_send_frames_async(uint32_t frame_count);
void isp_frame_stream_cancel(void);
void isp_frame_capture_wait_idle(uint32_t timeout_ms);
uint8_t isp_frame_stream_is_busy(void);
/* On-demand frame path: start/stop clear stream state (no push thread). */
bk_err_t isp_frame_stream_start(uint16_t width, uint16_t height, uint16_t fps);
bk_err_t isp_frame_stream_stop(void);

#ifdef __cplusplus
}
#endif
