#pragma once
#include <stdint.h>
#include <common/bk_err.h>
#include "components/bk_encode/bk_h264_encode_ctlr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    H264E_STREAM_SESSION_SERVICE_TCP = 0,
    H264E_STREAM_SESSION_SERVICE_UDP,
} h264e_stream_session_service_t;

typedef struct
{
    h264e_stream_session_service_t service;
    uint8_t auto_force_idr;
} h264e_stream_session_config_t;

typedef bk_err_t (*h264e_stream_session_media_start_cb_t)(void *user_data);
typedef bk_err_t (*h264e_stream_session_media_stop_cb_t)(void *user_data);

bk_err_t h264e_stream_session_init(const h264e_stream_session_config_t *config);
bk_err_t h264e_stream_session_init_local(const h264e_stream_session_config_t *config);
bk_err_t h264e_stream_session_deinit(void);
bk_err_t h264e_stream_session_deinit_local(void);
bk_err_t h264e_stream_session_start(void);
bk_err_t h264e_stream_session_stop(void);
bk_err_t h264e_stream_session_register_media_ops(h264e_stream_session_media_start_cb_t start,
                                            h264e_stream_session_media_stop_cb_t stop,
                                            void *user_data);

bk_err_t h264e_stream_session_bind_encoder(bk_h264_encode_ctlr_handle_t encoder);
bk_err_t h264e_stream_session_unbind_encoder(void);

int h264e_stream_session_send_h264(uint8_t *data, uint32_t length);

const char *h264e_stream_session_get_form_json(void);
const char *h264e_stream_session_get_rate_ctrl_schema_json(void);
bk_err_t h264e_stream_session_get_video_config(uint16_t *width,
                                               uint16_t *height,
                                               uint16_t *fps,
                                               uint32_t *gop_frame_count);
#ifdef __cplusplus
}
#endif