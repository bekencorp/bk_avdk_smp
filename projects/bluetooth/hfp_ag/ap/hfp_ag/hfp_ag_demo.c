/**
 * @file hfp_ag_demo.c
 *
 * HFP AG (Audio Gateway) demo - control plane.
 *
 * The AG plays the "phone/gateway" role: an HF unit (headset/car-kit) connects
 * to it, and the AG reports call & network status and reacts to the HF's AT
 * requests. This demo drives the public components/bluetooth/bk_dm_hfp_ag.h API.
 * In hfp_ag_v3 the bridge lives inside the SDK (ble_pub/ui/bt_hfp_ag_*),
 * dispatched on ui_ethermind_ctx_thread; the demo is a pure consumer of the
 * public API.
 */

#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "hfp_ag_demo.h"
#include "components/bluetooth/bk_dm_hfp_ag.h"

#include "audio_record.h"
#include "audio_play.h"
#include "ring_buffer_particle.h"
#include "bt_manager.h"

#include <driver/sbc_types.h>
#include <driver/sbc.h>
#include "modules/sbc_encoder.h"
#include "gpio_driver.h"
#include <driver/gpio.h>
#include "bluetooth_user_config.h"

#define LOG_TAG "hfpag_app"

#define AG_MSBC_SAMPLES_PER_FRAME       (8 * 15) // subband * blocks
#define AG_MSBC_FRAME_BYTES             (AG_MSBC_SAMPLES_PER_FRAME * 2)   /* 240 */
#define MSBC_EXPECT_FRAME_LEN           (4 + ((4 * 8 * 1) >> 3) + ((15 * 26 + 7) >> 3)) //without header

/* Max encoded SCO frame bytes buffered per RX packet (mSBC air frame ~= 60). */
#define AG_SCO_RX_FRAME_MAX             128

#define AG_MIC_THREAD_PRI               (BEKEN_DEFAULT_WORKER_PRIORITY - 1)
#define AG_SPK_THREAD_PRI               (BEKEN_DEFAULT_WORKER_PRIORITY - 1)

/* fix: serialize DAC(speaker)/ADC(mic) bring-up to avoid the shared-audio init race that
   intermittently kills the mic ADC (constant -32614). Speaker opens the DAC first, then the
   mic opens the ADC. Set 0 to restore the original concurrent bring-up. */
#define AG_SERIALIZE_MIC_SPK            1


#define PLATFORM_SPK_GAIN_MAX 0x3f
#define PLATFORM_MIC_GAIN_MAX 0x3f
#define HFP_GAIN_MAX 15


typedef struct
{
    uint8_t  peer_addr[6];
    uint8_t  peer_valid;
    uint8_t  slc_ok;
    uint8_t  auto_intercom;            /* start the intercom automatically once SLC is up */

    int      num_active;
    int      num_held;
    char     call_number[24];

    volatile uint8_t audio_running;
#if AG_SERIALIZE_MIC_SPK
    volatile uint8_t ag_spk_opened;    /* set by ag_speaker_task after DAC open; ag_mic_task waits on it */
#endif
    bk_hf_codec_type_t audio_codec;    /* CODEC_VOICE_CVSD (8k) or CODEC_VOICE_MSBC (16k) */
    beken_thread_t     mic_thread;
    beken_thread_t     spk_thread;
    beken_semaphore_t  spk_sema;
    beken_semaphore_t  audio_exit_sema;
    audio_record_t    *rec_obj;
    audio_play_t      *play_obj;
    ring_buffer_particle_ctx spk_rb;
    uint8_t            spk_rb_ready;

    sbcdecodercontext_t sbc_dec;
    SbcEncoderContext   sbc_enc;

    uint16_t air_tx_packet_len;
    uint8_t  hfp_mic_vol;
    uint8_t  hfp_spk_vol;

    uint8_t  signal;                   /* +CIND signal strength, 0-5 */
    uint8_t  batt_lev;                 /* +CIND battery charge, 0-5 */

    uint8_t  hf_batt_level;            /* HF battery normalized to 0-100% (Apple IPHONEACCEV / HFP BIEV) */
    uint8_t  hf_docked;                /* HF (Apple) dock/charge state from AT+IPHONEACCEV */
    uint8_t  xapl_feat;                /* HF (Apple) feature bitmask from AT+XAPL */

    uint8_t  hf_enhanced_safety;       /* Standard HFP Enhanced Safety indicator, 0-1 */
} hfp_ag_ctx_t;

/* All demo state lives in one context object (was ~20 file-scope globals). */
static hfp_ag_ctx_t s_ag =
{
    .audio_codec = CODEC_VOICE_CVSD,
    .hfp_mic_vol = 12,
    .hfp_spk_vol = 12,
    .signal      = 5,
    .batt_lev    = 5,
};

static void demo_set_peer(const uint8_t *addr)
{
    if (addr)
    {
        os_memcpy(s_ag.peer_addr, addr, sizeof(s_ag.peer_addr));
        s_ag.peer_valid = 1;
    }
}

static uint32_t ag_sample_rate(void)
{
    return (s_ag.audio_codec == CODEC_VOICE_MSBC) ? 16000 : 8000;
}

static uint8_t ag_hfp_mic_vol_to_gain(uint8_t hfp_mic_vol)
{
    return ((PLATFORM_MIC_GAIN_MAX + 1) * 1.0 / (HFP_GAIN_MAX + 1)) * hfp_mic_vol;
}

static uint8_t ag_hfp_spk_vol_to_gain(uint8_t hfp_spk_vol)
{
    return ((PLATFORM_SPK_GAIN_MAX + 1) * 1.0 / (HFP_GAIN_MAX + 1)) * hfp_spk_vol;
}

