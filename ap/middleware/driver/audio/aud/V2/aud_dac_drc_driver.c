// Copyright 2026 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// BK7259 DAC hardware DRC: preset / user-param conversion + register driver.
// HW path: spk_a2dp → Resample → EQ → DRC → Mix (CALL/HINT bypass DRC).

#include <common/bk_include.h>
#include <sdkconfig.h>
#include <components/log.h>
#include <driver/aud_dac_drc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "aud_hal.h"

#define TAG "aud_drc"

#define DRC_FULL_SCALE 0x7FFFFFu
#define DRC_K_UNITY    256u
#define AUD_DAC_DRC_DW 24

/**
 * Internal HW register table (k / p / st). Customers use
 * aud_dac_drc_param_cfg_t (mode=2 uses p_reg = p_val>>16).
 */
typedef struct {
    uint16_t k_val[8];
    uint32_t p_val[7];
    int32_t  st_val[8];
    int      drc_bypass;
} aud_dac_drc_cfg_t;

/** Internal L2 float form (mode=1 converts fixed-point → this). */
typedef struct {
    float low_boost_db;
    float threshold_dbfs;
    float compress_strength;
} aud_dac_drc_user_cfg_t;

/* -------------------------------------------------------------------------- */
/* Helpers                                                                    */
/* -------------------------------------------------------------------------- */

static int32_t aud_dac_drc_sext24(int32_t v)
{
    v &= 0x00FFFFFF;
    if (v & 0x00800000) {
        v |= (int32_t)0xFF000000;
    }
    return v;
}

#if CONFIG_AUD_DAC_DRC

static int32_t drc_sat(int64_t v, int out_w)
{
    int64_t max_v = (((int64_t)1 << (out_w - 1)) - 1);
    int64_t min_v = -max_v;

    if (v > max_v) {
        return (int32_t)max_v;
    }
    if (v < min_v) {
        return (int32_t)min_v;
    }
    return (int32_t)v;
}

static int32_t drc_div_sat(int64_t sat_in, int dw_out, int dw_div)
{
    int bit_msb       = (int)((sat_in >> (dw_div - 1)) & 1);
    int lower_nonzero = ((sat_in & (((int64_t)1 << (dw_div - 1)) - 1)) != 0);
    int round_r       = (sat_in < 0) ? (bit_msb && lower_nonzero) : bit_msb;

    return drc_sat((sat_in >> dw_div) + round_r, dw_out);
}

static int32_t drc_mul_k(int32_t din, uint16_t k)
{
    return drc_div_sat((int64_t)din * (int64_t)k, AUD_DAC_DRC_DW, 8);
}

static void cfg_clear(aud_dac_drc_cfg_t *cfg)
{
    memset(cfg, 0, sizeof(*cfg));
}

static void load_default_table(aud_dac_drc_cfg_t *cfg)
{
    static const uint16_t k[] = {
        0x1F5, 0x18E, 0x13C, 0xFB, 0xC7, 0x9E, 0x7E, 0x64
    };
    static const uint32_t st_raw[] = {
        0x0, 0x66F2D, 0x10A7F8, 0x1CD5DC, 0x29BC08, 0x368ACB, 0x42C016, 0x4E1059
    };
    static const uint8_t p_reg[] = { 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70 };
    int i;

    cfg_clear(cfg);
    for (i = 0; i < 8; i++) {
        cfg->k_val[i] = k[i];
    }
    for (i = 0; i < 8; i++) {
        cfg->st_val[i] = aud_dac_drc_sext24((int32_t)st_raw[i]);
    }
    for (i = 0; i < 7; i++) {
        cfg->p_val[i] = (uint32_t)p_reg[i] << 16;
    }
    cfg->drc_bypass = 0;
}

