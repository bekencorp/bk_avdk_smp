#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/avdk_pixel_types.h>
#include <components/log.h>
#include "cJSON.h"
#include "network_transfer.h"
#include "h264e_stream_priv.h"
#define TAG "h264e_stream_proto"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define H264E_STREAM_JSON_MAX_LEN      (2048)
#define H264E_STREAM_RESPONSE_MAX_LEN  (16384)

#define H264E_STREAM_JSONRPC_PARSE_ERROR      (-32700)
#define H264E_STREAM_JSONRPC_INVALID_REQUEST  (-32600)
#define H264E_STREAM_JSONRPC_METHOD_NOT_FOUND (-32601)
#define H264E_STREAM_JSONRPC_INVALID_PARAMS   (-32602)
#define H264E_STREAM_JSONRPC_INTERNAL_ERROR   (-32603)
#define H264E_STREAM_JSONRPC_ENCODER_UNBOUND  (-32001)
#define H264E_STREAM_JSONRPC_SDK_FAILED       (-32002)
#define H264E_STREAM_JSONRPC_INVALID_STATE    (-32003)

#define H264E_STREAM_FORM_JSON \
    "{\"id\":\"h264e-stream-config\",\"title\":\"H264E Stream Encoder Config\"," \
    "\"desc\":\"Dynamic form for PC tool. Submit params through JSON-RPC set_config.\"," \
    "\"groups\":[" \
    "{\"title\":\"Service\",\"fields\":[{\"type\":\"radio\",\"id\":\"mode\",\"label\":\"Service Mode\"," \
    "\"default\":\"tcp\",\"required\":true,\"options\":[{\"label\":\"TCP\",\"value\":\"tcp\"},{\"label\":\"UDP\",\"value\":\"udp\"}]}]}," \
    "{\"id\":\"video\",\"title\":\"Video\",\"fields\":[" \
    "{\"type\":\"select\",\"id\":\"resolution\",\"label\":\"Resolution\",\"default\":\"1920_1080\"," \
    "\"options\":[{\"label\":\"1920x1080\",\"value\":\"1920_1080\"},{\"label\":\"1280x720\",\"value\":\"1280_720\"},{\"label\":\"640x480\",\"value\":\"640_480\"}]}," \
    "{\"type\":\"number\",\"id\":\"width\",\"label\":\"Width\",\"default\":1920,\"readonly\":false,\"min\":320,\"max\":1920,\"unit\":\"pixel\"}," \
    "{\"type\":\"number\",\"id\":\"height\",\"label\":\"Height\",\"default\":1080,\"readonly\":false,\"min\":240,\"max\":1080,\"unit\":\"pixel\"}," \
    "{\"type\":\"number\",\"id\":\"fps\",\"label\":\"FPS\",\"default\":25,\"min\":1,\"max\":30,\"step\":1,\"unit\":\"fps\"}," \
    "{\"type\":\"number\",\"id\":\"bitrateKbps\",\"label\":\"Target Bitrate\",\"default\":1200,\"min\":64,\"max\":8000,\"step\":64,\"unit\":\"kbps\"}]}," \
    "{\"id\":\"rateCtrl\",\"title\":\"Writable Rate Control\",\"fields\":[" \
    "{\"type\":\"number\",\"id\":\"bitrate\",\"label\":\"Bitrate\",\"description\":\"Unit bps. 0 means fixed QP mode.\"," \
    "\"default\":1200000,\"min\":0,\"max\":8000000,\"step\":64000,\"unit\":\"bps\"}," \
    "{\"type\":\"slider\",\"id\":\"qpMinI\",\"label\":\"I Min QP\",\"default\":18,\"min\":0,\"max\":51,\"step\":1}," \
    "{\"type\":\"slider\",\"id\":\"qpMaxI\",\"label\":\"I Max QP\",\"default\":40,\"min\":0,\"max\":51,\"step\":1}," \
    "{\"type\":\"slider\",\"id\":\"qpMinP\",\"label\":\"P Min QP\",\"default\":22,\"min\":0,\"max\":51,\"step\":1}," \
    "{\"type\":\"slider\",\"id\":\"qpMaxP\",\"label\":\"P Max QP\",\"default\":44,\"min\":0,\"max\":51,\"step\":1}]}," \
    "{\"id\":\"vcencRateCtrl\",\"title\":\"Full VCEncRateCtrl Fields (schema only)\"," \
    "\"desc\":\"These fields exist in VCEncRateCtrl but are not applied by current bk_h264_encode_rate_ctrl_t path.\"," \
    "\"fields\":[" \
    "{\"type\":\"number\",\"id\":\"crf\",\"label\":\"crf\",\"readonly\":false,\"min\":0,\"max\":51}," \
    "{\"type\":\"switch\",\"id\":\"pictureRc\",\"label\":\"pictureRc\",\"readonly\":false}," \
    "{\"type\":\"select\",\"id\":\"ctbRc\",\"label\":\"ctbRc\",\"readonly\":false,\"options\":[{\"label\":\"disable\",\"value\":0},{\"label\":\"subjective\",\"value\":1},{\"label\":\"precise\",\"value\":2},{\"label\":\"mixed\",\"value\":3}]}," \
    "{\"type\":\"select\",\"id\":\"blockRCSize\",\"label\":\"blockRCSize\",\"readonly\":false,\"options\":[{\"label\":\"64x64\",\"value\":0},{\"label\":\"32x32\",\"value\":1},{\"label\":\"16x16\",\"value\":2}]}," \
    "{\"type\":\"switch\",\"id\":\"pictureSkip\",\"label\":\"pictureSkip\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"qpHdr\",\"label\":\"qpHdr\",\"readonly\":false,\"min\":-1,\"max\":51}," \
    "{\"type\":\"number\",\"id\":\"qpMinPB\",\"label\":\"qpMinPB\",\"readonly\":false,\"min\":0,\"max\":51}," \
    "{\"type\":\"number\",\"id\":\"qpMaxPB\",\"label\":\"qpMaxPB\",\"readonly\":false,\"min\":0,\"max\":51}," \
    "{\"type\":\"number\",\"id\":\"qpMinI\",\"label\":\"qpMinI\",\"readonly\":false,\"min\":0,\"max\":51}," \
    "{\"type\":\"number\",\"id\":\"qpMaxI\",\"label\":\"qpMaxI\",\"readonly\":false,\"min\":0,\"max\":51}," \
    "{\"type\":\"number\",\"id\":\"bitPerSecond\",\"label\":\"bitPerSecond\",\"readonly\":false,\"min\":10000,\"unit\":\"bps\"}," \
    "{\"type\":\"number\",\"id\":\"cpbMaxRate\",\"label\":\"cpbMaxRate\",\"readonly\":false,\"unit\":\"bps\"}," \
    "{\"type\":\"switch\",\"id\":\"fillerData\",\"label\":\"fillerData\",\"readonly\":false}," \
    "{\"type\":\"switch\",\"id\":\"hrd\",\"label\":\"hrd\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"hrdCpbSize\",\"label\":\"hrdCpbSize\",\"readonly\":false,\"unit\":\"bit\"}," \
    "{\"type\":\"number\",\"id\":\"bitrateWindow\",\"label\":\"bitrateWindow\",\"readonly\":false,\"min\":1,\"max\":300,\"unit\":\"frame\"}," \
    "{\"type\":\"number\",\"id\":\"intraQpDelta\",\"label\":\"intraQpDelta\",\"readonly\":false,\"min\":-12,\"max\":12}," \
    "{\"type\":\"number\",\"id\":\"fixedIntraQp\",\"label\":\"fixedIntraQp\",\"readonly\":false,\"min\":0,\"max\":51}," \
    "{\"type\":\"number\",\"id\":\"bitVarRangeI\",\"label\":\"bitVarRangeI\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"bitVarRangeP\",\"label\":\"bitVarRangeP\",\"readonly\":false,\"min\":10,\"max\":10000}," \
    "{\"type\":\"number\",\"id\":\"bitVarRangeB\",\"label\":\"bitVarRangeB\",\"readonly\":false,\"min\":10,\"max\":10000}," \
    "{\"type\":\"number\",\"id\":\"tolMovingBitRate\",\"label\":\"tolMovingBitRate\",\"readonly\":false,\"min\":0,\"max\":2000}," \
    "{\"type\":\"number\",\"id\":\"monitorFrames\",\"label\":\"monitorFrames\",\"readonly\":false,\"min\":10,\"max\":120}," \
    "{\"type\":\"number\",\"id\":\"targetPicSize\",\"label\":\"targetPicSize\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"smoothPsnrInGOP\",\"label\":\"smoothPsnrInGOP\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"u32StaticSceneIbitPercent\",\"label\":\"u32StaticSceneIbitPercent\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"rcQpDeltaRange\",\"label\":\"rcQpDeltaRange\",\"readonly\":false,\"min\":0,\"max\":51}," \
    "{\"type\":\"number\",\"id\":\"rcBaseMBComplexity\",\"label\":\"rcBaseMBComplexity\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"picQpDeltaMin\",\"label\":\"picQpDeltaMin\",\"readonly\":false,\"min\":-10,\"max\":-1}," \
    "{\"type\":\"number\",\"id\":\"picQpDeltaMax\",\"label\":\"picQpDeltaMax\",\"readonly\":false,\"min\":1,\"max\":10}," \
    "{\"type\":\"number\",\"id\":\"longTermQpDelta\",\"label\":\"longTermQpDelta\",\"readonly\":false,\"min\":-51,\"max\":51}," \
    "{\"type\":\"switch\",\"id\":\"vbr\",\"label\":\"vbr\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"rcMode\",\"label\":\"rcMode\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"tolCtbRcInter\",\"label\":\"tolCtbRcInter\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"tolCtbRcIntra\",\"label\":\"tolCtbRcIntra\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"tolRcUnderflow\",\"label\":\"tolRcUnderflow\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"maxIprop\",\"label\":\"maxIprop\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"minIprop\",\"label\":\"minIprop\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"changePos\",\"label\":\"changePos\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"ctbRcRowQpStep\",\"label\":\"ctbRcRowQpStep\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"ctbRcRowQpDeltaRange\",\"label\":\"ctbRcRowQpDeltaRange\",\"readonly\":false}," \
    "{\"type\":\"switch\",\"id\":\"ctbRcQpDeltaReverse\",\"label\":\"ctbRcQpDeltaReverse\",\"readonly\":false}," \
    "{\"type\":\"number\",\"id\":\"frameRateNum\",\"label\":\"frameRateNum\",\"readonly\":false,\"min\":1,\"max\":1048575}," \
    "{\"type\":\"number\",\"id\":\"frameRateDenom\",\"label\":\"frameRateDenom\",\"readonly\":false,\"min\":1}," \
    "{\"type\":\"switch\",\"id\":\"hieQpDeltaEnable\",\"label\":\"hieQpDeltaEnable\",\"readonly\":false}]}," \
    "{\"id\":\"ctrl\",\"title\":\"Control\",\"fields\":[{\"type\":\"switch\",\"id\":\"forceIdr\",\"label\":\"Force IDR after apply\",\"default\":true}]}" \
    "],\"submit\":{\"label\":\"Apply\",\"method\":\"set_config\",\"build\":\"tree\"}}"

