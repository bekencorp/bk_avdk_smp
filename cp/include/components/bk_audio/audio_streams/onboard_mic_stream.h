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


#ifndef _ONBOARD_MIC_STREAM_H_
#define _ONBOARD_MIC_STREAM_H_

#include <components/bk_audio/audio_pipeline/audio_element.h>
#include <components/audio_param_ctrl.h>
#include <driver/aud_adc_types.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   ADC mode configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    uint8_t                 chl_num;          /*!< mic channel number */
    uint8_t                 bits;             /*!< Bit wide (16 or 24 bits) */
    uint32_t                sample_rate;      /*!< mic sample rate */
    int32_t                 dig_gain;         /*!< audio adc digital gain: value range: 0x00 ~ 0x3f(-45db ~ 18db, 0x2d: 0db), suggest: 0x2d */
    int32_t                 ana_gain;         /*!< audio adc analog gain: value range: , suggest: */
    aud_adc_mode_t          mode;             /*!< mic interface mode: signal_ended/differen */
    aud_clk_t               clk_src;          /*!< audio clock: XTAL(26MHz)/APLL */
} adc_cfg_t;

/**
 * @brief   Onboard MIC Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    adc_cfg_t               adc_cfg;            /*!< ADC mode configuration */
    uint32_t                frame_size;         /*!< the length of one frame (bytes) */
    int                     out_block_size;     /*!< Size of output block */
    int                     out_block_num;      /*!< Number of output block */
    int                     multi_out_port_num; /*!< The number of multiple output audio port */
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
} onboard_mic_stream_cfg_t;



#define ONBOARD_MIC_STREAM_TASK_STACK          (1024)
#define ONBOARD_MIC_STREAM_TASK_CORE           (1)
#define ONBOARD_MIC_STREAM_TASK_PRIO           (3)

#define ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT() DEFAULT_ONBOARD_MIC_ADC_STREAM_CONFIG()

#define DEFAULT_ONBOARD_MIC_ADC_STREAM_CONFIG() {           \
    .adc_cfg = {                                            \
                   .chl_num = 1,                            \
                   .bits = 16,                              \
                   .sample_rate = 8000,                     \
                   .dig_gain = 0x2D,                        \
                   .ana_gain = 0x00,                        \
                   .mode = AUD_ADC_MODE_DIFFEN,             \
                   .clk_src = AUD_CLK_XTAL,                 \
               },                                           \
    .frame_size = 320,                                      \
    .out_block_size = 320,                                  \
    .out_block_num = 2,                                     \
    .multi_out_port_num = 0,                                \
    .task_stack = ONBOARD_MIC_STREAM_TASK_STACK,            \
    .task_core = ONBOARD_MIC_STREAM_TASK_CORE,              \
    .task_prio = ONBOARD_MIC_STREAM_TASK_PRIO,              \
}

/**
 * @brief      Create a handle to an Audio Element to stream data to another Element.
 *
 * @param[in]      config  The configuration
 *
 * @return         The Audio Element handle
 *                 - Not NULL: success
 *                 - NULL: failed
 */
audio_element_handle_t onboard_mic_stream_init(onboard_mic_stream_cfg_t *config);

/**
 * @brief      Updata onboard mic stream digital gain.
 *
 * @param[in]      onboard_mic_stream  element handle
 * @param[in]      gain  mic digital gain, range: 0x00 ~ 0x3f(-45db ~ 18db, 0x2d: 0db)
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_mic_stream_set_digital_gain(audio_element_handle_t onboard_mic_stream, uint8_t gain);

/**
 * @brief      Get onboard mic stream digital gain.
 *
 * @param[in]      onboard_mic_stream  element handle
 * @param[in,out]  gain  mic digital gain
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_mic_stream_get_digital_gain(audio_element_handle_t onboard_mic_stream, uint8_t *gain);

/**
 * @brief      Update onboard mic stream analog gain.
 *
 * @param[in]      onboard_mic_stream  element handle
 * @param[in]      gain  mic analog gain, range: 0x00 ~ 0x3f
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_mic_stream_set_analog_gain(audio_element_handle_t onboard_mic_stream, uint8_t gain);

/**
 * @brief      Get onboard mic stream analog gain.
 *
 * @param[in]      onboard_mic_stream  element handle
 * @param[in,out]  gain  mic analog gain
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_mic_stream_get_analog_gain(audio_element_handle_t onboard_mic_stream, uint8_t *gain);

#ifdef __cplusplus
}
#endif

#endif
