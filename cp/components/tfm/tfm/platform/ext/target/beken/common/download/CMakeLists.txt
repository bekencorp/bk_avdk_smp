if (BL2_DOWNLOAD)
	# platform/CMakeLists.txt already declares `add_library(download STATIC)`
	# under BL2_DOWNLOAD; populate that pre-declared target instead of
	# re-creating it (avoids "target already exists" CMP0002 error).
	target_sources(download
		PRIVATE
			src/core/download_entry.c
			src/util/download_wdt.c
			src/transport/download_uart.c
			src/protocol/download_protocol.c
			src/protocol/download_response.c
			src/protocol/download_common_cmd.c
			src/protocol/download_flash_cmd.c
			src/flash/download_flash_access.c
			src/util/crc32.c
			src/util/download_timer.c
			# BK7259 flash HAL adapter.
			src/flash/download_flash_adapter.c
	)

	string(REGEX REPLACE "beken/" "" SOC "${TFM_PLATFORM}")
	set(download_inc
		.
		./include
		./src
		./inc
		./inc/pal
		./hal/inc
		# bl_bk_reg.h includes "soc/bk7259/reg_base.h", which lives under cp/include.
		${IDK_PATH}/include
		# os_port.c (added to this target by common/idk/port) needs bk_arch.h.
		${IDK_PATH}/middleware/arch/cortex-m/include
		# boot_update.c: #include "flash_partition.h" (cp/include/driver).
		${IDK_PATH}/include/driver
		# download_boot.h: #include "region_defs.h" (beken/<soc>/partition).
		${TFM_BEKEN}/${SOC}/partition
		${TFM_BEKEN}/bk7234n/armino/driver/hal
		${TFM_BEKEN}/bk7234n/armino/driver
		../${TFM_SOC}/cmsis_core
		${TFM_BEKEN}/retarget
		${TFM_BEKEN}/partition
		../${TFM_SOC}/partition
		${TFM_BEKEN}/${SOC}/armino_min/include
		../armino/include
		../${TFM_SOC}/armino/config
		./../../../cmsis/
		${ARMINO_MIDDLEWARE_REPO_PATH}/boards/${SOC}/partitions
		${PARTITION_HDR_DIR}
		./../../../../../bl2/ext/mcuboot/include/
	)

	target_compile_definitions(download
		PUBLIC
			CONFIG_BL2_DOWNLOAD
			# The download uses its own width types (download_types.h: u32/s32 =
			# long, matching this toolchain's uint32_t). Define HAVE_UTYPES so the
			# SDK's cp/include/common/bk_typedef.h does NOT also typedef u8..s32
			# (it uses u32=unsigned int), which otherwise conflicts.
			HAVE_UTYPES
	)

	if(CONFIG_BL2_UPDATE_WITH_PC)
		target_compile_definitions(download
			PUBLIC
				CONFIG_BL2_UPDATE_WITH_PC
		)
	endif()

	target_compile_options(download PUBLIC ${BL2_COMPILER_CP_FLAG})
	target_include_directories(download PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
	target_include_directories(download PRIVATE ${download_inc})
	# BEFORE: project partitions_gen.h must beat PROJECT_PARTITIONS_DIR / stub
	# copies that share the same filename but different macros.
	target_include_directories(download BEFORE PRIVATE ${PARTITION_HDR_DIR})
	target_link_libraries(platform_bl2 PUBLIC download)
	link_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()


