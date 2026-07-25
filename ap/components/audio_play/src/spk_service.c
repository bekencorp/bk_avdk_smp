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
#include "spk_service.h"

#define SPK_SERVICE_TAG "spk_svc"
#define LOGE(...) BK_LOGE(SPK_SERVICE_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(SPK_SERVICE_TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(SPK_SERVICE_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(SPK_SERVICE_TAG, ##__VA_ARGS__)

#if (CONFIG_AUDIO_PLAY && CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE)

#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#include <components/bk_audio/audio_pipeline/rb_port.h>
#include <components/bk_audio/audio_pipeline/audio_port.h>
#include <components/bk_audio/audio_streams/raw_stream.h>
#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>
#include <driver/aud_dac.h>

/* Speaker runs stereo so music (A2DP) needs no down-mix; mono sources (call /
 * prompt) are expanded to L=R by the onboard speaker stream. */
#define SPK_SERVICE_SPK_CHL         (2)
#define SPK_SERVICE_BITS            (16)
#define SPK_SERVICE_MAIN_SOURCE     SPK_SERVICE_SRC_A2DP
#define SPK_SERVICE_MULTI_IN_PORTS  (2)  /* CALL + HINT auxiliary inputs */

/* Maximum auxiliary source rate. Aux DMA ring buffers are pre-allocated for this
 * rate at init; the real per-source rate (<= this) is set at attach time. */
#define SPK_SERVICE_AUX_MAX_RATE    (16000)

/* Per-source frame/DMA-period duration (ms). Single source of truth: used both
 * to size the init frame_size[] below and passed to
 * onboard_speaker_stream_set_aux_format() at attach so the two never diverge. */
#define SPK_SERVICE_FRAME_MS        (20)

/* Stereo frame in bytes for a given rate at SPK_SERVICE_FRAME_MS. */
#define SPK_SERVICE_FRAME_BYTES(rate) ((rate) * SPK_SERVICE_SPK_CHL * (SPK_SERVICE_BITS / 8) * SPK_SERVICE_FRAME_MS / 1000)

/* Max time a source pump blocks writing into an aux port before dropping the
 * frame. A few 20ms frames of slack absorbs normal jitter without dead-locking
 * the pump if the speaker stalls. */
#define SPK_SERVICE_AUX_WRITE_TIMEOUT_MS   (60)

/* Idle linger before the shared speaker/DAC auto-deinits after the last source
 * detaches. 0 = never auto-deinit (legacy resident behaviour). Kconfig provides
 * the value; keep a fallback so the file still builds standalone. */
#ifndef CONFIG_SPK_SERVICE_IDLE_LINGER_MS
#define CONFIG_SPK_SERVICE_IDLE_LINGER_MS  (0)
#endif
#define SPK_SERVICE_IDLE_LINGER_MS         (CONFIG_SPK_SERVICE_IDLE_LINGER_MS)

/* All ducking tunables come from Kconfig so a customer configures them in the
 * project defconfig without editing this component. Fallback defaults keep the
 * file building standalone. */
#ifndef CONFIG_SPK_SERVICE_DUCK_ENABLE
#define CONFIG_SPK_SERVICE_DUCK_ENABLE  (1)
#endif
#define SPK_DUCK_ENABLE            (CONFIG_SPK_SERVICE_DUCK_ENABLE)

#ifndef CONFIG_SPK_SERVICE_DUCK_HINT_ON_A2DP_DB
#define CONFIG_SPK_SERVICE_DUCK_HINT_ON_A2DP_DB  (-12)
#endif
#ifndef CONFIG_SPK_SERVICE_DUCK_HINT_ON_CALL_DB
#define CONFIG_SPK_SERVICE_DUCK_HINT_ON_CALL_DB  (-6)
#endif
#define SPK_DUCK_HINT_ON_A2DP_DB   ((float)CONFIG_SPK_SERVICE_DUCK_HINT_ON_A2DP_DB)  /* prompt ducks music */
#define SPK_DUCK_HINT_ON_CALL_DB   ((float)CONFIG_SPK_SERVICE_DUCK_HINT_ON_CALL_DB)  /* prompt ducks voice (lightly) */
/* Note: CALL does NOT duck A2DP - music and voice are mutually exclusive by the
 * app focus policy (call pauses music), so that path never fires in normal use. */

/* Per-source gain smoothing so duck in/out doesn't click. A newly attached
 * source is set to its target instantly (comes in already ducked); only level
 * CHANGES on already-active sources are ramped. */
