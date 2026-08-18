/**
 * @file bk_a2dp_source_service.c
 *
 * A2DP source service layer.
 *
 * PCM feeding uses an internal ring buffer (scheme A): the app owns file read
 * + decode and calls bk_a2dp_source_service_write_pcm(); this module owns the
 * ring buffer, SBC encoder, resample worker and the three stack callbacks
 * (data / resample / encode) plus their registration.
 *
 * P2 scope: pipeline (ring buffer + callbacks + SBC + resample). The A2DP
 * connection state machine and avdtp media start/suspend still live in the demo
 * (migrated in P3); the demo calls bk_a2dp_source_service_set_codec_cfg() with
 * the negotiated SBC params it receives from the AUDIO_SOURCE_CFG event, then
 * bk_a2dp_source_service_media_start() to arm the pipeline, and finally starts
 * the avdtp stream itself.
 */

#include <os/os.h>
#include <os/mem.h>
#include <components/log.h>

#include <modules/sbc_encoder.h>
#if CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC
#include <modules/fdk_aac_enc/aacenc_lib.h>
#endif
#include <modules/audio_rsp_types.h>

#include "components/bluetooth/bk_dm_a2dp.h"
#include "bk_a2dp_source_service.h"
#include "bk_a2dp_source_pcm_service.h"
#include "ring_buffer_particle.h"

#define TAG "a2dp_src_svc"

enum
{
    A2DP_SRC_SRV_DEBUG_LEVEL_ERROR,
    A2DP_SRC_SRV_DEBUG_LEVEL_WARNING,
    A2DP_SRC_SRV_DEBUG_LEVEL_INFO,
    A2DP_SRC_SRV_DEBUG_LEVEL_DEBUG,
    A2DP_SRC_SRV_DEBUG_LEVEL_VERBOSE,
};

#define A2DP_SRC_SRV_DEBUG_LEVEL A2DP_SRC_SRV_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(A2DP_SRC_SRV_DEBUG_LEVEL >= A2DP_SRC_SRV_DEBUG_LEVEL_ERROR)   BK_LOGE(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(A2DP_SRC_SRV_DEBUG_LEVEL >= A2DP_SRC_SRV_DEBUG_LEVEL_WARNING) BK_LOGW(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(A2DP_SRC_SRV_DEBUG_LEVEL >= A2DP_SRC_SRV_DEBUG_LEVEL_INFO)    BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(A2DP_SRC_SRV_DEBUG_LEVEL >= A2DP_SRC_SRV_DEBUG_LEVEL_DEBUG)   BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(A2DP_SRC_SRV_DEBUG_LEVEL >= A2DP_SRC_SRV_DEBUG_LEVEL_VERBOSE) BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

#define SBC_SAMPLE_DEPTH        16
#define DECODE_TRIGGER_TIME     50          /* ms of PCM to buffer before back-pressure */
#define RB_HEADROOM_SIZE        (16 * 1024) /* bytes above trigger for one decoded frame */

#if CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC
#define AAC_OUT_BUF_SIZE        2048    /* max one AAC-LC AU (1024 samples stereo) + margin */
#endif

/* All file-scope state, grouped into one context. */
typedef struct
{
    /* app event callback */
    bk_a2dp_source_service_event_cb_t event_cb;
    void                             *event_user_data;

    /* pipeline state */
    ring_buffer_particle_ctx rb_ctx;
    beken_semaphore_t        produce_sema;         /* set by data_cb after it consumes PCM */
    uint32_t                 trigger_size;
    SbcEncoderContext        sbc_ctx;
#if CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC
    HANDLE_AACENCODER        aac_enc_handle;
    uint8_t                 *aac_out_buf;
#endif
    uint8_t                  rsp_inited;
    volatile uint8_t         running;
    volatile uint8_t         pause;

    /* formats */
    uint32_t                 src_rate;
    uint8_t                  src_ch;
    uint8_t                  src_bits;
    bk_a2dp_mcc_t            sbc_cap;              /* negotiated SBC params (from AUDIO_SOURCE_CFG) */
    uint8_t                  cap_valid;

    /* connection state machine (P3) */
    uint8_t                  profile_inited;       /* bk_bt_a2dp_source_init done */
    uint8_t                  conn_state;           /* bk_a2dp_connection_state_t */
    uint8_t                  start_status;         /* avdtp stream started */
    uint8_t                  local_action_pending; /* local media-ctrl waiting for AUDIO_STATE */
    uint8_t                  peer_bda[6];
    uint32_t                 mtu;
    beken_semaphore_t        api_sema;             /* sync prof-state / connect / audio-cfg / media-ctrl */
} a2dp_source_service_ctx_t;

