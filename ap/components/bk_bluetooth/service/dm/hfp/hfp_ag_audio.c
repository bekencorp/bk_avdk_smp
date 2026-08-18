/**
 * @file hfp_ag_audio.c
 *
 * HFP AG (Audio Gateway) SCO audio engine.
 *
 * Uses the 4.0.1 audio framework for codec handling (like hfp_hf_audio.c):
 *   - downlink (HF mic -> AG speaker): audio_play with decoder_type MSBC/PCM.
 *     The BT stack delivers SCO RX on the HCI read context via
 *     hfp_ag_audio_service_feed_rx(); that path MUST stay light, so it only
 *     copies the packet into a ring buffer and wakes the speaker task. The
 *     speaker task (not the BT context) does the blocking audio_play_write_data
 *     -- writing straight from the HCI context would stall SCO RX and garble
 *     the downlink audio.
 *   - uplink (AG mic -> HF): audio_record with encoder_type SBC/PCM; the
 *     recorder returns encoded frames sent over SCO with
 *     bk_bt_hf_ag_voice_out_write().
 *
 * No standalone SBC codec (bk_sbc_decoder_* / sbc_encoder_*) is used, so the
 * hardware SBC driver (CONFIG_SBC / sbc_ll_macro_def.h) is not required.
 */

#include <math.h>
#include <string.h>
#include <stdint.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>

#include "hfp_ag_audio.h"
#include <driver/aud_dac_types.h>
#include "audio_play.h"
#include "audio_record.h"
#include "ring_buffer_particle.h"
#include "components/bluetooth/bk_dm_hfp_ag.h"
#include "components/log.h"

#define TAG "hfp_ag_aud"

enum
{
    HFP_AG_AUD_DEBUG_LEVEL_ERROR,
    HFP_AG_AUD_DEBUG_LEVEL_WARNING,
    HFP_AG_AUD_DEBUG_LEVEL_INFO,
    HFP_AG_AUD_DEBUG_LEVEL_DEBUG,
    HFP_AG_AUD_DEBUG_LEVEL_VERBOSE,
};

#define HFP_AG_AUD_DEBUG_LEVEL HFP_AG_AUD_DEBUG_LEVEL_INFO

#define LOGE(format, ...) do{if(HFP_AG_AUD_DEBUG_LEVEL >= HFP_AG_AUD_DEBUG_LEVEL_ERROR)   BK_LOGE(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGW(format, ...) do{if(HFP_AG_AUD_DEBUG_LEVEL >= HFP_AG_AUD_DEBUG_LEVEL_WARNING) BK_LOGW(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGI(format, ...) do{if(HFP_AG_AUD_DEBUG_LEVEL >= HFP_AG_AUD_DEBUG_LEVEL_INFO)    BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGD(format, ...) do{if(HFP_AG_AUD_DEBUG_LEVEL >= HFP_AG_AUD_DEBUG_LEVEL_DEBUG)   BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define LOGV(format, ...) do{if(HFP_AG_AUD_DEBUG_LEVEL >= HFP_AG_AUD_DEBUG_LEVEL_VERBOSE) BK_LOGI(TAG, "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

#define AG_SPK_THREAD_PRI        (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
#define AG_MIC_THREAD_PRI        (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
#define AG_AUDIO_THREAD_STACK    (1024 * 4)

/* Serialize DAC(speaker)/ADC(mic) bring-up to avoid the shared-audio init race
 * that can intermittently kill the mic ADC. The speaker opens the DAC first,
 * then the mic waits for that before opening the ADC. Set 0 for concurrent. */
#define AG_SERIALIZE_MIC_SPK     1

#define SCO_CVSD_SAMPLES_PER_FRAME       (60)
#define MSBC_SCO_PACKET_LEN              (60)

/* Max encoded SCO frame bytes buffered per RX packet (mSBC air frame ~= 60). */
#define AG_SCO_RX_FRAME_MAX              (128)
#define AG_SPK_RB_SIZE                   (4096)

#define HFP_GAIN_MAX 15

/* HFP DAC digital-gain mapping curve (matches hfp_hf_audio.c). SCO source
 * levels are low; keep a small positive boost at max volume. cfg.volume and
 * audio_play_set_volume() take a dB float, NOT a raw 0..63 register value. */
