/*
 * HFP SCO Offload Player - CP full-duplex voice driver (see header).
 *
 * Ported from ap .../service/dm/hfp/hfp_hf_audio.c, restructured for offload:
 *   - Downlink: Layer 4 report/ctrl callbacks (controller context) copy+enqueue
 *     only; a worker task opens audio_play (CALL DAC source) and writes frames.
 *   - Uplink: a mic task captures + encodes (CVSD/mSBC) and injects each frame
 *     via hfp_sco_offload_tx() (bk_ble_hci_to_controller), reusing the same read
 *     pacing as the AP path so we never burst the controller SCO TX pool.
 *   - Volume: HFP +VGS (0~15) cached and applied at START / live while playing.
 *
 * The AP-only EQ (CONFIG_ADK_EQ_ALGORITHM) and audio-param bind (CONFIG_AUD_PARAM_CTRL)
 * hooks are intentionally dropped here: their bt_hfp_audio_* symbols live on the AP
 * host side. Speaker/mic run without them.
 */
#include "hfp_sco_offload_player.h"

#if CONFIG_BLUETOOTH_HFP_SCO_OFFLOAD

#include <math.h>
#include <string.h>
#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>

#include "components/log.h"
#include "components/bluetooth/bk_dm_hfp_types.h"   /* CODEC_VOICE_CVSD / _MSBC */
#include "hfp_sco_offload.h"

#if CONFIG_AUDIO_PLAY
#include <driver/aud_dac_types.h>
#include "audio_play.h"
#include "spk_service.h"
#endif
#if CONFIG_AUDIO_RECORD
#include "audio_record.h"
#endif

#define TAG "sco_ply"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

/* ---- worker parameters ---- */
#define PLAYER_FRAME_MAX      (256u)   /* SCO frame is small (mSBC 60 / CVSD unit) */
#define PLAYER_Q_DEPTH        (12u)
#define PLAYER_TASK_PRIO      (5u)
#define PLAYER_TASK_STACK     (4096u)
#define MIC_TASK_PRIO         (4u)
#define MIC_TASK_STACK        (4096u)

/* HFP +VGS range and default (until the first VOLUME event). */
#define HFP_GAIN_MAX          (15u)    /* see HFP spec */
#define PLAYER_DEFAULT_VGS    (10u)

/* CVSD: one SCO frame carries 60 samples (16-bit) -> 120 bytes. */
#define SCO_CVSD_SAMPLES_PER_FRAME  (60)
/* mSBC encoder emits H2(2) + payload(57) = 59B; SCO packet is fixed 60B (+1 pad). */
#define MSBC_SCO_PACKET_LEN         (60)

/* HFP DAC digital-gain curve (voice source is quiet -> small positive boost). */
#define HFP_DAC_DB_MIN          (-36.0f)
#define HFP_DAC_DB_MAX          (8.0f)
#define HFP_DAC_DB_SMOOTH_GAMMA (2.35f)
#ifndef BK_AUD_DAC_DIG_GAIN_DB_SILENCE
#define BK_AUD_DAC_DIG_GAIN_DB_SILENCE (HFP_DAC_DB_MIN)
#endif

/* Speaker reassembly: at most one odd trailing byte + one SCO packet, so
 * audio_play always receives whole 16-bit samples. */
#define HF_SPK_BUF_SIZE  260

/* queue message types */
enum {
    PLAYER_MSG_FRAME = 0,   /* downlink codec frame */
    PLAYER_MSG_START,       /* session start (codec) */
    PLAYER_MSG_STOP,        /* session stop */
    PLAYER_MSG_VOLUME,      /* +VGS (carried in msg.len) */
};

typedef struct {
    uint8_t  type;
    uint8_t  codec;       /* CODEC_VOICE_CVSD / _MSBC */
    uint16_t handle;
    uint16_t len;         /* FRAME: frame len; VOLUME: vgs */
    uint8_t  data[PLAYER_FRAME_MAX];
} player_msg_t;

static beken_thread_t s_task;
static beken_queue_t  s_queue;
static volatile bool  s_running;

static uint8_t s_codec  = CODEC_VOICE_CVSD;
static uint8_t s_vgs    = PLAYER_DEFAULT_VGS;   /* last known +VGS */
static bool    s_playing;                       /* audio_play open done */

