if(NOT DEFINED PLATFORM_DIR)
    set(PLATFORM_DIR ../../.. CACHE PATH "Path to plaform dir")
endif()

include(${TFM_BEKEN}/accelerator/CMakeLists.txt)
# include(${CONFIG_TFM_SOURCE_PATH}/platform/ext/target/beken/accelerator/CMakeLists.txt)

################################ BL2 ###########################################
if(BL2)

    target_link_libraries(bl2_mbedcrypto
        PUBLIC
            bl2_crypto_hw
    )

    target_compile_definitions(bl2_mbedcrypto
        PUBLIC
            CRYPTO_HW_ACCELERATOR
            CRYPTO_HW_ACCELERATOR_OTP_PROVISIONING
    )

    target_compile_options(bl2_mbedcrypto
        PRIVATE
            $<$<C_COMPILER_ID:GNU>:-Wno-unused-parameter>
            $<$<C_COMPILER_ID:ARMClang>:-Wno-unused-parameter>
    )
endif()

if(TEST_NS)
    target_compile_definitions(test_ns_crypto_hw
        PUBLIC
            CRYPTO_HW_ACCELERATOR
            CRYPTO_HW_ACCELERATOR_OTP_PROVISIONING
    )

    target_compile_options(test_ns_crypto_hw
        PRIVATE
            $<$<C_COMPILER_ID:GNU>:-Wno-unused-parameter>
            $<$<C_COMPILER_ID:ARMClang>:-Wno-unused-parameter>
    )

   if(CMAKE_INSTALL_PREFIX)
        install(TARGETS test_ns_crypto_hw
            DESTINATION ${CMAKE_INSTALL_PREFIX}/platform/ext/accelerator/
            COMPONENT platform
        )
    endif()

endif()

if (TFM_PARTITION_CRYPTO)
target_link_libraries(crypto_service_mbedcrypto
    PUBLIC
        crypto_service_crypto_hw
)

target_compile_definitions(crypto_service_mbedcrypto
    PUBLIC
        CRYPTO_HW_ACCELERATOR
        LEGACY_DRIVER_API_ENABLED

)

target_compile_options(crypto_service_mbedcrypto
    PRIVATE
        $<$<C_COMPILER_ID:GNU>:-Wno-unused-parameter>
        $<$<C_COMPILER_ID:ARMClang>:-Wno-unused-parameter>
)
endif()
