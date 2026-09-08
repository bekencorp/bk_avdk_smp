armino_build_get_property(target ARMINO_SOC)

if(NOT "${target}" STREQUAL "bk7236" AND NOT "${target}" STREQUAL "bk7234"
   AND NOT "${target}" STREQUAL "bk7258")
    return()
endif()

set(incs)
set(srcs)

if (CONFIG_TFM)
	list(APPEND srcs
		tfm_ns_interface.c
		tfm/interface/src/tfm_psa_ns_api.c
	)
	list(APPEND incs
		tfm/interface/include
		tfm/secure_fw/include
		${CMAKE_BINARY_DIR}/armino/tfm/generated/interface/include
	)
endif()

if (CONFIG_TFM_SYSTEM_CLOCK)
	list(APPEND srcs
		tfm/interface/src/tfm_system_clock_ipc_api.c
	)
endif()

if (CONFIG_TFM_FWU)
	list(APPEND srcs
		tfm/interface/src/tfm_firmware_update_ipc_api.c
	)
endif()

if (CONFIG_TFM_CRYPTO)
	list(APPEND srcs
		tfm/interface/src/tfm_crypto_ipc_api.c
	)
endif()

if (CONFIG_TFM_PS)
       list(APPEND srcs
               tfm/interface/src/tfm_ps_ipc_api.c
       )
endif()


armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	REQUIRES partitions)

if (CONFIG_TFM)
armino_build_get_property(partition_hdr_dir PARTITIONS_HDR_DIR)
set(armino_dir ${COMPONENT_DIR}/../..)
set(tfm_dir ${COMPONENT_DIR}/tfm)
set(tfm_platform beken/${target})
set(tfm_toolchain_path ${CONFIG_TOOLCHAIN_PATH})
if(NOT tfm_toolchain_path)
	armino_build_get_property(tfm_toolchain_path TOOLCHAIN_DIR)
endif()
if(NOT IS_ABSOLUTE ${tfm_toolchain_path})
	set(tfm_toolchain_path ${armino_dir}/${tfm_toolchain_path})
endif()
set(tfm_build_dir ${CMAKE_BINARY_DIR}/armino/tfm)
set(tfm_install_dir ${CMAKE_BINARY_DIR}/tfm_install)

