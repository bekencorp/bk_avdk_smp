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


#include <math.h>

#include <common/bk_include.h>
#include <soc/soc.h>

#include "aud_hal.h"
#include "sys_hal.h"

#include "sys_driver.h"
#include "clock_driver.h"
#include <os/os.h>
#include <os/mem.h>
#include <driver/int.h>
#include <driver/aud_dac_types.h>
#include <driver/aud_dac.h>
#include <timer/timer_driver.h>

//#include <modules/pm.h>

#define TAG "aud_dac_drv"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define AUD_DAC_RETURN_ON_NOT_INIT() do {\
				if (!bk_aud_get_module_init_sta(AUD_MODULE_DAC)) {\
					return BK_ERR_AUD_DAC_NOT_INIT;\
				}\
			} while(0)

#define CONST_DIV_16K		(0x06590000)
#define CONST_DIV_32K		(0x01964000)
#define CONST_DIV_44_1K		(0x049B2368)
#define CONST_DIV_48K		(0x043B5554)


extern void delay(int num);
bk_err_t bk_aud_dac_dacl_disable_int(void);
bk_err_t bk_aud_dac_dacr_disable_int(void);

bk_err_t bk_aud_dac_init(aud_dac_config_t *dac_config)
{
	bk_err_t ret = BK_OK;
	BK_RETURN_ON_NULL(dac_config);

	if (bk_aud_get_module_init_sta(AUD_MODULE_DAC)) {
		LOGW("aud dac is init already %s\n", __func__);
		return BK_OK;
	}
	bk_aud_set_module_init_sta(AUD_MODULE_DAC, true);
	/* audio common driver init */
	if (BK_OK != bk_aud_driver_init()) {
		LOGE("%s, audio driver init fail, line: %d \n", __func__, __LINE__);
		ret = BK_ERR_AUD_DRV_NOT_INIT;
		goto fail;
	}

	/* select audio clock */
	bk_aud_clk_config(dac_config->clk_src);

	/*active dac*/
    sys_drv_aud_dac_ldcoc_en(1);
    sys_drv_aud_dac_rdcoc_en(1);
    sys_drv_aud_audbias_en(1);
    sys_drv_aud_dac_enbs_en(1);
    sys_drv_aud_dac_idacl_en(1);
    sys_drv_aud_dac_idacr_en(1);
    sys_drv_aud_dac_drv_en(1);

    /* Silicon limitation: keep both DAC digital L/R enables asserted, even when only one output channel is used. */
    audio_reg_hal_set_dac_cfg_dac_enable_l(1);
    audio_reg_hal_set_dac_cfg_dac_enable_r(1);

    sys_drv_aud_looprst0v9_en(1);
    bk_timer_delay_us(1000);
    sys_drv_aud_looprst0v9_en(0);

	audio_reg_hal_set_dac_cfg_dac_tx_anc_d2(2);
	if (dac_config->bits == 24) {
        LOGW("%s, Unsupported bits width: %d\n", __func__, dac_config->bits);
        ret = BK_FAIL;
        goto fail;
	} else {
		/* 16bit LR: L/R packed in one 32bit word; stereo_en HW-splits to dacl/dacr */
		if (dac_config->dac_chl == AUD_DAC_CHL_LR) {
			audio_reg_hal_set_dac_cfg_mono_sel(0x0);
			audio_reg_hal_set_dac_cfg_stereo_en(0x7);
			audio_reg_hal_set_interface_matrix_dac_l_chn_sel(0);
			audio_reg_hal_set_interface_matrix_dac_r_chn_sel(1);
		} else {
			audio_reg_hal_set_dac_cfg_mono_sel(0x7);
			audio_reg_hal_set_dac_cfg_stereo_en(0x0);
		}
	}
	audio_reg_hal_set_dac_cfg_drc_bypass(1);

	//enable dacl and dacr
#if 0
	switch (dac_config->dac_chl) 
	{
		case AUD_DAC_CHL_L:
			sys_drv_aud_dacr_en(0);
			sys_drv_aud_dacl_en(1);

//			TODO
//			audio_reg_hal_set_dac_cfg_mono_sel(2);
//			audio_reg_hal_set_dac_cfg_stereo_en(5);
//			audio_reg_hal_set_dac_cfg_dac_16b_sel(3);
//			audio_reg_hal_set_dac_cfg_dac_tx_anc_d2(2);
			break;

		case AUD_DAC_CHL_R:
			sys_drv_aud_dacr_en(1);
			sys_drv_aud_dacl_en(0);
			break;

		case AUD_DAC_CHL_LR:
			sys_drv_aud_dacr_en(1);
			sys_drv_aud_dacl_en(1);
			break;

		default:
			break;
	}
#endif
	//set dac work mode
	if (dac_config->work_mode == AUD_DAC_WORK_MODE_SIGNAL_END) {
		sys_drv_aud_dac_diffen_en(0);
	} else if (dac_config->work_mode == AUD_DAC_WORK_MODE_DIFFEN) {
		sys_drv_aud_dac_diffen_en(1);
	} else {
		LOGE("%s, audio dac work mode fail, line: %d \n", __func__, __LINE__);
		ret = BK_FAIL;
		goto fail;
	}

	bk_aud_dac_set_dig_gain_db(dac_config->dig_gain);
    bk_aud_dac_set_ana_gain_db(dac_config->ana_gain);

	audio_reg_hal_set_dac_cfg_clk_dac_inv(dac_config->dac_clk_invert);

	//TODO
	bk_aud_dac_set_bits_width(AUD_DAC_SOURCE_A2DP, dac_config->bits);
	bk_aud_dac_set_bits_width(AUD_DAC_SOURCE_CALL, dac_config->bits);
	bk_aud_dac_set_bits_width(AUD_DAC_SOURCE_HINT, dac_config->bits);

	/* default: dac hpf bypass */
	audio_reg_hal_set_dac_cfg_dac_hpf_bps(1);
//	aud_hal_set_dac_config0_dac_hpf1_bypass(1);
//	aud_hal_set_dac_config0_dac_hpf2_bypass(1);

#if 0
	if (BK_OK != bk_aud_dac_set_sample_rate(AUD_DAC_SOURCE_A2DP, dac_config->samp_rate)) {
		ret = BK_FAIL;
		goto fail;
	}
#endif
	return ret;

fail:
	bk_aud_set_module_init_sta(AUD_MODULE_DAC, false);
	return ret;
}