#define H264E_STREAM_RATE_CTRL_SCHEMA_JSON \
    "{\"supported\":[" \
    "{\"id\":\"bitrate\",\"vcenc\":\"bitPerSecond\",\"type\":\"u32\",\"unit\":\"bps\",\"description\":\"0=fixed_qp, nonzero=bitrate_rc\"}," \
    "{\"id\":\"qpMinI\",\"vcenc\":\"qpMinI\",\"type\":\"u32\",\"min\":0,\"max\":51,\"description\":\"I-frame min QP\"}," \
    "{\"id\":\"qpMaxI\",\"vcenc\":\"qpMaxI\",\"type\":\"u32\",\"min\":0,\"max\":51,\"description\":\"I-frame max QP\"}," \
    "{\"id\":\"qpMinP\",\"vcenc\":\"qpMinPB\",\"type\":\"u32\",\"min\":0,\"max\":51,\"description\":\"P/B-frame min QP exposed as P-frame min QP\"}," \
    "{\"id\":\"qpMaxP\",\"vcenc\":\"qpMaxPB\",\"type\":\"u32\",\"min\":0,\"max\":51,\"description\":\"P/B-frame max QP exposed as P-frame max QP\"}" \
    "],\"notExposed\":[" \
    "\"crf\",\"pictureRc\",\"ctbRc\",\"blockRCSize\",\"pictureSkip\",\"qpHdr\"," \
    "\"cpbMaxRate\",\"fillerData\",\"hrd\",\"hrdCpbSize\"," \
    "\"bitrateWindow\",\"intraQpDelta\",\"fixedIntraQp\"," \
    "\"bitVarRangeI\",\"bitVarRangeP\",\"bitVarRangeB\"," \
    "\"tolMovingBitRate\",\"monitorFrames\",\"targetPicSize\",\"smoothPsnrInGOP\"," \
    "\"u32StaticSceneIbitPercent\",\"rcQpDeltaRange\",\"rcBaseMBComplexity\"," \
    "\"picQpDeltaMin\",\"picQpDeltaMax\",\"longTermQpDelta\",\"vbr\",\"rcMode\"," \
    "\"tolCtbRcInter\",\"tolCtbRcIntra\",\"tolRcUnderflow\",\"maxIprop\",\"minIprop\"," \
    "\"changePos\",\"ctbRcRowQpStep\",\"ctbRcRowQpDeltaRange\",\"ctbRcQpDeltaReverse\"," \
    "\"frameRateNum\",\"frameRateDenom\",\"hieQpDeltaEnable\"]," \
    "\"vcencAllFields\":[" \
    "\"crf\",\"pictureRc\",\"ctbRc\",\"blockRCSize\",\"pictureSkip\",\"qpHdr\"," \
    "\"qpMinPB\",\"qpMaxPB\",\"qpMinI\",\"qpMaxI\",\"bitPerSecond\",\"cpbMaxRate\"," \
    "\"fillerData\",\"hrd\",\"hrdCpbSize\",\"bitrateWindow\",\"intraQpDelta\"," \
    "\"fixedIntraQp\",\"bitVarRangeI\",\"bitVarRangeP\",\"bitVarRangeB\"," \
    "\"tolMovingBitRate\",\"monitorFrames\",\"targetPicSize\",\"smoothPsnrInGOP\"," \
    "\"u32StaticSceneIbitPercent\",\"rcQpDeltaRange\",\"rcBaseMBComplexity\"," \
    "\"picQpDeltaMin\",\"picQpDeltaMax\",\"longTermQpDelta\",\"vbr\",\"rcMode\"," \
    "\"tolCtbRcInter\",\"tolCtbRcIntra\",\"tolRcUnderflow\",\"maxIprop\",\"minIprop\"," \
    "\"changePos\",\"ctbRcRowQpStep\",\"ctbRcRowQpDeltaRange\",\"ctbRcQpDeltaReverse\"," \
    "\"frameRateNum\",\"frameRateDenom\",\"hieQpDeltaEnable\"]," \
    "\"note\":\"Current high-level bk_h264_encode_rate_ctrl_t exposes bitrate/qpMinI/qpMaxI/qpMinP/qpMaxP only. qpMinP/qpMaxP map to VCEncRateCtrl qpMinPB/qpMaxPB.\"}"

