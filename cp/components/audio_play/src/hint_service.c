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


#include <os/os.h>
#include <os/mem.h>
#include <common/bk_include.h>
#include "hint_service.h"
#include "spk_service.h"
#include "audio_play.h"

#define HINT_SERVICE_TAG "hint_svc"
#define LOGE(...) BK_LOGE(HINT_SERVICE_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(HINT_SERVICE_TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(HINT_SERVICE_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(HINT_SERVICE_TAG, ##__VA_ARGS__)

#if (CONFIG_AUDIO_PLAY && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE)

#include <driver/aud_dac_types.h>

/* HINT is a mono 16-bit source; the DAC expands mono to L=R. */
#define HINT_BITS                  (16)
#define HINT_CHANS                 (1)
#define HINT_FRAME_MS              (20)
/* Per-frame back-pressure bound: the DAC drains the aux port at real time, so a
 * write blocks at most ~one frame. This is only a safety bound before we
 * re-check stop_req and retry - samples are never dropped. */
#define HINT_WRITE_TIMEOUT_MS      (100)

#define HINT_TASK_PRI              (BEKEN_DEFAULT_WORKER_PRIORITY)
#define HINT_TASK_STACK            (4096)

/* Max time hint_service_stop() waits for the play task to observe stop_req,
 * detach and exit. The per-frame write is bounded by HINT_WRITE_TIMEOUT_MS, so
 * a short clip drains well within this. */
#define HINT_STOP_WAIT_MS          (1500)

/* Worst-case 20ms frame (48k stereo). The frame buffer lives on the heap (not
 * the task stack) so a higher rate / stereo frame can never overflow it. */
#define HINT_MAX_RATE              (48000)
#define HINT_MAX_CHANS             (2)
#define HINT_FRAME_ISAMPLES_MAX    (HINT_MAX_RATE * HINT_MAX_CHANS * HINT_FRAME_MS / 1000)

/* Decode path (WAV/MP3): the encoded clip is fed into the audio_play decode
 * engine in small chunks and the decoded PCM is streamed into HINT via the
 * pcm_sink. FRAME_SIZE sizes the audio_play reader/pump buffer; DRAIN_IDLE_MS
 * is how long we wait with no sink activity before declaring the tail drained. */
#define HINT_FEED_CHUNK            (1024)
#define HINT_DECODE_FRAME_SIZE     (2048)
#define HINT_DECODE_DRAIN_IDLE_MS  (300)

typedef struct
{
    volatile bool     playing;
    volatile bool     stop_req;
    beken_thread_t    thread;

    /* start/stop serialisation + play-task exit handshake, created once */
    bool              module_ready;
    beken_mutex_t     lock;
    beken_semaphore_t done_sem;

    hint_format_t     fmt;

    /* raw PCM path */
    const int16_t    *pcm;         /* interleaved PCM clip to play */
    uint32_t          pcm_isamples;/* total interleaved int16 samples */
    uint32_t          rate;        /* PCM sample rate */
    uint8_t           chans;       /* PCM channel count (1/2) */

    /* decode path (WAV/MP3) */
    const uint8_t        *enc;     /* encoded clip bytes */
    uint32_t              enc_bytes;
    audio_play_t         *play;
    volatile bool         attached;      /* HINT aux attached (lazy, from sink) */
    volatile uint32_t     last_pcm_tick;
} hint_ctx_t;

static hint_ctx_t s_hint = {0};

/* Create the start/stop lock and the exit handshake semaphore exactly once.
 * s_hint is a static (never memset), so these handles persist for the process
 * lifetime. The semaphore starts empty; the play task posts it on exit. */
static bk_err_t hint_module_prepare(void)
{
    hint_ctx_t *ctx = &s_hint;

    if (ctx->module_ready)
    {
        return BK_OK;
    }

    if (BK_OK != rtos_init_mutex(&ctx->lock))
    {
        LOGE("%s mutex init fail\n", __func__);
        return BK_FAIL;
    }

    if (BK_OK != rtos_init_semaphore(&ctx->done_sem, 1))
    {
        LOGE("%s sem init fail\n", __func__);
        rtos_deinit_mutex(&ctx->lock);
        return BK_FAIL;
    }

    ctx->module_ready = true;
    return BK_OK;
}

/* Drop any stale completion token so a subsequent stop wait is precise. */
static void hint_drain_done_sem(hint_ctx_t *ctx)
{
    while (rtos_get_semaphore(&ctx->done_sem, 0) == BK_OK)
    {
    }
}