#define HFP_DAC_DB_MIN                 (-36.0f)
#define HFP_DAC_DB_MAX                 (8.0f)
#define HFP_DAC_DB_SMOOTH_GAMMA        (2.35f)
#define HFP_MIC_ADC_GAIN_DB            (16.0f)

typedef struct
{
    uint8_t  peer_addr[6];
    volatile uint8_t audio_running;
#if AG_SERIALIZE_MIC_SPK
    volatile uint8_t spk_opened;   /* set by spk task after audio_play_open; mic task waits on it */
#endif
    bk_hf_codec_type_t audio_codec;
    uint16_t air_tx_packet_len;

    beken_thread_t     spk_thread;
    beken_thread_t     mic_thread;
    beken_semaphore_t  spk_sema;
    beken_semaphore_t  audio_exit_sema;   /* counts both spk + mic task exits */
    audio_record_t    *rec_obj;
    audio_play_t      *play_obj;

    ring_buffer_particle_ctx spk_rb;
    uint8_t            spk_rb_ready;

    uint8_t  hfp_mic_vol;
    uint8_t  hfp_spk_vol;
} hfp_ag_audio_ctx_t;

static hfp_ag_audio_ctx_t s_aud =
{
    .audio_codec = CODEC_VOICE_CVSD,
    .hfp_mic_vol = 12,
    .hfp_spk_vol = 12,
};

/* Map an HFP volume step (0..15) to a DAC digital gain in dB. */
static float ag_hfp_spk_vol_to_db(uint8_t hfp_spk_vol)
{
    float norm, shaped;

    if (hfp_spk_vol == 0)
    {
        return HFP_DAC_DB_MIN;
    }

    norm   = (float)hfp_spk_vol / (float)HFP_GAIN_MAX;
    shaped = powf(norm, HFP_DAC_DB_SMOOTH_GAMMA);
    return HFP_DAC_DB_MIN + (HFP_DAC_DB_MAX - HFP_DAC_DB_MIN) * shaped;
}

void hfp_ag_audio_service_set_spk_gain(uint8_t hfp_spk_vol)
{
    float gain_db;

    if (hfp_spk_vol > HFP_GAIN_MAX)
    {
        hfp_spk_vol = HFP_GAIN_MAX;
    }

    s_aud.hfp_spk_vol = hfp_spk_vol;

    if (!s_aud.play_obj)
    {
        return;
    }

    gain_db = ag_hfp_spk_vol_to_db(hfp_spk_vol);
    audio_play_set_volume(s_aud.play_obj, gain_db);
    audio_play_control(s_aud.play_obj, hfp_spk_vol == 0 ? AUDIO_PLAY_MUTE : AUDIO_PLAY_UNMUTE);
    LOGI("set spk vol %d -> %d dB", hfp_spk_vol, (int)gain_db);
}

void hfp_ag_audio_service_set_mic_gain(uint8_t hfp_mic_vol)
{
    if (hfp_mic_vol > HFP_GAIN_MAX)
    {
        hfp_mic_vol = HFP_GAIN_MAX;
    }

    s_aud.hfp_mic_vol = hfp_mic_vol;

    /* Like hfp_hf_audio.c, the mic ADC gain is fixed at create time
     * (HFP_MIC_ADC_GAIN_DB); the HF's +VGM request is only tracked here. */
    LOGI("set mic vol %d (adc fixed %d dB)", hfp_mic_vol, (int)HFP_MIC_ADC_GAIN_DB);
}

/* Downlink RX, runs on the HCI SCO read context: keep it light. Only copy the
 * packet into the ring buffer and wake the speaker task; NEVER call the
 * (blocking) audio_play_write_data from here, or SCO RX stalls and garbles. */
