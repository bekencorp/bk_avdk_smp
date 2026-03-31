#ifndef _AUDIO_ENGINE_H_
#define _AUDIO_ENGINE_H_

#include <stdint.h>
#include <stdbool.h>
#include <components/bk_voice_service_types.h>
#include <components/bk_voice_service.h>
#include <components/bk_voice_read_service.h>
#include <components/bk_voice_write_service.h>

#if (CONFIG_ASR_SERVICE)
#include <components/bk_audio_asr_service.h>
#include <components/bk_audio_asr_service_types.h>
#include <components/bk_asr_service.h>
#include <components/bk_asr_service_types.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SPK_VOLUME_LEVEL        (11) //[0,10], 11 is max volume.

/* Global handles for audio engine */
struct audio_engine_ctx {
    voice_handle_t voice_handle;
    voice_read_handle_t read_handle;
    voice_write_handle_t write_handle;
#if (CONFIG_ASR_SERVICE)
    uint8_t asr_result;            /**< 1: wakeup, 2: standby */
    asr_handle_t asr_handle;
    aud_asr_handle_t aud_asr_handle;
#endif
    bool is_started;
};

/**
 * @brief Audio engine error codes
 */
typedef enum {
    AUDIO_ENGINE_SUCCESS = 0,            /**< Success */
    AUDIO_ENGINE_ERR_INIT_FAILED = -1,   /**< Init failed */
    AUDIO_ENGINE_ERR_INVALID_PARAM = -2, /**< Invalid parameters */
    AUDIO_ENGINE_ERR_NOT_STARTED = -3,   /**< Audio engine not started */
    AUDIO_ENGINE_ERR_VOICE_INIT = -4,    /**< Voice init failed */
    AUDIO_ENGINE_ERR_READ_INIT = -5,     /**< Voice read init failed */
    AUDIO_ENGINE_ERR_WRITE_INIT = -6,    /**< Voice write init failed */
    AUDIO_ENGINE_ERR_VOICE_START = -7,   /**< Voice start failed */
    AUDIO_ENGINE_ERR_READ_START = -8,    /**< Voice read start failed */
    AUDIO_ENGINE_ERR_WRITE_START = -9,   /**< Voice write start failed */
    AUDIO_ENGINE_ERR_VOICE_STOP = -10,   /**< Voice stop failed */
    AUDIO_ENGINE_ERR_READ_STOP = -11,    /**< Voice read stop failed */
    AUDIO_ENGINE_ERR_WRITE_STOP = -12,   /**< Voice write stop failed */
    AUDIO_ENGINE_ERR_ASR_INIT = -13,     /**< ASR init failed */
    AUDIO_ENGINE_ERR_ASR_START = -14,    /**< ASR start failed */
    AUDIO_ENGINE_ERR_ASR_STOP = -15,     /**< ASR stop failed */
} audio_engine_err_t;

/**
 * @brief Voice read callback function type
 */
typedef int (*audio_engine_read_callback_t)(unsigned char *data, unsigned int len, void *args);

/**
 * @brief Voice event callback function type
 */
typedef bk_err_t (*voice_event_callback_t)(voice_evt_t event, void *data, void *args);

/**
 * @brief Audio engine configuration structure
 */
typedef struct {
    uint32_t mic_sample_rate;      /**< 8000 or 16000 */
    uint32_t spk_sample_rate;      /**< 8000 or 16000 */
    
    /* Audio processing */
    uint8_t aec_enable;            /**< 0: disable, 1: enable AEC */
    uint8_t eq_enable;             /**< 0: disable, 1: mono EQ, 2: stereo EQ */

    /* Encoding/Decoding */
    audio_enc_type_t enc_type;     /**< AUDIO_ENC_TYPE_PCM, AUDIO_ENC_TYPE_G711A, AUDIO_ENC_TYPE_G711U, AUDIO_ENC_TYPE_G722 */
    audio_dec_type_t dec_type;     /**< AUDIO_DEC_TYPE_PCM, AUDIO_DEC_TYPE_G711A, AUDIO_DEC_TYPE_G711U, AUDIO_DEC_TYPE_G722 */
    
    /* Audio gain */
    int32_t dig_gain;
    uint8_t ana_gain;              /**< Audio dac analog gain */
    
    /* PA control */
    uint8_t pa_enable;             /**< 0: disable, 1: enable PA control */
    uint8_t pa_gpio;               /**< GPIO number for PA control */
    uint8_t pa_on_level;           /**< PA on level */
    uint32_t pa_on_delay;          /**< PA on delay in ms */
    uint32_t pa_off_delay;         /**< PA off delay in ms */
    
    /* Callback functions */
    voice_event_callback_t event_cb;   /**< Voice event callback function */
    audio_engine_read_callback_t read_cb;  /**< Audio engine read callback function */
    void *user_data;               /**< User data for callbacks */
} audio_engine_cfg_t;



/**
 * @brief Initialize and start audio engine
 * 
 * @param cfg Pointer to audio engine configuration structure
 * @return int 
 *         - 0: Success
 *         - < 0: Error codes (see audio_engine_err_t)
 */
int audio_engine_start(audio_engine_cfg_t *cfg);

/**
 * @brief Stop audio engine and cleanup resources
 * 
 * @return int 
 *         - 0: Success
 *         - < 0: Error codes (see audio_engine_err_t)
 */
int audio_engine_stop(void);

/** @brief Check if audio engine is currently running
 * 
 * @return bool 
 *         - true: Audio engine is running
 *         - false: Audio engine is not running
 */
bool audio_engine_is_running(void);

/**
 * @brief Write audio data to speaker
 * 
 * @param data Pointer to audio data
 * @param size Size of audio data in bytes
 * @param timeout_ms Timeout in milliseconds
 * @return int 
 *         - 0: Success
 *         - < 0: Error codes (see audio_engine_err_t)
 */
int audio_engine_write_data(const uint8_t *data, uint32_t size, uint32_t timeout_ms);

/**
 * @brief Get audio engine error string
 * 
 * @param err Error code
 * @return const char* Error description string
 */
const char *audio_engine_err_to_str(int err);

/**
 * @brief Get audio engine encoder type
 * 
 * @return audio_enc_type_t 
 */
audio_enc_type_t audio_engine_get_encoder_type(void);

/**
 * @brief Get audio engine decoder type
 * 
 * @return audio_dec_type_t 
 */
audio_dec_type_t audio_engine_get_decoder_type(void);

/**
 * @brief Convert string to audio encoder type
 * 
 * @param enc_str Encoder type string ("PCM", "G711A", "G711U", "G722", "OPUS")
 * @return audio_enc_type_t Encoder type enum value
 */
audio_enc_type_t audio_engine_str_to_enc_type(const char *enc_str);

/**
 * @brief Convert string to audio decoder type
 * 
 * @param dec_str Decoder type string ("PCM", "G711A", "G711U", "G722", "OPUS")
 * @return audio_dec_type_t Decoder type enum value
 */
audio_dec_type_t audio_engine_str_to_dec_type(const char *dec_str);
void audio_engine_volume_increase(void);
void audio_engine_volume_decrease(void);
int audio_engine_init(void);
int audio_engine_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* _AUDIO_ENGINE_H_ */