static bk_err_t ag_dac_set_gain(uint8_t *hfp_mic_vol, uint8_t *hfp_spk_vol)
{
    uint8_t gain = 0;

    if (hfp_spk_vol && s_ag.play_obj)
    {
        if (*hfp_spk_vol > HFP_GAIN_MAX)
        {
            *hfp_spk_vol = HFP_GAIN_MAX;
        }

        s_ag.hfp_spk_vol = *hfp_spk_vol;

        gain = ag_hfp_spk_vol_to_gain(*hfp_spk_vol);

        audio_play_set_volume(s_ag.play_obj, gain);

        if (gain == 0)
        {
            audio_play_control(s_ag.play_obj, AUDIO_PLAY_MUTE);
        }
        else
        {
            audio_play_control(s_ag.play_obj, AUDIO_PLAY_UNMUTE);
        }

        LOGI("%s set spk gain 0x%x", gain);
    }

    if (hfp_mic_vol && s_ag.rec_obj)
    {
        if (*hfp_mic_vol > HFP_GAIN_MAX)
        {
            *hfp_mic_vol = HFP_GAIN_MAX;
        }

        s_ag.hfp_mic_vol = *hfp_mic_vol;

        gain = ag_hfp_mic_vol_to_gain(*hfp_mic_vol);

        audio_play_set_adc_gain(s_ag.rec_obj, gain); // audio_record_set_adc_gain

        if (gain == 0)
        {
            audio_record_control(s_ag.rec_obj, AUDIO_RECORD_PAUSE);
        }
        else
        {
            audio_record_control(s_ag.rec_obj, AUDIO_RECORD_RESUME);
        }

        LOGI("set mic gain 0x%x", gain);
    }

    return BK_OK;
}

static void ag_intercom_recv_cb(const uint8_t *buf, uint32_t len)
{
    if (!s_ag.audio_running || !s_ag.spk_rb_ready)
    {
        return;
    }

#if HFP_RESET_AUDIO_DATA_WHEN_ERR
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
        LOGW("esco flag %d codec %d !!!", flag, s_ag.audio_codec);
    }

    if (CODEC_VOICE_CVSD == s_ag.audio_codec && flag)
    {
        os_memset((void *)buf, 0, len);
    }

#endif

    if (s_ag.audio_codec == CODEC_VOICE_MSBC)
    {
        uint8_t  frame[2 + AG_SCO_RX_FRAME_MAX];

        frame[0] = (uint8_t)(len & 0xffu);
        frame[1] = (uint8_t)((len >> 8) & 0xffu);
        os_memcpy(frame + 2, buf, len);
        (void)ring_buffer_particle_write(&s_ag.spk_rb, frame, (uint32_t)(2u + len));
    }
    else
    {
        (void)ring_buffer_particle_write(&s_ag.spk_rb, (uint8_t *)buf, len);
    }

    if (s_ag.spk_sema)
    {
        rtos_set_semaphore(&s_ag.spk_sema);
    }
}

static bk_err_t bk_sbc_frame_length_parse(uint8_t *buf, uint16_t len)
{
    // Check input parameters
    if (buf == NULL || len < 3)
    {
        LOGE("Invalid input parameters\n");
        return BK_FAIL;
    }

    uint8_t syncword = buf[0];
    uint8_t channel_mode = 0;
    uint8_t blocks = 0;
    uint8_t subbands = 0;
    uint8_t bitpool = 0;
    uint8_t num_channels = 1;
    int32_t calculated_frame_len = 0;

    // Determine whether it is an SBC frame or an mSBC frame
    if (syncword == 0x9C)    // SBC frame sync word
    {
        // Parse SBC frame header information
        channel_mode = (buf[1] >> 2) & 0x03;
        blocks = (((buf[1] >> 4) & 0x03) + 1) << 2; // 4, 8, 12, 16
        subbands = ((buf[1] & 0x01) + 1) << 2; // 4 or 8
        bitpool = buf[2];

        // Determine the number of channels based on the channel mode
        num_channels = (channel_mode == 0) ? 1 : 2; // 0 indicates MONO mode
    }
    else if (syncword == 0xAD)    // mSBC frame sync word (according to common definition)
    {
        // mSBC usually has a fixed configuration
        blocks = 15;
        subbands = 8;
        bitpool = 26;
        num_channels = 1;
        channel_mode = 0; // MONO mode

        // For some extended mSBC formats, they may contain additional configuration information
        if (buf[1] != 0 || buf[2] != 0)
        {
            channel_mode = (buf[1] >> 2) & 0x03;
            subbands = ((buf[1] & 0x01) + 1) << 2;
            bitpool = buf[2];
            num_channels = (channel_mode == 0) ? 1 : 2;
            LOGW("%s spec msbc 0x%02x%02x !!!\n", __func__, buf[1], buf[2]);
        }
    }
    else
    {
        // Not a valid SBC or mSBC frame header
        LOGE("Invalid syncword: 0x%x\n", syncword);
        return BK_FAIL;
    }

    // Check if the bitpool is out of range
    if (((channel_mode == 0 || channel_mode == 1) && (bitpool > (subbands << 4))) ||
            ((channel_mode == 2 || channel_mode == 3) && (bitpool > (subbands << 5))))
    {
        LOGE("Bitpool out of bounds: %d\n", bitpool);
        return BK_FAIL;
    }

    // Calculate the frame length
    calculated_frame_len = 4 + ((4 * subbands * num_channels) >> 3);

    if (channel_mode == 0 || channel_mode == 1)    // MONO or DUAL_CHANNEL
    {
        calculated_frame_len += ((blocks * num_channels * bitpool) + 7) >> 3;
    }
    else    // STEREO or JOINT_STEREO
    {
        if (channel_mode == 3)    // JOINT_STEREO
        {
            calculated_frame_len += (subbands + blocks * bitpool + 7) >> 3;
        }
        else    // STEREO
        {
            calculated_frame_len += (blocks * bitpool + 7) >> 3;
        }
    }

    // Check if the calculated frame length exceeds the input data length
    if (calculated_frame_len > len)
    {
        LOGE("Frame length %d exceeds buffer length %d\n", calculated_frame_len, len);
        return BK_FAIL;
    }

    // Return the calculated frame length
    return calculated_frame_len;
}

static void ag_speaker_task(void *arg)
{
    audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();
    static uint8_t tmp[1024 * 2];
    uint32_t got;

    (void)arg;
    cfg.nChans     = 1;
    cfg.sampRate   = ag_sample_rate();
    cfg.volume     = ag_hfp_spk_vol_to_gain(s_ag.hfp_spk_vol);
    cfg.frame_size = cfg.sampRate / 1000 * 20 * 2;   /* 20 ms, 16-bit mono */
    cfg.pool_size  = cfg.frame_size * 2;

    s_ag.play_obj = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);

    if (!s_ag.play_obj || audio_play_open(s_ag.play_obj) != 0)
    {
        LOGE("intercom speaker open fail");
        goto end;
    }

