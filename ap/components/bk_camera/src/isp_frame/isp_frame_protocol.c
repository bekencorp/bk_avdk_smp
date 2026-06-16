#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/avdk_pixel_types.h>
#include <components/log.h>
#include "cJSON.h"
#include "network_transfer.h"
#include "isp_frame_capture_config.h"
#include "isp_frame_priv.h"
#define TAG "isp_frame_proto"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define ISP_FRAME_JSON_MAX_LEN      (2048)
#define ISP_FRAME_RESPONSE_MAX_LEN  (16384)

#define ISP_FRAME_JSONRPC_PARSE_ERROR      (-32700)
#define ISP_FRAME_JSONRPC_INVALID_REQUEST  (-32600)
#define ISP_FRAME_JSONRPC_METHOD_NOT_FOUND (-32601)
#define ISP_FRAME_JSONRPC_INVALID_PARAMS   (-32602)
#define ISP_FRAME_JSONRPC_INTERNAL_ERROR   (-32603)
#define ISP_FRAME_JSONRPC_ENCODER_UNBOUND  (-32001)
#define ISP_FRAME_JSONRPC_SDK_FAILED       (-32002)
#define ISP_FRAME_JSONRPC_INVALID_STATE    (-32003)

static isp_frame_capture_config_t s_capture_config = {
    .rx_timeout = 1000000,
    .sensor_w = 1920,
    .sensor_h = 1080,
    .isp_w = 1920,
    .isp_h = 1080,
    .format = ISP_FRAME_FORMAT_NV12,
    .pattern = "RGGB",
};

const isp_frame_capture_config_t *isp_frame_session_get_capture_config(void)
{
    return &s_capture_config;
}

void isp_frame_capture_apply_resolution(uint16_t sensor_w, uint16_t sensor_h,
                                       uint16_t isp_w, uint16_t isp_h)
{
    if (sensor_w > 0)
    {
        s_capture_config.sensor_w = sensor_w;
    }
    if (sensor_h > 0)
    {
        s_capture_config.sensor_h = sensor_h;
    }
    if (isp_w > 0)
    {
        s_capture_config.isp_w = isp_w;
    }
    if (isp_h > 0)
    {
        s_capture_config.isp_h = isp_h;
    }
}

static char s_isp_frame_json_buffer[ISP_FRAME_JSON_MAX_LEN + 1];
static char s_isp_frame_response_buffer[ISP_FRAME_RESPONSE_MAX_LEN];
static char s_isp_frame_result_buffer[ISP_FRAME_RESPONSE_MAX_LEN];

static const char *isp_frame_status_text(isp_frame_status_t status)
{
    switch (status)
    {
        case ISP_FRAME_STATUS_OK:
            return "ok";
        case ISP_FRAME_STATUS_JSON_ERROR:
            return "json_error";
        case ISP_FRAME_STATUS_UNKNOWN_CMD:
            return "unknown_cmd";
        case ISP_FRAME_STATUS_ENCODER_UNBOUND:
            return "encoder_unbound";
        case ISP_FRAME_STATUS_INVALID_PARAM:
            return "invalid_param";
        case ISP_FRAME_STATUS_SDK_ERROR:
            return "sdk_error";
        case ISP_FRAME_STATUS_INVALID_STATE:
            return "invalid_state";
        default:
            return "unknown_error";
    }
}

static int isp_frame_ctrl_send_json(const char *json)
{
    int ret;
    size_t length;

    if (json == NULL)
    {
        return -1;
    }
    length = strlen(json);
    ret = ntwk_trans_ctrl_send((uint8_t *)json, length);
    LOGI("JSON-RPC <- send len=%u ret=%d\n", (uint32_t)length, ret);
    return ret;
}

static const char *isp_frame_jsonrpc_message(int code)
{
    switch (code)
    {
        case ISP_FRAME_JSONRPC_PARSE_ERROR:
            return "Parse error";
        case ISP_FRAME_JSONRPC_INVALID_REQUEST:
            return "Invalid Request";
        case ISP_FRAME_JSONRPC_METHOD_NOT_FOUND:
            return "Method not found";
        case ISP_FRAME_JSONRPC_INVALID_PARAMS:
            return "Invalid params";
        case ISP_FRAME_JSONRPC_INTERNAL_ERROR:
            return "Internal error";
        case ISP_FRAME_JSONRPC_ENCODER_UNBOUND:
            return "Encoder not bound";
        case ISP_FRAME_JSONRPC_SDK_FAILED:
            return "SDK call failed";
        case ISP_FRAME_JSONRPC_INVALID_STATE:
            return "Invalid state";
        default:
            return "Server error";
    }
}

static int isp_frame_jsonrpc_send_error(const char *id_json,
    int code,
    const char *field,
    const char *reason);