void hfp_ag_audio_service_feed_rx(const uint8_t *buf, uint32_t len)
{
    if (!s_aud.audio_running || !s_aud.spk_rb_ready || !buf || !len)
    {
        return;
    }

#if 1// CONFIG_BLUETOOTH_BTDM_COMPONENT_BT_HF_RESET_AUDIO_DATA_WHEN_ERR
    uint8_t *raw_data = ((uint8_t *)buf) - 3;
    uint8_t flag = ((raw_data[1] >> 4) & 0b11);
    const char *meanings[] =
    {
        "ok",
        "possible invalid",
        "no data",
        "partially lost"
    };

    if (flag != 0)
    {
        LOGW("esco flag %d %s. codec %d !!!", flag, meanings[flag], s_aud.audio_codec);
    }

    if (CODEC_VOICE_CVSD == s_aud.audio_codec && flag)
    {
        os_memset((void *)buf, 0, len);
    }

#endif

    if (s_aud.audio_codec == CODEC_VOICE_MSBC)
    {
        uint8_t frame[2 + AG_SCO_RX_FRAME_MAX];

        if (len > AG_SCO_RX_FRAME_MAX)
        {
            return;
        }

        /* length-prefix so the speaker task can feed whole frames to the decoder */
        frame[0] = (uint8_t)(len & 0xffu);
        frame[1] = (uint8_t)((len >> 8) & 0xffu);
        os_memcpy(frame + 2, buf, len);
        (void)ring_buffer_particle_write(&s_aud.spk_rb, frame, (uint32_t)(2u + len));
    }
    else
    {
        (void)ring_buffer_particle_write(&s_aud.spk_rb, (uint8_t *)buf, len);
    }

    if (s_aud.spk_sema)
    {
        rtos_set_semaphore(&s_aud.spk_sema);
    }
}

/* Speaker (downlink) task: drains the ring buffer and does the blocking
 * audio_play write off the BT context. Owns the audio_play object lifecycle. */
static void ag_spk_task(void *arg)
{
    static uint8_t tmp[1024 * 2];
    uint32_t got;

    audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();

    (void)arg;

    cfg.nChans       = 1;
    cfg.sampRate     = (s_aud.audio_codec == CODEC_VOICE_MSBC) ? 16000 : 8000;
    cfg.volume       = ag_hfp_spk_vol_to_db(s_aud.hfp_spk_vol);
    cfg.frame_size   = cfg.sampRate * cfg.nChans / 1000 * 20 * cfg.bitsPerSample / 8;
    cfg.pool_size    = cfg.frame_size * 2;
    cfg.decoder_type = (s_aud.audio_codec == CODEC_VOICE_MSBC) ? AUDIO_PLAY_DECODER_MSBC : AUDIO_PLAY_DECODER_PCM;
    /* Route to the CALL DAC source: the default A2DP source is fixed to 44.1/48k
     * and rejects the 8k/16k SCO rate ("A2DP 8k is not supported"). */
    cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT;
    cfg.main_dac_source   = AUD_DAC_SOURCE_CALL;

    s_aud.play_obj = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);
    if (!s_aud.play_obj || audio_play_open(s_aud.play_obj) != 0)
    {
        LOGE("intercom speaker open fail");
        if (s_aud.play_obj)
        {
            audio_play_destroy(s_aud.play_obj);
            s_aud.play_obj = NULL;
        }
        goto end;
    }

    LOGI("intercom speaker started (%dHz)", (int)cfg.sampRate);

#if AG_SERIALIZE_MIC_SPK
    /* DAC is up now; let the mic task proceed to bring up the ADC. */
    s_aud.spk_opened = 1;
#endif

    while (s_aud.audio_running)
    {
        if (s_aud.spk_sema)
        {
            rtos_get_semaphore(&s_aud.spk_sema, BEKEN_WAIT_FOREVER);
        }

        if (!s_aud.audio_running)
        {
            break;
        }

        if (s_aud.audio_codec == CODEC_VOICE_MSBC)
        {
            /* Each buffered item is [len:2][mSBC frame]; feed whole frames to the
             * pipeline mSBC decoder. */
            while (ring_buffer_particle_len(&s_aud.spk_rb) >= 2)
            {
                uint8_t  hdr[2];
                uint16_t enc;

                got = 0;
                ring_buffer_particle_read(&s_aud.spk_rb, hdr, sizeof(hdr), &got);
                if (got < 2)
                {
                    break;
                }

                enc = (uint16_t)(hdr[0] | (hdr[1] << 8));
                if (enc == 0 || enc > sizeof(tmp))
                {
                    break;
                }

                got = 0;
                ring_buffer_particle_read(&s_aud.spk_rb, tmp, enc, &got);
                if (got < enc)
                {
                    break;
                }

                (void)audio_play_write_data(s_aud.play_obj, (char *)tmp, enc);
            }
        }
        else
        {
            while (ring_buffer_particle_len(&s_aud.spk_rb) >= 2)
            {
                got = 0;
                ring_buffer_particle_read(&s_aud.spk_rb, tmp, sizeof(tmp), &got);
                got -= (got % 2);
                if (got == 0)
                {
                    break;
                }

                if (audio_play_write_data(s_aud.play_obj, (char *)tmp, got) <= 0)
                {
                    break;
                }
            }
        }
    }