static float clampf(float v, float lo, float hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static uint32_t target_out_at_p(uint32_t p24, float low_boost_db, float compress_strength)
{
    float frac;

    if (p24 == 0) {
        return 0;
    }

    frac = (float)p24 / (float)DRC_FULL_SCALE;
    {
        float exp = 1.0f + compress_strength * 2.0f;
        float gain_db = low_boost_db * powf(1.0f - frac, exp);
        float gain    = powf(10.0f, gain_db / 20.0f);
        double out    = (double)p24 * (double)gain + 0.5;

        if (out > (double)DRC_FULL_SCALE) {
            out = (double)DRC_FULL_SCALE;
        }
        return (uint32_t)out;
    }
}

static uint32_t drc_dbfs_to_linear(float dbfs)
{
    double lin = pow(10.0, (double)dbfs / 20.0) * (double)DRC_FULL_SCALE + 0.5;

    if (lin < 0.0) {
        lin = 0.0;
    }
    if (lin > (double)DRC_FULL_SCALE) {
        lin = (double)DRC_FULL_SCALE;
    }
    return (uint32_t)lin;
}

static uint16_t drc_db_to_k(float gain_db)
{
    double k = pow(10.0, (double)gain_db / 20.0) * 256.0 + 0.5;

    if (k < 1.0) {
        k = 1.0;
    }
    if (k > 65535.0) {
        k = 65535.0;
    }
    return (uint16_t)k;
}

static const char *drc_preset_name(aud_dac_drc_preset_t preset)
{
    switch (preset) {
    case AUD_DAC_DRC_PRESET_OFF:
        return "off";
    case AUD_DAC_DRC_PRESET_DEFAULT:
        return "default";
    case AUD_DAC_DRC_PRESET_SPEECH:
        return "speech";
    case AUD_DAC_DRC_PRESET_MUSIC:
        return "music";
    case AUD_DAC_DRC_PRESET_LOUDSPEAKER:
        return "loudspeaker";
    default:
        return "unknown";
    }
}

static int drc_validate_cfg(const aud_dac_drc_cfg_t *cfg, char *err, size_t err_len);

static int drc_user_cfg_to_hw(const aud_dac_drc_user_cfg_t *user, aud_dac_drc_cfg_t *cfg)
{
    float low_boost, threshold_dbfs, strength;
    uint32_t p0_lin, p_reg0;
    uint32_t p24[7];
    uint32_t y[7];
    uint16_t k[8];
    int32_t  st[8];
    int i;

    if (!user || !cfg) {
        return -1;
    }

    low_boost      = clampf(user->low_boost_db,      0.0f,  12.0f);
    threshold_dbfs = clampf(user->threshold_dbfs,   -40.0f,  0.0f);
    strength       = clampf(user->compress_strength, 0.0f,   1.0f);

    cfg_clear(cfg);

    p0_lin  = drc_dbfs_to_linear(threshold_dbfs);
    p_reg0  = p0_lin >> 16;
    if (p_reg0 < 0x01u) {
        p_reg0 = 0x01u;
    }

    for (i = 0; i < 7; i++) {
        uint32_t preg = p_reg0 + (uint32_t)i * 0x10u;

        p24[i] = preg << 16;
        if (p24[i] > DRC_FULL_SCALE) {
            p24[i] = DRC_FULL_SCALE;
        }
        cfg->p_val[i] = p24[i];
        y[i] = target_out_at_p(p24[i], low_boost, strength);
    }

    if (p24[0] > 0) {
        k[0] = (uint16_t)(((uint64_t)y[0] * 256u + p24[0] / 2u) / p24[0]);
        if (k[0] < 64) {
            k[0] = 64;
        }
    } else {
        k[0] = drc_db_to_k(low_boost);
    }
    st[0] = (int32_t)y[0] - drc_mul_k((int32_t)p24[0], k[0]);

    for (i = 1; i < 7; i++) {
        uint32_t dp = p24[i] - p24[i - 1];
        uint32_t dy = (y[i] > y[i - 1]) ? (y[i] - y[i - 1]) : 0u;

        if (dp > 0) {
            k[i] = (uint16_t)(((uint64_t)dy * 256u + dp / 2u) / dp);
        } else {
            k[i] = DRC_K_UNITY;
        }
        if (k[i] < 64) {
            k[i] = 64;
        }

        st[i] = (int32_t)y[i] - drc_mul_k((int32_t)p24[i], k[i]);
    }

    {
        uint32_t dp = DRC_FULL_SCALE - p24[6];
        uint32_t dy = DRC_FULL_SCALE - y[6];

        if (dp > 0) {
            k[7] = (uint16_t)(((uint64_t)dy * 256u + dp / 2u) / dp);
        } else {
            k[7] = DRC_K_UNITY;
        }
        if (k[7] < 64) {
            k[7] = 64;
        }

        st[7] = (int32_t)DRC_FULL_SCALE - drc_mul_k((int32_t)DRC_FULL_SCALE, k[7]);
    }

    for (i = 0; i < 8; i++) {
        cfg->k_val[i]  = k[i];
        cfg->st_val[i] = aud_dac_drc_sext24(st[i]);
    }

    cfg->drc_bypass = 0;
    return drc_validate_cfg(cfg, NULL, 0);
}

static int drc_preset_to_cfg(aud_dac_drc_preset_t preset, aud_dac_drc_cfg_t *cfg)
{
    aud_dac_drc_user_cfg_t user;

    if (!cfg) {
        return -1;
    }

    switch (preset) {
    case AUD_DAC_DRC_PRESET_OFF:
        cfg_clear(cfg);
        cfg->drc_bypass = 1;
        return 0;

    case AUD_DAC_DRC_PRESET_DEFAULT:
    case AUD_DAC_DRC_PRESET_LOUDSPEAKER:
        load_default_table(cfg);
        return 0;

    case AUD_DAC_DRC_PRESET_SPEECH:
        user = (aud_dac_drc_user_cfg_t){ 8.0f, -20.0f, 0.85f };
        return drc_user_cfg_to_hw(&user, cfg);

    case AUD_DAC_DRC_PRESET_MUSIC:
        user = (aud_dac_drc_user_cfg_t){ 3.0f, -15.0f, 0.5f };
        return drc_user_cfg_to_hw(&user, cfg);

    default:
        return -1;
    }
}

static int drc_validate_cfg(const aud_dac_drc_cfg_t *cfg, char *err, size_t err_len)
{
    uint32_t prev = 0;
    int i;

    if (!cfg) {
        if (err && err_len) {
            snprintf(err, err_len, "null cfg");
        }
        return -1;
    }

    for (i = 0; i < 7; i++) {
        uint32_t p = cfg->p_val[i] & 0x00FFFFFFu;

        if (p < prev) {
            if (err && err_len) {
                snprintf(err, err_len, "p_val[%d]=0x%06X not monotonic", i, p);
            }
            return -1;
        }
        prev = p;
        if (p > DRC_FULL_SCALE) {
            if (err && err_len) {
                snprintf(err, err_len, "p_val[%d] exceeds full scale", i);
            }
            return -1;
        }
    }

    for (i = 0; i < 8; i++) {
        if (cfg->k_val[i] == 0) {
            if (err && err_len) {
                snprintf(err, err_len, "k_val[%d] is zero", i);
            }
            return -1;
        }
    }

    return 0;
}

#endif /* CONFIG_AUD_DAC_DRC */

/* -------------------------------------------------------------------------- */
/* HW register driver                                                         */
/* -------------------------------------------------------------------------- */

static bk_err_t aud_dac_drc_validate(const aud_dac_drc_cfg_t *cfg)
{
#if CONFIG_AUD_DAC_DRC
    if (drc_validate_cfg(cfg, NULL, 0) != 0) {
        return BK_ERR_PARAM;
    }
    return BK_OK;
#else
    uint32_t prev = 0;
    int i;

    if (!cfg) {
        return BK_ERR_NULL_PARAM;
    }

    for (i = 0; i < 7; i++) {
        uint32_t p = cfg->p_val[i] & 0x00FFFFFFu;
        if (p < prev) {
            BK_LOGE(TAG, "p_val[%d] not monotonic\r\n", i);
            return BK_ERR_PARAM;
        }
        prev = p;
    }

    for (i = 0; i < 8; i++) {
        if (cfg->k_val[i] == 0) {
            BK_LOGE(TAG, "k_val[%d] is zero\r\n", i);
            return BK_ERR_PARAM;
        }
    }

    return BK_OK;
#endif
}

static void aud_dac_drc_hw_write(const aud_dac_drc_cfg_t *cfg)
{
    uint8_t p_reg[7];
    int i;

    for (i = 0; i < 7; i++) {
        p_reg[i] = (uint8_t)((cfg->p_val[i] >> 16) & 0xFFu);
    }

    audio_reg_hal_set_k_val_0_k_val_0(cfg->k_val[0]);
    audio_reg_hal_set_k_val_1_k_val_1(cfg->k_val[1]);
    audio_reg_hal_set_k_val_2_k_val_2(cfg->k_val[2]);
    audio_reg_hal_set_k_val_3_k_val_3(cfg->k_val[3]);
    audio_reg_hal_set_k_val_4_k_val_4(cfg->k_val[4]);
    audio_reg_hal_set_k_val_5_k_val_5(cfg->k_val[5]);
    audio_reg_hal_set_k_val_6_k_val_6(cfg->k_val[6]);
    audio_reg_hal_set_k_val_7_k_val_7(cfg->k_val[7]);

    audio_reg_hal_set_st_val_0_st_val_0((uint32_t)(aud_dac_drc_sext24(cfg->st_val[0]) & 0x00FFFFFF));
    audio_reg_hal_set_st_val_1_st_val_1((uint32_t)(aud_dac_drc_sext24(cfg->st_val[1]) & 0x00FFFFFF));
    audio_reg_hal_set_st_val_2_st_val_2((uint32_t)(aud_dac_drc_sext24(cfg->st_val[2]) & 0x00FFFFFF));
    audio_reg_hal_set_st_val_3_st_val_3((uint32_t)(aud_dac_drc_sext24(cfg->st_val[3]) & 0x00FFFFFF));
    audio_reg_hal_set_st_val_4_st_val_4((uint32_t)(aud_dac_drc_sext24(cfg->st_val[4]) & 0x00FFFFFF));
    audio_reg_hal_set_st_val_5_st_val_5((uint32_t)(aud_dac_drc_sext24(cfg->st_val[5]) & 0x00FFFFFF));
    audio_reg_hal_set_st_val_6_st_val_6((uint32_t)(aud_dac_drc_sext24(cfg->st_val[6]) & 0x00FFFFFF));
    audio_reg_hal_set_st_val_7_st_val_7((uint32_t)(aud_dac_drc_sext24(cfg->st_val[7]) & 0x00FFFFFF));

    audio_reg_hal_set_p_val_0_p_val_0(p_reg[0]);
    audio_reg_hal_set_p_val_1_p_val_1(p_reg[1]);
    audio_reg_hal_set_p_val_2_p_val_2(p_reg[2]);
    audio_reg_hal_set_p_val_3_p_val_3(p_reg[3]);
    audio_reg_hal_set_p_val_4_p_val_4(p_reg[4]);
    audio_reg_hal_set_p_val_5_p_val_5(p_reg[5]);
    audio_reg_hal_set_p_val_6_p_val_6(p_reg[6]);
}

static bk_err_t aud_dac_drc_apply_cfg(const aud_dac_drc_cfg_t *cfg)
{
    bk_err_t ret;

    if (!cfg) {
        return BK_ERR_NULL_PARAM;
    }

    if (cfg->drc_bypass) {
        audio_reg_hal_set_dac_cfg_drc_bypass(1);
        return BK_OK;
    }

    ret = aud_dac_drc_validate(cfg);
    if (ret != BK_OK) {
        return ret;
    }

    aud_dac_drc_hw_write(cfg);
    audio_reg_hal_set_dac_cfg_drc_bypass(0);

    BK_LOGI(TAG, "HW DRC enabled\r\n");
    return BK_OK;
}

#if CONFIG_AUD_DAC_DRC

/* -------------------------------------------------------------------------- */
/* Internal preset / L2 apply (used by apply_param_cfg)                       */
/* -------------------------------------------------------------------------- */

static bk_err_t aud_dac_drc_apply_preset(aud_dac_drc_preset_t preset)
{
    aud_dac_drc_cfg_t cfg;

    if (preset > AUD_DAC_DRC_PRESET_LOUDSPEAKER) {
        return BK_ERR_PARAM;
    }

    if (drc_preset_to_cfg(preset, &cfg) != 0) {
        return BK_ERR_PARAM;
    }

    BK_LOGI(TAG, "apply preset: %s\r\n", drc_preset_name(preset));
    return aud_dac_drc_apply_cfg(&cfg);
}

static bk_err_t aud_dac_drc_apply_user_cfg(const aud_dac_drc_user_cfg_t *user)
{
    aud_dac_drc_cfg_t cfg;

    if (!user) {
        return BK_ERR_NULL_PARAM;
    }

    if (drc_user_cfg_to_hw(user, &cfg) != 0) {
        return BK_ERR_PARAM;
    }

    return aud_dac_drc_apply_cfg(&cfg);
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                 */
/* -------------------------------------------------------------------------- */

bk_err_t bk_aud_dac_drc_apply_param_cfg(const aud_dac_drc_param_cfg_t *cfg)
{
    if (!cfg) {
        return BK_ERR_NULL_PARAM;
    }

    if (cfg->mode == 0) {
        return aud_dac_drc_apply_preset((aud_dac_drc_preset_t)cfg->preset);
    }

    if (cfg->mode == 2) {
        aud_dac_drc_cfg_t hw = {0};
        int i;

        for (i = 0; i < 8; i++) {
            hw.k_val[i] = cfg->k_val[i];
        }
        for (i = 0; i < 7; i++) {
            hw.p_val[i] = ((uint32_t)cfg->p_reg[i]) << 16;
        }
        for (i = 0; i < 8; i++) {
            hw.st_val[i] = cfg->st_val[i];
        }
        hw.drc_bypass = 0;
        BK_LOGI(TAG, "apply param_cfg mode=2 (raw 8-seg)\r\n");
        return aud_dac_drc_apply_cfg(&hw);
    }

    /* mode==1 or other: L2 */
    {
        aud_dac_drc_user_cfg_t user;

        user.low_boost_db      = (float)cfg->low_boost_db_x10 / 10.0f;
        user.threshold_dbfs    = (float)cfg->threshold_dbfs_x10 / 10.0f;
        user.compress_strength = (float)cfg->compress_strength_x100 / 100.0f;
        BK_LOGI(TAG, "apply param_cfg mode=1 (L2)\r\n");
        return aud_dac_drc_apply_user_cfg(&user);
    }
}

bk_err_t bk_aud_dac_drc_disable(void)
{
    aud_dac_drc_cfg_t cfg = {0};

    cfg.drc_bypass = 1;
    return aud_dac_drc_apply_cfg(&cfg);
}

#endif /* CONFIG_AUD_DAC_DRC */
