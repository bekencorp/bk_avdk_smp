// Copyright 2020-2021 Beken
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
//
#pragma once
#include <common/bk_include.h>
#include <driver/aud_common.h>

#include <driver/aud_adc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* @brief Overview about this API header
 *
 */

/**
 * @brief AUD API
 * @defgroup bk_api_aud AUD API group
 * @{
 */


/**
 * @brief     Init the adc module of audio
 *
 * This API init the adc module:
 *  - Init audio driver
 *  - Set audio adc sample rate, work mode...
 *  - Configure mic
 *
 * @param adc_config audio adc config
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: config is NULL
 *    - BK_ERR_AUD_DRV_NOT_INIT: audio driver is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_init(aud_adc_config_t *adc_config);

/**
 * @brief     Deinit adc module
 *
 * This API deinit the adc module of audio:
 *   - Disable adc and mic
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_deinit(void);

/**
 * @brief     Set the sample rate in adc work mode
 *
 * @param samp_rate adc sample rate of adc work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */

bk_err_t bk_aud_adc_set_samp_rate(uint32_t sample_rate);

/**
 * @brief     Set the adc gain
 *
 * @param value the gain value(0x0 ~ 0x3f)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
#if CONFIG_AUD_DRIVER_V1
bk_err_t bk_aud_adc_set_gain(uint32_t value);
#elif CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_adc_set_ana_gain(aud_adc_chl_t chl, uint32_t value);
bk_err_t bk_aud_adc_set_ana_gain_db(aud_adc_chl_t chl, int32_t db);
bk_err_t bk_aud_adc_get_ana_gain_db(aud_adc_chl_t chl, int32_t *db);

bk_err_t bk_aud_adc_set_dig_gain(aud_adc_chl_t chl, uint32_t value);
bk_err_t bk_aud_adc_set_dig_gain_db(aud_adc_chl_t chl, float db);
bk_err_t bk_aud_adc_get_dig_gain_db(aud_adc_chl_t chl, float *db);
#endif

/**
 * @brief     Set the adc channel
 *
 * @param chl the mic channel
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
#if CONFIG_AUD_DRIVER_V1
bk_err_t bk_aud_adc_set_chl(aud_adc_chl_t chl);
#endif

/**
 * @brief     Set the mic external interface mode
 *
 * @param mic_id the mic id
 * @param intf_mode single end or difference
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
#if CONFIG_AUD_DRIVER_V1
bk_err_t bk_aud_adc_set_mic_mode(aud_mic_id_t mic_id, aud_adc_mode_t mode);
#elif CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_adc_set_mic_mode(aud_adc_chl_t chl, aud_adc_mode_t mode);
#endif
/**
 * @brief     Get the adc fifo address
 *
 * @param adc_fifo_addr adc fifo address
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
#if CONFIG_AUD_DRIVER_V1
bk_err_t bk_aud_adc_get_fifo_addr(uint32_t *adc_fifo_addr);
#elif CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_adc_get_fifo_addr(aud_adc_mic_data_bus_t data_bus, uint32_t *fifo_addr);
#endif
/**
 * @brief     Get the audio adc fifo status information
 *
 * This API get the audio adc status:
 *   - Get fifo status
 *
 * @param adc_status adc fifo status and agc status
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
#if CONFIG_AUD_DRIVER_V1
bk_err_t bk_aud_adc_get_status(uint32_t *adc_status);
#elif CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_adc_get_fifo_status(uint32_t *status);
#endif
/**
 * @brief     Enable adc interrupt
 *
 * This API enable adc interrupt:
 *   - Enable audio adc interrupt
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_enable_int(void);

/**
 * @brief     Disable adc interrupt
 *
 * This API disable adc interrupt:
 *   - Disable adc interrupt if work mode is adc work mode
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_disable_int(void);

/**
 * @brief     Start adc
 *
 * This API start adc:
 *   - Enable adc if work mode is adc work mode
 *   - Enable dtmf if work mode is dtmf work mode
 *
 * Usage example:
 *
 *     //init audio adc module
 *     aud_adc_config_t adc_config = DEFAULT_AUD_ADC_CONFIG();
 *     bk_aud_adc_init(&adc_config);
 *     CLI_LOGI("init adc successful\n");
 *
 *     //start adc and dac
 *     bk_aud_start_adc();
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
#if CONFIG_AUD_DRIVER_V1
bk_err_t bk_aud_adc_start(void);
#elif CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_adc_start(aud_adc_chl_t chl);
#endif
/**
 * @brief     Stop audio adc
 *
 * This API stop adc:
 *   - Disable audio adc
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
#if CONFIG_AUD_DRIVER_V1
bk_err_t bk_aud_adc_stop(void);
#elif CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_adc_stop(aud_adc_chl_t chl);
#endif
/**
 * @brief     Get adc data
 *
 * This API get adc fifo data
 *
 * @param adc_data save audio adc fifo data
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
#if CONFIG_AUD_DRIVER_V1
bk_err_t bk_aud_adc_get_fifo_data(uint32_t *adc_data);
#elif CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_adc_get_fifo_data(aud_adc_mic_data_bus_t data_bus, uint32_t *adc_data);
#endif
/**
 * @brief     Set audio adc data write threshold
 *
 * This API set audio adc data write threshold:
 *          - audio adc interrupt will trigger when the amount of data in FIFO reaches the threshold
 *
 * @param value the value of write threshold
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
#if CONFIG_AUD_DRIVER_V1
bk_err_t bk_aud_adc_set_adcl_wr_threshold(uint32_t value);

#elif CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_adc_set_write_threshold(aud_adc_mic_data_bus_t data_bus, uint32_t value);

bk_err_t bk_aud_adc_set_read_threshold(aud_adc_mic_data_bus_t data_bus, uint32_t value);

#endif

/**
 * @brief     Register audio adc isr
 *
 * @param isr audio isr callback
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_register_isr(aud_isr_t isr);

/**
 * @brief     Config audio adc HPF(High Pass Filter)
 *
 * @param config the audio adc HPF parameters
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_hpf_config(aud_adc_hpf_config_t *config);

/**
 * @brief     Config audio adc AGC(Automatic Gain Control)
 *
 * @param config the audio adc AGC parameters
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_agc_config(aud_adc_agc_config_t *config);

/**
 * @brief     Enable audio adc loop test
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_start_loop_test(void);

/**
 * @brief     Disable audio adc loop test
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_ADC_NOT_INIT: audio adc is not init
 *    - others: other errors.
 */
bk_err_t bk_aud_adc_stop_loop_test(void);

#if CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_adc_set_bits_width(aud_adc_chl_t chl, uint8_t bits_width);

bk_err_t bk_aud_dmic_init(aud_dmic_config_t *dmic_config);

/**
 * @brief     Enable ADC channel(s) by channel bitmap
 * @param ch_bitmap channel bitmap (e.g. 1<<AUD_ADC_CHL_0 for adc channels)
 */
void bk_aud_adc_enable_used_channel(uint32_t ch_bitmap);

#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
