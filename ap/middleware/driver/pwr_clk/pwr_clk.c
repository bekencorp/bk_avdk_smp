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
#include <driver/pwr_clk.h>
#if CONFIG_MAILBOX
#include <driver/mailbox_channel.h>
#endif
#include <modules/pm.h>
#include "sys_driver.h"
#if CONFIG_PSRAM
#include <driver/psram.h>
#endif
#include <os/mem.h>
#include "sys_types.h"
#include <driver/aon_rtc.h>
#include <os/os.h>
#include <driver/psram.h>
#include <components/system.h>
#include "bk_pm_internal_api.h"
#include <common/bk_kernel_err.h>
#include "aon_pmu_hal.h"
#include "driver/pm_ap_core.h"

/*=====================DEFINE  SECTION  START=====================*/
#define TAG "AP"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define PM_SEND_CMD_CP1_RESPONSE_TIEM        (100)  //100ms
#define PM_BOOT_CP1_WAITING_TIEM             (500) // 0.5s
#define PM_CP1_RECOVERY_DEFAULT_VALUE        (0xFFFFFFFFFFFFFFFF)
#define PM_OPEN_CP1_TIMEOUT                  (20000) //20s
#define PM_SEMA_WAIT_FOREVER                 (0xFFFFFFFF)    /*Wait Forever*/

#define PM_BOOT_CP1_TRY_COUNT                (3)


/*=====================DEFINE  SECTION  END=====================*/

/*=====================VARIABLE  SECTION  START=================*/

#if CONFIG_MAILBOX
static volatile  pm_mailbox_communication_state_e s_pm_cp2_rtc_deepsleep_finish  = 0;
static volatile  pm_mailbox_communication_state_e s_pm_ap_get_cp_data_finish     = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp2_ctrl_state_finish     = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp2_deepsleep_finish      = 0;
static volatile  pm_mailbox_communication_state_e s_pm_cp2_wakeup_src_cfg_finish = 0;

//static mb_chnl_cmd_t                              s_pm_mb_data                   = {0};
#endif
static volatile  uint32_t                         s_pm_cp1_boot_try_count        = 0;

/*=====================VARIABLE  SECTION  END=================*/

/*================FUNCTION DECLARATION  SECTION  START========*/

#if CONFIG_MAILBOX
static void pm_cp1_mailbox_init();
static void pm_cp1_mailbox_rx_isr(int *pm_mb, mb_chnl_cmd_t *cmd_buf);
#endif


/*================FUNCTION DECLARATION  SECTION  END========*/

pm_lpo_src_e bk_clk_32k_customer_config_get(void)
{
#if CONFIG_LPO_MP_A_FORCE_USE_EXT32K
	uint32_t chip_id = aon_pmu_hal_get_chipid();
	if ((chip_id & PM_CHIP_ID_MASK) == (PM_CHIP_ID_MP_A & PM_CHIP_ID_MASK))
	{
		return PM_LPO_SRC_X32K;
	}
	else
	{
		#if CONFIG_EXTERN_32K
			return PM_LPO_SRC_X32K;
		#elif CONFIG_LPO_SRC_26M32K
			return PM_LPO_SRC_DIVD;
		#else
			return PM_LPO_SRC_ROSC;
		#endif
	}
#else
	#if CONFIG_EXTERN_32K
		return PM_LPO_SRC_X32K;
	#elif CONFIG_LPO_SRC_26M32K
		return PM_LPO_SRC_DIVD;
	#else
		return PM_LPO_SRC_ROSC;
	#endif
#endif
	return PM_LPO_SRC_ROSC;
}