static int isp_frame_jsonrpc_send_result(const char *id_json, const char *result_json)
{
    int len;
    const char *safe_id = (id_json != NULL) ? id_json : "null";
    const char *safe_result = (result_json != NULL) ? result_json : "{\"message\":\"ok\"}";

    len = snprintf(s_isp_frame_response_buffer, sizeof(s_isp_frame_response_buffer),
                   "{\"jsonrpc\":\"2.0\",\"result\":%s,\"id\":%s}\n",
                   safe_result, safe_id);
    if (len < 0 || len >= (int)sizeof(s_isp_frame_response_buffer))
    {
        LOGE("JSON-RPC result response truncated, len=%d buffer=%u\n",
             len, (uint32_t)sizeof(s_isp_frame_response_buffer));
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INTERNAL_ERROR, NULL, "response_too_large");
    }
    return isp_frame_ctrl_send_json(s_isp_frame_response_buffer);
}

static int isp_frame_jsonrpc_send_error(const char *id_json,
    int code,
    const char *field,
    const char *reason)
{
    int len;
    char data[160] = {0};
    const char *safe_id = (id_json != NULL) ? id_json : "null";
    const char *safe_reason = (reason != NULL) ? reason : isp_frame_jsonrpc_message(code);

    if (field != NULL)
    {
        snprintf(data, sizeof(data),
                 ",\"data\":{\"field\":\"%s\",\"reason\":\"%s\"}",
                 field, safe_reason);
    }

    len = snprintf(s_isp_frame_response_buffer, sizeof(s_isp_frame_response_buffer),
                   "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":%d,\"message\":\"%s\"%s},\"id\":%s}\n",
                   code, isp_frame_jsonrpc_message(code), data, safe_id);
    if (len < 0 || len >= (int)sizeof(s_isp_frame_response_buffer))
    {
        LOGE("JSON-RPC error response truncated, len=%d buffer=%u\n",
             len, (uint32_t)sizeof(s_isp_frame_response_buffer));
        return -1;
    }
    return isp_frame_ctrl_send_json(s_isp_frame_response_buffer);
}

int isp_frame_protocol_send_status(const char *cmd, uint32_t seq, isp_frame_status_t status, const char *message)
{
    char id_json[16];
    (void)cmd;
    snprintf(id_json, sizeof(id_json), "%u", seq);

    if (status == ISP_FRAME_STATUS_OK)
    {
        return isp_frame_jsonrpc_send_result(id_json, "{\"message\":\"ok\"}");
    }
    return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INTERNAL_ERROR, NULL, message);
}

static char *isp_frame_jsonrpc_id_to_string(cJSON *id)
{
    if (id == NULL)
    {
        return NULL;
    }
    return cJSON_PrintUnformatted(id);
}

static cJSON *isp_frame_json_get_alias(cJSON *object, const char *key1, const char *key2)
{
    cJSON *item;
    if (object == NULL)
    {
        return NULL;
    }
    item = cJSON_GetObjectItem(object, key1);
    if (item == NULL && key2 != NULL)
    {
        item = cJSON_GetObjectItem(object, key2);
    }
    return item;
}

static int isp_frame_json_get_u32_alias(cJSON *object,
    const char *key1,
    const char *key2,
    uint32_t *value)
{
    cJSON *item = isp_frame_json_get_alias(object, key1, key2);
    if (item == NULL)
    {
        return 0;
    }
    if (!cJSON_IsNumber(item) || item->valuedouble < 0)
    {
        return -1;
    }
    *value = (uint32_t)item->valuedouble;
    return 1;
}

static int isp_frame_build_frame_result_json(const isp_frame_capture_config_t *cfg,
    uint32_t frames_sent,
    char *buffer,
    size_t buffer_len)
{
    return snprintf(buffer, buffer_len,
        "{\"message\":\"ok\",\"sent\":%u,\"framesSent\":%u,\"width\":%u,\"height\":%u,"
        "\"format\":%u,\"bayerPattern\":\"%s\"}",
        frames_sent, frames_sent, cfg->isp_w, cfg->isp_h, cfg->format, cfg->pattern);
}

static int isp_frame_json_get_bool(cJSON *object, const char *key, uint8_t *value)
{
    cJSON *item = cJSON_GetObjectItem(object, key);
    if (item == NULL)
    {
        return 0;
    }
    if (!cJSON_IsBool(item))
    {
        return -1;
    }
    *value = cJSON_IsTrue(item) ? 1 : 0;
    return 1;
}


