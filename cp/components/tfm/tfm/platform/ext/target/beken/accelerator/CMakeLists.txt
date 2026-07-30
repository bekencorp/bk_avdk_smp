################################ BL2 ###########################################
# TrustEngine (Dubhe) source location.
# - psa_level3 layout: ${ROOT_PATH}/trustengine
# - BK7259 SDK layout: ${ARMINO_SDK_PATH}/components/bk_trustengine
#   (ARMINO_SDK_PATH = sdk/cp, passed by the Armino TF-M ExternalProject)
if(DEFINED ARMINO_SDK_PATH AND EXISTS "${ARMINO_SDK_PATH}/components/bk_trustengine")
    set(TE_DIR ${ARMINO_SDK_PATH}/components/bk_trustengine)
else()
    set(TE_DIR ${ROOT_PATH}/trustengine)
endif()

string(REGEX REPLACE "beken/" "" PLATFORM "${TFM_PLATFORM}")

if(BL2)
    target_compile_options(bl2_crypto_hw
        PUBLIC
            ${BL2_COMPILER_CP_FLAG}
    )

    # dubhe_dbg.c exists in psa_level3 trustengine but not in the SDK
    # bk_trustengine; include it only when present.
    if(EXISTS "${TE_DIR}/dubhe_driver/src/crypto/dbg/dubhe_dbg.c")
        target_sources(bl2_crypto_hw PRIVATE
            ${TE_DIR}/dubhe_driver/src/crypto/dbg/dubhe_dbg.c)
    endif()

    target_sources(bl2_crypto_hw
        PRIVATE
            ${TE_DIR}/dubhe_driver/src/common/dubhe_driver.c
            ${TE_DIR}/dubhe_driver/src/common/dubhe_event.c
            ${TE_DIR}/dubhe_driver/src/common/dubhe_intr_handler.c
            ${TE_DIR}/dubhe_driver/src/common/dubhe_sram_alloc.c
            ${TE_DIR}/dubhe_driver/src/crypto/aca/dubhe_aca.c
            ${TE_DIR}/dubhe_driver/src/crypto/aca/dubhe_aca_hw.c
            ${TE_DIR}/dubhe_driver/src/crypto/aead/dubhe_ccm.c
            ${TE_DIR}/dubhe_driver/src/crypto/aead/dubhe_gcm.c
            ${TE_DIR}/dubhe_driver/src/crypto/hash/dubhe_hash.c
            ${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_cbcmac.c
            ${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_cmac.c
            ${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_ghash.c
            ${TE_DIR}/dubhe_driver/src/crypto/otp/dubhe_otp.c
            ${TE_DIR}/dubhe_driver/src/crypto/sca/dubhe_sca.c
            ${TE_DIR}/dubhe_driver/src/crypto/trng/dubhe_trng.c
            ${TE_DIR}/dubhe_driver/src/hal_pal/pal_heap.c
            ${TE_DIR}/dubhe_driver/src/hal_pal/pal_log.c
            ${TE_DIR}/dubhe_driver/src/hal_pal/pal_mutex.c
            ${TE_DIR}/dubhe_driver/src/hal_pal/pal_preempt.c
            ${TE_DIR}/dubhe_driver/src/hal_pal/pal_semaphore.c
            #${TE_DIR}/dubhe_driver/src/hal_pal/pal_signal.c
            ${TE_DIR}/dubhe_driver/src/hal_pal/pal_string.c
            ${TE_DIR}/dubhe_driver/src/hal_pal/pal_time.c

            ${TE_DIR}/dubhe_alt/src/aes.c
            ${TE_DIR}/dubhe_alt/src/bignum_wrap.c
            ${TE_DIR}/dubhe_alt/src/ccm.c
            ${TE_DIR}/dubhe_alt/src/cmac.c
            ${TE_DIR}/dubhe_alt/src/gcm.c
            ${TE_DIR}/dubhe_alt/src/otp.c
            ${TE_DIR}/dubhe_alt/src/sha256.c

            ${TE_DIR}/dubhe_lib/bignum.c
            ${TE_DIR}/dubhe_lib/bignum_delta.c
            ${TE_DIR}/dubhe_lib/cipher.c
            ${TE_DIR}/dubhe_lib/cipher_wrap.c
            ${TE_DIR}/dubhe_lib/ctr_drbg.c
            ${TE_DIR}/dubhe_lib/dhm.c
            ${TE_DIR}/dubhe_lib/ecdh.c
            ${TE_DIR}/dubhe_lib/ecdsa.c
            ${TE_DIR}/dubhe_lib/ecp.c
            ${TE_DIR}/dubhe_lib/ecp_curves.c
            ${TE_DIR}/dubhe_lib/entropy_poll.c
            ${TE_DIR}/dubhe_lib/md.c
            ${TE_DIR}/dubhe_lib/oid.c
            ${TE_DIR}/dubhe_lib/pkparse.c
            ${TE_DIR}/dubhe_lib/rsa.c

            ${TE_DIR}/spe/src/misc.c
            ${TE_DIR}/crypto_hw.c
            # ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_aes.c
            # ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_rsa.c
            # ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_hash.c
            # ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_ecdsa.c
    )

    target_include_directories(bl2_crypto_hw
        PRIVATE
            ${TE_DIR}/dubhe_driver/inc/common
            ${TE_DIR}/dubhe_driver/inc/crypto
            ${PLATFORM_DIR}/ext/target/beken/common/hal/inc

            ${IDK_PATH}/include
            ${IDK_PATH}/include/soc/${PLATFORM}
            ${IDK_PATH}/middleware/soc/common/soc/include
            ${IDK_PATH}/middleware/soc/common/hal
            ${IDK_PATH}/middleware/soc/common/hal/include
            ${IDK_PATH}/middleware/soc/${PLATFORM}/soc
            ${IDK_PATH}/middleware/soc/${PLATFORM}/hal
            ${IDK_PATH}/components/bk_common/include
            ${IDK_PATH}/middleware/driver/common
            ${IDK_PATH}/middleware/driver/sys_ctrl
            ${IDK_PATH}/middleware/driver/include/bk_private
            # BK7259: sys_driver.h (pulled in by dubhe_otp.c) includes aspl_lock.h.
            ${IDK_PATH}/middleware/driver/aspl
            ${TARGET_PLATFORM_PATH}/idk
            ${TFM_BEKEN}/common/idk
            ${TFM_BEKEN}/common/idk/port
            ${CONFIG_TFM_SOURCE_PATH}/interface/include
    )


    target_include_directories(bl2_crypto_hw
        PRIVATE
            ${TE_DIR}/dubhe_alt/inc

            ${TFM_BEKEN}/common
            ${TFM_BEKEN}/common/cmsis_core
            ${TE_DIR}/spe/inc
            ${TE_DIR}/spe/inc/hal
            ${TE_DIR}/spe/inc/pal

        PUBLIC
            ${TFM_BEKEN}/accelerator
            ${PLATFORM_DIR}/include
            ${MBEDCRYPTO_PATH}/library
            ${MBEDCRYPTO_PATH}/include
            # ${PLATFORM_DIR}/ext/target/${TFM_PLATFORM}/armino/config
            ${CMAKE_SOURCE_DIR}/interface/include
            ${CMAKE_BINARY_DIR}/generated/interface/include
            ${PLATFORM_DIR}/ext/cmsis
            ${PLATFORM_DIR}/../secure_fw/spm/include
            ${PLATFORM_DIR}/ext/common
    )

    target_compile_definitions(bl2_crypto_hw
        PRIVATE
            ARM_CE_DUBHE
            DUBHE_SECURE
            ARM_CE_DUBHE_SCA
            ARM_CE_DUBHE_TRNG
            ARM_CE_DUBHE_OTP
            ARM_CE_DUBHE_HASH
            ARM_CE_DUBHE_ACA
            CONFIG_ENABLE_MCUBOOT_BL2
            # BL2 is a bare secure bootloader (no PM/RTOS framework). Select the
            # dubhe_driver TEE_M path so it gates the TrustEngine via the
            # sys_ll_*_encp register shim (see bk7259 sys_ll.h) instead of the
            # runtime PM-voting path (bk_pm_module_vote_power_ctrl / isr).
            TEE_M
            # TF-M 2.1.0 provides psa_wait(); drop the Beken spe/misc.c shim.
            CONFIG_TFM_2_1_0_PSA_API
    )

    target_link_libraries(bl2_crypto_hw
        PRIVATE
            bl2_mbedcrypto
    )

    target_include_directories(bl2_mbedcrypto
        PUBLIC
            ${TFM_BEKEN}/accelerator/
            ${TE_DIR}/dubhe_driver/inc/common
            ${TE_DIR}/dubhe_driver/inc/crypto
            ${TE_DIR}/dubhe_alt/inc
    )

endif()

############################ Crypto Service ####################################
if (TFM_PARTITION_CRYPTO)
target_sources(crypto_service_crypto_hw
    PRIVATE
        ${TE_DIR}/dubhe_driver/src/common/dubhe_driver.c
        ${TE_DIR}/dubhe_driver/src/common/dubhe_event.c
        ${TE_DIR}/dubhe_driver/src/common/dubhe_intr_handler.c
        ${TE_DIR}/dubhe_driver/src/common/dubhe_sram_alloc.c
        ${TE_DIR}/dubhe_driver/src/crypto/aca/dubhe_aca.c
        ${TE_DIR}/dubhe_driver/src/crypto/aca/dubhe_aca_hw.c
        ${TE_DIR}/dubhe_driver/src/crypto/aead/dubhe_ccm.c
        ${TE_DIR}/dubhe_driver/src/crypto/aead/dubhe_gcm.c
        ${TE_DIR}/dubhe_driver/src/crypto/hash/dubhe_hash.c
        ${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_cbcmac.c
        ${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_cmac.c
        ${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_ghash.c
        ${TE_DIR}/dubhe_driver/src/crypto/otp/dubhe_otp.c
        ${TE_DIR}/dubhe_driver/src/crypto/sca/dubhe_sca.c
        ${TE_DIR}/dubhe_driver/src/crypto/trng/dubhe_trng.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_heap.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_log.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_mutex.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_preempt.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_semaphore.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_signal.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_string.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_time.c

        ${TE_DIR}/dubhe_alt/src/aes.c
        ${TE_DIR}/dubhe_alt/src/bignum_wrap.c
        ${TE_DIR}/dubhe_alt/src/ccm.c
        ${TE_DIR}/dubhe_alt/src/cmac.c
        ${TE_DIR}/dubhe_alt/src/gcm.c
        ${TE_DIR}/dubhe_alt/src/otp.c
        ${TE_DIR}/dubhe_alt/src/sha256.c

        ${TE_DIR}/dubhe_lib/bignum.c
        ${TE_DIR}/dubhe_lib/bignum_delta.c
        ${TE_DIR}/dubhe_lib/cipher.c
        ${TE_DIR}/dubhe_lib/cipher_wrap.c
        ${TE_DIR}/dubhe_lib/ctr_drbg.c
        ${TE_DIR}/dubhe_lib/dhm.c
        ${TE_DIR}/dubhe_lib/ecdh.c
        ${TE_DIR}/dubhe_lib/ecdsa.c
        ${TE_DIR}/dubhe_lib/ecp.c
        ${TE_DIR}/dubhe_lib/ecp_curves.c
        ${TE_DIR}/dubhe_lib/entropy_poll.c
        ${TE_DIR}/dubhe_lib/md.c
        ${TE_DIR}/dubhe_lib/oid.c
        ${TE_DIR}/dubhe_lib/pkparse.c
        ${TE_DIR}/dubhe_lib/rsa.c

		${TE_DIR}/spe/src/misc.c
		${TE_DIR}/crypto_hw.c
		# ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_aes.c
		# ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_rsa.c
		# ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_hash.c
		# ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_ecdsa.c
)

target_include_directories(crypto_service_crypto_hw
    PRIVATE
        ${TE_DIR}/dubhe_driver/inc/common
        ${TE_DIR}/dubhe_driver/inc/crypto
        ${PLATFORM_DIR}/ext/target/beken/common/hal/inc
        ${IDK_PATH}/include
        ${IDK_PATH}/include/soc/${PLATFORM}
        ${IDK_PATH}/middleware/soc/common/soc/include
        ${IDK_PATH}/middleware/soc/common/hal
        ${IDK_PATH}/middleware/soc/common/hal/include
        ${IDK_PATH}/middleware/soc/${PLATFORM}/soc
        ${IDK_PATH}/middleware/soc/${PLATFORM}/hal
        ${IDK_PATH}/components/bk_common/include
        ${IDK_PATH}/middleware/driver/common
        ${IDK_PATH}/middleware/driver/sys_ctrl
        ${IDK_PATH}/middleware/driver/include/bk_private
        # BK7259: sys_driver.h (pulled in by dubhe_otp.c) includes aspl_lock.h.
        ${IDK_PATH}/middleware/driver/aspl
        ${TARGET_PLATFORM_PATH}/idk
        ${TFM_BEKEN}/common/idk
        ${TFM_BEKEN}/common/idk/port

        ${TE_DIR}/dubhe_alt/inc
        # ${PLATFORM_DIR}/ext/target/${TFM_PLATFORM}/armino/config
        ${CMAKE_SOURCE_DIR}/interface/include
        ${CMAKE_BINARY_DIR}/generated/interface/include

        # ${PLATFORM_DIR}/ext/target/${TFM_PLATFORM}
        # ${PLATFORM_DIR}/ext/target/${TFM_PLATFORM}/cmsis_core
        ${TFM_BEKEN}/common
        ${TFM_BEKEN}/common/cmsis_core
        ${TE_DIR}/spe/inc
        ${TE_DIR}/spe/inc/hal
        ${TE_DIR}/spe/inc/pal

    PUBLIC
        ${TFM_BEKEN}/accelerator
        ${PLATFORM_DIR}/include
        ${MBEDCRYPTO_PATH}/library
        ${MBEDCRYPTO_PATH}/include
        ${CMAKE_SOURCE_DIR}/../../cmsis/CMSIS_5/Device/ArmChina/STAR/Include
        ${PLATFORM_DIR}/../secure_fw/spm/include
        ${PLATFORM_DIR}/ext/common
)

target_link_libraries(crypto_service_crypto_hw
    PRIVATE
        crypto_service_mbedcrypto
)


target_compile_options(crypto_service_crypto_hw
    PUBLIC
        ${COMPILER_CP_FLAG}
)

target_compile_definitions(crypto_service_crypto_hw
    PRIVATE
        TEE_M
        ARM_CE_DUBHE
        DUBHE_SECURE
        ARM_CE_DUBHE_SCA
        ARM_CE_DUBHE_TRNG
        ARM_CE_DUBHE_OTP
        ARM_CE_DUBHE_HASH
        ARM_CE_DUBHE_ACA
        # TF-M 2.1.0 provides psa_wait(); drop the Beken spe/misc.c shim.
        CONFIG_TFM_2_1_0_PSA_API
        # BK7259 bring-up: route mbedtls_hardware_poll() (entropy source used by
        # psa_crypto_init) through the Dubhe TE200 TRNG (arm_ce_seed_read) instead
        # of bk_puf_get_random_number(). The PUF fallback in dubhe_lib/ctr_drbg.c
        # busy-waits on a hard-coded BK7234 register (0x4B1002C4) that does not
        # exist on BK7259, so the read never returns / faults. The Dubhe engine is
        # already initialised by crypto_hw_accelerator_init() before psa_crypto_init.
        CONFIG_TE200_ENTROPY=1
        $<$<BOOL:${SCA_DEFENSE}>:CONFIG_SCA_DEFENSE>
)
endif()

if (NOT BL2)
target_compile_definitions(crypto_service_crypto_hw
    PRIVATE
        TEE_M
        DUBHE_FOR_RUNTIME
)
endif()

if (TFM_PARTITION_CRYPTO)
target_include_directories(crypto_service_mbedcrypto
    PUBLIC
        ${TFM_BEKEN}/accelerator/
        ${TE_DIR}/dubhe_driver/inc/common
        ${TE_DIR}/dubhe_driver/inc/crypto
        ${TE_DIR}/dubhe_alt/inc
)

target_compile_definitions(crypto_service_mbedcrypto
    PRIVATE
        TEE_M
        ARM_CE_DUBHE
        DUBHE_SECURE
        ARM_CE_DUBHE_SCA
        ARM_CE_DUBHE_TRNG
        ARM_CE_DUBHE_OTP
        ARM_CE_DUBHE_HASH
        ARM_CE_DUBHE_ACA
        DUBHE_FOR_RUNTIME
)
endif()

if(TEST_NS)
target_sources(test_ns_crypto_hw
    PRIVATE
        ${TE_DIR}/dubhe_driver/src/common/dubhe_driver.c
        ${TE_DIR}/dubhe_driver/src/common/dubhe_event.c
        ${TE_DIR}/dubhe_driver/src/common/dubhe_intr_handler.c
        ${TE_DIR}/dubhe_driver/src/common/dubhe_sram_alloc.c
        ${TE_DIR}/dubhe_driver/src/crypto/aca/dubhe_aca.c
        ${TE_DIR}/dubhe_driver/src/crypto/aca/dubhe_aca_hw.c
        ${TE_DIR}/dubhe_driver/src/crypto/aead/dubhe_ccm.c
        ${TE_DIR}/dubhe_driver/src/crypto/aead/dubhe_gcm.c
        ${TE_DIR}/dubhe_driver/src/crypto/hash/dubhe_hash.c
        ${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_cbcmac.c
        ${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_cmac.c
        ${TE_DIR}/dubhe_driver/src/crypto/mac/dubhe_ghash.c
        ${TE_DIR}/dubhe_driver/src/crypto/otp/dubhe_otp.c
        ${TE_DIR}/dubhe_driver/src/crypto/sca/dubhe_sca.c
        ${TE_DIR}/dubhe_driver/src/crypto/trng/dubhe_trng.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_heap.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_log.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_mutex.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_preempt.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_semaphore.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_signal.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_string.c
        ${TE_DIR}/dubhe_driver/src/hal_pal/pal_time.c

        ${TE_DIR}/dubhe_alt/src/aes.c
        ${TE_DIR}/dubhe_alt/src/bignum_wrap.c
        ${TE_DIR}/dubhe_alt/src/ccm.c
        ${TE_DIR}/dubhe_alt/src/cmac.c
        ${TE_DIR}/dubhe_alt/src/gcm.c
        ${TE_DIR}/dubhe_alt/src/otp.c
        ${TE_DIR}/dubhe_alt/src/sha256.c

        ${TE_DIR}/dubhe_lib/bignum.c
        ${TE_DIR}/dubhe_lib/bignum_delta.c
        ${TE_DIR}/dubhe_lib/cipher.c
        ${TE_DIR}/dubhe_lib/cipher_wrap.c
        ${TE_DIR}/dubhe_lib/ctr_drbg.c
        ${TE_DIR}/dubhe_lib/dhm.c
        ${TE_DIR}/dubhe_lib/ecdh.c
        ${TE_DIR}/dubhe_lib/ecdsa.c
        ${TE_DIR}/dubhe_lib/ecp.c
        ${TE_DIR}/dubhe_lib/ecp_curves.c
        ${TE_DIR}/dubhe_lib/entropy_poll.c
        ${TE_DIR}/dubhe_lib/md.c
        ${TE_DIR}/dubhe_lib/oid.c
        ${TE_DIR}/dubhe_lib/pkparse.c
        ${TE_DIR}/dubhe_lib/rsa.c

        ${TE_DIR}/spe/src/misc.c
		${TE_DIR}/crypto_hw.c
		# ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_aes.c
		# ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_rsa.c
		# ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_hash.c
		# ${PLATFORM_DIR}/ext/target/beken/common/hal/crypto/hal_crypto_ecdsa.c
)

target_include_directories(test_ns_crypto_hw
    PRIVATE
        ${TE_DIR}/dubhe_driver/inc/common
        ${TE_DIR}/dubhe_driver/inc/crypto
        ${PLATFORM_DIR}/ext/target/beken/common/hal/inc

        ${TE_DIR}/dubhe_alt/inc
        ${PLATFORM_DIR}/ext/target/${TFM_PLATFORM}/armino/config
        ${CMAKE_SOURCE_DIR}/interface/include
        ${CMAKE_BINARY_DIR}/generated/interface/include

        # ${PLATFORM_DIR}/ext/target/${TFM_PLATFORM}
        # ${PLATFORM_DIR}/ext/target/${TFM_PLATFORM}/cmsis_core
        ${TFM_BEKEN}/common
        ${TFM_BEKEN}/common/cmsis_core
        ${TE_DIR}/spe/inc
        ${TE_DIR}/spe/inc/hal
        ${TE_DIR}/spe/inc/pal

    PUBLIC
        ${TFM_BEKEN}/accelerator
        ${PLATFORM_DIR}/include
        ${MBEDCRYPTO_PATH}/library
        ${MBEDCRYPTO_PATH}/include
        ${CMAKE_SOURCE_DIR}/../../cmsis/CMSIS_5/Device/ArmChina/STAR/Include
        ${PLATFORM_DIR}/../secure_fw/spm/include
        ${PLATFORM_DIR}/ext/common
        ${IDK_PATH}/include
        ${IDK_PATH}/include/soc/${PLATFORM}
        ${IDK_PATH}/middleware/soc/common/soc/include
        ${IDK_PATH}/middleware/soc/common/hal/include
        ${IDK_PATH}/middleware/soc/${PLATFORM}/soc
        ${IDK_PATH}/middleware/soc/${PLATFORM}/hal
        ${IDK_PATH}/components/bk_common/include
        ${IDK_PATH}/middleware/driver/sys_ctrl
        ${IDK_PATH}/middleware/driver/include/bk_private
        ${TARGET_PLATFORM_PATH}/idk
        ${TFM_BEKEN}/common/idk
        ${TFM_BEKEN}/common/idk/port
)

target_link_libraries(test_ns_crypto_hw
    PRIVATE
        crypto_service_mbedcrypto
)


target_compile_options(test_ns_crypto_hw
    PUBLIC
        ${COMPILER_CP_FLAG}
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wno-unused-parameter>
        $<$<C_COMPILER_ID:ARMClang>:-Wno-unused-parameter>
)

target_compile_definitions(test_ns_crypto_hw
    PUBLIC
        CRYPTO_HW_ACCELERATOR
        MBEDTLS_CONFIG_FILE="${TFM_MBEDCRYPTO_CONFIG_PATH}"

    PRIVATE
        TEE_M
        ARM_CE_DUBHE
        # TODO fix me, when all of these options enabled, NSPE crash!
        #DUBHE_SECURE
        #ARM_CE_DUBHE_SCA
        #ARM_CE_DUBHE_TRNG
        ARM_CE_DUBHE_OTP
        #ARM_CE_DUBHE_HASH
        #ARM_CE_DUBHE_ACA
        DUBHE_FOR_RUNTIME
	    CONFIG_NS_VT
)
endif()
