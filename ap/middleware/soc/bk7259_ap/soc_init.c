// Copyright 2020-2025 Beken
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

#include "components/log.h"
#include "components/system.h"
#include "aon_pmu_driver.h"
#include "sys_driver.h"
#include "driver/uart.h"
#include "bk_pm_internal_api.h"
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include "sdkconfig.h"
#include <reset_reason.h>
#include "cache.h"
#if CONFIG_L2_CACHE_ENABLE
#include "l2_cache.h"
#endif

#include "bk_arch.h"

#if CONFIG_DEEP_LV
#include "deep_lv.h"
#endif

#if CONFIG_CM_BACKTRACE
#include "cm_backtrace.h"
#endif

#if CONFIG_MPU
#include "mpu.h"
#endif

#include "multicore_driver.h"
#include "interrupt.h"
#include "soc_debug.h"

#define TAG "soc_init"

extern uint32_t relocate_vector_table(void);
extern void entry_main(void);
extern void core_init(void);

#if CONFIG_FORCE_PROTECT_CHANNEL
bk_err_t bk_mailbox_cc_init_on_current_core(int id);
#endif

/* L2 cache initialization */
#if CONFIG_L2_CACHE_ENABLE
static void arch_l2_cache_init(void)
{
    uint32_t core_id = rtos_get_core_id();
    
    /* Primary CPU (CPU2) initializes L2 cache */
    if (core_id == 2U) {
        if (l2_cache_init() != 0) {
            /* Initialization failed */
            return;
        }
    } else {
        /* Secondary CPU (CPU3) waits for L2 cache to be enabled */
        if (l2_cache_wait_enabled() != 0) {
            /* Timeout waiting for L2 cache enable */
            return;
        }
    }
}
#endif

#if CONFIG_SOC_SMP
void (*multicore_core1_func)(void) = NULL;
void multicore_launch_core1(void (*func)(void))
{
	multicore_core1_func = func;

#if CONFIG_CPU_HOTPLUG_BOOT_OFFLINE
	BK_LOGW(TAG, "AP CPU3 stays offline, use 'cpu online 3' to start it\n");
#else
    //cp smp(core0,core1) + ap smp(core0,core1)
	// bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU2, PM_POWER_MODULE_STATE_ON);
	bk_multicore_start(CPU3_CORE_ID);
#endif

	BK_LOGW(TAG, "%s end\n", __func__);
}

void _othercore_start(void)
{
    uint32_t core_id = portGET_CORE_ID();

    if (core_id == 1 && multicore_core1_func) {
        multicore_core1_func();
    }

    while(1) {
        BK_LOGW(TAG, "@core%d\r\n", core_id);
    }
}
#endif

void _soc_start(void)
{
    // // bk_sys_uart_write_string(0,"M55 ==> _soc_start\r\n");
#if CONFIG_SOC_SMP
    uint32_t core_id = portGET_CORE_ID();
    if (core_id == 0) {
#endif
    /* Only the primary core relocates to the SRAM vector table. The secondary
     * core keeps the vector table already selected by its boot path and must not
     * override it with the primary core's table. */
    relocate_vector_table();
#if CONFIG_SOC_SMP
    }
#endif
    // bk_sys_uart_write_string(0,"M55 ==> relocate_vector_table\r\n");
    
    /* Invalidate ICache before MPU configuration */
    cache_instr_invd_all();
    
#if CONFIG_MPU
    /* MPU must be configured after b_prep_entry_main() (data relocation) */
    /* and before cache is enabled */
    /* This ensures: */
    /* 1. Data relocation (b_data_copy) can access Flash and RAM without MPU restrictions */
    /* 2. Correct memory attributes are applied before cache is enabled */
    mpu_enable();
#endif // CONFIG_MPU
    // bk_sys_uart_write_string(0,"M55 ==> mpu_enable\r\n");
    
    /* Enable cache after MPU configuration (best practice) */
#if CONFIG_L2_CACHE_ENABLE
	/* Initialize L2 cache with multi-core coordination */
	arch_l2_cache_init();
#endif

	/* Discard stale data lines from enabled cache levels before L1 enable. */
#if CONFIG_CACHE_MAINTENANCE
	arch_dcache_invd_all();
#endif

#if CONFIG_ICACHE
	arch_icache_enable();
#endif

#if CONFIG_DCACHE
	arch_dcache_enable();
#endif

#if CONFIG_CACHE_MAINTENANCE
    flush_all_dcache();
#endif

    core_init();
    // bk_sys_uart_write_string(0,"M55 ==> core_init\r\n");

    #if CONFIG_CM_BACKTRACE
        cm_backtrace_init(FIREWARE_NAME, HARDWARE_VERSION, SOFTWARE_VERSION);
    #endif

#if CONFIG_SOC_SMP
    if (core_id == 0) {
#endif
    #if CONFIG_ATE_TEST && CONFIG_RESET_REASON
        extern int cmd_do_memcheck(void);
        cmd_do_memcheck();
    #endif

        entry_main();

        while(1){
            BK_LOGW(TAG, "@\r\n");
        };
#if CONFIG_SOC_SMP
    } else {
        #if CONFIG_FORCE_PROTECT_CHANNEL
        bk_mailbox_cc_init_on_current_core(rtos_get_core_id());
        #endif

        soc_isr_init();

        _othercore_start();
        while(1){
            BK_LOGW(TAG, "@\r\n");
        };
    }
#endif
}

__FLASH_BOOT_CODE void soc_prep_data_relocation(void)
{
#if CONFIG_SPE
/*Only spe can configure tcm permission*/
#if CONFIG_SUPPORT_ITCM
	TCM->ITCMCR |= SCB_ITCMCR_EN_Msk;
#endif

#if CONFIG_SUPPORT_DTCM
	TCM->DTCMCR |= SCB_DTCMCR_EN_Msk;
#endif
#endif
}

__FLASH_BOOT_CODE void soc_prep_hook(void)
{
//  reboot_tag_init();

#if CONFIG_SUPPORT_FPU
  /* Set low-power state for PDEPU                */
  /*  0b00  | ON, PDEPU is not in low-power state */
  /*  0b01  | ON, but the clock is off            */
  /*  0b10  | RET(ention)                         */
  /*  0b11  | OFF                                 */
  /* Clear ELPSTATE, value is 0b11 on Cold reset */
  PWRMODCTL->CPDLPSTATE &= ~(PWRMODCTL_CPDLPSTATE_ELPSTATE_Msk);
  /* Favor best FP/MVE performance by default, avoid EPU switch-ON delays */
  /* PDEPU ON, Clock OFF */
  PWRMODCTL->CPDLPSTATE |= 0x1 << PWRMODCTL_CPDLPSTATE_ELPSTATE_Pos;
#endif

  /* Enable Loop and branch info cache */
  SCB->CCR |= SCB_CCR_LOB_Msk;

}

void enable_dcache(int enable)
{
#if CONFIG_DCACHE
    if (enable == 0) {
        flush_all_dcache();
        SCB_DisableDCache();
        mpu_disable();
    } else {
        flush_all_dcache();
        mpu_enable();
        SCB_EnableDCache();
    }
#endif
}

void dlv_hook(void)
{
#if CONFIG_DEEP_LV
    if (dlv_is_startup()) {
        extern uint32_t __STACK_LIMIT;
        __set_MSPLIM((uint32_t)(&__STACK_LIMIT));
        dlv_system_init();
        dlv_startup();
    }
#endif
}
