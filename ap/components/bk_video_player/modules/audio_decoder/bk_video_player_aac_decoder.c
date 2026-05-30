// Copyright 2024-2025 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "os/os.h"
#include "os/mem.h"
#include "os/str.h"

#include "components/avdk_utils/avdk_check.h"
#include "components/avdk_utils/avdk_error.h"
#include "components/bk_video_player/bk_video_player_types.h"
#include "components/bk_video_player/audio_decoder/bk_video_player_aac_decoder.h"

#include <modules/aacdec.h>

#define TAG "vp_aac_dec"

#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

/* MPEG-4 sampling frequency table (ISO/IEC 14496-3 Table 1.16). */
static const uint32_t s_aac_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
    16000, 12000, 11025,  8000,  7350,     0,     0,     0,
};

typedef struct vp_aac_decoder_ctx_s
{
    HAACDecoder                 dec;
    video_player_audio_params_t params;            /* shallow copy (we don't need codec_config after init) */
    bool                        configured;        /* AACSetRawBlockParams done */
    AACFrameInfo                frame_info;        /* last frame info */
} vp_aac_decoder_ctx_t;

typedef struct
{
    video_player_audio_decoder_ops_t ops;
    vp_aac_decoder_ctx_t             ctx;
} vp_aac_decoder_instance_t;

static video_player_audio_decoder_ops_t s_ops_template;

static avdk_err_t vp_aac_decoder_deinit(struct video_player_audio_decoder_ops_s *ops);

/* -------------------------------------------------------------------------- */
/* AudioSpecificConfig parsing                                                 */
/*                                                                            */
/* Layout (5 bits + 4 bits + 4 bits = 13 bits, packed big-endian):            */
/*   audioObjectType            : 5 bits  (1=Main, 2=LC, 3=SSR, 4=LTP)        */
/*   samplingFrequencyIndex     : 4 bits  (0xF means explicit 24-bit rate)    */
/*   channelConfiguration       : 4 bits                                       */
/*                                                                            */
/* Most MP4 AAC files use this 2-byte form. Implicit SBR/PS extensions may    */
/* add bytes but we don't depend on them — base layer config is sufficient    */
/* for the Helix decoder. */
/* -------------------------------------------------------------------------- */
static avdk_err_t vp_aac_parse_asc(const uint8_t *cfg, uint32_t cfg_size,
                                   uint32_t *out_obj_type,
                                   uint32_t *out_sample_rate,
                                   uint32_t *out_channels)
{
    AVDK_RETURN_ON_FALSE(cfg && cfg_size >= 2U, AVDK_ERR_INVAL, TAG, "ASC too short: %u", cfg_size);

    const uint16_t hdr = (uint16_t)(((uint16_t)cfg[0] << 8) | (uint16_t)cfg[1]);
    uint32_t obj_type   = (hdr >> 11) & 0x1FU;
    uint32_t sr_index   = (hdr >> 7)  & 0x0FU;
    uint32_t channels   = (hdr >> 3)  & 0x0FU;

    /* Explicit 24-bit sample rate (rarely used; MP4 audio_rate fallback handles it). */
    uint32_t sample_rate = 0;
    if (sr_index == 0x0FU)
    {
        if (cfg_size < 5U)
        {
            LOGW("%s: explicit sample rate but ASC too short\n", __func__);
        }
        else
        {
            sample_rate = ((uint32_t)cfg[1] & 0x7FU) << 17;
            sample_rate |= (uint32_t)cfg[2] << 9;
            sample_rate |= (uint32_t)cfg[3] << 1;
            sample_rate |= (((uint32_t)cfg[4]) >> 7) & 0x01U;
        }
    }
    else
    {
        sample_rate = s_aac_sample_rates[sr_index];
    }

    if (out_obj_type)    *out_obj_type    = obj_type;
    if (out_sample_rate) *out_sample_rate = sample_rate;
    if (out_channels)    *out_channels    = channels;
    return AVDK_ERR_OK;
}

/* Map AAC audioObjectType to Helix profile constants. */
static int vp_aac_obj_type_to_profile(uint32_t obj_type)
{
    switch (obj_type)
    {
        case 1: return AAC_PROFILE_MP;
        case 2: return AAC_PROFILE_LC;
        case 3: return AAC_PROFILE_SSR;
        case 4: return AAC_PROFILE_LC; /* LTP -> fall back to LC for HE/normal use */
        default:
            LOGW("%s: unknown AAC obj_type=%u, fallback to LC\n", __func__, obj_type);
            return AAC_PROFILE_LC;
    }
}

/* -------------------------------------------------------------------------- */
/* Decoder ops implementation                                                  */
/* -------------------------------------------------------------------------- */