static a2dp_source_service_ctx_t s_a2dp_source_service_ctx;

/* Emit a service event to the app (helper for P3). */
static inline void source_emit(bk_a2dp_source_service_evt_t evt, void *arg)
{
    if (s_a2dp_source_service_ctx.event_cb)
    {
        s_a2dp_source_service_ctx.event_cb(evt, arg, s_a2dp_source_service_ctx.event_user_data);
    }
}

/* ====================================================================== */
/* A2DP profile event callback (connection state machine)                 */
/* ====================================================================== */

static void source_a2dp_cb(bk_a2dp_cb_event_t event, bk_a2dp_cb_param_t *p_param)
{
    switch (event)
    {
    case BK_A2DP_PROF_STATE_EVT:
        LOGI("a2dp prof action %d status %d", p_param->a2dp_prof_stat.action, p_param->a2dp_prof_stat.status);
        if (!p_param->a2dp_prof_stat.status && s_a2dp_source_service_ctx.api_sema)
        {
            rtos_set_semaphore(&s_a2dp_source_service_ctx.api_sema);
        }
        break;

    case BK_A2DP_CONNECTION_STATE_EVT:
    {
        uint8_t status = p_param->conn_state.state;
        uint8_t *bda = p_param->conn_state.remote_bda;
        LOGI("conn state %d [%02x:%02x:%02x:%02x:%02x:%02x]",
             status, bda[5], bda[4], bda[3], bda[2], bda[1], bda[0]);

        if (s_a2dp_source_service_ctx.conn_state != status)
        {
            s_a2dp_source_service_ctx.conn_state = status;
            if (status == BK_A2DP_CONNECTION_STATE_CONNECTED ||
                status == BK_A2DP_CONNECTION_STATE_DISCONNECTED)
            {
                if (s_a2dp_source_service_ctx.api_sema)
                {
                    rtos_set_semaphore(&s_a2dp_source_service_ctx.api_sema);
                }
            }
        }

        if (status == BK_A2DP_CONNECTION_STATE_CONNECTED)
        {
            os_memcpy(s_a2dp_source_service_ctx.peer_bda, bda, sizeof(s_a2dp_source_service_ctx.peer_bda));
            source_emit(BK_A2DP_SOURCE_SERVICE_EVT_CONNECTED, s_a2dp_source_service_ctx.peer_bda);
        }
        else if (status == BK_A2DP_CONNECTION_STATE_DISCONNECTED)
        {
            s_a2dp_source_service_ctx.start_status = 0;
            s_a2dp_source_service_ctx.mtu = 0;
            source_emit(BK_A2DP_SOURCE_SERVICE_EVT_DISCONNECTED, s_a2dp_source_service_ctx.peer_bda);
        }
    }
    break;

    case BK_A2DP_AUDIO_STATE_EVT:
        LOGI("audio state %d", p_param->audio_state.state);
        if (p_param->audio_state.state == BK_A2DP_AUDIO_STATE_STARTED)
        {
            if (s_a2dp_source_service_ctx.start_status == 0)
            {
                s_a2dp_source_service_ctx.start_status = 1;
                source_emit(BK_A2DP_SOURCE_SERVICE_EVT_STREAM_START, NULL);
                if (s_a2dp_source_service_ctx.api_sema && s_a2dp_source_service_ctx.local_action_pending)
                {
                    rtos_set_semaphore(&s_a2dp_source_service_ctx.api_sema);
                }
            }
        }
        else if (p_param->audio_state.state == BK_A2DP_AUDIO_STATE_SUSPEND)
        {
            if (s_a2dp_source_service_ctx.start_status == 1)
            {
                s_a2dp_source_service_ctx.start_status = 0;
                source_emit(BK_A2DP_SOURCE_SERVICE_EVT_STREAM_SUSPEND, NULL);
                if (s_a2dp_source_service_ctx.api_sema && s_a2dp_source_service_ctx.local_action_pending)
                {
                    rtos_set_semaphore(&s_a2dp_source_service_ctx.api_sema);
                }
            }
        }
        break;

    case BK_A2DP_AUDIO_SOURCE_CFG_EVT:
        os_memcpy(&s_a2dp_source_service_ctx.sbc_cap, &p_param->audio_source_cfg.mcc, sizeof(s_a2dp_source_service_ctx.sbc_cap));
        s_a2dp_source_service_ctx.cap_valid = 1;
        s_a2dp_source_service_ctx.mtu = p_param->audio_source_cfg.mtu;
        LOGI("audio cfg ch %d rate %d mtu %d",
             s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.channels, s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.sample_rate, s_a2dp_source_service_ctx.mtu);
        source_emit(BK_A2DP_SOURCE_SERVICE_EVT_AUDIO_CFG, &s_a2dp_source_service_ctx.sbc_cap);
        if (s_a2dp_source_service_ctx.api_sema)
        {
            rtos_set_semaphore(&s_a2dp_source_service_ctx.api_sema);
        }
        break;

    default:
        LOGD("unhandled a2dp event %d", event);
        break;
    }
}

