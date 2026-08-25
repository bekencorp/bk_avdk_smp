#include "a2dp_sink_audio.h"

#include <math.h>
#include <os/mem.h>
#include <os/os.h>
#include <stdbool.h>

#include "mpeg4_latm_dec.h"
#include "audio_play.h"
#include "spk_service.h"
#include "components/log.h"

#define TAG "bk_a2dp_audio"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)


#define CODEC_AUDIO_SBC 0x00U
#define CODEC_AUDIO_AAC 0x02U

#define A2DP_SBC_CHANNEL_MONO         0x08U
#define A2DP_SBC_CHANNEL_DUAL         0x04U
#define A2DP_SBC_CHANNEL_STEREO       0x02U
#define A2DP_SBC_CHANNEL_JOINT_STEREO 0x01U

/* A2DP DAC digital-gain mapping curve. Music peaks near 0 dBFS, so cap the DAC
 * digital gain at 0 dB to avoid clipping/distortion at max volume. */
#define A2DP_DAC_DB_MIN          (-36.0f)
#define A2DP_DAC_DB_MAX          (0.0f)
#define A2DP_DAC_DB_SMOOTH_GAMMA (2.35f)
#define AVRCP_GAIN_MAX           (64 - 1)

#define A2DP_POP_NOISE_SUPPRESS_ENABLE 1
#if A2DP_POP_NOISE_SUPPRESS_ENABLE
#define A2DP_START_UNMUTE_FRAME_THRESHOLD 3
#endif

#ifndef BK_AUD_DAC_DIG_GAIN_DB_SILENCE
#define BK_AUD_DAC_DIG_GAIN_DB_SILENCE (A2DP_DAC_DB_MIN)
#endif

/* A2DP downlink EQ (music). Flag below just picks whether an EQ node is inserted
 * into the audio_play pipeline; the tuned coefficients are project-owned and baked
 * in at create time (bt_a2dp_audio_fill_eq). Only built with CONFIG_ADK_EQ_ALGORITHM. */
#define A2DP_EQ_ENABLE       1                /* 0: bypass EQ, 1: insert EQ node */

typedef enum
{
    BK_A2DP_AUDIO_PLAYER_OPEN_READY_TO_START = 0,
    BK_A2DP_AUDIO_PLAYER_OPEN_NOT_READY,
    BK_A2DP_AUDIO_PLAYER_OPEN_ALREADY_OPEN,
    BK_A2DP_AUDIO_PLAYER_OPEN_FAILED,
} bk_a2dp_audio_player_open_result_t;

typedef struct
{
    bk_a2dp_mcc_t codec;
    audio_play_decoder_t decoder_type;
    uint16_t frame_length;
    uint16_t org_frame_len;
#if A2DP_POP_NOISE_SUPPRESS_ENABLE
    uint8_t target_volume;
    uint8_t start_frame_count;
    uint8_t playback_unmuted;
#endif
    onboard_speaker_pa_ctrl_t pa_ctrl;
} bk_a2dp_audio_ctx_t;

static audio_play_t *s_audio_play_obj = NULL;
static bk_a2dp_audio_ctx_t s_a2dp_audio =
{
    .decoder_type = AUDIO_PLAY_DECODER_SBC,
};

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
static int a2dp_sink_pcm_to_spk(void *user, void *pcm, uint32_t len)
{
    (void)user;
    return spk_service_write(SPK_SERVICE_SRC_A2DP, pcm, len);
}
#endif

/* Hook invoked before opening the A2DP audio player. A project that runs an
 * HFP speaker/mic task (e.g. the HFP demo) overrides this strong symbol to wait
 * for that task to tear down first. The weak default is a no-op for projects
 * without HFP. */
__attribute__((weak)) int32_t wait_hfp_speaker_mic_task_end(void)
{
    return 0;
}

static float a2dp_vol_to_dac_dig_gain_db(uint8_t vol)
{
    if (vol == 0 || AVRCP_GAIN_MAX == 0)
    {
        return BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
    }

    float norm = (float)vol / (float)AVRCP_GAIN_MAX;
    float shaped = powf(norm, A2DP_DAC_DB_SMOOTH_GAMMA);
    return A2DP_DAC_DB_MIN + (A2DP_DAC_DB_MAX - A2DP_DAC_DB_MIN) * shaped;
}

