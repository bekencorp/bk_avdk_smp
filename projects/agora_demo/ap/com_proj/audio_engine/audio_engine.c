#include "audio_engine.h"
#include <os/os.h>
#include <common/bk_include.h>
#include <common/bk_err.h>
#include <network_transfer.h>
#include <string.h>
#if CONFIG_AE_SUPPORT_PROMPT_TONE
#include "audio_engine_prompt_tone.h"
#endif
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2
#include <components/bk_audio/audio_streams/onboard_speaker_stream_v2.h>
#else
#include <components/bk_audio/audio_streams/onboard_speaker_stream.h>
#endif
#if CONFIG_BK_FACTORY_CONFIG
#include "bk_factory_config.h"
#endif

#if CONFIG_APP_EVT
#include "app_event.h"
#endif

#define TAG "audio_engine"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

#if CONFIG_AE_AUDIO_DECODER_G722 && (!CONFIG_ADK_G722_DECODER || !CONFIG_VOICE_SERVICE_G722_DECODER)
#error "CONFIG_AE_AUDIO_DECODER_G722 is enabled, but CONFIG_ADK_G722_DECODER or CONFIG_VOICE_SERVICE_G722_DECODER is not enabled"
#endif

#if CONFIG_AE_AUDIO_ENCODER_G722 && (!CONFIG_ADK_G722_ENCODER || !CONFIG_VOICE_SERVICE_G722_ENCODER)
#error "CONFIG_AE_AUDIO_ENCODER_G722 is enabled, but CONFIG_ADK_G722_ENCODER or CONFIG_VOICE_SERVICE_G722_ENCODER is not enabled"
#endif

#if CONFIG_AE_AUDIO_DECODER_OPUS && (!CONFIG_ADK_OPUS_DECODER || !CONFIG_VOICE_SERVICE_OPUS_DECODER)
#error "CONFIG_AE_AUDIO_DECODER_OPUS is enabled, but CONFIG_ADK_OPUS_DECODER or CONFIG_VOICE_SERVICE_OPUS_DECODER is not enabled"
#endif

#if CONFIG_AE_AUDIO_ENCODER_OPUS && (!CONFIG_ADK_OPUS_ENCODER || !CONFIG_VOICE_SERVICE_OPUS_ENCODER)
#error "CONFIG_AE_AUDIO_ENCODER_OPUS is enabled, but CONFIG_ADK_OPUS_ENCODER or CONFIG_VOICE_SERVICE_OPUS_ENCODER is not enabled"
#endif

#if (CONFIG_AE_AUDIO_ENCODER_G711A || CONFIG_AE_AUDIO_ENCODER_G711U) && (!CONFIG_ADK_G711_ENCODER || !CONFIG_VOICE_SERVICE_G711_ENCODER)
#error "(CONFIG_AE_AUDIO_ENCODER_G711A || CONFIG_AE_AUDIO_ENCODER_G711U) is enabled, but CONFIG_ADK_G711_ENCODER or CONFIG_VOICE_SERVICE_G711_ENCODER is not enabled"
#endif

#if (CONFIG_AE_AUDIO_DECODER_G711A || CONFIG_AE_AUDIO_DECODER_G711U) && (!CONFIG_ADK_G711_DECODER || !CONFIG_VOICE_SERVICE_G711_DECODER)
#error "(CONFIG_AE_AUDIO_DECODER_G711A || CONFIG_AE_AUDIO_DECODER_G711U) is enabled, but CONFIG_ADK_G711_DECODER or CONFIG_VOICE_SERVICE_G711_DECODER is not enabled"
#endif

/* Global handles for audio engine */
struct audio_engine_ctx g_audio_engine = {0};
audio_engine_cfg_t g_audio_engine_cfg = {0};

#if CONFIG_AE_SUPPORT_PROMPT_TONE
audio_engine_prompt_tone_handle_t g_audio_engine_prompt_tone = NULL;
#endif
uint8_t g_volume_level = 7;   // volume level, not gain.
static const float g_volume_gain[SPK_VOLUME_LEVEL] = {
	-36.00f, -33.06f, -29.82f, -26.22f, -22.26f, -17.88f,
	-13.03f, -7.68f, -1.76f, 4.77f, 12.00f
};

static uint8_t audio_engine_volume_get_diag_gain(void)
{
    if (g_volume_level > (SPK_VOLUME_LEVEL-1)) {
        g_volume_level = SPK_VOLUME_LEVEL-1;
    }
    return g_volume_gain[g_volume_level];
}

static int audio_engine_volume_init(void)
{
    LOGI("audio_engine_volume_init\n");

    #if CONFIG_BK_FACTORY_CONFIG
    int g_volume_level_size = 0;

    g_volume_level_size = bk_config_read("volume", (void *)&g_volume_level, 4);
    if (g_volume_level_size != 4)
    {
        LOGE("read volume config fail, use default config g_volume_level_size:%d\n", g_volume_level_size);
    }

    LOGI("Saved volume level: %d\n", g_volume_level);

    if (0 != bk_config_write("volume", (void *)&g_volume_level, 4))
    {
        LOGE("storage g_volume_level: %d fail\n", g_volume_level);
    }
    #else
    LOGI("not support factory config, use default config g_volume_level: %d\n", g_volume_level);
    #endif

    if (g_volume_level >= SPK_VOLUME_LEVEL) {
        g_volume_level = SPK_VOLUME_LEVEL - 1;
    }

    return AUDIO_ENGINE_SUCCESS;
}

static int audio_engine_adjust_voice_gain(float gain_db)
{
    voice_handle_t voice_handle = (voice_handle_t)g_audio_engine.voice_handle;
    audio_element_handle_t spk_str = NULL;
    spk_type_t spk_type = SPK_TYPE_INVALID;

    if (!voice_handle) {
        LOGE("audio engine adjust voice gain failed, voice_handle is NULL\n");
        return AUDIO_ENGINE_ERR_INVALID_PARAM;
    }

    bk_voice_get_spkstr(voice_handle, &spk_str);
    bk_voice_get_spkstr_type(voice_handle, &spk_type);

    if (spk_str && spk_type == SPK_TYPE_ONBOARD)
    {
        if (BK_OK != onboard_speaker_stream_set_digital_gain(spk_str, gain_db)) {
            LOGE("onboard_speaker_stream_set_digital_gain failed\n");
            return AUDIO_ENGINE_ERR_INVALID_PARAM;
        }
    }
    else
    {
        LOGE("audio engine adjust voice gain failed, spk_str is not onboard speaker stream\n");
        return AUDIO_ENGINE_ERR_INVALID_PARAM;
    }

    return AUDIO_ENGINE_SUCCESS;
}

