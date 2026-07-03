#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/avdk_pixel_types.h>
#include <components/log.h>
#include "cJSON.h"
#include "network_transfer.h"
#include "h264e_stream_priv.h"
#include "h264_encode_vcenc_rate_ctrl_priv.h"
#define TAG "h264e_stream_proto"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define H264E_STREAM_JSON_MAX_LEN      (8192)
#define H264E_STREAM_RESPONSE_MAX_LEN  (16384)

#define H264E_STREAM_JSONRPC_PARSE_ERROR      (-32700)
#define H264E_STREAM_JSONRPC_INVALID_REQUEST  (-32600)
#define H264E_STREAM_JSONRPC_METHOD_NOT_FOUND (-32601)
#define H264E_STREAM_JSONRPC_INVALID_PARAMS   (-32602)
#define H264E_STREAM_JSONRPC_INTERNAL_ERROR   (-32603)
#define H264E_STREAM_JSONRPC_ENCODER_UNBOUND  (-32001)
#define H264E_STREAM_JSONRPC_SDK_FAILED       (-32002)
#define H264E_STREAM_JSONRPC_INVALID_STATE    (-32003)

typedef struct
{
    char mode[8];
    uint16_t width;
    uint16_t height;
    uint16_t fps;
    uint32_t bitrate_kbps;
    uint32_t gop_frame_count;
    uint8_t force_idr;
    bk_h264_encode_rate_ctrl_t rate_ctrl;
    bk_h264_encode_vcenc_rate_ctrl_t vcenc_rate_ctrl;
    uint8_t vcenc_rate_ctrl_valid;
} h264e_stream_encoder_config_t;