#ifndef CONFIG_SPK_SERVICE_DUCK_RAMP_MS
#define CONFIG_SPK_SERVICE_DUCK_RAMP_MS       (30)
#endif
#ifndef CONFIG_SPK_SERVICE_DUCK_RAMP_STEP_MS
#define CONFIG_SPK_SERVICE_DUCK_RAMP_STEP_MS  (5)
#endif
#define SPK_DUCK_RAMP_MS           (CONFIG_SPK_SERVICE_DUCK_RAMP_MS)
#define SPK_DUCK_RAMP_STEP_MS      (CONFIG_SPK_SERVICE_DUCK_RAMP_STEP_MS)

/* Sentinel dB that mutes a source; the driver encoder maps db <= silence to a
 * zeroed (muted) gain register. */
#define SPK_GAIN_DB_SILENCE        (-100.0f)

typedef struct
{
    bool                    inited;
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t  raw_write;   /* main (A2DP) PCM input */
    audio_element_handle_t  speaker;     /* persistent onboard speaker element */

    /* auxiliary input ports, indexed by aud_dac_source_t (A2DP slot unused) */
    audio_port_handle_t     aux_port[AUD_DAC_SOURCE_MAX];
    bool                    attached[AUD_DAC_SOURCE_MAX];

    /* cached format of the main (music) source, used to re-lock the shared DAC
     * APLL to the music clock family after an aux attach/detach flips it */
    uint32_t                main_sample_rate;
    uint8_t                 main_chans;
    float                   src_base_db[AUD_DAC_SOURCE_MAX];
    bool                    src_muted[AUD_DAC_SOURCE_MAX];
    float                   src_cur_db[AUD_DAC_SOURCE_MAX];
} spk_service_ctx_t;

static spk_service_ctx_t s_spk_service = {0};

/* Module-scope synchronisation primitives, created once on the first
 * spk_service_init() and kept alive for the whole process lifetime (they are
 * intentionally NOT stored in s_spk_service, which gets memset on every
 * init/deinit cycle). This lets the idle timer and the attach/detach paths
 * share one lock across DAC teardown/rebuild without re-creating handles. */
static beken_mutex_t  s_spk_lock;
static beken2_timer_t s_idle_timer;
static bool           s_spk_module_ready;   /* lock (+timer) created */

/* internal, lock-held cores (public wrappers take s_spk_lock) */
static bk_err_t spk_service_attach_locked(const spk_source_cfg_t *cfg);
static bk_err_t spk_service_detach_locked(spk_service_src_t src);
static bk_err_t spk_service_deinit_locked(void);

static inline void spk_lock(void)   { if (s_spk_module_ready) rtos_lock_mutex(&s_spk_lock); }
static inline void spk_unlock(void) { if (s_spk_module_ready) rtos_unlock_mutex(&s_spk_lock); }

/* Number of currently attached sources; single source of truth is attached[]. */
static uint32_t spk_service_active_count(const spk_service_ctx_t *ctx)
{
    uint32_t n = 0;
    for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if (ctx->attached[i])
        {
            n++;
        }
    }
    return n;
}

/* Re-check under the lock that the service is still idle (a source may have
 * re-attached during the linger window / while the worker was scheduling) and,
 * if so, tear the shared DAC down. */
static void spk_service_idle_deinit_check(void)
{
    spk_service_ctx_t *ctx = &s_spk_service;

    spk_lock();
    if (ctx->inited && spk_service_active_count(ctx) == 0)
    {
        LOGI("idle linger expired, auto deinit\n");
        spk_service_deinit_locked();
    }
    spk_unlock();
}

/* Worker task that performs the actual (blocking) DAC teardown, then exits. */
static void spk_service_deinit_worker(void *arg)
{
    (void)arg;
    spk_service_idle_deinit_check();
    rtos_delete_thread(NULL);
}

/* Idle-linger timer expiry. Runs in the RTOS timer daemon context, so it must
 * NOT block: pipeline stop/wait + element/DAC deinit can take a while and would
 * stall every other timer (and risk the timer task's small stack). Offload the
 * teardown to a short-lived worker task; if we can't spawn one, fall back to an
 * inline teardown as a last resort. */