static int isp_frame_capture_to_json(const isp_frame_capture_config_t *cfg, char *buffer, size_t buffer_len)
{
    return snprintf(buffer, buffer_len,
        "{\"rxTimeout\":%u,\"sensorWidth\":%u,\"sensorHeight\":%u,\"ispW\":%u,\"ispH\":%u,"
        "\"format\":%u,\"pattern\":\"%s\"}",
        cfg->rx_timeout, cfg->sensor_w, cfg->sensor_h, cfg->isp_w, cfg->isp_h,
        cfg->format, cfg->pattern);
}

static int isp_frame_send_capture_response(const char *id_json, const char *message,
    const isp_frame_capture_config_t *cfg)
{
    char capture_json[256];
    char result[320];
    const char *safe_message = (message != NULL) ? message : "ok";

    isp_frame_capture_to_json(cfg, capture_json, sizeof(capture_json));
    snprintf(result, sizeof(result),
             "{\"message\":\"%s\",\"capture\":%s}",
             safe_message, capture_json);
    return isp_frame_jsonrpc_send_result(id_json, result);
}

static int isp_frame_format_from_string(const char *name, uint16_t *format)
{
    if (name == NULL || format == NULL)
    {
        return -1;
    }
    if (strcmp(name, "NV12") == 0)
    {
        *format = ISP_FRAME_FORMAT_NV12;
        return 0;
    }
    if (strcmp(name, "RAW10") == 0)
    {
        *format = ISP_FRAME_FORMAT_RAW10;
        return 0;
    }
    return -1;
}

static int isp_frame_parse_format_item(cJSON *item, uint16_t *format)
{
    if (item == NULL || format == NULL)
    {
        return -1;
    }

    if (cJSON_IsNumber(item))
    {
        uint32_t value = (uint32_t)item->valuedouble;

        if (!isp_frame_format_is_valid((uint16_t)value))
        {
            return -1;
        }
        *format = (uint16_t)value;
        return 0;
    }

    if (cJSON_IsString(item) && item->valuestring != NULL)
    {
        return isp_frame_format_from_string(item->valuestring, format);
    }

    return -1;
}

static int isp_frame_validate_pattern(const char *pattern)
{
    if (pattern == NULL)
    {
        return -1;
    }
    if (strcmp(pattern, "RGGB") == 0 || strcmp(pattern, "GRBG") == 0 ||
        strcmp(pattern, "GBRG") == 0 || strcmp(pattern, "BGGR") == 0)
    {
        return 0;
    }
    return -1;
}

static int isp_frame_parse_capture_config(cJSON *object,
    isp_frame_capture_config_t *config,
    const char **field)
{
    uint32_t value;
    cJSON *item;
    int ret;

    if (object == NULL || config == NULL)
    {
        return 0;
    }

    ret = isp_frame_json_get_u32_alias(object, "rxTimeout", "rx_timeout", &value);
    if (ret == 0)
    {
        ret = isp_frame_json_get_u32_alias(object, "timeoutMs", "timeout_ms", &value);
    }
    if (ret < 0 || (ret > 0 && (value < ISP_FRAME_RX_TIMEOUT_MS_MIN || value > ISP_FRAME_RX_TIMEOUT_MS_MAX)))
    {
        *field = "rxTimeout";
        return -1;
    }
    if (ret > 0)
    {
        config->rx_timeout = value;
    }

    ret = isp_frame_json_get_u32_alias(object, "sensorWidth", "sensor_w", &value);
    if (ret < 0 || (ret > 0 && (value < 1 || value > 8192)))
    {
        *field = "sensorWidth";
        return -1;
    }
    if (ret > 0)
    {
        config->sensor_w = (uint16_t)value;
    }

    ret = isp_frame_json_get_u32_alias(object, "sensorHeight", "sensor_h", &value);
    if (ret < 0 || (ret > 0 && (value < 1 || value > 8192)))
    {
        *field = "sensorHeight";
        return -1;
    }
    if (ret > 0)
    {
        config->sensor_h = (uint16_t)value;
    }

    ret = isp_frame_json_get_u32_alias(object, "ispW", "isp_w", &value);
    if (ret == 0)
    {
        ret = isp_frame_json_get_u32_alias(object, "ispWidth", NULL, &value);
    }
    if (ret < 0 || (ret > 0 && (value < 1 || value > 8192)))
    {
        *field = "ispW";
        return -1;
    }
    if (ret > 0)
    {
        config->isp_w = (uint16_t)value;
    }

    ret = isp_frame_json_get_u32_alias(object, "ispH", "isp_h", &value);
    if (ret == 0)
    {
        ret = isp_frame_json_get_u32_alias(object, "ispHeight", NULL, &value);
    }
    if (ret < 0 || (ret > 0 && (value < 1 || value > 8192)))
    {
        *field = "ispH";
        return -1;
    }
    if (ret > 0)
    {
        config->isp_h = (uint16_t)value;
    }

    item = isp_frame_json_get_alias(object, "format", NULL);
    if (item != NULL)
    {
        if (isp_frame_parse_format_item(item, &config->format) != 0)
        {
            *field = "format";
            return -1;
        }
    }

    item = isp_frame_json_get_alias(object, "pattern", "bayerPattern");
    if (item != NULL)
    {
        if (!cJSON_IsString(item) || item->valuestring == NULL ||
            isp_frame_validate_pattern(item->valuestring) != 0)
        {
            *field = "pattern";
            return -1;
        }
        snprintf(config->pattern, sizeof(config->pattern), "%s", item->valuestring);
    }

    return 0;
}