typedef struct
{
    char mode[8];
    char resolution[16];
    uint16_t width;
    uint16_t height;
    uint16_t fps;
    uint32_t bitrate_kbps;
    uint8_t force_idr;
    bk_h264_encode_rate_ctrl_t rate_ctrl;
} h264e_stream_encoder_config_t;

static h264e_stream_encoder_config_t s_encoder_config = {
    .mode = "tcp",
    .resolution = "1280_720",
    .width = 1280,
    .height = 720,
    .fps = 25,
    .bitrate_kbps = 1200,
    .force_idr = 1,
    .rate_ctrl = {
        .bitrate = 1200000,
        .qp_min_i = 18,
        .qp_max_i = 40,
        .qp_min_p = 22,
        .qp_max_p = 44,
    },
};

static char s_h264e_stream_json_buffer[H264E_STREAM_JSON_MAX_LEN + 1];
static char s_h264e_stream_response_buffer[H264E_STREAM_RESPONSE_MAX_LEN];
static char s_h264e_stream_result_buffer[H264E_STREAM_RESPONSE_MAX_LEN];

static const char *h264e_stream_status_text(h264e_stream_status_t status)
{
    switch (status)
    {
        case H264E_STREAM_STATUS_OK:
            return "ok";
        case H264E_STREAM_STATUS_JSON_ERROR:
            return "json_error";
        case H264E_STREAM_STATUS_UNKNOWN_CMD:
            return "unknown_cmd";
        case H264E_STREAM_STATUS_ENCODER_UNBOUND:
            return "encoder_unbound";
        case H264E_STREAM_STATUS_INVALID_PARAM:
            return "invalid_param";
        case H264E_STREAM_STATUS_SDK_ERROR:
            return "sdk_error";
        case H264E_STREAM_STATUS_INVALID_STATE:
            return "invalid_state";
        default:
            return "unknown_error";
    }
}

