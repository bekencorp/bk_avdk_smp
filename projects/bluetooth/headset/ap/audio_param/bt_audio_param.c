/*
 * BT audio param plumbing (not customer-tunable data).
 *
 * The EQ node itself is created by the SDK (a2dp/hfp fill the audio_play /
 * audio_record EQ config from DEFAULT_EQ_ALGORITHM_CONFIG). This file only wires
 * the coefficient tables from the *_audio_para.c files into the param-ctrl /
 * debug tool: on service start it binds the running audio element and pushes the
 * matching preset (bk_app_aud_service_bind -> eq_algorithm_set_config); on stop
 * it unbinds.
 *
 * The bindings are deliberately not EQ-specific: they receive the audio_play /
 * audio_record handle and resolve concrete elements (EQ today; speaker/mic/AEC
 * later) on demand via the audio_play / audio_record accessors, so new tunables
 * can be added without touching the SDK hook signatures.
 */

#include <os/os.h>
#include <os/mem.h>
#include <stdint.h>

#include <components/audio_param_ctrl.h>
#include <components/bk_audio/audio_algorithms/eq_algorithm.h>

#include "audio_play.h"
#include "audio_record.h"
#include "a2dp_sink_audio.h"
#include "hfp_hf_audio.h"
#include "audio_para.h"

#if CONFIG_AUD_PARAM_CTRL

/*
 * Service-slot mapping (the framework enum has no BT entries):
 *   downlink (A2DP or HFP speaker) -> AUD_SERVICE_SINGLE_SPK (eq_dl_config)
 *   HFP uplink (mic)               -> AUD_SERVICE_SINGLE_MIC (eq_ul_config)
 * A2DP and HFP downlink never run at the same time, so they share the SPK slot.
 */
static app_aud_para_t s_spk_para; /* A2DP / HFP downlink */
static app_aud_para_t s_mic_para; /* HFP uplink */

static void get_play_eq_alg(void *service, void *user_ctx, void **eq_alg)
{
    (void)user_ctx;
    *eq_alg = service ? audio_play_get_eq((audio_play_t *)service) : NULL;
}

static void get_record_eq_alg(void *service, void *user_ctx, void **eq_alg)
{
    (void)user_ctx;
    *eq_alg = service ? audio_record_get_eq((audio_record_t *)service) : NULL;
}

static const app_aud_service_adapter_t s_spk_adapter = {
    .get_eq_alg = get_play_eq_alg,
};

static const app_aud_service_adapter_t s_mic_adapter = {
    .get_eq_alg = get_record_eq_alg,
};

#if CONFIG_ADK_EQ_ALGORITHM
/* Pick the preset whose stored sample rate matches the running stream; fall back
 * to the first entry when nothing matches. */
static app_aud_eq_config_t *pick_by_rate(app_aud_eq_config_t *presets, uint32_t num, uint32_t sample_rate)
{
    for (uint32_t i = 0; i < num; i++)
    {
        if (presets[i].eq_load.samplerate == sample_rate)
        {
            return &presets[i];
        }
    }
    return num ? &presets[0] : NULL;
}
#endif

static void audio_bind_spk(audio_play_t *play, app_aud_eq_config_t *dl_presets, uint32_t num,
                           uint32_t sample_rate)
{
    if (!play)
    {
        return;
    }
    os_memset(&s_spk_para, 0, sizeof(s_spk_para));
    s_spk_para.service_type = AUD_SERVICE_SINGLE_SPK;
#if CONFIG_ADK_EQ_ALGORITHM
    {
        app_aud_eq_config_t *p = pick_by_rate(dl_presets, num, sample_rate);
        if (p)
        {
            s_spk_para.eq_dl_config = *p;
        }
    }
#else
    (void)dl_presets;
    (void)num;
    (void)sample_rate;
#endif
    bk_app_aud_service_bind(AUD_SERVICE_SINGLE_SPK, (void *)play, &s_spk_adapter, NULL, &s_spk_para);
}

void bt_a2dp_audio_bind(audio_play_t *play, uint32_t sample_rate)
{
#if CONFIG_ADK_EQ_ALGORITHM
    audio_bind_spk(play, g_a2dp_eq_dl_presets, g_a2dp_eq_dl_preset_num, sample_rate);
#else
    audio_bind_spk(play, NULL, 0, sample_rate);
#endif
}