static cJSON *isp_frame_params_get_capture_object(cJSON *params)
{
    cJSON *capture;

    if (params == NULL || !cJSON_IsObject(params))
    {
        return NULL;
    }

    capture = cJSON_GetObjectItem(params, "capture");
    if (capture != NULL && cJSON_IsObject(capture))
    {
        return capture;
    }

    capture = cJSON_GetObjectItem(params, "isp");
    if (capture != NULL && cJSON_IsObject(capture))
    {
        return capture;
    }

    if (cJSON_GetObjectItem(params, "rxTimeout") != NULL ||
        cJSON_GetObjectItem(params, "rx_timeout") != NULL ||
        cJSON_GetObjectItem(params, "ispW") != NULL ||
        cJSON_GetObjectItem(params, "isp_w") != NULL ||
        cJSON_GetObjectItem(params, "format") != NULL)
    {
        return params;
    }

    return NULL;
}

static int isp_frame_apply_capture_from_params(cJSON *params, const char **field)
{
    cJSON *capture;
    isp_frame_capture_config_t next;

    capture = isp_frame_params_get_capture_object(params);
    if (capture == NULL)
    {
        return 0;
    }

    next = s_capture_config;
    if (isp_frame_parse_capture_config(capture, &next, field) != 0)
    {
        return -1;
    }

    s_capture_config = next;
    LOGI("capture config applied: %ux%u format=%u rxTimeout=%u\n",
         s_capture_config.isp_w, s_capture_config.isp_h,
         s_capture_config.format, s_capture_config.rx_timeout);
    return 0;
}

static int isp_frame_capture_needs_camera_reopen(const isp_frame_capture_config_t *before,
                                              const isp_frame_capture_config_t *after)
{
    if (before == NULL || after == NULL)
    {
        return 0;
    }

    return (before->sensor_w != after->sensor_w) ||
           (before->sensor_h != after->sensor_h) ||
           (before->isp_w != after->isp_w) ||
           (before->isp_h != after->isp_h) ||
           (before->format != after->format);
}

static int isp_frame_handle_get_rate_ctrl(const char *id_json)
{
    return isp_frame_send_capture_response(id_json, "ok", &s_capture_config);
}

static int isp_frame_handle_set_rate_ctrl(cJSON *params, const char *id_json)
{
    isp_frame_capture_config_t next = s_capture_config;
    const char *field = NULL;
    cJSON *capture_json = cJSON_GetObjectItem(params, "capture");

    if (capture_json == NULL)
    {
        capture_json = params;
    }
    if (isp_frame_parse_capture_config(capture_json, &next, &field) != 0)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_PARAMS, field, "invalid capture");
    }

    s_capture_config = next;
    return isp_frame_send_capture_response(id_json, "ok", &s_capture_config);
}

static int isp_frame_handle_force_idr(const char *id_json)
{
    (void)isp_frame_session_get_ctx();
    return isp_frame_jsonrpc_send_result(id_json, "{\"message\":\"ok\"}");
}

static bk_err_t isp_frame_ensure_camera_ready(void)
{
    isp_frame_session_ctx_t *ctx = isp_frame_session_get_ctx();

    if (ctx->media_ready)
    {
        return BK_OK;
    }

    if (ctx->media_start == NULL)
    {
        return BK_ERR_STATE;
    }

    if (ctx->media_start(ctx->media_user_data) != BK_OK)
    {
        return BK_FAIL;
    }

    ctx->media_ready = 1;
    (void)isp_frame_stream_start(s_capture_config.isp_w, s_capture_config.isp_h, 0);
    return BK_OK;
}

static bk_err_t isp_frame_reopen_camera_with_config(void)
{
    isp_frame_session_ctx_t *ctx = isp_frame_session_get_ctx();

    (void)isp_frame_stream_stop();
    if (ctx->media_ready && ctx->media_stop != NULL)
    {
        if (ctx->media_stop(ctx->media_user_data) != BK_OK)
        {
            ctx->media_ready = 0;
            return BK_FAIL;
        }
    }
    ctx->media_ready = 0;
    return isp_frame_ensure_camera_ready();
}

