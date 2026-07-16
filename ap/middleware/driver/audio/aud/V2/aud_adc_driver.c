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

#include <math.h>

#include <common/bk_include.h>
#include "aud_hal.h"

#include "sys_driver.h"
#include "clock_driver.h"
#include <os/os.h>
#include <os/mem.h>
#include <driver/int.h>
#include <driver/gpio.h>
#include <driver/aud_adc.h>
#include <driver/aud_adc_types.h>
#include <timer/timer_driver.h>

#include "gpio_driver.h"

#define TAG "aud_adc_drv"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define AUD_ADC_RETURN_ON_NOT_INIT() do {\
		if (!bk_aud_get_module_init_sta(AUD_MODULE_ADC)) {\
			return BK_ERR_AUD_ADC_NOT_INIT;\
		}\
	} while(0)

#define AUD_ADC_RETURN_ON_NULL(ptr) do {\
		if (NULL == ptr) {\
			return BK_ERR_AUD_ADC_INVALID_PARAM;\
		}\
	} while(0)

extern void delay(int num);


bk_err_t bk_aud_dmic_en(uint32_t v)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
	audio_reg_hal_set_sys_cfg_digmic_en(v);
	return BK_OK;
}
bk_err_t bk_aud_dmic_select(uint32_t chl)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
	audio_reg_hal_set_sys_cfg_dmic_sel(chl);
	return BK_OK;
}

bk_err_t bk_aud_dmic_clk_sel(uint32_t v)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
	audio_reg_hal_set_sys_cfg_clk_mic_sel(v);
	return BK_OK;
}

bk_err_t bk_aud_dmic_init(aud_dmic_config_t *dmic_config)
{
    AUD_ADC_RETURN_ON_NULL(dmic_config);
    AUD_ADC_RETURN_ON_NOT_INIT();

    aud_dmic_mode_t dmic_mode  = dmic_config->dmic_mode;
    aud_dmic_channel_t channel = dmic_config->channel;

    bk_aud_dmic_clk_sel(0);
    bk_aud_dmic_select(channel);

    switch (dmic_mode) {
        case AUD_DMIC_MODE_0:
#if CONFIG_USR_GPIO_CFG_EN
            gpio_dev_map_by_func(GPIO_DEV_DMIC0_CLK);
            gpio_dev_map_by_func(GPIO_DEV_DMIC0_DAT);
#endif
            bk_aud_dmic_en(0x1);  // '001
            break;
        case AUD_DMIC_MODE_1:
#if CONFIG_USR_GPIO_CFG_EN
            gpio_dev_map_by_func(GPIO_DEV_DMIC1_CLK);
            gpio_dev_map_by_func(GPIO_DEV_DMIC1_DAT);
#endif
            bk_aud_dmic_en(0x6);  // '110
            break;
        default:
            LOGE("%s, line: %d, not support dmic mode: %d\n", __func__, __LINE__, dmic_mode);
            return BK_FAIL;
    }
    return BK_OK;
}



