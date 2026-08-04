/*
 * Copyright (c)     2023-2028, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "build_config_check.h"
#include "internal_status_code.h"
#include "fih.h"
#include "tfm_boot_data.h"
#include "memory_symbols.h"
#include "spm.h"
#include "tfm_hal_isolation.h"
#include "tfm_hal_platform.h"
#include "tfm_spm_log.h"
#include "tfm_version.h"
#include "tfm_plat_otp.h"
#include "tfm_plat_provisioning.h"
#include "ffm/backend.h"
#include "target_cfg.h"
#include "hal_hw_fih.h"
#include "hal_sw_fih.h"
#include "bk_sca_defense.h"
#include "anti_tamper.h"
#include "driver/flash.h"
#include "driver/ckmn.h"
#include "driver/adc.h"
#include "tfm_flash_partition.h"
#include "hal_hw_fih.h"
#ifdef CONFIG_TFM_ENABLE_PROFILING
#include "prof_intf_s.h"
#endif

uintptr_t spm_boundary = (uintptr_t)NULL;

static fih_int tfm_core_init(void)
{
    volatile enum tfm_plat_err_t plat_err = TFM_PLAT_ERR_SYSTEM_ERR;
    fih_int fih_rc = FIH_FAILURE;

    plat_err = bk_flash_driver_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }
    plat_err = TFM_PLAT_ERR_SYSTEM_ERR;
    plat_err = partition_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(TFM_HAL_ERROR_GENERIC));
    }
    /*
     * Access to any peripheral should be performed after programming
     * the necessary security components such as PPC/SAU.
     */
    FIH_CALL(tfm_hal_set_up_static_boundaries, fih_rc, &spm_boundary);
    if (fih_not_eq(fih_rc, fih_int_encode(TFM_HAL_SUCCESS))) {
        FIH_RET(fih_int_encode(SPM_ERROR_GENERIC));
    }
#ifdef TFM_FIH_PROFILE_ON
    FIH_CALL(tfm_hal_verify_static_boundaries, fih_rc);
    if (fih_not_eq(fih_rc, fih_int_encode(TFM_HAL_SUCCESS))) {
        tfm_core_panic();
    }
#endif

    FIH_CALL(tfm_hal_platform_init, fih_rc);
    if (fih_not_eq(fih_rc, fih_int_encode(TFM_HAL_SUCCESS))) {
        FIH_RET(fih_int_encode(SPM_ERROR_GENERIC));
    }
    /*
     * Print the TF-M version now that the platform has initialized
     * the logging backend.
     */
    SPMLOG_INFMSGVAL("Chip ID: ", sys_drv_get_chip_id());
#ifdef SW_VERSION
    SPMLOG_INFMSG("\033[1;34mSoftware version: " SW_VERSION "\033[0m\r\n");
#endif
    SPMLOG_INFMSG("\033[1;34mBooting TF-M "VERSION_FULLSTR"\033[0m\r\n");

    bk_sw_fih_set_data(FIH_SW_INDEX14);
    plat_err = TFM_PLAT_ERR_SYSTEM_ERR;
    plat_err = tfm_plat_otp_init();
    if (plat_err != TFM_PLAT_ERR_SUCCESS) {
        FIH_RET(fih_int_encode(SPM_ERROR_GENERIC));
    }
    bk_sw_fih_set_data(FIH_SW_INDEX15);

    enum plat_otp_lcs_t lcs;
    plat_err = tfm_plat_otp_read(PLAT_OTP_ID_LCS, sizeof(lcs), (uint8_t*)&lcs);
    bk_fih_set_src(FIH_DATA_LCS, lcs);
    bk_fih_set_dst(FIH_DATA_LCS, lcs);

    /* Configures architecture */
    tfm_arch_config_extensions();

    SPMLOG_INFMSG("\033[1;34m[Sec Thread] Secure image initializing!\033[0m\r\n");

    SPMLOG_DBGMSGVAL("TF-M isolation level is: ", TFM_ISOLATION_LEVEL);

#if (CONFIG_TFM_FLOAT_ABI == 2)
    SPMLOG_INFMSG("TF-M Float ABI: Hard\r\n");
#ifdef CONFIG_TFM_LAZY_STACKING
    SPMLOG_INFMSG("Lazy stacking enabled\r\n");
#else
    SPMLOG_INFMSG("Lazy stacking disabled\r\n");
#endif
#endif
    bk_sw_fih_set_data(FIH_SW_INDEX16);
    tfm_core_validate_boot_data();
    bk_sw_fih_set_data(FIH_SW_INDEX17);
    FIH_RET(fih_int_encode(SPM_SUCCESS));
}

int main(void)
{
#ifdef CONFIG_TFM_ENABLE_PROFILING
    PROFILING_INIT();
#endif
    /* BK7259 bring-up: drop the psa_level3 secure-hardening and non-secure
     * peripheral bring-up that ran before tfm_core_init() and hung the secure
     * image before TF-M logging was initialized. Per the bring-up decision
     * (drop FIH/anti-tamper, do NOT reuse
     * the non-secure driver stack), remove:
     *   - bk_sca_random_freq_init / bk_anti_tamper_enable / bk_ckmn_start
     *     (SCA / anti-tamper / clock-monitor hardening)
     *   - bk_adc_driver_init / temp_detect_init / volt_detect_set_config /
     *     temp_sensor_enable (ADC + temp/volt detect -- non-secure peripheral
     *     drivers, not needed to bring TF-M up and a likely early hang source)
     *   - bk_sw_fih_set_data markers
     * Keep only what TF-M core needs: systick, MSP limit, and tfm_core_init. */
    fih_int fih_rc = FIH_FAILURE;

    extern void systick_init(void);
    systick_init();
    tfm_arch_set_msplim(SPM_BOOT_STACK_TOP);

    FIH_LOOP4(fih_delay_init());

    FIH_CALL(tfm_core_init, fih_rc);
    if (fih_not_eq(fih_rc, fih_int_encode(SPM_SUCCESS))) {
        tfm_core_panic();
    }
    bk_sw_fih_set_data(FIH_SW_INDEX18);
    /* All isolation should have been set up at this point */
    FIH_LABEL_CRITICAL_POINT();

    /*
     * Prioritise secure exceptions to avoid NS being able to pre-empt
     * secure SVC or SecureFault. Do it before PSA API initialization.
     */
    tfm_arch_set_secure_exception_priorities();
    
#ifdef TFM_FIH_PROFILE_ON
    /* Check secure exception priority */
    FIH_CALL(tfm_arch_verify_secure_exception_priorities, fih_rc);
    if (fih_not_eq(fih_rc, FIH_SUCCESS)) {
         tfm_core_panic();
    }
#endif

    /* Further SPM initialization. */
    BACKEND_SPM_INIT();

    return 0;
}
