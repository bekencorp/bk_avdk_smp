// Copyright 2021-2025 Beken
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
#include <modules/pm.h>
#include <sys_types.h>
#include "sys_driver.h"
#include <driver/sys_pm.h>
#include <driver/pwr_clk.h>

#if CONFIG_INT_WDT
#include <driver/wdt.h>
#include <bk_wdt.h>
#endif
#if CONFIG_AON_WDT
#include <driver/aon_wdt.h>
#endif

#include "pm_power.h"
#include "pm_psram.h"
#include "pm_debug.h"

#define PM_AUXLDO_ENABLE_DELAY_US                      (100)

static uint32_t s_pm_bakp_pm_state                      = 0;
static uint32_t s_pm_ahpb_pm_state                      = 0;
static uint32_t s_pm_audio_pm_state                     = 0;
static uint32_t s_pm_video_pm_state                     = 0;
static uint32_t s_pm_phy_pm_state                       = 0;

static uint32_t s_pm_hssub_power_state                  = 0;
static uint32_t s_pm_ap_cpu_state                       = 0;
static uint32_t s_pm_video_post_state                   = 0;
static uint32_t s_pm_h26e_state                         = 0;
static uint32_t s_pm_isp_state                          = 0;
static uint32_t s_pm_npu_state                          = 0;
static uint32_t s_pm_auxldo_1p2v_vote_state             = 0;
static uint32_t s_pm_auxldo_1p8v_vote_state             = 0;
static uint32_t s_pm_auxldo_2p8v_vote_state             = 0;
static uint32_t s_pm_auxldo_3v_vote_state               = 0;

extern void bk_delay_us(UINT32 us);

/*=========================MODULES POWER CTRL START========================*/
bk_err_t bk_pm_module_power_on(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules, pm_power_module_name_e module)
{
	bk_err_t ret = BK_OK;
	uint32_t domain, submodule;
	GLOBAL_INT_DECLARATION();

	if (module < PM_POWER_MODULE_NAME_NONE) {
	} else {
		domain = module/PM_MODULE_SUB_POWER_DOMAIN_MAX;
		submodule = module%PM_MODULE_SUB_POWER_DOMAIN_MAX;
		switch (domain) {

			case POWER_DOMAIN_NAME_HSSUB_POWER:
			{
				GLOBAL_INT_DISABLE();
				s_pm_hssub_power_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				*pm_sleeped_modules &= ~(0x1ULL << domain);
				GLOBAL_INT_RESTORE();

				if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
			}
				break;
			case POWER_DOMAIN_NAME_AP_CPU:
			{
				GLOBAL_INT_DISABLE();
				s_pm_ap_cpu_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				*pm_sleeped_modules &= ~(0x1ULL << domain);
				GLOBAL_INT_RESTORE();

				if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
			}
				break;
			case POWER_DOMAIN_NAME_VIDEO_POST:
			{
				GLOBAL_INT_DISABLE();
				s_pm_video_post_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				*pm_sleeped_modules &= ~(0x1ULL << domain);
				GLOBAL_INT_RESTORE();

				if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
			}
				break;
			case POWER_DOMAIN_NAME_ISP:
			{
				GLOBAL_INT_DISABLE();
				s_pm_isp_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				*pm_sleeped_modules &= ~(0x1ULL << domain);
				GLOBAL_INT_RESTORE();

				if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
			}
				break;
			case POWER_DOMAIN_NAME_NPU:
			{
				GLOBAL_INT_DISABLE();
				s_pm_npu_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				*pm_sleeped_modules &= ~(0x1ULL << domain);
				GLOBAL_INT_RESTORE();

				if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
			}
				break;
			case POWER_DOMAIN_NAME_H26E:
			{
				GLOBAL_INT_DISABLE();
				s_pm_h26e_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				*pm_sleeped_modules &= ~(0x1ULL << domain);
				GLOBAL_INT_RESTORE();

				if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
			}
			default:
				ret = BK_FAIL;
				break;
		}
	}

	return ret;
}