static float a2dp_sink_audio_apply_gain(uint8_t avrcp_vol, bool unmute)
{
    uint8_t gain = avrcp_vol >> 1;
    float dig_gain_db = a2dp_vol_to_dac_dig_gain_db(gain);
    LOGI("a2dp_sink_audio_apply_gain gain: %d, dig_gain_db: %f, unmute: %d\n", gain, dig_gain_db, unmute);
    if(s_audio_play_obj)
    {
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        spk_service_set_volume(dig_gain_db);

        if (gain == 0)
        {
            spk_service_set_src_mute(SPK_SERVICE_SRC_A2DP, 1);
        }
#if A2DP_POP_NOISE_SUPPRESS_ENABLE
        else if (unmute)
        {
            spk_service_set_src_mute(SPK_SERVICE_SRC_A2DP, 0);
        }
        else
        {
            spk_service_set_src_mute(SPK_SERVICE_SRC_A2DP, 1);
        }
#else
        else
        {
            (void)unmute;
            spk_service_set_src_mute(SPK_SERVICE_SRC_A2DP, 0);
        }
#endif
#else /* single-source legacy path */
        audio_play_set_volume(s_audio_play_obj, dig_gain_db);

        if (gain == 0)
        {
            audio_play_control(s_audio_play_obj, AUDIO_PLAY_MUTE);
        }
#if A2DP_POP_NOISE_SUPPRESS_ENABLE
        else if (unmute)
        {
            audio_play_control(s_audio_play_obj, AUDIO_PLAY_UNMUTE);
        }
        else
        {
            audio_play_control(s_audio_play_obj, AUDIO_PLAY_MUTE);
        }
#else
        else
        {
            (void)unmute;
            audio_play_control(s_audio_play_obj, AUDIO_PLAY_UNMUTE);
        }
#endif
#endif /* CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE */
    }
    return dig_gain_db;
}

float a2dp_sink_audio_set_gain(uint8_t avrcp_vol)
{
#if A2DP_POP_NOISE_SUPPRESS_ENABLE
    s_a2dp_audio.target_volume = avrcp_vol;
    return a2dp_sink_audio_apply_gain(avrcp_vol, s_a2dp_audio.playback_unmuted);
#else
    return a2dp_sink_audio_apply_gain(avrcp_vol, true);
#endif
}

static void a2dp_sink_audio_try_unmute_after_buffering(void)
{
#if A2DP_POP_NOISE_SUPPRESS_ENABLE
    if (!s_audio_play_obj || s_a2dp_audio.playback_unmuted)
    {
        return;
    }

    s_a2dp_audio.start_frame_count++;
    if (s_a2dp_audio.start_frame_count >= A2DP_START_UNMUTE_FRAME_THRESHOLD)
    {
        s_a2dp_audio.playback_unmuted = 1;
        a2dp_sink_audio_apply_gain(s_a2dp_audio.target_volume, true);
    }
#endif
}

static void a2dp_sink_audio_player_close(void)
{
    if (s_audio_play_obj)
    {
#if CONFIG_AUD_PARAM_CTRL
        bt_a2dp_audio_unbind();
#endif
        audio_play_close(s_audio_play_obj);
        audio_play_destroy(s_audio_play_obj);
        s_audio_play_obj = NULL;
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        /* Release the A2DP main source; the shared speaker/DAC stays alive. */
        spk_service_detach(SPK_SERVICE_SRC_A2DP);
#endif
        LOGI("%s audio play closed\n", __func__);
    }
}