static int isp_frame_handle_isp_start(cJSON *params, const char *id_json)
{
    isp_frame_capture_config_t prev = s_capture_config;
    const char *field = NULL;
    isp_frame_session_ctx_t *ctx = isp_frame_session_get_ctx();

    if (isp_frame_apply_capture_from_params(params, &field) != 0)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_PARAMS, field, "invalid capture");
    }

    if (isp_frame_stream_is_busy())
    {
        isp_frame_stream_cancel();
        isp_frame_capture_wait_idle(500);
    }

    if (ctx->media_ready && isp_frame_capture_needs_camera_reopen(&prev, &s_capture_config))
    {
        if (isp_frame_reopen_camera_with_config() != BK_OK)
        {
            return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_SDK_FAILED, NULL, "isp_reconfig_failed");
        }
    }
    else if (!ctx->media_ready)
    {
        if (isp_frame_ensure_camera_ready() != BK_OK)
        {
            return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_SDK_FAILED, NULL, "isp_start_failed");
        }
    }
    else
    {
        (void)isp_frame_stream_start(s_capture_config.isp_w, s_capture_config.isp_h, 0);
    }

    return isp_frame_jsonrpc_send_result(id_json, "{\"message\":\"ok\",\"state\":\"running\"}");
}

static int isp_frame_handle_isp_stop(const char *id_json)
{
    isp_frame_session_ctx_t *ctx = isp_frame_session_get_ctx();
    int ret;

    isp_frame_stream_cancel();
    /* Reply before waiting on capture thread so PC RPC does not time out. */
    ret = isp_frame_jsonrpc_send_result(id_json, "{\"message\":\"ok\",\"state\":\"stopped\"}");

    /* Turn off camera first so preview read/send exits quickly, then wait capture task. */
    if (ctx->media_ready && ctx->media_stop != NULL)
    {
        (void)ctx->media_stop(ctx->media_user_data);
    }
    ctx->media_ready = 0;
    (void)isp_frame_stream_stop();
    LOGI("isp stop done, capture_busy=%u\n", isp_frame_stream_is_busy());
    return ret;
}

static int isp_frame_handle_isp_preview(cJSON *params, const char *id_json)
{
    int len;
    const char *field = NULL;
    isp_frame_capture_config_t prev = s_capture_config;

    if (!isp_frame_session_get_ctx()->media_ready)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_STATE, NULL, "isp_not_started");
    }

    if (isp_frame_apply_capture_from_params(params, &field) != 0)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_PARAMS, field, "invalid capture");
    }
    if (isp_frame_capture_needs_camera_reopen(&prev, &s_capture_config) &&
        isp_frame_reopen_camera_with_config() != BK_OK)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_SDK_FAILED, NULL, "isp_reconfig_failed");
    }

    if (isp_frame_send_frames_async(1) != BK_OK)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_SDK_FAILED, NULL, "preview_start_failed");
    }

    len = isp_frame_build_frame_result_json(&s_capture_config, 1,
        s_isp_frame_result_buffer,
        sizeof(s_isp_frame_result_buffer));
    if (len < 0 || len >= (int)sizeof(s_isp_frame_result_buffer))
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INTERNAL_ERROR, NULL, "response_too_large");
    }
    if (len > 0 && (size_t)len < sizeof(s_isp_frame_result_buffer))
    {
        size_t off = (size_t)len - 1;
        snprintf(&s_isp_frame_result_buffer[off], sizeof(s_isp_frame_result_buffer) - off,
                 ",\"accepted\":true,\"async\":true}");
    }
    return isp_frame_jsonrpc_send_result(id_json, s_isp_frame_result_buffer);
}

static int isp_frame_handle_capture_frames(cJSON *params, const char *id_json)
{
    cJSON *item;
    uint32_t frame_count = 1;
    const char *field = NULL;
    isp_frame_capture_config_t prev = s_capture_config;

    if (!isp_frame_session_get_ctx()->media_ready)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_STATE, NULL, "isp_not_started");
    }

    if (isp_frame_apply_capture_from_params(params, &field) != 0)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_PARAMS, field, "invalid capture");
    }
    if (isp_frame_capture_needs_camera_reopen(&prev, &s_capture_config) &&
        isp_frame_reopen_camera_with_config() != BK_OK)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_SDK_FAILED, NULL, "isp_reconfig_failed");
    }

    if (params != NULL && cJSON_IsObject(params))
    {
        item = cJSON_GetObjectItem(params, "frameCount");
        if (item == NULL)
        {
            item = cJSON_GetObjectItem(params, "frame_count");
        }
        if (cJSON_IsNumber(item) && item->valuedouble >= 1)
        {
            frame_count = (uint32_t)item->valuedouble;
        }
        else if (item != NULL)
        {
            return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_PARAMS,
                "frameCount", "positive number expected");
        }
    }

    if (isp_frame_send_frames_async(frame_count) != BK_OK)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_SDK_FAILED, NULL, "capture_start_failed");
    }

    {
        int len = isp_frame_build_frame_result_json(&s_capture_config, frame_count,
            s_isp_frame_result_buffer, sizeof(s_isp_frame_result_buffer));

        if (len < 0 || len >= (int)sizeof(s_isp_frame_result_buffer))
        {
            return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INTERNAL_ERROR, NULL, "response_too_large");
        }
        if (len > 0 && (size_t)len < sizeof(s_isp_frame_result_buffer))
        {
            size_t off = (size_t)len - 1;
            snprintf(&s_isp_frame_result_buffer[off], sizeof(s_isp_frame_result_buffer) - off,
                     ",\"accepted\":true,\"async\":true}");
        }
        return isp_frame_jsonrpc_send_result(id_json, s_isp_frame_result_buffer);
    }
}