void audio_engine_volume_increase(void)
{
    LOGI("volume up\r\n");

    if (g_volume_level >= (SPK_VOLUME_LEVEL - 1))
    {
        LOGI("volume have reached maximum: level %u, +12 dB\n", (SPK_VOLUME_LEVEL - 1));
        return;
    }

    if (BK_OK == audio_engine_adjust_voice_gain(g_volume_gain[g_volume_level + 1]))
    {
        g_volume_level += 1;
        #if CONFIG_BK_FACTORY_CONFIG
        if (0 != bk_config_write("volume", (void *)&g_volume_level, 4))
        {
            LOGE("storage volume: %d fail\n", g_volume_level);
        }
        #else
        LOGE("not support factory config, storage volume: %d fail\n", g_volume_level);
        #endif
        LOGI("current volume level: %u, %.1f dB\n", g_volume_level, g_volume_gain[g_volume_level]);
    }
    else
    {
        LOGI("set volume fail\n");
    }
}

void audio_engine_volume_decrease(void)
{
    LOGI("volume down\r\n");

    if (g_volume_level == 0)
    {
        LOGI("volume have reached minimum: level 0, %.1f dB\n", g_volume_gain[0]);
        return;
    }

    if (BK_OK == audio_engine_adjust_voice_gain(g_volume_gain[g_volume_level - 1]))
    {
        g_volume_level -= 1;
        #if CONFIG_BK_FACTORY_CONFIG
        if (0 != bk_config_write("volume", (void *)&g_volume_level, 4))
        {
            LOGE("storage volume: %d fail\n", g_volume_level);
        }
        #else
        LOGE("not support factory config, storage volume: %d fail\n", g_volume_level);
        #endif
        LOGI("current volume level: %u, %.1f dB\n", g_volume_level, g_volume_gain[g_volume_level]);
    }
    else
    {
        LOGI("set volume fail\n");
    }
}

#if (CONFIG_ASR_SERVICE)
#if CONFIG_WANSON_ARMINO_ASR
#include "bk_wanson_asr_intf.h"
#elif CONFIG_BEKEN_KWS
#include "bk_kws_asr.h"
#endif
static const char *g_audio_engine_asr_text = NULL;
static float g_audio_engine_asr_score = 0.0f;

void bk_audio_engine_asr_result_handle(void *p1, void *p2)
{
    const char *result = NULL;
    uint8_t asr_result = 0;

    /* BK7259 ASR callback passes result and score via p1/p2 pointers. */
    if (p1 != NULL) {
        result = *((char **)p1);
    }
    if (result == NULL) {
        LOGE("ASR result is NULL\n");
        return;
    }
    (void)p2;

#if CONFIG_BEKEN_KWS
    //LOGD("result : %s\n", result);
    if (os_strcmp(result, "Hi Armino") == 0)
    {
        LOGI("Armino\r\n");
        asr_result = 1;
    } else if ((os_strcmp(result, "Byebye") == 0))
    {
        LOGI("%s \n", "Byebye");
        asr_result = 2;
    } else if (os_strcmp(result, "Play Music") == 0)
    {
        LOGI("play music\r\n");
        asr_result = 3;
    } else if (os_strcmp(result, "Stop Play") == 0)
    {
        LOGI("stop play\r\n");
        asr_result = 4;
    }else if (os_strcmp(result, "Next song") == 0)
    {
        LOGI("next song\r\n");
        asr_result = 5;
    } else if (os_strcmp(result, "Volume Up") == 0)
    {
        LOGI("volume up\r\n");
        asr_result = 6;
    } else if (os_strcmp(result, "Volume Down") == 0)
    {
        LOGI("volume down\r\n");
        asr_result = 7;
    }
#endif

#if (CONFIG_WANSON_ASR_GROUP_VERSION_WORDS_V1)
    if (os_strcmp(result, "嗨阿米诺") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 0 ");
        asr_result = 1;
    }
    else if (os_strcmp(result, "嘿阿米楼") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 1 ");
        asr_result = 1;
    }
    else if (os_strcmp(result, "嘿儿米楼") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 2 ");
        asr_result = 1;
    }
    else if (os_strcmp(result, "嘿鹅迷楼") == 0)
    {
        LOGI("%s \n", "hi armino, cmd: 3 ");
        asr_result = 1;
    }
    else if (os_strcmp(result, "拜拜阿米诺") == 0)
    {
        LOGI("%s \n", "byebye armino, cmd: 0 ");
        asr_result = 2;
    }
    else if (os_strcmp(result, "拜拜阿米楼") == 0)
    {
        LOGI("%s \n", "byebye armino, cmd: 1 ");
        asr_result = 2;
    }
    else
    {
        //nothing
    }
#else
    if (os_strcmp(result, "你好阿米诺") == 0)
    {
        LOGI("%s \n", "nihao armino, cmd: 0 ");
        asr_result = 1;
    }
    else if (os_strcmp(result, "再见阿米诺") == 0)
    {
        LOGI("%s \n", "zaijian armino, cmd: 1 ");
        asr_result = 2;
    }
    else
    {
        //nothing
    }
#endif

#if CONFIG_BEKEN_KWS
    if ((asr_result < 0) || (asr_result > 7)) {
        LOGE("Invalid asr_result: %d, valid range is 0-7\n", asr_result);
        return;
    }
#else
    if ((asr_result == 0) || (asr_result > 2)) {
        LOGE("Invalid asr_result: %d, valid range is 1-2\n", asr_result);
        return;
    }
#endif
    g_audio_engine.asr_result = asr_result;
    LOGD("ASR result set to: %d\n", g_audio_engine.asr_result);

#if 0
    audio_element_handle_t spk_element = bk_voice_get_spk_element(g_audio_engine.voice_handle);
    if (spk_element)
    {
        if (g_audio_engine.asr_result == 1) {
            onboard_speaker_stream_set_input_port_data_valid(spk_element, 0, true);
        } else if (g_audio_engine.asr_result == 2) {
            onboard_speaker_stream_set_input_port_data_valid(spk_element, 0, false);
        } else {
            //nothing todo
        }
    }
#endif

#if CONFIG_APP_EVT
    bk_err_t ret = BK_FAIL;
    if (g_audio_engine.asr_result == 1) {
        ret = app_event_send_msg(APP_EVT_ASR_WAKEUP, 0);
        if (BK_OK != ret) {
            LOGE("Failed to send APP_EVT_ASR_WAKEUP event, ret: %d\n", ret);
        } else {
            LOGD("APP_EVT_ASR_WAKEUP event sent successfully\n");
        }
    }
    else if (g_audio_engine.asr_result == 2) {
        ret = app_event_send_msg(APP_EVT_ASR_STANDBY, 0);
        if (BK_OK != ret) {
            LOGE("Failed to send APP_EVT_ASR_STANDBY event, ret: %d\n", ret);
        } else {
            LOGD("APP_EVT_ASR_STANDBY event sent successfully\n");
        }
    }
    else if (g_audio_engine.asr_result == 6) {
        audio_engine_volume_increase();
    }
    else if (g_audio_engine.asr_result == 7) {
        audio_engine_volume_decrease();
    }
    else if (g_audio_engine.asr_result >= 3 && g_audio_engine.asr_result <= 5) {
        //nothing to do
    }
    else {
        LOGE("Unexpected asr_result: %d\n", g_audio_engine.asr_result);
    }
#else
    //LOGW("CONFIG_APP_EVT is not enabled, skipping event notification\n");
#endif
}