bk_err_t bk_pm_module_power_off(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules, pm_power_module_name_e module)
{
	uint32_t domain, submodule;
	GLOBAL_INT_DECLARATION();

	if (module < PM_POWER_MODULE_NAME_NONE) {
	} else {
		domain = module/PM_MODULE_SUB_POWER_DOMAIN_MAX;
		submodule = module%PM_MODULE_SUB_POWER_DOMAIN_MAX;
		switch (domain) {

			case POWER_DOMAIN_NAME_HSSUB_POWER:
			{
				GLOBAL_INT_DISABLE();
				s_pm_hssub_power_state &= ~(0x1 << submodule);
				if (0x0 == s_pm_hssub_power_state) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_OFF);
					*pm_off_modules |= (0x1 << domain);
					*pm_on_modules &= ~(0x1 << domain);
				}
				GLOBAL_INT_RESTORE();
			}
				break;

			case POWER_DOMAIN_NAME_AP_CPU:
			{
				GLOBAL_INT_DISABLE();
				s_pm_ap_cpu_state &= ~(0x1 << submodule);

				if (0x0 == s_pm_ap_cpu_state) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_OFF);
					*pm_off_modules |= (0x1 << domain);
					*pm_on_modules &= ~(0x1 << domain);
				}
				GLOBAL_INT_RESTORE();
			}
				break;

			case POWER_DOMAIN_NAME_VIDEO_POST:
			{
				GLOBAL_INT_DISABLE();
				s_pm_video_post_state &= ~(0x1 << submodule);
				if (0x0 == s_pm_video_post_state) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_OFF);
					*pm_off_modules |= (0x1 << domain);
					*pm_on_modules &= ~(0x1 << domain);
				}
				GLOBAL_INT_RESTORE();
			}
				break;

			case POWER_DOMAIN_NAME_ISP:
			{
				GLOBAL_INT_DISABLE();
				s_pm_isp_state &= ~(0x1 << submodule);
				if (0x0 == s_pm_isp_state) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_OFF);
					*pm_off_modules |= (0x1 << domain);
					*pm_on_modules &= ~(0x1 << domain);
				}
				GLOBAL_INT_RESTORE();
			}
				break;

			case POWER_DOMAIN_NAME_NPU:
			{
				GLOBAL_INT_DISABLE();
				s_pm_npu_state &= ~(0x1 << submodule);
				if (0x0 == s_pm_npu_state) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_OFF);
					*pm_off_modules |= (0x1 << domain);
					*pm_on_modules &= ~(0x1 << domain);
				}
				GLOBAL_INT_RESTORE();
			}
				break;
			case POWER_DOMAIN_NAME_H26E:
			{
				GLOBAL_INT_DISABLE();
				s_pm_h26e_state &= ~(0x1 << submodule);
				if (0x0 == s_pm_h26e_state) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_OFF);
					*pm_off_modules |= (0x1 << domain);
					*pm_on_modules &= ~(0x1 << domain);
				}
				GLOBAL_INT_RESTORE();
			}
				break;
			default:
				break;
		}
	}

	return BK_OK;
}

int32_t bk_pm_module_power_state_get(pm_power_module_name_e module)
{
	uint32_t domain;
	int32_t state;

	if (module < PM_POWER_MODULE_NAME_NONE) {
		return sys_drv_module_power_state_get(module);
	}

	domain = module/PM_MODULE_SUB_POWER_DOMAIN_MAX;
	switch (domain) {
		case POWER_DOMAIN_NAME_AP_CPU:
		case POWER_DOMAIN_NAME_VIDEO_POST:
		case POWER_DOMAIN_NAME_H26E:
		case POWER_DOMAIN_NAME_ISP:
		case POWER_DOMAIN_NAME_NPU:
			state = sys_drv_module_power_state_get(domain);
			break;

		default:
			BK_LOGD(NULL, "pm module[%d] not support ,get power state fail.\r\n", module);
			state = BK_ERR_NOT_SUPPORT;
			break;
	}
	return state;
}

void pm_check_power_on_module(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules)
{

}

void pm_check_power_off_module(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules)
{

}

uint32_t bk_pm_get_audio_vote_pwr_state(void)
{
	return s_pm_audio_pm_state;
}
uint32_t bk_pm_get_video_vote_pwr_state(void)
{
	return s_pm_video_pm_state ;
}

uint32_t bk_pm_phy_pm_state_get()
{
	return s_pm_phy_pm_state;
}
/*=========================MODULES POWER CTRL END========================*/