static void spk_service_idle_timeout(void *larg, void *rarg)
{
    (void)larg;
    (void)rarg;
    beken_thread_t th = NULL;

    if (BK_OK != rtos_create_thread(&th, BEKEN_DEFAULT_WORKER_PRIORITY,
                                    "spk_deinit", spk_service_deinit_worker,
                                    4096, NULL))
    {
        LOGE("%s spawn deinit worker fail, inline teardown\n", __func__);
        spk_service_idle_deinit_check();
    }
}

/* Create the persistent lock (and, if linger is enabled, the one-shot idle
 * timer) exactly once. Safe to call on every init. */
static bk_err_t spk_service_module_prepare(void)
{
    if (s_spk_module_ready)
    {
        return BK_OK;
    }

    if (BK_OK != rtos_init_mutex(&s_spk_lock))
    {
        LOGE("%s mutex init fail\n", __func__);
        return BK_FAIL;
    }

    if (SPK_SERVICE_IDLE_LINGER_MS > 0)
    {
        if (BK_OK != rtos_init_oneshot_timer(&s_idle_timer, SPK_SERVICE_IDLE_LINGER_MS,
                                             spk_service_idle_timeout, NULL, NULL))
        {
            LOGE("%s idle timer init fail\n", __func__);
            rtos_deinit_mutex(&s_spk_lock);
            return BK_FAIL;
        }
    }

    s_spk_module_ready = true;
    return BK_OK;
}

/* Stop a pending auto-deinit (a source is (re)attaching). Lock must be held. */
static void spk_service_cancel_idle_timer(void)
{
    if (SPK_SERVICE_IDLE_LINGER_MS <= 0)
    {
        return;
    }
    if (rtos_is_oneshot_timer_init(&s_idle_timer) && rtos_is_oneshot_timer_running(&s_idle_timer))
    {
        rtos_stop_oneshot_timer(&s_idle_timer);
    }
}

/* Arm the auto-deinit linger (the service just went idle). Lock must be held. */
static void spk_service_arm_idle_timer(void)
{
    if (SPK_SERVICE_IDLE_LINGER_MS <= 0)
    {
        return;
    }
    if (!rtos_is_oneshot_timer_init(&s_idle_timer))
    {
        return;
    }
    if (rtos_is_oneshot_timer_running(&s_idle_timer))
    {
        rtos_stop_oneshot_timer(&s_idle_timer);
    }
    rtos_start_oneshot_timer(&s_idle_timer);
}

/* Map an auxiliary DAC source to its speaker multi-input port id.
 * With bitmap {A2DP, CALL, HINT} and main = A2DP, the non-main active sources
 * are assigned multi-input port ids in order: CALL -> 1, HINT -> 2. */
static uint8_t spk_service_aux_port_id(spk_service_src_t src)
{
    switch (src)
    {
        case SPK_SERVICE_SRC_CALL: return 1;
        case SPK_SERVICE_SRC_HINT: return 2;
        default:                   return 0; /* main / invalid */
    }
}

static uint8_t spk_service_aux_priority(spk_service_src_t src)
{
    /* lower value = higher priority; call outranks prompt */
    return (src == SPK_SERVICE_SRC_CALL) ? 1 : 2;
}

/* Re-lock the shared DAC APLL to the main (music) source's clock family.
 *
 * The DAC has a single APLL: 44.1k music lives in the 90.3168MHz family while
 * the 16k/8k CALL/HINT aux sources live in the 98.304MHz family. Every
 * bk_aud_dac_set_sample_rate() call (done on aux attach/detach) reprograms that
 * shared APLL, so bringing a prompt/call up or down would otherwise flip the
 * clock and detune a concurrently playing 44.1k music stream (audible wobble).
 *
 * We refuse to SW-resample the music (too costly), so instead: whenever the main
 * source is active, re-assert its sample rate here to pull the APLL back to the
 * music family. The short prompt/call then plays under the music clock (a small,
 * unnoticeable rate offset) rather than making the music wobble. When no music
 * is playing (pure prompt, or call while A2DP is suspended) there is nothing to
 * re-lock and the aux source keeps its own exact clock. */
static void spk_service_relock_main_clock(spk_service_ctx_t *ctx)
{
    if (ctx->attached[SPK_SERVICE_MAIN_SOURCE] && ctx->main_sample_rate && ctx->speaker)
    {
        onboard_speaker_stream_set_param(ctx->speaker, (int)ctx->main_sample_rate,
                                         SPK_SERVICE_BITS, (int)ctx->main_chans,
                                         (aud_dac_source_t)SPK_SERVICE_MAIN_SOURCE);
    }
}