#if CONFIG_BEKEN_KWS

int bk_tflite_asr_init(void)
{
	bk_kws_init(NULL);
	return 1;
}

int bk_tflite_asr_recog(void *read_buf, uint32_t read_size, void *p1, void *p2)
{
	int16_t result = 0;
	bk_tflite_ASR_Recog((short*)read_buf, read_size, p1, p2, &result);
	//LOGD("%s , %d\n", g_audio_engine_asr_text, result);
	return result;
}

#endif

#endif

/**
 * @brief Initialize and start audio engine
 * 
 * @param cfg Pointer to audio engine configuration structure
 * @return int 
 *         - 0: Success
 *         - < 0: Error codes
 *             - -1: Invalid parameters
 *             - -2: Voice init failed
 *             - -3: Voice read init failed
 *             - -4: Voice write init failed
 *             - -5: Voice start failed
 *             - -6: Voice read start failed
 *             - -7: Voice write start failed
 */
#if CONFIG_AUDIO_PARA
extern app_aud_para_t *get_app_aud_cust_para(app_aud_service_type_t service_type);
extern void set_app_aud_cust_service_handle(void *service_handle, app_aud_service_type_t service_type);

void bk_audio_set_voc_cust_params(voice_cfg_t * voice_cfg, app_aud_service_type_t service_type)
{
	app_aud_para_t * cust_aud_para = NULL;
	{
		cust_aud_para = get_app_aud_cust_para(AUD_SERVICE_AI_VOC);
		if (cust_aud_para == NULL)
		{
			LOGE("get_app_aud_cust_para fail\n");
		} else
		{
			bk_aud_debug_get_audpara(cust_aud_para, AUD_SERVICE_AI_VOC);
		}

#if CONFIG_VOICE_SERVICE_EQ
		if (voice_cfg->eq_en)
		{
			if (cust_aud_para && cust_aud_para->eq_dl_config.app_eq_en)
			{
				if (1)//(spk_sample_rate == cust_aud_para->eq_dl_config.eq_load.samplerate)
				{
					voice_cfg->eq_en = cust_aud_para->eq_dl_config.eq_en;
					voice_cfg->eq_cfg.eq_alg_cfg.eq_cal_para.eq_en       = cust_aud_para->eq_dl_config.eq_en;
					voice_cfg->eq_cfg.eq_alg_cfg.eq_cal_para.filters     = cust_aud_para->eq_dl_config.filters;
					voice_cfg->eq_cfg.eq_alg_cfg.eq_cal_para.globle_gain = cust_aud_para->eq_dl_config.globle_gain;
					os_memcpy(&voice_cfg->eq_cfg.eq_alg_cfg.eq_cal_para.eq_para, &cust_aud_para->eq_dl_config.eq_para, sizeof(eq_para_t)*cust_aud_para->eq_dl_config.filters);
					os_memcpy(&voice_cfg->eq_cfg.eq_alg_cfg.eq_cal_para.eq_load, &cust_aud_para->eq_dl_config.eq_load, sizeof(app_eq_load_t));
				} else
				{
					voice_cfg->eq_en = 0;
					LOGE("voice dl eq init fail, spk_sample_rate not match\n");
				}
			}
		}
#endif

		if (voice_cfg->mic_type == MIC_TYPE_ONBOARD)
		{
			if (cust_aud_para && cust_aud_para->sys_config.app_sys_en)
			{
				// voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.ana_gain = cust_aud_para->sys_config.mic0_analog_gain;
				// voice_cfg->mic_cfg.onboard_mic_cfg.adc_cfg.dig_gain = cust_aud_para->sys_config.mic0_digital_gain;
			}
		}
		if (voice_cfg->spk_type == SPK_TYPE_ONBOARD)
		{
			if (cust_aud_para && cust_aud_para->sys_config.app_sys_en)
			{
				// voice_cfg->spk_cfg.onboard_spk_cfg.ana_gain = cust_aud_para->sys_config.speaker_chan0_analog_gain;
				// voice_cfg->spk_cfg.onboard_spk_cfg.dig_gain = cust_aud_para->sys_config.speaker_chan0_digital_gain;
			}
		}
		if (voice_cfg->aec_en)
		{
			if (cust_aud_para && cust_aud_para->aec_v3_config.app_aec_en)
			{
				voice_cfg->aec_en = cust_aud_para->aec_v3_config.aec_enable;
				voice_cfg->aec_cfg.aec_alg_cfg.aec_cfg.delay_points = cust_aud_para->aec_v3_config.mic_delay;
				voice_cfg->aec_cfg.aec_alg_cfg.aec_cfg.ec_depth     = cust_aud_para->aec_v3_config.ec_depth;
				voice_cfg->aec_cfg.aec_alg_cfg.aec_cfg.ref_scale    = cust_aud_para->aec_v3_config.ref_scale;
				voice_cfg->aec_cfg.aec_alg_cfg.aec_cfg.ns_level     = cust_aud_para->aec_v3_config.ns_level;
				voice_cfg->aec_cfg.aec_alg_cfg.aec_cfg.ns_para      = cust_aud_para->aec_v3_config.ns_para;
			}
		}
	}
}
#endif

#if CONFIG_ADK_AEC_V3_ALGORITHM_COMPONENT_V2
static int bk_aec_phase_callback(int32_t phs_sm2, int vad_state)
{
    // LOGI("phs_sm2=%d\r\n", (int)phs_sm2);
    return 1;
}
#endif

