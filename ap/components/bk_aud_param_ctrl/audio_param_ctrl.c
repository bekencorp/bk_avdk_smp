
#include <components/audio_param_ctrl.h>
#include <components/bk_audio/audio_pipeline/audio_pipeline.h>
#if CONFIG_ADK_ONBOARD_MIC_STREAM_V2
#include <components/bk_audio/audio_streams/onboard_mic_stream_v2.h>
#endif
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2
#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>
#endif
#if CONFIG_ADK_AEC_V3_ALGORITHM_COMPONENT_V2
#include <components/bk_audio/audio_algorithms/aec_v3_algorithm_v2.h>
#endif
#if CONFIG_ADK_EQ_ALGORITHM
#include <components/bk_audio/audio_algorithms/eq_algorithm.h>
#endif

#if CONFIG_VOICE_SERVICE
#include <components/bk_voice_service.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_read_service_types.h>
#include <components/bk_voice_write_service.h>
#include <components/bk_voice_write_service_types.h>
#endif

#if (CONFIG_ASR_SERVICE)
#include <components/bk_audio_asr_service.h>
#include <components/bk_audio_asr_service_types.h>
#include <components/bk_asr_service.h>
#include <components/bk_asr_service_types.h>
#endif

#define TAG "AUDIO_PARAM"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#define AUDIO_PARAM_CHECK_NULL(ptr, act) do {\
        if (ptr == NULL) {\
            LOGE("%s, %d, AUDIO_PARAM_NULL fail \n", __func__, __LINE__);\
            {act;};\
        }\
    } while(0)
#define AUDIO_PARAM_CHECK_TYPE(type, act) do {\
        if (type > (AUD_SERVICE_MAX-1)) {\
            LOGE("%s, %d, service_type %d is invalid \n", __func__, __LINE__, service_type);\
            {act;};\
        }\
    } while(0)

typedef struct
{
	void * service;
	app_aud_service_type_t service_type;
	const app_aud_service_adapter_t *adapter;
	void *user_ctx;
}aud_service_param_ctrl_t;

aud_service_param_ctrl_t aud_service_param_ctrl[AUD_SERVICE_MAX];

void bk_app_aud_get_service_handle(void * service, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(service, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);

    aud_service_param_ctrl[service_type].service = service;
    aud_service_param_ctrl[service_type].service_type = service_type;

    switch (service_type) {
        case AUD_SERVICE_DOORBELL_VOC:
            break;
        case AUD_SERVICE_ASR:
            break;
        case AUD_SERVICE_AI_VOC:
            break;
        case AUD_SERVICE_SINGLE_SPK:
        case AUD_SERVICE_SINGLE_MIC:
            break;
        default:
            break;
    }
 }

void bk_app_aud_register_service_adapter(app_aud_service_type_t service_type,
                                         const app_aud_service_adapter_t *adapter,
                                         void *user_ctx)
{
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
    aud_service_param_ctrl[service_type].adapter = adapter;
    aud_service_param_ctrl[service_type].user_ctx = user_ctx;
}

void bk_app_aud_unregister_service_adapter(app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
    aud_service_param_ctrl[service_type].adapter = NULL;
    aud_service_param_ctrl[service_type].user_ctx = NULL;
}

static void bk_app_aud_get_mic_info(app_aud_service_type_t service_type, audio_element_handle_t *mic_str, int *mic_type)
{
    *mic_str = NULL;
    *mic_type = MIC_TYPE_INVALID;

    if (aud_service_param_ctrl[service_type].adapter && aud_service_param_ctrl[service_type].adapter->get_mic_info)
    {
        void *mic = NULL;
        aud_service_param_ctrl[service_type].adapter->get_mic_info(aud_service_param_ctrl[service_type].service,
                                                                   aud_service_param_ctrl[service_type].user_ctx,
                                                                   &mic,
                                                                   mic_type);
        *mic_str = (audio_element_handle_t)mic;
        return;
    }

    switch (service_type)
    {
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE
            if (aud_service_param_ctrl[service_type].service)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;
                mic_type_t type = MIC_TYPE_INVALID;
                bk_voice_get_micstr(voice_handle, mic_str);
                bk_voice_get_micstr_type(voice_handle, &type);
                *mic_type = type;
            }
        #endif
            break;
        case AUD_SERVICE_ASR:
        #if (CONFIG_ASR_SERVICE)
            if (aud_service_param_ctrl[service_type].service && aud_service_param_ctrl[service_type].service_type == AUD_SERVICE_ASR)
            {
                asr_handle_t asr_handle = (asr_handle_t)aud_service_param_ctrl[service_type].service;
                *mic_str = asr_handle->mic_str;
                *mic_type = asr_handle->mic_type;
            }
        #endif
            break;
        default:
            break;
    }
}

