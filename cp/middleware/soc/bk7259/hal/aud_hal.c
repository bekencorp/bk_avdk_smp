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

#include <common/bk_include.h>
#include "sys_driver.h"
#include "aud_hal.h"

#define SYS_ANA_REG18_ISELAUD_DEFAULT_VAL                      (0x01)
#define SYS_ANA_REG18_AUDCK_RLCEN1V_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG18_LCHCKINVEN1V_DEFAULT_VAL                 (0x01)
#define SYS_ANA_REG18_ENAUDBIAS_DEFAULT_VAL                    (0x00)
#define SYS_ANA_REG18_ENADCBIAS_DEFAULT_VAL                    (0x00)
#define SYS_ANA_REG18_ENMICBIAS_DEFAULT_VAL                    (0x00)
#define SYS_ANA_REG18_ADCCKINVEN1V_DEFAULT_VAL                 (0x00)
#define SYS_ANA_REG18_DACFB2ST0V9_DEFAULT_VAL                  (0x01)
#define SYS_ANA_REG18_NC1_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG18_MICBIAS_TRM_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG18_MICBIAS_VOC_DEFAULT_VAL                  (0x10)
#define SYS_ANA_REG18_VREFSEL1V_DEFAULT_VAL                    (0x01)
#define SYS_ANA_REG18_CAPSWSPI_DEFAULT_VAL                     (0x1f)
#define SYS_ANA_REG18_ADREF_SEL_DEFAULT_VAL                    (0x02)
#define SYS_ANA_REG18_NC0_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG18_RESERVED_BIT_26_30_DEFAULT_VAL           (0x00)
#define SYS_ANA_REG18_SPI_DACCKPSSEL_DEFAULT_VAL               (0x00)

#define SYS_ANA_REG19_ISEL_DEFAULT_VAL                         (0x02)
#define SYS_ANA_REG19_MICIRSEL1_DEFAULT_VAL                    (0x01)
#define SYS_ANA_REG19_MICDACIT_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG19_MICDACIH_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG19_MICSINGLEEN_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG19_DCCOMPEN_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG19_MICGAIN_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG19_MICDACEN_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG19_STG2LSEN1V_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG19_OPENLOOPCAL1V_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG19_CALLATCH_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG19_VCMSEL_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG19_DWAMODE_DEFAULT_VAL                      (0x01)
#define SYS_ANA_REG19_R2REN_DEFAULT_VAL                        (0x00)
#define SYS_ANA_REG19_NC_26_27_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG19_MICEN_DEFAULT_VAL                        (0x00)
#define SYS_ANA_REG19_RST_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG19_BPDWA1V_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG19_HCEN1STG_DEFAULT_VAL                     (0x01)

#define SYS_ANA_REG20_HPDAC_DEFAULT_VAL                        (0x01)
#define SYS_ANA_REG20_CALCON_SEL_DEFAULT_VAL                   (0x01)
#define SYS_ANA_REG20_OSCDAC_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG20_OCENDAC_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG20_VCMSEL_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG20_ADJDACREF_DEFAULT_VAL                    (0x10)
#define SYS_ANA_REG20_DCOCHG_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG20_DIFFEN_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG20_ENDACCAL_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG20_NC2_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG20_LENDCOC_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG20_NC1_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG20_LENVCMD_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG20_DACDRVEN_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG20_NC0_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG20_DACLEN_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG20_DACG_DEFAULT_VAL                         (0x0f)
#define SYS_ANA_REG20_DACMUTE_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG20_DACDWAMODE_SEL_DEFAULT_VAL               (0x01)
#define SYS_ANA_REG20_DACSEL_DEFAULT_VAL                       (0x0f)

#define SYS_ANA_REG21_LMDCIN_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG21_NC1_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG21_SPIRST_OVC_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG21_NC0_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG21_ENIDACL_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG21_DAC3RDHC0V9_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG21_HC2S_DEFAULT_VAL                         (0x01)
#define SYS_ANA_REG21_RFB_CTRL_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG21_VCMSEL_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG21_ENBS_DEFAULT_VAL                         (0x00)
#define SYS_ANA_REG21_CALCK_SEL0V9_DEFAULT_VAL                 (0x00)
#define SYS_ANA_REG21_BPDWA0V9_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG21_LOOPRST0V9_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG21_OCT0V9_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG21_SOUT0V9_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG21_HC0V9_DEFAULT_VAL                        (0x00)