#if AG_SERIALIZE_MIC_SPK
    /* DAC up now; let the mic proceed to bring up the ADC (serialized, no shared-audio race). */
    s_ag.ag_spk_opened = 1;
#endif

    LOGI("intercom speaker started (%dHz)", (int)cfg.sampRate);

    while (s_ag.audio_running)
    {
        if (s_ag.spk_sema)
        {
            rtos_get_semaphore(&s_ag.spk_sema, BEKEN_WAIT_FOREVER);
        }

        if (s_ag.audio_codec == CODEC_VOICE_MSBC)
        {
            /* Each buffered frame is [len:2][encoded mSBC]; recover the boundary
             * from the length prefix, then decode (kept off the HCI RX context).
             * recv_cb writes header+payload atomically, so once the 2-byte header
             * is present the whole frame is present. */
            while (ring_buffer_particle_len(&s_ag.spk_rb) >= 2)
            {
                uint8_t  hdr[2];
                uint16_t enc;
                uint8_t frame_bytes1 = 0;

                got = 0;
                ring_buffer_particle_read(&s_ag.spk_rb, hdr, sizeof(hdr), &got);

                if (got < 2)
                {
                    break;
                }

                enc = (uint16_t)(hdr[0] | (hdr[1] << 8));

                if (enc == 0 || enc > sizeof(tmp))
                {
                    break;   /* unexpected; stop draining this pass */
                }

                got = 0;
                ring_buffer_particle_read(&s_ag.spk_rb, tmp, enc, &got);

                if (got < enc)
                {
                    break;
                }

                int16_t frame_len = 0;

                if (tmp[2] != 0xad)
                {
                    LOGE("msbc frame sync word error 0x%02x", tmp[2]);
                }
                else
                {
                    frame_len = bk_sbc_frame_length_parse(tmp + 2, enc - 2);

                    if (frame_len != MSBC_EXPECT_FRAME_LEN)
                    {
                        LOGE("msbc frame length error %d", frame_len);
                    }
                    else if (bk_sbc_decoder_frame_decode(&s_ag.sbc_dec, tmp + 2, (int32_t)enc - 2) >= 0)
                    {
                        (void)audio_play_write_data(s_ag.play_obj, (char *)s_ag.sbc_dec.pcm_sample, AG_MSBC_FRAME_BYTES);
                    }
                }
            }
        }
        else
        {
            while (ring_buffer_particle_len(&s_ag.spk_rb) >= 2)
            {
                got = 0;
                ring_buffer_particle_read(&s_ag.spk_rb, tmp, sizeof(tmp), &got);
                got -= (got % 2);

                if (got == 0)
                {
                    break;
                }

                if (audio_play_write_data(s_ag.play_obj, (char *)tmp, got) <= 0)
                {
                    break;
                }
            }
        }
    }

end:

    if (s_ag.play_obj)
    {
        audio_play_close(s_ag.play_obj);
        audio_play_destroy(s_ag.play_obj);
        s_ag.play_obj = NULL;
    }

    LOGI("intercom speaker end");

    if (s_ag.audio_exit_sema)
    {
        rtos_set_semaphore(&s_ag.audio_exit_sema);
    }

    rtos_delete_thread(NULL);
}