bk_err_t bk_aud_adc_init(aud_adc_config_t *adc_config)
{
    bk_err_t ret = BK_OK;
    BK_RETURN_ON_NULL(adc_config);

    if (bk_aud_get_module_init_sta(AUD_MODULE_ADC)) {
        LOGW("aud adc is init already %s\n", __func__);
        ret = BK_FAIL;
        goto fail;
    }

    bk_aud_set_module_init_sta(AUD_MODULE_ADC, true);

    /* audio common driver init */
    if (BK_OK != bk_aud_driver_init()) {
        LOGE("%s, audio driver init fail, line: %d \n", __func__, __LINE__);
        return BK_ERR_AUD_DRV_NOT_INIT;
    }

    /* select audio clock */
    bk_aud_clk_config(adc_config->clk_src);

    //enable audio adc power
    sys_drv_aud_adcbias_en(1);
    sys_drv_aud_micbias_en(1);
    sys_drv_aud_audbias_en(1);
    bk_timer_delay_us(1000);

    /* config mic analog gain in dB */
    bk_aud_adc_set_ana_gain_db(AUD_ADC_CHL_0, adc_config->chl_cfg[0].ana_gain);
    bk_aud_adc_set_ana_gain_db(AUD_ADC_CHL_1, adc_config->chl_cfg[1].ana_gain);
    bk_aud_adc_set_ana_gain_db(AUD_ADC_CHL_2, adc_config->chl_cfg[2].ana_gain);

    /* config adc channel digital gain in dB */
    bk_aud_adc_set_dig_gain_db(AUD_ADC_CHL_0, adc_config->chl_cfg[0].dig_gain);
    bk_aud_adc_set_dig_gain_db(AUD_ADC_CHL_1, adc_config->chl_cfg[1].dig_gain);
    bk_aud_adc_set_dig_gain_db(AUD_ADC_CHL_2, adc_config->chl_cfg[2].dig_gain);

    /* config mic mode */
    bk_aud_adc_set_mic_mode(AUD_ADC_CHL_0, adc_config->chl_cfg[0].adc_mode);
    bk_aud_adc_set_mic_mode(AUD_ADC_CHL_1, adc_config->chl_cfg[1].adc_mode);
    bk_aud_adc_set_mic_mode(AUD_ADC_CHL_2, adc_config->chl_cfg[2].adc_mode);

    /* bits width */
    bk_aud_adc_set_bits_width(AUD_ADC_CHL_0, adc_config->chl_cfg[0].bits);
    bk_aud_adc_set_bits_width(AUD_ADC_CHL_1, adc_config->chl_cfg[1].bits);
    bk_aud_adc_set_bits_width(AUD_ADC_CHL_2, adc_config->chl_cfg[2].bits);

    /* Configure AEC loopback: 0=off, 1/2/3=hardware loopback mode */
    {
        uint8_t aec_en = adc_config->aec_en & 0x3;
        if (aec_en) {
            audio_reg_hal_set_adc_cfg_aec_en(1);//(aec_en);
            audio_reg_hal_set_adc_cfg_aec_16b_sel(1);//(aec_en);
        } else
		{
            audio_reg_hal_set_adc_cfg_aec_en(0);
            audio_reg_hal_set_adc_cfg_aec_16b_sel(0);
		}
    }

    audio_reg_hal_set_adc_cfg_clk_adc_inv(adc_config->adc_samp_edge);

    //aud_hal_set_adc_config0_adc_hpf1_bypass(1);
    //aud_hal_set_adc_config0_adc_hpf2_bypass(1);

    LOGD("configure mic and adc\r\n");

    if (BK_OK != bk_aud_adc_set_samp_rate(adc_config->sample_rate)) {
        ret = BK_FAIL;
        goto fail;
    }

    return ret;

fail:
    bk_aud_set_module_init_sta(AUD_MODULE_ADC, false);
    return ret;
}

bk_err_t bk_aud_adc_deinit(void)
{
	AUD_ADC_RETURN_ON_NOT_INIT();

	bk_aud_adc_stop(AUD_ADC_CHL_0);
	bk_aud_adc_stop(AUD_ADC_CHL_1);
	bk_aud_adc_stop(AUD_ADC_CHL_2);

	//enable mic1 and mic2
	sys_drv_aud_mic1_en(0);
	sys_drv_aud_mic2_en(0);
	sys_drv_aud_mic3_en(0);

	//disable audio adc power
	sys_drv_aud_adcbias_en(0);
	sys_drv_aud_micbias_en(0);

	//config system registers about audio adc to default value
	sys_drv_aud_mic1_gain_set(0);
	sys_drv_aud_mic2_gain_set(0);
	sys_drv_aud_mic3_gain_set(0);

    audio_reg_hal_set_adc_gain_cfg2_adc_chn0_gain(0);
    audio_reg_hal_set_adc_gain_cfg1_adc_chn1_gain(0);
    audio_reg_hal_set_adc_gain_cfg5_adc_chn2_gain(0);

    audio_reg_hal_set_adc_cfg_aec_en(0x0);
    audio_reg_hal_set_adc_cfg_aec_16b_sel(0x0);

	audio_reg_hal_set_adc_cfg_clk_adc_inv(AUD_ADC_SAMP_EDGE_RISING);

	//aud_hal_set_adc_config0_adc_hpf1_bypass(0);
	//aud_hal_set_adc_config0_adc_hpf2_bypass(0);

	bk_aud_adc_set_samp_rate(8000);

	bk_aud_set_module_init_sta(AUD_MODULE_ADC, false);

	bk_aud_driver_deinit();

	return BK_OK;
}