#if CONFIG_AUDIO_PLAY
static audio_play_t *s_audio_play_obj;
static uint8_t  s_hf_spk_buf[HF_SPK_BUF_SIZE];
static uint16_t s_hf_spk_residual;
#endif

/* ---- uplink mic task ---- */
#if CONFIG_AUDIO_RECORD
static beken_thread_t    s_mic_task;
static volatile uint8_t  s_mic_run;
static beken_semaphore_t s_mic_exit_sema;
static audio_record_t   *s_audio_record_obj;
static uint8_t           s_mic_sco_data[1024];
#endif

#if CONFIG_AUDIO_PLAY
static onboard_speaker_pa_ctrl_t s_hfp_pa_ctrl = DEFAULT_ONBOARD_SPEAKER_PA_CTRL();

void hfp_sco_offload_player_set_pa_ctrl(const onboard_speaker_pa_ctrl_t *pa)
{
    if (pa) {
        s_hfp_pa_ctrl = *pa;
    } else {
        onboard_speaker_pa_ctrl_t off = DEFAULT_ONBOARD_SPEAKER_PA_CTRL();
        s_hfp_pa_ctrl = off;
    }
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    spk_service_set_pa_ctrl(pa);
#endif
}
#endif

#if CONFIG_AUDIO_PLAY && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
/* spk_service owns the DAC in multi-source mode, so audio_play must hand its
 * decoded PCM over instead of opening a speaker of its own. */
static int hfp_sco_pcm_to_spk(void *user, void *pcm, uint32_t len)
{
    (void)user;
    return spk_service_write(SPK_SERVICE_SRC_CALL, pcm, len);
}
#endif

/* ============================ volume ============================ */
static float hfp_vgs_to_dac_dig_gain_db(uint8_t vgs)
{
    if (vgs == 0 || HFP_GAIN_MAX == 0) {
        return BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
    }
    {
        float norm   = (float)vgs / (float)HFP_GAIN_MAX;
        float shaped = powf(norm, HFP_DAC_DB_SMOOTH_GAMMA);
        return HFP_DAC_DB_MIN + (HFP_DAC_DB_MAX - HFP_DAC_DB_MIN) * shaped;
    }
}

static void player_apply_gain(uint8_t vgs)
{
#if CONFIG_AUDIO_PLAY
    float gain_db = hfp_vgs_to_dac_dig_gain_db(vgs);

    if (!s_audio_play_obj) {
        return;   /* not up yet: cached in s_vgs, applied at START */
    }
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
    spk_service_set_volume(gain_db);
    spk_service_set_src_mute(SPK_SERVICE_SRC_CALL, (vgs == 0) ? 1 : 0);
#else
    audio_play_set_volume(s_audio_play_obj, gain_db);
    audio_play_control(s_audio_play_obj, (vgs == 0) ? AUDIO_PLAY_MUTE : AUDIO_PLAY_UNMUTE);
#endif
#else
    (void)vgs;
#endif
}

