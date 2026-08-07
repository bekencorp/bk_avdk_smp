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



#include <common/bk_include.h>
#include "aud_hal.h"
#include "sys_driver.h"
#include "clock_driver.h"
#include <os/os.h>
#include <os/mem.h>
#include <driver/int.h>
#include <modules/pm.h>
#include "sys_hal.h"
#include "cpu_id.h"

#include "aud_hal.h"

#include <driver/aud_common.h>

#include <driver/aud_adc.h>
#include <driver/aud_adc_types.h>
#include <driver/aud_dtmf.h>
#include <driver/aud_dtmf_types.h>
#include <driver/aud_dmic.h>
#include <driver/aud_dmic_types.h>
#include <driver/aud_dac.h>
#include <driver/aud_dac_types.h>
#include <timer/timer_driver.h>


#if CONFIG_SOC_BK7259
#define SYS_ANA_REG20_ISELAUD_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG20_AUDCK_RLCEN_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG20_LCHCKINVEN_DEFAULT_VAL                    (0x01)
#define SYS_ANA_REG20_ENAUDBIAS_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG20_ENADCBIAS_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG20_ENMICBIAS_DEFAULT_VAL                     (0x00)
#define SYS_ANA_REG20_ADCCKINVEN_DEFAULT_VAL                    (0x01)
#define SYS_ANA_REG20_SPI0_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG20_ADCTSTEN_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG20_MICBIAS_TRM_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG20_MICBIAS_VOC_DEFAULT_VAL                   (0x10)
#define SYS_ANA_REG20_VREFSEL_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG20_CAPSW_DEFAULT_VAL                         (0x1f)
#define SYS_ANA_REG20_ADCREF_SEL_DEFAULT_VAL                    (0x02)
#define SYS_ANA_REG20_ADCVCMSEL_DEFAULT_VAL                     (0x01)
#define SYS_ANA_REG20_SPI1_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG20_AUDADJREF_DEFAULT_VAL                     (0x10)

#define SYS_ANA_REG21_ISEL_MIC1_DEFAULT_VAL                     (0x03)
#define SYS_ANA_REG21_MICIRSEL1_MIC1_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG21_VCMSEL_MIC1_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG21_ENFSR_MIC1_DEFAULT_VAL                    (0x01)
#define SYS_ANA_REG21_ENOPOCLIP_MIC1_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG21_DA2ADEN_MIC1_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG21_IMATCH_MIC1_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG21_IMATCH_EN_MIC1_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG21_DCCOMPEN_MIC1_DEFAULT_VAL                 (0x00)
#define SYS_ANA_REG21_MICSINGLEEN_MIC1_DEFAULT_VAL              (0x00)
#define SYS_ANA_REG21_NC_14_DEFAULT_VAL                         (0x00)
#define SYS_ANA_REG21_MICGAIN_MIC1_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG21_NC_19_23_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG21_DWAMODE_MIC1_DEFAULT_VAL                  (0x01)
#define SYS_ANA_REG21_NC_25_27_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG21_MICEN_MIC1_DEFAULT_VAL                    (0x00)
#define SYS_ANA_REG21_RST_MIC1_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG21_BPDWA1V_MIC1_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG21_HCEN1STG_MIC1_DEFAULT_VAL                 (0x00)

#define SYS_ANA_REG27_ISEL_MIC2_DEFAULT_VAL                     (0x03)
#define SYS_ANA_REG27_MICIRSEL1_MIC2_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG27_VCMSEL_MIC2_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG27_ENFSR_MIC2_DEFAULT_VAL                    (0x01)
#define SYS_ANA_REG27_ENOPOCLIP_MIC2_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG27_DA2ADEN_MIC2_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG27_IMATCH_MIC2_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG27_IMATCH_EN_MIC2_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG27_DCCOMPEN_MIC2_DEFAULT_VAL                 (0x00)
#define SYS_ANA_REG27_MICSINGLEEN_MIC2_DEFAULT_VAL              (0x00)
#define SYS_ANA_REG27_NC_14_DEFAULT_VAL                         (0x00)
#define SYS_ANA_REG27_MICGAIN_MIC2_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG27_NC_19_23_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG27_DWAMODE_MIC2_DEFAULT_VAL                  (0x01)
#define SYS_ANA_REG27_NC_25_27_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG27_MICEN_MIC2_DEFAULT_VAL                    (0x00)
#define SYS_ANA_REG27_RST_MIC2_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG27_BPDWA1V_MIC2_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG27_HCEN1STG_MIC2_DEFAULT_VAL                 (0x00)

#define SYS_ANA_REG28_ISEL_MIC3_DEFAULT_VAL                     (0x03)
#define SYS_ANA_REG28_MICIRSEL1_MIC3_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG28_VCMSEL_MIC3_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG28_ENFSR_MIC3_DEFAULT_VAL                    (0x01)
#define SYS_ANA_REG28_ENOPOCLIP_MIC3_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG28_DA2ADEN_MIC3_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG28_IMATCH_MIC3_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG28_IMATCH_EN_MIC3_DEFAULT_VAL                (0x00)
#define SYS_ANA_REG28_DCCOMPEN_MIC3_DEFAULT_VAL                 (0x00)
#define SYS_ANA_REG28_MICSINGLEEN_MIC3_DEFAULT_VAL              (0x00)
#define SYS_ANA_REG28_NC_14_DEFAULT_VAL                         (0x00)
#define SYS_ANA_REG28_MICGAIN_MIC3_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG28_NC_19_23_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG28_DWAMODE_MIC3_DEFAULT_VAL                  (0x01)
#define SYS_ANA_REG28_NC_25_27_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG28_MICEN_MIC3_DEFAULT_VAL                    (0x00)
#define SYS_ANA_REG28_RST_MIC3_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG28_BPDWA1V_MIC3_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG28_HCEN1STG_MIC3_DEFAULT_VAL                 (0x00)