static uint32_t ana_reg18_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG18_ISELAUD_DEFAULT_VAL & SYS_ANA_REG18_ISELAUD_MASK) << SYS_ANA_REG18_ISELAUD_POS);
	value |= ((SYS_ANA_REG18_AUDCK_RLCEN1V_DEFAULT_VAL & SYS_ANA_REG18_AUDCK_RLCEN1V_MASK) << SYS_ANA_REG18_AUDCK_RLCEN1V_POS);
	value |= ((SYS_ANA_REG18_LCHCKINVEN1V_DEFAULT_VAL & SYS_ANA_REG18_LCHCKINVEN1V_MASK) << SYS_ANA_REG18_LCHCKINVEN1V_POS);
	value |= ((SYS_ANA_REG18_ENAUDBIAS_DEFAULT_VAL & SYS_ANA_REG18_ENAUDBIAS_MASK) << SYS_ANA_REG18_ENAUDBIAS_POS);
	value |= ((SYS_ANA_REG18_ENADCBIAS_DEFAULT_VAL & SYS_ANA_REG18_ENADCBIAS_MASK) << SYS_ANA_REG18_ENADCBIAS_POS);
	value |= ((SYS_ANA_REG18_ENMICBIAS_DEFAULT_VAL & SYS_ANA_REG18_ENMICBIAS_MASK) << SYS_ANA_REG18_ENMICBIAS_POS);
	value |= ((SYS_ANA_REG18_ADCCKINVEN1V_DEFAULT_VAL & SYS_ANA_REG18_ADCCKINVEN1V_MASK) << SYS_ANA_REG18_ADCCKINVEN1V_POS);
	value |= ((SYS_ANA_REG18_DACFB2ST0V9_DEFAULT_VAL & SYS_ANA_REG18_DACFB2ST0V9_MASK) << SYS_ANA_REG18_DACFB2ST0V9_POS);
	value |= ((SYS_ANA_REG18_NC1_DEFAULT_VAL & SYS_ANA_REG18_NC1_MASK) << SYS_ANA_REG18_NC1_POS);
	value |= ((SYS_ANA_REG18_MICBIAS_TRM_DEFAULT_VAL & SYS_ANA_REG18_MICBIAS_TRM_MASK) << SYS_ANA_REG18_MICBIAS_TRM_POS);
	value |= ((SYS_ANA_REG18_MICBIAS_VOC_DEFAULT_VAL & SYS_ANA_REG18_MICBIAS_VOC_MASK) << SYS_ANA_REG18_MICBIAS_VOC_POS);
	value |= ((SYS_ANA_REG18_VREFSEL1V_DEFAULT_VAL & SYS_ANA_REG18_VREFSEL1V_MASK) << SYS_ANA_REG18_VREFSEL1V_POS);
	value |= ((SYS_ANA_REG18_CAPSWSPI_DEFAULT_VAL & SYS_ANA_REG18_CAPSWSPI_MASK) << SYS_ANA_REG18_CAPSWSPI_POS);
	value |= ((SYS_ANA_REG18_ADREF_SEL_DEFAULT_VAL & SYS_ANA_REG18_ADREF_SEL_MASK) << SYS_ANA_REG18_ADREF_SEL_POS);
	value |= ((SYS_ANA_REG18_NC0_DEFAULT_VAL & SYS_ANA_REG18_NC0_MASK) << SYS_ANA_REG18_NC0_POS);
	value |= ((SYS_ANA_REG18_RESERVED_BIT_26_30_DEFAULT_VAL & SYS_ANA_REG18_RESERVED_BIT_26_30_MASK) << SYS_ANA_REG18_RESERVED_BIT_26_30_POS);
	value |= ((SYS_ANA_REG18_SPI_DACCKPSSEL_DEFAULT_VAL & SYS_ANA_REG18_SPI_DACCKPSSEL_MASK) << SYS_ANA_REG18_SPI_DACCKPSSEL_POS);

	return value;
}

