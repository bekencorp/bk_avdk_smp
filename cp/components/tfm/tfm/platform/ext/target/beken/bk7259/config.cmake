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
set(CONFIG_TFM_BOOT_STORE_MEASUREMENTS   OFF         CACHE BOOL      "Store measurement values from all the boot stages. Used for initial attestation token." FORCE)
set(CONFIG_TFM_USE_TRUSTZONE            ON)
set(MBEDCRYPTO_BUILD_TYPE               debug CACHE STRING "Build type of Mbed Crypto library")
set(PLATFORM_FLIH_IRQ_TEST_SUPPORT      ON       CACHE BOOL      "Whether the platform has FLIH test support")
set(PLATFORM_HAS_ISOLATION_L3_SUPPORT   OFF      CACHE BOOL      "Whether the platform support isolation l3")
set(MCUBOOT_HW_ROLLBACK_PROT            OFF         CACHE BOOL      "Enable security counter validation against non-volatile HW counters")
set(BL2_VALIDATE_ENABLED_BY_EFUSE       ON           CACHE BOOL       "Whether to enable bl2 validate by efuse")
set(CONFIG_SUPPORT_SWD_DEBUG            OFF           CACHE BOOL        "Whether to enable SWD dedbug")

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
set(CONFIG_TFM_MPU                      OFF           CACHE BOOL       "disable tfm MPU")
set(TFM_PARTITION_PROTECTED_STORAGE     OFF           CACHE BOOL       "disable PS partition" FORCE)

# Secure runtime (SPE) log level is driven by CONFIG_TFM_LOG_LEVEL from the
# project defconfig, independent from the bootloader level (CONFIG_TFM_BL2_LOG_LEVEL
# -> MCUBOOT_LOG_LEVEL). The build type is minsizerel, which pre-sets
# TFM_SPM_LOG_LEVEL=SILENCE (config/build_type/minsizerel.cmake), so FORCE the
# mapped value here. The SPM level set has no WARNING tier: map it onto INFO.
if(NOT DEFINED CONFIG_TFM_LOG_LEVEL OR CONFIG_TFM_LOG_LEVEL STREQUAL "")
	set(CONFIG_TFM_LOG_LEVEL "INFO")
endif()
if(CONFIG_TFM_LOG_LEVEL STREQUAL "OFF")
	set(_bk_spm_log_level TFM_SPM_LOG_LEVEL_SILENCE)
elseif(CONFIG_TFM_LOG_LEVEL STREQUAL "ERROR")
	set(_bk_spm_log_level TFM_SPM_LOG_LEVEL_ERROR)
elseif(CONFIG_TFM_LOG_LEVEL STREQUAL "DEBUG")
	set(_bk_spm_log_level TFM_SPM_LOG_LEVEL_DEBUG)
else() # WARNING / INFO
	set(_bk_spm_log_level TFM_SPM_LOG_LEVEL_INFO)
endif()
set(TFM_SPM_LOG_LEVEL    ${_bk_spm_log_level}    CACHE STRING "SPE SPM log level from CONFIG_TFM_LOG_LEVEL" FORCE)

# Secure Partition (unprivileged) logging is kept SILENCE: the beken secure world
# logs from privileged code through the SPM sink, so the partition raw-log path
# (printf -> SVC OUTPUT_UNPRIV_STRING) is not needed. Forcing SILENCE keeps the
# emitter and its SPM handler both out, so no partition SVC log traffic is issued.
set(TFM_PARTITION_LOG_LEVEL    TFM_PARTITION_LOG_LEVEL_SILENCE    CACHE STRING "Secure Partition log level" FORCE)
