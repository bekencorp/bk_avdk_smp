#include "hfp_hf_audio.h"

#include <math.h>
#include <string.h>
#include <stdint.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include <driver/aud_dac_types.h>
#include "audio_play.h"
#include "spk_service.h"
#include "audio_record.h"
#include "components/bluetooth/bk_dm_hfp.h"
#include "components/log.h"
#include "a2dp_sink_audio.h"

#define TAG "bk_hfp_audio"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define SCO_CVSD_SAMPLES_PER_FRAME       (60)

/* The sbc encoder in mSBC mode already emits a valid (cycling) 2-byte H2 sync
 * header followed by the 57-byte mSBC payload, so its output frame is 59 bytes.
 * The mSBC SCO packet is a fixed 60 bytes: encoder frame + one trailing pad. */
#define MSBC_ENC_FRAME_LEN               (59)
#define MSBC_SCO_PACKET_LEN              (60)

#define HF_MIC_THREAD_PRI       BEKEN_DEFAULT_WORKER_PRIORITY-1

/* HFP DAC digital-gain mapping curve. SCO voice source levels are low, so keep a
 * small positive boost at max volume to make calls loud enough. */
#define HFP_DAC_DB_MIN                 (-36.0f)
#define HFP_DAC_DB_MAX                 (8.0f)
#define HFP_DAC_DB_SMOOTH_GAMMA        (2.35f)

#ifndef BK_AUD_DAC_DIG_GAIN_DB_SILENCE
#define BK_AUD_DAC_DIG_GAIN_DB_SILENCE (HFP_DAC_DB_MIN)
#endif

#define HFP_GAIN_MAX 15 //see hfp protocol

/* HFP EQ: DL = speaker, UL = mic. Flags below just pick whether an EQ node is
 * inserted; the tuned coefficients are project-owned and baked in at create time
 * (bt_hfp_audio_dl_fill_eq / _ul_fill_eq). Only built with CONFIG_ADK_EQ_ALGORITHM. */
#define HFP_DL_EQ_ENABLE      1                /* downlink: 0 bypass, 1 insert EQ node */
#define HFP_UL_EQ_ENABLE      1                /* uplink:   0 bypass, 1 insert EQ node */

/* Speaker reassembly buffer: holds at most one odd trailing byte plus one SCO
 * packet, so audio_play always receives whole 16-bit samples. */
#define HF_SPK_BUF_SIZE 260

static uint8_t bt_audio_hfp_hf_codec = CODEC_VOICE_CVSD;
static uint8_t s_hfp_peer_addr [ 6 ] = {0};

static uint8_t hf_mic_sco_data [ 1024 ] = {0};

static volatile uint8_t hf_auido_start = 0;

static beken_thread_t hf_mic_thread_handle = NULL;
static audio_play_t *s_audio_play_obj;
static audio_record_t *s_audio_record_obj;
static beken_semaphore_t hf_mic_speaker_exit_sema = NULL;

static uint8_t s_hf_spk_buf[HF_SPK_BUF_SIZE];
static uint16_t s_hf_spk_residual = 0;

/* Last HFP speaker gain (+VGS). The phone may push it before the call audio is
 * up, so cache it here and re-apply once the CALL source is attached. 0xFF: none. */
static uint8_t s_pending_hfp_vol = 0xFF;

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
static int hfp_hf_pcm_to_spk(void *user, void *pcm, uint32_t len)
{
    (void)user;
    return spk_service_write(SPK_SERVICE_SRC_CALL, pcm, len);
}
#endif

#ifdef CONFIG_AUDIO
static void mic_task(void *arg);
static int mic_task_init();
#endif

static float hfp_vol_to_dac_dig_gain_db(uint8_t vol)
{
    if (vol == 0 || HFP_GAIN_MAX == 0) {
        return BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
    }

    {
        float norm = (float)vol / (float)HFP_GAIN_MAX;
        float shaped = powf(norm, HFP_DAC_DB_SMOOTH_GAMMA);
        return HFP_DAC_DB_MIN + (HFP_DAC_DB_MAX - HFP_DAC_DB_MIN) * shaped;
    }
}