static uint32_t ana_reg19_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG19_ISEL_DEFAULT_VAL & SYS_ANA_REG19_ISEL_MASK) << SYS_ANA_REG19_ISEL_POS);
	value |= ((SYS_ANA_REG19_MICIRSEL1_DEFAULT_VAL & SYS_ANA_REG19_MICIRSEL1_MASK) << SYS_ANA_REG19_MICIRSEL1_POS);
	value |= ((SYS_ANA_REG19_MICDACIT_DEFAULT_VAL & SYS_ANA_REG19_MICDACIT_MASK) << SYS_ANA_REG19_MICDACIT_POS);
	value |= ((SYS_ANA_REG19_MICDACIH_DEFAULT_VAL & SYS_ANA_REG19_MICDACIH_MASK) << SYS_ANA_REG19_MICDACIH_POS);
	value |= ((SYS_ANA_REG19_MICSINGLEEN_DEFAULT_VAL & SYS_ANA_REG19_MICSINGLEEN_MASK) << SYS_ANA_REG19_MICSINGLEEN_POS);
	value |= ((SYS_ANA_REG19_DCCOMPEN_DEFAULT_VAL & SYS_ANA_REG19_DCCOMPEN_MASK) << SYS_ANA_REG19_DCCOMPEN_POS);
	value |= ((SYS_ANA_REG19_MICGAIN_DEFAULT_VAL & SYS_ANA_REG19_MICGAIN_MASK) << SYS_ANA_REG19_MICGAIN_POS);
	value |= ((SYS_ANA_REG19_MICDACEN_DEFAULT_VAL & SYS_ANA_REG19_MICDACEN_MASK) << SYS_ANA_REG19_MICDACEN_POS);
	value |= ((SYS_ANA_REG19_STG2LSEN1V_DEFAULT_VAL & SYS_ANA_REG19_STG2LSEN1V_MASK) << SYS_ANA_REG19_STG2LSEN1V_POS);
	value |= ((SYS_ANA_REG19_OPENLOOPCAL1V_DEFAULT_VAL & SYS_ANA_REG19_OPENLOOPCAL1V_MASK) << SYS_ANA_REG19_OPENLOOPCAL1V_POS);
	value |= ((SYS_ANA_REG19_CALLATCH_DEFAULT_VAL & SYS_ANA_REG19_CALLATCH_MASK) << SYS_ANA_REG19_CALLATCH_POS);
	value |= ((SYS_ANA_REG19_VCMSEL_DEFAULT_VAL & SYS_ANA_REG19_VCMSEL_MASK) << SYS_ANA_REG19_VCMSEL_POS);
	value |= ((SYS_ANA_REG19_DWAMODE_DEFAULT_VAL & SYS_ANA_REG19_DWAMODE_MASK) << SYS_ANA_REG19_DWAMODE_POS);
	value |= ((SYS_ANA_REG19_R2REN_DEFAULT_VAL & SYS_ANA_REG19_R2REN_MASK) << SYS_ANA_REG19_R2REN_POS);
	value |= ((SYS_ANA_REG19_NC_26_27_DEFAULT_VAL & SYS_ANA_REG19_NC_26_27_MASK) << SYS_ANA_REG19_NC_26_27_POS);
	value |= ((SYS_ANA_REG19_MICEN_DEFAULT_VAL & SYS_ANA_REG19_MICEN_MASK) << SYS_ANA_REG19_MICEN_POS);
	value |= ((SYS_ANA_REG19_RST_DEFAULT_VAL & SYS_ANA_REG19_RST_MASK) << SYS_ANA_REG19_RST_POS);
	value |= ((SYS_ANA_REG19_BPDWA1V_DEFAULT_VAL & SYS_ANA_REG19_BPDWA1V_MASK) << SYS_ANA_REG19_BPDWA1V_POS);
	value |= ((SYS_ANA_REG19_HCEN1STG_DEFAULT_VAL & SYS_ANA_REG19_HCEN1STG_MASK) << SYS_ANA_REG19_HCEN1STG_POS);

	return value;
}