/* Request the running tone to stop and wait for the task to detach and exit.
 * Caller must hold ctx->lock. No-op (BK_OK) when nothing is playing. Returns
 * BK_FAIL if the play task did not exit within HINT_STOP_WAIT_MS, in which case
 * the task is still alive and owns the shared ctx state (see hint_service_play).
 */
static bk_err_t hint_stop_locked(hint_ctx_t *ctx)
{
    if (!ctx->playing)
    {
        return BK_OK;
    }

    ctx->stop_req = true;
    if (rtos_get_semaphore(&ctx->done_sem, HINT_STOP_WAIT_MS) != BK_OK)
    {
        LOGW("%s play task exit timeout\n", __func__);
        return BK_FAIL;
    }
    return BK_OK;
}

/* Common exit epilogue for both play tasks: detach HINT, clear state, wake any
 * stop()/interrupting start() waiter, then self-delete. */
static void hint_task_finish(hint_ctx_t *ctx)
{
    spk_service_detach(SPK_SERVICE_SRC_HINT);
    ctx->attached = false;
    /* TODO(ducking): restore A2DP/CALL gain here once ducking is added. */

    ctx->pcm     = NULL;
    ctx->enc     = NULL;
    ctx->thread  = NULL;
    ctx->playing = false;
    rtos_set_semaphore(&ctx->done_sem);
    rtos_delete_thread(NULL);
}

/* raw PCM path: stream the interleaved clip straight into the HINT aux source. */
static void hint_pcm_task(void *arg)
{
    hint_ctx_t *ctx = (hint_ctx_t *)arg;
    const uint8_t chans = ctx->chans ? ctx->chans : 1;
    /* interleaved samples per 20ms frame */
    uint32_t frame_isamples = ctx->rate * chans * HINT_FRAME_MS / 1000u;
    const uint32_t total = ctx->pcm_isamples;
    int16_t *frame;
    uint32_t n = 0;

    if (frame_isamples > HINT_FRAME_ISAMPLES_MAX)
    {
        frame_isamples = HINT_FRAME_ISAMPLES_MAX;
    }

    /* 20ms frame buffer on the heap, not the task stack (see HINT_MAX_*). */
    frame = (int16_t *)os_malloc(HINT_FRAME_ISAMPLES_MAX * sizeof(int16_t));
    if (!frame)
    {
        LOGE("%s frame alloc fail\n", __func__);
        goto out;
    }

    /* TODO(ducking): optionally attenuate A2DP/CALL here so the prompt stands
     * out, then restore below. Intentionally a no-op for now. */

    while (!ctx->stop_req && n < total)
    {
        uint32_t cnt = (total - n < frame_isamples) ? (total - n) : frame_isamples;
        int w;

        os_memcpy(frame, ctx->pcm + n, cnt * sizeof(int16_t));

        /* Back-pressured write: block until the aux port has room, so the producer
         * is paced by the DAC's real drain rate instead of a fixed timer (no drops).
         * On a timeout (w == 0) we simply re-check stop_req and retry the SAME frame,
         * so no samples are lost. Hardware still mixes this on top of music/call. */
        do
        {
            w = spk_service_write_ex(SPK_SERVICE_SRC_HINT, frame,
                                     cnt * sizeof(int16_t), HINT_WRITE_TIMEOUT_MS);
        } while (w == 0 && !ctx->stop_req);

        if (w < 0)
        {
            LOGW("%s write fail, abort\n", __func__);
            break;
        }

        if (ctx->stop_req)
        {
            break;
        }

        n += cnt;
    }

out:
    if (frame)
    {
        os_free(frame);
    }
    LOGI("%s done (%u isamples)\n", __func__, n);
    hint_task_finish(ctx);
}

/* Decode path sink: receives decoded PCM frames from the audio_play pump. On the
 * first frame it discovers the decoded rate/channels and attaches HINT lazily,
 * then back-pressure-writes each frame into the HINT aux source. */