bk_err_t bk_aud_dac_deinit(void)
{
	AUD_DAC_RETURN_ON_NOT_INIT();

	bk_aud_dac_stop(AUD_DAC_CHL_LR);

	audio_reg_hal_set_dac_cfg_dac_hpf_bps(0);

	/* disable int */
	bk_aud_dac_dacl_disable_int();
	bk_aud_dac_dacr_disable_int();

	//enable dacl and dacr
	sys_drv_aud_dacr_en(0);
	sys_drv_aud_dacl_en(0);

	/*active dac*/
	sys_drv_aud_dac_bias_en(0);
	sys_drv_aud_dac_drv_en(0);
	sys_drv_aud_dac_dcoc_en(0);
	sys_drv_aud_dac_idacl_en(0);
	sys_drv_aud_dac_idacr_en(0);

	sys_drv_aud_dac_diffen_en(1);

	bk_aud_dac_set_dig_gain_db(BK_AUD_DAC_DIG_GAIN_DB_SILENCE);
	bk_aud_dac_set_ana_gain_db(0);
	audio_reg_hal_set_dac_cfg_clk_dac_inv(0);

	//aud_hal_set_dac_config0_dac_hpf1_bypass(0);
	//aud_hal_set_dac_config0_dac_hpf2_bypass(0);

	//bk_aud_dac_set_sample_rate(8000);
	/* reset */
	//TODO
	bk_err_t ret = bk_aud_set_module_init_sta(AUD_MODULE_DAC, false);
	bk_aud_driver_deinit();
	return ret;
}

bk_err_t bk_aud_dac_set_sample_rate(aud_dac_source_t source, uint32_t sample_rate)
{
	AUD_DAC_RETURN_ON_NOT_INIT();

    uint32_t lpf_bps1  = 0;
    uint32_t lpf_bps2  = 0;
    uint32_t lpf_bps3  = 0;
    uint32_t resample_bypass = 1;
    uint32_t spl_sel  = 0;

    uint32_t srindex  = 0;

    switch (source)
    {
        case AUD_DAC_SOURCE_A2DP:
            switch (sample_rate)
            {
                case 352800:
                case 384000:
                    lpf_bps1 = 1;
                case 176400:
                case 192000:
                    lpf_bps2 = 1;
                case 88200:
                case 96000:
                    lpf_bps3 = 1;
                    spl_sel  = 1;
                    break;
                case 48000:
                    break;
                case 44100:
                    resample_bypass = 1;
                    break;
                default:
                    LOGW("%s, %d, music a2dp channel not support sample_rate: %d, use default 48000\n", __func__, __LINE__, sample_rate);
                    return BK_FAIL;
                    break;
            }
            audio_reg_hal_set_dac_cfg_dac_spl_sel(spl_sel);
            audio_reg_hal_set_dac_cfg_dac_lpf_bps3(lpf_bps3);
            audio_reg_hal_set_dac_cfg_dac_lpf_bps2(lpf_bps2);
            audio_reg_hal_set_dac_cfg_dac_lpf_bps1(lpf_bps1);
            audio_reg_hal_set_dac_cfg1_rsp_bps(resample_bypass);
            break;

        case AUD_DAC_SOURCE_CALL:
        case AUD_DAC_SOURCE_HINT:
            switch (sample_rate)
            {
                case 48000:
                    break;
                case 16000:
                    srindex = 1;
                    break;
                case 8000:
                    srindex = 2;
                    break;
                default:
                    LOGE("%s, %d, call and hint channel not support sample_rate: %d\n", __func__, __LINE__, sample_rate);
                    return BK_FAIL;
            }
            if (source == AUD_DAC_SOURCE_CALL) {
                audio_reg_hal_set_dac_cfg_call_spl_sel(srindex);
            } else {
                audio_reg_hal_set_dac_cfg_hint_spl_sel(srindex);
            }
            break;

        default:
            return BK_FAIL;
    }

    /* config apll frequency */
    bk_aud_apll_config((sample_rate / 44100 * 44100 == sample_rate) ? AUD_APLL_FREQ_90P3168_MHZ : AUD_APLL_FREQ_98P3040_MHZ);

    return BK_OK;
}