static void bk_app_aud_get_spk_info(app_aud_service_type_t service_type, audio_element_handle_t *spk_str, int *spk_type)
{
    *spk_str = NULL;
    *spk_type = SPK_TYPE_INVALID;

    if (aud_service_param_ctrl[service_type].adapter && aud_service_param_ctrl[service_type].adapter->get_spk_info)
    {
        void *spk = NULL;
        aud_service_param_ctrl[service_type].adapter->get_spk_info(aud_service_param_ctrl[service_type].service,
                                                                   aud_service_param_ctrl[service_type].user_ctx,
                                                                   &spk,
                                                                   spk_type);
        *spk_str = (audio_element_handle_t)spk;
        return;
    }

    switch (service_type)
    {
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE
            if (aud_service_param_ctrl[service_type].service)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;
                spk_type_t type = SPK_TYPE_INVALID;
                bk_voice_get_spkstr(voice_handle, spk_str);
                bk_voice_get_spkstr_type(voice_handle, &type);
                *spk_type = type;
            }
        #endif
            break;
        default:
            break;
    }
}

static void bk_app_aud_get_eq_alg(app_aud_service_type_t service_type, audio_element_handle_t *eq_alg)
{
    *eq_alg = NULL;

    if (aud_service_param_ctrl[service_type].adapter && aud_service_param_ctrl[service_type].adapter->get_eq_alg)
    {
        void *eq = NULL;
        aud_service_param_ctrl[service_type].adapter->get_eq_alg(aud_service_param_ctrl[service_type].service,
                                                                 aud_service_param_ctrl[service_type].user_ctx,
                                                                 &eq);
        *eq_alg = (audio_element_handle_t)eq;
        return;
    }

    switch (service_type)
    {
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE && CONFIG_VOICE_SERVICE_EQ
            if (aud_service_param_ctrl[service_type].service)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;
                bk_voice_get_eq_alg(voice_handle, eq_alg);
            }
        #endif
            break;
        default:
            break;
    }
}

static void bk_app_aud_get_aec_alg(app_aud_service_type_t service_type, audio_element_handle_t *aec_alg)
{
    *aec_alg = NULL;

    if (aud_service_param_ctrl[service_type].adapter && aud_service_param_ctrl[service_type].adapter->get_aec_alg)
    {
        void *aec = NULL;
        aud_service_param_ctrl[service_type].adapter->get_aec_alg(aud_service_param_ctrl[service_type].service,
                                                                  aud_service_param_ctrl[service_type].user_ctx,
                                                                  &aec);
        *aec_alg = (audio_element_handle_t)aec;
        return;
    }

    switch (service_type)
    {
        case AUD_SERVICE_AI_VOC:
        case AUD_SERVICE_DOORBELL_VOC:
        #if CONFIG_VOICE_SERVICE && CONFIG_ADK_AEC_V3_ALGORITHM_COMPONENT_V2
            if (aud_service_param_ctrl[service_type].service)
            {
                voice_handle_t voice_handle = (voice_handle_t)aud_service_param_ctrl[service_type].service;
                bk_voice_get_aec_alg(voice_handle, aec_alg);
            }
        #endif
            break;
        default:
            break;
    }
}

