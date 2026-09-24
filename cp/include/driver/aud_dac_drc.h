// Copyright 2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
#pragma once

#include <common/bk_err.h>
#include <driver/aud_dac_drc_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DAC A2DP-path hardware DRC APIs
 *
 * HW path: spk_a2dp → Resample → EQ → DRC → Mix.
 * Only AUD_DAC_SOURCE_A2DP goes through DRC; CALL / HINT mix after DRC
 * and are NOT compressed by these APIs.
 *
 * There is no separate "enable" API at this layer:
 *   - calling apply_param_cfg() writes registers and clears drc_bypass (DRC on)
 *   - bk_aud_dac_drc_disable() / PRESET_OFF sets bypass (DRC off)
 * Upper-layer flags such as aud_dac_config_t.a2dp_drc_en /
 * app_aud_drc_config_t.app_drc_en only decide whether those layers call
 * into here; they are not checked inside these functions.
 *
 * Customer entry: bk_aud_dac_drc_apply_param_cfg()
 *   - AUD_DAC_DRC_PARAM_CFG_PRESET / L2 / OFF macros, or mode=2 raw table
 * Bypass helper: bk_aud_dac_drc_disable()
 *
 * Prerequisite: audio / DAC driver already initialized
 * (e.g. after bk_aud_dac_init()). Safe to call at runtime to retune.
 */

#if CONFIG_AUD_DAC_DRC

/**
 * @brief Apply DRC parameter block (recommended entry)
 *
 * Always applies when called (no enable gate inside). Use macros in
 * aud_dac_drc_types.h for common cases:
 *   - AUD_DAC_DRC_PARAM_CFG_PRESET(...)
 *   - AUD_DAC_DRC_PARAM_CFG_L2(...)
 *   - AUD_DAC_DRC_PARAM_CFG_OFF()  (bypass; same effect as disable)
 *   - mode=2: fill k_val / p_reg / st_val yourself (expert 8-seg)
 *
 * @param cfg parameter block; must not be NULL
 *            mode 0 → preset, 1 → L2, 2 → raw 8-seg (see types)
 *
 * @return
 *    - BK_OK: succeed
 *    - BK_ERR_NULL_PARAM: cfg is NULL
 *    - BK_ERR_PARAM: invalid preset / L2 / raw table
 *    - others: other errors
 */
bk_err_t bk_aud_dac_drc_apply_param_cfg(const aud_dac_drc_param_cfg_t *cfg);

/**
 * @brief Bypass / turn off A2DP-path HW DRC
 *
 * Sets drc_bypass only; does not clear previously written k/p/st tables.
 * Next successful apply_param_cfg() will enable DRC again.
 *
 * @return
 *    - BK_OK: succeed
 *    - others: other errors
 */
bk_err_t bk_aud_dac_drc_disable(void);

#endif /* CONFIG_AUD_DAC_DRC */

#ifdef __cplusplus
}
#endif