static avdk_err_t vp_aac_decoder_get_supported_formats(const struct video_player_audio_decoder_ops_s *ops,
                                                       const video_player_audio_format_t **formats,
                                                       uint32_t *format_count)
{
    (void)ops;
    if (formats == NULL || format_count == NULL) return AVDK_ERR_INVAL;
    static const video_player_audio_format_t s_formats[] = {
        VIDEO_PLAYER_AUDIO_FORMAT_AAC,
    };
    *formats = s_formats;
    *format_count = (uint32_t)(sizeof(s_formats) / sizeof(s_formats[0]));
    return AVDK_ERR_OK;
}

static avdk_err_t vp_aac_decoder_init(struct video_player_audio_decoder_ops_s *ops,
                                      video_player_audio_params_t *params)
{
    vp_aac_decoder_instance_t *self = __containerof(ops, vp_aac_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self,   AVDK_ERR_INVAL, TAG, "instance is NULL");
    AVDK_RETURN_ON_FALSE(params, AVDK_ERR_INVAL, TAG, "params is NULL");
    vp_aac_decoder_ctx_t *ctx = &self->ctx;

    if (params->format != VIDEO_PLAYER_AUDIO_FORMAT_AAC)
    {
        LOGW("%s: AAC decoder only supports format=AAC (got %u)\n", __func__, params->format);
        return AVDK_ERR_UNSUPPORTED;
    }

    if (ctx->dec != NULL)
    {
        LOGW("%s: already initialized, deinit first\n", __func__);
        vp_aac_decoder_deinit(ops);
    }

    /* Resolve nChans / sampRate / profile from AudioSpecificConfig if present, falling
     * back to params->channels / sample_rate from the container metadata. */
    uint32_t obj_type   = 2U; /* default LC */
    uint32_t sample_rate = params->sample_rate;
    uint32_t channels    = params->channels;

    if (params->codec_config != NULL && params->codec_config_size > 0U)
    {
        avdk_err_t pret = vp_aac_parse_asc(params->codec_config, params->codec_config_size,
                                           &obj_type, &sample_rate, &channels);
        if (pret != AVDK_ERR_OK)
        {
            LOGW("%s: parse ASC failed, ret=%d (using container fallback)\n", __func__, pret);
        }
        else
        {
            LOGI("%s: ASC parsed: obj_type=%u sr=%u ch=%u\n", __func__, obj_type, sample_rate, channels);
        }
    }
    else
    {
        LOGW("%s: no AudioSpecificConfig from container; using ch=%u sr=%u\n",
             __func__, channels, sample_rate);
    }

    if (channels == 0U || channels > AAC_MAX_NCHANS)
    {
        LOGE("%s: bad channels=%u (max=%u)\n", __func__, channels, (unsigned)AAC_MAX_NCHANS);
        return AVDK_ERR_INVAL;
    }
    if (sample_rate == 0U)
    {
        LOGE("%s: bad sample_rate\n", __func__);
        return AVDK_ERR_INVAL;
    }

    /* Update params to reflect what the decoder will actually output (16-bit PCM). */
    params->channels         = channels;
    params->sample_rate      = sample_rate;
    params->bits_per_sample  = 16U;

    ctx->dec = AACInitDecoder();
    if (ctx->dec == NULL)
    {
        LOGE("%s: AACInitDecoder failed\n", __func__);
        return AVDK_ERR_NOMEM;
    }

    /* MP4 AAC carries raw blocks (no ADTS header) — we MUST configure the codec
     * up-front via AACSetRawBlockParams, otherwise AACDecode() will reject the
     * very first packet with ERR_AAC_INVALID_FRAME (-5). */
    AACFrameInfo info;
    os_memset(&info, 0, sizeof(info));
    info.nChans       = (int)channels;
    info.sampRateCore = (int)sample_rate;
    info.sampRateOut  = (int)sample_rate;
    info.bitsPerSample = 16;
    info.profile      = vp_aac_obj_type_to_profile(obj_type);

    int sret = AACSetRawBlockParams(ctx->dec, /*copyLast=*/0, &info);
    if (sret != 0)
    {
        LOGE("%s: AACSetRawBlockParams failed, ret=%d\n", __func__, sret);
        AACFreeDecoder(ctx->dec);
        ctx->dec = NULL;
        return AVDK_ERR_GENERIC;
    }

    os_memcpy(&ctx->params, params, sizeof(*params));
    ctx->params.codec_config      = NULL;
    ctx->params.codec_config_size = 0;
    ctx->frame_info               = info;
    ctx->configured               = true;

    LOGI("%s: AAC decoder initialized: ch=%u sr=%u profile=%d\n",
         __func__, channels, sample_rate, info.profile);
    return AVDK_ERR_OK;
}

static avdk_err_t vp_aac_decoder_deinit(struct video_player_audio_decoder_ops_s *ops)
{
    vp_aac_decoder_instance_t *self = __containerof(ops, vp_aac_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self, AVDK_ERR_INVAL, TAG, "instance is NULL");
    vp_aac_decoder_ctx_t *ctx = &self->ctx;

    if (ctx->dec != NULL)
    {
        AACFreeDecoder(ctx->dec);
        ctx->dec = NULL;
    }
    ctx->configured = false;
    return AVDK_ERR_OK;
}

