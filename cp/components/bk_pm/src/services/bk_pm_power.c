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


static bool s_pm_phy_calibration_state                  = false;
static bool s_pm_is_phy_reinit_flag                     = false;
static uint32_t s_pm_vote_power_module                  = false;

// static uint32_t s_pm_encp_pm_state                      = 0;
static uint32_t s_pm_bakp_pm_state                      = 0;
static uint32_t s_pm_ahpb_pm_state                      = 0;
static uint32_t s_pm_audio_pm_state                     = 0;
static uint32_t s_pm_video_pm_state                     = 0;
// static uint32_t s_pm_btsp_pm_state                      = 0;
// static uint32_t s_pm_thread_pm_state                    = 0;
static uint32_t s_pm_phy_pm_state                       = 0;

static uint32_t s_pm_cpu1_bakp_audp_state               = 0;
static uint32_t s_pm_vehp_spi_debug_state               = 0;
static uint32_t s_pm_wrlp_encp_state                    = 0;
static uint32_t s_pm_hssub_power_state                  = 0;
static uint32_t s_pm_ap_cpu_state                       = 0;

static void pm_module_check_power_off(uint32_t *pm_off_modules, uint32_t *pm_on_modules, pm_power_module_name_e module);


/*=========================MODULES POWER CTRL START========================*/
bk_err_t bk_pm_module_power_on(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules, pm_power_module_name_e module)
{
	bk_err_t ret = BK_OK;
	uint32_t domain, submodule;
	GLOBAL_INT_DECLARATION();

	if (module < PM_POWER_MODULE_NAME_NONE) {
		LOGI("pm_module_power_on module[%d] NONE\r\n",module);
	} else {
		domain = module/PM_MODULE_SUB_POWER_DOMAIN_MAX;
		submodule = module%PM_MODULE_SUB_POWER_DOMAIN_MAX;
		switch (domain) {
			case PM_POWER_DOMAIN_0://POWER_DOMAIN_NAME_CP_CPU1_BAKP_AUDP:
			{
				GLOBAL_INT_DISABLE();
				s_pm_cpu1_bakp_audp_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				GLOBAL_INT_RESTORE();

				if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
			}
				break;
			case PM_POWER_DOMAIN_1://POWER_DOMAIN_NAME_VEHP_SPI_DEBUG:
			{
				GLOBAL_INT_DISABLE();
				s_pm_vehp_spi_debug_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				GLOBAL_INT_RESTORE();

				if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
			}
				break;
			case PM_POWER_DOMAIN_2://POWER_DOMAIN_NAME_WRLP_ENCP:
			{
				GLOBAL_INT_DISABLE();
				s_pm_wrlp_encp_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);

				bk_pm_vote_power_module_set(module);

				GLOBAL_INT_RESTORE();
				//if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				//}
			}
				break;
			case PM_POWER_DOMAIN_3://POWER_SUB_DOMAIN_NAME_AP_CPU:
			{
				if (s_pm_ap_cpu_state ==0)
				{
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
				GLOBAL_INT_DISABLE();
				s_pm_ap_cpu_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				GLOBAL_INT_RESTORE();
			}
				break;
			case PM_POWER_DOMAIN_4://POWER_DOMAIN_NAME_HSSUB_POWER:
			{
				GLOBAL_INT_DISABLE();
				s_pm_hssub_power_state |= (0x1 << submodule);
				*pm_off_modules &= ~(0x1 << domain);
				*pm_on_modules |= (0x1 << domain);
				GLOBAL_INT_RESTORE();

				if (sys_drv_module_power_state_get(domain)) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_ON);
				}
			}
				break;
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
			case PM_POWER_DOMAIN_0://POWER_DOMAIN_NAME_CP_CPU1_BAKP_AUDP:
			{
				GLOBAL_INT_DISABLE();
				s_pm_cpu1_bakp_audp_state &= ~(0x1 << submodule);
				if (0x0 == s_pm_cpu1_bakp_audp_state) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_OFF);
					*pm_off_modules |= (0x1 << domain);
					*pm_on_modules &= ~(0x1 << domain);
				}
				GLOBAL_INT_RESTORE();
			}
				break;
			case PM_POWER_DOMAIN_1://POWER_DOMAIN_NAME_VEHP_SPI_DEBUG:
			{
				GLOBAL_INT_DISABLE();
				s_pm_vehp_spi_debug_state &= ~(0x1 << submodule);
				if (0x0 == s_pm_vehp_spi_debug_state) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_OFF);
					*pm_off_modules |= (0x1 << domain);
					*pm_on_modules &= ~(0x1 << domain);
				}
				GLOBAL_INT_RESTORE();
			}
				break;
			case PM_POWER_DOMAIN_2://POWER_DOMAIN_NAME_WRLP_ENCP:
			{
				GLOBAL_INT_DISABLE();
				s_pm_wrlp_encp_state &= ~(0x1 << submodule);
				if (0x0 == s_pm_wrlp_encp_state) {
					sys_drv_module_power_ctrl(domain, PM_POWER_MODULE_STATE_OFF);
					*pm_off_modules |= (0x1 << domain);
					*pm_on_modules &= ~(0x1 << domain);
				}
				GLOBAL_INT_RESTORE();
			}
				break;
			case PM_POWER_DOMAIN_3://POWER_DOMAIN_NAME_AP_CPU:
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
			case PM_POWER_DOMAIN_4://POWER_DOMAIN_NAME_HSSUB_POWER:
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
		case PM_POWER_DOMAIN_0://POWER_DOMAIN_NAME_CP_CPU1_BAKP_AUDP:
		case PM_POWER_DOMAIN_1://POWER_DOMAIN_NAME_VEHP_SPI_DEBUG:
		case PM_POWER_DOMAIN_2://POWER_DOMAIN_NAME_WRLP_ENCP:
		case PM_POWER_DOMAIN_3://POWER_DOMAIN_NAME_AP_CPU:
		case PM_POWER_DOMAIN_4://POWER_DOMAIN_NAME_HSSUB_POWER:
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
	if (!(*pm_on_modules & ((0x1 << (PM_POWER_SUB_DOMAIN_BTDM%PM_MODULE_SUB_POWER_DOMAIN_MAX))))) // when the module not power on , set the module sleep state
	{
		//*pm_sleeped_modules |= 0x1ULL << PM_SLEEP_MODULE_NAME_BTSP;
		*pm_off_modules |= (0x1 << (PM_POWER_SUB_DOMAIN_BTDM%PM_MODULE_SUB_POWER_DOMAIN_MAX));
		// BK_LOGD(NULL, "bt not power on \r\n");
	}

	if (!(*pm_on_modules & ((0x1 << PM_POWER_SUB_DOMAIN_MAC%PM_MODULE_SUB_POWER_DOMAIN_MAX)))) // when the module not power on , set the module sleep state
	{
		//*pm_sleeped_modules |= 0x1ULL << PM_SLEEP_MODULE_NAME_WIFIP_MAC;
		*pm_off_modules |= (0x1 << (PM_POWER_SUB_DOMAIN_MAC%PM_MODULE_SUB_POWER_DOMAIN_MAX));
		// BK_LOGD(NULL, "wifi not power on \r\n");
	}

	// if (!(*pm_on_modules & (0x1 << PM_POWER_MODULE_NAME_AUDP))) // when the module not power on , set the module sleep state
	// {
	// 	*pm_sleeped_modules |= 0x1ULL << PM_POWER_MODULE_NAME_AUDP;
	// 	*pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_AUDP;
	// 	// BK_LOGD(NULL, "audio not power on \r\n");
	// }

	// if (!(*pm_on_modules & (0x1 << PM_POWER_MODULE_NAME_VIDP))) // when the module not power on , set the module sleep state
	// {
	// 	*pm_sleeped_modules |= 0x1ULL << PM_POWER_MODULE_NAME_VIDP;
	// 	*pm_off_modules |= 0x1 << PM_POWER_MODULE_NAME_VIDP;
	// 	// BK_LOGD(NULL, "video not power on \r\n");
	// }
}

