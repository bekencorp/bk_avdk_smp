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
#include "bk_sys_ctrl.h"
#include "sys_driver.h"
#include <driver/int.h>
#include <driver/timer.h>
#include <driver/gpio.h>
#include <driver/dma.h>
#include <driver/uart.h>
#include <driver/wdt.h>
#include <driver/aon_wdt.h>
#include <driver/wwdt.h>
#include <driver/efuse.h>
#include <driver/ckmn.h>
#include <os/mem.h>
#include <driver/adc.h>
#include <driver/aon_rtc.h>
#include <modules/pm.h>
#include <driver/psram.h>
#include <driver/lin.h>
#include <driver/ipi_driver.h>
#include "bk_driver.h"
#include "interrupt_base.h"
#include <driver/otp.h>
#include <driver/pwr_clk.h>
#include "bk_api_ipc.h"

#if CONFIG_AP_EMUBOOT
#include "sys_sw_regs.h"
#endif

#if CONFIG_AON_PMU
#include "aon_pmu_driver.h"
#endif

#if CONFIG_FLASH
#include <driver/flash.h>
#endif

#if CONFIG_EASY_FLASH
#include "bk_ef.h"
#endif

#if CONFIG_CALENDAR
#include <driver/calendar.h>
#endif

#if CONFIG_ATE
#include <components/ate.h>
#endif

#if CONFIG_TOUCH_PM_SUPPORT
#include <driver/touch.h>
#endif

#if CONFIG_CHIP_SUPPORT
#include "modules/chip_support.h"
#endif

#if CONFIG_SDMADC
#include <driver/sdmadc.h>
#endif

#if CONFIG_HW_ROTATE_PFC
#include <driver/rott_driver.h>
#endif

#if CONFIG_GET_UID_ENABLE
#include <components/bk_uid.h>
#endif

#if CONFIG_HSPL
#include "hspl_driver.h"
#endif

#if CONFIG_PWM
#include <driver/pwm.h>
#endif

#if CONFIG_SUPPORT_IRDA
#include <driver/irda.h>
#endif

#if CONFIG_LIN
#include <driver/lin.h>
#endif