/* ====================================================================== */
/* Stack callbacks (registered with the SDK a2dp source)                  */
/* ====================================================================== */

static int32_t source_data_cb(uint8_t *buf, int32_t len)
{
    uint32_t read_len = 0;

    if (!s_a2dp_source_service_ctx.running || s_a2dp_source_service_ctx.pause)
    {
        return 0;
    }

    if (ring_buffer_particle_len(&s_a2dp_source_service_ctx.rb_ctx) < (uint32_t)len)
    {
        LOGE("ring buffer not enough data %d < %d", ring_buffer_particle_len(&s_a2dp_source_service_ctx.rb_ctx), (int)len);
    }
    else
    {
        ring_buffer_particle_read(&s_a2dp_source_service_ctx.rb_ctx, buf, len, &read_len);
    }

    if (s_a2dp_source_service_ctx.produce_sema)
    {
        rtos_set_semaphore(&s_a2dp_source_service_ctx.produce_sema);
    }

    return read_len;
}

static int32_t source_encode_cb(uint8_t type, uint8_t *in_addr, uint32_t *in_len, uint8_t *out_addr, uint32_t *out_len)
{
    int32_t encode_len = 0;
    bt_audio_encode_req_t req = {0};
    int ret = 0;

    req.in_addr = in_addr;
    req.type = type;
    req.out_len_ptr = (typeof(req.out_len_ptr))&encode_len;

    if (type == 0)
    {
        req.handle = &s_a2dp_source_service_ctx.sbc_ctx;
    }
#if CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC
    else if (type == BK_A2DP_CODEC_TYPE_AAC)
    {
        req.handle = s_a2dp_source_service_ctx.aac_enc_handle;
        req.in_bytes = (in_len ? *in_len : 0);
        req.out_addr = s_a2dp_source_service_ctx.aac_out_buf;
        req.out_buf_size = AAC_OUT_BUF_SIZE;
    }
#endif
    else
    {
        LOGE("type not match %d", type);
        return -1;
    }

    ret = bk_a2dp_source_pcm_service_encode_req(&req);

    if (ret)
    {
        LOGE("encode req err %d !!", ret);
        return -1;
    }

    if (encode_len > (int32_t)*out_len)
    {
        LOGE("encode len %d > out_len %d", encode_len, *out_len);
        return -1;
    }

#if CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC
    if (type == BK_A2DP_CODEC_TYPE_AAC)
    {
        /* AAC: numOutBytes can be 0 for the first frames (encoder priming) */
        if (encode_len > 0)
        {
            os_memcpy(out_addr, s_a2dp_source_service_ctx.aac_out_buf, encode_len);
        }
        *out_len = encode_len;
        return 0;
    }
#endif

    if (!encode_len)
    {
        LOGE("encode err %d", encode_len);
        return -1;
    }

    os_memcpy(out_addr, s_a2dp_source_service_ctx.sbc_ctx.stream, encode_len);
    *out_len = encode_len;

    return 0;
}

static int32_t source_resample_cb(uint8_t *in_addr, uint32_t *in_len, uint8_t *out_addr, uint32_t *out_len)
{
    int32_t ret = 0;
    uint32_t input_len = *in_len;
    uint32_t output_len = *out_len;
    bt_audio_resample_req_t req = {0};

    if (!s_a2dp_source_service_ctx.rsp_inited)
    {
        LOGE("resample not init");
        return -1;
    }

    req.in_addr = in_addr;
    req.out_addr = out_addr;
    req.in_bytes_ptr = &input_len;
    req.out_bytes_ptr = &output_len;

    ret = bk_a2dp_source_pcm_service_rsp_req(&req);

    if (ret)
    {
        LOGE("rsp req err %d !!", ret);
        return -1;
    }

    *in_len = input_len;
    *out_len = output_len;

    return ret;
}

#if CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC
/* ====================================================================== */
/* AAC encoder (FDK)                                                      */
/* ====================================================================== */

