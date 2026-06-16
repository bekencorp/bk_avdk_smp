#pragma once

#include <stdint.h>
#include "components/bk_encode/bk_h264_encode_ctlr.h"
#include "h264e_stream_session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    H264E_STREAM_STATUS_OK = 0,
    H264E_STREAM_STATUS_JSON_ERROR = 1,
    H264E_STREAM_STATUS_UNKNOWN_CMD = 2,
    H264E_STREAM_STATUS_ENCODER_UNBOUND = 3,
    H264E_STREAM_STATUS_INVALID_PARAM = 4,
    H264E_STREAM_STATUS_SDK_ERROR = 5,
    H264E_STREAM_STATUS_INVALID_STATE = 6,
} h264e_stream_status_t;

typedef struct
{
    h264e_stream_session_config_t config;
    bk_h264_encode_ctlr_handle_t encoder;
    h264e_stream_session_media_start_cb_t media_start;
    h264e_stream_session_media_stop_cb_t media_stop;
    void *media_user_data;
    uint8_t initialized;
    uint8_t started;
    uint8_t network_started;
} h264e_stream_session_ctx_t;

h264e_stream_session_ctx_t *h264e_stream_session_get_ctx(void);
int h264e_stream_protocol_handle(uint8_t *data, uint32_t length);
int h264e_stream_protocol_send_status(const char *cmd, uint32_t seq, h264e_stream_status_t status, const char *message);

#ifdef __cplusplus
}
#endif