end:
    if (s_aud.play_obj)
    {
        audio_play_close(s_aud.play_obj);
        audio_play_destroy(s_aud.play_obj);
        s_aud.play_obj = NULL;
    }

    LOGI("intercom speaker end");

    if (s_aud.audio_exit_sema)
    {
        rtos_set_semaphore(&s_aud.audio_exit_sema);
    }

    rtos_delete_thread(NULL);
}

/* Uplink (mic) task: AG mic -> HF over SCO. The recorder encodes mSBC internally. */
static void ag_mic_task(void *arg)
{
    static uint8_t mic_buf[1024];
    uint8_t is_msbc = (CODEC_VOICE_MSBC == s_aud.audio_codec);
    uint16_t pending = 0;
    uint16_t cvsd_send_unit = SCO_CVSD_SAMPLES_PER_FRAME * 2;
    int read_cap;

    audio_record_cfg_t cfg = DEFAULT_AUDIO_RECORD_CONFIG();

    (void)arg;

#if AG_SERIALIZE_MIC_SPK
    /* Wait for the speaker DAC to come up before opening the mic ADC, so the two
     * never init the shared audio subsystem concurrently. Bounded so a speaker
     * open failure cannot hang the mic forever. */
    {
        int wait_ms = 0;

        while (s_aud.audio_running && !s_aud.spk_opened && wait_ms < 3000)
        {
            rtos_delay_milliseconds(5);
            wait_ms += 5;
        }

        LOGI("mic: spk_opened=%d after %dms", s_aud.spk_opened, wait_ms);
    }
#endif

    cfg.nChans        = 1;
    cfg.sampRate      = is_msbc ? 16000 : 8000;
    cfg.bitsPerSample = 16;
    cfg.adc_gain      = HFP_MIC_ADC_GAIN_DB;
    cfg.frame_size    = cfg.sampRate * cfg.nChans / 1000 * 20 * cfg.bitsPerSample / 8;
    cfg.pool_size     = cfg.frame_size * 2;
    cfg.encoder_type  = is_msbc ? AUDIO_RECORD_ENCODER_SBC : AUDIO_RECORD_ENCODER_PCM;
    cfg.ch_bitmap     = ONBOARD_MIC_ADC_ACTIVE_CH_0_BIT;

    s_aud.rec_obj = audio_record_create(AUDIO_RECORD_ONBOARD_MIC, &cfg);
    if (!s_aud.rec_obj || audio_record_open(s_aud.rec_obj) != 0)
    {
        LOGE("intercom mic open fail");
        goto end;
    }

    LOGI("intercom mic started (%dHz) %s", (int)cfg.sampRate, is_msbc ? "mSBC" : "CVSD");

    read_cap = is_msbc ? MSBC_SCO_PACKET_LEN : (cvsd_send_unit * 2);

    while (s_aud.audio_running)
    {
        int read_len = audio_record_read_data(s_aud.rec_obj,
                                               (char *)(mic_buf + pending),
                                               read_cap - pending);
        if (read_len <= 0)
        {
            continue;
        }

        if (is_msbc)
        {
            /* recorder frame already carries the cycling H2 header; append one
             * pad byte to build the fixed 60-byte mSBC SCO packet. */
            if (read_len + 1 <= (int)sizeof(mic_buf))
            {
                mic_buf[read_len] = 0;
                bk_bt_hf_ag_voice_out_write(s_aud.peer_addr, mic_buf, read_len + 1);
            }
        }
        else
        {
            uint16_t off = 0;

            pending += (uint16_t)read_len;
            while (pending - off >= cvsd_send_unit)
            {
                bk_bt_hf_ag_voice_out_write(s_aud.peer_addr, mic_buf + off, cvsd_send_unit);
                off += cvsd_send_unit;
            }
            if (off)
            {
                pending -= off;
                if (pending)
                {
                    os_memmove(mic_buf, mic_buf + off, pending);
                }
            }
        }
    }

end:
    if (s_aud.rec_obj)
    {
        audio_record_close(s_aud.rec_obj);
        audio_record_destroy(s_aud.rec_obj);
        s_aud.rec_obj = NULL;
    }

    LOGI("intercom mic end");

    if (s_aud.audio_exit_sema)
    {
        rtos_set_semaphore(&s_aud.audio_exit_sema);
    }

    rtos_delete_thread(NULL);
}