static void ag_mic_task(void *arg)
{
    audio_record_cfg_t cfg = DEFAULT_AUDIO_RECORD_CONFIG();
    static uint8_t mic_buf[1024 * 4];
    uint16_t fill = 0;
    uint16_t frame_bytes = 0;
    int read_size = 0;
    uint8_t last_pcm_data[AG_MSBC_FRAME_BYTES];

    if (s_ag.audio_codec == CODEC_VOICE_MSBC)
    {
        frame_bytes = AG_MSBC_FRAME_BYTES;
        read_size = frame_bytes * 1;
        cfg.frame_size = frame_bytes;
        cfg.pool_size = frame_bytes * 2;
    }
    else if (s_ag.audio_codec == CODEC_VOICE_CVSD)
    {
        frame_bytes = s_ag.air_tx_packet_len * 16 / 8; // air frame bytes -> * 16 convert to PCM bytes -> / 8 downsample to 8k
        read_size = frame_bytes * 2;
    }

    (void)arg;
    cfg.nChans   = 1;
    cfg.sampRate = ag_sample_rate();
    cfg.adc_gain = ag_hfp_mic_vol_to_gain(s_ag.hfp_mic_vol);

#if AG_SERIALIZE_MIC_SPK
    /* wait until the speaker DAC is up before bringing up the mic ADC, so the two do not init
       the shared audio subsystem concurrently (race -> dead ADC / constant -32614). Bounded
       wait so a speaker-open failure can't hang the mic forever. */
    {
        int wait_ms = 0;

        while (s_ag.audio_running && !s_ag.ag_spk_opened && wait_ms < 3000)
        {
            rtos_delay_milliseconds(5);
            wait_ms += 5;
        }

        LOGI("intercom mic: spk_opened=%d after %dms", s_ag.ag_spk_opened, wait_ms);
    }
#endif

    s_ag.rec_obj = audio_record_create(AUDIO_RECORD_ONBOARD_MIC, &cfg);

    if (!s_ag.rec_obj || audio_record_open(s_ag.rec_obj) != 0)
    {
        LOGE("intercom mic open fail");
        goto end;
    }

    LOGI("intercom mic started (%dHz)", (int)cfg.sampRate);

    while (s_ag.audio_running)
    {
        int size;

        if (fill + read_size > sizeof(mic_buf))
        {
            fill = 0;   /* safety: should not happen */
        }

        size = audio_record_read_data(s_ag.rec_obj, (char *)(mic_buf + fill), read_size);

        if (size > 0)
        {
            uint16_t off = 0;
            fill += (uint16_t)size;

            while ((fill - off) >= frame_bytes)
            {
                if (os_memcmp(mic_buf + off, last_pcm_data, frame_bytes) != 0)
                {
                    os_memcpy(last_pcm_data, mic_buf + off, frame_bytes);
                }
                else
                {
                    LOGE("pcm data repeat %d", frame_bytes);
                }

                if (s_ag.audio_codec == CODEC_VOICE_MSBC)
                {
                    /* SBC-encode one 120-sample frame; enc.stream[-2..] carries
                     * the 2-byte sync header expected on air. */
                    int32_t produced = sbc_encoder_encode(&s_ag.sbc_enc, (int16_t *)(mic_buf + off));

                    if (produced > 0)
                    {
                        uint16_t final_send = produced + 2;
                        LOGV("msbc produced len %d, first 2 bytes 0x%02x%02x before 0x%02x%02x", produced,
                             s_ag.sbc_enc.stream[0], s_ag.sbc_enc.stream[1],
                             s_ag.sbc_enc.stream[- 2], s_ag.sbc_enc.stream[- 1]);

                        if (produced != MSBC_EXPECT_FRAME_LEN)
                        {
                            LOGE("msbc produced len %d, not equal to %d", produced, MSBC_EXPECT_FRAME_LEN);
                        }

                        if (s_ag.air_tx_packet_len > final_send)
                        {
                            final_send = s_ag.air_tx_packet_len;
                            s_ag.sbc_enc.stream[MSBC_EXPECT_FRAME_LEN] = 0;

                            for (size_t i = 0; i < MSBC_EXPECT_FRAME_LEN; i++)
                            {
                                s_ag.sbc_enc.stream[MSBC_EXPECT_FRAME_LEN] += s_ag.sbc_enc.stream[i];
                            }
                        }

                        bk_bt_hf_ag_voice_out_write(s_ag.peer_addr,
                                                    (uint8_t *)&s_ag.sbc_enc.stream[-2],
                                                    final_send);
                    }
                }
                else
                {
                    bk_bt_hf_ag_voice_out_write(s_ag.peer_addr, mic_buf + off, frame_bytes);
                }

                off += frame_bytes;
            }

            if (off && (fill - off))
            {
                os_memmove(mic_buf, mic_buf + off, fill - off);
            }

            fill -= off;
            os_memset(mic_buf + fill, 0, sizeof(mic_buf) - fill);
        }
    }

end:

    if (s_ag.rec_obj)
    {
        audio_record_close(s_ag.rec_obj);
        audio_record_destroy(s_ag.rec_obj);
        s_ag.rec_obj = NULL;
    }

    LOGI("intercom mic end");

    if (s_ag.audio_exit_sema)
    {
        rtos_set_semaphore(&s_ag.audio_exit_sema);
    }

    rtos_delete_thread(NULL);
}

static void ag_intercom_start(bk_hf_codec_type_t codec)
{
    if (s_ag.audio_running)
    {
        return;
    }

    s_ag.audio_codec = (codec == CODEC_VOICE_MSBC) ? CODEC_VOICE_MSBC : CODEC_VOICE_CVSD;

    if (s_ag.audio_codec == CODEC_VOICE_MSBC)
    {
        bk_sbc_decoder_init(&s_ag.sbc_dec);
        sbc_encoder_init(&s_ag.sbc_enc, 16000, 1);
        sbc_encoder_ctrl(&s_ag.sbc_enc, SBC_ENCODER_CTRL_CMD_SET_MSBC_ENCODE_MODE, (uint32_t)0);
    }

    if (ring_buffer_particle_init(&s_ag.spk_rb, 4096) < 0)
    {
        LOGE("intercom rb init fail");
        return;
    }

    s_ag.spk_rb_ready = 1;

    if (rtos_init_semaphore(&s_ag.spk_sema, 1) != kNoErr ||
            rtos_init_semaphore(&s_ag.audio_exit_sema, 2) != kNoErr)
    {
        LOGE("intercom sema init fail");
    }

    s_ag.audio_running = 1;
#if AG_SERIALIZE_MIC_SPK
    s_ag.ag_spk_opened = 0;
#endif
    rtos_create_thread(&s_ag.spk_thread, AG_SPK_THREAD_PRI, "ag_spk",
                       (beken_thread_function_t)ag_speaker_task, 4096, (beken_thread_arg_t)0);
    rtos_create_thread(&s_ag.mic_thread, AG_MIC_THREAD_PRI, "ag_mic",
                       (beken_thread_function_t)ag_mic_task, 4096, (beken_thread_arg_t)0);
    LOGI("intercom start codec %d", s_ag.audio_codec);
}

static void ag_intercom_stop(void)
{
    if (!s_ag.audio_running)
    {
        return;
    }

    s_ag.audio_running = 0;

    /* wake the speaker task so it can observe the stop flag */
    if (s_ag.spk_sema)
    {
        rtos_set_semaphore(&s_ag.spk_sema);
    }

    /* wait for both mic + speaker tasks to exit */
    if (s_ag.audio_exit_sema)
    {
        rtos_get_semaphore(&s_ag.audio_exit_sema, BEKEN_WAIT_FOREVER);
        rtos_get_semaphore(&s_ag.audio_exit_sema, BEKEN_WAIT_FOREVER);
        rtos_deinit_semaphore(&s_ag.audio_exit_sema);
        s_ag.audio_exit_sema = NULL;
    }

    s_ag.mic_thread = NULL;
    s_ag.spk_thread = NULL;

    if (s_ag.spk_sema)
    {
        rtos_deinit_semaphore(&s_ag.spk_sema);
        s_ag.spk_sema = NULL;
    }

    if (s_ag.audio_codec == CODEC_VOICE_MSBC)
    {
        bk_sbc_decoder_deinit();
    }

    s_ag.spk_rb_ready = 0;
    ring_buffer_particle_deinit(&s_ag.spk_rb);
    LOGI("intercom stop");
}

/* -------------------------------------------------------------------------- */
/*  Apple HFP extensions (AT+XAPL / AT+IPHONEACCEV) sent by an Apple accessory  */
/* -------------------------------------------------------------------------- */

/* AT+XAPL=<vendorID>-<productID>-<version>,<features>
   features bit1(2)=battery reporting, bit2(4)=dock/charge, bit3(8)=Siri, bit4(16)=NR. */