/* ============================ uplink mic task ============================ */
#if CONFIG_AUDIO_RECORD
static void mic_task(void *arg)
{
    uint8_t is_msbc = (CODEC_VOICE_MSBC == s_codec);
    audio_record_cfg_t cfg = DEFAULT_AUDIO_RECORD_CONFIG();

    (void)arg;
    cfg.nChans        = 1;
    cfg.sampRate      = is_msbc ? 16000 : 8000;
    cfg.bitsPerSample = 16;
    cfg.adc_gain      = 16.0;
    cfg.frame_size    = cfg.sampRate * cfg.nChans / 1000 * 20 * cfg.bitsPerSample / 8;
    cfg.pool_size     = cfg.frame_size * 2;
    cfg.encoder_type  = is_msbc ? AUDIO_RECORD_ENCODER_SBC : AUDIO_RECORD_ENCODER_PCM;
    cfg.ch_bitmap     = ONBOARD_MIC_ADC_ACTIVE_CH_0_BIT;

    s_audio_record_obj = audio_record_create(AUDIO_RECORD_ONBOARD_MIC, &cfg);
    if (!s_audio_record_obj) {
        LOGE("%s create audio record err\n", __func__);
        goto end;
    }
    if (audio_record_open(s_audio_record_obj) != 0) {
        LOGE("%s open audio record err\n", __func__);
        goto end;
    }
    LOGI("%s init success (codec=%u)\n", __func__, (unsigned)s_codec);

    {
        uint16_t pending        = 0;
        uint16_t cvsd_send_unit = SCO_CVSD_SAMPLES_PER_FRAME * 2;
        /* Cap the per-read amount so we never burst many SCO packets at once and
         * overflow the controller SCO TX pool (matches AP pacing). */
        int read_cap = is_msbc ? MSBC_SCO_PACKET_LEN : (cvsd_send_unit * 2);

        while (s_mic_run) {
            int read_len = audio_record_read_data(s_audio_record_obj,
                                                  (char *)(s_mic_sco_data + pending),
                                                  read_cap - pending);
            if (read_len <= 0) {
                continue;
            }
            if (is_msbc) {
                /* encoder frame already carries a valid cycling H2 header; append
                 * one pad byte to build the fixed 60-byte mSBC SCO packet. */
                if (read_len + 1 <= (int)sizeof(s_mic_sco_data)) {
                    s_mic_sco_data[read_len] = 0;
                    hfp_sco_offload_tx(s_mic_sco_data, (uint16_t)(read_len + 1));
                }
            } else {
                uint16_t off = 0;
                pending += read_len;
                while (pending - off >= cvsd_send_unit) {
                    hfp_sco_offload_tx(s_mic_sco_data + off, cvsd_send_unit);
                    off += cvsd_send_unit;
                }
                if (off) {
                    pending -= off;
                    if (pending) {
                        memmove(s_mic_sco_data, s_mic_sco_data + off, pending);
                    }
                }
            }
        }
    }

end:
    if (s_audio_record_obj) {
        audio_record_close(s_audio_record_obj);
        audio_record_destroy(s_audio_record_obj);
        s_audio_record_obj = NULL;
    }
    LOGI("%s exit\n", __func__);
    if (s_mic_exit_sema) {
        rtos_set_semaphore(&s_mic_exit_sema);
    }
    s_mic_task = NULL;
    rtos_delete_thread(NULL);
}
#endif /* CONFIG_AUDIO_RECORD */

static void mic_task_start(void)
{
#if CONFIG_AUDIO_RECORD
    if (s_mic_task) {
        return;
    }
    s_mic_run = 1;
    if (rtos_create_thread(&s_mic_task, MIC_TASK_PRIO, "hfp_sco_mic",
                           (beken_thread_function_t)mic_task,
                           MIC_TASK_STACK, NULL) != BK_OK) {
        LOGE("create mic task fail\n");
        s_mic_run = 0;
        s_mic_task = NULL;
    }
#endif
}

static void mic_task_stop(void)
{
#if CONFIG_AUDIO_RECORD
    if (!s_mic_task) {
        return;
    }
    if (rtos_init_semaphore(&s_mic_exit_sema, 1) != BK_OK) {
        s_mic_exit_sema = NULL;
    }
    s_mic_run = 0;
    if (s_mic_exit_sema) {
        rtos_get_semaphore(&s_mic_exit_sema, BEKEN_WAIT_FOREVER);
        rtos_deinit_semaphore(&s_mic_exit_sema);
        s_mic_exit_sema = NULL;
    }
    s_mic_task = NULL;
#endif
}

