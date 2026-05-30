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
#include <driver/psram.h>
#include "sys_hal.h"
#include "driver/aon_rtc.h"

#if CONFIG_AP_EMUBOOT
#include "cmsis_gcc.h"
#include "sys_sw_regs.h"
#endif

#include "bk_arch.h"

#if CONFIG_DEEP_LV
#include "deep_lv/deep_lv.h"
#include "system_star.h"
#endif

#if CONFIG_CM_BACKTRACE
#include "cm_backtrace.h"
#endif

#if CONFIG_MPU
#include "mpu.h"
#endif

#if CONFIG_WDT_EN
#include "wdt_driver.h"
#include <driver/wdt.h>
#endif

#if CONFIG_SOC_SMP
#include "multicore_driver.h"
#endif

#include "interrupt.h"

#define TAG "soc_init"

extern uint32_t relocate_vector_table(void);
extern void entry_main(void);
extern void core_init(void);

#if CONFIG_FORCE_PROTECT_CHANNEL
bk_err_t bk_mailbox_cc_init_on_current_core(int id);
#endif

#if CONFIG_SOC_SMP
void (*multicore_core1_func)(void) = NULL;

void multicore_launch_core1(void (*func)(void))
{
	multicore_core1_func = func;

    //cp smp(core0,core1) + ap smp(core0,core1)
	// bk_pm_module_vote_power_ctrl(PM_POWER_MODULE_NAME_CPU1, PM_POWER_MODULE_STATE_ON);
	bk_multicore_start(CPU1_CORE_ID);

	BK_LOGW(TAG, "%s end\n", __func__);
}

void _othercore_start(void)
{
    uint32_t core_id = portGET_CORE_ID();

    if (core_id == 1U && multicore_core1_func != NULL) {
        multicore_core1_func();
    }

    while (1) {
        BK_LOGW(TAG, "@core%d\r\n", (int)core_id);
    }
}
#endif

bool is_psram_init_done = false;

void _soc_start(void)
{
#if CONFIG_SOC_SMP
    uint32_t core_id = portGET_CORE_ID();
    if (core_id == 0) {
#endif
    relocate_vector_table();
#if CONFIG_SOC_SMP
    }
#endif

#if CONFIG_MPU
    mpu_enable();
#endif

#if CONFIG_DCACHE
    SCB_EnableDCache();
    SCB_CleanInvalidateDCache();
#endif

    core_init();
#if CONFIG_CM_BACKTRACE
    cm_backtrace_init(FIREWARE_NAME, HARDWARE_VERSION, SOFTWARE_VERSION);
#endif
#if CONFIG_SOC_SMP
    if (core_id == 0U) {
#endif
#if CONFIG_ATE_TEST && CONFIG_RESET_REASON
    extern int cmd_do_memcheck(void);
    cmd_do_memcheck();
#endif

    pm_hardware_init();

#if CONFIG_AP_EMUBOOT
// #if (CONFIG_PSRAM)
//     bk_psram_init();
//     is_psram_init_done = true;
// #endif
	bk_sys_sw_regs_ptr()->flash_init_done = BK_SYS_SW_REGS_FLASH_INIT_NOT_DONE;
	__DSB();
	flush_dcache((void *)&bk_sys_sw_regs_ptr()->flash_init_done, sizeof(bk_sys_sw_regs_ptr()->flash_init_done));
	__DSB();
	bk_start_ap_system();
#endif

    entry_main();

    while (1) {
        BK_LOGW(TAG, "@\r\n");
    };
#if CONFIG_SOC_SMP
    } else {
        #if CONFIG_FORCE_PROTECT_CHANNEL
        bk_mailbox_cc_init_on_current_core(rtos_get_core_id());
        #endif

        soc_isr_init();

        _othercore_start();
        while (1) {
            BK_LOGW(TAG, "@\r\n");
        };
    }
#endif
}

void soc_prep_data_relocation(void)
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

void soc_prep_hook(void)
{
#if CONFIG_SOC_SMP
    uint32_t core_id = portGET_CORE_ID();
    if (core_id == 0) {
#endif
    /*power ,clock, analog init*/
    sys_drv_early_init();

#if CONFIG_AP_EMUBOOT
    sys_drv_flash_set_clk_div(2);
    sys_drv_flash_cksel(1);
#endif

#if (CONFIG_INT_WDT || CONFIG_TASK_WDT)
    bk_wdt_force_feed();
#endif

    reboot_tag_init();
#if CONFIG_SOC_SMP
    }
#endif

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

#if CONFIG_DEEP_LV
/* Direct AON GPIO register access: GPIO_20 -> JTAG_TCK(SWCLK), GPIO_21 -> JTAG_TMS(SWDIO).
 * Equivalent to gpio_dev_unprotect_map(GPIO_20, GPIO_DEV_JTAG_TCK) and gpio_dev_unprotect_map(GPIO_21, GPIO_DEV_JTAG_TMS).
 * gpio_fun_sel is in cfg register bit[24:31], FUNC_CODE_SWCLK=30, FUNC_CODE_SWDIO=31.
 */
#include "soc/reg_base.h"
#define JTAG_GPIO20_CFG_ADDR   (SOC_AON_GPIO_REG_BASE + (20u * 4u))
#define JTAG_GPIO21_CFG_ADDR   (SOC_AON_GPIO_REG_BASE + (21u * 4u))
#define GPIO_FUN_SEL_SHIFT     24u
#define GPIO_FUN_SEL_MASK      0x00FFFFFFu
#define FUNC_CODE_SWCLK        30u
#define FUNC_CODE_SWDIO        31u
static inline void early_jtag_gpio_map(void)
{
	volatile uint32_t *cfg20 = (volatile uint32_t *)JTAG_GPIO20_CFG_ADDR;
	volatile uint32_t *cfg21 = (volatile uint32_t *)JTAG_GPIO21_CFG_ADDR;
	*cfg20 = (*cfg20 & GPIO_FUN_SEL_MASK) | (FUNC_CODE_SWCLK << GPIO_FUN_SEL_SHIFT);
	*cfg21 = (*cfg21 & GPIO_FUN_SEL_MASK) | (FUNC_CODE_SWDIO << GPIO_FUN_SEL_SHIFT);
}
uint32_t  g_debug_jtag_enable = 1;
#endif
void dlv_hook(void)
{
#if CONFIG_DEEP_LV
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    asm volatile ("nop");
    if (dlv_is_startup())
    {
#if CONFIG_DEEP_LV_DEBUG
        GPIO_UP(27);//1
        GPIO_DOWN(27);
		early_jtag_gpio_map();
#endif
		bk_wdt_force_feed();
		bk_rtc_update_base_time();
		uint64_t current = bk_aon_rtc_get_us();
		sys_hal_set_low_voltage_wakeup_time_us(current);

        extern uint32_t __STACK_LIMIT;
        __set_MSPLIM((uint32_t)(&__STACK_LIMIT));

        dlv_system_init();
        dlv_startup();
    }
#endif
}

void dlv_system_init(void)
{

}