static uint32_t ana_reg20_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG20_HPDAC_DEFAULT_VAL & SYS_ANA_REG20_HPDAC_MASK) << SYS_ANA_REG20_HPDAC_POS);
	value |= ((SYS_ANA_REG20_CALCON_SEL_DEFAULT_VAL & SYS_ANA_REG20_CALCON_SEL_MASK) << SYS_ANA_REG20_CALCON_SEL_POS);
	value |= ((SYS_ANA_REG20_OSCDAC_DEFAULT_VAL & SYS_ANA_REG20_OSCDAC_MASK) << SYS_ANA_REG20_OSCDAC_POS);
	value |= ((SYS_ANA_REG20_OCENDAC_DEFAULT_VAL & SYS_ANA_REG20_OCENDAC_MASK) << SYS_ANA_REG20_OCENDAC_POS);
	value |= ((SYS_ANA_REG20_VCMSEL_DEFAULT_VAL & SYS_ANA_REG20_VCMSEL_MASK) << SYS_ANA_REG20_VCMSEL_POS);
	value |= ((SYS_ANA_REG20_ADJDACREF_DEFAULT_VAL & SYS_ANA_REG20_ADJDACREF_MASK) << SYS_ANA_REG20_ADJDACREF_POS);
	value |= ((SYS_ANA_REG20_DCOCHG_DEFAULT_VAL & SYS_ANA_REG20_DCOCHG_MASK) << SYS_ANA_REG20_DCOCHG_POS);
	value |= ((SYS_ANA_REG20_DIFFEN_DEFAULT_VAL & SYS_ANA_REG20_DIFFEN_MASK) << SYS_ANA_REG20_DIFFEN_POS);
	value |= ((SYS_ANA_REG20_ENDACCAL_DEFAULT_VAL & SYS_ANA_REG20_ENDACCAL_MASK) << SYS_ANA_REG20_ENDACCAL_POS);
	value |= ((SYS_ANA_REG20_NC2_DEFAULT_VAL & SYS_ANA_REG20_NC2_MASK) << SYS_ANA_REG20_NC2_POS);
	value |= ((SYS_ANA_REG20_LENDCOC_DEFAULT_VAL & SYS_ANA_REG20_LENDCOC_MASK) << SYS_ANA_REG20_LENDCOC_POS);
	value |= ((SYS_ANA_REG20_NC1_DEFAULT_VAL & SYS_ANA_REG20_NC1_MASK) << SYS_ANA_REG20_NC1_POS);
	value |= ((SYS_ANA_REG20_LENVCMD_DEFAULT_VAL & SYS_ANA_REG20_LENVCMD_MASK) << SYS_ANA_REG20_LENVCMD_POS);
	value |= ((SYS_ANA_REG20_DACDRVEN_DEFAULT_VAL & SYS_ANA_REG20_DACDRVEN_MASK) << SYS_ANA_REG20_DACDRVEN_POS);
	value |= ((SYS_ANA_REG20_NC0_DEFAULT_VAL & SYS_ANA_REG20_NC0_MASK) << SYS_ANA_REG20_NC0_POS);
	value |= ((SYS_ANA_REG20_DACLEN_DEFAULT_VAL & SYS_ANA_REG20_DACLEN_MASK) << SYS_ANA_REG20_DACLEN_POS);
	value |= ((SYS_ANA_REG20_DACG_DEFAULT_VAL & SYS_ANA_REG20_DACG_MASK) << SYS_ANA_REG20_DACG_POS);
	value |= ((SYS_ANA_REG20_DACMUTE_DEFAULT_VAL & SYS_ANA_REG20_DACMUTE_MASK) << SYS_ANA_REG20_DACMUTE_POS);
	value |= ((SYS_ANA_REG20_DACDWAMODE_SEL_DEFAULT_VAL & SYS_ANA_REG20_DACDWAMODE_SEL_MASK) << SYS_ANA_REG20_DACDWAMODE_SEL_POS);
	value |= ((SYS_ANA_REG20_DACSEL_DEFAULT_VAL & SYS_ANA_REG20_DACSEL_MASK) << SYS_ANA_REG20_DACSEL_POS);

	return value;
}