/* ============================ downlink playback ============================ */
static void player_audio_start(uint8_t codec)
{
    s_codec = codec;
#if CONFIG_AUDIO_PLAY
    {
        audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();
        bk_err_t ret;

        cfg.nChans       = 1;
        cfg.sampRate     = (CODEC_VOICE_MSBC == codec) ? 16000 : 8000;
        cfg.volume       = BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
        cfg.frame_size   = cfg.sampRate * cfg.nChans / 1000 * 20 * cfg.bitsPerSample / 8;
        cfg.pool_size    = cfg.frame_size * 2;
        cfg.decoder_type = (CODEC_VOICE_MSBC == codec) ? AUDIO_PLAY_DECODER_MSBC
                                                       : AUDIO_PLAY_DECODER_PCM;
        cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT;
        cfg.main_dac_source   = AUD_DAC_SOURCE_CALL;
        cfg.pa_ctrl_en   = s_hfp_pa_ctrl.pa_ctrl_en;
        cfg.pa_ctrl_gpio = s_hfp_pa_ctrl.pa_ctrl_gpio;
        cfg.pa_on_level  = s_hfp_pa_ctrl.pa_on_level;
        cfg.pa_on_delay  = s_hfp_pa_ctrl.pa_on_delay;
        cfg.pa_off_delay = s_hfp_pa_ctrl.pa_off_delay;

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        spk_service_set_pa_ctrl(&s_hfp_pa_ctrl);
        if (BK_OK != spk_service_init()) {
            LOGE("%s spk_service init err\n", __func__);
            return;
        }
        cfg.pcm_sink      = hfp_sco_pcm_to_spk;
        cfg.pcm_sink_user = NULL;
        {
            spk_source_cfg_t scfg = {
                .src           = SPK_SERVICE_SRC_CALL,
                .nChans        = cfg.nChans,
                .sampRate      = cfg.sampRate,
                .bitsPerSample = cfg.bitsPerSample,
                .frame_size    = cfg.frame_size,
                .volume        = cfg.volume,
            };
            if (BK_OK != spk_service_attach(&scfg)) {
                LOGE("%s spk_service attach CALL err\n", __func__);
                return;
            }
        }
#endif
        s_hf_spk_residual = 0;
        s_audio_play_obj  = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);
        if (!s_audio_play_obj) {
            LOGE("%s create audio play err\n", __func__);
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
            spk_service_detach(SPK_SERVICE_SRC_CALL);
#endif
            return;
        }
        if ((ret = audio_play_open(s_audio_play_obj)) != 0) {
            LOGE("%s open audio play err %d\n", __func__, ret);
            audio_play_destroy(s_audio_play_obj);
            s_audio_play_obj = NULL;
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
            spk_service_detach(SPK_SERVICE_SRC_CALL);
#endif
            return;
        }
    }
#endif /* CONFIG_AUDIO_PLAY */

    s_playing = true;
    player_apply_gain(s_vgs);      /* apply any +VGS the phone sent pre-start */
    mic_task_start();              /* uplink */
    LOGI("START codec=%u vgs=%u\n", (unsigned)codec, (unsigned)s_vgs);
}

static void player_audio_stop(void)
{
    mic_task_stop();               /* uplink first */
#if CONFIG_AUDIO_PLAY
    if (s_audio_play_obj) {
        audio_play_close(s_audio_play_obj);
        audio_play_destroy(s_audio_play_obj);
        s_audio_play_obj = NULL;
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
        spk_service_detach(SPK_SERVICE_SRC_CALL);
#endif
    }
    s_hf_spk_residual = 0;
#endif
    s_playing = false;
    LOGI("STOP\n");
}

static void player_audio_write(const uint8_t *data, uint16_t len)
{
#if CONFIG_AUDIO_PLAY
    uint16_t total, write_len;

    if (!s_audio_play_obj || !data || !len) {
        return;
    }
    if ((uint32_t)s_hf_spk_residual + len > sizeof(s_hf_spk_buf)) {
        s_hf_spk_residual = 0;
        if (len > sizeof(s_hf_spk_buf)) {
            LOGE("%s packet too large %u\n", __func__, len);
            return;
        }
    }
    memcpy(s_hf_spk_buf + s_hf_spk_residual, data, len);
    total = s_hf_spk_residual + len;

    /* audio_play expects whole 16-bit samples; defer one odd trailing byte. */
    write_len = total - (total % 2);
    if (write_len) {
        int size = audio_play_write_data(s_audio_play_obj, (char *)s_hf_spk_buf, write_len);
        if (size <= 0) {
            LOGE("%s audio_play_write_data err %d %u\n", __func__, size, write_len);
        }
    }
    s_hf_spk_residual = total - write_len;
    if (s_hf_spk_residual) {
        s_hf_spk_buf[0] = s_hf_spk_buf[write_len];
    }
#else
    (void)data;
    (void)len;
#endif
}

/* ============================ offload callbacks (ctrl ctx) ============================ */
static void player_on_frame(uint16_t handle, uint8_t codec,
                            const uint8_t *frame, uint16_t len)
{
    player_msg_t msg;

    if (!s_running || !frame || len == 0) {
        return;
    }
    if (len > PLAYER_FRAME_MAX) {
        len = PLAYER_FRAME_MAX;   /* truncation guard, should not trigger */
    }
    msg.type   = PLAYER_MSG_FRAME;
    msg.codec  = codec;
    msg.handle = handle;
    msg.len    = len;
    memcpy(msg.data, frame, len);

    if (rtos_push_to_queue(&s_queue, &msg, 0) != BK_OK) {
        LOGW("frame dropped, queue full (len=%u)\n", (unsigned)len);
    }
}

