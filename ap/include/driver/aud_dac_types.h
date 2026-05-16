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

#include <driver/int_types.h>
#include <common/bk_include.h>
#include <driver/hal/hal_aud_types.h>
#include <driver/aud_common.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief audio dac defines
 * @defgroup bk_api_aud_defs macos
 * @ingroup bk_api_aud
 * @{
 */

#define BK_ERR_AUD_DAC_BASE                (BK_ERR_AUD_BASE - 40)
#define BK_ERR_AUD_DAC_NOT_INIT            (BK_ERR_AUD_DAC_BASE - 1) /**< audio dac not init */


/**
 * @}
 */

/**
 * @brief audio dac enum defines
 * @defgroup bk_api_aud_enum audio dac enums
 * @ingroup bk_api_aud
 * @{
 */

typedef enum {
	AUD_DAC_DISABLE = 0,    /**< 0：disable dac */
	AUD_DAC_ENABLE,         /**< 1：enable dac */
	AUD_DAC_OTHERS,
} aud_dac_enable_t;

typedef enum {
	AUD_DAC_CHL_L = 0,    /**< enable dac left channel */
	AUD_DAC_CHL_R,        /**< enable dac right channel */
	AUD_DAC_CHL_LR,       /**< enable dac left and right channel */
	AUD_DAC_CHL_MAX,
} aud_dac_chl_t;

typedef enum {
	AUD_DAC_WORK_MODE_DIFFEN = 0,
	AUD_DAC_WORK_MODE_SIGNAL_END,
	AUD_DAC_WORK_MODE_MAX,
} aud_dac_work_mode_t;

typedef enum
{
	AUD_DAC_HPF_BYPASS_DISABLE = 0,    /**< AUD DAC hpf bypass disable */
	AUD_DAC_HPF_BYPASS_ENABLE,         /**< AUD DAC hpf bypass enable */
	AUD_DAC_HPF_BYPASS_OTHERS,
} aud_dac_hpf_bypass_t;

typedef enum {
	AUD_DAC_CLK_INVERT_RISING = 0,    /**< AUD DAC output clock edge rising */
	AUD_DAC_CLK_INVERT_FALLING,       /**< AUD DAC output clock edge falling */
	AUD_DAC_CLK_INVERT_OTHERS,
} aud_dac_clk_invert_t;

typedef enum {
	AUD_DACR_INT_DISABLE = 0,    /**< AUD DAC right channel interrupt disable */
	AUD_DACR_INT_ENABLE,         /**< AUD DAC right channel interrupt enable */
	AUD_DACL_INT_DISABLE,        /**< AUD DAC left channel interrupt disable */
	AUD_DACL_INT_ENABLE,         /**< AUD DAC left channel interrupt enable */
	AUD_DAC_INT_OTHERS,
} aud_dac_int_enable_t;

typedef enum {
	AUD_DAC_FILT_DISABLE = 0,    /**< AUD DAC filter disable */
	AUD_DAC_FILT_ENABLE,         /**< AUD DAC filter enable */
	AUD_DAC_FILT_OTHERS,
} aud_dac_filt_enable_t;

typedef enum {
	AUD_DAC_FRACMOD_MANUAL_DISABLE = 0,    /**< disable dac fractional frequency division of manual set */
	AUD_DAC_FRACMOD_MANUAL_ENABLE,         /**< enable dac fractional frequency division of manual set */
	ADU_DAC_FRACMOD_MANUAL_OTHERS,
} aud_dac_fracmod_manual_t;

typedef enum {
	AUD_DACR_NEAR_FULL_MASK = 1,           /**< AUD DAC right channel fifo near full */
	AUD_DACL_NEAR_FULL_MASK = 1 << 1,      /**< AUD DAC left channel fifo near full */
	AUD_DACR_NEAR_EMPTY_MASK = 1 << 4,     /**< AUD DAC right channel fifo near empty */
	AUD_DACL_NEAR_EMPTY_MASK = 1 << 5,     /**< AUD DAC left channel fifo near empty */
	AUD_DACR_FIFO_FULL_MASK = 1 << 8,      /**< AUD DAC right channel fifo full */
	AUD_DACL_FIFO_FULL_MASK = 1 << 9,      /**< AUD DAC left channel fifo full */
	AUD_DACR_FIFO_EMPTY_MASK = 1 << 12,    /**< AUD DAC right channel fifo empty */
	AUD_DACL_FIFO_EMPTY_MASK = 1 << 13,    /**< AUD DAC left channel fifo empty */
} aud_dac_status_mask_t;

/**
 * @}
 */

/**
 * @brief AUD struct defines
 * @defgroup bk_api_aud_structs structs in AUD
 * @ingroup bk_api_aud
 * @{
 */

#if CONFIG_AUD_DRIVER_V1
typedef struct {
	/* audio_config */
	aud_dac_chl_t dac_chl;                  /**< AUD dac channel */
	uint32_t samp_rate;                     /**< AUD dac sample rate */
	aud_dac_work_mode_t work_mode;          /**< AUD dac work mode */
	uint16_t dac_gain;                      /**< AUD dac gain set */
	aud_dac_clk_invert_t dac_clk_invert;    /**< AUD dac output clock edge select */
	aud_clk_t clk_src;
} aud_dac_config_t;

#define DEFAULT_AUD_DAC_CONFIG() {                           \
        .dac_chl   = AUD_DAC_CHL_L,                          \
        .samp_rate = 8000,                                   \
        .work_mode = AUD_DAC_WORK_MODE_DIFFEN,               \
        .dac_gain  = 0x2D,                                   \
        .dac_clk_invert = AUD_DAC_CLK_INVERT_RISING,         \
        .clk_src        = AUD_CLK_XTAL,                      \
    }