static int hint_pcm_sink(void *user, void *pcm, uint32_t len)
{
    hint_ctx_t *ctx = (hint_ctx_t *)user;
    int off = 0;

    if (!ctx || ctx->stop_req)
    {
        return (int)len;
    }

    if (!ctx->attached)
    {
        uint32_t rate = 0;
        uint8_t  ch = 0, bits = 0;

        if (audio_play_get_pcm_info(ctx->play, &rate, &ch, &bits) != BK_OK)
        {
            /* decoder info not ready yet (rare): drop this frame and wait for
             * the next - the decoder fills info before it emits real PCM. */
            return (int)len;
        }
        if (ch != 1 && ch != 2)
        {
            ch = 1;
        }

        {
            spk_source_cfg_t scfg =
            {
                .src           = SPK_SERVICE_SRC_HINT,
                .nChans        = ch,
                .sampRate      = rate,
                .bitsPerSample = HINT_BITS,
                .frame_size    = rate * ch / 1000u * HINT_FRAME_MS * (HINT_BITS / 8),
                .volume        = 0.0f,
            };
            if (spk_service_attach(&scfg) != BK_OK)
            {
                LOGE("%s attach HINT err\n", __func__);
                ctx->stop_req = true;
                return (int)len;
            }
        }
        ctx->attached = true;
        LOGI("%s HINT attached rate %u ch %u\n", __func__, rate, ch);
    }

    ctx->last_pcm_tick = rtos_get_time();

    while (off < (int)len && !ctx->stop_req)
    {
        int w = spk_service_write_ex(SPK_SERVICE_SRC_HINT,
                                     (const uint8_t *)pcm + off,
                                     (uint32_t)((int)len - off), HINT_WRITE_TIMEOUT_MS);
        if (w < 0)
        {
            break;
        }
        off += w;   /* w == 0 -> port full, retry same offset (no drop) */
    }

    return (int)len;
}

/* Decode path: feed the encoded WAV/MP3 clip through the audio_play decode
 * engine (raw -> decoder -> reader -> pump -> hint_pcm_sink). */
static void hint_decode_task(void *arg)
{
    hint_ctx_t *ctx = (hint_ctx_t *)arg;
    audio_play_cfg_t cfg = DEFAULT_AUDIO_PLAY_CONFIG();
    uint32_t n = 0;

    cfg.decoder_type  = (ctx->fmt == HINT_FMT_MP3) ? AUDIO_PLAY_DECODER_MP3
                                                    : AUDIO_PLAY_DECODER_WAV;
    cfg.frame_size    = HINT_DECODE_FRAME_SIZE;
    cfg.pcm_sink      = hint_pcm_sink;
    cfg.pcm_sink_user = ctx;

    ctx->play = audio_play_create(AUDIO_PLAY_ONBOARD_SPEAKER, &cfg);
    if (!ctx->play)
    {
        LOGE("%s create audio_play fail\n", __func__);
        goto out;
    }
    if (audio_play_open(ctx->play) != BK_OK)
    {
        LOGE("%s open audio_play fail\n", __func__);
        goto out;
    }

    ctx->last_pcm_tick = rtos_get_time();

    /* push the encoded source into the decode pipeline in small chunks */
    while (n < ctx->enc_bytes && !ctx->stop_req)
    {
        uint32_t chunk = ctx->enc_bytes - n;
        int w;

        if (chunk > HINT_FEED_CHUNK)
        {
            chunk = HINT_FEED_CHUNK;
        }

        w = audio_play_write_data(ctx->play, (char *)(ctx->enc + n), chunk);
        if (w > 0)
        {
            n += (uint32_t)w;
        }
        else
        {
            /* AEL_IO_DONE / error: source refused more, stop feeding and drain */
            break;
        }
    }

    /* End-of-stream: tell the decoder no more input is coming so it flushes its
     * final frames and stops cleanly (no input-read timeout spam, esp. MP3). */
    if (!ctx->stop_req)
    {
        audio_play_write_eos(ctx->play);
    }

    /* wait for the decoder + pump to drain the tail. The pump flags EOS as soon
     * as all decoded PCM has been delivered; fall back to an idle-timeout in case
     * the decoder never reports done (e.g. a truncated clip). */
    while (!ctx->stop_req)
    {
        if (audio_play_pcm_ended(ctx->play))
        {
            break;
        }
        if ((rtos_get_time() - ctx->last_pcm_tick) > HINT_DECODE_DRAIN_IDLE_MS)
        {
            break;
        }
        rtos_delay_milliseconds(20);
    }

out:
    if (ctx->play)
    {
        audio_play_close(ctx->play);
        audio_play_destroy(ctx->play);
        ctx->play = NULL;
    }
    LOGI("%s done (fed %u/%u bytes)\n", __func__, n, ctx->enc_bytes);
    hint_task_finish(ctx);
}