int audio_engine_start(audio_engine_cfg_t *cfg)
{
    int ret = 0;

    if (!cfg) {
        LOGE("Invalid configuration pointer\n");
        return AUDIO_ENGINE_ERR_INVALID_PARAM;
    }

    /* Check if already started */
    if (g_audio_engine.is_started) {
        LOGD("Audio engine already started\n");
        return AUDIO_ENGINE_SUCCESS;
    }

    os_memcpy(&g_audio_engine_cfg, cfg, sizeof(audio_engine_cfg_t));

    /* Validate parameters */
    if (cfg->mic_sample_rate != 8000 && cfg->mic_sample_rate != 16000) {
        LOGE("Invalid mic sample rate: %d\n", cfg->mic_sample_rate);
        return AUDIO_ENGINE_ERR_INVALID_PARAM;
    }

    if (cfg->spk_sample_rate != 8000 && cfg->spk_sample_rate != 16000) {
        LOGE("Invalid spk sample rate: %d\n", cfg->spk_sample_rate);
        return AUDIO_ENGINE_ERR_INVALID_PARAM;
    }

    /* Initialize voice configuration */
    voice_cfg_t *voice_cfg = (voice_cfg_t *)os_malloc(sizeof(voice_cfg_t));
    if (!voice_cfg) {
        LOGE("Failed to allocate memory for voice_cfg\n");
        return AUDIO_ENGINE_ERR_INIT_FAILED;
    }
    os_memset(voice_cfg, 0x00, sizeof(voice_cfg_t));

    /* Configure microphone */
    voice_cfg->mic_type = MIC_TYPE_ONBOARD;
    onboard_mic_stream_cfg_t onboard_mic_cfg = ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT();
    // onboard_mic_cfg.adc_cfg.dig_gain = 0x30;
    // onboard_mic_cfg.adc_cfg.ana_gain = 0x08;
    onboard_mic_cfg.adc_cfg.sample_rate = cfg->mic_sample_rate;
    /* one frame size, 20ms */
    if (cfg->mic_sample_rate == 8000) {
            onboard_mic_cfg.frame_size = 160;
    } else {
            onboard_mic_cfg.frame_size = 320*2;
    }

#if CONFIG_ADK_ONBOARD_MIC_STREAM_V2
    onboard_mic_cfg.ch_bitmap = ONBOARD_MIC_ADC_DEFAULT_ACTIVE_CH_BITS;
    onboard_mic_cfg.adc_cfg.chl_num = 0;
    for(uint32_t j = 0; j < AUD_ADC_CHL_MAX; j++)
    {
        if(onboard_mic_cfg.ch_bitmap & (1 << j))
        {
            onboard_mic_cfg.adc_cfg.chl_num++;
        }
    }
    onboard_mic_cfg.adc_cfg.aec_en = cfg->aec_enable;
#endif

    voice_cfg->mic_cfg.onboard_mic_cfg = onboard_mic_cfg;

    /* Configure AEC */
    voice_cfg->aec_en = cfg->aec_enable;
    aec_v3_algorithm_cfg_t aec_cfg;
    if (cfg->aec_enable) {
        aec_cfg = (aec_v3_algorithm_cfg_t)DEFAULT_AEC_V3_ALGORITHM_CONFIG();
        aec_cfg.aec_cfg.mode = AEC_MODE_HARDWARE;
        if (aec_cfg.aec_cfg.mode == AEC_MODE_HARDWARE)
        {
            //onboard_mic_cfg.adc_cfg.chl_num = 2;
        #if CONFIG_ADK_AEC_V3_ALGORITHM_COMPONENT_V2
            aec_cfg.aec_cfg.ns_type        = NS_TRADITION;
            aec_cfg.aec_phase_cb           = bk_aec_phase_callback;
            aec_cfg.aec_cfg.ec_only_output = 1;
            aec_cfg.aec_cfg.multi_output_use_ec_out = 1;
            aec_cfg.dual_ch            = 1;
            aec_cfg.multi_in_port_num  = 0;
            aec_cfg.vad_cfg.vad_enable = 1;
            aec_cfg.vad_cfg.vad_eng_threshold = 500;
        #endif
            voice_cfg->mic_cfg.onboard_mic_cfg = onboard_mic_cfg;
        }

        voice_cfg->aec_cfg.aec_alg_cfg = aec_cfg;
    }

    /* Configure encoder */
    voice_cfg->enc_en = true;
    voice_cfg->enc_type = cfg->enc_type;
    if (cfg->enc_type == AUDIO_ENC_TYPE_G711A) {
        g711_encoder_cfg_t g711_enc_cfg = DEFAULT_G711_ENCODER_CONFIG();
        if (cfg->mic_sample_rate == 8000) {
            g711_enc_cfg.buf_sz = 160;
            g711_enc_cfg.out_block_size = 160;
        } else {
            g711_enc_cfg.buf_sz = 320;
            g711_enc_cfg.out_block_size = 320;
        }
        voice_cfg->enc_cfg.g711_enc_cfg = g711_enc_cfg;
        voice_cfg->read_pool_size = (cfg->mic_sample_rate == 8000) ? 160 : 320;
    }
    #if CONFIG_VOICE_SERVICE_G722_ENCODER
    else if (cfg->enc_type == AUDIO_ENC_TYPE_G722) {
        g722_encoder_cfg_t g722_enc_cfg = DEFAULT_G722_ENCODER_CONFIG();
        if (cfg->mic_sample_rate == 8000) {
            g722_enc_cfg.buf_sz = 160;
            g722_enc_cfg.out_block_size = 80;
        } else {
            g722_enc_cfg.buf_sz = 320;
            g722_enc_cfg.out_block_size = 160;
        }
        voice_cfg->enc_cfg.g722_enc_cfg = g722_enc_cfg;
        voice_cfg->read_pool_size = (cfg->mic_sample_rate == 8000) ? 160 : 320;
    }
    #endif
    #if CONFIG_VOICE_SERVICE_OPUS_ENCODER
    else if (cfg->enc_type == AUDIO_ENC_TYPE_OPUS)
    {
        opus_enc_cfg_t opus_enc_cfg = DEFAULT_OPUS_ENC_CONFIG();
        voice_cfg->enc_cfg.opus_enc_cfg = opus_enc_cfg;
    }
    #endif
    else if (cfg->enc_type == AUDIO_ENC_TYPE_PCM) {
        voice_cfg->read_pool_size = (cfg->mic_sample_rate == 8000) ? 320 : 640;
    }

    /* Configure decoder */
    voice_cfg->dec_en = true;
    voice_cfg->dec_type = cfg->dec_type;
    if (cfg->dec_type == AUDIO_DEC_TYPE_G711A) {
        g711_decoder_cfg_t g711_dec_cfg = DEFAULT_G711_DECODER_CONFIG();
        if (cfg->spk_sample_rate == 8000) {
            g711_dec_cfg.buf_sz = 160;
            g711_dec_cfg.out_block_size = 320;
        } else {
            g711_dec_cfg.buf_sz = 320;
            g711_dec_cfg.out_block_size = 640;
        }
        voice_cfg->dec_cfg.g711_dec_cfg = g711_dec_cfg;
        voice_cfg->write_pool_size = (cfg->spk_sample_rate == 8000) ? 160 : 320;
    }
    #if CONFIG_VOICE_SERVICE_G722_DECODER
    else if (cfg->dec_type == AUDIO_DEC_TYPE_G722) {
        g722_decoder_cfg_t g722_dec_cfg = DEFAULT_G722_DECODER_CONFIG();
        if (cfg->spk_sample_rate == 8000) {
            g722_dec_cfg.buf_sz = 80;
            g722_dec_cfg.out_block_size = 320;
        } else {
            g722_dec_cfg.buf_sz = 160;
            g722_dec_cfg.out_block_size = 640;
        }
        voice_cfg->dec_cfg.g722_dec_cfg = g722_dec_cfg;
        voice_cfg->write_pool_size = (cfg->spk_sample_rate == 8000) ? 80 : 160;
    }
    #endif
    #if CONFIG_VOICE_SERVICE_OPUS_DECODER
    else if (cfg->dec_type == AUDIO_DEC_TYPE_OPUS)
    {
        opus_dec_cfg_t opus_dec_cfg = DEFAULT_OPUS_DEC_CONFIG();
        voice_cfg->dec_cfg.opus_dec_cfg = opus_dec_cfg;
    }
#endif
    else if (cfg->dec_type == AUDIO_DEC_TYPE_PCM) {
        voice_cfg->write_pool_size = (cfg->spk_sample_rate == 8000) ? 320 : 640;
    }

    /* Configure speaker */
    voice_cfg->spk_type = SPK_TYPE_ONBOARD;
    onboard_speaker_stream_cfg_t onboard_spk_cfg = ONBOARD_SPEAKER_STREAM_CFG_DEFAULT();
#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2
    for (uint32_t i = 0; i < AUD_DAC_SOURCE_MAX; i++)
    {
        onboard_spk_cfg.sample_rate[i] = cfg->spk_sample_rate;
        onboard_spk_cfg.frame_size[i]  = (cfg->spk_sample_rate == 8000) ? 320 : 640;
    }
    onboard_spk_cfg.dac_source_bitmap = ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT | ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT;
#else
    onboard_spk_cfg.sample_rate = cfg->spk_sample_rate;
    onboard_spk_cfg.frame_size = (cfg->spk_sample_rate == 8000) ? 320 : 640;
#endif

    onboard_spk_cfg.pa_ctrl_en   = cfg->pa_enable;
    onboard_spk_cfg.pa_ctrl_gpio = cfg->pa_gpio;
    onboard_spk_cfg.pa_on_level  = cfg->pa_on_level;
    onboard_spk_cfg.pa_on_delay  = cfg->pa_on_delay;
    onboard_spk_cfg.pa_off_delay = cfg->pa_off_delay;
    // onboard_spk_cfg.dig_gain = cfg->dig_gain;
    // onboard_spk_cfg.ana_gain = cfg->ana_gain;
#if CONFIG_AE_SUPPORT_PROMPT_TONE
    onboard_spk_cfg.multi_in_port_num++;
#endif

    voice_cfg->spk_cfg.onboard_spk_cfg = onboard_spk_cfg;

    /* Configure EQ */
#if CONFIG_VOICE_SERVICE_EQ
    if (cfg->eq_enable > 0) {
        voice_cfg->eq_en = true;
        eq_algorithm_cfg_t eq_cfg = DEFAULT_EQ_ALGORITHM_CONFIG();
        eq_cfg.eq_chl_num = cfg->eq_enable;
        voice_cfg->eq_cfg.eq_alg_cfg = eq_cfg;
    } else {
        voice_cfg->eq_en = false;
    }
#endif

#if (CONFIG_ASR_SERVICE)
    asr_cfg_t asr_cfg = {0};
    asr_cfg_t t_asr_cfg = ASR_BY_ONBOARD_MIC_CFG_DEFAULT();
    asr_cfg = t_asr_cfg;
    asr_cfg.asr_en = true;
    if (cfg->mic_sample_rate != asr_cfg.asr_sample_rate)
    {
    #if CONFIG_ADK_RSP_ALGORITHM
        asr_cfg.asr_rsp_en = true;
        asr_cfg.rsp_cfg.rsp_alg_cfg.rsp_cfg.src_rate = cfg->mic_sample_rate;
    #else
        asr_cfg.asr_rsp_en = false;
        LOGE("Need Open the aud resample Macro\n");
        goto cleanup_asr;
    #endif
    } else
    {
        asr_cfg.asr_rsp_en = false;
    }
    if (voice_cfg->aec_en) {
        voice_cfg->aec_cfg.aec_alg_cfg.multi_out_port_num++;
    } else
    {
        voice_cfg->mic_cfg.onboard_mic_cfg.multi_out_port_num++;
    }
#endif

#if CONFIG_AUDIO_PARA
    bk_audio_set_voc_cust_params(voice_cfg, AUD_SERVICE_AI_VOC);
#endif

    /* Configure event callback */
    voice_cfg->event_handle = cfg->event_cb;
    voice_cfg->args = cfg->user_data;

    /* Initialize voice service */
    g_audio_engine.voice_handle = bk_voice_init(voice_cfg);
    if (!g_audio_engine.voice_handle) {
        LOGE("Voice init failed\n");
        ret = AUDIO_ENGINE_ERR_VOICE_INIT;
        goto cleanup;
    }

#if (CONFIG_ASR_SERVICE)
    if (asr_cfg.asr_en == true)
    {
        asr_cfg.args		 = NULL;
        asr_cfg.event_handle = NULL;
        g_audio_engine.asr_handle = bk_asr_create(&asr_cfg);
        if (!g_audio_engine.asr_handle)
        {
            LOGE("asr init fail\n");
            goto cleanup_asr;
        }
        g_audio_engine.asr_handle->mic_str = (audio_element_handle_t)bk_voice_get_mic_str(g_audio_engine.voice_handle, voice_cfg);
        if (cfg->mic_sample_rate == 16000) {
            asr_cfg.read_pool_size = cfg->mic_sample_rate * 2 * 20 / 1000;
        }
        else if (cfg->mic_sample_rate == 8000) {
            asr_cfg.read_pool_size = 2 * cfg->mic_sample_rate * 2 * 20 / 1000;
        }

        bk_asr_init(&asr_cfg, g_audio_engine.asr_handle);

        {
            aud_asr_cfg_t aud_asr_cfg = AUDIO_ASR_CFG_DEFAULT();
            aud_asr_cfg.asr_handle	  = g_audio_engine.asr_handle;
            aud_asr_cfg.aud_asr_result_handle = bk_audio_engine_asr_result_handle;
            #if CONFIG_WANSON_ARMINO_ASR
            aud_asr_cfg.aud_asr_init    = bk_wanson_asr_common_init;
            aud_asr_cfg.aud_asr_deinit  = bk_wanson_asr_common_deinit;
            aud_asr_cfg.aud_asr_recog   = bk_wanson_asr_recog;
            aud_asr_cfg.max_read_size   = 960;
            #elif CONFIG_BEKEN_KWS
            aud_asr_cfg.aud_asr_init   = bk_tflite_asr_init;
            aud_asr_cfg.aud_asr_deinit = NULL;
            aud_asr_cfg.aud_asr_recog  = bk_tflite_asr_recog;
            aud_asr_cfg.max_read_size  = 1280;
            aud_asr_cfg.task_stack     = 25 * 1024;
            aud_asr_cfg.mem_type       = AUDIO_MEM_TYPE_SRAM;
            #endif
            aud_asr_cfg.p1                    = (void *)&g_audio_engine_asr_text;
            aud_asr_cfg.p2                    = (void *)&g_audio_engine_asr_score;
            g_audio_engine.aud_asr_handle = bk_aud_asr_init(&aud_asr_cfg);
            if (!g_audio_engine.aud_asr_handle)
            {
                LOGE("aud asr init fail\n");
                goto cleanup_asr;
            }
        }
    }
#endif

#if CONFIG_AUDIO_PARA
    bk_app_aud_get_service_handle((void *)g_audio_engine.voice_handle, AUD_SERVICE_AI_VOC);
    set_app_aud_cust_service_handle((void *)g_audio_engine.voice_handle, AUD_SERVICE_AI_VOC);
#endif

    /* Initialize voice read service */
    voice_read_cfg_t voice_read_cfg = VOICE_READ_CFG_DEFAULT();
    voice_read_cfg.voice_handle = g_audio_engine.voice_handle;
    voice_read_cfg.max_read_size = (cfg->mic_sample_rate * 2 * 20 / 1000) / 4; // one frame size(20ms)
    voice_read_cfg.voice_read_callback = cfg->read_cb;
    voice_read_cfg.args = cfg->user_data;

    g_audio_engine.read_handle = bk_voice_read_init(&voice_read_cfg);
    if (!g_audio_engine.read_handle) {
        LOGE("Voice read init failed\n");
        ret = AUDIO_ENGINE_ERR_READ_INIT;
        goto cleanup_voice;
    }

    /* Initialize voice write service */
    voice_write_cfg_t voice_write_cfg = VOICE_WRITE_CFG_DEFAULT();
    voice_write_cfg.voice_handle = g_audio_engine.voice_handle;
    voice_write_cfg.node_size = voice_cfg->write_pool_size;
    voice_write_cfg.node_num = 10;// 10 frames buffer

    #if CONFIG_VOICE_SERVICE_OPUS_DECODER
    if(cfg->enc_type == AUDIO_ENC_TYPE_OPUS)
    {
        voice_write_cfg.write_buf_type = PORT_TYPE_FB;
        voice_write_cfg.node_size = 160;
        voice_write_cfg.node_num = 16;
    }
    #endif
    g_audio_engine.write_handle = bk_voice_write_init(&voice_write_cfg);
    if (!g_audio_engine.write_handle) {
        LOGE("Voice write init failed\n");
        ret = AUDIO_ENGINE_ERR_WRITE_INIT;
        goto cleanup_read;
    }

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_V2
    if (BK_OK != audio_engine_adjust_voice_gain(g_volume_gain[g_volume_level])) {
        LOGE("apply initial speaker digital gain failed\n");
    }
#endif

    /* Start voice service */
    if (BK_OK != bk_voice_start(g_audio_engine.voice_handle)) {
        LOGE("Voice start failed\n");
        ret = AUDIO_ENGINE_ERR_VOICE_START;
        goto cleanup_write;
    }

    /* Start voice read service */
    if (BK_OK != bk_voice_read_start(g_audio_engine.read_handle)) {
        LOGE("Voice read start failed\n");
        ret = AUDIO_ENGINE_ERR_READ_START;
        goto cleanup_voice_start;
    }

    /* Start voice write service */
    if (BK_OK != bk_voice_write_start(g_audio_engine.write_handle)) {
        LOGE("Voice write start failed\n");
        ret = AUDIO_ENGINE_ERR_WRITE_START;
        goto cleanup_read_start;
    }

#if (CONFIG_ASR_SERVICE)
    if (asr_cfg.asr_en == true)
    {
        if (BK_OK != bk_asr_start(g_audio_engine.asr_handle))
        {
            LOGE("asr start fail\n");
            goto cleanup_asr;
        }
        if (BK_OK != bk_aud_asr_start(g_audio_engine.aud_asr_handle))
        {
            LOGE("aud asr start fail\n");
            goto cleanup_asr;
        }
    }
#endif
    if(voice_cfg)
    {
        os_free(voice_cfg);
        voice_cfg = NULL;
    }
    g_audio_engine.is_started = true;
    LOGI("Audio engine started successfully\n");
    return AUDIO_ENGINE_SUCCESS;

cleanup_read_start:
    bk_voice_read_stop(g_audio_engine.read_handle);
cleanup_voice_start:
    bk_voice_stop(g_audio_engine.voice_handle);
cleanup_write:
    bk_voice_write_deinit(g_audio_engine.write_handle);
    g_audio_engine.write_handle = NULL;
cleanup_read:
    bk_voice_read_deinit(g_audio_engine.read_handle);
    g_audio_engine.read_handle = NULL;
cleanup_voice:
    bk_voice_deinit(g_audio_engine.voice_handle);
    g_audio_engine.voice_handle = NULL;
#if (CONFIG_ASR_SERVICE)
cleanup_asr:
    if (g_audio_engine.aud_asr_handle) {
        bk_aud_asr_stop(g_audio_engine.aud_asr_handle);
    }
    if (g_audio_engine.asr_handle) {
        bk_asr_stop(g_audio_engine.asr_handle);
    }
    if (g_audio_engine.aud_asr_handle) {
        bk_aud_asr_deinit(g_audio_engine.aud_asr_handle);
    }
    if (g_audio_engine.asr_handle) {
        bk_asr_deinit(g_audio_engine.asr_handle);
    }
    g_audio_engine.asr_handle = NULL;
    g_audio_engine.aud_asr_handle = NULL;
#endif

cleanup:
    if(voice_cfg)
    {
        os_free(voice_cfg);
        voice_cfg = NULL;
    }
    return ret;
}