#elif CONFIG_AUD_DRIVER_V2

/** Max digital gain in dB (maps to register full-scale linear, ~0x3FFFFFFF) */
#define BK_AUD_DAC_DIG_GAIN_DB_MAX        (12.0f)
/** dB returned when register linear gain is 0 (mute / -inf dB) */
#define BK_AUD_DAC_DIG_GAIN_DB_SILENCE    (-100.0f)

#define DAC_DIG_GAIN_FRAC_MASK   (0x0FFFFFFFu)
#define DAC_DIG_GAIN_FRAC_SCALE  (268435456u) /* 1<<28 */

#define DAC_ANA_GAIN_REG_MAX       (0x7u)
#define DAC_ANA_GAIN_STEP_DB       (1)
#define BK_AUD_DAC_ANA_GAIN_DB_MAX ((int32_t)DAC_ANA_GAIN_REG_MAX * DAC_ANA_GAIN_STEP_DB)


typedef enum {
	AUD_DAC_SOURCE_A2DP = 0,
	AUD_DAC_SOURCE_CALL,
	AUD_DAC_SOURCE_HINT,
	AUD_DAC_SOURCE_MAX,
} aud_dac_source_t;

typedef enum {
	AUD_DAC_SPK0_A2DP_FIFO_ALMOST_EMPTY_MASK = 1,
	AUD_DAC_SPK0_CALL_FIFO_ALMOST_EMPTY_MASK = 1 << 1,
	AUD_DAC_SPK0_HINT_FIFO_ALMOST_EMPTY_MASK = 1 << 2,
	AUD_DAC_SPK1_A2DP_FIFO_ALMOST_EMPTY_MASK = 1 << 3,
	AUD_DAC_SPK1_CALL_FIFO_ALMOST_EMPTY_MASK = 1 << 4,
	AUD_DAC_SPK1_HINT_FIFO_ALMOST_EMPTY_MASK = 1 << 5,
	AUD_DAC_SPK0_A2DP_FIFO_ALMOST_FULL_MASK  = 1 << 6,
	AUD_DAC_SPK0_CALL_FIFO_ALMOST_FULL_MASK  = 1 << 7,
	AUD_DAC_SPK0_HINT_FIFO_ALMOST_FULL_MASK  = 1 << 8,
	AUD_DAC_SPK1_A2DP_FIFO_ALMOST_FULL_MASK  = 1 << 9,
	AUD_DAC_SPK1_CALL_FIFO_ALMOST_FULL_MASK  = 1 << 10,
	AUD_DAC_SPK1_HINT_FIFO_ALMOST_FULL_MASK  = 1 << 11,
} aud_dac_fifo_status_mask_t;