void pm_check_power_off_module(uint32_t *pm_off_modules, uint32_t *pm_on_modules, uint64_t *pm_sleeped_modules)
{

}

static void pm_module_check_power_off(uint32_t *pm_off_modules, uint32_t *pm_on_modules, pm_power_module_name_e module)
{
	if (PM_POWER_MODULE_STATE_ON == sys_drv_module_power_state_get(module))
	{
		if (pm_debug_mode() & 0x2)
		{
			BK_LOGD(NULL, "pm_module_check_power_off module[%d][%d]\r\n", module, sys_drv_module_power_state_get(module));
		}
		sys_drv_module_power_ctrl(module, PM_POWER_MODULE_STATE_OFF);

		*pm_off_modules |= 0x1 << module;
		*pm_on_modules &= ~(0x1 << module);
	}
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

/*=========================RF POWER CTRL END========================*/

/*=========================SPECIFIC API START========================*/
bool bk_pm_phy_cali_state_get()
{
	return s_pm_phy_calibration_state;
}
bk_err_t bk_pm_phy_cali_state_set(bool cali_state)
{
	s_pm_phy_calibration_state = cali_state;
	return BK_OK;
}

bool bk_pm_phy_reinit_flag_get()
{
	return s_pm_is_phy_reinit_flag;
}
bk_err_t bk_pm_phy_reinit_flag_set(bool reinit_flag)
{
	s_pm_is_phy_reinit_flag = reinit_flag;
	return BK_OK;
}

void bk_pm_phy_reinit_flag_clear()
{
	s_pm_is_phy_reinit_flag = false;
}

uint32_t bk_pm_vote_power_module_get()
{
	return s_pm_vote_power_module;
}
bk_err_t bk_pm_vote_power_module_set(uint32_t vote_power_module)
{
	s_pm_vote_power_module = vote_power_module;
	return BK_OK;
}
/*=========================SPECIFIC API END========================*/

/*=========================DEBUG/TEST CTRL START========================*/
void pm_power_dump(void)
{
	LOGD("pm video,audio:0x%x 0x%x \r\n",s_pm_video_pm_state,s_pm_audio_pm_state);
	LOGD("pm ahpb,bakp:0x%x 0x%x\r\n",s_pm_ahpb_pm_state,s_pm_bakp_pm_state);
	LOGD("pm cpu1_bakp_audp:0x%x\r\n",s_pm_cpu1_bakp_audp_state);
	LOGD("pm vehp_spi_debug:0x%x\r\n",s_pm_vehp_spi_debug_state);
	LOGD("pm wrlp_encp:0x%x\r\n",s_pm_wrlp_encp_state);
	LOGD("pm hssub_power:0x%x\r\n",s_pm_hssub_power_state);
}

void pm_power_modules_dump_with_sleep_mode(pm_sleep_mode_e sleep_mode)
{
	LOGI("%s2 0x%X 0x%X 0x%X 0x%X\r\n", pm_sleep_mode_to_string(sleep_mode),
		s_pm_ahpb_pm_state, s_pm_video_pm_state, s_pm_audio_pm_state, s_pm_bakp_pm_state);
}

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

	#if CONFIG_PSRAM && CONFIG_PSRAM_AS_SYS_MEMORY
	pm_debug_psram();
	#endif

	return BK_OK;
}
/*=========================DEBUG/TEST CTRL END========================*/