void bk_app_aud_set_service_off(app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
    //LOGD("%s, service_type:%d", __func__, service_type);
    aud_service_param_ctrl[service_type].service = NULL;
    aud_service_param_ctrl[service_type].service_type = AUD_SERVICE_MAX;
    switch (service_type) {
        case AUD_SERVICE_DOORBELL_VOC:
            break;
        case AUD_SERVICE_ASR:
            break;
        case AUD_SERVICE_AI_VOC:
            break;
        case AUD_SERVICE_SINGLE_SPK:
        case AUD_SERVICE_SINGLE_MIC:
            break;
        default:
            break;
    }
}

bk_err_t bk_app_aud_service_bind(app_aud_service_type_t service_type,
                                 void *service_handle,
                                 const app_aud_service_adapter_t *adapter,
                                 void *user_ctx,
                                 app_aud_para_t *para)
{
    AUDIO_PARAM_CHECK_TYPE(service_type, return BK_FAIL);
    if (service_handle == NULL || para == NULL) {
        LOGE("%s, %d, service_handle or para is NULL\n", __func__, __LINE__);
        return BK_FAIL;
    }

    para->service_handle = service_handle;

    bk_app_aud_get_service_handle(service_handle, service_type);

    if (adapter) {
        bk_app_aud_register_service_adapter(service_type, adapter, user_ctx);
    }

    /* hand the table to the debug tool and select it as the active service */
    bk_aud_debug_get_audpara(para, service_type);
    bk_aud_debug_set_service_type(service_type);

    /* apply enabled default parameters */
    if (para->sys_config.app_sys_en) {
        bk_app_update_aud_sys_config(&para->sys_config, service_type);
    }
    if (para->aec_v3_config.app_aec_en) {
        bk_app_update_aud_aec_v3_config(&para->aec_v3_config, service_type);
    }
    if (service_type == AUD_SERVICE_SINGLE_MIC) {
        if (para->eq_ul_config.app_eq_en) {
            bk_app_update_aud_eq_config(&para->eq_ul_config, service_type);
        }
    } else {
        if (para->eq_dl_config.app_eq_en) {
            bk_app_update_aud_eq_config(&para->eq_dl_config, service_type);
        }
    }

    return BK_OK;
}