/**
 * @brief Stop audio engine and cleanup resources
 * 
 * @return int 
 *         - 0: Success
 *         - -1: Audio engine not started
 *         - -2: Stop operations failed
 */
int audio_engine_stop(void)
{
    if (!g_audio_engine.is_started) {
        LOGD("Audio engine not started\n");
        return AUDIO_ENGINE_ERR_NOT_STARTED;
    }

    int ret = AUDIO_ENGINE_SUCCESS;

#if (CONFIG_ASR_SERVICE)
    if (g_audio_engine.aud_asr_handle) {
        if (BK_OK != bk_aud_asr_stop(g_audio_engine.aud_asr_handle)) {
            LOGE("aud asr stop failed\n");
            ret = AUDIO_ENGINE_ERR_ASR_STOP;
        }
    }
    if (g_audio_engine.asr_handle) {
        if (BK_OK != bk_asr_stop(g_audio_engine.asr_handle)) {
            LOGE("asr stop failed\n");
            ret = AUDIO_ENGINE_ERR_ASR_STOP;
        }
    }
    if (g_audio_engine.aud_asr_handle) {
        bk_aud_asr_deinit(g_audio_engine.aud_asr_handle);
    }
    if (g_audio_engine.asr_handle) {
        bk_asr_deinit(g_audio_engine.asr_handle);
    }
    g_audio_engine.asr_result = 0;
    g_audio_engine.asr_handle = NULL;
    g_audio_engine.aud_asr_handle = NULL;
#endif

#if CONFIG_AE_SUPPORT_PROMPT_TONE
    if (BK_OK != audio_engine_prompt_tone_deinit(g_audio_engine_prompt_tone))
    {
        LOGE("deinit audio engine prompt tone fail\n");
        ret = BK_FAIL;
    }
    g_audio_engine_prompt_tone = NULL;
#endif

    /* Stop voice write service */
    if (g_audio_engine.write_handle) {
        if (BK_OK != bk_voice_write_stop(g_audio_engine.write_handle)) {
            LOGE("Voice write stop failed\n");
            ret = AUDIO_ENGINE_ERR_WRITE_STOP;
        }
    }

    /* Stop voice read service */
    if (g_audio_engine.read_handle) {
        if (BK_OK != bk_voice_read_stop(g_audio_engine.read_handle)) {
            LOGE("Voice read stop failed\n");
            ret = AUDIO_ENGINE_ERR_READ_STOP;
        }
    }

    /* Stop voice service */
    if (g_audio_engine.voice_handle) {
        if (BK_OK != bk_voice_stop(g_audio_engine.voice_handle)) {
            LOGE("Voice stop failed\n");
            ret = AUDIO_ENGINE_ERR_VOICE_STOP;
        }
    }

    /* Deinit voice write service */
    if (g_audio_engine.write_handle) {
        bk_voice_write_deinit(g_audio_engine.write_handle);
        g_audio_engine.write_handle = NULL;
    }

    /* Deinit voice read service */
    if (g_audio_engine.read_handle) {
        bk_voice_read_deinit(g_audio_engine.read_handle);
        g_audio_engine.read_handle = NULL;
    }

    /* Deinit voice service */
    if (g_audio_engine.voice_handle) {
        bk_voice_deinit(g_audio_engine.voice_handle);
        g_audio_engine.voice_handle = NULL;
    }

    g_audio_engine.is_started = false;
    LOGI("Audio engine stopped successfully\n");
    return ret;
}