float hfp_hf_audio_set_gain(uint8_t hfp_vol)
{
    float gain_db = hfp_vol_to_dac_dig_gain_db(hfp_vol);
    //LOGD("%s set hfp gain step = %u dig_db = %.2f\n", __func__, hfp_vol, gain_db);

    if(s_audio_play_obj)
    {
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        /* Persistent-speaker path: master volume on the global digital gain,
         * mute gated per-source so a concurrent music stream is untouched. */
        spk_service_set_volume(gain_db);
        spk_service_set_src_mute(SPK_SERVICE_SRC_CALL, (hfp_vol == 0) ? 1 : 0);
#else
        audio_play_set_volume(s_audio_play_obj, gain_db);

        if (hfp_vol == 0)
        {
            audio_play_control(s_audio_play_obj, AUDIO_PLAY_MUTE);
        }
        else
        {
            audio_play_control(s_audio_play_obj, AUDIO_PLAY_UNMUTE);
        }
#endif
    }
    else
    {
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        /* Call audio not up yet: remember the gain and apply it on attach so we
         * don't disturb a concurrently playing music stream's global volume. */
        s_pending_hfp_vol = hfp_vol;
        LOGD("%s cache hfp vol %u until call audio starts\n", __func__, hfp_vol);
#else
        LOGE("%s audio play not enable\n", __func__);
#endif
    }

    return gain_db;
}

void hfp_hf_audio_start(uint8_t codec, const uint8_t *peer_addr)
{
    bt_audio_hfp_hf_codec = codec;

    if (peer_addr)
    {
        os_memcpy(s_hfp_peer_addr, peer_addr, sizeof(s_hfp_peer_addr));
    }

    LOGI("BT_AUDIO_VOICE_START_MSG \r\n");

#ifdef CONFIG_AUDIO
    bk_err_t ret = 0;

    audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();

    cfg.nChans   = 1;
    cfg.sampRate = ((CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec) ? 16000 : 8000);
    cfg.volume   = -9.0;
    cfg.frame_size = cfg.sampRate * cfg.nChans / 1000 * 20 * cfg.bitsPerSample / 8;
    cfg.pool_size  = cfg.frame_size * 2;
    cfg.decoder_type = (CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec) ? AUDIO_PLAY_DECODER_MSBC : AUDIO_PLAY_DECODER_PCM;
    cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT;
    cfg.main_dac_source   = AUD_DAC_SOURCE_CALL;

#if HFP_DL_EQ_ENABLE && CONFIG_ADK_EQ_ALGORITHM
    {
        eq_algorithm_cfg_t eq_cfg = DEFAULT_EQ_ALGORITHM_CONFIG();
        eq_cfg.eq_mode       = EQ_MODE_SOFTWARE;
        eq_cfg.eq_chl_num    = cfg.nChans;
        eq_cfg.eq_frame_size = cfg.nChans ? (int)(cfg.frame_size / cfg.nChans) : (int)cfg.frame_size;
#if CONFIG_AUD_PARAM_CTRL
        cfg.eq_enable = bt_hfp_audio_dl_fill_eq(&eq_cfg.eq_cal_para, cfg.sampRate);
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
        return;
    }
    cfg.pcm_sink      = hfp_hf_pcm_to_spk;
    cfg.pcm_sink_user = NULL;
    {
        spk_source_cfg_t scfg =
        {
            .src           = SPK_SERVICE_SRC_CALL,
            .nChans        = cfg.nChans,
            .sampRate      = cfg.sampRate,
            .bitsPerSample = cfg.bitsPerSample,
            .frame_size    = cfg.frame_size,
            .volume        = cfg.volume,
        };
        if (BK_OK != spk_service_attach(&scfg))
        {
            LOGE("%s spk_service attach CALL err\n", __func__);
            return;
        }
    }
#else
    /* The A2DP player and the HFP player share the same DAC, so wait for the
     * A2DP audio path to release it before opening the call player. */
    LOGI("%s wait a2dp task end\n", __func__);
    a2dp_sink_audio_wait_player_end();
#endif

    s_hf_spk_residual = 0;

    s_audio_play_obj = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);
    if(!s_audio_play_obj)
    {
        LOGE("%s create audio play err\n", __func__);
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        spk_service_detach(SPK_SERVICE_SRC_CALL);
#endif
        return;
    }

    if((ret = audio_play_open(s_audio_play_obj)) != 0)
    {
        LOGE("%s open audio play err %d\n", __func__, ret);
        audio_play_destroy(s_audio_play_obj);
        s_audio_play_obj = NULL;
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        spk_service_detach(SPK_SERVICE_SRC_CALL);
#endif
        return;
    }

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    /* Apply any speaker gain the phone sent before the call audio was ready. */
    if (s_pending_hfp_vol != 0xFF)
    {
        hfp_hf_audio_set_gain(s_pending_hfp_vol);
        s_pending_hfp_vol = 0xFF;
    }
#endif

#if CONFIG_AUD_PARAM_CTRL
    bt_hfp_audio_dl_bind(s_audio_play_obj, cfg.sampRate);
#endif

    hf_auido_start = 1;
    mic_task_init();

    LOGI("hfp audio init ok\r\n");
#endif
}

