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
#include "interrupt.h"
#include <soc/bk7259/int_types_impl.h>
#if defined(DUBHE_SECURE)
#include <modules/pm.h>
#include "sys_types.h"
#if CONFIG_SYS_SW_REGS
#include "sys_sw_regs.h"
#endif
/*
 * The OTP APB block lives in the CP-controlled WRLP_ENCP power domain, so the
 * AP cannot power it locally. TE200's OTP controller shares that block with the
 * CP OTP driver, which powers it off when its own holder count reaches zero.
 */
extern bool bk_pm_mailbox_is_ready(void);
extern bk_err_t bk_pm_module_vote_cp_power_ctrl(pm_power_module_name_e module,
                                                pm_power_module_state_e power_state);
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

#if defined(DUBHE_SECURE)
/*
 * TE200 keeps a dedicated permanent reference in the shared OTP APB power word
 * (BK_SWR_OTP_PWR_TE200_HOLD, see sys_sw_regs_layout.h). It must not be folded
 * into the AP holder count: that count is also used by transient AP OTP
 * transactions, so treating a non-zero value as "already held" could mistake an
 * in-flight transaction for the permanent hold.
 */
static bool s_otp_apb_usable;

#define DUBHE_OTP_APB_CLAIM_STEP_US     (1000U)
/* A power-off vote waits up to 100 ms for the CP response, keep the whole
 * window plus margin instead of a few milliseconds of guesswork. */
#define DUBHE_OTP_APB_CLAIM_TIMEOUT_US  (300U * 1000U)

/*
 * Take TE200's permanent reference in the shared OTP APB power word and ask
 * the CP to power the sub domain on. TE200's OTP controller shares the block
 * with the CP OTP driver, which powers it off only when no reference is left,
 * so the reference has to be taken before the vote.
 *
 * The vote waits for a CP mailbox response, so this must run from thread
 * context with interrupts enabled, before do_dubhe_driver_init() masks them.
 */
static int dubhe_otp_apb_power_on( void )
{
#if CONFIG_SYS_SW_REGS
    volatile uint32_t *hold = &bk_sys_sw_regs_ptr()->otp_apb_pwr_hold;
    uint32_t word = 0;
    uint32_t waited;
    bool claimed = false;

    if (!bk_pm_mailbox_is_ready()) {
        PAL_LOG_ERR("te200: PM mailbox not ready, OTP APB not protected\n");
        return -1;
    }

    /* Set the dedicated hold bit. Setting it is idempotent, so a bit left
     * by the previous boot/image is reused and the claim cannot grow
     * across warm reboots. The AP holder count is not touched here: it
     * belongs to the OTP driver's transient transactions. */
    for (waited = 0; waited < DUBHE_OTP_APB_CLAIM_TIMEOUT_US;
         waited += DUBHE_OTP_APB_CLAIM_STEP_US) {
        word = __atomic_load_n(hold, __ATOMIC_ACQUIRE);
        if ((word & BK_SWR_OTP_PWR_TURNING_OFF) == 0) {
            if ((word & BK_SWR_OTP_PWR_TE200_HOLD) != 0) {
                claimed = true; /* already held by this or a previous image */
                break;
            }
            if (__atomic_compare_exchange_n(hold, &word,
                                            word | BK_SWR_OTP_PWR_TE200_HOLD, false,
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
        /* Without a reference the CP may switch the sub domain off under
         * the TE200 OTP access, which hangs the AP with interrupts masked.
         * Fail the init instead of touching unpowered hardware. */
        PAL_LOG_ERR("te200: OTP APB hold claim timed out, pwr=0x%x\n", word);
        return -1;
    }

    if (bk_pm_module_vote_cp_power_ctrl(PM_POWER_SUB_DOMAIN_OTP_APB,
                                        PM_POWER_MODULE_STATE_ON) != BK_OK) {
        PAL_LOG_ERR("te200: OTP APB power vote failed\n");
        /* Drop the hold bit so the domain can be powered down again. This is
         * done even when the bit came from a previous image: TE200 will not be
         * initialized in this boot (s_otp_apb_usable stays false), so keeping
         * the reference would only leak power for the whole run. Only bit17 is
         * cleared, the holder counts may belong to a concurrent AP transaction. */
        (void)__atomic_and_fetch(hold, ~BK_SWR_OTP_PWR_TE200_HOLD,
                                 __ATOMIC_ACQ_REL);
        return -1;
    }
    return 0;
#else
    PAL_LOG_ERR("te200: SYS_SW_REGS disabled, OTP APB hold not shared\n");
    return -1;
#endif
}
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
    int ret = 0;

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
    return ret;
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
#if defined(DUBHE_SECURE)
        /* TE200's OTP controller shares the OTP APB block with the CP's OTP
         * driver. The CP's release path powers the sub domain off once no
         * reference is left, putting the macro into standby under the AP's
         * feet. Take TE200's permanent hold bit and vote the domain on before
         * the first TE200 OTP access. Both steps must run here: the vote waits
         * for a CP mailbox response, which cannot happen inside
         * do_dubhe_driver_init() where interrupts are masked. */
        s_otp_apb_usable = (dubhe_otp_apb_power_on() == 0);
#endif
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
