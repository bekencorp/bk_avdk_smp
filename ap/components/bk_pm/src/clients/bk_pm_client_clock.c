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
#include "pm_debug.h"
#include "sys_driver.h"
#include "aon_pmu_driver.h"


#define PM_HIGHEST_CPU_FREQ                     (CONFIG_PM_CPU_FRQ_HIGHEST)
#define PM_CPU_FRQ_NONE                         (-1)

static int8_t s_pm_cpu_freq[PM_DEV_ID_MAX] = {0};
static pm_cpu_freq_e s_pm_current_cpu_freq = PM_CPU_FRQ_DEFAULT;

/*=========================CLK/FREQ CTRL START========================*/

pm_cpu_freq_e bk_pm_current_max_cpu_freq_get()
{
	return s_pm_current_cpu_freq;
}

pm_cpu_freq_e bk_pm_module_current_cpu_freq_get(pm_dev_id_e module)
{
	if ((uint32_t)module >= PM_DEV_ID_MAX)
	{
		return PM_CPU_FRQ_DEFAULT;
	}

	return s_pm_cpu_freq[module];
}

bk_err_t bk_pm_module_vote_cpu_freq(pm_dev_id_e module, pm_cpu_freq_e cpu_freq)
{
	if (((uint32_t)module >= PM_DEV_ID_MAX) || ((uint32_t)cpu_freq > PM_CPU_FRQ_DEFAULT))
	{
		return BK_ERR_PARAM;
	}

	if (pm_debug_mode() & 0x2)
	{
		BK_LOGD(NULL, "current freq = %d,dev_id = %d vote freq = %d \r\n",s_pm_current_cpu_freq, module, cpu_freq);
	}
	bk_err_t ret            = BK_OK;
	uint32_t i              = 0;
	int32_t freq_max        = 0;
	int32_t freq_max_index  = 0;
	bool bcpu_freq_highest  = false;
	int32_t vote_freq       = cpu_freq;
	int8_t previous_vote;

	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	previous_vote = s_pm_cpu_freq[module];

	/*save the cpu freq first*/
	if (PM_CPU_FRQ_DEFAULT == vote_freq)
	{
		vote_freq = PM_CPU_FRQ_NONE; // it will use the PM_DEV_ID_DEFAULT vote cpu frequency
	}

	/*Appli do not need to be concerned with the specific CPU frequency*/
	if(vote_freq == PM_CPU_FRQ_HIGHEST)
	{
		vote_freq = PM_HIGHEST_CPU_FREQ;
		bcpu_freq_highest = true;
	}
	else
	{
		bcpu_freq_highest = false;
	}
	s_pm_cpu_freq[module] = vote_freq;

	/*get the max cpu freq*/
	freq_max = s_pm_cpu_freq[0];
	for (i = 1; i < PM_DEV_ID_MAX; i++)
	{
		if (freq_max < s_pm_cpu_freq[i])
		{
			freq_max = s_pm_cpu_freq[i];
			freq_max_index = i;
		}
	}
	if(bcpu_freq_highest == true)
	{
		s_pm_cpu_freq[module] = PM_CPU_FRQ_HIGHEST;
	}

	/*dpd need cpu freq 120M, when dpd calibration, it force dpd need cpu freq, if dpd cali finish, restore it*/
	if((module == PM_DEV_ID_PHY_DPD_CALI)&&(vote_freq != PM_CPU_FRQ_NONE))
	{
		freq_max = vote_freq;
	}
	else
	{
		//when dpd cali finish, it will use the highest cpu freq in votes cpu freq.
		if(freq_max == PM_CPU_FRQ_HIGHEST)
		{
			freq_max = PM_HIGHEST_CPU_FREQ;
		}
	}
	/*updete the current cpu frequency*/
	if (s_pm_current_cpu_freq == freq_max)
	{
		GLOBAL_INT_RESTORE();
		return BK_OK;
	}
	else
	{
		ret = sys_drv_switch_cpu_bus_freq_unlocked(freq_max);
	}
	if (ret == BK_OK)
	{
		if(bcpu_freq_highest == true)
		{
			s_pm_current_cpu_freq = PM_CPU_FRQ_HIGHEST;
		}
		else
		{
			s_pm_current_cpu_freq = freq_max;
		}
	}
	else
	{
		s_pm_cpu_freq[module] = previous_vote;
	}

	GLOBAL_INT_RESTORE();

	if (ret != BK_OK)
	{
		LOGI("switch cpu freq[%d] error\r\n", freq_max);
		return ret;
	}

	if (pm_debug_mode() & 0x2)
	{
		LOGI("Switch cpu freq %d %d\r\n", freq_max, freq_max_index);
	}
	return BK_OK;
}

bk_err_t bk_pm_clock_ctrl(pm_dev_clk_e module, pm_dev_clk_pwr_e clock_state)
{
	GLOBAL_INT_DECLARATION();
	GLOBAL_INT_DISABLE();
	sys_drv_dev_clk_pwr_ctrl(module, clock_state);
	GLOBAL_INT_RESTORE();
	return BK_OK;
}

/*=========================CLK/FREQ CTRL END========================*/