static uint32_t ana_reg21_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG21_LMDCIN_DEFAULT_VAL & SYS_ANA_REG21_LMDCIN_MASK) << SYS_ANA_REG21_LMDCIN_POS);
	value |= ((SYS_ANA_REG21_NC1_DEFAULT_VAL & SYS_ANA_REG21_NC1_MASK) << SYS_ANA_REG21_NC1_POS);
	value |= ((SYS_ANA_REG21_SPIRST_OVC_DEFAULT_VAL & SYS_ANA_REG21_SPIRST_OVC_MASK) << SYS_ANA_REG21_SPIRST_OVC_POS);
	value |= ((SYS_ANA_REG21_NC0_DEFAULT_VAL & SYS_ANA_REG21_NC0_MASK) << SYS_ANA_REG21_NC0_POS);
	value |= ((SYS_ANA_REG21_ENIDACL_DEFAULT_VAL & SYS_ANA_REG21_ENIDACL_MASK) << SYS_ANA_REG21_ENIDACL_POS);
	value |= ((SYS_ANA_REG21_DAC3RDHC0V9_DEFAULT_VAL & SYS_ANA_REG21_DAC3RDHC0V9_MASK) << SYS_ANA_REG21_DAC3RDHC0V9_POS);
	value |= ((SYS_ANA_REG21_HC2S_DEFAULT_VAL & SYS_ANA_REG21_HC2S_MASK) << SYS_ANA_REG21_HC2S_POS);
	value |= ((SYS_ANA_REG21_RFB_CTRL_DEFAULT_VAL & SYS_ANA_REG21_RFB_CTRL_MASK) << SYS_ANA_REG21_RFB_CTRL_POS);
	value |= ((SYS_ANA_REG21_VCMSEL_DEFAULT_VAL & SYS_ANA_REG21_VCMSEL_MASK) << SYS_ANA_REG21_VCMSEL_POS);
	value |= ((SYS_ANA_REG21_ENBS_DEFAULT_VAL & SYS_ANA_REG21_ENBS_MASK) << SYS_ANA_REG21_ENBS_POS);
	value |= ((SYS_ANA_REG21_CALCK_SEL0V9_DEFAULT_VAL & SYS_ANA_REG21_CALCK_SEL0V9_MASK) << SYS_ANA_REG21_CALCK_SEL0V9_POS);
	value |= ((SYS_ANA_REG21_BPDWA0V9_DEFAULT_VAL & SYS_ANA_REG21_BPDWA0V9_MASK) << SYS_ANA_REG21_BPDWA0V9_POS);
	value |= ((SYS_ANA_REG21_LOOPRST0V9_DEFAULT_VAL & SYS_ANA_REG21_LOOPRST0V9_MASK) << SYS_ANA_REG21_LOOPRST0V9_POS);
	value |= ((SYS_ANA_REG21_OCT0V9_DEFAULT_VAL & SYS_ANA_REG21_OCT0V9_MASK) << SYS_ANA_REG21_OCT0V9_POS);
	value |= ((SYS_ANA_REG21_SOUT0V9_DEFAULT_VAL & SYS_ANA_REG21_SOUT0V9_MASK) << SYS_ANA_REG21_SOUT0V9_POS);
	value |= ((SYS_ANA_REG21_HC0V9_DEFAULT_VAL & SYS_ANA_REG21_HC0V9_MASK) << SYS_ANA_REG21_HC0V9_POS);

	return value;
}

bk_err_t aud_hal_adc_hpf_config(aud_adc_hpf_config_t *config)
{
	aud_hal_set_adc_config1_adc_hpf2_coef_b0(config->adc_hpf2_coef_B0);
	aud_hal_set_adc_config1_adc_hpf2_coef_b1(config->adc_hpf2_coef_B1);
	aud_hal_set_adc_config0_adc_hpf2_coef_b2(config->adc_hpf2_coef_B2);

	aud_hal_set_adc_config2_adc_hpf2_coef_a0(config->adc_hpf2_coef_A0);
	aud_hal_set_adc_config2_adc_hpf2_coef_a1(config->adc_hpf2_coef_A1);

	aud_hal_set_adc_config0_adc_hpf2_bypass(config->adc_hpf2_bypass_enable);
	aud_hal_set_adc_config0_adc_hpf1_bypass(config->adc_hpf1_bypass_enable);

	return BK_OK;
}