static int source_aac_encoder_init(void)
{
    uint8_t  channels    = s_a2dp_source_service_ctx.sbc_cap.cie.aac_codec.channels ? s_a2dp_source_service_ctx.sbc_cap.cie.aac_codec.channels : 2;
    uint32_t sample_rate = s_a2dp_source_service_ctx.sbc_cap.cie.aac_codec.sample_rate ? s_a2dp_source_service_ctx.sbc_cap.cie.aac_codec.sample_rate : 44100;
    CHANNEL_MODE mode    = (channels == 2) ? MODE_2 : MODE_1;
    uint32_t bitrate     = (channels == 2) ? 128000 : 96000;

    if (s_a2dp_source_service_ctx.aac_enc_handle)
    {
        aacEncClose(&s_a2dp_source_service_ctx.aac_enc_handle);
        s_a2dp_source_service_ctx.aac_enc_handle = NULL;
    }

    if (aacEncOpen(&s_a2dp_source_service_ctx.aac_enc_handle, 0x01, channels) != AACENC_OK)
    {
        LOGE("aacEncOpen fail");
        return -1;
    }

    if (aacEncoder_SetParam(s_a2dp_source_service_ctx.aac_enc_handle, AACENC_AOT, AOT_AAC_LC) != AACENC_OK) goto fail;
    if (aacEncoder_SetParam(s_a2dp_source_service_ctx.aac_enc_handle, AACENC_SAMPLERATE, sample_rate) != AACENC_OK) goto fail;
    if (aacEncoder_SetParam(s_a2dp_source_service_ctx.aac_enc_handle, AACENC_CHANNELMODE, mode) != AACENC_OK) goto fail;
    if (aacEncoder_SetParam(s_a2dp_source_service_ctx.aac_enc_handle, AACENC_CHANNELORDER, 1) != AACENC_OK) goto fail;
    if (aacEncoder_SetParam(s_a2dp_source_service_ctx.aac_enc_handle, AACENC_BITRATE, bitrate) != AACENC_OK) goto fail;
    /* A2DP carries raw AAC-LC Access Units over RTP: no ADTS/LATM framing */
    if (aacEncoder_SetParam(s_a2dp_source_service_ctx.aac_enc_handle, AACENC_TRANSMUX, TT_MP4_RAW) != AACENC_OK) goto fail;
    if (aacEncoder_SetParam(s_a2dp_source_service_ctx.aac_enc_handle, AACENC_AFTERBURNER, 0) != AACENC_OK) goto fail;

    /* Encoder initialization call (NULL bufs) */
    if (aacEncEncode(s_a2dp_source_service_ctx.aac_enc_handle, NULL, NULL, NULL, NULL) != AACENC_OK) goto fail;

    if (!s_a2dp_source_service_ctx.aac_out_buf)
    {
        s_a2dp_source_service_ctx.aac_out_buf = os_malloc(AAC_OUT_BUF_SIZE);
        if (!s_a2dp_source_service_ctx.aac_out_buf)
        {
            LOGE("aac out buf alloc fail");
            goto fail;
        }
    }

    LOGI("aac encoder init ok: ch %d rate %d br %d", channels, sample_rate, bitrate);
    return 0;

fail:
    if (s_a2dp_source_service_ctx.aac_enc_handle)
    {
        aacEncClose(&s_a2dp_source_service_ctx.aac_enc_handle);
        s_a2dp_source_service_ctx.aac_enc_handle = NULL;
    }
    LOGE("aac encoder init fail");
    return -1;
}
#endif /* CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC */

/* ====================================================================== */
/* SBC encoder                                                            */
/* ====================================================================== */