#define SYS_ANA_REG29_HPDAC_DEFAULT_VAL                         (0x01)
#define SYS_ANA_REG29_ISELSTG_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG29_OSCDAC_DEFAULT_VAL                        (0x00)
#define SYS_ANA_REG29_OCENDAC_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG29_VSELDCO_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG29_SRSEL_DEFAULT_VAL                         (0x00)
#define SYS_ANA_REG29_HPOEN_DEFAULT_VAL                         (0x00)
#define SYS_ANA_REG29_LBWEN_DEFAULT_VAL                         (0x01)
#define SYS_ANA_REG29_CALSEL_DEFAULT_VAL                        (0x01)
#define SYS_ANA_REG29_BP2VLDO_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG29_DCOCHG_DEFAULT_VAL                        (0x00)
#define SYS_ANA_REG29_DIFFEN_DEFAULT_VAL                        (0x01)
#define SYS_ANA_REG29_ENDACCAL_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG29_RENDCOC_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG29_LENDCOC_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG29_RENVCMD_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG29_LENVCMD_DEFAULT_VAL                       (0x01)
#define SYS_ANA_REG29_DACDRVEN_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG29_DACREN_DEFAULT_VAL                        (0x00)
#define SYS_ANA_REG29_DACLEN_DEFAULT_VAL                        (0x00)
#define SYS_ANA_REG29_DACG_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG29_DACMUTE_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG29_DACDWAMODE_SEL_DEFAULT_VAL                (0x01)
#define SYS_ANA_REG29_CKPSEL_DEFAULT_VAL                        (0x00)
#define SYS_ANA_REG29_NC_29_31_DEFAULT_VAL                      (0x04)

#define SYS_ANA_REG30_LMDCIN_DEFAULT_VAL                        (0x80)
#define SYS_ANA_REG30_RMDCIN_DEFAULT_VAL                        (0x80)
#define SYS_ANA_REG30_SPIRST_OVC_DEFAULT_VAL                    (0x00)
#define SYS_ANA_REG30_ENIDACR_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG30_ENIDACL_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG30_DAC3RDHC0V9_DEFAULT_VAL                   (0x00)
#define SYS_ANA_REG30_HC2S_DEFAULT_VAL                          (0x01)
#define SYS_ANA_REG30_SNG_FB_EN_DEFAULT_VAL                     (0x01)
#define SYS_ANA_REG30_RFB_CTRL_DEFAULT_VAL                      (0x01)
#define SYS_ANA_REG30_ENBS_DEFAULT_VAL                          (0x00)
#define SYS_ANA_REG30_CALCK_SEL0V9_DEFAULT_VAL                  (0x00)
#define SYS_ANA_REG30_BPDWA0V9_DEFAULT_VAL                      (0x00)
#define SYS_ANA_REG30_LOOPRST0V9_DEFAULT_VAL                    (0x00)
#define SYS_ANA_REG30_OCT0V9_DEFAULT_VAL                        (0x00)
#define SYS_ANA_REG30_SOUT0V9_DEFAULT_VAL                       (0x00)
#define SYS_ANA_REG30_HC0V9_DEFAULT_VAL                         (0x02)
#endif

#define AUD_RETURN_ON_NOT_INIT() do {\
		if (!s_aud_driver_is_init) {\
			return BK_ERR_AUD_NOT_INIT;\
		}\
	} while(0)

static bool s_aud_driver_is_init = false;
static aud_module_init_sta_t s_aud_module_init_sta = {0};
#if (CONFIG_AUDIO_ADC || CONFIG_AUDIO_DMIC || CONFIG_AUDIO_DTMF || CONFIG_AUDIO_DAC)
static aud_isr_handle_t s_aud_isr = {NULL};
#endif
static void aud_isr(void);
extern void delay(int num);


#if CONFIG_SOC_BK7259

static uint32_t ana_reg20_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG20_ISELAUD_DEFAULT_VAL & SYS_ANA_REG20_ISELAUD_MASK) << SYS_ANA_REG20_ISELAUD_POS);
	value |= ((SYS_ANA_REG20_AUDCK_RLCEN_DEFAULT_VAL & SYS_ANA_REG20_AUDCK_RLCEN_MASK) << SYS_ANA_REG20_AUDCK_RLCEN_POS);
	value |= ((SYS_ANA_REG20_LCHCKINVEN_DEFAULT_VAL & SYS_ANA_REG20_LCHCKINVEN_MASK) << SYS_ANA_REG20_LCHCKINVEN_POS);
	value |= ((SYS_ANA_REG20_ENAUDBIAS_DEFAULT_VAL & SYS_ANA_REG20_ENAUDBIAS_MASK) << SYS_ANA_REG20_ENAUDBIAS_POS);
	value |= ((SYS_ANA_REG20_ENADCBIAS_DEFAULT_VAL & SYS_ANA_REG20_ENADCBIAS_MASK) << SYS_ANA_REG20_ENADCBIAS_POS);
	value |= ((SYS_ANA_REG20_ENMICBIAS_DEFAULT_VAL & SYS_ANA_REG20_ENMICBIAS_MASK) << SYS_ANA_REG20_ENMICBIAS_POS);
	value |= ((SYS_ANA_REG20_ADCCKINVEN_DEFAULT_VAL & SYS_ANA_REG20_ADCCKINVEN_MASK) << SYS_ANA_REG20_ADCCKINVEN_POS);
	value |= ((SYS_ANA_REG20_SPI0_DEFAULT_VAL & SYS_ANA_REG20_SPI_MASK) << SYS_ANA_REG20_SPI_POS);
	value |= ((SYS_ANA_REG20_ADCTSTEN_DEFAULT_VAL & SYS_ANA_REG20_ADCTSTEN_MASK) << SYS_ANA_REG20_ADCTSTEN_POS);
	value |= ((SYS_ANA_REG20_MICBIAS_TRM_DEFAULT_VAL & SYS_ANA_REG20_MICBIAS_TRM_MASK) << SYS_ANA_REG20_MICBIAS_TRM_POS);
	value |= ((SYS_ANA_REG20_MICBIAS_VOC_DEFAULT_VAL & SYS_ANA_REG20_MICBIAS_VOC_MASK) << SYS_ANA_REG20_MICBIAS_VOC_POS);
	value |= ((SYS_ANA_REG20_VREFSEL_DEFAULT_VAL & SYS_ANA_REG20_VREFSEL_MASK) << SYS_ANA_REG20_VREFSEL_POS);
	value |= ((SYS_ANA_REG20_CAPSW_DEFAULT_VAL & SYS_ANA_REG20_CAPSW_MASK) << SYS_ANA_REG20_CAPSW_POS);
	value |= ((SYS_ANA_REG20_ADCREF_SEL_DEFAULT_VAL & SYS_ANA_REG20_ADCREF_SEL_MASK) << SYS_ANA_REG20_ADCREF_SEL_POS);
	value |= ((SYS_ANA_REG20_ADCVCMSEL_DEFAULT_VAL & SYS_ANA_REG20_ADCVCMSEL_MASK) << SYS_ANA_REG20_ADCVCMSEL_POS);
	value |= ((SYS_ANA_REG20_SPI1_DEFAULT_VAL & SYS_ANA_REG20_SPI_1_MASK) << SYS_ANA_REG20_SPI_1_POS);
	value |= ((SYS_ANA_REG20_AUDADJREF_DEFAULT_VAL & SYS_ANA_REG20_AUDADJREF_MASK) << SYS_ANA_REG20_AUDADJREF_POS);

	return value;
}

