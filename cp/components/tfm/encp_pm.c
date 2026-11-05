// Copyright 2023-2028 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");

/* Non-secure PM adapter for the TE200 sleep handover.
 *
 * The WRLP_ENCP domain (TE200 + OTP + WiFi/BT) loses power on every LV sleep,
 * so the engine is handed to the Secure world before the domain votes are
 * cleared and rebuilt after wake. Both calls are plain SG veneers into TF-M and
 * complete synchronously; when psa_encp_resume() returns, the Secure world has
 * already reprogrammed the engine and the OTP secure range.
 *
 * Deliberately independent of CONFIG_TRUSTENGINE: the Secure world owns the
 * TE200 even when the NS image does not use it directly, and the OTP secure
 * protection must be rebuilt in both cases. */

#include <common/bk_include.h>
#include <modules/pm.h>

#if defined(CONFIG_TFM_ENCP_NSC) && CONFIG_TFM_ENCP_NSC

extern int psa_encp_quiesce(void);
extern int psa_encp_resume(void);

#if CONFIG_TRUSTENGINE
/* Release the NS-side runtime state once the Secure world has rebuilt the
 * engine, so the next crypto call redoes the Normal-window bring-up. */
extern void dubhe_ns_after_encp_resume(void);
#endif

static int encp_lv_enter(uint64_t sleep_time, void *args)
{
    (void)sleep_time;
    (void)args;

    return psa_encp_quiesce();
}

static int encp_lv_exit(uint64_t sleep_time, void *args)
{
    int ret;

    (void)sleep_time;
    (void)args;

    /* LV entry cleared the ENCP sub-domain votes, so re-vote it ON before the
     * Secure world touches any TE200 or OTP register. */
    bk_pm_module_vote_power_ctrl(PM_POWER_SUB_DOMAIN_SHANHAI, PM_POWER_MODULE_STATE_ON);

    ret = psa_encp_resume();

#if CONFIG_TRUSTENGINE
    dubhe_ns_after_encp_resume();
#endif

    return ret;
}

void bk_encp_pm_init(void)
{
    pm_cb_conf_t enter = {encp_lv_enter, NULL};
    pm_cb_conf_t exit = {encp_lv_exit, NULL};

    bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_SECURE_WORLD, &enter, &exit);
}

#endif /* CONFIG_TFM_ENCP_NSC */

/*************************** The End Of File*****************************/