include(ExternalProject)
ExternalProject_Add(tfm_project
	SOURCE_DIR ${tfm_dir}
	CONFIGURE_COMMAND ""
	BUILD_IN_SOURCE 1
	CMAKE_ARGS
	BUILD_COMMAND
		cmake
		-G Ninja
		-B ${tfm_build_dir}
		-DCMAKE_PREFIX_PATH=${tfm_toolchain_path}
		-DCMAKE_INSTALL_PREFIX=${tfm_install_dir}
		-DARMINO_SDK_PATH=${armino_path}
		-DTFM_PLATFORM=${tfm_platform}
		-DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake
		-DCMAKE_BUILD_TYPE=${CONFIG_TFM_BUILD_TYPE}
		-DTEST_S=${CONFIG_TFM_TEST_S}
		-DTEST_NS=${CONFIG_TFM_TEST_NS}
		-DUSER=$ENV{USER}
		-DBL2=${CONFIG_TFM_BL2}
		-DTFM_PROFILE=profile_medium
		-DFPGA=${CONFIG_TFM_FPGA}
		-DOTP_NV_COUNTERS_RAM_EMULATION=${CONFIG_OTP_NV_COUNTERS_RAM_EMULATION}
		-DPLATFORM_DEFAULT_OTP_WRITEABLE=${CONFIG_PLATFORM_DEFAULT_OTP_WRITEABLE}
		-DCONFIG_TFM_LOG_LEVEL=${CONFIG_TFM_LOG_LEVEL}
		-DMCUBOOT_LOG_LEVEL=${CONFIG_TFM_BL2_LOG_LEVEL}
		-DPARTITION_HDR_DIR=${partition_hdr_dir}
		-DMCUBOOT_UPGRADE_STRATEGY=${CONFIG_BL2_UPGRADE_STRATEGY}
		-DTFM_AON_PMU_LL_NSC=${CONFIG_TFM_AON_PMU_LL_NSC}
		-DTFM_SYS_LL_NSC=${CONFIG_TFM_SYS_LL_NSC}
		-DCONFIG_TFM_BK7236_V5=${CONFIG_TFM_BK7236_V5}
		-DBL2_DOWNLOAD=${CONFIG_TFM_BL2_DOWNLOAD}
		-DBL2_SECURE_DEBUG=${CONFIG_TFM_SECURE_DEBUG}
		-DCONFIG_REG_ACCESS_NSC=${CONFIG_TFM_REG_ACCESS_NSC}
		-DCONFIG_MPC_NSC=${CONFIG_TFM_MPC_NSC}
		-DCONFIG_FLASH_NSC=${CONFIG_TFM_FLASH_NSC}
		-DCONFIG_DUBHE_KEY_LADDER_NSC=${CONFIG_TFM_DUBHE_KEY_LADDER_NSC}
		-DCONFIG_TFM_READ_FLASH_NSC=${CONFIG_TFM_READ_FLASH_NSC}
        -DCONFIG_OTP_NSC=${CONFIG_TFM_OTP_NSC}
		-DCONFIG_INT_TARGET_NSC=${CONFIG_TFM_INT_TARGET_NSC}
		-DCONFIG_TFM_MPU=${CONFIG_TFM_MPU}
		-DCONFIG_TFM_PANIC_DEAD_LOOP=${CONFIG_TFM_PANIC_DEAD_LOOP}
		-DCONFIG_XIP_NO_VERSION=${CONFIG_XIP_NO_VERSION}
		-DCONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE=${CONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE}
		-DCONFIG_BL2_SKIP_VALIDATE=${CONFIG_BL2_SKIP_VALIDATE}
		-DCONFIG_BL2_WDT=${CONFIG_BL2_WDT}
		-DCONFIG_PM_NSC=${CONFIG_PM_NSC}
		-DCONFIG_AES_GCM_NSC=${CONFIG_TFM_AES_GCM_NSC}
		-DCONFIG_STARTUP_PERFORMANCE=${CONFIG_STARTUP_PERFORMANCE}
		-DCONFIG_TFM_RAM_SIZE=${CONFIG_CPU0_SPE_RAM_SIZE}
		-DCONFIG_TFM_HEAP_SIZE=${CONFIG_TFM_HEAP_SIZE}
		-DCONFIG_BL2_WDT_PERIOD=${CONFIG_BL2_WDT_PERIOD}
		-DCONFIG_INITIAL_ATTESTATION=${CONFIG_INITIAL_ATTESTATION}
		-DCONFIG_BL2_UPDATE_WITH_PC=${CONFIG_BL2_UPDATE_WITH_PC}
		-DCONFIG_TFM_CRYPTO_IOVEC_BUFFER_SIZE=${CONFIG_TFM_CRYPTO_IOVEC_BUFFER_SIZE}
		-DCONFIG_TFM_MBEDTLS_RSA_C=${CONFIG_TFM_MBEDTLS_RSA_C}
		-DCONFIG_TFM_MBEDTLS_PKCS1_V15=${CONFIG_TFM_MBEDTLS_PKCS1_V15}
		-DCONFIG_BL2_UPDATE_WITH_OTA=${CONFIG_BL2_UPDATE_WITH_OTA}
		&& cd ${tfm_build_dir}
		&& ninja -- install
	BUILD_BYPRODUCTS ${tfm_build_dir}/secure_fw/libtfm_s_veneers.a
	BUILD_ALWAYS 1
	INSTALL_COMMAND ""
	)

add_dependencies(${COMPONENT_LIB} tfm_project)
add_library(lib_tfm STATIC IMPORTED GLOBAL)
add_dependencies(lib_tfm tfm_project)
if (CONFIG_TFM_SYSTEM_CLOCK OR CONFIG_TFM_FWU)
set_target_properties(lib_tfm PROPERTIES IMPORTED_LOCATION ${tfm_build_dir}/secure_fw/libtfm_s_veneers.a)
target_link_libraries(${COMPONENT_LIB} PUBLIC lib_tfm)
target_compile_definitions(${COMPONENT_LIB}
	PRIVATE
		TFM_PSA_API
)
endif()
endif()