static int isp_frame_handle_start_encode(cJSON *params, const char *id_json)
{
    return isp_frame_handle_isp_start(params, id_json);
}

static int isp_frame_handle_stop_encode(const char *id_json)
{
    return isp_frame_handle_isp_stop(id_json);
}

const char *isp_frame_session_get_form_json(void)
{
    return "{\"id\":\"isp-wifi-config\",\"title\":\"ISP Capture Config\","
        "\"desc\":\"ISP startup parameters aligned with isp_dump_tool. Format (NV12/RAW10) is runtime tunable.\","
        "\"groups\":["
        "{\"id\":\"capture\",\"title\":\"Capture\",\"fields\":["
        "{\"type\":\"number\",\"id\":\"rxTimeout\",\"label\":\"RX timeout (ms)\",\"default\":1000000,"
        "\"min\":1,\"max\":1000000,\"description\":\"ISP frame read timeout in milliseconds\"},"
        "{\"type\":\"number\",\"id\":\"sensorWidth\",\"label\":\"sensorWidth\",\"default\":1920,\"min\":1,\"max\":8192,\"unit\":\"pixel\"},"
        "{\"type\":\"number\",\"id\":\"sensorHeight\",\"label\":\"sensorHeight\",\"default\":1080,\"min\":1,\"max\":8192,\"unit\":\"pixel\"},"
        "{\"type\":\"number\",\"id\":\"ispW\",\"label\":\"ISP W\",\"default\":1920,\"min\":1,\"max\":8192,\"unit\":\"pixel\"},"
        "{\"type\":\"number\",\"id\":\"ispH\",\"label\":\"ISP H\",\"default\":1080,\"min\":1,\"max\":8192,\"unit\":\"pixel\"},"
        "{\"type\":\"select\",\"id\":\"format\",\"label\":\"Format\",\"default\":23,"
        "\"options\":[{\"label\":\"NV12\",\"value\":23},{\"label\":\"RAW10\",\"value\":21}]},"
        "{\"type\":\"select\",\"id\":\"pattern\",\"label\":\"Pattern\",\"default\":\"RGGB\","
        "\"options\":[{\"label\":\"RGGB\",\"value\":\"RGGB\"},{\"label\":\"GRBG\",\"value\":\"GRBG\"},"
        "{\"label\":\"GBRG\",\"value\":\"GBRG\"},{\"label\":\"BGGR\",\"value\":\"BGGR\"}]}]}"
        "],\"submit\":{\"label\":\"Apply\",\"method\":\"doorbell.isp.setParameterValues\",\"build\":\"tree\"}}";
}

const char *isp_frame_session_get_rate_ctrl_schema_json(void)
{
    return "{\"supported\":["
        "{\"id\":\"rxTimeout\",\"type\":\"u32\",\"min\":1,\"max\":1000000,\"description\":\"Frame read timeout (ms)\"},"
        "{\"id\":\"sensorWidth\",\"type\":\"u16\",\"min\":1,\"max\":8192,\"description\":\"Sensor input width\"},"
        "{\"id\":\"sensorHeight\",\"type\":\"u16\",\"min\":1,\"max\":8192,\"description\":\"Sensor input height\"},"
        "{\"id\":\"ispW\",\"type\":\"u16\",\"min\":1,\"max\":8192,\"description\":\"ISP output width\"},"
        "{\"id\":\"ispH\",\"type\":\"u16\",\"min\":1,\"max\":8192,\"description\":\"ISP output height\"},"
        "{\"id\":\"format\",\"type\":\"u16\",\"enum\":[21,23],\"description\":\"Pixel format code (21=RAW10, 23=NV12)\"},"
        "{\"id\":\"pattern\",\"type\":\"string\",\"enum\":[\"RGGB\",\"GRBG\",\"GBRG\",\"BGGR\"],\"description\":\"Bayer pattern for RAW10\"}"
        "],\"note\":\"Apply via set_config before turnOn/start_encode. Only format supports runtime change after ISP is up.\"}";
}

