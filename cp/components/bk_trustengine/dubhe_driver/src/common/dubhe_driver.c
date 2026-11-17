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
#include "pal_time.h"
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
#if defined(DUBHE_SECURE)
#include "sys_types.h"
#if CONFIG_SYS_SW_REGS
#include "sys_sw_regs.h"
#endif
#endif
#if !defined(TEE_M)
#include <modules/pm.h>
#if !defined(DUBHE_SECURE)
#include "arch_interrupt.h"
#include "sdkconfig.h"
#endif
#endif

unsigned long _g_Dubhe_RegBase;
static int do_dubhe_driver_init( unsigned long dbh_base_addr );
static void dubhe_delay_us(uint32 num);
bool dubhe_inited = false;
/* Set while the engine is rebuilt after an ENCP power loss: every sub-block is
 * already in its power-on reset state, so the per-module reset pulse is
 * redundant. */
bool dubhe_skip_module_reset = false;
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

/* Invoked by the TF-M ENCP PM adapter after the Secure world rebuilt the
 * engine: release the NS-side ACA handle and force the next crypto call to redo
 * the Normal-window bring-up. */
void dubhe_ns_after_encp_resume(void)
{
#if defined( ARM_CE_DUBHE_ACA )
    dubhe_aca_driver_cleanup();
#endif
    s_ns_runtime_ready = false;
}
#endif

static void dubhe_delay_us(uint32 num) {

	volatile uint32 i, j, us_count;
	us_count = 4;
	for (i = 0; i < num; i++)
		for (j = 0; j < us_count; j++)
			;

}

#if defined(DUBHE_SECURE)
/*
 * TE200 on the CP keeps its own permanent reference in the shared OTP APB power
 * word (BK_SWR_OTP_PWR_TE200_CP_HOLD). It must not be folded into the CP holder
 * count: that count is also used by transient CP OTP transactions, so treating
 * a non-zero value as "already held" could mistake an in-flight transaction for
 * the permanent hold. The bit is CP-owned; the AP never sets or clears it.
 */
static bool s_otp_apb_usable;

#if !defined(TEE_M)
#define DUBHE_OTP_APB_CLAIM_STEP_US     (1000U)
/* A peer's OFF vote can hold TURNING_OFF for up to its 100 ms vote timeout,
 * keep the whole window plus margin. */
#define DUBHE_OTP_APB_CLAIM_TIMEOUT_US  (300U * 1000U)

/*
 * Take TE200's permanent reference in the shared OTP APB power word and vote
 * the sub domain on. The CP OTP driver powers the domain off only when no
 * reference is left in the word, so the reference has to be taken before the
 * vote.
 *
 * Must run from thread context with interrupts enabled, before
 * do_dubhe_driver_init() masks them.
 */
static int dubhe_otp_apb_power_on( void )
{
#if CONFIG_SYS_SW_REGS
    volatile uint32_t *hold = &bk_sys_sw_regs_ptr()->otp_apb_pwr_hold;
    uint32_t word = 0;
    uint32_t waited;
    bool claimed = false;

    /* Setting the bit is idempotent, so a bit left by the previous boot/image
     * is reused and the claim cannot grow across warm reboots. The CP holder
     * count is not touched here: it belongs to the OTP driver's transactions. */
    for (waited = 0; waited < DUBHE_OTP_APB_CLAIM_TIMEOUT_US;
         waited += DUBHE_OTP_APB_CLAIM_STEP_US) {
        word = __atomic_load_n(hold, __ATOMIC_ACQUIRE);
        if ((word & BK_SWR_OTP_PWR_TURNING_OFF) == 0) {
            if ((word & BK_SWR_OTP_PWR_TE200_CP_HOLD) != 0) {
                claimed = true; /* already held by this or a previous image */
                break;
            }
            if (__atomic_compare_exchange_n(hold, &word,
                                            word | BK_SWR_OTP_PWR_TE200_CP_HOLD, false,
                                            __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
                claimed = true;
                break;
            }
            continue;
        }
        /* A power-off is in flight, let it retire and retry. */
        pal_udelay( DUBHE_OTP_APB_CLAIM_STEP_US );
    }
    if (!claimed) {
        /* Without a reference the AP may switch the sub domain off under the
         * TE200 OTP access, which hangs the CP with interrupts masked. Fail the
         * init instead of touching unpowered hardware. */
        PAL_LOG_ERR("te200: OTP APB hold claim timed out, pwr=0x%x\n", word);
        return -1;
    }

    if (bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_OTP_APB,
                                     PM_POWER_MODULE_STATE_ON) != BK_OK) {
        PAL_LOG_ERR("te200: OTP APB power vote failed\n");
        /* Drop the hold bit so the domain can be powered down again. TE200 will
         * not be initialized in this boot, so keeping the reference would only
         * leak power for the whole run. Only this bit is cleared, the holder
         * counts may belong to a concurrent OTP transaction. */
        (void)__atomic_and_fetch(hold, ~BK_SWR_OTP_PWR_TE200_CP_HOLD,
                                 __ATOMIC_ACQ_REL);
        return -1;
    }
    return 0;
#else
    PAL_LOG_ERR("te200: SYS_SW_REGS disabled, OTP APB hold not shared\n");
    return -1;
#endif
}
#endif /* !TEE_M */
#endif /* DUBHE_SECURE */

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
    int ret = 0;