# ------------------------------------------------------------------------------
# Secure firmware without a TF-M secure runtime: build only the MCUboot BL2 from
# the tfm/bl2 tree. No tfm_s, no S/NS isolation. BL2 verifies the merged
# primary_all (CP + AP) image with EC-P256 and boots the active DIRECT_XIP slot.
# ------------------------------------------------------------------------------
if (CONFIG_SECURITY_FIRMWARE AND NOT CONFIG_TFM)
	armino_build_get_property(partition_hdr_dir PARTITIONS_HDR_DIR)
	set(armino_dir ${COMPONENT_DIR}/../..)
	set(tfm_dir ${COMPONENT_DIR}/tfm)
	set(tfm_platform beken/${target})
	set(tfm_toolchain_path ${CONFIG_TOOLCHAIN_PATH})
	if(NOT tfm_toolchain_path)
		armino_build_get_property(tfm_toolchain_path TOOLCHAIN_DIR)
	endif()
	if(NOT IS_ABSOLUTE ${tfm_toolchain_path})
		set(tfm_toolchain_path ${armino_dir}/${tfm_toolchain_path})
	endif()
	set(tfm_build_dir ${CMAKE_BINARY_DIR}/armino/tfm)
	set(tfm_install_dir ${CMAKE_BINARY_DIR}/tfm_install)

	set(bl2_upgrade_strategy ${CONFIG_BL2_UPGRADE_STRATEGY})
	if(NOT bl2_upgrade_strategy)
		set(bl2_upgrade_strategy "DIRECT_XIP")
	endif()
	set(bl2_log_level ${CONFIG_TFM_BL2_LOG_LEVEL})
	if(NOT bl2_log_level)
		set(bl2_log_level "INFO")
	endif()
	set(bl2_tfm_log_level ${CONFIG_TFM_LOG_LEVEL})
	if(NOT bl2_tfm_log_level)
		set(bl2_tfm_log_level "OFF")
	endif()
	set(bl2_wdt_period ${CONFIG_BL2_WDT_PERIOD})
	if(NOT bl2_wdt_period)
		set(bl2_wdt_period "0x1F40")
	endif()

	# BL2 consumes the physical (PHY) partition layout as partitions_gen.h in
	# PARTITION_HDR_DIR. Generate it from the SMP-produced partition table so it
	# always matches the project's auto_partitions.csv (doc 7.1 dual-slot layout).
	# beken_utils lives in the AVDK root (code/tools), one level above the CP tree.
	set(avdk_dir ${COMPONENT_DIR}/../../..)
	set(beken_utils_main ${avdk_dir}/tools/env_tools/beken_utils/main.py)
	set(smp_partitions_csv ${CMAKE_BINARY_DIR}/../partitions/partitions.csv)

	include(ExternalProject)
	ExternalProject_Add(bl2_project
		SOURCE_DIR ${tfm_dir}
		CONFIGURE_COMMAND ""
		BUILD_IN_SOURCE 1
		CMAKE_ARGS
		BUILD_COMMAND
			mkdir -p ${partition_hdr_dir}
			&& cd ${partition_hdr_dir}
			&& python3 ${beken_utils_main} gen partition --partition_csv ${smp_partitions_csv} --ota_type XIP --out_hdr_file partitions_gen.h --out_layout_file partition_layout.h --secureboot
			&& cd ${tfm_dir}
			&& cmake
			-G Ninja
			-S ${tfm_dir}
			-B ${tfm_build_dir}
			-DCMAKE_PREFIX_PATH=${tfm_toolchain_path}
			-DCMAKE_INSTALL_PREFIX=${tfm_install_dir}
			-DARMINO_SDK_PATH=${armino_path}
			-DTFM_PLATFORM=${tfm_platform}
			-DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake
			-DCMAKE_BUILD_TYPE=minsizerel
			-DUSER=$ENV{USER}
			-DBL2=ON
			-DNS=OFF
			-DTFM_PROFILE=profile_medium
			-DCONFIG_TFM_LOG_LEVEL=${bl2_tfm_log_level}
			-DMCUBOOT_LOG_LEVEL=${bl2_log_level}
			-DPARTITION_HDR_DIR=${partition_hdr_dir}
			-DMCUBOOT_UPGRADE_STRATEGY=${bl2_upgrade_strategy}
			-DCONFIG_TFM_BK7236_V5=1
			-DBL2_DOWNLOAD=${CONFIG_TFM_BL2_DOWNLOAD}
			-DCONFIG_XIP_NO_VERSION=${CONFIG_XIP_NO_VERSION}
			-DCONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE=${CONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE}
			-DCONFIG_BL2_WDT=${CONFIG_BL2_WDT}
			-DCONFIG_BL2_WDT_PERIOD=${bl2_wdt_period}
			&& cd ${tfm_build_dir}
			&& ninja bl2 bl2_B bin/bl2.bin bin/bl2.elf bin/bl2_B.bin bin/bl2_B.elf
			&& mkdir -p ${tfm_install_dir}/bin
			&& cp -f ${tfm_build_dir}/bin/bl2.bin ${tfm_install_dir}/bin/bl2.bin
			&& cp -f ${tfm_build_dir}/bin/bl2.elf ${tfm_install_dir}/bin/bl2.elf
			&& cp -f ${tfm_build_dir}/bin/bl2_B.bin ${tfm_install_dir}/bin/bl2_B.bin
			&& cp -f ${tfm_build_dir}/bin/bl2_B.elf ${tfm_install_dir}/bin/bl2_B.elf
		BUILD_BYPRODUCTS ${tfm_install_dir}/bin/bl2.bin
		BUILD_ALWAYS 1
		INSTALL_COMMAND ""
		)

	add_dependencies(${COMPONENT_LIB} bl2_project)
endif()