static int h264e_stream_ctrl_send_json(const char *json)
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

static const char *h264e_stream_jsonrpc_message(int code)
{
    switch (code)
    {
        case H264E_STREAM_JSONRPC_PARSE_ERROR:
            return "Parse error";
        case H264E_STREAM_JSONRPC_INVALID_REQUEST:
            return "Invalid Request";
        case H264E_STREAM_JSONRPC_METHOD_NOT_FOUND:
            return "Method not found";
        case H264E_STREAM_JSONRPC_INVALID_PARAMS:
            return "Invalid params";
        case H264E_STREAM_JSONRPC_INTERNAL_ERROR:
            return "Internal error";
        case H264E_STREAM_JSONRPC_ENCODER_UNBOUND:
            return "Encoder not bound";
        case H264E_STREAM_JSONRPC_SDK_FAILED:
            return "SDK call failed";
        case H264E_STREAM_JSONRPC_INVALID_STATE:
            return "Invalid state";
        default:
            return "Server error";
    }
}

static int h264e_stream_jsonrpc_send_error(const char *id_json,
    int code,
    const char *field,
    const char *reason);

static int h264e_stream_jsonrpc_send_result(const char *id_json, const char *result_json)
{
    int len;
    const char *safe_id = (id_json != NULL) ? id_json : "null";
    const char *safe_result = (result_json != NULL) ? result_json : "{\"message\":\"ok\"}";

    len = snprintf(s_h264e_stream_response_buffer, sizeof(s_h264e_stream_response_buffer),
                   "{\"jsonrpc\":\"2.0\",\"result\":%s,\"id\":%s}\n",
                   safe_result, safe_id);
    if (len < 0 || len >= (int)sizeof(s_h264e_stream_response_buffer))
    {
        LOGE("JSON-RPC result response truncated, len=%d buffer=%u\n",
             len, (uint32_t)sizeof(s_h264e_stream_response_buffer));
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INTERNAL_ERROR, NULL, "response_too_large");
    }
    return h264e_stream_ctrl_send_json(s_h264e_stream_response_buffer);
}

static int h264e_stream_jsonrpc_send_error(const char *id_json,
    int code,
    const char *field,
    const char *reason)
{
    int len;
    char data[160] = {0};
    const char *safe_id = (id_json != NULL) ? id_json : "null";
    const char *safe_reason = (reason != NULL) ? reason : h264e_stream_jsonrpc_message(code);

    if (field != NULL)
    {
        snprintf(data, sizeof(data),
                 ",\"data\":{\"field\":\"%s\",\"reason\":\"%s\"}",
                 field, safe_reason);
    }

    len = snprintf(s_h264e_stream_response_buffer, sizeof(s_h264e_stream_response_buffer),
                   "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":%d,\"message\":\"%s\"%s},\"id\":%s}\n",
                   code, h264e_stream_jsonrpc_message(code), data, safe_id);
    if (len < 0 || len >= (int)sizeof(s_h264e_stream_response_buffer))
    {
        LOGE("JSON-RPC error response truncated, len=%d buffer=%u\n",
             len, (uint32_t)sizeof(s_h264e_stream_response_buffer));
        return -1;
    }
    return h264e_stream_ctrl_send_json(s_h264e_stream_response_buffer);
}

int h264e_stream_protocol_send_status(const char *cmd, uint32_t seq, h264e_stream_status_t status, const char *message)
{
    char id_json[16];
    (void)cmd;
    snprintf(id_json, sizeof(id_json), "%u", seq);

    if (status == H264E_STREAM_STATUS_OK)
    {
        return h264e_stream_jsonrpc_send_result(id_json, "{\"message\":\"ok\"}");
    }
    return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INTERNAL_ERROR, NULL, message);
}

static char *h264e_stream_jsonrpc_id_to_string(cJSON *id)
{
    if (id == NULL)
    {
        return NULL;
    }
    return cJSON_PrintUnformatted(id);
}

static cJSON *h264e_stream_json_get_alias(cJSON *object, const char *key1, const char *key2)
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