static void spk_service_write_src_gain(spk_service_src_t src, float db)
{
    bk_aud_dac_spk0_set_source_gain_db((aud_dac_source_t)src, db);
    bk_aud_dac_spk1_set_source_gain_db((aud_dac_source_t)src, db);
}

static inline bool spk_service_src_audible(const spk_service_ctx_t *ctx, spk_service_src_t s)
{
    return ctx->attached[s] && !ctx->src_muted[s];
}

static float spk_service_duck_db(const spk_service_ctx_t *ctx, spk_service_src_t victim)
{
    float duck = 0.0f;
#if SPK_DUCK_ENABLE
    if (victim != SPK_SERVICE_SRC_HINT && spk_service_src_audible(ctx, SPK_SERVICE_SRC_HINT))
    {
        duck = (victim == SPK_SERVICE_SRC_A2DP) ? SPK_DUCK_HINT_ON_A2DP_DB
             : (victim == SPK_SERVICE_SRC_CALL) ? SPK_DUCK_HINT_ON_CALL_DB : 0.0f;
    }
#else
    (void)ctx; (void)victim;
#endif
    return duck;
}

static float spk_service_src_effective_db(const spk_service_ctx_t *ctx, spk_service_src_t src)
{
    if (ctx->src_muted[src])
    {
        return SPK_GAIN_DB_SILENCE;
    }
    return ctx->src_base_db[src] + spk_service_duck_db(ctx, src);
}

static void spk_service_apply_gains_locked(spk_service_ctx_t *ctx, int instant_src)
{
    float target[AUD_DAC_SOURCE_MAX];
    bool  ramp[AUD_DAC_SOURCE_MAX];
    bool  any_ramp = false;

    for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        ramp[i] = false;
        if (!ctx->attached[i])
        {
            continue;
        }
        target[i] = spk_service_src_effective_db(ctx, (spk_service_src_t)i);

        if ((int)i == instant_src)
        {
            spk_service_write_src_gain((spk_service_src_t)i, target[i]);
            ctx->src_cur_db[i] = target[i];
        }
        else if (target[i] != ctx->src_cur_db[i])
        {
            ramp[i] = true;
            any_ramp = true;
        }
    }

    if (!any_ramp)
    {
        return;
    }

    int steps = SPK_DUCK_RAMP_MS / SPK_DUCK_RAMP_STEP_MS;
    if (steps < 1)
    {
        steps = 1;
    }

    float start[AUD_DAC_SOURCE_MAX];
    for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if (ramp[i])
        {
            start[i] = ctx->src_cur_db[i];
        }
    }

    for (int s = 1; s <= steps; s++)
    {
        float f = (float)s / (float)steps;
        for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++)
        {
            if (ramp[i])
            {
                spk_service_write_src_gain((spk_service_src_t)i,
                                           start[i] + (target[i] - start[i]) * f);
            }
        }
        rtos_delay_milliseconds(SPK_DUCK_RAMP_STEP_MS);
    }

    for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if (ramp[i])
        {
            ctx->src_cur_db[i] = target[i];
        }
    }
}