bk_err_t aud_hal_adc_agc_config(aud_adc_agc_config_t *config)
{
	aud_hal_set_agc_config0_agc_noise_thrd(config->agc_noise_thrd);
	aud_hal_set_agc_config0_agc_noise_high(config->agc_noise_high);
	aud_hal_set_agc_config0_agc_noise_low(config->agc_noise_low);

	aud_hal_set_agc_config1_agc_noise_min(config->agc_noise_min);
	aud_hal_set_agc_config1_agc_noise_tout(config->agc_noise_tout);
	aud_hal_set_agc_config1_agc_high_dur(config->agc_high_dur);
	aud_hal_set_agc_config1_agc_low_dur(config->agc_low_dur);
	aud_hal_set_agc_config1_agc_min(config->agc_min);
	aud_hal_set_agc_config1_agc_max(config->agc_max);
	aud_hal_set_agc_config1_agc_ng_method(config->agc_ng_method);
	aud_hal_set_agc_config1_agc_ng_enable(config->agc_ng_enable);

	aud_hal_set_agc_config2_agc_decay_time(config->agc_decay_time);
	aud_hal_set_agc_config2_agc_attack_time(config->agc_attack_time);
	aud_hal_set_agc_config2_agc_high_thrd(config->agc_high_thrd);
	aud_hal_set_agc_config2_agc_low_thrd(config->agc_low_thrd);
	aud_hal_set_agc_config2_agc_iir_coef(config->agc_iir_coef);
	aud_hal_set_agc_config2_agc_enable(config->agc_enable);
	aud_hal_set_agc_config2_manual_pga_value(config->manual_pga_value);
	aud_hal_set_agc_config2_manual_pga(config->manual_pga_enable);

	return BK_OK;
}


bk_err_t aud_hal_dac_hpf_config(aud_dac_hpf_config_t *config)
{
	aud_hal_set_dac_config1_dac_hpf2_coef_b0(config->dac_hpf2_coef_B0);
	aud_hal_set_dac_config1_dac_hpf2_coef_b1(config->dac_hpf2_coef_B1);
	aud_hal_set_dac_config0_dac_hpf2_coef_b2(config->dac_hpf2_coef_B2);

	aud_hal_set_dac_config2_dac_hpf2_coef_a1(config->dac_hpf2_coef_B0);
	aud_hal_set_dac_config2_dac_hpf2_coef_a2(config->dac_hpf2_coef_B0);

	if (config->dac_hpf2_bypass_enable == AUD_DAC_HPF_BYPASS_ENABLE) {
		aud_hal_set_dac_config0_dac_hpf2_bypass(1);
	} else {
		aud_hal_set_dac_config0_dac_hpf2_bypass(0);
	}

	if (config->dac_hpf1_bypass_enable == AUD_DAC_HPF_BYPASS_ENABLE) {
		aud_hal_set_dac_config0_dac_hpf1_bypass(1);
	} else {
		aud_hal_set_dac_config0_dac_hpf1_bypass(0);
	}

	return BK_OK;
}