bk_err_t bk_aud_dac_spk0_set_source_gain(aud_dac_source_t source, uint32_t value)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            audio_reg_hal_set_dac_gain_cfg0_spk0_a2dp_gain(value);
            break;

        case AUD_DAC_SOURCE_CALL:
            audio_reg_hal_set_dac_gain_cfg1_spk0_call_gain(value);
            break;

        case AUD_DAC_SOURCE_HINT:
            audio_reg_hal_set_dac_gain_cfg2_spk0_hint_gain(value);
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_dac_spk1_set_source_gain(aud_dac_source_t source, uint32_t value)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            audio_reg_hal_set_dac_gain_cfg3_spk1_a2dp_gain(value);
            break;

        case AUD_DAC_SOURCE_CALL:
            audio_reg_hal_set_dac_gain_cfg4_spk1_call_gain(value);
            break;

        case AUD_DAC_SOURCE_HINT:
            audio_reg_hal_set_dac_gain_cfg5_spk1_hint_gain(value);
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_dac_spk0_get_source_gain(aud_dac_source_t source, uint32_t *value)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            *value = audio_reg_hal_get_dac_gain_cfg0_spk0_a2dp_gain();
            break;

        case AUD_DAC_SOURCE_CALL:
            *value = audio_reg_hal_get_dac_gain_cfg1_spk0_call_gain();
            break;

        case AUD_DAC_SOURCE_HINT:
            *value = audio_reg_hal_get_dac_gain_cfg2_spk0_hint_gain();
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_dac_spk1_get_source_gain(aud_dac_source_t source, uint32_t *value)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            *value = audio_reg_hal_get_dac_gain_cfg3_spk1_a2dp_gain();
            break;

        case AUD_DAC_SOURCE_CALL:
            *value = audio_reg_hal_get_dac_gain_cfg4_spk1_call_gain();
            break;

        case AUD_DAC_SOURCE_HINT:
            *value = audio_reg_hal_get_dac_gain_cfg5_spk1_hint_gain();
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_dac_set_dig_gain(uint32_t value)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    audio_reg_hal_set_dac_l_gain_mix_dac_l_gain(value);
    audio_reg_hal_set_dac_r_gain_mix_dac_r_gain(value);
    return BK_OK;
}

bk_err_t bk_aud_dac_get_dig_gain(uint32_t *value)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
    *value = audio_reg_hal_get_dac_l_gain_mix_dac_l_gain();
	return BK_OK;
}

bk_err_t bk_aud_dac_set_ana_gain(uint32_t value)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
	sys_drv_aud_dacg_set(value);
	return BK_OK;
}

static uint32_t bk_aud_dac_ana_gain_db_to_reg(int32_t db)
{
    if (db <= 0) {
        return 0;
    }
    if (db > BK_AUD_DAC_ANA_GAIN_DB_MAX) {
        db = (int32_t)BK_AUD_DAC_ANA_GAIN_DB_MAX;
    }

    {
        uint32_t reg = (uint32_t)db; /* 1dB/step */
        if (reg > DAC_ANA_GAIN_REG_MAX) {
            reg = DAC_ANA_GAIN_REG_MAX;
        }
        return reg;
    }
}

static int32_t bk_aud_dac_ana_gain_reg_to_db(uint32_t reg)
{
    if (reg > DAC_ANA_GAIN_REG_MAX) {
        reg = DAC_ANA_GAIN_REG_MAX;
    }
    return (int32_t)reg;
}

bk_err_t bk_aud_dac_set_ana_gain_db(int32_t db)
{
    AUD_DAC_RETURN_ON_NOT_INIT();

    if (db <= 0) {
        db = 0;
    } else if (db > BK_AUD_DAC_ANA_GAIN_DB_MAX) {
        db = (int32_t)BK_AUD_DAC_ANA_GAIN_DB_MAX;
    }

    return bk_aud_dac_set_ana_gain(bk_aud_dac_ana_gain_db_to_reg(db));
}