static int h264e_stream_json_get_u32_alias(cJSON *object,
    const char *key1,
    const char *key2,
    uint32_t *value)
{
    cJSON *item = h264e_stream_json_get_alias(object, key1, key2);
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

static int h264e_stream_json_get_bool(cJSON *object, const char *key, uint8_t *value)
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


static int h264e_stream_rate_ctrl_to_json(const bk_h264_encode_rate_ctrl_t *rate_ctrl,
    char *buffer,
    size_t buffer_len)
{
        return snprintf(buffer, buffer_len,
        "\"rateControl\":{\"bitrate\":%u,\"qpMinI\":%u,\"qpMaxI\":%u,\"qpMinP\":%u,\"qpMaxP\":%u}",
        rate_ctrl->bitrate,
        rate_ctrl->qp_min_i,
        rate_ctrl->qp_max_i,
        rate_ctrl->qp_min_p,
        rate_ctrl->qp_max_p);
}

static int h264e_stream_send_rate_ctrl_response(const char *id_json,
    const char *message,
    const bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    char result[320];
    char rate_json[160] = {0};
    const char *safe_message = (message != NULL) ? message : "ok";

    if (rate_ctrl != NULL)
    {
        h264e_stream_rate_ctrl_to_json(rate_ctrl, rate_json, sizeof(rate_json));
        snprintf(result, sizeof(result),
                 "{\"message\":\"%s\",%s}",
                 safe_message, rate_json);
    } else
    {
        snprintf(result, sizeof(result),
                 "{\"message\":\"%s\"}",
                 safe_message);
    }
    return h264e_stream_jsonrpc_send_result(id_json, result);
}

static int h264e_stream_validate_rate_ctrl(const bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    if (rate_ctrl->qp_min_i > 51 || rate_ctrl->qp_max_i > 51 ||
        rate_ctrl->qp_min_p > 51 || rate_ctrl->qp_max_p > 51)
    {
        return -1;
    }
    if (rate_ctrl->qp_min_i && rate_ctrl->qp_max_i && rate_ctrl->qp_min_i > rate_ctrl->qp_max_i)
    {
        return -1;
    }
    if (rate_ctrl->qp_min_p && rate_ctrl->qp_max_p && rate_ctrl->qp_min_p > rate_ctrl->qp_max_p)
    {
        return -1;
    }
    return 0;
}

static int h264e_stream_apply_u32_to_u8(uint32_t value, uint8_t *dst)
{
    if (value > 255)
    {
        return -1;
    }
    *dst = (uint8_t)value;
    return 0;
}

static int h264e_stream_parse_rate_ctrl(cJSON *object,
    bk_h264_encode_rate_ctrl_t *rate_ctrl,
    uint8_t *changed,
    const char **field)
{
    uint32_t value;
    int ret;

    if (object == NULL || rate_ctrl == NULL || changed == NULL)
    {
        return 0;
    }

    ret = h264e_stream_json_get_u32_alias(object, "bitrate", "bitPerSecond", &value);
    if (ret < 0)
    {
        *field = "rateCtrl.bitrate";
        return -1;
    }
    if (ret > 0)
    {
        rate_ctrl->bitrate = value;
        *changed = 1;
    }

    ret = h264e_stream_json_get_u32_alias(object, "qpMinI", "qp_min_i", &value);
    if (ret < 0 || (ret > 0 && h264e_stream_apply_u32_to_u8(value, &rate_ctrl->qp_min_i) != 0))
    {
        *field = "rateCtrl.qpMinI";
        return -1;
    }
    *changed |= (ret > 0);
    ret = h264e_stream_json_get_u32_alias(object, "qpMaxI", "qp_max_i", &value);
    if (ret < 0 || (ret > 0 && h264e_stream_apply_u32_to_u8(value, &rate_ctrl->qp_max_i) != 0))
    {
        *field = "rateCtrl.qpMaxI";
        return -1;
    }
    *changed |= (ret > 0);
    ret = h264e_stream_json_get_u32_alias(object, "qpMinP", "qp_min_p", &value);
    if (ret == 0)
    {
        ret = h264e_stream_json_get_u32_alias(object, "qpMinPB", NULL, &value);
    }
    if (ret < 0 || (ret > 0 && h264e_stream_apply_u32_to_u8(value, &rate_ctrl->qp_min_p) != 0))
    {
        *field = "rateCtrl.qpMinP";
        return -1;
    }
    *changed |= (ret > 0);
    ret = h264e_stream_json_get_u32_alias(object, "qpMaxP", "qp_max_p", &value);
    if (ret == 0)
    {
        ret = h264e_stream_json_get_u32_alias(object, "qpMaxPB", NULL, &value);
    }
    if (ret < 0 || (ret > 0 && h264e_stream_apply_u32_to_u8(value, &rate_ctrl->qp_max_p) != 0))
    {
        *field = "rateCtrl.qpMaxP";
        return -1;
    }
    *changed |= (ret > 0);

    if (h264e_stream_validate_rate_ctrl(rate_ctrl) != 0)
    {
        *field = "rateCtrl";
        return -1;
    }
    return 0;
}

static void h264e_stream_sync_rate_ctrl_from_encoder(h264e_stream_session_ctx_t *ctx,
    bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    if (ctx != NULL && ctx->encoder != NULL &&
        bk_h264_encode_get_rate_ctrl(ctx->encoder, rate_ctrl) == AVDK_ERR_OK)
    {
        s_encoder_config.rate_ctrl = *rate_ctrl;
    }
}

static int h264e_stream_handle_get_rate_ctrl(const char *id_json)
{
    bk_h264_encode_rate_ctrl_t rate_ctrl = s_encoder_config.rate_ctrl;
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();
    if (ctx->encoder == NULL)
    {
        return h264e_stream_send_rate_ctrl_response(id_json, "ok", &rate_ctrl);
    }
    if (bk_h264_encode_get_rate_ctrl(ctx->encoder, &rate_ctrl) != AVDK_ERR_OK)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "get_rate_ctrl_failed");
    }
    s_encoder_config.rate_ctrl = rate_ctrl;
    return h264e_stream_send_rate_ctrl_response(id_json, "ok", &rate_ctrl);
}

