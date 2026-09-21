#include "audio_param_adapter.h"
#include <common/bk_err.h>
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_player_service.h>

extern app_aud_para_t * get_app_aud_cust_para(app_aud_service_type_t service_type);

/*
 * Level-1 integration: the service is wired by the customer, but mic/spk still
 * use the SDK onboard_mic/speaker_stream elements, aec uses aec_v3 and eq uses
 * the SDK eq algorithm. So only the get_*_info hooks are needed; the SDK
 * applies the parameters with its default implementation.
 *
 * These callbacks locate the elements from a voice service handle.
 */
static void voc_get_mic_info(void *service_handle, void *user_ctx, void **mic_str, int *mic_type)
{
    (void)user_ctx;
    if (!service_handle || !mic_str || !mic_type) {
        return;
    }

    audio_element_handle_t mic = NULL;
    mic_type_t type = MIC_TYPE_INVALID;
    if (BK_OK == bk_voice_get_micstr(service_handle, &mic)) {
        bk_voice_get_micstr_type(service_handle, &type);
        *mic_str = mic;
        *mic_type = type;
    }
}

static void voc_get_spk_info(void *service_handle, void *user_ctx, void **spk_str, int *spk_type)
{
    (void)user_ctx;
    if (!service_handle || !spk_str || !spk_type) {
        return;
    }

    audio_element_handle_t spk = NULL;
    spk_type_t type = SPK_TYPE_INVALID;
    if (BK_OK == bk_voice_get_spkstr(service_handle, &spk)) {
        bk_voice_get_spkstr_type(service_handle, &type);
        *spk_str = spk;
        *spk_type = type;
    }
}

static void voc_get_eq_alg(void *service_handle, void *user_ctx, void **eq_alg)
{
    (void)user_ctx;
    if (!service_handle || !eq_alg) {
        return;
    }

#if CONFIG_VOICE_SERVICE_EQ
    audio_element_handle_t eq = NULL;
    if (BK_OK == bk_voice_get_eq_alg(service_handle, &eq)) {
        *eq_alg = eq;
    }
#endif
}

static void voc_get_aec_alg(void *service_handle, void *user_ctx, void **aec_alg)
{
    (void)user_ctx;
    if (!service_handle || !aec_alg) {
        return;
    }

    audio_element_handle_t aec = NULL;
    if (BK_OK == bk_voice_get_aec_alg(service_handle, &aec)) {
        *aec_alg = aec;
    }
}

static void player_get_spk_info(void *service_handle, void *user_ctx, void **spk_str, int *spk_type)
{
    (void)user_ctx;
    if (!service_handle || !spk_str || !spk_type) {
        return;
    }

    audio_element_handle_t spk = NULL;
    spk_type_t type = SPK_TYPE_INVALID;
    if (BK_OK == bk_player_get_spkstr(service_handle, &spk)) {
        bk_player_get_spkstr_type(service_handle, &type);
        *spk_str = spk;
        *spk_type = type;
    }
}


static const app_aud_service_adapter_t s_voice_adapter = {
    .get_mic_info = voc_get_mic_info,
    .get_spk_info = voc_get_spk_info,
    .get_eq_alg   = voc_get_eq_alg,
    .get_aec_alg  = voc_get_aec_alg,
};

static const app_aud_service_adapter_t s_player_adapter = {
    .get_spk_info = player_get_spk_info,
};


void media_audio_param_bind_spk_handle(void *spk_handle)
{
    bk_app_aud_service_bind(AUD_SERVICE_SINGLE_SPK, spk_handle, &s_player_adapter, NULL,
                            get_app_aud_cust_para(AUD_SERVICE_SINGLE_SPK));
}

void media_audio_param_unbind_spk_handle(void)
{
    bk_app_aud_service_unbind(AUD_SERVICE_SINGLE_SPK);
}

void media_audio_param_bind_voc_handle(void *voc_handle)
{
    bk_app_aud_service_bind(AUD_SERVICE_DOORBELL_VOC, voc_handle, &s_voice_adapter, NULL,
                            get_app_aud_cust_para(AUD_SERVICE_DOORBELL_VOC));
}

void media_audio_param_unbind_voc_handle(void)
{
    bk_app_aud_service_unbind(AUD_SERVICE_DOORBELL_VOC);
}