static int source_sbc_encoder_init(void)
{
    bt_err_t ret = 0;
    uint8_t alloc_mode = 0;
    uint8_t block_mode = 3;
    uint8_t channle_mode = 3;
    uint8_t samp_rate_select = 2;
    uint8_t subband = 1;

    os_memset(&s_a2dp_source_service_ctx.sbc_ctx, 0, sizeof(s_a2dp_source_service_ctx.sbc_ctx));

    sbc_encoder_init(&s_a2dp_source_service_ctx.sbc_ctx, s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.sample_rate, 1);

    switch (s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.alloc_mode)
    {
    case 2: alloc_mode = 1; break;                 /* SNR */
    default: case 1: alloc_mode = 0; break;        /* LOUDNESS */
    }
    ret = sbc_encoder_ctrl(&s_a2dp_source_service_ctx.sbc_ctx, SBC_ENCODER_CTRL_CMD_SET_ALLOCATION_METHOD, alloc_mode);
    if (ret != SBC_ENCODER_ERROR_OK) { LOGE("SET_ALLOCATION_METHOD err %d", ret); return ret; }

    ret = sbc_encoder_ctrl(&s_a2dp_source_service_ctx.sbc_ctx, SBC_ENCODER_CTRL_CMD_SET_BITPOOL, s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.bit_pool);
    if (ret != SBC_ENCODER_ERROR_OK) { LOGE("SET_BITPOOL err %d", ret); return ret; }

    switch (s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.block_len)
    {
    case 4: block_mode = 0; break;
    case 8: block_mode = 1; break;
    case 12: block_mode = 2; break;
    default: case 16: block_mode = 3; break;
    }
    ret = sbc_encoder_ctrl(&s_a2dp_source_service_ctx.sbc_ctx, SBC_ENCODER_CTRL_CMD_SET_BLOCK_MODE, block_mode);
    if (ret != SBC_ENCODER_ERROR_OK) { LOGE("SET_BLOCK_MODE err %d", ret); return ret; }

    switch (s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.channel_mode)
    {
    case 8: channle_mode = 0; break;
    case 4: channle_mode = 1; break;
    case 2: channle_mode = 2; break;
    default: case 1: channle_mode = 3; break;
    }
    ret = sbc_encoder_ctrl(&s_a2dp_source_service_ctx.sbc_ctx, SBC_ENCODER_CTRL_CMD_SET_CHANNEL_MODE, channle_mode);
    if (ret != SBC_ENCODER_ERROR_OK) { LOGE("SET_CHANNEL_MODE err %d", ret); return ret; }

    switch (s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.sample_rate)
    {
    case 16000: samp_rate_select = 0; break;
    case 32000: samp_rate_select = 1; break;
    default: case 44100: samp_rate_select = 2; break;
    case 48000: samp_rate_select = 3; break;
    }
    ret = sbc_encoder_ctrl(&s_a2dp_source_service_ctx.sbc_ctx, SBC_ENCODER_CTRL_CMD_SET_SAMPLE_RATE_INDEX, samp_rate_select);
    if (ret != SBC_ENCODER_ERROR_OK) { LOGE("SET_SAMPLE_RATE_INDEX err %d", ret); return ret; }

    switch (s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.subbands)
    {
    case 4: subband = 0; break;
    default: case 8: subband = 1; break;
    }
    ret = sbc_encoder_ctrl(&s_a2dp_source_service_ctx.sbc_ctx, SBC_ENCODER_CTRL_CMD_SET_SUBBAND_MODE, subband);
    if (ret != SBC_ENCODER_ERROR_OK) { LOGE("SET_SUBBAND_MODE err %d", ret); return ret; }

    ret = bk_a2dp_source_pcm_service_encode_init_req(NULL, 1);
    if (ret) { LOGE("encode init req err %d !!", ret); return -1; }

    LOGI("sbc encode count %d", s_a2dp_source_service_ctx.sbc_ctx.pcm_length);
    return 0;
}

static void source_sbc_encoder_deinit(void)
{
    bk_a2dp_source_pcm_service_encode_init_req(NULL, 0);
    os_memset(&s_a2dp_source_service_ctx.sbc_ctx, 0, sizeof(s_a2dp_source_service_ctx.sbc_ctx));
}

/* ====================================================================== */
/* Public API                                                             */
/* ====================================================================== */

int bk_a2dp_source_service_register_event_cb(bk_a2dp_source_service_event_cb_t cb, void *user_data)
{
    s_a2dp_source_service_ctx.event_cb = cb;
    s_a2dp_source_service_ctx.event_user_data = user_data;
    return 0;
}

int bk_a2dp_source_service_init(const bk_a2dp_source_service_cfg_t *cfg)
{
    int err;

    (void)cfg;

    if (s_a2dp_source_service_ctx.profile_inited)
    {
        return 0;
    }

    if (!s_a2dp_source_service_ctx.api_sema)
    {
        err = rtos_init_semaphore(&s_a2dp_source_service_ctx.api_sema, 1);
        if (err) { LOGE("api sema init err %d", err); return -1; }
    }

    bk_bt_a2dp_register_callback(source_a2dp_cb);

    err = bk_bt_a2dp_source_init();
    if (err) { LOGE("a2dp source init err %d", err); return -1; }

    err = rtos_get_semaphore(&s_a2dp_source_service_ctx.api_sema, 6000);  /* BK_A2DP_PROF_STATE_EVT */
    if (err) { LOGE("wait prof state err %d", err); return -1; }

    s_a2dp_source_service_ctx.profile_inited = 1;
    LOGI("inited");
    return 0;
}

