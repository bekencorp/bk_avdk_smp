/**
 * @if copyright_display
 *      Copyright (C), 2018-2021, Arm Technology (China) Co., Ltd.
 *      All rights reserved
 *
 *      The content of this file or document is CONFIDENTIAL and PROPRIETARY
 *      to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 *      License Agreement between Licensee and Arm Technology (China) Co., Ltd
 *      restricting among other things, the use, reproduction, distribution
 *      and transfer.  Each of the embodiments, including this information and
 *      any derivative work shall retain this copyright notice.
 * @endif
 */

#include <stdint.h>
#include "pal_log.h"
#include "dubhe_event.h"
#include "dubhe_regs.h"
#include "dubhe_aca.h"
#include "dubhe_sca.h"
#include "dubhe_hash.h"
#include "dubhe_trng.h"
#include "dubhe_otp.h"
#include "dubhe_driver.h"
#include "system_hw.h"
#include "bk_misc.h"
#include <driver/int.h>
#include "sys_driver.h"
#if !defined(TEE_M)
#include <modules/pm.h>
#if !defined(DUBHE_SECURE)
#include "arch_interrupt.h"
#include "sdkconfig.h"
#if defined(CONFIG_TFM_REG_ACCESS_NSC) && CONFIG_TFM_REG_ACCESS_NSC
/* Avoid including tfm_reg_nsc.h here: tfm/interface also ships mbedtls/ headers. */
uint32_t psa_reg_read(uint32_t addr);
void psa_reg_write(uint32_t addr, uint32_t value);
#endif
#endif
#endif

unsigned long _g_Dubhe_RegBase;
static int do_dubhe_driver_init( unsigned long dbh_base_addr );
static void dubhe_delay_us(uint32 num);
bool dubhe_inited = false;
#if !defined(TEE_M) && !defined(DUBHE_SECURE)
static bool s_ns_runtime_ready;

static void dubhe_ns_clear_engine_intr( void )
{
    uint32_t st;

    if (_g_Dubhe_RegBase == 0) {
        return;
    }

    st = DBH_READ_REGISTER( SCA, SCA_INTR_STAT );
    if (st) {
        DBH_WRITE_REGISTER( SCA, SCA_INTR_STAT, st );
    }
    st = DBH_READ_REGISTER( HASH, HASH_INTR_STAT );
    if (st) {
        DBH_WRITE_REGISTER( HASH, HASH_INTR_STAT, st );
    }
    st = DBH_READ_REGISTER( ACA, ACA_INTR_STAT );
    if (st) {
        DBH_WRITE_REGISTER( ACA, ACA_INTR_STAT, st );
    }
}

/* TOP_CTRL is Secure-only; NS enables clocks via SPE NSC. */
static int dubhe_ns_ensure_clocks( void )
{
#if defined(CONFIG_TFM_REG_ACCESS_NSC) && CONFIG_TFM_REG_ACCESS_NSC
    uint32_t te_s = (uint32_t)SOC_GET_S_ADDR(SOC_SHANHAI_BASE);
    uint32_t clk_addr = te_s + DBH_BASE_TOP_CTRL + DBH_CLK_CTRL_REG_OFFSET;
    uint32_t clk;

    clk = psa_reg_read(clk_addr);
    clk |= (1UL << DBH_CLK_CTRL_HASH_EN_BIT_SHIFT)
         | (1UL << DBH_CLK_CTRL_SCA_EN_BIT_SHIFT)
         | (1UL << DBH_CLK_CTRL_ACA_EN_BIT_SHIFT)
         | (1UL << DBH_CLK_CTRL_TRNG_EN_BIT_SHIFT);
    clk &= ~(1UL << DBH_CLK_CTRL_DMA_AHB_EN_BIT_SHIFT);
    psa_reg_write(clk_addr, clk);
    dubhe_delay_us(10);
    return 0;
#else
    return -1;
#endif
}
#endif

static void dubhe_delay_us(uint32 num) {

	volatile uint32 i, j, us_count;
	us_count = 4;
	for (i = 0; i < num; i++)
		for (j = 0; j < us_count; j++)
			;

}

static void dubhe_dma_disable(void)
{
    uint32_t value = 0;

    value = DBH_READ_REGISTER( TOP_CTRL, CLK_CTRL );
    DBH_REG_FLD_SET( CLK_CTRL, DMA_AHB_EN, value, 0 );
    DBH_WRITE_REGISTER( TOP_CTRL, CLK_CTRL, value );
}