void hfp_hf_audio_stop(void)
{
    LOGI("BT_AUDIO_VOICE_STOP_MSG \r\n");
#ifdef CONFIG_AUDIO
    if (hf_mic_thread_handle)
    {
        if (kNoErr != rtos_init_semaphore(&hf_mic_speaker_exit_sema, 1))
        {
            LOGE("init sema fail, %d \n", __LINE__);
        }
        hf_auido_start = 0;

        LOGI("%s wait mic thread end\n", __func__);
        if (hf_mic_speaker_exit_sema)
        {
            rtos_get_semaphore(&hf_mic_speaker_exit_sema, BEKEN_WAIT_FOREVER);
        }
        LOGI("%s thread end !!!\n", __func__);
        hf_mic_thread_handle = NULL;

        if (hf_mic_speaker_exit_sema)
        {
            rtos_deinit_semaphore(&hf_mic_speaker_exit_sema);
            hf_mic_speaker_exit_sema = NULL;
        }
    }
    else
    {
        hf_auido_start = 0;
    }

    /* The speaker path runs in the caller (audio queue) context, so no player
     * write can be in flight here; close it directly. */
    if (s_audio_play_obj)
    {
#if CONFIG_AUD_PARAM_CTRL
        bt_hfp_audio_dl_unbind();
#endif
        bk_err_t ret = audio_play_close(s_audio_play_obj);
        if (ret)
        {
            LOGE("%s close audio play err %d\n", __func__, ret);
        }

        ret = audio_play_destroy(s_audio_play_obj);
        if (ret)
        {
            LOGE("%s destroy audio play err %d\n", __func__, ret);
        }

        s_audio_play_obj = NULL;
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        /* Release the CALL aux source; the shared speaker/DAC stays alive. */
        spk_service_detach(SPK_SERVICE_SRC_CALL);
#endif
    }
#endif
}

void hfp_hf_audio_handle_data(const uint8_t *data, uint16_t len)
{
#ifdef CONFIG_AUDIO
    uint16_t total;
    uint16_t write_len;

    if (!s_audio_play_obj || !data || !len)
    {
        return;
    }

    if ((uint32_t)s_hf_spk_residual + len > sizeof(s_hf_spk_buf))
    {
        /* SCO frames are far smaller than the buffer; only defensive. */
        s_hf_spk_residual = 0;
        if (len > sizeof(s_hf_spk_buf))
        {
            LOGE("%s packet too large %d\n", __func__, len);
            return;
        }
    }

    os_memcpy(s_hf_spk_buf + s_hf_spk_residual, data, len);
    total = s_hf_spk_residual + len;

    /* audio_play expects whole 16-bit samples; defer one odd trailing byte. */
    write_len = total - (total % 2);

    if (write_len)
    {
        int size = audio_play_write_data(s_audio_play_obj, (char *)s_hf_spk_buf, write_len);
        if (size <= 0)
        {
            LOGE("%s audio_play_write_data err %d %d\n", __func__, size, write_len);
        }
    }

    s_hf_spk_residual = total - write_len;
    if (s_hf_spk_residual)
    {
        s_hf_spk_buf[0] = s_hf_spk_buf[write_len];
    }
#else
    (void)data;
    (void)len;
#endif
}

#ifdef CONFIG_AUDIO
static int mic_task_init()
{
    bk_err_t ret = BK_OK;
    if (!hf_mic_thread_handle)
    {
        ret = rtos_create_thread(&hf_mic_thread_handle,
                                 HF_MIC_THREAD_PRI,
                                 "bt_hf_mic",
                                 (beken_thread_function_t)mic_task,
                                 4096,
                                 (beken_thread_arg_t)0);
        if (ret != kNoErr)
        {
            LOGE("mic task fail \r\n");
        }

        return kNoErr;
    }
    else
    {
        LOGE("%s mic task already exist \r\n", __func__);
        return kInProgressErr;
    }

    return kNoErr;
}