int bk_a2dp_source_service_deinit(void)
{
    if (s_a2dp_source_service_ctx.profile_inited)
    {
        bk_bt_a2dp_source_deinit();
        rtos_get_semaphore(&s_a2dp_source_service_ctx.api_sema, 6000);  /* BK_A2DP_PROF_STATE_EVT */
        s_a2dp_source_service_ctx.profile_inited = 0;
    }

    bk_bt_a2dp_register_callback(NULL);

    if (s_a2dp_source_service_ctx.api_sema)
    {
        rtos_deinit_semaphore(&s_a2dp_source_service_ctx.api_sema);
        s_a2dp_source_service_ctx.api_sema = NULL;
    }

    s_a2dp_source_service_ctx.conn_state = 0;
    s_a2dp_source_service_ctx.start_status = 0;
    s_a2dp_source_service_ctx.cap_valid = 0;
    return 0;
}

int bk_a2dp_source_service_connect(const uint8_t bda[6])
{
    int err;

    if (!bda)
    {
        return -1;
    }

    if (!s_a2dp_source_service_ctx.profile_inited)
    {
        err = bk_a2dp_source_service_init(NULL);
        if (err) { return err; }
    }

    if (s_a2dp_source_service_ctx.conn_state == BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        LOGW("already connected");
        return 0;
    }
    if (s_a2dp_source_service_ctx.conn_state != BK_A2DP_CONNECTION_STATE_DISCONNECTED)
    {
        LOGE("not idle, disconnect first");
        return -1;
    }

    os_memcpy(s_a2dp_source_service_ctx.peer_bda, bda, sizeof(s_a2dp_source_service_ctx.peer_bda));

    err = bk_bt_a2dp_source_connect((uint8_t *)bda);
    if (err) { LOGE("connect err %d", err); return -1; }

    err = rtos_get_semaphore(&s_a2dp_source_service_ctx.api_sema, 12000);  /* CONNECTED */
    if (err) { LOGE("wait connect err"); return -1; }

    err = rtos_get_semaphore(&s_a2dp_source_service_ctx.api_sema, 6000);   /* AUDIO_SOURCE_CFG */
    if (err) { LOGE("wait cap err"); return -1; }

    LOGI("connect complete");
    return 0;
}

/* Non-blocking connect: kick off the A2DP source connection and return
 * immediately; CONNECTED / AUDIO_CFG arrive later via the registered event
 * callback. Use this from contexts that MUST NOT block (e.g. a timer handler),
 * otherwise the blocking waits above stall that thread -- and if it is the
 * timer service thread, the BT stack's own software timers stall too, which
 * stalls the media-channel L2CAP setup. */
int bk_a2dp_source_service_connect_async(const uint8_t bda[6])
{
    int err;

    if (!bda)
    {
        return -1;
    }

    if (!s_a2dp_source_service_ctx.profile_inited)
    {
        err = bk_a2dp_source_service_init(NULL);
        if (err) { return err; }
    }

    if (s_a2dp_source_service_ctx.conn_state == BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        LOGW("already connected");
        return 0;
    }
    if (s_a2dp_source_service_ctx.conn_state != BK_A2DP_CONNECTION_STATE_DISCONNECTED)
    {
        LOGE("not idle, disconnect first");
        return -1;
    }

    os_memcpy(s_a2dp_source_service_ctx.peer_bda, bda, sizeof(s_a2dp_source_service_ctx.peer_bda));

    err = bk_bt_a2dp_source_connect((uint8_t *)bda);
    if (err) { LOGE("connect err %d", err); return -1; }

    return 0;
}

int bk_a2dp_source_service_disconnect(void)
{
    int err;

    if (s_a2dp_source_service_ctx.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        LOGW("not connected");
        return 0;
    }

    err = bk_bt_a2dp_source_disconnect(s_a2dp_source_service_ctx.peer_bda);
    if (err) { LOGE("disconnect err %d", err); return -1; }

    err = rtos_get_semaphore(&s_a2dp_source_service_ctx.api_sema, 6000);   /* DISCONNECTED */
    if (err) { LOGE("wait disconnect err"); return -1; }

    LOGI("disconnect complete");
    return 0;
}