/* get audio dac analog gain */
bk_err_t bk_aud_dac_get_ana_gain(uint32_t *gain)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    *gain = sys_drv_aud_dacg_get();
    return BK_OK;
}

bk_err_t bk_aud_dac_get_ana_gain_db(int32_t *db)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    if (db == NULL) {
        LOGE("%s,%d db is NULL!\n", __func__, __LINE__);
        return BK_FAIL;
    }

    *db = bk_aud_dac_ana_gain_reg_to_db(sys_drv_aud_dacg_get());
    return BK_OK;
}

static uint32_t bk_aud_dac_dig_gain_db_to_reg(float db)
{
    if (db != db) {
        return 0;
    }
    if (db > BK_AUD_DAC_DIG_GAIN_DB_MAX) {
        db = BK_AUD_DAC_DIG_GAIN_DB_MAX;
    }
    if (db <= BK_AUD_DAC_DIG_GAIN_DB_SILENCE) {
        return 0;
    }

    float linear = powf(10.0f, db / 20.0f);
    float linear_max = powf(10.0f, BK_AUD_DAC_DIG_GAIN_DB_MAX / 20.0f);

    if (linear > linear_max) {
        linear = linear_max;
    }
    if (linear <= 0.0f) {
        return 0;
    }

    float reg_max_lin = 3.0f + (float)DAC_DIG_GAIN_FRAC_MASK / (float)DAC_DIG_GAIN_FRAC_SCALE;

    if (linear > reg_max_lin) {
        linear = reg_max_lin;
    }

    uint32_t int_part = (uint32_t)floorf(linear);

    if (int_part > 3u) {
        int_part = 3u;
    }
    float frac_f = linear - (float)int_part;
    uint32_t frac = (uint32_t)(frac_f * (float)DAC_DIG_GAIN_FRAC_SCALE + 0.5f);

    if (frac > DAC_DIG_GAIN_FRAC_MASK) {
        frac = DAC_DIG_GAIN_FRAC_MASK;
    }
    return (int_part << 28) | frac;
}

static float bk_aud_dac_dig_gain_reg_to_db(uint32_t reg)
{
    uint32_t sign     = (reg >> 30) & 1u;
    uint32_t int_part = (reg >> 28) & 3u;
    uint32_t frac     = reg & DAC_DIG_GAIN_FRAC_MASK;
    float mag         = (float)int_part + (float)frac / (float)DAC_DIG_GAIN_FRAC_SCALE;

    if (sign) {
        mag = -mag;
    }
    if (mag == 0.0f) {
        return BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
    }
    {
        float a = (mag < 0.0f) ? -mag : mag;
        return 20.0f * log10f(a);
    }
}

bk_err_t bk_aud_dac_set_dig_gain_db(float db)
{
    if (db != db) {
        LOGE("%s,%d db is NaN!\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if (db > BK_AUD_DAC_DIG_GAIN_DB_MAX) {
        db = BK_AUD_DAC_DIG_GAIN_DB_MAX;
    }
    if (db <= BK_AUD_DAC_DIG_GAIN_DB_SILENCE) {
        db = BK_AUD_DAC_DIG_GAIN_DB_SILENCE;
    }
    uint32_t reg = bk_aud_dac_dig_gain_db_to_reg(db);
    //LOGD("set dig gain to %f dB, reg: 0x%x\r\n", db, reg);
    bk_err_t ret = bk_aud_dac_set_dig_gain(reg);
    if (ret != BK_OK) {
        LOGE("%s,%d set dig gain to %f dB, reg: 0x%x fail!\n", __func__, __LINE__, db, reg);
        return ret;
    }

    return ret;
}

bk_err_t bk_aud_dac_get_dig_gain_db(float *db)
{
    if (db == NULL) {
        LOGE("%s,%d db is NULL!\n", __func__, __LINE__);
        return BK_FAIL;
    }
    uint32_t reg = 0;
    bk_err_t ret = bk_aud_dac_get_dig_gain(&reg);
    if (ret != BK_OK) {
        LOGE("%s,%d get dig gain fail!\n", __func__, __LINE__);
        return ret;
    }
    *db = bk_aud_dac_dig_gain_reg_to_db(reg);
    //LOGD("get dig gain from reg: 0x%x, db: %f\r\n", reg, *db);
    return ret;
}

bk_err_t bk_aud_dac_spk0_set_source_gain_db(aud_dac_source_t source, float db)
{
    if (db != db) {
        LOGE("%s,%d db is NaN!\n", __func__, __LINE__);
        return BK_FAIL;
    }
    return bk_aud_dac_spk0_set_source_gain(source, bk_aud_dac_dig_gain_db_to_reg(db));
}

bk_err_t bk_aud_dac_spk1_set_source_gain_db(aud_dac_source_t source, float db)
{
    if (db != db) {
        LOGE("%s,%d db is NaN!\n", __func__, __LINE__);
        return BK_FAIL;
    }
    return bk_aud_dac_spk1_set_source_gain(source, bk_aud_dac_dig_gain_db_to_reg(db));
}


bk_err_t bk_aud_dac_mute(void)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
	sys_drv_aud_dac_dacmute_en(1);
	return BK_OK;
}