/*=========================RF POWER CTRL START========================*/
static bool pm_auxldo_out_is_valid(const pm_auxldo_out_cfg_t *auxldo_out_cfg)
{
	switch (auxldo_out_cfg->ldo)
	{
		case AUXLDOS_SEL_1P2V:
			return auxldo_out_cfg->out < PM_AUXLDO_1P2V_OUT_MAX;

		case AUXLDOS_SEL_1P8V:
			return auxldo_out_cfg->out < PM_AUXLDO_1P8V_OUT_MAX;

		case AUXLDOS_SEL_2P8V:
			return auxldo_out_cfg->out < PM_AUXLDO_2P8V_OUT_MAX;

		case AUXLDOS_SEL_3V:
			return auxldo_out_cfg->out < PM_AUXLDO_3V_OUT_MAX;

		default:
			return false;
	}
}

static bk_err_t pm_auxldo_out_vote(const pm_auxldo_out_cfg_t *auxldo_out_cfg)
{
	bk_err_t ret = BK_OK;

	if (auxldo_out_cfg == NULL) {
		return BK_ERR_NULL_PARAM;
	}

	if (!pm_auxldo_out_is_valid(auxldo_out_cfg)) {
		return BK_ERR_PARAM;
	}

	if (auxldo_out_cfg->ldo != AUXLDOS_SEL_2P8V) {
		return sys_drv_auxldo_out_set(auxldo_out_cfg->ldo, auxldo_out_cfg->out);
	}

	/* Keep 2p8v output in vote mode with enable path */

	ret = sys_drv_auxldo_out_set(auxldo_out_cfg->ldo, auxldo_out_cfg->out);

	return ret;
}

static bool pm_auxldo_enable_is_valid(const pm_auxldo_enable_cfg_t *auxldo_enable_cfg)
{
	switch (auxldo_enable_cfg->ldo)
	{
		case AUXLDOS_SEL_1P2V:
		case AUXLDOS_SEL_1P8V:
		case AUXLDOS_SEL_3V:
			return (auxldo_enable_cfg->state < PM_AUXLDO_ENABLE_MAX);

		case AUXLDOS_SEL_2P8V:
			return (auxldo_enable_cfg->state < PM_AUXLDO_ENABLE_MAX);
		default:
			return false;
	}
}

static bk_err_t pm_auxldo_enable_vote(const pm_auxldo_enable_cfg_t *auxldo_enable_cfg)
{
	bk_err_t ret = BK_OK;
	uint32_t *vote_state = NULL;

	if (auxldo_enable_cfg == NULL) {
		return BK_ERR_NULL_PARAM;
	}

	if (!pm_auxldo_enable_is_valid(auxldo_enable_cfg)) {
		return BK_ERR_PARAM;
	}

	switch (auxldo_enable_cfg->ldo) {
		case AUXLDOS_SEL_1P2V:
			vote_state = &s_pm_auxldo_1p2v_vote_state;
			break;
		case AUXLDOS_SEL_1P8V:
			vote_state = &s_pm_auxldo_1p8v_vote_state;
			break;
		case AUXLDOS_SEL_2P8V:
			vote_state = &s_pm_auxldo_2p8v_vote_state;
			break;
		case AUXLDOS_SEL_3V:
			vote_state = &s_pm_auxldo_3v_vote_state;
			break;
		default:
			return BK_ERR_PARAM;
	}

	if (auxldo_enable_cfg->state == PM_AUXLDO_ENABLE)
	{
		if (*vote_state == 0U)
		{
			if(sys_drv_auxldo_enable_state_get(auxldo_enable_cfg->ldo) != PM_AUXLDO_ENABLE)
			{
				ret = sys_drv_auxldo_enable(auxldo_enable_cfg->ldo, (uint32_t)auxldo_enable_cfg->state);
				bk_delay_us(PM_AUXLDO_ENABLE_DELAY_US);
			}
		}

		*vote_state |= (uint32_t)auxldo_enable_cfg->user;
	}
	else
	{
		if (*vote_state & (uint32_t)auxldo_enable_cfg->user)
		{
			*vote_state &= ~((uint32_t)auxldo_enable_cfg->user);
			if (*vote_state == 0U)
			{
				ret = sys_drv_auxldo_enable(auxldo_enable_cfg->ldo, (uint32_t)auxldo_enable_cfg->state);
			}
		}
	}
	return ret;
}