bk_err_t bk_aud_adc_set_samp_rate(uint32_t sample_rate)
{
    AUD_ADC_RETURN_ON_NOT_INIT();

    uint32_t cic;
    uint32_t fir1_bps = 0;
    uint32_t fir2_bps = 0;
    uint32_t fir3_bps = 1;

    switch (sample_rate)
    {
        case 8000:
            cic = AUDIO_ADC_CIC_D192;
            break;

        case 11025:
        case 12000:
            cic = AUDIO_ADC_CIC_D128;
            break;

        case 16000:
            cic = AUDIO_ADC_CIC_D96;
            break;

        case 22050:
        case 24000:
            cic = AUDIO_ADC_CIC_D64;
            break;

        case 32000:
            cic = AUDIO_ADC_CIC_D48;
            break;

        case 44100:
        case 48000:
            cic = AUDIO_ADC_CIC_D32;
            break;

        case 64000:
            cic = AUDIO_ADC_CIC_D48;
            fir2_bps = 1;
            break;

        case 88200:
        case 96000:
            cic = AUDIO_ADC_CIC_D16;
            break;

        case 128000:
            cic = AUDIO_ADC_CIC_D48;
            fir1_bps = 1;
            fir2_bps = 1;
            break;

        case 176400:
        case 192000:
            cic = AUDIO_ADC_CIC_D16;
            fir2_bps = 1;
            break;

        case 352800:
        case 384000:
            cic = AUDIO_ADC_CIC_D16;
            fir1_bps = 1;
            fir2_bps = 1;
            break;

        default:
            LOGE("%s, line: %d, not support sample rate: %d\n", __func__, __LINE__, sample_rate);
            return BK_FAIL;
    }

    audio_reg_hal_set_sys_cfg_rx_sp_sel(cic);
    audio_reg_hal_set_adc_cfg_adc_lpf_bps1(fir1_bps);
    audio_reg_hal_set_adc_cfg_adc_lpf_bps2(fir2_bps);
    audio_reg_hal_set_adc_cfg_adc_lpf_bps3(fir3_bps);

    /* config apll frequency */
    bk_aud_apll_config((sample_rate / 44100 * 44100 == sample_rate) ? AUD_APLL_FREQ_90P3168_MHZ : AUD_APLL_FREQ_98P3040_MHZ);

    return BK_OK;
}