int dubhe_clk_enable( dubhe_module_type_t type )
{
    int ret        = 0;
    uint32_t value = 0;

    value = DBH_READ_REGISTER( TOP_CTRL, CLK_CTRL );
    switch ( type ) {
    case DBH_MODULE_HASH:
        DBH_REG_FLD_SET( CLK_CTRL, HASH_EN, value, 1 );
        break;
    case DBH_MODULE_SCA:
        DBH_REG_FLD_SET( CLK_CTRL, SCA_EN, value, 1 );
        break;
    case DBH_MODULE_ACA:
        DBH_REG_FLD_SET( CLK_CTRL, ACA_EN, value, 1 );
        break;
    case DBH_MODULE_OTP:
        DBH_REG_FLD_SET( CLK_CTRL, OTP_EN, value, 1 );
        break;
    case DBH_MODULE_TRNG:
        DBH_REG_FLD_SET( CLK_CTRL, TRNG_EN, value, 1 );
        break;
    default:
        PAL_LOG_ERR( "invalid dubhe module %d\n", type );
        ret = -1;
        return ret;
    }

    DBH_WRITE_REGISTER( TOP_CTRL, CLK_CTRL, value );

    return ret;
}

int dubhe_clk_disable( dubhe_module_type_t type )
{
    int ret        = 0;
    uint32_t value = 0;

    value = DBH_READ_REGISTER( TOP_CTRL, CLK_CTRL );
    switch ( type ) {
    case DBH_MODULE_HASH:
        DBH_REG_FLD_SET( CLK_CTRL, HASH_EN, value, 0 );
        break;
    case DBH_MODULE_SCA:
        DBH_REG_FLD_SET( CLK_CTRL, SCA_EN, value, 0 );
        break;
    case DBH_MODULE_ACA:
        DBH_REG_FLD_SET( CLK_CTRL, ACA_EN, value, 0 );
        break;
    case DBH_MODULE_OTP:
        DBH_REG_FLD_SET( CLK_CTRL, OTP_EN, value, 0 );
        break;
    case DBH_MODULE_TRNG:
        DBH_REG_FLD_SET( CLK_CTRL, TRNG_EN, value, 0 );
        break;
    default:
        PAL_LOG_ERR( "invalid dubhe module %d\n", type );
        ret = -1;
        return ret;
    }

    DBH_WRITE_REGISTER( TOP_CTRL, CLK_CTRL, value );

    return ret;
}

#if !defined(TEE_M) && defined(DUBHE_SECURE)
static int dubhe_lv_enter(uint64_t sleep_time, void *args)
{
    dubhe_driver_cleanup();
    return 0;
}

static int dubhe_lv_exit(uint64_t sleep_time, void *args)
{
    do_dubhe_driver_init(SOC_SHANHAI_BASE);
    return 0;
}

static void dubhe_lv_init(void)
{
    pm_cb_conf_t enter = {dubhe_lv_enter, NULL};
    pm_cb_conf_t exit = {dubhe_lv_exit, NULL};
    bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_SECURE_WORLD, &enter, &exit);
}
#endif

static void __BK_IRQ te200_isr(void)
{
#if !defined(TEE_M) && !defined(DUBHE_SECURE)
	/* Before runtime ready, keep NSEC masked; avoid Default_Handler WFI loop. */
	if (!s_ns_runtime_ready) {
		sys_drv_int_disable(ENCP_NSEC_INTERRUPT_CTRL_BIT);
		arch_int_disable_irq(INT_SRC_ENC_NSEC);
		arch_int_clear_pending_irq(INT_SRC_ENC_NSEC);
		return;
	}
#endif
	extern int dubhe_intr_handler( void );
	dubhe_intr_handler();
}

static int do_dubhe_driver_init( unsigned long dbh_base_addr )
{
    uint32_t int_level = rtos_disable_int();

#if defined(TEE_M)
    if (sys_ll_get_cpu_power_sleep_wakeup_pwd_encp() != 0) {
        sys_ll_set_cpu_power_sleep_wakeup_pwd_encp(0);
    }
#elif defined(DUBHE_SECURE)
    bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_ENCP_TRUSTENGINE, PM_POWER_MODULE_STATE_ON);
#endif
    dubhe_delay_us(100);

    _g_Dubhe_RegBase = dbh_base_addr;

#if defined(DUBHE_SECURE)
    dubhe_dma_disable();

#if defined( ARM_CE_DUBHE_ACA )
    dubhe_aca_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_HASH )
    dubhe_clk_enable( DBH_MODULE_HASH );
    arm_ce_hash_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_SCA )
    arm_ce_sca_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_TRNG )
    arm_ce_trng_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_OTP )
    arm_ce_otp_driver_init( );
