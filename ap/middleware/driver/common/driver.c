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
#include <driver/pwm.h>
#include <driver/timer.h>
#include <driver/gpio.h>
#include <driver/dma.h>
#include <driver/uart.h>
#include <driver/wwdt.h>
#include <driver/efuse.h>
#include <driver/ckmn.h>
#include <os/mem.h>
#include <driver/adc.h>
#include <driver/spi.h>
#include <driver/i2c.h>
#include <driver/aon_rtc.h>
#include <modules/pm.h>
#include <driver/psram.h>
#include <driver/lin.h>
#include <driver/ipi_driver.h>
#include "bk_driver.h"
#include "interrupt_base.h"
#include "bk_wdt.h"
#include <driver/otp.h>
#include <driver/pwr_clk.h>
#include "bk_rtos_debug.h"
#if CONFIG_AP_EMUBOOT
#include "bk_misc.h"
#include "sys_sw_regs.h"
#endif
#if CONFIG_SARADC_MB
#include "saradc_client.h"
#endif
#if CONFIG_PHY_MB
#include "phy_client.h"
#endif
#include "bk_api_ipc.h"

#if CONFIG_SECURITY
#include "bk_security.h"
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

#if CONFIG_FLASH && CONFIG_AP_EMUBOOT
#define CP_FLASH_INIT_WAIT_TIMEOUT_US 5000000U
#define CP_FLASH_INIT_WAIT_POLL_US    100U

static bk_err_t wait_for_cp_flash_init_done(void)
{
	uint32_t waited_us = 0;

	while (bk_sys_sw_regs_get_flash_init_done() != BK_SYS_SW_REGS_FLASH_INIT_DONE) {
		if (waited_us >= CP_FLASH_INIT_WAIT_TIMEOUT_US) {
			BK_LOGE(NULL, "wait cp flash init timeout\r\n");
			return BK_FAIL;
		}

		bk_delay_us(CP_FLASH_INIT_WAIT_POLL_US);
		waited_us += CP_FLASH_INIT_WAIT_POLL_US;
	}

	return BK_OK;
}
#endif

#if CONFIG_SDCARD
#include "sdio_storage_driver.h"
#endif

#if CONFIG_QSPI
#include <driver/qspi.h>
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

#if CONFIG_HIGH_PERFORMANCE_DMA
#include <driver/hpdma.h>
#endif

#if CONFIG_GET_UID_ENABLE
#include <components/bk_uid.h>
#endif

#if CONFIG_HSPL
#include "hspl_driver.h"
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
    set_ap_startup_index(AP_ENTER_DRIVER_EARLY_INIT);

#if CONFIG_AON_PMU
	aon_pmu_drv_init();
#endif

#if CONFIG_POWER_CLOCK_RF
	power_clk_rf_init();
#endif

#if CONFIG_HSPL
	bk_hspl_driver_init();
#endif

#if CONFIG_IPI
	bk_ipi_driver_init();
#endif

	/* Keep AP UART/GPIO/ATE in driver_init for AP_EMUBOOT wait barrier; only WWDT driver init is early. */
#if CONFIG_SUPPORT_WWDT
	bk_wwdt_driver_init();
#endif

    set_ap_startup_index(AP_EXIT_DRIVER_EARLY_INIT);
	return 0;
}

int driver_init(void) {
	sys_drv_init();

#if CONFIG_FLASH && CONFIG_AP_EMUBOOT
	/* CP programs flash / mux first; AP must not touch UART/IPC/flash until CP sets
	 * flash_init_done. A blind long delay used to mask this race; wait explicitly here. */
	if (wait_for_cp_flash_init_done() != BK_OK) {
		return BK_FAIL;
	}
#endif

	bk_gpio_driver_init();

	//Important notice!!!!!
	//ATE uses UART TX PIN as the detect ATE mode pin,
	//so it should be called after GPIO init and before UART init.
	//or caused ATE can't work or UART can't work
#if CONFIG_ATE
	bk_ate_init();
#endif

	//Important notice!
	//Before UART is initialized, any call of BK_LOG_RAW/os_print/BK_LOGx may
	//cause problems, such as crash etc!
	bk_uart_driver_init();

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

#if CONFIG_HIGH_PERFORMANCE_DMA
	bk_hpdma_driver_init();
#endif

#if (CONFIG_TASK_WDT)
	bk_task_wdt_driver_init();
#endif

#if CONFIG_MAILBOX
	extern bk_err_t ipc_init(void);
	extern bk_err_t mb_ipc_init(void);
	ipc_init();
#if CONFIG_MAILBOX_IPC
	mb_ipc_init();
#endif
	bk_ipc_init();
	bk_pm_mailbox_init();
#if CONFIG_SLAVE_HEART_BEAT
	extern bk_err_t mb_ipc_heartbeat_init(void);
	mb_ipc_heartbeat_init();
#endif
#endif

	os_show_memory_config_info();

#if CONFIG_FLASH
#if CONFIG_AP_EMUBOOT
	if (bk_flash_driver_init() != BK_OK) {
		BK_LOGE(NULL, "ap flash driver init failed\r\n");
		return BK_FAIL;
	}
#else
	bk_flash_driver_init();
#endif
#endif

#if CONFIG_EASY_FLASH
	easyflash_init();
#endif

#if CONFIG_SECURITY
	bk_secrity_init();
#endif

#if CONFIG_PWM
	bk_pwm_driver_init();
#endif

#if CONFIG_SARADC && CONFIG_SARADC_MB
	bk_saradc_driver_init();
#endif

#if CONFIG_PHY_MB
	bk_phy_driver_init();
#endif

#if CONFIG_SPI
	bk_spi_driver_init();
#endif

#if CONFIG_I2C
	bk_i2c_driver_init();
#endif

#if CONFIG_QSPI
	bk_qspi_driver_init();
#endif

#if CONFIG_AON_RTC_KEEP_TIME_SUPPORT
extern void aon_rtc_update_boot_time();
	aon_rtc_update_boot_time();
#endif

#if ((CONFIG_SDCARD))
{
	sdio_dwc_interrupt_init();
}
#endif

#if CONFIG_CALENDAR
	bk_calendar_driver_init();
#endif

//call it after LOG is valid.
#if CONFIG_ATE
	BK_LOGD(NULL, "ate enabled is %d\r\n", ate_is_enabled());
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

#if CONFIG_CKMN
	bk_ckmn_driver_init();
#endif
#if CONFIG_LIN
	bk_lin_driver_init();
#endif

#if (CONFIG_MBEDTLS_ACCELERATOR || CONFIG_TRUSTENGINE)
	extern int dubhe_driver_init( unsigned long dbh_base_addr );
	dubhe_driver_init(SOC_SHANHAI_BASE);
#endif

#if CONFIG_GET_UID_ENABLE
	bk_uid_driver_init();
#endif
	BK_LOGD(NULL, "driver_init end\r\n");

	return 0;
}