bk_err_t bk_aud_dac_unmute(void)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
	sys_drv_aud_dac_dacmute_en(0);
	return BK_OK;
}

bk_err_t bk_aud_dac_spk0_write_data(aud_dac_source_t source, uint32_t pcm_value)
{
    AUD_DAC_RETURN_ON_NOT_INIT();

    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            audio_fifo_hal_set_spk0_a2dp_port_spk0_a2dp(pcm_value);
            break;

        case AUD_DAC_SOURCE_CALL:
            audio_fifo_hal_set_spk0_call_port_spk0_call(pcm_value);
            break;

        case AUD_DAC_SOURCE_HINT:
            audio_fifo_hal_set_spk0_hint_port_spk0_hint(pcm_value);
            break;

        default:
            return BK_FAIL;
    }
    return BK_OK;
}

uint32_t bk_aud_dac_spk0_read_data(void)
{
    return audio_fifo_hal_get_spk0_call_port_spk0_call();
}

bk_err_t bk_aud_dac_spk1_write_data(aud_dac_source_t source, uint32_t pcm_value)
{
    AUD_DAC_RETURN_ON_NOT_INIT();

    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            audio_fifo_hal_set_spk1_a2dp_port_spk1_a2dp(pcm_value);
            break;

        case AUD_DAC_SOURCE_CALL:
            audio_fifo_hal_set_spk1_call_port_spk1_call(pcm_value);
            break;

        case AUD_DAC_SOURCE_HINT:
            audio_fifo_hal_set_spk1_hint_port_spk1_hint(pcm_value);
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_dac_spk0_set_read_threshold(aud_dac_source_t source, uint16_t value)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            audio_reg_hal_set_spk0_fifo_cfg_spk0_a2dp_rd_thrd(value);
            break;

        case AUD_DAC_SOURCE_CALL:
            audio_reg_hal_set_spk0_fifo_cfg_spk0_call_rd_thrd(value);
            break;

        case AUD_DAC_SOURCE_HINT:
            audio_reg_hal_set_spk0_fifo_cfg_spk0_hint_rd_thrd(value);
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_dac_spk0_set_write_threshold(aud_dac_source_t source, uint16_t value)
{
	AUD_DAC_RETURN_ON_NOT_INIT();

    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            audio_reg_hal_set_spk0_fifo_cfg_spk0_a2dp_wr_thrd(value);
            break;

        case AUD_DAC_SOURCE_CALL:
            audio_reg_hal_set_spk0_fifo_cfg_spk0_call_wr_thrd(value);
            break;

        case AUD_DAC_SOURCE_HINT:
            audio_reg_hal_set_spk0_fifo_cfg_spk0_hint_wr_thrd(value);
            break;

        default:
            return BK_FAIL;
    }

	return BK_OK;
}

bk_err_t bk_aud_dac_spk1_set_read_threshold(aud_dac_source_t source, uint16_t value)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            audio_reg_hal_set_spk1_fifo_cfg_spk1_a2dp_rd_thrd(value);
            break;

        case AUD_DAC_SOURCE_CALL:
            audio_reg_hal_set_spk1_fifo_cfg_spk1_call_rd_thrd(value);
            break;

        case AUD_DAC_SOURCE_HINT:
            audio_reg_hal_set_spk1_fifo_cfg_spk1_hint_rd_thrd(value);
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_dac_spk1_set_write_threshold(aud_dac_source_t source, uint16_t value)
{
	AUD_DAC_RETURN_ON_NOT_INIT();

    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            audio_reg_hal_set_spk1_fifo_cfg_spk1_a2dp_wr_thrd(value);
            break;

        case AUD_DAC_SOURCE_CALL:
            audio_reg_hal_set_spk1_fifo_cfg_spk1_call_wr_thrd(value);
            break;

        case AUD_DAC_SOURCE_HINT:
            audio_reg_hal_set_spk1_fifo_cfg_spk1_hint_wr_thrd(value);
            break;

        default:
            return BK_FAIL;
    }

	return BK_OK;
}