bk_err_t spk_service_init(void)
{
    spk_service_ctx_t *ctx = &s_spk_service;
    bk_err_t ret;

    if (BK_OK != spk_service_module_prepare())
    {
        return BK_FAIL;
    }

    spk_lock();

    if (ctx->inited)
    {
        /* Already alive: someone wants audio again, so abort any pending
         * auto-deinit and reuse the running DAC. */
        spk_service_cancel_idle_timer();
        spk_unlock();
        return BK_OK;
    }

    os_memset(ctx, 0x00, sizeof(*ctx));

    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    ctx->pipeline = audio_pipeline_init(&pipeline_cfg);
    if (!ctx->pipeline)
    {
        LOGE("%s pipeline init fail\n", __func__);
        goto fail;
    }

    /* main PCM writer: A2DP source frame at 48k stereo by default */
    raw_stream_cfg_t raw_cfg = DEFAULT_RAW_STREAM_CONFIG();
    raw_cfg.type = AUDIO_STREAM_WRITER;
    raw_cfg.out_block_size = SPK_SERVICE_FRAME_BYTES(DEFAULT_AUD_DAC_SAMPLE_RATE);
    raw_cfg.out_block_num = 8;
    raw_cfg.output_port_type = PORT_TYPE_RB;
    ctx->raw_write = raw_stream_init(&raw_cfg);
    if (!ctx->raw_write)
    {
        LOGE("%s raw_stream init fail\n", __func__);
        goto fail;
    }

    onboard_speaker_stream_cfg_t spk_cfg = DEFAULT_ONBOARD_SPEAKER_STREAM_CONFIG();
    spk_cfg.chl_num  = SPK_SERVICE_SPK_CHL;
    spk_cfg.dac_chl  = AUD_DAC_CHL_LR;
    spk_cfg.bits     = SPK_SERVICE_BITS;
    spk_cfg.multi_in_port_num  = SPK_SERVICE_MULTI_IN_PORTS;
    spk_cfg.multi_out_port_num = 0;
    spk_cfg.dac_source_bitmap  = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT
                               | ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT
                               | ONBOARD_SPEAKER_STREAM_DAC_SOURCE_HINT_BIT;
    spk_cfg.main_dac_source    = (aud_dac_source_t)SPK_SERVICE_MAIN_SOURCE;
    /* The persistent speaker is brought up before any call/prompt exists, so the
     * real aux rates are unknown here. Size the CALL/HINT slots for the MAXIMUM
     * supported aux rate (16k) - this only sizes their DMA ring buffers. Each aux
     * source's true rate/frame geometry is applied later at attach time via
     * onboard_speaker_stream_set_aux_format() (an 8k call shrinks to fit). */
    spk_cfg.sample_rate[AUD_DAC_SOURCE_A2DP] = DEFAULT_AUD_DAC_SAMPLE_RATE;
    spk_cfg.sample_rate[AUD_DAC_SOURCE_CALL] = SPK_SERVICE_AUX_MAX_RATE;
    spk_cfg.sample_rate[AUD_DAC_SOURCE_HINT] = SPK_SERVICE_AUX_MAX_RATE;
    spk_cfg.frame_size[AUD_DAC_SOURCE_A2DP]  = SPK_SERVICE_FRAME_BYTES(DEFAULT_AUD_DAC_SAMPLE_RATE);
    spk_cfg.frame_size[AUD_DAC_SOURCE_CALL]  = SPK_SERVICE_FRAME_BYTES(SPK_SERVICE_AUX_MAX_RATE);
    spk_cfg.frame_size[AUD_DAC_SOURCE_HINT]  = SPK_SERVICE_FRAME_BYTES(SPK_SERVICE_AUX_MAX_RATE);

    ctx->speaker = onboard_speaker_stream_init(&spk_cfg);
    if (!ctx->speaker)
    {
        LOGE("%s onboard_speaker init fail\n", __func__);
        goto fail;
    }

    if (BK_OK != audio_pipeline_register(ctx->pipeline, ctx->raw_write, "raw_write"))
    {
        goto fail;
    }
    if (BK_OK != audio_pipeline_register(ctx->pipeline, ctx->speaker, "spk"))
    {
        goto fail;
    }

    {
        const char *link_tag[2] = {"raw_write", "spk"};
        if (BK_OK != audio_pipeline_link(ctx->pipeline, link_tag, 2))
        {
            goto fail;
        }
    }

    ret = audio_pipeline_run(ctx->pipeline);
    if (ret != BK_OK)
    {
        LOGE("%s pipeline run fail %d\n", __func__, ret);
        goto fail;
    }

    ctx->inited = true;
    spk_unlock();
    LOGI("%s ok\n", __func__);
    return BK_OK;

fail:
    if (ctx->pipeline)
    {
        if (ctx->speaker)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->speaker);
        }
        if (ctx->raw_write)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->raw_write);
        }
    }
    if (ctx->speaker)
    {
        audio_element_deinit(ctx->speaker);
    }
    if (ctx->raw_write)
    {
        audio_element_deinit(ctx->raw_write);
    }
    if (ctx->pipeline)
    {
        audio_pipeline_deinit(ctx->pipeline);
    }
    os_memset(ctx, 0x00, sizeof(*ctx));
    spk_unlock();
    return BK_FAIL;
}