static int isp_frame_handle_get_config_schema(const char *id_json)
{
    int len = snprintf(s_isp_frame_result_buffer, sizeof(s_isp_frame_result_buffer),
                       "{\"form\":%s}", isp_frame_session_get_form_json());
    if (len < 0 || len >= (int)sizeof(s_isp_frame_result_buffer))
    {
        LOGE("config schema result truncated, len=%d buffer=%u\n",
             len, (uint32_t)sizeof(s_isp_frame_result_buffer));
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INTERNAL_ERROR, NULL, "schema_too_large");
    }

    return isp_frame_jsonrpc_send_result(id_json, s_isp_frame_result_buffer);
}

static int isp_frame_handle_get_minimal_config_schema(const char *id_json)
{
    return isp_frame_jsonrpc_send_result(id_json,
        "{\"id\":\"isp-wifi-config\",\"title\":\"ISP Capture Config\","
        "\"groups\":[],\"submit\":{\"label\":\"Apply\",\"method\":\"doorbell.isp.setParameterValues\"}}");
}

static int isp_frame_config_to_result(char *buffer, size_t buffer_len)
{
    char capture_json[256];

    isp_frame_capture_to_json(&s_capture_config, capture_json, sizeof(capture_json));
    return snprintf(buffer, buffer_len,
        "{\"values\":%s,\"isp\":%s,\"config\":{\"isp\":%s,\"capture\":%s}}",
        capture_json, capture_json, capture_json, capture_json);
}

static int isp_frame_handle_get_config(const char *id_json)
{
    isp_frame_config_to_result(s_isp_frame_result_buffer, sizeof(s_isp_frame_result_buffer));
    return isp_frame_jsonrpc_send_result(id_json, s_isp_frame_result_buffer);
}

static int isp_frame_handle_set_config(cJSON *params, const char *id_json)
{
    isp_frame_capture_config_t prev = s_capture_config;
    const char *field = NULL;
    isp_frame_session_ctx_t *ctx = isp_frame_session_get_ctx();

    if (params == NULL || !cJSON_IsObject(params))
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_PARAMS, "params", "object expected");
    }

    if (isp_frame_apply_capture_from_params(params, &field) != 0)
    {
        return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_PARAMS, field, "invalid capture config");
    }

    if (ctx->media_ready && isp_frame_capture_needs_camera_reopen(&prev, &s_capture_config))
    {
        if (isp_frame_reopen_camera_with_config() != BK_OK)
        {
            return isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_SDK_FAILED, NULL, "isp_reconfig_failed");
        }
    }

    return isp_frame_handle_get_config(id_json);
}