static void player_on_ctrl(hfp_sco_offload_evt_t evt,
                           const hfp_sco_offload_session_t *info)
{
    player_msg_t msg;

    if (!s_running || !info) {
        return;
    }
    memset(&msg, 0, sizeof(msg));
    msg.codec  = info->codec;
    msg.handle = info->handle;

    switch (evt) {
    case HFP_SCO_OFFLOAD_EVT_START:
        msg.type = PLAYER_MSG_START;
        break;
    case HFP_SCO_OFFLOAD_EVT_STOP:
        msg.type = PLAYER_MSG_STOP;
        break;
    case HFP_SCO_OFFLOAD_EVT_VOLUME:
        msg.type = PLAYER_MSG_VOLUME;
        msg.len  = info->volume;
        break;
    default:
        return;
    }

    if (rtos_push_to_queue(&s_queue, &msg, 0) != BK_OK) {
        LOGW("ctrl msg dropped, queue full (type=%u)\n", (unsigned)msg.type);
    }
}

/* ============================ worker task ============================ */
static void player_handle_msg(player_msg_t *msg)
{
    switch (msg->type) {
    case PLAYER_MSG_START:
        if (s_playing) {
            LOGW("START while playing, restart\n");
            player_audio_stop();
        }
        player_audio_start(msg->codec);
        break;
    case PLAYER_MSG_STOP:
        player_audio_stop();
        break;
    case PLAYER_MSG_VOLUME:
        s_vgs = (uint8_t)msg->len;
        player_apply_gain(s_vgs);
        LOGI("VOLUME vgs=%u (playing=%d)\n", (unsigned)s_vgs, (int)s_playing);
        break;
    case PLAYER_MSG_FRAME:
        player_audio_write(msg->data, msg->len);
        break;
    default:
        LOGW("unknown msg type %u\n", (unsigned)msg->type);
        break;
    }
}

static void player_task(void *arg)
{
    player_msg_t msg;

    (void)arg;
    while (s_running) {
        if (rtos_pop_from_queue(&s_queue, &msg, BEKEN_WAIT_FOREVER) != BK_OK) {
            continue;
        }
        player_handle_msg(&msg);
    }
    s_task = NULL;
    rtos_delete_thread(NULL);
}

/* ============================ init / deinit ============================ */
bk_err_t hfp_sco_offload_player_init(void)
{
    int ret;

    if (s_running) {
        return BK_OK;
    }
    s_codec     = CODEC_VOICE_CVSD;
    s_vgs       = PLAYER_DEFAULT_VGS;
    s_playing   = false;

    ret = rtos_init_queue(&s_queue, "hfp_sco_player_q",
                          sizeof(player_msg_t), PLAYER_Q_DEPTH);
    if (ret != BK_OK) {
        LOGE("init queue failed %d\n", ret);
        return BK_FAIL;
    }

    s_running = true;
    ret = rtos_create_thread(&s_task, PLAYER_TASK_PRIO, "hfp_sco_player",
                             (beken_thread_function_t)player_task,
                             PLAYER_TASK_STACK, NULL);
    if (ret != BK_OK) {
        s_running = false;
        rtos_deinit_queue(&s_queue);
        LOGE("create task failed %d\n", ret);
        return BK_FAIL;
    }

    /* register with Layer 4 only after the worker is up */
    hfp_sco_offload_register_report_cb(player_on_frame);
    hfp_sco_offload_register_ctrl_cb(player_on_ctrl);
    LOGI("cp hfp sco offload player inited\n");
    return BK_OK;
}

void hfp_sco_offload_player_deinit(void)
{
    if (!s_running) {
        return;
    }
    /* detach from Layer 4 first to stop new events/frames from enqueuing */
    hfp_sco_offload_register_report_cb(NULL);
    hfp_sco_offload_register_ctrl_cb(NULL);
    s_running = false;
    /* task exits and deletes itself once it sees s_running=false after pop */
    rtos_deinit_queue(&s_queue);
    if (s_playing) {
        player_audio_stop();
    }
    LOGI("cp hfp sco offload player deinited\n");
}

#endif /* CONFIG_BLUETOOTH_HFP_SCO_OFFLOAD */