bk_err_t bk_aud_dac_set_bits_width(aud_dac_source_t source, uint8_t bits_width)
{
    AUD_DAC_RETURN_ON_NOT_INIT();

    uint32_t dac_16b_sel = audio_reg_hal_get_dac_cfg_dac_16b_sel();

    if (bits_width == 16) {
        switch (source) {
            case AUD_DAC_SOURCE_A2DP:
                dac_16b_sel |= 1;
                break;

            case AUD_DAC_SOURCE_CALL:
                dac_16b_sel |= 1<<1;
                break;

            case AUD_DAC_SOURCE_HINT:
                dac_16b_sel |= 1<<2;
                break;

            default:
                return BK_FAIL;
        }
    } else if (bits_width == 24) {
        switch (source) {
            case AUD_DAC_SOURCE_A2DP:
                dac_16b_sel &= ~1;
                break;

            case AUD_DAC_SOURCE_CALL:
                dac_16b_sel &= ~(1<<1);
                break;

            case AUD_DAC_SOURCE_HINT:
                dac_16b_sel &= ~(1<<2);
                break;

            default:
                return BK_FAIL;
        }
    } else {
        LOGE("%s, bits_width: %d, not support\n", __func__, __LINE__, bits_width);
        return BK_FAIL;
    }

    audio_reg_hal_set_dac_cfg_dac_16b_sel(dac_16b_sel);

    return BK_OK;
}

bk_err_t bk_aud_dac_dacl_enable_int(void)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
//	sys_drv_aud_int_en(1);
	//aud_hal_set_fifo_config_dacl_int_en(1);
	//0x28[15:0]
	//TODO
    uint32_t int_mask = audio_reg_hal_get_aud_int_ctrl_aud_interrupt_mask();
    int_mask |= (7 << 16);      //enable spk0 a2dp、call、hint interrupt
    audio_reg_hal_set_aud_int_ctrl_aud_interrupt_mask(int_mask);

	return BK_OK;
}

bk_err_t bk_aud_dac_dacr_enable_int(void)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
//	sys_drv_aud_int_en(1);
	//aud_hal_set_fifo_config_dacr_int_en(1);
	//0x28[15:0]
	//TODO
    uint32_t int_mask = audio_reg_hal_get_aud_int_ctrl_aud_interrupt_mask();
    int_mask |= (7 << 19);      //enable spk1 a2dp、call、hint interrupt
    audio_reg_hal_set_aud_int_ctrl_aud_interrupt_mask(int_mask);

	return BK_OK;
}

bk_err_t bk_aud_dac_dacl_disable_int(void)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
	//aud_hal_set_fifo_config_dacl_int_en(0);
	//0x28[15:0]
	//TODO
    uint32_t int_mask = audio_reg_hal_get_aud_int_ctrl_aud_interrupt_mask();
    int_mask &= ~(7 << 16);      //disable spk0 a2dp、call、hint interrupt
    audio_reg_hal_set_aud_int_ctrl_aud_interrupt_mask(int_mask);

	return BK_OK;
}

bk_err_t bk_aud_dac_dacr_disable_int(void)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
	//aud_hal_set_fifo_config_dacr_int_en(0);
	//0x28[15:0]
	//TODO
    uint32_t int_mask = audio_reg_hal_get_aud_int_ctrl_aud_interrupt_mask();
    int_mask &= ~(7 << 19);      //disable spk1 a2dp、call、hint interrupt
    audio_reg_hal_set_aud_int_ctrl_aud_interrupt_mask(int_mask);

	return BK_OK;
}

