#-------------------------------------------------------------------------------
# Copyright (c)     2023-2028, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_policy(SET CMP0076 NEW)

set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

#========================= Platform region defs ===============================#
string(REGEX REPLACE "beken/" "" SOC "${TFM_PLATFORM}")

#========================= Platform BL2 =======================================#
if(BL2)
	set(platform_bl2_src
			PRIVATE
				# cmsis_core/system_cmsdk_bk7236.c
				# cmsis_core/mpu.c
				# cmsis_core/cache.c
				# ${TFM_BEKEN}/cmsis_drivers/Driver_Flash.c
				# ${TFM_BEKEN}/native_drivers/arm_uart_drv.c
				# ${TFM_BEKEN}/cmsis_drivers/Driver_USART.c
				${CMAKE_CURRENT_SOURCE_DIR}/boot_hal_bl2.c
				${CMAKE_CURRENT_SOURCE_DIR}/int_bl2.c
				# ${TFM_BEKEN}/bl2/hooks_bl2.c
				${CMAKE_CURRENT_SOURCE_DIR}/hooks_bl2.c
				${CMAKE_CURRENT_SOURCE_DIR}/flash_map.c
				${CMAKE_CURRENT_SOURCE_DIR}/boot_param.c
				${CMAKE_CURRENT_SOURCE_DIR}/boot_param_ops.c
				# compressed-overwrite confirm flag: one copy for BL2 and SPE
				${CMAKE_CURRENT_SOURCE_DIR}/ota_confirm.c
				# ${TFM_BEKEN}/partition/flash_partition.c
				# ${TFM_BEKEN}/nv_otp/rotpk.c
				# ${TFM_BEKEN}/nv_otp/otp_flash.c
		)

		target_compile_definitions(platform_bl2
			PUBLIC
			-DMCUBOOT_IMAGE_ACCESS_HOOKS=1
			-DCRYPTO_HW_ACCELERATOR
			-DCONFIG_ENABLE_MCUBOOT_BL2=1
			-DCONFIG_FLASH_FAST_ERASE=1
			-DCONFIG_BL2_REG_BASE=1
			__EN_ICACHE
			__EN_DCACHE
			$<$<BOOL:${BL2_VALIDATE_ENABLED_BY_EFUSE}>:CONFIG_BL2_VALIDATE_ENABLED_BY_EFUSE>
			$<$<BOOL:${CONFIG_SUPPORT_SWD_DEBUG}>:CONFIG_SUPPORT_SWD_DEBUG>
		)

	set(platform_bl2_inc
		PUBLIC
			# partition
			${ARMINO_MIDDLEWARE_REPO_PATH}/boards/${SOC}/partitions
			${PARTITION_HDR_DIR}
			${TFM_BEKEN}
			${TFM_BEKEN}/partition
			cmsis_core
			${TFM_BEKEN}/retarget
			${TFM_BEKEN}/bl2
			# boot_param.h (A/B record layout) so bl2_main.c can include it
			${CMAKE_CURRENT_SOURCE_DIR}

		PRIVATE
			.
			${PLATFORM_DIR}/..
			${TFM_BEKEN}/native_drivers
			${MCUBOOT_PATH}/boot/bootutil/include
			${MCUBOOT_PATH}/boot/bootutil/config

			${CMAKE_BINARY_DIR}/bl2/ext/mcuboot
			${PLATFORM_DIR}/../bl2/ext/mcuboot/include
			${PLATFORM_DIR}/ext/target/beken/common/hal/accelerator/dubhe_alt/inc
	)

	if(CRYPTO_HW_ACCELERATOR)
		list(APPEND platform_bl2_inc
			${TFM_BEKEN}/common/hal/inc/
			${TFM_BEKEN}/common/hal/accelerator/spe/inc/hal
			${TFM_BEKEN}/common/hal/accelerator/spe/inc/pal
			${TFM_BEKEN}/common/hal/accelerator/dubhe_driver/inc/crypto
			${TFM_BEKEN}/armino/driver/sys_ctrl/
			./armino/driver/include/
			${TFM_BEKEN}/armino/include/driver/
		)
	endif()

		list(APPEND platform_bl2_inc
			${TFM_BEKEN}/secure_dbg/device_agent/inc/
			${TFM_BEKEN}/secure_dbg/device_agent/src/
		)


		# list(APPEND platform_bl2_src
		# 	${TFM_BEKEN}/secure_dbg/device_agent/src/hal_jtag.c)

	if(BL2_SECURE_DEBUG)
		list(APPEND platform_bl2_src
			${TFM_BEKEN}/secure_dbg/device_agent/src/hal_comm_adapt.c
			${TFM_BEKEN}/secure_dbg/device_agent/src/hal_comm_channel.c
			${TFM_BEKEN}/secure_dbg/device_agent/src/hal_secure_debug.c
			${TFM_BEKEN}/secure_dbg/device_agent/src/securedebug.c
			${TFM_BEKEN}/secure_dbg/device_agent/src/securedebug_protocol.c
			${TFM_BEKEN}/secure_dbg/device_agent/src/secure_debug_app.c
		)

		target_compile_definitions(platform_bl2
			PUBLIC
				CONFIG_BL2_SECURE_DEBUG
		)
	endif()
	if(CONFIG_BL2_WDT)
		target_compile_definitions(platform_bl2
			PUBLIC
				CONFIG_BL2_WDT
		)
	endif()

	if(CONFIG_BL2_WDT_PERIOD)
		target_compile_definitions(platform_bl2
			PUBLIC
				CONFIG_BL2_WDT_PERIOD
		)
	endif()

	# Compressed-overwrite OTA (secureboot_overwrite). Driven by
	# CONFIG_BL2_UPGRADE_STRATEGY="OVERWRITE_ONLY" (cp/config/bk7259/defconfig ->
	# -DMCUBOOT_UPGRADE_STRATEGY). BL2 links the BK7259 identity-mapping
	# decompressor (common/bl2/decompress_bl2.c, guarded internally by
	# CONFIG_OTA_OVERWRITE) plus the LZMA decode backend from the compress
	# component. Plaintext v1: no flash_driver AES path is pulled in; the
	# decompressor writes primary_all with bk_flash_write_bytes (armino_min).
	if(MCUBOOT_UPGRADE_STRATEGY STREQUAL "OVERWRITE_ONLY")
		list(APPEND platform_bl2_src
			${CMAKE_CURRENT_SOURCE_DIR}/decompress_bl2.c
			${ARMINO_SDK_PATH}/components/compress/decompress.c
			${ARMINO_SDK_PATH}/components/compress/lzma.c
			${ARMINO_SDK_PATH}/components/compress/lzma1900/LzmaDec.c
		)

		list(APPEND platform_bl2_inc
			${ARMINO_SDK_PATH}/components/compress/include
			${ARMINO_SDK_PATH}/components/compress/lzma1900
		)

		# LZMA1900 backend selection for the compress component sources.
		target_compile_definitions(platform_bl2
			PUBLIC
				CONFIG_LZMA1900=1
		)
	endif()

	if(CONFIG_XIP_NO_VERSION)
		target_compile_definitions(platform_bl2
			PUBLIC
				CONFIG_XIP_NO_VERSION
		)
	endif()

    if(CONFIG_STARTUP_PERFORMANCE)
    target_compile_definitions(platform_s PUBLIC CONFIG_STARTUP_PERFORMANCE)
    target_compile_definitions(platform_ns PUBLIC CONFIG_STARTUP_PERFORMANCE)
    target_compile_definitions(platform_bl2 PUBLIC CONFIG_STARTUP_PERFORMANCE)
    target_compile_definitions(bootutil PUBLIC CONFIG_STARTUP_PERFORMANCE)
    endif()

	if(CONFIG_ENCRYPT_PRIVATE_KEY)
	target_compile_definitions(platform_s PUBLIC CONFIG_ENCRYPT_PRIVATE_KEY)
	target_compile_definitions(platform_ns PUBLIC CONFIG_ENCRYPT_PRIVATE_KEY)
	target_compile_definitions(platform_bl2 PUBLIC CONFIG_ENCRYPT_PRIVATE_KEY)
	target_compile_definitions(bootutil PUBLIC CONFIG_ENCRYPT_PRIVATE_KEY)
	endif()

	
	target_include_directories(platform_bl2 PUBLIC ${platform_bl2_inc})
	# BEFORE: project partitions_gen.h must beat PROJECT_PARTITIONS_DIR / stub
	# copies that share the same filename but different macros.
	target_include_directories(platform_bl2 BEFORE PUBLIC ${PARTITION_HDR_DIR})

	# Compressed-overwrite: bootutil (loader.c) must also see the generated
	# partition headers so CONFIG_OTA_* / OVERWRITE_CONFIRM and the
	# PARTITION_OTA_CONTROL enum + partition_get_phy_offset() prototype resolve,
	# plus common/bl2 for ota_confirm.h (the confirm-flag API loader.c calls) and
	# bl2_flash_map.h.
	if(MCUBOOT_UPGRADE_STRATEGY STREQUAL "OVERWRITE_ONLY")
		target_include_directories(bootutil BEFORE PUBLIC
			${PARTITION_HDR_DIR}
			${TFM_BEKEN}/common/partition
			${TFM_BEKEN}/common/bl2
		)
	endif()

	target_sources(platform_bl2 PRIVATE ${platform_bl2_src})
endif()