static void mic_task(void *arg)
{
    int32_t ret = 0;
    uint8_t is_msbc = (CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec);

    audio_record_cfg_t cfg = DEFAULT_AUDIO_RECORD_CONFIG();

    cfg.nChans   = 1;
    cfg.sampRate = ((CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec) ? 16000 : 8000);
    cfg.bitsPerSample = 16;
    cfg.adc_gain = 16.0;
    cfg.frame_size = cfg.sampRate * cfg.nChans / 1000 * 20 * cfg.bitsPerSample / 8;
    cfg.pool_size  = cfg.frame_size * 2;
    cfg.encoder_type = (CODEC_VOICE_MSBC == bt_audio_hfp_hf_codec) ? AUDIO_RECORD_ENCODER_SBC : AUDIO_RECORD_ENCODER_PCM;
    cfg.ch_bitmap = ONBOARD_MIC_ADC_ACTIVE_CH_0_BIT;

#if HFP_UL_EQ_ENABLE && CONFIG_ADK_EQ_ALGORITHM
    {
        eq_algorithm_cfg_t eq_cfg = DEFAULT_EQ_ALGORITHM_CONFIG();
        eq_cfg.eq_mode       = EQ_MODE_SOFTWARE;
        eq_cfg.eq_chl_num    = cfg.nChans;
        eq_cfg.eq_frame_size = cfg.nChans ? (int)(cfg.frame_size / cfg.nChans) : (int)cfg.frame_size;
#if CONFIG_AUD_PARAM_CTRL
        cfg.eq_enable = bt_hfp_audio_ul_fill_eq(&eq_cfg.eq_cal_para, cfg.sampRate);
#else
        cfg.eq_enable = 1;
#endif
        cfg.eq_cfg    = eq_cfg;
    }
#endif

#if CONFIG_AUDIO_RECORD
    s_audio_record_obj = audio_record_create(AUDIO_RECORD_ONBOARD_MIC, &cfg);

    if(!s_audio_record_obj)
    {
        LOGE("%s create audio record err\n", __func__);
        goto end;
    }

    if((ret = audio_record_open(s_audio_record_obj)) != 0)
    {
        LOGE("%s open audio record err\n", __func__, ret);
        goto end;
    }
#if CONFIG_AUD_PARAM_CTRL
    bt_hfp_audio_ul_bind(s_audio_record_obj, cfg.sampRate);
#endif
#endif

    LOGI("%s init success!! \r\n", __func__);

    uint16_t pending = 0;
    uint16_t cvsd_send_unit = SCO_CVSD_SAMPLES_PER_FRAME * 2;

    /* Cap the per-read amount so we never burst many SCO packets at once and
     * overflow the controller's SCO TX pool:
     *  - mSBC uses a frame-buffer port that returns exactly one (H2 + payload)
     *    frame per read, so any cap >= one frame works.
     *  - CVSD is raw PCM read up to the requested length, so bound it to a couple
     *    of SCO frames (matches the legacy behaviour and keeps pacing). */
    int read_cap = is_msbc ? MSBC_SCO_PACKET_LEN : (cvsd_send_unit * 2);

    while (hf_auido_start)
    {
#if CONFIG_AUDIO_RECORD
        int read_len = audio_record_read_data(s_audio_record_obj,
                                              (char *)(hf_mic_sco_data + pending),
                                              read_cap - pending);
        if (read_len <= 0)
        {
            continue;
        }
        //LOGI("%s read len %d\n", __func__, read_len);
        if (is_msbc)
        {
            /* The encoder frame already carries a valid cycling H2 header; just
             * append one pad byte to build the fixed 60-byte mSBC SCO packet. */
            if (read_len + 1 <= (int)sizeof(hf_mic_sco_data))
            {
                hf_mic_sco_data[read_len] = 0;
                bk_bt_hf_client_voice_out_write(s_hfp_peer_addr, hf_mic_sco_data, read_len + 1);
            }
        }
        else
        {
            uint16_t off = 0;

            pending += read_len;
            while (pending - off >= cvsd_send_unit)
            {
                bk_bt_hf_client_voice_out_write(s_hfp_peer_addr, hf_mic_sco_data + off, cvsd_send_unit);
                off += cvsd_send_unit;
            }
            if (off)
            {
                pending -= off;
                if (pending)
                {
                    os_memmove(hf_mic_sco_data, hf_mic_sco_data + off, pending);
                }
            }
        }
#else
        break;
#endif
    }

end:
    LOGD("%s exit start!! \r\n", __func__);
#if CONFIG_AUD_PARAM_CTRL
    bt_hfp_audio_ul_unbind();
#endif
#if CONFIG_AUDIO_RECORD
    ret = audio_record_close(s_audio_record_obj);
    if(ret)
    {
        LOGE("%s close audio record err %d\n", __func__, ret);
    }

    ret = audio_record_destroy(s_audio_record_obj);

    if(ret)
    {
        LOGE("%s destroy audio record err %d\n", __func__, ret);
    }
#endif
    s_audio_record_obj = NULL;

    LOGI("%s end!! %d\r\n", __func__, hf_auido_start);

    if (hf_mic_speaker_exit_sema)
    {
        rtos_set_semaphore(&hf_mic_speaker_exit_sema);
    }

    rtos_delete_thread(NULL);
}

int32_t hfp_hf_audio_wait_player_end(void)
{
    while(hf_mic_thread_handle || s_audio_play_obj)
    {
        rtos_delay_milliseconds(20);
    }
    return 0;
}
#endif