/**
 * @brief Check if audio engine is currently running
 * 
 * @return bool 
 *         - true: Audio engine is running
 *         - false: Audio engine is not running
 */
bool audio_engine_is_running(void)
{
    return g_audio_engine.is_started;
}
/**
 * @brief Get audio engine encoder type
 * 
 * @return audio_enc_type_t 
 */
audio_enc_type_t audio_engine_get_encoder_type(void)
{
    if (!g_audio_engine.is_started) {
        return AUDIO_ENC_TYPE_INVALID;
    }
    return g_audio_engine_cfg.enc_type;
}
/**
 * @brief Get audio engine decoder type
 * 
 * @return audio_dec_type_t 
 */
audio_dec_type_t audio_engine_get_decoder_type(void)
{
    if (!g_audio_engine.is_started) {
        return AUDIO_DEC_TYPE_INVALID;
    }
    return g_audio_engine_cfg.dec_type;
}
/**
 * @brief Write audio data to speaker
 * 
 * @param data Pointer to audio data
 * @param size Size of audio data in bytes
 * @param timeout_ms Timeout in milliseconds
 * @return int 
 *         - 0: Success
 *         - < 0: Error codes
 */
/* Voice read callback example */
static int voice_read_callback(unsigned char *data, unsigned int len, void *args)
{
    int ret = 0;

    #if CONFIG_BK_NETWORK_TRANSFER
    #if (CONFIG_ASR_SERVICE)
    if (g_audio_engine.asr_result == 1)
    #endif
    {
        ret = ntwk_trans_send_audio(data, len, g_audio_engine_cfg.enc_type);
    }
    #else
    ret = bk_voice_write_frame_data(g_audio_engine.write_handle, (char *)data, len);
    #endif

    if (ret != len)
    {
        //LOGE("%s, %d, bk_voice_write_frame_data: %d != %d\n", __func__, __LINE__, ret, len);
    }
    else
    {
        //LOGD("%s, %d, len: %d\n", __func__, __LINE__, len);
    }

    return ret;
}
int audio_engine_write_data(const uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    if (!g_audio_engine.is_started || !g_audio_engine.write_handle) {
        return AUDIO_ENGINE_ERR_NOT_STARTED;
    }

    #if (CONFIG_ASR_SERVICE)
    if (g_audio_engine.asr_result != 1)
    {
        return AUDIO_ENGINE_ERR_ASR_STOP;
    }
    #endif
    return bk_voice_write_frame_data(g_audio_engine.write_handle, (char *)data, size);
}