void bk_app_aud_service_unbind(app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
    bk_app_aud_unregister_service_adapter(service_type);
    bk_aud_debug_get_audpara(NULL, service_type);
    bk_app_aud_set_service_off(service_type);
}

 void bk_app_update_aud_sys_config(app_aud_sys_config_t *sys_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(sys_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
    audio_element_handle_t mic_str = NULL;
    audio_element_handle_t spk_str = NULL;
    int mic_type = MIC_TYPE_INVALID;
    int spk_type = SPK_TYPE_INVALID;

    bk_app_aud_get_mic_info(service_type, &mic_str, &mic_type);
    //os_printf("[+]%s, mic_str:0x%x, mic_type:%d\r\n", __func__, mic_str, mic_type);
    #if CONFIG_ADK_ONBOARD_MIC_STREAM_V2
    if (mic_str && mic_type == MIC_TYPE_ONBOARD)
    {
        onboard_mic_stream_set_digital_gain(mic_str, (float)(int32_t)sys_config->mic0_digital_gain, AUD_ADC_CHL_0);
        onboard_mic_stream_set_analog_gain(mic_str, (int32_t)sys_config->mic0_analog_gain, AUD_ADC_CHL_0);
        onboard_mic_stream_set_digital_gain(mic_str, (float)(int32_t)sys_config->mic1_digital_gain, AUD_ADC_CHL_1);
        onboard_mic_stream_set_analog_gain(mic_str, (int32_t)sys_config->mic1_analog_gain, AUD_ADC_CHL_1);
        onboard_mic_stream_set_digital_gain(mic_str, (float)(int32_t)sys_config->mic2_digital_gain, AUD_ADC_CHL_2);
        onboard_mic_stream_set_analog_gain(mic_str, (int32_t)sys_config->mic2_analog_gain, AUD_ADC_CHL_2);
    }
    #endif
    bk_app_aud_get_spk_info(service_type, &spk_str, &spk_type);
    //os_printf("[+]%s, spk_str:0x%x, spk_type:%d\r\n", __func__, spk_str, spk_type);
    #if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2
    if (spk_str && spk_type == SPK_TYPE_ONBOARD)
    {
        onboard_speaker_stream_set_digital_gain(spk_str, (float)sys_config->spk0_digital_gain);
        onboard_speaker_stream_set_analog_gain(spk_str, (int32_t)sys_config->spk0_analog_gain);
    }
    #endif
}

void bk_app_load_aud_sys_config(app_aud_sys_config_t *sys_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(sys_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
    audio_element_handle_t mic_str = NULL;
    audio_element_handle_t spk_str = NULL;
    int mic_type = MIC_TYPE_INVALID;
    int spk_type = SPK_TYPE_INVALID;

    bk_app_aud_get_mic_info(service_type, &mic_str, &mic_type);
    #if CONFIG_ADK_ONBOARD_MIC_STREAM_V2
    if (mic_str && mic_type == MIC_TYPE_ONBOARD)
    {
        float mic_dig_gain = 0;
        int32_t mic_ana_gain = 0;
        onboard_mic_stream_get_digital_gain(mic_str, &mic_dig_gain, AUD_ADC_CHL_0);
        onboard_mic_stream_get_analog_gain(mic_str, &mic_ana_gain, AUD_ADC_CHL_0);
        sys_config->mic0_digital_gain = (int8_t)mic_dig_gain;
        sys_config->mic0_analog_gain = (int8_t)mic_ana_gain;
    }
    #endif
    bk_app_aud_get_spk_info(service_type, &spk_str, &spk_type);
    #if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2
    if (spk_str && spk_type == SPK_TYPE_ONBOARD)
    {
        float spk_dig_gain = 0;
        int32_t spk_ana_gain = 0;
        onboard_speaker_stream_get_digital_gain(spk_str, &spk_dig_gain);
        onboard_speaker_stream_get_analog_gain(spk_str, &spk_ana_gain);
        sys_config->spk0_digital_gain = (int8_t)spk_dig_gain;
        sys_config->spk0_analog_gain = (int8_t)spk_ana_gain;
    }
    #endif
}

void bk_app_update_aud_aec_v3_config(app_aud_aec_v3_config_t *aec_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(aec_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
#if CONFIG_ADK_AEC_V3_ALGORITHM_COMPONENT_V2
    //LOGD("[+]%s, ec_depth:%d\n", __func__, aec_config->ec_depth);
    audio_element_handle_t aec_alg = NULL;
    bk_app_aud_get_aec_alg(service_type, &aec_alg);
    if (aec_alg)
    {
        aec_v3_algorithm_set_config(aec_alg, (void *)aec_config);
    }
#endif
}

void bk_app_load_aud_aec_v3_config(app_aud_aec_v3_config_t *aec_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(aec_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
#if CONFIG_ADK_AEC_V3_ALGORITHM_COMPONENT_V2
    //LOGD("[+]%s, ec_depth:%d\n", __func__, aec_config->ec_depth);
    audio_element_handle_t aec_alg = NULL;
    bk_app_aud_get_aec_alg(service_type, &aec_alg);
    if (aec_alg)
    {
        aec_v3_algorithm_get_config(aec_alg, (void *)aec_config);
    }
#endif
}

void bk_app_update_aud_eq_config(app_aud_eq_config_t *eq_config, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(eq_config, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
#if CONFIG_ADK_EQ_ALGORITHM
    audio_element_handle_t eq_alg = NULL;
    bk_app_aud_get_eq_alg(service_type, &eq_alg);
    if (eq_alg)
    {
        eq_algorithm_set_config(eq_alg, (void *)eq_config);
    }
#endif
}

void bk_app_load_aud_eq_config(app_eq_load_t *eq_load, app_aud_service_type_t service_type)
{
    AUDIO_PARAM_CHECK_NULL(eq_load, return);
    AUDIO_PARAM_CHECK_TYPE(service_type, return);
#if CONFIG_ADK_EQ_ALGORITHM
    audio_element_handle_t eq_alg = NULL;
    bk_app_aud_get_eq_alg(service_type, &eq_alg);
    if (eq_alg)
    {
        eq_algorithm_get_config(eq_alg, (void *)eq_load);
    }
#endif
}

