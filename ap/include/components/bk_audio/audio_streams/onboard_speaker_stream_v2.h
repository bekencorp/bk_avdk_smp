// Copyright 2025-2026 Beken
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


#ifndef _ONBOARD_SPEAKER_STREAM_H_
#define _ONBOARD_SPEAKER_STREAM_H_

#include <components/audio_param_ctrl.h>
#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/bk_audio/audio_pipeline/audio_port_info_list.h>
#include <driver/aud_dac_types.h>

#include <modules/audio_rsp.h>
#include <modules/audio_rsp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Onboard speaker runtime status.
 */
typedef struct
{
    bool is_playing;        /*!< valid voice output state based on threshold + hysteresis */
    uint8_t energy_level;   /*!< energy level mapped to 0~100 for UI meter */
} onboard_speaker_stream_status_t;

/**
 * @brief      Onboard speaker status callback.
 *
 * @param[in]  onboard_speaker_stream  element handle
 * @param[in]  status                  current speaker status
 * @param[in]  user_data               user private data passed by register api
 */
typedef void (*onboard_speaker_status_cb_t)(audio_element_handle_t onboard_speaker_stream,
                                            const onboard_speaker_stream_status_t *status,
                                            void *user_data);

/**
 * @brief   Onboard Speaker Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    uint8_t                 chl_num;            /*!< speaker channel number */
    uint32_t                sample_rate[AUD_DAC_SOURCE_MAX];/*!< speaker sample rate */
    float                   dig_gain;           /*!< audio dac digital gain in dB */
    int32_t                 ana_gain;           /*!< audio dac analog gain in dB (integer step) */
    aud_dac_work_mode_t     work_mode;          /*!< audio dac mode: signal_ended/differen */
    uint8_t                 bits;               /*!< Bit wide (8, 16, 24, 32 bits) */
    aud_clk_t               clk_src;            /*!< audio clock: XTAL(26MHz)/APLL */
    int                     multi_in_port_num;  /*!< The number of multiple input audio port */
    int                     multi_out_port_num; /*!< The number of multiple output audio port */
    uint32_t                frame_size[AUD_DAC_SOURCE_MAX];/*!< the length of one frame speaker data dma carried (suggest 20ms data) */
    uint32_t                pool_length;        /*!< speaker data pool size, the unit is byte */
    uint32_t                pool_play_thold;    /*!< the play threshold of pool, the unit is byte */
    uint32_t                pool_pause_thold;   /*!< the pause threshold of pool, the unit is byte */
    bool                    pa_ctrl_en;         /*!< control pa enable */
    uint16_t                pa_ctrl_gpio;       /*!< the gpio id of control pa */
    uint8_t                 pa_on_level;        /*!< the gpio level of turn on pa, 0: low level, 1: high level */
    uint32_t                pa_on_delay;        /*!< the delay time(ms) of turn on pa after enable audio dac. [dac init -> delay -> pa turn on] */
    uint32_t                pa_off_delay;       /*!< the delay time(ms) of disable audio dac after turn off pa. [mute -> pa turn off -> delay -> dac deinit] */
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
    uint32_t                dac_source_bitmap;  /*!< bitmap of active dac source,bit[x]:0:source_x inactive;1:source_x active*/
    aud_dac_source_t        main_dac_source;    /*!< main input source mapped to element->in */
    uint8_t                 play_energy_threshold; /*!< voice-play enter threshold in range 0~100, energy_level > threshold means enter playing */
    uint8_t                 play_energy_hysteresis; /*!< hysteresis in range 0~100, exit threshold = max(0, play_energy_threshold - hysteresis) */
    onboard_speaker_status_cb_t status_cb;      /*!< status callback registered at init */
    void                    *status_cb_user_data;/*!< user data of status callback */
} onboard_speaker_stream_cfg_t;

#define ONBOARD_SPEAKER_STREAM_TASK_STACK          (1536)
#define ONBOARD_SPEAKER_STREAM_TASK_CORE           (1)
#define ONBOARD_SPEAKER_STREAM_TASK_PRIO           (3)

#define ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT      (1 << AUD_DAC_SOURCE_A2DP)
#define ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT      (1 << AUD_DAC_SOURCE_CALL)
#define ONBOARD_SPEAKER_STREAM_DAC_SOURCE_HINT_BIT      (1 << AUD_DAC_SOURCE_HINT)

#define DEFAULT_ACTIVE_DAC_SOURCE_BITMAP (ONBOARD_SPEAKER_STREAM_DAC_SOURCE_A2DP_BIT |\
                                          ONBOARD_SPEAKER_STREAM_DAC_SOURCE_CALL_BIT |\
                                          ONBOARD_SPEAKER_STREAM_DAC_SOURCE_HINT_BIT)
#define DEFAULT_AUD_DAC_PROCESS_SEM_CNT 3

#define DEFAULT_DAC_SOURCE            AUD_DAC_SOURCE_A2DP
#define DEFAULT_AUD_DAC_SAMPLE_RATE   48000
#define DEFAULT_AUD_DAC_RSP_BUF_SIZE  DEFAULT_AUD_DAC_SAMPLE_RATE*20/1000*4 //48000/20ms/24bits

#define MAX_CH_NUM (2)

#define ONBOARD_SPEAKER_STREAM_CFG_DEFAULT() DEFAULT_ONBOARD_SPEAKER_STREAM_CONFIG()