static uint32_t ana_reg21_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG21_ISEL_MIC1_DEFAULT_VAL & SYS_ANA_REG21_ISEL_MIC1_MASK) << SYS_ANA_REG21_ISEL_MIC1_POS);
	value |= ((SYS_ANA_REG21_MICIRSEL1_MIC1_DEFAULT_VAL & SYS_ANA_REG21_MICIRSEL1_MIC1_MASK) << SYS_ANA_REG21_MICIRSEL1_MIC1_POS);
	value |= ((SYS_ANA_REG21_VCMSEL_MIC1_DEFAULT_VAL & SYS_ANA_REG21_VCMSEL_MIC1_MASK) << SYS_ANA_REG21_VCMSEL_MIC1_POS);
	value |= ((SYS_ANA_REG21_ENFSR_MIC1_DEFAULT_VAL & SYS_ANA_REG21_ENFSR_MIC1_MASK) << SYS_ANA_REG21_ENFSR_MIC1_POS);
	value |= ((SYS_ANA_REG21_ENOPOCLIP_MIC1_DEFAULT_VAL & SYS_ANA_REG21_ENOPOCLIP_MIC1_MASK) << SYS_ANA_REG21_ENOPOCLIP_MIC1_POS);
	value |= ((SYS_ANA_REG21_DA2ADEN_MIC1_DEFAULT_VAL & SYS_ANA_REG21_DA2ADEN_MIC1_MASK) << SYS_ANA_REG21_DA2ADEN_MIC1_POS);
	value |= ((SYS_ANA_REG21_IMATCH_MIC1_DEFAULT_VAL & SYS_ANA_REG21_IMATCH_MIC1_MASK) << SYS_ANA_REG21_IMATCH_MIC1_POS);
	value |= ((SYS_ANA_REG21_IMATCH_EN_MIC1_DEFAULT_VAL & SYS_ANA_REG21_IMATCH_EN_MIC1_MASK) << SYS_ANA_REG21_IMATCH_EN_MIC1_POS);
	value |= ((SYS_ANA_REG21_DCCOMPEN_MIC1_DEFAULT_VAL & SYS_ANA_REG21_DCCOMPEN_MIC1_MASK) << SYS_ANA_REG21_DCCOMPEN_MIC1_POS);
	value |= ((SYS_ANA_REG21_MICSINGLEEN_MIC1_DEFAULT_VAL & SYS_ANA_REG21_MICSINGLEEN_MIC1_MASK) << SYS_ANA_REG21_MICSINGLEEN_MIC1_POS);
	value |= ((SYS_ANA_REG21_NC_14_DEFAULT_VAL & SYS_ANA_REG21_NC_14_14_MASK) << SYS_ANA_REG21_NC_14_14_POS);
	value |= ((SYS_ANA_REG21_MICGAIN_MIC1_DEFAULT_VAL & SYS_ANA_REG21_MICGAIN_MIC1_MASK) << SYS_ANA_REG21_MICGAIN_MIC1_POS);
	value |= ((SYS_ANA_REG21_NC_19_23_DEFAULT_VAL & SYS_ANA_REG21_NC_19_23_MASK) << SYS_ANA_REG21_NC_19_23_POS);
	value |= ((SYS_ANA_REG21_DWAMODE_MIC1_DEFAULT_VAL & SYS_ANA_REG21_DWAMODE_MIC1_MASK) << SYS_ANA_REG21_DWAMODE_MIC1_POS);
	value |= ((SYS_ANA_REG21_NC_25_27_DEFAULT_VAL & SYS_ANA_REG21_NC_25_27_MASK) << SYS_ANA_REG21_NC_25_27_POS);
	value |= ((SYS_ANA_REG21_MICEN_MIC1_DEFAULT_VAL & SYS_ANA_REG21_MICEN_MIC1_MASK) << SYS_ANA_REG21_MICEN_MIC1_POS);
	value |= ((SYS_ANA_REG21_RST_MIC1_DEFAULT_VAL & SYS_ANA_REG21_RST_MIC1_MASK) << SYS_ANA_REG21_RST_MIC1_POS);
	value |= ((SYS_ANA_REG21_BPDWA1V_MIC1_DEFAULT_VAL & SYS_ANA_REG21_BPDWA1V_MIC1_MASK) << SYS_ANA_REG21_BPDWA1V_MIC1_POS);
	value |= ((SYS_ANA_REG21_HCEN1STG_MIC1_DEFAULT_VAL & SYS_ANA_REG21_HCEN1STG_MIC1_MASK) << SYS_ANA_REG21_HCEN1STG_MIC1_POS);

	return value;
}