/**
 * @brief Get audio engine error string
 * 
 * @param err Error code
 * @return const char* Error description string
 */
const char *audio_engine_err_to_str(int err)
{
    switch (err) {
        case AUDIO_ENGINE_SUCCESS:
            return "Success";
        case AUDIO_ENGINE_ERR_INVALID_PARAM:
            return "Invalid parameters";
        case AUDIO_ENGINE_ERR_NOT_STARTED:
            return "Audio engine not started";
        case AUDIO_ENGINE_ERR_VOICE_INIT:
            return "Voice init failed";
        case AUDIO_ENGINE_ERR_READ_INIT:
            return "Voice read init failed";
        case AUDIO_ENGINE_ERR_WRITE_INIT:
            return "Voice write init failed";
        case AUDIO_ENGINE_ERR_VOICE_START:
            return "Voice start failed";
        case AUDIO_ENGINE_ERR_READ_START:
            return "Voice read start failed";
        case AUDIO_ENGINE_ERR_WRITE_START:
            return "Voice write start failed";
        case AUDIO_ENGINE_ERR_VOICE_STOP:
            return "Voice stop failed";
        case AUDIO_ENGINE_ERR_READ_STOP:
            return "Voice read stop failed";
        case AUDIO_ENGINE_ERR_WRITE_STOP:
            return "Voice write stop failed";
        default:
            return "Unknown error";
    }
}
/* Voice event callback example */
static bk_err_t voice_event_callback(voice_evt_t event, void *data, void *args)
{
    LOGI("Voice event: %d\n", event);
    return BK_OK;
}