bk_err_t bk_pm_auxldo_ctrl_vote(const pm_auxldo_ctrl_cfg_t *auxldo_cfg)
{
	bk_err_t ret                  = BK_OK;
	pm_auxldo_out_cfg_t out_cfg   = {0};
	pm_auxldo_enable_cfg_t en_cfg = {0};

	if (auxldo_cfg == NULL) {
		LOGD("auxldo_cfg is NULL\r\n");
		return BK_ERR_NULL_PARAM;
	}

	out_cfg.ldo = auxldo_cfg->ldo;
	out_cfg.out = auxldo_cfg->out;

	en_cfg.ldo = auxldo_cfg->ldo;
	en_cfg.state = auxldo_cfg->state;
	en_cfg.user = auxldo_cfg->user;

	ret = pm_auxldo_out_vote(&out_cfg);
	if (ret != BK_OK) {
		LOGD("pm_auxldo_out_vote failed ret:%d\r\n", ret);
		return ret;
	}

	ret = pm_auxldo_enable_vote(&en_cfg);
	if (ret != BK_OK) {
		LOGD("pm_auxldo_enable_vote failed ret:%d\r\n", ret);
		return ret;
	}

	return BK_OK;
}
/*=========================RF POWER CTRL END========================*/

/*=========================SPECIFIC API START========================*/

/*=========================SPECIFIC API END========================*/

/*=========================DEBUG/TEST CTRL START========================*/
void pm_power_dump(void)
{
	LOGD("pm video,audio:0x%x 0x%x \r\n",s_pm_video_pm_state,s_pm_audio_pm_state);
	LOGD("pm ahpb,bakp:0x%x 0x%x\r\n",s_pm_ahpb_pm_state,s_pm_bakp_pm_state);
}

void pm_power_modules_dump_with_sleep_mode(pm_sleep_mode_e sleep_mode)
{
	LOGI("%s2 0x%X 0x%X 0x%X 0x%X\r\n", pm_sleep_mode_to_string(sleep_mode),
		s_pm_ahpb_pm_state, s_pm_video_pm_state, s_pm_audio_pm_state, s_pm_bakp_pm_state);
}

// TODO: rename or remove
bk_err_t pm_debug_module_state(void)
{
	#if CONFIG_PSRAM && CONFIG_PSRAM_AS_SYS_MEMORY
	pm_cp1_psram_malloc_state_get();
	#endif

	if(s_pm_ahpb_pm_state > 0)
	{
		LOGD("Ahbp not PD[module:0x%x]\r\n",s_pm_ahpb_pm_state);
	}
	if(s_pm_bakp_pm_state > 0)
	{
		LOGI("Bakp not PD[module:0x%x]\r\n",s_pm_bakp_pm_state);
	}
	if(s_pm_video_pm_state > 0)
	{
		LOGD("Video not PD[modulue:0x%x]\r\n",s_pm_video_pm_state);
	}
	if(s_pm_audio_pm_state > 0)
	{
		LOGD("Audio not PD[modulue:0x%x]\r\n",s_pm_audio_pm_state);
	}

	if(!bk_pm_module_power_state_get(PM_POWER_MODULE_NAME_CPU1))
	{
		LOGD("Cp1 not PD[state:0x%x]\r\n",bk_pm_module_power_state_get(PM_POWER_MODULE_NAME_CPU1));
	}

	// TODO: why not using bk_pm_module_power_state_get(PM_POWER_MODULE_NAME_CPU2) ?
	// if(!(REG_READ(PM_DEBUG_SYS_REG_BASE+0x6*4)&0x2))
	// {
	// 	BK_LOGD(NULL, "Cp2 not PD[state:0x%x]\r\n",REG_READ(PM_DEBUG_SYS_REG_BASE+0x6*4));
	// }

	#if CONFIG_PSRAM && CONFIG_PSRAM_AS_SYS_MEMORY
	pm_debug_psram();
	#endif

	return BK_OK;
}
/*=========================DEBUG/TEST CTRL END========================*/