static void ag_handle_xapl(const char *s)
{
    const char *p = os_strchr((char *)s, '=');
    const char *comma;
    int feat = 0;
    int idlen;

    if (!p)
    {
        return;
    }

    p++;
    comma = os_strchr((char *)p, ',');

    if (comma)
    {
        feat  = (int)os_strtoul(comma + 1, NULL, 10);
        idlen = (int)(comma - p);
    }
    else
    {
        idlen = (int)os_strlen(p);
    }

    s_ag.xapl_feat = (uint8_t)feat;
    LOGI("HF Apple XAPL id=%.*s features=0x%x", idlen, p, feat);
}

/* AT+IPHONEACCEV=<num_pairs>,<key1>,<val1>,<key2>,<val2>,...
 *   num_pairs : number of key/value pairs that follow
 *   key 1 = battery level (val '0'-'9', battery% = (val+1)*10)
 *   key 2 = dock state    (val 0 = undocked, 1 = docked)
 * e.g. AT+IPHONEACCEV=1,1,3   or   AT+IPHONEACCEV=2,1,9,2,1
 *
 * NOTE: advance with os_strchr() over each ',' and pass NULL as os_strtoul()'s endptr.
 * On this target strtoul() does not update endptr, so the &end-chaining approach parses
 * nothing (like ag_handle_xapl, which works because it uses NULL). */
static void ag_handle_iphoneaccev(const char *s)
{
    const char *p = os_strchr(s, '=');
    int n, i;

    if (!p)
    {
        return;
    }

    p++;                                    /* -> num_pairs */
    n = (int)os_strtoul(p, NULL, 10);

    for (i = 0; i < n; i++)
    {
        int key, val;

        p = os_strchr(p, ',');              /* -> ',' before key */

        if (!p)
        {
            break;
        }

        key = (int)os_strtoul(++p, NULL, 10);

        p = os_strchr(p, ',');              /* -> ',' before val */

        if (!p)
        {
            break;
        }

        val = (int)os_strtoul(++p, NULL, 10);

        if (key == 1)
        {
            if (val > 9) { val = 9; }

            s_ag.hf_batt_level = (uint8_t)((val + 1) * 10);
            LOGI("HF Apple battery level %d (%u%%)", val, (unsigned)s_ag.hf_batt_level);
        }
        else if (key == 2)
        {
            s_ag.hf_docked = (uint8_t)(val ? 1 : 0);
            LOGI("HF Apple dock/charge %d", val);
        }
        else
        {
            LOGI("HF Apple accev key %d val %d", key, val);
        }
    }
}

/* -------------------------------------------------------------------------- */
/*  AG event callback                                                          */
/* -------------------------------------------------------------------------- */