static bk_a2dp_audio_player_open_result_t a2dp_sink_audio_player_open(uint8_t open_vote,
                                                                      uint8_t mix_multi_channel,
                                                                      uint8_t avrcp_vol)
{
    (void)mix_multi_channel;
    uint8_t vote = 0;
    audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();
    audio_play_decoder_t decoder_type = s_a2dp_audio.decoder_type;
    uint16_t frame_length = s_a2dp_audio.frame_length;
    bk_err_t ret;

    if (s_audio_play_obj)
    {
        LOGE("%s audio play already open\n", __func__);
        return BK_A2DP_AUDIO_PLAYER_OPEN_ALREADY_OPEN;
    }

    for (uint32_t i = BK_A2DP_AUDIO_OPEN_VOTE_START; i < BK_A2DP_AUDIO_OPEN_VOTE_END; ++i)
    {
        vote |= (1 << i);
    }

    if (vote != open_vote)
    {
        LOGE("%s vote not full, can't start audio play 0x%x\n", __func__, open_vote);
        return BK_A2DP_AUDIO_PLAYER_OPEN_NOT_READY;
    }

    if (decoder_type != AUDIO_PLAY_DECODER_SBC && decoder_type != AUDIO_PLAY_DECODER_AAC)
    {
        LOGE("%s unsupported decoder type %d\n", __func__, decoder_type);
        return BK_A2DP_AUDIO_PLAYER_OPEN_FAILED;
    }

#if !CONFIG_ADK_AAC_DECODER
    if (decoder_type == AUDIO_PLAY_DECODER_AAC)
    {
        LOGE("%s AAC decoder not supported\n", __func__);
        return BK_A2DP_AUDIO_PLAYER_OPEN_FAILED;
    }
#endif

    cfg.nChans = (decoder_type == AUDIO_PLAY_DECODER_SBC) ?
                 s_a2dp_audio.codec.cie.sbc_codec.channels :
                 s_a2dp_audio.codec.cie.aac_codec.channels;
    cfg.sampRate = (decoder_type == AUDIO_PLAY_DECODER_SBC) ?
                   s_a2dp_audio.codec.cie.sbc_codec.sample_rate :
                   s_a2dp_audio.codec.cie.aac_codec.sample_rate;
#if A2DP_POP_NOISE_SUPPRESS_ENABLE
    s_a2dp_audio.target_volume = avrcp_vol;
    s_a2dp_audio.start_frame_count = 0;
    s_a2dp_audio.playback_unmuted = 0;
    cfg.volume = BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
#else
    cfg.volume = a2dp_vol_to_dac_dig_gain_db(avrcp_vol >> 1);
#endif
    cfg.frame_size = cfg.sampRate * cfg.nChans / 1000 * 20 * cfg.bitsPerSample / 8;
    cfg.pool_size = cfg.frame_size * 2;
    cfg.decoder_type = decoder_type;
    cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT;
    cfg.pa_ctrl_en   = s_a2dp_audio.pa_ctrl.pa_ctrl_en;
    cfg.pa_ctrl_gpio = s_a2dp_audio.pa_ctrl.pa_ctrl_gpio;
    cfg.pa_on_level  = s_a2dp_audio.pa_ctrl.pa_on_level;
    cfg.pa_on_delay  = s_a2dp_audio.pa_ctrl.pa_on_delay;
    cfg.pa_off_delay = s_a2dp_audio.pa_ctrl.pa_off_delay;

#if A2DP_EQ_ENABLE && CONFIG_ADK_EQ_ALGORITHM
    {
        eq_algorithm_cfg_t eq_cfg = DEFAULT_EQ_ALGORITHM_CONFIG();
        eq_cfg.eq_mode       = EQ_MODE_SOFTWARE;
        eq_cfg.eq_chl_num    = cfg.nChans;
        eq_cfg.eq_frame_size = cfg.nChans ? (int)(cfg.frame_size / cfg.nChans) : (int)cfg.frame_size;
#if CONFIG_AUD_PARAM_CTRL
        cfg.eq_enable = bt_a2dp_audio_fill_eq(&eq_cfg.eq_cal_para, cfg.sampRate);
#else
        cfg.eq_enable = 1;
#endif
        cfg.eq_cfg    = eq_cfg;
    }
#endif

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    if (BK_OK != spk_service_init())
    {
        LOGE("%s spk_service init err\n", __func__);
        return BK_A2DP_AUDIO_PLAYER_OPEN_FAILED;
    }
    cfg.pcm_sink      = a2dp_sink_pcm_to_spk;
    cfg.pcm_sink_user = NULL;
    {
        spk_source_cfg_t scfg =
        {
            .src           = SPK_SERVICE_SRC_A2DP,
            .nChans        = cfg.nChans,
            .sampRate      = cfg.sampRate,
            .bitsPerSample = cfg.bitsPerSample,
            .frame_size    = cfg.frame_size,
            .volume        = cfg.volume,
        };
        if (BK_OK != spk_service_attach(&scfg))
        {
            LOGE("%s spk_service attach A2DP err\n", __func__);
            return BK_A2DP_AUDIO_PLAYER_OPEN_FAILED;
        }
    }
#else
    wait_hfp_speaker_mic_task_end();
#endif

    s_audio_play_obj = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);
    if (!s_audio_play_obj)
    {
        LOGE("%s create audio play err\n", __func__);
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        spk_service_detach(SPK_SERVICE_SRC_A2DP);
#endif
        return BK_A2DP_AUDIO_PLAYER_OPEN_FAILED;
    }

    ret = audio_play_open(s_audio_play_obj);
    if (ret != BK_OK)
    {
        LOGE("%s open audio play err %d\n", __func__, ret);
        audio_play_destroy(s_audio_play_obj);
        s_audio_play_obj = NULL;
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        spk_service_detach(SPK_SERVICE_SRC_A2DP);
#endif
        return BK_A2DP_AUDIO_PLAYER_OPEN_FAILED;
    }