static uint32_t ana_reg27_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG27_ISEL_MIC2_DEFAULT_VAL & SYS_ANA_REG27_ISEL_MIC2_MASK) << SYS_ANA_REG27_ISEL_MIC2_POS);
	value |= ((SYS_ANA_REG27_MICIRSEL1_MIC2_DEFAULT_VAL & SYS_ANA_REG27_MICIRSEL1_MIC2_MASK) << SYS_ANA_REG27_MICIRSEL1_MIC2_POS);
	value |= ((SYS_ANA_REG27_VCMSEL_MIC2_DEFAULT_VAL & SYS_ANA_REG27_VCMSEL_MIC2_MASK) << SYS_ANA_REG27_VCMSEL_MIC2_POS);
	value |= ((SYS_ANA_REG27_ENFSR_MIC2_DEFAULT_VAL & SYS_ANA_REG27_ENFSR_MIC2_MASK) << SYS_ANA_REG27_ENFSR_MIC2_POS);
	value |= ((SYS_ANA_REG27_ENOPOCLIP_MIC2_DEFAULT_VAL & SYS_ANA_REG27_ENOPOCLIP_MIC2_MASK) << SYS_ANA_REG27_ENOPOCLIP_MIC2_POS);
	value |= ((SYS_ANA_REG27_DA2ADEN_MIC2_DEFAULT_VAL & SYS_ANA_REG27_DA2ADEN_MIC2_MASK) << SYS_ANA_REG27_DA2ADEN_MIC2_POS);
	value |= ((SYS_ANA_REG27_IMATCH_MIC2_DEFAULT_VAL & SYS_ANA_REG27_IMATCH_MIC2_MASK) << SYS_ANA_REG27_IMATCH_MIC2_POS);
	value |= ((SYS_ANA_REG27_IMATCH_EN_MIC2_DEFAULT_VAL & SYS_ANA_REG27_IMATCH_EN_MIC2_MASK) << SYS_ANA_REG27_IMATCH_EN_MIC2_POS);
	value |= ((SYS_ANA_REG27_DCCOMPEN_MIC2_DEFAULT_VAL & SYS_ANA_REG27_DCCOMPEN_MIC2_MASK) << SYS_ANA_REG27_DCCOMPEN_MIC2_POS);
	value |= ((SYS_ANA_REG27_MICSINGLEEN_MIC2_DEFAULT_VAL & SYS_ANA_REG27_MICSINGLEEN_MIC2_MASK) << SYS_ANA_REG27_MICSINGLEEN_MIC2_POS);
	value |= ((SYS_ANA_REG27_NC_14_DEFAULT_VAL & SYS_ANA_REG27_NC_14_14_MASK) << SYS_ANA_REG27_NC_14_14_POS);
	value |= ((SYS_ANA_REG27_MICGAIN_MIC2_DEFAULT_VAL & SYS_ANA_REG27_MICGAIN_MIC2_MASK) << SYS_ANA_REG27_MICGAIN_MIC2_POS);
	value |= ((SYS_ANA_REG27_NC_19_23_DEFAULT_VAL & SYS_ANA_REG27_NC_19_23_MASK) << SYS_ANA_REG27_NC_19_23_POS);
	value |= ((SYS_ANA_REG27_DWAMODE_MIC2_DEFAULT_VAL & SYS_ANA_REG27_DWAMODE_MIC2_MASK) << SYS_ANA_REG27_DWAMODE_MIC2_POS);
	value |= ((SYS_ANA_REG27_NC_25_27_DEFAULT_VAL & SYS_ANA_REG27_NC_25_27_MASK) << SYS_ANA_REG27_NC_25_27_POS);
	value |= ((SYS_ANA_REG27_MICEN_MIC2_DEFAULT_VAL & SYS_ANA_REG27_MICEN_MIC2_MASK) << SYS_ANA_REG27_MICEN_MIC2_POS);
	value |= ((SYS_ANA_REG27_RST_MIC2_DEFAULT_VAL & SYS_ANA_REG27_RST_MIC2_MASK) << SYS_ANA_REG27_RST_MIC2_POS);
	value |= ((SYS_ANA_REG27_BPDWA1V_MIC2_DEFAULT_VAL & SYS_ANA_REG27_BPDWA1V_MIC2_MASK) << SYS_ANA_REG27_BPDWA1V_MIC2_POS);
	value |= ((SYS_ANA_REG27_HCEN1STG_MIC2_DEFAULT_VAL & SYS_ANA_REG27_HCEN1STG_MIC2_MASK) << SYS_ANA_REG27_HCEN1STG_MIC2_POS);

	return value;
}