void bt_a2dp_audio_unbind(void)
{
    bk_app_aud_service_unbind(AUD_SERVICE_SINGLE_SPK);
}

/* Bake the rate-matched preset from a given table into a create-time eq_cal_para.
 * Shared by A2DP / HFP-DL / HFP-UL. Returns the value for cfg.eq_enable:
 *   - app_eq_en gates whether the customer coefficients are copied in;
 *   - eq_en decides whether the EQ node runs (drives cfg.eq_enable).
 * When no preset matches (or param-ctrl data absent) it returns 1 so the
 * create-time DEFAULT_EQ_ALGORITHM_CONFIG() EQ stays on. */
static int fill_eq_from_presets(struct _app_eq_t *eq_cal_para,
                                app_aud_eq_config_t *presets, uint32_t num,
                                uint32_t sample_rate)
{
#if CONFIG_ADK_EQ_ALGORITHM
    app_aud_eq_config_t *p = pick_by_rate(presets, num, sample_rate);

    if (!eq_cal_para || !p)
    {
        return 1;
    }

    if (p->app_eq_en)
    {
        uint32_t filters = p->filters;
        if (filters > CON_AUD_EQ_BANDS)
        {
            filters = CON_AUD_EQ_BANDS;
        }
        eq_cal_para->eq_en       = p->eq_en;
        eq_cal_para->filters     = filters;
        eq_cal_para->globle_gain = p->globle_gain;
        os_memcpy(&eq_cal_para->eq_para, &p->eq_para, sizeof(eq_para_t) * filters);
        os_memcpy(&eq_cal_para->eq_load, &p->eq_load, sizeof(app_eq_load_t));
    }

    return p->eq_en ? 1 : 0;
#else
    (void)eq_cal_para;
    (void)presets;
    (void)num;
    (void)sample_rate;
    return 1;
#endif
}

int bt_a2dp_audio_fill_eq(struct _app_eq_t *eq_cal_para, uint32_t sample_rate)
{
    return fill_eq_from_presets(eq_cal_para, g_a2dp_eq_dl_presets,
                                g_a2dp_eq_dl_preset_num, sample_rate);
}

void bt_hfp_audio_dl_bind(audio_play_t *play, uint32_t sample_rate)
{
#if CONFIG_ADK_EQ_ALGORITHM
    audio_bind_spk(play, g_hfp_eq_dl_presets, g_hfp_eq_dl_preset_num, sample_rate);
#else
    audio_bind_spk(play, NULL, 0, sample_rate);
#endif
}

void bt_hfp_audio_dl_unbind(void)
{
    bk_app_aud_service_unbind(AUD_SERVICE_SINGLE_SPK);
}

int bt_hfp_audio_dl_fill_eq(struct _app_eq_t *eq_cal_para, uint32_t sample_rate)
{
    return fill_eq_from_presets(eq_cal_para, g_hfp_eq_dl_presets,
                                g_hfp_eq_dl_preset_num, sample_rate);
}

void bt_hfp_audio_ul_bind(audio_record_t *record, uint32_t sample_rate)
{
    if (!record)
    {
        return;
    }
    os_memset(&s_mic_para, 0, sizeof(s_mic_para));
    s_mic_para.service_type = AUD_SERVICE_SINGLE_MIC;
#if CONFIG_ADK_EQ_ALGORITHM
    {
        app_aud_eq_config_t *p = pick_by_rate(g_hfp_eq_ul_presets, g_hfp_eq_ul_preset_num, sample_rate);
        if (p)
        {
            s_mic_para.eq_ul_config = *p;
        }
    }
#else
    (void)sample_rate;
#endif
    bk_app_aud_service_bind(AUD_SERVICE_SINGLE_MIC, (void *)record, &s_mic_adapter, NULL, &s_mic_para);
}

void bt_hfp_audio_ul_unbind(void)
{
    bk_app_aud_service_unbind(AUD_SERVICE_SINGLE_MIC);
}

int bt_hfp_audio_ul_fill_eq(struct _app_eq_t *eq_cal_para, uint32_t sample_rate)
{
    return fill_eq_from_presets(eq_cal_para, g_hfp_eq_ul_presets,
                                g_hfp_eq_ul_preset_num, sample_rate);
}

#endif /* CONFIG_AUD_PARAM_CTRL */