int isp_frame_protocol_handle(uint8_t *data, uint32_t length)
{
    cJSON *root;
    cJSON *jsonrpc;
    cJSON *method;
    cJSON *params;
    char *id_json = NULL;
    int ret;

    if (data == NULL || length == 0 || length > ISP_FRAME_JSON_MAX_LEN)
    {
        return isp_frame_jsonrpc_send_error(NULL, ISP_FRAME_JSONRPC_PARSE_ERROR, NULL, "invalid length");
    }
    memcpy(s_isp_frame_json_buffer, data, length);
    s_isp_frame_json_buffer[length] = '\0';

    root = cJSON_ParseWithLength(s_isp_frame_json_buffer, length);
    if (root == NULL)
    {
        return isp_frame_jsonrpc_send_error(NULL, ISP_FRAME_JSONRPC_PARSE_ERROR, NULL, "parse failed");
    }

    id_json = isp_frame_jsonrpc_id_to_string(cJSON_GetObjectItem(root, "id"));
    jsonrpc = cJSON_GetObjectItem(root, "jsonrpc");
    method = cJSON_GetObjectItem(root, "method");
    params = cJSON_GetObjectItem(root, "params");
    if (cJSON_IsString(method) && method->valuestring != NULL)
    {
        LOGI("JSON-RPC -> method=%s len=%u\n", method->valuestring, length);
    }

    if (!cJSON_IsObject(root) ||
        !cJSON_IsString(jsonrpc) || strcmp(jsonrpc->valuestring, "2.0") != 0 ||
        !cJSON_IsString(method) || method->valuestring == NULL ||
        id_json == NULL)
    {
        ret = isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_INVALID_REQUEST, NULL, NULL);
        goto exit;
    }

    if (strcmp(method->valuestring, "get_config_schema") == 0 ||
        strcmp(method->valuestring, "ispFrame.getParameterSchema") == 0 ||
        strcmp(method->valuestring, "doorbell.isp.getParameterSchema") == 0 ||
        strcmp(method->valuestring, "isp.getParameterSchema") == 0 ||
        strcmp(method->valuestring, "isp_capture.getParameterSchema") == 0)
    {
        ret = isp_frame_handle_get_config_schema(id_json);
    }
    else if (strcmp(method->valuestring, "get_config") == 0 ||
             strcmp(method->valuestring, "ispFrame.getConfig") == 0 ||
             strcmp(method->valuestring, "doorbell.isp.getConfig") == 0 ||
             strcmp(method->valuestring, "isp.getConfig") == 0 ||
             strcmp(method->valuestring, "isp_capture.getConfig") == 0)
    {
        ret = isp_frame_handle_get_config(id_json);
    }
    else if (strcmp(method->valuestring, "set_config") == 0 ||
             strcmp(method->valuestring, "ispFrame.setConfig") == 0 ||
             strcmp(method->valuestring, "doorbell.isp.setParameterValues") == 0 ||
             strcmp(method->valuestring, "isp.setParameterValues") == 0 ||
             strcmp(method->valuestring, "isp_capture.setParameterValues") == 0)
    {
        ret = isp_frame_handle_set_config(params, id_json);
    }
    else if (strcmp(method->valuestring, "get_rate_ctrl") == 0 ||
             strcmp(method->valuestring, "doorbell.encoder.getVcencRateControl") == 0)
    {
        ret = isp_frame_handle_get_rate_ctrl(id_json);
    }
    else if (strcmp(method->valuestring, "set_rate_ctrl") == 0 ||
             strcmp(method->valuestring, "doorbell.isp.setVcencRateControl") == 0 ||
             strcmp(method->valuestring, "isp.setVcencRateControl") == 0)
    {
        ret = isp_frame_handle_set_rate_ctrl(params, id_json);
    }
    else if (strcmp(method->valuestring, "force_idr") == 0)
    {
        ret = isp_frame_handle_force_idr(id_json);
    }
    else if (strcmp(method->valuestring, "doorbell.isp.start") == 0 ||
             strcmp(method->valuestring, "ispFrame.start") == 0 ||
             strcmp(method->valuestring, "isp.start") == 0 ||
             strcmp(method->valuestring, "isp_capture.start") == 0)
    {
        ret = isp_frame_handle_isp_start(params, id_json);
    }
    else if (strcmp(method->valuestring, "doorbell.isp.stop") == 0 ||
             strcmp(method->valuestring, "ispFrame.stop") == 0 ||
             strcmp(method->valuestring, "isp.framestop") == 0 ||
             strcmp(method->valuestring, "isp.stop") == 0 ||
             strcmp(method->valuestring, "isp_capture.stop") == 0)
    {
        ret = isp_frame_handle_isp_stop(id_json);
    }
    else if (strcmp(method->valuestring, "doorbell.isp.preview") == 0 ||
             strcmp(method->valuestring, "ispFrame.preview") == 0 ||
             strcmp(method->valuestring, "ispFrame.capturePreview") == 0 ||
             strcmp(method->valuestring, "ispFrame.captureFrame") == 0 ||
             strcmp(method->valuestring, "isp.preview") == 0 ||
             strcmp(method->valuestring, "isp_capture.preview") == 0)
    {
        ret = isp_frame_handle_isp_preview(params, id_json);
    }
    else if (strcmp(method->valuestring, "doorbell.isp.captureFrames") == 0 ||
             strcmp(method->valuestring, "ispFrame.captureFrames") == 0 ||
             strcmp(method->valuestring, "ispFrame.save") == 0 ||
             strcmp(method->valuestring, "isp.captureFrames") == 0 ||
             strcmp(method->valuestring, "isp_capture.captureFrames") == 0)
    {
        ret = isp_frame_handle_capture_frames(params, id_json);
    }
    else if (strcmp(method->valuestring, "start_encode") == 0 ||
             strcmp(method->valuestring, "doorbell.camera.turnOn") == 0)
    {
        ret = isp_frame_handle_isp_start(params, id_json);
    }
    else if (strcmp(method->valuestring, "doorbell.camera.turnOff") == 0 ||
             strcmp(method->valuestring, "stop_encode") == 0 ||
             strcmp(method->valuestring, "device:stop-preview") == 0)
    {
        ret = isp_frame_handle_isp_stop(id_json);
    }
    else
    {
        LOGE("unknown method: %s\n", method->valuestring);
        ret = isp_frame_jsonrpc_send_error(id_json, ISP_FRAME_JSONRPC_METHOD_NOT_FOUND, "method", method->valuestring);
    }

exit:
    if (id_json != NULL)
    {
        cJSON_free(id_json);
    }
    cJSON_Delete(root);
    return ret;
}