static uint32_t ana_reg28_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG28_ISEL_MIC3_DEFAULT_VAL & SYS_ANA_REG28_ISEL_MIC3_MASK) << SYS_ANA_REG28_ISEL_MIC3_POS);
	value |= ((SYS_ANA_REG28_MICIRSEL1_MIC3_DEFAULT_VAL & SYS_ANA_REG28_MICIRSEL1_MIC3_MASK) << SYS_ANA_REG28_MICIRSEL1_MIC3_POS);
	value |= ((SYS_ANA_REG28_VCMSEL_MIC3_DEFAULT_VAL & SYS_ANA_REG28_VCMSEL_MIC3_MASK) << SYS_ANA_REG28_VCMSEL_MIC3_POS);
	value |= ((SYS_ANA_REG28_ENFSR_MIC3_DEFAULT_VAL & SYS_ANA_REG28_ENFSR_MIC3_MASK) << SYS_ANA_REG28_ENFSR_MIC3_POS);
	value |= ((SYS_ANA_REG28_ENOPOCLIP_MIC3_DEFAULT_VAL & SYS_ANA_REG28_ENOPOCLIP_MIC3_MASK) << SYS_ANA_REG28_ENOPOCLIP_MIC3_POS);
	value |= ((SYS_ANA_REG28_DA2ADEN_MIC3_DEFAULT_VAL & SYS_ANA_REG28_DA2ADEN_MIC3_MASK) << SYS_ANA_REG28_DA2ADEN_MIC3_POS);
	value |= ((SYS_ANA_REG28_IMATCH_MIC3_DEFAULT_VAL & SYS_ANA_REG28_IMATCH_MIC3_MASK) << SYS_ANA_REG28_IMATCH_MIC3_POS);
	value |= ((SYS_ANA_REG28_IMATCH_EN_MIC3_DEFAULT_VAL & SYS_ANA_REG28_IMATCH_EN_MIC3_MASK) << SYS_ANA_REG28_IMATCH_EN_MIC3_POS);
	value |= ((SYS_ANA_REG28_DCCOMPEN_MIC3_DEFAULT_VAL & SYS_ANA_REG28_DCCOMPEN_MIC3_MASK) << SYS_ANA_REG28_DCCOMPEN_MIC3_POS);
	value |= ((SYS_ANA_REG28_MICSINGLEEN_MIC3_DEFAULT_VAL & SYS_ANA_REG28_MICSINGLEEN_MIC3_MASK) << SYS_ANA_REG28_MICSINGLEEN_MIC3_POS);
	value |= ((SYS_ANA_REG28_NC_14_DEFAULT_VAL & SYS_ANA_REG28_NC_14_14_MASK) << SYS_ANA_REG28_NC_14_14_POS);
	value |= ((SYS_ANA_REG28_MICGAIN_MIC3_DEFAULT_VAL & SYS_ANA_REG28_MICGAIN_MIC3_MASK) << SYS_ANA_REG28_MICGAIN_MIC3_POS);
	value |= ((SYS_ANA_REG28_NC_19_23_DEFAULT_VAL & SYS_ANA_REG28_NC_19_23_MASK) << SYS_ANA_REG28_NC_19_23_POS);
	value |= ((SYS_ANA_REG28_DWAMODE_MIC3_DEFAULT_VAL & SYS_ANA_REG28_DWAMODE_MIC3_MASK) << SYS_ANA_REG28_DWAMODE_MIC3_POS);
	value |= ((SYS_ANA_REG28_NC_25_27_DEFAULT_VAL & SYS_ANA_REG28_NC_25_27_MASK) << SYS_ANA_REG28_NC_25_27_POS);
	value |= ((SYS_ANA_REG28_MICEN_MIC3_DEFAULT_VAL & SYS_ANA_REG28_MICEN_MIC3_MASK) << SYS_ANA_REG28_MICEN_MIC3_POS);
	value |= ((SYS_ANA_REG28_RST_MIC3_DEFAULT_VAL & SYS_ANA_REG28_RST_MIC3_MASK) << SYS_ANA_REG28_RST_MIC3_POS);
	value |= ((SYS_ANA_REG28_BPDWA1V_MIC3_DEFAULT_VAL & SYS_ANA_REG28_BPDWA1V_MIC3_MASK) << SYS_ANA_REG28_BPDWA1V_MIC3_POS);
	value |= ((SYS_ANA_REG28_HCEN1STG_MIC3_DEFAULT_VAL & SYS_ANA_REG28_HCEN1STG_MIC3_MASK) << SYS_ANA_REG28_HCEN1STG_MIC3_POS);

	return value;
}

