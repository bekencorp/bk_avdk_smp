# BK7259 TF-M Armino component (CP side).
#
# Builds TF-M 2.1.0 (psa_level3 baseline) as an ExternalProject producing
# bl2.bin + tfm_s.bin into ${CMAKE_BINARY_DIR}/tfm_install/outputs, and
# compiles the NS-side PSA interface into the Armino app.
#
# Simplified secure scheme (BK7259):
#   - MCUboot (BL2) only on CP. CP BL2 verifies the single combined
#     primary_all (CP tfm_s + cp_app + AP tfm_s + ap_app).
#   - AP is launched from CP TF-M (SPE), AP runs its own TF-M first.
#
# Only built on bk7259 (CP/M52). The TF-M source tree must be present under
# components/tfm/tfm (see README; imported from psa_level3 TF-M 2.1.0 and a
# new beken/bk7259 platform). When absent, the component degrades to a no-op
# interface library so non-secure builds keep working.

armino_build_get_property(target ARMINO_SOC)

set(incs)
set(srcs)

set(tfm_src_dir ${COMPONENT_DIR}/tfm)
set(tfm_present FALSE)
if (EXISTS ${tfm_src_dir}/CMakeLists.txt)
    set(tfm_present TRUE)
endif()

if (CONFIG_TFM AND tfm_present)
    # TF-M 2.1.0 NS interface sources (names differ from the older Dev2.0.x):
    #   tfm_psa_ns_api.c        -> tfm_tz_psa_ns_api.c (TrustZone variant)
    #   tfm_crypto_ipc_api.c    -> tfm_crypto_api.c
    #   tfm_ps_ipc_api.c        -> tfm_ps_api.c
    #   tfm_firmware_update_ipc_api.c -> tfm_fwu_api.c
    # The RTOS NS interface (mutex dispatch) is provided by TF-M itself
    # (os_wrapper/tfm_ns_interface_rtos.c), so we do NOT compile our own.
    list(APPEND srcs
        # TrustZone NS variant: tfm_tz_psa_ns_api.c provides psa_call() (via the
        # secure veneer). Do NOT also compile tfm_psa_call.c, which provides a
        # second psa_call() definition (multiple-definition link error).
        tfm/interface/src/tfm_tz_psa_ns_api.c
        tfm/interface/src/os_wrapper/tfm_ns_interface_rtos.c
    )
    list(APPEND incs
        tfm/interface/include
        # tfm_crypto_defs.h -> crypto_keys/tfm_builtin_key_ids.h
        tfm/interface/include/crypto_keys
        tfm/secure_fw/include
        # config_tfm.h -> config_base.h lives in tfm/config.
        tfm/config
        # mbedtls/build_info.h -> mbedtls/mbedtls_config.h. The NS interface
        # (tfm_crypto_api.c) compiles against the same mbedtls-3.6.0 headers
        # used by the secure world.
        mbedtls-3.6.0/include
        ${CMAKE_BINARY_DIR}/armino/tfm/generated/interface/include
    )
    if (CONFIG_TFM_CRYPTO)
        list(APPEND srcs tfm/interface/src/tfm_crypto_api.c)
    endif()
    if (CONFIG_TFM_PS)
        list(APPEND srcs tfm/interface/src/tfm_ps_api.c)
    endif()
    if (CONFIG_TFM_FWU)
        list(APPEND srcs tfm/interface/src/tfm_fwu_api.c)
    endif()
endif()

armino_component_register(SRCS "${srcs}"
    INCLUDE_DIRS "${incs}"
    REQUIRES partitions)

if (CONFIG_TFM AND tfm_present)
    # The NS-side PSA crypto interface (tfm_crypto_api.c) is a *client*: the PSA
    # multipart operation structs (psa/crypto_struct.h) only expose the IPC
    # `.handle` member when MBEDTLS_PSA_CRYPTO_CLIENT is defined and the full
    # server (MBEDTLS_PSA_CRYPTO_C) is NOT. The default mbedtls-3.6.0
    # mbedtls_config.h enables MBEDTLS_PSA_CRYPTO_C, which is wrong for the NS
    # client. Point the NS interface at TF-M's profile_medium *client* mbedcrypto
    # config (MBEDTLS_PSA_CRYPTO_CLIENT set, MBEDTLS_PSA_CRYPTO_C unset).
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        MBEDTLS_CONFIG_FILE="tfm_mbedcrypto_config_profile_medium_client.h"
    )
    target_include_directories(${COMPONENT_LIB} PRIVATE
        ${COMPONENT_DIR}/tfm/lib/ext/mbedcrypto/mbedcrypto_config
    )
endif()