bk_err_t bk_aud_adc_set_ana_gain(aud_adc_chl_t chl, uint32_t value)
{
    AUD_ADC_RETURN_ON_NOT_INIT();
    switch (chl) {
        case AUD_ADC_CHL_0:
            sys_drv_aud_mic1_gain_set(value);
            break;

        case AUD_ADC_CHL_1:
            sys_drv_aud_mic2_gain_set(value);
            break;

        case AUD_ADC_CHL_2:
            sys_drv_aud_mic3_gain_set(value);
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

static uint32_t bk_aud_adc_ana_gain_db_to_reg(int32_t db)
{
    if (db <= 0) {
        return 0;
    }
    if (db > BK_AUD_ADC_ANA_GAIN_DB_MAX) {
        db = (int32_t)BK_AUD_ADC_ANA_GAIN_DB_MAX;
    }

    {
        uint32_t reg = (uint32_t)((db + 1) / 2); /* 2dB/step, round to nearest */
        if (reg > ADC_ANA_GAIN_REG_MAX) {
            reg = ADC_ANA_GAIN_REG_MAX;
        }
        return reg;
    }
}

static int32_t bk_aud_adc_ana_gain_reg_to_db(uint32_t reg)
{
    if (reg > ADC_ANA_GAIN_REG_MAX) {
        reg = ADC_ANA_GAIN_REG_MAX;
    }
    return (int32_t)(reg * 2u);
}

static bk_err_t bk_aud_adc_get_ana_gain_reg(aud_adc_chl_t chl, uint32_t *value)
{
    switch (chl) {
        case AUD_ADC_CHL_0:
            *value = sys_ll_get_ana_reg21_micgain_mic1();
            break;

        case AUD_ADC_CHL_1:
            *value = sys_ll_get_ana_reg27_micgain_mic2();
            break;

        case AUD_ADC_CHL_2:
            *value = sys_ll_get_ana_reg28_micgain_mic3();
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_adc_set_ana_gain_db(aud_adc_chl_t chl, int32_t db)
{
    AUD_ADC_RETURN_ON_NOT_INIT();

    if (db <= 0) {
        db = 0;
    } else if (db > BK_AUD_ADC_ANA_GAIN_DB_MAX) {
        db = (int32_t)BK_AUD_ADC_ANA_GAIN_DB_MAX;
    }

    return bk_aud_adc_set_ana_gain(chl, bk_aud_adc_ana_gain_db_to_reg(db));
}

bk_err_t bk_aud_adc_get_ana_gain_db(aud_adc_chl_t chl, int32_t *db)
{
    AUD_ADC_RETURN_ON_NOT_INIT();
    if (db == NULL) {
        LOGE("%s,%d db is NULL!\n", __func__, __LINE__);
        return BK_FAIL;
    }

    uint32_t reg = 0;
    bk_err_t ret = bk_aud_adc_get_ana_gain_reg(chl, &reg);
    if (ret != BK_OK) {
        LOGE("%s,%d get adc ana gain fail!\n", __func__, __LINE__);
        return ret;
    }

    *db = bk_aud_adc_ana_gain_reg_to_db(reg);
    return BK_OK;
}

bk_err_t bk_aud_adc_set_dig_gain(aud_adc_chl_t chl, uint32_t value)
{
    AUD_ADC_RETURN_ON_NOT_INIT();

    switch (chl) {
        case AUD_ADC_CHL_0:
            audio_reg_hal_set_adc_gain_cfg2_adc_chn0_gain(value);
            break;

        case AUD_ADC_CHL_1:
            audio_reg_hal_set_adc_gain_cfg1_adc_chn1_gain(value);
            break;

        case AUD_ADC_CHL_2:
            audio_reg_hal_set_adc_gain_cfg5_adc_chn2_gain(value);
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

static uint32_t bk_aud_adc_dig_gain_db_to_reg(float db)
{
    if (db != db) {
        return 0;
    }
    if (db > BK_AUD_ADC_DIG_GAIN_DB_MAX) {
        db = BK_AUD_ADC_DIG_GAIN_DB_MAX;
    }
    if (db <= BK_AUD_ADC_DIG_GAIN_DB_SILENCE) {
        return 0;
    }

    float linear = powf(10.0f, db / 20.0f);
    float linear_max = powf(10.0f, BK_AUD_ADC_DIG_GAIN_DB_MAX / 20.0f);

    if (linear > linear_max) {
        linear = linear_max;
    }
    if (linear <= 0.0f) {
        return 0;
    }

    float reg_max_lin = (float)ADC_DIG_GAIN_INT_MASK
                      + (float)ADC_DIG_GAIN_FRAC_MASK / (float)ADC_DIG_GAIN_FRAC_SCALE;

    if (linear > reg_max_lin) {
        linear = reg_max_lin;
    }

    uint32_t int_part = (uint32_t)floorf(linear);
    if (int_part > ADC_DIG_GAIN_INT_MASK) {
        int_part = ADC_DIG_GAIN_INT_MASK;
    }

    float frac_f = linear - (float)int_part;
    uint32_t frac = (uint32_t)(frac_f * (float)ADC_DIG_GAIN_FRAC_SCALE + 0.5f);
    if (frac > ADC_DIG_GAIN_FRAC_MASK) {
        frac = ADC_DIG_GAIN_FRAC_MASK;
    }

    return (int_part << ADC_DIG_GAIN_INT_SHIFT) | frac;
}

static float bk_aud_adc_dig_gain_reg_to_db(uint32_t reg)
{
    uint32_t int_part = (reg >> ADC_DIG_GAIN_INT_SHIFT) & ADC_DIG_GAIN_INT_MASK;
    uint32_t frac = reg & ADC_DIG_GAIN_FRAC_MASK;
    float linear = (float)int_part + (float)frac / (float)ADC_DIG_GAIN_FRAC_SCALE;

    if (linear == 0.0f) {
        return BK_AUD_ADC_DIG_GAIN_DB_SILENCE;
    }

    return 20.0f * log10f(linear);
}

static bk_err_t bk_aud_adc_get_dig_gain_reg(aud_adc_chl_t chl, uint32_t *value)
{
    switch (chl) {
        case AUD_ADC_CHL_0:
            *value = audio_reg_hal_get_adc_gain_cfg2_adc_chn0_gain();
            break;

        case AUD_ADC_CHL_1:
            *value = audio_reg_hal_get_adc_gain_cfg1_adc_chn1_gain();
            break;

        case AUD_ADC_CHL_2:
            *value = audio_reg_hal_get_adc_gain_cfg5_adc_chn2_gain();
            break;

        default:
            return BK_FAIL;
    }

    return BK_OK;
}

bk_err_t bk_aud_adc_set_dig_gain_db(aud_adc_chl_t chl, float db)
{
    AUD_ADC_RETURN_ON_NOT_INIT();

    if (db != db) {
        LOGE("%s,%d db is NaN!\n", __func__, __LINE__);
        return BK_FAIL;
    }
    if (db > BK_AUD_ADC_DIG_GAIN_DB_MAX) {
        db = BK_AUD_ADC_DIG_GAIN_DB_MAX;
    }
    if (db <= BK_AUD_ADC_DIG_GAIN_DB_SILENCE) {
        db = BK_AUD_ADC_DIG_GAIN_DB_SILENCE;
    }

    uint32_t reg = bk_aud_adc_dig_gain_db_to_reg(db);
    bk_err_t ret = bk_aud_adc_set_dig_gain(chl, reg);
    if (ret != BK_OK) {
        LOGE("%s,%d set adc dig gain to %f dB, reg: 0x%x fail!\n", __func__, __LINE__, db, reg);
    }

    return ret;
}

bk_err_t bk_aud_adc_get_dig_gain_db(aud_adc_chl_t chl, float *db)
{
    AUD_ADC_RETURN_ON_NOT_INIT();
    if (db == NULL) {
        LOGE("%s,%d db is NULL!\n", __func__, __LINE__);
        return BK_FAIL;
    }

    uint32_t reg = 0;
    bk_err_t ret = bk_aud_adc_get_dig_gain_reg(chl, &reg);
    if (ret != BK_OK) {
        LOGE("%s,%d get adc dig gain fail!\n", __func__, __LINE__);
        return ret;
    }

    *db = bk_aud_adc_dig_gain_reg_to_db(reg);
    return BK_OK;
}

bk_err_t bk_aud_adc_set_mic_mode(aud_adc_chl_t chl, aud_adc_mode_t mode)
{
	AUD_ADC_RETURN_ON_NOT_INIT();

	uint32_t intf_value = 0;

	if (mode == AUD_ADC_MODE_SIGNAL_END) {
		intf_value = 1;
	} else {
		intf_value = 0;
	}

	switch (chl) {
		case AUD_ADC_CHL_0:
			sys_drv_aud_mic1_single_en(intf_value);
			break;

		case AUD_ADC_CHL_1:
			sys_drv_aud_mic2_single_en(intf_value);
			break;

		case AUD_ADC_CHL_2:
			sys_drv_aud_mic3_single_en(intf_value);
			break;

		default:
			break;
	}

	return BK_OK;
}

/* get adc fifo port address */
bk_err_t bk_aud_adc_get_fifo_addr(aud_adc_mic_data_bus_t data_bus, uint32_t *fifo_addr)
{
    if (data_bus == AUD_ADC_MIC_DATA_BUS_1) {
        aud_hal_adc_get_mic1_data_bus_fifo_addr(fifo_addr);
    } else {
        aud_hal_adc_get_mic0_data_bus_fifo_addr(fifo_addr);
    }

    return BK_OK;
}

/* get adc fifo data */
bk_err_t bk_aud_adc_get_fifo_data(aud_adc_mic_data_bus_t data_bus, uint32_t *adc_data)
{
	AUD_ADC_RETURN_ON_NOT_INIT();

    if (data_bus == AUD_ADC_MIC_DATA_BUS_1) {
        *adc_data = audio_fifo_hal_get_mic1_data_bus_mic1_data_bus();
    } else {
        *adc_data = audio_fifo_hal_get_mic0_data_bus_mic0_data_bus();
    }

	return BK_OK;
}

/* get audio adc fifo and agc status */
bk_err_t bk_aud_adc_get_fifo_status(uint32_t *status)
{
	AUD_ADC_RETURN_ON_NOT_INIT();

	*status = audio_reg_hal_get_adc_ro_sts_adc_fifo_status();
	return BK_OK;
}

/* start adc to dac test */
bk_err_t bk_aud_adc_start_loop_test(void)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
	//aud_hal_set_fifo_config_loop_adc2dac(1);
	audio_reg_hal_set_dac_cfg_spk2mic_tst(1); ///???
	//TODO

	return BK_OK;
}

/* stop adc to dac test */
bk_err_t bk_aud_adc_stop_loop_test(void)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
	//aud_hal_set_fifo_config_loop_adc2dac(0);
	audio_reg_hal_set_dac_cfg_spk2mic_tst(0); ///???
	//TODO
	return BK_OK;
}

/* enable adc interrupt */
bk_err_t bk_aud_adc_enable_int(void)
{
	AUD_ADC_RETURN_ON_NOT_INIT();

    uint32_t int_mask = audio_reg_hal_get_aud_int_ctrl_aud_interrupt_mask();
    int_mask |= (2 << 22);      //enable mic1 mic2
    audio_reg_hal_set_aud_int_ctrl_aud_interrupt_mask(int_mask);

	return BK_OK;
}

/* disable adc interrupt */
bk_err_t bk_aud_adc_disable_int(void)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
    uint32_t int_mask = audio_reg_hal_get_aud_int_ctrl_aud_interrupt_mask();
    int_mask &= ~(2 << 22);      //disable mic1 mic2
    audio_reg_hal_set_aud_int_ctrl_aud_interrupt_mask(int_mask);

	return BK_OK;
}

/* enable adc and adc start work */
bk_err_t bk_aud_adc_start(aud_adc_chl_t chl)
{
    AUD_ADC_RETURN_ON_NOT_INIT();

    /* enable analog mic */
    switch (chl) {
        case AUD_ADC_CHL_0:
            sys_drv_aud_mic1_en(1);
            sys_drv_aud_mic1_rst_set(1);
            bk_timer_delay_us(100);
            sys_drv_aud_mic1_rst_set(0);
            break;

        case AUD_ADC_CHL_1:
            sys_drv_aud_mic2_en(1);
            sys_drv_aud_mic2_rst_set(1);
            bk_timer_delay_us(100);
            sys_drv_aud_mic2_rst_set(0);
            break;

        case AUD_ADC_CHL_2:
            sys_drv_aud_mic3_en(1);
            sys_drv_aud_mic3_rst_set(1);
            bk_timer_delay_us(100);
            sys_drv_aud_mic3_rst_set(0);
            break;

        default:
            return BK_FAIL;
    }

#if 0

    uint32_t adc_en =audio_reg_hal_get_adc_cfg_adc_en();

    adc_en |= 1<<chl;
    audio_reg_hal_set_adc_cfg_adc_en(adc_en);

    /* enable mic0 data bus */
    uint32_t en_mic = audio_reg_hal_get_buf_ctrl_en_mic();
    en_mic |= 1;
    audio_reg_hal_set_buf_ctrl_en_mic(en_mic);

    /* 当所有adc通道仅有adc0使能时，置一该bit使能mic0_data_bus读出 */
    /* Set bit8 to 1 to enable mic0_data_bus read port, when only enable adc0 channel */
    if (adc_en == 1<<AUD_ADC_CHL_0 ) {
        uint32_t sw_board = audio_reg_hal_get_dac_cfg1_sw_board();
        sw_board |= 1<<3;
        audio_reg_hal_set_dac_cfg1_sw_board(sw_board);
    }
#endif
    return BK_OK;
}

/* disable adc and adc stop work */
bk_err_t bk_aud_adc_stop(aud_adc_chl_t chl)
{
	AUD_ADC_RETURN_ON_NOT_INIT();

    uint32_t adc_en =audio_reg_hal_get_adc_cfg_adc_en();

    adc_en &= ~(1<<chl);
    audio_reg_hal_set_adc_cfg_adc_en(adc_en);

    /* check whether all channel disable, disable mic0 data bus */
    if (adc_en == 0) {
        audio_reg_hal_set_buf_ctrl_en_mic(0);
    }

    if (chl == AUD_ADC_CHL_0) {
        uint32_t sw_board = audio_reg_hal_get_dac_cfg1_sw_board();
        sw_board &= ~(1<<3);
        audio_reg_hal_set_dac_cfg1_sw_board(sw_board);
    }

    /* enable analog mic */
    switch (chl) {
        case AUD_ADC_CHL_0:
            sys_drv_aud_mic1_en(0);
            break;

        case AUD_ADC_CHL_1:
            sys_drv_aud_mic2_en(0);
            break;

        case AUD_ADC_CHL_2:
            sys_drv_aud_mic3_en(0);
            break;

        default:
            return BK_FAIL;
    }

	return BK_OK;
}


void bk_aud_adc_mic_data_bus_en(void)
{
    /* enable mic data bus */
    uint32_t en_mic = audio_reg_hal_get_buf_ctrl_en_mic();
    en_mic |= 1;
    audio_reg_hal_set_buf_ctrl_en_mic(en_mic);
}

void bk_aud_adc_enable_used_channel(uint32_t ch_bitmap)
{
    bk_aud_adc_mic_data_bus_en();

    audio_reg_hal_set_adc_cfg_adc_en(ch_bitmap);

    /* Set bit8 to 1 to enable mic0_data_bus read port, when only enable adc0 channel */
    if (ch_bitmap == 1 << AUD_ADC_CHL_0)
    {
        uint32_t sw_board = audio_reg_hal_get_dac_cfg1_sw_board();
        sw_board |= (1 << 3);
        audio_reg_hal_set_dac_cfg1_sw_board(sw_board);
    }

    bk_aud_apll_spi_trigger();
}

bk_err_t bk_aud_adc_set_bits_width(aud_adc_chl_t chl, uint8_t bits_width)
{
    AUD_ADC_RETURN_ON_NOT_INIT();

    uint32_t adc_16b_sel =audio_reg_hal_get_adc_cfg_adc_16b_sel();

    if (bits_width == 16)
    {
        adc_16b_sel |= 1<<chl;
    }
    else if (bits_width == 24)
    {
        adc_16b_sel &= ~(1<<chl);
    }
    else
    {
        LOGE("%s, bits_width: %d, not support\n", __func__, bits_width);
        return BK_FAIL;
    }

    audio_reg_hal_set_adc_cfg_adc_16b_sel(adc_16b_sel);

    return BK_OK;
}

/* set mic_data_bus write threshold */
bk_err_t bk_aud_adc_set_write_threshold(aud_adc_mic_data_bus_t data_bus, uint32_t value)
{
    AUD_ADC_RETURN_ON_NOT_INIT();
    if (data_bus == AUD_ADC_MIC_DATA_BUS_1) {
        audio_reg_hal_set_mic_fifo_cfg_mic1_wr_thrd(value);
    } else {
        audio_reg_hal_set_mic_fifo_cfg_mic0_wr_thrd(value);
    }

    return BK_OK;
}

/* set mic_data_bus read threshold */
bk_err_t bk_aud_adc_set_read_threshold(aud_adc_mic_data_bus_t data_bus, uint32_t value)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
    if (data_bus == AUD_ADC_MIC_DATA_BUS_1) {
        audio_reg_hal_set_mic_fifo_cfg_mic0_rd_thrd(value);
    } else {
        audio_reg_hal_set_mic_fifo_cfg_mic0_rd_thrd(value);
    }

	return BK_OK;
}

/* register audio interrupt */
bk_err_t bk_aud_adc_register_isr(aud_isr_t isr)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
	return bk_aud_register_aud_isr(AUD_ISR_ADCL, isr);
}

/* hpf config */
bk_err_t bk_aud_adc_hpf_config(aud_adc_hpf_config_t *config)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
	return aud_hal_adc_hpf_config(config);
}