static uint32_t ana_reg29_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG29_HPDAC_DEFAULT_VAL & SYS_ANA_REG29_HPDAC_MASK) << SYS_ANA_REG29_HPDAC_POS);
	value |= ((SYS_ANA_REG29_ISELSTG_DEFAULT_VAL & SYS_ANA_REG29_ISELSTG_MASK) << SYS_ANA_REG29_ISELSTG_POS);
	value |= ((SYS_ANA_REG29_OSCDAC_DEFAULT_VAL & SYS_ANA_REG29_OSCDAC_MASK) << SYS_ANA_REG29_OSCDAC_POS);
	value |= ((SYS_ANA_REG29_OCENDAC_DEFAULT_VAL & SYS_ANA_REG29_OCENDAC_MASK) << SYS_ANA_REG29_OCENDAC_POS);
	value |= ((SYS_ANA_REG29_VSELDCO_DEFAULT_VAL & SYS_ANA_REG29_VSELDCO_MASK) << SYS_ANA_REG29_VSELDCO_POS);
	value |= ((SYS_ANA_REG29_SRSEL_DEFAULT_VAL & SYS_ANA_REG29_SRSEL_MASK) << SYS_ANA_REG29_SRSEL_POS);
	value |= ((SYS_ANA_REG29_HPOEN_DEFAULT_VAL & SYS_ANA_REG29_HPOEN_MASK) << SYS_ANA_REG29_HPOEN_POS);
	value |= ((SYS_ANA_REG29_LBWEN_DEFAULT_VAL & SYS_ANA_REG29_LBWEN_MASK) << SYS_ANA_REG29_LBWEN_POS);
	value |= ((SYS_ANA_REG29_CALSEL_DEFAULT_VAL & SYS_ANA_REG29_CALSEL_MASK) << SYS_ANA_REG29_CALSEL_POS);
	value |= ((SYS_ANA_REG29_BP2VLDO_DEFAULT_VAL & SYS_ANA_REG29_BP2VLDO_MASK) << SYS_ANA_REG29_BP2VLDO_POS);
	value |= ((SYS_ANA_REG29_DCOCHG_DEFAULT_VAL & SYS_ANA_REG29_DCOCHG_MASK) << SYS_ANA_REG29_DCOCHG_POS);
	value |= ((SYS_ANA_REG29_DIFFEN_DEFAULT_VAL & SYS_ANA_REG29_DIFFEN_MASK) << SYS_ANA_REG29_DIFFEN_POS);
	value |= ((SYS_ANA_REG29_ENDACCAL_DEFAULT_VAL & SYS_ANA_REG29_ENDACCAL_MASK) << SYS_ANA_REG29_ENDACCAL_POS);
	value |= ((SYS_ANA_REG29_RENDCOC_DEFAULT_VAL & SYS_ANA_REG29_RENDCOC_MASK) << SYS_ANA_REG29_RENDCOC_POS);
	value |= ((SYS_ANA_REG29_LENDCOC_DEFAULT_VAL & SYS_ANA_REG29_LENDCOC_MASK) << SYS_ANA_REG29_LENDCOC_POS);
	value |= ((SYS_ANA_REG29_RENVCMD_DEFAULT_VAL & SYS_ANA_REG29_RENVCMD_MASK) << SYS_ANA_REG29_RENVCMD_POS);
	value |= ((SYS_ANA_REG29_LENVCMD_DEFAULT_VAL & SYS_ANA_REG29_LENVCMD_MASK) << SYS_ANA_REG29_LENVCMD_POS);
	value |= ((SYS_ANA_REG29_DACDRVEN_DEFAULT_VAL & SYS_ANA_REG29_DACDRVEN_MASK) << SYS_ANA_REG29_DACDRVEN_POS);
	value |= ((SYS_ANA_REG29_DACREN_DEFAULT_VAL & SYS_ANA_REG29_DACREN_MASK) << SYS_ANA_REG29_DACREN_POS);
	value |= ((SYS_ANA_REG29_DACLEN_DEFAULT_VAL & SYS_ANA_REG29_DACLEN_MASK) << SYS_ANA_REG29_DACLEN_POS);
	value |= ((SYS_ANA_REG29_DACG_DEFAULT_VAL & SYS_ANA_REG29_DACG_MASK) << SYS_ANA_REG29_DACG_POS);
	value |= ((SYS_ANA_REG29_DACMUTE_DEFAULT_VAL & SYS_ANA_REG29_DACMUTE_MASK) << SYS_ANA_REG29_DACMUTE_POS);
	value |= ((SYS_ANA_REG29_DACDWAMODE_SEL_DEFAULT_VAL & SYS_ANA_REG29_DACDWAMODE_SEL_MASK) << SYS_ANA_REG29_DACDWAMODE_SEL_POS);
	value |= ((SYS_ANA_REG29_CKPSEL_DEFAULT_VAL & SYS_ANA_REG29_CKPSEL_MASK) << SYS_ANA_REG29_CKPSEL_POS);
	value |= ((SYS_ANA_REG29_NC_29_31_DEFAULT_VAL & SYS_ANA_REG29_NC_29_31_MASK) << SYS_ANA_REG29_NC_29_31_POS);

	return value;
}

static uint32_t ana_reg30_value_cal(void)
{
	uint32_t value = 0;

	value |= ((SYS_ANA_REG30_LMDCIN_DEFAULT_VAL & SYS_ANA_REG30_LMDCIN_MASK) << SYS_ANA_REG30_LMDCIN_POS);
	value |= ((SYS_ANA_REG30_RMDCIN_DEFAULT_VAL & SYS_ANA_REG30_RMDCIN_MASK) << SYS_ANA_REG30_RMDCIN_POS);
	value |= ((SYS_ANA_REG30_SPIRST_OVC_DEFAULT_VAL & SYS_ANA_REG30_SPIRST_OVC_MASK) << SYS_ANA_REG30_SPIRST_OVC_POS);
	value |= ((SYS_ANA_REG30_ENIDACR_DEFAULT_VAL & SYS_ANA_REG30_ENIDACR_MASK) << SYS_ANA_REG30_ENIDACR_POS);
	value |= ((SYS_ANA_REG30_ENIDACL_DEFAULT_VAL & SYS_ANA_REG30_ENIDACL_MASK) << SYS_ANA_REG30_ENIDACL_POS);
	value |= ((SYS_ANA_REG30_DAC3RDHC0V9_DEFAULT_VAL & SYS_ANA_REG30_DAC3RDHC0V9_MASK) << SYS_ANA_REG30_DAC3RDHC0V9_POS);
	value |= ((SYS_ANA_REG30_HC2S_DEFAULT_VAL & SYS_ANA_REG30_HC2S_MASK) << SYS_ANA_REG30_HC2S_POS);
	value |= ((SYS_ANA_REG30_SNG_FB_EN_DEFAULT_VAL & SYS_ANA_REG30_SNG_FB_EN_MASK) << SYS_ANA_REG30_SNG_FB_EN_POS);
	value |= ((SYS_ANA_REG30_RFB_CTRL_DEFAULT_VAL & SYS_ANA_REG30_RFB_CTRL_MASK) << SYS_ANA_REG30_RFB_CTRL_POS);
	value |= ((SYS_ANA_REG30_ENBS_DEFAULT_VAL & SYS_ANA_REG30_ENBS_MASK) << SYS_ANA_REG30_ENBS_POS);
	value |= ((SYS_ANA_REG30_CALCK_SEL0V9_DEFAULT_VAL & SYS_ANA_REG30_CALCK_SEL0V9_MASK) << SYS_ANA_REG30_CALCK_SEL0V9_POS);
	value |= ((SYS_ANA_REG30_BPDWA0V9_DEFAULT_VAL & SYS_ANA_REG30_BPDWA0V9_MASK) << SYS_ANA_REG30_BPDWA0V9_POS);
	value |= ((SYS_ANA_REG30_LOOPRST0V9_DEFAULT_VAL & SYS_ANA_REG30_LOOPRST0V9_MASK) << SYS_ANA_REG30_LOOPRST0V9_POS);
	value |= ((SYS_ANA_REG30_OCT0V9_DEFAULT_VAL & SYS_ANA_REG30_OCT0V9_MASK) << SYS_ANA_REG30_OCT0V9_POS);
	value |= ((SYS_ANA_REG30_SOUT0V9_DEFAULT_VAL & SYS_ANA_REG30_SOUT0V9_MASK) << SYS_ANA_REG30_SOUT0V9_POS);
	value |= ((SYS_ANA_REG30_HC0V9_DEFAULT_VAL & SYS_ANA_REG30_HC0V9_MASK) << SYS_ANA_REG30_HC0V9_POS);

	return value;
}