#define DEFAULT_ONBOARD_SPEAKER_STREAM_CONFIG() {              \
        .chl_num = 1,                                          \
        .sample_rate[0] = 48000,                               \
        .sample_rate[1] = 16000,                               \
        .sample_rate[2] = 16000,                               \
        .dig_gain = -7.0f,                                     \
        .ana_gain = 4,                                         \
        .work_mode = AUD_DAC_WORK_MODE_DIFFEN,                 \
        .bits = 16,                                            \
        .clk_src = AUD_CLK_APLL,                               \
        .multi_in_port_num  = 0,                               \
        .multi_out_port_num = 1,                               \
        .frame_size[0] = 320,                                  \
        .frame_size[1] = 320,                                  \
        .frame_size[2] = 320,                                  \
        .pool_length = 0,                                      \
        .pool_play_thold  = 0,                                 \
        .pool_pause_thold = 0,                                 \
        .pa_ctrl_en = false,                                   \
        .pa_ctrl_gpio = 0,                                     \
        .pa_on_level  = 0,                                     \
        .pa_on_delay  = 0,                                     \
        .pa_off_delay = 0,                                     \
        .task_stack = ONBOARD_SPEAKER_STREAM_TASK_STACK,       \
        .task_core  = ONBOARD_SPEAKER_STREAM_TASK_CORE,        \
        .task_prio  = ONBOARD_SPEAKER_STREAM_TASK_PRIO,        \
        .dac_source_bitmap = DEFAULT_ACTIVE_DAC_SOURCE_BITMAP, \
        .main_dac_source   = DEFAULT_DAC_SOURCE,               \
        .play_energy_threshold = 5,                            \
        .play_energy_hysteresis = 2,                           \
        .status_cb = NULL,                                     \
        .status_cb_user_data = NULL,                           \
    }

/**
 * @brief      Create a handle to an Audio Element to stream data from another Element to play.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t onboard_speaker_stream_init(onboard_speaker_stream_cfg_t *config);

/**
 * @brief      Updata onboard speaker stream include sample rate, bits and channel number on running.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in]      rate  sample rate
 * @param[in]      bits  sample bit
 * @param[in]      ch  channel number
 * @param[in]      dma_src  dma source
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_set_param(audio_element_handle_t onboard_speaker_stream, int rate, int bits, int ch, aud_dac_source_t dma_src);

/**
 * @brief      Update onboard speaker stream digital gain
 *
 * Gain in dB: 20*log10(linear), linear encoded in DAC register (see bk_aud_dac_dig_gain_db_to_reg).
 * Typical range: about (-inf, BK_AUD_DAC_DIG_GAIN_DB_MAX] dB; fractional dB allowed (e.g. -6.5f).
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in]      gain_db  digital gain in dB
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_set_digital_gain(audio_element_handle_t onboard_speaker_stream, float gain_db);

/**
 * @brief      Get onboard speaker stream digital gain in dB (from cached register value).
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in,out]  gain_db  output: gain in dB (see bk_aud_dac_dig_gain_reg_to_db)
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_get_digital_gain(audio_element_handle_t onboard_speaker_stream, float *gain_db);

/**
 * @brief      Control onboard audio dac mute.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in]      value  mute value (1: mute, 0: unmute)
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_dac_mute_en(audio_element_handle_t onboard_speaker_stream, uint8_t value);

#if CONFIG_ADK_ONBOARD_SPEAKER_STREAM_SUPPORT_MULTIPLE_SOURCE
/**
 * @brief      Get the input port information of the onboard speaker stream based on the port ID.
 *
 * @param[in]      onboard_speaker_stream  The element handle of the onboard speaker stream
 * @param[in]      port_id  Valid audio port ID (0: element->in, >=1: element->multi_in)
 * @param[out]     port_info  Pointer to the structure used to store the obtained audio port information pointer
 *
 * @return         Result
 *                 - BK_OK: Success
 *                 - Others: Failure
 */
bk_err_t onboard_speaker_stream_get_input_port_info_by_port_id(audio_element_handle_t onboard_speaker_stream, uint8_t port_id, audio_port_info_t **port_info);

/**
 * @brief      Set input audio port info.
 * @note       Do not call set_input_port_info in audio_port_info_t->notify_cb. 
 *             Because the callback function is called in the context of the audio port, and the mutex lock is not allowed to be used in the callback function.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in]      port_info  audio port info
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_set_input_port_info(audio_element_handle_t onboard_speaker_stream, audio_port_info_t *port_info);
#endif

/**
 * @brief      Update onboard speaker stream analog gain.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in]      gain_db  speaker analog gain in dB
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_set_analog_gain(audio_element_handle_t onboard_speaker_stream, int32_t gain_db);

/**
 * @brief      Get onboard speaker stream analog gain.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in,out]  gain_db  speaker analog gain in dB
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_get_analog_gain(audio_element_handle_t onboard_speaker_stream, int32_t *gain_db);

/**
 * @brief      Get onboard speaker stream runtime status.
 *
 * @param[in]      onboard_speaker_stream  element handle
 * @param[in,out]  status  output runtime status, includes playing state and energy level
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_speaker_stream_get_status(audio_element_handle_t onboard_speaker_stream, onboard_speaker_stream_status_t *status);

#ifdef __cplusplus
}
#endif

#endif
