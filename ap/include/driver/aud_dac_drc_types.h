// Copyright 2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A2DP-path DRC preset. Used by aud_dac_drc_param_cfg_t.preset (mode==0).
 * OFF leaves HW drc_bypass=1 (same effect as bk_aud_dac_drc_disable()).
 */
typedef enum {
    AUD_DAC_DRC_PRESET_OFF = 0,
    AUD_DAC_DRC_PRESET_DEFAULT,
    AUD_DAC_DRC_PRESET_SPEECH,
    AUD_DAC_DRC_PRESET_MUSIC,
    AUD_DAC_DRC_PRESET_LOUDSPEAKER,
} aud_dac_drc_preset_t;

/**
 * A2DP HW DRC parameter block (Preset / L2 / raw 8-seg).
 * Pure DRC params — no "enable" flag. Calling
 * bk_aud_dac_drc_apply_param_cfg() applies immediately; upper layers
 * (a2dp_drc_en / app_drc_en) only gate whether they call it.
 *
 * mode:
 *   0 = preset (OFF/DEFAULT/SPEECH/MUSIC/LOUDSPEAKER)
 *   1 = L2 (low_boost_db_x10 / threshold_dbfs_x10 / compress_strength_x100)
 *   2 = raw 8-seg (k_val / p_reg / st_val); p_reg is hi-8 of 24-bit p_val
 */
typedef struct {
    uint8_t mode;                    /**< 0=preset, 1=L2, 2=raw 8-seg */
    uint8_t preset;                  /**< aud_dac_drc_preset_t when mode==0 */
    uint8_t rsvd[2];
    int16_t low_boost_db_x10;        /**< e.g. 80 => 8.0 dB */
    int16_t threshold_dbfs_x10;      /**< e.g. -200 => -20.0 dBFS */
    uint16_t compress_strength_x100; /**< e.g. 85 => 0.85 */
    uint16_t k_val[8];               /**< Q8 slope, 256 = 0 dB */
    uint8_t  p_reg[7];               /**< hi-8 of 24-bit thresholds */
    uint8_t  rsvd2;
    int32_t  st_val[8];              /**< 24-bit signed segment offsets */
} aud_dac_drc_param_cfg_t;

/** mode=0, preset=OFF (bypass curve). */
#define AUD_DAC_DRC_PARAM_CFG_OFF() {                 \
    .mode = 0,                                      \
    .preset = AUD_DAC_DRC_PRESET_OFF,               \
}

/** mode=0 preset shortcut. */
#define AUD_DAC_DRC_PARAM_CFG_PRESET(_p) {            \
    .mode = 0,                                      \
    .preset = (_p),                                 \
}

/** mode=1 L2 shortcut (x10 / x100 fixed-point). */
#define AUD_DAC_DRC_PARAM_CFG_L2(_boost_x10, _thr_x10, _str_x100) { \
    .mode = 1,                                      \
    .low_boost_db_x10 = (_boost_x10),               \
    .threshold_dbfs_x10 = (_thr_x10),               \
    .compress_strength_x100 = (_str_x100),          \
}

#ifdef __cplusplus
}
#endif