int bk_a2dp_source_service_stream_start(void)
{
    int err;

    if (s_a2dp_source_service_ctx.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        LOGE("not connected");
        return -1;
    }
    if (s_a2dp_source_service_ctx.start_status)
    {
        LOGW("already started");
        return 0;
    }

    s_a2dp_source_service_ctx.local_action_pending = 1;
    err = bk_a2dp_media_ctrl(BK_A2DP_MEDIA_CTRL_START);
    if (err) { LOGE("media ctrl start err %d", err); s_a2dp_source_service_ctx.local_action_pending = 0; return -1; }

    err = rtos_get_semaphore(&s_a2dp_source_service_ctx.api_sema, 6000);   /* AUDIO_STATE STARTED */
    s_a2dp_source_service_ctx.local_action_pending = 0;
    if (err) { LOGE("wait started err"); return -1; }

    return 0;
}

int bk_a2dp_source_service_stream_suspend(void)
{
    int err;

    if (s_a2dp_source_service_ctx.conn_state != BK_A2DP_CONNECTION_STATE_CONNECTED)
    {
        LOGE("not connected");
        return -1;
    }
    if (!s_a2dp_source_service_ctx.start_status)
    {
        LOGW("already suspended");
        return 0;
    }

    s_a2dp_source_service_ctx.local_action_pending = 1;
    err = bk_a2dp_media_ctrl(BK_A2DP_MEDIA_CTRL_SUSPEND);
    if (err) { LOGE("media ctrl suspend err %d", err); s_a2dp_source_service_ctx.local_action_pending = 0; return -1; }

    err = rtos_get_semaphore(&s_a2dp_source_service_ctx.api_sema, 6000);   /* AUDIO_STATE SUSPEND */
    s_a2dp_source_service_ctx.local_action_pending = 0;
    if (err) { LOGE("wait suspend err"); return -1; }

    return 0;
}

int bk_a2dp_source_service_set_pcm_format(uint32_t sample_rate, uint8_t ch, uint8_t bits)
{
    if (!sample_rate || !ch || !bits)
    {
        return -1;
    }

    s_a2dp_source_service_ctx.src_rate = sample_rate;
    s_a2dp_source_service_ctx.src_ch = ch;
    s_a2dp_source_service_ctx.src_bits = bits;
    s_a2dp_source_service_ctx.trigger_size = sample_rate * ch * (bits / 8) * DECODE_TRIGGER_TIME / 1000;
    return 0;
}

int bk_a2dp_source_service_set_codec_cfg(const bk_a2dp_mcc_t *cap)
{
    if (!cap)
    {
        return -1;
    }

    os_memcpy(&s_a2dp_source_service_ctx.sbc_cap, cap, sizeof(s_a2dp_source_service_ctx.sbc_cap));
    s_a2dp_source_service_ctx.cap_valid = 1;
    return 0;
}