//TODO only init driver model and necessary drivers
#if CONFIG_POWER_CLOCK_RF
#define   MODULES_POWER_OFF_ENABLE (1)
#define   ROSC_DEBUG_EN            (0)
#define   MODULES_CLK_ENABLE       (0)
extern void clock_dco_cali(UINT32 speed);
void power_clk_rf_init()
{
    uint32_t param =0;
	/*power on all the modules for bringup test*/

	module_name_t use_module = MODULE_NAME_WIFI;
    /*1. power on all the modules*/
	#if MODULES_POWER_OFF_ENABLE
	    sys_drv_module_power_ctrl(POWER_MODULE_NAME_ENCP,POWER_MODULE_STATE_OFF);
		//sys_drv_module_power_ctrl(POWER_MODULE_NAME_BAKP,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_AUDP,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_VIDP,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_BTSP,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_WIFIP_MAC,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_WIFI_PHY,POWER_MODULE_STATE_OFF);
		sys_drv_module_power_ctrl(POWER_MODULE_NAME_CPU1,POWER_MODULE_STATE_OFF);
	#else
	    power_module_name_t module = POWER_MODULE_NAME_MEM1;
        for(module = POWER_MODULE_NAME_MEM1 ; module < POWER_MODULE_NAME_NONE ; module++)
        {
            sys_drv_module_power_ctrl(module,POWER_MODULE_STATE_ON);
        }
    #endif
    /*2. enable the analog clock*/
    sys_drv_module_RF_power_ctrl(use_module ,POWER_MODULE_STATE_ON);

	/*3.enable all the modules clock*/
	#if MODULES_CLK_ENABLE
	dev_clk_pwr_id_t devid = 0;
	for(devid = 0; devid < 32; devid++)
	{
	    bk_pm_clock_ctrl(devid, CLK_PWR_CTRL_PWR_UP);
    }
	#endif

	/*tempreture det enable for VIO*/
	param = 0;
	param = sys_drv_analog_get(ANALOG_REG6);
	param |= (0x1 << SYS_ANA_REG6_EN_TEMPDET_POS)|(0x7 << SYS_ANA_REG6_RXTAL_LP_POS)|(0x7 << SYS_ANA_REG6_RXTAL_HP_POS);
	param &= ~(0x1 << SYS_ANA_REG6_EN_SLEEP_POS);
	sys_drv_analog_set(ANALOG_REG6, param);

	/*let rosc to bt/wifi ip*/
	param = 0;
	param = aon_pmu_drv_reg_get(PMU_REG0x41);
	param |= 0x1 << 24;
	aon_pmu_drv_reg_set(PMU_REG0x41,param);
	/*wake delay of Xtal*/

	/* rosc calibration start*/
	/*a.open rosc debug*/
#if ROSC_DEBUG_EN
	param = 0;
	param = sys_drv_analog_get(ANALOG_REG5);
	param |= 0x1 << SYS_ANA_REG5_CK_TST_ENBALE_POS;
	sys_drv_analog_set(ANALOG_REG5,param);

	param = 0;
	param = sys_drv_analog_get(ANALOG_REG11);
	param |= 0x1 << SYS_ANA_REG11_TEST_EN_POS;
	sys_drv_analog_set(ANALOG_REG11,param);

	param = 0;
	param = sys_drv_analog_get(ANALOG_REG4);
	param |= (0x1 << SYS_ANA_REG4_ROSC_TSTEN_POS);//Rosc test enable
	sys_drv_analog_set(ANALOG_REG4,param);
#endif

	/*b.config calibration*/
	param = 0;
	param = sys_drv_analog_get(ANALOG_REG4);
	param &= ~(SYS_ANA_REG4_ROSC_CAL_INTVAL_MASK << SYS_ANA_REG4_ROSC_CAL_INTVAL_POS);//clear the data
	param |= 0x4 << SYS_ANA_REG4_ROSC_CAL_INTVAL_POS;//Rosc Calibration Interlval 0.25s~2s (4:1s)
	param |= 0x1 << SYS_ANA_REG4_ROSC_CAL_MODE_POS;//0x1: 32K ;0x0: 31.25K
	param |= 0x1 << SYS_ANA_REG4_ROSC_CAL_EN_POS;//Rosc Calibration Enable
	param &= ~(0x1 << SYS_ANA_REG4_ROSC_MANU_EN_POS);//0:close Rosc Calibration Manual Mode
	sys_drv_analog_set(ANALOG_REG4,param);

	/*c.trigger calibration*/
	param = 0;
	param = sys_drv_analog_get(ANALOG_REG4);
	param &= ~(0x1 << SYS_ANA_REG4_ROSC_CAL_TRIG_POS);//trigger clear
	sys_drv_analog_set(ANALOG_REG4,param);

	param = 0;
	param = sys_drv_analog_get(ANALOG_REG4);
	param |= (0x1 << SYS_ANA_REG4_ROSC_CAL_TRIG_POS);//trigger enable
	sys_drv_analog_set(ANALOG_REG4,param);
}
#endif

int driver_early_init(void)
{
#if (CONFIG_WAKEUP)
	bk_wakeup_driver_init();
#endif

#if CONFIG_AON_PMU
	aon_pmu_drv_init();
#endif

#if CONFIG_POWER_CLOCK_RF
	power_clk_rf_init();
#endif

#if CONFIG_EFUSE
	bk_efuse_driver_init();
#endif

#if CONFIG_AON_RTC
	bk_aon_rtc_driver_init();
#endif

#if CONFIG_HSPL
	bk_hspl_driver_init();
#endif

#if CONFIG_IPI
	bk_ipi_driver_init();
#endif

	/* Console (+ WWDT driver only): after clock/HSPL; bk_wwdt_start() in system_main before scheduler. */
	sys_drv_init();
	bk_gpio_driver_init();
	// Important notice!!!!!
	// ATE uses UART TX PIN as the detect ATE mode pin,
	// so it should be called after GPIO init and before UART init.
	// or caused ATE can't work or UART can't work
#if CONFIG_ATE
	bk_ate_init();
#endif
	// Important notice!
	// Before UART is initialized, any call of BK_LOG_RAW/os_print/BK_LOGx may
	// cause problems, such as crash etc!
	bk_uart_driver_init();
#if CONFIG_SUPPORT_WWDT
	bk_wwdt_driver_init();
#endif

#if CONFIG_CKMN
	bk_ckmn_driver_init();
#endif

	return 0;
}