#endif

#if defined( DUBHE_FOR_RUNTIME )
    dubhe_event_init( );
#endif

#if !defined(TEE_M)
    bk_int_isr_register(INT_SRC_ENC_SEC,  te200_isr,  NULL);
    sys_drv_int_enable(ENCP_SEC_INTERRUPT_CTRL_BIT);
#endif

#else /* Normal channel: SPE owns power/TOP_CTRL; defer engine bring-up to first use */

#if !defined(TEE_M)
    /*
     * arch_int_init_all_irq() enables every NVIC line. ENC_NSEC vector is still
     * Default_Handler until registered; a pending Normal-channel IRQ would WFI-loop.
     * Install ISR early, then keep both SYS source and NVIC masked until first use.
     */
    s_ns_runtime_ready = false;
    sys_drv_int_disable(ENCP_NSEC_INTERRUPT_CTRL_BIT);
    bk_int_isr_register(INT_SRC_ENC_NSEC, te200_isr, NULL);
    arch_int_disable_irq(INT_SRC_ENC_NSEC);
    arch_int_clear_pending_irq(INT_SRC_ENC_NSEC);
#endif

#endif /* DUBHE_SECURE */

    rtos_enable_int(int_level);
    return 0;
}

int dubhe_driver_init( unsigned long dbh_base_addr )
{
    bool need_init_driver = false;
    int ret = 0;

    if (dubhe_inited == false) {
        dubhe_inited = true;
        need_init_driver = true;

#if !defined(TEE_M) && defined(DUBHE_SECURE)
        dubhe_lv_init();
        bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_ENCP_TRUSTENGINE, PM_POWER_MODULE_STATE_ON);
#elif defined(TEE_M)
        if (sys_ll_get_cpu_power_sleep_wakeup_pwd_encp() != 0) {
            sys_ll_set_cpu_power_sleep_wakeup_pwd_encp(0);
        }
#endif
    }

#if defined(TEE_M)
    if ((need_init_driver == true) || (sys_ll_get_cpu_power_sleep_wakeup_pwd_encp() != 0)) {
#else
    if (need_init_driver == true) {
#endif
        ret = do_dubhe_driver_init(dbh_base_addr);
    }

    return ret;
}

#if !defined(TEE_M) && !defined(DUBHE_SECURE)
void dubhe_ns_prepare_runtime( void )
{
    uint32_t int_level;

    if (s_ns_runtime_ready) {
        return;
    }

    int_level = rtos_disable_int();

    if (dubhe_ns_ensure_clocks() != 0) {
        PAL_LOG_ERR("ns te: ensure clocks failed, continue\n");
    }

#if defined( DUBHE_FOR_RUNTIME )
    dubhe_event_init( );
#endif

#if defined( ARM_CE_DUBHE_ACA )
    dubhe_aca_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_HASH )
    arm_ce_hash_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_SCA )
    arm_ce_sca_driver_init( );
#endif
#if defined( ARM_CE_DUBHE_TRNG )
    arm_ce_trng_driver_init( );
#endif

    dubhe_ns_clear_engine_intr();
    arch_int_clear_pending_irq(INT_SRC_ENC_NSEC);
    arch_int_enable_irq(INT_SRC_ENC_NSEC);
    sys_drv_int_enable(ENCP_NSEC_INTERRUPT_CTRL_BIT);
    s_ns_runtime_ready = true;
    rtos_enable_int(int_level);
}
#endif

void dubhe_driver_cleanup( void )
{

#if !defined(TEE_M)
#if defined(DUBHE_SECURE)
    sys_drv_int_disable(ENCP_SEC_INTERRUPT_CTRL_BIT);
    bk_int_isr_unregister(INT_SRC_ENC_SEC);
#else
    sys_drv_int_disable(ENCP_NSEC_INTERRUPT_CTRL_BIT);
    bk_int_isr_unregister(INT_SRC_ENC_NSEC);
    s_ns_runtime_ready = false;
#endif
#endif

#if defined( ARM_CE_DUBHE_ACA )
    dubhe_aca_driver_cleanup( );
    extern void mbedtls_dubhe_cleanup( );
    mbedtls_dubhe_cleanup( );
#endif

    dubhe_event_cleanup( );
#if !defined(TEE_M) && defined(DUBHE_SECURE)
    bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_ENCP_TRUSTENGINE, PM_POWER_MODULE_STATE_OFF);
#endif
    dubhe_delay_us(100);
}

/*************************** The End Of File*****************************/