static int h264e_stream_handle_set_rate_ctrl(cJSON *params, const char *id_json)
{
    const char *field = NULL;
    uint8_t changed = 0;
    bk_h264_encode_rate_ctrl_t rate_ctrl = s_encoder_config.rate_ctrl;
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();
    cJSON *rate_ctrl_json = h264e_stream_json_get_alias(params, "rateCtrl", "rate_ctrl");
    if (rate_ctrl_json == NULL)
    {
        rate_ctrl_json = cJSON_GetObjectItem(params, "rateControl");
    }

    if (ctx->encoder == NULL)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_ENCODER_UNBOUND, NULL, NULL);
    }
    h264e_stream_sync_rate_ctrl_from_encoder(ctx, &rate_ctrl);
    if (rate_ctrl_json == NULL)
    {
        rate_ctrl_json = params;
    }
    if (h264e_stream_parse_rate_ctrl(rate_ctrl_json, &rate_ctrl, &changed, &field) != 0 || !changed)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_PARAMS, field, "invalid rateCtrl");
    }
    if (bk_h264_encode_set_rate_ctrl(ctx->encoder, &rate_ctrl) != AVDK_ERR_OK)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "set_rate_ctrl_failed");
    }
    if (ctx->config.auto_force_idr)
    {
        (void)bk_h264_encode_force_idr(ctx->encoder);
    }
    s_encoder_config.rate_ctrl = rate_ctrl;
    return h264e_stream_send_rate_ctrl_response(id_json, "ok", &rate_ctrl);
}

static int h264e_stream_handle_force_idr(const char *id_json)
{
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();
    if (ctx->encoder == NULL)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_ENCODER_UNBOUND, NULL, NULL);
    }
    if (bk_h264_encode_force_idr(ctx->encoder) != AVDK_ERR_OK)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "force_idr_failed");
    }
    return h264e_stream_jsonrpc_send_result(id_json, "{\"message\":\"ok\"}");
}

static int h264e_stream_handle_start_encode(const char *id_json)
{
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();
    if (ctx->encoder == NULL)
    {
        if (ctx->media_start == NULL)
        {
            return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_ENCODER_UNBOUND, NULL, NULL);
        }
        if (ctx->media_start(ctx->media_user_data) != BK_OK)
        {
            return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "media_start_failed");
        }
    }
    if (ctx->encoder == NULL)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_ENCODER_UNBOUND, NULL, "encoder_not_bound_after_media_start");
    }
    if (bk_h264_encode_set_rate_ctrl(ctx->encoder, &s_encoder_config.rate_ctrl) != AVDK_ERR_OK)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "set_rate_ctrl_failed");
    }
    if (bk_h264_encode_start(ctx->encoder) != AVDK_ERR_OK)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "start_encode_failed");
    }
    return h264e_stream_jsonrpc_send_result(id_json, "{\"message\":\"ok\"}");
}

static int h264e_stream_handle_stop_encode(const char *id_json)
{
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();
    if (ctx->media_stop != NULL)
    {
        (void)ctx->media_stop(ctx->media_user_data);
    }
    return h264e_stream_jsonrpc_send_result(id_json, "{\"message\":\"ok\"}");
}

const char *h264e_stream_session_get_form_json(void)
{
    return H264E_STREAM_FORM_JSON;
}

const char *h264e_stream_session_get_rate_ctrl_schema_json(void)
{
    return H264E_STREAM_RATE_CTRL_SCHEMA_JSON;
}

static int h264e_stream_handle_get_config_schema(const char *id_json)
{
    int len = snprintf(s_h264e_stream_result_buffer, sizeof(s_h264e_stream_result_buffer),
                       "{\"form\":%s}", h264e_stream_session_get_form_json());
    if (len < 0 || len >= (int)sizeof(s_h264e_stream_result_buffer))
    {
        LOGE("config schema result truncated, len=%d buffer=%u\n",
             len, (uint32_t)sizeof(s_h264e_stream_result_buffer));
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INTERNAL_ERROR, NULL, "schema_too_large");
    }

    return h264e_stream_jsonrpc_send_result(id_json, s_h264e_stream_result_buffer);
}

static int h264e_stream_handle_get_minimal_config_schema(const char *id_json)
{
    return h264e_stream_jsonrpc_send_result(id_json,
        "{\"id\":\"h264e-stream-config\",\"title\":\"H264E Stream Encoder Config\","
        "\"groups\":[],\"submit\":{\"label\":\"Apply\",\"method\":\"doorbell.config.setParameterValues\"}}");
}

static int h264e_stream_config_to_result(char *buffer, size_t buffer_len)
{
    bk_h264_encode_rate_ctrl_t *rc = &s_encoder_config.rate_ctrl;
    return snprintf(buffer, buffer_len,
        "{\"values\":{\"mode\":\"%s\",\"resolution\":\"%s\",\"width\":%u,\"height\":%u,"
        "\"fps\":%u,\"bitrateKbps\":%u,\"bitrate\":%u,\"qpMinI\":%u,\"qpMaxI\":%u,"
        "\"qpMinP\":%u,\"qpMaxP\":%u,\"forceIdr\":%s},"
        "\"config\":{\"mode\":\"%s\",\"video\":{\"resolution\":\"%s\",\"width\":%u,\"height\":%u,"
        "\"fps\":%u,\"bitrateKbps\":%u},\"rateCtrl\":{\"bitrate\":%u,\"qpMinI\":%u,"
        "\"qpMaxI\":%u,\"qpMinP\":%u,\"qpMaxP\":%u},\"ctrl\":{\"forceIdr\":%s}}}",
        s_encoder_config.mode, s_encoder_config.resolution, s_encoder_config.width,
        s_encoder_config.height, s_encoder_config.fps, s_encoder_config.bitrate_kbps,
        rc->bitrate, rc->qp_min_i, rc->qp_max_i, rc->qp_min_p, rc->qp_max_p,
        s_encoder_config.force_idr ? "true" : "false",
        s_encoder_config.mode, s_encoder_config.resolution, s_encoder_config.width,
        s_encoder_config.height, s_encoder_config.fps, s_encoder_config.bitrate_kbps,
        rc->bitrate, rc->qp_min_i, rc->qp_max_i, rc->qp_min_p, rc->qp_max_p,
        s_encoder_config.force_idr ? "true" : "false");
}