#if defined(TEE_M)
    if (sys_ll_get_cpu_power_sleep_wakeup_pwd_encp() != 0) {
        sys_ll_set_cpu_power_sleep_wakeup_pwd_encp(0);
    }
#elif defined(DUBHE_SECURE)
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_SHANHAI, PM_POWER_MODULE_STATE_ON);
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
    if (s_otp_apb_usable) {
        ret = arm_ce_otp_driver_init( );
        if (ret != 0) {
            PAL_LOG_ERR("te200: otp driver init failed: %d\n", ret);
        }
    } else {
        PAL_LOG_ERR("te200: OTP APB unavailable, skip otp driver init\n");
        ret = -1;
    }
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
    return ret;
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
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_SHANHAI, PM_POWER_MODULE_STATE_ON);
#elif defined(TEE_M)
        if (sys_ll_get_cpu_power_sleep_wakeup_pwd_encp() != 0) {
            sys_ll_set_cpu_power_sleep_wakeup_pwd_encp(0);
        }
#endif

#if defined(DUBHE_SECURE)
#if !defined(TEE_M)
        /* TE200's OTP controller shares the OTP APB block with the AP and CP
         * OTP drivers. Take TE200's permanent hold bit and vote the domain on
         * before the first TE200 OTP access; both steps must run here, with
         * interrupts still enabled, because do_dubhe_driver_init() masks them
         * and the claim may have to wait for an in-flight power-off. */
        s_otp_apb_usable = (dubhe_otp_apb_power_on() == 0);
#else
        /* BL2 / TF-M secure world: neither the AP nor the CP OTP driver is
         * running, so nobody can power the sub domain off under TE200, and the
         * shared power word is not available either (CONFIG_SYS_SW_REGS is off
         * in the secure build). Keep the OTP init unconditional: gating it here
         * makes dubhe_driver_init() fail and FIH_PANIC BL2. */
        s_otp_apb_usable = true;
#endif
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

/* Force a full engine bring-up regardless of the one-shot "dubhe_inited" gate.
 * Used by the Secure world sleep-resume gateway: the ENCP domain may have lost
 * power, so every TE200 register must be programmed again. */
void dubhe_driver_reinit( unsigned long dbh_base_addr )
{
    dubhe_inited = true;
    (void)do_dubhe_driver_init( dbh_base_addr );
}

/* Minimal engine bring-up for the Secure-world LV resume path. The WRLP_ENCP
 * domain lost power, so only the registers needed to make the engines usable
 * are programmed; the TRNG product settings are replayed from the cold-boot
 * copy instead of being rebuilt from flash constants. */
void dubhe_driver_lv_resume( unsigned long dbh_base_addr )
{
    uint32_t int_level = rtos_disable_int();

    dubhe_inited = true;
    _g_Dubhe_RegBase = dbh_base_addr;

    /* All sub-block clocks on, DMA AHB clock gated. */
    DBH_WRITE_REGISTER( TOP_CTRL, CLK_CTRL, 0x1bu );
    DBH_WRITE_REGISTER( TOP_CTRL, RESET_CTRL, 0x0u );

    /* Power-on reset already cleared every sub-block. */
    dubhe_skip_module_reset = true;

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
    arm_ce_trng_restore( );
#endif

    dubhe_skip_module_reset = false;

    /* The OTP manager is only needed when OTP data is accessed; its bring-up
     * waits for the slow on-chip initialization and is done lazily on first
     * use instead of on the wake-up path. */

#if defined( DUBHE_FOR_RUNTIME )
    dubhe_event_init( );
#endif

    rtos_enable_int(int_level);
}

#if !defined(TEE_M) && !defined(DUBHE_SECURE)
void dubhe_ns_prepare_runtime( void )
{
    uint32_t int_level;

    if (s_ns_runtime_ready) {
        return;
    }

    int_level = rtos_disable_int();

    /* TE200 clocks are configured by the Secure world (cold boot and fastboot
     * resume), so the Normal channel only needs its own sub-block setup. */

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
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_SHANHAI, PM_POWER_MODULE_STATE_OFF);
#endif
    dubhe_delay_us(100);
}

/*************************** The End Of File*****************************/