bk_err_t aud_hal_dac_filt_config(aud_dac_eq_config_t *config)
{
	aud_hal_set_flt0_coef_a1a2_flt0_a1((config->flt0_A1 >> 6) & 0xFFFF);
	aud_hal_set_flt0_ext_coef_flt0_a1_l6bit((config->flt0_A1) & 0x3F);
	aud_hal_set_flt0_coef_a1a2_flt0_a2((config->flt0_A2 >> 6) & 0xFFFF);
	aud_hal_set_flt0_ext_coef_flt0_a2_l6bit((config->flt0_A2) & 0x3F);
	aud_hal_set_flt0_coef_b0b1_flt0_b0((config->flt0_B0 >> 6) & 0xFFFF);
	aud_hal_set_flt0_ext_coef_flt0_b0_l6bit((config->flt0_B0) & 0x3F);
	aud_hal_set_flt0_coef_b0b1_flt0_b1((config->flt0_B1 >> 6) & 0xFFFF);
	aud_hal_set_flt0_ext_coef_flt0_b1_l6bit((config->flt0_B1) & 0x3F);
	aud_hal_set_flt0_coef_b2_flt0_b2((config->flt0_B2 >> 6) & 0xFFFF);
	aud_hal_set_flt0_ext_coef_flt0_b2_l6bit((config->flt0_B2) & 0x3F);

	aud_hal_set_flt1_coef_a1a2_flt1_a1((config->flt1_A1 >> 6) & 0xFFFF);
	aud_hal_set_flt1_ext_coef_flt1_a1_l6bit((config->flt1_A1) & 0x3F);
	aud_hal_set_flt1_coef_a1a2_flt1_a2((config->flt1_A2 >> 6) & 0xFFFF);
	aud_hal_set_flt1_ext_coef_flt1_a2_l6bit((config->flt1_A2) & 0x3F);
	aud_hal_set_flt1_coef_b0b1_flt1_b0((config->flt1_B0 >> 6) & 0xFFFF);
	aud_hal_set_flt1_ext_coef_flt1_b0_l6bit((config->flt1_B0) & 0x3F);
	aud_hal_set_flt1_coef_b0b1_flt1_b1((config->flt1_B1 >> 6) & 0xFFFF);
	aud_hal_set_flt1_ext_coef_flt1_b1_l6bit((config->flt1_B1) & 0x3F);
	aud_hal_set_flt1_coef_b2_flt1_b2((config->flt1_B2 >> 6) & 0xFFFF);
	aud_hal_set_flt1_ext_coef_flt1_b2_l6bit((config->flt1_B2) & 0x3F);

	aud_hal_set_flt2_coef_a1a2_flt2_a1((config->flt2_A1 >> 6) & 0xFFFF);
	aud_hal_set_flt2_ext_coef_flt2_a1_l6bit((config->flt2_A1) & 0x3F);
	aud_hal_set_flt2_coef_a1a2_flt2_a2((config->flt2_A2 >> 6) & 0xFFFF);
	aud_hal_set_flt2_ext_coef_flt2_a2_l6bit((config->flt2_A2) & 0x3F);
	aud_hal_set_flt2_coef_b0b1_flt2_b0((config->flt2_B0 >> 6) & 0xFFFF);
	aud_hal_set_flt2_ext_coef_flt2_b0_l6bit((config->flt2_B0) & 0x3F);
	aud_hal_set_flt2_coef_b0b1_flt2_b1((config->flt2_B1 >> 6) & 0xFFFF);
	aud_hal_set_flt2_ext_coef_flt2_b1_l6bit((config->flt2_B1) & 0x3F);
	aud_hal_set_flt2_coef_b2_flt2_b2((config->flt2_B2 >> 6) & 0xFFFF);
	aud_hal_set_flt2_ext_coef_flt2_b2_l6bit((config->flt2_B2) & 0x3F);

	aud_hal_set_flt3_coef_a1a2_flt3_a1((config->flt3_A1 >> 6) & 0xFFFF);
	aud_hal_set_flt3_ext_coef_flt3_a1_l6bit((config->flt3_A1) & 0x3F);
	aud_hal_set_flt3_coef_a1a2_flt3_a2((config->flt3_A2 >> 6) & 0xFFFF);
	aud_hal_set_flt3_ext_coef_flt3_a2_l6bit((config->flt3_A2) & 0x3F);
	aud_hal_set_flt3_coef_b0b1_flt3_b0((config->flt3_B0 >> 6) & 0xFFFF);
	aud_hal_set_flt3_ext_coef_flt3_b0_l6bit((config->flt3_B0) & 0x3F);
	aud_hal_set_flt3_coef_b0b1_flt3_b1((config->flt3_B1 >> 6) & 0xFFFF);
	aud_hal_set_flt3_ext_coef_flt3_b1_l6bit((config->flt3_B1) & 0x3F);
	aud_hal_set_flt3_coef_b2_flt3_b2((config->flt3_B2 >> 6) & 0xFFFF);
	aud_hal_set_flt3_ext_coef_flt3_b2_l6bit((config->flt3_B2) & 0x3F);

	return BK_OK;
}