bk_err_t bk_aud_apll_spi_trigger(void)
{
    sys_drv_apll_spi_trigger_set(1);

#if 0//CONFIG_TIMER_US
	GPIO_DOWN(8);GPIO_UP(8);
	bk_timer_delay_us(1000);
	GPIO_DOWN(8);
#else
	uint32_t tick = rtos_get_time();
	uint32_t tick1 = 0;
	while (1)
	{
		tick1 = rtos_get_time();
		if ((tick1 - tick) > 2)
		{
			break;
		}
	}
#endif
	sys_drv_apll_spi_trigger_set(0);
	return BK_OK;
}

bk_err_t bk_aud_apll_config(aud_apll_freq_t freq)
{
	for (uint8_t i = 0; i < 2; i++) {
		uint32_t apll_coefs = 0;

	#if 0
		if (1 == sys_drv_get_apll_en_status())
		{
			/* check frequency */
			//TODO
			return BK_OK;
		}
	#endif

		switch (freq)
		{
			case AUD_APLL_FREQ_98P3040_MHZ:
				apll_coefs = 0x973CA70;
				break;

			case AUD_APLL_FREQ_90P3168_MHZ:
				apll_coefs = 0x8AF2ECA;
				break;

			default:
				return BK_FAIL;
		}


		//set apll clock config
		sys_drv_apll_en(1);
		sys_drv_apll_cal_val_set(apll_coefs);
		//sys_drv_apll_config_set(0xC2A0AE86);//need check, TODO

		sys_drv_apll_spi_trigger_set(1);
		delay(10);
		sys_drv_apll_spi_trigger_set(0);
	}
    return BK_OK;
}

#endif

bk_err_t bk_aud_clk_config(aud_clk_t clk)
{
	if (clk == AUD_CLK_APLL) {
#if CONFIG_SOC_BK7259

		sys_drv_aud_select_clock(1); /// 0:XTAL, 1:APLL
		sys_drv_aud_set_ckdiv(0);
#endif //#if CONFIG_SOC_BK7236XX
	} else {
		sys_drv_aud_select_clock(0);
	}
	return BK_OK;
}

bk_err_t bk_aud_clk_deconfig(void)
{
	sys_drv_aud_select_clock(0);
	//set apll clock config
	sys_drv_apll_en(0);
	return BK_OK;
}

bk_err_t bk_aud_driver_init(void)
{
	if (s_aud_driver_is_init)
		return BK_OK;

	//bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_ON);
	sys_drv_aud_select_clock(0);
	sys_drv_apll_en(1);
	//bk_pm_clock_ctrl(PM_CLK_ID_AUDIO, CLK_PWR_CTRL_PWR_UP);

#if CONFIG_SOC_BK7259

    /* enable apb clock */
    audio_reg_hal_set_sys_cfg_apb_clk_en_dis(1);

	bk_int_isr_register(INT_SRC_AUDIO, aud_isr, NULL);

	/*init audio paramters*/
	//sys_drv_aud_int_en(1);
#if CONFIG_SOC_SMP
	sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_AUDIO, 1);
#else
	sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_AUDIO, 1);
#endif
	//sys_hal_aud_aud_en(1);

	sys_drv_set_ana_reg25_value(0xC2A06AA6); /// fix value - 260116  //sys 0x59

	sys_drv_set_ana_reg20_value(0x81BF8045);
	sys_drv_set_ana_reg21_value(0x01000013);
	sys_drv_set_ana_reg27_value(0x01000013);
	sys_drv_set_ana_reg28_value(0x01000013);
	sys_drv_set_ana_reg29_value(0x8807A303);
	sys_drv_set_ana_reg30_value(0x80708080);

#endif
	s_aud_driver_is_init = true;
	return BK_OK;
}