typedef struct {
	/* audio_config */
	aud_dac_chl_t dac_chl;                  /**< AUD dac channel */
	uint32_t sample_rate;                   /**< AUD dac sample rate */
	uint8_t bits;                           /**< AUD dac bits width */
	aud_dac_work_mode_t work_mode;          /**< AUD dac work mode */
	/**
	 * Digital gain register : bit[30] sign, bit[29:28] integer (0~3),
	 * bit[27:0] fraction; linear G = (sign?-1:1)*(int + frac/2^28),
	 * dB = 20*log10(|G|), valid gain range about (-inf, +12] dB.
	 */
	float dig_gain;                         /**< AUD dac digital gain in dB, range: (-inf, 12.0] */
	int32_t ana_gain;                       /**< AUD dac analog gain in dB, range: [0, 7], 1dB/step */
	aud_dac_clk_invert_t dac_clk_invert;    /**< AUD dac output clock edge select */
	aud_clk_t clk_src;
} aud_dac_config_t;

#define DEFAULT_AUD_DAC_CONFIG() {                      \
    .dac_chl     = AUD_DAC_CHL_LR,                      \
    .sample_rate = 16000,                               \
    .bits      = 16,                                    \
    .work_mode = AUD_DAC_WORK_MODE_DIFFEN,              \
    .dig_gain  = -7.0f,                                 \
    .ana_gain  = 4,                                     \
    .dac_clk_invert = AUD_DAC_CLK_INVERT_RISING,        \
    .clk_src        = AUD_CLK_APLL,                     \
}
#endif

typedef struct {
	aud_dac_hpf_bypass_t dac_hpf2_bypass_enable;    /**< AUD DAC hpf2 disable */
	aud_dac_hpf_bypass_t dac_hpf1_bypass_enable;    /**< AUD DAC hpf1 disable */

	uint16_t dac_hpf2_coef_B0;                      /**< AUD DAC HPF2 coefficient B0 */
	uint16_t dac_hpf2_coef_B1;                      /**< AUD DAC HPF2 coefficient B1 */
	uint16_t dac_hpf2_coef_B2;                      /**< AUD DAC HPF2 coefficient B2 */

	/* dac_config2 */
	uint16_t dac_hpf2_coef_A1;                      /**< AUD DAC HPF2 coefficient A1 */
	uint16_t dac_hpf2_coef_A2;                      /**< AUD DAC HPF2 coefficient A2 */

} aud_dac_hpf_config_t;

typedef struct {
	int32_t flt0_A1;
	int32_t flt0_A2;
	int32_t flt0_B0;
	int32_t flt0_B1;
	int32_t flt0_B2;

	int32_t flt1_A1;
	int32_t flt1_A2;
	int32_t flt1_B0;
	int32_t flt1_B1;
	int32_t flt1_B2;

	int32_t flt2_A1;
	int32_t flt2_A2;
	int32_t flt2_B0;
	int32_t flt2_B1;
	int32_t flt2_B2;

	int32_t flt3_A1;
	int32_t flt3_A2;
	int32_t flt3_B0;
	int32_t flt3_B1;
	int32_t flt3_B2;
}aud_dac_eq_config_t;

/**
 * @}
 */


#ifdef __cplusplus
}
#endif