static avdk_err_t vp_aac_decoder_decode(struct video_player_audio_decoder_ops_s *ops,
                                        video_player_buffer_t *in_buffer,
                                        video_player_buffer_t *out_buffer)
{
    vp_aac_decoder_instance_t *self = __containerof(ops, vp_aac_decoder_instance_t, ops);
    AVDK_RETURN_ON_FALSE(self,                AVDK_ERR_INVAL,   TAG, "instance is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer,           AVDK_ERR_INVAL,   TAG, "in_buffer is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer,          AVDK_ERR_INVAL,   TAG, "out_buffer is NULL");
    AVDK_RETURN_ON_FALSE(in_buffer->data,     AVDK_ERR_INVAL,   TAG, "in_buffer->data is NULL");
    AVDK_RETURN_ON_FALSE(out_buffer->data,    AVDK_ERR_INVAL,   TAG, "out_buffer->data is NULL");

    vp_aac_decoder_ctx_t *ctx = &self->ctx;
    AVDK_RETURN_ON_FALSE(ctx->dec && ctx->configured, AVDK_ERR_GENERIC, TAG, "decoder not initialized");

    if (in_buffer->length == 0U)
    {
        out_buffer->length = 0;
        return AVDK_ERR_OK;
    }

    /* Helix expects pointers to a moveable cursor, decoder advances inbuf and
     * decreases bytesLeft by the number of bytes actually consumed. We feed one
     * MP4 sample at a time which usually contains exactly one AAC raw block. */
    unsigned char *inp     = in_buffer->data;
    int            in_left = (int)in_buffer->length;
    short         *outp    = (short *)out_buffer->data;

    /* Required output capacity for one AAC frame (1024 samples * channels * 2 bytes). */
    const uint32_t need = AAC_MAX_NSAMPS * (uint32_t)ctx->frame_info.nChans * 2U;
    if (out_buffer->length < need)
    {
        LOGE("%s: out buffer too small, need=%u got=%u\n", __func__, need, out_buffer->length);
        out_buffer->length = 0;
        return AVDK_ERR_NOMEM;
    }

    int rc = AACDecode(ctx->dec, &inp, &in_left, outp);
    if (rc != ERR_AAC_NONE)
    {
        LOGE("%s: AACDecode failed, ret=%d (in_len=%u)\n",
             __func__, rc, in_buffer->length);
        out_buffer->length = 0;
        out_buffer->pts    = in_buffer->pts;
        return AVDK_ERR_GENERIC;
    }

    AACGetLastFrameInfo(ctx->dec, &ctx->frame_info);
    const uint32_t pcm_bytes = (uint32_t)ctx->frame_info.outputSamps * 2U; /* 16-bit */

    out_buffer->length = pcm_bytes;
    out_buffer->pts    = in_buffer->pts;

    if (in_left > 0)
    {
        LOGV("%s: %d bytes left in input after decode\n", __func__, in_left);
    }
    return AVDK_ERR_OK;
}

/* -------------------------------------------------------------------------- */
/* Create / destroy / template                                                 */
/* -------------------------------------------------------------------------- */

static struct video_player_audio_decoder_ops_s *vp_aac_decoder_create(void)
{
    vp_aac_decoder_instance_t *inst = (vp_aac_decoder_instance_t *)os_malloc(sizeof(vp_aac_decoder_instance_t));
    if (inst == NULL)
    {
        LOGE("%s: alloc instance failed\n", __func__);
        return NULL;
    }
    os_memset(inst, 0, sizeof(*inst));
    os_memcpy(&inst->ops, &s_ops_template, sizeof(video_player_audio_decoder_ops_t));
    return &inst->ops;
}

static void vp_aac_decoder_destroy(struct video_player_audio_decoder_ops_s *ops)
{
    if (ops == NULL || ops == &s_ops_template) return;
    vp_aac_decoder_instance_t *inst = __containerof(ops, vp_aac_decoder_instance_t, ops);
    /* Make sure the underlying decoder is freed if the caller forgot deinit(). */
    if (inst->ctx.dec != NULL)
    {
        AACFreeDecoder(inst->ctx.dec);
        inst->ctx.dec = NULL;
    }
    os_free(inst);
}

static video_player_audio_decoder_ops_t s_ops_template = {
    .create                = vp_aac_decoder_create,
    .destroy               = vp_aac_decoder_destroy,
    .get_supported_formats = vp_aac_decoder_get_supported_formats,
    .init                  = vp_aac_decoder_init,
    .deinit                = vp_aac_decoder_deinit,
    .decode                = vp_aac_decoder_decode,
};

const video_player_audio_decoder_ops_t *bk_video_player_get_aac_decoder_ops(void)
{
    return &s_ops_template;
}