void hfp_ag_audio_service_start(bk_hf_codec_type_t codec, const uint8_t *peer_addr, uint16_t air_tx_len)
{
    if (s_aud.audio_running)
    {
        return;
    }

    if (peer_addr)
    {
        os_memcpy(s_aud.peer_addr, peer_addr, sizeof(s_aud.peer_addr));
    }

    s_aud.audio_codec = (codec == CODEC_VOICE_MSBC) ? CODEC_VOICE_MSBC : CODEC_VOICE_CVSD;
    s_aud.air_tx_packet_len = air_tx_len;

    if (ring_buffer_particle_init(&s_aud.spk_rb, AG_SPK_RB_SIZE) < 0)
    {
        LOGE("spk ring buffer init fail");
        return;
    }
    s_aud.spk_rb_ready = 1;

    if (rtos_init_semaphore(&s_aud.spk_sema, 1) != kNoErr ||
            rtos_init_semaphore(&s_aud.audio_exit_sema, 2) != kNoErr)
    {
        LOGE("audio sema init fail");
    }

    s_aud.audio_running = 1;
#if AG_SERIALIZE_MIC_SPK
    s_aud.spk_opened = 0;
#endif

    /* Heavy audio bring-up + the blocking player write run on these dedicated
     * tasks, never on the small-stack BT callback thread. */
    rtos_create_thread(&s_aud.spk_thread, AG_SPK_THREAD_PRI, "ag_spk",
                       (beken_thread_function_t)ag_spk_task, AG_AUDIO_THREAD_STACK, (beken_thread_arg_t)0);
    rtos_create_thread(&s_aud.mic_thread, AG_MIC_THREAD_PRI, "ag_mic",
                       (beken_thread_function_t)ag_mic_task, AG_AUDIO_THREAD_STACK, (beken_thread_arg_t)0);

    LOGI("intercom start codec %d", s_aud.audio_codec);
}

void hfp_ag_audio_service_stop(void)
{
    if (!s_aud.audio_running && !s_aud.spk_thread && !s_aud.mic_thread)
    {
        return;
    }

    s_aud.audio_running = 0;

    /* wake the speaker task out of its semaphore wait so it observes the stop */
    if (s_aud.spk_sema)
    {
        rtos_set_semaphore(&s_aud.spk_sema);
    }

    /* wait for both spk + mic tasks to exit (they close their audio objects) */
    if (s_aud.audio_exit_sema)
    {
        rtos_get_semaphore(&s_aud.audio_exit_sema, BEKEN_WAIT_FOREVER);
        rtos_get_semaphore(&s_aud.audio_exit_sema, BEKEN_WAIT_FOREVER);
        rtos_deinit_semaphore(&s_aud.audio_exit_sema);
        s_aud.audio_exit_sema = NULL;
    }

    s_aud.spk_thread = NULL;
    s_aud.mic_thread = NULL;

    if (s_aud.spk_sema)
    {
        rtos_deinit_semaphore(&s_aud.spk_sema);
        s_aud.spk_sema = NULL;
    }

    s_aud.spk_rb_ready = 0;
    ring_buffer_particle_deinit(&s_aud.spk_rb);

    LOGI("intercom stop");
}

uint8_t hfp_ag_audio_service_is_running(void)
{
    return s_aud.audio_running;
}
