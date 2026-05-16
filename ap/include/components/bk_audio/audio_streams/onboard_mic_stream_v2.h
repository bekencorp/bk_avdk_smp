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
 * @brief   Onboard MIC Stream configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct
{
    aud_adc_config_t        adc_cfg;            /*!< ADC mode configuration */
    aud_dmic_config_t       dmic_cfg;           /*!< DMIC mode configuration */
    uint32_t                frame_size;         /*!< the length of one frame (bytes) */
    int                     out_block_size;     /*!< Size of output block */
    int                     out_block_num;      /*!< Number of output block */
    int                     multi_out_port_num; /*!< The number of multiple output audio port */
    int                     task_stack;         /*!< Task stack size */
    int                     task_core;          /*!< Task running in core (0 or 1) */
    int                     task_prio;          /*!< Task priority (based on freeRTOS priority) */
    uint32_t                ch_bitmap;          /*!< Active adc channel bitmap,bit[x]:0:ch_x inactive;1:ch_x active */
} onboard_mic_stream_cfg_t;



#define ONBOARD_MIC_STREAM_TASK_STACK          (1024)
#define ONBOARD_MIC_STREAM_TASK_CORE           (1)
#define ONBOARD_MIC_STREAM_TASK_PRIO           (3)

#define ONBOARD_MIC_ADC_STREAM_CFG_DEFAULT() DEFAULT_ONBOARD_MIC_ADC_STREAM_CONFIG()

#define ONBOARD_MIC_ADC_ACTIVE_CH_0_BIT (1 << AUD_ADC_CHL_0)
#define ONBOARD_MIC_ADC_ACTIVE_CH_1_BIT (1 << AUD_ADC_CHL_1)
#define ONBOARD_MIC_ADC_ACTIVE_CH_2_BIT (1 << AUD_ADC_CHL_2)

#define ONBOARD_MIC_ADC_DEFAULT_ACTIVE_CH_BITS (ONBOARD_MIC_ADC_ACTIVE_CH_0_BIT | \
                                                ONBOARD_MIC_ADC_ACTIVE_CH_1_BIT | \
                                                ONBOARD_MIC_ADC_ACTIVE_CH_2_BIT )

#define DEFAULT_ONBOARD_MIC_ADC_STREAM_CONFIG() {                       \
    .adc_cfg = {                                                        \
                    .chl_num     = 3,                                   \
                    .sample_rate = 8000,                                \
                    .adc_samp_edge = AUD_ADC_SAMP_EDGE_RISING,          \
                    .clk_src = AUD_CLK_APLL,                            \
                    .aec_en = 0,                                        \
                    .chl_cfg =                                          \
                    {                                                   \
                        {                                               \
                            .dig_gain = 0x1c000,                        \
                            .ana_gain = 0x07,                           \
                            .adc_mode = AUD_ADC_MODE_DIFFEN,            \
                            .bits = 16,                                 \
                        },                                              \
                        {                                               \
                            .dig_gain = 0x1c000,                        \
                            .ana_gain = 0x07,                           \
                            .adc_mode = AUD_ADC_MODE_DIFFEN,            \
                            .bits = 16,                                 \
                        },                                              \
                        {                                               \
                            .dig_gain = 0x1c000,                        \
                            .ana_gain = 0x07,                           \
                            .adc_mode = AUD_ADC_MODE_DIFFEN,            \
                            .bits = 16,                                 \
                        },                                              \
                    },                                                  \
               },                                                       \
    .dmic_cfg = {                                                       \
                    .dmic_clk_gpio  = GPIO_6,                           \
                    .dmic_data_gpio = GPIO_5,                           \
                    .dmic_mode      = AUD_DMIC_MODE_1,                  \
                    .channel        = AUD_DMIC_CHANNEL_L,               \
                },                                                      \
    .frame_size     = 320,                                              \
    .out_block_size = 320,                                              \
    .out_block_num  = 2,                                                \
    .multi_out_port_num = 1,                                            \
    .task_stack = ONBOARD_MIC_STREAM_TASK_STACK,                        \
    .task_core  = ONBOARD_MIC_STREAM_TASK_CORE,                         \
    .task_prio  = ONBOARD_MIC_STREAM_TASK_PRIO,                         \
    .ch_bitmap  = ONBOARD_MIC_ADC_DEFAULT_ACTIVE_CH_BITS,               \
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
 * @param[in]      ch    mic adc channel, range: AUD_ADC_CHL_0/AUD_ADC_CHL_1/AUD_ADC_CHL_2
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_mic_stream_set_digital_gain(audio_element_handle_t onboard_mic_stream, uint8_t gain, aud_adc_chl_t ch);

/**
 * @brief      Get onboard mic stream digital gain.
 *
 * @param[in]      onboard_mic_stream  element handle
 * @param[in,out]  gain  mic digital gain
 * @param[in]      ch    mic adc channel, range: AUD_ADC_CHL_0/AUD_ADC_CHL_1/AUD_ADC_CHL_2
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_mic_stream_get_digital_gain(audio_element_handle_t onboard_mic_stream, uint8_t *gain, aud_adc_chl_t ch);

/**
 * @brief      Update onboard mic stream analog gain.
 *
 * @param[in]      onboard_mic_stream  element handle
 * @param[in]      gain  mic analog gain, range: 0x00 ~ 0x3f
 * @param[in]      ch    mic adc channel, range: AUD_ADC_CHL_0/AUD_ADC_CHL_1/AUD_ADC_CHL_2
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_mic_stream_set_analog_gain(audio_element_handle_t onboard_mic_stream, uint8_t gain, aud_adc_chl_t ch);

/**
 * @brief      Get onboard mic stream analog gain.
 *
 * @param[in]      onboard_mic_stream  element handle
 * @param[in,out]  gain  mic analog gain
 * @param[in]      ch    mic adc channel, range: AUD_ADC_CHL_0/AUD_ADC_CHL_1/AUD_ADC_CHL_2
 *
 * @return         Result
 *                 - BK_OK: success
 *                 - other: failed
 */
bk_err_t onboard_mic_stream_get_analog_gain(audio_element_handle_t onboard_mic_stream, uint8_t *gain, aud_adc_chl_t ch);

#ifdef __cplusplus
}
#endif

#endif