bk_err_t hint_service_play(const void *buf, uint32_t bytes, hint_format_t fmt,
                           uint32_t pcm_rate, uint8_t pcm_chans)
{
    hint_ctx_t *ctx = &s_hint;
    bk_err_t ret = BK_FAIL;
    bool is_pcm = (fmt == HINT_FMT_PCM);

    if (!buf || bytes < sizeof(int16_t))
    {
        return BK_FAIL;
    }
    if (is_pcm && (!pcm_rate || (pcm_chans != 1 && pcm_chans != 2)))
    {
        return BK_FAIL;
    }

    if (BK_OK != hint_module_prepare())
    {
        return BK_FAIL;
    }

    rtos_lock_mutex(&ctx->lock);

    /* interrupt-and-replace: stop any tone already playing. If the old task does
     * not exit within the wait window it is still running and still owns the
     * shared ctx (thread/playing/pcm/enc/play). Starting a new task now would let
     * two tasks share one singleton ctx (esp. ctx->play) and corrupt each other,
     * so refuse this request instead. stop_req stays set, so the wedged task will
     * still exit and a later play() self-heals. */
    if (hint_stop_locked(ctx) != BK_OK)
    {
        LOGE("%s previous tone still running, reject new play\n", __func__);
        goto out;
    }
    /* clear any stale completion token so our own stop wait later stays precise */
    hint_drain_done_sem(ctx);

    /* Bring up the shared speaker/DAC (idempotent). The PCM path attaches HINT
     * here (rate/chans are known); the decode path attaches lazily from the sink
     * once the decoder reports the decoded format. */
    if (BK_OK != spk_service_init())
    {
        LOGE("%s spk_service init err\n", __func__);
        goto out;
    }

    ctx->fmt      = fmt;
    ctx->attached = false;
    ctx->stop_req = false;
    ctx->play     = NULL;

    if (is_pcm)
    {
        spk_source_cfg_t scfg =
        {
            .src           = SPK_SERVICE_SRC_HINT,
            .nChans        = pcm_chans,
            .sampRate      = pcm_rate,
            .bitsPerSample = HINT_BITS,
            .frame_size    = pcm_rate * pcm_chans / 1000u * HINT_FRAME_MS * (HINT_BITS / 8),
            .volume        = 0.0f,
        };
        if (BK_OK != spk_service_attach(&scfg))
        {
            LOGE("%s attach HINT err\n", __func__);
            goto out;
        }
        ctx->attached     = true;
        ctx->pcm          = (const int16_t *)buf;
        ctx->pcm_isamples = bytes / sizeof(int16_t);
        ctx->rate         = pcm_rate;
        ctx->chans        = pcm_chans;
        ctx->playing      = true;

        if (BK_OK != rtos_create_thread(&ctx->thread, HINT_TASK_PRI, "hint_play",
                                        hint_pcm_task, HINT_TASK_STACK, ctx))
        {
            LOGE("%s create pcm task err\n", __func__);
            ctx->playing = false;
            ctx->thread  = NULL;
            spk_service_detach(SPK_SERVICE_SRC_HINT);
            ctx->attached = false;
            goto out;
        }
    }
    else
    {
        ctx->enc       = (const uint8_t *)buf;
        ctx->enc_bytes = bytes;
        ctx->playing   = true;

        if (BK_OK != rtos_create_thread(&ctx->thread, HINT_TASK_PRI, "hint_dec",
                                        hint_decode_task, HINT_TASK_STACK, ctx))
        {
            LOGE("%s create decode task err\n", __func__);
            ctx->playing = false;
            ctx->thread  = NULL;
            goto out;
        }
    }

    LOGI("%s started (fmt %d, %u bytes)\n", __func__, fmt, bytes);
    ret = BK_OK;

out:
    rtos_unlock_mutex(&ctx->lock);
    return ret;
}

bk_err_t hint_service_play_pcm(const void *pcm, uint32_t bytes, uint32_t sampRate)
{
    return hint_service_play(pcm, bytes, HINT_FMT_PCM, sampRate, HINT_CHANS);
}

bk_err_t hint_service_stop(void)
{
    hint_ctx_t *ctx = &s_hint;

    if (BK_OK != hint_module_prepare())
    {
        return BK_FAIL;
    }

    rtos_lock_mutex(&ctx->lock);
    hint_stop_locked(ctx);
    rtos_unlock_mutex(&ctx->lock);
    return BK_OK;
}

bool hint_service_is_playing(void)
{
    return s_hint.playing;
}

#else /* feature disabled: safe stubs so callers still link */

bk_err_t hint_service_play(const void *buf, uint32_t bytes, hint_format_t fmt, uint32_t pcm_rate, uint8_t pcm_chans)
{
    (void)buf; (void)bytes; (void)fmt; (void)pcm_rate; (void)pcm_chans; return BK_ERR_NOT_SUPPORT;
}
bk_err_t hint_service_play_pcm(const void *pcm, uint32_t bytes, uint32_t rate)  { (void)pcm; (void)bytes; (void)rate; return BK_ERR_NOT_SUPPORT; }
bk_err_t hint_service_stop(void)                                                { return BK_OK; }
bool     hint_service_is_playing(void)                                          { return false; }

#endif