bk_err_t bk_aud_driver_deinit(void)
{
	if (!s_aud_driver_is_init) {
		return BK_OK;
	}

	/* check module init status */
	if (s_aud_module_init_sta.adc_is_init || s_aud_module_init_sta.dmic_is_init || s_aud_module_init_sta.dtmf_is_init || s_aud_module_init_sta.dac_is_init) {
		return BK_OK;
	}

	//reset audo configure
#if CONFIG_AUDIO_ADC
	bk_aud_adc_deinit();
#endif
#if CONFIG_AUDIO_DAC
	bk_aud_dac_deinit();
#endif
#if CONFIG_AUDIO_DTMF
	//bk_aud_dtmf_deinit();
#endif
#if CONFIG_AUDIO_DMIC
	//bk_aud_dmic_deinit();
#endif

	//disable audio interrupt
#if CONFIG_SOC_SMP
    sys_drv_set_int_en(CPU2_CORE_ID, INT_SRC_AUDIO, 0);
#else
    sys_drv_set_int_en(rtos_get_core_id(), INT_SRC_AUDIO, 0);
#endif
	//ungister isr
	bk_int_isr_unregister(INT_SRC_AUDIO);

#if CONFIG_SOC_BK7259
	/* enable apb clock */
	audio_reg_hal_set_sys_cfg_apb_clk_en_dis(1); ////

	// config analog register
#if 0
	// Temporarily disabled: Disable audio clock  --- 20260109-yong.li
	// TODO: Re-enable after fixing the issue
	sys_hal_aud_clock_en(0);   /// 
#endif

	sys_drv_set_ana_reg20_value(0);
	sys_drv_set_ana_reg21_value(0);
	sys_drv_set_ana_reg27_value(0);
	sys_drv_set_ana_reg28_value(0);
	sys_drv_set_ana_reg29_value(0);
	sys_drv_set_ana_reg30_value(0);

	sys_drv_aud_audbias_en(0);
	bk_aud_hardware_reset_release();

	bk_timer_delay_us(50);

#endif
	//bk_aud_clk_deconfig();

	//bk_pm_clock_ctrl(PM_CLK_ID_AUDIO, CLK_PWR_CTRL_PWR_DOWN);
	//bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_OFF);

	s_aud_driver_is_init = false;
	return BK_OK;
}

/* set module init status */
bk_err_t bk_aud_set_module_init_sta(aud_module_id_t id, bool val)
{
	switch (id) {
		case AUD_MODULE_ADC:
			s_aud_module_init_sta.adc_is_init = val;
			break;
		case AUD_MODULE_DMIC:
			s_aud_module_init_sta.dmic_is_init = val;
			break;
		case AUD_MODULE_DTMF:
			s_aud_module_init_sta.dtmf_is_init = val;
			break;
		case AUD_MODULE_DAC:
			s_aud_module_init_sta.dac_is_init = val;
			break;

		default:
			return BK_FAIL;
			break;
	}

	return BK_OK;
}

/* set module init status */
bool bk_aud_get_module_init_sta(aud_module_id_t id)
{
	bool status = false;

	switch (id) {
		case AUD_MODULE_ADC:
			status = s_aud_module_init_sta.adc_is_init;
			break;
		case AUD_MODULE_DMIC:
			status = s_aud_module_init_sta.dmic_is_init;
			break;
		case AUD_MODULE_DTMF:
			status = s_aud_module_init_sta.dtmf_is_init;
			break;
		case AUD_MODULE_DAC:
			status = s_aud_module_init_sta.dac_is_init;
			break;

		default:
			status = false;
			break;
	}

	return status;
}


/* register audio interrupt */
bk_err_t bk_aud_register_aud_isr(aud_isr_id_t isr_id, aud_isr_t isr)
{
//	AUD_RETURN_ON_INVALID_ISR_ID(isr_id);
	bk_err_t ret = BK_OK;
	uint32_t int_level = rtos_enter_critical();

	switch (isr_id) {
#if CONFIG_AUDIO_ADC
		case AUD_ISR_ADCL: /**< adcl_int_en */
			s_aud_isr.aud_adcl_fifo_handler = isr;
			break;
#endif
#if 0//CONFIG_AUDIO_DMIC
		case AUD_ISR_DMIC:
			s_aud_isr.aud_dmic_fifo_handler = isr;
			break;
#endif
#if 0//CONFIG_AUDIO_DTMF
		case AUD_ISR_DTMF:	  /**< dtmf_int_en */
			s_aud_isr.aud_dtmf_fifo_handler = isr;
			break;
#endif
#if CONFIG_AUDIO_DAC
		case AUD_ISR_DACL:	  /**< dacl_int_en */
			s_aud_isr.aud_dacl_fifo_handler = isr;
			break;
		case AUD_ISR_DACR:	  /**< dacr_int_en */
			s_aud_isr.aud_dacr_fifo_handler = isr;
			break;
#endif

		default:
			ret = BK_FAIL;
			break;
	}

	rtos_exit_critical(int_level);

	return ret;
}

/* audio check interrupt flag and excute correponding isr function when enter interrupt */
static void aud_isr_common(void)
{
#if CONFIG_AUDIO_ADC
#endif

#if 0//CONFIG_AUDIO_DMIC
	uint32_t dmic_int_status = aud_hal_get_fifo_status_dmic_int_flag();
	if (dmic_int_status) {
		if (s_aud_isr.aud_dmic_fifo_handler) {
			s_aud_isr.aud_dmic_fifo_handler();
		}
	}
#endif

#if 0//CONFIG_AUDIO_DTMF
	uint32_t dtmf_int_status = aud_hal_get_fifo_status_dtmf_int_flag();
	if (dtmf_int_status) {
		if (s_aud_isr.aud_dtmf_fifo_handler) {
			s_aud_isr.aud_dtmf_fifo_handler();
		}
	}
#endif

#if CONFIG_AUDIO_DAC
#endif

}

/* audio interrupt enter*/
static void aud_isr(void)
{
	aud_isr_common();
}

void bk_aud_hardware_reset(void)
{
    /* Reset audio registers: AUD_REG_0x2 (0x4101a008) and AUD_REG_0x5E (0x4101a178) */
    audio_reg_hal_set_reserved0_value(0x1);   // soft reset
    audio_reg_ll_set_interface_matrix_value(0x43210);
}

void bk_aud_hardware_reset_release(void)
{
    audio_reg_ll_set_interface_matrix_value(0x0);
    audio_reg_hal_set_reserved0_value(0);
    audio_reg_hal_set_reserved0_value(1);
    audio_reg_hal_set_reserved0_value(0);
}