static void hfp_ag_demo_cb(bk_hf_ag_cb_event_t event, bk_hf_ag_cb_param_t *param)
{
    switch (event)
    {
    case BK_HF_AG_CONNECTION_STATE_EVT:
        demo_set_peer(param->remote_addr);
        LOGI("conn state=%d peer_feat=0x%x chld=0x%x",
             param->conn_stat.state, param->conn_stat.peer_feat, param->conn_stat.chld_feat);

        if (param->conn_stat.state == BK_HF_AG_CONNECTION_STATE_SLC_CONNECTED)
        {
            s_ag.slc_ok = 1;
            bt_manager_set_connect_state(BT_STATE_PROFILE_CONNECTED);
            LOGI("SLC connected, ready for call control");
        }
        else if (param->conn_stat.state == BK_HF_AG_CONNECTION_STATE_DISCONNECTED)
        {
            s_ag.slc_ok = 0;
            ag_intercom_stop();
        }

        break;

    case BK_HF_AG_AUDIO_STATE_EVT:
        LOGI("audio state=%d codec=%d interval=%d tx_packet_len=%d rx_packet_len=%d packet_type=%d", param->audio_stat.state, param->audio_stat.codec, param->audio_stat.interval, param->audio_stat.tx_packet_len, param->audio_stat.rx_packet_len, param->audio_stat.packet_type);
        s_ag.air_tx_packet_len = param->audio_stat.tx_packet_len;

        /* SCO up with the negotiated codec -> start the intercom voice path */
        if (param->audio_stat.state == BK_HF_AG_AUDIO_STATE_CONNECTED)
        {
            ag_intercom_start((param->audio_stat.codec == CODEC_VOICE_MSBC) ? CODEC_VOICE_MSBC : CODEC_VOICE_CVSD);
        }
        else if (param->audio_stat.state == BK_HF_AG_AUDIO_STATE_DISCONNECTED)
        {
            ag_intercom_stop();
        }

        break;

    case BK_HF_AG_BVRA_REQ_EVT:
        LOGI("HF requests voice recognition = %d", param->vra_req.value);
        /* accept the request (a product could reject with +CME ERROR if VR is unavailable) */
        bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        bk_bt_hf_ag_vra_control(s_ag.peer_addr, param->vra_req.value);
        break;

    case BK_HF_AG_VOLUME_CONTROL_EVT:
        LOGI("HF volume: %s = %d",
             (param->volume_control.type == BK_HF_VOLUME_CONTROL_TARGET_SPK) ? "spk" : "mic",
             param->volume_control.volume);

        if (param->volume_control.type == BK_HF_VOLUME_CONTROL_TARGET_SPK)
        {
            uint8_t spk_vol = param->volume_control.volume;
            ag_dac_set_gain(NULL, &spk_vol);
        }
        else
        {
            uint8_t mic_vol = param->volume_control.volume;
            ag_dac_set_gain(&mic_vol, NULL);
        }

        break;

    case BK_HF_AG_NREC_REQ_EVT:
        LOGI("HF NREC = %d", param->nrec.state);
        bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        break;

    case BK_HF_AG_VTS_REQ_EVT:
        LOGI("HF DTMF code = %s", param->vts_req.code ? param->vts_req.code : "");
        bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        break;

    case BK_HF_AG_BCS_RESPONSE_EVT:
        LOGI("codec negotiated (BCS) = %d", param->bcs_rep.codec);

        if (param->bcs_rep.codec == CODEC_VOICE_MSBC)
        {
            s_ag.audio_codec = CODEC_VOICE_MSBC;
        }
        else
        {
            s_ag.audio_codec = CODEC_VOICE_CVSD;
        }

        break;

    case BK_HF_AG_BRSF_EVT:
        LOGI("HF supported features (BRSF) = 0x%x", (unsigned)param->brsf.peer_feat);
        break;

    case BK_HF_AG_BIEV_UPDATE_EVT:
        if (param->biev.ind_id == BK_HF_IND_ID_BATTERY_LEVEL)
        {
            s_ag.hf_batt_level = (uint8_t)param->biev.value;
            LOGI("HF indicator battery level = %u%%", (unsigned)param->biev.value);
        }
        else if (param->biev.ind_id == BK_HF_IND_ID_ENHANCED_SAFETY)
        {
            s_ag.hf_enhanced_safety = (uint8_t)param->biev.value;
            LOGI("HF indicator enhanced safety = %u", (unsigned)param->biev.value);
        }
        else
        {
            LOGW("unknown HF indicator id=%u value=%u",
                 (unsigned)param->biev.ind_id, (unsigned)param->biev.value);
        }

        break;

    case BK_HF_AG_CHLD_REQ_EVT:
        LOGI("HF CHLD request type=%d index=%d", param->chld.type, param->chld.index);
        /* a product would run the hold/multiparty op here; report success to the HF */
        bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        break;

    case BK_HF_AG_BTRH_REQ_EVT:
        LOGI("HF BTRH request action=%d", param->btrh.action);
        /* reply order per HFP: +BTRH:<status> first, then OK */
        bk_bt_hf_ag_btrh_response(s_ag.peer_addr, (bk_hf_btrh_status_t)param->btrh.action);
        bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        break;

    case BK_HF_AG_CODEC_EVT:
    {
        /* peer reported its supported codec list (AT+BAC); app may pick via 'hfp_ag codec' */
        char list[48];
        int  off = 0;
        uint8_t i;

        for (i = 0; i < param->codec_info.num && off < (int)sizeof(list) - 6; i++)
        {
            off += snprintf(list + off, sizeof(list) - off, "%s%d",
                            (i ? "," : ""), param->codec_info.codecs[i]);

            if (param->codec_info.codecs[i] == CODEC_VOICE_MSBC)
            {
                //bk_bt_hf_ag_set_codec(s_ag.peer_addr, param->codec_info.codecs[i]);
            }
        }

        LOGI("peer codecs=[%s] (0=CVSD 1=mSBC) -> use 'hfp_ag codec cvsd|msbc' to choose", list);
        break;
    }

    case BK_HF_AG_UNAT_REQ_EVT:
    {
        const char *at = param->unat_req.unat ? param->unat_req.unat : "";
        param->unat_req.app_response = 1;

        if (os_strstr((char *)at, "XAPL"))
        {
            /* Apple handshake: reply +XAPL then OK so the accessory proceeds to AT+IPHONEACCEV */
            ag_handle_xapl(at);
            bk_bt_hf_ag_unknown_at_send(s_ag.peer_addr, "+XAPL=iPhone,2");
            bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        }
        else if (os_strstr((char *)at, "IPHONEACCEV"))
        {
            ag_handle_iphoneaccev(at);
            bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        }
        else
        {
            LOGE("HF unknown AT: %s", at);
            bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_ERR, 0);
        }

        break;
    }

    /* ---- queries that the AG must answer ---- */
    case BK_HF_AG_CIND_REQ_EVT:
        LOGI("HF queries indicators (CIND) signal=%d battery=%d", s_ag.signal, s_ag.batt_lev);
        bk_bt_hf_ag_cind_response(param->remote_addr,
                                  s_ag.num_active ? BK_HF_CALL_STATUS_CALL_IN_PROGRESS : BK_HF_CALL_STATUS_NO_CALLS,
                                  BK_HF_CALL_SETUP_STATUS_IDLE,
                                  BK_HF_NETWORK_STATE_AVAILABLE,
                                  s_ag.signal,
                                  BK_HF_ROAMING_STATUS_INACTIVE,
                                  s_ag.batt_lev,
                                  BK_HF_CALL_HELD_STATUS_NONE);
        break;

    case BK_HF_AG_COPS_REQ_EVT:
        LOGI("HF queries operator (COPS)");
        bk_bt_hf_ag_cops_response(param->remote_addr, "CMCC");
        break;

    case BK_HF_AG_CNUM_REQ_EVT:
        LOGI("HF queries subscriber number (CNUM)");
        bk_bt_hf_ag_cnum_response(param->remote_addr, "10086", BK_HF_SUBSCRIBER_SERVICE_TYPE_VOICE);
        break;

    case BK_HF_AG_CLCC_REQ_EVT:
        LOGI("HF queries current calls (CLCC)");

        if (s_ag.num_active > 0)
        {
            bk_bt_hf_ag_clcc_response(param->remote_addr, 1,
                                      BK_HF_CURRENT_CALL_DIRECTION_INCOMING,
                                      BK_HF_CURRENT_CALL_STATUS_ACTIVE,
                                      BK_HF_CURRENT_CALL_MODE_VOICE,
                                      BK_HF_CURRENT_CALL_MPTY_TYPE_SINGLE,
                                      s_ag.call_number[0] ? s_ag.call_number : NULL,
                                      BK_HF_CALL_ADDR_TYPE_UNKNOWN);
        }

        /* end of list */
        bk_bt_hf_ag_clcc_response(param->remote_addr, 0,
                                  BK_HF_CURRENT_CALL_DIRECTION_OUTGOING,
                                  BK_HF_CURRENT_CALL_STATUS_ACTIVE,
                                  BK_HF_CURRENT_CALL_MODE_VOICE,
                                  BK_HF_CURRENT_CALL_MPTY_TYPE_SINGLE,
                                  NULL, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
        break;

    /* ---- call control requests from HF ---- */
    case BK_HF_AG_ATA_REQ_EVT:
        LOGI("HF answered the call (ATA)");
        /* OK first (command response), then the call-status +CIEV updates */
        bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        hfp_ag_demo_answer();
        break;

    case BK_HF_AG_CHUP_REQ_EVT:
        LOGI("HF hung up the call (CHUP)");
        /* OK first (command response), then the call-status +CIEV updates */
        bk_bt_hf_ag_cmee_send(param->remote_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
        hfp_ag_demo_hangup();
        break;

    case BK_HF_AG_DIAL_REQ_EVT:
        demo_set_peer(param->remote_addr);
        LOGI("HF dial request type=%d value=%s", param->out_call.type,
             param->out_call.num_or_loc ? param->out_call.num_or_loc : "(redial)");

        if (param->out_call.type == BK_HF_AG_DIAL_TYPE_MEMORY)
        {
            /* This demo has no phonebook; a product should resolve the supplied location. */
            LOGW("memory dial location %s is not configured",
                 param->out_call.num_or_loc ? param->out_call.num_or_loc : "");
            bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_CME,
                                  BK_HF_CME_OPERATION_NOT_SUPPORTED);
        }
        else
        {
            /* accept the outgoing call: OK first, then the call-setup +CIEV progression */
            bk_bt_hf_ag_cmee_send(s_ag.peer_addr, BK_HF_AT_RESPONSE_CODE_OK, 0);
            hfp_ag_demo_dial_out(param->out_call.num_or_loc);
        }
        break;

    default:
        LOGW("unhandled AG event %d", event);
        break;
    }
}