bk_err_t aud_hal_dtmf_config(aud_dtmf_config_t *config)
{
	/* dtmf_config0 */
	aud_hal_set_dtmf_config0_tone_pattern(config->tone_pattern);
	aud_hal_set_dtmf_config0_tone_mode(config->tone_mode);
	aud_hal_set_dtmf_config0_tone_pause_time(config->tone_pause_time);
	aud_hal_set_dtmf_config0_tone_active_time(config->tone_active_time);

	/* dtmf_config1 */
	aud_hal_set_dtmf_config1_tone1_step(config->tone1_step);
	aud_hal_set_dtmf_config1_tone1_attu(config->tone1_attu);

	/* dtmf_config2 */
	aud_hal_set_dtmf_config2_tone2_step(config->tone2_step);
	aud_hal_set_dtmf_config2_tone2_attu(config->tone2_attu);

	return BK_OK;
}

/* get adc fifo port address */
bk_err_t aud_hal_adc_get_fifo_addr(uint32_t *adc_fifo_addr)
{
	//*adc_fifo_addr = aud_ll_get_adc_fifo_addr();
	*adc_fifo_addr = AUD_ADC_FPORT_ADDR;
	return BK_OK;
}

/* get dac fifo port address */
bk_err_t aud_hal_dac_get_fifo_addr(uint32_t *dac_fifo_addr)
{
	//*dac_fifo_addr = aud_ll_get_dac_fifo_addr();
	*dac_fifo_addr = AUD_DAC_FPORT_ADDR;
	return BK_OK;
}

/* get dtmf fifo port address */
bk_err_t aud_hal_dtmf_get_fifo_addr(uint32_t *dtmf_fifo_addr)
{
	//*dtmf_fifo_addr = aud_ll_get_dtmf_fifo_addr();
	*dtmf_fifo_addr = AUD_DTMF_FPORT_ADDR;
	return BK_OK;
}

/* get dmic fifo port address */
bk_err_t aud_hal_dmic_get_fifo_addr(uint32_t *dmic_fifo_addr)
{
	//*dmic_fifo_addr = aud_ll_get_dmic_fifo_addr();
	*dmic_fifo_addr = AUD_DMIC_FPORT_ADDR;
	return BK_OK;
}

extern void delay(int num);
bk_err_t aud_hal_clk_config(aud_clk_t clk)
{
	if (clk == AUD_CLK_APLL) {
		sys_drv_aud_select_clock(1);
		//set apll clock config
		sys_drv_apll_en(1);
		sys_drv_apll_cal_val_set(0x973CA70);   //(0x8973CA6F);
		sys_drv_apll_config_set(0xC2A06A86);   //(0xC2A0AE86);
		sys_drv_apll_spi_trigger_set(1);
		delay(10);
		sys_drv_apll_spi_trigger_set(0);
		/* selet apll */
		aud_hal_set_audio_config_apll_sel(1);
	} else {
		sys_drv_aud_select_clock(0);
		/* selet xtal */
		aud_hal_set_audio_config_apll_sel(0);
	}

	return BK_OK;
}

void aud_hal_init(void)
{
	aud_hal_set_clk_control_soft_reset(1);
	sys_hal_set_ana_reg18_value(ana_reg18_value_cal());
	sys_hal_set_ana_reg19_value(ana_reg19_value_cal());
	sys_hal_set_ana_reg20_value(ana_reg20_value_cal());
	sys_hal_set_ana_reg21_value(ana_reg21_value_cal()); //mic1
	sys_hal_set_ana_reg27_value(0x91800006); //mic2
	sys_drv_aud_audbias_en(1);
}

void aud_hal_deinit(void)
{
	sys_hal_set_ana_reg18_value(0);
	sys_hal_set_ana_reg19_value(0);
	sys_hal_set_ana_reg20_value(0);
	sys_hal_set_ana_reg21_value(0);
	sys_drv_aud_audbias_en(0);
	aud_hal_set_clk_control_soft_reset(0);
	aud_hal_set_clk_control_soft_reset(1);
	aud_hal_set_clk_control_soft_reset(0);
}