#if CONFIG_AUD_PARAM_CTRL
    bt_a2dp_audio_bind(s_audio_play_obj, cfg.sampRate);
#endif

#if A2DP_POP_NOISE_SUPPRESS_ENABLE
    a2dp_sink_audio_apply_gain(avrcp_vol, false);
#else
    a2dp_sink_audio_apply_gain(avrcp_vol, true);
#endif
    return BK_A2DP_AUDIO_PLAYER_OPEN_READY_TO_START;
}

static bool a2dp_sink_audio_player_is_open(void)
{
    return s_audio_play_obj != NULL;
}

static uint32_t a2dp_sink_audio_player_get_free_frame_num(void)
{
    if (!s_audio_play_obj)
    {
        return 0;
    }

    return UINT32_MAX;
}

static bk_err_t a2dp_sink_audio_player_write_frame_data(char *data, uint32_t len)
{
    int size;

    if (!s_audio_play_obj)
    {
        return BK_FAIL;
    }

    size = audio_play_write_data(s_audio_play_obj, data, len);
    if (size > 0)
    {
        a2dp_sink_audio_try_unmute_after_buffering();
        return BK_OK;
    }

    return BK_FAIL;
}

#if CONFIG_ADK_AAC_DECODER
#define A2DP_AAC_FRAME_BUFFER_MAX_SIZE 1024
#define A2DP_ACC_SINGLE_CHANNEL_MAX_FRAM_SIZE 768
#define A2DP_AAC_LEN_FIELD_OFFSET 9
#define ADTS_HEADER_SIZE 7

static uint8_t adts_get_sr_index(uint32_t sample_rate)
{
    static const uint32_t sr_table[] =
    {
        96000, 88200, 64000, 48000, 44100, 32000,
        24000, 22050, 16000, 12000, 11025, 8000, 7350
    };
    uint8_t i;

    for (i = 0; i < sizeof(sr_table) / sizeof(sr_table[0]); i++)
    {
        if (sample_rate == sr_table[i])
        {
            return i;
        }
    }

    return 4;
}

static void adts_header_generate(uint8_t *header, uint32_t aac_len, uint8_t channels, uint32_t sample_rate)
{
    uint8_t freq_idx = adts_get_sr_index(sample_rate);
    uint32_t frame_len = aac_len + ADTS_HEADER_SIZE;

    header[0] = 0xFF;
    header[1] = 0xF1;
    header[2] = (1 << 6) | (freq_idx << 2) | ((channels >> 2) & 0x1);
    header[3] = ((channels & 0x3) << 6) | ((frame_len >> 11) & 0x3);
    header[4] = (frame_len >> 3) & 0xFF;
    header[5] = ((frame_len & 0x7) << 5) | 0x1F;
    header[6] = 0xFC;
}
#endif

static bk_err_t sbc_frame_length_parse(uint8_t *buf, uint16_t len)
{
    uint8_t syncword;
    uint8_t channel_mode = 0;
    uint8_t blocks = 0;
    uint8_t subbands = 0;
    uint8_t bitpool = 0;
    uint8_t num_channels = 1;
    int32_t calculated_frame_len = 0;

    if (buf == NULL || len < 3)
    {
        return BK_FAIL;
    }

    syncword = buf[0];
    if (syncword == 0x9C)
    {
        channel_mode = (buf[1] >> 2) & 0x03;
        blocks = (((buf[1] >> 4) & 0x03) + 1) << 2;
        subbands = ((buf[1] & 0x01) + 1) << 2;
        bitpool = buf[2];
        num_channels = (channel_mode == 0) ? 1 : 2;
    }
    else if (syncword == 0xAD)
    {
        blocks = 15;
        subbands = 8;
        bitpool = 26;
        num_channels = 1;
        channel_mode = 0;
    }
    else
    {
        return BK_FAIL;
    }

    calculated_frame_len = 4 + ((4 * subbands * num_channels) >> 3);
    if (channel_mode == 0 || channel_mode == 1)
    {
        calculated_frame_len += ((blocks * num_channels * bitpool) + 7) >> 3;
    }
    else if (channel_mode == 3)
    {
        calculated_frame_len += (subbands + blocks * bitpool + 7) >> 3;
    }
    else
    {
        calculated_frame_len += (blocks * bitpool + 7) >> 3;
    }

    if (calculated_frame_len > len)
    {
        return BK_FAIL;
    }

    return calculated_frame_len;
}