static int h264e_stream_handle_get_config(const char *id_json)
{
    bk_h264_encode_rate_ctrl_t rate_ctrl = s_encoder_config.rate_ctrl;
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();

    h264e_stream_sync_rate_ctrl_from_encoder(ctx, &rate_ctrl);
    h264e_stream_config_to_result(s_h264e_stream_result_buffer, sizeof(s_h264e_stream_result_buffer));
    return h264e_stream_jsonrpc_send_result(id_json, s_h264e_stream_result_buffer);
}

static void h264e_stream_apply_resolution(const char *resolution, h264e_stream_encoder_config_t *config)
{
    if (strcmp(resolution, "1920_1080") == 0)
    {
        config->width = 1920;
        config->height = 1080;
        snprintf(config->resolution, sizeof(config->resolution), "1920_1080");
    }
    else if (strcmp(resolution, "1280_720") == 0)
    {
        config->width = 1280;
        config->height = 720;
        snprintf(config->resolution, sizeof(config->resolution), "1280_720");
    }
    else if (strcmp(resolution, "640_480") == 0)
    {
        config->width = 640;
        config->height = 480;
        snprintf(config->resolution, sizeof(config->resolution), "640_480");
    }
}

static int h264e_stream_parse_video_config(cJSON *object,
    h264e_stream_encoder_config_t *config,
    const char **field)
{
    cJSON *resolution;
    uint32_t value;
    int ret;

    if (object == NULL)
    {
        return 0;
    }

    resolution = cJSON_GetObjectItem(object, "resolution");
    if (resolution != NULL)
    {
        if (!cJSON_IsString(resolution) || resolution->valuestring == NULL)
        {
            *field = "video.resolution";
            return -1;
        }
        if (strcmp(resolution->valuestring, "1920_1080") != 0 &&
            strcmp(resolution->valuestring, "1280_720") != 0 &&
            strcmp(resolution->valuestring, "640_480") != 0)
        {
            *field = "video.resolution";
            return -1;
        }
        h264e_stream_apply_resolution(resolution->valuestring, config);
    }

    ret = h264e_stream_json_get_u32_alias(object, "width", NULL, &value);
    if (ret < 0 || (ret > 0 && (value < 320 || value > 1920)))
    {
        *field = "video.width";
        return -1;
    }
    if (ret > 0)
    {
        config->width = (uint16_t)value;
    }

    ret = h264e_stream_json_get_u32_alias(object, "height", NULL, &value);
    if (ret < 0 || (ret > 0 && (value < 240 || value > 1080)))
    {
        *field = "video.height";
        return -1;
    }
    if (ret > 0)
    {
        config->height = (uint16_t)value;
    }

    ret = h264e_stream_json_get_u32_alias(object, "fps", "frame_rate", &value);
    if (ret < 0 || (ret > 0 && (value < 1 || value > 30)))
    {
        *field = "video.fps";
        return -1;
    }
    if (ret > 0)
    {
        config->fps = (uint16_t)value;
    }

    ret = h264e_stream_json_get_u32_alias(object, "bitrateKbps", "bitrate_kbps", &value);
    if (ret < 0 || (ret > 0 && (value < 64 || value > 8000)))
    {
        *field = "video.bitrateKbps";
        return -1;
    }
    if (ret > 0)
    {
        config->bitrate_kbps = value;
    }
    return 0;
}

static int h264e_stream_handle_set_config(cJSON *params, const char *id_json)
{
    h264e_stream_encoder_config_t next = s_encoder_config;
    bk_h264_encode_rate_ctrl_t rate_ctrl = s_encoder_config.rate_ctrl;
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();
    cJSON *mode;
    cJSON *video;
    cJSON *rate_ctrl_json;
    cJSON *ctrl;
    uint8_t changed = 0;
    uint8_t bool_value;
    const char *field = NULL;

    if (params == NULL || !cJSON_IsObject(params))
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_PARAMS, "params", "object expected");
    }

    mode = h264e_stream_json_get_alias(params, "mode", "serviceType");
    if (mode != NULL)
    {
        if (!cJSON_IsString(mode) || mode->valuestring == NULL)
        {
            return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_PARAMS, "mode", "string expected");
        }
        if (strcmp(mode->valuestring, "tcp") == 0 || strcmp(mode->valuestring, "udp") == 0)
        {
            snprintf(next.mode, sizeof(next.mode), "%s", mode->valuestring);
        }
        else
        {
            return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_PARAMS, "mode", "unsupported mode");
        }
    }

    video = cJSON_GetObjectItem(params, "video");
    if (video == NULL)
    {
        video = params;
    }
    if (h264e_stream_parse_video_config(video, &next, &field) != 0)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_PARAMS, field, "invalid video config");
    }

    rate_ctrl_json = h264e_stream_json_get_alias(params, "rateCtrl", "rate_ctrl");
    if (rate_ctrl_json == NULL)
    {
        rate_ctrl_json = cJSON_GetObjectItem(params, "rateControl");
    }
    if (rate_ctrl_json == NULL)
    {
        rate_ctrl_json = params;
    }
    h264e_stream_sync_rate_ctrl_from_encoder(ctx, &rate_ctrl);
    if (h264e_stream_parse_rate_ctrl(rate_ctrl_json, &rate_ctrl, &changed, &field) != 0)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_PARAMS, field, "invalid rateCtrl");
    }

    ctrl = cJSON_GetObjectItem(params, "ctrl");
    if (ctrl != NULL)
    {
        if (h264e_stream_json_get_bool(ctrl, "forceIdr", &bool_value) < 0)
        {
            return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_PARAMS, "ctrl.forceIdr", "boolean expected");
        }
        next.force_idr = bool_value;
    }

    if (changed && ctx->encoder != NULL)
    {
        if (bk_h264_encode_set_rate_ctrl(ctx->encoder, &rate_ctrl) != AVDK_ERR_OK)
        {
            return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "set_rate_ctrl_failed");
        }
        if (next.force_idr || ctx->config.auto_force_idr)
        {
            (void)bk_h264_encode_force_idr(ctx->encoder);
        }
    }
    if (changed)
    {
        next.rate_ctrl = rate_ctrl;
    }

    s_encoder_config = next;
    return h264e_stream_handle_get_config(id_json);
}