/* get dac fifo port address */
bk_err_t bk_aud_dac_spk0_get_fifo_addr(aud_dac_source_t source, uint32_t *fifo_addr)
{
    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            aud_hal_dac_spk0_get_a2dp_fifo_addr(fifo_addr);
            break;

        case AUD_DAC_SOURCE_CALL:
            aud_hal_dac_spk0_get_call_fifo_addr(fifo_addr);
            break;

        case AUD_DAC_SOURCE_HINT:
            aud_hal_dac_spk0_get_hint_fifo_addr(fifo_addr);
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_dac_spk1_get_fifo_addr(aud_dac_source_t source, uint32_t *fifo_addr)
{
    switch (source) {
        case AUD_DAC_SOURCE_A2DP:
            aud_hal_dac_spk1_get_a2dp_fifo_addr(fifo_addr);
            break;

        case AUD_DAC_SOURCE_CALL:
            aud_hal_dac_spk1_get_call_fifo_addr(fifo_addr);
            break;

        case AUD_DAC_SOURCE_HINT:
            aud_hal_dac_spk1_get_hint_fifo_addr(fifo_addr);
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_dac_get_fifo_status(uint32_t *status)
{
	AUD_DAC_RETURN_ON_NOT_INIT();

	*status = audio_reg_hal_get_dac_ro_sts_dac_fifo_status();

	return BK_OK;
}

bk_err_t bk_aud_dac_spk0_source_enable(aud_dac_source_t source, uint32_t enable)
{
    AUD_DAC_RETURN_ON_NOT_INIT();

    uint32_t en_spk0  = audio_reg_hal_get_buf_ctrl_en_spk0();
    uint32_t en_spk1  = audio_reg_hal_get_buf_ctrl_en_spk1();
    uint32_t sw_board = audio_reg_hal_get_dac_cfg1_sw_board();

    if (enable) {
        switch (source) {
            case AUD_DAC_SOURCE_A2DP:
                en_spk0 |= 1;
                sw_board |= 1;
                break;

            case AUD_DAC_SOURCE_CALL:
                en_spk0 |= 1<<1;
                sw_board |= 1<<1;
                break;

            case AUD_DAC_SOURCE_HINT:
                en_spk0 |= 1<<2;
                sw_board |= 1<<2;
                break;

            default:
                return BK_FAIL;
        }
    } else {
        switch (source) {
            case AUD_DAC_SOURCE_A2DP:
                en_spk0 &= ~1;
                /* check spk1 a2dp enable status */
                if ((en_spk1 & 1) == 0) {
                    sw_board &= ~1;
                }
                break;

            case AUD_DAC_SOURCE_CALL:
                en_spk0 &= ~(1<<1);
                /* check spk1 call enable status */
                if ((en_spk1 & 1<<1) == 0) {
                    sw_board &= ~(1<<1);
                }
                break;

            case AUD_DAC_SOURCE_HINT:
                en_spk0 &= ~(1<<2);
                /* check spk1 hint enable status */
                if ((en_spk1 & 1<<2) == 0) {
                    sw_board &= ~(1<<2);
                }
                break;

            default:
                return BK_FAIL;
        }
    }

    audio_reg_hal_set_buf_ctrl_en_spk0(en_spk0);
    audio_reg_hal_set_dac_cfg1_sw_board(sw_board);
    return BK_OK;
}


bk_err_t bk_aud_dac_spk1_source_enable(aud_dac_source_t source, uint32_t enable)
{
    AUD_DAC_RETURN_ON_NOT_INIT();

    uint32_t en_spk0  = audio_reg_hal_get_buf_ctrl_en_spk0();
    uint32_t en_spk1  = audio_reg_hal_get_buf_ctrl_en_spk1();
    uint32_t sw_board = audio_reg_hal_get_dac_cfg1_sw_board();

    if (enable) {
        switch (source) {
            case AUD_DAC_SOURCE_A2DP:
                en_spk1 |= 1;
                sw_board |= 1;
                break;

            case AUD_DAC_SOURCE_CALL:
                en_spk1 |= 1<<1;
                sw_board |= 1<<1;
                break;

            case AUD_DAC_SOURCE_HINT:
                en_spk1 |= 1<<2;
                sw_board |= 1<<2;
                break;

            default:
                return BK_FAIL;
        }
    } else {
        switch (source) {
            case AUD_DAC_SOURCE_A2DP:
                en_spk1 &= ~1;
                /* check spk0 a2dp enable status */
                if ((en_spk0 & 1) == 0) {
                    sw_board &= ~1;
                }
                break;

            case AUD_DAC_SOURCE_CALL:
                en_spk1 &= ~(1<<1);
                /* check spk0 call enable status */
                if ((en_spk0 & 1<<1) == 0) {
                    sw_board &= ~(1<<1);
                }
                break;

            case AUD_DAC_SOURCE_HINT:
                en_spk1 &= ~(1<<2);
                /* check spk0 hint enable status */
                if ((en_spk0 & 1<<2) == 0) {
                    sw_board &= ~(1<<2);
                }
                break;

            default:
                return BK_FAIL;
        }
    }

    audio_reg_hal_set_buf_ctrl_en_spk1(en_spk1);
    audio_reg_hal_set_dac_cfg1_sw_board(sw_board);
    return BK_OK;
}

bk_err_t bk_aud_dac_source_enable(uint8_t spk, aud_dac_source_t source, uint32_t enable)
{
    bk_err_t ret = BK_OK;

    (void)spk;

    ret = bk_aud_dac_spk0_source_enable(source, enable);
    if (ret != BK_OK)
    {
        return ret;
    }

    return bk_aud_dac_spk1_source_enable(source, enable);
}

bk_err_t bk_aud_dac_start(aud_dac_chl_t dac_chl)
{
    AUD_DAC_RETURN_ON_NOT_INIT();
    switch (dac_chl) {
        case AUD_DAC_CHL_L:
            sys_drv_aud_dacl_en(1);
            sys_drv_aud_dacr_en(1);   ///
            break;

        case AUD_DAC_CHL_R:
            sys_drv_aud_dacl_en(1);   ///
            sys_drv_aud_dacr_en(1);
            break;

        case AUD_DAC_CHL_LR:
            sys_drv_aud_dacl_en(1);
            sys_drv_aud_dacr_en(1);
            break;

        default:
            LOGE("%s, not support dac channel: %d, line: %d \n", __func__, dac_chl, __LINE__);
            return BK_FAIL;
            break;
    }
    bk_aud_apll_spi_trigger();
    return BK_OK;
}

bk_err_t bk_aud_dac_stop(aud_dac_chl_t dac_chl)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
	switch (dac_chl) {
		case AUD_DAC_CHL_L:
			sys_drv_aud_dacl_en(0);
			sys_drv_aud_dacr_en(0);
			break;

		case AUD_DAC_CHL_R:
			sys_drv_aud_dacl_en(0);
			sys_drv_aud_dacr_en(0);
			break;

		case AUD_DAC_CHL_LR:
			sys_drv_aud_dacl_en(0);
			sys_drv_aud_dacr_en(0);
			break;

		default:
			LOGE("%s, not support dac channel: %d, line: %d \n", __func__, dac_chl, __LINE__);
			return BK_FAIL;
			break;
	}

	return BK_OK;
}

#if 0
bk_err_t bk_aud_dac_eq_config(aud_dac_eq_config_t *config)
{
	AUD_DAC_RETURN_ON_NOT_INIT();
	BK_RETURN_ON_NULL(config);

	aud_hal_dac_filt_config(config);
	aud_hal_set_extend_cfg_filt_enable(1);

	return BK_OK;
}

bk_err_t bk_aud_dac_eq_deconfig(void)
{
	AUD_DAC_RETURN_ON_NOT_INIT();

	aud_dac_eq_config_t config;
	aud_hal_set_extend_cfg_filt_enable(0);
	os_memset((void *)&config, 0, sizeof(aud_dac_eq_config_t));
	aud_hal_dac_filt_config(&config);

	return BK_OK;
}
#endif

/* register audio interrupt */
bk_err_t bk_aud_dac_register_isr(aud_isr_id_t isr_id, aud_isr_t isr)
{
	AUD_DAC_RETURN_ON_NOT_INIT();

	return bk_aud_register_aud_isr(isr_id, isr);
}

#if 0
/* set bypass audio dac dwa */
bk_err_t bk_aud_dac_set_dwa_bypass(uint8_t value)
{
	AUD_DAC_RETURN_ON_NOT_INIT();

	return sys_drv_aud_dac_bypass_dwa_en(value);
}
#endif

bk_err_t bk_aud_dac_get_fifo_addr(aud_dac_source_t dac_source, uint8_t ch, dma_dev_t *dma_dev, uint32_t *dac_fifo_addr)
{
    bk_err_t ret = BK_OK;

    if (dma_dev == NULL || dac_fifo_addr == NULL) {
        LOGE("%s,%d dma_dev:0x%x or dac_fifo_addr:0x%x is invalid!\n", __func__, __LINE__,dma_dev,dac_fifo_addr);
        return BK_FAIL;
    }

    if(0 == ch)
    {
        switch (dac_source)
        {
            case AUD_DAC_SOURCE_A2DP:
                *dma_dev = DMA_DEV_AUD_SPK0;
                bk_aud_dac_spk0_get_fifo_addr(AUD_DAC_SOURCE_A2DP, dac_fifo_addr);
                break;
            case AUD_DAC_SOURCE_CALL:
                *dma_dev = DMA_DEV_AUD_SPK0_CALL;
                bk_aud_dac_spk0_get_fifo_addr(AUD_DAC_SOURCE_CALL, dac_fifo_addr);
                break;
            case AUD_DAC_SOURCE_HINT:
                *dma_dev = DMA_DEV_AUD_SPK0_HINT;
                bk_aud_dac_spk0_get_fifo_addr(AUD_DAC_SOURCE_HINT, dac_fifo_addr);
                break;
            default:
                LOGE("%s,%d dac_source:%d is invalid!\n", __func__, __LINE__,dac_source);
                ret = BK_FAIL;
                break;
        }
    }
    else if(1 == ch)
    {
        switch (dac_source)
        {
            case AUD_DAC_SOURCE_A2DP:
                *dma_dev = DMA_DEV_AUD_SPK1_A2DP;
                bk_aud_dac_spk1_get_fifo_addr(AUD_DAC_SOURCE_A2DP, dac_fifo_addr);
                break;
            case AUD_DAC_SOURCE_CALL:
                *dma_dev = DMA_DEV_AUD_SPK1_CALL;
                bk_aud_dac_spk1_get_fifo_addr(AUD_DAC_SOURCE_CALL, dac_fifo_addr);
                break;
            case AUD_DAC_SOURCE_HINT:
                *dma_dev = DMA_DEV_AUD_SPK1_HINT;
                bk_aud_dac_spk1_get_fifo_addr(AUD_DAC_SOURCE_HINT, dac_fifo_addr);
                break;
            default:
                LOGE("%s,%d dac_source:%d is invalid!\n", __func__, __LINE__,dac_source);
                ret = BK_FAIL;
                break;
        }
    }
    else
    {
        ret = BK_FAIL;
        LOGE("%s,%d ch:%d is invalid!\n", __func__, __LINE__,ch);
    }

    return ret;
}