void a2dp_sink_audio_set_config(const bk_a2dp_mcc_t *codec)
{
    if (codec)
    {
        s_a2dp_audio.codec = *codec;
    }
}

void a2dp_sink_audio_set_pa_ctrl(const onboard_speaker_pa_ctrl_t *pa)
{
    if (pa)
    {
        s_a2dp_audio.pa_ctrl = *pa;
    }
    else
    {
        onboard_speaker_pa_ctrl_t off = DEFAULT_ONBOARD_SPEAKER_PA_CTRL();
        s_a2dp_audio.pa_ctrl = off;
    }
}

static bk_err_t a2dp_sink_audio_prepare(const bk_a2dp_mcc_t *codec)
{
    uint16_t frame_length = 0;

    if (!codec)
    {
        return BK_FAIL;
    }

    s_a2dp_audio.codec = *codec;

    if (codec->type == CODEC_AUDIO_SBC)
    {
        uint8_t chnl_mode = codec->cie.sbc_codec.channel_mode;
        uint8_t chnls = codec->cie.sbc_codec.channels;
        uint8_t subbands = codec->cie.sbc_codec.subbands;
        uint8_t blocks = codec->cie.sbc_codec.block_len;
        uint8_t bitpool = codec->cie.sbc_codec.bit_pool;

        if (chnl_mode == A2DP_SBC_CHANNEL_MONO || chnl_mode == A2DP_SBC_CHANNEL_DUAL)
        {
            frame_length = 4 + ((4 * subbands * chnls) >> 3) + ((blocks * chnls * bitpool + 7) >> 3);
        }
        else if (chnl_mode == A2DP_SBC_CHANNEL_STEREO)
        {
            frame_length = 4 + ((4 * subbands * chnls) >> 3) + ((blocks * bitpool + 7) >> 3);
        }
        else
        {
            frame_length = 4 + ((4 * subbands * chnls) >> 3) + ((subbands + blocks * bitpool + 7) >> 3);
        }

        s_a2dp_audio.org_frame_len = frame_length;
        s_a2dp_audio.frame_length = frame_length * 2;
        s_a2dp_audio.decoder_type = AUDIO_PLAY_DECODER_SBC;
        LOGI("CODEC_AUDIO_SBC cm:%d, c:%d, s:%d, b:%d, bi:%d, frame_length:%d \n", chnl_mode, chnls, subbands, blocks, bitpool, frame_length);
    }
    else if (codec->type == CODEC_AUDIO_AAC)
    {
#if CONFIG_ADK_AAC_DECODER
        s_a2dp_audio.frame_length = A2DP_ACC_SINGLE_CHANNEL_MAX_FRAM_SIZE * codec->cie.aac_codec.channels;
        s_a2dp_audio.org_frame_len = 0;
        s_a2dp_audio.decoder_type = AUDIO_PLAY_DECODER_AAC;
        LOGI("CODEC_AUDIO_AAC : ch:%d, frame_length:%d \n", codec->cie.aac_codec.channels, frame_length);
#else
        LOGW("AAC decoder disabled\n");
        return BK_FAIL;
#endif
    }
    else
    {
        LOGW("Unsupported codec %u\n", codec->type);
        return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t a2dp_sink_audio_open(uint32_t open_vote,
                              uint8_t mix_multi_channel,
                              uint8_t volume)
{
    bk_a2dp_audio_player_open_result_t open_result;

    open_result = a2dp_sink_audio_player_open(open_vote,
                                              mix_multi_channel,
                                              volume);
    if (open_result == BK_A2DP_AUDIO_PLAYER_OPEN_FAILED)
    {
        return BK_FAIL;
    }
    if (open_result != BK_A2DP_AUDIO_PLAYER_OPEN_READY_TO_START)
    {
        return BK_OK;
    }

    return BK_OK;
}

bk_err_t a2dp_sink_audio_start(const bk_a2dp_mcc_t *codec,
                               uint32_t open_vote,
                               uint8_t mix_multi_channel,
                               uint8_t volume)
{
    if (a2dp_sink_audio_prepare(codec) != BK_OK)
    {
        return BK_FAIL;
    }

    return a2dp_sink_audio_open(open_vote, mix_multi_channel, volume);
}

void a2dp_sink_audio_stop(void)
{
#if A2DP_POP_NOISE_SUPPRESS_ENABLE
    s_a2dp_audio.playback_unmuted = 0;
    a2dp_sink_audio_apply_gain(s_a2dp_audio.target_volume, false);
#endif
    a2dp_sink_audio_player_close();
}

void a2dp_sink_audio_handle_data(uint8_t *data,
                                 uint16_t len)
{
    uint8_t *fb = data;

    if (!a2dp_sink_audio_player_is_open() || !fb || !len)
    {
        LOGE("%s audio play not open or data is null or len is 0\n", __func__);
        return;
    }

    if (s_a2dp_audio.codec.type == CODEC_AUDIO_SBC)
    {
        uint8_t payload_header = *fb++;
        uint8_t frame_num = payload_header & 0xF;
        uint32_t payload_len = len - 1;

        if (payload_len != s_a2dp_audio.org_frame_len * frame_num)
        {
            for (uint32_t i = 0; i < payload_len;)
            {
                int32_t tmp_frame_len = sbc_frame_length_parse(fb + i, payload_len - i);
                if (tmp_frame_len <= 0)
                {
                    LOGE("%s index %d is not 0x9c (0x%x)!!!\n", __func__, i, fb[i]);
                    break;
                }
                if (a2dp_sink_audio_player_get_free_frame_num() >= (uint32_t)tmp_frame_len)
                {
                    a2dp_sink_audio_player_write_frame_data((char *)(fb + i), tmp_frame_len);
                }
                else
                {
                    LOGW("%s audio_play frame buffer(sbc) is full \n", __func__);
                    break;
                }
                i += tmp_frame_len;
            }
        }
        else if (a2dp_sink_audio_player_get_free_frame_num() >= payload_len)
        {
            a2dp_sink_audio_player_write_frame_data((char *)fb, payload_len);
        }
        else
        {
            LOGW("%s audio_play frame buffer(sbc) is full \n", __func__);
        }
    }
    else if (s_a2dp_audio.codec.type == CODEC_AUDIO_AAC)
    {
#if CONFIG_ADK_AAC_DECODER
        uint8_t *inbuf = NULL;
        uint8_t *end = fb + len;
        uint32_t inlen = 0;
        uint8_t len_byte = 255;

        if (len <= A2DP_AAC_LEN_FIELD_OFFSET)
        {
            LOGE("%s aac packet too short: %u\n", __func__, len);
            return;
        }

        inbuf = &fb[A2DP_AAC_LEN_FIELD_OFFSET];
        do
        {
            if (inbuf >= end)
            {
                LOGE("%s aac length field exceeds packet, len %u\n", __func__, len);
                return;
            }

            len_byte = *inbuf++;
            inlen += len_byte;
        } while (len_byte == 255);

        if (a2dp_sink_audio_player_get_free_frame_num())
        {
            uint8_t *output = NULL;
            uint32_t output_len = 0;
            if (mpeg4_latm_decode(fb, len, &output, &output_len))
            {
                LOGE("====%s latm decode err, discard it\n", __func__);
                return;
            }
            else if (len - (output - fb) != output_len ||
                     inlen != output_len ||
                     output != inbuf)
            {
                LOGE("====%s len not match, total %d, type1 offset %d len %d, type2 offset %d len %d\n",
                     __func__, len, output - fb, inbuf - fb, output_len, inlen);
                return;
            }

            {
                if (inlen <= A2DP_AAC_FRAME_BUFFER_MAX_SIZE)
                {
                    uint8_t frame_buf[A2DP_AAC_FRAME_BUFFER_MAX_SIZE + ADTS_HEADER_SIZE];
                    adts_header_generate(frame_buf,
                                         inlen,
                                         s_a2dp_audio.codec.cie.aac_codec.channels,
                                         s_a2dp_audio.codec.cie.aac_codec.sample_rate);
                    os_memcpy(frame_buf + ADTS_HEADER_SIZE, inbuf, inlen);
                    a2dp_sink_audio_player_write_frame_data((char *)frame_buf, inlen + ADTS_HEADER_SIZE);
                }
                else
                {
                    LOGW("%s aac frame too large: %d\n", __func__, inlen);
                }
            }
        }
        else
        {
            LOGI("%s audio_play frame buffer(aac) is full \n", __func__);
        }
#endif
    }
}

int32_t a2dp_sink_audio_wait_player_end(void)
{
    while (a2dp_sink_audio_player_is_open())
    {
        rtos_delay_milliseconds(20);
    }

    return 0;
}
