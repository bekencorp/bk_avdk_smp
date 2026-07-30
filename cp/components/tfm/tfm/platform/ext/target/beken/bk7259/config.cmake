set(BL2                                 ON          CACHE BOOL      "Whether to build BL2")
# Image signature = EC-P256 (matches security.csv root_key_type=ec256 and the
# generated stub/security.h MCUBOOT_SIGN_EC256). The TF-M default is RSA-3072,
# which would enable MBEDTLS_RSA_C without all profile_medium prerequisites.
# EC-P256 signing requires MCUBOOT_USE_PSA_CRYPTO=ON (TF-M check_config.cmake).
set(MCUBOOT_SIGNATURE_TYPE              "EC-P256"   CACHE STRING    "Algorithm to use for signature validation" FORCE)
set(MCUBOOT_USE_PSA_CRYPTO              ON          CACHE BOOL      "Enable MCUboot PSA crypto (required for EC-P256)" FORCE)
# BL2 serial-download (common/download/) is being ported to bk7259. It is driven
# by the armino TFM Kconfig (CONFIG_TFM_BL2_DOWNLOAD, default y) which injects
# -DBL2_DOWNLOAD=y on the cmake command line.
set(BL2_DOWNLOAD                        ON          CACHE BOOL      "Whether to build BL2_DOWNLOAD")
set(DEFAULT_MCUBOOT_FLASH_MAP           OFF         CACHE BOOL      "Whether to use the default flash map defined by TF-M project")
set(MCUBOOT_IMAGE_NUMBER                1           CACHE STRING    "Whether to combine S and NS into either 1 image, or sign each seperately")
set(PLATFORM_DEFAULT_IMAGE_SIGNING      OFF         CACHE BOOL      "Use default image signing implementation")
# set(CONFIG_TFM_BOOT_STORE_MEASUREMENTS  OFF         CACHE BOOL      "Store measurement values from all the boot stages. Used for initial attestation token.")
set(CONFIG_TFM_USE_TRUSTZONE            ON)
set(MBEDCRYPTO_BUILD_TYPE               debug CACHE STRING "Build type of Mbed Crypto library")
set(PLATFORM_FLIH_IRQ_TEST_SUPPORT      ON       CACHE BOOL      "Whether the platform has FLIH test support")
# Phase-1: start at Isolation Level 2 (SAU+MPC+PPC). L3 is an evolution.
set(PLATFORM_HAS_ISOLATION_L3_SUPPORT   OFF      CACHE BOOL      "Whether the platform support isolation l3")
# BK7259 bring-up: disable MCUboot HW anti-rollback. 
set(MCUBOOT_HW_ROLLBACK_PROT            OFF         CACHE BOOL      "Enable security counter validation against non-volatile HW counters")
set(BL2_VALIDATE_ENABLED_BY_EFUSE       ON           CACHE BOOL       "Whether to enable bl2 validate by efuse")
set(CONFIG_SUPPORT_SWD_DEBUG            OFF           CACHE BOOL        "Whether to enable SWD dedbug")

# Phase-1: FIH / SCA disabled to reduce bring-up complexity. Re-enable for
# PSA certification (see DLD §9.3 / config decision D7).
set(MCUBOOT_FIH_PROFILE                 "OFF"         CACHE STRING    "Fault injection hardening profile [OFF, LOW, MEDIUM, HIGH]")
set(TFM_FIH_PROFILE                     "OFF"         CACHE STRING    "Fault injection hardening profile [OFF, LOW, MEDIUM, HIGH]")
set(HW_FIH                              OFF          CACHE BOOL       "Whether to enable hardware FIH")
set(TFM_HW_FIH                          OFF          CACHE BOOL       "Whether to enable TFM hardware FIH")
set(TFM_SW_FIH                          OFF          CACHE BOOL       "Whether to enable TFM software FIH")
set(BL2_HW_FIH                          OFF          CACHE BOOL       "Whether to enable bl2 hardware FIH")
set(BL2_SW_FIH                          OFF          CACHE BOOL       "Whether to enable bl2 software FIH")

set(SCA_DEFENSE                         OFF          CACHE BOOL       "Whether to enable SCA defense")

# set(TFM_DUMMY_PROVISIONING              OFF          CACHE BOOL      "Provision with dummy values. NOT to be used in production")
set(PS_NUM_ASSETS                    "20"      CACHE STRING    "The maximum number of assets to be stored in the Protected Storage area")
set(PS_MAX_ASSET_SIZE                "1024"      CACHE STRING    "The maximum asset size to be stored in the Protected Storage area")
set(ITS_MAX_ASSET_SIZE               "512"       CACHE STRING    "The maximum asset size to be stored in the Internal Trusted Storage area")
set(ITS_NUM_ASSETS                   "20"        CACHE STRING    "The maximum number of assets to be stored in the Internal Trusted Storage area")

set(CONFIG_PANIC_DEAD_LOOP              OFF           CACHE BOOL       "Whether to enable panic dead loop")
# BK7259 bring-up: disable the TF-M MPU (privileged/unprivileged + inter-partition
# memory isolation). Two reasons:
#   1) The crypto partition (PSA-RoT, runs PRIVILEGED) accesses the Dubhe crypto
#      engine MMIO at 0x42110000 during psa_crypto_init(). That region is not
#      mapped by any MPU region, so once the MPU is enabled the access faults.
#   2) tfm_hal_set_up_static_boundaries() calls enable_scb_dcache() right after
#      mpu_armv8m_enable(); with the default memory map the Dubhe MMIO can become
#      D-cacheable, and the cached MMIO writes surface as an imprecise BusFault.
# Disabling CONFIG_TFM_MPU skips both mpu_armv8m_enable() and enable_scb_dcache().
# PSA-RoT partitions are PRIVILEGED regardless of the MPU (privileged flag comes
# from IS_PARTITION_PSA_ROT), so turning the MPU off does not change their
# privilege level; it only drops inter-partition memory isolation, which is
# acceptable for bring-up where the priority is to reach the NS world.
set(CONFIG_TFM_MPU                      OFF          CACHE BOOL       "BK7259 bring-up: disable tfm MPU")

# BK7259 bring-up: temporarily disable the Protected Storage partition.
# profile_medium.cmake turns it ON, but its SFN init (tfm_ps_entry) touches flash
# via the thin-shim driver (and routes requests through the platform partition),
# which stalls the SPM scheduler so ns_agent (lowest priority) never runs and we
# never reach the NS world. Priority right now is to see the NS app log.
#
# NOTE: Internal Trusted Storage (ITS) is intentionally kept ON: the crypto
# partition has a hard manifest dependency on TFM_INTERNAL_TRUSTED_STORAGE_SERVICE
# and mbedcrypto's psa_crypto_storage.c links against psa_its_* symbols, so
# disabling ITS breaks the build. FORCE overrides the profile_medium cache value.
set(TFM_PARTITION_PROTECTED_STORAGE        OFF   CACHE BOOL "BK7259 bring-up: disable PS partition" FORCE)

# BK7259 bring-up: the build type is minsizerel, which forces
# TFM_SPM_LOG_LEVEL=SILENCE (see config/build_type/minsizerel.cmake), so no TF-M
# SPM log is emitted on UART1. Force DEBUG level so the TFM_EXCEPTION_INFO_DUMP
# fault context (printed via SPMLOG_DBGMSG in exception_info.c) is visible.
set(TFM_SPM_LOG_LEVEL    TFM_SPM_LOG_LEVEL_DEBUG    CACHE STRING "BK7259 bring-up: force TF-M SPM log to DEBUG" FORCE)