static void start_connect(uint8_t *remote_addr)
{
    bk_bt_hf_ag_slc_connect(remote_addr);
}

static void stop_connect()
{

}

static void start_disconnect(uint8_t *remote_addr)
{
    bk_bt_hf_ag_slc_disconnect(bt_manager_get_connected_device());
}

static void gap_event_cb(bk_gap_bt_cb_event_t event, bk_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case BK_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT:
        break;

    case BK_BT_GAP_ACL_CONN_CMPL_STAT_EVT:
        break;

    case BK_BT_GAP_AUTH_CMPL_EVT:
        break;

    case BK_BT_GAP_LINK_KEY_NOTIF_EVT:
    {

    }
    break;

    case BK_BT_GAP_LINK_KEY_REQ_EVT:
    {
    }
    break;

    default:
        break;
    }
}

int hfp_ag_demo_init(void)
{
    int ret;

    LOGI("%s", __func__);

    ret = bk_bt_hf_ag_register_callback(hfp_ag_demo_cb);

    if (ret)
    {
        LOGE("register_callback err %d", ret);
        return -1;
    }

    ret = bk_bt_hf_ag_init();

    if (ret)
    {
        LOGE("init err %d", ret);
        return -1;
    }

    /* Configure the AG feature bitmaps after init and before SLC setup. The SDP
       "SupportedFeatures" attribute and the +BRSF features are DIFFERENT bitfields:
       SDP uses BK_HF_AG_SDP_FEAT_*, +BRSF uses BK_HF_AG_FEAT_*. Query the allowed set
       first, then enable only the desired subset (masked to what the stack supports). */
    {
        uint16_t sdp_feat  = 0;
        uint32_t brsf_feat = 0;
        uint32_t chld_feat = 0;

        bk_bt_hf_ag_sdp_feature_operation(BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED, &sdp_feat);
        bk_bt_hf_ag_brsf_feature_operation(BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED, &brsf_feat);
        bk_bt_hf_ag_chld_feature_operation(BK_HF_AG_FEATURE_API_METHOD_GET_ALLOWED, &chld_feat);
        LOGI("AG allowed features: sdp=0x%x brsf=0x%x chld=0x%x",
             sdp_feat, (unsigned)brsf_feat, (unsigned)chld_feat);

        sdp_feat  &= (BK_HF_AG_SDP_FEAT_3WAY | BK_HF_AG_SDP_FEAT_WBS);
        brsf_feat &= (BK_HF_AG_FEAT_3WAY | BK_HF_AG_FEAT_REJECT | BK_HF_AG_FEAT_ECS |
                      BK_HF_AG_FEAT_ECC  | BK_HF_AG_FEAT_EXTERR | BK_HF_AG_FEAT_CODEC);
        chld_feat &= (
                         BK_HF_CHLD_FEAT_REL |
                         BK_HF_CHLD_FEAT_REL_ACC |
                         BK_HF_CHLD_FEAT_HOLD_ACC |
                         BK_HF_CHLD_FEAT_MERGE |
                         BK_HF_CHLD_FEAT_MERGE_DETACH);

        bk_bt_hf_ag_sdp_feature_operation(BK_HF_AG_FEATURE_API_METHOD_SET, &sdp_feat);
        bk_bt_hf_ag_brsf_feature_operation(BK_HF_AG_FEATURE_API_METHOD_SET, &brsf_feat);
        bk_bt_hf_ag_chld_feature_operation(BK_HF_AG_FEATURE_API_METHOD_SET, &chld_feat);
        LOGI("AG features set: sdp=0x%x brsf=0x%x chld=0x%x",
             sdp_feat, (unsigned)brsf_feat, (unsigned)chld_feat);
    }

    btm_callback_s btm_cb =
    {
        .gap_cb = gap_event_cb,
        .start_connect_cb = start_connect,
        .start_disconnect_cb = start_disconnect,
        .stop_connect_cb = stop_connect,
    };

    bt_manager_register_callback(&btm_cb);

    /* incoming SCO PCM (HF mic -> AG speaker) for the two-way intercom */
    bk_bt_hf_ag_register_data_callback(ag_intercom_recv_cb, NULL);

    return 0;
}

void hfp_ag_demo_connect(const uint8_t *addr)
{
    demo_set_peer(addr);
    bk_bt_hf_ag_slc_connect(s_ag.peer_addr);
}

void hfp_ag_demo_disconnect(void)
{
    if (s_ag.peer_valid)
    {
        bk_bt_hf_ag_slc_disconnect(s_ag.peer_addr);
    }
}