/* Tear down the shared speaker/DAC. Lock must be held by the caller. */
static bk_err_t spk_service_deinit_locked(void)
{
    spk_service_ctx_t *ctx = &s_spk_service;

    if (!ctx->inited)
    {
        return BK_OK;
    }

    /* drop any still-attached auxiliary ports */
    for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        if (ctx->attached[i] && i != SPK_SERVICE_MAIN_SOURCE)
        {
            spk_service_detach_locked((spk_service_src_t)i);
        }
    }

    if (ctx->pipeline)
    {
        audio_pipeline_stop(ctx->pipeline);
        audio_pipeline_wait_for_stop(ctx->pipeline);
        audio_pipeline_terminate(ctx->pipeline);

        if (ctx->speaker)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->speaker);
        }
        if (ctx->raw_write)
        {
            audio_pipeline_unregister(ctx->pipeline, ctx->raw_write);
        }
    }

    if (ctx->speaker)
    {
        audio_element_deinit(ctx->speaker);   /* this de-inits the DAC */
    }
    if (ctx->raw_write)
    {
        audio_element_deinit(ctx->raw_write);
    }
    if (ctx->pipeline)
    {
        audio_pipeline_deinit(ctx->pipeline);
    }

    os_memset(ctx, 0x00, sizeof(*ctx));
    LOGI("%s ok\n", __func__);
    return BK_OK;
}

bk_err_t spk_service_deinit(void)
{
    bk_err_t ret;

    spk_lock();
    /* explicit teardown wins over any pending linger */
    spk_service_cancel_idle_timer();
    ret = spk_service_deinit_locked();
    spk_unlock();
    return ret;
}

static bk_err_t spk_service_attach_locked(const spk_source_cfg_t *cfg)
{
    spk_service_ctx_t *ctx = &s_spk_service;

    if (!cfg || !ctx->inited)
    {
        return BK_FAIL;
    }

    if ((int)cfg->src >= (int)AUD_DAC_SOURCE_MAX)
    {
        LOGE("%s invalid src %d\n", __func__, cfg->src);
        return BK_FAIL;
    }

    if (ctx->attached[cfg->src])
    {
        LOGW("%s src %d already attached\n", __func__, cfg->src);
        return BK_OK;
    }

    if (cfg->src == SPK_SERVICE_MAIN_SOURCE)
    {
        /* Main source reuses the persistent raw_write -> speaker link. Only the
         * DAC source rate/channels need to follow the incoming PCM format. */
        bk_err_t ret = onboard_speaker_stream_set_param(ctx->speaker, (int)cfg->sampRate,
                                                        SPK_SERVICE_BITS, cfg->nChans,
                                                        (aud_dac_source_t)cfg->src);
        if (ret != BK_OK)
        {
            LOGE("%s main set_param fail %d\n", __func__, ret);
            return BK_FAIL;
        }

        /* remember the music format so aux attach/detach can re-lock the shared
         * APLL to this clock family (see spk_service_relock_main_clock) */
        ctx->main_sample_rate = cfg->sampRate;
        ctx->main_chans       = cfg->nChans;
    }
    else
    {
        /* Auxiliary source: allocate a ring-buffer input port and bind it to a
         * speaker multi-input port so the DAC hardware-mixes it. */
        ringbuf_port_cfg_t rb_cfg = {.ringbuf_size = cfg->frame_size * 4};
        audio_port_handle_t port = ringbuf_port_init(&rb_cfg);
        if (!port)
        {
            LOGE("%s aux ringbuf_port init fail\n", __func__);
            return BK_FAIL;
        }

        audio_port_info_t port_info = DEFAULT_AUDIO_PORT_INFO();
        port_info.port_id     = spk_service_aux_port_id(cfg->src);
        port_info.priority    = spk_service_aux_priority(cfg->src);
        port_info.chl_num     = cfg->nChans;
        port_info.sample_rate = cfg->sampRate;
        port_info.bits        = SPK_SERVICE_BITS;
        port_info.spk_source  = (uint8_t)cfg->src;
        port_info.port        = port;
        port_info.port_data_valid = true;

        if (BK_OK != onboard_speaker_stream_set_input_port_info(ctx->speaker, &port_info))
        {
            LOGE("%s aux set_input_port_info fail\n", __func__);
            audio_port_deinit(port);
            return BK_FAIL;
        }

        /* Aux sources are hardware-resampled by the DAC (only A2DP has a SW
         * resampler in the speaker process). Align this source's FIFO rate AND
         * its frame geometry (frame_size + DMA transfer_len) to the incoming PCM
         * rate so an 8k call / 16k prompt is drained at its true rate. This is
         * per-source and does not touch the global (stereo) output channel config. */
        if (BK_OK != onboard_speaker_stream_set_aux_format(ctx->speaker,
                                                           (aud_dac_source_t)cfg->src,
                                                           (int)cfg->sampRate,
                                                           SPK_SERVICE_FRAME_MS))
        {
            LOGW("%s aux set_aux_format %u fail\n", __func__, cfg->sampRate);
        }

        ctx->aux_port[cfg->src] = port;

        /* configuring the aux rate above flipped the shared APLL; if music is
         * playing, pull it back to the music clock family so it doesn't wobble */
        spk_service_relock_main_clock(ctx);
    }

    ctx->attached[cfg->src] = true;
    LOGI("%s src %d attached\n", __func__, cfg->src);
    return BK_OK;
}