/* agc config */
bk_err_t bk_aud_adc_agc_config(aud_adc_agc_config_t *config)
{
	AUD_ADC_RETURN_ON_NOT_INIT();
	return aud_hal_adc_agc_config(config);
}

void bk_aud_adc_dump_reg(void)
{
	uint32_t adc_bits     = audio_reg_hal_get_adc_cfg_adc_16b_sel();
	uint32_t rx_sp_sel    = audio_reg_hal_get_sys_cfg_rx_sp_sel();
	uint32_t adc_lpf_bps1 = audio_reg_hal_get_adc_cfg_adc_lpf_bps1();
	uint32_t adc_lpf_bps2 = audio_reg_hal_get_adc_cfg_adc_lpf_bps2();
	uint32_t adc_lpf_bps3 = audio_reg_hal_get_adc_cfg_adc_lpf_bps3();
	uint32_t dig_gain     = audio_reg_hal_get_adc_gain_cfg2_adc_chn0_gain();// adc chl0
	uint32_t ana_gain     = sys_ll_get_ana_reg21_micgain_mic1();            // mic1
	LOGD("adc_bits:%d, rx_sp_sel:%d, adc_lpf_bps1:%d, adc_lpf_bps2:%d, adc_lpf_bps3:%d, dig_gain:0x%x, ana_gain:0x%x\n",
        adc_bits, rx_sp_sel, adc_lpf_bps1, adc_lpf_bps2, adc_lpf_bps3, dig_gain, ana_gain);
}