int driver_init(void) {
	/* sys/gpio/ate/uart/wwdt driver init were migrated to driver_early_init(). */

#if CONFIG_CHIP_SUPPORT
	if(!bk_is_chip_supported()) {
		return BK_FAIL;
	}
#endif

#if CONFIG_TIMER
	bk_timer_driver_init();
#endif

#if CONFIG_GENERAL_DMA
	bk_dma_driver_init();
#endif

#if (CONFIG_INT_WDT || CONFIG_TASK_WDT)
	bk_wdt_driver_init();
#endif

// #if CONFIG_AON_WDT && !CONFIG_INT_AON_WDT
// 	bk_aon_wdt_stop();
// #endif

#if CONFIG_GET_UID_ENABLE
	/* Init UID (create lock + read OTP once + publish snapshot) BEFORE mailbox/IPC
	 * comes up. The AP GET_CHIP_UID RPC is served from the IPC worker thread that
	 * ipc_init() creates below, so doing this first guarantees the OTP is read
	 * exactly once while still single-threaded and the snapshot is published
	 * before any RPC can arrive. */
	bk_uid_driver_init();
#endif

#if CONFIG_MAILBOX
	extern bk_err_t ipc_init(void);
	extern bk_err_t mb_ipc_init(void);
	ipc_init();
#if CONFIG_MAILBOX_IPC
	mb_ipc_init();
#endif
	bk_ipc_init();
#if CONFIG_SLAVE_HEART_BEAT
	extern bk_err_t mb_ipc_heartbeat_init(void);
	mb_ipc_heartbeat_init();
#endif
#endif

	os_show_memory_config_info();

#if CONFIG_FLASH
#if CONFIG_AP_EMUBOOT
	bk_sys_sw_regs_set_flash_init_done(BK_SYS_SW_REGS_FLASH_INIT_NOT_DONE);

	if (bk_flash_driver_init() != BK_OK) {
		BK_LOGE(NULL, "cp flash driver init failed\r\n");
		return BK_FAIL;
	}

	
	bk_sys_sw_regs_set_flash_init_done(BK_SYS_SW_REGS_FLASH_INIT_DONE);
#else
	bk_flash_driver_init();
#endif
#endif

#if CONFIG_EASY_FLASH
	easyflash_init();
#endif

#if CONFIG_SARADC
	bk_adc_driver_init();
#endif

#if CONFIG_CALENDAR
	bk_calendar_driver_init();
#endif

//call it after LOG is valid.
#if CONFIG_ATE
	BK_LOGD(NULL,"ate enabled is %d\r\n", ate_is_enabled());
#endif

#if CONFIG_TOUCH_PM_SUPPORT
	bk_touch_pm_init();
#endif

#if CONFIG_SDMADC
	//bk_sdmadc_driver_init();
#endif

#if CONFIG_HW_ROTATE_PFC
//	bk_rott_driver_init();
#endif

#if CONFIG_LIN
	bk_lin_driver_init();
#endif

#if (CONFIG_TRUSTENGINE)
	extern int dubhe_driver_init( unsigned long dbh_base_addr );
	dubhe_driver_init(SOC_SHANHAI_BASE);
#endif

#if CONFIG_USB //&& CONFIG_MENTOR_USB
	bk_usb_driver_init();
#endif

#if CONFIG_PWM
	bk_pwm_driver_init();
#endif

#if CONFIG_SUPPORT_IRDA
	bk_irda_driver_init();
#endif
#if !CONFIG_PM_AP_POWERDOWN_WHEN_LV
#if (CONFIG_PSRAM)
	extern bool is_psram_init_done;
	if (!is_psram_init_done) {
		bk_psram_init();
		is_psram_init_done = true;
	}
#endif
#endif
	BK_LOGD(NULL,"driver_init end\r\n");

	return 0;
}

