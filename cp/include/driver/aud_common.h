// Copyright 2023-2024 Beken
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

#ifdef __cplusplus
extern "C" {
#endif

#define BK_ERR_AUD_DRV_BASE             BK_ERR_AUD_BASE
#define BK_ERR_AUD_DRV_NOT_INIT         (BK_ERR_AUD_BASE - 1) /**< AUD driver not init */


typedef struct {
	bool adc_is_init;
	bool dmic_is_init;
	bool dtmf_is_init;
	bool dac_is_init;
} aud_module_init_sta_t;

typedef enum {
	AUD_MODULE_ADC = 0,     /**< ADC */
	AUD_MODULE_DMIC = 1,    /**< DMIC */
	AUD_MODULE_DTMF = 2,    /**< DTMF */
	AUD_MODULE_DAC = 3,     /**< DAC */
	AUD_MODULE_MAX          /**< AUD module id max */
} aud_module_id_t;

typedef enum {
#if CONFIG_AUDIO_ADC
	AUD_ISR_ADCL = 0,    /**< adcl_int_en */
#endif
#if CONFIG_AUDIO_DMIC
	AUD_ISR_DMIC = 2,    /**< dmic_int_en */
#endif
#if CONFIG_AUDIO_DTMF
	AUD_ISR_DTMF = 3,    /**< dtmf_int_en */
#endif
#if CONFIG_AUDIO_DAC
	AUD_ISR_DACL = 4,    /**< dacl_int_en */
	AUD_ISR_DACR = 5,    /**< dacr_int_en */
#endif
	AUD_ISR_MAX
} aud_isr_id_t;

typedef void (*aud_isr_t)(void);

typedef struct
{
#if CONFIG_AUDIO_ADC
	aud_isr_t aud_adcl_fifo_handler;
#endif
#if CONFIG_AUDIO_DMIC
	aud_isr_t aud_dmic_fifo_handler;
#endif
#if CONFIG_AUDIO_DTMF
	aud_isr_t aud_dtmf_fifo_handler;
#endif
#if CONFIG_AUDIO_DAC
	aud_isr_t aud_dacl_fifo_handler;
	aud_isr_t aud_dacr_fifo_handler;
#endif
} aud_isr_handle_t;


typedef enum {
	AUD_CLK_XTAL = 0,    /**< XTAL: 26MHz */
	AUD_CLK_APLL,        /**< APLL: 24.576MHz*/
	AUD_CLK_MAX
} aud_clk_t;

#if CONFIG_AUD_DRIVER_V2
typedef enum
{
    AUD_APLL_FREQ_98P3040_MHZ = 0,
    AUD_APLL_FREQ_90P3168_MHZ,
} aud_apll_freq_t;
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
 * @brief     Init the AUD driver
 *
 * This API init the resoure common:
 *   - Init AUD driver control memory
 *   - Configure clock and power
 *   - Configure mic enable
 *   - Register AUD isr interrupt
 *
 * This API should be called before any other AUD APIs.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_driver_init(void);

/**
 * @brief     Deinit the AUD driver
 *
 * This API free all resource related to AUD, power down AUD and mic.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_driver_deinit(void);

/**
 * @brief     Set the audio module initialization status
 *
 * @param id audio module id
 * @param val audio module initialization status
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_set_module_init_sta(aud_module_id_t id, bool val);

/**
 * @brief     Get the audio module initialization status
 *
 * @param id audio module id
 *
 * @return
 *    - true: audio module already has been initialized
 *    - false: audio module has not been initialized.
 */
bool bk_aud_get_module_init_sta(aud_module_id_t id);

/**
 * @brief     Config audio clock
 *
 * @param clk audio clock source
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_clk_config(aud_clk_t clk);

/**
 * @brief     Deconfig audio clock
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_clk_deconfig(void);


/**
 * @brief     Register audio isr
 *
 * This API register audio isr.
 *
 * @param isr_id
 * @param isr audio isr callback
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_register_aud_isr(aud_isr_id_t isr_id, aud_isr_t isr);


/**
 * @brief     config mic0 analog gain
 *
 * This API config mic0 analog gain.
 *
 * @param ana_mic0_gain
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */

bk_err_t bk_aud_set_ana_mic0_gain(uint8_t ana_mic0_gain);

/**
 * @brief     config mic1 analog gain
 *
 * This API config mic1 analog gain.
 *
 * @param ana_mic1_gain
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */

bk_err_t bk_aud_set_ana_mic1_gain(uint8_t ana_mic1_gain);

/**
 * @brief     config audio dac analog gain
 *
 * This API config audio dac analog gain.
 *
 * @param ana_dac_gain
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_set_ana_dac_gain(uint8_t ana_dac_gain);

#if CONFIG_AUD_DRIVER_V2
bk_err_t bk_aud_apll_config(aud_apll_freq_t freq);

bk_err_t bk_aud_apll_spi_trigger(void);

void bk_aud_hardware_reset(void);

void bk_aud_hardware_reset_release(void);

#if CONFIG_AUD_PM_FAST_HOT
/**
 * @brief Backup AUD digital regs + related ANA/clock context before AUDP / CPU-down.
 *
 * Only available when CONFIG_AUD_PM_FAST_HOT=y.
 * Saves audio_reg block, ANA20/21/25/27-30, APLL/aud clock selects, and HW EQ
 * coefficient memory. Does not touch DMA (platform owns bk_dma_pm_*).
 * No-op with BK_OK if AUD driver is not initialized.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors.
 */
bk_err_t bk_aud_pm_backup(void);

/**
 * @brief Restore AUD context after AP fast resume.
 *
 * Only available when CONFIG_AUD_PM_FAST_HOT=y.
 * Votes AUDP_AUDIO power + AUDIO clock on, then restores ANA / digital regs /
 * EQ coef and clears DAC/ADC/buf run enables so service open can re-arm.
 * Ends by calling bk_aud_pm_restore_notify() so audio components can reset
 * software state (e.g. spk open_cnt), and sets the power-restore pending
 * flag for the application. Does not touch DMA / GPIO.
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_AUD_DRV_NOT_INIT: driver not init
 *    - BK_FAIL: no valid backup
 */
bk_err_t bk_aud_pm_restore(void);

/**
 * @brief Component hook after AUD PM restore (weak default is empty).
 *
 * Overridden by audio streams (e.g. onboard speaker) to clear open_cnt /
 * stop stale DMA software state. Invoked only from bk_aud_pm_restore().
 */
void bk_aud_pm_restore_notify(void);

/**
 * @brief Whether bk_aud_pm_restore() has run and app has not acked yet.
 *
 * Application may use this before starting services (e.g. pipeline reset_port).
 * Cleared by bk_aud_pm_clear_power_restore_flag() after all services are up.
 */
bool bk_aud_pm_power_restore_pending(void);

/**
 * @brief Ack fast-boot AUD restore; call after application finished starting services.
 */
void bk_aud_pm_clear_power_restore_flag(void);

/**
 * @brief Whether a valid AUD PM backup exists.
 */
bool bk_aud_pm_backup_valid(void);
#endif /* CONFIG_AUD_PM_FAST_HOT */
#endif
/**
 * @}
 */

#ifdef __cplusplus
}
#endif