int bk_a2dp_source_service_media_start(void)
{
    int err = 0;

    if (!s_a2dp_source_service_ctx.cap_valid || !s_a2dp_source_service_ctx.src_rate)
    {
        LOGE("pcm format / codec cfg not set");
        return -1;
    }

    if (!s_a2dp_source_service_ctx.produce_sema)
    {
        err = rtos_init_semaphore(&s_a2dp_source_service_ctx.produce_sema, 1);
        if (err) { LOGE("sema init err %d", err); goto fail; }
    }

    if (!ring_buffer_particle_is_init(&s_a2dp_source_service_ctx.rb_ctx))
    {
        err = ring_buffer_particle_init(&s_a2dp_source_service_ctx.rb_ctx, s_a2dp_source_service_ctx.trigger_size + RB_HEADROOM_SIZE);
        if (err) { LOGE("ring buffer init err %d", err); goto fail; }
    }

    err = bk_a2dp_source_pcm_service_init();
    if (err) { LOGE("pcm worker init err %d", err); goto fail; }

#if CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_A2DP_SOURCE_AAC
    if (s_a2dp_source_service_ctx.sbc_cap.type == BK_A2DP_CODEC_TYPE_AAC)
    {
        err = source_aac_encoder_init();
        if (err) { LOGE("aac encoder init err %d", err); goto fail; }
    }
    else
#endif
    {
        err = source_sbc_encoder_init();
        if (err) { LOGE("sbc encoder init err %d", err); goto fail; }
    }

    uint8_t  codec_ch   = (s_a2dp_source_service_ctx.sbc_cap.type == BK_A2DP_CODEC_TYPE_AAC) ? s_a2dp_source_service_ctx.sbc_cap.cie.aac_codec.channels : s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.channels;
    uint32_t codec_rate = (s_a2dp_source_service_ctx.sbc_cap.type == BK_A2DP_CODEC_TYPE_AAC) ? s_a2dp_source_service_ctx.sbc_cap.cie.aac_codec.sample_rate : s_a2dp_source_service_ctx.sbc_cap.cie.sbc_codec.sample_rate;

    if (s_a2dp_source_service_ctx.src_rate != codec_rate || s_a2dp_source_service_ctx.src_ch != codec_ch || s_a2dp_source_service_ctx.src_bits != SBC_SAMPLE_DEPTH)
    {
        if (!s_a2dp_source_service_ctx.rsp_inited)
        {
            bt_audio_resample_init_req_t req = {0};

            req.rsp_cfg.src_rate  = s_a2dp_source_service_ctx.src_rate;
            req.rsp_cfg.src_ch    = codec_ch;
            req.rsp_cfg.src_bits  = s_a2dp_source_service_ctx.src_bits;
            req.rsp_cfg.dest_rate = codec_rate;
            req.rsp_cfg.dest_ch   = codec_ch;
            req.rsp_cfg.dest_bits = SBC_SAMPLE_DEPTH;
            req.rsp_cfg.complexity = 0;
            req.rsp_cfg.down_ch_idx = 0;

            err = bk_a2dp_source_pcm_service_rsp_init_req(&req, 1);
            if (err) { LOGE("resample init err %d", err); goto fail; }
            s_a2dp_source_service_ctx.rsp_inited = 1;
        }
    }

    /* Arm stack: tell it the source PCM format and our three callbacks. */
    bk_a2dp_source_set_pcm_data_format(s_a2dp_source_service_ctx.src_rate, s_a2dp_source_service_ctx.src_bits, s_a2dp_source_service_ctx.src_ch);
    bk_a2dp_source_register_data_callback(source_data_cb);
    bk_a2dp_source_register_pcm_encode_callback(source_encode_cb);
    bk_a2dp_source_register_pcm_resample_callback(source_resample_cb);

    s_a2dp_source_service_ctx.pause = 0;
    s_a2dp_source_service_ctx.running = 1;
    return 0;

fail:
    bk_a2dp_source_service_media_stop();
    return -1;
}

int bk_a2dp_source_service_media_stop(void)
{
    s_a2dp_source_service_ctx.running = 0;

    /* wake any blocked write_pcm and give it a moment to leave write_pcm
     * before we tear the ring buffer down. */
    if (s_a2dp_source_service_ctx.produce_sema)
    {
        rtos_set_semaphore(&s_a2dp_source_service_ctx.produce_sema);
        rtos_delay_milliseconds(50);
    }

    if (s_a2dp_source_service_ctx.rsp_inited)
    {
        bk_a2dp_source_pcm_service_rsp_init_req(NULL, 0);
        s_a2dp_source_service_ctx.rsp_inited = 0;
    }

    source_sbc_encoder_deinit();
    bk_a2dp_source_pcm_service_deinit();

    if (ring_buffer_particle_is_init(&s_a2dp_source_service_ctx.rb_ctx))
    {
        ring_buffer_particle_deinit(&s_a2dp_source_service_ctx.rb_ctx);
    }

    if (s_a2dp_source_service_ctx.produce_sema)
    {
        rtos_deinit_semaphore(&s_a2dp_source_service_ctx.produce_sema);
        s_a2dp_source_service_ctx.produce_sema = NULL;
    }

    s_a2dp_source_service_ctx.trigger_size = 0;
    return 0;
}

int32_t bk_a2dp_source_service_write_pcm(const uint8_t *pcm, uint32_t len)
{
    if (!pcm || !len)
    {
        return 0;
    }

    while (s_a2dp_source_service_ctx.running)
    {
        /* back-pressure: keep the buffer under the trigger high-water mark */
        if (ring_buffer_particle_len(&s_a2dp_source_service_ctx.rb_ctx) > s_a2dp_source_service_ctx.trigger_size)
        {
            rtos_get_semaphore(&s_a2dp_source_service_ctx.produce_sema, BEKEN_WAIT_FOREVER);
            continue;
        }

        if (ring_buffer_particle_write(&s_a2dp_source_service_ctx.rb_ctx, (uint8_t *)pcm, len) == 0)
        {
            return (int32_t)len;
        }

        /* buffer full: wait for the consumer, then retry the same frame */
        rtos_get_semaphore(&s_a2dp_source_service_ctx.produce_sema, BEKEN_WAIT_FOREVER);
    }

    return -1;
}

void bk_a2dp_source_service_pcm_pause(bool pause)
{
    s_a2dp_source_service_ctx.pause = pause ? 1 : 0;
}