/**
 * @brief Convert string to audio encoder type
 * 
 * @param enc_str Encoder type string ("PCM", "G711A", "G711U", "G722", "OPUS")
 * @return audio_enc_type_t Encoder type enum value
 */
audio_enc_type_t audio_engine_str_to_enc_type(const char *enc_str)
{
    if (!enc_str) {
        LOGE("Invalid encoder type string pointer\n");
        return AUDIO_ENC_TYPE_INVALID;
    }

    if (strcasecmp(enc_str, "PCM") == 0) {
        return AUDIO_ENC_TYPE_PCM;
    } else if (strcasecmp(enc_str, "G711A") == 0) {
        return AUDIO_ENC_TYPE_G711A;
    } else if (strcasecmp(enc_str, "G711U") == 0) {
        return AUDIO_ENC_TYPE_G711U;
    } else if (strcasecmp(enc_str, "G722") == 0) {
        return AUDIO_ENC_TYPE_G722;
    } else if (strcasecmp(enc_str, "OPUS") == 0) {
        return AUDIO_ENC_TYPE_OPUS;
    } else {
        LOGE("Unknown encoder type string: %s\n", enc_str);
        return AUDIO_ENC_TYPE_INVALID;
    }
}

/**
 * @brief Convert string to audio decoder type
 * 
 * @param dec_str Decoder type string ("PCM", "G711A", "G711U", "G722", "OPUS")
 * @return audio_dec_type_t Decoder type enum value
 */
audio_dec_type_t audio_engine_str_to_dec_type(const char *dec_str)
{
    if (!dec_str) {
        LOGE("Invalid decoder type string pointer\n");
        return AUDIO_DEC_TYPE_INVALID;
    }

    if (strcasecmp(dec_str, "PCM") == 0) {
        return AUDIO_DEC_TYPE_PCM;
    } else if (strcasecmp(dec_str, "G711A") == 0) {
        return AUDIO_DEC_TYPE_G711A;
    } else if (strcasecmp(dec_str, "G711U") == 0) {
        return AUDIO_DEC_TYPE_G711U;
    } else if (strcasecmp(dec_str, "G722") == 0) {
        return AUDIO_DEC_TYPE_G722;
    } else if (strcasecmp(dec_str, "OPUS") == 0) {
        return AUDIO_DEC_TYPE_OPUS;
    } else {
        LOGE("Unknown decoder type string: %s\n", dec_str);
        return AUDIO_DEC_TYPE_INVALID;
    }
}

int audio_engine_init(void)
{
    audio_engine_cfg_t cfg = {
        .mic_sample_rate = CONFIG_AE_AUDIO_ADC_SAMP_RATE,
        .spk_sample_rate = CONFIG_AE_AUDIO_DAC_SAMP_RATE,
        .aec_enable   = 3,
        .eq_enable    = 0,
        .enc_type     = AUDIO_ENC_TYPE_INVALID,
        .dec_type     = AUDIO_DEC_TYPE_INVALID,
        .event_cb     = voice_event_callback,
        .read_cb      = voice_read_callback,
        .user_data    = NULL,
        .pa_enable    = CONFIG_AE_ENABLE_PA_CNTRL,
        .pa_gpio      = CONFIG_AE_PA_CNTRL_GPIO,
        .pa_on_level  = CONFIG_AE_PA_ON_LEVEL,
        .pa_on_delay  = CONFIG_AE_PA_ON_DELAY,
        .pa_off_delay = CONFIG_AE_PA_OFF_DELAY,
        // .dig_gain = CONFIG_AE_DEFAULT_DIG_GAIN,
        // .ana_gain = CONFIG_AE_DEFAULT_ANA_GAIN,
    };

    if (AUDIO_ENGINE_SUCCESS != audio_engine_volume_init()) {
        LOGE("Failed to init volume\n");
        return AUDIO_ENGINE_ERR_INIT_FAILED;
    }

    LOGD("audio encoder type: %s\n", CONFIG_AE_AUDIO_ENCODER_TYPE);
    LOGD("audio decoder type: %s\n", CONFIG_AE_AUDIO_DECODER_TYPE);
    cfg.enc_type = audio_engine_str_to_enc_type(CONFIG_AE_AUDIO_ENCODER_TYPE);
    cfg.dec_type = audio_engine_str_to_dec_type(CONFIG_AE_AUDIO_DECODER_TYPE);

    int ret = audio_engine_start(&cfg);
    if (ret != AUDIO_ENGINE_SUCCESS) {
        LOGE("Failed to start audio engine: %s\n", audio_engine_err_to_str(ret));
        return ret;
    }

#if CONFIG_AE_SUPPORT_PROMPT_TONE
    audio_engine_prompt_tone_cfg_t prompt_tone_cfg = DEFAULT_AUDIO_ENGINE_PROMPT_TONE_CONFIG();
    bk_voice_get_spkstr(g_audio_engine.voice_handle, &prompt_tone_cfg.spk_stream);
    g_audio_engine_prompt_tone = audio_engine_prompt_tone_init(&prompt_tone_cfg);
    if (!g_audio_engine_prompt_tone)
    {
        LOGE("Failed to init prompt tone\n");
        return AUDIO_ENGINE_ERR_INIT_FAILED;
    }

    extern int bk_aud_engine_cli_init(void);
    bk_aud_engine_cli_init();
#endif

    LOGI("audio engine started successfully\n");
    return AUDIO_ENGINE_SUCCESS;
}

int audio_engine_deinit(void)
{
    return audio_engine_stop();
}