static h264e_stream_encoder_config_t s_encoder_config = {
    .mode = "tcp",
    .width = 2304,
    .height = 1296,
    .fps = 20,
    .bitrate_kbps = 1200,
    .gop_frame_count = 20,
    .force_idr = 1,
    .rate_ctrl = {
        .bitrate = 1200000,
        .qp_min_i = 18,
        .qp_max_i = 40,
        .qp_min_p = 22,
        .qp_max_p = 44,
    },
    .vcenc_rate_ctrl = {
        .picture_rc = 1,
        .ctb_rc = 0,
        .block_rc_size = 0,
        .picture_skip = 0,
        .qp_hdr = -1,
        .qp_min_pb = 22,
        .qp_max_pb = 44,
        .qp_min_i = 18,
        .qp_max_i = 40,
        .bit_per_second = 1200000,
        .bitrate_window = 20,
        .frame_rate_num = 20,
        .frame_rate_denom = 1,
    },
    .vcenc_rate_ctrl_valid = 0,
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

static int h264e_stream_json_get_bool_alias(cJSON *object,
    const char *key1,
    const char *key2,
    uint32_t *value)
{
    cJSON *item = h264e_stream_json_get_alias(object, key1, key2);
    if (item == NULL)
    {
        return 0;
    }
    if (cJSON_IsBool(item))
    {
        *value = cJSON_IsTrue(item) ? 1U : 0U;
        return 1;
    }
    if (cJSON_IsNumber(item) && (item->valuedouble == 0 || item->valuedouble == 1))
    {
        *value = (uint32_t)item->valuedouble;
        return 1;
    }
    return -1;
}

static int h264e_stream_json_get_i32_alias(cJSON *object,
    const char *key1,
    const char *key2,
    int *value)
{
    cJSON *item = h264e_stream_json_get_alias(object, key1, key2);
    if (item == NULL)
    {
        return 0;
    }
    if (!cJSON_IsNumber(item))
    {
        return -1;
    }
    *value = (int)item->valuedouble;
    return 1;
}

static int h264e_stream_json_get_float_alias(cJSON *object,
    const char *key1,
    const char *key2,
    float *value)
{
    cJSON *item = h264e_stream_json_get_alias(object, key1, key2);
    if (item == NULL)
    {
        return 0;
    }
    if (!cJSON_IsNumber(item))
    {
        return -1;
    }
    *value = (float)item->valuedouble;
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

static void h264e_stream_rate_ctrl_from_vcenc(const bk_h264_encode_vcenc_rate_ctrl_t *vcenc,
    bk_h264_encode_rate_ctrl_t *rate_ctrl)
{
    rate_ctrl->bitrate = vcenc->picture_rc ? vcenc->bit_per_second : 0U;
    rate_ctrl->qp_min_i = (uint8_t)vcenc->qp_min_i;
    rate_ctrl->qp_max_i = (uint8_t)vcenc->qp_max_i;
    rate_ctrl->qp_min_p = (uint8_t)vcenc->qp_min_pb;
    rate_ctrl->qp_max_p = (uint8_t)vcenc->qp_max_pb;
}

static void h264e_stream_vcenc_from_rate_ctrl(const bk_h264_encode_rate_ctrl_t *rate_ctrl,
    bk_h264_encode_vcenc_rate_ctrl_t *vcenc)
{
    vcenc->bit_per_second = rate_ctrl->bitrate;
    vcenc->picture_rc = (rate_ctrl->bitrate != 0) ? 1U : 0U;
    vcenc->qp_min_i = rate_ctrl->qp_min_i;
    vcenc->qp_max_i = rate_ctrl->qp_max_i;
    vcenc->qp_min_pb = rate_ctrl->qp_min_p;
    vcenc->qp_max_pb = rate_ctrl->qp_max_p;
    vcenc->qp_hdr = (rate_ctrl->bitrate == 0) ? (int)rate_ctrl->qp_min_i : -1;
}

static int h264e_stream_vcenc_rate_ctrl_to_json(const bk_h264_encode_vcenc_rate_ctrl_t *rc,
    char *buffer,
    size_t buffer_len)
{
    return snprintf(buffer, buffer_len,
        "\"vcencRateCtrl\":{\"crf\":%d,\"pictureRc\":%u,\"ctbRc\":%u,"
        "\"blockRCSize\":%u,\"pictureSkip\":%u,\"qpHdr\":%d,"
        "\"qpMinPB\":%u,\"qpMaxPB\":%u,\"qpMinI\":%u,\"qpMaxI\":%u,"
        "\"bitPerSecond\":%u,\"cpbMaxRate\":%u,\"fillerData\":%u,"
        "\"hrd\":%u,\"hrdCpbSize\":%u,\"bitrateWindow\":%u,"
        "\"intraQpDelta\":%d,\"fixedIntraQp\":%u,"
        "\"bitVarRangeI\":%d,\"bitVarRangeP\":%d,\"bitVarRangeB\":%d,"
        "\"tolMovingBitRate\":%d,\"monitorFrames\":%d,\"targetPicSize\":%d,"
        "\"smoothPsnrInGOP\":%d,\"u32StaticSceneIbitPercent\":%u,"
        "\"rcQpDeltaRange\":%u,\"rcBaseMBComplexity\":%u,"
        "\"picQpDeltaMin\":%d,\"picQpDeltaMax\":%d,\"longTermQpDelta\":%d,"
        "\"vbr\":%d,\"rcMode\":%u,\"tolCtbRcInter\":%.3f,\"tolCtbRcIntra\":%.3f,"
        "\"tolRcUnderflow\":%d,\"maxIprop\":%u,\"minIprop\":%u,"
        "\"changePos\":%d,\"ctbRcRowQpStep\":%d,\"ctbRcRowQpDeltaRange\":%d,"
        "\"ctbRcQpDeltaReverse\":%u,\"frameRateNum\":%u,"
        "\"frameRateDenom\":%u,\"hieQpDeltaEnable\":%u}",
        rc->crf, rc->picture_rc, rc->ctb_rc,
        rc->block_rc_size, rc->picture_skip, rc->qp_hdr,
        rc->qp_min_pb, rc->qp_max_pb, rc->qp_min_i, rc->qp_max_i,
        rc->bit_per_second, rc->cpb_max_rate, rc->filler_data,
        rc->hrd, rc->hrd_cpb_size, rc->bitrate_window,
        rc->intra_qp_delta, rc->fixed_intra_qp,
        rc->bit_var_range_i, rc->bit_var_range_p, rc->bit_var_range_b,
        rc->tol_moving_bit_rate, rc->monitor_frames, rc->target_pic_size,
        rc->smooth_psnr_in_gop, rc->static_scene_i_bit_percent,
        rc->rc_qp_delta_range, rc->rc_base_mb_complexity,
        rc->pic_qp_delta_min, rc->pic_qp_delta_max, rc->long_term_qp_delta,
        rc->vbr, rc->rc_mode, rc->tol_ctb_rc_inter, rc->tol_ctb_rc_intra,
        rc->tol_rc_underflow, rc->max_i_prop, rc->min_i_prop,
        rc->change_pos, rc->ctb_rc_row_qp_step, rc->ctb_rc_row_qp_delta_range,
        rc->ctb_rc_qp_delta_reverse, rc->frame_rate_num,
        rc->frame_rate_denom, rc->hie_qp_delta_enable);
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

static int h264e_stream_validate_vcenc_rate_ctrl(const bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl)
{
    if (rate_ctrl->qp_min_i > 51 || rate_ctrl->qp_max_i > 51 ||
        rate_ctrl->qp_min_pb > 51 || rate_ctrl->qp_max_pb > 51 ||
        rate_ctrl->fixed_intra_qp > 51)
    {
        return -1;
    }
    if (rate_ctrl->qp_hdr > 51 || rate_ctrl->qp_hdr < -1)
    {
        return -1;
    }
    if (rate_ctrl->qp_min_i && rate_ctrl->qp_max_i && rate_ctrl->qp_min_i > rate_ctrl->qp_max_i)
    {
        return -1;
    }
    if (rate_ctrl->qp_min_pb && rate_ctrl->qp_max_pb && rate_ctrl->qp_min_pb > rate_ctrl->qp_max_pb)
    {
        return -1;
    }
    if (rate_ctrl->frame_rate_num == 0 || rate_ctrl->frame_rate_denom == 0)
    {
        return -1;
    }
    return 0;
}

static int h264e_stream_parse_vcenc_rate_ctrl(cJSON *object,
    bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl,
    uint8_t *changed,
    const char **field)
{
    uint32_t u32_value;
    int i32_value;
    float float_value;
    int ret;

    if (object == NULL || rate_ctrl == NULL || changed == NULL)
    {
        return 0;
    }

#define PARSE_VCENC_U32(json_name, alias_name, member_name, field_name) \
    do { \
        ret = h264e_stream_json_get_u32_alias(object, json_name, alias_name, &u32_value); \
        if (ret < 0) { *field = field_name; return -1; } \
        if (ret > 0) { rate_ctrl->member_name = u32_value; *changed = 1; } \
    } while (0)

#define PARSE_VCENC_BOOL(json_name, alias_name, member_name, field_name) \
    do { \
        ret = h264e_stream_json_get_bool_alias(object, json_name, alias_name, &u32_value); \
        if (ret < 0) { *field = field_name; return -1; } \
        if (ret > 0) { rate_ctrl->member_name = u32_value; *changed = 1; } \
    } while (0)

#define PARSE_VCENC_I32(json_name, alias_name, member_name, field_name) \
    do { \
        ret = h264e_stream_json_get_i32_alias(object, json_name, alias_name, &i32_value); \
        if (ret < 0) { *field = field_name; return -1; } \
        if (ret > 0) { rate_ctrl->member_name = i32_value; *changed = 1; } \
    } while (0)

#define PARSE_VCENC_FLOAT(json_name, alias_name, member_name, field_name) \
    do { \
        ret = h264e_stream_json_get_float_alias(object, json_name, alias_name, &float_value); \
        if (ret < 0) { *field = field_name; return -1; } \
        if (ret > 0) { rate_ctrl->member_name = float_value; *changed = 1; } \
    } while (0)

    PARSE_VCENC_I32("crf", NULL, crf, "vcencRateCtrl.crf");
    PARSE_VCENC_BOOL("pictureRc", "picture_rc", picture_rc, "vcencRateCtrl.pictureRc");
    PARSE_VCENC_U32("ctbRc", "ctb_rc", ctb_rc, "vcencRateCtrl.ctbRc");
    PARSE_VCENC_U32("blockRCSize", "block_rc_size", block_rc_size, "vcencRateCtrl.blockRCSize");
    PARSE_VCENC_BOOL("pictureSkip", "picture_skip", picture_skip, "vcencRateCtrl.pictureSkip");
    PARSE_VCENC_I32("qpHdr", "qp_hdr", qp_hdr, "vcencRateCtrl.qpHdr");
    PARSE_VCENC_U32("qpMinPB", "qp_min_pb", qp_min_pb, "vcencRateCtrl.qpMinPB");
    PARSE_VCENC_U32("qpMaxPB", "qp_max_pb", qp_max_pb, "vcencRateCtrl.qpMaxPB");
    PARSE_VCENC_U32("qpMinI", "qp_min_i", qp_min_i, "vcencRateCtrl.qpMinI");
    PARSE_VCENC_U32("qpMaxI", "qp_max_i", qp_max_i, "vcencRateCtrl.qpMaxI");
    PARSE_VCENC_U32("bitPerSecond", "bit_per_second", bit_per_second, "vcencRateCtrl.bitPerSecond");
    PARSE_VCENC_U32("cpbMaxRate", "cpb_max_rate", cpb_max_rate, "vcencRateCtrl.cpbMaxRate");
    PARSE_VCENC_BOOL("fillerData", "filler_data", filler_data, "vcencRateCtrl.fillerData");
    PARSE_VCENC_BOOL("hrd", NULL, hrd, "vcencRateCtrl.hrd");
    PARSE_VCENC_U32("hrdCpbSize", "hrd_cpb_size", hrd_cpb_size, "vcencRateCtrl.hrdCpbSize");
    PARSE_VCENC_U32("bitrateWindow", "bitrate_window", bitrate_window, "vcencRateCtrl.bitrateWindow");
    PARSE_VCENC_I32("intraQpDelta", "intra_qp_delta", intra_qp_delta, "vcencRateCtrl.intraQpDelta");
    PARSE_VCENC_U32("fixedIntraQp", "fixed_intra_qp", fixed_intra_qp, "vcencRateCtrl.fixedIntraQp");
    PARSE_VCENC_I32("bitVarRangeI", "bit_var_range_i", bit_var_range_i, "vcencRateCtrl.bitVarRangeI");
    PARSE_VCENC_I32("bitVarRangeP", "bit_var_range_p", bit_var_range_p, "vcencRateCtrl.bitVarRangeP");
    PARSE_VCENC_I32("bitVarRangeB", "bit_var_range_b", bit_var_range_b, "vcencRateCtrl.bitVarRangeB");
    PARSE_VCENC_I32("tolMovingBitRate", "tol_moving_bit_rate", tol_moving_bit_rate, "vcencRateCtrl.tolMovingBitRate");
    PARSE_VCENC_I32("monitorFrames", "monitor_frames", monitor_frames, "vcencRateCtrl.monitorFrames");
    PARSE_VCENC_I32("targetPicSize", "target_pic_size", target_pic_size, "vcencRateCtrl.targetPicSize");
    PARSE_VCENC_I32("smoothPsnrInGOP", "smooth_psnr_in_gop", smooth_psnr_in_gop, "vcencRateCtrl.smoothPsnrInGOP");
    PARSE_VCENC_U32("u32StaticSceneIbitPercent", "static_scene_i_bit_percent", static_scene_i_bit_percent, "vcencRateCtrl.u32StaticSceneIbitPercent");
    PARSE_VCENC_U32("rcQpDeltaRange", "rc_qp_delta_range", rc_qp_delta_range, "vcencRateCtrl.rcQpDeltaRange");
    PARSE_VCENC_U32("rcBaseMBComplexity", "rc_base_mb_complexity", rc_base_mb_complexity, "vcencRateCtrl.rcBaseMBComplexity");
    PARSE_VCENC_I32("picQpDeltaMin", "pic_qp_delta_min", pic_qp_delta_min, "vcencRateCtrl.picQpDeltaMin");
    PARSE_VCENC_I32("picQpDeltaMax", "pic_qp_delta_max", pic_qp_delta_max, "vcencRateCtrl.picQpDeltaMax");
    PARSE_VCENC_I32("longTermQpDelta", "long_term_qp_delta", long_term_qp_delta, "vcencRateCtrl.longTermQpDelta");
    PARSE_VCENC_BOOL("vbr", NULL, vbr, "vcencRateCtrl.vbr");
    PARSE_VCENC_U32("rcMode", "rc_mode", rc_mode, "vcencRateCtrl.rcMode");
    PARSE_VCENC_FLOAT("tolCtbRcInter", "tol_ctb_rc_inter", tol_ctb_rc_inter, "vcencRateCtrl.tolCtbRcInter");
    PARSE_VCENC_FLOAT("tolCtbRcIntra", "tol_ctb_rc_intra", tol_ctb_rc_intra, "vcencRateCtrl.tolCtbRcIntra");
    PARSE_VCENC_I32("tolRcUnderflow", "tol_rc_underflow", tol_rc_underflow, "vcencRateCtrl.tolRcUnderflow");
    PARSE_VCENC_U32("maxIprop", "max_i_prop", max_i_prop, "vcencRateCtrl.maxIprop");
    PARSE_VCENC_U32("minIprop", "min_i_prop", min_i_prop, "vcencRateCtrl.minIprop");
    PARSE_VCENC_I32("changePos", "change_pos", change_pos, "vcencRateCtrl.changePos");
    PARSE_VCENC_I32("ctbRcRowQpStep", "ctb_rc_row_qp_step", ctb_rc_row_qp_step, "vcencRateCtrl.ctbRcRowQpStep");
    PARSE_VCENC_I32("ctbRcRowQpDeltaRange", "ctb_rc_row_qp_delta_range", ctb_rc_row_qp_delta_range, "vcencRateCtrl.ctbRcRowQpDeltaRange");
    PARSE_VCENC_BOOL("ctbRcQpDeltaReverse", "ctb_rc_qp_delta_reverse", ctb_rc_qp_delta_reverse, "vcencRateCtrl.ctbRcQpDeltaReverse");
    PARSE_VCENC_U32("frameRateNum", "frame_rate_num", frame_rate_num, "vcencRateCtrl.frameRateNum");
    PARSE_VCENC_U32("frameRateDenom", "frame_rate_denom", frame_rate_denom, "vcencRateCtrl.frameRateDenom");
    PARSE_VCENC_BOOL("hieQpDeltaEnable", "hie_qp_delta_enable", hie_qp_delta_enable, "vcencRateCtrl.hieQpDeltaEnable");

#undef PARSE_VCENC_U32
#undef PARSE_VCENC_BOOL
#undef PARSE_VCENC_I32
#undef PARSE_VCENC_FLOAT

    if (h264e_stream_validate_vcenc_rate_ctrl(rate_ctrl) != 0)
    {
        *field = "vcencRateCtrl";
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

static void h264e_stream_sync_vcenc_rate_ctrl_from_encoder(h264e_stream_session_ctx_t *ctx,
    bk_h264_encode_vcenc_rate_ctrl_t *rate_ctrl)
{
    if (ctx != NULL && ctx->encoder != NULL &&
        h264e_stream_encode_get_vcenc_rate_ctrl(ctx->encoder, rate_ctrl) == AVDK_ERR_OK)
    {
        if (rate_ctrl->crf < 0 && s_encoder_config.vcenc_rate_ctrl.crf >= 0)
        {
            rate_ctrl->crf = s_encoder_config.vcenc_rate_ctrl.crf;
        }
        s_encoder_config.vcenc_rate_ctrl = *rate_ctrl;
        h264e_stream_rate_ctrl_from_vcenc(rate_ctrl, &s_encoder_config.rate_ctrl);
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
    h264e_stream_vcenc_from_rate_ctrl(&rate_ctrl, &s_encoder_config.vcenc_rate_ctrl);
    s_encoder_config.vcenc_rate_ctrl_valid = 0;
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
    if (s_encoder_config.vcenc_rate_ctrl_valid)
    {
        if (h264e_stream_encode_set_vcenc_rate_ctrl(ctx->encoder, &s_encoder_config.vcenc_rate_ctrl) != AVDK_ERR_OK)
        {
            return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "set_vcenc_rate_ctrl_failed");
        }
    }
    else if (bk_h264_encode_set_rate_ctrl(ctx->encoder, &s_encoder_config.rate_ctrl) != AVDK_ERR_OK)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "set_rate_ctrl_failed");
    }
    if (s_encoder_config.gop_frame_count > 0)
    {
        (void)bk_h264_encode_set_gop_frame_count(ctx->encoder, s_encoder_config.gop_frame_count);
    }
    if (bk_h264_encode_start(ctx->encoder) != AVDK_ERR_OK)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "start_encode_failed");
    }
    if (ctx->config.auto_force_idr)
    {
        (void)bk_h264_encode_force_idr(ctx->encoder);
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

bk_err_t h264e_stream_session_get_video_config(uint16_t *width,
                                               uint16_t *height,
                                               uint16_t *fps,
                                               uint32_t *gop_frame_count)
{
    if (width != NULL)
    {
        *width = s_encoder_config.width;
    }
    if (height != NULL)
    {
        *height = s_encoder_config.height;
    }
    if (fps != NULL)
    {
        *fps = s_encoder_config.fps;
    }
    if (gop_frame_count != NULL)
    {
        *gop_frame_count = s_encoder_config.gop_frame_count;
    }
    return BK_OK;
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

static int h264e_stream_config_to_result(char *buffer, size_t buffer_len)
{
    bk_h264_encode_rate_ctrl_t *rc = &s_encoder_config.rate_ctrl;
    char vcenc_json[4096];
    int vcenc_len = h264e_stream_vcenc_rate_ctrl_to_json(&s_encoder_config.vcenc_rate_ctrl,
                                                         vcenc_json,
                                                         sizeof(vcenc_json));

    if (vcenc_len < 0 || vcenc_len >= (int)sizeof(vcenc_json))
    {
        return -1;
    }

    return snprintf(buffer, buffer_len,
        "{\"values\":{\"mode\":\"%s\",\"width\":%u,\"height\":%u,"
        "\"fps\":%u,\"bitrateKbps\":%u,\"gopFrameCount\":%u,"
        "\"bitrate\":%u,\"qpMinI\":%u,\"qpMaxI\":%u,"
        "\"qpMinP\":%u,\"qpMaxP\":%u,\"forceIdr\":%s},"
        "\"config\":{\"mode\":\"%s\",\"video\":{\"width\":%u,\"height\":%u,"
        "\"fps\":%u,\"bitrateKbps\":%u,\"gopFrameCount\":%u},"
        "\"rateCtrl\":{\"bitrate\":%u,\"qpMinI\":%u,"
        "\"qpMaxI\":%u,\"qpMinP\":%u,\"qpMaxP\":%u},%s,"
        "\"ctrl\":{\"forceIdr\":%s}}}",
        s_encoder_config.mode, s_encoder_config.width,
        s_encoder_config.height, s_encoder_config.fps, s_encoder_config.bitrate_kbps,
        s_encoder_config.gop_frame_count,
        rc->bitrate, rc->qp_min_i, rc->qp_max_i, rc->qp_min_p, rc->qp_max_p,
        s_encoder_config.force_idr ? "true" : "false",
        s_encoder_config.mode, s_encoder_config.width,
        s_encoder_config.height, s_encoder_config.fps, s_encoder_config.bitrate_kbps,
        s_encoder_config.gop_frame_count,
        rc->bitrate, rc->qp_min_i, rc->qp_max_i, rc->qp_min_p, rc->qp_max_p,
        vcenc_json,
        s_encoder_config.force_idr ? "true" : "false");
}

static int h264e_stream_handle_get_config(const char *id_json)
{
    bk_h264_encode_rate_ctrl_t rate_ctrl = s_encoder_config.rate_ctrl;
    bk_h264_encode_vcenc_rate_ctrl_t vcenc_rate_ctrl = s_encoder_config.vcenc_rate_ctrl;
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();
    uint32_t gop_frame_count;
    int len;

    h264e_stream_sync_rate_ctrl_from_encoder(ctx, &rate_ctrl);
    h264e_stream_sync_vcenc_rate_ctrl_from_encoder(ctx, &vcenc_rate_ctrl);
    if (ctx != NULL && ctx->encoder != NULL &&
        bk_h264_encode_get_gop_frame_count(ctx->encoder, &gop_frame_count) == AVDK_ERR_OK)
    {
        s_encoder_config.gop_frame_count = gop_frame_count;
    }

    len = h264e_stream_config_to_result(s_h264e_stream_result_buffer, sizeof(s_h264e_stream_result_buffer));
    if (len < 0 || len >= (int)sizeof(s_h264e_stream_result_buffer))
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INTERNAL_ERROR, NULL, "config_too_large");
    }
    return h264e_stream_jsonrpc_send_result(id_json, s_h264e_stream_result_buffer);
}

static int h264e_stream_parse_video_config(cJSON *object,
    h264e_stream_encoder_config_t *config,
    const char **field)
{
    uint32_t value;
    int ret;

    if (object == NULL)
    {
        return 0;
    }

    ret = h264e_stream_json_get_u32_alias(object, "width", NULL, &value);
    if (ret < 0 || (ret > 0 && (value < 320 || value > 2304)))
    {
        *field = "video.width";
        return -1;
    }
    if (ret > 0)
    {
        config->width = (uint16_t)value;
    }

    ret = h264e_stream_json_get_u32_alias(object, "height", NULL, &value);
    if (ret < 0 || (ret > 0 && (value < 240 || value > 1296)))
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
    ret = h264e_stream_json_get_u32_alias(object, "gopFrameCount", "gop_frame_count", &value);
    if (ret == 0)
    {
        ret = h264e_stream_json_get_u32_alias(object, "idrInterval", "idr_interval", &value);
    }
    if (ret < 0 || (ret > 0 && (value < 1 || value > 300)))
    {
        *field = "video.gopFrameCount";
        return -1;
    }
    if (ret > 0)
    {
        config->gop_frame_count = value;
    }
    return 0;
}

static int h264e_stream_handle_set_config(cJSON *params, const char *id_json)
{
    h264e_stream_encoder_config_t next = s_encoder_config;
    bk_h264_encode_rate_ctrl_t rate_ctrl = s_encoder_config.rate_ctrl;
    bk_h264_encode_vcenc_rate_ctrl_t vcenc_rate_ctrl = s_encoder_config.vcenc_rate_ctrl;
    h264e_stream_session_ctx_t *ctx = h264e_stream_session_get_ctx();
    cJSON *mode;
    cJSON *video;
    cJSON *rate_ctrl_json;
    cJSON *vcenc_rate_ctrl_json;
    cJSON *ctrl;
    uint8_t changed = 0;
    uint8_t vcenc_changed = 0;
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
    if (ctx->encoder != NULL &&
        (next.width != s_encoder_config.width ||
         next.height != s_encoder_config.height ||
         next.fps != s_encoder_config.fps))
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_STATE,
                                               "video", "video change requires stop/start encode");
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
    h264e_stream_sync_vcenc_rate_ctrl_from_encoder(ctx, &vcenc_rate_ctrl);
    if (h264e_stream_parse_rate_ctrl(rate_ctrl_json, &rate_ctrl, &changed, &field) != 0)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_PARAMS, field, "invalid rateCtrl");
    }
    if (changed)
    {
        h264e_stream_vcenc_from_rate_ctrl(&rate_ctrl, &vcenc_rate_ctrl);
    }

    vcenc_rate_ctrl_json = h264e_stream_json_get_alias(params, "vcencRateCtrl", "vcenc_rate_ctrl");
    if (h264e_stream_parse_vcenc_rate_ctrl(vcenc_rate_ctrl_json, &vcenc_rate_ctrl, &vcenc_changed, &field) != 0)
    {
        return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_INVALID_PARAMS, field, "invalid vcencRateCtrl");
    }
    if (vcenc_changed)
    {
        h264e_stream_rate_ctrl_from_vcenc(&vcenc_rate_ctrl, &rate_ctrl);
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

    if (ctx->encoder != NULL && next.gop_frame_count != s_encoder_config.gop_frame_count)
    {
        if (bk_h264_encode_set_gop_frame_count(ctx->encoder, next.gop_frame_count) != AVDK_ERR_OK)
        {
            return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "set_gop_failed");
        }
    }

    if (vcenc_changed && ctx->encoder != NULL)
    {
        if (h264e_stream_encode_set_vcenc_rate_ctrl(ctx->encoder, &vcenc_rate_ctrl) != AVDK_ERR_OK)
        {
            return h264e_stream_jsonrpc_send_error(id_json, H264E_STREAM_JSONRPC_SDK_FAILED, NULL, "set_vcenc_rate_ctrl_failed");
        }
        if (next.force_idr || ctx->config.auto_force_idr)
        {
            (void)bk_h264_encode_force_idr(ctx->encoder);
        }
    }
    else if (changed && ctx->encoder != NULL)
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
        next.vcenc_rate_ctrl = vcenc_rate_ctrl;
        next.vcenc_rate_ctrl_valid = 0;
    }
    if (vcenc_changed)
    {
        next.vcenc_rate_ctrl = vcenc_rate_ctrl;
        next.vcenc_rate_ctrl_valid = 1;
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
        strcmp(method->valuestring, "h264e.getParameterSchema") == 0 ||
        strcmp(method->valuestring, "encode_preview.getParameterSchema") == 0)
    {
        ret = h264e_stream_handle_get_config_schema(id_json);
    }
    else if (strcmp(method->valuestring, "get_config") == 0 ||
             strcmp(method->valuestring, "h264EScream.getConfig") == 0 ||
             strcmp(method->valuestring, "h264e.getConfig") == 0 ||
             strcmp(method->valuestring, "encode_preview.getConfig") == 0)
    {
        ret = h264e_stream_handle_get_config(id_json);
    }
    else if (strcmp(method->valuestring, "set_config") == 0 ||
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
    else if (strcmp(method->valuestring, "h264EScream.turnOff") == 0 ||
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