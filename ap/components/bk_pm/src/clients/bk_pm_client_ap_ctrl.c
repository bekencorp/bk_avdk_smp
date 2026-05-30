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

#include <common/bk_include.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <sys_sw_regs.h>
#include "cache.h"
#include "pm_debug.h"
#include "bk_pm_internal_api.h"

#define PM_SEND_CMD_CP1_RESPONSE_TIEM        (100)  //100ms

#if CONFIG_MAILBOX
pm_mailbox_communication_state_e bk_pm_ap_ctrl_state_get(void);
bk_err_t bk_pm_ap_ctrl_state_set(pm_mailbox_communication_state_e state);
#endif

bk_err_t bk_pm_module_vote_boot_ap_ctrl(pm_boot_ap_module_name_e module,pm_power_module_state_e power_state)
{
#if CONFIG_MAILBOX
    uint64_t previous_tick  = 0;
    uint64_t current_tick   = 0;
    bk_err_t ret            = 0;
    bk_pm_ap_ctrl_state_set(PM_MAILBOX_COMMUNICATION_INIT);

    ret = pm_cp1_mailbox_send_data(PM_CTRL_AP_STATE_CMD, module,power_state,0);
    if(ret != BK_OK)
    {
        return BK_FAIL;
    }

    previous_tick = pm_cp1_aon_rtc_counter_get();
    current_tick = previous_tick;
    while((current_tick - previous_tick) < (PM_SEND_CMD_CP1_RESPONSE_TIEM*PM_AON_RTC_DEFAULT_TICK_COUNT))
    {
        if (bk_pm_ap_ctrl_state_get()) // wait the cp0 response
        {
            break;
        }
        current_tick = pm_cp1_aon_rtc_counter_get();
    }

    if(!bk_pm_ap_ctrl_state_get())
    {
        LOGE("ap vote ctrl ap time out\r\n");
    }
#endif//CONFIG_MAILBOX

    return BK_OK;
}

bool bk_pm_ap_first_boot_get(void)
{
	pm_shared_info_t shared_info = {0};

	bk_sys_sw_regs_get_pm_shared_info(&shared_info);
	return (shared_info.pm_ap_work_state & PM_AP_WORK_STATE_FIRST_BOOT) != 0;
}

bk_err_t __attribute__((weak)) bk_pm_ap_boot_success_set(bool boot_success)
{
	pm_shared_info_t shared_info = {0};

	bk_sys_sw_regs_get_pm_shared_info(&shared_info);
	if (boot_success) {
		shared_info.pm_ap_work_state |= PM_AP_WORK_STATE_BOOT_SUCCESS;
	} else {
		shared_info.pm_ap_work_state &= (uint8_t)~PM_AP_WORK_STATE_BOOT_SUCCESS;
	}
	bk_sys_sw_regs_update_pm_shared_info(&shared_info, BK_SYS_SW_REGS_PM_SHARED_INFO_FIELD_AP_WORK_STATE, BK_SYS_SW_REGS_LOCK_ENABLE);
	__DSB();
	flush_dcache((void *)&bk_sys_sw_regs_ptr()->pm_shared_info, sizeof(bk_sys_sw_regs_ptr()->pm_shared_info));
	__DSB();
	return BK_OK;
}