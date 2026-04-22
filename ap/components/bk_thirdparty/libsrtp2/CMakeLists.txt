
set(SRTP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/libsrtp")
set(incs)
set(srcs)

if(CONFIG_LIBSRTP)
# Apply patch to fix srtp_remove_stream signature
execute_process(
    COMMAND patch -p1 -N --dry-run -i ${CMAKE_CURRENT_SOURCE_DIR}/patches/srtp.patch
    WORKING_DIRECTORY ${SRTP_ROOT}
    RESULT_VARIABLE PATCH_RESULT
    ERROR_QUIET
)

if(PATCH_RESULT EQUAL 0)
    execute_process(
        COMMAND patch -p1 -N -i ${CMAKE_CURRENT_SOURCE_DIR}/patches/srtp.patch
        WORKING_DIRECTORY ${SRTP_ROOT}
    )
endif()

list(APPEND incs
    "${SRTP_ROOT}/include"
    "${SRTP_ROOT}/crypto/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/port"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# Core (no backend): always needed
list(APPEND srcs
    "${SRTP_ROOT}/srtp/srtp.c"
    "${SRTP_ROOT}/crypto/cipher/cipher.c"
    "${SRTP_ROOT}/crypto/cipher/cipher_test_cases.c"
    "${SRTP_ROOT}/crypto/cipher/null_cipher.c"
    "${SRTP_ROOT}/crypto/hash/auth.c"
    "${SRTP_ROOT}/crypto/hash/auth_test_cases.c"
    "${SRTP_ROOT}/crypto/hash/null_auth.c"
    "${SRTP_ROOT}/crypto/kernel/crypto_kernel.c"
    "${SRTP_ROOT}/crypto/kernel/alloc.c"
    "${SRTP_ROOT}/crypto/kernel/err.c"
    "${SRTP_ROOT}/crypto/kernel/key.c"
    "${SRTP_ROOT}/crypto/math/datatypes.c"
    "${SRTP_ROOT}/crypto/replay/rdb.c"
    "${SRTP_ROOT}/crypto/replay/rdbx.c"
)

# Backend: use MbedTLS/PSA only (no internal aes_icm/hmac/sha1/aes)
if(CONFIG_MBEDTLS_USE_PSA_CRYPTO)
    list(APPEND srcs
        "${SRTP_ROOT}/crypto/cipher/aes_icm_mbedtls.c"
        "${SRTP_ROOT}/crypto/hash/hmac_mbedtls.c"
    )
else()
    list(APPEND srcs
        "${SRTP_ROOT}/crypto/cipher/aes_icm.c"
        "${SRTP_ROOT}/crypto/cipher/aes.c"
        "${SRTP_ROOT}/crypto/hash/hmac.c"
        "${SRTP_ROOT}/crypto/hash/sha1.c"
    )
endif()

if (CONFIG_FULL_MBEDTLS)
if (CONFIG_MBEDTLS_USE_PSA_CRYPTO)
list(APPEND srcs
    "${SRTP_ROOT}/crypto/cipher/aes_gcm_mbedtls.c"
)
endif()
endif()

endif() # CONFIG_LIBSRTP

armino_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS ${incs}
    PRIV_REQUIRES psa_mbedtls lwip_intf_v2_1
)

if(CONFIG_LIBSRTP)
# psa/crypto.h is in psa_mbedtls; ensure dependents can find it when building with MBEDTLS
#get_filename_component(PSA_MBEDTLS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../psa_mbedtls ABSOLUTE)
#if(EXISTS "${PSA_MBEDTLS_DIR}/mbedtls/include/psa/crypto.h")
#    target_include_directories(${COMPONENT_LIB} PRIVATE "${PSA_MBEDTLS_DIR}/mbedtls/include")
#endif()

# Reduce paranoia while building libsrtp2
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=format
    -Wno-error=unused-function
    -Wno-error=unused-variable
    -Wno-error=unused-but-set-variable
    -Wno-error=sign-compare
    -Wno-error=type-limits
    -Wno-error=pointer-sign
    -Wno-error=implicit-function-declaration
    -Wno-error=strict-prototypes
    -Wno-error=old-style-definition
    -Wno-error=incompatible-pointer-types
    -Wno-conflicting-types
)

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    HAVE_CONFIG_H
)

endif() # CONFIG_LIBSRTP