int h264e_stream_protocol_handle(uint8_t *data, uint32_t length)
{
    cJSON *root;
    cJSON *jsonrpc;
    cJSON *method;
    cJSON *params;
    char *id_json = NULL;
    int ret;

    if (data == NULL || length == 0 || length > H264E_STREAM_JSON_MAX_LEN)
    {
        return h264e_stream_jsonrpc_send_error(NULL, H264E_STREAM_JSONRPC_PARSE_ERROR, NULL, "invalid length");
    }
    memcpy(s_h264e_stream_json_buffer, data, length);
    s_h264e_stream_json_buffer[length] = '\0';

    root = cJSON_ParseWithLength(s_h264e_stream_json_buffer, length);
    if (root == NULL)
    {
        return h264e_stream_jsonrpc_send_error(NULL, H264E_STREAM_JSONRPC_PARSE_ERROR, NULL, "parse failed");
    }

    id_json = h264e_stream_jsonrpc_id_to_string(cJSON_GetObjectItem(root, "id"));
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
        ret = h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_REQUEST, NULL, NULL);
        goto exit;
    }

    if (strcmp(method->valuestring, "get_config_schema") == 0 ||
        strcmp(method->valuestring, "h264EScream.getParameterSchema") == 0 ||
        strcmp(method->valuestring, "doorbell.config.getParameterSchema") == 0 ||
        strcmp(method->valuestring, "h264e.getParameterSchema") == 0 ||
        strcmp(method->valuestring, "encode_preview.getParameterSchema") == 0)
    {
        ret = h264e_stream_handle_get_config_schema(id_json);
    }
    else if (strcmp(method->valuestring, "get_config") == 0 ||
             strcmp(method->valuestring, "h264EScream.getConfig") == 0 ||
             strcmp(method->valuestring, "doorbell.encoder.getVcencRateControl") == 0 ||
             strcmp(method->valuestring, "h264e.getConfig") == 0 ||
             strcmp(method->valuestring, "encode_preview.getConfig") == 0)
    {
        ret = h264e_stream_handle_get_config(id_json);
    }
    else if (strcmp(method->valuestring, "set_config") == 0 ||
             strcmp(method->valuestring, "doorbell.config.setParameterValues") == 0 ||
             strcmp(method->valuestring, "h264e.setParameterValues") == 0 ||
             strcmp(method->valuestring, "encode_preview.setParameterValues") == 0)
    {
        ret = h264e_stream_handle_set_config(params, id_json);
    }
    else if (strcmp(method->valuestring, "get_rate_ctrl") == 0 ||
             strcmp(method->valuestring, "h264EScream.getRateControl") == 0 ||
             strcmp(method->valuestring, "h264e.getRateControl") == 0 ||
             strcmp(method->valuestring, "encode_preview.getRateControl") == 0)
    {
        ret = h264e_stream_handle_get_rate_ctrl(id_json);
    }
    else if (strcmp(method->valuestring, "set_rate_ctrl") == 0 ||
             strcmp(method->valuestring, "h264EScream.setRateControl") == 0 ||
             strcmp(method->valuestring, "doorbell.encoder.setVcencRateControl") == 0 ||
             strcmp(method->valuestring, "h264e.setRateControl") == 0 ||
             strcmp(method->valuestring, "encode_preview.setRateControl") == 0)
    {
        ret = h264e_stream_handle_set_rate_ctrl(params, id_json);
    }
    else if (strcmp(method->valuestring, "force_idr") == 0 ||
             strcmp(method->valuestring, "h264e.forceIdr") == 0 ||
             strcmp(method->valuestring, "encode_preview.forceIdr") == 0)
    {
        ret = h264e_stream_handle_force_idr(id_json);
    }
    else if (strcmp(method->valuestring, "start_encode") == 0 ||
             strcmp(method->valuestring, "h264EScream.turnOn") == 0 ||
             strcmp(method->valuestring, "h264e.start") == 0 ||
             strcmp(method->valuestring, "encode_preview.start") == 0)
    {
        ret = h264e_stream_handle_start_encode(id_json);
    }
    else if (strcmp(method->valuestring, "doorbell.camera.turnOn") == 0)
    {
        ret = h264e_stream_handle_start_encode(id_json);
    }
    else if (strcmp(method->valuestring, "doorbell.camera.turnOff") == 0 ||
             strcmp(method->valuestring, "h264EScream.turnOff") == 0 ||
             strcmp(method->valuestring, "h264e.stop") == 0 ||
             strcmp(method->valuestring, "encode_preview.stop") == 0)
    {
        ret = h264e_stream_handle_stop_encode(id_json);
    }
    else if (strcmp(method->valuestring, "device:stop-preview") == 0)
    {
        ret = h264e_stream_handle_stop_encode(id_json);
    }
    else
    {
        LOGE("unknown method: %s\n", method->valuestring);
        ret = h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_METHOD_NOT_FOUND, "method", method->valuestring);
    }

exit:
    if (id_json != NULL)
    {
        cJSON_free(id_json);
    }
    cJSON_Delete(root);
    return ret;
}