void hfp_ag_demo_incoming_call(const char *number)
{
    if (!s_ag.slc_ok)
    {
        LOGE("SLC not ready");
        return;
    }

    os_memset(s_ag.call_number, 0, sizeof(s_ag.call_number));

    if (number)
    {
        strncpy(s_ag.call_number, number, sizeof(s_ag.call_number) - 1);
    }

    /* tell HF a call is incoming (callsetup = incoming) */
    bk_bt_hf_ag_devices_status_indchange(s_ag.peer_addr,
                                         BK_HF_CALL_STATUS_NO_CALLS,
                                         BK_HF_CALL_SETUP_STATUS_INCOMING,
                                         BK_HF_NETWORK_STATE_AVAILABLE, 5);

    bk_bt_hf_ag_ring(s_ag.peer_addr);
    if (number)
    {
        bk_bt_hf_ag_clip_report(s_ag.peer_addr, number, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    }
    LOGI("incoming call from %s", number ? number : "?");
}

void hfp_ag_demo_dial_out(const char *number)
{
    if (!s_ag.slc_ok)
    {
        return;
    }

    /* keep the previous number on a re-dial (number == NULL) so we can still report it */
    if (number)
    {
        os_memset(s_ag.call_number, 0, sizeof(s_ag.call_number));
        strncpy(s_ag.call_number, number, sizeof(s_ag.call_number) - 1);
    }

    char *cur = s_ag.call_number[0] ? s_ag.call_number : NULL;

    /* dialing -> alerting; report the current call number */
    bk_bt_hf_ag_out_call(s_ag.peer_addr, 0, 0,
                         BK_HF_CALL_STATUS_NO_CALLS,
                         BK_HF_CALL_SETUP_STATUS_OUTGOING_DIALING,
                         cur, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    bk_bt_hf_ag_out_call(s_ag.peer_addr, 0, 0,
                         BK_HF_CALL_STATUS_NO_CALLS,
                         BK_HF_CALL_SETUP_STATUS_OUTGOING_ALERTING,
                         cur, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    LOGI("dialing out %s", cur ? cur : "(redial)");

    /* auto-establish the SCO voice path so the intercom starts without a manual 'audio on' */
    bk_bt_hf_ag_audio_connect(s_ag.peer_addr);
    LOGI("auto audio on after dial");
}

void hfp_ag_demo_answer(void)
{
    s_ag.num_active = 1;
    s_ag.num_held   = 0;
    bk_bt_hf_ag_answer_call(s_ag.peer_addr, s_ag.num_active, s_ag.num_held,
                            BK_HF_CALL_STATUS_CALL_IN_PROGRESS,
                            BK_HF_CALL_SETUP_STATUS_IDLE,
                            s_ag.call_number[0] ? s_ag.call_number : NULL,
                            BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    LOGI("call active");
}

void hfp_ag_demo_hangup(void)
{
    s_ag.num_active = 0;
    s_ag.num_held   = 0;
    bk_bt_hf_ag_end_call(s_ag.peer_addr, s_ag.num_active, s_ag.num_held,
                         BK_HF_CALL_STATUS_NO_CALLS,
                         BK_HF_CALL_SETUP_STATUS_IDLE,
                         NULL, BK_HF_CALL_ADDR_TYPE_UNKNOWN);
    os_memset(s_ag.call_number, 0, sizeof(s_ag.call_number));

    /* also tear down the SCO voice path: dial auto-connects audio, so without this
       the intercom/SCO would keep running and the call would seem un-hangup-able. */
    ag_intercom_stop();
    bk_bt_hf_ag_audio_disconnect(s_ag.peer_addr);

    LOGI("call ended");
}

void hfp_ag_demo_audio(uint8_t connect)
{
    if (connect)
    {

        if (!s_ag.slc_ok)
        {
            LOGE("SLC not ready, connect HF first");
            return;
        }

        /* AG initiates the (e)SCO link (with codec negotiation). The intercom
         * voice path starts from BK_HF_AG_AUDIO_STATE_EVT once the SCO is up and
         * the negotiated codec (CVSD/mSBC) is known. */
        bk_bt_hf_ag_audio_connect(s_ag.peer_addr);
    }
    else
    {
        ag_intercom_stop();
        bk_bt_hf_ag_audio_disconnect(s_ag.peer_addr);
    }
}

void hfp_ag_demo_set_codec(uint8_t msbc)
{
    bk_bt_hf_ag_set_codec(s_ag.peer_addr, msbc ? CODEC_VOICE_MSBC : CODEC_VOICE_CVSD);
    LOGI("codec preference set to %s", msbc ? "mSBC" : "CVSD");
}

void hfp_ag_demo_custom_cmd(const char *atcmd)
{
    bk_bt_hf_ag_unknown_at_send(s_ag.peer_addr, (char *)atcmd);
}

void hfp_ag_demo_send_vgs(uint8_t spk_vol)
{
    if (spk_vol > HFP_GAIN_MAX)
    {
        spk_vol = HFP_GAIN_MAX;
    }

    bk_bt_hf_ag_volume_control(s_ag.peer_addr, BK_HF_VOLUME_CONTROL_TARGET_SPK, spk_vol);
}

void hfp_ag_demo_send_vgm(uint8_t mic_vol)
{
    if (mic_vol > HFP_GAIN_MAX)
    {
        mic_vol = HFP_GAIN_MAX;
    }

    bk_bt_hf_ag_volume_control(s_ag.peer_addr, BK_HF_VOLUME_CONTROL_TARGET_MIC, mic_vol);
}

void hfp_ag_demo_set_battery(uint8_t level)
{
    if (level > 5)
    {
        level = 5;
    }

    s_ag.batt_lev = level;

    if (!s_ag.slc_ok)
    {
        LOGW("battery=%d stored, report deferred (SLC not ready)", level);
        return;
    }

    /* unsolicited battery indicator update to the HF (generic +CIEV report) */
    bk_bt_hf_ag_ciev_report(s_ag.peer_addr, BK_HF_AG_CIND_IDX_BATTCHG, level);
    LOGI("report battery level %d", level);
}

void hfp_ag_demo_gpio(uint8_t gpio, uint8_t high)
{
    LOGI("%s gpio %d, high %d", __func__, gpio, high);

    gpio_dev_unmap(gpio);
    bk_gpio_disable_pull(gpio);
    bk_gpio_enable_output(gpio);
    bk_gpio_set_output_value(gpio, high);
}