bk_err_t spk_service_attach(const spk_source_cfg_t *cfg)
{
    bk_err_t ret;

    spk_lock();
    /* a source is (re)attaching: abort any pending idle auto-deinit so we reuse
     * the running DAC instead of tearing it down and rebuilding it */
    spk_service_cancel_idle_timer();
    ret = spk_service_attach_locked(cfg);
    if (ret == BK_OK)
    {
        spk_service_apply_gains_locked(&s_spk_service, (int)cfg->src);
    }
    /* If this attach failed and nothing else is keeping the DAC busy, arm the
     * idle linger. Otherwise a bring-up sequence of init()+attach() whose only
     * source fails to attach would leave the shared DAC running forever (nobody
     * ever calls detach() to trigger the auto-deinit). */
    else if (spk_service_active_count(&s_spk_service) == 0)
    {
        spk_service_arm_idle_timer();
    }
    spk_unlock();
    return ret;
}

static bk_err_t spk_service_detach_locked(spk_service_src_t src)
{
    spk_service_ctx_t *ctx = &s_spk_service;

    if (!ctx->inited || (int)src >= (int)AUD_DAC_SOURCE_MAX)
    {
        return BK_FAIL;
    }

    if (!ctx->attached[src])
    {
        return BK_OK;
    }

    if (src != SPK_SERVICE_MAIN_SOURCE)
    {
        /* Unbind the multi-input port (port = NULL) then free it. */
        audio_port_info_t port_info = DEFAULT_AUDIO_PORT_INFO();
        port_info.port_id  = spk_service_aux_port_id(src);
        port_info.priority = spk_service_aux_priority(src);
        port_info.port     = NULL;
        onboard_speaker_stream_set_input_port_info(ctx->speaker, &port_info);

        if (ctx->aux_port[src])
        {
            audio_port_deinit(ctx->aux_port[src]);
            ctx->aux_port[src] = NULL;
        }

        /* the aux source is gone; if music is still playing, restore the shared
         * APLL to the music clock family so the music returns to correct pitch */
        ctx->attached[src] = false;
        spk_service_relock_main_clock(ctx);
        LOGI("%s src %d detached\n", __func__, src);
        return BK_OK;
    }

    ctx->attached[src] = false;
    LOGI("%s src %d detached\n", __func__, src);
    return BK_OK;
}

bk_err_t spk_service_detach(spk_service_src_t src)
{
    bk_err_t ret;

    spk_lock();
    ret = spk_service_detach_locked(src);
    if (ret == BK_OK)
    {
        spk_service_apply_gains_locked(&s_spk_service, -1);

        if (spk_service_active_count(&s_spk_service) == 0)
        {
            spk_service_arm_idle_timer();
        }
    }
    spk_unlock();
    return ret;
}

int spk_service_write(spk_service_src_t src, const void *buffer, uint32_t len)
{
    spk_service_ctx_t *ctx = &s_spk_service;

    if (!ctx->inited || !buffer || !len || (int)src >= (int)AUD_DAC_SOURCE_MAX)
    {
        return -1;
    }

    if (!ctx->attached[src])
    {
        return -1;
    }

    if (src == SPK_SERVICE_MAIN_SOURCE)
    {
        return raw_stream_write(ctx->raw_write, (char *)buffer, (int)len);
    }

    if (!ctx->aux_port[src])
    {
        return -1;
    }

    /* Bounded write + drop-on-timeout: never block the source pump forever on a
     * stalled/slow speaker. audio_port_write() returns the bytes written (>0) on
     * success or a negative timeout/error code. If the aux port stays full past
     * the timeout, drop this frame (report it as consumed) so the upstream pump
     * keeps running instead of dead-locking and back-pressuring SCO/decoder. */
    if (audio_port_write(ctx->aux_port[src], (char *)buffer, (int)len,
                         SPK_SERVICE_AUX_WRITE_TIMEOUT_MS / portTICK_RATE_MS) <= 0)
    {
        LOGW("%s src %d aux port full, drop %u bytes\n", __func__, src, len);
        return (int)len;
    }

    return (int)len;
}