if (CONFIG_TFM AND tfm_present)
    armino_build_get_property(partition_hdr_dir PARTITIONS_HDR_DIR)
    armino_build_get_property(armino_path ARMINO_PATH)

    set(project_partitions_dir $ENV{PARTITIONS_DIR})
    if (NOT project_partitions_dir)
        message(FATAL_ERROR "PARTITIONS_DIR is not set")
    endif()
    set(ram_regions_header ${project_partitions_dir}/ram_regions.h)

    function(get_ram_region_value macro_name output_var)
        if (NOT EXISTS ${ram_regions_header})
            message(FATAL_ERROR "Missing generated RAM regions header: ${ram_regions_header}")
        endif()
        file(STRINGS ${ram_regions_header} macro_lines
            REGEX "^[ \t]*#define[ \t]+${macro_name}[ \t]+0x[0-9A-Fa-f]+[ \t]*$")
        list(LENGTH macro_lines macro_count)
        if (NOT macro_count EQUAL 1)
            message(FATAL_ERROR
                "Expected exactly one ${macro_name} in ${ram_regions_header}, found ${macro_count}")
        endif()
        string(REGEX REPLACE
            "^[ \t]*#define[ \t]+${macro_name}[ \t]+(0x[0-9A-Fa-f]+)[ \t]*$"
            "\\1" macro_value "${macro_lines}")
        set(${output_var} ${macro_value} PARENT_SCOPE)
    endfunction()

    get_ram_region_value(CONFIG_CP_SPE_RAM_ADDR cp_spe_ram_addr)
    get_ram_region_value(CONFIG_CP_SPE_RAM_SIZE cp_spe_ram_size)
    get_ram_region_value(CONFIG_CP_RAM_ADDR cp_ram_addr)
    get_ram_region_value(CONFIG_CP_RAM_SIZE cp_ram_size)
    math(EXPR cp_spe_ram_end "${cp_spe_ram_addr} + ${cp_spe_ram_size}")
    math(EXPR cp_ram_end "${cp_ram_addr} + ${cp_ram_size}")
    if (NOT cp_spe_ram_addr EQUAL cp_ram_addr OR cp_spe_ram_end GREATER cp_ram_end)
        message(FATAL_ERROR "CP_SPE_RAM must be a prefix of CP_RAM")
    endif()

    set(tfm_platform beken/bk7259)
    # Use the SAME toolchain as the main SDK build (COMPILER_TOOLCHAIN_PATH from
    # soc_config.mk = GCC 14.3.rel1, which supports cortex-m52). The stale
    # CONFIG_TOOLCHAIN_PATH (GCC 10.3) does NOT support cortex-m52.
    set(tfm_toolchain_path "$ENV{COMPILER_TOOLCHAIN_PATH}")
    if (NOT tfm_toolchain_path)
        set(tfm_toolchain_path ${CONFIG_TOOLCHAIN_PATH})
    endif()
    if (NOT IS_ABSOLUTE ${tfm_toolchain_path})
        set(tfm_toolchain_path ${armino_path}/${tfm_toolchain_path})
    endif()
    set(tfm_build_dir ${CMAKE_BINARY_DIR}/armino/tfm)
    set(tfm_install_dir ${CMAKE_BINARY_DIR}/tfm_install)

    # External libraries referenced by local path (no network fetch).
    # Mirrors psa_level3 run.sh: MCUBOOT/MBEDCRYPTO/QCBOR/CMSIS + IDK/ROOT.
    #   MCUboot   : imported alongside TF-M under this component
    #   Mbedcrypto: TF-M needs a COMPLETE Mbed TLS tree (with framework/).
    #     The SDK psa_mbedtls/mbedtls (3.6.6) is a trimmed copy without
    #     framework/, so TF-M's full build fails on it. Use the complete
    #     mbedtls-3.6.0 imported from psa_level3 for the Secure world.
    #     (NS world keeps using the SDK psa_mbedtls 3.6.6 separately.)
    #   QCBOR/CMSIS: copied from psa_level3 into this component
    #   IDK/ROOT  : SDK cp tree (drivers/soc/include) = ARMINO_SDK_PATH
    set(tfm_mcuboot_path ${COMPONENT_DIR}/mcuboot)
    set(tfm_mbedcrypto_path ${COMPONENT_DIR}/mbedtls-3.6.0)
    set(tfm_qcbor_path ${COMPONENT_DIR}/QCBOR-1.2)
    set(tfm_cmsis_path ${COMPONENT_DIR}/CMSIS_6)
    set(tfm_idk_path ${armino_path})
    set(tfm_root_path ${COMPONENT_DIR})
    set(tfm_beken_path ${tfm_src_dir}/platform/ext/target/beken)

    include(ExternalProject)
    ExternalProject_Add(tfm_project
        SOURCE_DIR ${tfm_src_dir}
        CONFIGURE_COMMAND ""
        BUILD_IN_SOURCE 1
        BUILD_COMMAND
            cmake -G Ninja -B ${tfm_build_dir}
                -DCMAKE_PREFIX_PATH=${tfm_toolchain_path}
                -DCMAKE_INSTALL_PREFIX=${tfm_install_dir}
                -DARMINO_SDK_PATH=${armino_path}
                -DTFM_PLATFORM=${tfm_platform}
                -DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake
                -DMCUBOOT_PATH=${tfm_mcuboot_path}
                -DMBEDCRYPTO_PATH=${tfm_mbedcrypto_path}
                -DQCBOR_PATH=${tfm_qcbor_path}
                -DCMSIS_PATH=${tfm_cmsis_path}
                -DIDK_PATH=${tfm_idk_path}
                -DROOT_PATH=${tfm_root_path}
                -DTFM_BEKEN=${tfm_beken_path}
                -DCONFIG_TFM_SOURCE_PATH=${tfm_src_dir}
                -DCMAKE_BUILD_TYPE=${CONFIG_TFM_BUILD_TYPE}
                -DTFM_PROFILE=${CONFIG_TFM_PROFILE}
                -DTFM_ISOLATION_LEVEL=${CONFIG_TFM_ISOLATION_LEVEL}
                -DBL2=${CONFIG_TFM_BL2}
                -DBL2_DOWNLOAD=${CONFIG_TFM_BL2_DOWNLOAD}
                -DTEST_S=${CONFIG_TFM_TEST_S}
                -DTEST_NS=${CONFIG_TFM_TEST_NS}
                -DCRYPTO_HW_ACCELERATOR=ON
                -DMCUBOOT_UPGRADE_STRATEGY=${CONFIG_BL2_UPGRADE_STRATEGY}
                -DCONFIG_XIP_NO_VERSION=${CONFIG_XIP_NO_VERSION}
                -DCONFIG_TFM_LOG_LEVEL=${CONFIG_TFM_LOG_LEVEL}
                -DMCUBOOT_LOG_LEVEL=${CONFIG_TFM_BL2_LOG_LEVEL}
                -DPARTITION_HDR_DIR=${partition_hdr_dir}
                # Project-level generated partitions dir (holds ram_regions.h,
                # used by the kept SDK sys bring-up via driver/pwr_clk.h).
                -DPROJECT_PARTITIONS_DIR=${project_partitions_dir}
                -DCONFIG_TFM_RAM_SIZE=${cp_spe_ram_size}
                -DCONFIG_TFM_HEAP_SIZE=${CONFIG_TFM_HEAP_SIZE}
                -DTFM_PARTITION_INITIAL_ATTESTATION=OFF
                # Beken NSC gateways: forward the Armino switch into the TF-M
                # secure build (consumed by common/secure/CMakeLists.txt).
                -DCONFIG_REG_ACCESS_NSC=${CONFIG_TFM_REG_ACCESS_NSC}
                -DCONFIG_AP_BOOT_NSC=${CONFIG_TFM_AP_BOOT_NSC}
            && cd ${tfm_build_dir} && ninja -- install
        BUILD_BYPRODUCTS ${tfm_build_dir}/secure_fw/libtfm_s_veneers.a
        BUILD_ALWAYS 1
        INSTALL_COMMAND ""
    )
    # TF-M/MCUboot includes partitions_gen.h from PARTITION_HDR_DIR. Ensure the
    # project partition header exists before configuring the external build so
    # it never falls back to the stale platform stub.
    add_dependencies(tfm_project generated_files)
    add_dependencies(${COMPONENT_LIB} tfm_project)

    # Link the CMSE import library (libtfm_s_veneers.a) into the NS image. The
    # secure build emits it via -Wl,--cmse-implib (secure_fw/CMakeLists.txt); it
    # holds the SG-stub veneers the NS world jumps through for both standard TF-M
    # PSA client calls (tfm_psa_*_veneer) and Beken platform NSC gateways
    # (psa_reg_read, ...). Without it the NS .gnu.sgstubs is empty and any veneer
    # call is undefined. Imported as a target so tfm_project builds it first.
    add_library(lib_tfm_s_veneers STATIC IMPORTED GLOBAL)
    set_target_properties(lib_tfm_s_veneers PROPERTIES
        IMPORTED_LOCATION ${tfm_build_dir}/secure_fw/libtfm_s_veneers.a)
    add_dependencies(lib_tfm_s_veneers tfm_project)
    target_link_libraries(${COMPONENT_LIB} PUBLIC lib_tfm_s_veneers)
endif()
