target_include_directories(platform_s PUBLIC .)
# BEFORE: project partitions_gen.h must beat PROJECT_PARTITIONS_DIR / stub
# copies that share the same filename but different macros.
target_include_directories(platform_s BEFORE PRIVATE ${PARTITION_HDR_DIR})

# Bring the project-generated OTP map (otp_map_1/otp_map_2 from otp1.csv/otp2.csv,
# with the real offset/size/security) into the secure image so the OTP NSC
# gateways and bk_tfm_mpc.c use it instead of the committed stub _otp. The
# generated _otp.h/_otp.c are staged into PARTITION_HDR_DIR (added BEFORE the
# stub above) by cp/components/tfm/CMakeLists.txt - staging only those two files
# keeps the generated tfm_hal_ppc.h in armino/partitions/_build from shadowing
# the BK7259 stub no-op. otp_s.c embeds the map via #include "_otp.c" (resolved
# from PARTITION_HDR_DIR), matching the SDK otp_driver pattern; it is always
# built so bk_tfm_mpc.c can drive the OTP2 MPC LUT from otp_map_2[].security even
# when the OTP NSC feature is off.
target_sources(platform_s PRIVATE otp_s.c)
# boot_param.h (A/B record layout + shared ping-pong algorithm) lives in bl2/;
# the runtime confirm path reuses it. PRIVATE so bl2 headers are not re-exported
# to platform_s dependents.
target_include_directories(platform_s PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../bl2)

set(src
	tfm_hal_isolation.c
	tfm_hal_platform.c
	bk_tfm_ap_power.c
	bk_tfm_ap_shim.c
	bk_tfm_cp_dump.c
	bk_tfm_mpc.c
	bk_tfm_ppc.c
	bk_efuse.c
	bk_tfm_spm_log.c
	hal_hw_fih.c
	bk_systick_monitor.c
	hal_sw_fih.c
	bk_sca_defense.c
	boot_param_confirm.c
	# shared A/B back-end (flash ops + CRC32), one copy for BL2 and SPE
	${CMAKE_CURRENT_LIST_DIR}/../bl2/boot_param_ops.c
	# shared compressed-overwrite confirm flag, one copy for BL2 and SPE
	${CMAKE_CURRENT_LIST_DIR}/../bl2/ota_confirm.c
)

target_sources(platform_s PRIVATE ${src})

# NSC register access gateways for the NS cpu0_app (see reg_nsc.c).
if(CONFIG_REG_ACCESS_NSC)
	target_sources(platform_s PRIVATE reg_nsc.c)
	target_compile_definitions(platform_s PUBLIC CONFIG_REG_ACCESS_NSC)
endif()

# NSC OTP access gateways for the NS cpu0_app (see otp_nsc.c). The gateways call
# the always-built secure OTP driver (otp_s.c).
if(CONFIG_OTP_NSC)
	target_sources(platform_s PRIVATE otp_nsc.c)
	target_compile_definitions(platform_s PUBLIC CONFIG_OTP_NSC)
endif()

# NSC AP secure-prepare gateway (see ap_boot_nsc.c).
if(CONFIG_AP_BOOT_NSC)
	target_sources(platform_s PRIVATE ap_boot_nsc.c)
	target_compile_definitions(platform_s PUBLIC CONFIG_AP_BOOT_NSC)
endif()

if(CONFIG_SLEEP_RETENTION_NSC)
	target_sources(platform_s PRIVATE
		tfm_sleep_context.c
	)
	target_compile_definitions(platform_s PUBLIC CONFIG_SLEEP_RETENTION_NSC)
endif()

target_compile_definitions(platform_s
PUBLIC
	$<$<BOOL:${HW_FIH}>:CONFIG_HW_FIH>
	$<$<BOOL:${TFM_HW_FIH}>:CONFIG_TFM_HW_FIH>
	$<$<BOOL:${TFM_SW_FIH}>:CONFIG_TFM_SW_FIH>
	$<$<BOOL:${SCA_DEFENSE}>:CONFIG_SCA_DEFENSE>
	$<$<BOOL:${CONFIG_SUPPORT_SWD_DEBUG}>:CONFIG_SUPPORT_SWD_DEBUG>
)

if (BL2)
	target_include_directories(platform_bl2 PUBLIC .)

	target_sources(platform_bl2
		PUBLIC
			bk_tfm_ppc.c
			bk_efuse.c
			bk_tfm_spm_log.c
			bk_tfm_log.c
			hal_hw_fih.c
			bk_systick_monitor.c
			hal_sw_fih.c
			bk_sca_defense.c
	)

	if (SOC STREQUAL "bk7236")
		target_sources(platform_bl2
			PUBLIC
				${IDK_PATH}/middleware/driver/efuse/efuse_driver.c
				${IDK_PATH}/middleware/soc/common/hal/efuse_hal.c
		)
	endif()
	target_compile_definitions(platform_bl2
		PUBLIC
			$<$<BOOL:${HW_FIH}>:CONFIG_HW_FIH>
			$<$<BOOL:${BL2_HW_FIH}>:CONFIG_BL2_HW_FIH>
			$<$<BOOL:${BL2_SW_FIH}>:CONFIG_BL2_SW_FIH>
			$<$<BOOL:${SCA_DEFENSE}>:CONFIG_SCA_DEFENSE>
	)
endif()