int spk_service_write_ex(spk_service_src_t src, const void *buffer, uint32_t len, uint32_t timeout_ms)
{
    spk_service_ctx_t *ctx = &s_spk_service;
    TickType_t ticks;
    int w;

    if (!ctx->inited || !buffer || !len || (int)src >= (int)AUD_DAC_SOURCE_MAX)
    {
        return -1;
    }

    if (!ctx->attached[src])
    {
        return -1;
    }

    if (src == SPK_SERVICE_MAIN_SOURCE)
    {
        /* main path already back-pressures via the raw_stream ring buffer */
        return raw_stream_write(ctx->raw_write, (char *)buffer, (int)len);
    }

    if (!ctx->aux_port[src])
    {
        return -1;
    }

    ticks = (timeout_ms == SPK_SERVICE_WAIT_FOREVER)
            ? portMAX_DELAY : (timeout_ms / portTICK_RATE_MS);

    /* audio_port_write() -> rb_write() returns the number of bytes actually
     * written (== len on full success), or a negative error/timeout code.
     * On timeout report "would block" (0) WITHOUT dropping, so the caller can
     * retry and pace itself to the DAC's real drain rate. */
    w = audio_port_write(ctx->aux_port[src], (char *)buffer, (int)len, ticks);
    if (w <= 0)
    {
        return 0;
    }

    return w;
}

bk_err_t spk_service_set_volume(float gain_db)
{
    spk_service_ctx_t *ctx = &s_spk_service;

    if (!ctx->inited || !ctx->speaker)
    {
        return BK_FAIL;
    }

    return onboard_speaker_stream_set_digital_gain(ctx->speaker, gain_db);
}

bk_err_t spk_service_set_mute(uint8_t mute)
{
    spk_service_ctx_t *ctx = &s_spk_service;

    if (!ctx->inited || !ctx->speaker)
    {
        return BK_FAIL;
    }

    return onboard_speaker_stream_dac_mute_en(ctx->speaker, mute);
}

bk_err_t spk_service_set_src_mute(spk_service_src_t src, uint8_t mute)
{
    spk_service_ctx_t *ctx = &s_spk_service;

    if ((int)src >= (int)AUD_DAC_SOURCE_MAX)
    {
        return BK_FAIL;
    }

    spk_lock();
    if (!ctx->inited)
    {
        spk_unlock();
        return BK_FAIL;
    }

    /* Record the per-source mute gate, then re-apply gains: the muted/unmuted
     * source is set instantly, and since a muted source no longer acts as a duck
     * trigger, any source it was ducking ramps back up (and vice versa). */
    ctx->src_muted[src] = mute ? true : false;
    spk_service_apply_gains_locked(ctx, (int)src);
    spk_unlock();
    return BK_OK;
}

bool spk_service_is_running(void)
{
    return s_spk_service.inited;
}

#else /* feature disabled: provide safe stubs so callers still link */

bk_err_t spk_service_init(void)                          { return BK_ERR_NOT_SUPPORT; }
bk_err_t spk_service_deinit(void)                        { return BK_OK; }
bk_err_t spk_service_attach(const spk_source_cfg_t *cfg) { (void)cfg; return BK_ERR_NOT_SUPPORT; }
bk_err_t spk_service_detach(spk_service_src_t src)       { (void)src; return BK_OK; }
int      spk_service_write(spk_service_src_t src, const void *buffer, uint32_t len)
{
    (void)src; (void)buffer; (void)len; return -1;
}
int      spk_service_write_ex(spk_service_src_t src, const void *buffer, uint32_t len, uint32_t timeout_ms)
{
    (void)src; (void)buffer; (void)len; (void)timeout_ms; return -1;
}
bk_err_t spk_service_set_volume(float gain_db)           { (void)gain_db; return BK_ERR_NOT_SUPPORT; }
bk_err_t spk_service_set_mute(uint8_t mute)              { (void)mute; return BK_ERR_NOT_SUPPORT; }
bk_err_t spk_service_set_src_mute(spk_service_src_t src, uint8_t mute) { (void)src; (void)mute; return BK_ERR_NOT_SUPPORT; }
bool     spk_service_is_running(void)                    { return false; }

#endif
