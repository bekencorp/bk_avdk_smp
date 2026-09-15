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
#include "interrupt.h"
#include <soc/bk7259/int_types_impl.h>
#if defined(DUBHE_SECURE)
#include <modules/pm.h>
#endif

unsigned long _g_Dubhe_RegBase;
static int do_dubhe_driver_init( unsigned long dbh_base_addr );
bool dubhe_inited = false;
#if !defined(DUBHE_SECURE)
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

#if defined(DUBHE_SECURE)
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

#if CONFIG_PM_AP_FAST_BOOT_ENABLE
static bool s_ap_fast_suspended = false;

extern bk_err_t bk_pm_module_vote_cp_power_ctrl(pm_power_module_name_e module,
                                                pm_power_module_state_e power_state);

static void dubhe_vote_cp_encp_power(pm_power_module_state_e state)
{
    bk_pm_module_vote_cp_power_ctrl(PM_POWER_SUB_DOMAIN_SHANHAI, state);
}

static bk_err_t dubhe_ap_fast_quiesce(void *arg)
{
    (void)arg;

    /* Idempotent: the framework may re-run quiesce after an aborted suspend,
     * and a second cleanup would underflow the unsigned nest count. */
    if (s_ap_fast_suspended) {
        return BK_OK;
    }

    dubhe_driver_cleanup();
    s_ap_fast_suspended = true;
    return BK_OK;
}

static bk_err_t dubhe_ap_fast_resume(void *arg)
{
    (void)arg;

    if (!s_ap_fast_suspended) {
        return BK_OK;
    }

    s_ap_fast_suspended = false;
    /* Re-assert the CP-held ENCP vote before touching any TE200 register.
     * Idempotent, and this callback runs in the PM task with interrupts on. */
    dubhe_vote_cp_encp_power(PM_POWER_MODULE_STATE_ON);
    do_dubhe_driver_init(SOC_SHANHAI_BASE);
    return BK_OK;
}

/* Quiesce runs in reverse priority order and resume in priority order, so
 * TE200 must sit below its service/application consumers: it is torn down
 * after they stop issuing crypto and rebuilt before they resume. */
static const pm_ap_fast_pm_ops_t s_dubhe_ap_fast_ops = {
    .name = "te200",
    .quiesce = dubhe_ap_fast_quiesce,
    .resume = dubhe_ap_fast_resume,
    .priority = PM_AP_FAST_PRIORITY_PERIPHERAL,
};
#endif /* CONFIG_PM_AP_FAST_BOOT_ENABLE */
#endif

/* GCC 14+: ISR must be compiled with general-regs-only when FPU is enabled. */
#pragma GCC push_options
#pragma GCC target("general-regs-only")
static void __BK_IRQ te200_isr(void)
{
#if !defined(DUBHE_SECURE)
	if (!s_ns_runtime_ready) {
		return;
	}
#endif
	extern int dubhe_intr_handler(void);
	dubhe_intr_handler();
}
#pragma GCC pop_options

static int do_dubhe_driver_init( unsigned long dbh_base_addr )
{
    uint32_t int_level = rtos_disable_int();

#if defined(DUBHE_SECURE)
    // bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_ENCP_TRUSTENGINE, PM_POWER_MODULE_STATE_ON);
#endif
    dubhe_delay_us(100);

    _g_Dubhe_RegBase = dbh_base_addr;

#if defined(DUBHE_SECURE)
    bk_interrupt_register_m55sub_int(INT_SRC_CP_ENC_SEC, te200_isr);
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

#else /* Normal: SPE owns power/TOP_CTRL; defer ENC_NSEC IRQ until first use */

    s_ns_runtime_ready = false;

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

#if defined( DUBHE_FOR_RUNTIME )
    dubhe_event_init( );
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

#if defined(DUBHE_SECURE)
        dubhe_lv_init();
#if CONFIG_PM_AP_FAST_BOOT_ENABLE
        bk_pm_ap_fast_ops_register(&s_dubhe_ap_fast_ops);
        dubhe_vote_cp_encp_power(PM_POWER_MODULE_STATE_ON);
#endif
        // bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_ENCP_TRUSTENGINE, PM_POWER_MODULE_STATE_ON);
#endif
    }

    if (need_init_driver == true) {
        ret = do_dubhe_driver_init(dbh_base_addr);
    }

    return ret;
}

#if !defined(DUBHE_SECURE)
void dubhe_ns_prepare_runtime( void )
{
    uint32_t int_level;

    if (s_ns_runtime_ready) {
        return;
    }

    int_level = rtos_disable_int();

    if (_g_Dubhe_RegBase == 0) {
        rtos_enable_int(int_level);
        return;
    }

#if defined( DUBHE_FOR_RUNTIME )
    dubhe_event_init( );
#endif

    dubhe_ns_clear_engine_intr();
    bk_interrupt_register_m55sub_int(INT_SRC_CP_ENC_NSEC, te200_isr);
    s_ns_runtime_ready = true;
    rtos_enable_int(int_level);
}
#endif

void dubhe_driver_cleanup( void )
{

#if defined(DUBHE_SECURE)
    bk_interrupt_unregister_m55sub_int(INT_SRC_CP_ENC_SEC);
#else
    if (s_ns_runtime_ready) {
        bk_interrupt_unregister_m55sub_int(INT_SRC_CP_ENC_NSEC);
        s_ns_runtime_ready = false;
    }
#endif

#if defined( ARM_CE_DUBHE_ACA )
    dubhe_aca_driver_cleanup( );
    extern void mbedtls_dubhe_cleanup( );
    mbedtls_dubhe_cleanup( );
#endif

    dubhe_event_cleanup( );
#if defined(DUBHE_SECURE)
    // bk_pm_module_vote_power_ctrl(POWER_SUB_MODULE_NAME_ENCP_TRUSTENGINE, PM_POWER_MODULE_STATE_OFF);
#endif
    dubhe_delay_us(100);
}

/*************************** The End Of File*****************************/
